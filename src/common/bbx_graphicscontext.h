#ifndef bbx_graphics_context_h
#define bbx_graphics_context_h

#ifndef BBX_RGBA
typedef unsigned long BBX_RGBA;
#endif
#ifndef bbx_rgba
#define bbx_rgba(r,g,b,a) ((BBX_RGBA)( (r << 24) | (g << 16) | (b << 8) | (a) ))
#endif

typedef struct bbx_gradient BBX_GRADIENT;
BBX_GRADIENT *bbx_createlineargradient(double x1, double y1, double x2, double y2);
BBX_GRADIENT *bbx_createradialgradient(double x1, double y1, double r1, double x2, double y2, double r2);
BBX_GRADIENT *bbx_gradient_clone(BBX_GRADIENT *grad);
void bbx_gradient_kill(BBX_GRADIENT *g);
int bbx_gradient_addcolorstop(BBX_GRADIENT *g, double offset, BBX_RGBA col);

typedef struct bbx_pattern BBX_PATTERN;
BBX_PATTERN *bbx_createpattern(unsigned char *rgba, int width, int height, int filltype);
BBX_PATTERN *bbx_pattern_clone(BBX_PATTERN *pat);
void bbx_pattern_kill(BBX_PATTERN *p);

typedef struct bbx_gc BBX_GC;

BBX_GC *bbx_graphicscontext(unsigned char *rgba, int width, int height);
void bbx_graphicscontext_kill(BBX_GC *gc);

void bbx_gc_setstrokecolor(BBX_GC *gc, BBX_RGBA col);
void bbx_gc_setstrokewidth(BBX_GC *gc, double width);
void bbx_gc_setfillcolor(BBX_GC *gc, BBX_RGBA col);
void bbx_gc_setfillgradient(BBX_GC *gc, BBX_GRADIENT *g);
void bbx_gc_setfillpattern(BBX_GC *gc, BBX_PATTERN *p);
void bbx_gc_setstrokegradient(BBX_GC *gc, BBX_GRADIENT *g);
void bbx_gc_setstrokepattern(BBX_GC *gc, BBX_PATTERN *p);
void bbx_gc_setlinejoin(BBX_GC *gc, const char *jointype);
void bbx_gc_setlinecap(BBX_GC *gc, const char *capstyle);

void bbx_gc_rotate(BBX_GC *gc, double theta);
void bbx_gc_translate(BBX_GC *gc, double x, double y);
void bbx_gc_scale(BBX_GC *gc, double s);
void bbx_gc_scalexy(BBX_GC *gc, double sx, double sy);
void bbx_gc_transform(BBX_GC *gc, double a, double b, double c, double d, double e, double f);
void bbx_gc_settransfrom(BBX_GC *gc, double a, double b, double c, double d, double e, double f);

void bbx_gc_addcubic(BBX_GC *gc, double x1, double y1, double x2, double y2, double x3, double y3);
void bbx_gc_addquadratic(BBX_GC *gc, double x1, double y1, double x2, double y2);
void bbx_gc_lineto(BBX_GC *gc, double x, double y);
void bbx_gc_beginpath(BBX_GC *gc);
void bbx_gc_moveto(BBX_GC *gc, double x, double y);
void bbx_gc_closepath(BBX_GC *gc);

void bbx_gc_arc(BBX_GC *gc, double cx, double cy, double r, double stheta, double etheta, int ccw);
void bbx_gc_arcto(BBX_GC *gc, double tx1, double ty1, double x2, double y2, double r);

int bbx_gc_pointinpath(BBX_GC *gc, double x, double y);

void bbx_gc_fill(BBX_GC *gc);
void bbx_gc_stroke(BBX_GC *gc);
int bbx_gc_clip(BBX_GC *gc);

void bbx_gc_rect(BBX_GC *gc, double x, double y, double width, double height);
void bbx_gc_fillrect(BBX_GC *gc, double x, double y, double width, double height);
void bbx_gc_strokerect(BBX_GC *gc, double x, double y, double width, double height);
int bbx_gc_clearrect(BBX_GC *gc, double x, double y, double width, double height);
void bbx_gc_circle(BBX_GC *gc, double cx, double cy, double r);
void bbx_gc_fillcircle(BBX_GC *gc, double cx, double cy, double r);

void bbx_gc_drawimage(BBX_GC *gc, unsigned char *rgba, int width, int height, int x, int y);
void bbx_gc_drawimagex(BBX_GC *gc, unsigned char *rgba, int width, int height,
	int sx, int sy, int swidth, int sheight, int dx, int dy, int dwidth, int dheight);

int bbx_gc_save(BBX_GC *gc);
int bbx_gc_restore(BBX_GC *gc);

#endif
