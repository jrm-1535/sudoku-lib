/*
  sudoku hint.h

  Suduku game: hint related declarations
*/

#ifndef __HINT_H__
#define __HINT_H__

#include "sudoku.h"

extern int get_number_from_map( unsigned short map );

static inline unsigned short get_map_from_number( int number )
{
    assert( number >= 0 && number < 9 );
    return 1 << number;
}

extern char sudoku_get_symbol( sudoku_cell_t *cell );
extern sudoku_hint_type find_hint( int *row_hint, int *col_hint );

#endif /* __HINT_H__ */
