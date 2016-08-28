#ifndef rgbautil_h
#define rgbautil_h

void rgbaclear(unsigned char *rgba, int width, int height, unsigned long rgb);
void rgbapaste(unsigned char *rgba, int width, int height, unsigned char *sub, int swidth, int sheight, int x, int y);
void rgbapasterot(unsigned char *rgba, int width, int height, unsigned char *sub, int swidth, int sheight, int x, int y, double theta);
int rgbarotatebyshear(unsigned char *rgba, int width, int height, double cx, double cy, double theta, unsigned char *out);

unsigned char *rgbarot90(unsigned char *rgba, int width, int height);
#endif
