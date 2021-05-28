/*  subsets.c

    This file implements hints about various instances of subsets
    (pairs, triplets, quadruplets) candidates.
*/

#include "hsupport.h"
#include "subsets.h"

/* Look for naked and hidden subsets (size > 1). This must be done after looking
   for naked singles in order to benefit from the penciled-symbol cleanup. */

static bool get_next_combination(int *comb, int k, int n)
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
        int symbol = extract_bit_from_map( &symbol_map );
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
                hdesc->hint_type = HIDDEN_SUBSET;
                hdesc->action = SET;
                hdesc->n_symbols = 2;
                hdesc->symbol_map = symbol_map;
                hdesc->hint_pencil = true;

                if ( crs[0].n_extra ) {
                    hdesc->hints[hdesc->n_hints++] = crs[0].cr;
                    hdesc->selection = crs[0].cr;
                } else { 
                    hdesc->triggers[hdesc->n_triggers] = crs[0].cr;
                    hdesc->flavors[hdesc->n_triggers] = REGULAR_TRIGGER | PENCIL;
                    ++hdesc->n_triggers;
                }
                if ( crs[1].n_extra ) { // if both, selection will be on the 2nd one.
                    hdesc->hints[hdesc->n_hints++] = crs[1].cr;
                    hdesc->selection = crs[1].cr;
                } else {
                    hdesc->triggers[hdesc->n_triggers] = crs[1].cr;
                    hdesc->flavors[hdesc->n_triggers] = REGULAR_TRIGGER | PENCIL;
                    ++hdesc->n_triggers;
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
                hdesc->hint_type = NAKED_SUBSET;
                hdesc->action = REMOVE;
                hdesc->n_symbols = 2;           // symbol map has 2 symbols but only 1 
                hdesc->symbol_map = symbol_map; // of the 2 is to remove from each hint
                hdesc->hint_pencil = true;

                for ( int i = 0; i < n_partial; ++i ) { // symbols from naked pair should be removed
                    hdesc->hints[hdesc->n_hints++] = prs[i].cr;
                    if ( 1 == prs[i].n_extra ) {
                        hdesc->selection = prs[i].cr;
                        res = 1;    // new single
                    }
                }
                for ( int i = 0; i < n_included; ++i ) {
                    if ( 0 == crs[i].n_extra ) {    // the naked pair cells are trigger
                        hdesc->triggers[hdesc->n_triggers] = crs[i].cr;
                        hdesc->flavors[hdesc->n_triggers] = REGULAR_TRIGGER | PENCIL;
                        ++hdesc->n_triggers;
                    } else {                        // symbols from naked pair should be removed
                        hdesc->hints[hdesc->n_hints++] = crs[i].cr;
                        if ( 1 == crs[i].n_extra ) {
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
                    hdesc->hint_type = HIDDEN_SUBSET;
                    hdesc->action = SET;
                    hdesc->n_symbols = 3;
                    hdesc->symbol_map = max_map;
                    hdesc->hint_pencil = true;

                    if ( crs[0].n_extra ) {
                        hdesc->hints[hdesc->n_hints++] = crs[0].cr;
                        hdesc->selection = crs[0].cr;
                    } else { 
                        hdesc->triggers[hdesc->n_triggers] = crs[0].cr;
                        hdesc->flavors[hdesc->n_triggers] = REGULAR_TRIGGER | PENCIL;
                        ++hdesc->n_triggers;
                    }
                    if ( crs[1].n_extra ) {
                        hdesc->hints[hdesc->n_hints++] = crs[1].cr;
                        hdesc->selection = crs[1].cr;
                    } else {
                        hdesc->triggers[hdesc->n_triggers] = crs[1].cr;
                        hdesc->flavors[hdesc->n_triggers] = REGULAR_TRIGGER | PENCIL;
                        ++hdesc->n_triggers;
                    }
                    if ( crs[2].n_extra ) {
                        hdesc->hints[hdesc->n_hints++] = crs[2].cr;
                        hdesc->selection = crs[2].cr;
                    } else {
                        hdesc->triggers[hdesc->n_triggers] = crs[2].cr;
                        hdesc->flavors[hdesc->n_triggers] = REGULAR_TRIGGER | PENCIL;
                        ++hdesc->n_triggers;
                    }
                    return 0;       // return hidden triplet to cleanup
                } // else already processed naked triplet, ignore

            } else if ( 0 == crs[0].n_extra && 0 == crs[1].n_extra && 0 == crs[2].n_extra ) {
                // naked triplet and cells with same symbols: it reduces symbols in partial cells
                hdesc->hint_type = NAKED_SUBSET;
                hdesc->action = REMOVE;
                hdesc->n_symbols = 3;
                hdesc->symbol_map = max_map;
                hdesc->hint_pencil = true;

                hdesc->hints[hdesc->n_hints++] = crs[0].cr;
                hdesc->hints[hdesc->n_hints++] = crs[1].cr;
                hdesc->hints[hdesc->n_hints++] = crs[2].cr;
                int res = 0;
                for ( int i = 0; i < n_partial; ++i ) {
                    hdesc->hints[hdesc->n_hints++] = prs[i].cr;
                    if ( 1 == prs[i].n_extra ) {
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

extern bool check_subsets( hint_desc_t *hdesc )
/* return false if no subset or no candidate that can be removed,
   or true if subset and some candidates can be removed. */
{
    for ( locate_t by = LOCATE_BY_ROW; by <= LOCATE_BY_BOX; ++by ) {
        for ( int ref = 0; ref < SUDOKU_N_SYMBOLS; ++ref ) { // ref = row, col or in box cell index
            int symbols[9];
            int n_symbols = get_symbols( by, ref, symbols );
            if ( n_symbols < 2 ) continue;

            int res = check_pairs( by, ref, n_symbols, symbols, hdesc );
            if ( res != -1 ) return true;

            res = check_triplets( by, ref, n_symbols, symbols, hdesc );
            if ( res != -1 ) return true;
            // TODO: add quadruplets
        }
    }
    return false;
}

