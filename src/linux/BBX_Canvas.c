#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>
#include "BabyX.h"

#if 0

typedef struct
{
  BABYX *bbx;
  Window win;
  void (*event_handler)(void *obj, XEvent *event);
  int (*message_handler)(void *obj, int message, int a, int b, void *params);
  /* private stuff */
  void *(mousefunc)(void *ptr, int action, int x, int y, int buttons);
  void *ptr;
  unsigned char *rgba;
  int width;
  int height;
  unsigned long background;

  XImage *img;
} BBX_Canvas;


#endif

XImage *CreateTrueColorImage(Display *display, Visual *visual, unsigned char *image, int width, int height);

static void event_handler(void *obj, XEvent *event);
static void resizergba(unsigned char *dest, int dwidth, int dheight, unsigned char *src, int swidth, int sheight);

BBX_Canvas *bbx_canvas(BABYX *bbx, BBX_Panel*parent, int width, int height, BBX_RGBA bg)
{
  return BBX_canvas(bbx, parent->win, width, height, bg);
}

BBX_Canvas *BBX_canvas(BABYX *bbx, Window parent, int width, int height, BBX_RGBA bg)
{
  BBX_Canvas *answer;
  int i;

  answer = bbx_malloc(sizeof(BBX_Canvas));
  answer->bbx = bbx;
  answer->win = XCreateSimpleWindow(bbx->dpy, parent, 0, 0, 100, 100,
				    0, BlackPixel(bbx->dpy, 0), BBX_RgbaToX(bg));
  XSelectInput(bbx->dpy, answer->win, ExposureMask);

  answer->event_handler = event_handler;
  answer->message_handler = 0;
  answer->rgba = bbx_malloc(width * height *4);
  answer->width = width;
  answer->height = height;
  answer->background = bg;
  for(i=0;i<width*height;i++)
  {
    answer->rgba[i*4] = bbx_red(bg);
    answer->rgba[i*4+1] = bbx_green(bg);
    answer->rgba[i*4+2] = bbx_blue(bg);
    answer->rgba[i*4+3] = 0xFF;
  }

  answer->img = CreateTrueColorImage(bbx->dpy, 
		  DefaultVisual(bbx->dpy, bbx->screen),
				     answer->rgba,
                                     width,
                                     height);

  BBX_Register(bbx, answer->win, event_handler, 0, answer);

  return answer;  
}

void bbx_canvas_kill(BBX_Canvas *can)
{
  if(can)
  {
    free(can->rgba);
    BBX_Deregister(can->bbx, can->win);
    XDestroyImage(can->img);
    XDestroyWindow(can->bbx->dpy, can->win);
    free(can);
  }
}

void bbx_canvas_setimage(BBX_Canvas *can, unsigned char *rgba, int width, int height)
{
  int i, j;
  int red, green, blue, alpha;
  int bgred, bggreen, bgblue;

  bgred = bbx_red(can->background);
  bggreen = bbx_green(can->background);
  bgblue = bbx_blue(can->background);

  if(width == can->width && height == can->height)
    memcpy(can->rgba, rgba, width * height * 4);
  else
    resizergba(can->rgba, can->width, can->height, rgba, width, height);
 
  for(i=0;i<can->width * can->height;i++)
    if(can->rgba[i*4+3] != 0xFF)
    {
      j = i*4;
      alpha = can->rgba[j+3];
      red = bgred * (255-alpha) + can->rgba[j] * alpha;
      green = bggreen * (255-alpha) + can->rgba[j+1] * alpha;
      blue = bgblue * (255-alpha) + can->rgba[j+2] * alpha;
      red /= 255;
      green /= 255;
      blue /= 255;
      can->rgba[j] = red;
      can->rgba[j+1] = green;
      can->rgba[j+2] = blue;
      can->rgba[j+3] = 255;
    }
  
} 

unsigned char *bbx_canvas_rgba(BBX_Canvas *can, int *width, int *height)
{
  *width = can->width;
  *height = can->height;
  return can->rgba;
}

void bbx_canvas_flush(BBX_Canvas *can)
{
  int i, j;
  BABYX *bbx = can->bbx;

  j = 0;
  for(i=0;i<can->width * can->height;i++)
  {   
    can->img->data[j] =  can->rgba[j+2];
    can->img->data[j+1] = can->rgba[j+1];
    can->img->data[j+2] = can->rgba[j];
    can->img->data[j+3] = can->rgba[j+3];
    j+=4;
  }
  BBX_InvalidateWindow(bbx, can->win);
  
}

void bbx_canvas_setmousefunc(BBX_Canvas *can, void (*mousefunc)(void *ptr, int action, int x, int y, int buttons), void *ptr)
{
  XSetWindowAttributes swa;

  can->mousefunc = mousefunc;
  can->ptr = ptr;
  if(can->mousefunc)
    swa.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;  
  else
    swa.event_mask = ExposureMask;

  XChangeWindowAttributes(can->bbx->dpy, can->win, CWEventMask, &swa);  
}

static void event_handler(void *obj, XEvent *event)
{
  BBX_Canvas *can = obj;
  int x, y, width, height;
  BABYX *bbx;
  int button;

  bbx = can->bbx;

  switch(event->type)
  {
  case Expose: 
    bbx_getsize(bbx, can, &width, &height);      
     XPutImage(bbx->dpy, can->win, bbx->gc, can->img, 0, 0, 0, 0, width, height);
     break;
  case ButtonPress:
    button = 0;
    if(event->xbutton.button == Button1)
      button = BBX_MOUSE_BUTTON1;
    else if(event->xbutton.button == Button2)
      button = BBX_MOUSE_BUTTON2;
    else if(event->xbutton.button == Button3)
      button = BBX_MOUSE_BUTTON3;
    x = event->xmotion.x;
    y = event->xmotion.y;
    if(can->mousefunc)
      (*can->mousefunc)(can->ptr, BBX_MOUSE_CLICK, x, y, button); 
    break;
  case ButtonRelease:
    button = 0;
    if(event->xbutton.button == Button1)
      button = BBX_MOUSE_BUTTON1;
    else if(event->xbutton.button == Button2)
      button = BBX_MOUSE_BUTTON2;
    else if(event->xbutton.button == Button3)
      button = BBX_MOUSE_BUTTON3;
  
    x = event->xmotion.x;
    y = event->xmotion.y;
    if(can->mousefunc)
      (*can->mousefunc)(can->ptr, BBX_MOUSE_RELEASE, x, y, button);
  
    break;
  case MotionNotify:
    button = 0;
    if(event->xmotion.state & Button1Mask)
       button |= BBX_MOUSE_BUTTON1;
    if(event->xmotion.state & Button2Mask)
       button |= BBX_MOUSE_BUTTON2;
    if(event->xmotion.state & Button3Mask)
       button |= BBX_MOUSE_BUTTON3;
    x = event->xmotion.x;
    y = event->xmotion.y;
    if(can->mousefunc)
      (*can->mousefunc)(can->ptr, BBX_MOUSE_MOVE, x, y, button);
    break;
  }

}


XImage *CreateTrueColorImage(Display *display, Visual *visual, unsigned char *image, int width, int height)
{
    int i, j;
    unsigned char *image32=(unsigned char *)bbx_malloc(width*height*4);
    unsigned char *p=image32;
    for(i=0; i<width; i++)
    {
        for(j=0; j<height; j++)
        {
          
	  *p++= image[2]; /* blue */
	  *p++= image[1]; /* green */
	  *p++= image[0]; /* red */
          p++;  
           image += 4;
	}
    }
    return XCreateImage(display, visual, 24, ZPixmap, 0, (char *) image32, width, height, 32, 0);
}

static void resizergba(unsigned char *dest, int dwidth, int dheight, unsigned char *src, int swidth, int sheight)
{
  float a = 0.0f, b = 0.0f;
  int red, green, blue, alpha;
  float dx, dy;
  float rx, ry;
  int x, y;
  int index0, index1, index2, index3;

  dx = ((float) swidth)/dwidth;
  dy = ((float) sheight)/dheight;
  for(y=0, ry = 0;y<dheight-1;y++, ry += dy)
  {
    b = ry - (int) ry;
    for(x=0, rx = 0;x<dwidth-1;x++, rx += dx)
    {
      a = rx - (int) rx;
      index0 = (int)ry * swidth + (int) rx;
      index1 = index0 + 1;
      index2 = index0 + swidth;     
      index3 = index0 + swidth + 1;

      red = src[index0*4] * (1.0f-a)*(1.0f-b);
      green = src[index0*4+1] * (1.0f-a)*(1.0f-b);
      blue = src[index0*4+2] * (1.0f-a)*(1.0f-b);
      alpha = src[index0*4+3] * (1.0f-a)*(1.0f-b);
      red += src[index1*4] * (a)*(1.0f-b);
      green += src[index1*4+1] * (a)*(1.0f-b);
      blue += src[index1*4+2] * (a)*(1.0f-b);
      alpha += src[index1*4+3] * (a)*(1.0f-b);
      red += src[index2*4] * (1.0f-a)*(b);
      green += src[index2*4+1] * (1.0f-a)*(b);
      blue += src[index2*4+2] * (1.0f-a)*(b);
      alpha += src[index2*4+3] * (1.0f-a)*(b);
      red += src[index3*4] * (a)*(b);
      green += src[index3*4+1] * (a)*(b);
      blue += src[index3*4+2] * (a)*(b);
      alpha += src[index3*4+3] * (a)*(b);
    
      red = red < 0 ? 0 : red > 255 ? 255 : red;
      green = green < 0 ? 0 : green > 255 ? 255 : green;
      blue = blue < 0 ? 0 : blue > 255 ? 255 : blue;
      alpha = alpha < 0 ? 0 : alpha > 255 ? 255 : alpha;

      dest[(y*dwidth+x)*4] = red;
      dest[(y*dwidth+x)*4+1] = green;
      dest[(y*dwidth+x)*4+2] = blue;
      dest[(y*dwidth+x)*4+3] = alpha;
    }
    index0 = (int)ry * swidth + (int) rx;
    index1 = index0;
    index2 = index0 + swidth;     
    index3 = index0 + swidth;   

    red = src[index0*4] * (1.0f-a)*(1.0f-b);
    green = src[index0*4+1] * (1.0f-a)*(1.0f-b);
    blue = src[index0*4+2] * (1.0f-a)*(1.0f-b);
    alpha = src[index0*4+3] * (1.0f-a)*(1.0f-b);
    red += src[index1*4] * (a)*(1.0f-b);
    green += src[index1*4+1] * (a)*(1.0f-b);
    blue += src[index1*4+2] * (a)*(1.0f-b);
    alpha += src[index1*4+3] * (a)*(1.0f-b);
    red += src[index2*4] * (1.0f-a)*(b);
    green += src[index2*4+1] * (1.0f-a)*(b);
    blue += src[index2*4+2] * (1.0f-a)*(b);
    alpha += src[index2*4+3] * (1.0f-a)*(b);
    red += src[index3*4] * (a)*(b);
    green += src[index3*4+1] * (a)*(b);
    blue += src[index3*4+2] * (a)*(b);
    alpha += src[index3*4+3] * (a)*(b);
        
    red = red < 0 ? 0 : red > 255 ? 255 : red;
    green = green < 0 ? 0 : green > 255 ? 255 : green;
    blue = blue < 0 ? 0 : blue > 255 ? 255 : blue;

    alpha = alpha < 0 ? 0 : alpha > 255 ? 255 : alpha;
    dest[(y*dwidth+x)*4] = red;
    dest[(y*dwidth+x)*4+1] = green;
    dest[(y*dwidth+x)*4+2] = blue;
    dest[(y*dwidth+x)*4+3] = alpha;
  }
  index0 = (int)ry * swidth + (int) rx;
  index1 = index0;
  index2 = index0 + swidth;     
  index3 = index0 + swidth;   

  for(x=0, rx = 0;x<dwidth-1;x++, rx += dx)
  {
    a = rx - (int) rx;
    index0 = (int)ry * swidth + (int) rx;
    index1 = index0 + 1;
    index2 = index0;     
    index3 = index0;

    red = src[index0*4] * (1.0f-a)*(1.0f-b);
    green = src[index0*4+1] * (1.0f-a)*(1.0f-b);
    blue = src[index0*4+2] * (1.0f-a)*(1.0f-b);
    alpha = src[index0*4+3] * (1.0f-a)*(1.0f-b);
    red += src[index1*4] * (a)*(1.0f-b);
    green += src[index1*4+1] * (a)*(1.0f-b);
    blue += src[index1*4+2] * (a)*(1.0f-b);
    alpha += src[index1*4+3] * (a)*(1.0f-b);
    red += src[index2*4] * (1.0f-a)*(b);
    green += src[index2*4+1] * (1.0f-a)*(b);
    blue += src[index2*4+2] * (1.0f-a)*(b);
    alpha += src[index2*4+3] * (1.0f-a)*(b);
    red += src[index3*4] * (a)*(b);
    green += src[index3*4+1] * (a)*(b);
    blue += src[index3*4+2] * (a)*(b);
    alpha += src[index3*4+3] * (a)*(b);

    red = red < 0 ? 0 : red > 255 ? 255 : red;
    green = green < 0 ? 0 : green > 255 ? 255 : green;
    blue = blue < 0 ? 0 : blue > 255 ? 255 : blue;
    alpha = alpha < 0 ? 0 : alpha > 255 ? 255 : alpha;
      
    dest[(y*dwidth+x)*4] = red;
    dest[(y*dwidth+x)*4+1] = green;
    dest[(y*dwidth+x)*4+2] = blue;
    dest[(y*dwidth+x)*4+3] = alpha;
  }
  
  dest[(y*dwidth+x)*4] = src[((sheight-1)*swidth+swidth-1)*4];
  dest[(y*dwidth+x)*4+1] = src[((sheight-1)*swidth+swidth-1)*4+1];
  dest[(y*dwidth+x)*4+2] = src[((sheight-1)*swidth+swidth-1)*4+2];
  dest[(y*dwidth+x)*4+3] = src[((sheight-1)*swidth+swidth-1)*4+3];
}  
