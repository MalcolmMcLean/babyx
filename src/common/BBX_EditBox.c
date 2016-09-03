#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "BabyX.h"

typedef struct
{
  BABYX *bbx;
  BBX_Panel *pan;
  BBX_Canvas *can;
  BBX_Scrollbar *sb;
  void (*fptr)(void *ptr);
  void *ptr;
  int pos;
  struct bitmap_font *font;
  char **lines;
  int Nlines;
  int topline;
  int Nshown;
  int state;
  int disabled;
  int curline;
  int cursorpos;
  void *ticker;
  int sel1line;
  int sel1pos;
  int sel2line;
  int sel2pos;
} EDITBOX;


static void layout(void *obj, int width, int height);
static void mousefunc(void *obj, int action, int x, int y, int buttons);
static void keyfunc(void *obj, int ch);
static void tickcaret(void *obj);
static void settext(EDITBOX *box, char *text);
static void drawme(EDITBOX *box);
static void drawselected(unsigned char *rgba, int width, int height, int x, int y, char *str, int N, struct bitmap_font *font);
static void scrollme(void *obj, int pos);
static int addchar(EDITBOX *box, int ch);
static char *getselectedtext(EDITBOX *box);
static void deleteselectedtext(EDITBOX *box);
static int reformattext(EDITBOX *box);
static char **texttolines(struct bitmap_font *fs, char *text, int width);
static char **addline(char **buff, int N, char *str, int len, int *capacity);
static int prevskip(char *utf8);

BBX_EditBox *bbx_editbox(BABYX *bbx, BBX_Panel *parent, void (*fptr)(void *ptr, char *text), void *ptr)
{
  EDITBOX *obj;

  obj = bbx_malloc(sizeof(EDITBOX));
  obj->bbx = bbx;
  obj->pan = bbx_panel(bbx, parent, "editbox", layout, obj); 
  obj->can = bbx_canvas(bbx, obj->pan, 100, 100, 0);
  obj->sb = bbx_scrollbar(bbx, obj->pan, BBX_SCROLLBAR_VERTICAL, scrollme, obj);
  
  obj->font = bbx->user_font2;
  obj->state = 0;
  obj->Nlines = 0;
  obj->lines = 0;
  obj->curline = 0;
  obj->cursorpos = 0;
  obj->disabled = 0;
  obj->topline = 0;
  obj->Nshown = 1;
  obj->ticker = 0;
  obj->sel1line = -1;
  obj->sel1pos = -1;
  obj->sel2line = -1;
  obj->sel2pos = -1;
 
  settext(obj, "");
  bbx_panel_setmousefunc(obj->pan, mousefunc, obj);
  bbx_panel_setkeyfunc(obj->pan, keyfunc, obj); 
  
  return obj->pan;

}

void bbx_editbox_kill(BBX_EditBox *obj)
{
  int i;
  EDITBOX *box;

  if(obj)
  {
    box = bbx_panel_getptr(obj);
    if(box)
    {
      bbx_canvas_kill(box->can);
      bbx_scrollbar_kill(box->sb);
      for(i=0;i<box->Nlines;i++)
        free(box->lines[i]);
      free(box->lines);
      free(box);
    }
    bbx_panel_kill(obj);
  }
}

void bbx_editbox_settext(BBX_EditBox *obj, char *text)
{
  EDITBOX *box = bbx_panel_getptr(obj);
  settext(box, text);
  box->topline = 0; 
  bbx_scrollbar_set(box->sb, box->Nlines, box->Nshown, box->topline);
  box->cursorpos = 0;
  box->curline = -1;

  drawme(box);
}

void bbx_editbox_disable(BBX_EditBox *obj)
{
  EDITBOX *box = bbx_panel_getptr(obj);
  box->state = 0;
  if(box->ticker)
    bbx_removeticker(box->bbx, box->ticker);
  box->ticker = 0;
  box->disabled = 1;
  drawme(box);
}

void bbx_editbox_enable(BBX_EditBox *obj)
{
  EDITBOX *box = bbx_panel_getptr(obj);
  box->disabled = 0;
  drawme(box);
}



char *bbx_editbox_gettext(BBX_EditBox *obj)
{
  int len = 0;
  char *answer;
  int i;
  EDITBOX *box = bbx_panel_getptr(obj);

  for(i=0;i<box->Nlines;i++)
    len += strlen(box->lines[i]);
  answer = bbx_malloc(len + 1);
  if(!answer)
    return 0;
  len = 0;
  for(i=0;i<box->Nlines;i++)
  {
    strcpy(answer + len, box->lines[i]);
    len += strlen(box->lines[i]);
  }
  answer[len] = 0;
  return answer;
}

void bbx_editbox_setfont(BBX_EditBox *obj, struct bitmap_font *font)
{
  EDITBOX *box = bbx_panel_getptr(obj);
  
  box->font = font;
  reformattext(box);
  drawme(box);
}

static void layout(void *obj, int width, int height)
{
  EDITBOX *box = obj;

  bbx_canvas_kill(box->can);
  box->can = bbx_canvas(box->bbx, box->pan, width-20, height, 0);
  bbx_setpos(box->bbx, box->can, 0, 0, width-20, height);
  bbx_setpos(box->bbx, box->sb, width-20, 0, 20, height);
  reformattext(box);
  drawme(box);

}

static void mousefunc(void *obj, int action, int x, int y, int buttons)
{
    int font_height;
    int i;
    int msg_x;
    char *line;
    int curline;
    int curpos;
    EDITBOX *box = obj;

    if(action == BBX_MOUSE_CLICK)
    {
     
      font_height = box->font->ascent + box->font->descent;
      box->curline = (y-5)/font_height + box->topline;
     if(box->curline >= box->Nlines)
       box->curline = box->Nlines-1;
     else if(box->curline < box->topline)
       box->curline = box->topline;
     if(box->curline >= 0 && box->curline < box->Nlines)
       line = box->lines[box->curline];
     else
       return;
     msg_x  = 5;
     for(i=0;line[i];i+= bbx_utf8_skip(line + i))
       if(x <=  bbx_utf8width(box->font, line, i) + msg_x)
           break;
     box->cursorpos = i;
     box->sel1line = -1;
     box->sel1line = -1;
     box->sel2pos = -1;
     box->sel2line = -1;
	 bbx_setfocus(box->bbx, box->pan);
 
     drawme(box);
    } 	
    else if(action == BBX_MOUSE_MOVE && buttons != 0)
    {
      font_height = box->font->ascent + box->font->descent;
      curline = (y-5)/font_height + box->topline;
      if(curline >= box->Nlines)
        curline = box->Nlines-1;
      else if(curline < 0)
        curline = 0;
      if(curline >= 0 && curline < box->Nlines)
        line = box->lines[curline];
     else
       return;
     msg_x  = 5;
     for(i=0;line[i];i+= bbx_utf8_skip(line + i))
       if(x <=  bbx_utf8width(box->font, line, i) + msg_x)
           break;
     curpos = i;
     if(curline < box->topline)
     {
       box->topline = curline;
       bbx_scrollbar_setpos(box->sb, box->topline);
     }
     if(curline >= box->topline + box->Nshown)
     {
	if(box->topline < box->Nlines - box->Nshown)
	{
	  box->topline++;
          bbx_scrollbar_setpos(box->sb, box->topline);
        }
     }

     if(box->curline < curline || (box->curline == curline && box->cursorpos < curpos))
     {
       box->sel1line = box->curline;
       box->sel1pos = box->cursorpos;
       box->sel2line = curline;
       box->sel2pos = curpos;
     } 
     else
     {
       box->sel1line = curline;
       box->sel1pos = curpos;
       box->sel2line = box->curline;
       box->sel2pos = box->cursorpos;
     }  
   
	 drawme(box);
    }
  
}


static void keyfunc(void *obj, int ch)
{
  EDITBOX *box = obj;
  char *line;
  int nb;
  int i;
  int curwidth;

  /* copy still enabled */
  if(box->disabled && ch != 3)
    return;
  line = box->lines[box->curline];
  if(ch < 0)
  {
    switch(ch)
    {
      
 case BBX_KEY_BACKSPACE:
    if(box->cursorpos > 0)
    {
      nb = prevskip(line + box->cursorpos);
      memmove(&line[box->cursorpos-nb], &line[box->cursorpos],
	      strlen(line)-box->cursorpos+1);
      box->cursorpos-=nb; 
    }
    else if(box->cursorpos == 0 && box->curline > 0)
    {
      box->curline--;
      box->cursorpos = strlen(box->lines[box->curline]);
      if(box->cursorpos > 0)
      {
        nb = prevskip(box->lines[box->curline] + box->cursorpos);
        box->cursorpos-= nb;
        box->lines[box->curline][box->cursorpos] = 0;
      }
    }
    reformattext(box);
    break;
     
  case BBX_KEY_DELETE:
    if(box->cursorpos < (int) strlen(line))
    {
      nb = bbx_utf8_skip(line + box->cursorpos);
      memmove(line + box->cursorpos, line+box->cursorpos +nb,
	      strlen(line) - box->cursorpos - nb + 1); 
    }
    else if(box->curline < box->Nlines -1)
    {
      line = box->lines[box->curline +1];
      if(line[0])
      {
        nb = prevskip(line + strlen(line));
        memmove(line, line+nb, strlen(line) - nb + 1);
      }
    }
    reformattext(box);
    break;
  case BBX_KEY_ESCAPE:
  case BBX_KEY_HOME:
    box->cursorpos = 0;
    break;
    
  case BBX_KEY_LEFT:
    if(box->cursorpos > 0)
      box->cursorpos -= prevskip(line + box->cursorpos);
    break;                         
  case BBX_KEY_UP:
    if(box->curline > 0)
    {
      box->curline--;
      curwidth = bbx_utf8width(box->font, line, box->cursorpos);
      line = box->lines[box->curline];
      for(i=0; line[i];i+= bbx_utf8_skip(line + i))
        if(bbx_utf8width(box->font, line, i) >= curwidth)
	  break;
      box->cursorpos = i;
      if(box->curline < box->topline)
      {
	box->topline = box->curline;
        bbx_scrollbar_setpos(box->sb, box->topline);
      }
    }
 
    break;                           
  case BBX_KEY_RIGHT:
    if(box->cursorpos < (int) strlen(line))
      box->cursorpos+= bbx_utf8_skip(line + box->cursorpos);
    break;                        
  case BBX_KEY_DOWN:
    if(box->curline < box->Nlines -1)
    {
      box->curline++;
      curwidth = bbx_utf8width(box->font, line, box->cursorpos);
      line = box->lines[box->curline];
      for(i=0; line[i];i+= bbx_utf8_skip(line +i))
        if(bbx_utf8width(box->font, line, i) >= curwidth)
	  break;
      box->cursorpos = i;
      if(box->curline >= box->topline + box->Nshown)
	if(box->topline < box->Nlines - box->Nshown)
	{
	  box->topline++;
          bbx_scrollbar_setpos(box->sb, box->topline);
        }

    }
    break;                      
  case BBX_KEY_END:
    box->cursorpos = strlen(line);                           
    break;
 case BBX_KEY_GOTFOCUS:  
   if(!box->ticker)
          box->ticker = bbx_addticker(box->bbx, 400, tickcaret, box);
   box->state = 1;
     break;
   case BBX_KEY_LOSTFOCUS:
        if(box->ticker)
	{
          bbx_removeticker(box->bbx, box->ticker);
          box->ticker = 0;
	}
        box->state = 0;
        break;

    }

  }
  if(ch > 0)
  {
    if(ch == 3)
    {
      char *temp = getselectedtext(box);
      if(temp)
        bbx_copytexttoclipboard(box->bbx, temp);
      free(temp);
    }
    else if(ch == 22)
    {
      char *temp = bbx_gettextfromclipboard(box->bbx);
      for(i=0;temp[i]; i+= bbx_utf8_skip(temp+i))
	addchar(box, bbx_utf8_getch(temp+i));
      free(temp);
    }
    else if(ch == 24)
    {
      char *temp = getselectedtext(box);
      if(temp)
      {
        bbx_copytexttoclipboard(box->bbx, temp);
        deleteselectedtext(box);
      }
      free(temp);
    }
    else
    {
      box->sel1line = -1;
      box->sel1pos = -1;
      box->sel2line = -1;
      box->sel2pos = -1;
      addchar(box, ch);
    } 
  }
  drawme(box);

}

static void tickcaret(void *obj)
{
  EDITBOX *box = obj;

  box->state ^= 1;
  drawme(box);
}

static void settext(EDITBOX *box, char *text)
{
    int width, height;
    int i;

    bbx_canvas_rgba(box->can, &width, &height);

    if(width < 100)
      width = 100;
    for(i=0;i<box->Nlines;i++)
      free(box->lines[i]);
    free(box->lines);
   
    box->lines = texttolines(box->font, text, width-10);
    box->Nlines = 0;
    for(i=0;box->lines[i];i++)
      box->Nlines++;
    box->Nshown = height/ (box->font->ascent + box->font->descent);
    if(box->Nshown < 1)
      box->Nshown = 1;
    if(box->Nlines > 0)
    {
      box->curline = box->Nlines -1;
      box->cursorpos = strlen(box->lines[box->curline]);
    } 
    else
    {
      if(box->lines)
        free(box->lines);
      box->lines = bbx_malloc(2 * sizeof(char *));
      box->lines[0] = bbx_malloc(2);
      box->lines[0][0] = 0;
      box->lines[1] = 0;
      box->Nlines = 1;
      box->curline = 0;
      box->cursorpos = 0;
      box->Nshown = 1;
    }      
}

static void drawme(EDITBOX *box)
{
  int i;
  int adj;
  int xadj;
  int x, y, width, height;
  unsigned char *rgba;
  BBX_RGBA white;
  BBX_RGBA lightgrey;
  BBX_RGBA red;
  BBX_RGBA black;

  rgba = bbx_canvas_rgba(box->can, &width, &height);
  black = bbx_color("black");
  white = bbx_color("white");
  red = bbx_color("red");
  lightgrey = bbx_color("light gray");

  if(box->disabled)
    bbx_rectangle(rgba, width, height, 0, 0, width-1, height-1, lightgrey);
  else
    bbx_rectangle(rgba, width, height, 0, 0, width-1, height-1, white);

  for(i=box->topline; i < box->topline + box->Nshown; i++)
  {
    if(!box->lines[i])
      break;
    y = (i - box->topline) * (box->font->ascent + box->font->descent) 
      + box->font->ascent;

    if(strchr(box->lines[i], '\n'))
      adj = 1;
    else
      adj = 0;

    if(box->sel1pos == -1 || i < box->sel1line || i > box->sel2line)
       bbx_drawutf8(rgba, width, height, 5, y, box->lines[i], strlen(box->lines[i]) - adj, box->font, black);
    else if(i > box->sel1line && i < box->sel2line)
      drawselected(rgba, width, height, 5, y, box->lines[i], strlen(box->lines[i]) - adj, box->font);
    else
    {
      if(i == box->sel1line)
      {
        if(box->lines[i][box->sel1pos-1] == '\n')
          xadj = 1;
        else
          xadj = 0;
	bbx_drawutf8(rgba, width, height, 5, y, box->lines[i], box->sel1pos-xadj, box->font, black);
        xadj = box->sel1pos;
        x = 5 + bbx_utf8width(box->font, box->lines[i], box->sel1pos);   
      }
      else
      {
        xadj = 0;
        x = 5;
      }
      if(i == box->sel2line)
      {
	if(strlen(box->lines[i]) == box->sel2pos)
         drawselected(rgba, width, height, x, y, box->lines[i] + xadj, box->sel2pos-xadj - adj, box->font);
        else
	{
           drawselected(rgba, width, height, x, y, box->lines[i] + xadj, box->sel2pos-xadj, box->font);
           x = 5 + bbx_utf8width(box->font, box->lines[i], box->sel2pos); 
           bbx_drawutf8(rgba, width, height, x, y, box->lines[i] + box->sel2pos, strlen(box->lines[i]) - adj - box->sel2pos, box->font, black);
        }
      }
      else
       drawselected(rgba, width, height, x, y, box->lines[i] + xadj, strlen(box->lines[i] + xadj) - adj, box->font);
      
    }


    if(box->state == 1 && i == box->curline && !box->disabled)
    {
      x = bbx_utf8width(box->font, box->lines[i], box->cursorpos) +5;
      bbx_line(rgba, width, height, x, y - box->font->ascent, x, y + box->font->descent, red); 
    }
  }
  bbx_canvas_flush(box->can);

}

static void drawselected(unsigned char *rgba, int width, int height, int x, int y, char *str, int N, struct bitmap_font *font)
{
  int twidth;
  int y1, y2;
  BBX_RGBA blue;

  blue = bbx_color("blue");
  twidth = bbx_utf8width(font, str, N);
  y1 = y - font->ascent;
  y2 = y + font->descent;
  bbx_rectangle(rgba, width, height, x, y1, x + twidth, y2, blue);
  bbx_drawutf8(rgba, width, height, x, y, str, N, font, bbx_color("white"));
  
}

static void scrollme(void *obj, int pos)
{
  EDITBOX *box = obj;
  if(pos < box->Nlines)
    box->topline = pos;
  drawme(box);
}

static int addchar(EDITBOX *box, int ch)
{
  char *line;
  BABYX *bbx = box->bbx;
  int len;
  int width, height;
  int nb;
  char buff[32];

  bbx_getsize(bbx, box->pan, &width, &height);

  nb = bbx_utf8_putch(buff, ch);
  line = box->lines[box->curline];
  len = strlen(line);
  line = realloc(line, len + nb + 1);
  if(!line)
    return -1;
  if(box->cursorpos >= 0 && box->cursorpos <= len)
  {
    memmove(line + box->cursorpos +nb, line + box->cursorpos, len + 1 - box->cursorpos);
    memcpy(line + box->cursorpos, buff, nb);
    box->lines[box->curline] = line;

    box->cursorpos+=nb;
    len += nb;
    if(len > 0 && line[len-1] == '\n')
      len--;
	if (ch == '\n' || bbx_utf8width(box->font, line, len) > width - 30 - 1 * 2)
	{
		reformattext(box);
	}
	if (ch == '\n')
	{
		if (box->curline < box->Nlines - 1)
		{
			box->cursorpos = 0;
			box->curline++;
		}
	}
  } 

  return 0;  
}

static char *getselectedtext(EDITBOX *box)
{
  int N = 0;
  int i;
  char *answer;

  if(box->sel1line == -1 || box->sel1pos == -1)
    return 0;
  
  for(i=box->sel1line; i <= box->sel2line;i++)
    N += strlen(box->lines[i]);
  N -= box->sel1pos;
  N -= strlen(box->lines[box->sel2line]);
  N += box->sel2pos;
  answer = bbx_malloc(N + 1);
  answer[0] = 0;
  for(i=box->sel1line; i <= box->sel2line;i++)
  {
    if(i== box->sel1line && i == box->sel2line)
    {
      memcpy(answer, box->lines[i] + box->sel1pos, box->sel2pos - box->sel1pos);
      answer[box->sel2pos - box->sel1pos] = 0;
    }
    else if(i == box->sel1line)
      strcat(answer, box->lines[i] + box->sel1pos);
    else if(i == box->sel2line)
    {
      memcpy(answer + strlen(answer), box->lines[i], box->sel2pos);
      answer[N] = 0;
    }
    else
      strcat(answer, box->lines[i]);
  }

  return answer;
}

static void deleteselectedtext(EDITBOX *box)
{
  int i;

  if(box->sel1line == -1)
    return;
  for(i=box->sel1line;i<=box->sel2line;i++)
  {
    if(i==box->sel1line && i == box->sel2line)
    {
      memmove(box->lines[i]+box->sel1pos, 
             box->lines[i]+box->sel2pos,
             strlen(box->lines[i]) - box->sel2pos + 1); 
    }
    else if(i==box->sel1line)
    {
      box->lines[i][box->sel1pos] = 0;
    }
    else if(i == box->sel2line)
    {
      memmove(box->lines[i], 
              box->lines[i]+box->sel2pos, 
              strlen(box->lines[i]) - box->sel2pos + 1);
    }
    else
      box->lines[i][0] = 0;
  }
  if(box->curline == box->sel2line && box->cursorpos == box->sel2pos)
  {
    box->curline = box->sel1line;
    box->cursorpos = box->sel1pos;
  }
  box->sel1line = -1;
  box->sel1pos = -1;
  box->sel2line = -1;
  box->sel2pos = -1;
  reformattext(box);
}

static int reformattext(EDITBOX *box)
{
  char *buff;
  int len = 0;
  int cursorindex = 0;
  int i;
 
  for(i=0;i<box->Nlines;i++)
  {
    if(i == box->curline)
      cursorindex = len + box->cursorpos;
    len += strlen(box->lines[i]);
  }
  buff = bbx_malloc(len+1);
  len = 0;
  for(i=0;i<box->Nlines;i++)
  {
    strcpy(buff+len, box->lines[i]);
    len += strlen(box->lines[i]); 
  }
  settext(box, buff);
//  free(buff);
  len = 0;
  for(i=0;i<box->Nlines;i++)
  {
    len += strlen(box->lines[i]);
    if(len > cursorindex)
    {
      box->curline = i;
      box->cursorpos = cursorindex + strlen(box->lines[i]) - len; 
      break;
    } 
  }
  if(i == box->Nlines)
  {
    box->curline = box->Nlines -1;
    if(box->curline >= 0)
      box->cursorpos = strlen(box->lines[box->curline]);
  }   
  if(box->curline >= box->topline + box->Nshown)
    box->topline = box->curline - box->Nshown + 1;
  else if(box->topline + box->Nshown <= box->curline)
    box->topline = box->curline;
  if(box->topline > box->Nlines - box->Nshown)
    box->topline = box->Nlines - box->Nshown;
  if(box->topline < 0)
    box->topline = 0;
  bbx_scrollbar_set(box->sb, box->Nlines, box->Nshown, box->topline);

  free(buff);
  return 0;
}


static char **texttolines(struct bitmap_font *fs, char *text, int width)
{
  int i;
  char **answer;
  int Nlines = 0;
  int capacity = 100;
  int displaylen;
  int starti = 0;
  int savei;
  int nb;

  answer = bbx_malloc(capacity * sizeof(char *));
  
  displaylen = 0;
  i = 0;
  while(text[i])
  {
    if(text[i] == '\n')
    {
      displaylen = 0;
      i++;
      answer = addline(answer, Nlines, text + starti, i-starti, &capacity);
      Nlines++;
      starti = i;
      continue;
    }
    nb = bbx_utf8_skip(text + i);
    displaylen += bbx_utf8width(fs, text+i, nb);
    i+= nb;
    if(displaylen > width)
    {
      savei = i;
      i-= prevskip(text + i);
      while(i > 0 && displaylen > 0)
      {
        displaylen -= bbx_utf8width(fs, text + i, bbx_utf8_skip(text+i));
        i -= prevskip(text + i);
        if(isspace(text[i]))
          break;
      }
      if(displaylen <= 0)
      {
        i = savei - prevskip(text + savei);
        i -= prevskip(text + i);
      }
      i+= bbx_utf8_skip(text + i);
      answer = addline(answer, Nlines, text + starti, i-starti, &capacity);
      Nlines++;
      starti = i;
      displaylen = 0;
    }
  }
  if(i > starti)
  {
    answer = addline(answer, Nlines, text + starti, i -starti, &capacity);
    Nlines++;
  }
  answer[Nlines] = bbx_strdup("");
  answer[Nlines + 1] = 0;
  return answer;
} 

static char **addline(char **buff, int N, char *str, int len, int *capacity)
{
  char *line;
  char **temp;

  line = bbx_malloc(len + 1);
  if(!line)
    return 0;
  memcpy(line, str, len);
  line[len] = 0;
  
  if(N >= *capacity -3)
  {
    temp = bbx_realloc(buff, *capacity * 2 * sizeof(char *));
    if(!temp)
      return 0;
    buff = temp;
    *capacity *= 2;
  }
  buff[N] = line;

  return buff;
} 

static int prevskip(char *utf8)
{
  int nb = 1;
  while( (utf8[-nb] & 0xC0) == 0x80)
    nb++;
  return nb;
}
