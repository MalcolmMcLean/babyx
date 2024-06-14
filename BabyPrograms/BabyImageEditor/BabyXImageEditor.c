#include <stdlib.h>
#include <string.h>
#include "BabyX.h"
#include "paletteeditor.h"
#include "loadimage.h"
#include "makepalette.h"


typedef struct
{
	BABYX *bbx;
	BBX_Panel *root;
    BBX_Canvas *can;
	BBX_Button *ok_but;
    BBX_Menubar *menubar;
    unsigned char *image;
    unsigned char *index;
    PALETTE pal;
    int Npal;
    int width;
    int height;
} APP;

void createapp(void *obj, BABYX *bbx, BBX_Panel *root);
void killapp(void *obj);
void layoutapp(void *obj, int width, int height);
void ok_pressed(void *obj);
void menuhandler(void *obj, int id);

int generateindexfromrgba(unsigned char *index, int width, int height, unsigned char *rgba, unsigned char *pal, int Npal);
int generatergbafromindex(unsigned char *rgba, int width, int height, unsigned char *index, unsigned char *pal, int Npal);

int main(int argc, char**argv)
{
	APP app;
    
    if (argc == 2)
    {
        unsigned char *rgba;
        int err;
        
        rgba = loadrgba(argv[1], &app.width, &app.height, &err);
        if (!rgba)
            exit(EXIT_FAILURE);
        app.image = rgba;
        app.pal.rgb = malloc(3 * 256);
        app.pal.N = 256;
        app.index = malloc(app.width * app.height);
        memset(app.index, 0, app.width *app.height);
        makepalette(app.pal.rgb, 256, rgba, app.width, app.height);
        generateindexfromrgba(app.index, app.width, app.height, app.image, app.pal.rgb, app.pal.N);
        generatergbafromindex(app.image, app.width, app.height, app.index, app.pal.rgb, app.Npal);
    }
    else
        exit(EXIT_FAILURE);
    
	startbabyx("Baby X Image Editor", 20 + app.width, 80 + app.height, createapp, layoutapp, &app);
    free(app.image);

	return 0;
}


void createapp(void *obj, BABYX *bbx, BBX_Panel *root)
{
	APP *app = obj;
    BBX_Popup *filemenu;
    BBX_Popup *palettemenu;
    BBX_Popup *helpmenu;
    BBX_Popup *custompalettesmenu;
	app->bbx = bbx;
	app->root = root;
    app->can = bbx_canvas(bbx, root, app->width, app->height, bbx_color("white"));
	app->ok_but = bbx_button(bbx, root, "OK", ok_pressed, app);
    
    
    //app->pal.N = 256;
    //app->pal.rgb = malloc(256 * 3);
    //creategrey(app->pal.rgb, 256);

 //   app->ticker = 0;
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

    /*
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
    bbx_label_setbackground(app->octaves_lab, bbx_color("LightGray"));
    bbx_label_setbackground(app->persistence_lab, bbx_color("LightGray"));
    bbx_label_setbackground(app->scale_lab, bbx_color("LightGray"));
    bbx_label_setbackground(app->deltax_lab, bbx_color("LightGray"));
    bbx_label_setbackground(app->deltay_lab, bbx_color("LightGray"));
    bbx_label_setbackground(app->deltat_lab, bbx_color("LightGray"));

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
     */
}

void killapp(void *obj)
{
	APP *app = obj;

    bbx_canvas_kill(app->can);
	bbx_button_kill(app->ok_but);
}


void layoutapp(void *obj, int width, int height)
{
	APP *app = obj;
    
    bbx_setpos(app->bbx, app->menubar, 0, 0, width, 20);
    bbx_setpos(app->bbx, app->can, 10, 10, app->width, app->height);
	bbx_setpos(app->bbx, app->ok_but, width / 2 - 25, height - 50, 50, 25);
    bbx_canvas_setimage(app->can, app->image, app->width, app->height);
    bbx_canvas_flush(app->can);
}

void ok_pressed(void *obj)
{
	APP *app = obj;

	killapp(app);
	stopbabyx(app->bbx);
}

void menuhandler(void *obj, int id)
{
  APP *app = obj;
  char *fname;
    
  if (id == 4)
  {
      unsigned char *can;
      openpaletteeditor(app->bbx, &app->pal);
      generatergbafromindex(app->image, app->width, app->height, app->index, app->pal.rgb, app->pal.N);
      bbx_canvas_setimage(app->can, app->image, app->width, app->height);
      bbx_canvas_flush(app->can);
  }
     
/*
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
 */
}

int generateindexfromrgba(unsigned char *index, int width, int height, unsigned char *rgba, unsigned char *pal, int Npal)
{
    int x, y;
    int j;
    int best;
    int bestj;
    BBX_RGBA pix;
    int d;
    
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            best = 10000;
            bestj = 0;
            for (j = 0; j < Npal; j++)
            {
                pix = bbx_rgba(
                               rgba[(y * width + x) * 4],
                               rgba[(y * width + x) * 4 + 1],
                               rgba[(y * width + x) * 4 + 2],
                               rgba[(y * width + x) * 4 + 3]);
                d = abs((int) bbx_red(pix) - pal[j * 3]) +
                    abs((int) bbx_green(pix) - pal[j * 3 + 1]) +
                    abs((int) bbx_blue(pix) - pal[j * 3 + 2]);
                
                if (d < best)
                {
                    best = d;
                    bestj = j;
                }
            }
            index[y*width+x] = bestj;
        }
    }
        
    return 0;
}
/*
 *pal rgb
 */
int generatergbafromindex(unsigned char *rgba, int width, int height, unsigned char *index, unsigned char *pal, int Npal)
{
    int x, y;
    int ci;
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            ci = index[y * width+ x];
            rgba[(y * width + x) * 4] = pal[ci * 3 + 0];
            rgba[(y * width + x) * 4 + 1] = pal[ci * 3 + 1];
            rgba[(y * width + x) * 4 + 2] = pal[ci * 3 + 2];
        }
    }
    
    return  0;
}
