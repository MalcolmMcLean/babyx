
#ifndef anneal_h
#define anneal_h

/*
  Fairly standard simulate annealing schedule
  Params: obj - the object to minimise
          clone - clone function
          kill - destructor function
          copy - function to copy
          score - the score function
          mutate - move a step
          Nevaluations - number of times to evaluate score function
          ptr - pointer to pass to parameterised functions
 */
int anneal(void *obj, 
           void * (*clone)(void *obj, void *ptr),
           void (*kill)(void *obj, void *ptr), 
           int  (*copy)(void *dest, void *src, void *ptr),
           double (*score) (void *obj, void *ptr),
           void (*mutate) (void *obj, void *ptr),
           int Nevaluations, void *ptr);

#endif
