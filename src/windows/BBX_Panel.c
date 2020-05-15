#include <stdlib.h>
#include <string.h>

#include "BabyX.h"

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void TranslateCoords(HWND hwnd, HWND other, int *x, int *y);
static int message_handler(void *obj, int message, int a, int b, void *params);

ATOM BBX_RegisterPanel(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = 0; //LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINHELLO4));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH) GetStockObject(GRAY_BRUSH);
	wcex.lpszMenuName = 0; //MAKEINTRESOURCE(IDC_WINHELLO4);
	wcex.lpszClassName = "BBXPanel"; //szWindowClass;
	wcex.hIconSm = 0; //LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

BBX_Panel *BBX_panel(BABYX *bbx, HWND parent, char *tag, void(*changesize)(void *ptr, int width, int height), void *ptr);

static int specialkey(WPARAM ks);

BBX_Panel *bbx_panel(BABYX *bbx, BBX_Panel *parent, char *tag, void (*changesize)(void *ptr, int width, int height), void *ptr)
{
  return BBX_panel(bbx, parent->win, tag, changesize, ptr);
}

BBX_Panel *BBX_panel(BABYX *bbx, HWND parent, char *tag, void (*changesize)(void *ptr, int width, int height), void *ptr)
{
  BBX_Panel *answer;
  BBX_RGBA grey;

  answer = bbx_malloc(sizeof(BBX_Panel));
  answer->bbx = bbx;
  grey = bbx_color("gray");
  
  answer->event_handler = 0;
  answer->message_handler = message_handler;
  answer->changesize = changesize;
  answer->ptr = ptr;
  answer->tag = bbx_strdup(tag);
  answer->width = -1;
  answer->height = -1;
  answer->mousefunc = 0;
  answer->closefunc = 0;
  answer->keyfunc = 0;
  answer->modal = 0;
  answer->hbrush = CreateSolidBrush(RGB(bbx_red(grey), bbx_green(grey), bbx_blue(grey)));

  answer->win = CreateWindow("BBXPanel", "", WS_CHILD , 0, 0, 10, 10, parent, 0, bbx->hinstance, answer);
  BBX_Register(bbx, answer->win, 0, message_handler, answer);

  return answer;  
}

void bbx_panel_kill(BBX_Panel *pan)
{
  if(pan)
  {
    free(pan->tag);
    BBX_Deregister(pan->bbx, pan->win);
    DestroyWindow(pan->win);
	DeleteObject(pan->hbrush);
    free(pan);
  }
}

char *bbx_panel_gettag(BBX_Panel *pan)
{
  return pan->tag;
}

void bbx_panel_setbackground(BBX_Panel *obj, BBX_RGBA col)
{
	//HDC hdc;
	//PAINTSTRUCT ps;
	DeleteObject(obj->hbrush);
	obj->hbrush = CreateSolidBrush(RGB(bbx_red(col), bbx_green(col), bbx_blue(col)));
	//hdc = BeginPaint(obj->win, &ps);
	//SendMessage(obj->win, WM_ERASEBKGND, (WPARAM) hdc, 0);
	//EndPaint(obj->win, &hdc);
	InvalidateRect(obj->win, 0, TRUE);
  
}

void bbx_panel_setmousefunc(BBX_Panel *pan, void (*mousefunc)(void *ptr, int action, int x, int y, int buttons), void *ptr)
{
  pan->mousefunc = mousefunc;
  pan->ptr = ptr; 
}

void bbx_panel_setkeyfunc(BBX_Panel *pan, void (*keyfunc)(void *ptr, int ch), void *ptr)
{
  pan->keyfunc = keyfunc;
  pan->ptr = ptr; 
}

void *bbx_panel_getptr(BBX_Panel *pan)
{
  return pan->ptr;
}

int bbx_panel_gotmouse(BBX_Panel *pan, int *x, int *y)
{
	POINT pt;
	RECT rect;
	GetCursorPos(&pt);
	GetWindowRect(pan->win, &rect);
	if (pt.x >= rect.left && pt.x < rect.right && pt.y >= rect.top && pt.y < rect.bottom)
	{
		ScreenToClient(pan->win, &pt);
		if (x)
			*x = pt.x;
		if (y)
			*y = pt.y;
		return 1;
	}
	if (x)
		*x = -1;
	if (y)
		*y = -1; 
	return 0;
	/*
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
	*/
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BBX_Panel *pan;
	HDC hdc;
	int x, y;
	int button;
	RECT rect;
	int ch;

	pan = (BBX_Panel *)GetWindowLongPtr(hwnd, GWL_USERDATA);
	switch (msg)
	{
	case WM_CREATE:
		pan = (BBX_Panel *) ((CREATESTRUCT *)lParam)->lpCreateParams;
		SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG)((CREATESTRUCT *)lParam)->lpCreateParams);
		pan->win = hwnd;
		break;
	case	WM_ERASEBKGND:
		hdc = (HDC)wParam;
		GetClientRect(hwnd, &rect);
		SelectObject(hdc, pan->hbrush);
		Rectangle(hdc, rect.left-1, rect.top-1, rect.right+1, rect.bottom+1);
		return 0;
		break;
		/*
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		GetClientRect(hwnd, &rect);
		SelectObject(hdc, pan->hbrush);
		Rectangle(hdc, rect.left - 1, rect.top - 1, rect.right + 1, rect.bottom + 1);
		EndPaint(hwnd, &ps);
		break;
		*/
  //case WM_SIZE:
   // width = LOWORD(lParam);
   // height = HIWORD(lParam);
   // if(pan->changesize)
   // {
     // if(height != pan->height || width != pan->width)
       // (*pan->changesize)(pan->ptr, width, height);
      //pan->height = height;
      //pan->width = width;
   // }
   // break;
   
  case WM_LBUTTONDOWN:
	  x = GET_X_LPARAM(lParam);
	  y = GET_Y_LPARAM(lParam);
	  if (pan->keyfunc)
		  SetFocus(hwnd);
	  if (pan->mousefunc)
		  (*pan->mousefunc)(pan->ptr, BBX_MOUSE_CLICK, x, y, BBX_MOUSE_BUTTON1);
	  else
	  {
		  TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
		  SendMessage(GetParent(hwnd), WM_LBUTTONDOWN, wParam, MAKELPARAM(x, y));
	  }
	  break;
  case WM_LBUTTONUP:
	  x = GET_X_LPARAM(lParam);
	  y = GET_Y_LPARAM(lParam);
	  if (pan->mousefunc)
		  (*pan->mousefunc)(pan->ptr, BBX_MOUSE_RELEASE, x, y, BBX_MOUSE_BUTTON1);
	  else
	  {
		  TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
		  SendMessage(GetParent(hwnd), WM_LBUTTONUP, wParam, MAKELPARAM(x, y));
	  }
	  break;
  case WM_RBUTTONDOWN:
	  x = GET_X_LPARAM(lParam);
	  y = GET_Y_LPARAM(lParam);
	  if (pan->mousefunc)
		  (*pan->mousefunc)(pan->ptr, BBX_MOUSE_CLICK, x, y, BBX_MOUSE_BUTTON2);
	  else
	  {
		  TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
		  SendMessage(GetParent(hwnd), WM_RBUTTONDOWN, wParam, MAKELPARAM(x, y));
	  }
	  if (pan->keyfunc)
		  SetFocus(hwnd);
	  break;
  case WM_RBUTTONUP:
	  x = GET_X_LPARAM(lParam);
	  y = GET_Y_LPARAM(lParam);
	  if (pan->mousefunc)
		  (*pan->mousefunc)(pan->ptr, BBX_MOUSE_RELEASE, x, y, BBX_MOUSE_BUTTON2);
	  else
	  {
		  TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
		  SendMessage(GetParent(hwnd), WM_RBUTTONUP, wParam, MAKELPARAM(x, y));
	  }
	  break;
  case WM_MBUTTONDOWN:
	  x = GET_X_LPARAM(lParam);
	  y = GET_Y_LPARAM(lParam);
	  if (pan->mousefunc)
		  (*pan->mousefunc)(pan->ptr, BBX_MOUSE_CLICK, x, y, BBX_MOUSE_BUTTON3);
	  else
	  {
		  TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
		  SendMessage(GetParent(hwnd), WM_MBUTTONDOWN, wParam, MAKELPARAM(x, y));
	  }
	  if (pan->keyfunc)
		  SetFocus(hwnd);
	  break;
  case WM_MBUTTONUP:
	  x = GET_X_LPARAM(lParam);
	  y = GET_Y_LPARAM(lParam);
	  if (pan->mousefunc)
		  (*pan->mousefunc)(pan->ptr, BBX_MOUSE_RELEASE, x, y, BBX_MOUSE_BUTTON3);
	  else
	  {
		  TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
		  SendMessage(GetParent(hwnd), WM_MBUTTONUP, wParam, MAKELPARAM(x, y));
	  }
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
    if(pan->mousefunc)
      (*pan->mousefunc)(pan->ptr, BBX_MOUSE_MOVE, x, y, button);
	else
	{
		TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
		SendMessage(GetParent(hwnd), WM_MOUSEMOVE, wParam, MAKELPARAM(x, y));
	}
    break;
  case WM_KEYDOWN:
	ch = specialkey(wParam);
    if(ch != 0 && pan->keyfunc)
    {
      (*pan->keyfunc)(pan->ptr, ch);
    }
	if (ch)
		return 0;
	else
		DefWindowProc(hwnd, msg, wParam, lParam);
    break;
  case WM_CHAR:
	if (pan->keyfunc)
	{
		ch = wParam;
		if (ch == 0x08 || ch == '\r' || ch == '\n')
			ch = BBX_KEY_BACKSPACE;
		else
			(*pan->keyfunc)(pan->ptr, ch);
	}
	break;
  case WM_SETFOCUS:
    if(pan->keyfunc)
      (*pan->keyfunc)(pan->ptr, BBX_KEY_GOTFOCUS);
    break;
  case WM_KILLFOCUS:
    if(pan->keyfunc)
       (*pan->keyfunc)(pan->ptr, BBX_KEY_LOSTFOCUS);
    break;
  default:
	  return DefWindowProc(hwnd, msg, wParam, lParam);
    }
	return 0;
}

static void TranslateCoords(HWND hwnd, HWND other, int *x, int *y)
{
	POINT pt;
	pt.x = *x;
	pt.y = *y;
	ClientToScreen(hwnd, &pt);
	ScreenToClient(other, &pt);
	*x = pt.x;
	*y = pt.y;

}
static int message_handler(void *obj, int message, int a, int b, void *params)
{
	BBX_Panel *pan = obj;
	if (message == BBX_RESIZE)
	{
		if (pan->changesize)
			(*pan->changesize)(pan->ptr, a, b);
	}
	return 0;
}


static int specialkey(WPARAM ks)
{
	switch (ks)
	{
	case VK_BACK: return BBX_KEY_BACKSPACE;
	case VK_DELETE: return BBX_KEY_DELETE;
	case VK_ESCAPE: return BBX_KEY_ESCAPE;
	case VK_HOME: return BBX_KEY_HOME;
	case VK_LEFT: return BBX_KEY_LEFT;
	case VK_UP: return BBX_KEY_UP;
	case VK_RIGHT: return BBX_KEY_RIGHT;
	case VK_DOWN: return BBX_KEY_DOWN;
	case VK_END: return BBX_KEY_END;
	case VK_RETURN: return '\n';



		//  #define XK_Shift_L      0xFFE1  /* Left shift */
		//#define XK_Shift_R      0xFFE2  /* Right shift */
		//#define XK_Control_L        0xFFE3  /* Left control */
		//#define XK_Control_R        0xFFE4  /* Right control */
		// #define XK_Caps_Lock        0xFFE5  /* Caps lock */
		//#define XK_Shift_Lock       0xFFE6  /* Shift lock */


     default: return 0;

	}
}
		


