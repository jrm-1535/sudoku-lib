/*
  sudoku rand.h

  Suduku game: random generator related declarations
*/

#ifndef __RAND_H__
#define __RAND_H__

extern void set_random_seed ( unsigned int seed );

/* return a pseudo-random number min_val through max_val, both included */
extern int random_value ( int min_val, int max_val );

#endif /* __RAND_H__ */
