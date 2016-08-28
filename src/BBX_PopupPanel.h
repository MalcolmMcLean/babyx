#ifndef bbx_popuppanel_h
#define bbx_popuppanel_h

BBX_Panel *bbx_popuppanel(BABYX *bbx, BBX_Panel *parent, char *tag, void (*changesize)(void *ptr, int width, int height), void *ptr, int x, int y);
BBX_Panel *BBX_popuppanel(BABYX *bbx, HWND parent, char *tag, void (*changesize)(void *ptr, int width, int height), void *ptr, int x, int y);

void bbx_popuppanel_makemodal(BBX_DialogPanel *pan);
void bbx_popuppanel_dropmodal(BBX_DialogPanel *pan);

#endif
