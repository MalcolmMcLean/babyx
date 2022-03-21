#include <stdlib.h>
#include <string.h>
#include "BabyX.h"

#if 0

typedef struct
{
  BABYX *bbx;
  Window win;
  void (*event_handler)(void *obj, XEvent *event);
  int (*message_handler)(void *obj, int message, int a, int b, void *params);
  /* private stuff */
  void *(mousefunc)(void *ptr, int action, int x, int y, int buttons);
  void *ptr;
  unsigned char *rgba;
  int width;
  int height;
  unsigned long background;

  XImage *img;
} BBX_Canvas;


#endif

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static HBITMAP MakeBitmap(unsigned char *rgba, int width, int height, VOID **buff);
static void TranslateCoords(HWND hwnd, HWND other, int *x, int *y);
static void resizergba(unsigned char *dest, int dwidth, int dheight, unsigned char *src, int swidth, int sheight);

ATOM BBX_RegisterCanvas(HINSTANCE hInstance)
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
	wcex.lpszClassName = "BBXCanvas"; //szWindowClass;
	wcex.hIconSm = 0; //LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

BBX_Canvas *bbx_canvas(BABYX *bbx, BBX_Panel*parent, int width, int height, BBX_RGBA bg)
{
  return BBX_canvas(bbx, parent->win, width, height, bg);
}

BBX_Canvas *BBX_canvas(BABYX *bbx, HWND parent, int width, int height, BBX_RGBA bg)
{
  BBX_Canvas *answer;
  int i;

  answer = bbx_malloc(sizeof(BBX_Canvas));
  answer->bbx = bbx;
  answer->event_handler = 0;
  answer->message_handler = 0;
  answer->rgba = bbx_malloc(width * height *4);
  answer->width = width;
  answer->height = height;
  answer->background = bg;
  answer->mousefunc = 0;
  for(i=0;i<width*height;i++)
  {
    answer->rgba[i*4] = bbx_red(bg);
    answer->rgba[i*4+1] = bbx_green(bg);
    answer->rgba[i*4+2] = bbx_blue(bg);
    answer->rgba[i*4+3] = 0xFF;
  }

  answer->img = MakeBitmap(answer->rgba, width, height, &answer->pvBits);
  answer->win = CreateWindow("BBXCanvas", "", WS_CHILD, 0, 0, width, height, parent, 0, bbx->hinstance, answer);

  BBX_Register(bbx, answer->win, 0, 0, answer);

  return answer;  
}

void bbx_canvas_kill(BBX_Canvas *can)
{
  if(can)
  {
    free(can->rgba);
    BBX_Deregister(can->bbx, can->win);
	DeleteObject(can->img);
    DestroyWindow(can->win);
    free(can);
  }
}

void bbx_canvas_setimage(BBX_Canvas *can, unsigned char *rgba, int width, int height)
{
  int i, j;
  int red, green, blue, alpha;
  int bgred, bggreen, bgblue;

  bgred = bbx_red(can->background);
  bggreen = bbx_green(can->background);
  bgblue = bbx_blue(can->background);

  if(width == can->width && height == can->height)
    memcpy(can->rgba, rgba, width * height * 4);
  else
    resizergba(can->rgba, can->width, can->height, rgba, width, height);
 
  for(i=0;i<can->width * can->height;i++)
    if(can->rgba[i*4+3] != 0xFF)
    {
      j = i*4;
      alpha = can->rgba[j+3];
      red = bgred * (255-alpha) + can->rgba[j] * alpha;
      green = bggreen * (255-alpha) + can->rgba[j+1] * alpha;
      blue = bgblue * (255-alpha) + can->rgba[j+2] * alpha;
      red /= 255;
      green /= 255;
      blue /= 255;
      can->rgba[j] = red;
      can->rgba[j+1] = green;
      can->rgba[j+2] = blue;
      can->rgba[j+3] = 255;
    }
  
} 

unsigned char *bbx_canvas_rgba(BBX_Canvas *can, int *width, int *height)
{
  *width = can->width;
  *height = can->height;
  return can->rgba;
}

void bbx_canvas_flush(BBX_Canvas *can)
{
  BABYX *bbx = can->bbx;
  int x, y;
  int width, height;
  int red, green, blue, alpha;
  unsigned char *rgba;

  rgba = can->rgba;
  width = can->width;
  height = can->height;

  for (y = 0; y < height; y++)
  {
	  for (x = 0; x < width; x++)
	  {
		  red = rgba[(y*width + x) * 4];
		  green = rgba[(y*width + x) * 4 + 1];
		  blue = rgba[(y*width + x) * 4 + 2];
		  alpha = rgba[(y*width + x) * 4 + 3];
		  red = (red * alpha) >> 8;
		  green = (green * alpha) >> 8;
		  blue = (blue * alpha) >> 8;
		  ((UINT32 *)can->pvBits)[(height - y - 1) * width + x] = (alpha << 24) | (red << 16) | (green << 8) | blue;
	  }
  }
  BBX_InvalidateWindow(bbx, can->win);
}

void bbx_canvas_setmousefunc(BBX_Canvas *can, void (*mousefunc)(void *ptr, int action, int x, int y, int buttons), void *ptr)
{
  can->mousefunc = mousefunc;
  can->ptr = ptr;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BBX_Canvas *can;
	HDC hdc;
	HDC hdcbmp;
	PAINTSTRUCT ps;
	int x, y;
	int button;

	can = (BBX_Canvas *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (msg)
	{
	case WM_CREATE:
		can = (BBX_Canvas *)((CREATESTRUCT *)lParam)->lpCreateParams;
		can->win = hwnd;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)((CREATESTRUCT *)lParam)->lpCreateParams);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		hdcbmp = CreateCompatibleDC(hdc);
		SelectObject(hdcbmp, can->img);
		BitBlt(hdc, 0, 0, can->width, can->height, hdcbmp, 0, 0, SRCCOPY);
		DeleteDC(hdcbmp);
	    EndPaint(hwnd, &ps);
		break;
	case WM_LBUTTONDOWN:
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		if (can->mousefunc)
			(*can->mousefunc)(can->ptr, BBX_MOUSE_CLICK, x, y, BBX_MOUSE_BUTTON1);
		else
		{
			TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
			SendMessage(GetParent(hwnd), WM_LBUTTONDOWN, wParam, MAKELPARAM(x, y));
		}
		break;
	case WM_LBUTTONUP:
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		if (can->mousefunc)
			(*can->mousefunc)(can->ptr, BBX_MOUSE_RELEASE, x, y, BBX_MOUSE_BUTTON1);
		else
		{
			TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
			SendMessage(GetParent(hwnd), WM_LBUTTONUP, wParam, MAKELPARAM(x, y));
		}
		break;
	case WM_RBUTTONDOWN:
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		if (can->mousefunc)
			(*can->mousefunc)(can->ptr, BBX_MOUSE_CLICK, x, y, BBX_MOUSE_BUTTON2);
		else
		{
			TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
			SendMessage(GetParent(hwnd), WM_RBUTTONDOWN, wParam, MAKELPARAM(x, y));
		}
		break;
	case WM_RBUTTONUP:
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		if (can->mousefunc)
			(*can->mousefunc)(can->ptr, BBX_MOUSE_RELEASE, x, y, BBX_MOUSE_BUTTON2);
		else
		{
			TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
			SendMessage(GetParent(hwnd), WM_RBUTTONUP, wParam, MAKELPARAM(x, y));
		}
		break;
	case WM_MBUTTONDOWN:
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		if (can->mousefunc)
			(*can->mousefunc)(can->ptr, BBX_MOUSE_CLICK, x, y, BBX_MOUSE_BUTTON3);
		else
		{
			TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
			SendMessage(GetParent(hwnd), WM_MBUTTONDOWN, wParam, MAKELPARAM(x, y));
		}
		break;
	case WM_MBUTTONUP:
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		if (can->mousefunc)
			(*can->mousefunc)(can->ptr, BBX_MOUSE_RELEASE, x, y, BBX_MOUSE_BUTTON3);
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
		if (can->mousefunc)
			(*can->mousefunc)(can->ptr, BBX_MOUSE_MOVE, x, y, button);
		else
		{
			TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
			SendMessage(GetParent(hwnd), WM_MOUSEMOVE, wParam, MAKELPARAM(x, y));
		}
		break;
	case WM_CHAR:
		SendMessage(GetParent(hwnd), WM_CHAR, wParam, lParam);
		break;
	case WM_SETFOCUS:
		SendMessage(GetParent(hwnd), WM_SETFOCUS, wParam, lParam);
		break;
	case WM_KILLFOCUS:
		SendMessage(GetParent(hwnd), WM_KILLFOCUS, wParam, lParam);
		break;
	//case WM_DESTROY:
		//break;
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


static HBITMAP MakeBitmap(unsigned char *rgba, int width, int height, VOID **buff)
{
	VOID *pvBits;          // pointer to DIB section 
	HBITMAP answer;
	BITMAPINFO bmi;
	HDC hdc;
	int x, y;
	int red, green, blue, alpha;

	// setup bitmap info   
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;         // four 8-bit components 
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = width * height * 4;

	hdc = CreateCompatibleDC(GetDC(0));
	answer = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			red = rgba[(y*width + x) * 4];
			green = rgba[(y*width + x) * 4 + 1];
			blue = rgba[(y*width + x) * 4 + 2];
			alpha = rgba[(y*width + x) * 4 + 3];
			red = (red * alpha) >> 8;
			green = (green * alpha) >> 8;
			blue = (blue * alpha) >> 8;
			((UINT32 *)pvBits)[(height - y - 1) * width + x] = (alpha << 24) | (red << 16) | (green << 8) | blue;
		}
	}
	DeleteDC(hdc);

	*buff = pvBits;

	return answer;
}



static void resizergba(unsigned char *dest, int dwidth, int dheight, unsigned char *src, int swidth, int sheight)
{
  float a = 0.0f, b = 0.0f;
  float red, green, blue, alpha;
  float dx, dy;
  float rx, ry;
  int x, y;
  int index0, index1, index2, index3;

  dx = ((float) swidth)/dwidth;
  dy = ((float) sheight)/dheight;
  for(y=0, ry = 0;y<dheight-1;y++, ry += dy)
  {
    b = ry - (int) ry;
    for(x=0, rx = 0;x<dwidth-1;x++, rx += dx)
    {
      a = rx - (int) rx;
      index0 = (int)ry * swidth + (int) rx;
      index1 = index0 + 1;
      index2 = index0 + swidth;     
      index3 = index0 + swidth + 1;

      red = src[index0*4] * (1.0f-a)*(1.0f-b);
      green = src[index0*4+1] * (1.0f-a)*(1.0f-b);
      blue = src[index0*4+2] * (1.0f-a)*(1.0f-b);
      alpha = src[index0*4+3] * (1.0f-a)*(1.0f-b);
      red += src[index1*4] * (a)*(1.0f-b);
      green += src[index1*4+1] * (a)*(1.0f-b);
      blue += src[index1*4+2] * (a)*(1.0f-b);
      alpha += src[index1*4+3] * (a)*(1.0f-b);
      red += src[index2*4] * (1.0f-a)*(b);
      green += src[index2*4+1] * (1.0f-a)*(b);
      blue += src[index2*4+2] * (1.0f-a)*(b);
      alpha += src[index2*4+3] * (1.0f-a)*(b);
      red += src[index3*4] * (a)*(b);
      green += src[index3*4+1] * (a)*(b);
      blue += src[index3*4+2] * (a)*(b);
      alpha += src[index3*4+3] * (a)*(b);
    
      red = red < 0 ? 0 : red > 255 ? 255 : red;
      green = green < 0 ? 0 : green > 255 ? 255 : green;
      blue = blue < 0 ? 0 : blue > 255 ? 255 : blue;
      alpha = alpha < 0 ? 0 : alpha > 255 ? 255 : alpha;

      dest[(y*dwidth+x)*4] = (unsigned char) red;
      dest[(y*dwidth+x)*4+1] = (unsigned char) green;
      dest[(y*dwidth+x)*4+2] = (unsigned char) blue;
      dest[(y*dwidth+x)*4+3] = (unsigned char) alpha;
    }
    index0 = (int)ry * swidth + (int) rx;
    index1 = index0;
    index2 = index0 + swidth;     
    index3 = index0 + swidth;   

    red = src[index0*4] * (1.0f-a)*(1.0f-b);
    green = src[index0*4+1] * (1.0f-a)*(1.0f-b);
    blue = src[index0*4+2] * (1.0f-a)*(1.0f-b);
    alpha = src[index0*4+3] * (1.0f-a)*(1.0f-b);
    red += src[index1*4] * (a)*(1.0f-b);
    green += src[index1*4+1] * (a)*(1.0f-b);
    blue += src[index1*4+2] * (a)*(1.0f-b);
    alpha += src[index1*4+3] * (a)*(1.0f-b);
    red += src[index2*4] * (1.0f-a)*(b);
    green += src[index2*4+1] * (1.0f-a)*(b);
    blue += src[index2*4+2] * (1.0f-a)*(b);
    alpha += src[index2*4+3] * (1.0f-a)*(b);
    red += src[index3*4] * (a)*(b);
    green += src[index3*4+1] * (a)*(b);
    blue += src[index3*4+2] * (a)*(b);
    alpha += src[index3*4+3] * (a)*(b);
        
    red = red < 0 ? 0 : red > 255 ? 255 : red;
    green = green < 0 ? 0 : green > 255 ? 255 : green;
    blue = blue < 0 ? 0 : blue > 255 ? 255 : blue;

    alpha = alpha < 0 ? 0 : alpha > 255 ? 255 : alpha;
    dest[(y*dwidth+x)*4] = (unsigned char) red;
    dest[(y*dwidth+x)*4+1] = (unsigned char) green;
    dest[(y*dwidth+x)*4+2] = (unsigned char) blue;
    dest[(y*dwidth+x)*4+3] = (unsigned char) alpha;
  }
  index0 = (int)ry * swidth + (int) rx;
  index1 = index0;
  index2 = index0 + swidth;     
  index3 = index0 + swidth;   

  for(x=0, rx = 0;x<dwidth-1;x++, rx += dx)
  {
    a = rx - (int) rx;
    index0 = (int)ry * swidth + (int) rx;
    index1 = index0 + 1;
    index2 = index0;     
    index3 = index0;

    red = src[index0*4] * (1.0f-a)*(1.0f-b);
    green = src[index0*4+1] * (1.0f-a)*(1.0f-b);
    blue = src[index0*4+2] * (1.0f-a)*(1.0f-b);
    alpha = src[index0*4+3] * (1.0f-a)*(1.0f-b);
    red += src[index1*4] * (a)*(1.0f-b);
    green += src[index1*4+1] * (a)*(1.0f-b);
    blue += src[index1*4+2] * (a)*(1.0f-b);
    alpha += src[index1*4+3] * (a)*(1.0f-b);
    red += src[index2*4] * (1.0f-a)*(b);
    green += src[index2*4+1] * (1.0f-a)*(b);
    blue += src[index2*4+2] * (1.0f-a)*(b);
    alpha += src[index2*4+3] * (1.0f-a)*(b);
    red += src[index3*4] * (a)*(b);
    green += src[index3*4+1] * (a)*(b);
    blue += src[index3*4+2] * (a)*(b);
    alpha += src[index3*4+3] * (a)*(b);

    red = red < 0 ? 0 : red > 255 ? 255 : red;
    green = green < 0 ? 0 : green > 255 ? 255 : green;
    blue = blue < 0 ? 0 : blue > 255 ? 255 : blue;
    alpha = alpha < 0 ? 0 : alpha > 255 ? 255 : alpha;
      
    dest[(y*dwidth+x)*4] = (unsigned char) red;
    dest[(y*dwidth+x)*4+1] = (unsigned char) green;
    dest[(y*dwidth+x)*4+2] = (unsigned char) blue;
    dest[(y*dwidth+x)*4+3] = (unsigned char) alpha;
  }
  
  dest[(y*dwidth+x)*4] = src[((sheight-1)*swidth+swidth-1)*4];
  dest[(y*dwidth+x)*4+1] = src[((sheight-1)*swidth+swidth-1)*4+1];
  dest[(y*dwidth+x)*4+2] = src[((sheight-1)*swidth+swidth-1)*4+2];
  dest[(y*dwidth+x)*4+3] = src[((sheight-1)*swidth+swidth-1)*4+3];
}  
