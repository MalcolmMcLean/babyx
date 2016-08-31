#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>

#include "BabyX.h"


extern struct bitmap_font fred_font;

static void event_handler(void *obj, XEvent *event);
static void render(BBX_Label *lab);
static XImage *CreateTrueColorImage(Display *display, Visual *visual, unsigned long col, int width, int height);
static int getNlines(char *str);

BBX_Label *bbx_label(BABYX *bbx, BBX_Panel *parent, char *text)
{
  return BBX_label(bbx, parent->win, text);
}

BBX_Label *BBX_label(BABYX *bbx, Window parent, char *text)
{
  BBX_Label *answer;

  answer = bbx_malloc(sizeof(BBX_Label));
  answer->bbx = bbx;
  answer->win = XCreateSimpleWindow(bbx->dpy, parent, 0, 0, 100, 100,
				    0, BlackPixel(bbx->dpy, 0), WhitePixel(bbx->dpy, 0));
 XSelectInput(bbx->dpy, answer->win, ExposureMask | StructureNotifyMask);

  answer->event_handler = event_handler;
  answer->message_handler = 0;
  answer->text = bbx_strdup(text);
  answer->fgcol = bbx_color("black");
  answer->bgcol = bbx_color("gray");
  answer->img = 0;
  answer->font = bbx->gui_font;
  answer->align = BBX_ALIGN_CENTER;

  BBX_Register(bbx, answer->win, event_handler, 0, answer);

  return answer;  
}  

void bbx_label_kill(BBX_Label *obj)
{
  if(obj)
  {
    BBX_Deregister(obj->bbx, obj->win);
    XDestroyWindow(obj->bbx->dpy, obj->win);
    if(obj->img)
      XDestroyImage(obj->img);
    free(obj->text);
    free(obj);
  }
}

void bbx_label_settext(BBX_Label *obj, char *text)
{
  free(obj->text);
  obj->text = bbx_strdup(text);
  render(obj);
  BBX_InvalidateWindow(obj->bbx, obj->win);
}

void bbx_label_setalignment(BBX_Label *obj, int align)
{
  if(obj->align != align)
  {
    obj->align = align;
    render(obj);
    BBX_InvalidateWindow(obj->bbx, obj->win);
  }
}

void bbx_label_setbackground(BBX_Label *obj, BBX_RGBA col)
{
  if(obj->bgcol != col)
  {
    obj->bgcol = col;
    render(obj);
    BBX_InvalidateWindow(obj->bbx, obj->win);
  }
}

void bbx_label_setforeground(BBX_Label *obj, BBX_RGBA col)
{
  if(obj->fgcol != col)
  {
    obj->fgcol = col;
    render(obj);
    BBX_InvalidateWindow(obj->bbx, obj->win);
  }
}

void bbx_label_setfont(BBX_Label *obj, struct bitmap_font *fs)
{
  obj->font = fs;
  render(obj);
  BBX_InvalidateWindow(obj->bbx, obj->win);
}

int bbx_label_getpreferredsize(BBX_Label *lab, int *width, int *height)
{
    int w = 0;
    int h;
    int Nlines;
    char *line;
    char *end;
    int i;
    int temp;
    int len;

    Nlines = getNlines(lab->text);
    
    line = lab->text;
    end = strchr(line, '\n');
     
    for(i=0;i<Nlines;i++)
    {
        if(end)
          len = end - line;
        else
          len = strlen(line);
        temp = bbx_utf8width(lab->font, line, len);
        if(w < temp)
          w = temp;
        if(end)
	{
          line = end + 1;
          end = strchr(line, '\n');
	}
    }

    h = Nlines * (lab->font->ascent + lab->font->descent);
    *width = w + 5;
    *height = h;
    return 0;
}
 
static void event_handler(void *obj, XEvent *event)
{
  BBX_Label *lab = obj;
  int width, height;
  BABYX *bbx;
 

  bbx = lab->bbx;

  switch(event->type)
  {
  case Expose:
    if(!lab->img)
      return;
     bbx_getsize(bbx, lab, &width, &height);      
     XPutImage(bbx->dpy, lab->win, bbx->gc, lab->img, 0, 0, 0, 0, width, height);
     break;
  case ConfigureNotify:
    if(lab->img)
      XDestroyImage(lab->img);
    lab->img = CreateTrueColorImage(bbx->dpy,  
                                    DefaultVisual(bbx->dpy, bbx->screen),
				    lab->bgcol,
                                    event->xconfigure.width,
                                    event->xconfigure.height);
    render(lab);
    break;
   
  }

}

static void render(BBX_Label *lab)
{
  unsigned char *buff;
  int font_height;
  int Nlines;
  int i, j;
  char *line;
  char *end;
  int len;
  int msg_x, msg_y, msg_len;
  int width, height;

  if(!lab->img)
    return;
  
  width = lab->img->width;
  height = lab->img->height;

  buff = malloc(width * height * 4);
  for(i=0;i<width*height;i++)
  {
    buff[i*4] = bbx_red(lab->bgcol);
    buff[i*4+1] = bbx_green(lab->bgcol);
    buff[i*4+2] = bbx_blue(lab->bgcol);
    buff[i*4+3] = 0xFF;
  }
         
  font_height = lab->font->ascent + lab->font->descent;

  Nlines = getNlines(lab->text);
  line = lab->text;
  end = strchr(line, '\n');
     
  for(i=0;i<Nlines;i++)
  {
    if(end)
      len = end - line;
    else
      len = strlen(line);
    msg_len = bbx_textwidth(lab->font, line, len);
        
    if(lab->align == BBX_ALIGN_CENTER)
       msg_x  = (width - msg_len) / 2;
    else if(lab->align == BBX_ALIGN_RIGHT)
       msg_x = width - msg_len;
     else
        msg_x = 0;
      msg_y  = i*font_height + lab->font->ascent + (height - Nlines *font_height)/2; 

     bbx_drawutf8(buff, width, height, msg_x, msg_y, line, len, lab->font, lab->fgcol);
     if(end != 0)
     {
       line = end + 1;
       end = strchr(line, '\n');
     }
   }
  j = 0;
  for(i=0;i<width * height;i++)
  {   
    lab->img->data[j] =  buff[j+2];
    lab->img->data[j+1] = buff[j+1];
    lab->img->data[j+2] = buff[j];
    lab->img->data[j+3] = buff[j+3];
    j+=4;
  }
  free(buff);
 
}

static XImage *CreateTrueColorImage(Display *display, Visual *visual, unsigned long col, int width, int height)
{
    int i, j;
    unsigned char *image32=(unsigned char *)bbx_malloc(width*height*4);
    unsigned char *p=image32;
    for(i=0; i<width; i++)
    {
        for(j=0; j<height; j++)
        {
          
	  *p++= col & 0xFF; /* blue */
	  *p++= (col >>8 ) & 0xFF; /* green */
	  *p++= (col >> 16) & 0xFF; /* red */
          p++;  
	}
    }
    return XCreateImage(display, visual, 24, ZPixmap, 0, (char *) image32, width, height, 32, 0);
}

static int getNlines(char *str)
{
  int answer = 0;
  int i;

  for(i=0;str[i];i++)
    if(str[i] == '\n')
      answer++;
  if(i > 0 && str[i-1] != '\n')
    answer++;
  return answer;
}


