#ifndef bbx_messagebox_h
#define bbx_nessagebox_h

#define BBX_MB_CANCEL 0 
#define BBX_MB_OK 1
#define BBX_MB_OK_CANCEL 2
#define BBX_MB_YES_NO_CANCEL 3
#define BBX_MB_YES 5
#define BBX_MB_NO 6


int bbx_messagebox(BABYX *bbx, int type, char *title, char *msgfmt, ...);
 
#endif
