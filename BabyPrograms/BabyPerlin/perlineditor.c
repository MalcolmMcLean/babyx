#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <ctype.h>
#include <math.h>

#include "simplexnoise.h"
#include "colourschemes.h"
#include "gif.h"
#include "BabyX.h"

#include "paletteeditor.h"
#include "paletteadjuster.h"

typedef struct
{
  BABYX *bbx;
  BBX_Panel *root;
  BBX_Menubar *menubar;
  BBX_Canvas *can;
  BBX_Scrollbar *scrollh;
  BBX_Scrollbar *scrollv;
  BBX_Label *width_lab;
  BBX_LineEdit *width_edt;
  BBX_Label *height_lab;
  BBX_LineEdit *height_edt;
  BBX_Label *octaves_lab;
  BBX_Spinner *octaves_spn;
  BBX_Label *persistence_lab;
  BBX_Spinner *persistence_spn;
  BBX_Label *scale_lab;
  BBX_Spinner *scale_spn;
  BBX_Label *deltax_lab;
  BBX_Spinner *deltax_spn;
  BBX_Label *deltay_lab;
  BBX_Spinner *deltay_spn;
  BBX_Label *deltat_lab;
  BBX_Spinner *deltat_spn;
  BBX_CheckBox *animate_chk;
  PalAdjuster *paletteadjuster_wgt;
  PALETTE pal;
  int width;
  int height;
  double octaves;
  double persistence;
  double scale;
  double delta_x;
  double delta_y;
  double delta_t;
  int xpos;
  int ypos;
  int t;
  void *ticker;
} APP;

APP app;

void createapp(void *obj, BABYX *bbx, BBX_Panel *root);
void layoutapp(void *obj, int width, int height);
static void setvalues(APP *app);

static void editwidth(void *obj, char *text); 
static void editheight(void *obj, char *text);
//static void editoctaves(void *obj, char *text);
//static void editpersistence(void *obj, char *text);
//static void editscale(void *obj, char *text);
//static void editdeltax(void *obj, char *text);
//static void editdeltay(void *obj, char *text);
//static void editdeltat(void *obj, char *text);

static void spinoctaves(void *obj, double value);
static void spinpersistence(void *obj, double value);
static void spinscale(void *obj, double value);
static void spindeltax(void *obj, double value);
static void spindeltay(void *obj, double value);
static void spindeltat(void *obj, double value);

static void checkanimate(void *obj, int state);
static void paletteadjusted(void *obj, unsigned char *rgb, int N);
static void resize(APP *app, int width, int height);
static void scrollhorizontal(void *obj, int pos);
static void scrollvertical(void *obj, int pos);
void menuhandler(void *obj, int id);
void openfile(APP *app, char *fname);
void loadpalettefromgif(APP *app);
static void saveasgif(APP *app, char *fname);
void aboutdialog(BABYX *bbx);

char *trim(char *str);

//int main(void)

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstnace, LPSTR lpCommandLine, int nCmdShow)
{
  app.width = 200;
  app.height = 2000/16;
  app.octaves = 2.3;
  app.persistence = 1.0;
  app.scale = 2.0;
  app.delta_x = 0.01;
  app.delta_y = 0.01;
  app.delta_t = 0.005;
  app.t = 0;
  startbabyx(hInstance, "Perlin editor", 530, 400, createapp, layoutapp, &app);  

  return 0;
}

void ticktick(void *ptr)
{
   APP *app = ptr;
  unsigned char *rgba;
  int width, height;
  int x, y;
  int t;
  float noise;
  float loBound = 0.0f;
  float hiBound = 255.99f;
  int index;
 

  rgba = bbx_canvas_rgba(app->can, &width, &height);
  t = app->t++;
  for(y=0;y<height;y++)
    for(x=0;x<width;x++)
    {
       noise = scaled_octave_noise_3d( (float) app->octaves,
                           (float) app->persistence,
                           (float) app->scale,
                           loBound,
                           hiBound,
			   (float) ((x + app->xpos) * app->delta_x),
			   (float) ((y + app->ypos) * app->delta_y),
				(float)       (t * app->delta_t));

       index = (int) noise;

      rgba[(y*width+x)*4] = (unsigned char) app->pal.rgb[index*3];
      rgba[(y*width+x)*4+1] = (unsigned char) app->pal.rgb[index*3+1];
      rgba[(y*width+x)*4+2] = (unsigned char) app->pal.rgb[index*3+2];
      rgba[(y*width+x)*4+3] = 255;
    }
  bbx_canvas_flush(app->can);
}

void clickok(void *ptr)
{
  APP *app = ptr;
  unsigned char *rgba;
  int width, height;
  int x, y;
  float noise;
  float loBound = 0.0f;
  float hiBound = 255.99f;
  int index;

  if(app->ticker)
    return;
 
  rgba = bbx_canvas_rgba(app->can, &width, &height);
  for(y=0;y<height;y++)
    for(x=0;x<width;x++)
    {
       noise = scaled_octave_noise_2d( (float) app->octaves,
                           (float) app->persistence,
                           (float) app->scale,
                           loBound,
                           hiBound,
			   (float) ((x + app->xpos) * app->delta_x),
			   (float) ((y + app->ypos) * app->delta_y));

       index = (int) noise;

      rgba[(y*width+x)*4] = (unsigned char) app->pal.rgb[index*3];
      rgba[(y*width+x)*4+1] = (unsigned char) app->pal.rgb[index*3+1];
      rgba[(y*width+x)*4+2] = (unsigned char) app->pal.rgb[index*3+2];
      rgba[(y*width+x)*4+3] = 255;
    }
  bbx_canvas_flush(app->can);
  
}

void createapp(void *obj, BABYX *bbx, BBX_Panel *root)
{
  APP *app = obj;
  BBX_Popup *filemenu;
  BBX_Popup *palettemenu;
  BBX_Popup *helpmenu;
  BBX_Popup *custompalettesmenu;

  
  app->pal.N = 256;
  app->pal.rgb = malloc(256 * 3);
  creategrey(app->pal.rgb, 256);

  app->ticker = 0;
  app->bbx = bbx;
  app->root = root;
  bbx_panel_setbackground(root, bbx_color("LightGray"));
  app->menubar = bbx_menubar(bbx, root, menuhandler, app);

  custompalettesmenu = bbx_popup(bbx);
  bbx_popup_append(custompalettesmenu, 101, "Jet", "", 0);
  bbx_popup_append(custompalettesmenu, 102, "Gray", "", 0);
  bbx_popup_append(custompalettesmenu, 103, "Red Green", "", 0);
  bbx_popup_append(custompalettesmenu, 104, "Blue Yellow", "", 0);
  bbx_popup_append(custompalettesmenu, 105, "Flame", "", 0);
  bbx_popup_append(custompalettesmenu, 106, "Rainbow", "", 0);
  bbx_popup_append(custompalettesmenu, 107, "Land Sea", "", 0);
  bbx_popup_append(custompalettesmenu, 108, "Relief", "", 0);
  bbx_popup_append(custompalettesmenu, 109, "Zebra", "", 0);
  bbx_popup_append(custompalettesmenu, 111, "Union Jack", "", 0);
  bbx_popup_append(custompalettesmenu, 112, "Ocean", "", 0);
  bbx_popup_append(custompalettesmenu, 113, "No Green", "", 0);

  filemenu = bbx_popup(bbx);
  bbx_popup_append(filemenu, 1, "Save as GIF", "...", 0);
  bbx_popup_append(filemenu, 2, "Exit", "", 0);

  palettemenu = bbx_popup(bbx);
  bbx_popup_append(palettemenu, 4, "Edit palette", "...", 0);
  bbx_popup_append(palettemenu, 5, "Custom palette", "", custompalettesmenu);
  bbx_popup_append(palettemenu, 6, "Load palette from gif", "...", 0);

  helpmenu = bbx_popup(bbx);
  bbx_popup_append(helpmenu, 3, "About", "", 0);

  bbx_menubar_addmenu(app->menubar, "File", filemenu);
  bbx_menubar_addmenu(app->menubar, "Palette", palettemenu);
  bbx_menubar_addmenu(app->menubar, "Help", helpmenu);

  app->can = bbx_canvas(bbx, root, app->width, app->height, BBX_Color("white"));
  app->scrollh = 0;
  app->scrollv = 0;
  app->xpos = 0;
  app->ypos = 0;

  app->width_lab = bbx_label(bbx, root, "Width");
  app->width_edt = bbx_lineedit(bbx, root, "", editwidth, app);
  app->height_lab = bbx_label(bbx, root, "Height");
  app->height_edt = bbx_lineedit(bbx, root, "", editheight, app);
  app->octaves_lab = bbx_label(bbx, root, "Octaves"); 
  // app->octaves_edt = bbx_lineedit(bbx, root, "", editoctaves, app);
  app->octaves_spn = bbx_spinner(bbx, root, 2, 1, 10, 1, spinoctaves, app);
  app->persistence_lab = bbx_label(bbx, root, "Persistence");
  // app->persistence_edt = bbx_lineedit(bbx, root,"", editpersistence, app);
  app->persistence_spn = bbx_spinner(bbx, root, 1.0, FLT_EPSILON, FLT_MAX, 0.01, spinpersistence, app);
  app->scale_lab = bbx_label(bbx, root, "Scale");
  // app->scale_edt = bbx_lineedit(bbx, root, "", editscale, app);
  app->scale_spn = bbx_spinner(bbx, root, 1.0, FLT_EPSILON, FLT_MAX, 0.01, spinscale, app);
  app->deltax_lab = bbx_label(bbx, root, "dx");
  // app->deltax_edt = bbx_lineedit(bbx, root, "", editdeltax, app);
  app->deltax_spn = bbx_spinner(bbx, root, 1.0, FLT_EPSILON, FLT_MAX, 0.01, spindeltax, app);
  app->deltay_lab = bbx_label(bbx, root, "dy");
  // app->deltay_edt = bbx_lineedit(bbx, root, "", editdeltay, app);
  app->deltay_spn = bbx_spinner(bbx, root, 1.0, FLT_EPSILON, FLT_MAX, 0.01, spindeltay, app);
  app->deltat_lab = bbx_label(bbx, root, "dt");
  //app->deltat_edt = bbx_lineedit(bbx, root, "", editdeltat, app);
  app->deltat_spn = bbx_spinner(bbx, root, 1.0, FLT_EPSILON, FLT_MAX, 0.01, spindeltat, app);

  app->animate_chk = bbx_checkbox(bbx, root, "Animate", checkanimate, app);

  app->paletteadjuster_wgt = paladjuster(bbx, root, app->pal.rgb, app->pal.N, paletteadjusted, app); 

  bbx_label_setbackground(app->width_lab, bbx_color("LightGray"));
  bbx_label_setbackground(app->height_lab, bbx_color("LightGray"));
  bbx_label_setbackground(app->octaves_lab, BBX_Color("LightGray"));
  bbx_label_setbackground(app->persistence_lab, BBX_Color("LightGray"));
  bbx_label_setbackground(app->scale_lab, BBX_Color("LightGray"));
  bbx_label_setbackground(app->deltax_lab, BBX_Color("LightGray"));
  bbx_label_setbackground(app->deltay_lab, BBX_Color("LightGray"));
  bbx_label_setbackground(app->deltat_lab, BBX_Color("LightGray"));

  bbx_spinner_setmode(app->persistence_spn, BBX_SPINNER_REAL | BBX_SPINNER_INTERACTIVE | BBX_SPINNER_LOGARITHMIC);
  bbx_spinner_setformat(app->persistence_spn, "%.2g");

  bbx_spinner_setmode(app->scale_spn, BBX_SPINNER_REAL | BBX_SPINNER_INTERACTIVE | BBX_SPINNER_LOGARITHMIC);
  bbx_spinner_setformat(app->scale_spn, "%.2g");

  bbx_spinner_setmode(app->deltax_spn, BBX_SPINNER_REAL | BBX_SPINNER_INTERACTIVE | BBX_SPINNER_LOGARITHMIC);
  bbx_spinner_setformat(app->deltax_spn, "%.2g");

  bbx_spinner_setmode(app->deltay_spn, BBX_SPINNER_REAL | BBX_SPINNER_INTERACTIVE | BBX_SPINNER_LOGARITHMIC);
  bbx_spinner_setformat(app->deltay_spn, "%.2g");

  bbx_spinner_setmode(app->deltat_spn, BBX_SPINNER_REAL | BBX_SPINNER_INTERACTIVE | BBX_SPINNER_LOGARITHMIC);
  bbx_spinner_setformat(app->deltat_spn, "%.2g");

  setvalues(app);

} 

void killapp(APP *app)
{
  bbx_menubar_kill(app->menubar);
  bbx_canvas_kill(app->can);
  bbx_scrollbar_kill(app->scrollh);
  bbx_scrollbar_kill(app->scrollv);
  bbx_label_kill(app->width_lab);
  bbx_lineedit_kill(app->width_edt);
  bbx_label_kill(app->height_lab);
  bbx_lineedit_kill(app->height_edt);
  bbx_label_kill(app->octaves_lab);
  bbx_spinner_kill(app->octaves_spn);
  bbx_label_kill(app->persistence_lab);
  bbx_spinner_kill(app->persistence_spn);
  bbx_label_kill(app->scale_lab);
  bbx_spinner_kill(app->scale_spn);
  bbx_label_kill(app->deltax_lab);
  bbx_spinner_kill(app->deltax_spn);
  bbx_label_kill(app->deltay_lab);
  bbx_spinner_kill(app->deltay_spn);
  bbx_label_kill(app->deltat_lab);
  bbx_spinner_kill(app->deltat_spn);
  bbx_checkbox_kill(app->animate_chk);
  paladjuster_kill(app->paletteadjuster_wgt);

  free(app->pal.rgb);
}

void layoutapp(void *obj, int width, int height)
{
  APP *app = obj;
  BABYX *bbx = app->bbx;
  int cwidth, cheight;

  bbx_setpos(bbx, app->menubar, 0, 0, width, 20); 

  cwidth = app->width < 300 ? app->width : 300;
  cheight = app->height < 256 ? app->height : 256;
  bbx_setpos(bbx, app->can, 10, 30, cwidth, cheight);

  bbx_setpos(bbx, app->width_lab, width-200, 30, 100, 25);
  bbx_setpos(bbx, app->height_lab, width-200, 60, 100, 25); 
  bbx_setpos(bbx, app->octaves_lab, width-200, 90, 100, 25);
  bbx_setpos(bbx, app->persistence_lab, width-200, 120, 100, 25);
  bbx_setpos(bbx, app->scale_lab, width-200, 150, 100, 25);
  bbx_setpos(bbx, app->deltax_lab, width-200, 180, 100, 25);
  bbx_setpos(bbx, app->deltay_lab, width-200, 210, 100, 25);
  bbx_setpos(bbx, app->deltat_lab, width-200, 240, 100, 25);

  bbx_setpos(bbx, app->width_edt, width-100, 30, 90, 25);
  bbx_setpos(bbx, app->height_edt, width-100, 60, 90, 25); 
  bbx_setpos(bbx, app->octaves_spn, width-100, 90, 90, 25);
  bbx_setpos(bbx, app->persistence_spn, width-100, 120, 90, 25);
  bbx_setpos(bbx, app->scale_spn, width-100, 150, 90, 25);
  bbx_setpos(bbx, app->deltax_spn, width-100, 180, 90, 25);
  bbx_setpos(bbx, app->deltay_spn, width-100, 210, 90, 25);
  bbx_setpos(bbx, app->deltat_spn, width-100, 240, 90, 25);

  bbx_setpos(bbx, app->animate_chk, 10, height-80, 100, 20);
  bbx_setpos(bbx, app->paletteadjuster_wgt, 10, height-50, 256 + 16 + 70+70, 50);
 
  clickok(app);

}

static void setvalues(APP *app)
{
  char buff[256];

  sprintf(buff, "%d", app->width);
  bbx_lineedit_settext(app->width_edt, buff);
  sprintf(buff, "%d", app->height);
  bbx_lineedit_settext(app->height_edt, buff);
  //sprintf(buff, "%g", app->octaves);
  //bbx_lineedit_settext(app->octaves_edt, buff);
  bbx_spinner_setvalue(app->octaves_spn, ceil(app->octaves)); 
  //sprintf(buff, "%g", app->persistence);
  //bbx_lineedit_settext(app->persistence_edt, buff);
  bbx_spinner_setvalue(app->scale_spn, app->scale); 
  bbx_spinner_setvalue(app->deltax_spn, app->delta_x); 
  bbx_spinner_setvalue(app->deltay_spn, app->delta_y); 
  bbx_spinner_setvalue(app->deltat_spn, app->delta_t); 
  bbx_spinner_setvalue(app->persistence_spn, app->persistence); 
  /*
  sprintf(buff, "%g", app->scale);
  bbx_lineedit_settext(app->scale_edt, buff);
  sprintf(buff, "%g", app->delta_x);
  bbx_lineedit_settext(app->deltax_edt, buff);
  sprintf(buff, "%g", app->delta_y);
  bbx_lineedit_settext(app->deltay_edt, buff);
  sprintf(buff, "%g", app->delta_t);
  bbx_lineedit_settext(app->deltat_edt, buff);
  */
}


static void editwidth(void *obj, char *text)
{
  APP *app = obj;
  char *trimmed;
  char *end;
  long width;

  trimmed = trim(text);
  width = strtol(trimmed, &end, 10);
  if(*end == 0 && width > 0)
  {
    resize(app, width, app->height);
  }
  free(trimmed);
}


static void editheight(void *obj, char *text)
{
  APP *app = obj;
  char *trimmed;
  char *end;
  long height;

  trimmed = trim(text);
  height = strtol(trimmed, &end, 10);
  if(*end == 0 && height > 0)
  {
    resize(app, app->width, height);
  }
  free(trimmed);
}

/*
static void editoctaves(void *obj, char *text)
{
  APP *app = obj;
  char *trimmed;
  char *end;
  double octaves;

  trimmed =trim(text);
  octaves = strtod(trimmed, &end);
  if(*end == 0 && octaves > 0 & octaves < 20)
  {
    app->octaves = octaves;
    clickok(app);
  }
  free(trimmed);
}
*/

static void spinoctaves(void *obj, double value)
{
  APP *app = obj;
  double octaves;

  octaves = value;
  if(octaves >= 0 && octaves <= 10)
  {
    app->octaves = octaves;
    clickok(app);
  }
}

/*
static void editpersistence(void *obj, char *text)
{
  APP *app = obj;
  char *trimmed;
  char *end;
  double persistence;

  trimmed = trim(text);
  persistence = strtod(trimmed, &end);
  if(*end == 0 && persistence > 0)
  {
    app->persistence = persistence;
    clickok(app);
  }
  free(trimmed);
}

*/

static void spinpersistence(void *obj, double value)
{
  APP *app = obj;
  double persistence;

  persistence = value;
  if(persistence > 0)
  {
    app->persistence = persistence;
    clickok(app);
  }
}

/*
static void editscale(void *obj, char *text)
{
  APP *app = obj;
  char *trimmed;
  char *end;
  double scale;

  trimmed = trim(text);
  scale = strtod(trimmed, &end);
  if(*end == 0 && scale > 0)
  {
    app->scale = scale;
    clickok(app);
  }
  free(trimmed);
}

*/

static void spinscale(void *obj, double value)
{
  APP *app = obj;

  app->scale = value;
  clickok(app);
}

/*
static void editdeltax(void *obj, char *text)
{
  APP *app = obj;
  char *trimmed;
  char *end;
  double delta_x;

  trimmed = trim(text);
  delta_x = strtod(trimmed, &end);
  if(*end == 0)
  {
    app->delta_x = delta_x;
    clickok(app);
  }
  free(trimmed);
}

*/

static void spindeltax(void *obj, double value)
{
  APP *app = obj;
  app->delta_x = value;
  clickok(app);
}
/*
static void editdeltay(void *obj, char *text)
{
  APP *app = obj;
  char *trimmed;
  char *end;
  double delta_y;

  trimmed = trim(text);
  delta_y = strtod(trimmed, &end);
  if(*end == 0)
  {
    app->delta_y = delta_y;
    clickok(app);
  }
  free(trimmed);
}
*/


static void spindeltay(void *obj, double value)
{
  APP *app = obj;
  app->delta_y = value;
  clickok(app);
}

/*
static void editdeltat(void *obj, char *text)
{
  APP *app = obj;
  char *trimmed;
  char *end;
  double delta_t;

  trimmed = trim(text);
  delta_t = strtod(trimmed, &end);
  if(*end == 0)
  {
    app->delta_t = delta_t;
    clickok(app);
  }
  free(trimmed);
}
*/

static void spindeltat(void *obj, double value)
{
  APP *app = obj;

  app->delta_t = value;
  clickok(app);
}

static void checkanimate(void *obj, int state)
{
  APP *app = obj;

  if(state)
    app->ticker = bbx_addticker(app->bbx, 1000/25, ticktick, app);
  else if(app->ticker)
  {
    bbx_removeticker(app->bbx, app->ticker);
    app->ticker = 0;
  }
}

static void paletteadjusted(void *obj, unsigned char *rgb, int N)
{
  APP *app = obj;
  memcpy(app->pal.rgb, rgb, N * 3);
  clickok(app);
}



static void resize(APP *app, int width, int height)
{
  int cwidth, cheight;

  if(width <=0 || height <= 0)
    return;
  if(app->width == width && app->height == height)
    return;
  bbx_canvas_kill(app->can);
  cwidth = width > 300 ? 300 : width;
  cheight = height > 256 ? 256 : height; 
  app->can = bbx_canvas(app->bbx, app->root, cwidth, cheight, 0);
  bbx_setpos(app->bbx, app->can, 10, 30, cwidth, cheight);
  if(cwidth == width)
  {
    if(app->scrollh)
      bbx_scrollbar_kill(app->scrollh);
    app->scrollh = 0;
  }
  else
  {
    if(!app->scrollh)
    {
      app->scrollh = bbx_scrollbar(app->bbx, app->root, BBX_SCROLLBAR_HORIZONTAL, scrollhorizontal, app);
    }
    bbx_setpos(app->bbx, app->scrollh, 10, cheight + 30, 300, 20);
    bbx_scrollbar_set(app->scrollh, width, 300, 0);
  }
 
  if(cheight == height)
  {
    if(app->scrollv)
    
      bbx_scrollbar_kill(app->scrollv);
    app->scrollv = 0;
  }
  else
  {
    if(!app->scrollv)
    {
      app->scrollv = bbx_scrollbar(app->bbx, app->root, BBX_SCROLLBAR_VERTICAL, scrollvertical, app); 
    }
    bbx_setpos(app->bbx, app->scrollv, cwidth + 10, 30, 20, 256);
    bbx_scrollbar_set(app->scrollv, height, 256, 0);
  }
  app->width = width;
  app->height = height;
  app->xpos = 0;
  app->ypos = 0;
  clickok(app);
}

static void scrollhorizontal(void *obj, int pos)
{
  APP *app = obj;

  if(pos >= 0 && pos <= app->width - 300)
    app->xpos = pos;
  clickok(app);
}

static void scrollvertical(void *obj, int pos)
{
  APP *app = obj;

  if(pos >= 0 && pos <= app->height - 300)
    app->ypos = pos;
  clickok(app);
}

void menuhandler(void *obj, int id)
{
  APP *app = obj;
  char *fname;  
  switch(id)
  {
  case 1:
    fname = bbx_getsavefile(app->bbx, "*.gif");
    if(fname)
    {
      saveasgif(app, fname);
    }
    free(fname);
    break;
  case 2:
    killapp(app);
    stopbabyx(app->bbx);
    break;
  case 3:
    aboutdialog(app->bbx);
    break;
  case 4:
    openpaletteeditor(app->bbx, &app->pal);
    paladjuster_set(app->paletteadjuster_wgt, app->pal.rgb, app->pal.N);
    clickok(app);
     break;
  case 6:
    loadpalettefromgif(app);
    paladjuster_set(app->paletteadjuster_wgt, app->pal.rgb, app->pal.N);
    break;
  case 101: createjet(app->pal.rgb, app->pal.N);
    paladjuster_set(app->paletteadjuster_wgt, app->pal.rgb, app->pal.N);
     clickok(app);
     break;
  case 102: creategrey(app->pal.rgb, app->pal.N);
    paladjuster_set(app->paletteadjuster_wgt, app->pal.rgb, app->pal.N);
    clickok(app);
     break;  
  case 103: createredgreen(app->pal.rgb, app->pal.N);
    paladjuster_set(app->paletteadjuster_wgt, app->pal.rgb, app->pal.N);
    clickok(app);
    break;
  case 104: createblueyellow(app->pal.rgb, app->pal.N);
    paladjuster_set(app->paletteadjuster_wgt, app->pal.rgb, app->pal.N);
    clickok(app);
    break;
  case 105: createflame(app->pal.rgb, app->pal.N);
    paladjuster_set(app->paletteadjuster_wgt, app->pal.rgb, app->pal.N);
    clickok(app);
    break;
  case 106: createrainbow(app->pal.rgb, app->pal.N);
    paladjuster_set(app->paletteadjuster_wgt, app->pal.rgb, app->pal.N);
    clickok(app);
    break;
  case 107: createlandsea(app->pal.rgb, app->pal.N);
    paladjuster_set(app->paletteadjuster_wgt, app->pal.rgb, app->pal.N);
    clickok(app);
    break;
  case 108: createrelief(app->pal.rgb, app->pal.N);
    paladjuster_set(app->paletteadjuster_wgt, app->pal.rgb, app->pal.N);
    clickok(app);
    break;
  case 109: createzebra(app->pal.rgb, app->pal.N);
    paladjuster_set(app->paletteadjuster_wgt, app->pal.rgb, app->pal.N);
    clickok(app);
    break; 
  case 111: createunionjack(app->pal.rgb, app->pal.N);
    paladjuster_set(app->paletteadjuster_wgt, app->pal.rgb, app->pal.N);
    clickok(app);
    break;
  case 112: createocean(app->pal.rgb, app->pal.N);
    paladjuster_set(app->paletteadjuster_wgt, app->pal.rgb, app->pal.N);
    clickok(app);
    break;
  case 113: createnogreen(app->pal.rgb, app->pal.N);
    paladjuster_set(app->paletteadjuster_wgt, app->pal.rgb, app->pal.N);
    clickok(app);
    break;
  }
}



void openfile(APP *app, char *fname)
{
 
}

void loadpalettefromgif(APP *app)
{
  char *fname;
  unsigned char pal[256*3];
  unsigned char *img;
  int width, height;
  int transparent;

  fname = bbx_getopenfile(app->bbx, "*.gif");
  if(!fname)
    return;
  img = loadgif(fname, &width, &height, pal, &transparent);
  if(!img)
  {
  }
  else
  {
    app->pal.rgb = realloc(app->pal.rgb, 256* 3);
    if(!app->pal.rgb)
      exit(EXIT_FAILURE);
    memcpy(app->pal.rgb, pal, 3 * 256);
    app->pal.N = 256;
    clickok(app);
  }
  free(img);  
  free(fname);
}

void saveimage(APP *app, char *fname)
{
}

static void saveasgif(APP *app, char *fname)
{
  unsigned char *img;
  int x, y;
  int width, height;
  int index;
  float loBound = 0.0f;
  float hiBound = app->pal.N - 0.01f;
  float noise;
 
  width = app->width;
  height = app->height;
 
  img = malloc(width * height);
  if(!img)
    return;

  for(y=0;y<height;y++)
    for(x=0;x<width;x++)
    {
       noise = scaled_octave_noise_2d( (float) app->octaves,
                           (float) app->persistence,
                           (float) app->scale,
                           loBound,
                           hiBound,
                           (float) (x * app->delta_x),
                           (float) (y * app->delta_y));

       index = (int) noise;
       img[y*width+x] = (unsigned char) index;
    }
  savegif(fname, img, width, height, app->pal.rgb, app->pal.N, -1, 0, 0);
  
  free(img);
}

typedef struct
{
  BABYX *bbx;
  BBX_DialogPanel *pan;
  BBX_Label *text_lab;
  BBX_Button *ok_but;
} ABOUT;

void about_layout(void *obj, int width, int height);
void about_kill(void *obj);
void about_ok(void *obj);

void aboutdialog(BABYX *bbx)
{
  ABOUT about;

  about.bbx = bbx;
  about.pan = bbx_dialogpanel(bbx, "About Perlin Editor", 500, 300, about_layout, &about);
  about.text_lab = bbx_label(bbx, about.pan, 
               "Perlin editor\n"
               "\n"
               "by Malcolm McLean\n"
               "\n"
               "Generate textures with Perlin noise\n"
               "");
  about.ok_but = bbx_button(bbx, about.pan, "Ok", about_ok, &about);
  bbx_dialogpanel_setclosefunc(about.pan, about_kill, &about);


  bbx_dialogpanel_makemodal(about.pan);
  bbx_button_kill(about.ok_but);
  bbx_label_kill(about.text_lab);
  bbx_dialogpanel_kill(about.pan);
  
}

void about_layout(void *obj, int width, int height)
{
  ABOUT *about = obj;
  BABYX *bbx = about->bbx;
  bbx_setpos(bbx, about->text_lab, 10, 10, width-20, height - 50);
  bbx_setpos(bbx, about->ok_but, (width - 50)/ 2, height - 30, 50, 25);
}

void about_kill(void *obj)
{
	//ABOUT *about = obj;
	//bbx_button_kill(about->ok_but);
	//bbx_label_kill(about->text_lab);
	
}


void about_ok(void *obj)
{
  ABOUT *about = obj;
  bbx_dialogpanel_dropmodal(about->pan);
}


char *trim(char *str)
{
  char *answer = malloc(strlen(str) +1);
  int i = 0;
  if(!answer)
    return 0;
  while(isspace( (unsigned char) *str))
    str++;
  while(*str)
    answer[i++] = *str++;
  answer[i] = 0;
  i--;
  while(i >= 0 && isspace( (unsigned char) answer[i]))
    answer[i--] = 0;
  return answer;
}
