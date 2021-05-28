
/*  chains.c

    This file implements hints about various instances of forbidding chains.
*/

#include "hsupport.h"
#include "chains.h"

/* Look for single and multiple forbidding chains. This must be done after looking
   for naked singles in order to benefit from the pencil clean up. */

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

static bool locate_forbidden_candidates( chain_link_t *chain, int n_links,
                                         int symbol_mask, hint_desc_t *hdesc )
//cell_ref_t * hints, int *selection_row, int *selection_col
/* Based on reversed polarities inside a single chain:
    a cell at the intersection of row including one polarity and
                                  col including the other polarity is forbidden */
{
    struct prow {
        int chain_offset /* in chain */, index /* in row or col */;
    } prows[ SUDOKU_N_ROWS], pcols[ SUDOKU_N_COLS ];

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

        int n_hints = 0;
        for ( int i = 0; i < n_prows; ++i ) {   // make up candidates and check if they contain the symbol
            for ( int j = 0; j < n_pcols; ++j ) {
                if ( chain[prows[i].chain_offset].polarity != chain[pcols[j].chain_offset].polarity ) {
                    sudoku_cell_t *cell = get_cell( prows[i].index, pcols[j].index );
                    if ( cell->n_symbols > 1 && ( symbol_mask & cell->symbol_map ) ) {
                        hdesc->hints[n_hints].row = prows[i].index;
                        hdesc->hints[n_hints].col = pcols[j].index;
                        ++n_hints;

                        if ( 2 == cell->n_symbols ) {
                            hdesc->selection.row = prows[i].index;
                            hdesc->selection.col = pcols[j].index;
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
            hdesc->n_hints = n_hints;
            hdesc->hint_pencil = true;
            hdesc->symbol_map = symbol_mask;
            hdesc->n_symbols = 1;
            hdesc->action = REMOVE;   // TODO: change to the proper action
            return true;                        // stop here since segment generated hints
        }
        start = end + 1;                        // else try next segment, till the end of chain
        if ( start == n_links ) break;
    }
    return false;
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

static bool find_chain_exclusions( chain_link_t *chain, int symbol_mask,
                                   int seg1_beg, int seg1_end,
                                   int seg2_beg, int seg2_end,
                                   hint_desc_t *hdesc )
{
    int polarity = 0;
    int excluded = -1;
    int prev_seg1, prev_seg2;

    int n_hints = 0;
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
            hdesc->selection.row = chain[excluded].row;
            hdesc->selection.col = chain[excluded].col;

            int end = ( excluded < seg2_beg ) ? seg1_end : seg2_end;
            int polarity = chain[excluded].polarity;
            for ( int k = excluded; k <= end; ++ k ) {
                if ( chain[k].polarity != polarity ) continue;
                hdesc->hints[n_hints + hdesc->n_hints].row = chain[k].row;
                hdesc->hints[n_hints + hdesc->n_hints].col = chain[k].col;
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
                if ( ! is_cell_in_chain( chain, seg1_beg, seg1_end,
                                         chain[seg1_index].row, chain[seg2_index].col ) &&
                     ! is_cell_in_chain( chain, seg2_beg, seg2_end,
                                         chain[seg1_index].row, chain[seg2_index].col ) ) {
                    cell = get_cell( chain[seg1_index].row, chain[seg2_index].col );
                    if ( cell->symbol_map & symbol_mask ) {
                        hdesc->hints[n_hints + hdesc->n_hints].row = chain[seg1_index].row;
                        hdesc->hints[n_hints + hdesc->n_hints].col = chain[seg2_index].col;
                        ++n_hints;
                        continue;
                    }
                }
                if ( ! is_cell_in_chain( chain, seg1_beg, seg1_end,
                                         chain[seg2_index].row, chain[seg1_index].col ) &&
                     ! is_cell_in_chain( chain, seg2_beg, seg2_end,
                                         chain[seg2_index].row, chain[seg1_index].col ) ) {
                    cell = get_cell( chain[seg2_index].row, chain[seg1_index].col );
                    if ( cell->symbol_map & symbol_mask ) {
                        hdesc->hints[n_hints + hdesc->n_hints].row = chain[seg2_index].row;
                        hdesc->hints[n_hints + hdesc->n_hints].col = chain[seg1_index].col;
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
    hdesc->n_hints += n_hints;
    return n_hints > 0;
}

static bool hide_inactive_segments( chain_link_t *chain, chain_segment_t *segments, int n_segments )
{
    bool active = false;
    for ( int i = 0; i < n_segments; ++i ) {
        if ( segments[i].active ) {
            active = true;
            continue;
        }
        for ( int j = segments[i].beg; j <= segments[i].end; ++ j ) {
             chain[j].polarity = 0;
        }
    }
    return active;
}

static int process_weak_relations( chain_link_t *chain, int n_links, int symbol_mask, hint_desc_t *hdesc )
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

    for ( int i = 0; i < n_segments-1; ++ i ) {   // find weak links beween 2 segments
        for ( int j = i+1; j < n_segments; ++ j ) {
            if ( find_chain_exclusions( chain, symbol_mask,
                                        segments[i].beg, segments[i].end,
                                         segments[j].beg, segments[j].end,
                                         hdesc ) ) {
                // segments i & j provide some hints: make them active
                printf( "Active segments %d, %d\n", i, j );
                segments[i].active = segments[j].active = true;
            }
        }
    }
    return hide_inactive_segments( chain, segments, n_segments );
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
        int index = extract_bit_from_map( &map );
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

static void print_forbidden_candidates( hint_desc_t *hdesc )
{
    for ( int i = 0; i < hdesc->n_hints; ++i ) {
        printf( "Forbidden candidate @row %d, col %d\n", hdesc->hints[i].row, hdesc->hints[i].col );
    }
}

static void setup_chain_hints_triggers( chain_link_t *chain, int n_links, hint_desc_t *hdesc )
{
    hdesc->hint_type = CHAIN;
    for ( int i = 0; i < n_links; ++i ) {
        if ( 0 == chain[i].polarity ) continue;

        hdesc->triggers[hdesc->n_triggers].row = chain[i].row;
        hdesc->triggers[hdesc->n_triggers].col = chain[i].col;
        hdesc->flavors[hdesc->n_triggers] = PENCIL | (( chain[i].head ) ? HEAD : 0) | 
                                            (( 1 == chain[i].polarity) ? REGULAR_TRIGGER : ALTERNATE_TRIGGER);
        ++hdesc->n_triggers;
    }
}

extern bool search_for_forbidding_chains( hint_desc_t *hdesc )
{
    int candidate_map = get_candidate_map();

    candidate_row_location_t crloc[SUDOKU_N_ROWS];
    candidate_col_location_t ccloc[SUDOKU_N_COLS];
    candidate_box_location_t cbloc[SUDOKU_N_BOXES];

    while ( true ) {
        int candidate = extract_bit_from_map( &candidate_map );
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

        if ( locate_forbidden_candidates( chain, n_links, 1 << candidate, hdesc ) ) {
            printf("    %d direct hints:\n", hdesc->n_hints );
            print_forbidden_candidates( hdesc );
            setup_chain_hints_triggers( chain, n_links, hdesc );
            return true;
        }
        if ( process_weak_relations( chain, n_links, 1 << candidate, hdesc ) ) { 
            printf("    %d weak relation hints:\n", hdesc->n_hints );
            print_forbidden_candidates( hdesc );
            setup_chain_hints_triggers( chain, n_links, hdesc );
            return true;
        }
    }
    return false;
}

