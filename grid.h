
#ifndef __GRID_H__
#define __GRID_H__

#include "sudoku.h"
#include "debug.h"

/*
  Sudoku grid management: cell reference, selection, modification and conflict detection

  The sudoku game can be seen as a 2 dimension array ( 9 rows * 9 columns )
  of cells. Internally, rows and columns are in the range [0-8]. Each cell contains
  0, 1 or n possible symbols (from 0 up to 9), each symbol being stored as a bit in
  a bitmap, and entered and retrieved as a number from 0 to 8. A cell with 0 symbol
  is considered as empty. A cell with 1 symbol is called a single, and a cell with
  more than 1 symbol is considered as a cell with penciled candidates.

  The array of cells is called here a grid. 
  Each cell belongs to one row, one column and one box:

    | 0 | 1 | 2| 3| 4 | 5 | 6 | 7 | 8 |
  --|---------------------------------|
  0 |          |          |           |
  1 |   box 0  |   box 1  |   box 2   |
  2 |          |          |           |
  -------------------------------------
  3 |          |          |           |
  4 |   box 3  |   box 4  |   box 5   |
  5 |          |          |           |
  -------------------------------------
  6 |          |          |           |
  7 |   box 6  |   box 7  |   box 8   |
  8 |          |          |           |
  -------------------------------------
*/

#define SUDOKU_N_BOXES 9

extern int get_n_bits_from_map( int map );
extern int extract_bit_from_map( int *map );

extern void get_selected_row_col( int *row, int *col );
extern void select_row_col( int row, int col );

extern bool is_cell_given( int row, int col );
extern void make_cells_given( void );

extern bool is_cell_empty( int row, int col );

extern void set_cell_symbol( int row, int col, int symbol, bool is_given );
extern void add_cell_candidate( int row, int col, int symbol );
extern void toggle_cell_candidate( int row, int col, int symbol );
extern void set_cell_candidates( int row, int col, int n_candidates, int candidate_map );
extern void remove_cell_candidates( int row, int col, int n_candidates, int candidate_map );
extern void erase_cell( int row, int col );
extern bool get_cell_type_n_map( int row, int col, uint8_t *nsp, int *mp );

extern sudoku_cell_t * get_cell( int row, int col );
extern bool sudoku_get_cell_definition( int row, int col, sudoku_cell_t *cell );
extern void check_cell_integrity( sudoku_cell_t *c );

extern int get_number_from_map( unsigned short map );

static inline unsigned short get_map_from_number( int number )
{
    SUDOKU_ASSERT( number >= 0 && number < 9 );
    return 1 << number;
}

typedef struct {
    int row, col;
} cell_ref_t;       // reference to a cell by row & col

extern bool is_game_solved( void );

#define SOLVED_COUNT (SUDOKU_N_ROWS*SUDOKU_N_COLS)
extern int count_single_symbol_cells( void );

extern int get_singles_matching_map_in_game( int symbol_map, cell_ref_t *singles );

static inline bool is_single_ref( cell_ref_t *cr )
{
    sudoku_cell_t *cell = get_cell( cr->row, cr->col );
    return 1 == cell->n_symbols;
}

extern bool remove_grid_conflicts( void );
extern void fill_in_cell( int row, int col, bool no_conflict );

extern void reset_grid_errors( void );
extern size_t update_grid_errors( int row, int col );

typedef enum {
    HINT = 1, REGULAR_TRIGGER = 2, WEAK_TRIGGER = 4, ALTERNATE_TRIGGER = 8, // exclusive
    HEAD = 16, PENCIL = 32                                                  // can be combined
} cell_attrb_t;

extern void set_cell_attributes( int row, int col, cell_attrb_t attrb );
extern void reset_cell_attributes( void );

extern void print_grid( void );
extern void print_grid_pencils( void );

#endif /* __GRID_H__ */
