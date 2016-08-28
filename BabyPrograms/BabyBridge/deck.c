#include <stdlib.h>
#include "deck.h"

CARD *sorteddeck(void)
{
  CARD *answer;
  int i;

  answer = malloc(52 * sizeof(CARD));
  if(!answer)
    return 0;
  for(i=0;i<52;i++)
  {
    answer[i].suit = (i/13)+1;
    answer[i].rank = (i % 13)+1;
  }
  return answer;
}

void deckcomplement(CARD *in, int N, CARD *out, int *Nret)
{
  unsigned char flags[52] = {0};
  int i, j = 0;
  int index;

  for(i=0;i<N;i++)
  {
    index = (in[i].suit-1) * 13 + (in[i].rank-1);
    flags[index] = 1;
  }
  for(i=0;i<52;i++)
  {
    if(flags[i] == 0)
    {
      out[j].rank = (i % 13) + 1;
      out[j].suit = i/13 + 1;
      j++;
    }
  }
  if(Nret)
    *Nret = j;
}

static int acehigh(int rank)
{
  if(rank == 1)
    return 14;
  else
    return rank;
} 

static int compfunc(const void *e1, const void *e2)
{
  const CARD *c1 = e1;
  const CARD *c2 = e2;

  if(c1->suit != c2->suit)
    return c1->suit - c2->suit;
  else
    return acehigh(c2->rank) - acehigh(c1->rank);
}

void sortbysuit(CARD *cards, int N)
{
  qsort(cards, N, sizeof(CARD), compfunc);
}
