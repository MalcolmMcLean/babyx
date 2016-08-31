#ifndef bbx_scrollbar_h
#define bbx_scrollbar_h

#define BBX_SCROLLBAR_VERTICAL 1
#define BBX_SCROLLBAR_HORIZONTAL 2


typedef struct
{
  BABYX *bbx;
  HWND win;
  void (*event_handler)(void *obj);
  int (*message_handler)(void *obj, int message, int a, int b, void *params);
  /* private stuff */
  void (*fptr)(void *ptr, int pos);
  void *ptr;
  int direction;
  int lastx;
  int lasty;
  int maxx;
  int maxy;
  int ongrab;
  int width;
  int height;
} BBX_ScrollThumb;

typedef struct
{
  BABYX *bbx;
  HWND win;
  void (*event_handler)(void *obj);
  int (*message_handler)(void *obj, int message, int a, int b, void *params);
  /* private stuff */
  void (*change)(void *ptr, int pos);
  void *ptr;
  int direction;
  int range;
  int visible;
  int pos;
  BBX_ScrollThumb *thumb;
  int thumbwidth;
  int thumbheight;
} BBX_Scrollbar;

ATOM BBX_RegisterScrollbar(HINSTANCE hInstance);
BBX_Scrollbar *bbx_scrollbar(BABYX *bbx, BBX_Panel *parent, int direction, void (*fptr)(void *ptr, int pos), void *ptr);
BBX_Scrollbar *BBX_scrollbar(BABYX *bbx, HWND parent, int direction, void (*fptr)(void *ptr, int pos), void *ptr);
void bbx_scrollbar_kill(BBX_Scrollbar *obj);
BBX_ScrollThumb *bbx_scrollthumb(BABYX *bbx, HWND parent, int direction, void (*fptr)(void *ptr, int pos), void *ptr);
void bbx_scrollthumb_kill(BBX_ScrollThumb *obj);

int bbx_scrollbar_set(BBX_Scrollbar *obj, int range, int visible, int pos);
int bbx_scrollbar_getpos(BBX_Scrollbar *obj);
void bbx_scrollbar_setpos(BBX_Scrollbar *obj, int pos);

#endif
