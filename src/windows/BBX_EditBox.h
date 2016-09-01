#ifndef bbx_editbox_h
#define bbx_editbox_h

typedef BBX_Panel BBX_EditBox;

BBX_EditBox *bbx_editbox(BABYX *bbx, BBX_Panel *parent, void (*fptr)(void *ptr, char *text), void *ptr);
BBX_EditBox *BBX_editbox(BABYX *bbx, HWND parent, void (*fptr)(void *ptr, char *text), void *ptr);
void bbx_editbox_kill(BBX_EditBox *obj);
void bbx_editbox_settext(BBX_EditBox *obj, char *text);
char *bbx_editbox_gettext(BBX_EditBox *obj);
void bbx_editbox_disable(BBX_EditBox *obj);
void bbx_editbox_enable(BBX_EditBox *obj);
void bbx_editbox_setfont(BBX_EditBox *obj, struct bitmap_font *font);

#endif


