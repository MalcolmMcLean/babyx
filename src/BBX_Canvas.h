#ifndef BBX_Canvas_h
#define BBX_Canvas_h

typedef struct
{
  BABYX *bbx;
  Window win;
  void (*event_handler)(void *obj, XEvent *event);
  int (*message_handler)(void *obj, int message, int a, int b, void *params);
  /* private stuff */
  void (*mousefunc)(void *ptr, int action, int x, int y, int buttons);
  void *ptr;
  unsigned char *rgba;
  int width;
  int height;
  BBX_RGBA background;

  XImage *img;
} BBX_Canvas;

BBX_Canvas *bbx_canvas(BABYX *bbx, BBX_Panel *parent, int width, int height, BBX_RGBA bg);
void bbx_canvas_kill(BBX_Canvas *can);
BBX_Canvas *BBX_canvas(BABYX *bbx, Window parent, int width, int height, BBX_RGBA bg);
void bbx_canvas_setimage(BBX_Canvas *can, unsigned char *rgba, int width, int height);
unsigned char *bbx_canvas_rgba(BBX_Canvas *can, int *width, int *height);
void bbx_canvas_flush(BBX_Canvas *can);
void bbx_canvas_setmousefunc(BBX_Canvas *can, void (*mousefunc)(void *ptr, int action, int x, int y, int buttons), void *ptr);

#endif
