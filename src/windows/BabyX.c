#include <Windows.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

extern struct bitmap_font vera12_font;
extern struct bitmap_font fixed_font;
#include "BabyX.h"

static int subfiletime(FILETIME *t1, FILETIME *t2);

BABYX *BabyX(HINSTANCE hinstance)
{
  BABYX *answer;
  HWND hwnd = GetConsoleWindow();
  ShowWindow(hwnd, 0);


  answer = malloc(sizeof(BABYX));
  answer->hinstance = hinstance;
  answer->child = 0;
  answer->Nchildren = 0;
  answer->childCapacity = 0;
  //answer->screen = DefaultScreen(dpy);
  //answer->gc = XCreateGC(dpy, RootWindow(dpy, 0), 0, 0);
  answer->gui_font = &vera12_font;
  answer->user_font2 = &fixed_font;
  // answer->gui_font  = XLoadQueryFont(dpy, "-*-clean-bold-r-*-*-14-*-*-*-*-*-*-*");
  //answer->gui_font  = XLoadQueryFont(dpy, "-sun-open look glyph-----14-140-75-75-p-128-sunolglyph-1");
  //answer->gui_font  = XLoadQueryFont(dpy,  "-schumacher-clean-medium-r-normal-*-12-*-*-*-*-*-*-*");
  //answer->user_font = XLoadQueryFont(dpy, "9x15");
  answer->modalpush = 0;
  answer->running = 0;
  answer->ticker = 0;
  answer->clipboard = 0;

  // XSetFont(dpy, answer->gc, answer->gui_font->fid);
  //XSetForeground(dpy, answer->gc, BlackPixel(dpy, 0));

  BBX_RegisterDialogPanel(answer->hinstance);
  BBX_RegisterPopupPanel(answer->hinstance);
  BBX_RegisterPanel(answer->hinstance);
  BBX_RegisterCanvas(answer->hinstance);
  BBX_RegisterLabel(answer->hinstance);
  BBX_RegisterScrollbar(answer->hinstance);
  answer->clipboard = 0;


  return answer;
}

void killbabyx(BABYX *bbx)
{
  if(bbx)
  {
    BBX_clipboard_kill(bbx->clipboard);
    free(bbx->child);
    // XFreeFont(bbx->dpy, bbx->gui_font);
    //XFreeFont(bbx->dpy, bbx->user_font);
    //XFreeGC(bbx->dpy, bbx->gc);
    free(bbx);
  }
}

void startbabyx(
				char *name, 
                int width, 
                int height,
		void (*create)(void *ptr, BABYX *bbx, BBX_Panel *root),
                void (*layout)(void *ptr, int width, int height),
                void *ptr)
{
  BBX_DialogPanel *root;
  BABYX *bbx;
//  XEvent event;
  MSG msg;
  FILETIME tick;
  FILETIME tock;
  BBX_TICKER *tickptr;
  int motionflagged;
  HINSTANCE hInstance;
   
  hInstance = GetModuleHandle(0);

 //if ( (dpy = XOpenDisplay(NULL)) == NULL ) {
	//exit(EXIT_FAILURE);
    //}

  bbx = BabyX(hInstance);
  root = bbx_dialogpanel(bbx, name, width, height, layout, ptr);
  bbx->clipboard = BBX_clipboard(root->win);
  (*create)(ptr, bbx, root);
  ShowWindow(root->win, TRUE);
  UpdateWindow(root->win);
 
  bbx->running = 1;

  GetSystemTimeAsFileTime(&tick);
  while ( bbx->running ) 
  {
    motionflagged = 0;
    while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
		//GetMessage(&msg, 0, 0, 0);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_DESTROY && msg.hwnd == root->win)
			bbx->running = 0;

		
       //if(event.type == DestroyNotify && event.xdestroywindow.window == root->win)
      // {
      //   bbx->running = 0;
      // }
      // if(event.type == MotionNotify)
      // {
      //   if(motionflagged++)
      //     continue;
      // }
      
     //  BBX_Event(bbx, &event);
     }
     
	Sleep(1);
   // usleep(100);
   // gettimeofday(&tock, 0);
	GetSystemTimeAsFileTime(&tock);
	if (subfiletime(&tock, &tick) > 1)
	{
		tick = tock;
		for (tickptr = bbx->ticker; tickptr != 0; tickptr = tickptr->next)
		{
			int elapsed = subfiletime(&tock, &tickptr->tick);
			if (elapsed >= tickptr->interval)
			{
				(*tickptr->fptr)(tickptr->ptr);
				tickptr->tick = tock;
			}
		}
	}
	
  }
  bbx_dialogpanel_kill(root);
  killbabyx(bbx);

}

void stopbabyx(BABYX *bbx)
{
  bbx->running = 0;
}

static int subfiletime(FILETIME *t1, FILETIME *t2)
{
	if (t1->dwHighDateTime == t2->dwHighDateTime)
		return (int)(t1->dwLowDateTime - t2->dwLowDateTime)/10000;
	else
		return(int)(t1->dwLowDateTime + ~t2->dwLowDateTime +1 )/10000;

}