extern unsigned char ace_of_hearts_rgba[];
extern unsigned char ace_of_clubs_rgba[];
extern unsigned char ace_of_diamonds_rgba[];
extern unsigned char ace_of_spades_rgba[];

extern unsigned char two_of_hearts_rgba[];
extern unsigned char two_of_clubs_rgba[];
extern unsigned char two_of_diamonds_rgba[];
extern unsigned char two_of_spades_rgba[];

extern unsigned char three_of_hearts_rgba[];
extern unsigned char three_of_clubs_rgba[];
extern unsigned char three_of_diamonds_rgba[];
extern unsigned char three_of_spades_rgba[];

extern unsigned char four_of_hearts_rgba[];
extern unsigned char four_of_clubs_rgba[];
extern unsigned char four_of_diamonds_rgba[];
extern unsigned char four_of_spades_rgba[];

extern unsigned char five_of_hearts_rgba[];
extern unsigned char five_of_clubs_rgba[];
extern unsigned char five_of_diamonds_rgba[];
extern unsigned char five_of_spades_rgba[];

extern unsigned char six_of_hearts_rgba[];
extern unsigned char six_of_clubs_rgba[];
extern unsigned char six_of_diamonds_rgba[];
extern unsigned char six_of_spades_rgba[];

extern unsigned char seven_of_hearts_rgba[];
extern unsigned char seven_of_clubs_rgba[];
extern unsigned char seven_of_diamonds_rgba[];
extern unsigned char seven_of_spades_rgba[];

extern unsigned char eight_of_hearts_rgba[];
extern unsigned char eight_of_clubs_rgba[];
extern unsigned char eight_of_diamonds_rgba[];
extern unsigned char eight_of_spades_rgba[];

extern unsigned char nine_of_hearts_rgba[];
extern unsigned char nine_of_clubs_rgba[];
extern unsigned char nine_of_diamonds_rgba[];
extern unsigned char nine_of_spades_rgba[];

extern unsigned char ten_of_hearts_rgba[];
extern unsigned char ten_of_clubs_rgba[];
extern unsigned char ten_of_diamonds_rgba[];
extern unsigned char ten_of_spades_rgba[];

extern unsigned char jack_of_hearts_rgba[];
extern unsigned char jack_of_clubs_rgba[];
extern unsigned char jack_of_diamonds_rgba[];
extern unsigned char jack_of_spades_rgba[];

extern unsigned char queen_of_hearts_rgba[];
extern unsigned char queen_of_clubs_rgba[];
extern unsigned char queen_of_diamonds_rgba[];
extern unsigned char queen_of_spades_rgba[];

extern unsigned char king_of_hearts_rgba[];
extern unsigned char king_of_clubs_rgba[];
extern unsigned char king_of_diamonds_rgba[];
extern unsigned char king_of_spades_rgba[];

#include "deck.h"

static unsigned char *list[52] =
  {
    ace_of_hearts_rgba,
    two_of_hearts_rgba,
    three_of_hearts_rgba,
    four_of_hearts_rgba,
    five_of_hearts_rgba,
    six_of_hearts_rgba,
    seven_of_hearts_rgba,
    eight_of_hearts_rgba,
    nine_of_hearts_rgba,
    ten_of_hearts_rgba,
    jack_of_hearts_rgba,
    queen_of_hearts_rgba,
    king_of_hearts_rgba,
 ace_of_clubs_rgba,
    two_of_clubs_rgba,
    three_of_clubs_rgba,
    four_of_clubs_rgba,
    five_of_clubs_rgba,
    six_of_clubs_rgba,
    seven_of_clubs_rgba,
    eight_of_clubs_rgba,
    nine_of_clubs_rgba,
    ten_of_clubs_rgba,
    jack_of_clubs_rgba,
    queen_of_clubs_rgba,
    king_of_clubs_rgba,
    ace_of_diamonds_rgba,
    two_of_diamonds_rgba,
    three_of_diamonds_rgba,
    four_of_diamonds_rgba,
    five_of_diamonds_rgba,
    six_of_diamonds_rgba,
    seven_of_diamonds_rgba,
    eight_of_diamonds_rgba,
    nine_of_diamonds_rgba,
    ten_of_diamonds_rgba,
    jack_of_diamonds_rgba,
    queen_of_diamonds_rgba,
    king_of_diamonds_rgba,
    ace_of_spades_rgba,
    two_of_spades_rgba,
    three_of_spades_rgba,
    four_of_spades_rgba,
    five_of_spades_rgba,
    six_of_spades_rgba,
    seven_of_spades_rgba,
    eight_of_spades_rgba,
    nine_of_spades_rgba,
    ten_of_spades_rgba,
    jack_of_spades_rgba,
    queen_of_spades_rgba,
    king_of_spades_rgba,
  };

unsigned char *card_rgba(CARD *card, int *width, int *height)
{
  *width = 75;
  *height = 108;
  return list[(card->suit-1)*13 + card->rank-1];
}
