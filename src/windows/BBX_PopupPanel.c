#include "BabyX.h"


BBX_Panel *BBX_popuppanel(BABYX *bbx, HWND parent, char *tag, void (*changesize)(void *ptr, int width, int height), void *ptr, int x, int y);

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static int message_handler(void *obj, int message, int a, int b, void *params);



ATOM BBX_RegisterPopupPanel(HINSTANCE hInstance)
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
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wcex.lpszMenuName = 0; //MAKEINTRESOURCE(IDC_WINHELLO4);
	wcex.lpszClassName = "BBXPopupPanel"; //szWindowClass;
	wcex.hIconSm = 0; //LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}
BBX_Panel *bbx_popuppanel(BABYX *bbx, BBX_Panel *parent, char *tag, void (*changesize)(void *ptr, int width, int height), void *ptr, int x, int y)
{
  return BBX_popuppanel(bbx, parent->win, tag, changesize, ptr, x, y);
}

BBX_Panel *BBX_popuppanel(BABYX *bbx, HWND parent, char *tag, void (*changesize)(void *ptr, int width, int height), void *ptr, int x, int y)
{
  BBX_Panel *answer;
  POINT pt;

  answer = bbx_malloc(sizeof(BBX_Panel));
  answer->bbx = bbx;
  //grey = BBX_Color("gray");
  //dimgrey = BBX_Color("dim gray");
  //answer->win = XCreateSimpleWindow(bbx->dpy, parent, 0, 0, 100, 100,
//				    1, dimgrey, grey);
  //XSelectInput(bbx->dpy, answer->win, StructureNotifyMask);
  //XTranslateCoordinates(bbx->dpy, parent, root, x, y, &x, &y, &child);
  //swa.override_redirect = True;
  //XChangeWindowAttributes(bbx->dpy, answer->win, CWOverrideRedirect, &swa);
  //XReparentWindow(bbx->dpy, answer->win, root, x, y);
  
  answer->event_handler = 0;
  answer->message_handler = 0;
  answer->changesize = changesize;
  answer->ptr = ptr;
  answer->tag = bbx_strdup(tag);
  answer->width = -1;
  answer->height = -1;
  answer->mousefunc = 0;
  answer->keyfunc = 0;
  answer->closefunc = 0;

  pt.x = x;
  pt.y = y;
  ClientToScreen(parent, &pt);

  answer->win = CreateWindow("BBXPopupPanel", "", WS_POPUP | WS_BORDER |WS_VISIBLE, pt.x, pt.y, 100, 100, 0 
	  /*parent*/, 0, bbx->hinstance, answer);

  BBX_Register(bbx, answer->win, 0, message_handler, answer);

  return answer;  
}

void bbx_popuppanel_makemodal(BBX_DialogPanel *pan)
{
  //ShowWindow(pan->win, SW_SHOWNORMAL);
  BBX_MakeModal(pan->bbx, pan->win);
}

void bbx_popuppanel_dropmodal(BBX_DialogPanel *pan)
{
  BBX_DropModal(pan->bbx);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  BBX_Panel *pan;
  int button;
  int x, y;

  pan = (BBX_Panel *)GetWindowLong(hwnd, GWL_USERDATA);
  switch(msg)
  {
  case WM_CREATE:
	  pan = (BBX_Panel *)((CREATESTRUCT *)lParam)->lpCreateParams;
	  pan->win = hwnd;
	  SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG)((CREATESTRUCT *)lParam)->lpCreateParams);
	  break;
  case WM_SIZE:
	  /*
    width = LOWORD(lParam);
    height = HIWORD(lParam);
    if(pan->changesize)
    {
      if(height != pan->height || width != pan->width)
        (*pan->changesize)(pan->ptr, width, height);
      pan->height = height;
      pan->width = width;
    }
	*/
    break;
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
  default:
	  return DefWindowProc(hwnd, msg, wParam, lParam);
  
  }
  return 0;
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
