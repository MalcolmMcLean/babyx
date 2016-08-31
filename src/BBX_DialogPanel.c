#include <stdlib.h>
#include <string.h>

#include "BabyX.h"

static void event_handler(void *obj, XEvent *event);
static Window BBX_DialogWindow(BABYX *bbx, char *title, int width, int height);

BBX_DialogPanel *bbx_dialogpanel(BABYX *bbx, char *name, int width, int height, void (*changesize)(void *ptr, int width, int height), void *ptr)
{
  BBX_Panel *answer;

  answer = bbx_malloc(sizeof(BBX_Panel));
  answer->bbx = bbx;
  answer->win = BBX_DialogWindow(bbx, name, width, height);
  XSelectInput(bbx->dpy, answer->win, StructureNotifyMask);

  answer->event_handler = event_handler;
  answer->message_handler = 0;
  answer->changesize = changesize;
  answer->ptr = ptr;
  answer->tag = bbx_strdup("dialog");
  answer->width = -1;
  answer->height = -1;
  answer->mousefunc  =0;
  answer->closefunc = 0;
  answer->mousefunc = 0;

  BBX_Register(bbx, answer->win, event_handler, 0, answer);

  return answer;
}

void bbx_dialogpanel_kill(BBX_DialogPanel *pan)
{
  if(pan)
  {
    BBX_Deregister(pan->bbx, pan->win);
    XDestroyWindow(pan->bbx->dpy, pan->win);
    free(pan->tag);
    free(pan);
  }
}

void bbx_dialogpanel_makemodal(BBX_DialogPanel *pan)
{
  XMapWindow(pan->bbx->dpy, pan->win);
  BBX_MakeModal(pan->bbx, pan->win);
}

void bbx_dialogpanel_dropmodal(BBX_DialogPanel *pan)
{
  BBX_DropModal(pan->bbx);
}

void bbx_dialogpanel_setclosefunc(BBX_DialogPanel *pan, void (*closefunc)(void *ptr), void *ptr)
{
  pan->closefunc = closefunc;
  pan->ptr = ptr;
}

static void event_handler(void *obj, XEvent *event)
{
  BBX_Panel *pan = obj;
  int width, height;

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
  case ClientMessage:
    if(pan->closefunc)
      (*pan->closefunc)(pan->ptr);
    else
      exit(0);
    break;
 
  }
}

static Window BBX_DialogWindow(BABYX *bbx, char *title, int width, int height)
{
    Window       win;
    int          x, y;
    unsigned int display_width, display_height;
    int border_width = 1;
    Atom wmDelete;

      /*  Get screen size from display structure macro   */
    display_width  = DisplayWidth(bbx->dpy, bbx->screen);
    display_height = DisplayHeight(bbx->dpy, bbx->screen);


    /*  Set initial window size and position, and create it  */

    x = (display_width - width)/2;
    y = (display_height - height)/2;
   

    win = XCreateSimpleWindow(bbx->dpy, RootWindow(bbx->dpy, bbx->screen),
			      x, y, width, height, border_width,
			      BlackPixel(bbx->dpy, bbx->screen),
			      BBX_Color("Gray") );


    XStoreName(bbx->dpy, win, title);

    wmDelete=XInternAtom(bbx->dpy, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(bbx->dpy, win, &wmDelete, 1);


    return win;
}

