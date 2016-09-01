#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BabyX.h"
#include "tabpanel.h"
#include "datepicker.h"

char *fslurp(char *fname);

typedef struct
{
  BABYX *bbx;
  BBX_Menubar *menu;
  BBX_Label *lab;
  BBX_Button *but;
  BBX_RadioBox *rad;
  BBX_CheckBox *chk;
  BBX_LineEdit *edt;
  BBX_Canvas *can;
  BBX_EditBox *box;
  BBX_Spinner *spn;
  BBX_RGBA col;
} APP;

void create(void *obj, BABYX *bbx, BBX_Panel *root);
void layout(void *obj, int width, int height);
static void clickbutton(void *obj);
static void dodialog(APP *app);
static void menuchosen(void *obj, int id);
static void canvasfunc(void *obj, int action, int x, int y, int buttons);

int main(void)
{
  APP *app;
  app = malloc(sizeof(APP));
  startbabyx("Baby X", 500, 200, create, layout, app);
  free(app);
  return 0;
}

void create(void *obj, BABYX *bbx, BBX_Panel *root)
{
  APP *app = obj;
  char *radios[5] = {"Radio 1", "Radio 2", "Radio 3", "Radio 4", "Disabled"};


  app->bbx = bbx;
 
  app->lab = bbx_label(bbx, root, "Hello Baby X");

  BBX_Popup *pop = bbx_popup(bbx);
  bbx_popup_append(pop, 1, "Open ...", "", 0);
  bbx_popup_append(pop, 2, "Disabled", "", 0);
  bbx_popup_append(pop, 3, "Save ...", "", 0);
  bbx_popup_append(pop, 10, "Exit", "", 0);

    
  BBX_Popup *sub = bbx_popup(bbx);
  bbx_popup_append(sub, 4, "Red", "#FF0000", 0);
  bbx_popup_append(sub, 5, "Alice Blue", "#FFF8FF", 0);
  bbx_popup_append(sub, 6, "Green", "#00FF00", 0);

  BBX_Popup *pop2 = bbx_popup(bbx);
  bbx_popup_append(pop2, 7, "Choose", "me", 0);
  bbx_popup_append(pop2, 8, "Colours", "", sub);
  bbx_popup_append(pop2, 9, "Another Menu Item", "", 0);
    
  app->menu = bbx_menubar(bbx, root, menuchosen, app);
  bbx_menubar_addmenu(app->menu, "File", pop);
  bbx_menubar_addmenu(app->menu, "Help", pop2);

  bbx_menubar_disable(app->menu, 2);

  app->but = bbx_button(bbx, root, "Click me", clickbutton, app);
  app->rad = bbx_radiobox(bbx, root, radios, 5, 0, 0);
  bbx_radiobox_disable(app->rad, 4);
  
  app->chk = bbx_checkbox(bbx, root, "Check me", 0, 0);

  app->edt = bbx_lineedit(bbx, root, "edit me", 0, 0);
  
  app->can = bbx_canvas(bbx, root, 100, 100, bbx_color("yellow"));
  bbx_canvas_setmousefunc(app->can, canvasfunc, app);

  app->box = bbx_editbox(bbx, root, 0, 0);
  bbx_editbox_settext(app->box, "Mary had a little lamb, its fleece was "
   "white as snow.\nAnd everywhere that Mary went, the lamb was sure to go.\n"
  "It followed her to school one day, that was against the rule.\n"
  "It made the children laugh and play, to see a lamb at school.");
  
  app->spn = bbx_spinner(bbx, root, 10, 0, 100, 1, 0, 0);
  
  app->col = bbx_rgb(255, 0, 0);
}

void layout(void *obj, int width, int height)
{
  APP *app =  obj;
  BABYX *bbx = app->bbx;
  int lab_width, lab_height;

  bbx_label_getpreferredsize(app->lab, &lab_width, &lab_height);
  bbx_setpos(bbx, app->lab, 10, 25, lab_width, lab_height);

  bbx_setpos(bbx, app->menu, 0, 0, width, 20);
  bbx_setpos(bbx, app->but, 100, height - 30, 100, 25);
  bbx_setpos(bbx, app->rad, 310, 100, 150, 150);
  bbx_setpos(bbx, app->chk, 310, 70, 150, 25);
  bbx_setpos(bbx, app->edt, 310, 40, 150, 25);
  bbx_setpos(bbx, app->can, 20, 50, 100, 100);
  bbx_setpos(bbx, app->box, 125, 40, 180, 100);
  bbx_setpos(bbx, app->spn, 20, 160, 70, 20);
}

void killcontrols(APP *app)
{
  bbx_menubar_kill(app->menu);
  bbx_label_kill(app->lab);
  bbx_button_kill(app->but);
  bbx_radiobox_kill(app->rad);
  bbx_checkbox_kill(app->chk);
  bbx_lineedit_kill(app->edt);
  bbx_canvas_kill(app->can);
  bbx_editbox_kill(app->box);
  bbx_spinner_kill(app->spn);
  
}

static void menuchosen(void *obj, int id)
{
  APP *app = obj;
  BABYX *bbx = app->bbx;
  char *fname;
  char *text;

  switch(id)
  {
  case 1:
    fname = bbx_getopenfile(bbx, "*");
    if(fname)
    {
        text = fslurp(fname);
        if(!text)
          bbx_messagebox(bbx, BBX_MB_OK, "Error", "Cant read file");
        else
	{
          if(strlen(text) > 4096)
            bbx_messagebox(bbx, BBX_MB_OK, "Error", "File too large (4k max)");
          else
            bbx_editbox_settext(app->box, text);
        }
        free(text);
	free(fname);
    }
    break;
  case 2:
    /* should never happen */
    break;
  case 3:
    fname = bbx_getsavefile(bbx, "*");
    if(fname)
      bbx_messagebox(bbx, BBX_MB_YES_NO_CANCEL, "File save", "You want to save\n%s.\n", fname);
    free(fname);
  case 4:
    pickdate(bbx, 2015, 3, 1, 0, 0, 0);
    app->col = bbx_rgba(0xFF, 0, 0, 255);
    break;
  case 5:
    app->col = bbx_color("AliceBlue");
    break;
  case 6:
    app->col = bbx_color("green");
    break;
  case 7:
    dodialog(app);
    break;
  case 8:
    /* should never happen */
    break;
  case 9:
    bbx_messagebox(bbx, BBX_MB_OK, "Message Box", "make this menu item\ndo whatever you want"); 
    break;
   case 10:
     killcontrols(app);
     stopbabyx(bbx);
     break;
  }
   
}

static void clickbutton(void *obj)
{
  APP *app = obj;
  int state;

  state = bbx_checkbox_getstate(app->chk);
  bbx_checkbox_setstate(app->chk, state ^ 1);

}

static void canvasfunc(void *obj, int action, int x, int y, int buttons)
{
  unsigned char *rgba;
  int width, height;
  APP *app = obj;

  rgba = bbx_canvas_rgba(app->can, &width, &height);
  if(buttons & BBX_MOUSE_BUTTON1)
  {
    rgba[(y*width+x)*4] = bbx_red(app->col);
    rgba[(y*width+x)*4+1] = bbx_green(app->col);
    rgba[(y*width+x)*4+2] = bbx_blue(app->col);
    bbx_canvas_flush(app->can);
  }
}


typedef struct
{
  BABYX *bbx;
  APP *app;
  BBX_RGBA col;
  BBX_DialogPanel *pan;
  BBX_LineEdit *red_edt;
  BBX_LineEdit *green_edt;
  BBX_LineEdit *blue_edt;
  BBX_Label *red_lab;
  BBX_Label *green_lab;
  BBX_Label *blue_lab;
  BBX_Canvas *can;
  BBX_Button *ok_but;
} Dialog;

static void layoutdialog(void *obj, int width, int height);
static void dlg_kill(Dialog *dlg);
static void dlg_ok(void *obj);
static void dlg_change(void *obj, char *text);

static void dodialog(APP *app)
{
  Dialog dlg;
  BABYX *bbx = app->bbx;
  char red_txt[32];
  char green_txt[32];
  char blue_txt[32];

  dlg.app = app;
  dlg.bbx = app->bbx;
  dlg.col = app->col;
  sprintf(red_txt, "%d", (int) bbx_red(dlg.col));
  sprintf(green_txt, "%d", (int) bbx_green(dlg.col));
  sprintf(blue_txt, "%d", (int) bbx_blue(dlg.col));
  dlg.pan = bbx_dialogpanel(bbx, "Colour", 200, 300, layoutdialog, &dlg);
  dlg.red_edt = bbx_lineedit(bbx, dlg.pan, red_txt, dlg_change, &dlg);
  dlg.green_edt = bbx_lineedit(bbx, dlg.pan, green_txt, dlg_change, &dlg);
  dlg.blue_edt = bbx_lineedit(bbx, dlg.pan, blue_txt, dlg_change, &dlg);
  dlg.red_lab = bbx_label(bbx, dlg.pan, "red");
  dlg.green_lab = bbx_label(bbx, dlg.pan, "green");
  dlg.blue_lab = bbx_label(bbx, dlg.pan, "blue");
  dlg.can = bbx_canvas(bbx, dlg.pan, 32, 32, app->col); 
  dlg.ok_but = bbx_button(bbx, dlg.pan, "Ok", dlg_ok, &dlg);
  bbx_dialogpanel_makemodal(dlg.pan);
  dlg_kill(&dlg);  
}

static void layoutdialog(void *obj, int width, int height)
{
  Dialog *dlg = obj;
  bbx_setpos(dlg->bbx, dlg->red_lab, 10, 10, 80, 20); 
  bbx_setpos(dlg->bbx, dlg->green_lab, 10, 60, 80, 20); 
  bbx_setpos(dlg->bbx, dlg->blue_lab, 10, 110, 80, 20);
  bbx_setpos(dlg->bbx, dlg->red_edt, 100, 10, 80, 20); 
  bbx_setpos(dlg->bbx, dlg->green_edt, 100, 60, 80, 20); 
  bbx_setpos(dlg->bbx, dlg->blue_edt, 100, 110, 80, 20);
  bbx_setpos(dlg->bbx, dlg->can, 100, 150, 32, 32);   
  bbx_setpos(dlg->bbx, dlg->ok_but, 50, height-30, 100, 25);
}

static void dlg_kill(Dialog *dlg)
{
  bbx_button_kill(dlg->ok_but);
  bbx_lineedit_kill(dlg->red_edt);
  bbx_lineedit_kill(dlg->green_edt);
  bbx_lineedit_kill(dlg->blue_edt);
  bbx_canvas_kill(dlg->can);
  bbx_dialogpanel_kill(dlg->pan);
}

static void dlg_ok(void *obj)
{
  Dialog *dlg = obj;

  dlg->app->col = dlg->col;
  bbx_dialogpanel_dropmodal(dlg->pan);
}

#define clamp(x, low, high) ( (x) < (low) ? (low) : (x) > (high) ? (high) : (x))
static void dlg_change(void *obj, char *text)
{
  Dialog *dlg = obj;
  char *redtext, *greentext, *bluetext;
  int red, green, blue;
  unsigned char *rgba;
  int width, height;
  int i;

  redtext = bbx_lineedit_gettext(dlg->red_edt);
  greentext = bbx_lineedit_gettext(dlg->green_edt);
  bluetext = bbx_lineedit_gettext(dlg->blue_edt);

  red = atoi(redtext);
  green = atoi(greentext);
  blue = atoi(bluetext);
  red = clamp(red, 0, 255);
  green = clamp(green, 0, 255);
  blue = clamp(blue, 0, 255);

  free(redtext);
  free(greentext);
  free(bluetext);

  rgba = bbx_canvas_rgba(dlg->can, &width, &height);
  for(i=0;i<width * height; i++)
  {
    rgba[i*4] = red;
    rgba[i*4+1] = green;
    rgba[i*4+2] = blue;
  }
  bbx_canvas_flush(dlg->can);
  dlg->col = bbx_rgb(red, green, blue);
  
}

#include <stdio.h>

char *fslurp(char *fname)
{
  char *answer = 0;
  long len = 0;
  FILE *fp;

  fp = fopen(fname, "r");
  if(!fp)
    return 0;
  fseek(fp, 0, SEEK_END);
  len = ftell(fp);
  answer = malloc(len + 1);
  fseek(fp, 0, SEEK_SET);
  fread(answer, 1, len, fp);
  answer[len] = 0;
  fclose(fp);
  return answer;
}
