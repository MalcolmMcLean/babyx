#include <stdlib.h>

#define uniform() (rand()/(RAND_MAX + 1.0))

void shuffle(void *ptr, int N, int size)
{
  int i, ii;
  unsigned char *cptr = ptr;
  unsigned char *tptr;
  unsigned char tempch;
  int target;

  for(i=0;i<N;i++)
  {
    target = i + (int) (uniform() * (N-i));
    tptr = cptr + target * size;
    for(ii=0;ii<size;ii++)
    {
      tempch = cptr[i*size+ii];
      cptr[i*size+ii] = tptr[ii];
      tptr[ii] = tempch;
    }
  }
}

void swap(void *ptr1, void *ptr2, size_t size)
{
  unsigned char *a = ptr1;
  unsigned char *b = ptr2;
  unsigned char temp;
  size_t i;

  for(i=0;i<size;i++)
  {
    temp = a[i];
    a[i] = b[i];
    b[i] = temp;
  }
}

void qsortx(void *array, int N, size_t size, int (*compfunc)(void *ptr, const void *e1, const void *e2), void *ptr)
{
  int flag = 1;
  int i;
  unsigned char *base = array;

  while(flag)
  {
    flag = 0;
    for(i=0;i<N-1;i++)
      if( (*compfunc)(ptr, base + i*size, base+(i+1)*size) > 0 )
      {
        swap(base + i*size, base + (i+1) * size, size);
        flag = 1;
      }
  }

} 
