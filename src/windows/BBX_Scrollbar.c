#include <stdlib.h>

#include "BabyX.h"


static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void PaintMe(BBX_Scrollbar *sb);
static int message_handler(void *obj, int message, int a, int b, void *params);
static void setthumbsize(BBX_Scrollbar *obj);
static LRESULT CALLBACK ThumbWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void PaintThumb(HWND hwnd, BBX_ScrollThumb *thumb);
static void GetWindowPos(HWND hWnd, int *x, int *y);
static void dragged(void *obj, int pos);

ATOM BBX_RegisterScrollbar(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	BBX_RGBA grey;

	grey = bbx_color("gray");

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = 0; //LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINHELLO4));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH) CreateSolidBrush(RGB(bbx_red(grey), bbx_green(grey), bbx_blue(grey)));
	wcex.lpszMenuName = 0; //MAKEINTRESOURCE(IDC_WINHELLO4);
	wcex.lpszClassName = "BBXScrollbar"; //szWindowClass;
	wcex.hIconSm = 0; //LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassEx(&wcex);

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = ThumbWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = 0; //LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINHELLO4));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = 0; //MAKEINTRESOURCE(IDC_WINHELLO4);
	wcex.lpszClassName = "BBXThumb"; //szWindowClass;
	wcex.hIconSm = 0; //LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

BBX_Scrollbar *bbx_scrollbar(BABYX *bbx, BBX_Panel *parent, int direction, void (*fptr)(void *ptr, int pos), void *ptr)
{
  return BBX_scrollbar(bbx, parent->win, direction, fptr, ptr);
}

BBX_Scrollbar *BBX_scrollbar(BABYX *bbx, HWND parent, int direction, void (*fptr)(void *ptr, int pos), void *ptr)
{
  BBX_Scrollbar *answer;

  answer = bbx_malloc(sizeof(BBX_Scrollbar));
  answer->bbx = bbx;


  answer->event_handler = 0;
  answer->message_handler = message_handler;
 
  answer->direction = direction;
  answer->change = fptr;
  answer->ptr = ptr;
  answer->range = 100;
  answer->visible = 1;
  answer->pos = 0;
  answer->win = CreateWindow("BBXScrollbar", "", WS_CHILD | WS_VISIBLE, 0, 0, 20, 100, parent, 0, bbx->hinstance, answer);

  answer->thumb = bbx_scrollthumb(bbx, answer->win, direction, dragged, answer);

  BBX_Register(bbx, answer->win, 0, message_handler, answer);

  return answer;  
}

void bbx_scrollbar_kill(BBX_Scrollbar *obj)
{
  if(obj)
  {
    bbx_scrollthumb_kill(obj->thumb);
    BBX_Deregister(obj->bbx, obj->win);
    DestroyWindow(obj->win);
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
    MoveWindow(obj->thumb->win, 0, y, obj->thumbwidth, obj->thumbheight, FALSE);
  }
  else if(obj->direction == BBX_SCROLLBAR_HORIZONTAL)
  {
    x = (int) (t * obj->thumb->maxx);
    MoveWindow(obj->thumb->win, x, 0, obj->thumbwidth, obj->thumbheight, FALSE);
  }
}


static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BBX_Scrollbar *sb;

	sb = (BBX_Scrollbar *)GetWindowLongPtr(hwnd, GWL_USERDATA);
	switch (msg)
	{
	case WM_CREATE:
		sb = (BBX_Scrollbar *)((CREATESTRUCT *)lParam)->lpCreateParams;
		SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG)((CREATESTRUCT *)lParam)->lpCreateParams);
		sb->win = hwnd;
		break;
	case WM_PAINT:
		PaintMe(sb);
		break;
	
		/*
	case WM_LBUTTONDOWN:
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		if (pan->mousefunc)
			(*pan->mousefunc)(pan->ptr, BBX_MOUSE_CLICK, x, y, BBX_MOUSE_BUTTON1);
		break;
	case WM_LBUTTONUP:
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		if (pan->mousefunc)
			(*pan->mousefunc)(pan->ptr, BBX_MOUSE_RELEASE, x, y, BBX_MOUSE_BUTTON1);
		break;
	case WM_MOUSEMOVE:
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		button = 0;
		if (wParam & MK_LBUTTON)
			button |= BBX_MOUSE_BUTTON1;
		if (wParam & MK_RBUTTON)
			button |= BBX_MOUSE_BUTTON2;
		if (wParam & MK_MBUTTON)
			button |= BBX_MOUSE_BUTTON3;
		if (pan->mousefunc)
			(*pan->mousefunc)(pan->ptr, BBX_MOUSE_MOVE, x, y, button);
		break;
		*/
	
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

static void PaintMe(BBX_Scrollbar *sb)
{
	PAINTSTRUCT ps;
	HDC hdc;
	RECT rect;
	BBX_RGBA higrey;
	BBX_RGBA dimgrey;
	HPEN hpenhi;
	HPEN hpenlo;
	HPEN hpenold;

	hdc = BeginPaint(sb->win, &ps);
	GetClientRect(sb->win, &rect);
	higrey = bbx_color("light gray");
	dimgrey = bbx_color("dim gray");
	hpenlo = CreatePen(PS_SOLID, 1, RGB(bbx_red(dimgrey), bbx_green(dimgrey), bbx_blue(dimgrey)));
	hpenhi = CreatePen(PS_SOLID, 1, RGB(bbx_red(higrey), bbx_green(higrey), bbx_blue(higrey)));
	MoveToEx(hdc, 0, 0, 0);
	
	hpenold = SelectObject(hdc, hpenlo);
	LineTo(hdc, rect.right - 1, 0);
	SelectObject(hdc, hpenhi);
	LineTo(hdc, rect.right - 1, rect.bottom - 1);
	LineTo(hdc, 0, rect.bottom - 1);
	SelectObject(hdc, hpenlo);
	LineTo(hdc, 0, 0);
	SelectObject(hdc, hpenold);
	DeleteObject(hpenhi);
	DeleteObject(hpenlo);

	EndPaint(sb->win, &ps);
}

static int message_handler(void *obj, int message, int a, int b, void *params)
{
	BBX_Scrollbar *bar = obj;
	switch (message)
	{
	case BBX_RESIZE:
		ShowWindow(bar->win, SW_NORMAL);
		setthumbsize(bar);
		bbx_scrollbar_setpos(bar, bar->pos);
		break;
	}
	return 0;
}
/*
static void event_handler(void *obj)
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
*/

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
  obj->thumbwidth = thumbwidth;
  obj->thumbheight = thumbheight;
  obj->thumb->width = thumbwidth;
  obj->thumb->height = thumbheight;
  MoveWindow(obj->thumb->win, 0, 0, obj->thumbwidth, obj->thumbheight, TRUE);
}

BBX_ScrollThumb *bbx_scrollthumb(BABYX *bbx, HWND parent, int direction, void (*fptr)(void *ptr, int pos), void *ptr)
{
  BBX_ScrollThumb *answer;

  answer = bbx_malloc(sizeof(BBX_ScrollThumb));
  answer->bbx = bbx;
 
  answer->event_handler = 0;
  answer->message_handler = 0;
  answer->fptr = fptr;
  answer->ptr = ptr;
  answer->direction = direction;
  answer->lastx = -1;
  answer->lasty = -1;
  answer->ongrab = 0;
  answer->win = CreateWindow("BBXThumb", "", WS_CHILD | WS_VISIBLE, 0, 0, 18, 18, parent, 0, bbx->hinstance, answer);

  BBX_Register(bbx, answer->win, 0, 0, answer);
  return answer;  
}

void bbx_scrollthumb_kill(BBX_ScrollThumb *obj)
{
  if(obj)
  {
    BBX_Deregister(obj->bbx, obj->win);
    DestroyWindow(obj->win);
    free(obj);
  }

}


static LRESULT CALLBACK ThumbWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BBX_ScrollThumb *thumb;
	int x, y;

	thumb = (BBX_ScrollThumb *)GetWindowLongPtr(hwnd, GWL_USERDATA);
	switch (msg)
	{
	case WM_CREATE:
		thumb = (BBX_ScrollThumb *)((CREATESTRUCT *)lParam)->lpCreateParams;
		SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG)((CREATESTRUCT *)lParam)->lpCreateParams);
		thumb->win = hwnd;
		break;
	case WM_PAINT:
		PaintThumb(hwnd, thumb);
		break;
	case WM_LBUTTONDOWN:
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		SetCapture(hwnd);
		thumb->ongrab = 1;
		thumb->lastx = x;
		thumb->lasty = y;
		break;
	case WM_LBUTTONUP:
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		ReleaseCapture();
		thumb->ongrab = 0;
		break;
	case WM_MOUSEMOVE:
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		if ((wParam & MK_LBUTTON) && thumb->ongrab)
		{
			int oldx, oldy;
			GetWindowPos(hwnd, &oldx, &oldy);
			if (thumb->direction == BBX_SCROLLBAR_VERTICAL)
			{
				int newy = oldy + y - thumb->lasty;
				if (newy >= 0 && newy <= thumb->maxy)
				{
					MoveWindow(thumb->win, 0, newy, thumb->width, thumb->height, TRUE);
					(*thumb->fptr)(thumb->ptr, newy);
				}
			}
			else if (thumb->direction == BBX_SCROLLBAR_HORIZONTAL)
			{
				int newx = oldx - thumb->lastx + x;
				if (newx >= 0 && newx <= thumb->maxx)
				{
					MoveWindow(thumb->win, newx, 0, thumb->width, thumb->height, TRUE);
					(*thumb->fptr)(thumb->ptr, newx);
				}
			}
		}
		//thumb->lastx = x;
		//thumb->lasty = y;
		break;
	
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

static void PaintThumb(HWND hwnd, BBX_ScrollThumb *thumb)
{
	PAINTSTRUCT ps;
	HDC hdc;
	HBRUSH hbrush;
	HPEN hpenhi;
	HPEN hpenlo;
	HPEN hpenold;
	RECT rect;
	BBX_RGBA grey;
	BBX_RGBA higrey;
	BBX_RGBA dimgrey;

	hdc = BeginPaint(hwnd, &ps);
	GetClientRect(hwnd, &rect);
	grey = bbx_color("gray");
	hbrush = CreateSolidBrush(RGB(bbx_red(grey), bbx_green(grey), bbx_blue(grey)));
	SelectObject(hdc, hbrush);
	Rectangle(hdc, rect.left, rect.top, rect.right+1, rect.bottom+1);
	higrey = bbx_color("light gray");
	dimgrey = bbx_color("dim gray");
	hpenlo = CreatePen(PS_SOLID, 1, RGB(bbx_red(dimgrey), bbx_green(dimgrey), bbx_blue(dimgrey)));
	hpenhi = CreatePen(PS_SOLID, 1, RGB(bbx_red(higrey), bbx_green(higrey), bbx_blue(higrey)));
	MoveToEx(hdc, 0, 0, 0);

	hpenold = SelectObject(hdc, hpenlo);
	LineTo(hdc, rect.right - 1, 0);
	SelectObject(hdc, hpenlo);
	LineTo(hdc, rect.right - 1, rect.bottom - 1);
	LineTo(hdc, 0, rect.bottom - 1);
	SelectObject(hdc, hpenlo);
	LineTo(hdc, 0, 0);
	MoveToEx(hdc, 1, 1, 0);
	SelectObject(hdc, hpenhi);
	LineTo(hdc, 1, rect.bottom - 1);

	if (thumb->direction == BBX_SCROLLBAR_VERTICAL)
	{
		MoveToEx(hdc, 2, rect.bottom / 2, 0);
		LineTo(hdc, rect.right - 2, rect.bottom/2);
		MoveToEx(hdc, 4, rect.bottom / 2 -4, 0);
		LineTo(hdc, rect.right - 4, rect.bottom/2 -4);
		MoveToEx(hdc, 4, rect.bottom / 2 + 4, 0);
		LineTo(hdc, rect.right - 4, rect.bottom/2+4);
		SelectObject(hdc, hpenlo);
		MoveToEx(hdc, 2, rect.bottom / 2+1, 0);
		LineTo(hdc, rect.right - 2, rect.bottom / 2+1);
		MoveToEx(hdc, 4, rect.bottom / 2 - 4+1, 0);
		LineTo(hdc, rect.right - 4, rect.bottom / 2 - 4+1);
		MoveToEx(hdc, 4, rect.bottom / 2 + 4+1, 0);
		LineTo(hdc, rect.right - 4, rect.bottom / 2 + 4+1);
	}
	else if (thumb->direction == BBX_SCROLLBAR_HORIZONTAL)
	{
		MoveToEx(hdc, rect.right/2, 2, 0);
		LineTo(hdc, rect.right/2, rect.bottom - 2);
		MoveToEx(hdc, rect.right/2-4, 4, 0);
		LineTo(hdc, rect.right/2-4, rect.bottom - 4);
		MoveToEx(hdc, rect.right/2+4, 4, 0);
		LineTo(hdc, rect.right/2+4, rect.bottom - 4);
		SelectObject(hdc, hpenlo);
		MoveToEx(hdc, rect.right / 2+1, 2, 0);
		LineTo(hdc, rect.right / 2+1, rect.bottom - 2);
		MoveToEx(hdc, rect.right / 2 - 4+1, 4, 0);
		LineTo(hdc, rect.right / 2 - 4+1, rect.bottom - 4);
		MoveToEx(hdc, rect.right / 2 + 4+1, 4, 0);
		LineTo(hdc, rect.right / 2 + 4+1, rect.bottom - 4);
	
	}
	SelectObject(hdc, hpenold);
	DeleteObject(hbrush);
	DeleteObject(hpenhi);
	DeleteObject(hpenlo);
	EndPaint(hwnd, &ps);
}

static void GetWindowPos(HWND hWnd, int *x, int *y)
{
	HWND hWndParent = GetParent(hWnd);
	POINT p = { 0 };

	MapWindowPoints(hWnd, hWndParent, &p, 1);

	(*x) = p.x;
	(*y) = p.y;
}
/*
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
      
      //XGrabButton(bbx->dpy, Button1, AnyModifier, thumb->win, True, ButtonReleaseMask | Button1Motionmask, GrabModeAsync, GrabModeAsync, None, None); 
      
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
*/

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
