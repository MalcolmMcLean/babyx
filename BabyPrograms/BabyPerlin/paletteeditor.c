#include <math.h>
#include <stdlib.h>

#include "BabyX.h"
#include "colorpicker.h"

#include "paletteeditor.h"

#define PI 3.14159265359

#define lerp(a,b,t)  ( (a) + ( (b)- (a) ) * (t) )

typedef struct
{
  BABYX *bbx;
  BBX_Panel *pan;
  BBX_Canvas *can;
  BBX_Button *rangehsv_but;
  BBX_Button *rangergb_but;
  BBX_Button *ok_but;
  PALETTE *pal;
  int sel1;
  int sel2;
  int onsel;
} PALETTE_EDITOR;

static void kill(void *obj);
static void layout(void *obj, int width, int height);
static void mousefunc(void *obj, int action, int x, int y, int buttons);
static void pressok(void *obj);
static void pressrunhsv(void *obj);
static void pressrunrgb(void *obj);
static void drawpalette(PALETTE_EDITOR *ped);

void openpaletteeditor(BABYX *bbx, PALETTE *pal)
{
  PALETTE_EDITOR ped;
  ped.pal = pal;

  ped.bbx = bbx;
  ped.pan = bbx_dialogpanel(bbx, "Palette Editor", 300, 300, layout, &ped);
  ped.can = bbx_canvas(bbx, ped.pan, 16*16, 16*16, BBX_Color("LightGray"));  
  ped.ok_but = bbx_button(bbx, ped.pan, "Ok", pressok, &ped);
  ped.rangehsv_but = bbx_button(bbx, ped.pan, "Run HSV", pressrunhsv, &ped);
  ped.rangergb_but = bbx_button(bbx, ped.pan, "Run RGB", pressrunrgb, &ped); 

  bbx_canvas_setmousefunc(ped.can, mousefunc, &ped);
  bbx_dialogpanel_setclosefunc(ped.pan, kill, &ped);

  ped.sel1 = 230; 
  ped.sel2 = 18;
  ped.onsel = 1;
  
  drawpalette(&ped);
  bbx_dialogpanel_makemodal(ped.pan);

  bbx_canvas_kill(ped.can);
  bbx_button_kill(ped.ok_but);
  bbx_button_kill(ped.rangehsv_but);
  bbx_button_kill(ped.rangergb_but);
  bbx_dialogpanel_kill(ped.pan);

}

static void kill(void *obj)
{
	PALETTE_EDITOR *ped = obj;
}
static void layout(void *obj, int width, int height)
{
  PALETTE_EDITOR *ped = obj;
  bbx_setpos(ped->bbx, ped->can, 10, 10, 256, 256);
  bbx_setpos(ped->bbx, ped->ok_but, 10, height-30, 50, 25);
  bbx_setpos(ped->bbx, ped->rangehsv_but, 70, height-30, 100, 25);
  bbx_setpos(ped->bbx, ped->rangergb_but, 180, height-30, 100, 25);
}

static void mousefunc(void *obj, int action, int x, int y, int buttons)
{
  PALETTE_EDITOR *ped = obj;
  int index;
  unsigned long oldcol;
  unsigned long newcol;
  
  index = y/16 * 16 + x/16;

  if(index >= 0 && index < ped->pal->N)
  {
    if(action == BBX_MOUSE_CLICK && (buttons & BBX_MOUSE_BUTTON1))
    {
      if(index == ped->sel1)
        ped->onsel = 1;
      else if(index == ped->sel2)
        ped->onsel = 2;
      else
      {
        if(ped->onsel == 1)
          ped->sel1 = index;
        else
	  ped->sel2 = index;
        ped->onsel = ped->onsel == 1 ? 2 : 1; 
        drawpalette(ped);
      }
    }
    else if(action == BBX_MOUSE_CLICK && (buttons & BBX_MOUSE_BUTTON2))
    { 
      oldcol = (ped->pal->rgb[index*3] << 16) |
             (ped->pal->rgb[index*3+1] << 8) |
             ped->pal->rgb[index*3+2];  
      newcol = pickcolor(ped->bbx, oldcol);
      ped->pal->rgb[index*3] = (newcol >> 16) & 0xFF;
      ped->pal->rgb[index*3+1] = (newcol >> 8) & 0xFF;
      ped->pal->rgb[index*3+2] = newcol & 0xFF;
      drawpalette(ped);
    }
  } 

}

static void drawpalette(PALETTE_EDITOR *ped)
{
  unsigned char *rgba;
  int width, height;
  int x, y;
  int ix, iy;
  int i;
  unsigned char *pal = ped->pal->rgb;
  int N = ped->pal->N;
  unsigned long col;
  unsigned long dimgrey;
  unsigned long higrey;

  rgba = bbx_canvas_rgba(ped->can, &width, &height);
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
    if(y*16+x == ped->sel1 || y*16+x == ped->sel2)
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
  bbx_canvas_flush(ped->can);
}

static void pressok(void *obj)
{
  PALETTE_EDITOR *ped = obj;

  bbx_dialogpanel_dropmodal(ped->pan);
}


static void pressrunhsv(void *obj)
{
  PALETTE_EDITOR *ped = obj;
  int low, high;
  int i;
  float t;
  unsigned long col;
  double hue;
  int s, v;
  double hue1, hue2;
  unsigned char s1, s2, v1, v2;

  if(ped->sel1 < ped->sel2)
  {
    low = ped->sel1;
    high = ped->sel2;
  }
  else
  {
    low = ped->sel2;
    high = ped->sel1;
  }
  col = ((int)ped->pal->rgb[low*3] << 16) | ((int) ped->pal->rgb[low*3+1] << 8) | (int) ped->pal->rgb[low*3+2];
  rgb2hsv(col, &hue1, &s1, &v1);

  col = ((int) ped->pal->rgb[high*3] << 16) | ((int) ped->pal->rgb[high*3+1] << 8) | (int) ped->pal->rgb[high*3+2];
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
    ped->pal->rgb[i*3] = (col >> 16) & 0xFF;
    ped->pal->rgb[i*3+1] = (col >> 8) & 0xFF;
    ped->pal->rgb[i*3+2] = col & 0xFF;
  }
  drawpalette(ped);
}


static void pressrunrgb(void *obj)
{
  PALETTE_EDITOR *ped = obj;
  int low, high;
  int i;
  float t;
  int red, green, blue;

  if(ped->sel1 < ped->sel2)
  {
    low = ped->sel1;
    high = ped->sel2;
  }
  else
  {
    low = ped->sel2;
    high = ped->sel1;
  }
  for(i=low+1;i<=high-1;i++)
  {
    t = (i-low)/((float) high-low);
    red = (int) lerp(ped->pal->rgb[low*3], ped->pal->rgb[high*3], t);
    green = (int) lerp(ped->pal->rgb[low*3+1], ped->pal->rgb[high*3+1], t);
    blue = (int) lerp(ped->pal->rgb[low*3+2], ped->pal->rgb[high*3+2], t);
    ped->pal->rgb[i*3] = red;
    ped->pal->rgb[i*3+1] = green;
    ped->pal->rgb[i*3+2] = blue;
  }
  drawpalette(ped);
}
