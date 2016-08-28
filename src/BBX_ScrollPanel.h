#ifndef bbx_scrollpanel_h
#define bbx_scrollpanel_h

typedef BBX_Panel BBX_ScrollPanel;
BBX_ScrollPanel *bbx_scrollpanel(BABYX *bbx, BBX_Panel *parent, int width, int height, int bars);
void bbx_scrollpanel_kill(BBX_ScrollPanel *obj);
int bbx_scrollpanel_add(BBX_ScrollPanel *pan, void *bbxobj, int x, int y, int width, int height);

#endif

