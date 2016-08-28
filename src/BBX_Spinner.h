#ifndef bbx_spinner_h
#define bbx_spinner_h

#define BBX_SPINNER_REAL 1
#define BBX_SPINNER_LOGARITHMIC 2
#define BBX_SPINNER_INTERACTIVE 4

typedef BBX_Panel BBX_Spinner;

BBX_Spinner *bbx_spinner(BABYX *bbx, BBX_Panel *pan, double val, double minval, double maxval, double delta, void (*change)(void *ptr, double val), void *ptr);
BBX_Spinner *BBX_spinner(BABYX *bbx, HWND parent, double val, double minval, double maxval, double delta, void (*change)(void *ptr, double val), void *ptr);
void bbx_spinner_kill(BBX_Spinner *sp);
double bbx_spinner_getvalue(BBX_Spinner *sp);
void bbx_spinner_setvalue(BBX_Spinner *sp, double val);
void bbx_spinner_setparams(BBX_Spinner *sp, double val, double minval, double maxval, double delta);
int bbx_spinner_setmode(BBX_Spinner *sp, int mode);
void bbx_spinner_setformat(BBX_Spinner *sp, char *fmt);
int bbx_spinner_spinning(BBX_Spinner *sp);
void bbx_spinner_enable(BBX_Spinner *sp);
void bbx_spinner_disable(BBX_Spinner *sp);

#endif

