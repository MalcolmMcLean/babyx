#ifndef bbx_radiobox_h
#define bbx_radiobox_h

typedef BBX_Panel BBX_RadioBox;
BBX_RadioBox *bbx_radiobox(BABYX *bbx, BBX_Panel *parent, char **text, int N, void (*fptr)(void *ptr, int index), void *ptr );
BBX_RadioBox *BBX_radiobox(BABYX *bbx, HWND parent, char **text, int N, void (*fptr)(void *ptr, int index), void *ptr );
void bbx_radiobox_kill(BBX_RadioBox *obj);
int bbx_radiobox_getselected(BBX_RadioBox *obj);
void bbx_radiobox_setselected(BBX_RadioBox *obj, int index);
int bbx_radiobox_disable(BBX_RadioBox *obj, int index);
int bbx_radiobox_enable(BBX_RadioBox *obj, int index);


#endif
