/*
  Sudoku hint finder
*/
#include <string.h>

#include "hint.h"
#include "gen.h"
#include "state.h"
#include "stack.h"

extern int get_number_from_map( unsigned short map )
{
    static 
    char s_mapping[ ] = {                             // speedup a bit with a table
    -1, 0, 1,-1, 2,-1,-1,-1, 3,-1,-1,-1,-1,-1,-1,-1,  // 0, 1:0, 2:1, 3, 4:2, 5..7, 8:3, 9..15
     4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 16:4,17..31
     5,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 32:5, 33..47
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 48..63, 63
     6,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 64:6, 65..79
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 80..95
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 96..111
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 112..127
     7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 128:7, 129..143
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 144..159
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 160..175
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 176..191
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 192..207
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 208..223
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 224..239
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 244..255
     8                                                // 256:8
    };

  if ( map > 256 ) return -1;
  return s_mapping[ map ];
}

extern char sudoku_get_symbol( sudoku_cell_t *cell )
{
    int val = get_number_from_map( cell->symbol_map );
    if ( -1 != val ) return '1' + val;
    return ' ';
}

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

static void get_other_boxes_in_same_box_row( int box, int *other_boxes )
{
    int box_row = 3 * (box / 3);    // [0,3,6]
    switch( box % 3 ) {             // [0,1,2]
    case 0:
        other_boxes[0] = box_row + 1;
        other_boxes[1] = box_row + 2;
        break;
    case 1:
        other_boxes[0] = box_row;
        other_boxes[1] = box_row + 2;
        break;
    case 2:
        other_boxes[0] = box_row;
        other_boxes[1] = box_row + 1;
        break;
    }
}

static void get_other_boxes_in_same_box_col( int box, int *other_boxes )
{
    int col_diff = box % 3;         // [0,1,2]
    switch( box / 3 ) {             // [0,1,2]
    case 0:
        other_boxes[0] = col_diff + 3;
        other_boxes[1] = col_diff + 6;
        break;
    case 1:
        other_boxes[0] = col_diff;
        other_boxes[1] = col_diff + 6;
        break;
    case 2:
        other_boxes[0] = col_diff;
        other_boxes[1] = col_diff + 3;
        break;
    }
}

typedef struct {
    int row, col;
} cell_ref_t;

static inline bool is_cell_ref_in_box( int box, cell_ref_t *cr )
{
    return box == get_surrounding_box( cr->row, cr->col );
}

typedef enum { LOCATE_BY_ROW, LOCATE_BY_COL, LOCATE_BY_BOX } locate_t;

static void get_cell_ref_in_set( locate_t by, int ref, int index, cell_ref_t *cr )
// by is LOCATE_BY_ROW, LOCATE_BY_COL or LOCATE_BY_BOX.
// ref is row, col or box id [0..8], index is col, row or cell index in box [0..8].
{
    assert( LOCATE_BY_ROW <= by && LOCATE_BY_BOX >= by );
    assert( ref >= 0 && ref < SUDOKU_N_SYMBOLS );
    assert( index >= 0 && index < SUDOKU_N_SYMBOLS );

    switch( by ) {
    case LOCATE_BY_ROW:
        cr->row = ref;
        cr->col = index;
        break;
    case LOCATE_BY_COL:
        cr->row = index;
        cr->col = ref;
        break;
    case LOCATE_BY_BOX:
        cr->row = (3 * ( ref / 3 )) + (index / 3);
        cr->col = (3 * ( ref % 3 )) + (index % 3);
        break;
    }
}

static void get_box_row_intersection( int box, int row, cell_ref_t *intersection )
// return 3 cells at the intersection of box and row if they do intersect
{
    int first_row = 3 * ( box / 3 );
    int first_col = 3 * ( box % 3 );

    if ( row >= first_row && row < first_row + 3 ) {
        int index = 0;
        for ( int c = first_col; c < first_col + 3; ++ c ) {
            intersection[index].row = row;
            intersection[index].col = c;
            ++index;
        }
    }
}

static void get_box_col_intersection( int box, int col, cell_ref_t *intersection )
// return 3 cells at the intersection of box and col if they do intersect
{
    int first_row = 3 * ( box / 3 );
    int first_col = 3 * ( box % 3 );

    if ( col >= first_col && col < first_col + 3 ) {
        int index = 0;
        for ( int r = first_row; r < first_row + 3; ++r ) {
            intersection[index].row = r;
            intersection[index].col = col;
            ++index;
        }
    }
}


struct _cache_cell {
    int    row, col;
    hint_e hint;
};
/* implement a hint cache with room for all cells intersecting
   row, col and box, that is:
    - in 1 row at most (SUDOKU_N_SYMBOLS), plus
    - in 1 col at most (SUDOKU_N_SYMBOLS), plus
    - in 1 box at most (SUDOKU_N_SYMBOLS)
*/
static struct _cache_cell hint_cache[ 3 * SUDOKU_N_SYMBOLS ];
static int                hint_cache_size;

static void cache_hint_cell( int row, int col, hint_e hint )
{
    hint_cache[hint_cache_size].row = row;
    hint_cache[hint_cache_size].col = col;
    hint_cache[hint_cache_size++].hint = hint;
}
#if 0
static int get_hint_cache_size( void )
{
    return hint_cache_size;
}
#endif
static void reset_hint_cache( void )
{
    hint_cache_size = 0;
}

static void write_cache_hints( void )
{
    for ( int i = 0; i < hint_cache_size; ++i ) {
        set_cell_hint( hint_cache[i].row, hint_cache[i].col, hint_cache[i].hint );
    }
}

typedef enum {
    NONE, SET, REMOVE, ADD
} hint_action_t;

typedef struct {
    sudoku_hint_type    hint_type;
    int                 n_hints;            // where symbols can be removed
    int                 n_triggers;         // where other symbols trigger hints/candidates
    int                 n_weak_triggers;    // where previous reduction created a trigger
    int                 n_candidates;       // where symbol could be placed 

    bool                hint_pencil;        // whether to show penciled symbols in hint cells
    bool                trigger_pencil;     // whether to show penciled symbols in trigger cells
    
    cell_ref_t          hints[ SUDOKU_N_SYMBOLS ];  // TODO: check how many are actually needed
    cell_ref_t          triggers[ SUDOKU_N_SYMBOLS ];
    cell_ref_t          weak_triggers[ SUDOKU_N_SYMBOLS ];
    cell_ref_t          candidates[ SUDOKU_N_SYMBOLS ];

    cell_ref_t          selection;

    hint_action_t       action;
    int                 n_symbols;  // only for SET
    int                 symbol_map;
} hint_desc_t;

static void init_hint_desc( hint_desc_t *hdesc )
{
    memset( hdesc, 0, sizeof(hint_desc_t) );
    hdesc->selection.row = hdesc->selection.col = -1;   // no selection by default
}

static void cache_hint_from_desc( hint_desc_t *hdesc )
{
    printf("cache_hint_from_desc:\n");
    printf(" hint_type %d\n", hdesc->hint_type);
    printf(" n_hints %d\n", hdesc->n_hints);
    printf(" n_triggers %d\n", hdesc->n_triggers);
    printf(" n_weak_triggers %d\n", hdesc->n_weak_triggers);

    int cell_attrb = ( hdesc->hint_pencil ) ? HINT_REGION | PENCIL : HINT_REGION;
    for ( int i = 0; i < hdesc->n_hints; ++i ) {
        cache_hint_cell( hdesc->hints[i].row, hdesc->hints[i].col, cell_attrb );
        printf(" hints[%d] = (%d, %d)\n", i, hdesc->hints[i].row, hdesc->hints[i].col);
    }
    cell_attrb = ( hdesc->trigger_pencil ) ? TRIGGER_REGION | PENCIL : TRIGGER_REGION;
    for ( int i = 0; i < hdesc->n_triggers; ++i ) {
        cache_hint_cell( hdesc->triggers[i].row, hdesc->triggers[i].col, cell_attrb );
        printf(" triggers[%d] = (%d, %d)\n", i, hdesc->triggers[i].row, hdesc->triggers[i].col);
    }
    for ( int i = 0; i < hdesc->n_weak_triggers; ++i ) {
        cache_hint_cell( hdesc->weak_triggers[i].row, hdesc->weak_triggers[i].col, WEAK_TRIGGER_REGION | PENCIL );
        printf(" weak triggers[%d] = (%d, %d)\n", i, hdesc->weak_triggers[i].row, hdesc->weak_triggers[i].col);
    }
    for ( int i = 0; i < hdesc->n_candidates; ++i ) {
        cache_hint_cell( hdesc->candidates[i].row, hdesc->candidates[i].col, ALTERNATE_TRIGGER_REGION | PENCIL );
    }

    printf(" selection (%d, %d) \n", hdesc->selection.row, hdesc->selection.col);
    printf(" action %d n_symbols=%d, map=0x%03x\n", hdesc->action, hdesc->n_symbols, hdesc->symbol_map);
}

/* 1. looking for naked singles - note that this must be done first
   in order to remove all known single symbols from other cells. */

static bool remove_symbol( int row, int col, int remove_mask )
{
    sudoku_cell_t *cell = get_cell( row, col );

//    if ( cell->n_symbols >= 1 && (cell->symbol_map & remove_mask) ) {
    if ( cell->symbol_map & remove_mask ) {
        SUDOKU_ASSERT ( cell->n_symbols > 1 );      // in theory not possible
        cell->symbol_map &= ~remove_mask;           // remove single symbol
        if ( 1 == --cell->n_symbols ) return true;  // found a new naked single
    }
    return false;
}

static bool check_box_of( int row, int col, int remove_mask, 
			              int *row_hint, int *col_hint )
{
    int first_row = row - (row % 3);
    int first_col = col - (col % 3);

    for ( int r = first_row; r < first_row + 3; ++r ) {
        for ( int c = first_col; c < first_col + 3; ++c ) {
            if ( r == row && c == col ) continue;   // don't check itself

            if ( remove_symbol( r, c, remove_mask ) ) {
                *row_hint = r;
                *col_hint = c; 
                return true;
            }
        }
    }
    return false;
}

static bool check_row_of( int row, int col, int remove_mask, int *col_hint )
{
    int first_col = col - (col % 3); // assuming box has been or will be processed for the same mask
    for ( int c = 0; c < SUDOKU_N_COLS; c++ ) {
        if ( c >= first_col && c < first_col + 3 ) continue;

        if ( remove_symbol( row, c, remove_mask ) ) {
            *col_hint = c;
            return true;
        }
    }
    return false;
}

static int check_col_of( int row, int col, int remove_mask, int *row_hint )
{
    int first_row = row - (row % 3); // assuming box has been or will be processed for the same mask
    for ( int r = 0; r < SUDOKU_N_ROWS; r++ ) {
        if ( r >= first_row && r < first_row + 3 ) continue;

        if ( remove_symbol( r, col, remove_mask ) ) {
            *row_hint = r;
            return true;
        }
    }
    return false;
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
            hdesc->triggers[hdesc->n_triggers].row = row;
            hdesc->triggers[hdesc->n_triggers].col = col;
            ++hdesc->n_triggers;
        } else {
            hdesc->selection.row = row;
            hdesc->selection.col = col;
            hdesc->hints[hdesc->n_hints].row = row;
            hdesc->hints[hdesc->n_hints].col = col;
            ++hdesc->n_hints;
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

static bool look_for_naked_singles( hint_desc_t *hdesc )
{
    for ( int col = 0; col < SUDOKU_N_COLS; col++ ) {
        for ( int row = 0; row < SUDOKU_N_ROWS; row++ ) {
            sudoku_cell_t *current_cell = get_cell( row, col );
            if ( 1 == current_cell->n_symbols ) {
                int remove_mask = current_cell->symbol_map;

                int row_hint, col_hint;
                if ( check_box_of( row, col, remove_mask, &row_hint, &col_hint ) ) {
                    return set_naked_single_hint_desc( row_hint, col_hint, remove_mask, hdesc );
                }
                if ( check_col_of( row, col, remove_mask, &row_hint ) ) {
                    return set_naked_single_hint_desc( row_hint, col, remove_mask, hdesc );
                }
                if ( check_row_of( row, col, remove_mask, &col_hint ) ) {
                    return set_naked_single_hint_desc( row, col_hint, remove_mask, hdesc );
                }
            }
        }
    }
    return false;
}

/* 2. Look for hidden singles. This must be done after looking for
   naked singles in order to benefit from the pencil clean up. */

static bool get_single_in_set( locate_t by, int ref, int single_mask, cell_ref_t *single )
{
    for ( int i = 0; i < SUDOKU_N_SYMBOLS; ++i ) {
        cell_ref_t cr;
        get_cell_ref_in_set( by, ref, i, &cr );
        sudoku_cell_t *cell = get_cell( cr.row, cr.col );
        if ( single_mask == cell->symbol_map ) {
            *single = cr;
            return true;
        }
    }
    return false;
}

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

static void set_hidden_single_trigger( cell_ref_t *cr, bool weak, hint_desc_t *hdesc )
{
    if ( weak ) {
        hdesc->weak_triggers[hdesc->n_weak_triggers++] = *cr;
        ++hdesc->n_weak_triggers;
    } else {
        hdesc->triggers[hdesc->n_triggers++] = *cr;
        ++hdesc->n_triggers;
    }
}

static inline bool is_single_ref( cell_ref_t *cr )
{
    sudoku_cell_t *cell = get_cell( cr->row, cr->col );
    return 1 == cell->n_symbols;
}

static void set_row_hint( int row, int col, int mask, hint_desc_t *hdesc )
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
                 get_single_in_set( LOCATE_BY_BOX, other_boxes[i], mask, &single ) ) {
                hdesc->triggers[hdesc->n_triggers++] = single;
                break;                  // no need for other trigger in the same box
            }
            checked = true;         // do not look in the box anymore
            if ( get_single_in_set( LOCATE_BY_COL, cr[j].col, mask, &single ) ) {
                hdesc->triggers[hdesc->n_triggers++] = single;
            } else {                    // a 'weak' trigger?
                hdesc->weak_triggers[hdesc->n_weak_triggers++] = cr[j];
            }                           // keep looking for other cells in intersection
        }
    }

    get_box_row_intersection( box, row, cr );
    for ( int j = 0; j < 3; ++j ) {
        if ( cr[j].col == col ) continue;
        if ( is_single_ref( &cr[j] ) ) continue; // no need for any trigger
        if ( get_single_in_set( LOCATE_BY_COL, cr[j].col, mask, &single ) ) {
            hdesc->triggers[hdesc->n_triggers++] = single;
        } else {
            hdesc->weak_triggers[hdesc->n_weak_triggers++] = cr[j];
        }
    }
}

static void set_col_hint( int col, int row, int mask, hint_desc_t *hdesc )
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
                 get_single_in_set( LOCATE_BY_BOX, other_boxes[i], mask, &single ) ) {
                hdesc->triggers[hdesc->n_triggers++] = single;
                break;                  // no need for other trigger in the same box
            }
            checked = true;             // do not look in the same box anymore
            if ( get_single_in_set( LOCATE_BY_ROW, cr[j].row, mask, &single ) ) {
                hdesc->triggers[hdesc->n_triggers++] = single;
            } else {                    // a 'weak' trigger?
                hdesc->weak_triggers[hdesc->n_weak_triggers++] = cr[j];
            }                           // keep looking for other cells in intersection
        }
    }

    get_box_col_intersection( box, col, cr );
    for ( int j = 0; j < 3; ++j ) {
        if ( cr[j].row == row ) continue;
        if ( is_single_ref( &cr[j] ) ) continue; // no need for any trigger
        if ( get_single_in_set( LOCATE_BY_ROW, cr[j].row, mask, &single ) ) {
            hdesc->triggers[hdesc->n_triggers++] = single;
        } else {
            hdesc->weak_triggers[hdesc->n_weak_triggers++] = cr[j];
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

static void set_box_hint( int box, int row_hint, int col_hint, int mask, hint_desc_t *hdesc )
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
printf("Setting trigger (%d, %d)\n", trigger_rows[i].row, trigger_rows[i].trigger );
        hdesc->triggers[hdesc->n_triggers].row = trigger_rows[i].row;
        hdesc->triggers[hdesc->n_triggers++].col = trigger_rows[i].trigger;
//        cache_hint_cell( trigger_rows[i].row, trigger_rows[i].trigger, TRIGGER_REGION );
    }
    for ( int j = 0; j < tcol; ++j ) {
        if ( -1 == trigger_cols[j].trigger ) continue;
printf("Setting trigger (%d, %d)\n", trigger_cols[j].trigger, trigger_cols[j].col );
        hdesc->triggers[hdesc->n_triggers].row = trigger_cols[j].trigger;
        hdesc->triggers[hdesc->n_triggers++].col = trigger_cols[j].col;
//        cache_hint_cell( trigger_cols[j].trigger, trigger_cols[j].col, TRIGGER_REGION );
    }
}

static void set_hidden_single_hint( locate_t by, int set, cell_ref_t *cr, int mask, hint_desc_t *hdesc )
{
    switch ( by ) {
    case LOCATE_BY_ROW:
        set_row_hint( set, cr->col, mask, hdesc );
        break;
    case LOCATE_BY_COL:
        set_col_hint( set, cr->row, mask, hdesc );
        break;
    case LOCATE_BY_BOX:
        set_box_hint( set, cr->row, cr->col, mask, hdesc );
        break;
    }
}

static bool look_for_hidden_singles( hint_desc_t *hdesc )
{
    for ( int by = LOCATE_BY_BOX; by >= LOCATE_BY_ROW; --by ) {
        for ( int set = 0; set < SUDOKU_N_SYMBOLS; ++set ) {
            cell_ref_t candidate;
            int mask = check_only_possible_symbols_in_set( (locate_t)by, set, &candidate );
            if ( -1 != mask ) {
                set_hidden_single_hint( by, set, &candidate, mask, hdesc );

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

/*
   3. Look for locked candicates (similar to hidden single but for multiple penciled symbols).
        More precisely :
        1) 1 row where a symbol p (Sp) can fit only in 2 or 3 cells in 1 box, because
            a) all other cells in the same row outside the box are other singles (Sa, Sb, ... Sf).
                Sa Sb Sc |  C  C  C | Sd Se Sf   in this row Sp can only appear in one of 3 cells marked
                         |  X  X  X |            with C in the 2nd box, forbidding Sp in other rows of
                         |  X  X  X |            the same box (X). Cells C are candidates, X are hints.
                ------------------------------
            b) a single with the same symbol (Sp) appears in columns of some other boxes, a.k.a. triggers.
                Sa !p !p |  C  C Sb | Sc !p Sd   in this row Sp can only appear in one of 2 cells marked
                   !p !p |  X  X  X |    !p      with C in the 2nd box, forbidding Sp in other rows of
                   !p !p |  X  X  X |    !p      the same box (X). Cells C are candidates, X are hints,
                ------------------------------
                    T !p |          |    !p      and T are triggers.
                   !p !p |          |    !p
                   !p !p |          |    !p
                ------------------------------
                   !p !p |          |     T
                   !p !p |          |    !p
                   !p  T |          |    !p
            c) the same symbol (Sp) appear in other rows (outside the box) as triggers (T).
                Sa !p Sb |  C  C Sc | Sd !p Se  A trigger T forbids other cells in each box to hold
                 T !p !p | !p !p !p | !p !p !p  SP (indicated as !p), and it forbids other cells in
                !p !p !p | !p !p !p | !p  T !p  the same row to hold SP.

                It does not bring any new hint in case c) alone, but in case of b+c or a+c, such as:
                Sa Sb Sc |  p  p  p | !p Sd !p  in this row, Sp can appear only in one of the 3 cells
                !p !p !p | !p !p !p | !p !p Sp  maked p in the second box, forbidding Sp in the 3rd
                 p  p  p |  X  X  X | !p !p !p  row of the same box.
                ------------------------------
                Would still provide a good hint (X) on the same box.

        2) 1 box where a symbol can appear only in 2 or 3 cells in 1 row within that box, because
            the same symbol (T) appears in 1 other other box in a row intersecting the box AND in yet
            another box in a column intersecting a row outside the first box:
                 X  X  X |  p  p !p | !p !p !p  A trigger T forbids other cells to hold Sp (indicated
                !p !p !p | !p !p !p | !p !p  T  as !p) in the same row. Another trigger T in a column
                 C  C  C | Sa Sb !p | !p !p !p  prevents the remaining cell in the row to hold Sp.
                ------------------------------
                         |        T |

         Rule for 1 & 2: find a row where a penciled symbol can occur only within 1 box (2 or 3 cells)
                          => the same symbol can be removed from the other cells in that box.
                          hint only if the symbol can be found in the other cells of the box.

        3) 1 column where a symbol can fit only in 2 or 3 cells in 1 box for reasons similar to 1),
          after transposing rows and columns: locked candidate in box column

        4) 1 box where a symbol can appear only in 2 or 3 cells in 1 box for reasons similar to 2).
                !p Sa  p |
                !p !p !p |  T
                !p Sb  p |
                -------------------------------
                !p  X  C |
                !p  X  C |
                !p  X  C |
                -------------------------------
                 T !p !p |
                !p !p !p |
                !p !p !p |

         Rule for 3 & 4: find a column where a penciled symbol can occur only within 1 box (2 or 3 cells)
                          => the same symbol can be removed from the other cells in that box.
                          hint only if the symbol can be found in the other cells of the box.

        This must be done after looking for naked singles in order to benefit from the pencil clean up.
*/

static int get_singles_in_game( int symbol_mask, cell_ref_t *singles )
{
    int count = 0;
    for ( int r = 0; r < SUDOKU_N_ROWS; ++r ) {
        for ( int c = 0; c < SUDOKU_N_COLS; ++c ) {
            sudoku_cell_t * cell = get_cell( r, c );
            if ( 1 == cell->n_symbols && ( symbol_mask & cell->symbol_map ) ) {
                singles[count].row = r;
                singles[count].col = c;
                ++count;
            }
        }
    }
    return count;
}

static cell_ref_t * get_single_in_box( cell_ref_t *singles, int n_singles, int box )
{
    int box_first_row, box_first_col;
    get_box_first_row_col( box, &box_first_row, &box_first_col );

    for ( int i = 0; i < n_singles; ++ i ) {
        if ( singles[i].row >= box_first_row && singles[i].row < box_first_row + 3 &&
             singles[i].col >= box_first_col && singles[i].col < box_first_col + 3 )
            return &singles[i];
    }
    return NULL;
}

static cell_ref_t * get_single_in_row( cell_ref_t *singles, int n_singles, int row )
{
    for ( int i = 0; i < n_singles; ++ i ) {
        if ( singles[i].row == row ) return &singles[i];
    }
    return NULL;
}

static cell_ref_t * get_single_in_col( cell_ref_t *singles, int n_singles, int col )
{
    for ( int i = 0; i < n_singles; ++ i ) {
        if ( singles[i].col == col ) return &singles[i];
    }
    return NULL;
}

static int extract_bit( int *map )
{
    if ( 0 == *map ) return -1;

    for ( int i = 0; ; ++i ) {
        int mask = 1 << i;
        if ( *map & mask ) {
            *map &= ~mask;
            return i;
        }
    }
}

typedef struct {
    int n_cols;             // for each row, the number of columns in which pencil is found
    int col_map;            // for each row, the candidate column map: 1 bit per column
} candidate_row_location_t; // candidates in the row (up to 9 for the whole row, or 3 if within a box)

typedef struct {
    candidate_row_location_t candidates[3]; // 1 in each intersection of box and row
} candidate_box_row_location_t;             // candidates in the whole row, split in 3 boxes.

static void get_locations_in_horizontal_boxes_with_pencil( int first_row, int pencil_map,
                                                           candidate_box_row_location_t *crloc )
// crloc is an array of 3 candidate_row_location_t structures, giving the
// columns where the penciled symbol is present in each horizontal box [0..2]
{
    for ( int r = 0; r < 3; ++r ) {     // 3 consecutive rows
        for ( int b = 0; b < 3; ++b ) { // 3 horizontal boxes intersecting each row
            crloc[r].candidates[b].n_cols = 0;
            crloc[r].candidates[b].col_map = 0;
        }

        for ( int c = 0; c < 9; ++c ) {
            sudoku_cell_t * cell = get_cell( r + first_row, c );
            if ( 1 < cell->n_symbols && ( pencil_map & cell->symbol_map ) ) {
                ++crloc[r].candidates[c/3].n_cols;
                crloc[r].candidates[c/3].col_map |= 1 << c; // actual column id
            }
        }
    }
}

typedef struct {
    int n_rows;             // for each column, the number of rows in which pencil is found
    int row_map;            // for each box column, the candidate row map: 1 bit per row
} candidate_col_location_t; // candidates in the col (up to 8 rows for the whole col, or 3 if within a box)

typedef struct {
    candidate_col_location_t candidates[3]; // 1 in each intersection of box and column
} candidate_box_col_location_t;             // candidates in the whole column, split in 3 boxes.

static void get_locations_in_vertical_boxes_with_pencil( int first_col, int pencil_map,
                                                         candidate_box_col_location_t *ccloc )
// ccloc is an array of 3 candidate_box_col_location_t structures, giving the
// rows where the penciled symbol is present in each vertical box [0..2]
{
    for ( int c = 0; c < 3; ++c ) {     // 3 consecutive columns
        for ( int b = 0; b < 3; ++b ) { // 3 vertical boxes intersecting each column
            ccloc[c].candidates[b].n_rows = 0;
            ccloc[c].candidates[b].row_map = 0;
        }

        for ( int r = 0; r < 9; ++r ) {
            sudoku_cell_t * cell = get_cell( r, c + first_col );
            if ( 1 < cell->n_symbols && ( pencil_map & cell->symbol_map ) ) {
                ++ccloc[c].candidates[r/3].n_rows;
                ccloc[c].candidates[r/3].row_map |= 1 << r; // actual row id
            }
        }
    }
}

static void fill_in_box_triggers( int first_row, int first_col,
                                  cell_ref_t *singles, int n_singles, hint_desc_t *hdesc )
{
    int n_triggers = 0;    // singles triggering hint (up to 7)
    for ( int i = 0; i < n_singles; ++i ) {
        if ( singles[i].row >= first_row && singles[i].row < first_row + 3 ) {
            for ( int c = first_col; c < first_col + 3; ++ c ) {
                bool skip = false;
                for ( int j = 0; j < n_triggers; ++j ) {
                    if ( hdesc->triggers[j].col == c ) {
                        skip = true;
                        break;
                    }
                }
                if ( skip ) continue;
                sudoku_cell_t * cell = get_cell( singles[i].row, c );
                if ( 1 == cell->n_symbols ) continue;

                hdesc->triggers[n_triggers] = singles[i];
                ++n_triggers;
                break;
            }
        } else if ( singles[i].col >= first_col && singles[i].col < first_col + 3 ) {
            for ( int r = first_row; r < first_row + 3; ++ r ) {
                bool skip = false;
                for ( int j = 0; j < n_triggers; ++j ) {
                    if ( hdesc->triggers[j].row == r ) {
                        skip = true;
                        break;
                    }
                }
                if ( skip ) continue;
                sudoku_cell_t * cell = get_cell( r, singles[i].col );
                if ( 1 == cell->n_symbols ) continue;

                hdesc->triggers[n_triggers] = singles[i];
                ++n_triggers;
                break;
            }
        }
    }
    hdesc->n_triggers = n_triggers;
}

static void fill_in_box_row_triggers( int box, int row, cell_ref_t *singles, int n_singles, hint_desc_t *hdesc )
{   // Make sure fill_in_box_row_triggers is called after fill_in_row_candidates
    // singles triggering hint (up to 7)
    int boxes[3];                                   // indirect box references
    bool required_in_box[3] = { true, true, true }; // triggers required for every col in every box

    get_other_boxes_in_same_box_row( box, boxes );  // get 2 other horizontally aligned boxes
    boxes[2] = box;                                 // last box is the hint box

    int n_triggers = 0, n_weak_triggers = 0;
    for ( int i = 0; i < 2; ++i ) {                 // first, triggers inside box (i is box index)
        cell_ref_t *single = get_single_in_box( singles, n_singles, boxes[i] );
        if ( NULL == single ) continue;

        hdesc->triggers[n_triggers] = *single;
        ++n_triggers;
        required_in_box[i] = false;                 // no more trigger for this box
    }

    for( int i = 0; i < 3; ++i ) {                  // second phase, triggers outside each box
        if ( ! required_in_box[i] ) continue;

        int box_first_col = 3 * ( boxes[i] % 3 );
        for ( int c = box_first_col; c < box_first_col + 3; ++ c ) {

            sudoku_cell_t * cell = get_cell( row, c );
            if ( 1 == cell->n_symbols ) continue;   // single, no need for trigger

            cell_ref_t *single = get_single_in_col( singles, n_singles, c );
            if ( single ) {
                hdesc->triggers[n_triggers] = *single;
                ++n_triggers;
            } else {
                int required = true;                // by default a weak trigger is required
                if ( 2 == i ) {                     // if hint box it may be instead a candidate
                    for ( int j = 0; j < hdesc->n_candidates; ++ j ) {
                        if ( hdesc->candidates[j].row == row && hdesc->candidates[j].col == c ) {
                            required = false;       // a candidate: weak trigger is not required
                            break;
                        }
                    }
                }
                if ( required ) {
                    hdesc->weak_triggers[n_weak_triggers].row = row;
                    hdesc->weak_triggers[n_weak_triggers].col = c;
                    ++n_weak_triggers;
                }
            }
        }
    }
    hdesc->n_weak_triggers = n_weak_triggers;
    hdesc->n_triggers = n_triggers;
}

static void fill_in_row_candidates( int row, candidate_row_location_t *cbrloc, hint_desc_t *hdesc )
{
    // candidate locations in row (up to 3)
    hdesc->n_candidates = cbrloc->n_cols;
    int candidate_map = cbrloc->col_map;
    assert( hdesc->n_candidates <= 3 );

    for ( int i = 0; i < hdesc->n_candidates; ++i ) {
        hdesc->candidates[i].row = row;
        hdesc->candidates[i].col = extract_bit( &candidate_map );
    }
}

static int fill_same_box_locked_row_hints( int box_row, int locked_row, int box,
                                           candidate_box_row_location_t *crloc, hint_desc_t *hdesc )
{
    // hint locations in other rows of the same box (up to 6)
    int n_hints = 0, n_singles = 0;

    for ( int r = 0; r < 3; ++r ) {
        if ( r == locked_row ) continue;

        if ( crloc[r].candidates[box].n_cols ) {
            int hint_map = crloc[r].candidates[box].col_map;

            while ( true ) {
                int hint_col = extract_bit( &hint_map );
                if ( -1 == hint_col ) break;

                hdesc->hints[n_hints].row = box_row + r;
                hdesc->hints[n_hints].col = hint_col;

                // check if it might be a new single
                sudoku_cell_t * cell = get_cell( box_row + r, hint_col );
                if ( 2 == cell->n_symbols ) {
                    ++n_singles;
                    if ( -1 == hdesc->selection.row ) {
                        hdesc->selection.row = box_row + r;
                        hdesc->selection.col = hint_col;
                    }
                }
                ++n_hints;
            }
        }
    }
    hdesc->n_hints = n_hints;
    return n_singles;
}

static int fill_other_boxes_locked_row_hints( int box_row, int locked_row, int box,
                                              candidate_box_row_location_t *crloc, hint_desc_t *hdesc )
{
    // hint locations in other cells of the same locked_row (up to 6)
    int n_hints = 0, n_singles = 0;

    for ( int b = 0; b < 3; ++b ) {
        if ( b == box ) continue;

        if ( crloc[locked_row].candidates[b].n_cols ) {
            int hint_map = crloc[locked_row].candidates[b].col_map;
            if ( 0 == n_hints ) init_hint_desc( hdesc ); // new hints are coming, re-init hdesc

            while ( true ) {
                int hint_col = extract_bit( &hint_map );
                if ( -1 == hint_col ) break;

                hdesc->hints[n_hints].row = box_row + locked_row;
                hdesc->hints[n_hints].col = hint_col;

                // check if it might become a new single
                sudoku_cell_t * cell = get_cell( box_row + locked_row, hint_col );
                if ( 2 == cell->n_symbols ) {
                    ++n_singles;
                    if ( -1 == hdesc->selection.row ) {
                        hdesc->selection.row = box_row + locked_row;
                        hdesc->selection.col = hint_col;
                    }
                }
                ++n_hints;
            }
        }
    }
    if( n_hints ) { // do not modify previous hdesc settings if no new hints;
        hdesc->n_hints = n_hints;
    }
    return n_hints;
}

static inline void set_locked_candidate_hint_descriptor( hint_desc_t *hdesc, int symbol_mask )
{
    hdesc->hint_type = LOCKED_CANDIDATE;
    hdesc->action = REMOVE;
    hdesc->n_symbols = 1;
    hdesc->symbol_map = symbol_mask;
    hdesc->hint_pencil = true;
}

static int get_locked_candidates_for_box_rows( int box_row, int symbol_mask,
                                               cell_ref_t *singles, int n_singles,
                                               hint_desc_t *hdesc )
// box_row is 0 for the first 3 horizontal boxes (0, 1, 2), 3 for (3, 4, 5), 6 for (6, 7, 8)
// returns -1 if no locked candidates, 0 if some exist and > 0 if those locked candidates
// determine new single(s).
{
    candidate_box_row_location_t crloc[3];
    get_locations_in_horizontal_boxes_with_pencil( box_row, symbol_mask, crloc );
//    printf( "pencil %d, n_locations %d\n", 1 + get_number_from_map(symbol_mask), n_locations );
    for ( int r = 0; r < 3; ++r ) {
        int n_boxes_with_symbol = 0, last_box_with_symbol;
        for ( int b = 0; b < 3; ++b ) { // count number of boxes with symbol
            if ( crloc[r].candidates[b].n_cols > 0 ) {
                last_box_with_symbol = b;
                ++n_boxes_with_symbol;
            }
        }
        if ( 0 == n_boxes_with_symbol ) continue;

        if ( 1 == n_boxes_with_symbol ) { // check if symbol in other rows for same box
            int n_possible_symbol_in_other_rows = 0;
            for ( int or = 0; or < 3; ++or ) {
                if ( or == r ) continue;
                n_possible_symbol_in_other_rows += crloc[or].candidates[last_box_with_symbol].n_cols;
            }
            if ( 0 == n_possible_symbol_in_other_rows ) continue; // try other rows, other boxes

            // fill in same box hints
            init_hint_desc( hdesc );  // new hints are coming, re-init hdesc
            set_locked_candidate_hint_descriptor( hdesc, symbol_mask );
            fill_in_row_candidates( box_row + r, &crloc[r].candidates[last_box_with_symbol], hdesc );
            fill_in_box_row_triggers( box_row + last_box_with_symbol, box_row + r, singles, n_singles, hdesc );
            return fill_same_box_locked_row_hints( box_row, r, last_box_with_symbol, crloc, hdesc );

        }
        // multiple boxes with symbol in the row: check if no symbol in other rows for same box
        for ( int b = 0; b < 3; ++ b ) {
            if ( crloc[r].candidates[b].n_cols > 0 ) { // for each box with with symbols
                int rows_with_symbol = 0;
                for ( int or = 0; or < 3; ++or ) {
                    if ( or == r ) continue;
                    if ( crloc[or].candidates[b].n_cols ) ++rows_with_symbol;
                }
                if ( rows_with_symbol ) continue; // try other box

                // found box b with no symbols outside r, fill in other boxes hints
                if ( fill_other_boxes_locked_row_hints( box_row, r, b, crloc, hdesc ) ) {
                    set_locked_candidate_hint_descriptor( hdesc, symbol_mask );
                    fill_in_row_candidates( box_row + r, &crloc[r].candidates[b], hdesc );
                    fill_in_box_triggers( box_row, b * 3, singles, n_singles, hdesc );
                    return ( -1 == hdesc->selection.row ) ? 0 : 1;
                }
            }
        }
    }
    return -1;
}

static void fill_in_box_col_triggers( int box, int col, cell_ref_t *singles, int n_singles, hint_desc_t *hdesc )
{   // Make sure fill_in_box_col_triggers is called after fill_in_col_candidates
    int boxes[3];                                   // indirect box references
    bool required_in_box[3] = { true, true, true };

    get_other_boxes_in_same_box_col( box, boxes );  // get 2 other vertically aligned boxes first
    boxes[2] = box;                                 // last box is the hint box

    int n_triggers = 0, n_weak_triggers = 0;

    for ( int i = 0; i < 2; ++i ) {                 // i is the box index
        cell_ref_t *single = get_single_in_box( singles, n_singles, boxes[i] );
        if ( NULL == single ) continue;

        hdesc->triggers[n_triggers] = *single;
        ++n_triggers;
        required_in_box[i] = false;
    }
    
    for( int i = 0; i < 3; ++i ) {
        if ( ! required_in_box[i] ) continue;

        int box_first_row = 3 * ( boxes[i] / 3 );
        for ( int r = box_first_row; r < box_first_row + 3; ++ r ) {

            sudoku_cell_t * cell = get_cell( r, col );
            if ( 1 == cell->n_symbols ) continue;

            cell_ref_t *single = get_single_in_row( singles, n_singles, r );
            if ( single ) {
                hdesc->triggers[n_triggers] = *single;
                ++n_triggers;
            } else {
                int required = true;                // by default a weak trigger is required
                if ( 2 == i ) {                     // if hint box it may be instead a candidate
                    for ( int j = 0; j < hdesc->n_candidates; ++ j ) {
                        if ( hdesc->candidates[j].row == r && hdesc->candidates[j].col == col ) {
                            required = false;       // a candidate: weak trigger is not required
                            break;
                        }
                    }
                }
                if ( required ) {
                    hdesc->weak_triggers[n_weak_triggers].row = r;
                    hdesc->weak_triggers[n_weak_triggers].col = col;
                    ++n_weak_triggers;
                }
            }
        }
    }
    hdesc->n_weak_triggers = n_weak_triggers;
    hdesc->n_triggers = n_triggers;
}

static void fill_in_col_candidates( int col, candidate_col_location_t *cbcloc, hint_desc_t *hdesc )
{
    // candidate locations in column (up to 3)
    hdesc->n_candidates = cbcloc->n_rows;
    int candidate_map = cbcloc->row_map;
    assert( hdesc->n_candidates <= 3 );

    for ( int i = 0; i < hdesc->n_candidates; ++i ) {
        hdesc->candidates[i].row = extract_bit( &candidate_map );
        hdesc->candidates[i].col = col;
    }
}

static int fill_same_box_locked_col_hints( int box_col, int locked_col, int box,
                                           candidate_box_col_location_t *ccloc, hint_desc_t *hdesc )
{
    // hint locations in other cols of the same box (up to 6)
    int n_hints = 0, n_singles = 0;

    for ( int c = 0; c < 3; ++c ) {
        if ( c == locked_col ) continue;

        if ( ccloc[c].candidates[box].n_rows ) {
            int hint_map = ccloc[c].candidates[box].row_map;

            while ( true ) {
                int hint_row = extract_bit( &hint_map );
                if ( -1 == hint_row ) break;

                hdesc->hints[n_hints].row = hint_row;
                hdesc->hints[n_hints].col = box_col + c;

                // check if it might be a new single
                sudoku_cell_t * cell = get_cell( hint_row, box_col + c );
                if ( 2 == cell->n_symbols ) {
                    ++n_singles;
                    if ( -1 == hdesc->selection.row ) {
                        hdesc->selection.row = hint_row;
                        hdesc->selection.col = box_col + c;
                    }
                }
                ++n_hints;
            }
        }
    }
    hdesc->n_hints = n_hints;
    return n_singles;
}

static int fill_other_boxes_locked_col_hints( int box_col, int locked_col, int box,
                                              candidate_box_col_location_t *ccloc, hint_desc_t *hdesc )
{
    // hint locations in other cells of the same col (up to 6)
    int n_hints = 0, n_singles = 0;

    for ( int b = 0; b < 3; ++b ) {
        if ( b == box ) continue;

        if ( ccloc[locked_col].candidates[b].n_rows ) {
            int hint_map = ccloc[locked_col].candidates[b].row_map;
            if ( 0 == n_hints ) init_hint_desc( hdesc ); // new hints are coming, re-init hdesc

            while ( true ) {
                int hint_row = extract_bit( &hint_map );
                if ( -1 == hint_row ) break;

                hdesc->hints[n_hints].row = hint_row;
                hdesc->hints[n_hints].col = box_col + locked_col;

                // check if it might become a new single
                sudoku_cell_t * cell = get_cell( hint_row, box_col + locked_col );
                if ( 2 == cell->n_symbols ) {
                    ++n_singles;
                    if ( -1 == hdesc->selection.row ) {
                        hdesc->selection.row = hint_row;
                        hdesc->selection.col = box_col + locked_col;
                    }
                }
                ++n_hints;
            }
        }
    }
    if( n_hints ) { // do not modify previous hdesc settings if no new hints;
        hdesc->n_hints = n_hints;
    }
    return n_hints;
}

static int get_locked_candidates_for_box_cols( int box_col, int symbol_mask,
                                               cell_ref_t *singles, int n_singles,
                                               hint_desc_t *hdesc )
// box_col is 0 for the first 3 vertical boxes (0, 3, 6), 3 for (1, 4, 7), 6 for (2, 5, 8)
// returns -1 if no locked candidates, 0 if some exist and > 0 if those locked candidates
// determine new single(s).
{
    candidate_box_col_location_t ccloc[3];
    get_locations_in_vertical_boxes_with_pencil( box_col, symbol_mask, ccloc );
//    printf( "pencil %d, n_locations %d\n", 1 + get_number_from_map(symbol_mask), n_locations );
    for ( int c = 0; c < 3; ++c ) {
        int n_boxes_with_symbol = 0, last_box_with_symbol;
        for ( int b = 0; b < 3; ++b ) { // count number of boxes with symbol
            if ( ccloc[c].candidates[b].n_rows > 0 ) {
                last_box_with_symbol = b;
                ++n_boxes_with_symbol;
            }
        }
        if ( 0 == n_boxes_with_symbol ) continue;

        if ( 1 == n_boxes_with_symbol ) { // check if symbol in other rows for same box
            int n_possible_symbol_in_other_cols = 0;
            for ( int oc = 0; oc < 3; ++oc ) {
                if ( oc == c ) continue;
                n_possible_symbol_in_other_cols += ccloc[oc].candidates[last_box_with_symbol].n_rows;
            }
            if ( 0 == n_possible_symbol_in_other_cols ) continue; // try other cols, other boxes

            // fill in same box hints
            init_hint_desc( hdesc );   // new hints are coming, re-init hdesc
            set_locked_candidate_hint_descriptor( hdesc, symbol_mask );
            fill_in_col_candidates( box_col + c, &ccloc[c].candidates[last_box_with_symbol], hdesc );
            fill_in_box_col_triggers( last_box_with_symbol * 3 + box_col/3, box_col + c,
                                      singles, n_singles, hdesc );
            return fill_same_box_locked_col_hints( box_col, c, last_box_with_symbol, ccloc, hdesc );

        }
        // multiple boxes with symbol in the col: check if no symbol in other cols for same box
        for ( int b = 0; b < 3; ++ b ) {
            if ( ccloc[c].candidates[b].n_rows > 0 ) { // for each box with with symbols
                int cols_with_symbol = 0;
                for ( int oc = 0; oc < 3; ++oc ) {
                    if ( oc == c ) continue;
                    if ( ccloc[oc].candidates[b].n_rows ) ++cols_with_symbol;
                }
                if ( cols_with_symbol ) continue; // try other box

                // found box b with no symbols outside c, fill in other boxes hints
                if ( fill_other_boxes_locked_col_hints( box_col, c, b, ccloc, hdesc ) ) {
                    set_locked_candidate_hint_descriptor( hdesc, symbol_mask );
                    fill_in_col_candidates( box_col + c, &ccloc[c].candidates[b], hdesc );
                    fill_in_box_triggers( b * 3, box_col, singles, n_singles, hdesc );
                    return ( -1 == hdesc->selection.row ) ? 0 : 1;
                }
            }
        }
    }
    return -1;
}

static int check_locked_candidates( hint_desc_t *hdesc )
// return 0 if no locked candidate, 1 if locked candidates and 2 if at least one hint leads to a new single
{
    int potential = 0;
    for ( int symbol = 0; symbol < SUDOKU_N_SYMBOLS; ++symbol ) {
        int symbol_mask = get_map_from_number( symbol );
        cell_ref_t singles[9];
        int n_singles = get_singles_in_game( symbol_mask, singles );

        if ( 5 == symbol )
            printf( "check_locked_candidates: ready to debug\n" );

        for ( int i = 0; i < 3; ++i ) {
            int in_rows = get_locked_candidates_for_box_rows( 3 * i, symbol_mask,
                                                              singles, n_singles, hdesc );
            if ( in_rows > 0 ) return 2;

            int in_cols = get_locked_candidates_for_box_cols( 3 * i, symbol_mask,
                                                              singles, n_singles, hdesc );
            if ( in_cols > 0 ) return 2;
            if ( in_rows == 0 || in_cols == 0 ) potential = 1;
        }
    }
    return potential;
}

/* 4. Look for naked and hidden subsets (size > 1). This must be done after looking
      for naked singles in order to benefit from the penciled-symbol cleanup. */

/*
  Generates the next combination of k indexes among n elements indexes.

  Where k is the size of the subsets to generate
  And   n the size of the original set.

  The array combination is used both to return the next 
  combination index and to remember the last combination used.
  Therefors, the combination must not be modified between calls.

  Initially combination must contain k indexes ordered as
  { 0, 1, 2, ..., k-3, k-2, k-1 }.

  The following combinations are
  { 0, 1, 2, ..., k-3, k-2, l } with k <= l < n, then
  { 0, 1, 2, ..., k-3, k-1, m } with k <= m < n, then
  { 0, 1, 2, ,,,, k-2, k-1, m } with k <= m < n, etc.

  Note that the function calculates the next subset of indexes
  without looking at the actual element array, so that the elements
  can be anything.

  Returns: TRUE if a valid combination was created,
           FALSE otherwise (no more subset available)
*/
static bool get_next_combination(int *comb, int k, int n)
{
    if ( k == n ) return false;
    assert( k > 0 && n > 0 && k < n );

    int i = k - 1;     // i is the last element of the subset
    ++comb[i];
    while ((i > 0) && (comb[i] >= n - k + 1 + i)) {
        --i;
        ++comb[i];
    }

    if (comb[0] > n - k) { // Combination (n-k, n-k+1, ..., n) reached
        return false;      // No more combinations can be generated
    }

    /* comb now looks like (..., x, n-j, ... n-2, n-1, n) where x < n-j-1
              Turn it into (..., x, x+1, x+2, ... x+j, x+j+1 ) */
    for (i = i + 1; i < k; ++i) {
        comb[i] = 1 + comb[i - 1];
    }
    return true;
}

static int get_symbols( locate_t by, int ref, int *symbols )
{
    int symbol_map = 0;
    for ( int index = 0; index < SUDOKU_N_SYMBOLS; ++index ) {
        cell_ref_t cr;
        get_cell_ref_in_set( by, ref, index, &cr );
        sudoku_cell_t *cell = get_cell( cr.row, cr.col );
        if ( cell->n_symbols > 1 ) {
            symbol_map |= cell->symbol_map;
        }
    }

    int n_symbols = 0;
    while (true) {
        int symbol = extract_bit( &symbol_map );
        if ( -1 == symbol ) break;
        symbols[n_symbols] = symbol;
        ++n_symbols;
    }
    return n_symbols;
}

typedef struct {
    int  n_extra;       // extraneous symbols
    int  n_matching;    // exact matching symbols
    cell_ref_t cr;
} subset_cell_ref_t;

static int get_cells_for_pair_map( locate_t by, int ref, int symbol_map, subset_cell_ref_t *crs,
                                   int *n_partial, subset_cell_ref_t *partial_refs )
{
    int n_cells = 0;
    *n_partial = 0;

    for ( int index = 0; index < SUDOKU_N_SYMBOLS; ++index ) {
        cell_ref_t cr;
        get_cell_ref_in_set( by, ref, index, &cr );
        sudoku_cell_t *cell = get_cell( cr.row, cr.col );
        if ( 1 < cell->n_symbols ) {
            if ( symbol_map == ( symbol_map & cell->symbol_map ) ) {
                crs[n_cells].cr = cr;
                crs[n_cells].n_matching = 2; // symbol_map fully included
                crs[n_cells].n_extra = cell->n_symbols - 2;
                ++n_cells;
            } else if ( symbol_map & cell->symbol_map ) {
                partial_refs[*n_partial].cr = cr;
                partial_refs[*n_partial].n_matching = 1;
                partial_refs[*n_partial].n_extra = cell->n_symbols - 1;
                ++ *n_partial;
            }
        }
    }
#if 0
    printf( "get_cells_for_pair_map, partial:" );
    for ( int i = 0; i < *n_partial; ++ i ) {
        printf( " (%d,%d)", partial_refs[i].cr.row, partial_refs[i].cr.col );
    }
    printf( "\n                        full:" );
    for ( int i = 0; i < n_cells; ++ i ) {
        printf( " (%d,%d)", crs[i].cr.row, crs[i].cr.col );
    }
    printf( "\n" );
#endif
    return n_cells;
}

static int check_pairs( locate_t by, int ref, int n_symbols, int *symbols, hint_desc_t *hdesc )
// return -1 (no hint), 0 hint but no new single, or 1 if hint and new single
{
    if ( n_symbols < 2 ) return -1;

    int pairs[] = { 0, 1 };

    while( true ) {
        int symbol_map = 1 << symbols[pairs[0]];
        symbol_map |= 1 << symbols[pairs[1]];

        subset_cell_ref_t crs[9];  // references to cells containing the whole pair + more
        subset_cell_ref_t prs[9];  // references to cells containing a subset of the pair
        int n_partial;
        int n_included = get_cells_for_pair_map( by, ref, symbol_map, crs, &n_partial, prs );

        if ( 0 == n_partial && 2 == n_included ) {
            // either hidden pair to cleanup or already processed pair
            if ( crs[0].n_extra || crs[1].n_extra ) { // hidden pair to cleanup
//                reset_hint_cache( ); // set hidden pairs
                init_hint_desc( hdesc );   // new hints are coming, re-init hdesc
                hdesc->hint_type = HIDDEN_SUBSET;
                hdesc->action = SET;
                hdesc->n_symbols = 2;
                hdesc->symbol_map = symbol_map;
                hdesc->hint_pencil = true;
                hdesc->trigger_pencil = true;

                if ( crs[0].n_extra ) {
//                    cache_hint_cell( crs[0].row, crs[0].col, HINT_REGION | PENCIL );
//                    *selection_row = crs[0].row;
//                    *selection_col = crs[0].col;
                    hdesc->hints[hdesc->n_hints++] = crs[0].cr;
                    hdesc->selection = crs[0].cr;
                } else { 
//                    cache_hint_cell( crs[0].row, crs[0].col, TRIGGER_REGION | PENCIL );
                    hdesc->triggers[hdesc->n_triggers++] = crs[0].cr;
                }
                if ( crs[1].n_extra ) { // if both, selection will be on the 2nd one.
//                    cache_hint_cell( crs[1].row, crs[1].col, HINT_REGION | PENCIL );
//                    *selection_row = crs[1].row;
//                    *selection_col = crs[1].col;
                    hdesc->hints[hdesc->n_hints++] = crs[1].cr;
                    hdesc->selection = crs[1].cr;
                } else {
//                    cache_hint_cell( crs[1].row, crs[1].col, TRIGGER_REGION | PENCIL );
                    hdesc->triggers[hdesc->n_triggers++] = crs[1].cr;
                }
                return 0;       // return hidden pair to cleanup (no new single)
            } // else already processed pair, ignore

        } else { // maybe a naked pair plus partial match or additional symbols
            int n_exact = 0;
            for ( int i = 0; i < n_included; ++i ) {
                if ( 0 == crs[i].n_extra ) ++n_exact;
            }
            if ( 2 == n_exact ) {               // set naked pair
                int res = 0;
//               reset_hint_cache( );
                init_hint_desc( hdesc );        // new hints are coming, re-init hdesc
                hdesc->hint_type = NAKED_SUBSET;
                hdesc->action = REMOVE;
                hdesc->n_symbols = 2;           // symbol map has 2 symbols but only 1 
                hdesc->symbol_map = symbol_map; // of the 2 is to remove from each hint
                hdesc->hint_pencil = true;
                hdesc->trigger_pencil = true;

                for ( int i = 0; i < n_partial; ++i ) { // symbols from naked pair should be removed
//                   cache_hint_cell( prs[i].row, prs[i].col, HINT_REGION | PENCIL );
                    hdesc->hints[hdesc->n_hints++] = prs[i].cr;
//printf( "Set to remove from %d,%d, n_hints %d\n", prs[i].cr.row, prs[i].cr.col, hdesc->n_hints );
                    if ( 1 == prs[i].n_extra ) {
//                        *selection_row = prs[i].row; 
//                        *selection_col = prs[i].col;
                        hdesc->selection = prs[i].cr;
                        res = 1;    // new single
                    }
                }
                for ( int i = 0; i < n_included; ++i ) {
                    if ( 0 == crs[i].n_extra ) {    // the naked pair cells are trigger
//                        cache_hint_cell( crs[i].row, crs[i].col, TRIGGER_REGION | PENCIL );
//printf( "Set naked %d,%d n_triggers %d\n", crs[i].cr.row, crs[i].cr.col, hdesc->n_triggers );
                        hdesc->triggers[hdesc->n_triggers++] = crs[i].cr;
                    } else {                        // symbols from naked pair should be removed
//                        cache_hint_cell( crs[i].row, crs[i].col, HINT_REGION | PENCIL );
//printf( "Set to remove from %d,%d n_hints %d\n", crs[i].cr.row, crs[i].cr.col, hdesc->n_hints );
                        hdesc->hints[hdesc->n_hints++] = crs[i].cr;
                        if ( 1 == crs[i].n_extra ) {
//                            *selection_row = crs[i].row; 
//                            *selection_col = crs[i].col;
                            hdesc->selection = crs[i].cr;
                            res = 1;    // new single
                        }
                    }
                }
                return res;
            }
        }
        if ( // 2 == n_symbols ||
            ! get_next_combination( pairs, 2, n_symbols ) ) break;
    }
    return -1;
}

static bool get_cells_for_triplet_map( locate_t by, int ref, int max_map, int *min_maps,
                                       subset_cell_ref_t *crs, int *n_partial, subset_cell_ref_t *partial_refs )
{
    int n_cells = 0;
    *n_partial = 0;

    int n_max = 0;
    int n_min[3] = { 0 }; // one count for each min_map

    for ( int index = 0; index < SUDOKU_N_SYMBOLS; ++index ) {  // index in subset( by, ref )
        cell_ref_t cr;
        get_cell_ref_in_set( by, ref, index, &cr );
        sudoku_cell_t *cell = get_cell( cr.row, cr.col );

        if ( 1 < cell->n_symbols ) {
            if ( max_map == ( max_map & cell->symbol_map ) ) {  // triplet symbols are included
                crs[n_cells].cr = cr;
                crs[n_cells].n_matching = 3;
                crs[n_cells].n_extra = cell->n_symbols - 3;
                ++n_cells;
                ++n_max;
            } else {
                int n_triplet_symbols = 0;
                int i = 0;
                for ( ; i < 3; ++i ) {
                    if ( min_maps[i] == ( min_maps[i] & cell->symbol_map ) ) {
                        crs[n_cells].cr = cr;
                        crs[n_cells].n_matching = 2;
                        crs[n_cells].n_extra = cell->n_symbols - 2;
                        ++n_cells;
                        ++n_min[i];
                        break;  // only 1 is possible since cell did not match max_map

                    } else if ( min_maps[i] & cell->symbol_map ) {
                        ++n_triplet_symbols;    // since min_maps have 2 symbols only, partial match is 1
                    }
                }
                if ( 3 == i && n_triplet_symbols ) {
                    partial_refs[*n_partial].cr = cr;
                    partial_refs[*n_partial].n_matching = n_triplet_symbols;
                    partial_refs[*n_partial].n_extra = cell->n_symbols - n_triplet_symbols;
                    ++ *n_partial;
                }
            }
        }
    }
    int n_match = 0;
    for ( int i = 0; i < 3; ++i ) {
        if ( 1 < n_min[i] ) return false; // cannot be a triplet
        if ( 1 == n_min[i] ) ++n_match;
    }
    if ( 3 == n_max + n_match )
        return true;  // potential triplet; maybe naked if no extra or hidden if no partial

    return false;
}

static int check_triplets( locate_t by, int ref, int n_symbols, int *symbols, hint_desc_t *hdesc )
// return -1 (no hint), 0 hint but no new single, or 1 if hint and new single
{
    if ( n_symbols < 3 ) return -1;

    int triplets[] = { 0, 1, 2 };

    while( true ) {
        int max_map = 1 << symbols[triplets[0]];
        max_map |= 1 << symbols[triplets[1]];
        max_map |= 1 << symbols[triplets[2]];

        int min_maps[] = { 1 << symbols[triplets[0]] | 1 << symbols[triplets[1]], 
                           1 << symbols[triplets[0]] | 1 << symbols[triplets[2]],
                           1 << symbols[triplets[1]] | 1 << symbols[triplets[2]] };

        int n_partial;
        subset_cell_ref_t crs[9];
        subset_cell_ref_t prs[9];

// TODO: check why is the logic different from pair?
        if ( get_cells_for_triplet_map( by, ref, max_map, min_maps, crs, &n_partial, prs ) ) {
            // a potential triplet. It can be naked if no extra or hidden if no partial
            if ( 0 == n_partial ) {  // either hidden triplet to cleanup or already naked triplet
                if ( crs[0].n_extra || crs[1].n_extra || crs[2].n_extra ) { // hidden that needs cleanup
//                    reset_hint_cache( ); // set hidden triplet
                    init_hint_desc( hdesc );   // new hints are coming, re-init hdesc
                    hdesc->hint_type = HIDDEN_SUBSET;
                    hdesc->action = SET;
                    hdesc->n_symbols = 3;
                    hdesc->symbol_map = max_map;
                    hdesc->hint_pencil = true;
                    hdesc->trigger_pencil = true;
//                    cache_hint_cell( crs[0].row, crs[0].col, HINT_REGION | PENCIL );
//                    cache_hint_cell( crs[1].row, crs[1].col, HINT_REGION | PENCIL );
//                    cache_hint_cell( crs[2].row, crs[2].col, HINT_REGION | PENCIL );
                    if ( crs[0].n_extra ) {
//                        *selection_row = crs[0].row;
//                        *selection_col = crs[0].col;
                        hdesc->hints[hdesc->n_hints++] = crs[0].cr;
                        hdesc->selection = crs[0].cr;
                    } else { 
//                    cache_hint_cell( crs[0].row, crs[0].col, TRIGGER_REGION | PENCIL );
                        hdesc->triggers[hdesc->n_triggers++] = crs[0].cr;
                    }
                    if ( crs[1].n_extra ) {
//                        *selection_row = crs[1].row;
//                        *selection_col = crs[1].col;
                        hdesc->hints[hdesc->n_hints++] = crs[1].cr;
                        hdesc->selection = crs[1].cr;
                    } else {
//                    cache_hint_cell( crs[0].row, crs[0].col, TRIGGER_REGION | PENCIL );
                        hdesc->triggers[hdesc->n_triggers++] = crs[1].cr;
                    }
                    if ( crs[2].n_extra ) {
//                        *selection_row = crs[2].row;
//                        *selection_col = crs[2].col;
                        hdesc->hints[hdesc->n_hints++] = crs[2].cr;
                        hdesc->selection = crs[2].cr;
                    } else {
//                    cache_hint_cell( crs[0].row, crs[0].col, TRIGGER_REGION | PENCIL );
                        hdesc->triggers[hdesc->n_triggers++] = crs[2].cr;
                    }
                    return 0;       // return hidden triplet to cleanup
                } // else already processed naked triplet, ignore

            } else if ( 0 == crs[0].n_extra && 0 == crs[1].n_extra && 0 == crs[2].n_extra ) {
                // naked triplet and cells with same symbols: it reduces symbols in partial cells
//                reset_hint_cache( );
                init_hint_desc( hdesc );   // new hints are coming, re-init hdesc
                hdesc->hint_type = NAKED_SUBSET;
                hdesc->action = REMOVE;
                hdesc->n_symbols = 3;
                hdesc->symbol_map = max_map;
                hdesc->hint_pencil = true;
                hdesc->trigger_pencil = true;

//                cache_hint_cell( crs[0].row, crs[0].col, TRIGGER_REGION | PENCIL );
//                cache_hint_cell( crs[1].row, crs[1].col, TRIGGER_REGION | PENCIL );
//                cache_hint_cell( crs[2].row, crs[2].col, TRIGGER_REGION | PENCIL );
                hdesc->hints[hdesc->n_hints++] = crs[0].cr;
                hdesc->hints[hdesc->n_hints++] = crs[1].cr;
                hdesc->hints[hdesc->n_hints++] = crs[2].cr;
                int res = 0;
                for ( int i = 0; i < n_partial; ++i ) {
//                    cache_hint_cell( prs[i].row, prs[i].col, HINT_REGION | PENCIL );
                    hdesc->hints[hdesc->n_hints++] = prs[i].cr;
                    if ( 1 == prs[i].n_extra ) {
//                        *selection_row = prs[i].row; 
//                        *selection_col = prs[i].col;
                        hdesc->selection = prs[i].cr;
                        res = 1;
                    }
                }
                return res;
            }
        }
        if ( 3 == n_symbols || ! get_next_combination( triplets, 3, n_symbols ) ) break;
    }
    return -1;
}

static int check_subsets( hint_desc_t *hdesc )
// return 1 if new single, 0 otherwise; hint_type is updated to NO_HINT, NAKED_SUBSET or HIDDEN_SUBSET
// return 0 if no subset, 1 if subset and 2 if subset and at least a new single
{
    for ( locate_t by = LOCATE_BY_ROW; by <= LOCATE_BY_BOX; ++by ) {
        for ( int ref = 0; ref < SUDOKU_N_SYMBOLS; ++ref ) { // ref = row, col or in box cell index
            int symbols[9];
            int n_symbols = get_symbols( by, ref, symbols );
            if ( n_symbols < 2 ) continue;

            int res = check_pairs( by, ref, n_symbols, symbols, hdesc );
            if ( res != -1 ) return 1 + res;

            res = check_triplets( by, ref, n_symbols, symbols, hdesc );
            if ( res != -1 ) return 1 + res;
            // TODO: add quadruplets
        }
    }
    return 0;
}

/* 5. Look for X wings, Swordfish & jellyfish. This must be done after looking
      for naked singles in order to benefit from the pencil clean up. */

typedef struct {
    int n_locations;
    int location_map;
} symbol_locations_t;

static void get_symbol_locations_in_set( locate_t by, int symbol_map, symbol_locations_t *sloc )
{
    for ( int ref = 0; ref < SUDOKU_N_SYMBOLS; ++ ref ) {
        sloc[ref].n_locations = 0;
        sloc[ref].location_map = 0;

        for ( int index = 0; index < SUDOKU_N_SYMBOLS; ++index ) {
            cell_ref_t cr;
            get_cell_ref_in_set( by, ref, index, &cr );
            sudoku_cell_t *cell = get_cell( cr.row, cr.col );
            if ( cell->n_symbols > 1 && ( symbol_map & cell->symbol_map ) ) {
                sloc[ref].location_map |= 1 << index;
                ++sloc[ref].n_locations;
            }
        }
    }
}

static int set_X_wings_n_fish_hints( locate_t by, int symbol_mask, int n_refs, int *refs,
                                     int location_map, int *sel_row, int *sel_col )
{
    int indexes[4] = { 0 };
    int n_indexes = 0, index_map = location_map;
    while ( true ) {
        int index = extract_bit( &index_map );
        if ( -1 == index ) break;
        indexes[n_indexes++] = index;
    }
    assert( n_indexes <= 4 );
    bool new_single = false;
    bool hint = false;

    // hint are the other cells of the same columns or rows (rc)
    for ( int rc = 0; rc < SUDOKU_N_SYMBOLS; ++ rc ) {
        bool same = false;
        for ( int i = 0 ; i < n_refs; ++i ) {
            if ( rc == refs[i] ) {
                same = true;
                break;
            }
        }
        if ( same ) continue;

        // If symbol appears, indicate as a hint
        for ( int i = 0 ; i < n_indexes; ++i ) {
            sudoku_cell_t *cell = ( LOCATE_BY_ROW == by ) ? get_cell( rc, indexes[i] )
                                                          : get_cell( indexes[i], rc );

            if ( cell->n_symbols > 1 && ( cell->symbol_map & symbol_mask ) ) {
                if ( ! hint ) reset_hint_cache( );
                hint = true;

                if ( 2 == cell->n_symbols ) {
                    *sel_row = ( LOCATE_BY_ROW == by ) ? rc : indexes[i];
                    *sel_col = ( LOCATE_BY_ROW == by ) ? indexes[i] : rc;
                    new_single = true;
                }
                if ( LOCATE_BY_ROW == by ) cache_hint_cell( rc, indexes[i], HINT_REGION | PENCIL );
                else                       cache_hint_cell( indexes[i], rc, HINT_REGION | PENCIL );
            }
        }
    }
    if ( hint ) {   // indicate triggers too
        for ( int i = 0; i < n_refs; ++i ) {
            for ( int j = 0; j < n_indexes; ++j ) {
                sudoku_cell_t *cell;
                if ( LOCATE_BY_ROW == by ) {
                    cell = get_cell( refs[i], indexes[j] );
                    if ( cell->n_symbols > 1 && ( cell->symbol_map & symbol_mask ) )
                        cache_hint_cell( refs[i], indexes[j], TRIGGER_REGION | PENCIL );
                } else {
                    cell = get_cell( indexes[j], refs[i] );
                    if ( cell->n_symbols > 1 && ( cell->symbol_map & symbol_mask ) )
                        cache_hint_cell( indexes[j], refs[i], TRIGGER_REGION | PENCIL );
                }
            }
        }
    }
    return ( new_single ) ? 1 : (( hint ) ? 0 : -1);
}

static int get_n_bits_from_map( int map )
{
    int n_bits = 0;
    for ( int i = 0; ; ++i ) {
        if ( 0 == map ) break;
        if ( map & 1 << i ) {
            ++n_bits;
            map &= ~ ( 1 << i );
        }
    }
    return n_bits;
}

static int find_next_matching_set( int n_times, int n_refs, int *refs, symbol_locations_t *ref_sloc )
{
    int cumulated_location_map = 0;
    for ( int i = 0; i < n_refs; ++i ) {
        cumulated_location_map |= ref_sloc[refs[i]].location_map;
    }

    for ( int i = 0; i < SUDOKU_N_SYMBOLS; ++i ) {
        if ( ref_sloc[i].n_locations >= 2 && ref_sloc[i].n_locations <= n_times ) {
            bool skip = false;
            for ( int j = 0; j < n_refs; ++j ) {
                if ( i == refs[j] ) {
                    skip = true;
                    break;
                }
            }
            if ( skip ) continue;

            int n_bits = get_n_bits_from_map( cumulated_location_map | ref_sloc[i].location_map );
            if ( n_bits <= n_times ) {
                refs[n_refs++] = i;
                if ( n_refs < n_times )
                    return find_next_matching_set( n_times, n_refs, refs, ref_sloc );
                return cumulated_location_map;
            }
        }
    }
    return 0;
}

static bool search_for_fish_configuration( int n_times, int *fish_refs, int *location_map,
                                           symbol_locations_t *ref_sloc )
{
    assert( n_times <= 4 );         // no more than 4 sets for jellyfish

    for ( int i = 0; i < 9; ++i ) {
        if ( ref_sloc[i].n_locations >= 2 && ref_sloc[i].n_locations <= n_times ) {
            fish_refs[0] = i;
            int loc_map = find_next_matching_set( n_times, 1, fish_refs, ref_sloc );
            if ( loc_map ) {
                *location_map = loc_map;
                return true;
            }
        }
    }
    return false;
}

#define NEW_SINGLE 0x100

static int search_for_X_wings_n_fish_hints( locate_t by, int symbol_map, int *sel_row, int *sel_col,
                                            symbol_locations_t *ref_sloc )
// returns 0 if no useful hint, XWING, XWING + NEW_SINGLE, SWORDFISH or SWORDFISH + NEW_SINGLE
{
    sudoku_hint_type hint = NO_HINT;
    int location_map = 0;
    int refs[4];                        // no more than 4 sets for jellyfish

    if ( search_for_fish_configuration( 2, refs, &location_map, ref_sloc ) ) {
        int res = set_X_wings_n_fish_hints( by, symbol_map, 2, refs,
                                            location_map, sel_row, sel_col );
        if ( -1 != res ) {
            if ( 1 == res ) return XWING + NEW_SINGLE;  // new single, stop here
            hint = XWING;
        }
    }

    if ( search_for_fish_configuration( 3, refs, &location_map, ref_sloc ) ) {
        int res = set_X_wings_n_fish_hints( by, symbol_map, 3, refs,
                                            location_map, sel_row, sel_col );
        if ( -1 != res ) {
            if ( 1 == res ) return SWORDFISH + NEW_SINGLE;  // new single, stop here
            hint = SWORDFISH;
        }
    }


    if ( search_for_fish_configuration( 4, refs, &location_map, ref_sloc ) ) {
        int res = set_X_wings_n_fish_hints( by, symbol_map, 4, refs,
                                            location_map, sel_row, sel_col );
        if ( -1 != res ) {
            if ( 1 == res ) return JELLYFISH + NEW_SINGLE;  // new single, stop here
            hint = JELLYFISH;
        }
    }
    return hint;
}

static int check_X_wings_Swordfish( sudoku_hint_type *hint_type,
                                    int *sel_row, int *sel_col )
// return -1 if no useful hint, 0 if some hints or 1 if a hint leads to a new single
{
    int last_hints = NO_HINT;
    for ( int symbol = 0; symbol < SUDOKU_N_SYMBOLS; ++symbol ) {

        int symbol_map = 1 << symbol;
        symbol_locations_t row_sloc[9], col_sloc[9];
        symbol_locations_t *sloc = row_sloc;
        for ( locate_t by = LOCATE_BY_ROW; by <= LOCATE_BY_COL; ++by ) {
            get_symbol_locations_in_set( by, symbol_map, sloc );
            sloc = col_sloc;
        }

        int hints;
        // 1. search for X-Wing, Swordfish or Jellyfish in rows
        hints = search_for_X_wings_n_fish_hints( LOCATE_BY_ROW, symbol_map,
                                                 sel_row, sel_col, row_sloc );
        if ( hints ) last_hints = hints & 0xff;
        if ( hints & NEW_SINGLE ) {
            *hint_type = last_hints;
            return 1;
        }

        // 2. search for X-Wing, Swordfish or Jellyfish in columns
        hints = search_for_X_wings_n_fish_hints( LOCATE_BY_COL, symbol_map,
                                                 sel_row, sel_col, col_sloc );
        if ( hints ) last_hints = hints & 0xff;
        if ( hints & NEW_SINGLE ) {
            *hint_type = last_hints;
            return 1;
        }
    }

    if ( NO_HINT != last_hints ) {
        *hint_type = last_hints;
        return 0;
    }
    return -1;
}

/* 6. Look for XY wings. This must be done after looking
      for naked singles in order to benefit from the pencil clean up. */

typedef enum {
    NO_XY_WING, XY_WING_2_HORIZONTAL_BOXES, XY_WING_2_VERTICAL_BOXES, XY_WING_3_BOXES
} xy_wing_type;

typedef struct {
    xy_wing_type    xyw_type;
    int             symbol_mask;
    int             n_hints;
    cell_ref_t      hints[5];       // at least 1 cell (3 boxes), at most 5 cells (2 boxes)
    cell_ref_t      triggers[3];    // always 3 cells
} xy_wing_hints_t;

static int get_common_symbol_mask( cell_ref_t *c0, cell_ref_t *c1 )
{
    sudoku_cell_t *cell_1 = get_cell( c0->row, c0->col );
    sudoku_cell_t *cell_2 = get_cell( c1->row, c1->col );
    return cell_1->symbol_map & cell_2->symbol_map;
}

static bool set_2_box_horizontal_hints( int b0_col, int b1_col, int c0_row, int c0_col,
                                        int c1_row, xy_wing_hints_t *xywh )
{
    int n_hints = 0;
    for ( int c = b1_col; c < b1_col + 3; ++c ) {
        xywh->hints[n_hints].row = c1_row;
        xywh->hints[n_hints].col = c;
        ++n_hints;
    }
    for ( int c = b0_col; c < b0_col + 3; ++c ) {
        if ( c == c0_col ) continue;
        xywh->hints[n_hints].row = c0_row;
        xywh->hints[n_hints].col = c;
        ++n_hints;
    }
    return true;
}

static bool set_2_box_vertical_hints( int b0_row, int b1_row, int c0_row, int c0_col,
                                      int c1_col, xy_wing_hints_t *xywh )
{
    int n_hints = 0;
    for ( int r = b1_row; r < b1_row + 3; ++r ) {
        xywh->hints[n_hints].row = r;
        xywh->hints[n_hints].col = c1_col;
        ++n_hints;
    }
    for ( int r = b0_row; r < b0_row + 3; ++r ) {
        if ( r == c0_row ) continue;
        xywh->hints[n_hints].row = r;
        xywh->hints[n_hints].col = c0_col;
        ++n_hints;
    }
    return true;
}

static bool check_2_box_geometry( int box0, int box1, // box0 has 2 cells, box1 only 1
                                  cell_ref_t *b0_c0, cell_ref_t *b0_c1, cell_ref_t *b1_c2,
                                  xy_wing_hints_t *xywh )
{
    int b0_row = 3 * (box0 / 3), b1_row = 3 * (box1 / 3);   // first row of each box
    int b0_col = 3 * (box0 % 3), b1_col = 3 * (box1 % 3);   // first col of each box
    
    xywh->n_hints = 5;
    xywh->triggers[0] = *b0_c0;         // assume a positive outcome
    xywh->triggers[1] = *b0_c1;
    xywh->triggers[2] = *b1_c2;

    if ( b0_row == b1_row ) {           // boxes are horizontally aligned
        if ( b0_c0->row == b0_c1->row ) return false;

        xywh->xyw_type = XY_WING_2_HORIZONTAL_BOXES;
        if ( b0_c0->row == b1_c2->row ) {
            xywh->symbol_mask = get_common_symbol_mask( b0_c1, b1_c2 );
            return set_2_box_horizontal_hints( b0_col, b1_col, b0_c0->row, b0_c0->col, b0_c1->row, xywh );

        } else if ( b0_c1->row == b1_c2->row ) {
            xywh->symbol_mask = get_common_symbol_mask( b0_c0, b1_c2 );
            return set_2_box_horizontal_hints( b0_col, b1_col, b0_c1->row, b0_c1->col, b0_c0->row, xywh );
        }
    } else if ( b0_col == b1_col ) {    // boxes are vertically aligned
        if ( b0_c0->col == b0_c1->col ) return false;

        xywh->xyw_type = XY_WING_2_VERTICAL_BOXES;
        if ( b0_c0->col == b1_c2->col ) {
            xywh->symbol_mask = get_common_symbol_mask( b0_c1, b1_c2 );
            return set_2_box_vertical_hints( b0_row, b1_row, b0_c0->row, b0_c0->col, b0_c1->col, xywh );

        } else if ( b0_c1->col == b1_c2->col ) {
            xywh->symbol_mask = get_common_symbol_mask( b0_c0, b1_c2 );
            return set_2_box_vertical_hints( b0_row, b1_row, b0_c1->row, b0_c1->col, b0_c0->col, xywh );
        }
    }
    return false;  // not a valid geometry
}

static bool check_3_box_geometry( cell_ref_t *pairs, xy_wing_hints_t *xywh )
{
    xywh->xyw_type = XY_WING_3_BOXES;
    xywh->n_hints = 1;
    xywh->triggers[0] = pairs[0];         // assume a positive outcome
    xywh->triggers[1] = pairs[1];
    xywh->triggers[2] = pairs[2];

    if ( pairs[0].row == pairs[1].row ) {           // p0 & p1 are horizontally aligned:     p0-p1
        if ( pairs[2].col == pairs[0].col ) {           // p0 & p2 are vertically aligned:   p2-hint
            xywh->symbol_mask = get_common_symbol_mask( &pairs[1], &pairs[2] );
            xywh->hints[0].row = pairs[2].row;              // hint row from p2
            xywh->hints[0].col = pairs[1].col;              // hint col from p1
            return true;                                                                //   p0-p1
        } else if ( pairs[2].col == pairs[1].col ) {    // p1 & p2 are vertically aligned: hint-p2
            xywh->symbol_mask = get_common_symbol_mask( &pairs[0], &pairs[2] );
            xywh->hints[0].row = pairs[2].row;              // hint row from p2
            xywh->hints[0].col = pairs[0].col;              // hint col from p0
            return true;
        }
    } else if ( pairs[0].row == pairs[2].row ) {    // p0 & p2 are horizontally aligned:     p0-p2
        if ( pairs[1].col == pairs[0].col ) {           // p0 & p1 are vertically aligned:   p1-hint
            xywh->symbol_mask = get_common_symbol_mask( &pairs[1], &pairs[2] );
            xywh->hints[0].row = pairs[1].row;              // hint row from p1
            xywh->hints[0].col = pairs[2].col;              // hint col from p2
            return true;                                                                //   p0-p2
        } else if ( pairs[1].col == pairs[2].col ) {    // p1 & p2 are vertically aligned: hint-p1
            xywh->symbol_mask = get_common_symbol_mask( &pairs[0], &pairs[1] );
            xywh->hints[0].row = pairs[1].row;              // hint row from p1
            xywh->hints[0].col = pairs[0].col;              // hint col from p0
            return true;
        }
    } else if ( pairs[1].row == pairs[2].row ) {    // p1 & p2 are horizontally aligned:     p1-p2
        if ( pairs[0].col == pairs[1].col ) {           // p0 & p1 are vertically aligned:   p0-hint
            xywh->symbol_mask = get_common_symbol_mask( &pairs[0], &pairs[2] );
            xywh->hints[0].row = pairs[0].row;              // hint row from p0
            xywh->hints[0].col = pairs[2].col;              // hint col from p2
            return true;                                                                //   p1-p2
        } else if ( pairs[0].col == pairs[2].col ) {    // p0 & p2 are vertically aligned: hint-p0
            xywh->symbol_mask = get_common_symbol_mask( &pairs[0], &pairs[1] );
            xywh->hints[0].row = pairs[0].row;              // hint row from p0
            xywh->hints[0].col = pairs[1].col;              // hint col from p1
            return true;
        }
    }
    return false;  // not a valid geometry
}

static inline int get_box_from_ref( cell_ref_t *ref )
{
    return 3 * (ref->row / 3) + ( ref->col / 3);
}

static bool check_xy_wing_geometry( cell_ref_t *pairs, xy_wing_hints_t *xywh )
{
    // check if the pairs are in 2 or 3 boxes and if the alignments are correct

    int box0 = get_box_from_ref( &pairs[0] );
    int box1 = get_box_from_ref( &pairs[1] );
    int box2 = get_box_from_ref( &pairs[2] );

    if ( box0 == box1 ) {
        if ( box0 == box2 ) return false;
        return check_2_box_geometry( box0, box2, &pairs[0], &pairs[1], &pairs[2], xywh );

    } else if ( box0 == box2 ) {
        return check_2_box_geometry( box0, box1, &pairs[0], &pairs[2], &pairs[1], xywh );

    } else if ( box1 == box2 ) {
        return check_2_box_geometry( box1, box0, &pairs[1], &pairs[2], &pairs[0], xywh );
    }
    return check_3_box_geometry( pairs, xywh );
}

static void get_pair_symbols( int map, int *s0_mask, int *s1_mask )
{
    int n_symbols = 0;
    for ( int i = 0; ; ++i ) {
        if ( 0 == map ) break;
        if ( map & 1 << i ) {
            if ( n_symbols == 0 ) {
                *s0_mask = 1 << i;
                n_symbols = 1;
            } else {
                assert( map == 1 << i );
                *s1_mask = 1 << i;
                return;
            }
            map &= ~ ( 1 << i );
        }
    }
    assert( 0 );
}

static bool get_3rd_matching_pair( int symbol_map, int n_pairs, cell_ref_t *pairs,
                                   cell_ref_t *matching_pairs, xy_wing_hints_t *xywh )
{
    for ( int j = 0; j < n_pairs; ++ j ) {
        sudoku_cell_t *cell = get_cell( pairs[j].row, pairs[j].col );
        if ( cell->symbol_map == symbol_map ) {
            matching_pairs[2] = pairs[j];
            if ( check_xy_wing_geometry( matching_pairs, xywh ) )
                return true;
        } // else try another 3rd pair
    }
    return false;
}

static bool search_for_xy_wing_matching_pairs( int n_pairs, cell_ref_t *pairs,
                                               cell_ref_t *matching_pairs, xy_wing_hints_t *xywh )
{
    sudoku_cell_t *cell_0 = get_cell( matching_pairs[0].row, matching_pairs[0].col );
    int symbol_map_0 = cell_0->symbol_map;                              // 2^s0 | 2^s1
    int s0_mask, s1_mask;
    get_pair_symbols( symbol_map_0, &s0_mask, &s1_mask );

    for ( int i = 0; i < n_pairs; ++i ) {
        sudoku_cell_t *cell_1 = get_cell( pairs[i].row, pairs[i].col );
        int map = cell_1->symbol_map;
        if ( map == symbol_map_0 ) continue;    // 2^s0 | 2^s1 : naked subset, try another pair

        matching_pairs[1] = pairs[i];
        if ( map & s0_mask ) {                  // 2^s0 | 2^s2 (new symbol s2)
            if ( get_3rd_matching_pair( s1_mask | (map & ~s0_mask), // 2^s1 | 2^s2 in remaining pairs
                                        n_pairs - (i+1), &pairs[i+1], matching_pairs, xywh ) )
                return true;

        } else if ( map & s1_mask ) {           // 2^s1 | 2^s2 (new symbol s2)
            if ( get_3rd_matching_pair( s0_mask | (map & ~s1_mask), // 2^s0 | 2^s2 in remaining pairs
                                        n_pairs - (i+1), &pairs[i+1], matching_pairs, xywh ) )
                return true;
        } // else try another second pair
    }
    return false;
}

static int get_symbol_pairs( cell_ref_t *refs )
// regardless their symbols, return the number of pairs collected.
{
    int n_refs = 0;
    for ( int r = 0; r < SUDOKU_N_ROWS; ++r ) {
        for ( int c = 0; c < SUDOKU_N_COLS; ++c ) {
            sudoku_cell_t *cell = get_cell( r, c );
            if ( 2 == cell->n_symbols ) {
                refs[n_refs].row = r;
                refs[n_refs].col = c;
                ++n_refs;
            }
        }
    }
    return n_refs;
}

static int set_xy_wing_hints( xy_wing_hints_t *xywh, int *selection_row, int *selection_col )
{
    /* check if hints are useful (at least 1 symbol can be removed from 1 cell)
       or even better if one hint cell will become a single after the hint */
    int n_candidates = 0;
    bool single = false;
    for ( int i = 0; i < xywh->n_hints; ++ i ) {
        sudoku_cell_t *cell = get_cell( xywh->hints[i].row, xywh->hints[i].col );
        if ( 1 < cell->n_symbols && ( xywh->symbol_mask & cell->symbol_map ) ) {
            if ( 0 == n_candidates ) reset_hint_cache( );
            ++n_candidates;
            cache_hint_cell( xywh->hints[i].row, xywh->hints[i].col, HINT_REGION | PENCIL );
            if ( 2 == cell->n_symbols && ! single) { // a first new single
                *selection_row = xywh->hints[i].row;
                *selection_col = xywh->hints[i].col;
                single = true;
            }
        }
    }
    if ( n_candidates ) {
        for ( int i = 0; i < 3; ++ i ) {
            cache_hint_cell( xywh->triggers[i].row, xywh->triggers[i].col, TRIGGER_REGION | PENCIL );
        }
        return (single) ? 1 : 0;
    }
    return -1;
}

static int search_for_xy_wing( sudoku_hint_type *hint_type, int *selection_row, int *selection_col )
{
    *hint_type = NO_HINT;
    cell_ref_t pairs[ SUDOKU_N_SYMBOLS * SUDOKU_N_SYMBOLS ]; // must be less than the complete grid
    int n_pairs = get_symbol_pairs( pairs );
    if ( n_pairs < 3 ) return -1;

    int potential = 0;
    xy_wing_hints_t xywh;
    cell_ref_t matching_pairs[3];

    for ( int i = 0; i < n_pairs; ++i ) {
        matching_pairs[0] = pairs[i];

        if ( search_for_xy_wing_matching_pairs( n_pairs-(i+1), &pairs[i+1], matching_pairs, &xywh ) ) {
            int res = set_xy_wing_hints( &xywh, selection_row, selection_col );
            if ( -1 == res ) continue;
            *hint_type = XY_WING;
            if ( 1 == res ) return 1;
            ++potential;
        }
    }
    if ( potential ) return 0;
    return -1;
}

/* 7. Look for single and multiple forbidding chains. This must be done after looking
      for naked singles in order to benefit from the pencil clean up. */

typedef struct {
    int n_cells;            // for each box, the number of cells in which symbol is found
    int cell_map;           // for each box, the candidate map: 1 bit per cell (0, 1, 2/3, 4, 5/6, 7, 8)
} candidate_box_location_t; // candidates in the box (up to 8 cells)

static int get_locations_in_rows_cols_boxes( int candidate_mask, candidate_row_location_t *crloc,
                                             candidate_col_location_t *ccloc, candidate_box_location_t *cbloc )
// the caller is responsible for allocating SUDOKU_N_ROWS row locations, 
//                                          SUDOKU_N_COLS col locations,
//                                      and SUDOKU_N_BOXES box locations
{
    for ( int b = 0; b < SUDOKU_N_BOXES; ++b ) {
        cbloc[b].n_cells = 0;
        cbloc[b].cell_map = 0;
    }

    for ( int c = 0; c < SUDOKU_N_COLS; ++c ) {
        ccloc[c].n_rows = 0;
        ccloc[c].row_map = 0;
    }

    int n_locations = 0;
    for ( int r = 0; r < SUDOKU_N_ROWS; ++r ) {
        crloc[r].n_cols = 0;
        crloc[r].col_map = 0;

        for ( int c = 0; c < SUDOKU_N_COLS; ++c ) {
            int b = get_surrounding_box( r, c );
            sudoku_cell_t *cell = get_cell( r, c );
            if ( cell->n_symbols > 1 && (cell->symbol_map & candidate_mask) ) {
                ++crloc[r].n_cols;
                crloc[r].col_map |= 1 << c;
                ++ccloc[c].n_rows;
                ccloc[c].row_map |= 1 << r;
                ++cbloc[b].n_cells;
                cbloc[b].cell_map |= 1 << get_cell_index_in_box( r, c );
                ++n_locations;
            }
        }
    }
// debug
    printf( "Symbol %c code %d (map 0x%03x) locations:\n",
            '1' + get_number_from_map(candidate_mask),
            get_number_from_map(candidate_mask), candidate_mask );
    for ( int r = 0; r < SUDOKU_N_ROWS; ++r ) {
        printf( " row %d, n_cols %d, col_map 0x%03x\n", r, crloc[r].n_cols, crloc[r].col_map );
    }
    printf("\n");
    for ( int c = 0; c < SUDOKU_N_COLS; ++c ) {
        printf( " col %d, n_rows %d, row_map 0x%03x\n", c, ccloc[c].n_rows, ccloc[c].row_map );
    }
    printf("\n");
    for ( int b = 0; b < SUDOKU_N_BOXES; ++b ) {
        printf( " box %d, n_cells %d, cell_map 0x%03x\n", b, cbloc[b].n_cells, cbloc[b].cell_map );
    }
    printf("\n");
    return n_locations;
}

static int get_candidate_map( void )
{
    int candidate_map = 0;
    for ( int r = 0; r < SUDOKU_N_ROWS; ++r ) {
        for ( int c = 0; c < SUDOKU_N_COLS; ++c ) {
            sudoku_cell_t *cell = get_cell( r, c );
            if ( cell->n_symbols > 1 ) {
                candidate_map |= cell->symbol_map;
            }
        }
    }
    return candidate_map;
}

typedef struct {
    bool    head;
    int     row, col, polarity;
} chain_link_t;

static int locate_forbidden_candidates( chain_link_t *chain, int n_links,
                                        int symbol_mask, cell_ref_t * hints,
                                        int *selection_row, int *selection_col )
/* Based on reversed polarities inside a single chain:
    a cell at the intersection of row including one polarity and
                                  col including the other polarity is forbidden */
{
    struct prow {
        int chain_offset /* in chain */, index /* in row or col */;
    } prows[ SUDOKU_N_ROWS], pcols[ SUDOKU_N_COLS ];

    int n_hints = 0;
    int start = 0, end;
    while ( true ) {                            // 1 segment at a time
        end = n_links-1;
        for ( int i = start; i <= end; ++i ) {
            if ( chain[i].head && i != start ) {
                end = i - 1;
                break;
            }
        }

        int n_prows = 0, n_pcols = 0;           // find tentative rows and columns for candidates
        for ( int i = start; i <= end; ++i ) {
            int row = chain[i].row;
            int col = chain[i].col;
            bool single_row = true, single_col = true;

            for ( int j = start; j <= end; ++j ) {
                if ( j == i ) continue;
                if ( chain[j].row == row ) single_row = false;
                if ( chain[j].col == col ) single_col = false;
            }
            if ( single_row ) {
                prows[n_prows].chain_offset = i;
                prows[n_prows].index = row;
                ++n_prows;
            } else if ( single_col ) {
                pcols[n_pcols].chain_offset = i;
                pcols[n_pcols].index = col;
                ++n_pcols;
            }
        }

        for ( int i = 0; i < n_prows; ++i ) {   // make up candidates and check if they contain the symbol
            for ( int j = 0; j < n_pcols; ++j ) {
                if ( chain[prows[i].chain_offset].polarity != chain[pcols[j].chain_offset].polarity ) {
                    sudoku_cell_t *cell = get_cell( prows[i].index, pcols[j].index );
                    if ( cell->n_symbols > 1 && ( symbol_mask & cell->symbol_map ) ) {
                        hints[n_hints].row = prows[i].index;
                        hints[n_hints].col = pcols[j].index;
                        ++n_hints;

                        if ( 2 == cell->n_symbols ) {
                            *selection_row = prows[i].index;
                            *selection_col = pcols[j].index;
                        }
                    }
                }
            }
        }
        if ( n_hints > 0 ) {                    // if successfull, eliminate other chains
            for ( int i = 0; i < start; ++ i ) {            // before and
                chain[i].polarity = 0;
            }
            for ( int i = end + 1; i < n_links; ++ i ) {    // after, if any
                chain[i].polarity = 0;
            }
            break;                              // stop here since segment generated hints
        }
        start = end + 1;                        // else try next segment, till the end of chain
        if ( start == n_links ) break;
    }
    return n_hints;
}

typedef struct {
    int  beg, end;  // where segment begins and ends in chain [end included]
    bool active;    // whether the segment is engaged in a relationship generating a hint
} chain_segment_t;

static int get_chain_segments( chain_link_t *chain, int n_links, chain_segment_t *segments )
{
    int n_segments = 0;
    int beg = 0;

    for ( int i = beg; i < n_links; ++i ) {
        if ( chain[i].head && i != beg ) {
            segments[n_segments].beg = beg;
            segments[n_segments].end = i-1;
            segments[n_segments].active = false;
            beg = i;
            ++n_segments;
        }
    }
    segments[n_segments].beg = beg;
    segments[n_segments].end = n_links-1;
    segments[n_segments].active = false;
    return 1 + n_segments;
}

static bool is_cell_in_chain( chain_link_t *chain, int beg, int end, int r, int c )
{
    for ( int i = beg; i <= end; ++i ) {
        if ( chain[i].row == r && chain[i].col == c ) return true;
    }
    return false;
}

static int find_chain_exclusions( chain_link_t *chain, int symbol_mask,
                                  int seg1_beg, int seg1_end, int seg2_beg, int seg2_end,
                                  int *selection_row, int *selection_col,
                                  cell_ref_t * hints, int n_hints )
{
    int polarity = 0;
    int excluded = -1;
    int prev_seg1, prev_seg2;

    for ( int seg1_index = seg1_beg; seg1_index <= seg1_end; ++ seg1_index ) {
        for ( int seg2_index = seg2_beg; seg2_index <= seg2_end; ++ seg2_index ) {
            // first loop to find any polarity clash
            if ( chain[seg1_index].row == chain[seg2_index].row ||
                 chain[seg1_index].col == chain[seg2_index].col ||
                 are_cells_in_same_box( chain[seg1_index].row, chain[seg1_index].col,
                                        chain[seg2_index].row, chain[seg2_index].col ) ) {
                if ( polarity == 0 ) {
                    polarity = chain[seg1_index].polarity * chain[seg2_index].polarity;
                    prev_seg1 = seg1_index;
                    prev_seg2 = seg2_index;
                } else if ( polarity != chain[seg1_index].polarity * chain[seg2_index].polarity ) {
                    // the 2 pair polarities clash, find the one that did not change
                    excluded = ( chain[prev_seg1].polarity ==
                                   chain[seg1_index].polarity ) ? prev_seg1 : prev_seg2;
                    break;
                }
            }
        }
        if ( -1 != excluded ) { // chain exclusion
            *selection_row = chain[excluded].row;
            *selection_col = chain[excluded].col;

            int end = ( excluded < seg2_beg ) ? seg1_end : seg2_end;
            int polarity = chain[excluded].polarity;
            for ( int k = excluded; k <= end; ++ k ) {
                if ( chain[k].polarity != polarity ) continue;
                hints[n_hints].row = chain[k].row;
                hints[n_hints].col = chain[k].col;
                ++n_hints;
            }
            break;
        }
    }

    if ( polarity ) {   // second loop to find any forced exclusions outside the chains
        int prev_hints = n_hints;
        int prev_seg1_polarity = chain[prev_seg1].polarity,
            prev_seg2_polarity = chain[prev_seg2].polarity;
        for ( int seg1_index = seg1_beg; seg1_index <= seg1_end; ++ seg1_index ) {
            if ( prev_seg1_polarity == chain[seg1_index].polarity ) continue;
            for ( int seg2_index = seg2_beg; seg2_index <= seg2_end; ++ seg2_index ) {
                if ( prev_seg2_polarity == chain[seg2_index].polarity ) continue;

                sudoku_cell_t *cell;    // try and find cells with symbol at the intersection of rows/cols
                if ( ! is_cell_in_chain( chain, seg1_beg, seg1_end, chain[seg1_index].row, chain[seg2_index].col ) &&
                     ! is_cell_in_chain( chain, seg2_beg, seg2_end, chain[seg1_index].row, chain[seg2_index].col ) ) {
               
                    cell = get_cell( chain[seg1_index].row, chain[seg2_index].col );
                    if ( cell->symbol_map & symbol_mask ) {
                        hints[n_hints].row = chain[seg1_index].row;
                        hints[n_hints].col = chain[seg2_index].col;
                        ++n_hints;
                        continue;
                    }
                }
                if ( ! is_cell_in_chain( chain, seg1_beg, seg1_end, chain[seg2_index].row, chain[seg1_index].col ) &&
                     ! is_cell_in_chain( chain, seg2_beg, seg2_end, chain[seg2_index].row, chain[seg1_index].col ) ) {
                    cell = get_cell( chain[seg2_index].row, chain[seg1_index].col );
                    if ( cell->symbol_map & symbol_mask ) {
                        hints[n_hints].row = chain[seg2_index].row;
                        hints[n_hints].col = chain[seg1_index].col;
                        ++n_hints;
                    }
                }
            }
        }

        if ( n_hints != prev_hints && 1 == polarity ) { // for clarity reverse second chain polarity
            printf("fix second chain polarity\n" );
            for ( int seg2_index = seg2_beg; seg2_index <= seg2_end; ++seg2_index ) {
                chain[seg2_index].polarity *= -1;
            }
        }
    }
    return n_hints;
}

static void hide_inactive_segments( chain_link_t *chain, chain_segment_t *segments, int n_segments )
{
    for ( int i = 0; i < n_segments; ++i ) {
        if ( segments[i].active ) continue;

        for ( int j = segments[i].beg; j <= segments[i].end; ++ j ) {
             chain[j].polarity = 0;
        }
    }
}

static int process_weak_relations( chain_link_t *chain, int n_links, int symbol_mask, 
                                   cell_ref_t * hints, int *selection_row, int *selection_col )
/* Based on weak links between cells in 2 chains: If they are in the same col, row or box
   they cannot both contain the symbol, so their polarity must be taken as opposite.
   1. If a second cell in the first chain with the same polarity is also in a weak link with
      a cell in the second chain which has a polarity opposite of the previous cell in the
      same chain, the case is impossible: the first chain cells cannot contain the symbol
      since it MUST be in one of the second chain cells.
   2. If all cells in the second chain have compatible polarities, then any cell at the
      intersection of a cell in the first chain and a cell in the second chain cannot have
      the same symbol, as it must be in either the first chain or the second one.
*/
{
    chain_segment_t segments[ SUDOKU_N_ROWS * SUDOKU_N_COLS ];
    int n_segments = get_chain_segments( chain, n_links, segments );
    int n_hints = 0;

    for ( int i = 0; i < n_segments-1; ++ i ) {   // find weak links beween 2 segments
        for ( int j = i+1; j < n_segments; ++ j ) {
            int prev_n_hints = n_hints;
            n_hints = find_chain_exclusions( chain, symbol_mask,
                                             segments[i].beg, segments[i].end,
                                             segments[j].beg, segments[j].end,
                                             selection_row, selection_col,
                                             hints, n_hints );
            printf("segment %d & %d: n_hints = %d\n", i, j, n_hints );
            if ( prev_n_hints != n_hints ) {    // segments i & j provide some hints: make them active
                printf( "Active segments %d, %d\n", i, j );
                segments[i].active = segments[j].active = true;
            }
        }
    }
    hide_inactive_segments( chain, segments, n_segments );
    return n_hints;
}

static void setup_chain_hints_triggers( chain_link_t *chain, int n_links,
                                        cell_ref_t * hints, int n_hints )
{
    for ( int i = 0; i < n_hints; ++i ) {
        cache_hint_cell( hints[i].row, hints[i].col, HINT_REGION | PENCIL );
    }
    for ( int i = 0; i < n_links; ++i ) {
        int chain_head = (  chain[i].head ) ? CHAIN_HEAD : 0;
        if ( chain[i].polarity == 1 ) {
            cache_hint_cell( chain[i].row, chain[i].col, chain_head | TRIGGER_REGION | PENCIL );
        } else if ( chain[i].polarity == -1 ) {
            cache_hint_cell( chain[i].row, chain[i].col, chain_head | ALTERNATE_TRIGGER_REGION | PENCIL );
        }
    }
}

static void print_chain( chain_link_t *chain, int n_links )
{
    for ( int i = 0; i < n_links; ++i ) {
        if ( chain[i].head ) {
            printf("%sHead: ", (i == 0) ? " " : "\n ");
        } else {
            printf("       ");
        }
        printf( "cell @row %d, col %d polarity %d\n", chain[i].row, chain[i].col, chain[i].polarity );
    }
}

static void print_forbidden_candidates( int n_hints, cell_ref_t *hints )
{
    for ( int i = 0; i < n_hints; ++i ) {
        printf( "Forbidden candidate @row %d, col %d\n", hints[i].row, hints[i].col );
    }
}

static int add_item_if_not_in_chain( chain_link_t *chain, int n_links,
                                     bool head, int row, int col, int *polarity )
{
    for ( int i = 0; i < n_links; ++i ) {
        if ( chain[i].row == row && chain[i].col == col ) {
            if ( chain[i].polarity != *polarity ) {
                printf( "@@@@ Polarity clash in chain %d (row %d, col %d)\n",
                        i, chain[i].row, chain[i].col );
            } else {
                printf( "@@@@ Same polarity in chain %d (row %d, col %d)\n",
                        i, chain[i].row, chain[i].col );
            }
            // if first clash, polarity will be correct for the second one
            // if second same, polarity does not matter anymore
            return n_links;
        }
    }
    chain[n_links].head = head;
    chain[n_links].row = row;
    chain[n_links].col = col;
    chain[n_links].polarity = *polarity;
    printf("Adding chain %d (head %d, row %d, col %d, polarity %d)\n",
            n_links, head, row, col, *polarity );
    *polarity *= -1;    // reverse polarity for the next one
    return n_links+1;
}

static int add_link_2_chain( chain_link_t *chain, int n_links, bool head, int polarity, cell_ref_t *link )
{
    for ( int i = 0; i < 2; ++i ) {
        n_links = add_item_if_not_in_chain( chain, n_links, head, link[i].row, link[i].col, &polarity );
        head = false;
    }
    return n_links;
}

static void remove_candidate_locations( locate_t by, int ref,
                                        candidate_row_location_t *crloc,
                                        candidate_col_location_t *ccloc,
                                        candidate_box_location_t *cbloc )
{
    switch( by ) {
    case LOCATE_BY_ROW: crloc[ref].n_cols = 0;  break;
    case LOCATE_BY_COL: ccloc[ref].n_rows = 0;  break;
    case LOCATE_BY_BOX: cbloc[ref].n_cells = 0; break;
    }
}

static void get_link( locate_t by, int ref,
                      candidate_row_location_t *crloc,
                      candidate_col_location_t *ccloc,
                      candidate_box_location_t *cbloc,
                      cell_ref_t *link_ref )
{
    int map;
    switch( by ) {
    case LOCATE_BY_ROW: map = crloc[ref].col_map; break;
    case LOCATE_BY_COL: map = ccloc[ref].row_map; break;
    case LOCATE_BY_BOX: map = cbloc[ref].cell_map; break;
    }

    for ( int i = 0; i < 2; ++i ) {
        int index = extract_bit( &map );
        switch( by ) {
        case LOCATE_BY_ROW:
            link_ref[i].row = ref;
            link_ref[i].col = index;
            break;
        case LOCATE_BY_COL:
            link_ref[i].row = index;
            link_ref[i].col = ref;
            break;
        case LOCATE_BY_BOX:
            link_ref[i].row = get_row_from_box_index( ref, index );
            link_ref[i].col = get_col_from_box_index( ref, index );
            break;
        }
    }
}

static int recursively_append_2_chain( chain_link_t *chain, int n_links, 
                                       locate_t by, int ref, bool head, int polarity,
                                       candidate_row_location_t *crloc,
                                       candidate_col_location_t *ccloc,
                                       candidate_box_location_t *cbloc )
{
    int n_links_before_appending = n_links;
    cell_ref_t link_ref[2];
    get_link( by, ref, crloc, ccloc, cbloc, link_ref ); // 1 link is 2 cell references
    n_links = add_link_2_chain( chain, n_links, head, polarity, link_ref );
    remove_candidate_locations( by, ref, crloc, ccloc, cbloc );

    for ( int i = n_links-1; i >= n_links_before_appending; --i ) {
        int r = chain[i].row;
        if ( 2 == crloc[r].n_cols ) {
            n_links = recursively_append_2_chain( chain, n_links, LOCATE_BY_ROW, r,
                                                  false, -1 * chain[i].polarity,
                                                  crloc, ccloc, cbloc );
        }
        int c = chain[i].col;
        if ( 2 == ccloc[c].n_rows ) {
            n_links = recursively_append_2_chain( chain, n_links, LOCATE_BY_COL, c,
                                                  false, -1 * chain[i].polarity,
                                                  crloc, ccloc, cbloc );
        }
        int b = get_surrounding_box( chain[i].row, chain[i].col );
        if ( 2 == cbloc[b].n_cells ) {
            n_links = recursively_append_2_chain( chain, n_links, LOCATE_BY_BOX, b,
                                                  false, -1 * chain[i].polarity,
                                                  crloc, ccloc, cbloc );
        }
    }
    return n_links;
}


static int search_for_forbidding_chains( int *selection_row, int *selection_col )
{
    int candidate_map = get_candidate_map();

    candidate_row_location_t crloc[SUDOKU_N_ROWS];
    candidate_col_location_t ccloc[SUDOKU_N_COLS];
    candidate_box_location_t cbloc[SUDOKU_N_BOXES];

    while ( true ) {
        int candidate = extract_bit( &candidate_map );
        if ( -1 == candidate ) break;

        int n_locations = get_locations_in_rows_cols_boxes( 1 << candidate, crloc, ccloc, cbloc );
        if ( n_locations < 4 ) continue; // cannot be a useful forbidding chain, skip

        chain_link_t chain[SUDOKU_N_ROWS*2 + SUDOKU_N_COLS*2 + SUDOKU_N_BOXES*2];
        int n_links = 0;

        for ( int r = 0; r < SUDOKU_N_ROWS; ++r ) {
            if ( 2 == crloc[r].n_cols ) {
                n_links = recursively_append_2_chain( chain, n_links, LOCATE_BY_ROW, r,
                                                      true, 1, crloc, ccloc, cbloc );
            }
        }
        for ( int c = 0; c < SUDOKU_N_COLS; ++c ) {
            if ( 2 == ccloc[c].n_rows ) {
                n_links = recursively_append_2_chain( chain, n_links, LOCATE_BY_COL, c,
                                                      true, 1, crloc, ccloc, cbloc );
            }
        }
        for ( int b = 0; b < SUDOKU_N_BOXES; ++b ) {
            if ( 2 == cbloc[b].n_cells ) {
                n_links = recursively_append_2_chain( chain, n_links, LOCATE_BY_BOX, b,
                                                      true, 1, crloc, ccloc, cbloc );
            }
        }
        if ( n_links < 4 ) continue;

        printf(">>> candidate %d:\n", candidate );
        print_chain( chain, n_links );

        cell_ref_t hints[SUDOKU_N_ROWS*2 + SUDOKU_N_COLS*2 + SUDOKU_N_BOXES*2];
        int n_hints = locate_forbidden_candidates( chain, n_links, 1 << candidate, hints,
                                                   selection_row, selection_col );
        if ( n_hints > 0 ) { 
            printf("    %d direct hints:\n", n_hints );
            print_forbidden_candidates( n_hints, hints );
            setup_chain_hints_triggers( chain, n_links, hints, n_hints );
            return 0;
        }
        n_hints = process_weak_relations( chain, n_links, 1 << candidate, hints,
                                          selection_row, selection_col );
        if ( n_hints > 0 ) { 
            printf("    %d weak relation hints:\n", n_hints );
            print_forbidden_candidates( n_hints, hints );
            setup_chain_hints_triggers( chain, n_links, hints, n_hints );
            return 0;
        }
    }
    return -1;
}

static sudoku_hint_type cache_hint_cells( int *selection_row, int *selection_col )
{
    int nb_states_to_save = get_redo_level();
    init_state_for_solving( nb_states_to_save );
    reset_hint_cache( );

    hint_desc_t hdesc;
    init_hint_desc( &hdesc );

    // 1. Search for a Naked Single - must be done first for removing single symbols from the pencils.
    if ( look_for_naked_singles( &hdesc ) ) {
        cache_hint_from_desc( &hdesc ); // to move in caller
        *selection_row = hdesc.selection.row;
        *selection_col = hdesc.selection.col;
        return hdesc.hint_type;
    }

    // 2. Search for a Hidden Single
    if ( look_for_hidden_singles( &hdesc ) ) {
        cache_hint_from_desc( &hdesc ); // to move in caller
        *selection_row = hdesc.selection.row;
        *selection_col = hdesc.selection.col;
        return hdesc.hint_type;
    }

    int res;

    // 3. Search for locked candidates
    res = check_locked_candidates( &hdesc );
    if ( 0 != res ) {
        cache_hint_from_desc( &hdesc ); // to move in caller
        *selection_row = hdesc.selection.row;
        *selection_col = hdesc.selection.col;
        return hdesc.hint_type;
    }

    // 4. Search for naked or hidden Subset
    res = check_subsets( &hdesc );
    if ( 0 != res ) {
        cache_hint_from_desc( &hdesc ); // to move in caller
        *selection_row = hdesc.selection.row;
        *selection_col = hdesc.selection.col;
        return hdesc.hint_type;
    }
#if 0
    // 5. Search for X-Wing, Swordfish and Jellyfish
    res = check_X_wings_Swordfish( &hint_type, selection_row, selection_col );
    if ( -1 != res ) {
        return hint_type;
    }

    // 6. Search for XY-Wing
    res = search_for_xy_wing( &hint_type, selection_row, selection_col );
    if ( -1 != res ) {
        return XY_WING;
    }

    // 7. Search for forbidding chains
    res = search_for_forbidding_chains( selection_row, selection_col );
    if ( -1 != res ) {
        return CHAIN;
    }

    if ( NO_HINT != hint_type ) {
        return hint_type;
    }
#endif
    return NO_HINT;
}

extern sudoku_hint_type find_hint( int *selection_row, int *selection_col )
{
//printf("Before starting hints:\n");
//print_grid_pencils();

    int sp = get_sp();
    set_low_water_mark( sp );

    sudoku_hint_type hint = cache_hint_cells( selection_row, selection_col );
    return_to_low_water_mark( sp );
//printf("Before calling write_cache_hints:\n");
//print_grid_pencils();
    write_cache_hints( );
//printf("After calling write_cache_hints:\n");
//print_grid_pencils();
    return hint;
}
