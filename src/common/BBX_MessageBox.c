#include <stdio.h>
#include <stdarg.h>

#include "BabyX.h"

typedef struct
{
  BABYX *bbx;
  BBX_Panel *pan;
  BBX_Label *msg_lab;
  BBX_Button *ok_but;
  BBX_Button *cancel_but; 
  BBX_Button *yes_but; 
  BBX_Button *no_but; 
  int type;
  int answer;
} MESSAGEBOX;

static void layout(void *obj, int width, int height);
static void pressyes(void *obj);
static void pressno(void *obj);
static void pressok(void *obj);
static void presscancel(void *obj);

int bbx_messagebox(BABYX *bbx, int type, char *title, char *msg, ...)
{
  char buffer[1024];
  va_list args;
  MESSAGEBOX mb;
  int pwidth, pheight;
  int width, height;

  va_start (args, msg);
  vsnprintf (buffer, 1024, msg, args);
  va_end (args);

  mb.bbx = bbx;
  mb.type = type;
  mb.pan = bbx_dialogpanel(bbx, title, 100, 100, layout, &mb);
  bbx_dialogpanel_setclosefunc(mb.pan, pressok, &mb);
  mb.msg_lab = bbx_label(bbx, mb.pan, buffer);
  bbx_label_setbackground(mb.msg_lab, bbx_color("gray"));
  mb.ok_but = 0;
  mb.cancel_but = 0;
  mb.yes_but = 0;
  mb.no_but = 0;

  switch(type)
  {
  case BBX_MB_OK:
    mb.ok_but = bbx_button(bbx, mb.pan, "OK", pressok, &mb);
    break;
  case BBX_MB_OK_CANCEL:
    mb.ok_but = bbx_button(bbx, mb.pan, "OK", pressok, &mb);
    mb.cancel_but = bbx_button(bbx, mb.pan, "Cancel", presscancel, &mb);
    break;
  case BBX_MB_YES_NO_CANCEL:
    mb.yes_but =  bbx_button(bbx, mb.pan, "Yes", pressyes, &mb);
    mb.no_but =  bbx_button(bbx, mb.pan, "No", pressno, &mb);
    mb.cancel_but = bbx_button(bbx, mb.pan, "Cancel", presscancel, &mb);
  default: 
    break;
  }
  bbx_label_getpreferredsize(mb.msg_lab, &pwidth, &pheight);
  width = pwidth+30;
  if(width < 200)
    width = 200;
  if(width > 600)
    width = 600;
  height = pheight+100;
  if(height < 150)
    height = 150;
  if(height > 500)
    height = 500;
 
  bbx_setsize(bbx, mb.pan, width, height);

  bbx_dialogpanel_makemodal(mb.pan);

  bbx_label_kill(mb.msg_lab);
  bbx_button_kill(mb.ok_but);
  bbx_button_kill(mb.cancel_but);
  bbx_button_kill(mb.yes_but);
  bbx_button_kill(mb.no_but);

  bbx_dialogpanel_kill(mb.pan);
  return mb.answer;
} 

static void layout(void *obj, int width, int height)
{
  int pwidth, pheight;
  MESSAGEBOX *mb = obj;
  BABYX *bbx = mb->bbx;

  bbx_label_getpreferredsize(mb->msg_lab, &pwidth, &pheight);
  switch(mb->type)
  {
  case BBX_MB_OK:
    bbx_setpos(bbx, mb->ok_but, width/2-30, height-40, 60, 30);
    break;
  case BBX_MB_OK_CANCEL:
    bbx_setpos(bbx, mb->ok_but, width/4-30, height-40, 60, 30);
    bbx_setpos(bbx, mb->cancel_but, width/2, height-40, 60, 30); 
    break;
  case BBX_MB_YES_NO_CANCEL:
    bbx_setpos(bbx, mb->yes_but, width/4-30, height-40, 60, 30);
    bbx_setpos(bbx, mb->no_but, width/2, height-40, 60, 30); 
    bbx_setpos(bbx, mb->cancel_but, width/2+50, height-40, 60, 30); 
    break; 
  }
  bbx_setpos(bbx, mb->msg_lab, (width -pwidth)/2, 25, pwidth, pheight);
}

static void pressyes(void *obj)
{
  MESSAGEBOX *mb = obj;
  mb->answer = BBX_MB_YES;
  bbx_dialogpanel_dropmodal(mb->pan);
}

static void pressno(void *obj)
{
  MESSAGEBOX *mb = obj;
  mb->answer = BBX_MB_NO;
  bbx_dialogpanel_dropmodal(mb->pan);
}

static void pressok(void *obj)
{
  MESSAGEBOX *mb = obj;
  mb->answer = BBX_MB_OK;
  bbx_dialogpanel_dropmodal(mb->pan);
}

static void presscancel(void *obj)
{
  MESSAGEBOX *mb = obj;
  mb->answer = BBX_MB_CANCEL;
  bbx_dialogpanel_dropmodal(mb->pan);
}
