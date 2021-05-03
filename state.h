
/*
  Sudoku state handling
*/

#ifndef __STATE_H__
#define __STATE_H__

#include <stdlib.h>
#include "sudoku.h"

/* The game state is the collection of cell values in a grid,
   the current selection and the list of bookmarks.

   It is possible to alter the state by modifying cells,
   by modifying the current selection ot by modifying
   bookmarks.
*/

extern int count_single_symbol_cells( void );
#define SOLVED_COUNT (SUDOKU_N_ROWS*SUDOKU_N_COLS)
static inline bool is_game_solved( void )
{
    return SOLVED_COUNT == count_single_symbol_cells();
}

/* individual cell manipulation */
extern bool sudoku_get_cell_definition( int row, int col, sudoku_cell_t *cell );
extern sudoku_cell_t * get_cell( int row, int col );  // exported to gen.c and hint.c

extern void check_cell_integrity( sudoku_cell_t *c );

extern void check_cell( int row, int col ); // exported to game.c
extern size_t update_grid_errors( int row, int col ); // exported to game.c
extern void reset_grid_errors( void ); // exported to game.c

typedef enum {
    HINT_REGION = 1,
    WEAK_TRIGGER_REGION = 2, TRIGGER_REGION = 4, ALTERNATE_TRIGGER_REGION = 8,
    CHAIN_HEAD = 16, PENCIL = 32
} hint_e;

extern void set_cell_hint( int row, int col, hint_e hint );
extern void reset_cell_hints( void );

extern void erase_cell( int row, int col ); // exported to sudoku_erase_selection in game.c
extern void set_cell_value( int r, int c, int v, bool is_given );     // exported to file.c
extern void add_cell_value( int r, int c, int v );                    // exported to file.c
extern bool get_cell_type_n_values( int r, int c,
                                    size_t *nsp, int *vmp );          // exported to file.c
extern int get_map_next_value( int *vmp );                            // exported to file.c

extern bool is_cell_given( int row, int col );

extern void fill_in_cell( int row, int col, bool no_conflict );
extern bool remove_conflicts( void );

/* update a group of cells */
extern void make_cells_given( void ); // exported to sudoku_commit_game in game.c

/* bookmark manipulation */
extern void erase_all_bookmarks( void );
extern int  new_bookmark( void ); // exported to sudoku_mark_state in game.c
extern int  check_at_bookmark( void );
extern bool return_to_last_bookmark( void ); // exported to sudoku_back_to_mark in game.c
extern int  get_bookmark( void );

/* undo/redo level manipulation */
extern void cancel_redo( void );
extern bool check_redo_level( void );
extern int  get_redo_level( void );

/* undo/redo return:
  0 if they did not do anything, 
  1 if they did undo/redo,
  2 if they did undo/redo AND removed/restored a bookmark.
*/
extern int  undo( void ); // exported to sudoku_undo in game.c
extern int  redo( void ); // exported to sudoku_redo in game.c

/* selection manipulation */
extern void get_selected_row_col( int *row, int *col );
extern void set_selected_row_col( int row, int col );

/* whole state manipulation */
extern void init_state( void );                              // exported to game.c
extern void init_state_for_solving( int nb_states_to_save ); // exported to game.c, gen.c & hint.c
extern void copy_state( int from );                          // exported to game.c && gen.c
extern int  push_state( void );                              // exported to game.c && gen.c

extern void print_grid( void );
extern void print_grid_pencils( void );
#endif /* __STATE_H__ */
