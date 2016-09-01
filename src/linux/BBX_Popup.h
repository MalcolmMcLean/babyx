#ifndef bbx_popup_h
#define bbx_popup_h


typedef struct
{
  int id;
  char *left;
  char *right;
  struct BBX_popup *sub;
  int disabled;
} BBX_Popup_MenuItem;

typedef struct BBX_popup
{
  BABYX *bbx;
  BBX_Popup_MenuItem *menu;
  int N;
} BBX_Popup;

typedef BBX_Panel BBX_PopUp2;

BBX_Popup *bbx_popup(BABYX *bbx);
void bbx_popup_kill(BBX_Popup *pop);
int bbx_popup_append(BBX_Popup *pop, int id, char *left, char *right, BBX_Popup *sub);
void bbx_popup_disable(BBX_Popup *pop, int id);
void bbx_popup_enable(BBX_Popup *pop, int id);

BBX_PopUp2 *bbx_popup2(BABYX *bbx, BBX_Panel *parent, int x, int y, BBX_Popup *pop, void (*chosen)(void *ptr, int id), void *ptr);
void bbx_popup2_kill(BBX_PopUp2 *pop);
void bbx_popup2_makemodal(BBX_PopUp2 *pop);
void bbx_popup2_dropmodal(BBX_PopUp2 *pop);
void bbx_popup2_doptr(BBX_PopUp2 *pop);

int BBX_PopupPopup(BBX_Popup *obj, Window parent, int x, int y);

int BBX_QuickPopup(BABYX *bbx, Window parent, int x, int y, char **str, int N);

#endif
