/*
  sudoku gen.h

  Suduku game: solver & generator representation related declarations
*/

#ifndef __GEN_H__
#define __GEN_H__

#include <stdlib.h>
#include "sudoku.h"
#include "common.h"

extern int check_current_game( int nb_states_to_save );

extern int find_one_solution( int nb_states_to_save );
extern int make_game( int game_number );

#endif /* __GEN_H__ */
