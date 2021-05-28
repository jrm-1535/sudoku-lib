/*  hsupport.h

    contains functions supporting multiple ways of locating spepcific cells in a grid
    it is intended to be used by various hint implementations.
*/

#include "hint.h"

static inline int get_surrounding_box( int row, int col )
{
/* Box ordering:
    0 [rows 0..2, cols 0..2] 1 [rows 0..2, cols 3..5] 2 [rows 0..2, cols 6..8]
    3 [rows 3..5, cols 0..2] 4 [rows 3..5, cols 3..5] 5 [rows 3..5, cols 6..8]
    6 [rows 6..8, cols 0..2] 7 [rows 6..8, cols 3..5] 8 [rows 6..8, cols 6..8]
*/
    return 3 * (row / 3) + (col / 3);
}

static inline void get_box_first_row_col( int box, int *row, int *col )
{
    *row = 3 * ( box / 3 );
    *col = 3 * ( box % 3 );
}

static inline bool are_cells_in_same_box( int r1, int c1, int r2, int c2 )
{
    return get_surrounding_box( r1, c1 ) == get_surrounding_box( r2, c2 );
}

static inline int get_cell_index_in_box( int row, int col )
{
    return 3 * ( row % 3 ) + ( col % 3 );
}

static inline int get_row_from_box_index( int box, int index )
{
    return 3 * ( box / 3 ) + ( index / 3 );
}

static inline int get_col_from_box_index( int box, int index )
{
    return 3 * ( box % 3 ) + ( index % 3 );
}

extern void get_other_boxes_in_same_box_row( int box, int *other_boxes );
extern void get_other_boxes_in_same_box_col( int box, int *other_boxes );

extern void get_box_row_intersection( int box, int row, cell_ref_t *intersection );
extern void get_box_col_intersection( int box, int col, cell_ref_t *intersection );

static inline int get_cell_ref_box( cell_ref_t *cr )
{
    return get_surrounding_box( cr->row, cr->col );
}

static inline bool is_cell_ref_in_box( int box, cell_ref_t *cr )
{
    return box == get_surrounding_box( cr->row, cr->col );
}

typedef enum { LOCATE_BY_ROW, LOCATE_BY_COL, LOCATE_BY_BOX } locate_t;
extern void get_cell_ref_in_set( locate_t by, int ref, int index, cell_ref_t *cr );

// get_single_for_mask_in_set looks for a single matching a given mask that fits in the given set
extern bool get_single_for_mask_in_set( locate_t by, int ref, int single_mask, cell_ref_t *single );

// Get the single that fits in a box among a list of all singles in the game
extern cell_ref_t * get_single_in_box( cell_ref_t *singles, int n_singles, int box );

// Get the single that fits in a row among a list of all singles in the game
extern cell_ref_t * get_single_in_row( cell_ref_t *singles, int n_singles, int row );

// Get the single that fits in a col among a list of all singles in the game
extern cell_ref_t * get_single_in_col( cell_ref_t *singles, int n_singles, int col );

typedef struct {
    int n_cols;             // for each row, the number of columns in which pencil is found
    int col_map;            // for each row, the candidate column map: 1 bit per column
} candidate_row_location_t; // candidates in the row (up to 9 for the whole row, or 3 if within a box)

typedef struct {
    int n_rows;             // for each column, the number of rows in which pencil is found
    int row_map;            // for each box column, the candidate row map: 1 bit per row
} candidate_col_location_t; // candidates in the col (up to 8 rows for the whole col, or 3 if within a box)

typedef struct {
    int n_cells;            // for each box, the number of cells in which symbol is found
    int cell_map;           // for each box, the candidate map: 1 bit per cell (0, 1, 2/3, 4, 5/6, 7, 8)
} candidate_box_location_t; // candidates in the box (up to 8 cells)

static inline void hint_desc_set_cell_ref_selection( hint_desc_t *hdesc, cell_ref_t *cr )
{
    hdesc->selection = *cr;
}

static inline void hint_desc_set_row_col_selection( hint_desc_t *hdesc, int row, int col )
{
    hdesc->selection.row = row;
    hdesc->selection.col = col;
}

static inline void hint_desc_add_cell_ref_hint( hint_desc_t *hdesc, cell_ref_t *cr )
{
    assert( hdesc->n_hints < SUDOKU_N_SYMBOLS );
    hdesc->hints[ hdesc->n_hints++ ] = *cr;
}

static inline void hint_desc_add_row_col_hint( hint_desc_t *hdesc, int row, int col )
{
    assert( hdesc->n_hints < SUDOKU_N_SYMBOLS );
    hdesc->hints[ hdesc->n_hints ].row = row;
    hdesc->hints[ hdesc->n_hints ].col = col;
    ++hdesc->n_hints;
}

static inline void hint_desc_add_cell_ref_trigger( hint_desc_t *hdesc, cell_ref_t *cr, cell_attrb_t attrb )
{
    assert( hdesc->n_triggers < SUDOKU_N_SYMBOLS );
    hdesc->triggers[ hdesc->n_triggers ] = *cr;
    hdesc->flavors[ hdesc->n_triggers ] = attrb;
    ++hdesc->n_triggers;
}

static inline void hint_desc_add_row_col_trigger( hint_desc_t *hdesc, int row, int col, cell_attrb_t attrb )
{
    assert( hdesc->n_triggers < SUDOKU_N_SYMBOLS );
    hdesc->triggers[ hdesc->n_triggers ].row = row;
    hdesc->triggers[ hdesc->n_triggers ].col = col;
    hdesc->flavors[ hdesc->n_triggers ] = attrb;
    ++hdesc->n_triggers;
}

