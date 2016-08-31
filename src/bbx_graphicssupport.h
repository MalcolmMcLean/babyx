#ifndef graphicssupport_h
#define graphicssupport_h

#include "BBX_Color.h"

int bbx_line(unsigned char *rgba, int width, int height, double x1, double y1, double x2, double y2, BBX_RGBA col);
int bbx_lineaa(unsigned char *rgba, int width, int height, double x1, double y1, double x2, double y2, double lwidth, BBX_RGBA col);
int bbx_rectangle(unsigned char *rgba, int width, int height, double x1, double y1, double x2, double y2, BBX_RGBA col);
int bbx_circle(unsigned char *rgba, int width, int height, double x, double y, double r, BBX_RGBA col);
int bbx_polygon(unsigned char *rgba, int width, int height, double *x, double *y, int N, BBX_RGBA col);
int bbx_polygonaa(unsigned char *rgba, int width, int height, double *x, double *y, int N, BBX_RGBA col);

#endif

