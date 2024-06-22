#include <stdlib.h>
#include <string.h>
#include "BabyX.h"
#include "paletteeditor.h"
#include "colorpicker.h"
#include "loadimage.h"
#include "gif.h"
#include "makepalette.h"

#include <stdio.h>
#define TOOL_NONE 0
#define TOOL_BRUSH 1
#define TOOL_FLOODFILL 2
#define TOOL_ERASER 3
// TOOL_EYEDROPPER
#define TOOL_RECTANGLE 4
#define TOOL_CIRCLE 5

typedef struct undo
{
    int i_width;
    int i_height;
    unsigned char *index;
    PALETTE pal;
    struct undo *prev;
    struct undo *next;
}UNDO;

typedef struct{
    BBX_RadioBox *brushtype_rad;
    BBX_Spinner *size_spn;
    
} BRUSH;

typedef struct{
    BBX_Label * floodfill_lab;
} FLOODFILL;

typedef struct
{
	BABYX *bbx;
	BBX_Panel *root;
    BBX_Canvas *can;
    BBX_Canvas * pal_can;
    BBX_Scrollbar *v_scroll_sb;
    BBX_Scrollbar *h_scroll_sb;
	BBX_Button *ok_but;
    BBX_Menubar *menubar;
    BBX_Spinner *zoom_spn;
    BBX_CheckBox *transparent_chk;
    BBX_Button *brush_but;
    BBX_Button *floodfill_but;
    BBX_Button *select_but;
    BRUSH brush;
    FLOODFILL floodfill;
    unsigned char *image;
    unsigned char *index;
    PALETTE pal;
    int Npal;
    int i_width;
    int i_height;
    int transparent;
    UNDO *undolist;
    UNDO *redolist;
    int dirty;
    int c_width;
    int c_height;
    int zoom;
    int vpos;
    int hpos;
    int sel_col;
    int current_tool;
} APP;

void createapp(void *obj, BABYX *bbx, BBX_Panel *root);
void killapp(void *obj);
void layoutapp(void *obj, int width, int height);
void canvasmouse(void *ptr, int action, int x, int y,int buttons);
void ok_pressed(void *obj);
void menuhandler(void *obj, int id);
void zoom(void *obj, double val);
void hscroll(void *obj, int pos);
void vscroll(void *obj, int pos);
void brush_pressed(void *obj);
void floodfill_pressed(void *obj);

void setcurrenttool(APP *app, int tool);
int redrawcanvas(APP * app);
int generateindexfromrgba(unsigned char *index, int width, int height, unsigned char *rgba, unsigned char *pal, int Npal);
int generatergbafromindex(unsigned char *rgba, int width, int height, unsigned char *index, unsigned char *pal, int Npal, int transparent);
void transparentbackground(unsigned char *rgba, int width, int height);

void initialdefaultpalette(unsigned char *pal, int Npal);
static void palettemouse(void *ptr, int action, int x, int y, int buttons);
static void drawpalette(APP *app);

void undo_commit(APP *app);
void undo_undo(APP *app);
void undo_redo(APP *app);
UNDO *makeundo(unsigned char *index, int width, int height, PALETTE *pal);
void undo_unlink(UNDO *undo);
void undo_kill(UNDO *undo);
void undo_kill_r(UNDO *undo);

char *getextension(char *fname);

int main(int argc, char**argv)
{
	APP app;
    
    if (argc == 2)
    {
        char *ext;
        unsigned char *rgba;
        int err;
        
        ext = getextension(argv[1]);
        
        if (!strcmp(ext, ".gif"))
        {
            app.pal.rgb = malloc(3 * 256);
            app.pal.N = 256;
            app.index = loadgif(argv[1], &app.i_width, &app.i_height, app.pal.rgb, &app.transparent);
            app.image = bbx_malloc(app.i_width * app.i_height * 4);
            generatergbafromindex(app.image, app.i_width, app.i_height, app.index, app.pal.rgb, app.Npal, app.transparent);
            app.zoom = 1;
            app.hpos = 0;
            app.vpos = 0;
            app.sel_col = 1;
            app.dirty = 0;
        }
        else
        {
            rgba = loadrgba(argv[1], &app.i_width, &app.i_height, &err);
            if (!rgba)
                exit(EXIT_FAILURE);
            app.image = rgba;
            app.pal.rgb = malloc(3 * 256);
            app.pal.N = 256;
            app.index = malloc(app.i_width * app.i_height);
            memset(app.index, 0, app.i_width *app.i_height);
            makepalette(app.pal.rgb+3, 255, rgba, app.i_width, app.i_height);
            app.transparent = 0;
            generateindexfromrgba(app.index, app.i_width, app.i_height, app.image, app.pal.rgb, app.pal.N);
            generatergbafromindex(app.image, app.i_width, app.i_height, app.index, app.pal.rgb, app.Npal, app.transparent);
          
            app.zoom = 1;
            app.hpos = 0;
            app.vpos = 0;
            app.sel_col = 1;
            app.dirty = 0;
        }
    }
    else if (argc == 1)
    {
        app.pal.rgb = malloc(3 * 256);
        app.pal.N = 256;
        app.i_width = 256;
        app.i_height = 256;
        app.index = bbx_malloc(256 * 256);
        app.image = bbx_malloc(app.i_width * app.i_height * 4);
        initialdefaultpalette(app.pal.rgb, 256);
        app.transparent = 0;
        memset(app.index, 255, app.i_width * app.i_height);
        generatergbafromindex(app.image, app.i_width, app.i_height, app.index, app.pal.rgb, app.Npal, app.transparent);
    
        app.zoom = 1;
        app.hpos = 0;
        app.vpos = 0;
        app.sel_col = 1;
        app.dirty = 0;
    }
    else
        exit(EXIT_FAILURE);
   
    
    app.c_width = 512;
    app.c_height = 512;
    
    app.current_tool = TOOL_NONE;
    
	startbabyx("Baby X Image Editor", 40 + app.c_width + 256 + 10, 100 + app.c_height, createapp, layoutapp, &app);
    free(app.image);

	return 0;
}


void createapp(void *obj, BABYX *bbx, BBX_Panel *root)
{
	APP *app = obj;
    BBX_Popup *filemenu;
    BBX_Popup *editmenu;
    BBX_Popup *palettemenu;
    BBX_Popup *helpmenu;
    BBX_Popup *custompalettesmenu;
	app->bbx = bbx;
	app->root = root;
    app->can = bbx_canvas(bbx, root, app->c_width, app->c_height, bbx_color("white"));
    bbx_canvas_setmousefunc(app->can, canvasmouse, app);
    app->pal_can = bbx_canvas(bbx, root, 16 *16, 16 * 16, bbx_color("white"));
    bbx_canvas_setmousefunc(app->pal_can, palettemouse, app);
    app->v_scroll_sb = bbx_scrollbar(bbx, root, BBX_SCROLLBAR_VERTICAL, vscroll, app);
    app->h_scroll_sb = bbx_scrollbar(bbx, root, BBX_SCROLLBAR_HORIZONTAL, hscroll, app);
    app->transparent_chk = bbx_checkbox(bbx, root, "Transparent", 0, app);
    app->brush_but = bbx_button(bbx, root, "Br", brush_pressed, app);
    app->floodfill_but = bbx_button(bbx, root, "Ff", floodfill_pressed, app);
    app->select_but = bbx_button(bbx, root, "Sel", 0, app);
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
    bbx_popup_append(filemenu, 211, "New", "...", 0);
    bbx_popup_append(filemenu, 210, "Open", "...", 0);
    bbx_popup_append(filemenu, 213, "Import", "...", 0);
    bbx_popup_append(filemenu, 1, "Save as GIF", "...", 0);
    bbx_popup_append(filemenu, 212, "Close", "", 0);
    bbx_popup_append(filemenu, 2, "Exit", "", 0);
    
    editmenu = bbx_popup(bbx);
    bbx_popup_append(editmenu, 201, "Undo", "", 0);
    bbx_popup_append(editmenu, 202, "Redo", "", 0);
    bbx_popup_append(editmenu, 203, "Select", "", 0);
    bbx_popup_append(editmenu, 204, "Resize", "...", 0);


    palettemenu = bbx_popup(bbx);
    bbx_popup_append(palettemenu, 4, "Edit palette", "...", 0);
    bbx_popup_append(palettemenu, 5, "Custom palette", "", custompalettesmenu);
    bbx_popup_append(palettemenu, 6, "Load palette from gif", "...", 0);

    helpmenu = bbx_popup(bbx);
    bbx_popup_append(helpmenu, 3, "About", "", 0);

    bbx_menubar_addmenu(app->menubar, "File", filemenu);
    bbx_menubar_addmenu(app->menubar, "Edit", editmenu);
    bbx_menubar_addmenu(app->menubar, "Palette", palettemenu);
    bbx_menubar_addmenu(app->menubar, "Help", helpmenu);
    
    app->zoom_spn = bbx_spinner(app->bbx, root, 1.0, 1.0, 10.0, 1, zoom, app);
    
    undo_commit(app);
    bbx_menubar_disable(app->menubar, 201);

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
    bbx_setpos(app->bbx, app->can, 10, 20, app->c_width, app->c_height);
    bbx_setpos(app->bbx, app->pal_can, 20 + app->c_width + 15, 25, 256, 256);
    bbx_setpos(app->bbx, app->v_scroll_sb, 10 + app->c_width, 20, 10, app->c_height);
    bbx_setpos(app->bbx, app->h_scroll_sb, 10, 20 + app->c_height, app->c_width, 10);
    bbx_setpos(app->bbx, app->transparent_chk, 20 + app->c_width  + 13, 285,120, 20);
    bbx_setpos(app->bbx, app->brush_but, 20 + app->c_width  + 13, 310, 40, 25);
    bbx_setpos(app->bbx, app->floodfill_but, 20 + app->c_width  + 13 + 50, 310, 40, 25);
    bbx_setpos(app->bbx, app->select_but, 20 + app->c_width  + 13 + 100, 310, 40, 25);
	bbx_setpos(app->bbx, app->ok_but, width / 2 - 25, height - 50, 50, 25);
    bbx_setpos(app->bbx, app->zoom_spn, 10, height - 50, 60, 25);
    
    
    int Nx, Ny;
    Nx = (app->i_width - app->hpos);
    Ny = (app->i_height - app->vpos);
    if (Nx * app->zoom > app->c_width)
        Nx = app->c_width/app->zoom;
    if (Ny * app->zoom > app->c_height)
        Ny = app->c_height/app->zoom;
    bbx_scrollbar_set(app->h_scroll_sb, app->i_width, Nx, app->hpos);
    bbx_scrollbar_set(app->v_scroll_sb, app->i_height, Ny, app->vpos);
    redrawcanvas(app);
    drawpalette(app);
}

void canvasmouse(void *ptr, int action, int x, int y, int buttons)
{
    APP *app = ptr;
    
    if (action == BBX_MOUSE_CLICK || (action == BBX_MOUSE_MOVE && (buttons & BBX_MOUSE_BUTTON1)))
    {
        int ix = app->hpos + x/app->zoom;
        int iy = app->vpos + y/app->zoom;
        
        app->index[iy * app->i_width + ix] = app->sel_col;
        app->dirty = 1;
        redrawcanvas(app);
    }
    if (action == BBX_MOUSE_RELEASE && app->dirty)
    {
        printf("Committing undo\n");
        undo_commit(app);
    }
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
    
    if (id == 1)
    {
        fname = bbx_getsavefile(app->bbx, "*.gif");
        if(fname)
        {
            savegif(fname, app->index, app->i_width, app->i_height, app->pal.rgb, app->pal.N, -1, 0, 0);
        }
        free(fname);
    }
  if (id == 4)
  {
      openpaletteeditor(app->bbx, &app->pal);
      generatergbafromindex(app->image, app->i_width, app->i_height, app->index, app->pal.rgb, app->pal.N, app->transparent);
      redrawcanvas(app);
      drawpalette(app);
  }
    if (id == 201)
    {
        undo_undo(app);
    }
    if (id == 202)
    {
        undo_redo(app);
    }
    if (id == 210)
    {
        unsigned char pal[256*3];
        int width, height;
        int transparent;
        unsigned char *index;
        char *fname;
        
        fname = bbx_getopenfile(app->bbx, "*.gif");
        if (!fname)
            return;
        index = loadgif(fname, &width, &height, pal, &transparent);
        if (!index)
            return;
        free(app->index);
        app->index = index;
        app->i_width = width;
        app->i_height = height;
        memcpy(app->pal.rgb, pal, 256 * 3);
        app->transparent = transparent;
        app->image = bbx_realloc(app->image, width * height * 4);
        
        app->hpos = 0;
        app->vpos = 0;
        
        int Nx, Ny;
        Nx = (app->i_width - app->hpos);
        Ny = (app->i_height - app->vpos);
        if (Nx * app->zoom > app->c_width)
            Nx = app->c_width/app->zoom;
        if (Ny * app->zoom > app->c_height)
            Ny = app->c_height/app->zoom;

        bbx_scrollbar_set(app->v_scroll_sb, app->i_height, Nx, app->vpos);
        bbx_scrollbar_set(app->h_scroll_sb, app->i_width, Ny, app->hpos);
        redrawcanvas(app);
        drawpalette(app);
    }
    if (id == 213)
    {
        unsigned char pal[256*3];
        int width, height;
        int transparent;
        unsigned char *index;
        char *fname;
        
        fname = bbx_getopenfile(app->bbx, "*.png");
        if (!fname)
            return;
        
        unsigned char *rgba;
        int err;
        
        rgba = loadrgba(fname, &app->i_width, &app->i_height, &err);
        if (!rgba)
            return;
        free(app->image);
        app->image = rgba;
        free(app->index);
        app->index = malloc(app->i_width * app->i_height);
        memset(app->index, 0, app->i_width *app->i_height);
        makepalette(app->pal.rgb+3, 255, rgba, app->i_width, app->i_height);
        app->transparent = 0;
        generateindexfromrgba(app->index, app->i_width, app->i_height, app->image, app->pal.rgb, app->pal.N);
        generatergbafromindex(app->image, app->i_width, app->i_height, app->index, app->pal.rgb, app->Npal, app->transparent);
        app->zoom = 1;
        app->hpos = 0;
        app->vpos = 0;
        app->sel_col = 1;
        app->dirty = 0;
        
        int Nx, Ny;
        Nx = (app->i_width - app->hpos);
        Ny = (app->i_height - app->vpos);
        if (Nx * app->zoom > app->c_width)
            Nx = app->c_width/app->zoom;
        if (Ny * app->zoom > app->c_height)
            Ny = app->c_height/app->zoom;

        bbx_scrollbar_set(app->v_scroll_sb, app->i_height, Nx, app->vpos);
        bbx_scrollbar_set(app->h_scroll_sb, app->i_width, Ny, app->hpos);
        
        redrawcanvas(app);
        drawpalette(app);
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

#include <stdio.h>

void zoom(void *obj, double val)
{
    APP *app = obj;
    int hpos, vpos;
    
    vpos = (app->vpos  + app->i_height/2) - (app->i_height * val)/(app->zoom * 2);
    if (vpos < 0)
        vpos = 0;
     
    hpos = 0;
    vpos = 0;
    app->zoom = val;
    app->vpos = vpos;
    int Nx, Ny;
    Nx = (app->i_width - app->hpos);
    Ny = (app->i_height - app->vpos);
    if (Nx * app->zoom > app->c_width)
        Nx = app->c_width/app->zoom;
    if (Ny * app->zoom > app->c_height)
        Ny = app->c_height/app->zoom;

    bbx_scrollbar_set(app->v_scroll_sb, app->i_height, Nx, vpos);
    bbx_scrollbar_set(app->h_scroll_sb, app->i_width, Ny, hpos);
    redrawcanvas(app);
}

void hscroll(void *obj, int pos)
{
    APP *app = obj;
    
    app->hpos = pos;
    redrawcanvas(app);
}

void vscroll(void *obj, int pos)
{
    APP *app = obj;
    
    app->vpos = pos;
    redrawcanvas(app);
}

void brush_pressed(void *obj)
{
    APP *app = obj;
    
    setcurrenttool(app, TOOL_BRUSH);
}

void floodfill_pressed(void *obj)
{
    APP *app = obj;
    
    setcurrenttool(app, TOOL_FLOODFILL);
}

void brush_selected(APP *app);
void brush_deselected(APP *app);

void floodfill_selected(APP *app);
void floodfill_deselected(APP *app);



void setcurrenttool(APP *app, int tool)
{
    switch(app->current_tool)
    {
        case TOOL_BRUSH: brush_deselected(app); break;
        case TOOL_FLOODFILL: floodfill_deselected(app); break;
    }
    app->current_tool = tool;
    switch(app->current_tool)
    {
        case TOOL_BRUSH: brush_selected(app); break;
        case TOOL_FLOODFILL: floodfill_selected(app); break;
    }
}

void brush_selected(APP *app)
{
    char* shapes[] = {"rectangle", "circle"};
    app->brush.brushtype_rad = bbx_radiobox(app->bbx, app->root, shapes, 2, 0, 0);
    app->brush.size_spn = bbx_spinner(app->bbx, app->root, 1, 1, 10, 1, 0, 0);
 
    bbx_setpos(app->bbx, app->brush.brushtype_rad, 20 + app->c_width  + 13, 350, 120, 50);
    bbx_setpos(app->bbx, app->brush.size_spn, 20 + app->c_width + 13, 410, 50, 25);
    
}

void brush_deselected(APP *app)
{
    bbx_radiobox_kill(app->brush.brushtype_rad);
    bbx_spinner_kill(app->brush.size_spn);
    app->brush.brushtype_rad = 0;
    app->brush.size_spn = 0;
}

void floodfill_selected(APP *app)
{
    app->floodfill.floodfill_lab = bbx_label(app->bbx, app->root, "Floodfill");
    bbx_setpos(app->bbx, app->floodfill.floodfill_lab, 20 + app->c_width  + 13, 350, 120, 25);
}

void floodfill_deselected(APP *app)
{
    bbx_label_kill(app->floodfill.floodfill_lab);
}

#include <stdio.h>
unsigned char *rgba_copy(unsigned char *rgba, int width, int height, int x, int y, int cwidth, int cheight);
int expandimage(unsigned char *dest, int width, int height, unsigned char *source, int swidth, int sheight);

int redrawcanvas(APP * app)
{
    unsigned char *rgba;
    int cw, ch;
    rgba = bbx_canvas_rgba(app->can, &cw, &ch);
    bbx_rectangle(rgba, cw, ch, 0.0, 0.0, cw, ch, bbx_color("yellow"));
    transparentbackground(rgba, cw, ch);
    generatergbafromindex(app->image, app->i_width, app->i_height, app->index, app->pal.rgb, app->pal.N, app->transparent);
    if (app->zoom > 0)
    {
        int zoom = app->zoom;
        int Nx, Ny;
        Nx = (app->i_width - app->hpos);
        Ny = (app->i_height - app->vpos);
        if (Nx * zoom > cw)
            Nx = cw/zoom;
        if (Ny * zoom > ch)
            Ny = ch/zoom;
  
        unsigned char *view = rgba_copy(app->image, app->i_width, app->i_height, app->hpos, app->vpos, Nx, Ny);
        unsigned char *expanded = bbx_malloc(Nx * zoom * Ny * zoom * 4);
        expandimage(expanded, Nx * zoom, Ny *zoom, view, Nx, Ny);
        bbx_paste(rgba, cw, ch, expanded, Nx*zoom, Ny*zoom, 0, 0);
        free(view);
        free(expanded);
    }

    bbx_canvas_flush(app->can);
}

int expandimage(unsigned char *dest, int width, int height, unsigned char *source, int swidth, int sheight)
{
    int zoom;
    int x, y;
    
    zoom = width / swidth;
    
    for (y = 0; y < sheight; y++)
    {
        for (x = 0; x < swidth; x++)
        {
            int iy = y * zoom;
            int ix = x * zoom;
            int jx, jy;
            
            for (jy = 0; jy < zoom; jy++)
            {
                for (jx=0; jx < zoom; jx++)
                {
                    dest[((iy + jy)*width + ix + jx) * 4] = source[(y*swidth+x) *4];
                    dest[((iy + jy)*width + ix + jx) * 4+1] = source[(y*swidth+x) *4+1];
                    dest[((iy + jy)*width + ix + jx) * 4+2] = source[(y*swidth+x) *4+2];
                    dest[((iy + jy)*width + ix + jx) * 4+3] = source[(y*swidth+x) *4+3];
                }
            }
        }
    }
}

unsigned char *rgba_copy(unsigned char *rgba, int width, int height, int x, int y, int cwidth, int cheight)
{
    unsigned char *copy;
    int cy = 0;
    int cx = 0;
    int sx, sy;
    
    copy = bbx_malloc(cwidth * cheight * 4);
    for (sy = y; sy < y + cheight; sy++)
    {
        for (sx = x; sx < x + cwidth; sx++)
        {
            copy[(cy * cwidth + cx)*4] = rgba[(sy * width +sx)*4];
            copy[(cy * cwidth + cx)*4+1] = rgba[(sy * width +sx)*4+1];
            copy[(cy * cwidth + cx)*4+2] = rgba[(sy * width +sx)*4+2];
            copy[(cy * cwidth + cx)*4+3] = rgba[(sy * width +sx)*4+3];
            cx++;
        }
        cy++;
        cx = 0;
    }
    
    return copy;
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
            if (rgba[(y * width + x) * 4 + 3] < 128)
                index[y*width+x] = 0;
        }
    }
        
    return 0;
}
/*
 *pal rgb
 */
int generatergbafromindex(unsigned char *rgba, int width, int height, unsigned char *index, unsigned char *pal, int Npal, int transparent)
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
            rgba[(y * width + x) * 4 + 3] = (ci == transparent ) ? 0 : 255;
        }
    }
    
    return  0;
}

void transparentbackground(unsigned char *rgba, int width, int height)
{
    BBX_RGBA dim = bbx_color("AliceWhite");
    BBX_RGBA light = bbx_color("LightGray");
    int x, y;
    
    for (y=0; y < height; y++)
        for(x = 0; x <width; x++)
        {
            if (((x/4) ^ (y/4)) & 0x1)
            {
                rgba[(y*width+x)*4+0] = bbx_red(dim);
                rgba[(y*width+x)*4+1] = bbx_green(dim);
                rgba[(y*width+x)*4+2] = bbx_blue(dim);
            }
            else
            {
                rgba[(y*width+x)*4+0] = bbx_red(light);
                rgba[(y*width+x)*4+1] = bbx_green(light);
                rgba[(y*width+x)*4+2] = bbx_blue(light);
            }
            //rgba[(y*width+x)*4+0] = 255;
            //rgba[(y*width+x)*4+1] = 0;
            //rgba[(y*width+x)*4+2] = 0;
            rgba[(y*width+x)*4+3] = 255;
        }
}

void initialdefaultpalette(unsigned char *pal, int Npal)
{
    int red, green, blue;
    int i = 0;
    
    if (Npal != 256)
        return;
    
    for (red = 0 ; red < 8; red++)
    {
        for (green = 0; green < 8; green++)
        {
            for (blue = 0; blue < 4; blue++)
            {
                pal[i * 3] = red * 32;
                pal[i * 3 + 1] = green * 32;
                pal[i * 3 + 2] = blue * 64;
                i++;
            }
        }
    }
   // sortpalette(pal, Npal);
}

static void palettemouse(void *ptr, int action, int x, int y, int buttons)
{
    APP *app = ptr;
    int ix, iy;
    
    ix = x / 16;
    iy = y / 16;

    
    if (action == BBX_MOUSE_CLICK)
    {
        app->sel_col = iy * 16 + ix;
        drawpalette(app);
    }
    if (action == BBX_MOUSE_CLICK &&
        ! (buttons & BBX_MOUSE_BUTTON1))
    {
        unsigned long oldrgb = ((unsigned long) app->pal.rgb[app->sel_col*3] << 16 |
                                 (unsigned long) app->pal.rgb[app->sel_col*3+1] << 8 |
                                 (unsigned long) app->pal.rgb[app->sel_col*3+ 2]);
        unsigned long rgb;
        rgb = pickcolor(app->bbx, oldrgb);
        if (rgb != oldrgb)
        {
            app->pal.rgb[app->sel_col*3] = (rgb >> 16) & 0xFF;
            app->pal.rgb[app->sel_col*3+1] = (rgb >> 8) & 0xFF;
            app->pal.rgb[app->sel_col*3+2] = rgb & 0xFF;
            drawpalette(app);
            redrawcanvas(app);
            undo_commit(app);
        }
    }
}

static void drawpalette(APP *app)
{
  unsigned char *rgba;
  int width, height;
  int x, y;
  int ix, iy;
  int i;
  unsigned char *pal = app->pal.rgb;
    int N = 256; //ped->pal->N;
  unsigned long col;
  unsigned long dimgrey;
  unsigned long higrey;

  rgba = bbx_canvas_rgba(app->pal_can, &width, &height);
  dimgrey = BBX_Color("DimGray");
  higrey = BBX_Color("LightGray");
  col = BBX_Color("Gray");
  for(i=0;i<width*height;i++)
  {
    rgba[i*4] = (col >> 16) & 0xFF;
    rgba[i*4+1] = (col >> 8) & 0xFF;
    rgba[i*4+2] = col & 0xFF;
  }

  for(i=0;i<N;i++)
  {
    x = i % 16;
    y = i / 16;
    if(y*16+x == app->sel_col)
    {
      for(iy=-1;iy<17;iy++)
        for(ix=-1;ix<17;ix++)
    {
          if(y + iy < 0 || y + iy >= height || x + ix < 0 || x + ix >= width)
            continue;
          rgba[((y*16+iy)*width+(x*16+ix))*4] = 0;
          rgba[((y*16+iy)*width+(x*16+ix))*4+1] = 255;
          rgba[((y*16+iy)*width+(x*16+ix))*4+2] = 255;
    }
    }
    else
    {
      for(iy=1;iy<15;iy++)
      {
        rgba[((y*16+iy)*width+(x*16+1))*4] = (dimgrey >> 16) & 0xFF;
        rgba[((y*16+iy)*width+(x*16+1))*4+1] = (dimgrey >> 8) & 0xFF;
        rgba[((y*16+iy)*width+(x*16+1))*4+2] = (dimgrey) & 0xFF;
      }
      for(ix=1;ix<15;ix++)
      {
        rgba[((y*16+1)*width+(x*16+ix))*4] = (dimgrey >> 16) & 0xFF;
        rgba[((y*16+1)*width+(x*16+ix))*4+1] = (dimgrey >> 8) & 0xFF;
        rgba[((y*16+1)*width+(x*16+ix))*4+2] = (dimgrey) & 0xFF;
      }
    
      for(iy=1;iy<15;iy++)
      {
        rgba[((y*16+iy)*width+(x*16+14))*4] = (higrey >> 16) & 0xFF;
        rgba[((y*16+iy)*width+(x*16+14))*4+1] = (higrey >> 8) & 0xFF;
        rgba[((y*16+iy)*width+(x*16+14))*4+2] = (higrey) & 0xFF;
      }
      for(ix=1;ix<15;ix++)
      {
        rgba[((y*16+14)*width+(x*16+ix))*4] = (higrey >> 16) & 0xFF;
        rgba[((y*16+14)*width+(x*16+ix))*4+1] = (higrey >> 8) & 0xFF;
        rgba[((y*16+14)*width+(x*16+ix))*4+2] = (higrey) & 0xFF;
      }
    
    }
    
    for(iy=0;iy<12;iy++)
      for(ix=0;ix<12;ix++)
      {
        rgba[((y*16+iy+2)*width+(x*16+ix+2))*4] = pal[i*3];
        rgba[((y*16+iy+2)*width+(x*16+ix+2))*4+1] = pal[i*3+1];
        rgba[((y*16+iy+2)*width+(x*16+ix+2))*4+2] = pal[i*3+2];
      }
    
  }
    
    for(iy=0;iy<12;iy++)
      for(ix=0;ix<12;ix++)
      {
          int tx = app->transparent % 16;
          int ty = app->transparent /16;
        rgba[((ty*16+iy+2)*width+(tx*16+ix+2))*4] = 0xFF;
        rgba[((ty*16+iy+2)*width+(tx*16+ix+2))*4+1] = 0xFF;
        rgba[((ty*16+iy+2)*width+(tx*16+ix+2))*4+2] = 0xFF;
      }
    bbx_lineaa(rgba, width, height, 3, 14, 14, 3, 2.0, bbx_color("red"));
  bbx_canvas_flush(app->pal_can);
}


void undo_commit(APP *app)
{
    UNDO *undo;
    undo = makeundo(app->index, app->i_width, app->i_height, &app->pal);
    undo->next = app->undolist;
    if (undo->next)
        undo->next->prev = undo;
    app->undolist = undo;
    undo_kill_r(app->redolist);
    app->redolist = 0;
    bbx_menubar_enable(app->menubar, 201);
    bbx_menubar_disable(app->menubar, 202);
    app->dirty = 0;
}

void undo_restore(APP *app, UNDO *undo)
{
    if (undo->i_width == app->i_width &&
        undo->i_height == app->i_height)
    {
        memcpy(app->index, undo->index, app->i_width * app->i_height);
    }
    if (undo->pal.N == app->pal.N)
    {
        memcpy(app->pal.rgb, undo->pal.rgb, app->pal.N * 3);
    }
}

void undo_undo(APP *app)
{
    if (app->undolist)
    {
       
        if (!app->dirty && app->undolist->next)
        {
            undo_restore(app, app->undolist->next);
        }
        else
        {
            undo_restore(app, app->undolist);
        }
        if (app->undolist->next)
        {
            UNDO *undo = app->undolist;
            app->undolist = app->undolist->next;
            undo_unlink(undo);
            undo->next = app->redolist;
            if (undo->next)
                undo->next->prev = undo;
            app->redolist = undo;
        }
        if (!app->dirty && !app->undolist->next)
        {
            bbx_menubar_disable(app->menubar, 201);
        }
       if (app->redolist)
           bbx_menubar_enable(app->menubar, 202);
        redrawcanvas(app);
        app->dirty = 0;
    }
}

void undo_redo(APP *app)
{
    if (app->redolist)
    {
        UNDO *redo = app->redolist;
        app->redolist = redo->next;
        undo_unlink(redo);
        
        undo_restore(app, redo);
        redo->next = app->undolist;
        if (redo->next)
            redo->next->prev = redo;
        app->undolist = redo;
        bbx_menubar_enable(app->menubar, 201);
        if (!app->redolist)
            bbx_menubar_disable(app->menubar, 202);
        redrawcanvas(app);
        app->dirty = 0;
    }
}
    

UNDO *makeundo(unsigned char *index, int width, int height, PALETTE *pal)
{
    UNDO *undo;
    
    undo = bbx_malloc(sizeof(UNDO));
    undo->index = bbx_malloc(width * height);
    memcpy(undo->index, index, width * height);
    undo->i_width = width;
    undo->i_height = height;
    undo->pal.rgb = bbx_malloc(pal->N * 3);
    memcpy(undo->pal.rgb, pal->rgb, pal->N * 3);
    undo->pal.N = pal->N;
    undo->prev = 0;
    undo->next = 0;
    
    return undo;
}

void undo_kill(UNDO *undo)
{
    if (undo)
    {
        free(undo->index);
        free(undo->pal.rgb);
        free(undo);
    }
}

void undo_kill_r(UNDO *undo)
{
    if (undo)
    {
        undo_kill_r(undo->next);
        undo_kill(undo);
    }
}

void undo_unlink(UNDO *undo)
{
    if (undo->prev)
        undo->prev->next = undo->next;
    if (undo->next)
        undo->next->prev = undo->prev;
    undo->next = 0;
    undo->prev = 0;
}


char *getextension(char *fname)
{
  char *ext;
  char *answer;

  ext = strrchr(fname, '.');
  if(!ext || ext == fname || strchr(ext, '/') || strchr(ext, '\\'))
  {
    answer = malloc(1);
    if(answer)
      answer[0] = 0;
  }
  else
  {
    answer = malloc(strlen(ext) + 1);
    if(answer)
      strcpy(answer, ext);
  }
  return answer;
}
