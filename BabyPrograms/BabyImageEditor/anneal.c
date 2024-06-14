
#include <stdlib.h>
#include <math.h>
#include <float.h>

static int metrop(double current, double test, double temperature);
static double getdecrement(double start, double end, int Nsteps);
static double stdev(double *x, int N);

/*
  All function pointers take a arbitrary pointer ptr as a parameter.
  This can be NULL.

  Function to create a local copy of an object
void *clone(void *obj, void *ptr);
  Function to destroy an object
void kill(void *obj, void *ptr);
  Function to copy one object to another (more efficient than clone)
int copy(void *dest, void *src, void *ptr);

  The score function to minimise
double score(void *obj, void *ptr);
  The move function
void mutate(void *obj, void *ptr);

  For parallel programming

  We need to be able to represent objects as serial streams of bytes.

  Convert serial representation to object  
int deserialise(void *obj, unsigned char *buff, int len, void *ptr);
  Serialise when we know maximum size (for efficiency)
int serialise(void *obj, unsigned char *buff, int capacity, void *ptr);
  Serialise when we don't know maximum size
void *serialisev(void *obj, int *len, void *ptr); 
*/

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
           int Nevaluations, void *ptr)
{
  void *current = 0;
  void *test = 0;
  double mindiff = DBL_MAX;
  int i;
  double initial[1024];
  double temperature = 1.0;
  double endtemp;
  double curscore;
  double testscore;
  double bestscore;
  double dec;

  current = clone(obj, ptr);
  if(!current)
    goto error_exit;
  test = clone(obj, ptr);
  if(!test)
    goto error_exit;

  curscore = score(current, ptr);
  bestscore = curscore;

  for(i=0;i<1024;i++)
  {
    copy(test, current, ptr);
    mutate(test, ptr);
    initial[i] = score(test,ptr);
    if( fabs(initial[i] - curscore) < mindiff)
      if(initial[i] != curscore)
        mindiff = fabs(initial[i] - curscore);
    if(metrop(curscore, initial[i], temperature))
    {
      copy(current, test, ptr);
      curscore = initial[i];
      if(bestscore > curscore)
      {
        bestscore = curscore;
        copy(obj, current, ptr);
      }
    }
  }   

 temperature = stdev(initial, 1024) * 2.0;

 endtemp = 1.0; //mindiff;// /3.0;
 dec = getdecrement(temperature, endtemp, Nevaluations);


 for(i=0;i<Nevaluations;i++)
 {
    copy(test, current, ptr);
    mutate(test, ptr);
    testscore = score(test,ptr);
    // if(testscore != curscore && fabs(testscore - curscore) < mindiff)
    // {
    //  mindiff = fabs(testscore - curscore);
    //  endtemp = mindiff;  // 3.0;
    //  dec = getdecrement(temperature, endtemp, Nevaluations - i);
    // }
    if(metrop(curscore, testscore, temperature))
    {
      copy(current, test, ptr);
      curscore = testscore;
      if(bestscore > curscore)
      {
        bestscore = curscore;
        copy(obj, current, ptr);
      }
    }

    temperature *= dec;
 }

 kill(test, ptr);
 kill(current, ptr);

 return 0;

error_exit:
 if(test)
   kill(test, ptr);
 if(current)
   kill(current, ptr);

 return -1;
}

static int metrop(double current, double test, double temperature)
{
  double p;

  if(test <= current)
    return 1;
  else
  {
    p = rand() /(RAND_MAX + 1.0);
    if( p < exp( (current - test) / temperature ) )
      return 1;
    else
      return 0;
  }
}

static double getdecrement(double start, double end, int Nsteps)
{
  /* start * answer^Nsteps = end */
  return  exp( (log(end) - log(start))/Nsteps);
}

static double stdev(double *x, int N)
{
  int i;
  double total = 0;
  double totalvar = 0;
  double mean;

  for(i=0;i<N;i++)
    total += x[i];
  mean = total/N;

  for(i=0;i<N;i++)
    totalvar += (x[i] - mean) * (x[i] - mean);

  return sqrt(totalvar/N);
}
