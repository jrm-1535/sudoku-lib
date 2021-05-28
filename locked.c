/*  locked.c

    This file implements hints about various instances of locked candidates.
*/

#include "hsupport.h"
#include "locked.h"

/*
   Look for locked candicates (similar to hidden single but for multiple penciled symbols).
      More precisely :
        1) 1 row where a symbol p (Sp) can fit only in 2 or 3 cells in 1 box, because
            a) all other cells in the same row outside the box are other singles (Sa, Sb, ... Sf).
                Sa Sb Sc |  C  C  C | Sd Se Sf   in this row Sp can only appear in one of 3 cells marked
                         |  H  H  H |            with C in the 2nd box, forbidding Sp in other rows of
                         |  H  H  H |            the same box (X). Cells C are candidates, H are hints.
                ------------------------------
            b) a single with the same symbol (Sp) appears in columns of some other boxes, a.k.a. triggers.
                Sa !p !p |  C  C Sb | Sc !p Sd   in this row Sp can only appear in one of 2 cells marked
                   !p !p |  H  H  H |    !p      with C in the 2nd box, forbidding Sp in other rows of
                   !p !p |  H  H  H |    !p      the same box (X). Cells C are candidates, H are hints,
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
                 p  p  p |  H  H  H | !p !p !p  row of the same box.
                ------------------------------
                Would still provide a good hint (H) on the same box.

        2) 1 box where a symbol can appear only in 2 or 3 cells in 1 row within that box, because
            the same symbol (T) appears in 1 other other box in a row intersecting the box AND in yet
            another box in a column intersecting a row outside the first box:
                 H  H  H |  p  p !p | !p !p !p  A trigger T forbids other cells to hold Sp (indicated
                !p !p !p | !p !p !p | !p !p  T  as !p) in the same row. Another trigger T in a column
                 C  C  C | Sa Sb !p | !p !p !p  prevents the remaining cell in the row to hold Sp.
                ------------------------------
                         |        T |

         Rule for 1 & 2: find a row where a candidate can occur only within 1 box (2 or 3 cells)
                          => the same symbol can be removed from the other cells in that box.
                          hint only if the symbol can be found in the other cells of the box.

        3) 1 column where a symbol can fit only in 2 or 3 cells in 1 box for reasons similar to 1),
          after transposing rows and columns: locked candidate in box column

        4) 1 box where a symbol can appear only in 2 or 3 cells in 1 box for reasons similar to 2).
                !p Sa  p |
                !p !p !p |  T
                !p Sb  p |
                -------------------------------
                !p  H  C |
                !p  H  C |
                !p  H  C |
                -------------------------------
                 T !p !p |
                !p !p !p |
                !p !p !p |

         Rule for 3 & 4: find a column where a candidate can occur only within 1 box (2 or 3 cells)
                          => the same symbol can be removed from the other cells in that box.
                          hint only if the symbol can be found in the other cells of the box.

        This must be done after looking for naked singles in order to benefit from pencils clean up.
*/

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
                hdesc->flavors[n_triggers] = REGULAR_TRIGGER;
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
                hdesc->flavors[n_triggers] = REGULAR_TRIGGER;
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

    int n_triggers = 0;
    for ( int i = 0; i < 2; ++i ) {                 // first, triggers inside box (i is box index)
        cell_ref_t *single = get_single_in_box( singles, n_singles, boxes[i] );
        if ( NULL == single ) continue;

        hdesc->triggers[n_triggers] = *single;
        hdesc->flavors[n_triggers] = REGULAR_TRIGGER;
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
                hdesc->flavors[n_triggers] = REGULAR_TRIGGER;
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
                    hdesc->triggers[n_triggers].row = row;
                    hdesc->triggers[n_triggers].col = c;
                    hdesc->flavors[n_triggers] = WEAK_TRIGGER | PENCIL;
                    ++n_triggers;
                }
            }
        }
    }
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
        hdesc->candidates[i].col = extract_bit_from_map( &candidate_map );
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
                int hint_col = extract_bit_from_map( &hint_map );
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

            while ( true ) {
                int hint_col = extract_bit_from_map( &hint_map );
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
    hdesc->n_hints = n_hints;
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

    int n_triggers = 0;

    for ( int i = 0; i < 2; ++i ) {                 // i is the box index
        cell_ref_t *single = get_single_in_box( singles, n_singles, boxes[i] );
        if ( NULL == single ) continue;

        hdesc->triggers[n_triggers] = *single;
        hdesc->flavors[n_triggers] = REGULAR_TRIGGER;
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
                hdesc->flavors[n_triggers] = REGULAR_TRIGGER;
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
                    hdesc->triggers[n_triggers].row = r;
                    hdesc->triggers[n_triggers].col = col;
                    hdesc->flavors[n_triggers] = WEAK_TRIGGER | PENCIL;
                    ++n_triggers;
                }
            }
        }
    }
    hdesc->n_triggers = n_triggers;
}

static void fill_in_col_candidates( int col, candidate_col_location_t *cbcloc, hint_desc_t *hdesc )
{
    // candidate locations in column (up to 3)
    hdesc->n_candidates = cbcloc->n_rows;
    int candidate_map = cbcloc->row_map;
    assert( hdesc->n_candidates <= 3 );

    for ( int i = 0; i < hdesc->n_candidates; ++i ) {
        hdesc->candidates[i].row = extract_bit_from_map( &candidate_map );
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
                int hint_row = extract_bit_from_map( &hint_map );
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

            while ( true ) {
                int hint_row = extract_bit_from_map( &hint_map );
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
    hdesc->n_hints = n_hints;
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

extern bool check_locked_candidates( hint_desc_t *hdesc )
/* return false if no locked candidate or if those candidates are already absent in other cells,
          true if locked candidates allowing to eliminate those candidates in other cells. */
{
    for ( int symbol = 0; symbol < SUDOKU_N_SYMBOLS; ++symbol ) {
        int symbol_mask = get_map_from_number( symbol );
        cell_ref_t singles[9];
        int n_singles = get_singles_matching_map_in_game( symbol_mask, singles );

        for ( int i = 0; i < 3; ++i ) {
            int in_rows = get_locked_candidates_for_box_rows( 3 * i, symbol_mask,
                                                              singles, n_singles, hdesc );
            if ( in_rows >= 0 ) return true;

            int in_cols = get_locked_candidates_for_box_cols( 3 * i, symbol_mask,
                                                              singles, n_singles, hdesc );
            if ( in_cols >= 0 ) return true;
        }
    }
    return false;
}

