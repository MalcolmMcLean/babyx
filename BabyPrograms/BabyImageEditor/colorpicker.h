#ifndef colorpicker_h
#define colorpicker_h

unsigned long pickcolor(BABYX *bbx, unsigned long rgb);
void rgb2hsv(unsigned long rgb, double *h, unsigned char *s, unsigned char *v);
unsigned long hsv2rgb(double h, int s, int v);

#endif
