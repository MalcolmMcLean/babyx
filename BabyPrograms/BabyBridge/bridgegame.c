#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "shuffle.h"
#include "deck.h"

#include "bridgegame.h"

GAMEPUBLIC *gamepublic(void)
{
  GAMEPUBLIC *gp;
  int i;

  gp = malloc(sizeof(GAMEPUBLIC));
  if(!gp)
    return 0;
  for(i=0;i<3;i++)
  {
    gp->overNS[i] = 0;
    gp->overEW[i] = 0;
    gp->underNS[i] = 0;
    gp->underEW[i] = 0;
  }
  gp->gameindex = 0;
  gp->historicNS = 0;
  gp->historicEW = 0;
  gp->rubbersNS = 0;
  gp->rubbersEW = 0;
  gp->Nbids = 0;
  gp->trumps = NOTRUMPS;
  gp->contract_level = 0;
  gp->tricksNS = 0;
  gp->tricksEW = 0;
  gp->Ndummy = 0;
  gp->Nplayed = 0;
  gp->declarer = 0;
  gp->lead = 0;
  gp->dummy_id = 0;
  gp->dealer = NORTH;
 
  return gp;
}

void killgamepublic(GAMEPUBLIC *gp)
{
  if(gp)
    free(gp);
}

GAME *newgame(void)
{
  GAME *game;

  game = malloc(sizeof(GAME));
  game->gp = gamepublic();
  deal(game);

  return game;
}

GAME *loadgame(char *fname, int *err)
{
  GAME *answer;
  GAMEPUBLIC *gp;
  FILE *fp;

  answer = malloc(sizeof(GAME));
  gp = malloc(sizeof(GAMEPUBLIC));
  if(!answer || !gp)
    goto out_of_memory;
  
  fp = fopen(fname, "rb");
  if(!fp)
  {
    if(err)
      *err = -2;
    free(answer);
    free(gp);
    return 0;
  }
 
  if( fread(answer, sizeof(GAME), 1, fp) != 1)
    goto parse_error;
  if( fread(gp, sizeof(GAMEPUBLIC), 1, fp) != 1)
    goto parse_error;
  answer->gp  = gp;
  fclose(fp);
  return answer;

 parse_error:
  free(answer);
  free(gp);
  if(err)
    *err = - 3;
  return 0;
 out_of_memory:
  free(answer);
  free(gp);
  if(err)
    *err = -1;
  return 0;
}

int savegame(GAME *game, char *fname)
{
  FILE *fp;
  GAMEPUBLIC *gp;
  int answer = 0;

  fp = fopen(fname, "wb");
  if(!fp)
    return -1;

  gp = game->gp;
  /* set to null to avoid writing a pointer to disk */
  game->gp = 0;
  if( fwrite(game, sizeof(GAME), 1, fp) == EOF)
    answer = -1;
  game->gp = gp;
  if( fwrite(gp, sizeof(GAMEPUBLIC), 1, fp) == EOF)
    answer = -1;
  fclose(fp);
  return answer;
}

void killgame(GAME *game)
{
  if(game)
  {
    killgamepublic(game->gp);
    free(game);
  }
}

void deal(GAME *game)
{
  CARD *cards;

  cards = sorteddeck();
  shuffle(cards, 52, sizeof(CARD));
  memcpy(game->north, cards, 13 * sizeof(CARD));
  memcpy(game->east, cards+13, 13 * sizeof(CARD));
  memcpy(game->south, cards+26, 13 * sizeof(CARD));
  memcpy(game->west, cards+39, 13 *sizeof(CARD));
  game->Nnorth = 13;
  game->Nsouth = 13;
  game->Neast = 13;
  game->Nwest = 13;
  gp_startnewhand(game->gp);
  free(cards);
}

int delcard(CARD *hand, int N, CARD *card)
{
  int i;

  for(i=0;i<N;i++)
    if(!memcmp(&hand[i], card, sizeof(CARD)))
    {
      memmove(&hand[i], &hand[i+1], (N - i -1) * sizeof(CARD));
      return 0; 
    }
  return -1;
}

int nextplayer(int player, int index)
{
  int i;

  for(i=0;i<index;i++)
  {
    switch(player)
    {
    case NORTH: player = EAST; break;
    case EAST: player = SOUTH; break;
    case SOUTH: player = WEST; break;
    case WEST: player = NORTH; break;
    }
  }
  return player;
}


int suitrank(int suit)
{
  switch(suit)
  {
  case CLUBS: return 1;
  case DIAMONDS: return 2;
  case HEARTS: return 3;
  case SPADES: return 4;
  case NOTRUMPS: return 5;
  default: assert(0); return -1;
  }
}

int bidding(GAMEPUBLIC *gp)
{
  int N;

  if(gp->Nbids < 4)
    return 1;
  N = gp->Nbids;
  if(gp->bidding[N-1].type == PASS &&
     gp->bidding[N-2].type == PASS &&
     gp->bidding[N-3].type == PASS)
    return 0;
  return 1;
}

int handover(GAMEPUBLIC *gp)
{
  if(gp->Nplayed == 52)
    return 1;
  return 0;
}

int togo(GAMEPUBLIC *gp)
{
  if(bidding(gp))
    return nextplayer(gp->dealer, (gp->Nbids + 1) % 4); 
  else
    return nextplayer(gp->lead, gp->Nplayed % 4); 
}

int getcurrentcontract(GAMEPUBLIC *gp, BID *bid, int *by, int *doubled, int *redoubled)
{
  int i;

  if(redoubled)
    *redoubled = 0;
  if(doubled)
    *doubled = 0;
  if(by)
    *by = 0;
  if(bid)
    bid->type = PASS;

  i = gp->Nbids;
  while(i--)
  {
    if(gp->bidding[i].type == REDOUBLE)
      if(redoubled)
        *redoubled = nextplayer(gp->dealer, i +1);
    if(gp->bidding[i].type == DOUBLE)
      if(doubled)
	*doubled = nextplayer(gp->dealer, i+1);
    if(gp->bidding[i].type == BIDDING)
    {
      if(by)
        *by = nextplayer(gp->dealer, i +1);
       if(bid)
         memcpy(bid, &gp->bidding[i], sizeof(BID));
       break;
    }
  }

  if(i < 0)
    return -1;
  return 0;
}

int bidlegal(GAMEPUBLIC *gp, BID *bid)
{
  BID current;
  int by;
  int doubled;
  int redoubled;

  if(!bidding(gp))
    return 0;

  getcurrentcontract(gp, &current, &by, &doubled, &redoubled);
  switch(bid->type)
  {
  case PASS: return 1;
  case BIDDING:
    if(bid->number < 1 || bid->number > 7)
      return 0;
    if(bid->suit != HEARTS &&
       bid->suit != CLUBS && 
       bid->suit != DIAMONDS && 
       bid->suit != SPADES &&
       bid->suit != NOTRUMPS)
      return 0;
    if(by == 0)
      return 1;
    if(bid->number < current.number)
      return 0;
    if(bid->number == current.number)
      if(suitrank(bid->suit) <= suitrank(current.suit))
	return 0;
    return 1; 
  case DOUBLE:
    if(by == 0)
      return 0;
    if(doubled != 0)
      return 0;
    if( by == NORTH || by == SOUTH )
      if(togo(gp) == NORTH || togo(gp) == SOUTH)
	return 0;
    if(by == EAST || by == WEST)
      if(togo(gp) == EAST || togo(gp) == WEST)
        return 0;
    return 1;
  case REDOUBLE:
    if(by == NORTH || by == SOUTH)
      if(togo(gp) == EAST || togo(gp) == WEST)
        return 0;
    if(by == EAST || by == WEST)
      if(togo(gp) == NORTH || togo(gp) == SOUTH)
        return 0; 
    if(doubled != 0 && redoubled == 0)
      return 1;
    return 0;
	 default:
		  return 0;
  }
}

int cardlegal(GAMEPUBLIC *gp, CARD *hand, CARD *card)
{
  int Nhand;
  int i;
  int suit;

  if(gp->Nplayed % 4)
  {
    Nhand = 13 - gp->Nplayed/4;
    suit = gp->played[(gp->Nplayed/4)*4].card.suit;
    if(card->suit == suit)
      return 1;
    for(i=0;i<Nhand;i++)
      if(hand[i].suit == suit)
        break;
    /* legal if out of suit */
    if(i == Nhand)
      return 1;
    return 0;
  }
  /* always legal if leading */
  return 1;
} 
static void getdeclarer(GAMEPUBLIC *gp)
{
  int i, j;

  i = gp->Nbids;
  while(i--)
    if(gp->bidding[i].type == BIDDING)
      break;
  for(j=0;j<=i;j++)
    if(gp->bidding[j].suit == gp->bidding[i].suit &&
       (i % 2) == (j %2) )
      break;
  gp->trumps = gp->bidding[i].suit;
  gp->contract_level = gp->bidding[i].number;
  gp->declarer = nextplayer(gp->dealer, (j +1) % 4);
  gp->lead = nextplayer(gp->declarer, 1);
  gp->dummy_id = nextplayer(gp->declarer, 2);    
}

int gp_vulnerable(GAMEPUBLIC *gp, int player)
{
  int i;

  if(player == NORTH || player == SOUTH)
    for(i=0;i<=gp->gameindex;i++)
      if(gp->underNS[i] >= 100)
        return 1;
  if(player == EAST || player == WEST)
    for(i=0;i<=gp->gameindex;i++)
      if(gp->underEW[i] >= 100)
        return 1;

  return 0;
}

void gp_executebid(GAMEPUBLIC *gp, BID *bid)
{
  memcpy(&gp->bidding[gp->Nbids], bid, sizeof(BID));
  gp->Nbids++; 
  if(bidding(gp) == 0)
  {
    getdeclarer(gp);
  }
}

int scorehand(GAMEPUBLIC *gp, int *under, int *over)
{
  BID contract;
  int by;
  int doubled;
  int redoubled;
  int vulnerable;
  int tricksmade;
  int underpoints = 0;
  int overpoints = 0;
  int partscore;

  vulnerable = 0;
  if(gp->Nplayed != 52)
  {
    *under = 0;
    *over = 0;
    return -1;
  }
  getcurrentcontract(gp, &contract, &by, &doubled, &redoubled);
  vulnerable = gp_vulnerable(gp, by);
  if(by == NORTH || by == SOUTH)
    tricksmade = gp->tricksNS - 6; 
  else
    tricksmade = gp->tricksEW - 6;
  if(tricksmade >= contract.number)
  {
    switch(contract.suit)
    {
    case CLUBS:
    case DIAMONDS:
      underpoints = contract.number * 20; 
      break;
    case HEARTS:
    case SPADES:
      underpoints = contract.number * 30;
      break;
    case NOTRUMPS:
      underpoints = 40 + (contract.number-1)*30;
      break;
    }
    if(doubled)
      underpoints *= 2;
    if(redoubled)
      underpoints *= 2;
    if(tricksmade > contract.number)
    {
      if(!doubled & !redoubled)
      {
        if(contract.suit == CLUBS || contract.suit == DIAMONDS)
          overpoints = 20 * (tricksmade - contract.number);
        else
          overpoints = 30 * (tricksmade - contract.number);
      }
      else if(doubled && !redoubled)
      {
        if(vulnerable)
          overpoints = 200 * (tricksmade - contract.number);
        else
          overpoints = 100 * (tricksmade - contract.number);
      }
      else if(redoubled) 
      {
        if(vulnerable)
          overpoints = 400 * (tricksmade - contract.number);
        else
          overpoints = 200 * (tricksmade - contract.number);
      }
    }
    if(doubled)
      overpoints += 50;
    if(redoubled)
      overpoints += 50;
    if(contract.number == 6)
    {
      if(vulnerable)
        overpoints += 750;
      else
        overpoints += 500;
    }
    if(contract.number == 7)
    {
      if(vulnerable)
	overpoints += 1500;
      else
        overpoints += 1000;
    }
  }
  else
  {
    if(vulnerable)
    {
      if(!doubled && !redoubled)
        overpoints = 100 * (contract.number - tricksmade);
      else if(doubled && !redoubled)
	overpoints = 200 + (contract.number - tricksmade -1) * 300;
      else if(redoubled)
        overpoints = 400 + (contract.number - tricksmade -1) * 600; 
    } 
    else
    {
      if(!doubled && !redoubled)
        overpoints = 50 * (contract.number - tricksmade);
      else if(doubled && !redoubled)
      {
        if(contract.number - tricksmade > 3)
          overpoints = 100 + 200*2 + (contract.number-tricksmade -3) * 300;
        else
	  overpoints = 100 + (contract.number - tricksmade -1) * 200;
      }
      else if(redoubled)
      {
         if(contract.number - tricksmade > 3)
          overpoints = 200 + 400*2 + (contract.number-tricksmade-3) * 600;
        else
	  overpoints = 200 + (contract.number - tricksmade -1) * 600; 
      }
    }
    overpoints = -overpoints;
  }
  *under = underpoints;
  *over = overpoints;

  if(by == NORTH || by == SOUTH)
    partscore = gp->underNS[gp->gameindex];
  else
    partscore = gp->underEW[gp->gameindex];
  if(!vulnerable && underpoints + partscore >= 100)
    return 1; 
  if(vulnerable && underpoints + partscore >= 100)
  {
    if(gp->gameindex == 1)
      *over += 700;
    else
      *over += 500;
    return 2;
  }
  return 0; 
}

int acehigh(int rank)
{
  if(rank == 1)
    return 14;
  else
    return rank;
}

int wintrick(CARDPLAY *trick, int trumps)
{
  int i;
  int best = 0;
  for(i=1;i<4;i++)
  {
    if(trick[i].card.suit == trick[best].card.suit)
    {
      if(acehigh(trick[i].card.rank) > acehigh(trick[best].card.rank))
        best = i;
    }
    else if(trick[i].card.suit == trumps)
      best = i;   
  }
  return trick[best].player;
}

void gp_executecard(GAMEPUBLIC *gp, CARD *card)
{
  int winner;
  memcpy(&gp->played[gp->Nplayed].card, card, sizeof(CARD));
  gp->played[gp->Nplayed].player = togo(gp);
  if(togo(gp) == gp->dummy_id)
  {
    delcard(gp->dummy, gp->Ndummy, card);
    gp->Ndummy--; 
  }
  gp->Nplayed++;
  if(gp->Nplayed == 1)
  {
    gp->Ndummy = 13;
  }
  if(gp->Nplayed % 4 == 0)
  {
    winner = wintrick(&gp->played[gp->Nplayed-4], gp->trumps);
    if(winner == NORTH || winner == SOUTH)
      gp->tricksNS++;
    else
      gp->tricksEW++;
    gp->lead = winner;   
  }
}

int gp_startnewhand(GAMEPUBLIC *gp)
{
  int under, over;
  int res;
  int by;
  BID contract;
  int i;

  if(gp->Nplayed == 52)
  {
    getcurrentcontract(gp, &contract, &by, 0, 0);
    res = scorehand(gp, &under, &over);
    if(by == NORTH || by == SOUTH)
    {
      if(over > 0)
        gp->overNS[gp->gameindex] += over;
      else
        gp->overEW[gp->gameindex] -= over;
      gp->underNS[gp->gameindex] += under;
    } 
    else if(by == EAST || by == WEST)
    {
      if(over > 0)
        gp->overEW[gp->gameindex] += over;
      else
        gp->overNS[gp->gameindex] -= over;
      gp->underEW[gp->gameindex] += under;
    } 
    if(res == 1)
      if(gp->gameindex < 2)
         gp->gameindex++;
    if(res == 2)
    {
      for(i=0;i<=gp->gameindex;i++)
      {
        gp->historicNS += gp->underNS[i];
        gp->historicNS += gp->overNS[i];
        gp->historicEW += gp->underEW[i];
        gp->historicEW += gp->overEW[i];
      }
      for(i=0;i<3;i++)
      {
        gp->underNS[i] = 0;
        gp->underEW[i] = 0;
        gp->overNS[i] = 0;
        gp->overEW[i] = 0;
      }
      gp->gameindex = 0;
    }
  
    gp->dealer = nextplayer(gp->dealer, 1);

    
  }
  gp->Nbids = 0;
  gp->Nplayed = 0;
  gp->Ndummy = 0;
  gp->tricksNS = 0;
  gp->tricksEW = 0;

  return 0;
}



int executebid(GAME *game, BID *bid)
{
  gp_executebid(game->gp, bid);
  if(bidding(game->gp))
    return 0;
  return 1;
}

void executeplay(GAME *game, CARD *card)
{
  switch( togo(game->gp) )
  {
  case NORTH:
    delcard(game->north, game->Nnorth, card);
    game->Nnorth--;
    break;
  case SOUTH:
    delcard(game->south, game->Nsouth, card);
    game->Nsouth--;
    break;
  case EAST:
    delcard(game->east, game->Neast, card);
    game->Neast--;
    break;
  case WEST:
    delcard(game->west, game->Nwest, card);
    game->Nwest--;
    break;
  }
  gp_executecard(game->gp, card);
  if(game->gp->Nplayed == 1)
  {
    switch(game->gp->dummy_id)
    {
    case NORTH: memcpy(game->gp->dummy, game->north, 13 * sizeof(CARD));
      break;
    case SOUTH: memcpy(game->gp->dummy, game->south, 13 * sizeof(CARD));
      break;
    case EAST: memcpy(game->gp->dummy, game->east, 13 * sizeof(CARD));
      break;
    case WEST: memcpy(game->gp->dummy, game->west, 13 * sizeof(CARD));
      break;
    }
  }
}
