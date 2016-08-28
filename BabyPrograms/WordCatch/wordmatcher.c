#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "wordmatcher.h"

extern char *english_words_10[4393];
extern char *english_words_20[7951];
extern char *english_words_35[36381];
extern char *english_words_40[6929];
extern char *english_words_50[31118];
extern char *english_words_55[6311];
extern char *english_words_60[13508];
extern char *english_words_70[39825];
//extern char *english_phrases[7289];


static char **matchlists(char **list, int N, char *wild, int Nmax, int *Nret);
static char **anagramlists(char **list, int N, char *word, int Nmax, int *Nret);
static char **catN(int N, ...);
static char **catlist(char **a, int Na, char **b, int Nb);
static int domatch(char *word, char *wild);
static int isanagram(char *str1, char *str2);
static void killlist(char **list, int N);
static int wordinlist(char **list, int N, char *word);
static int strcount(char *str, int ch);
static char *mystrdup(const char *str);


char **matchword(char *word, int level, int *N)
{
  char **list_10;
  char **list_20;
  char **list_35;
  char **list_40;
  char **list_50;
  char **list_55;
  char **list_60;
  char **list_70;
  char **list_phrases = 0;
  int N10;
  int N20;
  int N35;
  int N40;
  int N50;
  int N55;
  int N60;
  int N70;
  int Nphrases = 0;
  char **answer;

  *N = 0;

  switch(level)
  {
  case 0:
    list_10 = matchlists(english_words_10, 4393, word, 100, &N10);
	list_20 = matchlists(english_words_20, 7951, word, 100, &N20);
	list_35 = matchlists(english_words_35, 36381, word, 100, &N35);
	//list_phrases = matchlists(english_phrases, 7289, word, 100, &Nphrases);
	answer = catN(4, list_10, N10, list_20, N20, list_35, N35, list_phrases, Nphrases);
	if(!answer)
	   return 0;
	*N = N10 + N20 + N35 + Nphrases;
	free(list_10);
	free(list_20);
	free(list_35);
	free(list_phrases);
	return answer;
  case 1:
	list_10 = matchlists(english_words_10, 4393, word, 100, &N10);
	list_20 = matchlists(english_words_20, 7951, word, 100, &N20);
	list_35 = matchlists(english_words_35, 36381, word, 100, &N35);
	list_40 = matchlists(english_words_40, 6929, word, 100, &N40);
    list_50 = matchlists(english_words_50, 31118, word, 100, &N50);
    list_55 = matchlists(english_words_55, 6311, word, 100, &N55); 
	answer = catN(6, list_10, N10, list_20, N20, list_35, N35, list_40, N40, list_50, N50, list_55, N55);
	if(!answer)
	  return 0;
	*N = N10 + N20 + N35 + N40 + N50 + N55;
	free(list_10);
	free(list_20);
	free(list_35);
	free(list_40);
	free(list_50);
	free(list_55);
	return answer;
    
  case 2:
	list_10 = matchlists(english_words_10, 4393, word, 100, &N10);
	list_20 = matchlists(english_words_20, 7951, word, 100, &N20);
	list_35 = matchlists(english_words_35, 36381, word, 100, &N35);
	list_40 = matchlists(english_words_40, 6929, word, 100, &N40);
    list_50 = matchlists(english_words_50, 31118, word, 100, &N50);
    list_55 = matchlists(english_words_55, 6311, word, 100, &N55); 
	list_60 = matchlists(english_words_60, 13508, word, 100, &N60);
	list_70 = matchlists(english_words_70, 39825, word, 100, &N70);
	answer = catN(8, list_10, N10, list_20, N20, list_35, N35, 
		list_40, N40, list_50, N50, list_55, N55,
		list_60, N60, list_70, N70);
	if(!answer)
	  return 0;
	*N = N10 + N20 + N35 + N40 + N50 + N55 + N60 + N70;
	free(list_10);
	free(list_20);
	free(list_35);
	free(list_40);
	free(list_50);
	free(list_55);
	free(list_60);
	free(list_70);
	return answer;
  default:
    assert(level >= 0 && level < 3);
	return 0;
  }
  return 0;
}

char **findanagrams(char *word, int level, int *N)
{
  char **list_10;
  char **list_20;
  char **list_35;
  char **list_40;
  char **list_50;
  char **list_55;
  char **list_60;
  char **list_70;
  int N10;
  int N20;
  int N35;
  int N40;
  int N50;
  int N55;
  int N60;
  int N70;
  char **answer;

  *N = 0;

  switch(level)
  {
  case 0:
    list_10 = anagramlists(english_words_10, 4393, word, 100, &N10);
	list_20 = anagramlists(english_words_20, 7951, word, 100, &N20);
	list_35 = anagramlists(english_words_35, 36381, word, 100, &N35);
	answer = catN(3, list_10, N10, list_20, N20, list_35, N35);
	if(!answer)
	   return 0;
	*N = N10 + N20 + N35;
	free(list_10);
	free(list_20);
	free(list_35);
	return answer;
  case 1:
	list_10 = anagramlists(english_words_10, 4393, word, 100, &N10);
	list_20 = anagramlists(english_words_20, 7951, word, 100, &N20);
	list_35 = anagramlists(english_words_35, 36381, word, 100, &N35);
	list_40 = anagramlists(english_words_40, 6929, word, 100, &N40);
    list_50 = anagramlists(english_words_50, 31118, word, 100, &N50);
    list_55 = anagramlists(english_words_55, 6311, word, 100, &N55); 
	answer = catN(6, list_10, N10, list_20, N20, list_35, N35, list_40, N40, list_50, N50, list_55, N55);
	if(!answer)
	  return 0;
	*N = N10 + N20 + N35 + N40 + N50 + N55;
	free(list_10);
	free(list_20);
	free(list_35);
	free(list_40);
	free(list_50);
	free(list_55);
	return answer;
    
  case 2:
	list_10 = anagramlists(english_words_10, 4393, word, 100, &N10);
	list_20 = anagramlists(english_words_20, 7951, word, 100, &N20);
	list_35 = anagramlists(english_words_35, 36381, word, 100, &N35);
	list_40 = anagramlists(english_words_40, 6929, word, 100, &N40);
    list_50 = anagramlists(english_words_50, 31118, word, 100, &N50);
    list_55 = anagramlists(english_words_55, 6311, word, 100, &N55); 
	list_60 = anagramlists(english_words_60, 13508, word, 100, &N60);
	list_70 = anagramlists(english_words_70, 39825, word, 100, &N70);
	answer = catN(8, list_10, N10, list_20, N20, list_35, N35, 
		list_40, N40, list_50, N50, list_55, N55,
		list_60, N60, list_70, N70);
	if(!answer)
	  return 0;
	*N = N10 + N20 + N35 + N40 + N50 + N55 + N60 + N70;
	free(list_10);
	free(list_20);
	free(list_35);
	free(list_40);
	free(list_50);
	free(list_55);
	free(list_60);
	free(list_70);
	return answer;
  default:
    assert(level >= 0 && level < 3);
	return 0;
  }
  return 0;
}

int randword(char *ret, int len, int level)
{
  int i;
  int breaker = 0;

  do
  {
    i = rand() % 4393;
    while(i < 4393 && strlen(english_words_10[i]) != len)
		i++;
	if(i != 4393 && !strchr(english_words_10[i], '\''))
	{
	  strcpy(ret, english_words_10[i]);
	  return 0;
	}
  }
  while(breaker++ < 100);

  return -1;
}

int wordindictionary(char *word)
{
  if(wordinlist(english_words_10, 4393, word))
    return 10;
  if(wordinlist(english_words_20, 7951, word))
    return 20;
  if(wordinlist(english_words_35, 36381, word))
    return 35;
  if(wordinlist(english_words_40, 6929, word))
    return 40;
  if(wordinlist(english_words_50, 31118, word))
    return 50;
  if(wordinlist(english_words_55, 6311, word))
    return 55;
  if(wordinlist(english_words_60, 13508, word))
    return 60;
  if(wordinlist(english_words_70, 39825, word))
    return 70;

  return 0;
}

static char **matchlists(char **list, int N, char *wild, int Nmax, int *Nret)
{
  char **answer = 0;
  int buffsize = 0;
  int Nfound = 0;
  void *temp;
  int i;

  for(i=0;i<N;i++)
  {
    if(domatch(list[i], wild))
	{
	  if(Nfound == buffsize)
	  {
	    buffsize = buffsize * 2 + 10;
	    temp = realloc(answer, buffsize * sizeof(char *));
		if(!temp)
		  goto error_exit;
		answer = temp;
	  }
      answer[Nfound] = mystrdup(list[i]);
	  if(!answer[Nfound])
		  goto error_exit;
	  Nfound++;
	  if(Nfound == Nmax)
	    break;
	}
  }
  *Nret = Nfound;
  return answer;

error_exit:
  for(i=0;i<Nfound;i++)
    free(answer[i]);
  free(answer);
  *Nret = -1;
  return 0;
}

static char **anagramlists(char **list, int N, char *word, int Nmax, int *Nret)
{
  char **answer = 0;
  int buffsize = 0;
  int Nfound = 0;
  void *temp;
  int i;

  for(i=0;i<N;i++)
  {
    if(isanagram(list[i], word))
	{
	  if(Nfound == buffsize)
	  {
	    buffsize = buffsize * 2 + 10;
	    temp = realloc(answer, buffsize * sizeof(char *));
		if(!temp)
		  goto error_exit;
		answer = temp;
	  }
      answer[Nfound] = mystrdup(list[i]);
	  if(!answer[Nfound])
		  goto error_exit;
	  Nfound++;
	  if(Nfound == Nmax)
	    break;
	}
  }
  *Nret = Nfound;
  return answer;

error_exit:
  for(i=0;i<Nfound;i++)
    free(answer[i]);
  free(answer);
  *Nret = -1;
  return 0;
}

static char **catN(int N, ...)
{
  va_list vargs;
  char **answer = 0;
  char **temp;
  char **list;
  int Ntot = 0;
  int Nl;
  int i;

  va_start(vargs, N);
  for(i=0;i<N;i++)
  {
    list = va_arg(vargs, char **);
	Nl = va_arg(vargs, int);
	temp = catlist(answer, Ntot, list, Nl);
	free(answer);
	Ntot += Nl;
	answer = temp;
  }
  va_end(vargs);

  return answer;
}

static char **catlist(char **a, int Na, char **b, int Nb)
{
  char **answer;
  int i;

  answer = malloc((Na + Nb) * sizeof(char *));
  if(!answer)
	  return 0;
  for(i=0;i<Na;i++)
    answer[i] = a[i];
  for(i=0;i<Nb;i++)
    answer[i+Na] = b[i];

  return answer;
}


static int domatch(char *word, char *wild)
{
   int i;

   for(i=0;word[i];i++)
   {
     if(word[i] != wild[i] && wild[i] != '?')
	   return 0;
     if(word[i] == '\'')
	   return 0;
   }

   return (wild[i] == 0) ? 1 : 0;
}

static int isanagram(char *str1, char *str2)
{
  int i;

  if(strlen(str1) != strlen(str2))
    return 0;
  for(i=0;str1[i];i++)
    if(strcount(str1, str1[i]) != strcount(str2, str1[i]))
	  return 0;
  return 1;
}

static void killlist(char **list, int N)
{
  int i;

  if(list)
  {
    for(i=0;i<N;i++)
      free(list[i]);
    free(list);
  }
}

static int wordinlist(char **list, int N, char *word)
{
  int low = 0;
  int high = N-1;
  int mid;
  int cmp;

  while(low <= high)
  {
    mid = (low + high) /2;
	cmp = strcmp(list[mid], word);
	if(cmp == 0)
	  return 1;
	if(cmp < 0)
	  low = mid+1;
	else
	  high = mid-1;
  }

  return 0;

}

static int strcount(char *str, int ch)
{
  int answer = 0;
  while(*str)
    if(*str++ == ch)
	  answer++;
  return answer;
}

static char *mystrdup(const char *str)
{
  char *answer;

  answer = malloc(strlen(str)+1);
  if(answer)
    strcpy(answer, str);
  return answer;
}