#ifndef bbx_console_h
#define bbx_console_h

BBX_Panel *bbx_console(BABYX *bbx, BBX_Panel *parent);
void bbx_console_printf(BBX_Panel *obj, char *fmt, ...);
char *bbx_console_getline(BBX_Panel *obj);

#endif
