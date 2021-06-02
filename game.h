/*
  sudoku game.h

  Suduku game: game representation related declarations
*/

#ifndef __GAME_H__
#define __GAME_H__

#include "sudoku.h"

extern int  new_bookmark( void );
extern int  get_bookmark_number( void );
extern int  check_if_at_bookmark( void );
extern int  return_to_last_bookmark( void );

extern bool is_undo_possible( void );
extern int  undo( void );

extern bool is_redo_possible( void );
extern int  redo( void );

extern void start_game( void );
extern void reset_game( void );

extern void *save_current_game( void );
extern void *save_current_game_for_solving( void );
extern void restore_saved_game( void *game );
extern void update_saved_game( void *game );

extern void game_new_grid( void );
extern void game_new_empty_grid( void );
extern void game_new_filled_grid( void );
extern void game_previous_grid( void );

extern void game_set_cell_symbol( int row, int col, int symbol, bool is_given );
extern void game_toggle_cell_candidate( int row, int col, int symbol );

extern void game_erase_cell( int row, int col );
extern void game_fill_cell( int row, int col, bool no_conflict );

extern void set_game_time( unsigned long duration );
extern unsigned long get_game_duration( void );

extern void set_game_level( sudoku_level_t level );
extern sudoku_level_t get_game_level( void );

#endif /* __GAME_H__ */
