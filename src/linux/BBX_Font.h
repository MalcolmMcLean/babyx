#ifndef bbx_font_h
#define bbx_font_h

#include "BBX_Color.h"

int bbx_textwidth(struct bitmap_font *font, char *str, int N);
void bbx_drawstring(unsigned char *rgba, int width, int height, int x, int y, char *str, int N, struct bitmap_font *font, BBX_RGBA col);
int bbx_utf8width(struct bitmap_font *font, char *utf8, int N);
void bbx_drawutf8(unsigned char *rgba, int width, int height, int x, int y, char *utf8, int N, struct bitmap_font *font, BBX_RGBA col);
int bbx_font_getchar(unsigned char *out, struct bitmap_font *font, int ch);

int bbx_isutf8z(const char *str);
int bbx_utf8_skip(const char *utf8);
int bbx_utf8_getch(const char *utf8);
int bbx_utf8_putch(char *out, int ch);
int bbx_utf8_Nchars(const char *utf8);

#endif
