
/*  fishes.c

    This file implements hints about various instances of X-Wings,
    swordfish and jellyfish configurations.
*/

#include "hsupport.h"
#include "fishes.h"

/* Look for X wings, Swordfish & jellyfish. This must be done after looking
   for naked singles in order to benefit from the penciled candidates clean up. */

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
                                     int location_map, hint_desc_t *hdesc )
// returns -1 if no useful hint, 0 if some hints but no new single, 1 if hints with at least a new single
{
    int indexes[4] = { 0 };
    int n_indexes = 0, index_map = location_map;
    while ( true ) {
        int index = extract_bit_from_map( &index_map );
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
            cell_ref_t cr;
            get_cell_ref_in_set( by, rc, indexes[i], &cr );
            sudoku_cell_t *cell = get_cell( cr.row, cr.col );

            if ( cell->n_symbols > 1 && ( cell->symbol_map & symbol_mask ) ) {
                if ( ! hint ) {
                    hdesc->hint_pencil = true;
                    hdesc->action = REMOVE;
                    hdesc->n_symbols = 1;
                    hdesc->symbol_map = symbol_mask;
                    hint = true;
                }
                hdesc->hints[hdesc->n_hints++] = cr;
                if ( 2 == cell->n_symbols ) {
                    hdesc->selection = cr;
                    new_single = true;
                }
            }
        }
    }
    if ( hint ) {   // indicate triggers too
        for ( int i = 0; i < n_refs; ++i ) {
            for ( int j = 0; j < n_indexes; ++j ) {
                cell_ref_t cr;
                get_cell_ref_in_set( by, refs[i], indexes[j], &cr );
                sudoku_cell_t *cell = get_cell( cr.row, cr.col );

                if ( cell->n_symbols > 1 && ( cell->symbol_map & symbol_mask ) ) {
                    hdesc->triggers[hdesc->n_triggers] = cr;
                    hdesc->flavors[hdesc->n_triggers] = REGULAR_TRIGGER | PENCIL;
                    ++hdesc->n_triggers;
                }
            }
        }
    }
    return ( new_single ) ? 1 : (( hint ) ? 0 : -1);
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

static int search_for_X_wings_n_fish_hints( locate_t by, int symbol_map, 
                                            symbol_locations_t *ref_sloc, hint_desc_t *hdesc )
// returns -1 if no useful hint, 0 if some hints but no new single, 1 if hints with at least a new single
{
    int location_map = 0;
    int refs[4];                        // no more than 4 sets for jellyfish

    if ( search_for_fish_configuration( 2, refs, &location_map, ref_sloc ) ) {
        int res = set_X_wings_n_fish_hints( by, symbol_map, 2, refs,
                                            location_map, hdesc );
        if ( -1 != res ) {
            hdesc->hint_type = XWING;
            return res;
        }
    }

    if ( search_for_fish_configuration( 3, refs, &location_map, ref_sloc ) ) {
        int res = set_X_wings_n_fish_hints( by, symbol_map, 3, refs,
                                            location_map, hdesc );
        if ( -1 != res ) {
            hdesc->hint_type = SWORDFISH;
            return res;
        }
    }

    if ( search_for_fish_configuration( 4, refs, &location_map, ref_sloc ) ) {
        int res = set_X_wings_n_fish_hints( by, symbol_map, 4, refs,
                                            location_map, hdesc );
        if ( -1 != res ) {
            hdesc->hint_type = JELLYFISH;
            return res;
        }
    }
    return -1;
}

extern bool check_X_wings_Swordfish( hint_desc_t *hdesc )
// return 0 if no useful hint, 1 if some hints or 2 if at least one hint leads to a new single
{
    for ( int symbol = 0; symbol < SUDOKU_N_SYMBOLS; ++symbol ) {

        int symbol_map = 1 << symbol;
        symbol_locations_t row_sloc[9], col_sloc[9];
        get_symbol_locations_in_set( LOCATE_BY_ROW, symbol_map, row_sloc );
        get_symbol_locations_in_set( LOCATE_BY_COL, symbol_map, col_sloc );

        int res;
        // 1. search for X-Wing, Swordfish or Jellyfish in rows
        res = search_for_X_wings_n_fish_hints( LOCATE_BY_ROW, symbol_map,
                                               row_sloc, hdesc );
        if ( res != -1 ) return true;
 
        // 2. search for X-Wing, Swordfish or Jellyfish in columns
        res = search_for_X_wings_n_fish_hints( LOCATE_BY_COL, symbol_map,
                                               col_sloc, hdesc );
        if ( res != -1 ) return true;
    }
    return false;
}

