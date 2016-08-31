#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <X11/Xlib.h>

#include "BabyX.h"

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

void BBX_Tick(BABYX *bbx)
{
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

}

static Bool alwaysmatch(Display *dpy, XEvent *event, XPointer arg)
{
  return True;
}

int BBX_MakeModal(BABYX *bbx, Window win)
{
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
    
  return 0;
}

int BBX_DropModal(BABYX *bbx)
{
  if(bbx->modalpush > 0)
    bbx->modalpush--;
  return 0;
}

int BBX_Register(BABYX *bbx, Window win, void (*event_handler)(void *ptr, XEvent *event), int (*message_handler)(void *ptr, int message, int a, int b, void *params), void *ptr)
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

int BBX_Deregister(BABYX *bbx, Window win)
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
  gettimeofday(&tick->tick, 0);
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
  Window win;

  for(i=0;i<bbx->Nchildren;i++)
    if(bbx->child[i].ptr == obj)
    {
      win = bbx->child[i].window;
      XUnmapWindow(bbx->dpy, win);
      XMoveWindow(bbx->dpy, win, x, y);
      XResizeWindow(bbx->dpy, win, width, height); 
      XMapWindow(bbx->dpy, win);
      return 0;
    }
  return -1;
}


int bbx_setsize(BABYX *bbx, void *obj, int width, int height)
{
  int i;
  Window win;

  for(i=0;i<bbx->Nchildren;i++)
    if(bbx->child[i].ptr == obj)
    {
      win = bbx->child[i].window;
      XUnmapWindow(bbx->dpy, win);
      XResizeWindow(bbx->dpy, win, width, height); 
      XMapWindow(bbx->dpy, win);
      return 0;
    }
  return -1;
}

int bbx_getsize(BABYX *bbx, void *obj, int *width, int *height)
{
  int i;
  Window win;
  Window root;
  int x, y; 
  unsigned int uwidth, uheight, border_width, depth;

  for(i=0;i<bbx->Nchildren;i++)
    if(bbx->child[i].ptr == obj)
    {
      win = bbx->child[i].window;
      XGetGeometry(bbx->dpy, win, &root, &x, &y, &uwidth, 
		  &uheight, &border_width, &depth);
      *width = (int) uwidth;
      *height = (int) uheight;      
      return 0;
    }
  return -1;
}

int BBX_IsDescendant(BABYX *bbx, Window ancestor, Window descendant)
{
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
   
}


void BBX_InvalidateWindow(BABYX *bbx, Window win)
{
  XEvent event;

  event.xany.type = Expose; 
  event.xany.display = bbx->dpy;
  event.xany.window = win;
  event.xexpose.x  = 0;
  event.xexpose.y = 0;
  event.xexpose.width  = 1; 
  event.xexpose.height = 1; 
  event.xexpose.count = 0;
  
  XSendEvent(bbx->dpy, win, 0, 0, &event);
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
