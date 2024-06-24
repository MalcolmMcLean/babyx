#include <stdlib.h>
#include <string.h>
#include <math.h>
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
#define TOOL_SHAPE 3
#define TOOL_SELECT 4
// TOOL_EYEDROPPER
#define TOOL_RECTANGLE 4
#define TOOL_CIRCLE 5

#define SHAPE_NONE 0
#define SHAPE_RECTANGLE 1
#define SHAPE_CIRCLE 2

typedef struct undo
{
    int i_width;
    int i_height;
    unsigned char *index;
    PALETTE pal;
    int transparent;
    struct undo *prev;
    struct undo *next;
}UNDO;

typedef struct{
    BBX_RadioBox *brushtype_rad;
    BBX_Spinner *size_spn;
    int shape;
    int size;
} BRUSH;

typedef struct{
    BBX_Label * floodfill_lab;
} FLOODFILL;

typedef struct {
    BBX_Label *shape_lab;
    BBX_RadioBox *type_rad;
} SHAPE;

typedef struct {
    BBX_Label *select_lab;
    BBX_Spinner *x_spn;
    BBX_Spinner *y_spn;
    BBX_Spinner *width_spn;
    BBX_Spinner *height_spn;
    int x;
    int y;
    int width;
    int height;
} SELECT;

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
    SELECT select;
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
void select_pressed(void *obj);

void brush_canvasmouse(APP *app, int action, int x, int y, int buttons);
void floodfill_canvasmouse(APP *app, int action, int x, int y, int buttons);
void select_canvasmouse(APP *app, int action, int x, int y, int buttons);

void setcurrenttool(APP *app, int tool);
int redrawcanvas(APP * app);
int generateindexfromrgba(unsigned char *index, int width, int height, unsigned char *rgba, unsigned char *pal, int Npal);
int generatergbafromindex(unsigned char *rgba, int width, int height, unsigned char *index, unsigned char *pal, int Npal, int transparent);
void transparentbackground(unsigned char *rgba, int width, int height);

void initialdefaultpalette(unsigned char *pal, int Npal);
static void palettemouse(void *ptr, int action, int x, int y, int buttons);
static void drawpalette(APP *app);

void transparent_pressed(void *obj);

void undo_commit(APP *app);
void undo_undo(APP *app);
void undo_redo(APP *app);
UNDO *makeundo(unsigned char *index, int width, int height, PALETTE *pal, int transparent);
void undo_unlink(UNDO *undo);
void undo_kill(UNDO *undo);
void undo_kill_r(UNDO *undo);

int setsize(BABYX *bbx, int *width, int *height);

char *getextension(char *fname);
int floodfill4(unsigned char *grey, int width, int height, int x, int y, unsigned char target, unsigned char dest);

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
    
    app.brush.shape = SHAPE_RECTANGLE;
    app.brush.size = 1;
    
    app.select.x = 0;
    app.select.y = 0;
    app.select.width = app.i_width;
    app.select.height = app.i_height;
    
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
    app->transparent_chk = bbx_checkbox(bbx, root, "Transparent", transparent_pressed, app);
    app->brush_but = bbx_button(bbx, root, "Br", brush_pressed, app);
    app->floodfill_but = bbx_button(bbx, root, "Ff", floodfill_pressed, app);
    app->select_but = bbx_button(bbx, root, "Sel", select_pressed, app);
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
    
    if (action == BBX_MOUSE_CLICK && (buttons & BBX_MOUSE_BUTTON1))
    {
        if (app->current_tool == TOOL_FLOODFILL)
        {
            floodfill_canvasmouse(app, action, x, y, buttons);
        }
    }
    if (action == BBX_MOUSE_CLICK || (action == BBX_MOUSE_MOVE && (buttons & BBX_MOUSE_BUTTON1)))
    {
        int ix = app->hpos + x/app->zoom;
        int iy = app->vpos + y/app->zoom;
        
        if (app->current_tool == TOOL_BRUSH)
        {
            brush_canvasmouse(app, action, x, y, buttons);
        }
        else if (app->current_tool == TOOL_FLOODFILL)
        {
            
        }
        else if (app->current_tool == TOOL_SELECT)
        {
            select_canvasmouse(app, action, x, y, buttons);
        }
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
      undo_commit(app);
  }
    if (id == 6)
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
        free(index);
        memcpy(app->pal.rgb, pal, 256 * 3);
        app->transparent = transparent;
        redrawcanvas(app);
        drawpalette(app);
        undo_commit(app);
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

        bbx_scrollbar_set(app->v_scroll_sb, app->i_height, Ny, app->vpos);
        bbx_scrollbar_set(app->h_scroll_sb, app->i_width, Nx, app->hpos);
        redrawcanvas(app);
        drawpalette(app);
        undo_commit(app);
    }
    if (id == 211)
    {
        unsigned char pal[256*3];
        int width, height;
        int transparent;
        unsigned char *index;
        char *fname;
        
        width = 256;
        height = 256;
        setsize(app->bbx, &width, &height);
        app->index = bbx_malloc(width * height);
        memset(app->index, 255, width * height);
        app->i_width = width;
        app->i_height = height;
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

        bbx_scrollbar_set(app->v_scroll_sb, app->i_height, Ny, app->vpos);
        bbx_scrollbar_set(app->h_scroll_sb, app->i_width, Nx, app->hpos);
        redrawcanvas(app);
        drawpalette(app);
        undo_commit(app);
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

        bbx_scrollbar_set(app->v_scroll_sb, app->i_height, Ny, app->vpos);
        bbx_scrollbar_set(app->h_scroll_sb, app->i_width, Nx, app->hpos);
        
        redrawcanvas(app);
        drawpalette(app);
        undo_commit(app);
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
    app->hpos = hpos;
    int Nx, Ny;
    Nx = (app->i_width - app->hpos);
    Ny = (app->i_height - app->vpos);
    if (Nx * app->zoom > app->c_width)
        Nx = app->c_width/app->zoom;
    if (Ny * app->zoom > app->c_height)
        Ny = app->c_height/app->zoom;

    bbx_scrollbar_set(app->v_scroll_sb, app->i_height, Ny, vpos);
    bbx_scrollbar_set(app->h_scroll_sb, app->i_width, Nx, hpos);
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

void select_pressed(void *obj)
{
    APP *app = obj;
    
    setcurrenttool(app, TOOL_SELECT);
}


void brush_selected(APP *app);
void brush_deselected(APP *app);

void floodfill_selected(APP *app);
void floodfill_deselected(APP *app);

void select_selected(APP *app);
void select_deselected(APP *app);




void setcurrenttool(APP *app, int tool)
{
    switch(app->current_tool)
    {
        case TOOL_BRUSH: brush_deselected(app); break;
        case TOOL_FLOODFILL: floodfill_deselected(app); break;
        case TOOL_SELECT: select_deselected(app); break;
    }
    app->current_tool = tool;
    switch(app->current_tool)
    {
        case TOOL_BRUSH: brush_selected(app); break;
        case TOOL_FLOODFILL: floodfill_selected(app); break;
        case TOOL_SELECT: select_selected(app); break;
    }
}

void setselectedcolour(APP *app, int sel_col)
{
    if (sel_col == app->transparent)
        bbx_checkbox_setstate(app->transparent_chk, 1);
    else
        bbx_checkbox_setstate(app->transparent_chk, 0);
    if (sel_col != app->sel_col)
        app->sel_col = sel_col;
    drawpalette(app);
}

void brush_selected(APP *app)
{
    char* shapes[] = {"rectangle", "circle"};
    int brushindex = 0;
    app->brush.brushtype_rad = bbx_radiobox(app->bbx, app->root, shapes, 2, 0, 0);
    app->brush.size_spn = bbx_spinner(app->bbx, app->root, 1, 1, 10, 1, 0, 0);
    
    bbx_spinner_setvalue(app->brush.size_spn, app->brush.size);
    switch (app->brush.shape)
    {
        case SHAPE_RECTANGLE: brushindex = 0; break;
        case SHAPE_CIRCLE: brushindex = 1; break;
    }
    bbx_radiobox_setselected(app->brush.brushtype_rad, brushindex);
 
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

void brush_canvasmouse(APP *app, int action, int x, int y, int buttons)
{
    if (action == BBX_MOUSE_CLICK && (buttons & BBX_MOUSE_BUTTON1))
    {
        if (app->brush.brushtype_rad && app->brush.size_spn)
        {
            app->brush.size = (int) bbx_spinner_getvalue(app->brush.size_spn);
            app->brush.shape = bbx_radiobox_getselected(app->brush.brushtype_rad);
            if (app->brush.shape == 0)
                app->brush.shape = SHAPE_RECTANGLE;
            else
                app->brush.shape = SHAPE_CIRCLE;
        }
    }
    if ((action == BBX_MOUSE_CLICK || action == BBX_MOUSE_MOVE) && (buttons & BBX_MOUSE_BUTTON1))
    {
        int ix = app->hpos + x/app->zoom;
        int iy = app->vpos + y/app->zoom;
        int x, y;
        
        if (app->brush.shape == SHAPE_RECTANGLE)
        {
            ix -= app->brush.size / 2;
            iy -= app->brush.size / 2;
            for (y =0; y <app->brush.size; y++)
                for(x = 0; x < app->brush.size; x++ )
                {
                    if (iy + y >+ 0 && iy + y < app->i_height &&
                        ix + x >= 0 && ix + x < app->i_width)
                        app->index[(iy + y) * app->i_width + ix + x] = app->sel_col;
                }
        }
        else if (app->brush.shape == SHAPE_CIRCLE)
        {
            int size = app->brush.size;
            
            if ((size % 2) == 0)
                size++;
            
            ix -= size / 2;
            iy -= size / 2;
            for (y =0; y <size; y++)
                for(x = 0; x < size; x++ )
                {
                    double d = sqrt((x - size/2) * (x - size/2) + (y - size/2) * (y -size/2));
                    if (d > size/2.0)
                        continue;
                    if (iy + y >+ 0 && iy + y < app->i_height &&
                        ix + x >= 0 && ix + x < app->i_width)
                        app->index[(iy + y) * app->i_width + ix + x] = app->sel_col;
                }
        }
        app->dirty = 1;
        redrawcanvas(app);
    }
}

void floodfill_selected(APP *app)
{
    app->floodfill.floodfill_lab = bbx_label(app->bbx, app->root, "Floodfill");
    bbx_setpos(app->bbx, app->floodfill.floodfill_lab, 20 + app->c_width  + 13, 350, 120, 25);
}

void floodfill_deselected(APP *app)
{
    bbx_label_kill(app->floodfill.floodfill_lab);
    
    app->floodfill.floodfill_lab = 0;
}

void floodfill_canvasmouse(APP *app, int action, int x, int y, int buttons)
{
    if (action == BBX_MOUSE_CLICK && (buttons & BBX_MOUSE_BUTTON1))
    {
        int ix = app->hpos + x/app->zoom;
        int iy = app->vpos + y/app->zoom;
        
        floodfill4(app->index, app->i_width, app->i_height, ix, iy, app->index[iy * app->i_width + ix], app->sel_col);
        app->dirty = 1;
        redrawcanvas(app);
    }
}

void select_selected(APP *app)
{
    int x = app->select.x;
    int y = app->select.y;
    int width = app->select.width;
    int height = app->select.height;
    
    if (x > app->i_width -1 )
        x = 0;
    if (y > app->i_height - 1)
        y = 0;
    if (x + width > app->i_width)
        width = app->i_width - x;
    if (y + height > app->i_height)
        height = app->i_height - y;
    
    app->select.select_lab = bbx_label(app->bbx, app->root, "Select");
    app->select.x_spn = bbx_spinner(app->bbx, app->root, x, 0.0, app->i_width-1, 1, 0, 0);
    app->select.y_spn = bbx_spinner(app->bbx, app->root, y, 0.0, app->i_height-1, 1, 0, 0);
    app->select.width_spn = bbx_spinner(app->bbx, app->root, width, 0.0, app->i_width, 1, 0, 0);
    app->select.height_spn = bbx_spinner(app->bbx, app->root, height, 0.0, app->i_height, 1, 0, 0);
 
 
    bbx_setpos(app->bbx, app->select.select_lab, 20 + app->c_width  + 13, 350, 120, 25);
    bbx_setpos(app->bbx, app->select.x_spn, 20 + app->c_width + 13, 410, 50, 25);
    bbx_setpos(app->bbx, app->select.y_spn, 20 + app->c_width + 13 + 60, 410, 50, 25);
    bbx_setpos(app->bbx, app->select.width_spn, 20 + app->c_width + 13 + 120, 410, 50, 25);
    bbx_setpos(app->bbx, app->select.height_spn, 20 + app->c_width + 13 + 180, 410, 50, 25);
}

void select_deselected(APP *app)
{
    bbx_label_kill(app->select.select_lab);
    bbx_spinner_kill(app->select.x_spn);
    bbx_spinner_kill(app->select.y_spn);
    bbx_spinner_kill(app->select.width_spn);
    bbx_spinner_kill(app->select.height_spn);
    app->select.select_lab = 0;
    app->select.x_spn = 0;
    app->select.y_spn = 0;
    app->select.width_spn = 0;
    app->select.height_spn = 0;
}

void select_canvasmouse(APP *app, int action, int x, int y, int buttons)
{
    unsigned char *rgba;
    int cw, ch;
    
    if (action == BBX_MOUSE_CLICK && (buttons & BBX_MOUSE_BUTTON1))
    {
        int ix = app->hpos + x/app->zoom;
        int iy = app->vpos + y/app->zoom;
        
        app->select.x = ix;
        app->select.y = iy;
        app->select.width = 1;
        app->select.height = 1;
        redrawcanvas(app);
        bbx_spinner_setvalue(app->select.x_spn, app->select.x);
        bbx_spinner_setvalue(app->select.y_spn, app->select.y);
        bbx_spinner_setvalue(app->select.width_spn, app->select.width);
        bbx_spinner_setvalue(app->select.height_spn, app->select.height);
    }
    if (action == BBX_MOUSE_MOVE && (buttons & BBX_MOUSE_BUTTON1))
    {
        int ix = app->hpos + x/app->zoom;
        int iy = app->vpos + y/app->zoom;
        
        app->select.width = ix - app->select.x;
        app->select.height = iy - app->select.y;
        bbx_spinner_setvalue(app->select.x_spn, app->select.x);
        bbx_spinner_setvalue(app->select.y_spn, app->select.y);
        bbx_spinner_setvalue(app->select.width_spn, app->select.width);
        bbx_spinner_setvalue(app->select.height_spn, app->select.height);
        redrawcanvas(app);
        rgba = bbx_canvas_rgba(app->can, &cw, &ch);
        for (y = 0; y < ch; y++)
        {
            for (x = 0; x < cw; x++)
            {
                ix = app->hpos + x/app->zoom;
                iy = app->vpos + y/app->zoom;
                
                if (! (ix >= app->select.x &&
                       ix < app->select.x + app->select.width &&
                    iy >= app->select.y &&
                    iy < app->select.y + app->select.height))
                {
                    rgba[(y *cw + x) *4] /= 2;
                    rgba[(y *cw + x) *4+1] /= 2;
                    rgba[(y *cw + x) * 4 + 2] /= 2;
                }
            }
        }
        bbx_canvas_flush(app->can);
        
    }
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
        if (ix < 0 || ix >= 16 || iy < 0 || iy >= 16)
            return;
        setselectedcolour(app, iy * 16 + ix);
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
    
  if (app->transparent > 0)
  {
      int tx = app->transparent % 16;
      int ty = app->transparent /16;
      for(iy=0;iy<12;iy++)
          for(ix=0;ix<12;ix++)
          {
              rgba[((ty*16+iy+2)*width+(tx*16+ix+2))*4] = 0xFF;
              rgba[((ty*16+iy+2)*width+(tx*16+ix+2))*4+1] = 0xFF;
              rgba[((ty*16+iy+2)*width+(tx*16+ix+2))*4+2] = 0xFF;
          }
      bbx_lineaa(rgba, width, height, 3 +tx *16 , 14 +ty*16, 14 + tx *16, 3 + ty * 16, 2.0, bbx_color("red"));
  }
  bbx_canvas_flush(app->pal_can);
}

void transparent_pressed(void *obj)
{
    APP *app = obj;
    int transon = bbx_checkbox_getstate(app->transparent_chk);
    
    if (transon)
        app->transparent = app->sel_col;
    else
        app->transparent = -1;
    
    redrawcanvas(app);
    drawpalette(app);
    
    undo_commit(app);
}

void undo_commit(APP *app)
{
    UNDO *undo;
    undo = makeundo(app->index, app->i_width, app->i_height, &app->pal, app->transparent);
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
    else
    {
        app->index = bbx_realloc(app->index, undo->i_width * undo->i_height);
        app->image = bbx_realloc(app->image, undo->i_width * undo->i_height *4);
        app->i_width = undo->i_width;
        app->i_height = undo->i_height;
        app->hpos = 0;
        app->vpos = 0;
        
        memcpy(app->index, undo->index, app->i_width * app->i_height);
    }
    if (undo->pal.N == app->pal.N)
    {
        memcpy(app->pal.rgb, undo->pal.rgb, app->pal.N * 3);
    }
    app->transparent = undo->transparent;
}

void undo_undo(APP *app)
{
    if (app->undolist)
    {
       
        if (!app->dirty && app->undolist->next)
        {
            undo_restore(app, app->undolist->next);
            int Nx, Ny;
            Nx = (app->i_width - app->hpos);
            Ny = (app->i_height - app->vpos);
            if (Nx * app->zoom > app->c_width)
                Nx = app->c_width/app->zoom;
            if (Ny * app->zoom > app->c_height)
                Ny = app->c_height/app->zoom;

            bbx_scrollbar_set(app->v_scroll_sb, app->i_height, Ny, app->vpos);
            bbx_scrollbar_set(app->h_scroll_sb, app->i_width, Nx, app->hpos);
        }
        else
        {
            undo_restore(app, app->undolist);
            int Nx, Ny;
            Nx = (app->i_width - app->hpos);
            Ny = (app->i_height - app->vpos);
            if (Nx * app->zoom > app->c_width)
                Nx = app->c_width/app->zoom;
            if (Ny * app->zoom > app->c_height)
                Ny = app->c_height/app->zoom;

            bbx_scrollbar_set(app->v_scroll_sb, app->i_height, Ny, app->vpos);
            bbx_scrollbar_set(app->h_scroll_sb, app->i_width, Nx, app->hpos);
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
        setselectedcolour(app, app->sel_col);
        redrawcanvas(app);
        drawpalette(app);
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
        int Nx, Ny;
        Nx = (app->i_width - app->hpos);
        Ny = (app->i_height - app->vpos);
        if (Nx * app->zoom > app->c_width)
            Nx = app->c_width/app->zoom;
        if (Ny * app->zoom > app->c_height)
            Ny = app->c_height/app->zoom;

        bbx_scrollbar_set(app->v_scroll_sb, app->i_height, Ny, app->vpos);
        bbx_scrollbar_set(app->h_scroll_sb, app->i_width, Nx, app->hpos);
        if (redo->next)
            redo->next->prev = redo;
        app->undolist = redo;
        bbx_menubar_enable(app->menubar, 201);
        if (!app->redolist)
            bbx_menubar_disable(app->menubar, 202);
        setselectedcolour(app, app->sel_col);
        redrawcanvas(app);
        drawpalette(app);
        app->dirty = 0;
    }
}
    

UNDO *makeundo(unsigned char *index, int width, int height, PALETTE *pal, int transparent)
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
    undo->transparent = transparent;
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

typedef struct
{
    BABYX *bbx;
    BBX_Panel *pan;
    BBX_Label *width_lab;
    BBX_Label *height_lab;
    BBX_Spinner *width_spn;
    BBX_Spinner *height_spn;
    BBX_Button *ok_but;
    BBX_Button *cancel_but;
    int ok;
} SETSIZE;

void layoutsize(void *obj, int width, int height);
void killsize(void *ptr);
void pressok(void *obj);
void presscancel(void *obj);

int setsize(BABYX *bbx, int *width, int *height)
{
    SETSIZE ss;
    double hue;
    unsigned char s, v;
    
    ss.bbx = bbx;
    ss.pan = bbx_dialogpanel(bbx, "Set size", 400, 300, layoutsize, &ss);
    
    ss.width_lab = bbx_label(bbx, ss.pan, "Width");
    ss.height_lab = bbx_label(bbx, ss.pan, "Height");
    ss.width_spn = bbx_spinner(bbx, ss.pan, *width, 1, 4096, 1, 0, 0);
    ss.height_spn = bbx_spinner(bbx, ss.pan, *height, 1, 4096, 1, 0, 0);
    ss.ok_but = bbx_button(bbx, ss.pan, "Ok", pressok, &ss);
    ss.cancel_but = bbx_button(bbx, ss.pan, "Cancel", presscancel, &ss);
    
    ss.ok = 0;
    bbx_dialogpanel_setclosefunc(ss.pan, killsize, &ss);
    
    
    bbx_panel_setbackground(ss.pan, bbx_color("gray"));
    bbx_dialogpanel_makemodal(ss.pan);
    
    if (ss.ok)
    {
        *width = bbx_spinner_getvalue(ss.width_spn);
        *height = bbx_spinner_getvalue(ss.height_spn);
    }
    bbx_label_kill(ss.width_lab);
    bbx_spinner_kill(ss.width_spn);
    bbx_label_kill(ss.height_lab);
    bbx_spinner_kill(ss.height_spn);
    bbx_button_kill(ss.ok_but);
    bbx_button_kill(ss.cancel_but);
    bbx_dialogpanel_kill(ss.pan);
    
    return ss.ok ? 0 : -1;
}

void layoutsize(void *obj, int width, int height)
{
    SETSIZE *ss = obj;
    
    bbx_setpos(ss->bbx, ss->width_lab, 10, 10, 50, 25);
    bbx_setpos(ss->bbx, ss->width_spn, 65, 10, 50, 25);
    bbx_setpos(ss->bbx, ss->height_lab, 120, 10, 50, 25);
    bbx_setpos(ss->bbx, ss->height_spn, 175, 10, 50, 25);
    
    bbx_setpos(ss->bbx, ss->ok_but, 10, height - 50, 50, 25);
    bbx_setpos(ss->bbx, ss->cancel_but, 70, height - 50, 50, 25);
}

void killsize(void *ptr)
{
    
}

void pressok(void *obj)
{
    SETSIZE *ss = obj;
    ss->ok = 1;
    bbx_dialogpanel_dropmodal(ss->pan);
}

void presscancel(void *obj)
{
    SETSIZE *ss = obj;
    bbx_dialogpanel_dropmodal(ss->pan);
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

/**
  Floodfill4 - floodfill, 4 connectivity.

  @param[in,out] grey - the image (formally it's greyscale but it could be binary or indexed)
  @param width - image width
  @param height - image height
  @param x - seed point x
  @param y - seed point y
  @param target - the colour to flood
  @param dest - the colur to replace it by.
  @returns Number of pixels flooded.
*/
int floodfill4(unsigned char *grey, int width, int height, int x, int y, unsigned char target, unsigned char dest)
{
  int *qx = 0;
  int *qy = 0;
  int qN = 0;
  int qpos = 0;
  int qcapacity = 0;
  int wx, wy;
  int ex, ey;
  int tx, ty;
  int ix;
  int *temp;
  int answer = 0;

  if(grey[y * width + x] != target)
    return 0;
  qx = malloc(width * sizeof(int));
  qy = malloc(width * sizeof(int));
  if(qx == 0 || qy == 0)
    goto error_exit;
  qcapacity = width;
  qx[qpos] = x;
  qy[qpos] = y;
  qN = 1;

  while(qN != 0)
  {
    tx = qx[qpos];
    ty = qy[qpos];
    qpos++;
    qN--;
   
    if(qpos == 256)
    {
      memmove(qx, qx + 256, qN*sizeof(int));
      memmove(qy, qy + 256, qN*sizeof(int));
      qpos = 0;
    }
    if(grey[ty*width+tx] != target)
      continue;
    wx = tx;
    wy = ty;
    while(wx >= 0 && grey[wy*width+wx] == target)
      wx--;
    wx++;
    ex = tx;
    ey = ty;
    while(ex < width && grey[ey*width+ex] == target)
      ex++;
    ex--;
     

    for(ix=wx;ix<=ex;ix++)
    {
      grey[ty*width+ix] = dest;
      answer++;
    }

    if(ty > 0)
      for(ix=wx;ix<=ex;ix++)
      {
        if(grey[(ty-1)*width+ix] == target)
        {
          if(qpos + qN == qcapacity)
          {
            temp = realloc(qx, (qcapacity + width) * sizeof(int));
            if(temp == 0)
              goto error_exit;
            qx = temp;
            temp = realloc(qy, (qcapacity + width) * sizeof(int));
            if(temp == 0)
              goto error_exit;
            qy = temp;
            qcapacity += width;
          }
          qx[qpos+qN] = ix;
          qy[qpos+qN] = ty-1;
          qN++;
        }
      }
    if(ty < height -1)
      for(ix=wx;ix<=ex;ix++)
      {
        if(grey[(ty+1)*width+ix] == target)
        {
          if(qpos + qN == qcapacity)
          {
            temp = realloc(qx, (qcapacity + width) * sizeof(int));
            if(temp == 0)
              goto error_exit;
            qx = temp;
            temp = realloc(qy, (qcapacity + width) * sizeof(int));
            if(temp == 0)
              goto error_exit;
            qy = temp;
            qcapacity += width;
          }
          qx[qpos+qN] = ix;
          qy[qpos+qN] = ty+1;
          qN++;
        }
      }
  }

  free(qx);
  free(qy);

  return answer;
 error_exit:
  free(qx);
  free(qy);
  return -1;
}
