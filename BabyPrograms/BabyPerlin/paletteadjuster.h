#ifndef paletteadjuster_h
#define paletteadjuster_h

typedef BBX_Panel PalAdjuster;


PalAdjuster *paladjuster(BABYX *bbx, BBX_Panel *pan, unsigned char *rgb, int N,
			 void (*change)(void *ptr, unsigned char *rgb, int N), 
                         void *ptr);
void paladjuster_kill(PalAdjuster *padj);
void paladjuster_set(PalAdjuster *padj, unsigned char *rgb, int N);

#endif
