/*
  Sudoku game - random number interface
*/

#include <stdlib.h>
#include "rand.h"

extern void set_random_seed ( unsigned int seed )
{
    srand( seed );
}

extern int random_value ( int min_val, int max_val )
{
    if ( min_val == max_val ) return min_val;

    int modulo = (1+max_val-min_val);
    int limit = RAND_MAX/modulo;
    limit *= modulo;

    int randomv;
    do {
        randomv = rand();
    } while( randomv > limit );
    return min_val + (randomv % modulo);
}
