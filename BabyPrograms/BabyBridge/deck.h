#ifndef deck_h
#define deck_h

#define HEARTS 1
#define CLUBS 2
#define DIAMONDS 3
#define SPADES 4
#define JOKER 5

typedef struct
{
  int suit;
  int rank;  /*1 = ace, 13 = king */
} CARD;

CARD *sorteddeck(void);
void deckcomplement(CARD *in, int N, CARD *out, int *Nret);
void sortbysuit(CARD *cards, int N);

#endif

