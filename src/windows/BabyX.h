#ifndef babyx_h
#define babyx_h

#include <Windows.h>
#include <windowsx.h>

#include "font.h"
#include "BBX_Color.h"
#include "bbx_graphicssupport.h"
#include "BBX_Clipboard.h"

typedef struct
{
  void *ptr;
  HWND window;
  void (*event_handler)(void *ptr);
  int (*message_handler)(void *ptr, int message, int a, int b, void *params);
} BBX_CHILD;

typedef struct bbx_ticker
{
  struct bbx_ticker *next;
  int interval;
  int modelevel;
  FILETIME tick;  
  void (*fptr)(void *ptr);
  void *ptr;
} BBX_TICKER;

typedef struct
{
  HINSTANCE hinstance;
  BBX_CHILD *child;
  BBX_TICKER *ticker;
  BBX_Clipboard *clipboard;
  int Nchildren;
  int childCapacity;
  int screen;
  struct bitmap_font *gui_font;
  struct bitmap_font *user_font2;
  int modalpush;
  int running;
} BABYX;

BABYX *BabyX();
void killbabyx(BABYX *bbx);

void *bbx_addticker(BABYX *bbx, int ms_interval, void (*fptr)(void *ptr), void *ptr);
void bbx_removeticker(BABYX *bbx, void *ticker);
int bbx_setpos(BABYX *bbx, void *obj, int x, int y, int width, int height);
int bbx_setsize(BABYX *bbx, void *obj, int width, int height);
int bbx_getsize(BABYX *bbx, void *obj, int *width, int *height);
int bbx_setfocus(BABYX *bbx, void *obj);

void *bbx_malloc(int size);
void *bbx_realloc(void *ptr, int size);
char *bbx_strdup(const char *str);

int bbx_kbhit(BABYX *bbx, int code);

BBX_RGBA bbx_color(char *str);

void bbx_copytexttoclipboard(BABYX *bbx, char *text);
char *bbx_gettextfromclipboard(BABYX *bbx);

//int BBX_Event(BABYX *bbx, XEvent *event);
void BBX_Tick(BABYX *bbx);
int BBX_MakeModal(BABYX *bbx, HWND win);
int BBX_DropModal(BABYX *bbx);
int BBX_Register(BABYX *bbx, HWND win, void (*event_handler)(void *ptr), int (*message_handler)(void *ptr, int message, int a, int b, void *params), void *ptr);
int BBX_Deregister(BABYX *bbx, HWND win);
int BBX_IsDescendant(BABYX *bbx, HWND ancestor, HWND descendant);
void BBX_InvalidateWindow(BABYX *bbx, HWND win);
unsigned long BBX_Color(char *str);

/* mouse actions */
#define BBX_MOUSE_CLICK 1
#define BBX_MOUSE_MOVE 2
#define BBX_MOUSE_RELEASE 3

/* mouse button states */
#define BBX_MOUSE_BUTTON1 1
#define BBX_MOUSE_BUTTON2 2
#define BBX_MOUSE_BUTTON3 4

/* Messages */
#define BBX_RESIZE 100


#include "font.h"
#include "BBX_Font.h"
#include "BBX_Panel.h"
#include "BBX_DialogPanel.h"
#include "BBX_PopupPanel.h"
#include "BBX_Label.h"
#include "BBX_Canvas.h"
#include "BBX_Button.h"
#include "BBX_Scrollbar.h"
#include "BBX_LineEdit.h"
#include "BBX_EditBox.h"
#include "BBX_Popup.h"
#include "BBX_Menubar.h"
#include "BBX_ListBox.h"
#include "BBX_Spinner.h"
#include "BBX_MessageBox.h"
#include "BBX_CheckBox.h"
#include "BBX_RadioBox.h"
#include "BBX_ScrollPanel.h"
#include "BBX_FilePicker.h"

void startbabyx(
				char *name, 
                int width, 
                int height,
		void (*create)(void *ptr, BABYX *bbx, BBX_Panel *root),
                void (*layout)(void *ptr, int width, int height),
                void *ptr);
void stopbabyx(BABYX *bbx);


#endif
