/*
  Sudoku hint finder
*/
#include <string.h>

#include "grid.h"
#include "game.h"
#include "hint.h"
#include "hsupport.h"
#include "singles.h"
#include "locked.h"
#include "subsets.h"
#include "fishes.h"
#include "xywings.h"
#include "chains.h"

extern void get_other_boxes_in_same_box_row( int box, int *other_boxes )
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

extern void get_other_boxes_in_same_box_col( int box, int *other_boxes )
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

extern void get_cell_ref_in_set( locate_t by, int ref, int index, cell_ref_t *cr )
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

extern void get_box_row_intersection( int box, int row, cell_ref_t *intersection )
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

extern void get_box_col_intersection( int box, int col, cell_ref_t *intersection )
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

extern bool get_single_for_mask_in_set( locate_t by, int ref, int single_mask, cell_ref_t *single )
// look for a single matching a given mask that fits in the given set
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

extern cell_ref_t * get_single_in_box( cell_ref_t *singles, int n_singles, int box )
// Get the single that fits in a box among a list of all singles in the game
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

extern cell_ref_t * get_single_in_row( cell_ref_t *singles, int n_singles, int row )
// Get the single that fits in a row among a list of all singles in the game
{
    for ( int i = 0; i < n_singles; ++ i ) {
        if ( singles[i].row == row ) return &singles[i];
    }
    return NULL;
}

extern cell_ref_t * get_single_in_col( cell_ref_t *singles, int n_singles, int col )
// Get the single that fits in a col among a list of all singles in the game
{
    for ( int i = 0; i < n_singles; ++ i ) {
        if ( singles[i].col == col ) return &singles[i];
    }
    return NULL;
}

static void hint_desc_init( hint_desc_t *hdesc )
{
    memset( hdesc, 0, sizeof(hint_desc_t) );
    hdesc->selection.row = hdesc->selection.col = -1;   // no selection by default
}

static char *get_flavor( cell_attrb_t attrb )
{
    static char buffer[ 64 ];

    char *flavor = NULL;
    int n = 0;

#define RT  "REGULAR_TRIGGER "
#define WT  "WEAK_TRIGGER "
#define AT  "ALTERNATE_TRIGGER "

    if ( attrb & REGULAR_TRIGGER ) {
        flavor = RT;
        n = sizeof(RT);
    } else if ( attrb & WEAK_TRIGGER ) {
        flavor = WT;
        n = sizeof(WT);
    } else if ( attrb & ALTERNATE_TRIGGER ) {
        flavor = AT;
        n = sizeof(AT);
    } else {
        printf( "get_flavor: attrb=%d\n", attrb );
        SUDOKU_ASSERT( 0 );
    }

    strcpy( buffer, flavor );
    buffer[ n++ ] = '|';

#define HD  " HEAD "
#define PC  " PENCIL "

    if( attrb & HEAD ) {
        strcpy( &buffer[n], HD );
        n += sizeof(HD);
    }
    if ( attrb & PENCIL ) {
        strcpy( &buffer[n], PC );
    }
    return buffer;
}

static void set_cell_attributes_from_desc( hint_desc_t *hdesc )
{
    printf("set_cell_attributes_from_desc:\n");
    printf(" hint_type %d\n", hdesc->hint_type);
    printf(" n_hints %d\n", hdesc->n_hints);
    printf(" n_triggers %d\n", hdesc->n_triggers);
    printf(" n_candidates %d\n", hdesc->n_candidates);

    int attrb = ( hdesc->hint_pencil ) ? HINT | PENCIL : HINT;
    for ( int i = 0; i < hdesc->n_hints; ++i ) {
        set_cell_attributes( hdesc->hints[i].row, hdesc->hints[i].col, attrb );
        printf(" hints[%d] = (%d, %d)\n", i, hdesc->hints[i].row, hdesc->hints[i].col);
    }

    for ( int i = 0; i < hdesc->n_triggers; ++i ) {
        set_cell_attributes( hdesc->triggers[i].row, hdesc->triggers[i].col, hdesc->flavors[i] );
        printf(" triggers[%d] flavor %s = (%d, %d)\n", i, get_flavor(hdesc->flavors[i]),
               hdesc->triggers[i].row, hdesc->triggers[i].col);
    }
    for ( int i = 0; i < hdesc->n_candidates; ++i ) {
        set_cell_attributes( hdesc->candidates[i].row, hdesc->candidates[i].col, ALTERNATE_TRIGGER | PENCIL );
    }

    printf(" selection (%d, %d)\n", hdesc->selection.row, hdesc->selection.col);
    printf(" action %d n_symbols=%d, map=0x%03x\n", hdesc->action, hdesc->n_symbols, hdesc->symbol_map);
}

extern bool get_hint( hint_desc_t *hdp )
{
    hint_desc_init( hdp );
    // 1. Search for a Naked Single - must be done first for removing single symbols from the pencils.
    if ( look_for_naked_singles( hdp ) ) return true;
    // 2. Search for a Hidden Single
    if ( look_for_hidden_singles( hdp ) ) return true;
    // 3. Search for locked candidates
    if ( check_locked_candidates( hdp ) ) return true;
    // 4. Search for naked or hidden Subset
    if ( check_subsets( hdp ) ) return true;
    // 5. Search for X-Wing, Swordfish and Jellyfish
    if ( check_X_wings_Swordfish( hdp ) ) return true;
    // 6. Search for XY-Wing
    if ( search_for_xy_wing( hdp ) ) return true;
    // 7. Search for forbidding chains
    if ( search_for_forbidding_chains( hdp ) ) return true;

    SUDOKU_ASSERT( NO_HINT == hdp->hint_type );
    SUDOKU_ASSERT( 0 == hdp->n_hints );
    SUDOKU_ASSERT( 0 == hdp->n_triggers );
    SUDOKU_ASSERT( 0 == hdp->n_candidates );
    SUDOKU_ASSERT( -1 == hdp->selection.row && -1 == hdp->selection.col );

    return false;
}

extern bool act_on_hint( hint_desc_t *hdesc )
{
    SUDOKU_ASSERT( hdesc->n_hints );
    switch ( hdesc->action ) {
    case NONE: case ADD:
        printf("Action %d\n", hdesc->action );
        SUDOKU_ASSERT( 0 );
    case SET:
        for ( int i = 0; i < hdesc->n_hints; ++i ) {
            set_cell_candidates( hdesc->hints[i].row, hdesc->hints[i].col,
                                 hdesc->n_symbols, hdesc->symbol_map );
        }
        break;
    case REMOVE:
        for ( int i = 0; i < hdesc->n_hints; ++i ) {
            remove_cell_candidates( hdesc->hints[i].row, hdesc->hints[i].col,
                                    hdesc->n_symbols, hdesc->symbol_map );
        }
        break;
    }
    return is_game_solved(); 
}

static void show_hint_pencils( hint_desc_t *hdesc )
{
    if ( ! hdesc->hint_pencil ) return;
    for ( int i = 0; i < hdesc->n_hints; ++i ) {
        set_cell_attributes( hdesc->hints[i].row, hdesc->hints[i].col, PENCIL );
    }
}

extern int solve_step( void )
{
    void *game = save_current_game_for_solving();
    hint_desc_t hdesc;
    bool hint = get_hint( &hdesc );
    restore_saved_game( game );

    if ( hint ) {
        game_new_grid( );           // make sure this step will be undoable
        show_hint_pencils( &hdesc );
        if ( act_on_hint( &hdesc ) ) return 2;
        return 1;
    }
    return 0;
}

extern sudoku_hint_type find_hint( int *selection_row, int *selection_col )
{
//printf("Before starting hints:\n");
//print_grid_pencils();

    void *game = save_current_game_for_solving();
    hint_desc_t hdesc;
    bool hint = get_hint( &hdesc );
    restore_saved_game( game );

    if ( hint ) {
        *selection_row = hdesc.selection.row;
        *selection_col = hdesc.selection.col;
        set_cell_attributes_from_desc( &hdesc );
    }
    return hdesc.hint_type;
}
