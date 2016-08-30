#include <math.h>
#include "BabyX.h"

#define PI 3.14159265359
#define clamp(x, low, high) ((x) < (low) ? (low) : (x) > (high) ? (high) : (x))

typedef struct
{
  BABYX *bbx;
  BBX_DialogPanel *pan;
  BBX_Canvas *can;
  BBX_Canvas *hue_can;
  BBX_Canvas *satval_can;
  BBX_Button *ok_but;
  BBX_Button *cancel_but;
  BBX_Label *r_lab;
  BBX_Label *g_lab;
  BBX_Label *b_lab;
  BBX_Spinner *red_spn;
  BBX_Spinner *green_spn;
  BBX_Spinner *blue_spn;  
  int hue;
  int sat;
  int val;

} COLORPICKER;

static void kill(void *obj);
static void layout(void *obj, int width, int height);
static void updatespinners(COLORPICKER *cp);
static void redraw(COLORPICKER *cp);
static void drawswatch(COLORPICKER *cp);
static void drawhue(COLORPICKER *cp);
static void drawsatval(COLORPICKER *cp);
static void huemouse(void *obj, int action, int x, int y, int buttons);
static void satvalmouse(void *obj, int action, int x, int y, int buttons);
static void rgbspinnerspun(void *obj, double value);
static void pressok(void *obj);
static void presscancel(void *obj);

static void drawcircle(unsigned char *rgba, int width, int height, int x0, int y0, int radius, unsigned long col);
static void drawpixel(unsigned char *rgba, int width, int height, int x, int y, unsigned long col);

void rgb2hsv(unsigned long rgb, double *h, unsigned char *s, unsigned char *v);
unsigned long hsv2rgb(double h, int s, int v);

unsigned long pickcolor(BABYX *bbx, unsigned long rgb)
{
  COLORPICKER cp;
  double hue;
  unsigned char s, v;

  cp.bbx = bbx;
  cp.pan = bbx_dialogpanel(bbx, "Color Picker", 400, 300, layout, &cp);
  cp.can = bbx_canvas(bbx, cp.pan, 50, 50, rgb);
  cp.hue_can = bbx_canvas(bbx, cp.pan, 40, 256, rgb);
  cp.satval_can = bbx_canvas(bbx, cp.pan, 256, 256, rgb);
  cp.ok_but = bbx_button(bbx, cp.pan, "Ok", pressok, &cp);
  cp.cancel_but = bbx_button(bbx, cp.pan, "Cancel", presscancel, &cp);

  cp.r_lab = bbx_label(bbx, cp.pan, "R");
  cp.g_lab = bbx_label(bbx, cp.pan, "G");
  cp.b_lab = bbx_label(bbx, cp.pan, "B");
  cp.red_spn = bbx_spinner(bbx, cp.pan, (rgb >> 16) & 0xFF, 0, 255, 1, rgbspinnerspun, &cp);
  cp.green_spn = bbx_spinner(bbx, cp.pan, (rgb >> 8) & 0xFF, 0, 255, 1, rgbspinnerspun, &cp);
  cp.blue_spn = bbx_spinner(bbx, cp.pan, (rgb >> 0) & 0xFF, 0, 255, 1, rgbspinnerspun, &cp);

  bbx_canvas_setmousefunc(cp.hue_can, huemouse, &cp);  
  bbx_canvas_setmousefunc(cp.satval_can, satvalmouse, &cp);

  bbx_dialogpanel_setclosefunc(cp.pan, kill, &cp);

  bbx_spinner_setmode(cp.red_spn, BBX_SPINNER_INTERACTIVE);
  bbx_spinner_setmode(cp.green_spn, BBX_SPINNER_INTERACTIVE);
  bbx_spinner_setmode(cp.blue_spn, BBX_SPINNER_INTERACTIVE);

  bbx_panel_setbackground(cp.pan, bbx_color("gray"));

  rgb2hsv(rgb, &hue, &s, &v);
  cp.hue = (int) (hue/(2*PI) * 255.99);
  cp.sat = s;
  cp.val = v;

  redraw(&cp);
  bbx_dialogpanel_makemodal(cp.pan);
  
  bbx_label_kill(cp.r_lab);
  bbx_label_kill(cp.g_lab);
  bbx_label_kill(cp.b_lab);
  bbx_spinner_kill(cp.red_spn);
  bbx_spinner_kill(cp.green_spn);
  bbx_spinner_kill(cp.blue_spn);
  bbx_canvas_kill(cp.satval_can);
  bbx_canvas_kill(cp.hue_can);
  bbx_canvas_kill(cp.can);
  bbx_button_kill(cp.ok_but);
  bbx_button_kill(cp.cancel_but);
  bbx_dialogpanel_kill(cp.pan);

  return hsv2rgb(cp.hue/256.0 * 2*PI, cp.sat, cp.val);  
}

static void kill(void *obj)
{

}

static void layout(void *obj, int width, int height)
{
  COLORPICKER *cp = obj;

  bbx_setpos(cp->bbx, cp->can, 320, 20, 50, 50);
  bbx_setpos(cp->bbx, cp->hue_can, 260, 0, 40, 256);
  bbx_setpos(cp->bbx, cp->satval_can, 2, 2, 256, 256);

  bbx_setpos(cp->bbx, cp->r_lab, width-90, 80, 20, 20);
  bbx_setpos(cp->bbx, cp->g_lab, width-90, 110, 20, 20);
  bbx_setpos(cp->bbx, cp->b_lab, width-90, 140, 20, 20);
  bbx_setpos(cp->bbx, cp->red_spn, width-70, 80, 60, 20); 
  bbx_setpos(cp->bbx, cp->green_spn, width-70, 110, 60, 20); 
  bbx_setpos(cp->bbx, cp->blue_spn, width-70, 140, 60, 20); 
  bbx_setpos(cp->bbx, cp->ok_but, 10, height-30, 60, 25);
  bbx_setpos(cp->bbx, cp->cancel_but, 100, height-30, 60, 25);
}

static void updatespinners(COLORPICKER *cp)
{
  unsigned long rgb;

  rgb = hsv2rgb(cp->hue/256.0*2*PI, cp->sat, cp->val);
  bbx_spinner_setvalue(cp->red_spn, (rgb >> 16) & 0xFF);
  bbx_spinner_setvalue(cp->green_spn, (rgb >> 8) & 0xFF);
  bbx_spinner_setvalue(cp->blue_spn, rgb & 0xFF);
}

static void redraw(COLORPICKER *cp)
{
  drawhue(cp);
  drawsatval(cp);
  drawswatch(cp);
}

static void drawswatch(COLORPICKER *cp)
{
  unsigned char *rgba;
  int width, height;
  unsigned long col;
  int i;
  BBX_RGBA higrey;
  BBX_RGBA dimgrey;

  dimgrey = bbx_color("dim gray");
  higrey = bbx_color("light gray");
  col = hsv2rgb(cp->hue/256.0 *2 * PI, cp->sat, cp->val);
  rgba = bbx_canvas_rgba(cp->can, &width, &height);
  for(i=0;i<width*height;i++)
  {
    rgba[i*4] = (col >> 16) & 0xFF;
    rgba[i*4+1] = (col >> 8) & 0xFF;
    rgba[i*4+2] = col & 0xFF;
  }
  bbx_line(rgba, width, height, 0, 0, width-1, 0, dimgrey);
  bbx_line(rgba, width, height, 0, 0, 0, height-1, dimgrey);
  bbx_line(rgba, width, height, width-1, 0, width-1, height-1, higrey);
  bbx_line(rgba, width, height, 0, height-1, width-1, height-1, higrey);

  bbx_canvas_flush(cp->can);
}

static void drawhue(COLORPICKER *cp)
{
  unsigned char *rgba;
  int width, height;
  int i, x;
  unsigned long col;

  rgba = bbx_canvas_rgba(cp->hue_can, &width, &height);
  col = BBX_Color("LightGray");
  for(i=0;i<width*height;i++)
  {
    rgba[i*4] = (col >> 16) & 0xFF;
    rgba[i*4+1] = (col >> 8) & 0xFF;
    rgba[i*4+2] = col & 0xFF;
  }

  for(i=0;i<256;i++)
  {
    col  = hsv2rgb((i*2*PI)/256.0, 255, 255);
    for(x=10;x<30;x++)
    {
      rgba[(i*width+x)*4] = (col >> 16) & 0xFF;
      rgba[(i*width+x)*4+1] = (col >> 8) & 0xFF;
      rgba[(i*width+x)*4+2] = col & 0xFF;
    }
  }
  
  i = cp->hue;
  for(x=0;x<10;x++)
  {
    rgba[(i*width+x)*4] =  0;
    rgba[(i*width+x)*4+1] = 0;
    rgba[(i*width+x)*4+2] = 0;
  }
  for(x=30;x<width;x++)
  {
    rgba[(i*width+x)*4] =  0;
    rgba[(i*width+x)*4+1] = 0;
    rgba[(i*width+x)*4+2] = 0;
  }
 
  
  bbx_canvas_flush(cp->hue_can);
}

static void drawsatval(COLORPICKER *cp)
{
  unsigned char *rgba;
  int width, height;
  int s, v;
  double h;
  int x, y;
  unsigned long col;

  rgba = bbx_canvas_rgba(cp->satval_can, &width, &height);
  for(y=0;y<height;y++)
    for(x=0;x<width;x++)
    {
      s = (x * width)/256;
      v = (y * height)/256;
      h = cp->hue/256.0 *2 * PI;
      col = hsv2rgb(h, s, v);
      rgba[(y*width+x)*4] = (col >> 16) & 0xFF;
      rgba[(y*width+x)*4+1] = (col >> 8) & 0xFF;
      rgba[(y*width+x)*4+2] = col & 0xFF;
    }
  drawcircle(rgba, width, height, cp->sat, cp->val, 5, 0xFFFFFF);
  drawcircle(rgba, width, height, cp->sat, cp->val, 4, 0x000000);
  bbx_canvas_flush(cp->satval_can);
}

static void huemouse(void *obj, int action, int x, int y, int buttons)
{
  COLORPICKER *cp = obj;
  if( (action == BBX_MOUSE_MOVE || action == BBX_MOUSE_CLICK) && 
      (buttons & BBX_MOUSE_BUTTON1))
  {
    cp->hue = clamp(y, 0, 255);
    redraw(cp);
  }
  if( (action == BBX_MOUSE_RELEASE) && (buttons & BBX_MOUSE_BUTTON1))
  {
    cp->hue = clamp(y, 0, 255);
    redraw(cp);
    updatespinners(cp);
  }
 
}


static void satvalmouse(void *obj, int action, int x, int y, int buttons)
{
  COLORPICKER *cp = obj;
  if( (action == BBX_MOUSE_MOVE || action == BBX_MOUSE_CLICK) && 
     (buttons & BBX_MOUSE_BUTTON1))
  {
    cp->sat = clamp(x, 0, 255);
    cp->val = clamp(y, 0, 255);
    redraw(cp);
  }
  if( (action == BBX_MOUSE_RELEASE) && (buttons & BBX_MOUSE_BUTTON1))
  {
    cp->sat = clamp(x, 0, 255);
    cp->val = clamp(y, 0, 255);
    redraw(cp);
    updatespinners(cp);
  }
}

static void rgbspinnerspun(void *obj, double value)
{
   COLORPICKER *cp = obj;
   unsigned long rgb;
   int red, green, blue;
   double h;
   unsigned char s, v;

   red = (int) bbx_spinner_getvalue(cp->red_spn);
   green = (int) bbx_spinner_getvalue(cp->green_spn);
   blue = (int) bbx_spinner_getvalue(cp->blue_spn);
   rgb = (red << 16) | (green << 8) | blue;

   rgb2hsv(rgb, &h, &s, &v);
   cp->hue = (int) ((h/(2*PI)) * 255);
   cp->sat = s;
   cp->val = v;
     redraw(cp);
}


static void pressok(void *obj)
{
  COLORPICKER *cp = obj;
  bbx_dialogpanel_dropmodal(cp->pan); 
}

static void presscancel(void *obj)
{
  COLORPICKER *cp = obj;
  bbx_dialogpanel_dropmodal(cp->pan);
}


static void drawcircle(unsigned char *rgba, int width, int height, int x0, int y0, int radius, unsigned long col)
{
 
  int x = radius, y = 0;
  int radiusError = 1-x;
 
  while(x >= y)
  {
    drawpixel(rgba, width, height, x + x0, y + y0, col);
    drawpixel(rgba, width, height, y + x0, x + y0, col);
    drawpixel(rgba, width, height, -x + x0, y + y0, col);
    drawpixel(rgba, width, height, -y + x0, x + y0, col);
    drawpixel(rgba, width, height, -x + x0, -y + y0, col);
    drawpixel(rgba, width, height, -y + x0, -x + y0, col);
    drawpixel(rgba, width, height, x + x0, -y + y0, col);
    drawpixel(rgba, width, height, y + x0, -x + y0, col);
 
    y++;
        if(radiusError<0)
                radiusError+=2*y+1;
        else
        {
                x--;
                radiusError+=2*(y-x+1);
        }
  }
}

static void drawpixel(unsigned char *rgba, int width, int height, int x, int y, unsigned long col)
{
  int index;

  if(x >= 0 && x < width && y >= 0 && y < height)
  {
    index = (y * width + x)*4;
    rgba[index] = (col >> 16) & 0xFF;
    rgba[index+1] = (col >> 8) & 0xFF;
    rgba[index+2] = col & 0xFF;
  }
}

#define max2(a,b) ( (a) > (b) ? (a) : (b) )
#define min2(a,b) ( (a) < (b) ? (a) : (b) ) 
#define max3(a,b,c) max2(max2((a),(b)),(c))
#define min3(a,b,c) min2(min2((a),(b)),(c))
  
void rgb2hsv(unsigned long rgb, double *h, unsigned char *s, unsigned char *v)
{
  int r, g, b;
  int V;
  int S = 0;
  double hue;

  r = (rgb >> 16) & 0xFF;
  g = (rgb >> 8) & 0xFF;
  b = rgb & 0xFF;
   
  V = max3(r, g, b);
  if(V > 0)
    S = ((V - min3(r, g, b)) * 255)/ V;
  if(V == r)
    hue = (g - b)/(6.0)/S;
  else if(V == g)
    hue = 1.0/3.0 + (b - r)/(6.0)/S; 
  else if(V == b)
    hue = 2.0/3.0 +  (r - g)/(6.0)/S;

  *s = S;
  *v = V;
  if(hue < 0.0)
    hue += 1.0;
  if(hue >= 1.0)
    hue -= 1.0;
    
  *h = hue * 2 * PI;
}

unsigned long hsv2rgb(double h, int s, int v)
{
  double h1 = h/(2 * PI) * 6.0;
  int x;
  int c = (v * s)/255;
  int r, g, b;

  x = (int) (c *(1.0 - fabs( fmod(h1, 2) - 1 )));
  if(h1 < 1)
  {
    r = c;
    g = x;
    b = 0;
  }
  else if(h1 < 2)
  {
    r = x;
    g = c;
    b = 0;
  }
  else if(h1 < 3)
  {
    r = 0;
    g = c;
    b = x;
  }
  else if(h1 < 4)
  {
    r = 0;
    g = x;
    b = c;
  }
  else if(h1 < 5)
  {
    r = x;
    g = 0;
    b = c;
  }
  else 
  {
    r = c;
    g = 0;
    b = x;
  }
  r += v - c;
  g += v - c;
  b += v - c;

  return (r << 16) | (g << 8) | b;
 
}
