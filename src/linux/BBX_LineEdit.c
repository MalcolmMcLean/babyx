#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "BabyX.h"


typedef struct
{
  BABYX *bbx;
  BBX_Panel *pan;
  BBX_Canvas *can;
  char *text;
  void *ptr;
  void (*changed)(void *ptr, char *text);
  struct bitmap_font *font;
  int state;
  void *ticker;
  int dirty;
  int disabled;
  int cursorpos;
  int sel1;
  int sel2;
} LINEEDIT;


static void layout(void *obj, int width, int height);
static void mousefunc(void *obj, int action, int x, int y, int buttons);
static void keyfunc(void *obj, int ch);
static void tickcaret(void *obj);
static void redraw(LINEEDIT *edt);
static void trim(char *str);

BBX_LineEdit *bbx_lineedit(BABYX *bbx, BBX_Panel *parent, char *text, void (*fptr)(void *ptr, char *text), void *ptr )
{
  return BBX_lineedit(bbx, parent->win, text, fptr, ptr);
}

BBX_LineEdit *BBX_lineedit(BABYX *bbx, Window parent, char *text, void (*fptr)(void *ptr, char *text), void *ptr )
{
  LINEEDIT *obj;

  obj = bbx_malloc(sizeof(LINEEDIT));
  obj->bbx = bbx;
  obj->pan = BBX_panel(bbx, parent, "lineedit", layout, obj); 
  obj->can = bbx_canvas(bbx, obj->pan, 10, 10, 0);
  obj->changed = fptr;
  obj->ptr = ptr;
  obj->text = bbx_malloc(256);
  strcpy(obj->text, text);

  obj->font = bbx->user_font2;
  obj->state = 0;
  obj->ticker = 0;
  obj->dirty = 0;
  obj->disabled = 0;
  obj->cursorpos = strlen(obj->text);
  obj->sel1 = -1;
  obj->sel2 = -1;
  bbx_panel_setmousefunc(obj->pan, mousefunc, obj);
  bbx_panel_setkeyfunc(obj->pan, keyfunc, obj);

  return obj->pan;  
}

void bbx_lineedit_kill(BBX_LineEdit *le)
{
  LINEEDIT *edt;

  if(le)
  {
    edt = bbx_panel_getptr(le);
    if(edt)
    {
      bbx_canvas_kill(edt->can);
      free(edt->text);
      free(edt);
    }
    bbx_panel_kill(le);
  }  
}

char *bbx_lineedit_gettext(BBX_LineEdit *le)
{
  char *answer;
  LINEEDIT *edt = bbx_panel_getptr(le);
  
  answer = bbx_strdup(edt->text);
  trim(answer);

  return answer;
}

void bbx_lineedit_settext(BBX_LineEdit *le, char *text)
{
  LINEEDIT *edt = bbx_panel_getptr(le);
  if(strlen(text) < 256)
  {
    strcpy(edt->text, text);
    edt->cursorpos = strlen(text);
    redraw(edt);
  }
}

void bbx_lineedit_disable(BBX_LineEdit *le)
{
  LINEEDIT *edt = bbx_panel_getptr(le);

  edt->state = 0;
  edt->disabled = 1;
  redraw(edt); 
}


void bbx_lineedit_enable(BBX_LineEdit *le)
{
  LINEEDIT *edt = bbx_panel_getptr(le);
  edt->disabled = 0;
  redraw(edt); 
}



static void layout(void *obj, int width, int height)
{
  int cw, ch;
  LINEEDIT *edt = obj;

  bbx_canvas_rgba(edt->can, &cw, &ch);
  if(cw != width || ch != height)
  {
    bbx_canvas_kill(edt->can);
    edt->can = bbx_canvas(edt->bbx, edt->pan, width, height, 0); 
  }
  bbx_setpos(edt->bbx, edt->can, 0, 0, width, height);
  redraw(edt);
}

static void mousefunc(void *obj, int action, int x, int y, int buttons)
{
  LINEEDIT *edt = obj;
  int msg_x;
  int length;
  int width, height;
  int i; 

  if(action == BBX_MOUSE_CLICK)
  {
     bbx_getsize(edt->bbx, edt->pan, &width, &height);
     length = bbx_utf8width(edt->font, edt->text, strlen(edt->text));
     msg_x  = (width - length) / 2;
     for(i=0;edt->text[i];i+= bbx_utf8_skip(edt->text + i))
       if(x <=  bbx_utf8width(edt->font, edt->text, i) + msg_x)
           break;
     edt->cursorpos = i;
     edt->sel1 = -1;
     edt->sel2 = -1; 		 
     redraw(edt);
  }
 
  else if(action == BBX_MOUSE_MOVE && buttons != 0)
  {
     bbx_getsize(edt->bbx, edt->pan, &width, &height);
     length = bbx_utf8width(edt->font, edt->text, strlen(edt->text));
     msg_x  = (width - length) / 2;
     for(i=0;edt->text[i];i+= bbx_utf8_skip(edt->text + i))
       if(x <=  bbx_utf8width(edt->font, edt->text, i) + msg_x)
           break;

     if(i < edt->cursorpos)
     {
       edt->sel1 = i;
       edt->sel2 = edt->cursorpos;
     }
     else 
     {
       edt->sel1 = edt->cursorpos;
       if(edt->text[i])
         edt->sel2 = i + bbx_utf8_skip(edt->text + i);
       else
         edt->sel2 = i;
     } 
    redraw(edt);
  }
}

static void keyfunc(void *obj, int ch)
{
  LINEEDIT *edt = obj;
  char buff[32];
  int nb;

  if(edt->disabled)
    return;

  if(ch < 0)
  {
    switch(ch)
    {
      case BBX_KEY_GOTFOCUS:
        if(!edt->ticker)
          edt->ticker = bbx_addticker(edt->bbx, 400, tickcaret, edt);
        edt->state = 1;
        break;
      case BBX_KEY_LOSTFOCUS:
        if(edt->ticker)
	{
          bbx_removeticker(edt->bbx, edt->ticker);
          edt->ticker = 0;
	}
        edt->state = 0;
        break;
      case BBX_KEY_BACKSPACE:
        if(edt->cursorpos > 0)
        {
          nb = 1;
          while(edt->cursorpos -nb > 0)
	  {
            if( (edt->text[edt->cursorpos-nb] & 0xC0) == 0x80)
	       nb++;
	    else break;
          }         
          memmove(&edt->text[edt->cursorpos-nb], &edt->text[edt->cursorpos],
	      strlen(edt->text)-edt->cursorpos+1);
          edt->cursorpos-=nb;
          edt->dirty = 1; 
         }
	break;
      case BBX_KEY_DELETE:
        if(edt->cursorpos < strlen(edt->text))
        {
	  nb = bbx_utf8_skip(edt->text + edt->cursorpos);
          memmove(edt->text + edt->cursorpos, edt->text+edt->cursorpos +nb,
	      strlen(edt->text) - edt->cursorpos - nb + 1);
          edt->dirty = 1; 

        }
        break;
      case BBX_KEY_ESCAPE: break;
      case BBX_KEY_HOME: break;
 
      case BBX_KEY_LEFT:
        if(edt->cursorpos > 0)
        {
          nb = 1;
          while(edt->cursorpos -nb > 0)
          {
            if( (edt->text[edt->cursorpos-nb] & 0xC0) == 0x80)
	      nb++;
	    else
	    break;
          }
         edt->cursorpos-= nb;
       }
       break;                         
     case BBX_KEY_UP:
       break;                  
     case BBX_KEY_RIGHT:
       if(edt->cursorpos < strlen(edt->text))
         edt->cursorpos++;
       break;                         
     case BBX_KEY_DOWN:
       break;                          
     case BBX_KEY_END:
        edt->cursorpos = strlen(edt->text);                           
       break;

     default:
        break;
    }
  }
  else if(ch == 3)
  {
    if(edt->sel1 != -1)
    {
      char *temp;
       temp = bbx_malloc(edt->sel2 - edt->sel1 + 1);
       memcpy(temp, edt->text + edt->sel1, edt->sel2 - edt->sel1);
       temp[edt->sel2 - edt->sel1] = 0; 
       bbx_copytexttoclipboard(edt->bbx, temp);
       free(temp);
    }
  }
  else if( ch == 22 )
  {
    char *temp = bbx_gettextfromclipboard(edt->bbx);
    if(temp)
    {
      nb = strlen(temp);
      if(nb > 0 && nb  + strlen(edt->text) < 256)
      {
         memmove(edt->text + edt->cursorpos + nb,
            edt->text + edt->cursorpos,
            strlen(edt->text) - edt->cursorpos + 1);
          memcpy(edt->text + edt->cursorpos, temp, nb); 
         edt->cursorpos += nb;
         edt->dirty = 1;
      }
      free(temp);
    }
  }
  else if(ch == 24)
  {
    if(edt->sel1 != -1)
    {
       char *temp;
       temp = bbx_malloc(edt->sel2 - edt->sel1 + 1);
       memcpy(temp, edt->text + edt->sel1, edt->sel2 - edt->sel1);
       temp[edt->sel2 - edt->sel1] = 0; 
       bbx_copytexttoclipboard(edt->bbx, temp);
       free(temp);
       memmove(edt->text + edt->sel1, edt->text + edt->sel2, strlen(edt->text) - edt->sel2 + 1);
       edt->cursorpos = edt->sel1;
       edt->sel1 = -1;
       edt->sel2 = -1;
       
    }
  }
  else if(ch == '\n')
  {
    edt->dirty = 0;  
    if(edt->changed)
      (*edt->changed)(edt->ptr, edt->text);
  }
  else
  {
    edt->sel1 = -1;
    edt->sel2 = -1;
    nb = bbx_utf8_putch(buff, ch);
    if(strlen(edt->text) + nb < 256)
    {
      memmove(edt->text + edt->cursorpos + nb,
            edt->text + edt->cursorpos,
            strlen(edt->text) - edt->cursorpos + 1);
      memcpy(edt->text + edt->cursorpos, buff, nb); 
      edt->cursorpos += nb;
      edt->dirty = 1;
    } 
  }

  redraw(edt);
}

static void tickcaret(void *obj)
{
  LINEEDIT *edt = obj;

  edt->state ^= 1;
  redraw(edt);
}

static void redraw(LINEEDIT *edt)
{
  unsigned char *rgba;
  int width, height;
  BBX_RGBA bgcol;
  BBX_RGBA fgcol;
  BBX_RGBA dimgrey;
  BBX_RGBA higrey;
  BBX_RGBA red;
  BBX_RGBA blue;
  int msg_x, msg_y;
  int selend_x;
  int length;
  int font_height;

  rgba = bbx_canvas_rgba(edt->can, &width, &height);
  higrey = bbx_color("light gray");
  dimgrey = bbx_color("dim gray");
  red = bbx_color("red");
  blue = bbx_color("blue");
  if(edt->disabled)
  {
    bgcol = bbx_color("light gray");
    fgcol = bbx_color("black"); 
  }
  else
  {
    bgcol = bbx_color("white");
    fgcol = bbx_color("black");
  }
   
  bbx_rectangle(rgba, width, height, 0, 0, width-1, height-1, bgcol);  
  bbx_line(rgba, width, height, 0, 0, width-1, 0, dimgrey);
  bbx_line(rgba, width, height, 0, 0, 0, height-1, dimgrey);
  bbx_line(rgba, width, height, 0, height-1, width-1, height-1, higrey);
  bbx_line(rgba, width, height, width-1, 0, width-1, height-1, higrey);

  length = bbx_utf8width(edt->font, edt->text, strlen(edt->text));
  msg_x  = (width - length) / 2;

  font_height = edt->font->ascent + edt->font->descent;
   msg_y  = (height - font_height)/2 + edt->font->ascent;

   if(edt->state == 1 && !edt->disabled)
   {
         length = bbx_utf8width(edt->font, edt->text, edt->cursorpos);
         bbx_line(rgba, width, height, msg_x + length, msg_y -edt->font->ascent,
		  msg_x + length, msg_y + edt->font->descent, red); 
   } 
     
   if(edt->sel1 == -1)
   {
     bbx_drawutf8(rgba, width, height, msg_x, msg_y, edt->text, strlen(edt->text), edt->font, fgcol);
   }
   else
   {
       bbx_drawutf8(rgba, width, height, msg_x, msg_y, edt->text, edt->sel1, edt->font, fgcol);
       msg_x += bbx_utf8width(edt->font, edt->text, edt->sel1);
       selend_x = msg_x + bbx_utf8width(edt->font, edt->text + edt->sel1, edt->sel2 - edt->sel1);
       bbx_rectangle(rgba, width, height, msg_x, msg_y - edt->font->ascent,
		     selend_x, msg_y + edt->font->descent, blue);
       bbx_drawutf8(rgba, width, height, msg_x, msg_y, edt->text + edt->sel1, edt->sel2 - edt->sel1, edt->font, bbx_color("white"));
       msg_x += bbx_utf8width(edt->font, edt->text + edt->sel1, edt->sel2 - edt->sel1); 
       bbx_drawutf8(rgba, width, height, msg_x, msg_y, edt->text + edt->sel2, strlen(edt->text+edt->sel2), edt->font, bbx_color("black"));
   }
    
  bbx_canvas_flush(edt->can);
}

static void trim(char *str)
{
  char *ptr;

  ptr = str;
  while(isspace(*ptr))
    ptr++;
  if(ptr != str)
    memmove(str, ptr, strlen(ptr) + 1);
  if(str[0])
  {
    ptr = str + strlen(str) -1;
    while(isspace(*ptr))
      *ptr-- = 0;
  }
}
