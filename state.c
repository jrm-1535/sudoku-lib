
/*
  Sudoku play state handling
*/

#include <string.h>
#include "state.h"
#include "stack.h"
#include "gen.h"
#include "hint.h"

/*
  The game state is made of:
  - the current grid values, kept in cellArray[ stack_ptr ]
  - the current selection kept in rowArray[ stack_ptr ] and
                                  colArray[ stack_ptr ]

  In addition, the current bookmark values are kept in markStack,
  and the redo operation is stored in the redoLevel variable.

  States are kept in the cell, row and col arrays, so that it
  is always possible to undo an operation. It is possible to
  redo (undo the last undo) until no other operation is done,
  except undo (n consecutive undo operations can be redone as
  n consecutive redo) or back (n consecutive back operations
  can be redone as p consecutive redo). Any entry or other
  command prevents to redo. Once all possible redo have been
  done, the redo level is 0 and it becomes impossible to redo
  anything.

*/
static sudoku_cell_t cellArray [ MAX_DEPTH ] [ SUDOKU_N_ROWS ] [ SUDOKU_N_COLS ];
static int  rowArray[ MAX_DEPTH ], colArray[ MAX_DEPTH ];

static int  markStack[ NB_MARKS ];
static int  markLevel = 0;

static int  redoLevel = 0;
static bool lastMarkUndone = false;

extern void init_state( void )
{
    int csi = get_current_stack_index( );

    for ( int r = 0; r < SUDOKU_N_ROWS; r++ ) {
        for ( int c = 0; c < SUDOKU_N_COLS; c++ ) {
            cellArray[csi][r][c].state = 0;
            cellArray[csi][r][c].symbol_map = 0;
            cellArray[csi][r][c].n_symbols = 0;
        }
    }
    rowArray[ csi ] = colArray[ csi ] = -1;
}

extern void init_state_for_solving( int nb_states_to_save )
{
    int psi = get_current_stack_index( ), 
        csi = pushn( 1 + nb_states_to_save );

    SUDOKU_ASSERT ( csi != -1 );
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

extern void copy_state( int from )
{
    int tsi = get_current_stack_index(),
        fsi = get_stack_index( from );

    memcpy( &cellArray[tsi], &cellArray[fsi],
            sizeof(sudoku_cell_t) * SUDOKU_N_ROWS * SUDOKU_N_COLS );
    rowArray[ tsi ] = rowArray[ fsi ];
    colArray[ tsi ] = colArray[ fsi ];
}

extern int push_state( void )
{
    int psi = get_current_stack_index( );
    int csi = push();

    if ( csi != -1 ) {
        copy_state( psi );
        return csi;
    }
    printf("Stack overflow\n");
    return -1;
}

static int pop_state( void )
{
    int csi = pop();

    if ( csi >= 0 )
        return 0;
    return -1;
}

extern void cancel_redo( void )      // exported to game.c
{
    redoLevel = 0;
    lastMarkUndone = false;
}

extern bool check_redo_level( void )  // exported to game.c
{
    return redoLevel > 0;
}

extern int get_redo_level( void )    // exported to game.c
{
    return redoLevel;
}

static void add_to_redo_level( int val )
{
    SUDOKU_ASSERT( val > 0 );
    redoLevel += val;
}

extern int redo( void ) // exported to sudoku_redo in game.c
{
    if ( redoLevel > 0 ) {
        int status = 1;
        printf("Checking lastMarkUndone %d\n", lastMarkUndone );
        if ( lastMarkUndone ) {
            printf( "Checking sp %d @markLevel %d (val %d)\n",
                    get_sp(), markLevel, markStack[ markLevel ] );
            if ( markStack[ markLevel ] == get_sp() ) {
                markLevel++;
                lastMarkUndone = false;
                printf("Restored lastMark @level %d\n", markLevel );
                status = 2;
            }
        }
        printf( "Redo: decrementing redoLevel (was %d before) \n", redoLevel );
        --redoLevel;
        push();
        return status;
    }
    /*  if ( 0 == redoLevel )
            lastMarkUndone = FALSE; */
    return 0;
}

extern void erase_all_bookmarks( void )  // exported to game.c
{
    markLevel = 0;
    cancel_redo();
}

extern int get_bookmark( void )   // exported to game.c
{
    return markLevel;
}

extern int new_bookmark( void )   // exported to sudoku_mark_state in game.c
{
    if ( markLevel == NB_MARKS ) return 0;

    printf( "Mark: markLevel %d\n", markLevel);
    for ( int i = 0; i <= markLevel; i++ ) {
        printf ("   @%d mark value is %d\n", i, markStack[ i ]);
    }
    
    markStack[ markLevel ] = get_sp();
    printf(" added sp %d @markLevel %d\n", get_sp(), markLevel );
    return ++markLevel;
}

/* check if the current state is the same as the last bookmark.
   The return value is:
    0 if there was no bookmark set
    1 if the current state is not the same as the last bookmark
    2 if the current state is the same as the last bookmark
*/
extern int check_at_bookmark( void )   // exported to game.c
{
    if ( markLevel > 0 ) {
        SUDOKU_ASSERT( markLevel <= NB_MARKS );
        return ( (get_sp() == markStack[ markLevel-1]) ? 2 : 1 );
    }
    return 0;
}

extern int undo( void ) // exported to sudoku_undo in game.c
{
    if ( -1 != pop_state( ) ) {
        printf( "undo: incrementing redoLevel (was %d before) \n", redoLevel );
        ++redoLevel;
        printf("Checking markLevel %d\n", markLevel );
        if ( markLevel > 0 ) {
            if ( get_sp() == markStack[ markLevel-1] ) {
                markLevel--;
                lastMarkUndone = true;
                printf("Undone lastmark (new markLevel %d)\n", markLevel );
                return 2;
            }
        }
        return 1;
    }
    return 0;
}

/* return to last bookmark

  This actually undoes all operations till back to the last saved mark.,
  The last bookmark is removed from the stack.

*/
extern bool return_to_last_bookmark( void )  // exported to sudoku_back_to_mark in game.c
{
    if ( markLevel > 0 ) {
        int nsp, csp = get_sp( );
        SUDOKU_ASSERT( markLevel <= NB_MARKS );
        nsp = markStack[ --markLevel];
        add_to_redo_level( csp - nsp );
        set_sp( nsp );
        return markLevel;
    }
    return -1;
}

extern sudoku_cell_t * get_cell( int row, int col ) // exported to gen.c and hint.c
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

extern void get_selected_row_col( int *row, int *col )
{
    SUDOKU_ASSERT( row );
    SUDOKU_ASSERT( col );
    int csi = get_current_stack_index( );
    *row = rowArray[ csi ];
    *col = colArray[ csi ];
}

extern void set_selected_row_col( int row, int col )
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

extern void check_cell_integrity( sudoku_cell_t *c )
{
    int i, j, n, map, r = 1;
    for ( i = j = 0, n = c->n_symbols, map = c->symbol_map; i < SUDOKU_N_SYMBOLS && n; i ++ ) {
        if ( 1 & map ) {
            j++;
            n --;
        }
        map >>= 1;
    }
    if ( j != c->n_symbols ) {
        r = 0;
        printf( "less symbols in map %d than indicated in n_symbols %d\n", j, c->n_symbols );
    }
    if ( map ) {
        r = 0;
        printf( "Additional bits in map 0x%08x\n", c->symbol_map );
    }
    SUDOKU_ASSERT( r );
}

extern void check_cell( int row, int col )  // exported to game.c
{
    check_cell_integrity( get_cell( row, col ) );
}

extern bool is_cell_given( int row, int col )
{
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

extern int count_single_symbol_cells( void )
{
    int nb = 0, csi = get_current_stack_index( );
    for ( int c = 0; c < SUDOKU_N_COLS; c ++ ) {
        for ( int r = 0; r < SUDOKU_N_ROWS; r ++ ) {
            if ( 1 == cellArray[csi][r][c].n_symbols ) nb++;
        }
    }
    return nb;
}

extern void set_cell_value( int r, int c, int v, bool is_given ) // exported to file.c
{
    SUDOKU_ASSERT( r >= 0 && r < 9 && c >= 0 && c < 9 );
    SUDOKU_ASSERT( v >= 0 && v < 9 );
    sudoku_cell_t *ccell = get_cell( r, c );
    ccell->n_symbols = 1;
    if ( is_given ) ccell->state = SUDOKU_GIVEN;
    ccell->symbol_map = 1<<v;
}

extern void add_cell_value( int r, int c, int v )               // exported to file.c
{
    SUDOKU_ASSERT( r >= 0 && r < 9 && c >= 0 && c < 9 );
    SUDOKU_ASSERT( v >= 0 && v < 9 );

    sudoku_cell_t *ccell = get_cell( r, c );
    if ( ccell->symbol_map & get_map_from_number( v ) ) {
        return; // value is already in the map
    }
    ccell->n_symbols++;
    ccell->symbol_map |= get_map_from_number( v );

    check_cell( r, c );
}

extern bool get_cell_type_n_values( int r, int c,
                                    size_t *nsp, int *vmp )     // exported to file.c
{
    SUDOKU_ASSERT( r >= 0 && r < 9 && c >= 0 && c < 9 );
    SUDOKU_ASSERT( nsp && vmp );
    sudoku_cell_t *ccell = get_cell( r, c );
    *nsp = ccell->n_symbols;
    *vmp = ccell->symbol_map;
    return SUDOKU_IS_CELL_GIVEN( ccell->state );
}

extern int get_map_next_value( int *vmp )                       // exported to file.c
{
    int smap = SUDOKU_SYMBOL_MASK & *vmp;
    int j = 1;

    for ( int i = 0; i < SUDOKU_N_SYMBOLS; ++i ) {
        if ( smap & j ) {
            *vmp = smap ^ j;
            return '1'+ i;
        }
        j <<= 1;
    }
    return 0;
}

extern void erase_cell( int row, int col )  // exported to sudoku_erase_selection in game.c
{
    sudoku_cell_t *ccell = get_cell( row, col );
    SUDOKU_ASSERT( ! SUDOKU_IS_CELL_GIVEN( ccell->state ) );
    ccell->n_symbols = 0;
    ccell->symbol_map = 0;
    ccell->state = 0;
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

extern bool remove_conflicts( void ) {
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
    push_state();
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

extern void set_cell_hint( int row, int col, hint_e hint )
{
    int csi = get_current_stack_index( );
    if ( HINT_REGION & hint ) {
        cellArray[csi][row][col].state |= SUDOKU_HINT;
    } else if ( WEAK_TRIGGER_REGION & hint ) {
        cellArray[csi][row][col].state |= SUDOKU_WEAK_TRIGGER;
    } else if ( TRIGGER_REGION & hint ) {
        cellArray[csi][row][col].state |= SUDOKU_TRIGGER;
    } else if ( ALTERNATE_TRIGGER_REGION & hint ) {
        cellArray[csi][row][col].state |= SUDOKU_ALTERNATE_TRIGGER;
    }
    if ( CHAIN_HEAD & hint ) {
        cellArray[csi][row][col].state |= SUDOKU_CHAIN_HEAD;
    }
    if ( (PENCIL & hint) && (0 == cellArray[csi][row][col].n_symbols) ) {
        cellArray[csi][row][col].n_symbols =
            get_no_conflict_candidates( row, col, &cellArray[csi][row][col].symbol_map );
    }
//printf( "game: set_cell_hint csi=%d row=%d, col=%d hint=%d => state=0x%04x\n",
//        csi, row, col, hint, cellArray[csi][row][col].state );
}

extern void reset_cell_hints( void )
{
    int csi = get_current_stack_index( );

    for ( int r = 0; r < SUDOKU_N_ROWS; ++r ) {
        for ( int c = 0; c < SUDOKU_N_COLS; ++c ) {
            cellArray[csi][r][c].state &=
                ~SUDOKU_HINT & ~SUDOKU_CHAIN_HEAD &
                ~SUDOKU_WEAK_TRIGGER & ~SUDOKU_TRIGGER & ~SUDOKU_ALTERNATE_TRIGGER;
        }
    }
    printf( "game: reset all cell hints\n" );
}
#if 0
void make_subset_pencils( int nb, cell_ref_t *pencil_array, bool no_conflict )
{
    push_state();

    for ( int i = 0; i < nb; i++ ) {
        int row = pencil_array[i].row, col = pencil_array[i].col;
        cell *scell = get_cell( row, col );

        if ( 0 == scell->n_symbols ) {
            scell->n_symbols = SUDOKU_N_SYMBOLS;
            scell->symbol_map = SUDOKU_SYMBOL_MASK;

            if ( no_conflict ) {
                cell_ref_t error_array[MAX_CONFLICTS_PER_CELL];
                int nb_conflicts, nb_conflict_symbols, conflict_symbol_map;
 // FIXME
                nb_conflicts = check_cell_for_conflicts( row, col, error_array );
                nb_conflict_symbols = conflict_symbol_map = 0;
 
                for ( int j = 0; j < nb_conflicts; j ++ ) {
                    cell *ecell = get_cell( error_array[i].row, error_array[i].col );
                    if ( 0 == ( conflict_symbol_map & ecell->symbol_map ) ) {
                        nb_conflict_symbols ++;
                        conflict_symbol_map |= ecell->symbol_map;
                    }
                }
                scell->n_symbols -= nb_conflict_symbols;
                scell->symbol_map &= ~ conflict_symbol_map;
            }
        }
    }
}
#endif

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
