#ifndef loadttffont_h
#define loadttffont_h

struct bitmap_font *loadttffontsample(char *path, int points, char *textout, int N);
struct bitmap_font *loadttffontfull(char *path, int points);

#endif