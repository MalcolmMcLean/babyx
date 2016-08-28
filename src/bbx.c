#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <Windows.h>

#include "BabyX.h"

static int subfiletime(FILETIME *t1, FILETIME *t2);
int specialkeytovkey(int code);
static int unicodetoVkey(int code);

/*
int BBX_Event(BABYX *bbx, XEvent *event)
{
  int i;

  for(i=0;i<bbx->Nchildren;i++)
    if(bbx->child[i].window == event->xany.window)
    {
     if(event->type == SelectionRequest)
       BBX_clipboard_handleselectionrequest(bbx->clipboard, &event->xselectionrequest);
      (*bbx->child[i].event_handler)(bbx->child[i].ptr, event);
      return 1;
    }

  return 0;
}
*/

void BBX_Tick(BABYX *bbx)
{
	BBX_TICKER *tickptr;
	FILETIME tock;

	/*
  struct timeval tock;
  BBX_TICKER *tickptr;
  int elapsed;
    
  gettimeofday(&tock, 0);
  for(tickptr = bbx->ticker; tickptr != 0; tickptr = tickptr->next)
  {
    if(tickptr->modelevel != bbx->modalpush)
      continue;
    elapsed = (tock.tv_sec - tickptr->tick.tv_sec) * 1000 +
	(tock.tv_usec - tickptr->tick.tv_usec)/1000;
    if(elapsed >= tickptr->interval)
    {
        (*tickptr->fptr)(tickptr->ptr);
        tickptr->tick = tock;
    }
  }
  */
	GetSystemTimeAsFileTime(&tock);
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

static int subfiletime(FILETIME *t1, FILETIME *t2)
{
	if (t1->dwHighDateTime == t2->dwHighDateTime)
		return (int)(t1->dwLowDateTime - t2->dwLowDateTime) / 10000;
	else
		return(int)(t1->dwLowDateTime + ~t2->dwLowDateTime + 1) / 10000;

}


int BBX_MakeModal(BABYX *bbx, HWND win)
{
	MSG msg;
	int modalpush;

	modalpush = bbx->modalpush;
	bbx->modalpush++;

	while (bbx->running && bbx->modalpush > modalpush)
	{
		while (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))
		{
			GetMessage(&msg, 0, 0, 0);
			if (win == msg.hwnd || IsChild(win, msg.hwnd) || msg.message == WM_PAINT)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		Sleep(1);
		BBX_Tick(bbx);
	}
			//if (msg.message == WM_DESTROY && msg.hwnd == root->win)
				//bbx->running = 0;
	/*
  int modalpush = bbx->modalpush;
  XEvent event;
  int motionflagged = 0;

  while(XCheckIfEvent(bbx->dpy, &event, alwaysmatch, 0))
     BBX_Event(bbx, &event);  

  bbx->modalpush++;
  while(bbx->modalpush > modalpush)
  {
    motionflagged = 0;
    while(XPending(bbx->dpy))
    {
      if(bbx->modalpush <= modalpush)
        break;
       XNextEvent(bbx->dpy, &event);
       if(event.type == MotionNotify)
       {
         if(motionflagged++)
           continue;
       }
      if(event.type == Expose || event.type == SelectionRequest || BBX_IsDescendant(bbx, win, event.xany.window))
      { 
        BBX_Event(bbx, &event);
      }
    }
    usleep(100);
    BBX_Tick(bbx);
  }
  */
    
  return 0;
}

int BBX_DropModal(BABYX *bbx)
{
  if(bbx->modalpush > 0)
    bbx->modalpush--;
  return 0;
}

int BBX_Register(BABYX *bbx, HWND win, void (*event_handler)(void *ptr), int (*message_handler)(void *ptr, int message, int a, int b, void *params), void *ptr)
{
  void *temp;

  if(bbx->childCapacity == bbx->Nchildren)
  {
    temp = realloc(bbx->child, (bbx->childCapacity * 2 +1) * sizeof(BBX_CHILD));
    if(!temp)
      return -1;
    bbx->child = temp;
    bbx->childCapacity *= 2;
    bbx->childCapacity += 1;
  } 
  bbx->child[bbx->Nchildren].window = win;
  bbx->child[bbx->Nchildren].event_handler = event_handler;
  bbx->child[bbx->Nchildren].message_handler = message_handler;
  bbx->child[bbx->Nchildren].ptr = ptr;
  bbx->Nchildren++;

  return 0; 
}

int BBX_Deregister(BABYX *bbx, HWND win)
{
  int i;

  for(i=0;i<bbx->Nchildren;i++)
    if(bbx->child[i].window == win)
      break;
  if(i == bbx->Nchildren)
    return -1;
  memmove(&bbx->child[i], &bbx->child[i+1], (bbx->Nchildren-i-1) *sizeof(BBX_CHILD));
  bbx->Nchildren--;
  return 0;
}

void *bbx_addticker(BABYX *bbx, int ms_interval, void (*fptr)(void *ptr), void *ptr)
{
  BBX_TICKER *tick;

  tick = bbx_malloc(sizeof(BBX_TICKER));
  tick->next = bbx->ticker;
  tick->interval = ms_interval;
  GetSystemTimeAsFileTime(&tick->tick);
  tick->modelevel = bbx->modalpush;
  tick->fptr = fptr;
  tick->ptr = ptr;
  bbx->ticker = tick;

  return tick;


}

void bbx_removeticker(BABYX *bbx, void *ticker)
{
  BBX_TICKER *tick;
  BBX_TICKER *prev = 0;

  if (!ticker)
	  return;
  for(tick = bbx->ticker; tick; tick=tick->next)
  {
    if(tick == ticker)
    {
      if(prev)
        prev->next = tick->next;
      else
        bbx->ticker = tick->next;
      free(ticker);
      break;
    }
    prev = tick;
  }
}

int bbx_setpos(BABYX *bbx, void *obj, int x, int y, int width, int height)
{
  int i;
  HWND win;
  static int called = 0;
  static HDWP hdwp;
  int top = 0;

  if (!called)
  {
	// hdwp = BeginDeferWindowPos(50);
	  top = 1;
  }
  called = 1;
  for(i=0;i<bbx->Nchildren;i++)
    if(bbx->child[i].ptr == obj)
    {
      win = bbx->child[i].window;
	// if (!IsWindowVisible(win))
			MoveWindow(win, x, y, width, height, TRUE);
	// else
	   // hdwp = DeferWindowPos(hdwp, win, HWND_TOP, x, y, width, height, SWP_SHOWWINDOW);
	  if (bbx->child[i].message_handler)
		  (*bbx->child[i].message_handler)(obj, BBX_RESIZE, width, height, 0);
	  ShowWindow(win, SW_SHOWNORMAL);
	  
	  UpdateWindow(win);
	
	  if (top)
	  {
		  called = 0;
		//  EndDeferWindowPos(hdwp);
	  }
      return 0;
    }

  if (top)
  {
	  EndDeferWindowPos(hdwp);
	  called = 0;
  }
  return -1;
}


int bbx_setsize(BABYX *bbx, void *obj, int width, int height)
{
  int i;
  HWND win;
  POINT pt;
  RECT rect;

  for(i=0;i<bbx->Nchildren;i++)
    if(bbx->child[i].ptr == obj)
    {
      win = bbx->child[i].window;
	  GetWindowRect(win, &rect);
	  pt.x = rect.left;
	  pt.y = rect.top;
	  ScreenToClient(GetParent(win), &pt);
	  MoveWindow(win, pt.x, pt.y, width, height, TRUE);
	  if (bbx->child[i].message_handler)
		  (*bbx->child[i].message_handler)(obj, BBX_RESIZE, width, height, 0);
      return 0;
    }
  return -1;
}

int bbx_getpos(BABYX *bbx, void *obj, int *x, int *y, int *width, int *height)
{
	int i;
	HWND win;
	HWND hparent;
	RECT rect;
	POINT pt;

	for (i = 0; i<bbx->Nchildren; i++)
		if (bbx->child[i].ptr == obj)
		{
		win = bbx->child[i].window;
		hparent = GetParent(win);
		POINT p = { 0 };

		MapWindowPoints(win, hparent, &p, 1);

		(*x) = p.x;
		(*y) = p.y;
		
		return 0;
		}
	return -1;
}

int bbx_getsize(BABYX *bbx, void *obj, int *width, int *height)
{
  int i;
  HWND win;
  int x, y; 
  RECT rect;

  for(i=0;i<bbx->Nchildren;i++)
    if(bbx->child[i].ptr == obj)
    {
      win = bbx->child[i].window;
	  GetClientRect(win, &rect);
	  *width = rect.right - rect.left;
	  *height = rect.bottom - rect.top;
      return 0;
    }
  return -1;
}

int bbx_setfocus(BABYX *bbx, void *obj)
{
	int i;
	HWND win;

	for (i = 0; i<bbx->Nchildren; i++)
		if (bbx->child[i].ptr == obj)
		{
			win = bbx->child[i].window;
			SetFocus(win);
			return 0;
		}
		
	return -1;
}

int BBX_IsDescendant(BABYX *bbx, HWND ancestor, HWND descendant)
{
	/*
  Window win;
  Window root;
  Window parent;
  Window *children;
  unsigned int Nchildren;
  int i;

  win = descendant;
  if(win == ancestor)
    return 1;
  for(i=0;i<bbx->Nchildren;i++)
    if(bbx->child[i].window == win)
      break;
  if(i == bbx->Nchildren)
    return 0;

  while(1)
  {
     XQueryTree(bbx->dpy, win, &root, &parent, &children, &Nchildren);
     if(children)
       XFree(children);     
     if(parent == ancestor)
       return 1;
     if(parent == root)
       return 0;
     win = parent;
  }
  */

	return 0;
   
}


void BBX_InvalidateWindow(BABYX *bbx, HWND win)
{
	InvalidateRect(win, 0, 0);
	UpdateWindow(win);
}



void *bbx_malloc(int size)
{
  void *answer;

  if(size == 0)
    size = 1;
  answer = malloc(size);
  if(!answer)
  {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }
  return answer;
}

void *bbx_realloc(void *ptr, int size)
{
  void *answer;

  if(size == 0)
    size = 1;
  answer = realloc(ptr, size);
  if(!answer)
  {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }

  return answer;
}

char *bbx_strdup(const char *str)
{
  char *answer;

  answer = bbx_malloc(strlen(str) + 1);
  strcpy(answer, str);
  return answer;
}

int bbx_kbhit(BABYX *bbx, int code)
{
	SHORT state;
	int vKey;

	if (code < 0)
		vKey = specialkeytovkey(code);
	else
		vKey = unicodetoVkey(code);
	state = GetAsyncKeyState(vKey);
	return (state & 0x8000) ? 1 : 0;
}

int specialkeytovkey(int code)
{
	switch (code)
	{
	case BBX_KEY_BACKSPACE: return VK_BACK;
	case BBX_KEY_DELETE: return VK_DELETE;
	case BBX_KEY_ESCAPE: return VK_ESCAPE;
	case BBX_KEY_HOME: return VK_HOME;
	case BBX_KEY_LEFT: return VK_LEFT;
	case BBX_KEY_UP: return VK_UP;
	case BBX_KEY_RIGHT: return VK_RIGHT;
	case BBX_KEY_DOWN: return VK_DOWN; 
	case BBX_KEY_END: return VK_END;
	}
	return 0;
}

static int unicodetoVkey(int code)
{
	if (isalnum(code))
		return toupper(code);
	switch (code)
	{
	case '\n': return VK_RETURN;
	case ' ': return VK_SPACE;
	case '\t': return VK_TAB;
	case '*': return VK_MULTIPLY;
	case '+': return VK_ADD;
	case '/': return VK_DIVIDE;
	case '-': return VK_SUBTRACT;
	default: return 0;
	}
}