/*
  sudoku gen.h

  Suduku game: solver & generator
*/

#ifndef __GEN_H__
#define __GEN_H__

#include "game.h"

extern int check_current_game( void );
extern int find_one_solution( void );
extern sudoku_level_t make_game( int game_number );

#endif /* __GEN_H__ */
