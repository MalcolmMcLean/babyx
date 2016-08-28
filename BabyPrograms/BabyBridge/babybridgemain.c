#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "BabyX.h"

#include "shuffle.h"
#include "deck.h"
#include "bridgegame.h"
#include "bridgeai.h"
#include "cardpic.h"

extern struct bitmap_font verabd_font;

#define uniform() (rand()/(RAND_MAX + 1.0))
typedef struct
{
  BABYX *bbx;
  BBX_Panel *root;
  BBX_Panel *score_pan;
  BBX_Panel *current_pan;
  BBX_Menubar *menu;
  BBX_Label *bidder_lab[4];
  BBX_Label **bid_lab;
  int Nbids;
  BBX_Button *but;
  BBX_RadioBox *level_rad;
  BBX_RadioBox *suit_rad;
  BBX_Button *pass_but;
  BBX_Button *bid_but;
  BBX_Button *double_but;
  BBX_Button *redouble_but;
  BBX_Canvas *can;
  BBX_Canvas *dummy_can;
  BBX_Canvas *ncard_can;
  BBX_Canvas *scard_can;
  BBX_Canvas *ecard_can;
  BBX_Canvas *wcard_can;
  BBX_Label *contract_lab;
  BBX_Label *tricksEW_lab;
  BBX_Label *tricksNS_lab;
  BBX_Label *score_lab;
  BBX_Label *totNS_lab;
  BBX_Label *totEW_lab;
  BBX_Label *rubbers_lab;
  BBX_Label *game1_lab;
  BBX_Label *game2_lab;
  BBX_Label *game3_lab;
  BBX_Label *over_lab[3];
  BBX_Label *under_lab[3];
 
  GAME *game;
  int selcard;
  int seldummy;
  int state;

  int userhand_xpos[13];
  int userhand_ypos[13];
  int dummyhand_xpos[13];
  int dummyhand_ypos[13];
  double playtheta[4];

} APP;

void create(void *obj, BABYX *bbx, BBX_Panel *root);
void layout(void *obj, int width, int height);
void layoutscoring(void *obj, int width, int height);
static void clickbutton(void *obj);
static void aboutdialog(APP *app);
static void helpdialog(APP *app);
static void menuchosen(void *obj, int id);
static void canvasfunc(void *obj, int action, int x, int y, int buttons);
static void dummycanvasfunc(void *obj, int action, int x, int y, int buttons);

static void bidpass(void *obj);
static void bidbidding(void *obj);
static void biddouble(void *obj);
static void bidredouble(void *obj);
static void userbid(APP *app, BID *bid);
static void app_playcard(APP *app, CARD *card);
static void app_gathertricks(APP *app);

static void app_startbidding(APP *app);
static void app_endbidding(APP *app);
static void app_endplay(APP *app);
static void app_createplaywidgets(APP *app);
static void layoutcurrent(void *obj, int width, int height);
static void app_createplaycanvas(APP *app);
static void app_updateuserhand(APP *app);
static void app_updatedummy(APP *app);
static void app_createdummy(APP *app);
static void app_updatewidgets(APP *app);
static void app_updatescoring(APP *app);
static void app_addbid(APP *app, BID *bid);
static void getcardpos(int *xpos, int *ypos, CARD *hand, int N);
static void bidtostr(char *str, BID *bid);
static void sorthand(CARD *hand, int N, int trumps);

int _stdcall WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmndline, int show )
{
	APP *app;
	srand(time(0));
	app = malloc(sizeof(APP));
	startbabyx(hInst, "Baby Bridge", 1000, 800, create, layout, app);
	free(app);
	return 0;
}
/*
int main(void)
{
  APP *app;
  app = malloc(sizeof(APP));
  startbabyx("Baby Bridge", 1000, 800, create, layout, app);
  free(app);
  return 0;
}
*/

void create(void *obj, BABYX *bbx, BBX_Panel *root)
{
  APP *app = obj;
 
  app->game = newgame();
  sorthand(app->game->south, 13, 0);
  getcardpos(app->userhand_xpos, app->userhand_ypos, app->game->south, 13); 

  app->bbx = bbx;
  app->root = root;

  app->Nbids = 0;
  app->bid_lab = 0;

  bbx_panel_setbackground(root, bbx_color("DarkGreen")); 

  BBX_Popup *pop = bbx_popup(bbx);
  bbx_popup_append(pop, 1, "Open ...", "", 0);
  bbx_popup_append(pop, 2, "Save ...", "", 0);
  bbx_popup_append(pop, 3, "Exit", "", 0);

  BBX_Popup *pop2 = bbx_popup(bbx);
  bbx_popup_append(pop2, 4, "About", "", 0);
  bbx_popup_append(pop2, 5, "Help", "", 0);
  bbx_popup_append(pop2, 6, "Rules", "", 0);
    
  app->menu = bbx_menubar(bbx, root, menuchosen, app);
  bbx_menubar_addmenu(app->menu, "File", pop);
  bbx_menubar_addmenu(app->menu, "Help", pop2);

  bbx_menubar_disable(app->menu, 1);
  bbx_menubar_disable(app->menu, 2);
  bbx_menubar_disable(app->menu, 6);

  app->but = bbx_button(bbx, root, "Click me", clickbutton, app);
   
  
  app->can = bbx_canvas(bbx, root, 515, 150, bbx_color("DarkGreen"));
  app->dummy_can = 0;
  app->ncard_can = 0;
  app->scard_can = 0;
  app->ecard_can = 0;
  app->wcard_can = 0;
  app->current_pan = 0;
  bbx_canvas_setmousefunc(app->can, canvasfunc, app);


  app->score_pan = bbx_panel(bbx, root, "score", layoutscoring, app);
 // bbx_panel_setbackground(app->score_pan, bbx_color("white"));
 
  app->score_lab = bbx_label(bbx, app->score_pan, "Score");
  bbx_label_setfont(app->score_lab, &verabd_font);
  app->totNS_lab = bbx_label(bbx, app->score_pan, "NS: 0");
  app->totEW_lab = bbx_label(bbx, app->score_pan, "EW: 0");
  app->rubbers_lab = bbx_label(bbx, app->score_pan, "Rubbers");
  app->game1_lab = bbx_label(bbx, app->score_pan, "Game 1");
  app->game2_lab = bbx_label(bbx, app->score_pan, "Game 2");
  app->game3_lab = bbx_label(bbx, app->score_pan, "Game 3");
  app->over_lab[0] = bbx_label(bbx, app->score_pan, "0");
  app->under_lab[0] = bbx_label(bbx, app->score_pan, "0");
  app->over_lab[1] = bbx_label(bbx, app->score_pan, "0");
  app->under_lab[1] = bbx_label(bbx, app->score_pan, "0");
  app->over_lab[2] = bbx_label(bbx, app->score_pan, "0");
  app->under_lab[2] = bbx_label(bbx, app->score_pan, "0");

  app->current_pan = 0;
  app->contract_lab = 0;
  app->tricksEW_lab = 0;
  app->tricksNS_lab =0;
 
  app->selcard = -1;
  app->seldummy = -1;
  app->state = 0;
}

void layout(void *obj, int width, int height)
{
  APP *app =  obj;
  BABYX *bbx = app->bbx;

  bbx_setpos(bbx, app->menu, 0, 0, width, 20);
  bbx_setpos(bbx, app->but, (width-100)/2, height - 30, 100, 25);
 
  bbx_setpos(bbx, app->can, 100, height-200, 515, 150);
 
  bbx_setpos(bbx, app->score_pan, width - 200, 0, 200, height); 
}

void layoutscoring(void *obj, int width, int height)
{
  APP *app = obj;
  BABYX *bbx = app->bbx;

  bbx_setpos(bbx, app->score_lab, width - 200, 20, 200, 25); 
  bbx_setpos(bbx, app->totNS_lab, width - 200, 50, 50, 25);
  bbx_setpos(bbx, app->totEW_lab, width - 100,  50, 50, 25);
  bbx_setpos(bbx, app->rubbers_lab, width -200, 100, 100, 25);
  bbx_setpos(bbx, app->game1_lab, width - 200, 150, 100, 25);
  bbx_setpos(bbx, app->over_lab[0], width-200, 175, 200, 25);
  bbx_setpos(bbx, app->under_lab[0], width-200, 200, 200, 25);
  bbx_setpos(bbx, app->game2_lab, width - 200, 250, 100, 25);
  bbx_setpos(bbx, app->over_lab[1], width-200, 275, 200, 25);
  bbx_setpos(bbx, app->under_lab[1], width-200, 300, 200, 25);
  bbx_setpos(bbx, app->game3_lab, width - 200, 350, 100, 25);
  bbx_setpos(bbx, app->over_lab[2], width-200, 375, 200, 25);
  bbx_setpos(bbx, app->under_lab[2], width-200, 400, 200, 25);

}

void killcontrols(APP *app)
{
  bbx_menubar_kill(app->menu);
  bbx_button_kill(app->but);
  bbx_radiobox_kill(app->suit_rad);
  bbx_radiobox_kill(app->level_rad);
  bbx_canvas_kill(app->can); 
  bbx_canvas_kill(app->dummy_can);
  bbx_canvas_kill(app->ncard_can);
  bbx_canvas_kill(app->scard_can);
  bbx_canvas_kill(app->ecard_can);
  bbx_canvas_kill(app->wcard_can);
}

static void menuchosen(void *obj, int id)
{
  APP *app = obj;
  BABYX *bbx = app->bbx;
  char *fname;
  GAME *game;

  switch(id)
  {
  case 1:
    fname = bbx_getopenfile(bbx, "*");
    if(fname)
    {
      game = loadgame(fname, 0);
      if(game)
      {
        killgame(app->game);
        app->game = game;
      }
    }
    break;
  case 2:
    fname = bbx_getsavefile(bbx, "*"); 
    if(fname)
      savegame(app->game, fname);
    break;
  case 3:
     killcontrols(app);
     stopbabyx(bbx);
     break;
  case 4:
    aboutdialog(app);
    break;
  case 5:
    helpdialog(app);
    break;
  }
   
}

static void clickbutton(void *obj)
{
  APP *app = obj;
  int i;
  GAME *game;
  unsigned char *rgba;
  int width, height;
  unsigned char *cardrgba;
  int cw, ch;

  game = app->game;
 
  if(app->state == 0)
  {
    app_startbidding(app);
    app_updateuserhand(app);
    app->state = 1;
  }
  else if(app->state == 1)
  {
    if(!bidding(app->game->gp))
    {
      BID bid;
      getcurrentcontract(app->game->gp, &bid, 0, 0, 0);
      sorthand(app->game->south, 13, bid.suit);
      getcardpos(app->userhand_xpos, app->userhand_ypos, app->game->south, 13);
      app_updateuserhand(app);
      app_endbidding(app);
      app_createplaywidgets(app);
      app_createdummy(app);
      app_createplaycanvas(app);
      
      app->state = 2;
    }
    else if(togo(app->game->gp) != SOUTH)
    {
      BID bid;
      CARD *hand = 0;
      while(bidding(app->game->gp) && togo(app->game->gp) != SOUTH)
      {
        switch(togo(app->game->gp))
        {
          case NORTH: hand = game->north; break;
          case EAST: hand = game->east; break;
          case WEST: hand = game->west; break;
        }
        bid_ai(app->game->gp, hand, &bid);
        executebid(app->game, &bid);
        app_addbid(app, &bid);
      }
    }
  }
  else if(app->state == 2)
  {
    CARD card;
    CARD *hand;
    if(handover(app->game->gp))
    {
      app->state = 0;
      app_endplay(app);
      deal(game);
      sorthand(game->south, 13, 0);
      getcardpos(app->userhand_xpos, app->userhand_ypos, app->game->south, 13);
      app_updatescoring(app);
      return;
    }
        
	switch (togo(app->game->gp))
	{
	case NORTH:
		hand = game->north;
		if (app->game->gp->dummy_id != NORTH)
			playcard_ai(app->game->gp, hand, &card);
		else
		{
			if (app->seldummy >= 0 && app->seldummy < app->game->Nnorth)
			{
				memcpy(&card, &app->game->gp->dummy[app->seldummy], sizeof(CARD));
				if (!cardlegal(game->gp, hand, &card))
				{
					bbx_messagebox(app->bbx, BBX_MB_OK, "error", "Please follow suit");
					return;
				}
			}
			else
			{
				bbx_messagebox(app->bbx, BBX_MB_OK, "error", "Please select a card to play");
				return;
			}

        for(i=0;i<game->gp->Ndummy;i++)
			if(!memcmp(&game->gp->dummy[i], &card, sizeof(CARD)))
                 break;

             memmove(&app->dummyhand_xpos[i], &app->dummyhand_xpos[i+1], (13-i-1) * sizeof(int));
             memmove(&app->dummyhand_ypos[i], &app->dummyhand_ypos[i+1], (13-i-1) * sizeof(int));
             app->seldummy = -1;
      
        } 
        break;
      case SOUTH: 
        hand = game->south;
	if(app->game->gp->dummy_id == SOUTH)
          playcard_ai(app->game->gp, hand, &card);
	else if (app->selcard >= 0 && app->selcard < app->game->Nsouth)
	{
		memcpy(&card, &hand[app->selcard], sizeof(CARD));
		if (!cardlegal(game->gp, hand, &card))
		{
			bbx_messagebox(app->bbx, BBX_MB_OK, "error", "Please follow suit");
			return;
		}
	}
        else
	{
          bbx_messagebox(app->bbx, BBX_MB_OK, "error", "Please select a card to play");   
          return;
        }
        for(i=0;i<game->Nsouth;i++)
          if(!memcmp(&game->south[i], &card, sizeof(CARD)))
	     break;
        app->selcard = -1;

        memmove(&app->userhand_xpos[i], &app->userhand_xpos[i+1], (13-i-1) * sizeof(int));
        memmove(&app->userhand_ypos[i], &app->userhand_ypos[i+1], (13-i-1) * sizeof(int));
        break;
      case EAST: 
           hand = game->east;
           if(game->gp->dummy_id == EAST)
	   {
             playdummy_ai(game->gp, game->west, &card);
             for(i=0;i<game->gp->Ndummy;i++)
               if(!memcmp(&game->gp->dummy[i], &card, sizeof(CARD)))
                 break;

             memmove(&app->dummyhand_xpos[i], &app->dummyhand_xpos[i+1], (13-i-1) * sizeof(int));
             memmove(&app->dummyhand_ypos[i], &app->dummyhand_ypos[i+1], (13-i-1) * sizeof(int));
           }
           else 
             playcard_ai(app->game->gp, hand, &card); 
         break;
      case WEST: 
          hand = game->west;
          if(game->gp->dummy_id == WEST)
	  {
            playdummy_ai(app->game->gp, game->east, &card);
            for(i=0;i<game->gp->Ndummy;i++)
	      if(!memcmp(&game->gp->dummy[i], &card, sizeof(CARD)))
                 break;

             memmove(&app->dummyhand_xpos[i], &app->dummyhand_xpos[i+1], (13-i-1) * sizeof(int));
             memmove(&app->dummyhand_ypos[i], &app->dummyhand_ypos[i+1], (13-i-1) * sizeof(int));
          }
          else
            playcard_ai(app->game->gp, hand, &card); 
          break;
      }
    
    app_playcard(app, &card);
    executeplay(app->game, &card);
    if(app->game->gp->Nplayed == 1)
    {
       sorthand(app->game->gp->dummy, 13, app->game->gp->trumps);
       getcardpos(app->dummyhand_xpos, app->dummyhand_ypos, app->game->south, 13);
    }
       
    app_updatedummy(app); 
    app_updateuserhand(app);
    app_updatewidgets(app);
    if( (app->game->gp->Nplayed % 4) == 0)
      app->state = 3;
    if(app->state == 2 && 
       (togo(game->gp) != SOUTH && !(togo(game->gp) == NORTH && game->gp->dummy_id == NORTH)) )
      clickbutton(obj);
  }
  else if(app->state == 3)
  {
    app_gathertricks(app);
    app->state = 2;
  }

}

static void canvasfunc(void *obj, int action, int x, int y, int buttons)
{
  APP *app = obj;
  int index = -1;
  int i;
  int cw = 75;
  int ch = 108;

  if(action == BBX_MOUSE_CLICK && buttons & BBX_MOUSE_BUTTON1)
  {
    for(i=0;i<app->game->Nsouth;i++)
      if(x >= app->userhand_xpos[i] && x < app->userhand_xpos[i] + cw &&
         y >= app->userhand_ypos[i] && y < app->userhand_ypos[i] + ch)
            index = i;
         app->selcard = index;
    app_updateuserhand(app);
  }
}

static void dummycanvasfunc(void *obj, int action, int x, int y, int buttons)
{
  APP *app = obj;
  int index = -1;
  int i;
  int cw = 75;
  int ch = 108;

  if(action == BBX_MOUSE_CLICK && buttons & BBX_MOUSE_BUTTON1)
  {
    for(i=0;i<app->game->Nnorth;i++)
      if(x >= app->dummyhand_xpos[i] && x < app->dummyhand_xpos[i] + cw &&
         y >= app->dummyhand_ypos[i] && y < app->dummyhand_ypos[i] + ch)
            index = i;
         app->seldummy = index;
    app_updatedummy(app);
  }
 
}

static void bidpass(void *obj)
{
  APP *app = obj;
  BID bid;
  
  if(togo(app->game->gp) != SOUTH)
    return;

  bid.type = PASS;
  userbid(app, &bid);
}

static void bidbidding(void *obj)
{
  BID bid;
  APP *app = obj;
  
  if(togo(app->game->gp) != SOUTH)
    return;

  bid.type = BIDDING;
  bid.number = bbx_radiobox_getselected(app->level_rad);
  bid.suit = bbx_radiobox_getselected(app->suit_rad);

  bid.number += 1;
  switch(bid.suit)
  {
  case 0: bid.suit = CLUBS; break;
  case 1: bid.suit = DIAMONDS; break;
  case 2: bid.suit = HEARTS; break;
  case 3: bid.suit = SPADES; break;
  case 4: bid.suit = NOTRUMPS; break;

  }
  userbid(app, &bid);
 
}

static void biddouble(void *obj)
{
  BID bid;
  APP *app = obj;

  if(togo(app->game->gp) != SOUTH)
    return;

  bid.type = DOUBLE;
  userbid(app, &bid);
}

static void bidredouble(void *obj)
{
  BID bid;
  APP *app = obj;
  
  if(togo(app->game->gp) != SOUTH)
    return;
 
  bid.type = REDOUBLE;
  userbid(app, &bid);
}

static void userbid(APP *app, BID *bid)
{
  if(!bidlegal(app->game->gp, bid))
  {
    bbx_messagebox(app->bbx, BBX_MB_OK, "Sorry", "That bid is not allowed at  this stage");
    return;
  }
  executebid(app->game, bid);
  app_addbid(app, bid);
}

static void app_playcard(APP *app, CARD *card)
{
  unsigned char *blank;
  unsigned char *cardrgba;
  unsigned char *rotbuff;
  double theta;
  int cw, ch;

  if( (app->game->gp->Nplayed %4) == 0)
  {
    blank = malloc(50*72*4);
    bbx_rectangle(blank, 50, 72, 0, 0, 50, 72, bbx_color("DarkGreen"));
    bbx_canvas_setimage(app->ncard_can, blank, 50, 72);
    bbx_canvas_setimage(app->scard_can, blank, 50, 72);
    bbx_canvas_setimage(app->ecard_can, blank, 50, 72);
    bbx_canvas_setimage(app->wcard_can, blank, 50, 72);
    bbx_canvas_flush(app->ncard_can);
    bbx_canvas_flush(app->scard_can);
    bbx_canvas_flush(app->ecard_can);
    bbx_canvas_flush(app->wcard_can);
    free(blank);
    app->playtheta[0] = -0.1 + uniform() * 0.2;
    app->playtheta[1] = -0.1 + uniform() * 0.2;
    app->playtheta[2] = -0.1 + uniform() * 0.2;
    app->playtheta[3] = -0.1 + uniform() * 0.2;
  }
  cardrgba = card_rgba(card, &cw, &ch);
  rotbuff = malloc( (cw + 30) * (ch + 30) * 4);
  memset(rotbuff, 0x0, (cw+30) * (ch+30) * 4);
  theta = app->playtheta[app->game->gp->Nplayed %4];
  rgbapasterot(rotbuff, cw+30, ch+30, cardrgba, cw, ch, 15, 15, theta); 
  
  switch(togo(app->game->gp))
  {
  case NORTH:
    bbx_canvas_setimage(app->ncard_can, rotbuff, cw+30, ch+30);
    bbx_canvas_flush(app->ncard_can);
    break;
  case SOUTH:
    bbx_canvas_setimage(app->scard_can, rotbuff, cw+30, ch+30);
    bbx_canvas_flush(app->scard_can);
    break;
  case EAST:
     bbx_canvas_setimage(app->ecard_can, rotbuff, cw+30, ch+30);
     bbx_canvas_flush(app->ecard_can);
     break;
  case WEST:
     bbx_canvas_setimage(app->wcard_can, rotbuff, cw+30, ch+30);
     bbx_canvas_flush(app->wcard_can);
     break;
  }
  free(rotbuff);
}

static void app_gathertricks(APP *app)
{
    unsigned char *blank;

    blank = malloc(50*72*4);
    bbx_rectangle(blank, 50, 72, 0, 0, 50, 72, bbx_color("DarkGreen"));
    bbx_canvas_setimage(app->ncard_can, blank, 50, 72);
    bbx_canvas_setimage(app->scard_can, blank, 50, 72);
    bbx_canvas_setimage(app->ecard_can, blank, 50, 72);
    bbx_canvas_setimage(app->wcard_can, blank, 50, 72);
    bbx_canvas_flush(app->ncard_can);
    bbx_canvas_flush(app->scard_can);
    bbx_canvas_flush(app->ecard_can);
    bbx_canvas_flush(app->wcard_can);
    free(blank);
}

static void app_startbidding(APP *app)
{
  int player;
  int i;
  char *levelradio[7] = {"One", "Two", "Three", "Four", "Five", "Six", "Seven"};
  char *suitradio[5] = {"Clubs", "Diamonds", "Hearts", "Spades", "No Trumps"};
  BABYX *bbx;

  bbx = app->bbx;

  app->bidder_lab[0] = bbx_label(bbx, app->root, "North");
  app->bidder_lab[1] = bbx_label(bbx, app->root, "East");
  app->bidder_lab[2] = bbx_label(bbx, app->root, "South");
  app->bidder_lab[3] = bbx_label(bbx, app->root, "West");
  for(i=0;i<4;i++)
    bbx_label_setbackground(app->bidder_lab[i], bbx_color("LightGreen"));
  app->pass_but = bbx_button(bbx, app->root, "Pass", bidpass, app);
  app->bid_but = bbx_button(bbx, app->root, "Bid", bidbidding, app);
  app->double_but = bbx_button(bbx, app->root, "Double", biddouble, app);
  app->redouble_but = bbx_button(bbx, app->root, "Redouble", bidredouble, app);
  app->level_rad = bbx_radiobox(bbx, app->root, levelradio, 7, 0, 0);
  app->suit_rad = bbx_radiobox(bbx, app->root, suitradio, 5, 0, 0);

  bbx_setpos(bbx, app->bidder_lab[0], 10, 50, 90, 25);
  bbx_setpos(bbx, app->bidder_lab[1], 110, 50, 90, 25);
  bbx_setpos(bbx, app->bidder_lab[2], 210, 50, 90, 25);
  bbx_setpos(bbx, app->bidder_lab[3], 310, 50, 90, 25);
  bbx_setpos(bbx, app->level_rad, 625, 100, 150, 150);
  bbx_setpos(bbx, app->suit_rad, 625, 250, 150, 150);
  bbx_setpos(bbx, app->pass_but, 650, 410, 50, 25);
  bbx_setpos(bbx, app->bid_but, 700, 410, 50, 25);
  bbx_setpos(bbx, app->double_but, 650, 440, 100, 25);
  bbx_setpos(bbx, app->redouble_but, 650, 470, 100, 25);


  player = togo(app->game->gp);
  for(i=0;i<4;i++)
  {
    switch(player)
    {
    case NORTH: bbx_label_settext(app->bidder_lab[i], "North"); break;
    case SOUTH: bbx_label_settext(app->bidder_lab[i], "South"); break;
    case EAST: bbx_label_settext(app->bidder_lab[i], "East"); break;
    case WEST: bbx_label_settext(app->bidder_lab[i], "West"); break;
    }
    player = nextplayer(player, 1); 
  }
}

static void app_endbidding(APP *app)
{
  int i;
  for(i=0;i<4;i++)
  {
    bbx_label_kill(app->bidder_lab[i]);
    app->bidder_lab[i] = 0;
  }
  for(i=0;i<app->Nbids;i++)
    bbx_label_kill(app->bid_lab[i]);
  app->Nbids = 0;

  bbx_radiobox_kill(app->level_rad);
  app->level_rad = 0;
  bbx_radiobox_kill(app->suit_rad);
  app->suit_rad = 0;
  bbx_button_kill(app->pass_but);
  app->pass_but = 0;
  bbx_button_kill(app->bid_but);
  app->bid_but = 0;
  bbx_button_kill(app->double_but);
  app->double_but = 0;
  bbx_button_kill(app->redouble_but);
  app->redouble_but = 0;

}

static void app_endplay(APP *app)
{
  bbx_canvas_kill(app->ncard_can);
  app->ncard_can = 0;
  bbx_canvas_kill(app->scard_can);
  app->scard_can = 0;
  bbx_canvas_kill(app->ecard_can);
  app->ecard_can = 0;
  bbx_canvas_kill(app->wcard_can);
  app->wcard_can = 0;

  bbx_canvas_kill(app->dummy_can);
  app->dummy_can = 0;

  bbx_label_kill(app->contract_lab);
  app->contract_lab = 0;
  bbx_label_kill(app->tricksNS_lab);
  app->tricksNS_lab = 0;
  bbx_label_kill(app->tricksEW_lab);
  app->tricksEW_lab = 0;
  bbx_panel_kill(app->current_pan);
  app->current_pan = 0;
  

}
static void app_createplaywidgets(APP *app)
{
  BABYX *bbx = app->bbx;
  char contract[64];
  BID bid;
  int by;
  int doubled;
  int redoubled;
  char *bystr = 0;
  char *suitstr = 0;
  char *doublestr = "";
  char *pluralstr;

  getcurrentcontract(app->game->gp, &bid, &by, &doubled, &redoubled);
  switch(bid.suit)
  {
  case HEARTS: suitstr = "heart"; break;
  case CLUBS: suitstr = "club"; break;
  case DIAMONDS: suitstr = "diamond"; break;
  case SPADES: suitstr = "spade"; break;
  case NOTRUMPS: suitstr = "no trump"; break;
  }
  if(bid.suit == NOTRUMPS || bid.number > 1)
    pluralstr = "s";
  else
    pluralstr = "";
  switch(by)
  {
  case NORTH: bystr = "North"; break;
  case SOUTH: bystr = "South"; break;
  case EAST: bystr = "East"; break;
  case WEST: bystr = "West"; break;
  } 
  if(doubled)
    doublestr = " (doubled)";
  if(redoubled)
    doublestr = " (redoubled)";
  sprintf(contract, "%d %s%s by %s%s", bid.number, suitstr, pluralstr, bystr, doublestr);
  
  app->current_pan = bbx_panel(bbx, app->root, "current", layoutcurrent, app);
  app->contract_lab = bbx_label(bbx, app->current_pan, contract); 
  app->tricksNS_lab = bbx_label(bbx, app->current_pan, "North-South 0");
  app->tricksEW_lab = bbx_label(bbx, app->current_pan, "East-West  0");
  bbx_label_setbackground(app->contract_lab, bbx_color("LightGreen"));
  bbx_label_setbackground(app->tricksNS_lab, bbx_color("LightGreen"));
  bbx_label_setbackground(app->tricksEW_lab, bbx_color("LightGreen"));

  bbx_panel_setbackground(app->current_pan, bbx_color("LightGreen"));
  switch(by)
  {
  case NORTH: bbx_setpos(bbx, app->current_pan, 555, 300, 220, 110); break; 
  case SOUTH: bbx_setpos(bbx, app->current_pan, 555, 300, 220, 110); break; 
  case EAST: bbx_setpos(bbx, app->current_pan, 200, 150, 220, 110); break; 
  case WEST: bbx_setpos(bbx, app->current_pan, 200, 150, 220, 110); break; 
  } 

}

static void layoutcurrent(void *obj, int width, int height)
{
  APP *app = obj;

  bbx_setpos(app->bbx, app->contract_lab, 10, 10, 200, 25);
  bbx_setpos(app->bbx, app->tricksNS_lab, 10, 40, 150, 25);
  bbx_setpos(app->bbx, app->tricksEW_lab, 10, 70, 150, 25);
}

static void app_createplaycanvas(APP *app)
{
  BABYX *bbx = app->bbx;

  app->ncard_can = bbx_canvas(bbx, app->root, 75+30, 108+30, bbx_color("DarkGreen"));
  app->scard_can = bbx_canvas(bbx, app->root, 75+30, 108+30, bbx_color("DarkGreen"));
  app->wcard_can = bbx_canvas(bbx, app->root, 75+30, 108+30, bbx_color("DarkGreen"));
  app->ecard_can = bbx_canvas(bbx, app->root, 75+30, 108+30, bbx_color("DarkGreen"));
 
  bbx_setpos(bbx, app->ncard_can, 350, 300-18, 75+30, 108+30);
  bbx_setpos(bbx, app->scard_can, 350, 450-18, 75+30, 108+30);
  bbx_setpos(bbx, app->wcard_can, 250, 375-18, 75+30, 108+30);
  bbx_setpos(bbx, app->ecard_can, 450, 375-18, 75+30, 108+30);
}

static void app_createdummy(APP *app)
{
  BABYX *bbx = app->bbx;

  switch(app->game->gp->dummy_id)
  {
  case NORTH:
    app->dummy_can = bbx_canvas(bbx, app->root, 515, 150, bbx_color("DarkGreen"));
    bbx_canvas_setmousefunc(app->dummy_can, dummycanvasfunc, app);
    bbx_setpos(bbx, app->dummy_can, 75, 100, 515, 150);
    break;
  case EAST:
    app->dummy_can = bbx_canvas(bbx, app->root, 150, 515, bbx_color("DarkGreen"));
    bbx_setpos(bbx, app->dummy_can, 650, 100, 150, 515);
    break;
  case WEST:
    app->dummy_can = bbx_canvas(bbx, app->root, 150, 515, bbx_color("DarkGreen"));
    bbx_setpos(bbx, app->dummy_can, 20, 100, 150, 515);
    break;
  }
}

static void app_updatedummy(APP *app)
{
  CARD *hand;
  int Nhand;
  int width, height;
  unsigned char *rgba;
  unsigned char *cardrgba;
  unsigned char *bluergba;
  int cw, ch;
  unsigned char *rot;
  int i, ii;

  if(app->game->gp->dummy_id == SOUTH)
    return;

  hand = app->game->gp->dummy;
  Nhand = app->game->gp->Ndummy;

  rgba = bbx_canvas_rgba(app->dummy_can, &width, &height);
  bbx_rectangle(rgba, width, height, 0, 0, width, height, bbx_color("DarkGreen"));
  switch(app->game->gp->dummy_id)
  {
    case NORTH:
      for(i=0;i<Nhand;i++)
      {
        cardrgba = card_rgba(&hand[i], &cw, &ch);
        rgbapaste(rgba, width, height, cardrgba, cw, ch, app->dummyhand_xpos[i], app->dummyhand_ypos[i]);
        if(i == app->seldummy)
	{
          bluergba = malloc(75*108*4);
          for(ii=0;ii<75*108;ii++)
          {
            bluergba[ii*4] = 0;
            bluergba[ii*4+1] = 0;
            bluergba[ii*4+2] = 255;
            bluergba[ii*4+3] = 0x80;
          }
          rgbapaste(rgba, width, height, bluergba, 75, 108, app->dummyhand_xpos[i], app->dummyhand_ypos[i]);
          free(bluergba);
        }
      }
      break;
  case EAST:
      for(i=0;i<Nhand;i++)
      {
        cardrgba = card_rgba(&hand[i], &cw, &ch);
        rot = rgbarot90(cardrgba, cw, ch);
        rgbapaste(rgba, width, height, rot, ch, cw, 30-app->dummyhand_ypos[i], app->dummyhand_xpos[i]);
        free(rot);
      }
      break;
  case WEST:      
      for(i=0;i<Nhand;i++)
      {
        cardrgba = card_rgba(&hand[i], &cw, &ch);
        rot = rgbarot90(cardrgba, cw, ch);
        rgbapaste(rgba, width, height, rot, ch, cw, app->dummyhand_ypos[i], height - cw - app->dummyhand_xpos[i]);
        free(rot);
      }
      break;

  }
  bbx_canvas_flush(app->dummy_can);
}

static void app_updateuserhand(APP *app)
{
  CARD *hand;
  int Nhand;
  unsigned char *rgba;
  int width, height;
  unsigned char *cardrgba;
  unsigned char *bluergba;
  int cw, ch;
  int i;

  hand = app->game->south;
  Nhand = app->game->Nsouth;

  bluergba = malloc(75*108*4);
  for(i=0;i<75*108;i++)
  {
    bluergba[i*4] = 0;
    bluergba[i*4+1] = 0;
    bluergba[i*4+2] = 255;
    bluergba[i*4+3] = 0x80;
  }

  rgba = bbx_canvas_rgba(app->can, &width, &height);
  bbx_rectangle(rgba, width, height, 0, 0, width, height, bbx_color("DarkGreen"));
  for(i=0;i<Nhand;i++)
  {
    cardrgba = card_rgba(&hand[i], &cw, &ch);
    rgbapaste(rgba, width, height, cardrgba, cw, ch, app->userhand_xpos[i], 
      app->userhand_ypos[i]);
    if(i== app->selcard)
      rgbapaste(rgba, width, height, bluergba, 75, 108, app->userhand_xpos[i],
        app->userhand_ypos[i]);
  }
  bbx_canvas_flush(app->can);
  free(bluergba);
}

static void app_updatewidgets(APP *app)
{
  char buff[64];

  sprintf(buff, "North-South %d", app->game->gp->tricksNS);
  bbx_label_settext(app->tricksNS_lab, buff);
  sprintf(buff, "East-West %d", app->game->gp->tricksEW);
  bbx_label_settext(app->tricksEW_lab, buff);
}

static void app_updatescoring(APP *app)
{
  char buff[256];
  GAMEPUBLIC *gp;
  int i;

  gp = app->game->gp;
  sprintf(buff, "NS: %d", gp->historicNS);
  bbx_label_settext(app->totNS_lab, buff);
  sprintf(buff, "EW: %d", gp->historicEW);
  bbx_label_settext(app->totEW_lab, buff);
  sprintf(buff, "Rubbers %d %d", gp->rubbersNS, gp->rubbersEW);
  bbx_label_settext(app->rubbers_lab, buff);
  for(i=0;i<3;i++)
  {
    sprintf(buff, "% 6d % 6d", gp->overNS[i], gp->overEW[i]);
    bbx_label_settext(app->over_lab[i], buff);
    sprintf(buff, "% 6d % 6d", gp->underNS[i], gp->underEW[i]);
    bbx_label_settext(app->under_lab[i], buff);
  } 
}

static void app_addbid(APP *app, BID *bid)
{
  int row;
  int col;
  char str[64];

  bidtostr(str, bid); 
  row = app->Nbids / 4;
  col = app->Nbids % 4;

  app->bid_lab = realloc(app->bid_lab, (app->Nbids + 1) * sizeof(BBX_Label *));
  app->bid_lab[app->Nbids] = bbx_label(app->bbx, app->root, str);
  bbx_setpos(app->bbx, app->bid_lab[app->Nbids], col *100 + 10, row * 25 + 100, 100, 25);
  app->Nbids++;  
}

static void bidtostr(char *str, BID *bid)
{
  switch(bid->type)
  {
  case PASS: sprintf(str, "Pass"); break;
  case BIDDING: 
    switch(bid->suit)
    {
    case CLUBS: 
       sprintf(str, "%d Club%s", bid->number, bid->number > 1 ? "s": ""); 
       break;
    case DIAMONDS:
      sprintf(str, "%d Diamond%s", bid->number, bid->number > 1 ? "s": ""); 
      break;
    case HEARTS:
       sprintf(str, "%d Heart%s", bid->number, bid->number > 1 ? "s": ""); 
       break;
    case SPADES:
       sprintf(str, "%d Spade%s", bid->number, bid->number > 1 ? "s": ""); 
       break;
    case NOTRUMPS:
       sprintf(str, "%d No Trumps", bid->number);
       break; 
    }
    break;
  case DOUBLE: sprintf(str, "Double"); break;
  case REDOUBLE: sprintf(str, "Redouble"); break;
  }
}

static void getcardpos(int *xpos, int *ypos, CARD *hand, int N)
{
  int i, ii;

  for(i=0;i<N;i++)
    xpos[i] = 10 + (i * 30);
  for(i=1;i<N;i++)
    if(hand[i].suit != hand[i-1].suit)
      for(ii=i;ii<N;ii++)
        xpos[ii] += 10;
  for(i=0;i<N;i++)
    ypos[i] = abs(i-N/2)*3 + 2;
  for(i=7;i<N;i++)
    if(hand[i].suit != hand[i-1].suit)
      for(ii=i;ii<N;ii++)
         ypos[ii] += 2;
  for(i=5;i>=0;i--)
    if(hand[i].suit != hand[i+1].suit)
      for(ii=i;ii>=0;ii--)
	ypos[ii] += 2;
}

int suitorder(int suit, int trumps)
{
  if(suit == trumps)
    return 0;
  else if(trumps == HEARTS)
  {
    if(suit == CLUBS) return 1;
    if(suit == DIAMONDS) return 2;
    if(suit == SPADES) return 3;
  }
  else if(trumps == CLUBS)
  {
    if(suit == HEARTS) return 1;
    if(suit == SPADES) return 2;
    if(suit == DIAMONDS) return 3; 
  }
  else if(trumps == DIAMONDS)
  {
    if(suit == CLUBS) return 1;
    if(suit == HEARTS) return 2;
    if(suit == SPADES) return 3; 
  }
  else if(trumps == SPADES)
  {
    if(suit == HEARTS) return 1;
    if(suit == CLUBS) return 2;
    if(suit == DIAMONDS) return 3; 
  }
  else if(trumps == NOTRUMPS)
  {
    if(suit == CLUBS) return 0;
    if(suit == HEARTS) return 1;
    if(suit == DIAMONDS) return 2;
    if(suit == SPADES) return 3;
  }
  return -1;
}

static int compfunc(void *ptr, const void *e1, const void *e2)
{
  int trumps = *(int *)ptr;
  const CARD *c1 = e1;
  const CARD *c2 = e2;

  if(c1->suit != c2->suit)
    return suitorder(c1->suit, trumps) - suitorder(c2->suit, trumps);
  else return acehigh(c2->rank) - acehigh(c1->rank);  
    
} 

static void sorthand(CARD *hand, int N, int trumps)
{
  switch(trumps)
  {
  case 0: sortbysuit(hand, N); break;
  case CLUBS:
  case DIAMONDS:
  case HEARTS:
  case SPADES:
  case NOTRUMPS:
    qsortx(hand, N, sizeof(CARD), compfunc, &trumps);
    break;
  }
}

typedef struct
{
  BABYX *bbx;
  APP *app;
  BBX_DialogPanel *pan;
  BBX_Label *lab;
  BBX_Button *ok_but;
} AboutDialog;

static void layoutaboutdialog(void *obj, int width, int height);
static void aboutdlg_kill(AboutDialog *dlg);
static void aboutdlg_ok(void *obj);

static void aboutdialog(APP *app)
{
  AboutDialog dlg;
  BABYX *bbx = app->bbx;

  dlg.app = app;
  dlg.bbx = app->bbx;
  dlg.pan = bbx_dialogpanel(bbx, "About Baby Bridge", 300, 200, layoutaboutdialog, &dlg); 
  dlg.lab = bbx_label(bbx, dlg.pan, "Baby Bridge v1.0\n\nA contract bridge game\n\nby Malcolm McLean");
  dlg.ok_but = bbx_button(bbx, dlg.pan, "Ok", aboutdlg_ok, &dlg);
  bbx_dialogpanel_setclosefunc(dlg.pan, aboutdlg_ok, &dlg);
  bbx_dialogpanel_makemodal(dlg.pan);
  aboutdlg_kill(&dlg);  
}

static void layoutaboutdialog(void *obj, int width, int height)
{
  AboutDialog *dlg = obj;   
  bbx_setpos(dlg->bbx, dlg->lab, 10, 20, width-20, height-50); 
  bbx_setpos(dlg->bbx, dlg->ok_but, (width-100)/2, height-30, 100, 25);
}

static void aboutdlg_kill(AboutDialog *dlg)
{
  bbx_button_kill(dlg->ok_but);
  bbx_label_kill(dlg->lab);
  bbx_dialogpanel_kill(dlg->pan);
}

static void aboutdlg_ok(void *obj)
{
  AboutDialog *dlg = obj;

  bbx_dialogpanel_dropmodal(dlg->pan);
}


typedef struct
{
  BABYX *bbx;
  APP *app;
  BBX_DialogPanel *pan;
  BBX_Label *lab;
  BBX_Button *ok_but;
} HelpDialog;

static void layouthelpdialog(void *obj, int width, int height);
static void helpdlg_kill(AboutDialog *dlg);
static void helpdlg_ok(void *obj);

static void helpdialog(APP *app)
{
  AboutDialog dlg;
  BABYX *bbx = app->bbx;

  dlg.app = app;
  dlg.bbx = app->bbx;
  dlg.pan = bbx_dialogpanel(bbx, "Baby Bridge", 400, 250, layouthelpdialog, &dlg); 
  dlg.lab = bbx_label(bbx, dlg.pan, "");
  dlg.ok_but = bbx_button(bbx, dlg.pan, "Ok", helpdlg_ok, &dlg);
  bbx_dialogpanel_setclosefunc(dlg.pan, helpdlg_ok, &dlg);
  bbx_dialogpanel_makemodal(dlg.pan);
  helpdlg_kill(&dlg);  
}

static void layouthelpdialog(void *obj, int width, int height)
{
  AboutDialog *dlg = obj;   
  bbx_setpos(dlg->bbx, dlg->lab, 10, 20, width-20, height-50);
  bbx_label_settext(dlg->lab, 
 "Baby Bridge plays rubber bridge.\n\nThe player is always South.\n\nPress \"Click Me\" to advance the game.\n\nUses 2 clubs,Blackwood \nand Stayman conventions."); 
  bbx_setpos(dlg->bbx, dlg->ok_but, (width-100)/2, height-30, 100, 25);
}

static void helpdlg_kill(AboutDialog *dlg)
{
  bbx_button_kill(dlg->ok_but);
  bbx_label_kill(dlg->lab);
  bbx_dialogpanel_kill(dlg->pan);
}

static void helpdlg_ok(void *obj)
{
  AboutDialog *dlg = obj;

  bbx_dialogpanel_dropmodal(dlg->pan);
}
