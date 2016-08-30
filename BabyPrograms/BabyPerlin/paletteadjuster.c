#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "BabyX.h"

#define PI 3.14159265359
#define lerp(a,b,t)  ( (a) + ( (b)- (a) ) * (t) )

typedef BBX_Panel PalAdjuster;

typedef struct undo
{
  struct undo *next;
  unsigned char *rgb;
  int N;
  int startpos;
  int endpos;
  int midpos;
} UNDO;

typedef struct
{
  BABYX *bbx;
  BBX_Panel *pan;
  BBX_Canvas *can;
  BBX_Button *undo_but;
  BBX_RadioBox *mode_rad;
  unsigned char *rgb;
  unsigned char *adjrgb;
  int N;
  void (*change)(void *ptr, unsigned char *rgb, int N);
  void *ptr;
  int grabbed;
  int startpos;
  int midpos;
  int endpos;
  int grabmidpos;
  int oldpos;
  UNDO *undo;
} PALETTEADJUSTER;

static void layout(void *obj, int width, int height);
static void redraw(PALETTEADJUSTER *pa);
static void mousefunc(void *obj, int action, int x, int y, int buttons);
static void undo(void *obj);
static void adjustpalette(PALETTEADJUSTER *pa);
static void rescale(unsigned char *rgb, int N, unsigned char *out, int Nout);
static void runhsv(unsigned char *rgb, int low, int high);
static void runrgb(unsigned char *rgb, int low, int high);
static int getgrab(PALETTEADJUSTER *pa, int x, int y);

static void pushundo(PALETTEADJUSTER *pa);
static void popundo(PALETTEADJUSTER *pa);
static void undolist_kill(UNDO *undo);
static void undo_kill(UNDO *undo);

static void drawcircle(unsigned char *rgba, int width, int height, int x0, int y0, int radius, unsigned long col);
static void drawpixel(unsigned char *rgba, int width, int height, int x, int y, unsigned long col);
static void rgba_clear(unsigned char *rgba, int width, int height, unsigned long col);

static void rgb2hsv(unsigned long rgb, double *h, unsigned char *s, unsigned char *v);
static unsigned long hsv2rgb(double h, int s, int v);

PalAdjuster *paladjuster(BABYX *bbx, BBX_Panel *pan, unsigned char *rgb, int N,
			 void (*change)(void *ptr, unsigned char *rgb, int N), void *ptr)
{
  PALETTEADJUSTER *pa;
  char *modes[3] = {"HSV", "int."};

  pa = bbx_malloc(sizeof(PALETTEADJUSTER));
  pa->bbx = bbx;
  pa->pan = bbx_panel(bbx, pan, "paladjuster", layout, pa);
  pa->can = bbx_canvas(bbx, pa->pan, 256+16, 40, BBX_Color("Gray"));
  pa->undo_but = bbx_button(bbx, pa->pan, "Undo", undo, pa);
  pa->mode_rad = bbx_radiobox(bbx, pa->pan, modes, 2, 0, 0); 
  pa->rgb = bbx_malloc(N * 3);
  pa->adjrgb = bbx_malloc(N * 3);
  memcpy(pa->rgb, rgb, N * 3);
  pa->N = N;
  pa->change = change;
  pa->ptr = ptr;
  pa->grabbed = 0;
  pa->startpos = 0;
  pa->endpos = N -1;
  pa->midpos = N/2;
  pa->undo = 0;
  bbx_canvas_setmousefunc(pa->can, mousefunc, pa);
  bbx_button_disable(pa->undo_but);
  bbx_radiobox_setselected(pa->mode_rad, 0);
  redraw(pa);
  return pa->pan;
}

void paladjuster_kill(PalAdjuster *padj)
{
  PALETTEADJUSTER *pa;

  if(padj)
  {
    pa = bbx_panel_getptr(padj);
    if(pa)
    {
      undolist_kill(pa->undo);
      free(pa->rgb);
      free(pa->adjrgb);
      bbx_radiobox_kill(pa->mode_rad);
      bbx_canvas_kill(pa->can);
      bbx_button_kill(pa->undo_but);
      free(pa);
    }
     bbx_panel_kill(padj);
  }
}

void paladjuster_set(PalAdjuster *padj, unsigned char *rgb, int N)
{
  PALETTEADJUSTER *pa;

  pa = bbx_panel_getptr(padj);
  pushundo(pa);
  if(N == pa->N)
    memcpy(pa->rgb, rgb, N * 3);
  else
  {
    free(pa->rgb);
    pa->rgb = bbx_malloc(N*3);
    free(pa->adjrgb);
    pa->adjrgb = bbx_malloc(N*3);
    memcpy(pa->rgb, rgb, N *3);
    pa->startpos = 0;
    pa->endpos = N -1;
    pa->midpos = N/2;
  }
  redraw(pa);
}

static void layout(void *obj, int width, int height)
{
  PALETTEADJUSTER *pa = obj;

  bbx_setpos(pa->bbx, pa->can, 0, 0, 256+16, 40);
  bbx_setpos(pa->bbx, pa->mode_rad, 256+16 + 5, 0, 60, 40);
  bbx_setpos(pa->bbx, pa->undo_but, 256+16 + 5+65, 10, 60, 25);
}

static void mousefunc(void *obj, int action, int x, int y, int buttons)
{
  PALETTEADJUSTER *pa = obj;
  int oldpos;
  int xpos;

  if( action == BBX_MOUSE_CLICK && (buttons & BBX_MOUSE_BUTTON1) )
  {
    pa->grabbed = getgrab(pa, x, y);
    pa->grabmidpos = x - 8;
    memcpy(pa->adjrgb, pa->rgb, pa->N * 3);
    if(pa->grabbed == 4)
      pushundo(pa);
  }
  else if(action == BBX_MOUSE_RELEASE && (buttons & BBX_MOUSE_BUTTON1))
  {
    pa->grabbed = 0;
  }
  else if(pa->grabbed != 0 && action == BBX_MOUSE_MOVE && (buttons & BBX_MOUSE_BUTTON1))
  {
    xpos = x - 8;
    if(xpos < 0 || xpos >= pa->N)
      return;
    switch(pa->grabbed)
    {
    case 1:
      pa->midpos = xpos;
      if(pa->endpos < xpos)
        pa->endpos = xpos;
      if(pa->startpos > xpos)
        pa->startpos = xpos;
      break;
    case 2:
      pa->startpos = xpos;
      if(pa->midpos < xpos)
        pa->midpos = xpos;
      if(pa->endpos < xpos)
        pa->endpos = xpos;
      break;
    case 3:
      pa->endpos = xpos;
      if(pa->midpos > xpos)
        pa->midpos = xpos;
      if(pa->startpos > xpos)
        pa->startpos = xpos;
      break;
    case 4:
      oldpos = pa->midpos;
      pa->oldpos = pa->midpos;
      pa->midpos  = xpos;
      if(pa->endpos < xpos)
        pa->endpos = xpos;
      if(pa->startpos > xpos)
        pa->startpos = xpos;
      if(oldpos != pa->midpos)
	adjustpalette(pa);
      break;
    }
    redraw(pa);
  }
  
}

static void undo(void *obj)
{
  PALETTEADJUSTER *pa = obj;

  popundo(pa);
}

static void adjustpalette(PALETTEADJUSTER *pa)
{
  unsigned char r, g, b;
  int mode;
  unsigned char *tpal;

  mode = bbx_radiobox_getselected(pa->mode_rad);
  if(mode == 1)
  {
    tpal = bbx_malloc(pa->N * 3);
    memcpy(tpal, pa->adjrgb, pa->N * 3);
    rescale(pa->adjrgb + pa->startpos * 3, 
            pa->grabmidpos - pa->startpos+1, 
            tpal + pa->startpos * 3, 
	    pa->midpos - pa->startpos + 1);
    rescale(pa->adjrgb + pa->grabmidpos * 3, 
            pa->endpos - pa->grabmidpos+1, 
            tpal + pa->midpos * 3, 
            pa->endpos - pa->midpos + 1);
    memcpy(pa->rgb, tpal, pa->N * 3);
    free(tpal);
  }
  else
  {
    r = pa->rgb[pa->oldpos*3];
    g = pa->rgb[pa->oldpos*3+1];
    b = pa->rgb[pa->oldpos*3+2];
    pa->rgb[pa->midpos*3] = r;
    pa->rgb[pa->midpos*3+1] = g;
    pa->rgb[pa->midpos*3+2] = b;
    if(pa->midpos > pa->startpos)
    {
      if(mode == 0)
        runhsv(pa->rgb, pa->startpos, pa->midpos);
      else
        runrgb(pa->rgb, pa->startpos, pa->midpos);
    }
    if(pa->midpos < pa->endpos)
    {
      if(mode == 0)
        runhsv(pa->rgb, pa->midpos, pa->endpos);
      else
        runrgb(pa->rgb, pa->midpos, pa->endpos);
    }
  }
  if(pa->change)
      (*pa->change)(pa->ptr, pa->rgb, pa->N); 
}

static void rescale(unsigned char *rgb, int N, unsigned char *out, int Nout)
{
  int i;
  int j, jj;
  float t;

  for(i=0;i<Nout;i++)
  {
    t = (i/(float) Nout)*N;
    j = (int) t;
    jj = j +1;
    if(jj >= N)
      jj = j;
    t -= j;
    out[i*3] = lerp(rgb[j*3], rgb[jj*3], t);
    out[i*3+1] = lerp(rgb[j*3+1], rgb[jj*3+1], t);
    out[i*3+2] = lerp(rgb[j*3+2], rgb[jj*3+2], t);
  }
}

static void runhsv(unsigned char *rgb, int low, int high)
{
  int i;
  float t;
  unsigned long col;
  double hue;
  int s, v;
  double hue1, hue2;
  unsigned char s1, s2, v1, v2;

  col = ((int)rgb[low*3] << 16) | ((int)rgb[low*3+1] << 8) | (int) rgb[low*3+2];
  rgb2hsv(col, &hue1, &s1, &v1);
  col = ((int) rgb[high*3] << 16) | ((int)rgb[high*3+1] << 8) | (int) rgb[high*3+2];
  rgb2hsv(col, &hue2, &s2, &v2);

  if(fabs(hue1 - hue2) > PI)
  {
    if(hue1 < hue2)
      hue1 += 2 * PI;
    else
      hue2 += 2 * PI;
  }

  for(i=low+1;i<=high-1;i++)
  {
    t = (i-low)/((float) high-low);
    hue = lerp(hue1, hue2, t);
    hue = fmod(hue, 2 * PI);
    s = (int) lerp(s1, s2, t);
    v = (int) lerp(v1, v2, t);
    col = hsv2rgb(hue, s, v);
    rgb[i*3] = (col >> 16) & 0xFF;
    rgb[i*3+1] = (col >> 8) & 0xFF;
    rgb[i*3+2] = col & 0xFF;
  }
}

static void runrgb(unsigned char *rgb, int low, int high)
{
  int i;
  float t;
  int red, green, blue;

  for(i=low+1;i<=high-1;i++)
  {
    t = (i-low)/((float) high-low);
    red = (int) lerp(rgb[low*3], rgb[high*3], t);
    green = (int) lerp(rgb[low*3+1], rgb[high*3+1], t);
    blue = (int) lerp(rgb[low*3+2], rgb[high*3+2], t);
    rgb[i*3] = red;
    rgb[i*3+1] = green;
    rgb[i*3+2] = blue;
  }
}

static void redraw(PALETTEADJUSTER *pa)
{
  int width, height;
  unsigned char *rgba;
  int i, ii;

  rgba = bbx_canvas_rgba(pa->can, &width, &height);
  rgba_clear(rgba, width, height, BBX_Color("Gray"));
  for(i=0;i<pa->N;i++)
  {
    for(ii=0;ii<20;ii++)
    {
      rgba[((ii+10)*width+i+8)*4] = pa->rgb[i*3];
      rgba[((ii+10)*width+i+8)*4+1] = pa->rgb[i*3+1];
      rgba[((ii+10)*width+i+8)*4+2] = pa->rgb[i*3+2];
    }
  } 
  drawcircle(rgba, width, height, pa->midpos+8, 5, 2, 0xFF0000);
  for(i=8;i<10;i++)
    drawpixel(rgba, width, height, pa->midpos +8, i, 0xFF0000);

  drawcircle(rgba, width, height, pa->midpos+8, 35, 2, 0);
  for(i=30;i<33;i++)
    drawpixel(rgba, width, height, pa->midpos +8, i, 0x000000);
 
  drawcircle(rgba, width, height, pa->startpos+8, 35, 2, 0);
  for(i=30;i<33;i++)
    drawpixel(rgba, width, height, pa->startpos +8, i, 0x000000);

  drawcircle(rgba, width, height, pa->endpos+8, 35, 2, 0);
  for(i=30;i<33;i++)
    drawpixel(rgba, width, height, pa->endpos +8, i, 0x000000);

 
  bbx_canvas_flush(pa->can);
}

static int getgrab(PALETTEADJUSTER *pa, int x, int y)
{
  x -= 8;
  if( abs(y-35) <= 2 && abs(x-pa->midpos) <= 2)
    return 1;
  if(abs(y-35) <= 2 && abs(x-pa->startpos) <= 2)
    return 2;
  if(abs(y-35) <= 2 && abs(x-pa->endpos) <= 2)
    return 3;
  if(abs(y-5) <= 2 && abs(x-pa->midpos) <= 2)
    return 4;
  return 0;
}

static void pushundo(PALETTEADJUSTER *pa)
{
  UNDO *undo;
  int Nundos = 0;
  int i;
  UNDO *ptr;

  undo = bbx_malloc(sizeof(UNDO));
  undo->rgb = bbx_malloc(pa->N * 3);
  memcpy(undo->rgb, pa->rgb, pa->N * 3);
  undo->N = pa->N;
  undo->startpos = pa->startpos;
  undo->endpos = pa->endpos;
  undo->midpos = pa->midpos;
  undo->next = pa->undo;
  pa->undo = undo;
  for(ptr = pa->undo;ptr; ptr=ptr->next)
    Nundos++;
  if(Nundos > 10)
  {
    for(i=0, ptr = pa->undo; i < 10;i++,ptr=ptr->next)
      continue;
    undolist_kill(ptr->next);
    ptr->next = 0;
  }
  bbx_button_enable(pa->undo_but);
}

static void popundo(PALETTEADJUSTER *pa)
{
  UNDO *ptr;
  if(!pa->undo)
    return;
  if(pa->N != pa->undo->N)
    return;
  memcpy(pa->rgb, pa->undo->rgb, pa->N * 3);
  pa->N = pa->undo->N;
  pa->startpos = pa->undo->startpos;
  pa->endpos = pa->undo->endpos;
  pa->midpos = pa->undo->midpos;
  ptr = pa->undo->next;
  undo_kill(pa->undo);
  pa->undo = ptr;
  if(!pa->undo)
    bbx_button_disable(pa->undo_but);
 
  redraw(pa);
  if(pa->change)
    (*pa->change)(pa->ptr, pa->rgb, pa->N);

}

static void undolist_kill(UNDO *undo)
{
  UNDO *ptr;
  while(undo)
  {
    ptr = undo->next;
    undo_kill(undo);
    undo = ptr;
  }
}

static void undo_kill(UNDO *undo)
{
  if(undo)
  {
    free(undo->rgb);
    free(undo);
  }
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

static void rgba_clear(unsigned char *rgba, int width, int height, unsigned long col)
{
  int i;

  for(i=0;i<width*height;i++)
  {
    rgba[i*4] = (col >> 16) & 0xFF;
    rgba[i*4+1] = (col >> 8) & 0xFF;
    rgba[i*4+2] = col & 0xFF;
    rgba[i*4+3] = 0xFF;
  }
}


#define max2(a,b) ( (a) > (b) ? (a) : (b) )
#define min2(a,b) ( (a) < (b) ? (a) : (b) ) 
#define max3(a,b,c) max2(max2((a),(b)),(c))
#define min3(a,b,c) min2(min2((a),(b)),(c))
  
static void rgb2hsv(unsigned long rgb, double *h, unsigned char *s, unsigned char *v)
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

static unsigned long hsv2rgb(double h, int s, int v)
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

