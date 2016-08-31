#include <stdlib.h>

#include "BabyX.h"


static void event_handler(void *obj, XEvent *event);
static void setthumbsize(BBX_Scrollbar *obj);
static void thumb_event_handler(void *obj, XEvent *event);
static void dragged(void *obj, int pos);

BBX_Scrollbar *bbx_scrollbar(BABYX *bbx, BBX_Panel *parent, int direction, void (*fptr)(void *ptr, int pos), void *ptr)
{
  return BBX_scrollbar(bbx, parent->win, direction, fptr, ptr);
}

BBX_Scrollbar *BBX_scrollbar(BABYX *bbx, Window parent, int direction, void (*fptr)(void *ptr, int pos), void *ptr)
{
  BBX_Scrollbar *answer;

  answer = bbx_malloc(sizeof(BBX_Scrollbar));
  answer->bbx = bbx;
  answer->win = XCreateSimpleWindow(bbx->dpy, parent, 0, 0, 100, 100,
				    1, BBX_Color("dim gray"), BBX_Color("gray"));
 XSelectInput(bbx->dpy, answer->win, ExposureMask | StructureNotifyMask);


  answer->event_handler = event_handler;
  answer->message_handler = 0;
 
  answer->direction = direction;
  answer->change = fptr;
  answer->ptr = ptr;
  answer->range = 100;
  answer->visible = 1;
  answer->pos = 0;
  answer->thumb = bbx_scrollthumb(bbx, answer->win, direction, dragged, answer);

  BBX_Register(bbx, answer->win, event_handler, 0, answer);
  XMapWindow(bbx->dpy, answer->win);

  return answer;  
}

void bbx_scrollbar_kill(BBX_Scrollbar *obj)
{
  if(obj)
  {
    bbx_scrollthumb_kill(obj->thumb);
    BBX_Deregister(obj->bbx, obj->win);
    XDestroyWindow(obj->bbx->dpy, obj->win);
    free(obj);
  }
}

int bbx_scrollbar_set(BBX_Scrollbar *obj, int range, int visible, int pos)
{
  if(obj->range == range && obj->visible == visible && obj->pos == pos)
    return 0;
  obj->range = range;
  obj->visible = visible;
  setthumbsize(obj);
  
  bbx_scrollbar_setpos(obj, pos);
  BBX_InvalidateWindow(obj->bbx, obj->win);

  return 0;
}

int bbx_scrollbar_getpos(BBX_Scrollbar *obj)
{
  return obj->pos;
}

void bbx_scrollbar_setpos(BBX_Scrollbar *obj, int pos)
{
  double t;
  int x, y;
  BABYX *bbx = obj->bbx;

  obj->pos = pos;
  if(obj->range <= obj->visible)
  {
    obj->pos = 0;
    t = 0;
  }
  else if(pos > obj->range - obj->visible)
  {
    obj->pos = obj->range - obj->visible;
    t = 1.0;
  }
  else 
    t = ((double)pos) / (obj->range - obj->visible);

  if(obj->direction == BBX_SCROLLBAR_VERTICAL)
  {
    y = (int) (t * obj->thumb->maxy);
    XMoveWindow(bbx->dpy, obj->thumb->win, 0, y);
  }
  else if(obj->direction == BBX_SCROLLBAR_HORIZONTAL)
  {
    x = (int) (t * obj->thumb->maxx);
    XMoveWindow(bbx->dpy, obj->thumb->win, x, 0);
  }
}

static void event_handler(void *obj, XEvent *event)
{
  BBX_Scrollbar *bar = obj;
  int width, height;
  BABYX *bbx;
  
  bbx = bar->bbx;

  switch(event->type)
  {
  case Expose:
    bbx_getsize(bbx, bar, &width, & height);
     XClearWindow(bbx->dpy, bar->win);            
     break;
  case ConfigureNotify:
    setthumbsize(bar);
    bbx_scrollbar_setpos(bar, bar->pos);
    XMapWindow(bbx->dpy, bar->thumb->win); 
    break;

  }
}

static void setthumbsize(BBX_Scrollbar *obj)
{
  BABYX *bbx = obj->bbx;
   int width, height;
   int thumbwidth, thumbheight;
   double t;

   bbx_getsize(bbx, obj, & width, & height); 
   if(obj->range <= 0)
      t  = 1;
   else
      t = ((double)obj->visible) / obj->range;
  
  if(obj->direction == BBX_SCROLLBAR_VERTICAL)
  {
    thumbwidth = 18;
    thumbheight = (int) (t * height);
    if(thumbheight < 20)
      thumbheight = 20;
    if(thumbheight > height)
      thumbheight = height; 
     
    obj->thumb->maxy = height - thumbheight;
  }
  else if(obj->direction == BBX_SCROLLBAR_HORIZONTAL)
  {    
    thumbwidth = (int) (t * width);
    if(thumbwidth < 20)
      thumbwidth = 20;
    if(thumbwidth > width)
      thumbwidth = width;
    thumbheight = 18;

    obj->thumb->maxx = width - thumbwidth;
  }
  else
    return;
  XResizeWindow(bbx->dpy, obj->thumb->win, thumbwidth, thumbheight);
}

BBX_ScrollThumb *bbx_scrollthumb(BABYX *bbx, Window parent, int direction, void (*fptr)(void *ptr, int pos), void *ptr)
{
  BBX_ScrollThumb *answer;

  answer = bbx_malloc(sizeof(BBX_ScrollThumb));
  answer->bbx = bbx;
  answer->win = XCreateSimpleWindow(bbx->dpy, parent, 0, 0, 100, 100,
				    0, BlackPixel(bbx->dpy, 0), BBX_Color("light gray"));
 XSelectInput(bbx->dpy, answer->win, ExposureMask | KeyPressMask |
		 ButtonPressMask | ButtonReleaseMask | Button1MotionMask | StructureNotifyMask);

  
  answer->event_handler = event_handler;
  answer->message_handler = 0;
  answer->fptr = fptr;
  answer->ptr = ptr;
  answer->direction = direction;
  answer->lastx = -1;
  answer->lasty = -1;
  answer->ongrab = 0;

  BBX_Register(bbx, answer->win, thumb_event_handler, 0, answer);
  return answer;  
}

void bbx_scrollthumb_kill(BBX_ScrollThumb *obj)
{
  if(obj)
  {
    BBX_Deregister(obj->bbx, obj->win);
    XDestroyWindow(obj->bbx->dpy, obj->win);
    free(obj);
  }

}
static void thumb_event_handler(void *obj, XEvent *event)
{
  BBX_ScrollThumb *thumb = obj;
  Window root;
  int x, y, width, height;
  unsigned int uwidth, uheight, border_width, depth;
  BABYX *bbx;
  
  bbx = thumb->bbx;

  switch(event->type)
  {
  case Expose:
    //XGetGeometry(bbx->dpy, thumb->win, &root, &x, &y, &width, 
    //		  &height, &border_width, &depth);
    bbx_getsize(bbx, thumb, &width, &height);
     XClearWindow(bbx->dpy, thumb->win);       
     XSetForeground(bbx->dpy, bbx->gc, BBX_Color("gray"));     
     if(thumb->direction == BBX_SCROLLBAR_VERTICAL)
     {
       XDrawLine(bbx->dpy, thumb->win, bbx->gc, 4, height/2, width-4, height/2);
       XDrawLine(bbx->dpy, thumb->win, bbx->gc, 5, height/2+3, width-5, height/2+3);
       XDrawLine(bbx->dpy, thumb->win, bbx->gc, 5, height/2-3, width-5, height/2-3);
       XSetForeground(bbx->dpy, bbx->gc, BBX_Color("dim gray"));  
       XDrawLine(bbx->dpy, thumb->win, bbx->gc, 0, 0, width-1, 0);   
       XDrawLine(bbx->dpy, thumb->win, bbx->gc, 0, height-1, width-1, height-1);   
     }
     else if(thumb->direction == BBX_SCROLLBAR_HORIZONTAL)
     {
        XDrawLine(bbx->dpy, thumb->win, bbx->gc, width/2, 4, width/2, height-4);
        XDrawLine(bbx->dpy, thumb->win, bbx->gc, width/2+3, 5, width/2+3, height-5);
        XDrawLine(bbx->dpy, thumb->win, bbx->gc, width/2-3, 5, width/2-3, height-5);
        XSetForeground(bbx->dpy, bbx->gc, BBX_Color("dim gray"));  
       XDrawLine(bbx->dpy, thumb->win, bbx->gc, 0, 0, 0, height-1);   
       XDrawLine(bbx->dpy, thumb->win, bbx->gc, width-1, 0, width-1, height-1);   
     }
     XSetForeground(bbx->dpy, bbx->gc, BlackPixel(bbx->dpy, bbx->screen));  
     break;
  case ButtonPress:
    if(event->xbutton.button == 1 && thumb->ongrab == 0)
    {
      
      XGrabPointer(bbx->dpy, thumb->win, False,
		 ButtonPressMask |
                 ButtonReleaseMask |
                 Button1MotionMask 
		   // FocusChangeMask |
		   // EnterWindowMask |
		   //LeaveWindowMask,
		   ,
               GrabModeAsync,
               GrabModeAsync,
	       RootWindow(bbx->dpy, DefaultScreen(bbx->dpy)),
               None,
               CurrentTime);
      /*
      XGrabButton(bbx->dpy, Button1, AnyModifier, thumb->win, True, ButtonReleaseMask | Button1Motionmask, GrabModeAsync, GrabModeAsync, None, None); 
      */
      thumb->ongrab = 1;
      thumb->lastx = event->xbutton.x_root;
      thumb->lasty = event->xbutton.y_root;          
    } 
    break;
  case ButtonRelease:
    if(thumb->ongrab == 1)
    {
      XUngrabPointer(bbx->dpy, CurrentTime);
      //XUngrabButton(bbx->dpy, Button1, AnyModifier, thumb->win);
      thumb->ongrab = 0;
    }
    break;
  case MotionNotify:
    XGetGeometry(bbx->dpy, thumb->win, &root, &x, &y, &uwidth, 
		  &uheight, &border_width, &depth);
  
    if(thumb->direction == BBX_SCROLLBAR_VERTICAL)
    {
      int newy = y - thumb->lasty + event->xmotion.y_root;
      if(newy >= 0 && newy <= thumb->maxy)
      {
	XMoveWindow(bbx->dpy, thumb->win, 0, newy);
        (*thumb->fptr)(thumb->ptr, newy);
      } 
    }
    else if(thumb->direction == BBX_SCROLLBAR_HORIZONTAL)
    {
      int newx = x - thumb->lastx + event->xmotion.x_root;
      if(newx >= 0 && newx <= thumb->maxx)
      {
	XMoveWindow(bbx->dpy, thumb->win, newx, 0);
        (*thumb->fptr)(thumb->ptr, newx);
      }
    }
    thumb->lastx = event->xmotion.x_root;
    thumb->lasty = event->xmotion.y_root;
    break;

  }
}

static void dragged(void *obj, int pos)
{
  int newpos;
  double t;
  BBX_Scrollbar *bar = obj;
  
  if(bar->direction == BBX_SCROLLBAR_VERTICAL)
    t = ((double) pos) / bar->thumb->maxy;
  else if(bar->direction == BBX_SCROLLBAR_HORIZONTAL)
    t = ((double) pos) / bar->thumb->maxx;
  else
    return;
    
  newpos = (int) ( (bar->range - bar->visible) * t );
  if(newpos < 0)
     newpos = 0;
  else if(newpos > bar->range - bar->visible)
     newpos = bar->range - bar->visible;
 
  if(newpos != bar->pos)
  {
    bar->pos = newpos;
    if(bar->change)
      (*bar->change)(bar->ptr, bar->pos);
  }  
}
