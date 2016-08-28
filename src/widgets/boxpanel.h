#ifndef boxpanel_h
#define boxpanel_h

BBX_Panel *bbx_boxpanel(BABYX *bbx, BBX_Panel *parent, char *text, void (layout)(void *ptr, int width, int height), void *ptr);
void bbx_boxpanel_kill(BBX_Panel *obj);
void bbx_boxpanel_setalignment(BBX_Panel *obj, int alignment);
void bbx_boxpanel_setfont(BBX_Panel *obj, struct bitmap_font *font);
void bbx_boxpanel_settext(BBX_Panel *obj, char *text);

#endif
