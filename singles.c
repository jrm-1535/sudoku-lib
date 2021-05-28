/*  singles.c

    This file implements hints about naked singles and hidden singles.
    Those hints are the most common ones.
*/

#include "hsupport.h"
#include "singles.h"

/* 1. looking for naked singles - note that this must be done first
   in order to remove all known single symbols from other cells. */

static int remove_symbol( int row, int col, int remove_mask )
{
    sudoku_cell_t *cell = get_cell( row, col );

//    if ( cell->n_symbols >= 1 && (cell->symbol_map & remove_mask) ) {
    if ( cell->symbol_map & remove_mask ) {
        if ( cell->n_symbols == 1 ) {
            print_grid_pencils();
            printf( "remove_symbol mask = 0x%03x row %d, col %d\n", remove_mask, row, col );
        }
        SUDOKU_ASSERT ( cell->n_symbols > 1 );                  // in theory not possible
        cell->symbol_map &= ~remove_mask;                       // remove single symbol mask
        if ( 1 == --cell->n_symbols ) return cell->symbol_map;  // found a new naked single
    }
    return 0;
}

static int check_box_of( int row, int col, int remove_mask, 
			              int *row_hint, int *col_hint )
{
    int first_row = row - (row % 3);
    int first_col = col - (col % 3);

    for ( int r = first_row; r < first_row + 3; ++r ) {
        for ( int c = first_col; c < first_col + 3; ++c ) {
            if ( r == row && c == col ) continue;   // don't check itself

            int single_mask = remove_symbol( r, c, remove_mask );
            if ( single_mask ) {
                *row_hint = r;
                *col_hint = c; 
                return single_mask;
            }
        }
    }
    return 0;
}

static int check_row_of( int row, int col, int remove_mask, int *col_hint )
{
    int first_col = col - (col % 3); // assuming box has been or will be processed for the same mask

    for ( int c = 0; c < SUDOKU_N_COLS; c++ ) {
        if ( c >= first_col && c < first_col + 3 ) continue;

        int single_mask = remove_symbol( row, c, remove_mask );
        if ( single_mask ) {
            *col_hint = c;
            return single_mask;
        }
    }
    return 0;
}

static int check_col_of( int row, int col, int remove_mask, int *row_hint )
{
    int first_row = row - (row % 3); // assuming box has been or will be processed for the same mask

    for ( int r = 0; r < SUDOKU_N_ROWS; r++ ) {
        if ( r >= first_row && r < first_row + 3 ) continue;

        int single_mask = remove_symbol( r, col, remove_mask );
        if ( single_mask ) {
            *row_hint = r;
            return single_mask;
        }
    }
    return 0;
}

/* Indicating a naked single hint is done by changing the
   current selection to the location of the naked single,
   and by coloring cells int the neighboring row, col and box
   that are forcing the naked single. All those neighbors do
   not need to show their pencils for that purpose. */

static void set_naked_single_hint_desc_for_cell( int row, int col, bool *symbols,
                                                 bool trigger, hint_desc_t *hdesc )
{
    sudoku_cell_t *cell = get_cell( row, col );
    if ( 1 == cell->n_symbols ) {
        int sn = get_number_from_map( cell->symbol_map );
        if ( symbols[ sn ] ) return;

        if ( trigger ) {
            hint_desc_add_row_col_trigger( hdesc, row, col, REGULAR_TRIGGER );
        } else {
            hint_desc_set_row_col_selection( hdesc, row, col );
            hint_desc_add_row_col_hint( hdesc, row, col );
        }
        symbols[ sn ] = true;
    }
}

#define N_CELLS_PER_BOX SUDOKU_N_SYMBOLS

static bool set_naked_single_hint_desc( int row, int col, int symbol_mask, hint_desc_t *hdesc )
{
#if 0
  printf("Setting naked single hint for row %d, col %d\n", row, col );
#endif

    int first_row = 3 * ( row / 3 );
    int first_col = 3 * ( col / 3 );

    bool symbols[9] = { false }; // keep track of already triggered symbols

    hdesc->hint_type = NAKED_SINGLE;
    hdesc->action = SET;
    hdesc->n_symbols = 1;
    hdesc->symbol_map = symbol_mask;

    // first indicate hint or triggers in current box (easier to spot)
    for ( int i = 0; i < N_CELLS_PER_BOX; ++i ) {
        int r = first_row + (i / 3);
        int c = first_col + (i % 3);
        set_naked_single_hint_desc_for_cell( r, c, symbols, (r != row || c != col), hdesc );
    }

    // then indicate triggers in current row
    for ( int c = 0; c <  SUDOKU_N_COLS; ++c ) {
        if ( c >= first_col && c < first_col + 3 ) continue; // don't duplicate cells already in box
        set_naked_single_hint_desc_for_cell( row, c, symbols, true, hdesc );
    }

    // finally indicate triggers in current col
    for ( int r = 0 ; r < SUDOKU_N_ROWS; ++r ) {
        if ( r >= first_row && r < first_row + 3 ) continue; // don't duplicate cells already in box or row
        set_naked_single_hint_desc_for_cell( r, col, symbols, true, hdesc );
    }
    return true;
}

/* look for a cell in any row, col or box that has only one symbol
    - if not found, return false (no naked single, no possible reduction).
    - if found, remove that symbol in the whole row, col or box
    - if in the process of removing that symbol another cell
    - gets to a single symbol, sets that cell as naked single hint */

extern bool look_for_naked_singles( hint_desc_t *hdesc )
{
    for ( int col = 0; col < SUDOKU_N_COLS; col++ ) {
        for ( int row = 0; row < SUDOKU_N_ROWS; row++ ) {
            sudoku_cell_t *cell = get_cell( row, col );
            if ( 1 == cell->n_symbols ) {
                int remove_mask = cell->symbol_map;

                int row_hint, col_hint;
                int single_mask = check_box_of( row, col, remove_mask, &row_hint, &col_hint );
                if ( single_mask ) {
                    return set_naked_single_hint_desc( row_hint, col_hint, single_mask, hdesc );
                }
                single_mask = check_col_of( row, col, remove_mask, &row_hint );
                if ( single_mask ) {
                    return set_naked_single_hint_desc( row_hint, col, single_mask, hdesc );
                }
                single_mask = check_row_of( row, col, remove_mask, &col_hint );
                if ( single_mask ) {
                    return set_naked_single_hint_desc( row, col_hint, single_mask, hdesc );
                }
            }
        }
    }
    return false;
}

/* 2. Look for hidden singles. This must be done after looking for
   naked singles in order to benefit from the pencil clean up. */

static int check_only_possible_symbols_in_set( locate_t by, int ref, cell_ref_t *candidate )
{
    for ( int s = 0; s < SUDOKU_N_SYMBOLS; ++s ) {
        int mask = get_map_from_number( s ), n_hits = 0;

        for ( int i = 0; i < SUDOKU_N_SYMBOLS; ++i ) {
            cell_ref_t cr;
            get_cell_ref_in_set( by, ref, i, &cr );
            sudoku_cell_t *cell = get_cell( cr.row, cr.col );
            if ( mask & cell->symbol_map ) {
                if ( 1 == cell->n_symbols ) {     // single with symbol, exit set loop
                    SUDOKU_ASSERT( 0 == n_hits );
                    break;
                }
                if ( ++n_hits > 1 ) break;        // multiple possibilities, exit set loop
                *candidate = cr;                  // remember the hit
            }
        }
        if ( 1 == n_hits ) return mask;
    }
    return -1;
}

/* Indicating a hidden single hint is done by changing the
   current selection to the location of the hidden single,
   and by coloring the cells that trigger the hint (all cells
   with the hidden single symbol in other rows and columns
   intersecting the column, row or box where the hidden symbol
   is found). Those triggers prevent all cells other than the
   hidden single symbol cell in the same column, row or box to
   have the hidden single symbol */

static void set_row_triggers( int row, int col, int mask, hint_desc_t *hdesc )
{
    int box = get_surrounding_box( row, col );
    int other_boxes[2];
    get_other_boxes_in_same_box_row( box, other_boxes );
    cell_ref_t single, cr[3];

    for ( int i = 0; i < 2; ++i ) { // do first the 2 other aligned boxes
        get_box_row_intersection( other_boxes[i], row, cr );
        bool checked = false;
        for ( int j = 0; j < 3; ++j ) { // for each cell in the box-row intersection
            if ( is_single_ref( &cr[j] ) ) continue; // no need for any trigger

            if ( ! checked &&           // avoid checking the same box again
                 get_single_for_mask_in_set( LOCATE_BY_BOX, other_boxes[i], mask, &single ) ) {
                hint_desc_add_cell_ref_trigger( hdesc, &single, REGULAR_TRIGGER );
                break;                  // no need for other trigger in the same box
            }
            checked = true;         // do not look in the box anymore
            if ( get_single_for_mask_in_set( LOCATE_BY_COL, cr[j].col, mask, &single ) ) {
                hint_desc_add_cell_ref_trigger( hdesc, &single, REGULAR_TRIGGER );
            } else {                    // a 'weak' trigger?
                hint_desc_add_cell_ref_trigger( hdesc, &cr[j], WEAK_TRIGGER | PENCIL );
            }                           // keep looking for other cells in intersection
        }
    }

    get_box_row_intersection( box, row, cr );
    for ( int j = 0; j < 3; ++j ) {
        if ( cr[j].col == col ) continue;
        if ( is_single_ref( &cr[j] ) ) continue; // no need for any trigger
        if ( get_single_for_mask_in_set( LOCATE_BY_COL, cr[j].col, mask, &single ) ) {
            hint_desc_add_cell_ref_trigger( hdesc, &single, REGULAR_TRIGGER );
        } else {
            hint_desc_add_cell_ref_trigger( hdesc, &cr[j], WEAK_TRIGGER | PENCIL );
        }
    }
}

//TODO: it should be possible to combine set_row_triggers & set_col_triggers into a single function
static void set_col_triggers( int row, int col, int mask, hint_desc_t *hdesc )
{
    int box = get_surrounding_box( row, col );
    int other_boxes[2];
    get_other_boxes_in_same_box_col( box, other_boxes );
    cell_ref_t single, cr[3];

    for ( int i = 0; i < 2; ++i ) {     // do first the 2 other aligned boxes
        get_box_col_intersection( other_boxes[i], col, cr );
        bool checked = false;
        for ( int j = 0; j < 3; ++j ) { // for each cell in box-col intersection
            if ( is_single_ref( &cr[j] ) ) continue; // no need for any trigger

            if ( ! checked &&           // skip checking the same box again
                 get_single_for_mask_in_set( LOCATE_BY_BOX, other_boxes[i], mask, &single ) ) {
                hint_desc_add_cell_ref_trigger( hdesc, &single, REGULAR_TRIGGER );
                break;                  // no need for other trigger in the same box
            }
            checked = true;             // do not look in the same box anymore
            if ( get_single_for_mask_in_set( LOCATE_BY_ROW, cr[j].row, mask, &single ) ) {
                hint_desc_add_cell_ref_trigger( hdesc, &single, REGULAR_TRIGGER );
            } else {                    // a 'weak' trigger?
                hint_desc_add_cell_ref_trigger( hdesc, &cr[j], WEAK_TRIGGER | PENCIL );
            }                           // keep looking for other cells in intersection
        }
    }

    get_box_col_intersection( box, col, cr );
    for ( int j = 0; j < 3; ++j ) {
        if ( cr[j].row == row ) continue;
        if ( is_single_ref( &cr[j] ) ) continue; // no need for any trigger
        if ( get_single_for_mask_in_set( LOCATE_BY_ROW, cr[j].row, mask, &single ) ) {
            hint_desc_add_cell_ref_trigger( hdesc, &single, REGULAR_TRIGGER );
        } else {
            hint_desc_add_cell_ref_trigger( hdesc, &cr[j], WEAK_TRIGGER | PENCIL );
        }
    }
}

typedef struct {
    int row;
    int col_map;    // col map (cols and box row intersection that are not singles)
    int trigger;    // col (outside the box that contains the single)
} box_row_def_t;

typedef struct {
    int col;
    int row_map;    // row map (rows and box col intersection that are not singles)
    int trigger;    // row (outside the box that contains the single)
} box_col_def_t;

static void fill_box_rows_cols( int box, int row_hint, int col_hint,
                                int *n_box_row, box_row_def_t *br,
                                int *n_box_col, box_col_def_t *bc )
{
    int n_br = 0, n_bc = 0;
    for ( int i = 0; i < SUDOKU_N_SYMBOLS; ++i ) {
        cell_ref_t cr;
        get_cell_ref_in_set( LOCATE_BY_BOX, box, i, &cr );
        if ( cr.row == row_hint && cr.col == col_hint ) continue;

        sudoku_cell_t *cell = get_cell( cr.row, cr.col );
        if ( 1 == cell->n_symbols ) continue;

        if ( cr.row != row_hint ) {                     // hint row cannot be used
            if ( n_br && br[n_br-1].row == cr.row ) {   // rows always increment in a box
                br[n_br-1].col_map |= 1 << cr.col;
            }  else {
                br[n_br].row = cr.row;
                br[n_br].col_map = 1 << cr.col;
                ++n_br;
            }
        }
        if ( cr.col != col_hint ) {                     // hint col cannot be used
            bool new_col = true;
            for ( int j = 0; j < n_bc; ++j ) {          // cols wrap around in a box
                if ( bc[j].col == cr.col ) {
                    bc[j].row_map |= 1 << cr.row;
                    new_col = false;
                    break;
                }
            } 
            if ( new_col ) {
                bc[n_bc].col = cr.col;
                bc[n_bc].row_map = 1 << cr.row;
                ++n_bc;
            }
        }
    }
    *n_box_row = n_br;
    *n_box_col = n_bc;
}

static void set_box_triggers( int box, int row_hint, int col_hint, int mask, hint_desc_t *hdesc )
{
    int first_row = 3 * ( box / 3 );
    int first_col = 3 * ( box % 3 );

    box_row_def_t trigger_rows[3];
    box_col_def_t trigger_cols[3];
    int trow, tcol;

    fill_box_rows_cols( box, row_hint, col_hint, &trow, trigger_rows, &tcol, trigger_cols );
//debug
    printf( "Hidden single: %d trigger_rows, %d trigger_cols\n", trow, tcol );

    // validate rows that do have a trigger
    for ( int i = 0; i < trow; ++i ) {
        trigger_rows[i].trigger = -1;
        for ( int c = 0; c < SUDOKU_N_COLS; ++c ) {
            if ( c >= first_col && c < first_col + 3 ) continue;
            sudoku_cell_t *cell = get_cell( trigger_rows[i].row, c );
            if ( 1 == cell->n_symbols && ( mask & cell->symbol_map ) ) {
                trigger_rows[i].trigger = c;
                printf( "Validate: trigger_rows[%d].row=%d, trigger=%d\n",
                        i, trigger_rows[i].row, trigger_rows[i].trigger );
                break;
            }
        }
    }
    for ( int j = 0; j < tcol; ++j ) {
        trigger_cols[j].trigger = -1;
        for ( int r = 0; r < SUDOKU_N_ROWS; ++r ) {
            if ( r >= first_row && r < first_row + 3 ) continue;
            sudoku_cell_t *cell = get_cell( r, trigger_cols[j].col );
            if ( 1 == cell->n_symbols && ( mask & cell->symbol_map ) ) {
                trigger_cols[j].trigger = r;
                printf( "Validate: trigger_cols[%d].col=%d, trigger=%d\n",
                        j, trigger_cols[j].col, trigger_cols[j].trigger );
                break;
            }
        }
    }

    // remove redundant triggers
    for ( int i = 0; i < trow; ++i ) {
        if ( -1 == trigger_rows[i].trigger ) continue;
        for ( int j = 0; j < tcol; ++ j ) {
            if ( -1 == trigger_cols[j].trigger ) continue;
            printf( "Cleanup: trigger_rows[%d].row=%d, trigger_cols[%d].row_map=0x%03x trigger=%d\n",
                    i, trigger_rows[i].row, j, trigger_cols[j].row_map, trigger_cols[j].trigger );
            trigger_cols[j].row_map &= ~(1 << trigger_rows[i].row);
            if ( 0 == trigger_cols[j].row_map ) {
                trigger_cols[j].trigger = -1;
                printf( "After removing: trigger_cols[j].row_map=0x%03x, trigger=%d\n",
                        trigger_cols[j].row_map, trigger_cols[j].trigger );
            }
        }
    }
    for ( int j = 0; j < tcol; ++j ) {
        if ( -1 == trigger_cols[j].trigger ) continue;
        for ( int i = 0; i < trow; ++i ) {
            if ( -1 == trigger_rows[i].trigger ) continue;
            printf( "Cleanup: trigger_cols[%d].col=%d, trigger_rows[%d].col_map=0x%03x trigger=%d\n",
                    j, trigger_cols[j].col, i, trigger_rows[i].col_map, trigger_rows[i].trigger );
            trigger_rows[i].col_map &= ~(1 << trigger_cols[j].col);
            if ( 0 == trigger_rows[i].col_map ) {
                trigger_rows[i].trigger = -1;
                printf( "After removing: trigger_rows[i].row_map=0x%03x, trigger=%d\n",
                        trigger_rows[i].col_map, trigger_rows[i].trigger );
            }
        }
    }

    for ( int i = 0; i < trow; ++i ) {
        if ( -1 == trigger_rows[i].trigger ) continue;
//printf("Setting trigger (%d, %d)\n", trigger_rows[i].row, trigger_rows[i].trigger );
        hint_desc_add_row_col_trigger( hdesc, trigger_rows[i].row, trigger_rows[i].trigger, REGULAR_TRIGGER );
    }
    for ( int j = 0; j < tcol; ++j ) {
        if ( -1 == trigger_cols[j].trigger ) continue;
//printf("Setting trigger (%d, %d)\n", trigger_cols[j].trigger, trigger_cols[j].col );
        hint_desc_add_row_col_trigger( hdesc, trigger_cols[j].trigger, trigger_cols[j].col, REGULAR_TRIGGER );
    }
}

static void set_hidden_single_triggers( locate_t by, int set, cell_ref_t *cr, int mask, hint_desc_t *hdesc )
{
    switch ( by ) {
    case LOCATE_BY_ROW:
        set_row_triggers( set, cr->col, mask, hdesc );
        break;
    case LOCATE_BY_COL:
        set_col_triggers( cr->row, set, mask, hdesc );
        break;
    case LOCATE_BY_BOX:
        set_box_triggers( set, cr->row, cr->col, mask, hdesc );
        break;
    }
}

extern bool look_for_hidden_singles( hint_desc_t *hdesc )
{
    for ( int by = LOCATE_BY_BOX; by >= LOCATE_BY_ROW; --by ) {
        for ( int set = 0; set < SUDOKU_N_SYMBOLS; ++set ) {
            cell_ref_t candidate;
            int mask = check_only_possible_symbols_in_set( (locate_t)by, set, &candidate );
            if ( -1 != mask ) {
                set_hidden_single_triggers( by, set, &candidate, mask, hdesc );

                hint_desc_add_cell_ref_hint( hdesc, &candidate );
                hdesc->selection = candidate;
                hdesc->hint_type = HIDDEN_SINGLE;
                hdesc->action = SET;
                hdesc->n_symbols = 1;
                hdesc->symbol_map = mask;
                return true;
            }
        }
    }
    return false;
}

