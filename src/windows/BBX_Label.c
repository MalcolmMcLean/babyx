#include <stdlib.h>
#include <string.h>

#include "BabyX.h"

#include <assert.h>

extern struct bitmap_font fred_font;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void TranslateCoords(HWND hwnd, HWND other, int *x, int *y);
static void render(BBX_Label *lab);
static HBITMAP MakeBitmap(unsigned char *rgba, int width, int height);
static int getNlines(char *str);

ATOM BBX_RegisterLabel(HINSTANCE hInstance)
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
	wcex.lpszClassName = "BBXLabel"; //szWindowClass;
	wcex.hIconSm = 0; //LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

BBX_Label *bbx_label(BABYX *bbx, BBX_Panel *parent, char *text)
{
  return BBX_label(bbx, parent->win, text);
}

BBX_Label *BBX_label(BABYX *bbx, HWND parent, char *text)
{
  BBX_Label *answer;

  answer = bbx_malloc(sizeof(BBX_Label));
  answer->bbx = bbx;
  answer->event_handler = 0;
  answer->message_handler = 0;
  answer->text = bbx_strdup(text);
  answer->fgcol = bbx_color("black");
  answer->bgcol = bbx_color("gray");
  answer->img = 0;
  answer->font = bbx->gui_font;
  answer->align = BBX_ALIGN_CENTER;

  answer->win = CreateWindow("BBXLabel", "", WS_CHILD |WS_VISIBLE, 0, 0, 100, 30, parent, 0, bbx->hinstance, answer);
  assert(IsWindow(answer->win));

  BBX_Register(bbx, answer->win, 0, 0, answer);

  return answer;  
}  

void bbx_label_kill(BBX_Label *obj)
{
  if(obj)
  {
    BBX_Deregister(obj->bbx, obj->win);
    DestroyWindow(obj->win);
	DeleteObject(obj->img);
    free(obj->text);
    free(obj);
  }
}

void bbx_label_settext(BBX_Label *obj, char *text)
{
  free(obj->text);
  obj->text = bbx_strdup(text);
  //render(obj);
  BBX_InvalidateWindow(obj->bbx, obj->win);
}

void bbx_label_setalignment(BBX_Label *obj, int align)
{
  if(obj->align != align)
  {
    obj->align = align;
   // render(obj);
    BBX_InvalidateWindow(obj->bbx, obj->win);
  }
}

void bbx_label_setbackground(BBX_Label *obj, BBX_RGBA col)
{
  if(obj->bgcol != col)
  {
    obj->bgcol = col;
    //render(obj);
    BBX_InvalidateWindow(obj->bbx, obj->win);
  }
}

void bbx_label_setforeground(BBX_Label *obj, BBX_RGBA col)
{
  if(obj->fgcol != col)
  {
    obj->fgcol = col;
    //render(obj);
    BBX_InvalidateWindow(obj->bbx, obj->win);
  }
}

void bbx_label_setfont(BBX_Label *obj, struct bitmap_font *fs)
{
  obj->font = fs;
  //render(obj);
  BBX_InvalidateWindow(obj->bbx, obj->win);
}

int bbx_label_getpreferredsize(BBX_Label *lab, int *width, int *height)
{
    int w = 0;
    int h;
    int Nlines;
    char *line;
    char *end;
    int i;
    int temp;
    int len;

    Nlines = getNlines(lab->text);
    
    line = lab->text;
    end = strchr(line, '\n');
     
    for(i=0;i<Nlines;i++)
    {
        if(end)
          len = end - line;
        else
          len = strlen(line);
        temp = bbx_utf8width(lab->font, line, len);
        if(w < temp)
          w = temp;
        if(end)
	{
          line = end + 1;
          end = strchr(line, '\n');
	}
    }

    h = Nlines * (lab->font->ascent + lab->font->descent);
    *width = w + 5;
    *height = h;
    return 0;
}
 
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  BBX_Label *lab;
  int width, height;
  BABYX *bbx;
  PAINTSTRUCT ps;
  HDC hdc;
  HDC hdcbmp;
  RECT rect;
  int x, y;
 
  lab = (BBX_Label *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  if (lab)
	bbx = lab->bbx;

  switch(msg)
  {
  case WM_CREATE:
	  lab = (BBX_Label *)((CREATESTRUCT *)lParam)->lpCreateParams;
//	  lab->win = lab;
	  SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)((CREATESTRUCT *)lParam)->lpCreateParams);
	  break;
  case WM_PAINT:
	 render(lab);
	 hdc = BeginPaint(hwnd, &ps);
	 GetClientRect(hwnd, &rect);
	 width = rect.right - rect.left;
	 height = rect.bottom - rect.top;
	 hdcbmp = CreateCompatibleDC(hdc);
	 SelectObject(hdcbmp, lab->img);
	 BitBlt(hdc, 0, 0, width, height, hdcbmp, 0, 0, SRCCOPY);
	 DeleteDC(hdcbmp);
	 EndPaint(hwnd, &ps);
	 break;
     break;
  case WM_SIZE:
	  //render(lab, LOWORD(lParam), HIWORD(lParam););
    break;
  case WM_LBUTTONDOWN:
	  x = GET_X_LPARAM(lParam);
	  y = GET_Y_LPARAM(lParam);
	  TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
	  SendMessage(GetParent(hwnd), WM_LBUTTONDOWN, wParam, MAKELPARAM(x, y));
	  break;
  case WM_LBUTTONUP:
	  x = GET_X_LPARAM(lParam);
	  y = GET_Y_LPARAM(lParam);
	  TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
	  SendMessage(GetParent(hwnd), WM_LBUTTONUP, wParam, MAKELPARAM(x, y));
	  break;
  case WM_RBUTTONDOWN:
	  x = GET_X_LPARAM(lParam);
	  y = GET_Y_LPARAM(lParam);
	  TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
	  SendMessage(GetParent(hwnd), WM_RBUTTONDOWN, wParam, MAKELPARAM(x, y));
	  break;
  case WM_RBUTTONUP:
	  x = GET_X_LPARAM(lParam);
	  y = GET_Y_LPARAM(lParam);
	  TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
	  SendMessage(GetParent(hwnd), WM_RBUTTONUP, wParam, MAKELPARAM(x, y));
	  break;
  case WM_MBUTTONDOWN:
	  x = GET_X_LPARAM(lParam);
	  y = GET_Y_LPARAM(lParam);
	  TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
	  SendMessage(GetParent(hwnd), WM_MBUTTONDOWN, wParam, MAKELPARAM(x, y));
	  break;
  case WM_MBUTTONUP:
	  x = GET_X_LPARAM(lParam);
	  y = GET_Y_LPARAM(lParam);
	  TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
	  SendMessage(GetParent(hwnd), WM_MBUTTONUP, wParam, MAKELPARAM(x, y));
	  break;
  case WM_MOUSEMOVE:
	  x = GET_X_LPARAM(lParam);
	  y = GET_Y_LPARAM(lParam);
	  TranslateCoords(hwnd, GetParent(hwnd), &x, &y);
	  SendMessage(GetParent(hwnd), WM_MOUSEMOVE, wParam, MAKELPARAM(x, y));
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

static void render(BBX_Label *lab)
{
  unsigned char *buff;
  int font_height;
  int Nlines;
  int i;
  char *line;
  char *end;
  int len;
  int msg_x, msg_y, msg_len;
  int width, height;
  RECT rect;

  DeleteObject(lab->img);
  GetClientRect(lab->win, &rect);
  
  width = rect.right - rect.left;
  height = rect.bottom - rect.top;

  buff = malloc(width * height * 4);
  for(i=0;i<width*height;i++)
  {
    buff[i*4] = bbx_red(lab->bgcol);
    buff[i*4+1] = bbx_green(lab->bgcol);
    buff[i*4+2] = bbx_blue(lab->bgcol);
    buff[i*4+3] = 0xFF;
  }
         
  font_height = lab->font->ascent + lab->font->descent;

  Nlines = getNlines(lab->text);
  line = lab->text;
  end = strchr(line, '\n');
     
  for(i=0;i<Nlines;i++)
  {
    if(end)
      len = end - line;
    else
      len = strlen(line);
    msg_len = bbx_textwidth(lab->font, line, len);
        
    if(lab->align == BBX_ALIGN_CENTER)
       msg_x  = (width - msg_len) / 2;
    else if(lab->align == BBX_ALIGN_RIGHT)
       msg_x = width - msg_len;
     else
        msg_x = 0;
      msg_y  = i*font_height + lab->font->ascent + (height - Nlines *font_height)/2; 

     bbx_drawutf8(buff, width, height, msg_x, msg_y, line, len, lab->font, lab->fgcol);
     if(end != 0)
     {
       line = end + 1;
       end = strchr(line, '\n');
     }
   }
  lab->img = MakeBitmap(buff, width, height);
  free(buff);
 
}

static HBITMAP MakeBitmap(unsigned char *rgba, int width, int height)
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

	return answer;
}

static int getNlines(char *str)
{
  int answer = 0;
  int i;

  for(i=0;str[i];i++)
    if(str[i] == '\n')
      answer++;
  if(i > 0 && str[i-1] != '\n')
    answer++;
  return answer;
}


