
#include <string.h>
#include "grid.h"
#include "stack.h"

/*
  A grid is a snapshot of a game state at a given time. It is made of:
  - the current cell values, kept in cellArray[ stack_index ]
  - the current selection kept in rowArray[ stack_index ] and
                                  colArray[ stack_index ]

  The current game state is kept in cells and selection array, stored
  in a grid stack, so that it is always possible to undo an operation.
  The game stack management is in game.c, whereas grid.c only contains
  grid and selection operations.
*/
static sudoku_cell_t cellArray [ MAX_DEPTH ] [ SUDOKU_N_ROWS ] [ SUDOKU_N_COLS ];
static int  rowArray[ MAX_DEPTH ], colArray[ MAX_DEPTH ];

// empty_grid makes an empty grid with no selection in the current state
extern void empty_grid( stack_index_t csi )
{
    for ( int r = 0; r < SUDOKU_N_ROWS; r++ ) {
        for ( int c = 0; c < SUDOKU_N_COLS; c++ ) {
            cellArray[csi][r][c].state = 0;
            cellArray[csi][r][c].symbol_map = 0;
            cellArray[csi][r][c].n_symbols = 0;
        }
    }
    rowArray[ csi ] = colArray[ csi ] = -1;
}

// replace grid at index d with the one at index s
extern void copy_grid( stack_index_t d, stack_index_t s )
{
    memcpy( &cellArray[ d ], &cellArray[ s ],
            sizeof(sudoku_cell_t) * SUDOKU_N_ROWS * SUDOKU_N_COLS );
    rowArray[ d ] = rowArray[ s ];
    colArray[ d ] = colArray[ s ];
}

extern void copy_fill_grid( stack_index_t csi, stack_index_t psi )
{
    for ( int r = 0; r < SUDOKU_N_ROWS; r++ ) {
        for ( int c = 0; c < SUDOKU_N_COLS; c++ ) {
            if ( 1 <= cellArray[psi][r][c].n_symbols ) {
                     // don't touch cells with symbols
                cellArray[csi][r][c] = cellArray[psi][r][c];
            } else { // automatically populate for solving
                cellArray[csi][r][c].n_symbols = SUDOKU_N_SYMBOLS;
                cellArray[csi][r][c].symbol_map = SUDOKU_SYMBOL_MASK;
                cellArray[csi][r][c].state = 0;
            }
        }
    }
    rowArray[ csi ] = rowArray[ psi ];
    colArray[ csi ] = colArray[ psi ];
}

extern void get_selected_row_col( int *row, int *col )
{
    SUDOKU_ASSERT( row );
    SUDOKU_ASSERT( col );
    int csi = get_current_stack_index( );
    *row = rowArray[ csi ];
    *col = colArray[ csi ];
}

extern void select_row_col( int row, int col )
{
    int csi = get_current_stack_index( );
    int cur_row = rowArray[ csi ];
    if ( -1 != cur_row ) {
        int cur_col = colArray[ csi ];
        SUDOKU_ASSERT( -1 != cur_col );
        cellArray[csi][cur_row][cur_col].state &= ~SUDOKU_SELECTED;
    }

    if ( -1 != row ) {
        assert( -1 != col );
        cellArray[csi][row][col].state |= SUDOKU_SELECTED;
        update_grid_errors( row, col );
    } else {
        reset_grid_errors( );
    }

    rowArray[ csi ] = row;
    colArray[ csi ] = col;
}

extern sudoku_cell_t * get_cell( int row, int col ) // exported to solve.c and hint.c
{
    int csi = get_current_stack_index( );
    return &cellArray[csi][row][col];
}

extern bool sudoku_get_cell_definition( int row, int col, sudoku_cell_t *cell )
{
    assert( cell );
    if ( 0 <= row && 9 >= row && 0 <= col && 9 >= col ) {
        int csi = get_current_stack_index( );
//        printf("sudoku_get_cell_definition: csi=%d\n", csi );
        cell->state = cellArray[csi][row][col].state;
        cell->n_symbols = cellArray[csi][row][col].n_symbols;
        cell->symbol_map = cellArray[csi][row][col].symbol_map;
        return true;
    }
    return false;
}

// return the number of bits (candidates or locations) in a map. Used in hint.c
extern int get_n_bits_from_map( int map )
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

// return 1 bit (candidates or locations) at a time from a map. Used in hint.c & files.c
extern int extract_bit_from_map( int *map )
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

// return the symbol number given a cell map (assuming n_symbols is 1)
extern int get_number_from_map( unsigned short map )
{
    static char s_mapping[ ] = {                      // speedup a bit with a table
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

extern void reset_grid_errors( void )
{
    int csi = get_current_stack_index( );
    for ( int r = 0; r < SUDOKU_N_ROWS; ++ r ) {
        for ( int c = 0; c < SUDOKU_N_COLS; ++ c ) {
            sudoku_cell_t *cell = &cellArray[csi][r][c];
            cell->state &= ~SUDOKU_IN_ERROR;
        }
    }
}

extern size_t update_grid_errors( int row, int col )
{
    reset_grid_errors( );

    int csi = get_current_stack_index( );
    int mask = cellArray[csi][row][col].symbol_map;
    int n_errors = 0;

    for (int c = 0; c < SUDOKU_N_COLS; c ++ ) {
        if ( ( 1 == cellArray[csi][row][c].n_symbols ) && ( col != c ) ) {
            if ( mask & cellArray[csi][row][c].symbol_map ) {
                cellArray[csi][row][c].state |= SUDOKU_IN_ERROR;
                n_errors++;
            }
        }
    }

    for ( int r = 0; r < SUDOKU_N_ROWS; r ++ ) {
        if ( ( 1 == cellArray[csi][r][col].n_symbols ) && ( row != r ) ) {
            if ( mask & cellArray[csi][r][col].symbol_map ) {
                cellArray[csi][r][col].state |= SUDOKU_IN_ERROR;
                n_errors++;
            }
        }
    }

    int box_first_col = col - (col % 3); 
    int box_first_row = row - (row % 3);

    for ( int r = 0; r < SUDOKU_N_ROWS / 3; r++ ) {
        for ( int c = 0; c < SUDOKU_N_COLS / 3; c++ ) {
            if ( ( box_first_row + r != row ) && ( box_first_col + c != col ) ) {
                if ( 1 == cellArray[csi][box_first_row + r][box_first_col + c].n_symbols ) {
                    if ( mask & cellArray[csi][box_first_row + r][box_first_col + c].symbol_map ) {
                        cellArray[csi][box_first_row + r][box_first_col + c].state |= SUDOKU_IN_ERROR;
                        n_errors++;
                    }
                }
            }
        }
    }
    return n_errors;
}

extern void check_cell_integrity( sudoku_cell_t *c )
{
    int n_symbols = get_n_bits_from_map( c->symbol_map );
    if ( n_symbols == c->n_symbols ) return;

    char *err = ( n_symbols < c->n_symbols ) ? "less" : "more";
    printf( "%s symbols in cell map %d than indicated in n_symbols %d\n", err, n_symbols, c->n_symbols );
    SUDOKU_ASSERT( 0 );
}

extern bool is_cell_given( int row, int col )
{
    SUDOKU_ASSERT( row >= 0 && row < 9 && col >= 0 && col < 9 );
    int csi = get_current_stack_index( );
    return cellArray[csi][row][col].state & SUDOKU_GIVEN;
}

extern void make_cells_given( void )  // exported to sudoku_commit_game in game.c
{
    int csi = get_current_stack_index( );
    for ( int r = 0; r < SUDOKU_N_ROWS; r++ ) {
        for ( int c = 0; c < SUDOKU_N_COLS; c++ ) {
            if ( 1 == cellArray[csi][r][c].n_symbols )  {
                cellArray[csi][r][c].state = SUDOKU_GIVEN;
            }
        }
    }
}

extern bool is_cell_empty( int row, int col )
{
    SUDOKU_ASSERT( row >= 0 && row < 9 && col >= 0 && col < 9 );
    sudoku_cell_t *cell = get_cell( row, col );
    return 0 == cell->n_symbols;
}

extern void set_cell_symbol( int row, int col, int symbol, bool is_given )
{
    SUDOKU_ASSERT( row >= 0 && row < 9 && col >= 0 && col < 9 );
    SUDOKU_ASSERT( symbol >= 0 && symbol < 9 );
    sudoku_cell_t *cell = get_cell( row, col );
    cell->n_symbols = 1;
    if ( is_given ) cell->state = SUDOKU_GIVEN;
    cell->symbol_map = get_map_from_number( symbol );
}

extern void add_cell_candidate( int row, int col, int symbol )           // exported to file.c
{
    SUDOKU_ASSERT( row >= 0 && row < 9 && col >= 0 && col < 9 );
    SUDOKU_ASSERT( symbol >= 0 && symbol < 9 );

    sudoku_cell_t *cell = get_cell( row, col );
    SUDOKU_ASSERT( 0 == (SUDOKU_GIVEN & cell->state) );
    if ( cell->symbol_map & get_map_from_number( symbol ) ) {
        return; // value is already in the map
    }
    cell->n_symbols++;
    cell->symbol_map |= get_map_from_number( symbol );
    check_cell_integrity( cell );
}

extern void toggle_cell_candidate( int row, int col, int symbol )
{
    SUDOKU_ASSERT( row >= 0 && row < 9 && col >= 0 && col < 9 );
    SUDOKU_ASSERT( symbol >= 0 && symbol < 9 );

    sudoku_cell_t *cell = get_cell( row, col );
    int mask = get_map_from_number( symbol );

    if ( cell->symbol_map & mask ) {
        --cell->n_symbols;
        SUDOKU_TRACE( SUDOKU_INTERFACE_DEBUG, ( "Removing Symbol %d (0x%02x) remaining symbols %d\n",
                                                symbol, cell->symbol_map ^ mask, cell->n_symbols ) );
    } else { /* symbol was not set */
        ++cell->n_symbols;
        SUDOKU_TRACE( SUDOKU_INTERFACE_DEBUG, ( "Adding Symbol %d (0x%02x) total symbols %d\n",
                                                symbol, cell->symbol_map ^ mask, cell->n_symbols ) );
    }
    cell->symbol_map ^= mask;
    check_cell_integrity( cell );
}

extern void set_cell_candidates( int row, int col, int n_candidates, int candidate_map )
{
    SUDOKU_ASSERT( row >= 0 && row < 9 && col >= 0 && col < 9 );
    SUDOKU_ASSERT( n_candidates > 0 && n_candidates <= SUDOKU_N_SYMBOLS );

    sudoku_cell_t *cell = get_cell( row, col );
    cell->symbol_map = candidate_map;
    cell->n_symbols = n_candidates;
    check_cell_integrity( cell );
}

extern void remove_cell_candidates( int row, int col, int n_candidates, int candidate_map )
{
    SUDOKU_ASSERT( row >= 0 && row < 9 && col >= 0 && col < 9 );
    SUDOKU_ASSERT( n_candidates > 0 && n_candidates <= SUDOKU_N_SYMBOLS );

    sudoku_cell_t *cell = get_cell( row, col );
    int n_in_common = get_n_bits_from_map( cell->symbol_map & candidate_map );
    SUDOKU_ASSERT( n_candidates >= n_in_common );

    cell->n_symbols -= n_in_common;
    cell->symbol_map = cell->symbol_map & ~candidate_map;
    check_cell_integrity( cell );
}

extern bool get_cell_type_n_map( int row, int col, uint8_t *nsp, int *mp ) // exported to file.c
{
    SUDOKU_ASSERT( row >= 0 && row < 9 && col >= 0 && col < 9 );
    SUDOKU_ASSERT( nsp && mp );
    sudoku_cell_t *cell = get_cell( row, col );
    *nsp = cell->n_symbols;
    *mp = cell->symbol_map;
    return SUDOKU_IS_CELL_GIVEN( cell->state );
}

extern void erase_cell( int row, int col )  // exported to game_erase_cell in game.c
{
    sudoku_cell_t *cell = get_cell( row, col );
    SUDOKU_ASSERT( ! SUDOKU_IS_CELL_GIVEN( cell->state ) );
    cell->n_symbols = 0;
    cell->symbol_map = 0;
    cell->state &= SUDOKU_SELECTED;        // keep selection if any
}

extern int count_single_symbol_cells( void )
{
    int n = 0;
    for ( int c = 0; c < SUDOKU_N_COLS; c ++ ) {
        for ( int r = 0; r < SUDOKU_N_ROWS; r ++ ) {
            sudoku_cell_t *cell = get_cell( r, c );
            if ( 1 == cell->n_symbols ) ++n;
        }
    }
    return n;
}

extern bool is_game_solved( void )
{
    int n_symbols[ SUDOKU_N_SYMBOLS ] = { 0 };

    for ( int c = 0; c < SUDOKU_N_COLS; c ++ ) {
        for ( int r = 0; r < SUDOKU_N_ROWS; r ++ ) {
            sudoku_cell_t *cell = get_cell( r, c );
            if ( 1 == cell->n_symbols ) {
                int symbol = get_number_from_map( cell->symbol_map );
                ++n_symbols[ symbol ];
            }
        }
    }
    for ( int i = 0; i < SUDOKU_N_SYMBOLS; ++i ) {
        if ( n_symbols[i] != SUDOKU_N_SYMBOLS ) return false;
    }
    return true;
}

/* fills the singles array with the location of cells whose single symbol matches a
   bit in symbol_map. The single array must have been allocated by the caller with
   enough room to allow up to SUDOKU_N_SYMBOLS * number of bits in symbol_map.
   Return the number of singles that have been stored in singles array. */
extern int get_singles_matching_map_in_game( int symbol_map, cell_ref_t *singles )
{
    int count = 0;
    for ( int r = 0; r < SUDOKU_N_ROWS; ++r ) {
        for ( int c = 0; c < SUDOKU_N_COLS; ++c ) {
            sudoku_cell_t * cell = get_cell( r, c );
            if ( 1 == cell->n_symbols && ( symbol_map & cell->symbol_map ) ) {
                singles[count].row = r;
                singles[count].col = c;
                ++count;
            }
        }
    }
    return count;
}

static int get_no_conflict_candidates( int row, int col, uint16_t *pmap )
{
    int csi = get_current_stack_index( ),
        n_symbols = SUDOKU_N_SYMBOLS;
    uint16_t map = SUDOKU_SYMBOL_MASK;

    for ( int c = 0; c < SUDOKU_N_COLS; ++c ) {
        if ( ( 1 == cellArray[csi][row][c].n_symbols ) && ( col != c ) ) {
            if ( map & cellArray[csi][row][c].symbol_map ) {
                map &= ~ cellArray[csi][row][c].symbol_map;
                --n_symbols;
            }
        }
    }

    for ( int r = 0; r < SUDOKU_N_ROWS; r ++ ) {
        if ( ( 1 == cellArray[csi][r][col].n_symbols ) && ( row != r ) ) {
            if ( map & cellArray[csi][r][col].symbol_map ) {
                map &= ~ cellArray[csi][r][col].symbol_map;
                --n_symbols;
            }
        }
    }

    int box_first_col = col - (col % 3); 
    int box_first_row = row - (row % 3);

    for ( int r = 0; r < SUDOKU_N_ROWS / 3; r++ ) {
        for ( int c = 0; c < SUDOKU_N_COLS / 3; c++ ) {
            if ( ( box_first_row + r != row ) && ( box_first_col + c != col ) ) {
                sudoku_cell_t *cell = &cellArray[csi][box_first_row + r][box_first_col + c];
                if ( 1 == cell->n_symbols ) {
                    if ( map & cell->symbol_map ) {
                        map &= ~ cell->symbol_map;
                        --n_symbols;
                    }
                }
            }
        }
    }
printf( "get_no_conflict_candidates: row %d col %d, map 0x%03x n_symbols %d\n", row, col, map, n_symbols );
    *pmap = map;
    return n_symbols;
}

typedef struct { int row, col, mask; } symbol_location_t;

static int remove_symbol( symbol_location_t *queue, int *beyond, int row, int col, int mask )
/* returns -1 if it is an invalid cell, 0 if the cell does not contain the symbol mask or is a given,
   or 1 if it did and the new cell location has been added to the queue. */
{
    sudoku_cell_t *cell = get_cell( row, col );

    if ( cell->n_symbols > 1 ) {
        unsigned short map = cell->symbol_map;

        if ( map & mask ) {
            cell->symbol_map = (map & ~mask);
            if ( 1 == --cell->n_symbols ) {         // enqueue new single
                queue[*beyond].row = row;
                queue[*beyond].col = col;
                queue[*beyond].mask = cell->symbol_map;
                ++ *beyond;
            }
            return 1;                               // indicate symbols has been removed
        }

    } else if ( cell->symbol_map == mask ) {
        return -1;                                  // indicate an impossible case
    }
    return 0;                                       // symbol not in map, no effect
}

extern bool remove_grid_conflicts( void )
/* it returns false if the grid is invalid or true if it is a valid grid.
   If valid the grid has been cleaned up from any possible conflict */
{
    symbol_location_t queue[ SUDOKU_N_SYMBOLS * SUDOKU_N_SYMBOLS ];
    int start = 0;

    for ( int col = 0; col < SUDOKU_N_COLS; ++col ) {
        for ( int row = 0; row < SUDOKU_N_ROWS; ++row ) {
            sudoku_cell_t *cell = get_cell( row, col );
            if ( 1 != cell->n_symbols ) continue;

            queue[start].row = row;
            queue[start].col = col;
            queue[start].mask = cell->symbol_map;
            ++start;
        }
    }
    int beyond = start;
    start = 0;

    while ( start < beyond ) {
        int mask = queue[start].mask, row = queue[start].row, col = queue[start].col;
        ++start;

        for ( int r = 0; r < SUDOKU_N_ROWS; ++r ) { // intersection of all other rows
            if ( r == row ) continue;               // except itself, with the same column
            if ( -1 == remove_symbol( queue, &beyond, r, col, mask ) ) return false;
        }

        for ( int c = 0; c < SUDOKU_N_COLS; ++c ) {
            if ( c == col ) continue;
            if ( -1 == remove_symbol( queue, &beyond, row, c, mask ) ) return false;
        }

        int box_first_col = col - (col % 3); 
        int box_first_row = row - (row % 3);
        for ( int r = box_first_row; r < box_first_row + 3; ++r ) {
            for ( int c = box_first_col; c < box_first_col + 3; ++c ) {
                if ( ( r == row ) && ( c == col ) ) continue;
                if ( -1 == remove_symbol( queue, &beyond, r, c, mask ) ) return false;
            }
        }
        assert( beyond <= SUDOKU_N_SYMBOLS * SUDOKU_N_SYMBOLS );
    }
    return true;
}

extern void fill_in_cell( int row, int col, bool no_conflict )
{
    sudoku_cell_t *scell = get_cell( row, col );

#if 0
    SUDOKU_ASSERT( ! SUDOKU_IS_CELL_GIVEN( scell->state ) );
#else
    if ( SUDOKU_IS_CELL_GIVEN( scell->state ) ) return;
#endif

    // do not touch cells with symbols
    if ( 0 != scell->n_symbols ) return;

    if ( no_conflict ) { // remove conlicting pencils
        scell->n_symbols = get_no_conflict_candidates( row, col, &scell->symbol_map );
    } else {
        scell->n_symbols = SUDOKU_N_SYMBOLS;
        scell->symbol_map = SUDOKU_SYMBOL_MASK;
        update_grid_errors( row, col );
    }
}

extern void set_cell_attributes( int row, int col, cell_attrb_t attrb )
{
    int csi = get_current_stack_index( );
    if ( HINT & attrb ) {
        cellArray[csi][row][col].state |= SUDOKU_HINT;
    } else if ( WEAK_TRIGGER & attrb ) {
        cellArray[csi][row][col].state |= SUDOKU_WEAK_TRIGGER;
    } else if ( REGULAR_TRIGGER & attrb ) {
        cellArray[csi][row][col].state |= SUDOKU_TRIGGER;
    } else if ( ALTERNATE_TRIGGER & attrb ) {
        cellArray[csi][row][col].state |= SUDOKU_ALTERNATE_TRIGGER;
    }
    if ( HEAD & attrb ) {
        cellArray[csi][row][col].state |= SUDOKU_CHAIN_HEAD;
    }
    if ( (PENCIL & attrb) && (0 == cellArray[csi][row][col].n_symbols) ) {
        cellArray[csi][row][col].n_symbols =
            get_no_conflict_candidates( row, col, &cellArray[csi][row][col].symbol_map );
    }
//printf( "game: set_cell_hint csi=%d row=%d, col=%d hint=%d => state=0x%04x\n",
//        csi, row, col, hint, cellArray[csi][row][col].state );
}

extern void reset_cell_attributes( void )
{
    int csi = get_current_stack_index( );

    for ( int r = 0; r < SUDOKU_N_ROWS; ++r ) {
        for ( int c = 0; c < SUDOKU_N_COLS; ++c ) {
            cellArray[csi][r][c].state &=
                ~SUDOKU_HINT & ~SUDOKU_CHAIN_HEAD &
                ~SUDOKU_WEAK_TRIGGER & ~SUDOKU_TRIGGER & ~SUDOKU_ALTERNATE_TRIGGER;
        }
    }
    printf( "game: reset all cell attributes\n" );
}

// Debugging functions
extern void print_grid( void )
{
#if SUDOKU_PRETTY_PRINT
#if 0
    printf( "  | 1   2   3 | 4   5   6 | 7   8   9 |\n" );
#endif
    printf( "  |===+===+===+===+===+===+===+===+===|\n" );
    for ( int r = 0; r < SUDOKU_N_ROWS; r++ ) {
#if 0
        printf( "%d |", 1+r );
#else
        printf( "  |" );
#endif
        for ( int c = 0; c < SUDOKU_N_COLS; c++ ) {
            sudoku_cell_t *ccell = get_cell( r, c );
            unsigned char symbol;
            if ( 1 == ccell->n_symbols ) {
                symbol = sudoku_get_symbol( ccell );
            } else if ( ccell->n_symbols > 1 ) {
                symbol = '+';
            } else {
                symbol = ' ';
            }
            if ( c == 2 || c == 5 ) {
                printf( " %c I", symbol );
            } else {
                printf( " %c |", symbol );
            }
        }
        printf( "\n" );
        if ( r == 2 || r == 5 || r == 8 ) {
            printf( "  |===+===+===I===+===+===I===+===+===|\n" );
        } else {
            printf( "  |---+---+---I---+---+---I---+---+---|\n" );
        }
    }
#else
#if SUDOKU_SPECIAL_DEBUG
    printf( " |   0      1      2  |   3      4      5  |   6      7     8   |\n" );
    printf( " |======+======+======+======+======+======+======+======+======|\n" );
        /*    |1:xxxx 2:xxxx 3:xxxx 4:xxxx 5:xxxx 6:xxxx 7:xxxx 8:xxxx 9:xxxx */
#else
    printf( " | 0 1 2 | 3 4 5 | 6 7 8 |\n" );
    printf( " |=======+=======+=======|\n" );
#endif

    for ( int r = 0; r < SUDOKU_N_ROWS; r++ ) {
        if ( r == 3 || r == 6 ) {
#if SUDOKU_SPECIAL_DEBUG
            printf( " |======+======+======+======+======+======+======+======+======|\n" );
#else
            printf(" |-------+-------+-------|\n" );
#endif
        }
        printf( "%d|", r );

        for ( int c = 0; c < SUDOKU_N_COLS; c++ ) {
            sudoku_cell_t *ccell = get_cell( r, c );
            unsigned char symbol;
            if ( 1 == ccell->n_symbols ) {
                symbol = sudoku_get_symbol( ccell );
            } else {
                symbol = ' ';
            }
            if ( ( c == 2 ) || ( c == 5 ) || ( c == 8 ) ) {
#if SUDOKU_SPECIAL_DEBUG
                printf( "%c:%04x|", symbol, ccell->symbol_map );
            } else {
                printf( "%c:%04x ", symbol,  ccell->symbol_map );
#else
                printf( " %c |", symbol );
            } else {
                printf( " %c", symbol );
#endif
            }
        }
        printf( "\n" );
    }
#if SUDOKU_SPECIAL_DEBUG
    printf( " |======+======+======+======+======+======+======+======+======|\n" );
#else
    printf( " |=======+=======+=======|\n" );
#endif
#endif
}

static char *get_pencil_string( sudoku_cell_t *cell )
{
    static char buffer[10];

    if ( 1 == cell->n_symbols ) {
        strcpy( buffer, "         " );
        buffer[2] = '<';
        buffer[4] = sudoku_get_symbol( cell );
        buffer[6] = '>';
    } else {
        for ( int i = 0; i < SUDOKU_N_SYMBOLS; ++i ) {
            if ( cell->symbol_map & (1 << i) ) {
                buffer[i] = '1' + i;
            } else {
                if ( cell->n_symbols )
                    buffer[i] = ' ';
                else
                    buffer[i] = '.';
            }
        }
    }
    buffer[9] = 0;
    return buffer;
}

extern void print_grid_pencils( void )
{
    printf( " |    0         1         2    |    3         4         5    |    6         7         8    |\n" );
    printf( " |=========+=========+=========+=========+=========+=========+=========+=========+=========|\n" );
    for ( int r = 0; r < SUDOKU_N_ROWS; r++ ) {
        if ( r == 3 || r == 6 ) {
            printf( " |=========+=========+=========+=========+=========+=========+=========+=========+=========|\n" );
        }
        printf( "%d|", r );

        for ( int c = 0; c < SUDOKU_N_COLS; c++ ) {
            sudoku_cell_t *cell = get_cell( r, c );
            char *s = get_pencil_string( cell );
            if ( ( c == 2 ) || ( c == 5 ) || ( c == 8 ) ) {
                printf( "%s|", s );
            } else {
                printf( "%s:", s );
            }
        }
        printf( "\n" );
    }
    printf( " |=========+=========+=========+=========+=========+=========+=========+=========+=========|\n" );
}
