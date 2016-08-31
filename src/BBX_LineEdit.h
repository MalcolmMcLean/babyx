#ifndef bbx_lineedit_h
#define bbx_lineedit_h

typedef BBX_Panel BBX_LineEdit;

BBX_LineEdit *bbx_lineedit(BABYX *bbx, BBX_Panel *parent, char *text, void (*fptr)(void *ptr, char *text), void *ptr );
BBX_LineEdit *BBX_lineedit(BABYX *bbx, Window parent, char *text, void (*fptr)(void *ptr, char *text), void *ptr );
void bbx_lineedit_kill(BBX_LineEdit *edt);
char *bbx_lineedit_gettext(BBX_LineEdit *edt);
void bbx_lineedit_settext(BBX_LineEdit *edt, char *text);
void bbx_lineedit_disable(BBX_LineEdit *edt);
void bbx_lineedit_enable(BBX_LineEdit *edt);

#endif
