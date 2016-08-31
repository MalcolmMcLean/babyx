#ifndef bbx_panel_h
#define bbx_panel_h

#define BBX_KEY_BACKSPACE -100
#define BBX_KEY_DELETE -101
#define BBX_KEY_ESCAPE -102
#define BBX_KEY_HOME -103
#define BBX_KEY_LEFT -104
#define BBX_KEY_UP -105              
#define BBX_KEY_RIGHT -106       
#define BBX_KEY_DOWN -107             
#define BBX_KEY_END -108

#define BBX_KEY_GOTFOCUS -200
#define BBX_KEY_LOSTFOCUS -201

typedef struct
{
  BABYX *bbx;
  Window win;
  void (*event_handler)(void *obj, XEvent *event);
  int (*message_handler)(void *obj, int message, int a, int b, void *params);

  /* private stuff */
  char *tag;
  void (*changesize)(void *ptr, int width, int height);
  void *ptr;
  int width;
  int height;
  void (*mousefunc)(void *ptr, int action, int x, int y, int buttons);  
  void (*closefunc)(void *ptr);
  void (*keyfunc)(void *ptr, int ch);
} BBX_Panel;

BBX_Panel *bbx_panel(BABYX *bbx, BBX_Panel *parent, char *tag, void (*changesize)(void *ptr, int width, int height), void *ptr);
BBX_Panel *BBX_panel(BABYX *bbx, Window parent, char *tag, void (*changesize)(void *ptr, int width, int height), void *ptr);
void bbx_panel_kill(BBX_Panel *pan);
char *bbx_panel_gettag(BBX_Panel *pan);
void bbx_panel_setbackground(BBX_Panel *obj, BBX_RGBA col);
void bbx_panel_setmousefunc(BBX_Panel *pan, void (*mousefunc)(void *ptr, int action, int x, int y, int buttons), void *ptr);
void bbx_panel_setkeyfunc(BBX_Panel *pan, void (*keyfunc)(void *ptr, int ch), void *ptr);
void *bbx_panel_getptr(BBX_Panel *pan);
int bbx_panel_gotmouse(BBX_Panel *pan, int *x, int *y);

#endif
