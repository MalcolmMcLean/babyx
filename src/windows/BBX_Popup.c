#include <stdlib.h>
#include <string.h>
#include "BabyX.h"

BBX_Popup *bbx_popup(BABYX *bbx)
{

  BBX_Popup *answer;

  answer = bbx_malloc(sizeof(BBX_Popup));
  answer->bbx = bbx;
  answer->menu = 0;
  answer->N = 0;
 
  return answer;
}

int bbx_popup_append(BBX_Popup *pop, int id, char *left, char *right, BBX_Popup *sub)
{
  pop->menu = bbx_realloc(pop->menu, (pop->N +1) * sizeof(BBX_Popup_MenuItem));
  pop->menu[pop->N].id = id;
  pop->menu[pop->N].left = left ? bbx_strdup(left) : 0;
  pop->menu[pop->N].right = right ? bbx_strdup(right) : 0;
  pop->menu[pop->N].sub = sub;
  pop->menu[pop->N].disabled = 0;
  pop->N++;

  return 0;
}

void bbx_popup_kill(BBX_Popup *pop)
{
  int i;

  if(pop)
  {
    if(pop->menu)
    {
      for(i=0;i<pop->N;i++)
      {
        free(pop->menu[i].left);
        free(pop->menu[i].right);
        if(pop->menu[i].sub)
          bbx_popup_kill(pop->menu[i].sub);  
      }
      free(pop->menu);
    }
    free(pop);
  }
}

void bbx_popup_disable(BBX_Popup *pop, int id)
{
  int i;

  for(i=0;i<pop->N;i++)
  {
    if(pop->menu[i].id == id)
      pop->menu[i].disabled = 1;
    if(pop->menu[i].sub)
      bbx_popup_disable(pop->menu[i].sub, id);
  }
}


void bbx_popup_enable(BBX_Popup *pop, int id)
{
  int i;

  for(i=0;i<pop->N;i++)
  {
    if(pop->menu[i].id == id)
      pop->menu[i].disabled = 0;
    if(pop->menu[i].sub)
      bbx_popup_disable(pop->menu[i].sub, id);
  }
}

/*
static int getPopupDimensions(BBX_Popup *pop, int *width, int *height)
{
  int i;
  int maxwidth = 0;
  BABYX *bbx;
  int wd;
  bbx = pop->bbx;

  *height = (bbx->gui_font->ascent + bbx->gui_font->descent) * pop->N +10;
  for(i=0;i<pop->N;i++)
  {
    wd = 30;
    if(pop->menu[i].left)
      wd +=  XTextWidth(bbx->gui_font, pop->menu[i].left, strlen(pop->menu[i].left));
    if(pop->menu[i].right)
      wd +=  XTextWidth(bbx->gui_font, pop->menu[i].right, strlen(pop->menu[i].right));
    if(wd > maxwidth)
      maxwidth = wd;
  }
  *width = maxwidth;

  return 0;
} 
*/


typedef struct
{
  BABYX *bbx;
  HWND win;
  void (*event_handler)(void *obj);
  int (*message_handler)(void *obj, int message, int a, int b, void *params);
  /* private stuff */
  BBX_Popup *data;
  int sel;
  int answer;
} BBX_PopupWin;

/*
static BBX_PopupWin *bbx_popupwin(BABYX *bbx, Window parent, int x, int y, BBX_Popup *data);
static void bbx_popupwin_kill(BBX_PopupWin *obj);
static void event_handler(void *obj, XEvent *event);

int BBX_PopupPopup(BBX_Popup *obj, Window parent, int x, int y)
{
  BBX_PopupWin *pop;
  int answer;
  BABYX *bbx = obj->bbx;

  pop = bbx_popupwin(bbx, parent, x, y, obj);
  BBX_MakeModal(bbx, pop->win);
  answer = pop->answer;
  bbx_popupwin_kill(pop);
  return answer;  
}

static BBX_PopupWin *bbx_popupwin(BABYX *bbx, Window parent, int x, int y, BBX_Popup *data)
{
  BBX_PopupWin *answer;
  Window root;
  int width, height;
  Window child;
  XSetWindowAttributes swa;

  root = RootWindow(bbx->dpy, DefaultScreen(bbx->dpy));
  answer = bbx_malloc(sizeof(BBX_PopupWin));
  answer->bbx = bbx;

  getPopupDimensions(data, &width, &height);
  answer->win = XCreateSimpleWindow(bbx->dpy, parent, x, y, width, height,
				    1, BlackPixel(bbx->dpy, 0), BBX_Color("gray"));
  XTranslateCoordinates(bbx->dpy, parent, root, x, y, &x, &y, &child);
  swa.override_redirect = True;
  XChangeWindowAttributes(bbx->dpy, answer->win, CWOverrideRedirect, &swa);
  XReparentWindow(bbx->dpy, answer->win, root, x, y);
 XSelectInput(bbx->dpy, answer->win, ExposureMask | KeyPressMask | ButtonPressMask | PointerMotionMask | StructureNotifyMask);
 

  answer->event_handler = event_handler;
  answer->message_handler = 0;
  answer->data = data;
  answer->sel = -1;
  answer->answer = -1;

  BBX_Register(bbx, answer->win, event_handler, 0, answer);
  XMapWindow(bbx->dpy, answer->win);
  XGrabPointer(bbx->dpy, answer->win, False,
		 ButtonPressMask |
                 PointerMotionMask 
		   // FocusChangeMask |
		   // EnterWindowMask |
		   //LeaveWindowMask,
		   ,
               GrabModeAsync,
               GrabModeAsync,
	       RootWindow(bbx->dpy, DefaultScreen(bbx->dpy)),
               None,
               CurrentTime);
  return answer;  

}


static void bbx_popupwin_kill(BBX_PopupWin *obj)
{
  BBX_Deregister(obj->bbx, obj->win);
  XDestroyWindow(obj->bbx->dpy, obj->win);
  free(obj); 
}
 
static void event_handler(void *obj, XEvent *event)
{
  BBX_PopupWin *pop = obj;
  int msg_x, msg_y;
  int length;
  int font_height;
  int x, y;
  int width, height;
  BABYX *bbx;
  int i;
  char *left;
  char *right;
  BBX_Popup *sub;
  int disabled;
  
  bbx = pop->bbx;

  switch(event->type)
  {
  case Expose:
     XClearWindow(bbx->dpy, pop->win);
     bbx_getsize(bbx, pop, &width, &height);
     
     for(i=0;i<pop->data->N;i++)
     {
       left = pop->data->menu[i].left;      
       right = pop->data->menu[i].right;
       if(!left)
         left = "";
       if(!right)
         right = "";
        sub = pop->data->menu[i].sub;
        disabled = pop->data->menu[i].disabled;
        font_height = bbx->gui_font->ascent + bbx->gui_font->descent;
        msg_x  = 5;
        msg_y = (font_height)*i + bbx->gui_font->ascent + 5;
        if(i == pop->sel)
	{
	  XSetForeground(bbx->dpy, bbx->gc, BBX_Color("LightSteelBlue"));
          XFillRectangle(bbx->dpy, pop->win, bbx->gc, 0, i * font_height+5, width, font_height);
	  XSetForeground(bbx->dpy, bbx->gc, BBX_Color("white"));
	}
        else
          XSetForeground(bbx->dpy, bbx->gc, BlackPixel(bbx->dpy, bbx->screen));
       if(disabled)
          XSetForeground(bbx->dpy, bbx->gc, BBX_Color("light gray"));

        XDrawString(bbx->dpy, pop->win, bbx->gc, msg_x, msg_y,
		 left, strlen(left));

        length = XTextWidth(bbx->gui_font, right, strlen(right));
        msg_x = width - length - 20;
        XDrawString(bbx->dpy, pop->win, bbx->gc, msg_x, msg_y,
		 right, strlen(right));

	if(sub)
	{
            length = XTextWidth(bbx->gui_font, ">", 1);
            XDrawString(bbx->dpy, pop->win, bbx->gc, width-length-2, msg_y, ">", 1);     
        }

	XSetForeground(bbx->dpy, bbx->gc, BlackPixel(bbx->dpy, bbx->screen));
     }
     break;
  case MotionNotify:
    bbx_getsize(bbx, pop, &width, &height);
     x = event->xmotion.x;
     y = event->xmotion.y;
  
     if(x >= 0 && x < width && y >= 0 && y < height)
     {
       int index;
      
       index = (y-5)/(bbx->gui_font->ascent + bbx->gui_font->descent);

      
       if(index != pop->sel && 
          index < pop->data->N && 
          index >= 0 && 
          !pop->data->menu[index].disabled)
       {
         pop->sel = index;
         BBX_InvalidateWindow(bbx, pop->win);
       }
     }
     else
     {
       if(pop->sel != -1)
       {
         pop->sel = -1;
         BBX_InvalidateWindow(bbx, pop->win);
       }
     }
     
     break;
  case ButtonPress:
    bbx_getsize(bbx, pop, &width, &height);
    
    sub = 0;
    if(pop->sel != -1)
    {
      sub = pop->data->menu[pop->sel].sub;
      if(sub)
      {
        y = (bbx->gui_font->ascent + bbx->gui_font->descent)*pop->sel + 5;
	pop->answer = BBX_PopupPopup(sub, pop->win, width-1, y); 
      }
      else
        pop->answer = pop->data->menu[pop->sel].id;
    }
    XUngrabPointer(bbx->dpy, CurrentTime);
    BBX_DropModal(bbx);

    break;
  }

}

*/


typedef struct
{
  BABYX *bbx;
  HWND win;
  void (*event_handler)(void *obj);
  int (*message_handler)(void *obj, int message, int a, int b, void *params);
  /* private stuff */
  char **str;
  int N;
  int sel;
} BBX_QPopup;


/*
BBX_QPopup *bbx_qpopup(BABYX *bbx, Window parent, int x, int y, char **str, int N);
void bbx_qopopup_kill(BBX_QPopup *obj);
static void qpevent_handler(void *obj, XEvent *event);
static void getTextDimensions(XFontStruct *fs, char **str, int N, int *width, int *height);

int BBX_QuickPopup(BABYX *bbx, Window parent, int x, int y, char **str, int N)
{
  BBX_QPopup *qp;
  int answer;
  qp = bbx_qpopup(bbx, parent, x, y, str, N);
  BBX_MakeModal(bbx, qp->win);
  answer = qp->sel;
  bbx_qopopup_kill(qp);
  return answer;  
}

BBX_QPopup *bbx_qpopup(BABYX *bbx, Window parent, int x, int y, char **str, int N)
{
  BBX_QPopup *answer;
  Window root;
  int width, height;
  Window child;
  XSetWindowAttributes swa;

  root = RootWindow(bbx->dpy, DefaultScreen(bbx->dpy));
  answer = bbx_malloc(sizeof(BBX_QPopup));
  answer->bbx = bbx;

  getTextDimensions(bbx->gui_font, str, N, &width, &height);
  answer->win = XCreateSimpleWindow(bbx->dpy, parent, x, y, width, height,
				    1, BlackPixel(bbx->dpy, 0), WhitePixel(bbx->dpy, 0));
  XTranslateCoordinates(bbx->dpy, parent, root, x, y, &x, &y, &child);
  swa.override_redirect = True;
  XChangeWindowAttributes(bbx->dpy, answer->win, CWOverrideRedirect, &swa);
  XReparentWindow(bbx->dpy, answer->win, root, x, y);
 XSelectInput(bbx->dpy, answer->win, ExposureMask | KeyPressMask | ButtonPressMask | PointerMotionMask | StructureNotifyMask);
 

  answer->event_handler = qpevent_handler;
  answer->message_handler = 0;
  answer->str = str;
  answer->N = N;
  answer->sel = -1;

  BBX_Register(bbx, answer->win, qpevent_handler, 0, answer);
  XMapWindow(bbx->dpy, answer->win);
 XGrabPointer(bbx->dpy, answer->win, True,
		 ButtonPressMask |
                 ButtonReleaseMask |
                 PointerMotionMask 
		   // FocusChangeMask |
		   // EnterWindowMask |
		   //LeaveWindowMask,
		   ,
               GrabModeAsync,
               GrabModeAsync,
	       RootWindow(bbx->dpy, DefaultScreen(bbx->dpy)),
               None,
               CurrentTime);

 

  return answer;  
}  

void bbx_qopopup_kill(BBX_QPopup *obj)
{
  BBX_Deregister(obj->bbx, obj->win);
  XDestroyWindow(obj->bbx->dpy, obj->win);
  free(obj); 
}
 
static void qpevent_handler(void *obj, XEvent *event)
{
  BBX_QPopup *qp = obj;
  int msg_x, msg_y;
  int length;
  int font_height;
  int x, y, width, height;
  BABYX *bbx;
  int i;
  
  bbx = qp->bbx;

  switch(event->type)
  {
  case Expose:
     XClearWindow(bbx->dpy, qp->win);
     bbx_getsize(bbx, qp, &width, &height);
     
     font_height = bbx->gui_font->ascent + bbx->gui_font->descent; 
     for(i=0;i<qp->N;i++)
     {            
        length = XTextWidth(bbx->gui_font, qp->str[i], strlen(qp->str[i]));
        msg_x  = (width - length) / 2;
        msg_y = (bbx->gui_font->ascent + bbx->gui_font->descent)*i + bbx->gui_font->ascent + 5;
        if(i == qp->sel)
	{
          XSetForeground(bbx->dpy, bbx->gc, BBX_Color("blue"));
	  XFillRectangle(bbx->dpy, qp->win, bbx->gc, 0, i * font_height+5, width, font_height);
	  XSetForeground(bbx->dpy, bbx->gc, BBX_Color("white"));
        }
        XDrawString(bbx->dpy, qp->win, bbx->gc, msg_x, msg_y,
		 qp->str[i], strlen(qp->str[i]));
	XSetForeground(bbx->dpy, bbx->gc, BlackPixel(bbx->dpy, bbx->screen));
     }
     break;
  case MotionNotify:
     x = event->xmotion.x;
     y = event->xmotion.y;
     if(x >= 0 && x < width && y >= 0 && y < height)
     {
       int index;

       index = (y -5)/(bbx->gui_font->ascent + bbx->gui_font->descent);
       if(index != qp->sel && index < qp->N && index >= 0)
       {
         qp->sel = index;
         BBX_InvalidateWindow(bbx, qp->win);
       }
     }
     else
     {
       if(qp->sel != -1)
       {
         qp->sel = -1;
         BBX_InvalidateWindow(bbx, qp->win);
       }
     }
     
     break;
  case ButtonPress:
    //  case ButtonRelease:
    XUngrabPointer(bbx->dpy, CurrentTime);
    BBX_DropModal(bbx);
    break;

  }

}


static void getTextDimensions(XFontStruct *fs, char **str, int N, int *width, int *height)
 {
  int i;
  int w = 0;
  int h = 0;

  for(i=0;i<N;i++)
  {
    if(w <  XTextWidth(fs, str[i], strlen(str[i])))
      w =   XTextWidth(fs, str[i], strlen(str[i]));
  }
  h = (fs->ascent + fs->descent) * N;

  *width = w + 10;
  *height = h + 10;
 }
*/
