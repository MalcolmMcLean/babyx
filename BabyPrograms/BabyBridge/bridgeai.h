#ifndef bridgeai_h
#define bridgeai_h

void bid_ai(GAMEPUBLIC *gp, CARD *hand, BID *bid);
void playcard_ai(GAMEPUBLIC *gp, CARD *hand, CARD *card);
void playdummy_ai(GAMEPUBLIC *gp, CARD *declarer, CARD *card);

#endif
