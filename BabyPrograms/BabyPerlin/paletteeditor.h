#ifndef paletteeditor_h
#define paletteeditor_h

typedef struct
{
  unsigned char *rgb;
  int N;
} PALETTE;

void openpaletteeditor(BABYX *bbx, PALETTE *pal);

#endif
