#ifndef bbx_listbox_h
#define bbx_listbox_h

typedef BBX_Panel BBX_ListBox;

BBX_ListBox *bbx_listbox(BABYX *bbx, BBX_Panel *parent, void (*fptr)(void *ptr, int selected), void *ptr);
void bbx_listbox_kill(BBX_ListBox *obj); 
int bbx_listbox_addstring(BBX_ListBox *obj, char *str);
void bbx_listbox_clear(BBX_ListBox *obj);
int bbx_listbox_getselected(BBX_ListBox *box);
int bbx_listbox_setselected(BBX_ListBox *box, int idx);

#endif
