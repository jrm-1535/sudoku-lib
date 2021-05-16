/*
  Sudoku game backend
*/

#ifndef WIN32
#include <unistd.h>
#endif
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "grdstk.h"
#include "grid.h"
#include "game.h"

//#include "files.h"
//#include "rand.h"

/*
    A game is a stack of grids (9x9 cells + selection) and a set of bookmarks.
    Bookmarks indicate positions in the stack where a specific game grid can
    be found. The stack is used for undo/redo operations in a game.

    interaction undo/redo/modifications:

    For example, after undoing the last 3 moves, the grid stack may look like:

                grid stack
    top of stack -> KKK  redoLevel = 0
                    JJJ
                    III  <- new current grid after modifying a cell (III is gone)
    current grid -> HHH  redoLevel = 3
                    ...

    It is possible to redo the last 3 moves (III, JJ, KKK). But after modifying a cell,
    the next grid in the stack is updated is replaced to allow undoing the new change.
    This prevents redoing a previously undone move. 


    Interaction undo/redo/bookmarks:

       grid stack         mark stack
    current grid -> KKK
                    JJJ
                    III
                    HHH <- bookmark

    It is possible to jump directly to the top bookmark, which is the same as doing 
    undo 3 times. After that jump, the bookmark is gone, but it is still possible to
    redo the previous 3 operations.

    From the same original position it is also possible to undo 3 times to reach the
    same grid as that one pointed to by the bookmark:

       grid stack          mark stack
    top of stack -> KKK
                    JJJ
                    III
    current grid -> HHH <- bookmark2
                    GGG
                    FFF <- bookmark1
                    ...

    After 3 undo, bookmark2 is not visible anymore and bookmark1 becomes the new visible
    bookmark (if it exists). However bookmark2 is still in the mark stack, and after a
    redo it will reappear as the new top bookmark.    
*/
static int  redoLevel = 0;

static int  markLevel = 0;
static int  topMark = 0;

static stack_pointer_t markStack[ NB_MARKS ];

static void cancel_redo( void )
{
    redoLevel = 0;
    topMark = markLevel;
}

static void add_to_redo_level( int val )
{
    SUDOKU_ASSERT( val > 0 );
    redoLevel += val;
}

extern bool is_redo_possible( void ) // exported to sudoku.c
{
    return redoLevel > 0;
}

// returns 0 if no redo possible, 1 if redo done, 2 if redo done and back to last mark
extern int redo( void ) // exported to sudoku_redo in sudoku.c
{
    if ( redoLevel > 0 ) {
        printf( "Redo: decrementing redoLevel (was %d before) \n", redoLevel );
        --redoLevel;
        push();     // actually back to next grid in stack

        printf( "Checking sp %u @markLevel %d topMark %d (val %d)\n",
                get_sp(), markLevel, topMark, markStack[ markLevel ] );
        if ( topMark > markLevel && markStack[ markLevel ] == get_sp() ) {
            markLevel++;
            printf("Restored lastMark @level %d\n", markLevel );
            return 2;
        }
        return 1;
    }
    return 0;
}

static void erase_all_bookmarks( void )
{
    markLevel = topMark = 0;
    cancel_redo();
}

extern int get_bookmark_number( void )   // exported to sudoku.c
{
    return markLevel;
}

extern int new_bookmark( void )   // exported to sudoku_mark_state in sudoku.c
{
    if ( markLevel == NB_MARKS ) return 0;

    printf( "Mark: markLevel %d\n", markLevel);
    for ( int i = 0; i <= markLevel; ++i ) {
        printf ("   @%d mark value is %d\n", i, markStack[ i ]);
    }
    
    markStack[ markLevel ] = get_sp();
    printf(" added sp %d @markLevel %d\n", get_sp(), markLevel );
    set_low_water_mark( markStack[ markLevel ] );
    topMark = ++markLevel;
    return markLevel;
}

/* check if the current grid is the same as the last bookmark.
   The return value is:
    0 if there was no bookmark set
    1 if the current state is not the same as the last bookmark
    2 if the current state is the same as the last bookmark
*/
extern int check_if_at_bookmark( void )   // exported to sudoku.c
{
printf( "check_if_at_bookmark: sp=%u\n", get_sp() );
    if ( markLevel > 0 ) {
        SUDOKU_ASSERT( markLevel <= NB_MARKS );
        SUDOKU_ASSERT( markLevel <= topMark );
        return ( (get_sp() == markStack[ markLevel-1]) ? 2 : 1 );
    }
    return 0;
}

extern bool is_undo_possible( void )
{
    return ! is_stack_empty();
}

// return 0 if nothing to undo, 1 if undone, 2 if undone and last bookmark is removed as well.
extern int undo( void ) // exported to sudoku_undo in sudoku.c
{
    if ( -1 != pop( ) ) {
        printf( "undo: incrementing redoLevel (was %d before) \n", redoLevel );
        ++redoLevel;
        printf("Checking sp %u @markLevel %d\n", get_sp(), markLevel );
        if ( markLevel > 0 && get_sp() < markStack[ markLevel-1] ) {
            markLevel--;
            printf("Undone lastmark (new markLevel %d)\n", markLevel );
            return 2;
        }
        return 1;
    }
    return 0;
}

/* return to last bookmark

  This actually undoes all operations till back to the last saved mark.
  The last bookmark is removed from the stack.

*/
extern int return_to_last_bookmark( void )  // exported to sudoku_back_to_mark in sudoku.c
{
    SUDOKU_ASSERT( markLevel <= NB_MARKS );
    SUDOKU_ASSERT( markLevel <= topMark );

    if ( markLevel > 0 ) {

        stack_pointer_t csp = get_sp( );
        stack_pointer_t nsp = markStack[ markLevel-1 ];
        if ( csp > nsp ) {
            topMark = --markLevel;
            add_to_redo_level( csp - nsp );
            set_sp( nsp );
            return markLevel;
        }
    }
    return -1;
}

static time_t play_started;
static unsigned long already_played;

extern void set_game_time( unsigned long duration )
{
    time( &play_started );
    already_played = duration;
}

extern unsigned long get_game_duration( void )
{
    double diff;
    unsigned long duration;
    time_t end_time;

    time ( &end_time );
    diff = difftime( end_time, play_started );
    duration = (unsigned long)diff + already_played;
    return duration;
}

typedef struct {
    stack_pointer_t sp, lwm;
    stack_pointer_t markStack[ NB_MARKS ];
    int  redoLevel;
    int  markLevel;
    int  topMark;
} game_t;

extern void *save_current_game( void )
{
    static game_t current_game;
    current_game.sp = get_sp();
    current_game.lwm = get_low_water_mark();
    memcpy( current_game.markStack, markStack, sizeof( stack_pointer_t ) * NB_MARKS );
    current_game.redoLevel = redoLevel;
    current_game.markLevel = markLevel;
    current_game.topMark = topMark;
    return (void *)&current_game;
}

extern void *save_current_game_for_solving( void )
{
    game_t *game = save_current_game( );
    set_low_water_mark( game->sp );
    stack_index_t cur_grid = get_current_stack_index();
    stack_index_t top_grid = pushn( 1 + game->redoLevel );
    copy_fill_grid( top_grid, cur_grid );    // make a new grid for solving, above redo level
    return game;
}

extern void restore_saved_game( void *game )
{
    game_t *previous_game_p = (game_t *)game;
    memcpy( markStack, previous_game_p->markStack, sizeof( stack_pointer_t ) * NB_MARKS );
    redoLevel = previous_game_p->redoLevel;
    markLevel = previous_game_p->markLevel;
    topMark = previous_game_p->topMark;

    set_low_water_mark( previous_game_p->lwm );
    set_sp( previous_game_p->sp );
}

extern void update_saved_game( void *game )
{
    stack_index_t top_grid = get_current_stack_index();
    restore_saved_game( game );
    stack_index_t new_grid = push();        // same game with a new grid which
    copy_grid( new_grid, top_grid );        // is a copy of the top of stack
    cancel_redo();                          // no redo since stack is different
}

extern void reset_game( void )
{
    reset_stack( );
    empty_grid( get_current_stack_index( ) );
    erase_all_bookmarks();
}

extern void start_game( void )
{
    stack_index_t top_grid = get_current_stack_index();
    stack_index_t new_grid = reset_stack();
    copy_grid( new_grid, top_grid );
    erase_all_bookmarks();
}

extern void game_new_grid( void )           // save current grid and create a new copy
{
    stack_index_t psi = get_current_stack_index( ),
                  csi = push();

    copy_grid( csi, psi );
    cancel_redo();                          // no redo since stack is different
}

extern void game_new_empty_grid( void )     // save current grid and create a new empty grid
{
    stack_index_t csi = push();

    empty_grid( csi );
    cancel_redo();                          // no redo since stack is different
}

extern void game_new_filled_grid( void )    // save current grid and create a new copy, then fill empty cells
{
    stack_index_t psi = get_current_stack_index( ), 
                  csi = push( );

    copy_fill_grid( csi, psi );
    cancel_redo();                          // no redo since stack is different
}

extern void game_set_cell_symbol( int row, int col, int symbol, bool is_given ) // exported to file.c
{
    game_new_grid( );
    set_cell_symbol( row, col, symbol, is_given );
}

extern void game_toggle_cell_candidate( int row, int col, int symbol )
{
    game_new_grid( );
    toggle_cell_candidate( row, col, symbol );
}

extern void game_erase_cell( int row, int col )
{
    game_new_grid( );
    erase_cell( row, col );
}

extern void game_fill_cell( int row, int col, bool no_conflict )
{
    game_new_grid( );
    fill_in_cell( row, col, no_conflict );
}

