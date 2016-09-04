#ifndef rollup_h
#define rollup_h

BBX_Panel *bbx_rollup(BABYX *bbx, BBX_Panel *parent, int scrollbar);
void bbx_rollup_kill(BBX_Panel *obj);
BBX_Panel *bbx_rollup_addrollup(BBX_Panel *obj, char *text, int height, void(*layout)(void *ptr, int width, int height), void *ptr);
BBX_Panel *bbx_rollup_getpanel(BBX_Panel *obj, char *name);
int bbx_rollup_getstate(BBX_Panel *obj, char *name);
void bbx_rollup_closepanel(BBX_Panel *obj, char *name);
void bbx_rollup_openpanel(BBX_Panel *obj, char *name);

#endif
