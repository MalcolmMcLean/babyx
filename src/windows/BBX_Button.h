#ifndef bbx_button_h
#define bbx_button_h

#if 0
typedef struct
{
  BABYX *bbx;
  Window win;
  void (*event_handler)(void *obj, XEvent *event);
  int (*message_handler)(void *obj, int message, int a, int b, void *params);
  /* private stuff */
  void (*clickme)(void *ptr);
  void *ptr;
  char *text;
  int disabled;
} BBX_Button;

#endif

typedef BBX_Panel BBX_Button;

BBX_Button *bbx_button(BABYX *bbx, BBX_Panel *parent, char *text, void (*fptr)(void *ptr), void *ptr );
BBX_Button *BBX_button(BABYX *bbx, HWND parent, char *str, void (*fptr)(void *ptr), void *ptr);

void bbx_button_kill(BBX_Button *obj);
void bbx_button_settext(BBX_Button *obj, char *str);
void bbx_button_disable(BBX_Button *obj);
void bbx_button_enable(BBX_Button *obj);

#endif

