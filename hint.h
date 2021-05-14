/*
  sudoku hint.h

  Suduku game: hint related declarations
*/

#ifndef __HINT_H__
#define __HINT_H__

#include "sudoku.h"
#include "game.h"

extern char sudoku_get_symbol( sudoku_cell_t *cell );
extern sudoku_hint_type find_hint( int *row_hint, int *col_hint );

#endif /* __HINT_H__ */
