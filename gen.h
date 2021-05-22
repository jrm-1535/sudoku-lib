/*
  sudoku gen.h

  Suduku game: solver & generator
*/

#ifndef __GEN_H__
#define __GEN_H__

extern int check_current_game( void );
extern int find_one_solution( void );
extern int make_game( int game_number );

// return 0 if could not step, 1 if it did a step, 2 if the game is solved
extern int solve_step( void );

#endif /* __GEN_H__ */
