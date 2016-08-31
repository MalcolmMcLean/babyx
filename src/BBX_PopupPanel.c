#include "BabyX.h"


BBX_Panel *BBX_popuppanel(BABYX *bbx, Window parent, char *tag, void (*changesize)(void *ptr, int width, int height), void *ptr, int x, int y);

static void event_handler(void *obj, XEvent *event);


BBX_Panel *bbx_popuppanel(BABYX *bbx, BBX_Panel *parent, char *tag, void (*changesize)(void *ptr, int width, int height), void *ptr, int x, int y)
{
  return BBX_popuppanel(bbx, parent->win, tag, changesize, ptr, x, y);
}

BBX_Panel *BBX_popuppanel(BABYX *bbx, Window parent, char *tag, void (*changesize)(void *ptr, int width, int height), void *ptr, int x, int y)
{
  BBX_Panel *answer;
  unsigned long grey, dimgrey;
  Window root;
  Window child;
  XSetWindowAttributes swa;

  root = RootWindow(bbx->dpy, DefaultScreen(bbx->dpy));
  answer = bbx_malloc(sizeof(BBX_Panel));
  answer->bbx = bbx;
  grey = BBX_Color("gray");
  dimgrey = BBX_Color("dim gray");
  answer->win = XCreateSimpleWindow(bbx->dpy, parent, 0, 0, 100, 100,
				    1, dimgrey, grey);
  XSelectInput(bbx->dpy, answer->win, StructureNotifyMask);
  XTranslateCoordinates(bbx->dpy, parent, root, x, y, &x, &y, &child);
  swa.override_redirect = True;
  XChangeWindowAttributes(bbx->dpy, answer->win, CWOverrideRedirect, &swa);
  XReparentWindow(bbx->dpy, answer->win, root, x, y);
  XSelectInput(bbx->dpy, answer->win, ExposureMask | KeyPressMask | ButtonPressMask | PointerMotionMask | StructureNotifyMask);


  answer->event_handler = event_handler;
  answer->message_handler = 0;
  answer->changesize = changesize;
  answer->ptr = ptr;
  answer->tag = bbx_strdup(tag);
  answer->width = -1;
  answer->height = -1;
  answer->mousefunc = 0;
  answer->keyfunc = 0;
  answer->closefunc = 0;

  BBX_Register(bbx, answer->win, event_handler, 0, answer);

  return answer;  
}

void bbx_popuppanel_makemodal(BBX_DialogPanel *pan)
{
  XMapWindow(pan->bbx->dpy, pan->win);
  BBX_MakeModal(pan->bbx, pan->win);
}

void bbx_popuppanel_dropmodal(BBX_DialogPanel *pan)
{
  BBX_DropModal(pan->bbx);
}

static void event_handler(void *obj, XEvent *event)
{
  BBX_Panel *pan = obj;
  int width, height;
  int button;
  int x, y;

  switch(event->type)
  {
  case ConfigureNotify:
    width = event->xconfigure.width;
    height = event->xconfigure.height;
    if(pan->changesize)
    {
      if(height != pan->height || width != pan->width)
        (*pan->changesize)(pan->ptr, width, height);
      pan->height = height;
      pan->width = width;
    }
    break;
   
  case ButtonPress:
    button = 0;
    if(event->xbutton.button == Button1)
      button = BBX_MOUSE_BUTTON1;
    else if(event->xbutton.button == Button2)
      button = BBX_MOUSE_BUTTON2;
    else if(event->xbutton.button == Button3)
      button = BBX_MOUSE_BUTTON3;
    x = event->xmotion.x;
    y = event->xmotion.y;
    if(pan->mousefunc)
      (*pan->mousefunc)(pan->ptr, BBX_MOUSE_CLICK, x, y, button); 
    break;
  case ButtonRelease:
    button = 0;
    if(event->xbutton.button == Button1)
      button = BBX_MOUSE_BUTTON1;
    else if(event->xbutton.button == Button2)
      button = BBX_MOUSE_BUTTON2;
    else if(event->xbutton.button == Button3)
      button = BBX_MOUSE_BUTTON3;
  
    x = event->xmotion.x;
    y = event->xmotion.y;
    if(pan->mousefunc)
      (*pan->mousefunc)(pan->ptr, BBX_MOUSE_RELEASE, x, y, button);
  
    break;
  case MotionNotify:
    button = 0;
    if(event->xmotion.state & Button1Mask)
       button |= BBX_MOUSE_BUTTON1;
    if(event->xmotion.state & Button2Mask)
       button |= BBX_MOUSE_BUTTON2;
    if(event->xmotion.state & Button3Mask)
       button |= BBX_MOUSE_BUTTON3;
    x = event->xmotion.x;
    y = event->xmotion.y;
    if(pan->mousefunc)
      (*pan->mousefunc)(pan->ptr, BBX_MOUSE_MOVE, x, y, button);
    break;
   
  }

}
