#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#define CONM_ADDTEXT 0x8001
#define CONM_SETCOL  0x8002

typedef struct
{
  int topline;     // top line selected
  int topcolumn;   // top column selected
  int botline;     // bottom line selected
  int botcolumn;   // bottom column selected
  int topfixed;    // is the top the fixed point of the selection?
  int active;      // is the selection active?
  int mousegrabbed;// is the mouse grabbed
} SELECTION;

typedef struct colourchange
{
  int xpos;
  COLORREF col;
  struct colourchange *next;
} COLOURCHANGE;

typedef struct
{
  char *text;
  COLOURCHANGE *colourchange;
  COLORREF startcol;
  int ret;
} LINE;

typedef struct
{
  LINE *lines;
  int capacity;
  int nlines;
  int pos;
  int cursor;
  COLORREF colour;
  int width;
  int height;
  int fntwidth;
  int fntheight;
  SELECTION selection;
} TEXT;

static HWND hconsole = 0;


static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static TEXT *createtext(int capacity);
static void killtext(TEXT *text);
static void killcolchange(COLOURCHANGE *col);
static void paintme(HWND hwnd, TEXT *text);
static void paintselection(HDC hdc, TEXT *text);
static int lineselected(TEXT *text, int i);
static void getlineselection(TEXT *text, int i, int *start, int *end);
static void addwordwrap(HWND hwnd, TEXT *text, char *str);
static void addtext(HWND hwnd, TEXT *text, char *str);
static void scrollaline(TEXT *text);
static void ScrollMessage(HWND hwnd, TEXT *text, int msg, int val);
static void DoScrollBar(HWND hwnd, TEXT *text);
static void MouseMove(HWND hwnd, TEXT *text, WPARAM wParam, LPARAM lParam);
static void StartSelection(HWND hwnd, TEXT *text, int x, int y);
static void SelectText(TEXT *text, int x, int y);
static void CopyToClipboard(HWND hwnd, TEXT *text);
static void getselection(TEXT *text, char *buff);
static void xytolinecol(TEXT *text, int x, int y, int *line, int *col);
static void changecolour(TEXT *text, COLORREF col);
static COLORREF currentcolour(TEXT *text);

static char **strwrap(char *str, int x, int width, int *N);
static void killlist(char **list, int N);
static void getfontmetrics(HWND hwnd, TEXT *text, HFONT hfont);
static int firstwordlen(char *str);
static char *getline(char *str);
static char *mystrdup(char *str);



void RegisterConsole(HINSTANCE hInstance)
{
  WNDCLASSEX wndclass;

  wndclass.cbSize = sizeof(WNDCLASSEX);
  wndclass.style = CS_HREDRAW | CS_VREDRAW;
  wndclass.lpfnWndProc = WndProc;
  wndclass.cbClsExtra = 0;
  wndclass.cbWndExtra = 0;
  wndclass.hInstance = hInstance;
  wndclass.hIcon = LoadIcon(0, IDI_APPLICATION);
  wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
  wndclass.hbrBackground = CreateSolidBrush( RGB(0, 0, 0) );// (HBRUSH) (COLOR_MENU + 1);
  wndclass.lpszMenuName = 0;
  wndclass.lpszClassName = "MyConsole";
  wndclass.hIconSm = LoadIcon(0, IDI_APPLICATION);

  RegisterClassEx(&wndclass);
}

void MakeConsole(HINSTANCE hInstance)
{
  RegisterConsole(hInstance);
  
  hconsole = CreateWindowEx(
	  WS_EX_CLIENTEDGE,
	  "MyConsole",
	  "Console",
	  WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU | WS_CAPTION | WS_VSCROLL,
	  CW_USEDEFAULT,
	  CW_USEDEFAULT,
	  16 * 20 + 20,
	  16 * 15,
	  NULL,
	  NULL,
	  hInstance,
	  0
	  );

  ShowWindow(hconsole, SW_SHOWNORMAL);
  UpdateWindow(hconsole);
}

void Con_Printf(char *fmt, ...)
{
  //char buff[1024];
  va_list args;
  char *buff = malloc(1024 * 100);
  
  va_start(args, fmt);
  vsprintf(buff, fmt, args);
  SendMessage(hconsole, CONM_ADDTEXT, 0, (long) buff);
  va_end(args);
  free(buff);
}

void Win_Printf(HWND hwnd, char *fmt, ...)
{
  char buff[1024];
  va_list args;

  assert( GetWindowLong(hwnd, GWL_WNDPROC) == (long) WndProc );
  
  va_start(args, fmt);
  vsprintf(buff, fmt, args);
  SendMessage(hwnd, CONM_ADDTEXT, 0, (long) buff);
  va_end(args);
}

COLORREF Con_SetCol(HWND hwnd, COLORREF col)
{
  assert( GetWindowLong(hwnd, GWL_WNDPROC) == (long) WndProc );

  return (COLORREF) SendMessage(hwnd, CONM_SETCOL, 0, (long) col);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  TEXT *text;
  COLORREF prevcol;

  text = (TEXT *) GetWindowLong(hwnd, GWL_USERDATA);
  switch(msg)
  {
    case WM_CREATE:
	  ShowScrollBar(hwnd, SB_VERT, TRUE);
	  text = createtext(1024);
	  SetWindowLong(hwnd, GWL_USERDATA, (long) text);
	  getfontmetrics(hwnd, text, GetStockObject(ANSI_FIXED_FONT));
	  DoScrollBar(hwnd, text);
	  break;
    case WM_DESTROY:
	  killtext(text);
	  break;
	case WM_PAINT:
	  paintme(hwnd, text);
	  break;
	case WM_VSCROLL:
      ScrollMessage(hwnd, text, LOWORD(wParam), HIWORD(wParam));
	  break;
	case WM_LBUTTONDOWN:
	  StartSelection(hwnd, text, LOWORD(lParam), HIWORD(lParam));
	  break;
	case WM_LBUTTONUP:
	  if(text->selection.mousegrabbed)
	      ReleaseCapture();
	  text->selection.mousegrabbed = 0;
	  break;
	case WM_MOUSEMOVE:
	  MouseMove(hwnd, text, wParam, lParam);
	  break;
	case WM_KEYDOWN:
	  if(wParam == 'C' && GetKeyState(VK_CONTROL))
		CopyToClipboard(hwnd, text);
	  break;
	case CONM_ADDTEXT:
	  addwordwrap(hwnd, text, (char *) lParam);
	  break;
	case CONM_SETCOL:
	  prevcol = text->colour;
	  text->colour = lParam;
      return prevcol;
	  break;
  }
  return DefWindowProc(hwnd, msg, wParam, lParam);
}

static TEXT *createtext(int capacity)
{
  TEXT *answer;
  int i;

  answer = malloc(sizeof(TEXT));
  answer->lines = malloc(capacity * sizeof(LINE));
  for(i=0;i<capacity;i++)
  {
    answer->lines[i].text = 0;
	answer->lines[i].colourchange = 0;
	answer->lines[i].startcol = RGB(0,255,0);
	answer->lines[i].ret = 0;
  }
  answer->capacity = capacity;
  answer->nlines = 0;
  answer->pos = 0;
  answer->cursor = 0;
  answer->colour = RGB(0,255,0);
  
  answer->width = 0;
  answer->height = 0;

  answer->selection.active = 0;
  answer->selection.mousegrabbed = 0;

  return answer;
}

static void killtext(TEXT *text)
{
  int i;

  for(i=0;i<text->nlines;i++)
  {
    if(text->lines[i].colourchange)
	  killcolchange(text->lines[i].colourchange);
	if(text->lines[i].text)
	  free(text->lines[i].text);
  }
  free(text->lines);
  free(text);
}

static void killcolchange(COLOURCHANGE *col)
{
  if(col->next)
    killcolchange(col->next);
  free(col);
}

static void paintme(HWND hwnd, TEXT *text)
{
  PAINTSTRUCT ps;
  HDC hdc;
  int i;
  COLOURCHANGE *cc;
  char *substr;
  int len;

  hdc = BeginPaint(hwnd, &ps);
  
  SetBkMode(hdc, TRANSPARENT);
  SelectObject(hdc, GetStockObject(ANSI_FIXED_FONT));

  for(i=text->pos;i<text->pos + text->height;i++)
  {
    if(text->lines[i].text)
	{
	  SetTextColor(hdc, text->lines[i].startcol);
	  if(!text->lines[i].colourchange)
	    TextOut(hdc, 0, (i - text->pos) * text->fntheight, text->lines[i].text, strlen(text->lines[i].text));
	  else
	  {
	    /* line has colour changes */
		TextOut(hdc, 0, (i - text->pos) *text->fntheight, text->lines[i].text, text->lines[i].colourchange->xpos);
        cc = text->lines[i].colourchange;
		while(cc)
		{
		  SetTextColor(hdc, cc->col);
          substr = text->lines[i].text + cc->xpos;
		  if(cc->next)
		    len = cc->next->xpos - cc->xpos;
		  else
			len = strlen(text->lines[i].text) - cc->xpos;
		  TextOut(hdc, text->fntwidth * cc->xpos, (i - text->pos) *text->fntheight, substr, len);  
  	      cc = cc->next;
		}
	  }
	}
  }

  if(text->selection.active)
  {
	paintselection(hdc, text);
  }

  EndPaint(hwnd, &ps);
}

static void paintselection(HDC hdc, TEXT *text)
{
  int i;
  int start;
  int end;
  char *str;
  int x;
  int y;

  SetBkMode(hdc, OPAQUE);
  SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
  SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));

  for(i=text->pos;i<text->pos + text->height;i++)
  {
	if(text->lines[i].text)
	{
      if(lineselected(text, i))
	  {
        getlineselection(text, i, &start, &end);
	    str = text->lines[i].text + start;
		x = start * text->fntwidth;
		y = (i - text->pos) * text->fntheight;
	    TextOut(hdc, x, y, str, end - start);
	  }
	}
  }
}

static int lineselected(TEXT *text, int i)
{
  if(i >= text->selection.topline && i <= text->selection.botline)
	return 1;
  else 
    return 0;
}

static void getlineselection(TEXT *text, int i, int *start, int *end)
{
  if(text->selection.topline == i)
	  *start = text->selection.topcolumn;
  else 
	  *start = 0;

  if(text->selection.botline == i)
	  *end = text->selection.botcolumn + 1;
  else
	  *end = strlen( text->lines[i].text );
}

static void addwordwrap(HWND hwnd, TEXT *text, char *str)
{
  char *block;
  char **lines;
  int N;
  int i;

  while(*str)
  {
    block = getline(str);
    lines = strwrap(block, text->cursor, text->width -1, &N);
    for(i=0;i<N;i++)
	{
      addtext(hwnd, text, lines[i]);
	}
	str += strlen(block);
	if(*str == '\n')
	{
	  addtext(hwnd, text, "\n");
	  text->lines[text->nlines-1].ret = 1;
	  str++;
	}
	killlist(lines, N);
	free(block);
  }

  DoScrollBar(hwnd, text);
  if(text->nlines < text->height)
    InvalidateRect(hwnd, 0, 0);
  else
	InvalidateRect(hwnd, 0, 1);
  UpdateWindow(hwnd);
}

static void addtext(HWND hwnd, TEXT *text, char *str)
{
  /* see if a colourchange is in effect */
  if(*str && *str != '\n' && text->cursor)
  {
    if(currentcolour(text) != text->colour)
		changecolour(text, text->colour);
  }
  while(*str)
  {
    if(text->cursor == 0)
    {
	  text->lines[text->nlines].text = malloc(text->width + 1);
	  text->lines[text->nlines].startcol = text->colour;
	  memset(text->lines[text->nlines].text, 0, text->width + 1);
    }
	if(*str != '\n')
	{
      text->lines[text->nlines].text[text->cursor++] = *str++;
	  if(text->cursor == text->width)
		text->cursor = 0;
    }
	else
	{
	  str++;
	  text->cursor = 0;
	  //text->lines[text->nlines].ret = 1;
    }
	if(text->cursor == 0)
    {
      if(text->nlines == text->capacity - 1)
		scrollaline(text);
	  else
		text->nlines++;
	}
  }
  text->pos = text->nlines - text->height;
  if(text->pos < 0)
	text->pos = 0;
}

static void scrollaline(TEXT *text)
{
  if(text->lines[0].text)
	free(text->lines[0].text);
  if(text->lines[0].colourchange)
	killcolchange(text->lines[0].colourchange);

  memmove(&text->lines[0], &text->lines[1], sizeof(LINE) *(text->capacity - 1)); 
  text->lines[text->capacity - 1].text = 0;
  text->lines[text->capacity - 1].colourchange = 0;
  text->lines[text->capacity - 1].startcol = text->colour;
  text->lines[text->capacity - 1].ret = 0;

  text->selection.topline -= 1;
  text->selection.botline -= 1;
  if(text->selection.topline < 0)
	  text->selection.topline = 0;
  if(text->selection.topline > text->selection.botline || (text->selection.topline == text->selection.botline && text->selection.topcolumn > text->selection.botcolumn) )
	  text->selection.active = 0;
}


static void ScrollMessage(HWND hwnd, TEXT *text, int msg, int val)
{
  switch(msg) 
  { 
    // User clicked the shaft above the scroll box. 
 
    case SB_PAGEUP: 
      text->pos -= text->height; 
      break; 
 
    // User clicked the shaft below the scroll box. 
 
    case SB_PAGEDOWN: 
      text->pos += text->height; 
      break; 
 
    // User clicked the top arrow. 
 
    case SB_LINEUP: 
      text->pos--; 
      break; 
 
   // User clicked the bottom arrow. 
 
    case SB_LINEDOWN: 
      text->pos++; 
      break; 
 
   // User dragged the scroll box. 
 
   case SB_THUMBTRACK: 
     text->pos = val; 
     break; 
  }
  
  if(text->pos > text->nlines - text->height)
	text->pos = text->nlines - text->height;
  if(text->pos < 0)
	text->pos = 0;

  DoScrollBar(hwnd, text);
  InvalidateRect(hwnd, 0, 1);
  UpdateWindow(hwnd);
}
 
static void DoScrollBar(HWND hwnd, TEXT *text)
{
  SCROLLINFO si; 
  
  si.cbSize = sizeof(si); 
  si.fMask  = SIF_RANGE | SIF_POS | SIF_PAGE; 
  si.nMin   = 0; 
  si.nMax   = text->nlines -1; 
  si.nPage  = text->height; 
  si.nPos   = text->pos; 
        
  SetScrollInfo(hwnd, SB_VERT, &si, 1); 
  ShowScrollBar(hwnd, SB_VERT, TRUE);
}

static void MouseMove(HWND hwnd, TEXT *text, WPARAM wParam, LPARAM lParam)
{
  int x;
  int y;
  RECT rect;
  POINT pt;

  if( (wParam &MK_LBUTTON) == 0)
	return;

  GetClientRect(hwnd, &rect);

  x = LOWORD(lParam);
  y = HIWORD(lParam);
  if( x & 0x8000 )
	  x |= ( -1 << 16);
  if( y & 0x8000 )
	  y |= ( -1 << 16);

  if(y < 0)
  {
    if(text->pos)
		text->pos--;
	DoScrollBar(hwnd, text);
	pt.x = x;
	pt.y = 0;
	ClientToScreen(hwnd, &pt);
	SetCursorPos(pt.x, pt.y);
  }
  else if(y > rect.bottom)
  {
    if(text->pos < text->nlines - text->height)
	  text->pos++;
	DoScrollBar(hwnd, text);
	pt.x = x;
	pt.y = rect.bottom;
	ClientToScreen(hwnd, &pt);
	SetCursorPos(pt.x, pt.y);
  }

  SelectText(text, x, y);

  InvalidateRect(hwnd, 0, TRUE);
  UpdateWindow(hwnd);
}

static void StartSelection(HWND hwnd, TEXT *text, int x, int y)
{
  if(text->selection.active)
  {
    text->selection.active = 0;
	InvalidateRect(hwnd, 0, TRUE);
	UpdateWindow(hwnd);
  }
  text->selection.topfixed = 1;
  xytolinecol(text, x, y, &text->selection.topline, &text->selection.topcolumn);
  if(y == -1)
	  return;
  SetCapture(hwnd);
  text->selection.mousegrabbed = 1;
  SetFocus(hwnd);
}


static void CopyToClipboard(HWND hwnd, TEXT *text)
{
  int buffsize;
  char *buff;

  if(text->selection.active)
  {
	if( OpenClipboard(hwnd) == 0)
		return;

    buffsize = (text->selection.botline - text->selection.topline + 1) * (text->width + 1);
   
    buff = GlobalAlloc(GMEM_DDESHARE, buffsize);
	if(buff)
	{
      getselection(text, buff);

      EmptyClipboard();
      SetClipboardData(CF_TEXT, buff);
	}
    CloseClipboard();
  }
}

static void getselection(TEXT *text, char *buff)
{
  char *ptr;
  int i;
  int start;
  int end;

  ptr = buff;

  for(i=text->selection.topline; i <= text->selection.botline; i++)
  {
	if(!text->lines[i].text)
	  continue;
	if(text->lines[i].text[0] == 0)
	  continue;

    if(i == text->selection.topline)
	  start = text->selection.topcolumn;
	else
	  start = 0;
	if(i == text->selection.botline)
	  end = text->selection.botcolumn + 1;
	else
	  end = strlen(text->lines[i].text);

	strncpy(ptr, text->lines[i].text + start, end - start);
	ptr += end - start;
	if(!text->lines[i].ret)
	  strcat(ptr, " ");
	else
	  strcat(ptr, "\n");
	ptr++;
  }
  *ptr = 0;
}

static void xytolinecol(TEXT *text, int x, int y, int *line, int *col)
{
  if(text->nlines == 0 && text->cursor == 0)
  {
    *line = -1;
	*col = -1;
	return;
  }

  *line = y / text->fntheight + text->pos;
  *col = x / text->fntwidth;

  if(*line > text->nlines)
    *line = text->nlines;
  if(*line < 0)
	*line = 0;

  if(*col < 0)
	  *col = 0;
  if(text->lines[*line].text)
  {
    if(*col >= (int) strlen(text->lines[*line].text))
	  *col = (int) strlen(text->lines[*line].text) -1;
  }
  else
	*col = 0;

  assert(*line <= text->nlines);
  assert(*col < text->width);
}

static void changecolour(TEXT *text, COLORREF col)
{
  COLOURCHANGE *cc;
  COLOURCHANGE *newcol;

  newcol = malloc(sizeof(COLOURCHANGE));
  newcol->col = col;
  newcol->next = 0;
  newcol->xpos = text->cursor; 
  cc = text->lines[text->nlines].colourchange;
  if(cc)
  {
    while(cc->next)
	  cc = cc->next;
	cc->next = newcol;
  }
  else
	text->lines[text->nlines].colourchange = newcol;
}

/*
  Gets the currently set colour at the end of the line 
  Params: text - text structure
  Returns: the colour current.
*/
static COLORREF currentcolour(TEXT *text)
{
  COLOURCHANGE *cc;

  if(text->lines[text->nlines].colourchange)
  {
    cc = text->lines[text->nlines].colourchange;
	while(cc->next)
	  cc = cc->next;
	return cc->col;
  }

  else
    return text->lines[text->nlines].startcol;
}

static char **strwrap(char *str, int x, int width, int *N)
{
  char **answer = 0;
  int nlines = 0;
  char *last;
  int i;

  assert(str);

  answer = malloc(sizeof(char *));
  if(strlen(str) <= (unsigned) width - x)
  {
    answer[0] = mystrdup(str);
	*N = 1;
	return answer;
  }

  last = str + width - x;
  while(*last != ' ' && last > str)
	last--;
  if(last == str && firstwordlen(str) >= width)
	last = str + width - x;
  answer[0] = malloc(last - str + 2);
  memcpy(answer[0], str, last - str);
  answer[0][last-str] = 0;
  nlines = 1;
  str = last;
  if(isspace(*str))
    str++;

  while(strlen(str) > (unsigned) width)
  {
    nlines++;
	answer = realloc(answer, nlines * sizeof(char *));
    last = str + width;
	while(*last != ' ' && last > str)
	  last--;
	if(last == str)
	  last = str + width;
    answer[nlines-1] = malloc(last - str + 2);
    memcpy(answer[nlines-1], str, last - str);
	answer[nlines - 1][last-str] = 0;

	str = last;
	if(isspace(*str))
	  str++;
  }
  if(*str)
  {
	nlines++;
	answer = realloc(answer, nlines * sizeof(char *));
	answer[nlines-1] = mystrdup(str);
  }

  *N = nlines;

  for(i=0;i<nlines-1;i++)
	strcat(answer[i], "\n");

  return answer;
}

static void killlist(char **list, int N)
{
  int i;

  for(i=0;i<N;i++)
	free(list[i]);

  free(list);
}

static void getfontmetrics(HWND hwnd, TEXT *text, HFONT hfont)
{
  HDC hdc;
  SIZE wsz;
  SIZE hsz;
  RECT rect;

  GetClientRect(hwnd, &rect);
  hdc = GetDC(hwnd);
  SelectObject(hdc, hfont);
  GetTextExtentPoint(hdc, "M", 1, &wsz);
  GetTextExtentPoint(hdc, "ly", 2, &hsz);
  ReleaseDC(hwnd, hdc);

  text->width = (rect.right - rect.left)/wsz.cx;
  text->height = (rect.bottom - rect.top)/hsz.cy;
  text->fntwidth = wsz.cx;
  text->fntheight = hsz.cy;
}

static int firstwordlen(char *str)
{
  int answer = 0;

  while(*str && !isspace(*str))
  {
    str++;
	answer++;
  }

  return answer;
}

static char *getline(char *str)
{
  char *ptr;
  char *answer;

  ptr = str;

  while(*ptr && *ptr != '\n')
	ptr++;
 
  answer = malloc(ptr - str + 1);
  memcpy(answer, str, ptr - str);
  answer[ptr - str] = 0;

  return answer;
}

static char *mystrdup(char *str)
{
  char *answer;

  answer = malloc(strlen(str) + 1);
  strcpy(answer, str);
  return answer;
}



