#include <Windows.h>
#include <stdlib.h>
#include <string.h>

#include "BabyX.h"

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static HWND BBX_DialogWindow(BABYX *bbx, char *title, int width, int height, void *obj);

ATOM BBX_RegisterDialogPanel(HINSTANCE hInstance)
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
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = 0; //MAKEINTRESOURCE(IDC_WINHELLO4);
	wcex.lpszClassName = "BBXDialogPanel"; //szWindowClass;
	wcex.hIconSm = 0; //LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

BBX_DialogPanel *bbx_dialogpanel(BABYX *bbx, char *name, int width, int height, void (*changesize)(void *ptr, int width, int height), void *ptr)
{
  BBX_Panel *answer;
  BBX_RGBA grey;

  grey = bbx_color("gray");

  answer = bbx_malloc(sizeof(BBX_Panel));
  answer->bbx = bbx;
  answer->message_handler = 0;
  answer->changesize = changesize;
  answer->ptr = ptr;
  answer->tag = bbx_strdup("dialog");
  answer->width = -1;
  answer->height = -1;
  answer->mousefunc  =0;
  answer->closefunc = 0;
  answer->mousefunc = 0;
  answer->modal = 0;
  answer->hbrush = CreateSolidBrush(RGB(bbx_red(grey), bbx_green(grey), bbx_blue(grey)));
  answer->win = BBX_DialogWindow(bbx, name, width, height, answer);

 

  return answer;
}

void bbx_dialogpanel_kill(BBX_DialogPanel *pan)
{
  if(pan)
  {
    BBX_Deregister(pan->bbx, pan->win);
    DestroyWindow(pan->win);
    free(pan->tag);
    free(pan);
  }
}

void bbx_dialogpanel_makemodal(BBX_DialogPanel *pan)
{
  //XMapWindow(pan->bbx->dpy, pan->win);
	if (!pan->modal)
	{
		pan->modal = 1;
		ShowWindow(pan->win, SW_SHOWNORMAL);
		BBX_MakeModal(pan->bbx, pan->win);
	}
}

void bbx_dialogpanel_dropmodal(BBX_DialogPanel *pan)
{
  
	if (pan->modal)
	{
		pan->modal = 0;
		BBX_DropModal(pan->bbx);
	}
}

void bbx_dialogpanel_setclosefunc(BBX_DialogPanel *pan, void (*closefunc)(void *ptr), void *ptr)
{
  pan->closefunc = closefunc;
  pan->ptr = ptr;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  BBX_Panel *pan;
  int width, height;
  HDC hdc;
  PAINTSTRUCT ps;
  RECT rect;

  pan = (BBX_Panel *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  switch(msg)
  {
  case WM_CREATE:
	  pan = (BBX_Panel *)((CREATESTRUCT *)lParam)->lpCreateParams;
	  SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG) ((CREATESTRUCT *)lParam)->lpCreateParams);
	  pan->win = hwnd;
	  BBX_Register(pan->bbx, hwnd, 0, 0, pan);
	  break;
  case	WM_ERASEBKGND:
	  hdc = (HDC)wParam;
	  GetClientRect(hwnd, &rect);
	  SelectObject(hdc, pan->hbrush);
	  Rectangle(hdc, rect.left-1, rect.top-1, rect.right+1, rect.bottom+1);
	  return 0;
	  break;
  case WM_PAINT:
	  hdc = BeginPaint(hwnd, &ps);
	  // TODO: Add any drawing code here...
	  //TextOut(hdc, 10, 10, "Hello World", 11);
	  EndPaint(hwnd, &ps);
	  break;
  case WM_SIZE:
    width = LOWORD(lParam);
    height = HIWORD(lParam);
    if(pan->changesize)
    {
      if(height != pan->height || width != pan->width)
        (*pan->changesize)(pan->ptr, width, height);
      pan->height = height;
      pan->width = width;
    }
    break;
  case WM_DESTROY:
    if(pan->closefunc)
      (*pan->closefunc)(pan->ptr);
    else
      exit(0);
    break;
  default:
	  return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  return 0;
}

static HWND BBX_DialogWindow(BABYX *bbx, char *title, int width, int height, void *obj)
{
    HWND       win;
	HWND hroot;
	RECT       rect;
    int          x, y;
    unsigned int display_width, display_height;
    int border_width = 1;
    
      /*  Get screen size from display structure macro   */
 //   display_width  = DisplayWidth(bbx->dpy, bbx->screen);
 //   display_height = DisplayHeight(bbx->dpy, bbx->screen);
	hroot = GetDesktopWindow();
	GetWindowRect(hroot, &rect);
	display_width = rect.right - rect.left;
	display_height = rect.bottom - rect.top;

    /*  Set initial window size and position, and create it  */

    x = (display_width - width)/2;
    y = (display_height - height)/2;

	rect.left = x;
	rect.right = x + width;
	rect.top = y;
	rect.bottom = y + height;

	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, 0);
   

   // win = XCreateSimpleWindow(bbx->dpy, RootWindow(bbx->dpy, bbx->screen),
	//		      x, y, width, height, border_width,
	//		      BlackPixel(bbx->dpy, bbx->screen),
	//		      BBX_Color("Gray") );

	win = CreateWindow("BBXDialogPanel", title, WS_OVERLAPPEDWINDOW, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0, 0, bbx->hinstance, obj);

 
    return win;
}
