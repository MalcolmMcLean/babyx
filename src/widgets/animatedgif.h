#ifndef gif_h
#define gif_h

typedef struct
{
    int left;
    int top;
    int width;
    int height;
    int transparent;
    int disposal;
	int delay;
    unsigned char *image;
    unsigned char pal[256*3];
} GIFFRAME;

typedef struct
{
    GIFFRAME *frames;
    int Nframes;
    int transparent;
    int width;
    int height;
    int background;
    int current;
    unsigned char *store;
} GIF;

GIF *loadanimatedgif(char *fname);
GIF *floadanimatedgif(FILE *fp);
void killgif(GIF *gif);
unsigned char *gif_getfirstframe(GIF *gif, int *width, int *height, unsigned char *pal, int *transparent);
int gif_getnextframe(GIF *gif, unsigned char *buff);


unsigned char *loadgif(char *fname, int *width, int *height, unsigned char *pal, int *transparent);
int savegif(char *fname, unsigned char *data, int width, int height, unsigned char *pal, int palsize, int transparent, int important, int interlaced);

#endif
