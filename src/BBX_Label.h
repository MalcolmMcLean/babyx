#ifndef bbx_label_h
#define bbx_label_h

#define BBX_ALIGN_CENTER 1
#define BBX_ALIGN_LEFT 2
#define BBX_ALIGN_RIGHT 3

typedef struct
{
  BABYX *bbx;
  Window win;
  void (*event_handler)(void *obj, XEvent *event);
  int (*message_handler)(void *obj, int message, int a, int b, void *params);
  /* private stuff */
  char *text;
  BBX_RGBA fgcol;
  BBX_RGBA bgcol;
  struct bitmap_font *font;
  int align;
  XImage *img;
} BBX_Label;

BBX_Label *bbx_label(BABYX *bbx, BBX_Panel *parent, char *text);
BBX_Label *BBX_label(BABYX *bbx, Window parent, char *text);
void bbx_label_kill(BBX_Label *obj);
void bbx_label_settext(BBX_Label *obj, char *text);
void bbx_label_setalignment(BBX_Label *obj, int align);
void bbx_label_setbackground(BBX_Label *obj, BBX_RGBA col);
void bbx_label_setforeground(BBX_Label *obj, BBX_RGBA col);
int bbx_label_getpreferredsize(BBX_Label *lab, int *width, int *height);
void bbx_label_setfont(BBX_Label *obj, struct bitmap_font *font);

#endif

