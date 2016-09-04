#ifndef babyx_h
#define babyx_h

#include <sys/time.h>
#include <X11/Xlib.h>

#include "font.h"
#include "BBX_Color.h"
#include "../common/bbx_graphicssupport.h"
#include "BBX_Clipboard.h"

typedef struct
{
  void *ptr;
  Window window;
  void (*event_handler)(void *ptr, XEvent *event);
  int (*message_handler)(void *ptr, int message, int a, int b, void *params);
} BBX_CHILD;

typedef struct bbx_ticker
{
  struct bbx_ticker *next;
  int interval;
  int modelevel;
  struct timeval tick;  
  void (*fptr)(void *ptr);
  void *ptr;
} BBX_TICKER;

typedef struct
{
  Display *dpy;
  BBX_CHILD *child;
  BBX_TICKER *ticker;
  BBX_Clipboard *clipboard;
  int Nchildren;
  int childCapacity;
  int screen;
  struct bitmap_font *gui_font;
  XFontStruct *user_font;
  struct bitmap_font *user_font2;
  GC gc;  
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

void *bbx_malloc(int size);
void *bbx_realloc(void *ptr, int size);
char *bbx_strdup(const char *str);

BBX_RGBA bbx_color(char *str);

void bbx_copytexttoclipboard(BABYX *bbx, char *text);
char *bbx_gettextfromclipboard(BABYX *bbx);

int BBX_Event(BABYX *bbx, XEvent *event);
void BBX_Tick(BABYX *bbx);
int BBX_MakeModal(BABYX *bbx, Window win);
int BBX_DropModal(BABYX *bbx);
int BBX_Register(BABYX *bbx, Window win, void (*event_handler)(void *ptr, XEvent *event), int (*message_handler)(void *ptr, int message, int a, int b, void *params), void *ptr);
int BBX_Deregister(BABYX *bbx, Window win);
int BBX_IsDescendant(BABYX *bbx, Window ancestor, Window descendant);
void BBX_InvalidateWindow(BABYX *bbx, Window win);
unsigned long BBX_Color(char *str);

/* mouse actions */
#define BBX_MOUSE_CLICK 1
#define BBX_MOUSE_MOVE 2
#define BBX_MOUSE_RELEASE 3

/* mouse button states */
#define BBX_MOUSE_BUTTON1 1
#define BBX_MOUSE_BUTTON2 2
#define BBX_MOUSE_BUTTON3 4

#include "font.h"
#include "../common/BBX_Font.h"
#include "BBX_Panel.h"
#include "BBX_DialogPanel.h"
#include "BBX_PopupPanel.h"
#include "BBX_Label.h"
#include "BBX_Canvas.h"
#include "../common/BBX_Button.h"
#include "BBX_Scrollbar.h"
#include "../common/BBX_LineEdit.h"
#include "../common/BBX_EditBox.h"
#include "BBX_Popup.h"
#include "../common/BBX_Menubar.h"
#include "../common/BBX_ListBox.h"
#include "../common/BBX_Spinner.h"
#include "../common/BBX_MessageBox.h"
#include "../common/BBX_CheckBox.h"
#include "../common/BBX_RadioBox.h"
#include "BBX_ScrollPanel.h"
#include "BBX_FilePicker.h"

void startbabyx(char *name, 
                int width, 
                int height,
		void (*create)(void *ptr, BABYX *bbx, BBX_Panel *root),
                void (*layout)(void *ptr, int width, int height),
                void *ptr);
void stopbabyx(BABYX *bbx);


#endif
