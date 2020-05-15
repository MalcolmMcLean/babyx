#include <stdlib.h>
#include <string.h>

#include <X11/keysym.h>
#include <X11/Xutil.h>
#include "BabyX.h"

static void event_handler(void *obj, XEvent *event);
static int getunicode(XEvent *event);
static int specialkey(int ks);

BBX_Panel *bbx_panel(BABYX *bbx, BBX_Panel *parent, char *tag, void (*changesize)(void *ptr, int width, int height), void *ptr)
{
  return BBX_panel(bbx, parent->win, tag, changesize, ptr);
}

BBX_Panel *BBX_panel(BABYX *bbx, Window parent, char *tag, void (*changesize)(void *ptr, int width, int height), void *ptr)
{
  BBX_Panel *answer;
  unsigned long grey;

  answer = bbx_malloc(sizeof(BBX_Panel));
  answer->bbx = bbx;
  grey = BBX_Color("gray");
  answer->win = XCreateSimpleWindow(bbx->dpy, parent, 0, 0, 100, 100,
				    0, BlackPixel(bbx->dpy, 0), grey);
  XSelectInput(bbx->dpy, answer->win, StructureNotifyMask);

  answer->event_handler = event_handler;
  answer->message_handler = 0;
  answer->changesize = changesize;
  answer->ptr = ptr;
  answer->tag = bbx_strdup(tag);
  answer->width = -1;
  answer->height = -1;
  answer->mousefunc = 0;
  answer->closefunc = 0;
  answer->keyfunc = 0;

  BBX_Register(bbx, answer->win, event_handler, 0, answer);

  return answer;  
}

void bbx_panel_kill(BBX_Panel *pan)
{
  if(pan)
  {
    free(pan->tag);
    BBX_Deregister(pan->bbx, pan->win);
    XDestroyWindow(pan->bbx->dpy, pan->win);
    free(pan);
  }
}

char *bbx_panel_gettag(BBX_Panel *pan)
{
  return pan->tag;
}

void bbx_panel_setbackground(BBX_Panel *obj, BBX_RGBA col)
{
  XSetWindowAttributes swa;
  
  swa.background_pixel = BBX_RgbaToX(col);
  XChangeWindowAttributes(obj->bbx->dpy, obj->win, CWBackPixel, &swa);
}

void bbx_panel_setmousefunc(BBX_Panel *pan, void (*mousefunc)(void *ptr, int action, int x, int y, int buttons), void *ptr)
{
  XSetWindowAttributes swa;
  long mask;
  
  pan->mousefunc = mousefunc;
  pan->ptr = ptr;
  mask = StructureNotifyMask;
  if(pan->mousefunc)
    mask |= (ButtonPressMask | ButtonReleaseMask | PointerMotionMask);  
  if(pan->keyfunc)
    mask |=  (KeyPressMask | FocusChangeMask); 

  swa.event_mask = mask;

  XChangeWindowAttributes(pan->bbx->dpy, pan->win, CWEventMask, &swa);  
}

void bbx_panel_setkeyfunc(BBX_Panel *pan, void (*keyfunc)(void *ptr, int ch), void *ptr)
{
    XSetWindowAttributes swa;
  long mask;
  
  pan->keyfunc = keyfunc;
  pan->ptr = ptr;
  mask = StructureNotifyMask;
  if(pan->mousefunc)
    mask |= (ButtonPressMask | ButtonReleaseMask | PointerMotionMask);  
  if(pan->keyfunc)
    mask |=  (KeyPressMask | FocusChangeMask); 

  swa.event_mask = mask;

  XChangeWindowAttributes(pan->bbx->dpy, pan->win, CWEventMask, &swa);  
}

void *bbx_panel_getptr(BBX_Panel *pan)
{
  return pan->ptr;
}

int bbx_panel_gotmouse(BBX_Panel *pan, int *x, int *y)
{
    Window root_return;
    Window child_return;
    int root_x_return, root_y_return;
    int win_x_return, win_y_return;
    unsigned int mask_return;
    int width, height;

    bbx_getsize(pan->bbx, pan, &width, &height);
    XQueryPointer(pan->bbx->dpy, pan->win, &root_return, &child_return, 
                     &root_x_return, &root_y_return, 
		 &win_x_return, &win_y_return, &mask_return);
    if(win_x_return >= 0 && win_x_return < width && win_y_return >= 0 &&
       win_y_return < height)
    {
      if(x)
        *x = win_x_return;
      if(y)
        *y = win_y_return;
      return 1;
    }
    else
    {
      if(x)
	*x = -1;
      if(y)
        *y = -1;
      return 0;
    }
}

static void event_handler(void *obj, XEvent *event)
{
  BBX_Panel *pan = obj;
  int width, height;
  int button;
  int x, y;
  int ch;

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
    if(pan->keyfunc)
       XSetInputFocus(pan->bbx->dpy, pan->win, RevertToParent, CurrentTime);
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
  case KeyPress:   
    if(pan->keyfunc)
    {
      ch = getunicode(event);
      (*pan->keyfunc)(pan->ptr, ch);
    }
    break;
  case FocusIn:
    if(pan->keyfunc)
      (*pan->keyfunc)(pan->ptr, BBX_KEY_GOTFOCUS);
    break;
  case FocusOut:
    if(pan->keyfunc)
       (*pan->keyfunc)(pan->ptr, BBX_KEY_LOSTFOCUS);
    break;
    }

}

static int getunicode(XEvent *event)
{
  KeySym ks;
  int len;
  int ch = -1;
  char buff[32];

  ks = XLookupKeysym(&event->xkey, 0);
  ch = specialkey(ks);
  if(!ch)
  {
    len = XLookupString(&event->xkey, buff, 32, 0, 0);
    if(len)
      ch = buff[0];
    else
      ch = -1;
  }

  return ch;
}

static int specialkey(int ks)
{
  switch(ks)
  {
    case XK_BackSpace: return BBX_KEY_BACKSPACE;
    case XK_Delete: return BBX_KEY_DELETE;
    case XK_Escape: return BBX_KEY_ESCAPE;
    case XK_Home: return BBX_KEY_HOME;
    case XK_Left: return BBX_KEY_LEFT;    
    case XK_Up: return BBX_KEY_UP;                    
    case XK_Right: return BBX_KEY_RIGHT;       
    case XK_Down: return BBX_KEY_DOWN;             
    case XK_End: return BBX_KEY_END;
    case XK_Return: return '\n';
    

     
      //  #define XK_Shift_L      0xFFE1  /* Left shift */
      //#define XK_Shift_R      0xFFE2  /* Right shift */
      //#define XK_Control_L        0xFFE3  /* Left control */
      //#define XK_Control_R        0xFFE4  /* Right control */
      // #define XK_Caps_Lock        0xFFE5  /* Caps lock */
      //#define XK_Shift_Lock       0xFFE6  /* Shift lock */
       

 default: return 0;
      
  }
}
