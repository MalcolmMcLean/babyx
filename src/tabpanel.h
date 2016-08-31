#ifndef tabpanel_h
#define tabpanel_h

BBX_Panel *bbx_tab(BABYX *bbx, BBX_Panel *parent);
void bbx_tab_kill(BBX_Panel *obj);
int bbx_tab_addtab(BBX_Panel *pan, char *name, void(*layout)(void *ptr, int width, int height, int miny), void *ptr);
int bbx_tab_showtab(BBX_Panel *pan, char *name);
void bbx_tab_register(BBX_Panel *pan, void *obj);
void bbx_tab_deregister(BBX_Panel *pan, void *obj);

#endif
