#ifndef bbx_checkbox_h
#define bbx_checkbox_h

typedef BBX_Panel BBX_CheckBox;
BBX_CheckBox *bbx_checkbox(BABYX *bbx, BBX_Panel *parent, char *text, void (*fptr)(void *ptr, int state), void *ptr );
BBX_CheckBox *BBX_checkbox(BABYX *bbx, Window parent, char *text, void (*fptr)(void *ptr, int state), void *ptr );
void bbx_checkbox_kill(BBX_CheckBox *obj);
int bbx_checkbox_getstate(BBX_CheckBox *obj);
int bbx_checkbox_setstate(BBX_CheckBox *obj, int checked);
void bbx_checkbox_enable(BBX_CheckBox *obj);
void bbx_checkbox_disable(BBX_CheckBox *obj);

#endif
