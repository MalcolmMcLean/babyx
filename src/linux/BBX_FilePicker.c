#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "BabyX.h"

typedef struct
{
  int folder;
  char *name;
} DIRENTRY;

DIRENTRY *readdirectoryfilt(char *dir, int *N, int hidden, char *filt);
DIRENTRY *readdirectory(char *dir, int *N, int hidden);
void killentries(DIRENTRY *entries, int N);
static int compentries(const void *e1, const void *e2);

int wildcard_match(const char *data, const char *mask);
int wildcard_match_icase(const char *data, const char *mask);

typedef struct
{ 
  BABYX *bbx;
  BBX_Panel *pan; 
  BBX_ListBox *list;
  BBX_CheckBox *hidden_chk;
  BBX_Label *filter_lab;
  BBX_LineEdit *filter_edt;
  BBX_Button *ok_but;
  BBX_Button *cancel_but;
  char directory[FILENAME_MAX];
  int hidden;
  int positive;
  DIRENTRY *dir;
  int Nfiles;
  char *filt;
} OPENPICKER;


typedef struct
{ 
  BABYX *bbx;
  BBX_Panel *pan; 
  BBX_ListBox *list;
  BBX_LineEdit *answer_edt;
  BBX_CheckBox *hidden_chk;
  BBX_Label *filter_lab;
  BBX_LineEdit *filter_edt;
  BBX_Button *ok_but;
  BBX_Button *cancel_but;
  char directory[FILENAME_MAX];
  int hidden;
  int positive;
  DIRENTRY *dir;
  int Nfiles;
  char *filt;
} SAVEPICKER;


static void openlayout(void *obj, int width, int height);
static void openpresscancel(void *obj);
static void openpressok(void *obj);
static void opencheckhidden(void *obj, int state);
static void openeditfilter(void *obj, char *text);

static void savelayout(void *obj, int width, int height);
static void saveselectionchanged(void *obj, int selected);
static void savepresscancel(void *obj);
static void savepressok(void *obj);
static void savecheckhidden(void *obj, int state);
static void saveeditanswer(void *obj, char *text);
static void saveeditfilter(void *obj, char *text);

static void filllistbox(BBX_ListBox *list, DIRENTRY *entries, int N);

static void makepath(char *out, int N, char *dir, char *sub);
static void goupafolder(char *dir);
static void godownafolder(char *dir, char *sub);

static int fileexists(char *path);
static int is_hidden(char *fname);
static void trim(char *str);

char *bbx_getopenfile(BABYX *bbx, char *filt)
{
  OPENPICKER op;
  char *answer = 0;

  op.bbx = bbx;
  if(filt && strcmp(filt, ""))
    op.filt = bbx_strdup(filt);
  else
    op.filt = bbx_strdup("*");
 
  op.pan = bbx_dialogpanel(bbx, "Open file", 400, 500, openlayout, &op);
  bbx_dialogpanel_setclosefunc(op.pan, openpresscancel, &op);
  op.ok_but = bbx_button(bbx, op.pan, "OK", openpressok, &op);
  op.cancel_but = bbx_button(bbx, op.pan, "Cancel", openpresscancel, &op); 
  op.list = bbx_listbox(bbx, op.pan, 0, 0);
  
  op.hidden_chk = bbx_checkbox(bbx, op.pan, "Hidden", opencheckhidden, &op);
  getcwd(op.directory, FILENAME_MAX);
  op.filter_lab = bbx_label(bbx, op.pan, "Filter");
  op.filter_edt = bbx_lineedit(bbx, op.pan, op.filt, openeditfilter, &op); 
  op.dir = readdirectoryfilt(op.directory, &op.Nfiles, 0, op.filt);
  filllistbox(op.list, op.dir, op.Nfiles); 
  bbx_panel_setbackground(op.pan, bbx_color("gray"));
  
  op.hidden = 0;
  bbx_dialogpanel_makemodal(op.pan);

  if(op.positive)
  {
    char buff[FILENAME_MAX];
    int sel;
   
    sel = bbx_listbox_getselected(op.list);
    if(sel >= 0 && sel < op.Nfiles)
    {
      makepath(buff, FILENAME_MAX, op.directory, op.dir[sel].name);
      answer = bbx_strdup(buff);
    } 
  }

  free(op.filt);
  killentries(op.dir, op.Nfiles);

  bbx_checkbox_kill(op.hidden_chk);
  bbx_listbox_kill(op.list);
  bbx_button_kill(op.ok_but);
  bbx_button_kill(op.cancel_but);
  bbx_label_kill(op.filter_lab);
  bbx_lineedit_kill(op.filter_edt);
  bbx_dialogpanel_kill(op.pan);

  return answer;
}

char *bbx_getsavefile(BABYX *bbx, char *filt)
{
  SAVEPICKER sp;
  char *answer = 0;

  sp.bbx = bbx;
  if(filt && strcmp(filt, ""))
    sp.filt = bbx_strdup(filt);
  else
    sp.filt = bbx_strdup("*");
 
  sp.pan = bbx_dialogpanel(bbx, "Save file", 400, 500, savelayout, &sp);
  bbx_dialogpanel_setclosefunc(sp.pan, savepresscancel, &sp);
  sp.ok_but = bbx_button(bbx, sp.pan, "OK", savepressok, &sp);
  sp.cancel_but = bbx_button(bbx, sp.pan, "Cancel", savepresscancel, &sp); 
  sp.list = bbx_listbox(bbx, sp.pan, saveselectionchanged, &sp);
  sp.answer_edt = bbx_lineedit(bbx, sp.pan, "", saveeditanswer, &sp); 
  sp.hidden_chk = bbx_checkbox(bbx, sp.pan, "Hidden", savecheckhidden, &sp);
  getcwd(sp.directory, FILENAME_MAX);
  sp.filter_lab = bbx_label(bbx, sp.pan, "Filter");
  sp.filter_edt = bbx_lineedit(bbx, sp.pan, sp.filt, saveeditfilter, &sp); 
  sp.dir = readdirectoryfilt(sp.directory, &sp.Nfiles, 0, sp.filt);
  filllistbox(sp.list, sp.dir, sp.Nfiles); 
  bbx_panel_setbackground(sp.pan, bbx_color("gray"));
  
  sp.hidden = 0;
  bbx_dialogpanel_makemodal(sp.pan);

  if(sp.positive)
  {
    char buff[FILENAME_MAX];
    char *filename;
    int doit;

    filename = bbx_lineedit_gettext(sp.answer_edt);
    trim(filename);
    makepath(buff, FILENAME_MAX, sp.directory, filename);
    if(fileexists(buff))
    {
      doit = bbx_messagebox(bbx, BBX_MB_YES_NO_CANCEL, "Warning", "This will overwrite %s", filename);
      if(doit == BBX_MB_YES)
        answer = bbx_strdup(buff);
    }
    else
      answer = bbx_strdup(buff);
    free(filename); 
  }
  free(sp.filt);
  killentries(sp.dir, sp.Nfiles);

  bbx_checkbox_kill(sp.hidden_chk);
  bbx_listbox_kill(sp.list);
  bbx_lineedit_kill(sp.answer_edt);
  bbx_button_kill(sp.ok_but);
  bbx_button_kill(sp.cancel_but);
  bbx_label_kill(sp.filter_lab);
  bbx_lineedit_kill(sp.filter_edt);
  bbx_dialogpanel_kill(sp.pan);

  return answer;
}

static void openlayout(void *obj, int width, int height)
{
  OPENPICKER *op = obj;

  bbx_setpos(op->bbx, op->list, 10, 100, width-20, height-200);
  bbx_setpos(op->bbx, op->hidden_chk, 10, height-60, 100, 25);
  bbx_setpos(op->bbx, op->filter_lab, 110, height-60, 70, 25);
  bbx_setpos(op->bbx, op->filter_edt, 180, height-60, 150, 25);
  bbx_setpos(op->bbx, op->ok_but, width - 200, height - 30, 50, 25);
  bbx_setpos(op->bbx, op->cancel_but, width - 100, height-30, 75, 25);
}

static void openpresscancel(void *obj)
{
  OPENPICKER *op = obj;
  op->positive = 0;
  bbx_dialogpanel_dropmodal(op->pan);
}

static void openpressok(void *obj)
{
  OPENPICKER *op = obj;
  int sel;

  sel = bbx_listbox_getselected(op->list);
  if(sel >= 0 && sel < op->Nfiles)
  {
    if(op->dir[sel].folder)
    {
      if(!strcmp(op->dir[sel].name, "."))
	return;
      else if(!strcmp(op->dir[sel].name, ".."))
      {
        goupafolder(op->directory);
      }
      else
      {
	godownafolder(op->directory, op->dir[sel].name);
      }
      killentries(op->dir, op->Nfiles);
      op->dir = readdirectoryfilt(op->directory, &op->Nfiles, op->hidden, op->filt);
      filllistbox(op->list, op->dir, op->Nfiles); 
    }
    else
    {
      op->positive = 1;
      bbx_dialogpanel_dropmodal(op->pan);
    }
  }
}

static void opencheckhidden(void *obj, int state)
{
  OPENPICKER *op = obj;

  op->hidden = state;
  killentries(op->dir, op->Nfiles);
  op->dir = readdirectoryfilt(op->directory, &op->Nfiles, op->hidden, op->filt);
  filllistbox(op->list, op->dir, op->Nfiles); 
}

static void openeditfilter(void *obj, char *text)
{
  OPENPICKER *op = obj;

  free(op->filt);
  op->filt = bbx_strdup(text);
  trim(op->filt);
  if(!strlen(op->filt))
  {
    free(op->filt);
    op->filt = bbx_strdup("*");
    bbx_lineedit_settext(op->filter_edt, op->filt);
  }
  killentries(op->dir, op->Nfiles);
  op->dir = readdirectoryfilt(op->directory, &op->Nfiles, op->hidden, op->filt);
  filllistbox(op->list, op->dir, op->Nfiles);
}


static void savelayout(void *obj, int width, int height)
{
  SAVEPICKER *sp = obj;

  bbx_setpos(sp->bbx, sp->list, 10, 100, width-20, height-200);
  bbx_setpos(sp->bbx, sp->answer_edt, 20, height-90, width-40, 25);
  bbx_setpos(sp->bbx, sp->hidden_chk, 10, height-60, 100, 25);
  bbx_setpos(sp->bbx, sp->filter_lab, 110, height-60, 70, 25);
  bbx_setpos(sp->bbx, sp->filter_edt, 180, height-60, 150, 25);
  bbx_setpos(sp->bbx, sp->ok_but, width - 200, height - 30, 50, 25);
  bbx_setpos(sp->bbx, sp->cancel_but, width - 100, height-30, 75, 25);
}

static void saveselectionchanged(void *obj, int selected)
{
  SAVEPICKER *sp = obj;
  
  if(selected < 0 || selected > sp->Nfiles)
  {
    bbx_lineedit_settext(sp->answer_edt, "");
    return;
  }   
  if(sp->dir[selected].folder == 0)
  {
    bbx_lineedit_settext(sp->answer_edt, sp->dir[selected].name);
  }
}

static void savepresscancel(void *obj)
{
  SAVEPICKER *sp = obj;
  sp->positive = 0;
  bbx_dialogpanel_dropmodal(sp->pan);
}

static void savepressok(void *obj)
{
  SAVEPICKER *sp = obj;
  char *filename;
  int sel;

  sel = bbx_listbox_getselected(sp->list);
  if(sel >= 0 && sel < sp->Nfiles)
  {
    if(sp->dir[sel].folder)
    {
      if(!strcmp(sp->dir[sel].name, "."))
	return;
      else if(!strcmp(sp->dir[sel].name, ".."))
      {
        goupafolder(sp->directory);
      }
      else
      {
	godownafolder(sp->directory, sp->dir[sel].name);
      }
      killentries(sp->dir, sp->Nfiles);
      sp->dir = readdirectoryfilt(sp->directory, &sp->Nfiles, sp->hidden, sp->filt);
      filllistbox(sp->list, sp->dir, sp->Nfiles); 
    }
    else
    {
      filename = bbx_lineedit_gettext(sp->answer_edt);
      trim(filename);
      if(strlen(filename) > 0)
      {
        sp->positive = 1;
        bbx_dialogpanel_dropmodal(sp->pan);
      }
      free(filename);
    }
  }
  else
  {
      filename = bbx_lineedit_gettext(sp->answer_edt);
      trim(filename);
      if(strlen(filename) > 0)
      {
        sp->positive = 1;
        bbx_dialogpanel_dropmodal(sp->pan);
      }
      free(filename);
  }
}

static void saveeditanswer(void *obj, char *text)
{
  SAVEPICKER *sp  = obj;

  bbx_listbox_setselected(sp->list, -1);
}

static void savecheckhidden(void *obj, int state)
{
  SAVEPICKER *sp = obj;

  sp->hidden = state;
  killentries(sp->dir, sp->Nfiles);
  sp->dir = readdirectoryfilt(sp->directory, &sp->Nfiles, sp->hidden, sp->filt);
  filllistbox(sp->list, sp->dir, sp->Nfiles); 
}

static void saveeditfilter(void *obj, char *text)
{
  SAVEPICKER *sp = obj;

  free(sp->filt);
  sp->filt = bbx_strdup(text);
  trim(sp->filt);
  if(!strlen(sp->filt))
  {
    free(sp->filt);
    sp->filt = bbx_strdup("*");
    bbx_lineedit_settext(sp->filter_edt, sp->filt);
  }
  killentries(sp->dir, sp->Nfiles);
  sp->dir = readdirectoryfilt(sp->directory, &sp->Nfiles, sp->hidden, sp->filt);
  filllistbox(sp->list, sp->dir, sp->Nfiles);
}



static void filllistbox(BBX_ListBox *list, DIRENTRY *entries, int N)
{
  int i;
  char buff[FILENAME_MAX];

  bbx_listbox_clear(list);
  for(i=0;i<N;i++)
  {
    if(entries[i].folder)
    {
      sprintf(buff, "<%s>", entries[i].name);
      bbx_listbox_addstring(list, buff);
    }
    else
      bbx_listbox_addstring(list, entries[i].name);
  }

}

DIRENTRY *readdirectoryfilt(char *dir, int *N, int hidden, char *filt)
{
  DIRENTRY *answer;
  DIRENTRY *fulldir;
  int j = 0;
  int i;
  
  fulldir = readdirectory(dir, N, hidden);
  answer = bbx_malloc(*N * sizeof(DIRENTRY));
  for(i=0;i<*N;i++)
  {
    if(fulldir[i].folder == 1 || wildcard_match_icase(fulldir[i].name, filt))
    {
      answer[j].folder = fulldir[i].folder;
      answer[j].name = bbx_strdup(fulldir[i].name);
      j++;
    }
  }
  killentries(fulldir, *N);
  *N = j;
  answer = bbx_realloc(answer, j * sizeof(DIRENTRY));
  return answer;
}

DIRENTRY *readdirectory(char *dir, int *N, int hidden)
{
  struct dirent *de=NULL;
  struct stat statbuf;
  DIR *d = NULL;
  DIRENTRY *answer = bbx_malloc(100 * sizeof(DIRENTRY));
  DIRENTRY *temp;
  int capacity =  100;
  int Nread  = 0;
  char path[FILENAME_MAX];

  d=opendir(dir);
  if(d == NULL)
  {
    *N = 0;
    return 0;
  }

  // Loop while not NULL
  while( (de = readdir(d)) )
  {
    makepath(path, FILENAME_MAX, dir, de->d_name);
    if (stat(path, &statbuf) == -1)
       continue;
    if(hidden == 0 && is_hidden(de->d_name))
      continue;
    if(S_ISDIR(statbuf.st_mode))
      answer[Nread].folder = 1;
    else
      answer[Nread].folder = 0;
    answer[Nread].name = bbx_strdup(de->d_name);
    
    Nread++;

    if(Nread == capacity)
    {
      temp = bbx_realloc(answer, (capacity * 2) * sizeof(DIRENTRY));
      if(!temp)
      {
        *N = Nread;
        return answer;
      }
      answer = temp;
      capacity *= 2;
     }
  }

  closedir(d);
  qsort(answer, Nread, sizeof(DIRENTRY), compentries);
  *N = Nread;
  return answer;
}

static int compentries(const void *e1, const void *e2)
{
  const DIRENTRY *d1 = e1;
  const DIRENTRY *d2 = e2;

  if(d1->folder != d2->folder)
    return d2->folder - d1->folder;
  else
    return strcmp(d1->name, d2->name);
}

void killentries(DIRENTRY *entries, int N)
{
  int i;
  if(entries)
  {
    for(i=0;i<N;i++)
      free(entries[i].name);
    free(entries);
  }
}


/*
static int validdirectory(char *dir)
{
   struct stat statbuf;

   if (stat(dir, &statbuf) == -1)
       return 0;
    if(!S_ISDIR(statbuf.st_mode))
      return 0;
 
  return 1;
}
*/


static int is_hidden(char *fname)
{
  int len;
  if(fname && fname[0] == '.')
  {
    if(!strcmp(fname, ".") || !strcmp(fname, ".."))
      return 0;
    return 1;
  }
  if(fname)
  {
    len = strlen(fname);
    if(len > 0 && fname[len-1] == '~')
      return 1;
  }
  return 0;
}


static void makepath(char *out, int N, char *dir, char *sub)
{
  strcpy(out, dir);
  strcat(out, "/");
  strcat(out, sub);
}


static void goupafolder(char *dir)
{
  char *sep;

  sep = strrchr(dir, '/');
  if(sep && sep != dir)
    *sep = 0;
  if(sep == dir)
    sep[1] = 0;
}

static void godownafolder(char *dir, char *sub)
{
  strcat(dir, "/");
  strcat(dir, sub);
} 

static int fileexists(char *path)
{
  struct stat statbuf;

  return stat(path, &statbuf) == -1 ? 0 : 1; 
}

static void trim(char *str)
{
  size_t len;
  size_t i;

  len = strlen(str);
  while(len && isspace( (unsigned char) str[len-1]))
    str[len-- -1] = 0;
  for(i=0;i<len;i++)
    if(!isspace( (unsigned char) str[i]) )
      break;
  if(i > 0)
    memmove(str, str + i, len - i + 1);
   
}
/*
static int is_hidden(char *fname)
{
  int len;
  if(fname && fname[0] == '.')
  {
    if(!strcmp(fname, ".") || !strcmp(fname, ".."))
      return 0;
    return 1;
  }
  if(fname)
  {
    len = strlen(fname);
    if(len > 0 && fname[len-1] == '~')
      return 1;
  }
  return 0;
}
*/



/*
 * This code would not have been possible without the prior work and
 * suggestions of various sourced.  Special thanks to Robey for
 * all his time/help tracking down bugs and his ever-helpful advice.
 *
 * 04/09:  Fixed the "*\*" against "*a" bug (caused an endless loop)
 *
 *   Chris Fuller  (aka Fred1@IRC & Fwitz@IRC)
 *     crf@cfox.bchs.uh.edu
 *
 * I hereby release this code into the public domain
 *
 */

#include <ctype.h>

#define WILDS '*'  /* matches 0 or more characters (including spaces) */
#define WILDQ '?'  /* matches ecactly one character */

#define NOMATCH 0
#define MATCH (match+sofar)

static int wildcard_match_int(const char *data, const char *mask, int icase)
{
  const char *ma = mask, *na = data, *lsm = 0, *lsn = 0;
  int match = 1;
  int sofar = 0;

  /* null strings should never match */
  if ((ma == 0) || (na == 0) || (!*ma) || (!*na))
    return NOMATCH;
  /* find the end of each string */
  while (*(++mask));
  mask--;
  while (*(++data));
  data--;

  while (data >= na) {
    /* If the mask runs out of chars before the string, fall back on
     * a wildcard or fail. */
    if (mask < ma) {
      if (lsm) {
        data = --lsn;
        mask = lsm;
        if (data < na)
          lsm = 0;
        sofar = 0;
      }
      else
        return NOMATCH;
    }

    switch (*mask) {
    case WILDS:                /* Matches anything */
      do
        mask--;                    /* Zap redundant wilds */
      while ((mask >= ma) && (*mask == WILDS));
      lsm = mask;
      lsn = data;
      match += sofar;
      sofar = 0;                /* Update fallback pos */
      if (mask < ma)
        return MATCH;
      continue;                 /* Next char, please */
    case WILDQ:
      mask--;
      data--;
      continue;                 /* '?' always matches */
    }
    if (icase ? (toupper(*mask) == toupper(*data)) :
	(*mask == *data)) {     /* If matching char */
      mask--;
      data--;
      sofar++;                  /* Tally the match */
      continue;                 /* Next char, please */
    }
    if (lsm) {                  /* To to fallback on '*' */
      data = --lsn;
      mask = lsm;
      if (data < na)
        lsm = 0;                /* Rewind to saved pos */
      sofar = 0;
      continue;                 /* Next char, please */
    }
    return NOMATCH;             /* No fallback=No match */
  }
  while ((mask >= ma) && (*mask == WILDS))
    mask--;                        /* Zap leftover %s & *s */
  return (mask >= ma) ? NOMATCH : MATCH;   /* Start of both = match */
}

int wildcard_match(const char *data, const char *mask)
{
	return wildcard_match_int(data, mask, 0) != 0;
}

int wildcard_match_icase(const char *data, const char *mask)
{
	return wildcard_match_int(data, mask, 1) != 0;
}
