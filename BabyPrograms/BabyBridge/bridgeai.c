#include <stdlib.h>
#include <string.h>

#include "deck.h"
#include "bridgegame.h"


static int Nofsuit(CARD *hand, int N, int suit)
{
  int answer = 0;
  int i;
 
  for(i=0;i<N;i++)
    if(hand[i].suit == suit)
      answer++;
  return answer;
}

static int highestofsuit(CARD *hand, int N, int suit)
{
  int answer = 0;
  int i;
  
  for(i=0;i<N;i++)
    if(hand[i].suit == suit && (answer == 0 || 
				acehigh(hand[i].rank) > acehigh(answer)))
       answer = hand[i].rank;
  return answer;
} 

static int lowestofsuit(CARD *hand, int N, int suit)
{
  int answer = 0;
  int i;
  
  for(i=0;i<N;i++)
    if(hand[i].suit == suit && (answer == 0 || 
				acehigh(hand[i].rank) < acehigh(answer)))
       answer = hand[i].rank;
  return answer;
} 

static int points(CARD *hand)
{
  int i;
  int answer = 0;
  int suits[4] = {HEARTS, CLUBS, DIAMONDS, SPADES};
  int N;

  for(i=0;i<4;i++)
  {
    N = Nofsuit(hand, 13, suits[i]);
    if(N == 0)
      answer += 3;
    else if(N == 1)
      answer += 2;
    else if(N == 2)
      answer += 1;
  }

  for(i=0;i<13;i++)
  {
    if(hand[i].rank == 1)
      answer += 4;
    else if(hand[i].rank == 13 && Nofsuit(hand, 13, hand[i].suit) > 1)
      answer += 3;
    else if(hand[i].rank == 12 && Nofsuit(hand, 13, hand[i].suit) > 2)
      answer += 2;
    else if(hand[i].rank == 11 && Nofsuit(hand, 13, hand[i].suit) > 2)
      answer += 1;
  }

  return answer;    
}

/* hand with 4-3-3-3 pattern */
int balancedhand(CARD *hand)
{
  int suits[4] = {HEARTS, CLUBS, DIAMONDS, SPADES};
  int i;
  int N;

  for(i=0;i<4;i++)
  {
    N = Nofsuit(hand, 13, suits[i]);
    if(N != 3 && N != 4)
      return 0;
  }

  return 1;
}

static int compints(const void *e1, const void *e2)
{
  return *(const int *) e2 - *(const int *) e1; 
}
/*
  hands with pattern
 4-4-4-1, 5-4-2-2, 5-4-3-1, 6-3-2-2 and 6-3-3-1
*/
int tamehand(CARD *hand)
{
  int N[4];
  int suits[4] = {HEARTS, CLUBS, DIAMONDS, SPADES};
  static int patterns[5][4] = 
    {
      {4, 4, 4, 1},
      {5, 4, 2, 2},
      {5, 4, 3, 1},
      {6, 3, 2, 2},
      {6, 3, 3, 1},
    };
  int i;

  for(i=0;i<4;i++)
    N[i] = Nofsuit(hand, 13, suits[i]);
  qsort(N, 4, sizeof(int), compints);
  for(i=0;i<5;i++)
    if(!memcmp(&patterns[i][0], N, 4 * sizeof(int)))
      return 1;
  return 0;
}


int responding(GAMEPUBLIC *gp)
{
  if(gp->Nbids < 2)
    return 0;
  if(gp->bidding[gp->Nbids-2].type == BIDDING)
    return 1;
  return 0;
}

int rebidding(GAMEPUBLIC *gp)
{
  if(responding(gp) && 
     gp->Nbids >= 4 &&
     gp->Nbids <= 7 && 
     gp->bidding[gp->Nbids-4].type != PASS &&
     (gp->Nbids < 6 || gp->bidding[gp->Nbids-6].type == PASS) )
    return 1;
  return 0;
}


int strongestsuit(CARD *hand)
{
  int suits[4] = {HEARTS, CLUBS, DIAMONDS, SPADES};
  int N[4];
  int points[4] = {0};
  int besti;
  int i, ii;
  int best;

  for(i=0;i<4;i++)
    N[i] = Nofsuit(hand, 13, suits[i]);
  
  for(i=0;i<4;i++)
    for(ii=0;ii<13;ii++)
      if(hand[ii].suit == suits[i])
      {
        if(hand[ii].rank == 1)
          points[i] += 4;
        else if(hand[ii].rank == 13)
          points[i] += 3;
        else if(hand[ii].rank == 12)
          points[i] += 2;
        else if(hand[i].rank == 11)
          points[i] += 1;
      }

  besti = -1;
  best = 0;
  for(i=0;i<4;i++)
    if(N[i] > 3 && points[i] > best)
    {
      best = points[i];
      besti = i;
    }
  for(i=0;i<4;i++)
    if(N[i] > 4 && besti != i)
    {
      if(N[i] == 5 && points[i] > 2)
        besti = i;
      if(N[i] == 6 && points[i] >= 2)
        besti = i;
      if(N[i] >= 7)
        besti = i;
    }
  if(besti == -1)
    for(i=0;i<4;i++)
      if(best < points[i])
      {
        best = points[i];
        besti = i; 
      }

  return suits[besti];
}

int onenotrumphand(CARD *hand)
{
  int suits[4] = {HEARTS, CLUBS, DIAMONDS, SPADES};
  int N[4];
  int points[4] = {0};
  int i, ii;

  for(i=0;i<4;i++)
    N[i] = Nofsuit(hand, 13, suits[i]);
  for(i=0;i<4;i++)
    if(N[i] != 3 && N[i] != 4)
      return 0;
  for(i=0;i<4;i++)
    for(ii=0;ii<13;ii++)
      if(hand[ii].suit == suits[i])
      {
        if(hand[ii].rank == 1)
          points[i] += 4;
        else if(hand[ii].rank == 13)
          points[i] += 3;
        else if(hand[ii].rank == 12)
          points[i] += 2;
        else if(hand[i].rank == 11)
          points[i] += 1;
      }

  for(i=0;i<4;i++)
  {
    if(points[i] == 0 || (points[i] == 1 && N[i] == 3))
      return 0;
  }
  return 1;
}

int weakthreehand(CARD *hand)
{
  int Npoints;
  int suits[4] = {HEARTS, CLUBS, DIAMONDS, SPADES};
  int i;

  Npoints = points(hand);
  for(i=0; i<4;i++)
  {
    if(Nofsuit(hand, 13, suits[i]) >= 7 && Npoints < 13)
      return 1;
  } 

  return 0;
}

static void respond1ofasuit(GAMEPUBLIC *gp, CARD *hand, BID *bid)
{
  BID current;
  int Npoints;
  int suits[4] = {HEARTS, CLUBS, DIAMONDS, SPADES};
  int point[4] = {0};
  int i, ii;
  int bestsuit;
 
  getcurrentcontract(gp, &current, 0, 0, 0);
  Npoints = points(hand);
  bestsuit = strongestsuit(hand);

 
  for(i=0;i<4;i++)
    for(ii=0;ii<13;ii++)
      if(hand[ii].suit == suits[i])
      {
        if(hand[ii].rank == 1)
          point[i] += 4;
        else if(hand[ii].rank == 13)
          point[i] += 3;
        else if(hand[ii].rank == 12)
          point[i] += 2;
        else if(hand[i].rank == 11)
          point[i] += 1;
      }

  if(Npoints < 6)
  {
    bid->type = PASS;
    return;
  }
  if(Npoints < 10)
  {
    if(bestsuit == current.suit)
    {
      bid->type = BIDDING;
      bid->suit = bestsuit;
      bid->number = 2;
      return;
    }
    else if(suitrank(bestsuit) > suitrank(current.suit))
    {
      bid->type = BIDDING;
      bid->suit = bestsuit;
      bid->number = 1;
      return;
    }
    else if(Nofsuit(hand, 13, current.suit) >= 4)
    {
      bid->type = BIDDING;
      bid->suit = current.suit;
      bid->number = 2;
      return;
    } 
    else 
    {    
      bid->type = PASS;
      return;
    }
  }
  else if(Npoints > 13 && Nofsuit(hand, 13, current.suit) >= 4)
  {
    bid->type = BIDDING;
    bid->suit = NOTRUMPS;
    bid->number = 4;
  }
  else
  {
    bid->type = BIDDING;
    bid->suit = bestsuit;
    bid->number = 2;
  }   
}

static void respond1notrump(GAMEPUBLIC *gp, CARD *hand, BID *bid)
{
  int Npoints;
  int heartsok = 0;
  int spadesok = 0;
  Npoints = points(hand);
  if(Npoints < 6 && balancedhand(hand))
  {
    bid->type = PASS;
    return;
  }
  if(Nofsuit(hand, 13, HEARTS) >= 4 && highestofsuit(hand, 13, HEARTS) >= 12)
    heartsok = 1;
  if(Nofsuit(hand, 13, SPADES) >= 4 && highestofsuit(hand, 13, SPADES) >= 12)
    spadesok =1;
  if(heartsok && spadesok)
  {
    if(Nofsuit(hand, 13, HEARTS) < Nofsuit(hand, 13, SPADES))
      heartsok = 0;
   if(Nofsuit(hand, 13, SPADES) < Nofsuit(hand, 13, HEARTS))
     spadesok  = 0;
   if(heartsok && spadesok)
   {
     heartsok = 0;
   }
  }
  if(heartsok)
  {
    bid->type = BIDDING;
    bid->suit = HEARTS;
    bid->number = 2;
    return;
  }
  if(spadesok)
  {
    bid->type = BIDDING;
    bid->suit = SPADES;
    bid->number = 2;
    return;
  }
  else
  {
    bid->type = BIDDING;
    bid->suit = DIAMONDS;
    bid->number = 2;
    return;
  }
}

static void respond2clubs(GAMEPUBLIC *gp, CARD *hand, BID *bid)
{
  int Npoints;
  int strongest;

  Npoints = points(hand);
  if(Npoints <= 4)
  {
    bid->type = BIDDING;
    bid->number = 2;
    bid->suit = DIAMONDS;
    return;
  }
  if(Npoints > 4)
  {
    strongest = strongestsuit(hand);
    if( (strongest == CLUBS || strongest == DIAMONDS) && balancedhand(hand))
    {
      bid->type = BIDDING;
      bid->number = 2;
      bid->suit = NOTRUMPS;
      return;
    }
    if(Npoints < 7)
    {
      bid->type = BIDDING;
      bid->number = strongest == (CLUBS || strongest == DIAMONDS) ? 3 : 2;
      bid->suit = strongest;
      return;
    }
    else if(Npoints >= 7)
    {
      bid->type = BIDDING;
      bid->number = 3;
      bid->suit = strongest;
      return;
    }
  }
}

static void respondopposingbid(GAMEPUBLIC *gp, CARD *hand, BID *bid)
{
  int Npoints;
  BID current;
  int bestsuit;

  Npoints = points(hand);
  getcurrentcontract(gp, &current, 0, 0, 0);
  bestsuit = strongestsuit(hand);
  
  if(Npoints >= 10 && bestsuit != current.suit && current.number == 1)
  {
    if(Nofsuit(hand, 13, bestsuit) >= 5)
    {
      bid->type = BIDDING;
      bid->suit = bestsuit;
      bid->number = 2;
    }
  }

  bid->type = PASS;
}

static void respondblackwood(GAMEPUBLIC *gp, CARD *hand, BID *bid)
{
  BID current;
  int Naces = 0;
  int i;

  /* double if opponents stupid enough to interfere */
  getcurrentcontract(gp, &current, 0, 0, 0);
  if(current.suit != NOTRUMPS || current.number != 4)
  {
    bid->type = DOUBLE;
    return;
  }
  for(i=0;i<13;i++)
    if(hand[i].rank == 1)
      Naces++;
  switch(Naces)
  {
  case 0: bid->suit = CLUBS; bid->number = 5; break;
  case 1: bid->suit = DIAMONDS; bid->number = 5; break;
  case 2: bid->suit = HEARTS; bid->number = 5; break;
  case 3: bid->suit = SPADES; bid->number = 5; break;
  case 4: bid->suit = CLUBS; bid->number = 5; break;
  }
  bid->type = BIDDING;
  
}

static void slamafterblackwood(GAMEPUBLIC *gp, CARD *hand, BID *bid)
{
  BID current;
  int by;
  int doubled;
  int redoubled;
  int Npartneraces;
  int Naces = 0;
  int slamsuit = 0;
  int i;

  getcurrentcontract(gp, &current, &by, &doubled, &redoubled);
  /* double if opponents stupid enough to interfere */
  if(by != nextplayer(gp->dealer, gp->Nbids + 1 + 2))
  {
    if(!doubled)
      bid->type = DOUBLE;
    else
      bid->type = PASS;
    return;
  }
  /* partner didn't respond properly. Bale out */
  if(current.number != 5)
  {
    bid->type = PASS;
    return;
  }
  /* find the suit we've established */
  for(i=6; gp->Nbids-i >= 0; i-=2)
  {
    if(gp->bidding[i].type == BIDDING)
    {
      slamsuit = gp->bidding[i].suit;
      break;
    }
  }
  /* something's gone wrong. We've entered Blackwoord without a suit */ 
  if(slamsuit == 0)
  {
    slamsuit = strongestsuit(hand);
  }
  for(i=0;i<13;i++)
    if(hand[i].rank == 1)
      Naces++;
  switch(current.suit)
  {
  case CLUBS: if(Naces == 0) Npartneraces = 4; else Npartneraces = 0; break;
  case DIAMONDS: Npartneraces = 1; break;
  case HEARTS: Npartneraces = 2; break;
  case SPADES: Npartneraces = 3; break;
  }
  if(Naces + Npartneraces == 4)
  {
    bid->type = BIDDING;
    bid->number = 7;
    bid->suit = slamsuit;
    return;
  }
  if(Naces + Npartneraces == 3)
  {
    bid->type = BIDDING;
    bid->number = 6;
    bid->suit = slamsuit;
    return;
  }
  if(current.suit == slamsuit)
  {
    bid->type = PASS;
    return;
  } 
  if(suitrank(current.suit) < suitrank(slamsuit))
  {
    bid->type = BIDDING;
    bid->number = 5;
    bid->suit = slamsuit;
    return;
  }
  else
  {
    bid->type = BIDDING;
    bid->number = 6;
    bid->suit = slamsuit;
  }
}

static void rebid(GAMEPUBLIC *gp, CARD *hand, BID *bid)
{
  int Npoints;
  BID current;
  int by;
  int me;
  BID partnerbid;
  BID opener;
  int strongest;
  int i;
  int suits[4] = {HEARTS, CLUBS, DIAMONDS, SPADES};

  me = nextplayer(gp->dealer, gp->Nbids + 1);
  getcurrentcontract(gp, &current, &by, 0, 0);
  Npoints = points(hand);
  for(i=0;i<gp->Nbids;i++)
    if(gp->bidding[i].type == BIDDING && (i %4) == gp->Nbids%4)
    {
      memcpy(&opener, &gp->bidding[i], sizeof(BID));
      break;
    }
  memcpy(&partnerbid, &gp->bidding[gp->Nbids-2], sizeof(BID));
  if(by != nextplayer(me, 2))
  {
    bid->type = DOUBLE;
    return;
  }
  
  if(opener.number == 2 && opener.suit == CLUBS)
  {
    strongest = strongestsuit(hand);
    if(partnerbid.number == 2 && partnerbid.suit == DIAMONDS)
    {
      if(strongest == HEARTS || strongest == SPADES)
      {
        bid->type = BIDDING;
        bid->number = 2;
        bid->suit = strongest;
        return;
      }
      if(strongest == DIAMONDS)
      {
        bid->type = PASS;
        return;
      }
      if(strongest == CLUBS)
      {
        bid->type = BIDDING;
        bid->number = 3;
        bid->suit = CLUBS;
        return;
      }
    }
    if(strongest == partnerbid.suit)
    {
      if(partnerbid.number == 3 && strongest != CLUBS)
      {
        bid->type = BIDDING;
        bid->number = 4;
        bid->suit = NOTRUMPS;
        return;
      }
      else
      {
        if(Npoints >= 26)
	{
          bid->type = BIDDING;
          bid->number = 4;
          bid->suit = NOTRUMPS;
          return;
        }
        else
	{
          bid->type = BIDDING;
          bid->suit = strongest;
          bid->number = (strongest == HEARTS || strongest == SPADES) ? 4 : 5;
          return;
	}
      }
      if( balancedhand(hand) )
      {
        bid->type = BIDDING;
        bid->number = Npoints < 26 ? 2 : 3;
        if(bid->number < partnerbid.number)
          bid->number = partnerbid.number+1;
        bid->suit = NOTRUMPS;
        return;
      }
      if(strongest == HEARTS || strongest == SPADES)
      {
        bid->type = BIDDING;
        bid->number = Npoints < 26 ? 3 : 4;
        if(bid->number <= partnerbid.number && suitrank(strongest) < suitrank(partnerbid.suit))
	  bid->number++;
        bid->suit = strongest;
        return;
      }
      if(strongest == CLUBS || strongest == DIAMONDS)
      {
        bid->type = BIDDING;
        bid->number = Npoints < 26 ? 3 : 4;
        if(bid->number <= partnerbid.number && suitrank(strongest) < suitrank(partnerbid.suit))
	  bid->number++;
        bid->suit = strongest;
        return;
      }
    }
  } 

  if(opener.suit != partnerbid.suit)
  {
    if(partnerbid.suit == NOTRUMPS && partnerbid.number ==1)
    {
      if(Npoints <= 15)
      {
        if(balancedhand(hand) || tamehand(hand))
	{
           bid->type = PASS;
           return;
	}  
      }
      else if(Npoints <= 18)
      {
        if(tamehand(hand))
	{
          bid->type = BIDDING;
          bid->number = 2;
          bid->suit = NOTRUMPS;
          return;
	}
      }
      else if(Npoints >= 19)
      {
         if(balancedhand(hand) || tamehand(hand))
	 {
           bid->type = BIDDING;
           bid->number = 2;
           bid->suit = NOTRUMPS;
           return;
         }
      }
      bid->type = BIDDING;
      bid->number = 2;
      bid->suit = strongestsuit(hand);
      return;
    }
    if(partnerbid.suit != NOTRUMPS &&  Nofsuit(hand, 13, partnerbid.suit) >= 4)
    {
      if(Npoints <= 15)
      {
        if(partnerbid.number == 1)
	{
          bid->type = BIDDING;
          bid->suit = partnerbid.suit;
          bid->number = 2;
          return;
	}
      }
      else if(Npoints <= 18)
      {
        if(partnerbid.number <= 2)
	{
          bid->type = BIDDING;
          bid->suit = partnerbid.suit;
          bid->number = 3;
          return;
	}
      }
      else if(Npoints >= 19)
      {
        if(partnerbid.number <= 4)
        {
          bid->type = BIDDING;
          bid->suit = partnerbid.suit;
          bid->number = 4;
          return;
        }
      }
    } 
    if(partnerbid.number == 1 && partnerbid.suit != NOTRUMPS && Nofsuit(hand, 13, partnerbid.suit) < 4)
    {
      if(balancedhand(hand) && Npoints <= 20)
      {
        bid->type = BIDDING;
        bid->number = Npoints <= 15 ? 1 : 2;
        bid->suit = NOTRUMPS;
        return;
      }
    }
    if(partnerbid.suit != NOTRUMPS && Nofsuit(hand, 13, partnerbid.suit) < 4)
    {
      if(Nofsuit(hand, 13, opener.suit >= 6))
      {
        bid->type = BIDDING;
        bid->suit = opener.suit;
        bid->number = suitrank(bid->suit) < suitrank(partnerbid.suit) ? partnerbid.number : partnerbid.number + 1;
        if(Npoints > 15 && Npoints <= 18)
          if(bid->number < 3)
            bid->number = 3;
        if(Npoints >= 19)
          if(bid->number < 4)
            bid->number = 4;

        return;
      }
      for(i=0;i<4;i++)
        if(suits[i] != opener.suit && 
           suits[i] != partnerbid.suit && 
    Nofsuit(hand, 13, suits[i]) >= 4)
	  {
  bid->type = BIDDING;
  bid->suit = suits[i];
  bid->number = suitrank(suits[i]) > suitrank(partnerbid.suit) ? partnerbid.number : partnerbid.number + 1;
  if(Npoints >= 19)
    bid->number++;
  return;
        }
    }
     
  }
    
  bid->type = PASS;
}


int opening(GAMEPUBLIC *gp)
{
  int i;

  for(i=0;i<gp->Nbids;i++)
    if(gp->bidding[i].type != PASS)
      return 0;
  return 1;
}

int overbidding(GAMEPUBLIC *gp)
{
  if(gp->Nbids > 0 && gp->bidding[gp->Nbids-1].type == BIDDING)
    return 1;
  if(gp->Nbids > 2 && gp->bidding[gp->Nbids-3].type == BIDDING &&
     gp->bidding[gp->Nbids-2].type == PASS)
    return 1;
  return 0;
}

void chooseopeningbid(GAMEPUBLIC *gp, CARD *hand, BID *bid)
{
  int Npoints;

  Npoints = points(hand);
  if(weakthreehand(hand))
  {
    bid->type = BIDDING;
    bid->suit = strongestsuit(hand);
    bid->number = 3;
  }
  else if(Npoints < 12)
  {
    bid->type = PASS;
    return;
  }
  else if(onenotrumphand(hand))
  {
    bid->type = BIDDING;
    bid->suit = NOTRUMPS;
    bid->number = 1;
  }
  else if(balancedhand(hand) && (Npoints == 20 || Npoints == 21) )
  {
    bid->type = BIDDING;
    bid->suit = NOTRUMPS;
    bid->number = 2;
  }
  else if(Npoints >= 22)
  {
    bid->type = BIDDING;
    bid->suit = CLUBS;
    bid->number = 2;
  }
  else
  {
    bid->type = BIDDING;
    bid->suit = strongestsuit(hand);
    bid->number = 1;
  }
  
}

void bid_ai(GAMEPUBLIC *gp, CARD *hand, BID *bid)
{
  BID current;

  if(opening(gp))
  {
    chooseopeningbid(gp, hand, bid);
    return;
  }
  else if(responding(gp))
  {
    if(rebidding(gp))
    {
      rebid(gp, hand, bid);
      return;
    }
    getcurrentcontract(gp, &current, 0, 0, 0);
    if(current.number == 1 && current.suit != NOTRUMPS)
    {
      respond1ofasuit(gp, hand, bid);
      return;
    }
    if(current.number == 1 && current.suit == NOTRUMPS)
    {
      respond1notrump(gp, hand, bid);
      return;
    }
    if(current.number == 2 && current.suit == CLUBS)
    {
      respond2clubs(gp, hand, bid);
      return;
    }
    if(
       gp->Nbids >= 2 &&
       gp->bidding[gp->Nbids-2].type == BIDDING &&
       gp->bidding[gp->Nbids-2].suit == NOTRUMPS &&
       gp->bidding[gp->Nbids-2].number == 4)
    {
      respondblackwood(gp, hand, bid);
    }  
    if(gp->Nbids >= 4 &&
       gp->bidding[gp->Nbids-4].type == BIDDING &&
       gp->bidding[gp->Nbids-4].suit == NOTRUMPS &&
       gp->bidding[gp->Nbids-4].number == 4)
      {
        slamafterblackwood(gp, hand, bid);
      }
  
      
  }
  else if(overbidding(gp))
  {
    respondopposingbid(gp, hand, bid);
  }
  /* should never get here */
  bid->type = PASS;
}

int currentsuit(GAMEPUBLIC *gp)
{
  int index;

  if(togo(gp) == gp->lead)
    return NOTRUMPS;
  else
  {
    index = (gp->Nplayed/4)*4; 
    return gp->played[index].card.suit;
  }
}

int Ncardsinhand(GAMEPUBLIC *gp)
{
  return 13 - gp->Nplayed/4;
}

static int cardbeats(CARD *a, CARD *b, int suit, int trumps)
{
  if(a->suit == suit && b->suit == suit)
  {
    if(acehigh(a->rank) <= acehigh(b->rank))
      return 1;
    else
      return 0;
  }
  if(a->suit == trumps && b->suit != trumps)
    return 0;
  if(a->suit != trumps && b->suit == trumps)
    return 1;
  if(a->suit == trumps && b->suit == trumps)
  {
    if(acehigh(a->rank) <= acehigh(b->rank))
      return 1;
    else
      return 0;
  
  }
  if(a->suit != suit && b->suit == suit)
    return 1;
  return 0;
}

static int winner(GAMEPUBLIC *gp, CARD *card)
{
  int base;
  int suit;
  int i;

  suit = currentsuit(gp);
  base = (gp->Nplayed/4)*4;
  for(i=base; i<gp->Nplayed;i++)
    if(!cardbeats(&gp->played[i].card, card, suit, gp->trumps))
       return 0;
       
  return 1;  
}

int getmissingcards(GAMEPUBLIC *gp, CARD *cards, CARD *hand, int Nhand)
{
  CARD visible[52];
  int answer;
  int i, j;

  memcpy(visible, hand, Nhand * sizeof(CARD));
  memcpy(visible + Nhand, gp->dummy, gp->Ndummy * sizeof(CARD));
  j = Nhand + gp->Ndummy;
  for(i=0;i<gp->Nplayed;i++)
    memcpy(&visible[j++], &gp->played[i].card, sizeof(CARD));
  deckcomplement(visible, j, cards, &answer);

  return answer;
} 


static int getlikelywinners(GAMEPUBLIC *gp, CARD *ret, CARD *hand, int Nhand, CARD *declarer, int Ndeclarer)
{
  int i, ii;
  int j = 0;
  CARD missing[52];
  int Nmissing;
  int trumpsinplay;


  if(declarer)
     Nmissing = getmissingcards(gp, missing, declarer, Ndeclarer);
  else
     Nmissing = getmissingcards(gp, missing, hand, Nhand);
  trumpsinplay = Nofsuit(missing, Nmissing, gp->trumps);
  for(i=0;i<Nhand;i++)
  {
    for(ii=0;ii<Nmissing;ii++)
      if(missing[i].suit == hand[i].suit && acehigh(missing[ii].rank) > acehigh(hand[i].rank))
	 break;
    if(ii < Nmissing)
      continue;
    if(trumpsinplay && Nofsuit(missing, Nmissing, hand[i].suit) < 2)
       continue;
    memcpy(&ret[j++], &hand[i], sizeof(CARD));
  } 
    
  return j;
}

static void selectdiscard(GAMEPUBLIC *gp, CARD *hand, CARD *card)
{
  int Ncards;
  int besti =-1;
  int i;

  Ncards = Ncardsinhand(gp);
  for(i=0;i<Ncards;i++)
  {
    if(besti == -1)
      besti = i;
    else if(acehigh(hand[i].rank) < acehigh(hand[besti].rank))
      besti = i;
  }
  memcpy(card, &hand[besti], sizeof(CARD));
}

static void lastplayercard(GAMEPUBLIC *gp, CARD *hand, CARD *card)
{
  int Ncards;
  int suit;
  int besti = -1;
  int i;
 
  Ncards = Ncardsinhand(gp);
  suit = currentsuit(gp);
  if(winner(gp, &gp->played[gp->Nplayed -2].card))
  {
    if(Nofsuit(hand, Ncards, suit))
    {
      card->rank = lowestofsuit(hand, Ncards, suit);
      card->suit = suit;
    }
    else
      selectdiscard(gp, hand, card);
    return;
  }  
  if(Nofsuit(hand, Ncards, suit))
  {
    for(i=0;i<Ncards;i++)
      if(hand[i].suit == suit)
      {
        if(besti == -1)
          besti = i;
        else
	{
          if(winner(gp, &hand[i]) && !winner(gp, &hand[besti]) )
            besti = i;
          else if(!winner(gp, &hand[i]) && !winner(gp, &hand[besti]))
	  {
            if(acehigh(hand[i].rank) < acehigh(hand[besti].rank))
              besti = i;
	  }
	}
      }
    memcpy(card, &hand[besti], sizeof(CARD));
    return;
  }
  else if(Nofsuit(hand, Ncards, gp->trumps))
  {
    for(i=0;i<Ncards;i++)
      if(hand[i].suit == gp->trumps)
        if(winner(gp, &hand[i]))
	{
          if(besti == -1 || acehigh(hand[besti].rank) > acehigh(hand[i].rank))
            besti = i;
	}
    if(besti == -1)
      selectdiscard(gp, hand, card);
    else
      memcpy(card, &hand[besti], sizeof(CARD));
    return;
  }
  else
  {
    selectdiscard(gp, hand, card);
  }
}

void playhigh(GAMEPUBLIC *gp, CARD *hand, CARD *card)
{
  int Nhand;
  int suit;
  int besti = -1;
  int i;

  Nhand = Ncardsinhand(gp);
  suit = currentsuit(gp);
  if(suit && Nofsuit(hand, Nhand, suit) > 0)
  {
    for(i=0;i<Nhand;i++)
      if(hand[i].suit == suit)
      {
        if(besti == -1)
          besti = i;
        else if(winner(gp, &hand[i]) && acehigh(hand[i].rank) > acehigh(hand[besti].rank) )
          besti = i;
        else if(!winner(gp, &hand[i]) && !winner(gp, &hand[besti]))
          if(acehigh(hand[i].rank) < acehigh(hand[besti].rank))
            besti = i;

      }
    memcpy(card, &hand[besti], sizeof(CARD));
    return;
  }
  else if(Nofsuit(hand, Nhand, gp->trumps))
  {
    for(i=0;i<Nhand;i++)
      if(hand[i].suit == gp->trumps && winner(gp, &hand[i]) )
      {
        if(besti == -1)
          besti = i;
        else if(acehigh(hand[i].rank) < acehigh(hand[besti].rank))
          besti = i;
      }
    if(besti != -1)
    {
      memcpy(card, &hand[besti], sizeof(CARD));
      return;
    }
  }
  
  selectdiscard(gp, hand, card);  
}

void playlow(GAMEPUBLIC *gp, CARD *hand, CARD *card)
{
  int Nhand;
  int suit;
  int besti = -1;
  int i;

  Nhand = Ncardsinhand(gp);
  suit = currentsuit(gp);
  if(suit && Nofsuit(hand, Nhand, suit) > 0)
  {
    for(i=0;i<Nhand;i++)
      if(hand[i].suit == suit)
      {
        if(besti == -1)
          besti = i;
        else if(acehigh(hand[i].rank) < acehigh(hand[besti].rank))
          besti = i;

      }
    memcpy(card, &hand[besti], sizeof(CARD));
    return;
  }
  else if(Nofsuit(hand, Nhand, gp->trumps))
  {
    for(i=0;i<Nhand;i++)
      if(hand[i].suit == gp->trumps && winner(gp, &hand[i]))
      {
        if(acehigh(hand[i].rank) < acehigh(hand[besti].rank))
          besti = i;
      }
    if(besti != -1)
    {
      memcpy(card, &hand[besti], sizeof(CARD));
      return;
    }
  }
  
  selectdiscard(gp, hand, card);  
}

static int compranks(const void *e1, const void *e2)
{
  const CARD *c1 = e1;
  const CARD *c2 = e2;
  return acehigh(c2->rank) - acehigh(c1->rank);
}

void choose4thhighestlongestsuit(CARD *hand, CARD *card)
{
 int suits[4] = {HEARTS, CLUBS, DIAMONDS, SPADES};
 int strongest;
 CARD longest[13];
 int i; 
 int j = 0;
 int best = 0;
 int besti = -1;
 int N;

 strongest = strongestsuit(hand);
 for(i=0;i<4;i++)
 {
   N = Nofsuit(hand, 13, suits[i]);
   if(N > best || (N == best && suits[i] == strongest) )
   {
     best = N;
     besti = i;
   }
 }
 for(i=0;i<13;i++)
   if(hand[i].suit == suits[besti])
     memcpy(&longest[j++], &hand[i], sizeof(CARD));
 qsort(longest, N, sizeof(CARD), compranks);
 memcpy(card, &longest[3], sizeof(CARD));

}

void chooseopeninglead(GAMEPUBLIC *gp, CARD *hand, CARD *card)
{
  BID contract;
  int by;

  getcurrentcontract(gp, &contract, &by, 0, 0);
  if(contract.suit == NOTRUMPS)
  {
    choose4thhighestlongestsuit(hand, card);
    return;
  }
  choose4thhighestlongestsuit(hand, card);
 
} 

void leadthroughdummysstrength(GAMEPUBLIC *gp, CARD *hand, CARD *card)
{
  int suits[4] = {HEARTS, CLUBS, DIAMONDS, SPADES};
  CARD missing[52];
  int Nmissing;
  int Nhand;
  int bestsuit = 0;
  int best = 0;
  int missinghigh;
  int dummyhigh;
  int handhigh;
  int i;

  Nhand  = Ncardsinhand(gp);
  Nmissing = getmissingcards(gp, missing, hand, Nhand);
  for(i=0;i<4;i++)
  {
    if(gp->trumps == suits[i])
      continue;
    missinghigh = highestofsuit(missing, Nmissing, suits[i]);
    dummyhigh = highestofsuit(gp->dummy, gp->Ndummy, suits[i]);
    handhigh = highestofsuit(hand, Nhand, suits[i]); 
    if(dummyhigh && handhigh && acehigh(dummyhigh) > 10 &&
       (acehigh(dummyhigh) < acehigh(handhigh) ||
        acehigh(dummyhigh) < acehigh(missinghigh)) )
      if(best < acehigh(dummyhigh))
      {
	best = acehigh(dummyhigh);
        bestsuit = suits[i];
      }
  }
  if(bestsuit == 0)
  {
    for(i=0;i<4;i++)
    {
      dummyhigh = highestofsuit(gp->dummy, gp->Ndummy, suits[i]);
      handhigh = highestofsuit(hand, Nhand, suits[i]); 
      if(dummyhigh && handhigh)
        bestsuit = suits[i];
    }
  }
    
  if(bestsuit == 0)
    bestsuit = hand[0].suit;
  
  card->suit = bestsuit;
  card->rank = lowestofsuit(hand, Nhand, bestsuit);
}

void leadtodummysweakness(GAMEPUBLIC *gp, CARD *hand, CARD *card)
{
  int suits[4] = {HEARTS, CLUBS, DIAMONDS, SPADES};
  int highest[4];
  int bestsuit = 0;
  int Nhand;
  int i;
  int best = 100;

  Nhand  = Ncardsinhand(gp);

  for(i=0;i<4;i++)
    highest[i] = highestofsuit(gp->dummy, gp->Ndummy, suits[i]);
  for(i=0;i<4;i++)
    if(highest[i] == 0 && Nofsuit(hand, Nhand, suits[i]))
       bestsuit = suits[i];
  if(bestsuit == 0)
     for(i=0;i<4;i++)
       if(acehigh(highest[i]) < 10 && Nofsuit(hand, Nhand, suits[i]))
          bestsuit = suits[i];
  if(bestsuit == 0)
     if(bestsuit == 0)
       for(i=0;i<4;i++)
	 if(acehigh(highest[i]) < best && Nofsuit(hand, Nhand, suits[i]))
         {
	   best = highest[i];
          bestsuit = suits[i];
	 }
  if(bestsuit == 0)
    bestsuit = hand[0].suit;
  
  card->suit = bestsuit;
  card->rank = lowestofsuit(hand, Nhand, bestsuit);      
}


//
// GetLeadCard()
//
void getleadcard(GAMEPUBLIC *gp, CARD *hand, CARD *card, CARD *declarer)
{
  int best = 100;
  int besti = -1;
  CARD winners[13];
  int Nwinners;
  int suits[4] = {HEARTS, CLUBS, DIAMONDS, SPADES};
  int i;
  int Nhand;

  Nhand = Ncardsinhand(gp);
  /* first try to cash a winner */
  Nwinners = getlikelywinners(gp, winners, hand, Nhand, declarer, 
			      declarer ? Nhand : 0);
  if(Nwinners)
  {
    for(i=0;i<Nwinners;i++)
      if(winners[i].suit != gp->trumps)
      {
        memcpy(card, &winners[i], sizeof(CARD));
        return;
      }
  }

  /* no winners, so try a singleton, if we have trumps */  
  if(gp->trumps != NOTRUMPS && Nofsuit(hand, Nhand, gp->trumps) > 0)
  {
    for(i=0;i<Nhand;i++)
      if(hand[i].suit != gp->trumps && Nofsuit(hand, Nhand, hand[i].suit) ==1)
      {
          memcpy(card, &hand[i], sizeof(CARD));
          return;
      }       
  }
  
  /* no singleton, try our weakest card */  
  for(i=0;i<4;i++)
  {
    if(gp->trumps != suits[i] && Nofsuit(hand, Nhand, suits[i]))
    {
      if( acehigh(highestofsuit(hand, Nhand, suits[i])) < best )
      {
        best = acehigh(highestofsuit(hand, Nhand, suits[i]));
	besti = i;
      }
    }
  }
  if(besti != -1)
  {
    card->suit = suits[besti];
    card->rank = highestofsuit(hand, Nhand, suits[besti]);
    return;
  }
   
  /* we've got a hand full of losing trumps or something odd, just
     lead first card */
  memcpy(card, &hand[0], sizeof(CARD));
  return;
}

void playcard_ai(GAMEPUBLIC *gp, CARD *hand, CARD *card)
{
  int i;
  int Ncards;
  int suit;

  if( togo(gp) == gp->lead )
  {
    if(gp->Nplayed == 0)
      chooseopeninglead(gp, hand, card);
    else if(gp->lead == nextplayer(gp->dummy_id, 1))
      leadtodummysweakness(gp, hand, card);
    else if(gp->lead == nextplayer(gp->dummy_id, 3))
      leadthroughdummysstrength(gp, hand, card);
    else
      getleadcard(gp, hand, card, 0);
  }
  else if( (gp->Nplayed % 4) == 1)
    playlow(gp, hand, card);
  else if( (gp->Nplayed % 4) == 2)
    playhigh(gp, hand, card); 
  else if( (gp->Nplayed % 4) == 3)
    lastplayercard(gp, hand, card);
  else
  {
    Ncards = Ncardsinhand(gp);
    suit = currentsuit(gp);
    for(i=0;i<Ncards;i++)
      if(hand[i].suit == suit)
        break;
    if(i==Ncards)
      for(i=0;i<Ncards;i++)
        if(hand[i].suit == gp->trumps)
	  break;
    if(i == Ncards)
      i = 0;
    memcpy(card, &hand[i], sizeof(CARD));
  }
}

void playdummy_ai(GAMEPUBLIC *gp, CARD *declarer, CARD *card)
{
  switch(gp->Nplayed % 4)
  {
  case 0:
    getleadcard(gp, gp->dummy, card, declarer);
    break;
  case 1:
    playlow(gp, gp->dummy, card);
    break;
  case 2:
    playhigh(gp, gp->dummy, card);
    break;
  case 3:
    lastplayercard(gp, gp->dummy, card);
    break;
  }
}
