#ifndef datepicker_h
#define datepicker_h

/*
  Baby X date picker dialog

  year - eg 2014
  month - 1 = January
  day - 1 = first day of month, 0 means don't select day

  Returns 0 on success, -1 on cancel
*/
int pickdate(BABYX *bbx, int year, int month, int day, int *yearout, int *monthout, int *dayout);

#endif
