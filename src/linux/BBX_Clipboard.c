#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "BabyX.h"
#include <X11/Xatom.h>

BBX_Clipboard *BBX_clipboard(Display *dpy, Window win) 
{
  BBX_Clipboard *answer;

  answer = bbx_malloc(sizeof(BBX_Clipboard));
  answer->dpy = dpy;
  answer->win = win;
  
  answer->atom_UTF8_STRING = XInternAtom (dpy, "UTF8_STRING", False);
  answer->atom_CLIPBOARD  = XInternAtom (dpy, "CLIPBOARD", False);
  answer->atom_TARGETS = XInternAtom (dpy, "TARGETS", False);
  answer->text = 0;

  return answer;
}

void BBX_clipboard_kill(BBX_Clipboard *clip)
{
  if(clip)
  {
    free(clip->text);
    free(clip);
  }
}

  /* read the content of a window property as either a locale-dependent string or an utf8 string 
     works only for strings shorter than 1000000 bytes 
  */
  
static char *readWindowProperty(BBX_Clipboard *clip, 
                                Window window, 
                                Atom prop, 
                                Atom fmt /* XA_STRING or UTF8_STRING */, 
                                int deleteAfterReading) 
{
    char *returnData = 0;
    unsigned char *clipData;
    Atom actualType;
    int  actualFormat;
    unsigned long nitems, bytesLeft;

    if (XGetWindowProperty (clip->dpy, clip->win, prop, 
                            0L /* offset */, 1000000 /* length (max) */, False, 
                            AnyPropertyType /* format */, 
                            &actualType, &actualFormat, &nitems, &bytesLeft,
                            &clipData) == Success) 
    {
      if (actualType == clip->atom_UTF8_STRING && actualFormat == 8)
      {
        returnData = (char *) clipData;
      } 
      else if (actualType == XA_STRING && actualFormat == 8) {
        returnData = (char *) clipData;
      }
      
    }
    
    if (deleteAfterReading) 
    {
      XDeleteProperty (clip->dpy, window, prop);
    }

    return returnData;
  }

  /* send a SelectionRequest to the window owning the selection and waits for its answer (with a timeout) */
static char *requestSelectionContent(BBX_Clipboard *clip, Atom selection, Atom requested_format) 
{
    Atom property_name = XInternAtom(clip->dpy, "BBX_SEL", False); 
    /* the selection owner will be asked to set the JUCE_SEL property on the juce_messageWindowHandle with the selection content */
    XConvertSelection(clip->dpy, selection, requested_format, property_name, clip->win, CurrentTime);
    int gotReply = 0;
    int timeoutMs = 200; // will wait at most for 200 ms
    do {
      XEvent event;
      gotReply = XCheckTypedWindowEvent(clip->dpy, clip->win, SelectionNotify, &event);
      if (gotReply) {
        if (event.xselection.property == property_name) 
        {
          return readWindowProperty(clip, event.xselection.requestor,
                                                          event.xselection.property, requested_format, 1);

        } 
        else 
        {
          return 0; // the format we asked for was denied.. (event.xselection.property == None)
        }
      }
      /* not very elegant.. we could do a select() or something like that... however clipboard content requesting
         is inherently slow on x11, it often takes 50ms or more so... */
      sleep(4); 
      timeoutMs -= 4;
    } while (timeoutMs > 0);
   
    return 0;
  }

  /* called from the event loop in juce_linux_Messaging in response to SelectionRequest events */
void BBX_clipboard_handleselectionrequest(BBX_Clipboard *clip, XSelectionRequestEvent *evt) 
{

    /* the selection content is sent to the target window as a window property */    
    XSelectionEvent reply;
    reply.type = SelectionNotify;
    reply.display = evt->display;
    reply.requestor = evt->requestor;
    reply.selection = evt->selection;
    reply.target = evt->target;
    reply.property = None; // == "fail"
    reply.time = evt->time;

    char *data = 0;
    int property_format = 0, data_nitems = 0;
    if (evt->selection == XA_PRIMARY || evt->selection == clip->atom_CLIPBOARD) 
    {
      if (evt->target == XA_STRING) 
      {
        // format data according to system locale
        data = bbx_strdup((const char*)clip->text);
        data_nitems = strlen(data);
        property_format = 8; // bits/item
      } 
      else if (evt->target == clip->atom_UTF8_STRING) 
      {
        // translate to utf8
        data = bbx_strdup((const char*)clip->text);
        data_nitems = strlen(data);
        property_format = 8; // bits/item
      } 
      else if (evt->target == clip->atom_TARGETS) 
      {
        // another application wants to know what we are able to send
        data_nitems = 2;
        property_format = 32; // atoms are 32-bit        
        data = (char*)bbx_malloc(data_nitems * 4);
        ((Atom*)data)[0] = clip->atom_UTF8_STRING;
        ((Atom*)data)[1] = XA_STRING;
      }
    } 
    else 
    {
    }
    if (data) 
    {
      const size_t MAX_REASONABLE_SELECTION_SIZE = 1000000;
      // for very big chunks of data, we should use the "INCR" protocol , which is a pain in the *ss
      if (evt->property != None && strlen(data) < MAX_REASONABLE_SELECTION_SIZE) {
        XChangeProperty(evt->display, evt->requestor,
                        evt->property, evt->target,
                        property_format /* 8 or 32 */, PropModeReplace,
                        (const unsigned char*)data, data_nitems);
        reply.property = evt->property; // " == success"
      }
      free(data);
    }

    XSendEvent(evt->display, evt->requestor, 0, NoEventMask,
               (XEvent *) &reply);
  
}

void copyTextToClipboard(BBX_Clipboard *clip, char *text)
{
  if(clip->text)
    free(clip->text);
  clip->text = bbx_strdup(text);
  XSetSelectionOwner(clip->dpy, XA_PRIMARY, clip->win, CurrentTime);
  XSetSelectionOwner(clip->dpy, clip->atom_CLIPBOARD, clip->win, CurrentTime);
}

   
char *getTextFromClipboard(BBX_Clipboard *clip)
{
    /* 1) try to read from the "CLIPBOARD" selection first (the "high
       level" clipboard that is supposed to be filled by ctrl-C
       etc). When a clipboard manager is running, the content of this
       selection is preserved even when the original selection owner
       exits.

       2) and then try to read from "PRIMARY" selection (the "legacy" selection
       filled by good old x11 apps such as xterm)

       3) a third fallback could be CUT_BUFFER0 but they are obsolete since X10 !
       ( http://www.jwz.org/doc/x-cut-and-paste.html )

       There is not content negotiation here -- we just try to retrieve the selection first
       as utf8 and then as a locale-dependent string
    */
    Atom selection = XA_PRIMARY;
    Window selection_owner = None;
    char *content;

    if ((selection_owner = XGetSelectionOwner(clip->dpy, selection)) == None)
    {
      selection = clip->atom_CLIPBOARD;
      selection_owner = XGetSelectionOwner(clip->dpy, selection);
    }
    
    if (selection_owner != None) 
    {
      if (selection_owner == clip->win) 
      {
        content = bbx_strdup(clip->text);
      } 
      else 
      {
        /* first try: we want an utf8 string */
        content = requestSelectionContent(clip, selection, clip->atom_UTF8_STRING);
        if (!content) {
          /* second chance, ask for a good old locale-dependent string ..*/
          content = requestSelectionContent(clip, selection, XA_STRING);
        }
      }
    }
    return content;
  }


void bbx_copytexttoclipboard(BABYX *bbx, char *text)
{
  if(bbx->clipboard)
  {
    copyTextToClipboard(bbx->clipboard, text);
  }    
}

char *bbx_gettextfromclipboard(BABYX *bbx)
{
  if(bbx->clipboard)
  {
    return getTextFromClipboard(bbx->clipboard);
  }
  return 0;
}
