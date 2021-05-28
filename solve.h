/*
  sudoku solve.h

  Suduku game: solver & generator
*/

#ifndef __SOLVE_H__
#define __SOLVE_H__

#include "game.h"

extern int check_current_game( void );
extern int find_one_solution( void );
extern sudoku_level_t make_game( int game_number );

#endif /* __SOLVE_H__ */
