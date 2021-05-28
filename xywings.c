
/*  xywings.c

    This file implements hints about various instances of XY Wings.
*/

#include "hsupport.h"
#include "xywings.h"

/* Look for XY wings. This must be done after looking for naked
   singles in order to benefit from the penciled candidate clean up. */

static int get_common_symbol_mask( cell_ref_t *c0, cell_ref_t *c1 )
{
    sudoku_cell_t *cell_1 = get_cell( c0->row, c0->col );
    sudoku_cell_t *cell_2 = get_cell( c1->row, c1->col );
    return cell_1->symbol_map & cell_2->symbol_map;
}

typedef enum {
    NO_XY_WING, XY_WING_2_HORIZONTAL_BOXES, XY_WING_2_VERTICAL_BOXES, XY_WING_3_BOXES
} xy_wing_geometry;

static xy_wing_geometry set_2_box_horizontal_hints( int b0_col, int b1_col, int c0_row, int c0_col,
                                                    int c1_row, hint_desc_t *hdesc )
{
    bool single = false;
    int n_hints = 0;
    for ( int c = b1_col; c < b1_col + 3; ++c ) {
        sudoku_cell_t *cell = get_cell( c1_row, c );
        if ( 1 < cell->n_symbols && ( hdesc->symbol_map & cell->symbol_map ) ) {
            hdesc->hints[n_hints].row = c1_row;
            hdesc->hints[n_hints].col = c;
            if ( ! single && 2 == cell->n_symbols ) {
                hdesc->selection = hdesc->hints[n_hints];
                single = true;
            }
            ++n_hints;
        }
    }
    for ( int c = b0_col; c < b0_col + 3; ++c ) {
        if ( c == c0_col ) continue;

        sudoku_cell_t *cell = get_cell( c0_row, c );
        if ( 1 < cell->n_symbols && ( hdesc->symbol_map & cell->symbol_map ) ) {
            hdesc->hints[n_hints].row = c0_row;
            hdesc->hints[n_hints].col = c;
            if ( ! single && 2 == cell->n_symbols ) {
                hdesc->selection = hdesc->hints[n_hints];
                single = true;
            }
            ++n_hints;
        }
    }
    hdesc->n_hints = n_hints;
    if ( n_hints ) {
        hdesc->n_triggers = 3;
        return XY_WING_2_HORIZONTAL_BOXES;
    }
    return NO_XY_WING;
}

static xy_wing_geometry set_2_box_vertical_hints( int b0_row, int b1_row, int c0_row, int c0_col,
                                                  int c1_col, hint_desc_t *hdesc )
{
    bool single = false;
    int n_hints = 0;
    for ( int r = b1_row; r < b1_row + 3; ++r ) {
        sudoku_cell_t *cell = get_cell( r, c1_col );
        if ( 1 < cell->n_symbols && ( hdesc->symbol_map & cell->symbol_map ) ) {
            hdesc->hints[n_hints].row = r;
            hdesc->hints[n_hints].col = c1_col;
            if ( ! single && 2 == cell->n_symbols ) {
                hdesc->selection = hdesc->hints[n_hints];
                single = true;
            }
            ++n_hints;
        }
    }
    for ( int r = b0_row; r < b0_row + 3; ++r ) {
        if ( r == c0_row ) continue;

        sudoku_cell_t *cell = get_cell( r, c0_col );
        if ( 1 < cell->n_symbols && ( hdesc->symbol_map & cell->symbol_map ) ) {
            hdesc->hints[n_hints].row = r;
            hdesc->hints[n_hints].col = c0_col;
            if ( ! single && 2 == cell->n_symbols ) {
                hdesc->selection = hdesc->hints[n_hints];
                single = true;
            }
            ++n_hints;
        }
    }
    hdesc->n_hints = n_hints;
    if ( n_hints ) {
        hdesc->n_triggers = 3;
        return XY_WING_2_VERTICAL_BOXES;
    }
    return NO_XY_WING;
}

static xy_wing_geometry check_2_box_geometry( int box0, int box1, // box0 has 2 cells, box1 only 1
                                              cell_ref_t *b0_c0, cell_ref_t *b0_c1, cell_ref_t *b1_c2,
                                              hint_desc_t *hdesc )
{
    int b0_row = 3 * (box0 / 3), b1_row = 3 * (box1 / 3);   // first row of each box
    int b0_col = 3 * (box0 % 3), b1_col = 3 * (box1 % 3);   // first col of each box
    
    hdesc->triggers[0] = *b0_c0;         // assume a positive outcome
    hdesc->triggers[1] = *b0_c1;
    hdesc->triggers[2] = *b1_c2;

    hdesc->flavors[0] = REGULAR_TRIGGER | PENCIL;
    hdesc->flavors[1] = REGULAR_TRIGGER | PENCIL;
    hdesc->flavors[2] = REGULAR_TRIGGER | PENCIL;

    if ( b0_row == b1_row ) {           // boxes are horizontally aligned
        if ( b0_c0->row == b0_c1->row ) return NO_XY_WING;

        if ( b0_c0->row == b1_c2->row ) {
            hdesc->symbol_map = get_common_symbol_mask( b0_c1, b1_c2 );
            return set_2_box_horizontal_hints( b0_col, b1_col, b0_c0->row, b0_c0->col, b0_c1->row, hdesc );

        } else if ( b0_c1->row == b1_c2->row ) {
            hdesc->symbol_map = get_common_symbol_mask( b0_c0, b1_c2 );
            return set_2_box_horizontal_hints( b0_col, b1_col, b0_c1->row, b0_c1->col, b0_c0->row, hdesc );
        }
    } else if ( b0_col == b1_col ) {    // boxes are vertically aligned
        if ( b0_c0->col == b0_c1->col ) return NO_XY_WING;

        if ( b0_c0->col == b1_c2->col ) {
            hdesc->symbol_map = get_common_symbol_mask( b0_c1, b1_c2 );
            return set_2_box_vertical_hints( b0_row, b1_row, b0_c0->row, b0_c0->col, b0_c1->col, hdesc );

        } else if ( b0_c1->col == b1_c2->col ) {
            hdesc->symbol_map = get_common_symbol_mask( b0_c0, b1_c2 );
            return set_2_box_vertical_hints( b0_row, b1_row, b0_c1->row, b0_c1->col, b0_c0->col, hdesc );
        }
    }
    return NO_XY_WING;  // not a valid geometry
}

static xy_wing_geometry set_3_box_hints( cell_ref_t *pair_a, cell_ref_t *pair_b, hint_desc_t *hdesc )
{
    hdesc->symbol_map = get_common_symbol_mask( pair_a, pair_b );
    sudoku_cell_t *cell = get_cell( pair_a->row, pair_b->col );
    if ( 1 < cell->n_symbols && ( hdesc->symbol_map & cell->symbol_map ) ) {
        hdesc->hints[0].row = pair_a->row;              // hint row from pair_a
        hdesc->hints[0].col = pair_b->col;              // hint col from pair_b

        if ( 2 == cell->n_symbols ) hdesc->selection = hdesc->hints[0];

        hdesc->n_hints = 1;
        hdesc->n_triggers = 3;
        return XY_WING_3_BOXES; 
    }
    return NO_XY_WING;
}

static xy_wing_geometry check_3_box_geometry( cell_ref_t *pairs, hint_desc_t *hdesc )
{
    hdesc->triggers[0] = pairs[0];         // assume a positive outcome
    hdesc->triggers[1] = pairs[1];
    hdesc->triggers[2] = pairs[2];

    hdesc->flavors[0] = REGULAR_TRIGGER | PENCIL;
    hdesc->flavors[1] = REGULAR_TRIGGER | PENCIL;
    hdesc->flavors[2] = REGULAR_TRIGGER | PENCIL;

    if ( pairs[0].row == pairs[1].row ) {           // p0 & p1 are horizontally aligned:     p0-p1
        if ( pairs[2].col == pairs[0].col ) {           // p0 & p2 are vertically aligned:   p2-hint
            return set_3_box_hints( &pairs[2], &pairs[1], hdesc );  // hint p2->row, p1->col
        } else if ( pairs[2].col == pairs[1].col ) {    // p1 & p2 are vertically aligned: hint-p2
            return set_3_box_hints( &pairs[2], &pairs[0], hdesc );  // hint p2->row, p0->col
        }
    } else if ( pairs[0].row == pairs[2].row ) {    // p0 & p2 are horizontally aligned:     p0-p2
        if ( pairs[1].col == pairs[0].col ) {           // p0 & p1 are vertically aligned:   p1-hint
            return set_3_box_hints( &pairs[1], &pairs[2], hdesc );  // hint p1->row, p2->col
        } else if ( pairs[1].col == pairs[2].col ) {    // p1 & p2 are vertically aligned: hint-p1
            return set_3_box_hints( &pairs[1], &pairs[0], hdesc );  // hint p1->row, p0->col
        }
    } else if ( pairs[1].row == pairs[2].row ) {    // p1 & p2 are horizontally aligned:     p1-p2
        if ( pairs[0].col == pairs[1].col ) {           // p0 & p1 are vertically aligned:   p0-hint
            return set_3_box_hints( &pairs[0], &pairs[2], hdesc );  // hint p0->row, p2->col
        } else if ( pairs[0].col == pairs[2].col ) {    // p0 & p2 are vertically aligned: hint-p0
            return set_3_box_hints( &pairs[0], &pairs[1], hdesc );  // hint p0->row, p1->col
        }
    }
    return NO_XY_WING;  // not a valid geometry
}

static xy_wing_geometry check_xy_wing_geometry( cell_ref_t *pairs, hint_desc_t *hdesc )
{
    // check if the pairs are in 2 or 3 boxes and if their alignments are correct

    int box0 = get_cell_ref_box( &pairs[0] );
    int box1 = get_cell_ref_box( &pairs[1] );
    int box2 = get_cell_ref_box( &pairs[2] );

    if ( box0 == box1 ) {
        if ( box0 == box2 ) return NO_XY_WING;
        return check_2_box_geometry( box0, box2, &pairs[0], &pairs[1], &pairs[2], hdesc );

    } else if ( box0 == box2 ) {
        return check_2_box_geometry( box0, box1, &pairs[0], &pairs[2], &pairs[1], hdesc );

    } else if ( box1 == box2 ) {
        return check_2_box_geometry( box1, box0, &pairs[1], &pairs[2], &pairs[0], hdesc );
    }
    return check_3_box_geometry( pairs, hdesc );
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

static xy_wing_geometry get_3rd_matching_pair( int symbol_map, int n_pairs, cell_ref_t *pairs,
                                               cell_ref_t *matching_pairs, hint_desc_t *hdesc )
{
    for ( int j = 0; j < n_pairs; ++ j ) {
        sudoku_cell_t *cell = get_cell( pairs[j].row, pairs[j].col );
        if ( cell->symbol_map == symbol_map ) {
            matching_pairs[2] = pairs[j];
            xy_wing_geometry geometry = check_xy_wing_geometry( matching_pairs, hdesc );
            if ( NO_XY_WING != geometry ) return geometry;
        } // else try another 3rd pair
    }
    return NO_XY_WING;
}

static xy_wing_geometry search_for_xy_wing_in_matching_pairs( int n_pairs, cell_ref_t *pairs,
                                                              cell_ref_t *matching_pairs, hint_desc_t *hdesc )
{
    sudoku_cell_t *cell_0 = get_cell( matching_pairs[0].row, matching_pairs[0].col );
    int symbol_map_0 = cell_0->symbol_map;                              // 2^s0 | 2^s1
    int s0_mask, s1_mask;
    get_pair_symbols( symbol_map_0, &s0_mask, &s1_mask );

    for ( int i = 0; i < n_pairs; ++i ) {
        sudoku_cell_t *cell_1 = get_cell( pairs[i].row, pairs[i].col );
        int map = cell_1->symbol_map;
        if ( map == symbol_map_0 ) continue;    // 2^s0 | 2^s1 : naked subset, try another pair

        xy_wing_geometry geo;
        matching_pairs[1] = pairs[i];
        if ( map & s0_mask ) {                  // 2^s0 | 2^s2 (new symbol s2)
            geo = get_3rd_matching_pair( s1_mask | (map & ~s0_mask), // 2^s1 | 2^s2 in remaining pairs
                                         n_pairs - (i+1), &pairs[i+1], matching_pairs, hdesc );
            if ( NO_XY_WING != geo ) return true;

        } else if ( map & s1_mask ) {           // 2^s1 | 2^s2 (new symbol s2)
            geo = get_3rd_matching_pair( s0_mask | (map & ~s1_mask), // 2^s0 | 2^s2 in remaining pairs
                                         n_pairs - (i+1), &pairs[i+1], matching_pairs, hdesc );
            if ( NO_XY_WING != geo ) return true;
        } // else try another pair
    }
    return NO_XY_WING;
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

extern bool search_for_xy_wing( hint_desc_t *hdesc )
{
    cell_ref_t pairs[ SUDOKU_N_SYMBOLS * SUDOKU_N_SYMBOLS ]; // must be less than the complete grid
    int n_pairs = get_symbol_pairs( pairs );
    if ( n_pairs < 3 ) return -1;


    cell_ref_t matching_pairs[3];
    for ( int i = 0; i < n_pairs; ++i ) {
        matching_pairs[0] = pairs[i];
        xy_wing_geometry geo = search_for_xy_wing_in_matching_pairs( n_pairs-(i+1), &pairs[i+1],
                                                                     matching_pairs, hdesc );
        if ( NO_XY_WING == geo ) continue;

        hdesc->hint_type = XY_WING;
        hdesc->hint_pencil = true;
        hdesc->action = REMOVE;
        hdesc->n_symbols = 1;
        return true;
    }
    return false;
}

