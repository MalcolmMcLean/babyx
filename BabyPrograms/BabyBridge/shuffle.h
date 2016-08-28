#ifndef shuffle_h
#define shuffle_h

void shuffle(void *ptr, int N, int size);
void qsortx(void *array, int N, size_t size, int (*compfunc)(void *ptr, const void *e1, const void *e2), void *ptr);
#endif
