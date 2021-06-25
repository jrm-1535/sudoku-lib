/*
  Sudoku game backend
*/

#ifndef WIN32
#include <unistd.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "grid.h"
#include "game.h"
#include "solve.h"
#include "hint.h"
#include "debug.h"

#include "files.h"
#include "rand.h"

/* Game states:

   0 initially:       no game selected (same as game over)

   1 entering game:   entering values for a new game (game is not started)

    with four substates:
   1.0  current game game is empty (initially)
   1.1  current game has multiple solutions
   1.2  current game has no solution (but it can be still be fixed)
   1.3  current game has only one solution (game can be committed)

   2 game on going:  playing the game

    with two substates:
   2.0  nothing played yet, no cell selected (initially)
   2.1  a cell is selected for entry (allow filling that cell)
   2.2  something has been entered in a cell (warn if leaving now)

   3 game over

   Possible transitions:

   0 -> 1.0 (Make your game)
   0 -> 2.0 (New, Pick, Open game)

   1.0 -> 1.1 (select + input)
   1.1 -> 1.0 (undo)

   1.1 -> 1.1 (input leading to multiple solutions)
   1.1 -> 1.2 (input leading to no solution)
   1.1 -> 1.3 (input leading to single solution)

   1.2 -> 1.1 (undo or replacing input, leading to multiple solutions)
   1.2 -> 1.2 (input leading to no solution again)
   1.2 -> 1.3 (undo or replacing input, leading to single solution)

   1.3 -> 1.2 (input leading to no solution)
   1.3 -> 1.3 (input leading to single solution again)

   1.3 -> 2.0 (Accept this game)

   2.0 -> 1.0 (Make your game)
   2.0 -> 2.0 (New, pick, Open game)
   2.0 -> 2.1 (select cell)

   2.1 -> 1.0 (Make your game)
   2.1 -> 2.1 (select other cell)
   2.1 -> 2.2 (input in the selected cell)

   2.2 -> 1.0 (Make your game - with Warning! - TODO)
   2.2 -> 2.1 (undo)
   2.2 -> 2.0 (New, Pick, Open game - with warning)
   2.2 -> 3   (Game over)

   3   -> 1.0 (Make your game)
   3   -> 2.0 (New, Pick, Open game)

   Possible operations in each state:

   0   File Menu:  New, Pick, Open, Make your game, quit.
       Edit Menu:  none.
       Tools Menu: options.
       Help Menu:  all.
       No key press, no button selection on the grid

   1.0 File Menu: Cancel this game (in place of make your game), quit
       Edit Menu:  none.
       Tools Menu: options.
       Help Menu:  all.
       Key press and button selection on the grid

   1.1 File Menu: Cancel this game (in place of make your game), quit
       Edit Menu:  undo, erase.
       Tools Menu: options.
       Help Menu:  all.
       Key press and button selection on the grid

   1.2 File Menu: Cancel this game (in place of make your game), quit
       Edit Menu:  undo, erase.
       Tools Menu: options.
       Help Menu:  all.
       Key press and button selection on the grid

   1.3 File Menu: Accept this game (in place of make your game), quit
       Edit Menu:  undo, erase.
       Tools Menu: options.
       Help Menu:  all.
       Key press and button selection on the grid

   2.0 File Menu:  New, Pick, Open, Make your game, save, print, print setup, quit.
       Edit Menu:  none.
       Tools Menu: hint, do solve, conflict detection, check after each move, options.
       Help Menu:  all.
       Key press and button selection on the grid

   2.1 File Menu:  New, Pick, Open, Make your game, save, print, print setup, quit.
       Edit Menu:  none.
       Tools Menu: hint, do solve, Fill, conflict detection, check after each move, options.
       Help Menu:  all.
       Key press and button selection on the grid

   2.2 File Menu:  New, Pick, Open, Make your game, save, print, print setup, quit.
       Edit Menu:  undo, erase, mark.
       Tools Menu: check, hint, do_solve, Fill, conflict detection, check after each move, options.
       Help Menu:  all.
       Key press and button selection on the grid

   3.0 File Menu:  New, Pick, Open, Make your game, quit.
       Edit Menu:  none.
       Tools Menu: options.
       Help Menu:  all.
       No key press, no button selection on the grid

  State variables:

     game_state = { SUDOKU_INIT | SUDOKU_ENTER | SUDOKU_STARTED | SUDOKU_OVER }
                        0               1               2               3
     Depending on the current game state, substates are indicated by:
        in SUDOKU_ENTER_STATE (1),
         - stack empty (substate 0)
         - stack not empty (substates 1 & 2)
         - stack not empty and enter_game_valid (substate 3)
        in SUDOKU_STARTED (2),
         - stack empty (substate 0)
         - stack empty and selection (substate 1)
         - stack not empty (substate 2)
     States 0 and 3 have no substates.
     Note that state 1 has actually 4 substates, but substates 1 and 2 have no external difference.
*/

enum {
  SUDOKU_INIT, SUDOKU_ENTER, SUDOKU_STARTED, SUDOKU_OVER
};

static int sudoku_state = -1;

static void assert_game_state( int state, char *action )
{
    if ( state != sudoku_state ) {
        printf( "%s: Inconsistent state %d (should be %d)\n",
                action, sudoku_state, state );
        exit(1);
    }
}

static bool enter_game_valid = false;                   // initial value
static bool show_conflict = true, auto_check = false;   // default values

extern bool sudoku_is_game_on_going( void )
{
    return ( SUDOKU_STARTED == sudoku_state ) && is_undo_possible();
}

extern bool sudoku_is_selection_possible( void )
{
  return ( SUDOKU_ENTER == sudoku_state ) || ( SUDOKU_STARTED == sudoku_state );
}

extern bool sudoku_is_entering_game_on_going( void )
{
    return (SUDOKU_ENTER == sudoku_state) && is_undo_possible();
}

extern bool sudoku_is_entering_valid_game( void )
{
    return (SUDOKU_ENTER == sudoku_state) && enter_game_valid;
}

static bool is_game_started( void )
{
    return SUDOKU_STARTED == sudoku_state;
}

static bool is_game_on( void )
{
    return (SUDOKU_ENTER == sudoku_state) || (SUDOKU_STARTED == sudoku_state);
}
/*
static bool is_game_over( void )
{
    return SUDOKU_OVER == sudoku_state;
}
*/
static bool is_game_in_entering_state( void ) 
{
    return SUDOKU_ENTER == sudoku_state;
}

static sudoku_ui_table_t sudoku_ui_fcts;
#define SUDOKU_REDRAW( _c )                    sudoku_ui_fcts.redraw( _c )
#define SUDOKU_SET_WINDOW_NAME( _c, _p )       sudoku_ui_fcts.set_window_name( _c, _p )
#define SUDOKU_SET_STATUS( _c, _s, _v )        sudoku_ui_fcts.set_status( _c, _s, _v )
#define SUDOKU_SET_BACK_LEVEL( _c, _l )        sudoku_ui_fcts.set_back_level( _c, _l )
#define SUDOKU_SET_ENTER_MODE( _c, _m )        sudoku_ui_fcts.set_enter_mode( _c, _m )
#define SUDOKU_ENABLE_MENU( _c, _m )           sudoku_ui_fcts.enable_menu( _c, _m )
#define SUDOKU_DISABLE_MENU( _c, _m )          sudoku_ui_fcts.disable_menu( _c, _m )
#define SUDOKU_ENABLE_MENU_ITEM( _c, _m, _i )  sudoku_ui_fcts.enable_menu_item( _c, _m, _i )
#define SUDOKU_DISABLE_MENU_ITEM( _c, _m, _i ) sudoku_ui_fcts.disable_menu_item( _c, _m, _i )
#define SUDOKU_SUCCESS_DIALOG( _c, _d )        sudoku_ui_fcts.success_dialog( _c, _d )

static void set_game_state( const void *cntxt, int new_state )
{
    if ( new_state == sudoku_state ) return;

    switch (new_state) {
    case SUDOKU_INIT:
        sudoku_state = SUDOKU_INIT;
        printf("INIT state\n");
        SUDOKU_DISABLE_MENU( cntxt, SUDOKU_FILE_MENU);
        SUDOKU_DISABLE_MENU( cntxt, SUDOKU_EDIT_MENU );
        SUDOKU_DISABLE_MENU( cntxt, SUDOKU_TOOL_MENU );

        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_FILE_MENU, SUDOKU_NEW_ITEM );
        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_FILE_MENU, SUDOKU_PICK_ITEM );
        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_FILE_MENU, SUDOKU_OPEN_ITEM );
        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_FILE_MENU, SUDOKU_EXIT_ITEM );
        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_FILE_MENU, SUDOKU_ENTER_ITEM );

        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_TOOL_MENU, SUDOKU_OPTION_ITEM );
        break;

    case SUDOKU_ENTER:
        sudoku_state = SUDOKU_ENTER;
        printf("ENTER state\n");
        SUDOKU_DISABLE_MENU( cntxt, SUDOKU_FILE_MENU);
        SUDOKU_DISABLE_MENU( cntxt, SUDOKU_EDIT_MENU);
        SUDOKU_DISABLE_MENU( cntxt, SUDOKU_TOOL_MENU);

        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_FILE_MENU, SUDOKU_EXIT_ITEM );
        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_FILE_MENU, SUDOKU_ENTER_ITEM );
        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_ERASE_ITEM );

        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_TOOL_MENU, SUDOKU_OPTION_ITEM );
        break;

    case SUDOKU_STARTED:
        sudoku_state = SUDOKU_STARTED;
        printf("GAME_STARTED state\n");
        SUDOKU_ENABLE_MENU( cntxt, SUDOKU_EDIT_MENU );
        SUDOKU_ENABLE_MENU( cntxt, SUDOKU_TOOL_MENU );

        /* no fill until a cell is selected */
        SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_TOOL_MENU, SUDOKU_FILL_SEL_ITEM );
 
        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_FILE_MENU, SUDOKU_NEW_ITEM );
        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_FILE_MENU, SUDOKU_PICK_ITEM );
        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_FILE_MENU, SUDOKU_OPEN_ITEM );
        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_FILE_MENU, SUDOKU_SAVE_ITEM );
        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_FILE_MENU, SUDOKU_PRINT_ITEM );
        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_FILE_MENU, SUDOKU_PRINT_SETUP_ITEM );
        SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_UNDO_ITEM );
        SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_REDO_ITEM );
        SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_ERASE_ITEM );
        SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_BACK_ITEM );
        break;

    case SUDOKU_OVER:
        sudoku_state = SUDOKU_OVER;
        printf("GAME_OVER state\n");
        SUDOKU_DISABLE_MENU( cntxt, SUDOKU_EDIT_MENU );
        SUDOKU_DISABLE_MENU( cntxt, SUDOKU_TOOL_MENU );
        SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_FILE_MENU, SUDOKU_SAVE_ITEM );

        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_TOOL_MENU, SUDOKU_OPTION_ITEM );
        break;

    default:
        printf("set_game_state: inconsitent state %d\n", new_state );
        exit(1);
    }
}

static void update_selection_dependent_menus( const void *cntxt )
{
    int row, col;
    get_selected_row_col( &row, &col );
    printf("update_edit_menu selection row %d, col %d\n", row, col );
    if ( -1 == row ) {                          // not a valid selection
        if ( ! is_game_in_entering_state( ) ) {
            SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_TOOL_MENU, SUDOKU_FILL_SEL_ITEM );
        }
        SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_ERASE_ITEM );
    } else {
        printf("update_edit_menu selection: is_cell_empty: %d\n", is_cell_empty( row, col ) );
        if  ( ! is_game_in_entering_state( ) ) {
            SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_TOOL_MENU, SUDOKU_FILL_SEL_ITEM );
        }
        if ( ! is_cell_empty( row, col ) ) { // a valid and non-empty selection
            SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_ERASE_ITEM );
        } else {                            // a vild but empty selection
            SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_ERASE_ITEM );
        }
    }
}

static void update_edit_menu( const void *cntxt )
{
    SUDOKU_ASSERT ( SUDOKU_STARTED == sudoku_state );

    if ( is_undo_possible( ) ) {
        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_UNDO_ITEM );
    } else {
        SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_UNDO_ITEM );
    }

    if ( is_redo_possible( ) ) {
        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_REDO_ITEM );
    } else {
        SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_REDO_ITEM );
    }

    update_selection_dependent_menus( cntxt );

    if ( ! is_game_in_entering_state( ) ) {
        switch ( check_if_at_bookmark() ) {
        default:
            SUDOKU_ASSERT( 0 );
        case 2:
            SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_MARK_ITEM );
            SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_BACK_ITEM );
            break;
        case 1:
            SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_MARK_ITEM );
            SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_BACK_ITEM );
            break;
        case 0:
            SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_MARK_ITEM );
            SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_BACK_ITEM );
        }
    }
}

static void set_current_selection( const void *cntxt, int row, int col, bool force_redraw )
{
    int currentSelectedRow, currentSelectedCol;
    get_selected_row_col( &currentSelectedRow, &currentSelectedCol);

    if ( currentSelectedRow == row && currentSelectedCol == col ) {
        if ( force_redraw ) { SUDOKU_REDRAW( cntxt ); }
        return;
    }

     if ( row != -1 && col != -1 ) {
        SUDOKU_ASSERT( col >= 0 && col < SUDOKU_N_COLS && row >= 0 && row < SUDOKU_N_ROWS );
        if ( is_cell_given( row, col ) ) return;
    } else {
        assert( row == -1 && col == -1 );
    }

    select_row_col( row, col );
    update_selection_dependent_menus( cntxt );
    reset_cell_attributes();
    SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_BLANK, 0 );
    SUDOKU_REDRAW( cntxt );
}

extern void sudoku_set_selection( const void *cntxt, int row, int col )
{
    set_current_selection( cntxt, row, col, false );
}

static void remove_selection( const void *cntxt )
{
    select_row_col(-1, -1 );
    update_selection_dependent_menus( cntxt );
}

extern void sudoku_move_selection( const void *cntxt, sudoku_key_t how )
{
    int currentSelectedRow, newSelectedRow, currentSelectedCol, newSelectedCol;
 
    get_selected_row_col( &currentSelectedRow, &currentSelectedCol);
    get_selected_row_col( &newSelectedRow, &newSelectedCol );

    switch ( how ) {
    case SUDOKU_PAGE_UP:
        for ( newSelectedRow = 0; newSelectedRow < currentSelectedRow; newSelectedRow++ ) {
            if ( ! is_cell_given( newSelectedRow, newSelectedCol ) ) {
                sudoku_set_selection( cntxt, newSelectedRow, newSelectedCol );
	            return;
            }
        }
        break;

    case SUDOKU_UP_ARROW:
        while ( newSelectedRow > 0 ) {
            newSelectedRow --;
            if ( ! is_cell_given( newSelectedRow, newSelectedCol ) ) {
                sudoku_set_selection( cntxt, newSelectedRow, newSelectedCol );
                return;
            }
        }
        break;
    case SUDOKU_PAGE_DOWN:
        for ( newSelectedRow = SUDOKU_N_ROWS-1; newSelectedRow > currentSelectedRow; newSelectedRow-- ) {
            if ( ! is_cell_given( newSelectedRow, newSelectedCol ) ) {
                sudoku_set_selection( cntxt, newSelectedRow, newSelectedCol );
                return;
            }
        }
        break;
    case SUDOKU_DOWN_ARROW:
        while ( newSelectedRow < (SUDOKU_N_ROWS-1) ) {
            newSelectedRow ++;
            if ( ! is_cell_given( newSelectedRow, newSelectedCol ) ) {
                sudoku_set_selection( cntxt, newSelectedRow, newSelectedCol );
                return;
            }
        }
        break;
    case SUDOKU_LEFT_ARROW:
        while (  newSelectedCol > 0 ) {
            newSelectedCol --;
            if ( ! is_cell_given( newSelectedRow, newSelectedCol ) ) {
                sudoku_set_selection( cntxt, newSelectedRow, newSelectedCol );
                return;
            }
        }
        break;
    case SUDOKU_RIGHT_ARROW:
        while ( newSelectedCol < (SUDOKU_N_COLS-1) ) {
            newSelectedCol ++;
            if ( ! is_cell_given( newSelectedRow, newSelectedCol ) ) {
                sudoku_set_selection( cntxt, newSelectedRow, newSelectedCol );
                return;
            }
        }
        break;
    case SUDOKU_HOME_KEY:
        if ( newSelectedCol || newSelectedRow ) {
            newSelectedCol = newSelectedRow = 0;
            if ( ! is_cell_given( newSelectedRow, newSelectedCol ) ) {
                sudoku_set_selection( cntxt, newSelectedRow , newSelectedCol );
                return;
            }
        }
        break;
    case SUDOKU_END_KEY:
        if ( (newSelectedCol < (SUDOKU_N_COLS-1)) || (newSelectedRow < (SUDOKU_N_ROWS-1)) ) {
            newSelectedCol = SUDOKU_N_COLS-1;
            newSelectedRow = SUDOKU_N_ROWS-1;
            if ( ! is_cell_given( newSelectedRow, newSelectedCol ) ) {
                sudoku_set_selection( cntxt, newSelectedRow, newSelectedCol );
                return;
            }
        }
        break;
    default:
        break;
    }
    return;
}

static void start_new_game( const void *cntxt, const char *name )
{
    start_game();
    remove_selection( cntxt );
    reset_cell_attributes();

    SUDOKU_SET_BACK_LEVEL( cntxt, 0 );
    SUDOKU_SET_WINDOW_NAME( cntxt, name );
    set_game_state( cntxt, SUDOKU_STARTED );
    SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_BLANK, 0 );
    set_game_time( 0 );
    SUDOKU_REDRAW( cntxt );
}

static char *get_game_name( int game_number )
{
    static char name_buffer[ 16 ];
    sprintf( name_buffer, "s%d", game_number );
    return name_buffer;
}

static void do_game( const void *cntxt, int game_number )
{
    set_game_level( make_game( game_number ) );
    start_new_game( cntxt, get_game_name( game_number ) );
}

/* returns an integer game number from the number string. The number
   must be in the range [SUDOKU_MIN_GAME_NUMBER-SUDOKU_MAX_GAME_NUMBER] */
static long int parse_game_number( const char *s )
{
    char *end;
    long int n = strtol( s, &end, 10 );
    if ( *end != 0 || end == s ||
         n < SUDOKU_MIN_GAME_NUMBER || n > SUDOKU_MAX_GAME_NUMBER ) {
        printf( "Invalid decimal game number\n" );
        return -1;
    }
//    printf("n=%ld string %s(0x%p) end %s(0x%p)\n", n, s, s, end, end );
    return n;
}

static char *get_level_string( sudoku_level_t level )
{
    switch( level ) {
    case EASY:      return "EASY";
    case SIMPLE:    return "SIMPLE";
    case MODERATE:  return "MODERATE";
    case DIFFICULT: return "DIFFICULT";
    default:
        break;
    }
    return "";
}

static char *get_window_name_from_file_path( const char *file_path )
{
#define MAX_WINDOW_NAME 256
    static char window_name[ MAX_WINDOW_NAME ];

    const char *file_name = strrchr( file_path, PATH_SEPARATOR );
    if ( NULL == file_name ) file_name = file_path;
    else ++ file_name;

    SUDOKU_ASSERT( strlen( file_name ) < MAX_WINDOW_NAME - 16 ); // keep 16 bytes for level
    snprintf( window_name, MAX_WINDOW_NAME, "%s - %s",
              file_name, get_level_string( get_game_level( ) ) );
    return window_name;
}

extern void sudoku_game_init( const void *cntxt, sudoku_ui_table_t *fcts )
{
    SUDOKU_ASSERT( cntxt );
    SUDOKU_ASSERT( fcts );

    sudoku_ui_fcts = *fcts;
    SUDOKU_ASSERT( sudoku_ui_fcts.redraw );
    SUDOKU_ASSERT( sudoku_ui_fcts.set_window_name );
    SUDOKU_ASSERT( sudoku_ui_fcts.set_status );
    SUDOKU_ASSERT( sudoku_ui_fcts.set_back_level );
    SUDOKU_ASSERT( sudoku_ui_fcts.set_enter_mode );
    SUDOKU_ASSERT( sudoku_ui_fcts.enable_menu );
    SUDOKU_ASSERT( sudoku_ui_fcts.disable_menu );
    SUDOKU_ASSERT( sudoku_ui_fcts.enable_menu_item );
    SUDOKU_ASSERT( sudoku_ui_fcts.disable_menu_item );
    SUDOKU_ASSERT( sudoku_ui_fcts.success_dialog );

    reset_game( );
    set_game_state( cntxt, SUDOKU_INIT );
}

extern void sudoku_mark_state( const void *cntxt )
{
    SUDOKU_ASSERT( cntxt );
    assert_game_state ( SUDOKU_STARTED, "sudoku_mark_state" );

    int mark = new_bookmark( );
    if ( mark ) {
//        reset_cell_attributes();

        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_MARK,  mark  );
        SUDOKU_SET_BACK_LEVEL( cntxt, mark );
        update_edit_menu( cntxt );
    }
}

extern void sudoku_back_to_mark( const void *cntxt )
{
    SUDOKU_ASSERT( cntxt );
    printf("Back to mark - calling return to last bookmark\n");
    assert_game_state ( SUDOKU_STARTED, "sudoku_back_to_mark" );
    int mark = return_to_last_bookmark();
    printf("returned to mark %d\n", mark );
    if ( -1 != mark ) {
        reset_cell_attributes();
        printf("Setting new status and updating game\n");
        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_BACK, mark  );
        SUDOKU_SET_BACK_LEVEL( cntxt, mark );
        update_edit_menu( cntxt );
        SUDOKU_REDRAW( cntxt );
    } 
}

static bool check_from_current_position( void )
{
    void *game = save_current_game_for_solving( );
    bool res = find_one_solution( );
    restore_saved_game( game );
    return res;
}


extern void sudoku_step( const void *cntxt )
{
    SUDOKU_ASSERT( cntxt );
    if ( ! is_game_started() ) return;

    if ( ! check_from_current_position() ) {
        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_HINT, NO_SOLUTION );
        return;
    }
    reset_cell_attributes();

    int step = solve_step( );
    if ( step > 0 ) {
        if ( step == 2 ) {  // game over
            set_game_state( cntxt, SUDOKU_OVER );
            SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_OVER, 0 );
        } else {
            if ( show_conflict ) { reset_grid_errors( ); }
            SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_BLANK, 0 );
            update_edit_menu( cntxt );
        }
        SUDOKU_REDRAW( cntxt );
    }
}

extern sudoku_hint_type sudoku_hint( const void *cntxt )
{
    SUDOKU_ASSERT( cntxt );
    if ( ! is_game_started( ) ) return NO_HINT;

    reset_cell_attributes();
    if ( ! check_from_current_position() ) {
        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_HINT, NO_SOLUTION );
        return NO_SOLUTION;
    }

    int selection_row = -1, selection_col = -1;
    sudoku_hint_type hint = find_hint( &selection_row, &selection_col );
    SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_HINT, hint );

    if ( hint ) {
        select_row_col( selection_row, selection_col ); // set_current_selection would remove hints
        update_selection_dependent_menus( cntxt );
        SUDOKU_REDRAW( cntxt );
    }
    return hint;
}

extern void sudoku_fill( const void *cntxt, bool no_conflict )
{
    SUDOKU_ASSERT( cntxt );

    if ( ! is_game_started( ) ) return;

    reset_cell_attributes();
    int row, col;
    get_selected_row_col( &row, &col );
    if ( col != -1 && row != -1 ) {
        game_fill_cell( row, col, no_conflict );
        update_edit_menu( cntxt );
        SUDOKU_REDRAW( cntxt );
    }
}

extern void sudoku_fill_all( const void *cntxt, bool no_conflict )
{
    SUDOKU_ASSERT( cntxt );

    assert_game_state ( SUDOKU_STARTED, "sudoku_fill_all" );
    reset_cell_attributes();

    game_new_filled_grid( ); // new grid with all empty cells filled with candidates
    if ( no_conflict ) {
        if ( ! remove_grid_conflicts() ) {
            SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_CHECK, 0 );
        } else if ( is_game_solved() ) {
            set_game_state( cntxt, SUDOKU_OVER );
            SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_OVER, 0 );
            SUDOKU_REDRAW( cntxt );
            return;
        }
    }
    update_edit_menu( cntxt );
    SUDOKU_REDRAW( cntxt );
}

extern void sudoku_check_from_current_position( const void *cntxt )
{
    SUDOKU_ASSERT( cntxt );
    if ( ! is_game_started( ) ) return;
    reset_cell_attributes();

    if ( check_from_current_position( ) ) {
        SUDOKU_TRACE( SUDOKU_SOLVE_DEBUG, ("Solvable from that position!\n") );
        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_CHECK, 1 );
    } else {
        SUDOKU_TRACE( SUDOKU_SOLVE_DEBUG, ("Not Solvable from that position\n") );
        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_CHECK, 0 );
    }
}

extern void sudoku_solve_from_current_position( const void *cntxt )
{
    SUDOKU_ASSERT( cntxt );
    if ( ! is_game_started( ) ) return;

    reset_cell_attributes();

    void *game = save_current_game_for_solving( );
    if ( find_one_solution( ) ) {
        SUDOKU_TRACE( SUDOKU_SOLVE_DEBUG, ("SOLVED!\n")) ;
        update_saved_game( game );
        set_game_state( cntxt, SUDOKU_OVER );
        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_OVER, 0 );
    } else {
        SUDOKU_TRACE( SUDOKU_SOLVE_DEBUG, ( "Not Solvable from that position\n" ));
        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_CHECK, 0 );
        restore_saved_game( game );
    }
    SUDOKU_REDRAW( cntxt );
}

static void update_entering_state( const void *cntxt )
{
    reset_cell_attributes();
    switch( check_current_grid( ) ) {
    case 2:
        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_SEVERAL_SOLUTIONS, 0 );
        if ( enter_game_valid ) {
            enter_game_valid = false;
            SUDOKU_SET_ENTER_MODE( cntxt, SUDOKU_CANCEL_GAME );
        }
        break;
    case 1:
        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_ONE_SOLUTION_ONLY, 0 );
        if ( ! enter_game_valid ) {
            enter_game_valid = true;
            SUDOKU_SET_ENTER_MODE( cntxt, SUDOKU_COMMIT_GAME );
        }
        break;
    case 0:
        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_NO_SOLUTION, 0 );
        if ( enter_game_valid ) {
            enter_game_valid = false;
            SUDOKU_SET_ENTER_MODE( cntxt, SUDOKU_CANCEL_GAME );
        }
        break;
    }
}

#define SEC_IN_MIN  60
#define SEC_IN_HOUR (60*60)

static void get_paying_duration( sudoku_duration_t *dhms )
{
    unsigned long duration = get_game_duration( );

    int h, m;
    for ( h = 0; duration >= SEC_IN_HOUR; h++ )
        duration -= SEC_IN_HOUR;

    for ( m = 0; duration >= SEC_IN_MIN; m++ )
        duration -= SEC_IN_MIN;

    dhms->hours   = h;
    dhms->minutes = m;
    dhms->seconds = duration;
}

extern bool sudoku_how_long_playing( sudoku_duration_t *duration_hms )
{
    if ( SUDOKU_STARTED == sudoku_state ) {
        get_paying_duration( duration_hms );
        return true;
    }
    return false;
}

static void end_game( const void *cntxt )
{
    SUDOKU_ASSERT ( SUDOKU_STARTED == sudoku_state );
    sudoku_duration_t duration_hms;
    get_paying_duration( &duration_hms );
    printf( "         in %d hours, %d min, %d sec\n",
            duration_hms.hours, duration_hms.minutes, duration_hms.seconds );
    set_game_state( cntxt, SUDOKU_OVER );
    SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_OVER, 0 );
    SUDOKU_SUCCESS_DIALOG( cntxt, &duration_hms );
}

static bool toggle_symbol( const void *cntxt, int symbol, int row, int col )
{
    SUDOKU_TRACE( SUDOKU_INTERFACE_DEBUG, ("toggle_symbol %c @ row %d col %d\n", symbol, row, col) );
    SUDOKU_ASSERT( symbol > '0' && symbol <= '9' );

    SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_BLANK, 0 );
    if ( is_game_in_entering_state( ) ) {
        game_set_cell_symbol( row, col, symbol-'1', false );
        update_entering_state( cntxt );
        update_edit_menu( cntxt );
        SUDOKU_REDRAW( cntxt );
    } else if ( is_game_started( ) ){
        game_toggle_cell_candidate( row, col, symbol-'1' );
        if ( show_conflict ) { update_grid_errors( row, col ); }
        update_edit_menu( cntxt );
        SUDOKU_REDRAW( cntxt );

        if ( is_game_solved( ) ) {
            SUDOKU_TRACE( SUDOKU_INTERFACE_DEBUG, ("SOLVED!\n") );
            return true;
        }
        if ( auto_check ) {
            sudoku_check_from_current_position( cntxt );
        }
    }

    return false;
}

extern void sudoku_enter_symbol( const void *cntxt, int symbol )
{
    int row, col;
    get_selected_row_col( &row, &col );
    if ( -1 == row || -1 == col ) return;

    if ( symbol > '0' && symbol <= '9' ) {
#if SUDOKU_GRAPHIC_DEBUG
        printf("toggle key code %d\n", symbol );
#endif
        reset_cell_attributes();
        if ( toggle_symbol( cntxt, symbol, row, col ) ) end_game(cntxt);
    }
}

static void empty_game( const void *cntxt )
{
    reset_game();
    SUDOKU_REDRAW( cntxt );
}

extern void sudoku_toggle_entering_new_game( const void *cntxt )
{
    if ( is_game_in_entering_state( ) ) {
        // cancel entering mode
        SUDOKU_SET_ENTER_MODE( cntxt, SUDOKU_ENTER_GAME );
        set_game_state( cntxt, SUDOKU_INIT );
    } else {
        // starts entering new game mode
        SUDOKU_SET_ENTER_MODE( cntxt, SUDOKU_CANCEL_GAME );
        set_game_state( cntxt, SUDOKU_ENTER );
    }
    empty_game( cntxt );
}

extern int sudoku_toggle_conflict_detection( const void *cntxt  )
{
    bool res = show_conflict;
    show_conflict = ! res;
    if ( is_game_on() ) {
        if ( show_conflict ) {
            int col, row;
            get_selected_row_col( &row, &col );
            update_grid_errors( row, col );
        } else {
            reset_grid_errors( );
        }
        SUDOKU_REDRAW( cntxt );
    }
    return res;
}

extern int sudoku_toggle_auto_checking( const void *cntxt )
{
    bool res = auto_check;
    auto_check = ! res;
    if ( is_game_on() ) {
        if ( auto_check ) {
            sudoku_check_from_current_position( cntxt );
        } else {
            SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_BLANK, 0 );
        }
    }
    return res;
}

extern void sudoku_erase_selection( const void *cntxt )
{
    SUDOKU_ASSERT( cntxt );

    int row, col;
    get_selected_row_col( &row, &col );

    if ( -1 != row && -1 != col && ! is_cell_empty( row, col ) ) {
        SUDOKU_TRACE( SUDOKU_INTERFACE_DEBUG, ("Erase\n"));

        reset_cell_attributes();
        game_erase_cell( row, col );
        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_BLANK, 0 );
        update_edit_menu( cntxt );
        SUDOKU_REDRAW( cntxt );

        if ( is_game_in_entering_state( ) ) {
            update_entering_state( cntxt ); /* update the status message if needed */
        }
    }
}

extern void sudoku_commit_game( const void *cntxt, const char *game_name )
{
    SUDOKU_ASSERT ( SUDOKU_ENTER == sudoku_state );

    make_cells_given();
    SUDOKU_SET_ENTER_MODE( cntxt, SUDOKU_ENTER_GAME );
    SUDOKU_SET_WINDOW_NAME( cntxt, game_name );
    SUDOKU_SET_BACK_LEVEL( cntxt, 0 );
    set_game_state( cntxt, SUDOKU_STARTED );
    start_game( );
    set_game_time( 0 );
    SUDOKU_REDRAW( cntxt );
}

extern sudoku_level_t sudoku_pick_game( const void *cntxt, const char *number_string )
{
    if ( number_string ) {
        int game_number = parse_game_number( number_string );
        if ( -1 != game_number ) {
            do_game( cntxt, game_number );
            return get_game_level( );
        }
    }
    return 0;
}

extern sudoku_level_t sudoku_random_game( const void *cntxt )
{
    /* randomly choose a game number */
    int nb = random_value( SUDOKU_MIN_GAME_NUMBER, SUDOKU_MAX_GAME_NUMBER );
    do_game( cntxt, nb );
    return get_game_level( );
}

extern sudoku_level_t sudoku_open_file( const void *cntxt, const char *path )
{
    void *game = save_current_game( );  // in case load file fails
    if ( load_file( path ) ) {
        const char *name = get_window_name_from_file_path( path );
        start_new_game( cntxt, name );  // discards saved game & start new one
        return get_game_level( );
    }
    restore_saved_game( game );         // resume previous game
    return 0;
}

extern void sudoku_undo( const void *cntxt )
{
    SUDOKU_ASSERT( cntxt );
    if ( ! is_game_on( ) ) return;

    int undo_status = undo();
    if ( undo_status > 0 ) {
        SUDOKU_TRACE( SUDOKU_INTERFACE_DEBUG, ("Undo\n"));
        if ( is_game_in_entering_state( ) ) {
            update_entering_state( cntxt );       // updates the status message if needed
        } else {
            SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_BLANK, 0 );
        }

        if ( 2 == undo_status ) {
            SUDOKU_SET_BACK_LEVEL( cntxt, get_bookmark_number( ) );
        }
        reset_cell_attributes();
        update_edit_menu( cntxt );
        SUDOKU_REDRAW( cntxt );
    } else {
        SUDOKU_TRACE( SUDOKU_INTERFACE_DEBUG, ("Nothing to undo\n"));
    }
}

extern void sudoku_redo( const void *cntxt )
{
    SUDOKU_ASSERT( cntxt );
    if ( ! is_game_on( ) ) return;

    int redo_status = redo();
    if ( redo_status > 0 ) {
        SUDOKU_TRACE( SUDOKU_INTERFACE_DEBUG, ("Redo\n"));
        if ( is_game_in_entering_state( ) ) {
            update_entering_state( cntxt ); /* updates the status message if needed */
        } else {
            SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_BLANK, 0 );
        }

        if ( 2 == redo_status ) {
            SUDOKU_SET_BACK_LEVEL( cntxt, get_bookmark_number( ) );
        }
        reset_cell_attributes();
        update_edit_menu( cntxt );
        SUDOKU_REDRAW( cntxt );
    } else {
        SUDOKU_TRACE( SUDOKU_INTERFACE_DEBUG, ("Nothing to redo\n"));
    }
}

