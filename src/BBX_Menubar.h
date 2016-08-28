#ifndef bbx_menubar_h
#define bbx_menubar_h

#if 0
typedef struct
{
  BABYX *bbx;
  Window win;
  void (*event_handler)(void *obj, XEvent *event);
  int (*message_handler)(void *obj, int message, int a, int b, void *params);

  /* private stuff */
  void (*fptr)(void *ptr, int id);
  void *ptr;
  int N;
  char **str;
  int *xpos;
  BBX_Popup **sub;
} BBX_Menubar;

#endif

typedef BBX_Panel BBX_Menubar;

BBX_Menubar *bbx_menubar(BABYX *bbx, BBX_Panel *parent, void (*fptr)(void *ptr, int d), void *ptr);
BBX_Menubar *BBX_menubar(BABYX *bbx, HWND parent, void (*fptr)(void *ptr, int d), void *ptr);
void bbx_menubar_kill(BBX_Menubar *mb);
void bbx_menubar_addmenu(BBX_Menubar *mb, char *name, BBX_Popup *sub);
void bbx_menubar_disable(BBX_Menubar *mb, int id);
void bbx_menubar_enable(BBX_Menubar *mb, int id);
#endif
