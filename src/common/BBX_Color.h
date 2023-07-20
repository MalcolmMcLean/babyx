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

static BBX_RGBA bbx_alphablend(BBX_RGBA fg, BBX_RGBA bg)
{
   if (bbx_alpha(fg) == 255)
     return fg;
   else if(bbx_alpha(fg) == 0)
     return bg;
   else
   {
      int fgalpha = bbx_alpha(fg);
      int bgalpha = bbx_alpha(bg);
      int clalpha = 256 - fgalpha; 
      int resultalpha = fgalpha + ((bgalpha * clalpha) >> 8);
      int resultred = (bbx_red(fg) * fgalpha + ((bbx_red(bg) * bgalpha * 
clalpha) >> 8)) / resultalpha;
      int resultgreen = (bbx_green(fg) * fgalpha + ((bbx_green(bg) * 
bgalpha * clalpha) >> 8)) / resultalpha;
      int resultblue = (bbx_blue(fg) * fgalpha + ((bbx_blue(bg) * 
bgalpha * clalpha) >> 8)) / resultalpha;

     return bbx_rgba(resultred, resultgreen, resultblue, resultalpha);      
   } 
}  
#endif
