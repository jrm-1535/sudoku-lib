/*
  Sudoku generator
*/

#include "gen.h"
#include "hint.h"
#include "rand.h"
#include "state.h"
#include "stack.h"

#include <string.h>
#include <time.h>   /* for performance measurements */

bool check_debug = false;

static bool check_n_update_symbol_table( int row, int col, bool *symbol_table )
{
    sudoku_cell_t *cell = get_cell( row, col );
    if ( 1 == cell->n_symbols ) {
        int symb = get_number_from_map( cell->symbol_map );
        SUDOKU_ASSERT( -1 != symb );
        // cell may have been set to an impossible value in a previous check_n_set round
        if ( 1 == symbol_table[ symb ] ) return false;
        symbol_table[ symb ] = 1;
    }
    return true;
}

static bool check_n_set_hidden_singles_in_box( int box )
{
    bool is_symbol_done[ SUDOKU_N_SYMBOLS ] = { false };
    int first_row = 3 * ( box / 3 );
    int first_col = 3 * ( box % 3 );

    for ( int r = first_row; r < first_row + 3; ++r ) {
        for ( int c = first_col; c < first_col + 3; ++c ) {
            if ( ! check_n_update_symbol_table( r, c, is_symbol_done ) )
                return false;
        }
    }

    for ( int s = 0; s < SUDOKU_N_SYMBOLS; s++ ) {
        if ( is_symbol_done[ s ] ) continue;

        int last_row, last_col, n_candidates = 0, mask = get_map_from_number( s);
        for ( int r = first_row; r < first_row + 3; r++ ) {
            for ( int c = first_col; c < first_col + 3; c++ ) {
                sudoku_cell_t *cell = get_cell( r, c );
                if ( cell->n_symbols > 1 && ( mask & cell->symbol_map ) ) {
                    if ( ++n_candidates > 1 ) break;
                    last_row = r;
                    last_col = c;
                }
            }
            if ( n_candidates > 1 ) break;
        }
        if ( 1 == n_candidates ) {
            sudoku_cell_t *cell = get_cell( last_row, last_col );
#if SUDOKU_SOLVE_DEBUG
            printf( "set_only_symbol in box %d @(row %d, col %d), symbol %c\n", 
                    box, last_row, last_col, '1' + s );
#endif /* SUDOKU_SOLVE_DEBUG */
            cell->n_symbols = 1;
            cell->symbol_map = mask;
        }
    }
    return true;
}

static bool check_n_set_hidden_singles_in_row( int row )
{
    bool is_symbol_done[ SUDOKU_N_SYMBOLS ] = { false };
    for ( int c = 0; c < SUDOKU_N_COLS; c ++ ) {
        if ( ! check_n_update_symbol_table( row, c, is_symbol_done ) )
            return false;
    }

    for ( int s = 0; s < SUDOKU_N_SYMBOLS; s++ ) {
        if ( is_symbol_done[ s ] ) continue;

        int last_col, n_candidates = 0, mask = get_map_from_number( s );
        for ( int c = 0;  c < SUDOKU_N_ROWS; c ++ ) {
            sudoku_cell_t *cell = get_cell( row, c );
            if ( cell->n_symbols > 1 && ( mask & cell->symbol_map ) ) {
                if ( ++n_candidates > 1 ) break;
                last_col = c;
            }
        }
        if ( 1 == n_candidates ) {
            sudoku_cell_t *cell = get_cell( row, last_col );
#if SUDOKU_SOLVE_DEBUG
            printf("set_only_symbol in row @(row %d, col %d), symbol %c\n", 
                   row, last_col, '1' + s);
#endif /* SUDOKU_SOLVE_DEBUG */
            cell->n_symbols = 1;
            cell->symbol_map = mask;
        }
    }
    return true;
}

static bool check_n_set_hidden_singles_in_col( int col )
{
    bool is_symbol_done[ SUDOKU_N_SYMBOLS ] = { false };
    for ( int r = 0; r < SUDOKU_N_ROWS; ++ r ) {
        if ( ! check_n_update_symbol_table( r, col, is_symbol_done ) )
            return false;
    }

    for ( int s = 0; s < SUDOKU_N_SYMBOLS; ++ s ) {
        if ( is_symbol_done[ s ] ) continue;

        int last_row, n_candidates = 0, mask = get_map_from_number( s );
        for ( int r = 0;  r < SUDOKU_N_ROWS; r ++ ) {
            sudoku_cell_t *cell = get_cell( r, col );
            if ( cell->n_symbols > 1 && ( mask & cell->symbol_map ) ) {
                if ( ++n_candidates > 1 )  break;
                last_row = r;
            }
        }
        if ( 1 == n_candidates ) {  // only 1 candidate for symbol in col
            sudoku_cell_t *cell = get_cell( last_row, col );
#if SUDOKU_SOLVE_DEBUG
            printf( "set_only_symbol in col @(row %d, col %d), symbol %c\n", 
                    last_row, col, '1' + s );
#endif /* SUDOKU_SOLVE_DEBUG */
            cell->n_symbols = 1;
            cell->symbol_map = mask;
        }
    }
    return true;
}

static bool check_n_set_hidden_singles( void )
{
    for ( int c = 0; c < SUDOKU_N_COLS; c++ ) {
        if ( ! check_n_set_hidden_singles_in_col( c ) ) return false;
    }
    for ( int r = 0; r < SUDOKU_N_ROWS; r++ ) {
        if ( ! check_n_set_hidden_singles_in_row( r ) ) return false;
    }
    for ( int b = 0; b < SUDOKU_N_BOXES; b++ ) {
        if ( ! check_n_set_hidden_singles_in_box( b ) ) return false;
    }
    return true;
}

static int check_candidates( void )
{
    if ( ! remove_conflicts() ) return -1;

    int n_singles;
    while( true ) {
        if ( ! check_n_set_hidden_singles( ) ) return -1;
        n_singles = count_single_symbol_cells();
        if ( ! remove_conflicts() ) return -1;
        if ( n_singles == count_single_symbol_cells() ) break;
    }
    return n_singles;
}

static sudoku_cell_t * get_next_best_slot( int *prow, int *pcol )
/* find cells with the lowest number of candidates in the grid,
   which gives the lowest number of possible guesses. */
{
    int n_less_candidates_cells = 0;
    struct { int row, col; } less_candidates_cells[SUDOKU_N_ROWS * SUDOKU_N_COLS];
    int lowest_candidate_number = SUDOKU_N_SYMBOLS + 1;

    for ( int r = 0; r < SUDOKU_N_ROWS; ++r ) {
        for ( int c = 0; c < SUDOKU_N_COLS; ++c ) {
            sudoku_cell_t *cell = get_cell( r, c );

            if ( cell->n_symbols > 1 ) {

                if ( cell->n_symbols < lowest_candidate_number ) {
                    n_less_candidates_cells = 0;
                    lowest_candidate_number = cell->n_symbols;
                }
                if ( cell->n_symbols == lowest_candidate_number ) {
                    less_candidates_cells[n_less_candidates_cells].row = r;
                    less_candidates_cells[n_less_candidates_cells].col = c;
                    ++n_less_candidates_cells;
                }
            }
        }
    }
    assert ( lowest_candidate_number < SUDOKU_N_SYMBOLS + 1 );
    assert ( n_less_candidates_cells > 0 );
    int choice = random_value( 0, n_less_candidates_cells - 1 );
#if 0
printf( "get_next_best_slot: lowest_candidate_number=%d, n_less_candidates_cells=%d, choice=%d\n",
        lowest_candidate_number, n_less_candidates_cells, choice );
for ( int i = 0; i < n_less_candidates_cells; ++i ) {
    printf( "less_candidates_cells[%d].row=%d, .col=%d\n",
            i, less_candidates_cells[i].row, less_candidates_cells[i].col );
}
printf( "get_next_best_slot: row=%d, col=%d\n",
        less_candidates_cells[choice].row, less_candidates_cells[choice].col );
#endif
    *prow = less_candidates_cells[choice].row;
    *pcol = less_candidates_cells[choice].col;
    return  get_cell( less_candidates_cells[choice].row, less_candidates_cells[choice].col );
}

typedef struct {
    int stop_at, n_solutions;
    int n_naked, n_hidden;
    int n_guesses, n_wrong_guesses;
} game_rating_t;

static int try_one_candidate( game_rating_t *gr )
/* select at random one of the cells with the least number of candidates,
   and randomly try to set one of them as the cell value. If the move is
   impossible, backtrack. If the move results in all cells being singles,
   mark as solved. If the number of solutions has been reached, return it,
   else keep going by calling itself recursively. For recursion and back
   traking, use the undo stack.

   return 0 if no solution, or gr->n_solutions if some solution has been found,
   that:

    stop_at n_solutions returned
       1         0         0
       1         >0        1
       2         0         0
       2         1         1
       2         >1        2 */
{
    int r, c, saved_sp = get_sp( );
#if 1 // SUDOKU_SOLVE_DEBUG
    printf("#Entering try_one_candidate @level %d stop_at %d nb_sol %d\n", saved_sp, gr->stop_at, gr->n_solutions );
#endif
//    print_grid_pencils();

    sudoku_cell_t *cell = get_next_best_slot( &r, &c );
    printf("Best Slot row %d, col %d, n_symbols %d, map 0x%03x\n", r, c, cell->n_symbols, cell->symbol_map );

    int sn, nmask = 1, mask = cell->symbol_map;
    for ( sn = 0; sn < SUDOKU_N_SYMBOLS; sn ++ ) {
        if ( mask & nmask ) {
            int csi, res;

            csi = push_state();         // stack current state, down one level for new attempt
            if ( -1 == csi ) exit(2);
            printf("Trying symbol %d mask 0x%03x in best slot n_sol %d\n", sn, nmask, gr->n_solutions );
            cell = get_cell( r, c );    // same cell in new grid, to allow backtracking later
#if  SUDOKU_SOLVE_DEBUG
            printf("Level %d Setting single symbol %c at row %d, col %d (nb symb %d mask 0x%03x)\n",
                         get_sp(), '1'+sn, r, c, (int)(cell->n_symbols), (int)(cell->symbol_map) );
#endif /*  SUDOKU_SOLVE_DEBUG */
            cell->symbol_map = nmask;   
            cell->n_symbols = 1;
            ++gr->n_guesses;

            res = check_candidates( );      // -1 if impossible, or number of singles
            if ( SOLVED_COUNT == res ) {    // solved
// #if SUDOKU_SOLVE_DEBUG
                printf ("Solved!\n"); print_grid( );
// #endif /* SUDOKU_SOLVE_DEBUG */
                if ( ++gr->n_solutions == gr->stop_at ) {
                    printf( "Exiting try_one_candidate with n_solutions %d\n", gr->n_solutions );
                    return gr->stop_at;
                }
            }
            else if ( -1 != res ) { // not an impossible move, keep going
// #if SUDOKU_SOLVE_DEBUG
                printf("@Not solved at level %d\n\n", get_sp() );
// #endif /* SUDOKU_SOLVE_DEBUG */
                if ( gr->stop_at == try_one_candidate( gr ) ) {
                    printf("Returning from try_one_candidate %d\n", gr->stop_at );
                    return gr->stop_at;
                }
            }                       // else imposible, backtrack

#if    SUDOKU_SOLVE_DEBUG
            printf( "%%Wrong path!\n");
            printf( "%%Backtraking @level %d from %d\n\n", saved_sp, get_sp() );
#endif /* SUDOKU_SOLVE_DEBUG */

            ++gr->n_wrong_guesses;
            set_sp( saved_sp );
#if  SUDOKU_SOLVE_DEBUG
            print_grid();
#endif /* SUDOKU_SOLVE_DEBUG */
        }
        nmask <<= 1;
    }
    return gr->n_solutions;
}

static int solve_grid( int nb_states_to_save, game_rating_t *gr, bool multiple )
/* return 0, 1 or 2 according to the following table

    multiple n_solutions returned
      false       0         0
      false       >0        1
       true       0         0
       true       1         1
       true       >1        2 */
{
    memset( gr, 0, sizeof(*gr) );
    gr->stop_at = ( multiple ) ? 2 : 1;
// TODO: in a first pass call check_candidates and if solved return immediatley 1 solution only
    init_state_for_solving( nb_states_to_save );
    return try_one_candidate( gr );
}

static int check_grid_for_solution( void )
{
    int total_n_symbols = 0;
    int nb_row_symbols[SUDOKU_N_ROWS] = { 0 },
        nb_col_symbols[SUDOKU_N_COLS] = { 0 },
        nb_box_symbols[SUDOKU_N_BOXES] = { 0 };
    unsigned char row_symbols[SUDOKU_N_ROWS][SUDOKU_N_SYMBOLS] = { 0 },
        col_symbols[SUDOKU_N_COLS][SUDOKU_N_SYMBOLS] = { 0 },
        box_symbols[SUDOKU_N_BOXES][SUDOKU_N_SYMBOLS] = { 0 };

    for ( int col = 0; col < SUDOKU_N_COLS; col ++ ) {
        for ( int row = 0; row < SUDOKU_N_ROWS; row ++ ) {
            sudoku_cell_t *cell = get_cell( row, col );
            check_cell_integrity( cell );
            if ( 1 == cell->n_symbols ) {
                unsigned char symbol = sudoku_get_symbol( cell );
                int k, box = row - (row % 3) + (col / 3);

                /*	printf( "check array[%d][%d] adding symbol %c\n", col, row, symbol ); */

                for ( k = 0; k < nb_row_symbols[row]; k++ ) {
                    if ( symbol == row_symbols[row][k] ) {
                        printf("error on row %d duplicate symbol %c\n", row, symbol );
                        return -1;
                    }
                }

                for ( k = 0; k < nb_col_symbols[col]; k++ ) {
                    if ( symbol == col_symbols[col][k] ) {
                        printf("error on col %d duplicate symbol %c\n", col, symbol );
                        return -1;
                    }
                }

                for ( k = 0; k < nb_box_symbols[box]; k++ ) {
                    if ( symbol == box_symbols[box][k] ) {
                        printf("error on box %d duplicate symbol %c\n", box, symbol );
                        return -1;
                    }
                }
                row_symbols[row][nb_row_symbols[row]] = symbol;
                nb_row_symbols[row]++;
                col_symbols[col][nb_col_symbols[col]] = symbol;
                nb_col_symbols[col]++;
                box_symbols[box][nb_box_symbols[box]] = symbol;
                nb_box_symbols[box]++;
                total_n_symbols ++;
            }
        }
    }
    return total_n_symbols;
}

static void randomly_transpose( )
{
    int symbols[SUDOKU_N_SYMBOLS] = { -1 };
    int i, col, row;

    for ( i = 0; i < SUDOKU_N_SYMBOLS; i++ ) {
        int j, sn;

        do {
            sn  = random_value( 0, SUDOKU_N_SYMBOLS-1 );
            for ( j = 0; j < i; j++ ) {
                if ( symbols[ j ] == sn ) break;
            }
            if ( j == i ) {
                symbols[ i ] = sn;
                break;
            }
        } while (1);
    }
#if 1
    printf( "Symbols are transcoded as\n");
    for ( i = 0; i < SUDOKU_N_SYMBOLS; i++ )
        printf("%c->%c ", '1'+i, '1'+symbols[i] );
    printf("\n");
#endif
    for ( row = 0; row < SUDOKU_N_ROWS; row++ ) {
        for ( col = 0; col < SUDOKU_N_COLS; col++ ) {
            sudoku_cell_t * current_cell = get_cell( row, col );
            if ( 1 == current_cell->n_symbols ) {
                int symbol = get_number_from_map( current_cell->symbol_map );
                symbol = symbols[ symbol ];
                current_cell->symbol_map = get_map_from_number( symbol );
            }
        }
    }
//    print_grid( );
}

extern int find_one_solution( int nb_states_to_save )
{
    game_rating_t ratings;
    return solve_grid( nb_states_to_save, &ratings, false );
}

extern int check_current_game( int nb_states_to_save )
{
    int sp = get_sp();
 
#if 1 // SUDOKU_SOLVE_DEBUG
    printf("\n#### Checking current grid for solutions @level %d nb_states_to_save %d\n", sp, nb_states_to_save);
#endif
    print_grid_pencils();
    game_rating_t ratings;
    int res = solve_grid( nb_states_to_save, &ratings, true );
#if SUDOKU_SOLVE_DEBUG
    if ( 2 == res ) {
        printf("More than one solution!\n");
    } else if ( 0 == nb_sol ) {
        printf("Not even a single solution!\n");
    } else {
        printf("One solution only!\n");
    }
#endif

    set_sp( sp );
    return res;
}

static void initialize_empty_grid( void )
{
    for ( int row = 0; row < SUDOKU_N_ROWS; row++ ) {
        for ( int col = 0; col < SUDOKU_N_COLS; col++ ) {
            sudoku_cell_t *cell = get_cell( row, col );
            cell->state = 0;
            cell->symbol_map = 0;
            cell->n_symbols = 0;
        }
    }
}

#define MAX_TRIALS  1000

static bool solve_random_cell_array( unsigned int seed, game_rating_t *ratings )
{
    if ( 0 != seed ) set_random_seed( seed );

    reset_stack();
    initialize_empty_grid();
    int n_trials = 0;

    while ( true ) {
        int col = random_value( 0, SUDOKU_N_COLS-1 );
        int row = random_value( 0, SUDOKU_N_ROWS-1 );
        int symbol = random_value( 0, SUDOKU_N_SYMBOLS-1 );

        sudoku_cell_t *cell = get_cell( row, col );
        if ( 1 != cell->n_symbols ) {
      /*      printf("Set symbol %c @random row %d, col %d\n", '1'+symbol, row, col ); */
            cell->state = SUDOKU_GIVEN;
            cell->symbol_map = get_map_from_number( symbol );
            cell->n_symbols = 1;
            int res = solve_grid( 0, ratings, true );
            printf( "solve_random_cell_array: solve_grid returned %d\n", res );
            reset_stack();
            if ( 1 == res ) break;  // exactly one solution, exit
            if ( 0 == res ) {       // no solution, remove symbol and keep trying
                cell->state = 0;
                cell->symbol_map = 0;
                cell->n_symbols = 0;
            }                       // else multiple solutions, keep trying
            if ( ++n_trials > MAX_TRIALS ) return false;
        }
    }
    printf( "Generated unique solution grid with  %d symbols @level %d\n", count_single_symbol_cells(), get_sp() );
    print_grid_pencils();
    return true;
}

extern int make_game( int game_nb )
{
#ifdef TIME_MEASURE
    time_t start_time, end_time;

    if ( -1 == time( &start_time ) ) exit(1);
#endif /* TIME_MREASURE */

    printf("SUDOKU game_nb %d\n", game_nb );

    game_rating_t ratings;
    if ( ! solve_random_cell_array( game_nb, &ratings ) ) {
        printf("solve_random_cell_array did not find a unique solution!\n");
        exit(1);
    }
    printf("make_game: found unique solution\n");
    randomly_transpose( );
    if ( 1 !=  check_current_game( 0 ) ) {
        printf("no unique solution after randomly_transpose\n");
        exit(1);
    }

#ifdef TIME_MEASURE
    if( -1 == time ( &end_time ) ) {
        printf("Unable to get end time\n");
    } else {
        printf("It took %g seconds to find this grid\n", difftime(end_time, start_time) );
    }
#endif /* TIME_MEASURE */

    reset_stack( );
    find_one_solution( 0 );

#if 1
    printf( "Game ratings:\n" );
    printf( "  Naked singles %d Hidden singles %d\n", ratings.n_naked, ratings.n_hidden );
    printf( "  Guesses %d, Wrong guesses %d\n", ratings.n_guesses, ratings.n_wrong_guesses );
#endif
    int difficulty = ratings.n_guesses * 10 - ( ratings.n_naked * 2 ) - ( ratings.n_hidden );

    char *s;
    if ( difficulty > 20 ) {
        s = "Tough";
    } else if ( difficulty > 10 ) {
        s = "Medium";
    } else {
        s = "Easy";
    }
    printf("%s game\n", s );

  /*
    printf("Solution:\n");
    print_grid( );
  */
    reset_stack();
    return 0;
}
