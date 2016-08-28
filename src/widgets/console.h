#ifndef console_h
#define console_h

/* message to add text - wParam 0, lParam string */
#define CONM_ADDTEXT 0x8001

/* message to set text colour - lParam = new COLORREF */
/*           returns previous colour                  */
#define CONM_SETCOL 0x8002


void RegisterConsole(HINSTANCE hInstance);
void MakeConsole(HINSTANCE hInstance);
void Con_Printf(char *fmt, ...);
void Win_Printf(HWND hwnd, char *fmt, ...);
COLORREF Con_SetCol(HWND hwnd, COLORREF col);

#endif