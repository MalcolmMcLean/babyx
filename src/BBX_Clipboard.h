#ifndef bbx_clipboard_h
#define bbx_clipboard_h

typedef struct
{
 // Display *dpy;
 // Window win;
 // Atom atom_UTF8_STRING;
 // Atom atom_CLIPBOARD;
 // Atom atom_TARGETS;
  HWND hwnd;
  char *text;
} BBX_Clipboard;


BBX_Clipboard *BBX_clipboard(HWND win);
void BBX_clipboard_kill(BBX_Clipboard *clip);
//void BBX_clipboard_handleselectionrequest(BBX_Clipboard *clip, XSelectionRequestEvent *evt);

#endif
