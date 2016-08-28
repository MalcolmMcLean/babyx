#ifndef bbx_color_h
#define bbx_color_h

typedef unsigned long BBX_RGBA;

#define bbx_rgba(r,g,b,a) ((BBX_RGBA) ( ((r) << 24) | ((g) << 16) | ((b) << 8) | (a) ))
#define bbx_rgb(r, g, b) bbx_rgba(r,g,b, 255)
#define bbx_red(col) ((col >> 24) & 0xFF)
#define bbx_green(col) ((col >> 16) & 0xFF)
#define bbx_blue(col) ((col >> 8) & 0xFF)
#define bbx_alpha(col) (col & 0xFF)

#define BBX_RgbaToX(col) ( (col >> 8) & 0xFFFFFF )
  
#endif
