#include <stdlib.h>
#include <string.h>
#include "font.h"
#include "BBX_Color.h"
#include "BBX_Font.h"

int bbx_textwidth(struct bitmap_font *font, char *str, int N);
void bbx_drawstring(unsigned char *rgba, int width, int height, int x, int y, char *str, int N, struct bitmap_font *font, unsigned long col);
int bbx_font_getchar(unsigned char *out, struct bitmap_font *font, int ch);

static int findch(const unsigned short *idx, int N, int ch);
static void pastech(unsigned char *rgba, int width, int height, unsigned char *glyph, int gwidth, int gheight, int x, int y, BBX_RGBA);

/*
  Get the width of a text string, using the font
 */
int bbx_textwidth(struct bitmap_font *font, char *str, int N)
{
  int i;
  int idx;
  int answer = 0;  

  for(i=0;i<N;i++)
  {
    if(!str[i])
      break;
    idx = findch(font->index, font->Nchars, str[i]);
    if(idx != -1)
      answer += font->widths[idx];
    else
      answer += font->widths[0]; 
  }

  return answer;
} 

/*
  draw a text string on an rgba buffer
  Params: 
    rgba - the rgba buffer, red first byte alpha last
    width - rgba buffer width
    height - rgba buffer height
    x, y - co-ords at which to draw string, y = "the line"
    str - string to draw
    N - number of characters to draw
    font - font to use
    col - RGB text colour, red top 24-17 bits
 */
void bbx_drawstring(unsigned char *rgba, int width, int height, int x, int y, char *str, int N, struct bitmap_font *font, BBX_RGBA col)
{
  unsigned char *glyph;
  int idx;
  int dx;
  int i;

  for(i=0;i<N;i++)
  {
    if(!str[i])
      break;
    idx = findch(font->index, font->Nchars, str[i]);
    if(idx == -1)
      idx = 0;
    dx = font->widths[idx];
    glyph = font->bitmap + idx * font->width * font->height;
    pastech(rgba, width, height, glyph, font->width, font->height, x, y - font->ascent, col);
    x += dx;
  }
  
}

int bbx_utf8width(struct bitmap_font *font, char *utf8, int N)
{
  int pos = 0;
  int idx;
  int ch;
  int nb;
  int answer = 0;

  while(pos < N)
  {
    if(!utf8[pos])
      break;
    nb = bbx_utf8_skip(utf8 + pos);
    if(nb + pos > N)
      break;
    ch = bbx_utf8_getch(utf8 + pos);
    idx = findch(font->index, font->Nchars, ch);
    if(idx != -1)
      answer += font->widths[idx];
    else
      answer += font->widths[0];
    pos += nb; 
  }

  return answer;
}

void bbx_drawutf8(unsigned char *rgba, int width, int height, int x, int y, char *utf8, int N, struct bitmap_font *font, BBX_RGBA col)
{
 unsigned char *glyph;
  int idx;
  int dx;
  int pos = 0;
  int ch;

  while(pos < N)
  {
    if(!utf8[pos])
      break;
    ch = bbx_utf8_getch(utf8 + pos);
    idx = findch(font->index, font->Nchars, ch);
    if(idx == -1)
      idx = 0;
    dx = font->widths[idx];
    glyph = font->bitmap + idx * font->width * font->height;
    pastech(rgba, width, height, glyph, font->width, font->height, x, y - font->ascent, col);
    x += dx;
    pos += bbx_utf8_skip(utf8);
  }
}



/*
   get a character glyph
   Params: out - return for character glyph
           font - the font
           ch - index of character
   Returns: width of glyph on success, 0 on failure
 */
int bbx_font_getchar(unsigned char *out, struct bitmap_font *font, int ch)
{
  int idx;
  idx = findch(font->index, font->Nchars, ch);
  if(idx != -1)
  {
       memcpy(out, font->bitmap + idx * font->width * font->height, font->width * font->height);
       return font->widths[idx];
  }
  memset(out, 0, font->width * font->height);
  return -1;
}

static int findch(const unsigned short *idx, int N, int ch)
{
  int high, low, mid;

  low = 0;
  high = N-1;

  while(low <= high)
  {
    mid = (low + high)/2;
    if(idx[mid] == ch)
      return mid;
    else if(idx[mid] < ch)
      low = mid+1;
    else
      high = mid-1; 
  }
  return -1;
}

static void pastech(unsigned char *rgba, int width, int height, unsigned char *glyph, int gwidth, int gheight, int x, int y, unsigned long col)
{
  int sx, sy;
  int red, green, blue, alpha;
  int tx, ty, twidth, theight;
  int index;

  red = bbx_red(col);
  green = bbx_green(col);
  blue = bbx_blue(col);

  ty = y < 0 ? -y : 0;
  tx = x < 0 ? -x : 0;
  theight = y + gheight > height ? height - y : gheight;
  twidth = x + gwidth > width ? width - x :  gwidth; 
    
  for(sy=ty;sy<theight;sy++)
  {
    for(sx=tx;sx<twidth;sx++)
    {
      if(glyph[sy*gwidth+sx] == 0xFF)
      {
	index = ((sy+y)*width+sx+x)*4;
        rgba[index] = red;
        rgba[index+1] = green;
        rgba[index+2] = blue;
        rgba[index+3] = 0xFF;
      }
      else if(glyph[sy*gwidth+sx] != 0)
      {
        alpha = glyph[sy * gwidth +sx];
        index = ((sy+y)*width+sx+x)*4;
        rgba[index] = (alpha *(red - rgba[index]) + ((int)rgba[index] << 8))>>8; 
        rgba[index+1] = (alpha *(green - rgba[index+1]) + ((int)rgba[index+1] << 8))>>8; 
        rgba[index+2] = (alpha *(blue - rgba[index+2]) + ((int)rgba[index+2] << 8))>>8; 
        rgba[index+3] = 255 - (((255 - rgba[index+3]) * (255 - alpha)) >> 8); 
      }
    }  
  }
} 

static const unsigned int offsetsFromUTF8[6] = 
{
    0x00000000UL, 0x00003080UL, 0x000E2080UL,
    0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

static const unsigned char trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

int bbx_isutf8z(const char *str)
{
  int len = 0;
  int pos = 0;
  int nb;
  int i;
  int ch;

  while(str[len])
    len++;
  while(pos < len && *str)
  {
    nb = bbx_utf8_skip(str);
    if(nb < 1 || nb > 4)
      return 0;
    if(pos + nb > len)
      return 0;
    for(i=1;i<nb;i++)
      if( (str[i] & 0xC0) != 0x80 )
        return 0;
    ch = bbx_utf8_getch(str);
    if(ch < 0x80)
    {
      if(nb != 1)
        return 0;
    }
    else if(ch < 0x8000)
    {
      if(nb != 2)
        return 0;
    }
    else if(ch < 0x10000)
    {
      if(nb != 3)
        return 0;
    }
    else if(ch < 0x110000)
    {
      if(nb != 4)
        return 0;
    }
    pos += nb;
    str += nb;    
  }

  return 1;
}

int bbx_utf8_skip(const char *utf8)
{
  return trailingBytesForUTF8[(unsigned char) *utf8] + 1;
}

int bbx_utf8_getch(const char *utf8)
{
    int ch;
    int nb;

    nb = trailingBytesForUTF8[(unsigned char)*utf8];
    ch = 0;
    switch (nb) 
    {
            /* these fall through deliberately */
        case 3: ch += (unsigned char)*utf8++; ch <<= 6;
        case 2: ch += (unsigned char)*utf8++; ch <<= 6;
        case 1: ch += (unsigned char)*utf8++; ch <<= 6;
        case 0: ch += (unsigned char)*utf8++;
    }
    ch -= offsetsFromUTF8[nb];
    
    return ch;
}

int bbx_utf8_putch(char *out, int ch)
{
  char *dest = out;
  if (ch < 0x80) 
  {
     *dest++ = (char)ch;
  }
  else if (ch < 0x800) 
  {
    *dest++ = (ch>>6) | 0xC0;
    *dest++ = (ch & 0x3F) | 0x80;
  }
  else if (ch < 0x10000) 
  {
     *dest++ = (ch>>12) | 0xE0;
     *dest++ = ((ch>>6) & 0x3F) | 0x80;
     *dest++ = (ch & 0x3F) | 0x80;
  }
  else if (ch < 0x110000) 
  {
     *dest++ = (ch>>18) | 0xF0;
     *dest++ = ((ch>>12) & 0x3F) | 0x80;
     *dest++ = ((ch>>6) & 0x3F) | 0x80;
     *dest++ = (ch & 0x3F) | 0x80;
  }
  else
    return 0;
  return dest - out;
}

int bbx_utf8_charwidth(int ch)
{
	if (ch < 0x80)
	{
		return 1;
	}
	else if (ch < 0x800)
	{
		return 2;
	}
	else if (ch < 0x10000)
	{
		return 3;
	}
	else if (ch < 0x110000)
	{
		return 4;
	}
	else
		return 0;
}

int bbx_utf8_Nchars(const char *utf8)
{
  int answer = 0;

  while(*utf8)
  {
    utf8 += bbx_utf8_skip(utf8);
    answer++;
  }

  return answer;
}

/*
extern struct bitmap_font vera_font;
#include <stdio.h>
#include "lodepng.h"

int main(void)
{
  int width;
  int x, y;
  unsigned char *buff;
  unsigned char *rgba;

  width = bbx_textwidth(&vera_font,"Fred", 4);
  printf("width %d\n", width);
  buff = malloc(vera_font.width * vera_font.height);
  bbx_font_getchar(buff, &vera_font, 8211);
  for(y=0;y<vera_font.height;y++)
  {
    for(x=0;x<vera_font.width;x++)
      printf("%c", buff[y*vera_font.width+x] ? '#' : '.');
    printf("\n");
  }
  rgba = malloc(100*100*4);
  memset(rgba, 255, 100*100*4);
  bbx_drawstring(rgba, 100, 100, 10, 20, "Alfred", 6, &vera_font, 0xFF0000);   
  lodepng_encode32_file("fred.png", rgba, 100, 100);
  return 0;
}


*/
