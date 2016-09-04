#ifndef bbx_GIFanimationwidget_h
#define bbx_GIFanimationwidget_h

BBX_Panel *gifanimationwidget(BABYX *bbx, BBX_Panel *parent);
void gifanimationwidget_kill(BBX_Panel *wgt);
void gifanimation_setGIF(BBX_Panel *wgt, GIF *gif);
void gifanimation_setbackground(BBX_Panel *wgt, BBX_RGBA col);

#endif