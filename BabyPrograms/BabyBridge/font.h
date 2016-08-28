#ifndef font_h
#define font_h

/* bitmap font structure */
struct bitmap_font {
  unsigned char width;		/* max. character width */
  unsigned char height;		/* character height */
  int ascent;		        /* font ascent */
  int descent;		        /* font descent */
  unsigned short Nchars;        /* number of characters in font */
  unsigned char *widths;	/* width of each character */
  unsigned short *index;	/* encoding to character index */
  unsigned char *bitmap;	/* bitmap of all characters */
};

#endif
