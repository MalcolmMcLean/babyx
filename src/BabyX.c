#include <stdlib.h>
#include <time.h>
#include <unistd.h>

extern struct bitmap_font vera12_font;
extern struct bitmap_font fixed_font;
#include "BabyX.h"

BABYX *BabyX(Display *dpy)
{
  BABYX *answer;

  answer = malloc(sizeof(BABYX));
  answer->dpy = dpy;
  answer->child = 0;
  answer->Nchildren = 0;
  answer->childCapacity = 0;
  answer->screen = DefaultScreen(dpy);
  answer->gc = XCreateGC(dpy, RootWindow(dpy, 0), 0, 0);
  answer->gui_font = &vera12_font;
  answer->user_font2 = &fixed_font;
  // answer->gui_font  = XLoadQueryFont(dpy, "-*-clean-bold-r-*-*-14-*-*-*-*-*-*-*");
  //answer->gui_font  = XLoadQueryFont(dpy, "-sun-open look glyph-----14-140-75-75-p-128-sunolglyph-1");
  //answer->gui_font  = XLoadQueryFont(dpy,  "-schumacher-clean-medium-r-normal-*-12-*-*-*-*-*-*-*");
  answer->user_font = XLoadQueryFont(dpy, "9x15");
  answer->modalpush = 0;
  answer->running = 0;
  answer->ticker = 0;
  answer->clipboard = 0;

  // XSetFont(dpy, answer->gc, answer->gui_font->fid);
  XSetForeground(dpy, answer->gc, BlackPixel(dpy, 0));

  return answer;
}

void killbabyx(BABYX *bbx)
{
  if(bbx)
  {
    BBX_clipboard_kill(bbx->clipboard);
    free(bbx->child);
    // XFreeFont(bbx->dpy, bbx->gui_font);
    XFreeFont(bbx->dpy, bbx->user_font);
    XFreeGC(bbx->dpy, bbx->gc);
    free(bbx);
  }
}

void startbabyx(char *name, 
                int width, 
                int height,
		void (*create)(void *ptr, BABYX *bbx, BBX_Panel *root),
                void (*layout)(void *ptr, int width, int height),
                void *ptr)
{

  Display *dpy;
  BBX_DialogPanel *root;
  BABYX *bbx;
  XEvent event;
  struct timeval tock;
  BBX_TICKER *tickptr;
  int motionflagged;

 if ( (dpy = XOpenDisplay(NULL)) == NULL ) {
	exit(EXIT_FAILURE);
    }

  bbx = BabyX(dpy);
  root = bbx_dialogpanel(bbx, name, width, height, layout, ptr);
  bbx->clipboard = BBX_clipboard(dpy, root->win);
  XMapWindow(dpy, root->win);
  (*create)(ptr, bbx, root);

  bbx->running = 1;
  while ( bbx->running ) 
  {
    motionflagged = 0;
    while(XPending(dpy))
    {
       XNextEvent(dpy, &event);
       if(event.type == DestroyNotify && event.xdestroywindow.window == root->win)
       {
         bbx->running = 0;
       }
       if(event.type == MotionNotify)
       {
         if(motionflagged++)
           continue;
       }
      
       BBX_Event(bbx, &event);
     }
      
    usleep(100);
    gettimeofday(&tock, 0);
    for(tickptr = bbx->ticker; tickptr != 0; tickptr = tickptr->next)
    {
      int elapsed = (tock.tv_sec - tickptr->tick.tv_sec) * 1000 +
	(tock.tv_usec - tickptr->tick.tv_usec)/1000;
      if(elapsed >= tickptr->interval)
      {
        (*tickptr->fptr)(tickptr->ptr);
        tickptr->tick = tock;
      }
    }
  }
  bbx_dialogpanel_kill(root);
  killbabyx(bbx);
  XCloseDisplay(dpy); 
}

void stopbabyx(BABYX *bbx)
{
  bbx->running = 0;
}
