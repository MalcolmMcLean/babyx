#ifndef bridgegame_h
#define bridgegame_h

#define NOTRUMPS 6

#define PASS 1
#define BIDDING 2
#define DOUBLE 3
#define REDOUBLE 4

#define NORTH 1
#define EAST 2
#define SOUTH 3
#define WEST 4

typedef struct
{
  int type;
  int suit;
  int number;
} BID;

typedef struct
{
  CARD card;
  int player;
} CARDPLAY;

/* GAMEPUBLIC contains all the publically available information about the
   game, eg, bidding, dummy, cards played */

typedef struct
{
  int historicNS;
  int historicEW;
  int rubbersNS;
  int rubbersEW;
  int gameindex;
  int overNS[3];
  int overEW[3];
  int underNS[3];
  int underEW[3];
  BID bidding[4 * 20];
  int Nbids;
  int trumps;
  int contract_level;
  int tricksEW;
  int tricksNS;
  CARD dummy[13];
  int Ndummy;
  CARDPLAY played[52];
  int Nplayed;
  int declarer;
  int dummy_id;  
  int lead;
  int dealer;
} GAMEPUBLIC;

/* GAME also contains the hands */ 
typedef struct
{
  GAMEPUBLIC *gp;
  CARD north[13];
  int Nnorth;
  CARD east[13];
  int Neast;
  CARD west[13];
  int Nwest;
  CARD south[13];
  int Nsouth;
} GAME;

GAMEPUBLIC *gamepublic(void);
void killgamepublic(GAMEPUBLIC *gp);
GAME *newgame(void);
GAME *loadgame(char *fname, int *err);
int savegame(GAME *game, char *fname);
void killgame(GAME *game);

void deal(GAME *game);
int nextplayer(int player, int index);
int suitrank(int suit);
int bidding(GAMEPUBLIC *gp);
int handover(GAMEPUBLIC *gp);
int togo(GAMEPUBLIC *gp);
int getcurrentcontract(GAMEPUBLIC *gp, BID *bid, int *by, int *doubled, int *redoubled);
int bidlegal(GAMEPUBLIC *gp, BID *bid);
int cardlegal(GAMEPUBLIC *gp, CARD *hand, CARD *card);
void gp_executebid(GAMEPUBLIC *gp, BID *bid);
int scorehand(GAMEPUBLIC *gp, int *under, int *over);
int acehigh(int rank);
int wintrick(CARDPLAY *trick, int trumps);
void gp_executecard(GAMEPUBLIC *gp, CARD *card);
int gp_startnewhand(GAMEPUBLIC *gp);


int executebid(GAME *game, BID *bid);
void executeplay(GAME *game, CARD *card);

#endif
