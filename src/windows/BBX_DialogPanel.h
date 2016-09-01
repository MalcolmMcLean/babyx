#ifndef BBX_DialogPanel_h
#define BBX_DialogPanel_h

typedef BBX_Panel BBX_DialogPanel;
BBX_DialogPanel *bbx_dialogpanel(BABYX *bbx, char *name, int width, int height, void (*changesize)(void *ptr, int width, int height), void *ptr);
void bbx_dialogpanel_kill(BBX_DialogPanel *pan);
void bbx_dialogpanel_makemodal(BBX_DialogPanel *pan);
void bbx_dialogpanel_dropmodal(BBX_DialogPanel *pan);
void bbx_dialogpanel_setclosefunc(BBX_DialogPanel *pan, void (*closefunc)(void *ptr), void *ptr);

ATOM BBX_RegisterDialogPanel(HINSTANCE hInstance);

#endif

