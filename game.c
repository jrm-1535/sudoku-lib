/*
  Sudoku game backend
*/

#ifndef WIN32
#include <unistd.h>
#endif
#include <string.h>
#include <time.h>
#include "game.h"
#include "gen.h"
#include "hint.h"
#include "state.h"
#include "stack.h"
#include "files.h"
#include "rand.h"

sudoku_ui_table_t sudoku_ui_fcts;

/* States:

   0 initially:       no game selected (same as game over)

   1 entering game:   entering values for a new game (game is not started)

    with four substates:
   1.0  current game game is empty (initially)
   1.1  current game has multiple solutions
   1.2  current game has no solution (but it can be still be fixed)
   1.3  current game has only one solution (game can be selected)

   2 game on going:  playing the game

    with 2 substates:
   2.0  nothing played yet, no cell selected (initially)
   2.1  a cell is selected for entry (allow filling that cell)
   2.2  something has been entered (warn if leaving)

   3 game over

   Possible transitions:

   0 -> 1.0 (Make your game)
   0 -> 2.0 (New, Pick, Open game)

   1.0 -> 1.1 (select + input)
   1.1 -> 1.0 (undo)

   1.1 -> 1.1 (input)
   1.1 -> 1.2 (wrong input)
   1.1 -> 1.3 (input)

   1.2 -> 1.1 (undo or replacing wrong input)
   1.2 -> 1.2 (input)

   1.3 -> 1.2 (wrong input)
   1.3 -> 1.3 (input)

   1.3 -> 2.0 (Accept this game)

   2.0 -> 1.0 (Make your game)
   2.0 -> 2.0 (New, pick, Open game)
   2.0 -> 2.1 (select)

   2.1 -> 1.0 (Make your game)
   2.1 -> 2.1 (select other cell)
   2.1 -> 2.2 (input)

   2.2 -> 1.0 (Make your game with Warning!)
   2.2 -> 2.0 (undo) (New, Pick, Open game with Warning!)
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

     Depending on the current game state, substates are indicated by either
     - stack empty (substate 0) and enter_game_valid (substate 3) if in SUDOKU_ENTER state (1)
     - stack empty (substate 0) if in SUDOKU_STARTED (2)

     States 0 and 3 have no substates.
     Note that state 1 has actually 4 substates, but substates 1 and 2 have no external difference.
*/

enum {
  SUDOKU_INIT, SUDOKU_ENTER, SUDOKU_STARTED, SUDOKU_OVER
};

static int sudoku_state = -1;
static bool enter_game_valid = false;
static bool show_conflict = true, auto_check = false; /* default values */

static time_t play_started;
static unsigned long already_played;

static void assert_game_state( int state, char *action )
{
  if ( state != sudoku_state ) {
    printf("%s: Inconsistent state %d (should be %d)\n",
           action, sudoku_state, state );
    exit(1);
  }
}

extern bool sudoku_is_game_on_going( void )
{
    return ( SUDOKU_STARTED == sudoku_state ) && ( ! is_stack_empty() );
}

extern bool sudoku_is_selection_possible( void )
{
  return ( SUDOKU_ENTER == sudoku_state ) || ( SUDOKU_STARTED == sudoku_state );
}

extern bool sudoku_is_entering_game_on_going( void )
{
    return (SUDOKU_ENTER == sudoku_state) && ( ! is_stack_empty() );
}

extern bool sudoku_is_entering_valid_game( void )
{
    return (SUDOKU_ENTER == sudoku_state) && enter_game_valid;
}

static bool is_game_on( void )
{
    return (SUDOKU_ENTER == sudoku_state) || (SUDOKU_STARTED == sudoku_state);
}

static bool is_game_in_entering_state( void ) 
{
    return SUDOKU_ENTER == sudoku_state;
}

static void init_game_menu( void *cntxt, int new_state )
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
        printf("init_game_menu: inconsitent state %d\n", new_state );
        exit(1);
    }
}

static void update_edit_menu( void *cntxt )
{
  if ( is_stack_empty( ) ) {
    SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_UNDO_ITEM );
    SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_ERASE_ITEM );
  }
  else {
    SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_UNDO_ITEM );
    SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_ERASE_ITEM );
  }

  if ( check_redo_level( ) )
    SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_REDO_ITEM );
  else
    SUDOKU_DISABLE_MENU_ITEM( cntxt, SUDOKU_EDIT_MENU, SUDOKU_REDO_ITEM );

  if ( ! is_game_in_entering_state( ) ) {
    switch ( check_at_bookmark() ) {
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

extern void set_current_selection( void *cntxt, int row, int col, bool force_redraw )
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
        SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_TOOL_MENU, SUDOKU_FILL_SEL_ITEM );
    } else {
        assert( row == -1 && col == -1 );
        SUDOKU_DISABLE_MENU_ITEM(cntxt, SUDOKU_TOOL_MENU, SUDOKU_FILL_SEL_ITEM );
    }

    reset_cell_hints();
    SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_BLANK, 0 );

    set_selected_row_col( row, col );
    SUDOKU_REDRAW( cntxt );
}

extern void sudoku_set_selection( void *cntxt, int row, int col )
{
    set_current_selection( cntxt, row, col, false );
}

extern void sudoku_move_selection(  void *cntxt, sudoku_key_t how )
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

static void remove_selection( void *cntxt )
{
  set_selected_row_col(-1, -1 );
  SUDOKU_DISABLE_MENU_ITEM(cntxt, SUDOKU_TOOL_MENU, SUDOKU_FILL_SEL_ITEM );
}

extern void sudoku_mark_state( void *cntxt )
{
    int mark;

    SUDOKU_ASSERT( cntxt );

    assert_game_state ( SUDOKU_STARTED, "sudoku_mark_state" );
    mark = new_bookmark( );
    if ( mark ) {
        set_low_water_mark( get_sp() );
        cancel_redo();
        reset_cell_hints();

        printf("mark #%d set @sp=0x%08x\n", mark, get_sp());

        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_MARK,  mark  );
        SUDOKU_SET_BACK_LEVEL( cntxt, mark );
        update_edit_menu( cntxt );
        SUDOKU_TRACE( SUDOKU_UI_DEBUG, ("Mark #%d, stack_pointer %d\n", mark, get_sp()));
    }
}

extern void sudoku_back_to_mark( void *cntxt )
{
    SUDOKU_ASSERT( cntxt );
    printf("Back to mark - calling return to last bookmark\n");
    assert_game_state ( SUDOKU_STARTED, "sudoku_back_to_mark" );
    int mark = return_to_last_bookmark();
    printf("returned to mark %d\n", mark );
    if ( -1 != mark ) {
        reset_cell_hints();
        printf("Setting new status and updating game\n");
        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_BACK, mark  );
        SUDOKU_SET_BACK_LEVEL( cntxt, mark );
        update_edit_menu( cntxt );
        SUDOKU_REDRAW( cntxt );
    } 
}

static bool check_from_current_position( void )
{
    int saved_states, sp = get_sp();
    set_low_water_mark( sp );
    saved_states = get_redo_level();
    bool res = find_one_solution( saved_states );
    return_to_low_water_mark( sp );
    return res;
}

extern void sudoku_hint( void *cntxt )
{
    int selection_row = -1, selection_col = -1;
    sudoku_hint_type hint;

    SUDOKU_ASSERT( cntxt );
    assert_game_state ( SUDOKU_STARTED, "sudoku_hint" );

    reset_cell_hints();
    if ( ! check_from_current_position() ) {
        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_HINT, NO_SOLUTION );
        return;
    }

    hint = find_hint( &selection_row, &selection_col );
    SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_HINT, hint );

    if ( hint ) {
        if ( (-1 != selection_row) && (-1 != selection_col) ) {
            set_selected_row_col( selection_row, selection_col ); // set_current_selection would remove hints
            SUDOKU_ENABLE_MENU_ITEM( cntxt, SUDOKU_TOOL_MENU, SUDOKU_FILL_SEL_ITEM );
        } else {
            remove_selection( cntxt );
        }
        SUDOKU_REDRAW( cntxt );
    }
}

extern void sudoku_fill( void *cntxt, bool no_conflict )
{
    int row, col;

    SUDOKU_ASSERT( cntxt );

    assert_game_state ( SUDOKU_STARTED, "sudoku_fill" );
    reset_cell_hints();
    get_selected_row_col( &row, &col );
    if ( col != -1 && row != -1 ) {
        cancel_redo();
        fill_in_cell( row, col, no_conflict );
        update_edit_menu( cntxt );
        SUDOKU_REDRAW( cntxt );
    }
}

extern void sudoku_fill_all( void *cntxt, bool no_conflict )
{
    SUDOKU_ASSERT( cntxt );

    assert_game_state ( SUDOKU_STARTED, "sudoku_fill_all" );
    reset_cell_hints();
    cancel_redo();
    init_state_for_solving( 0 );    // fills all non empty cells
    if ( no_conflict ) {
        if ( ! remove_conflicts() ) {
            SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_CHECK, 0 );
        }
    }
    update_edit_menu( cntxt );
    SUDOKU_REDRAW( cntxt );
}

extern void sudoku_check_from_current_position( void *cntxt )
{
    SUDOKU_ASSERT( cntxt );
    assert_game_state ( SUDOKU_STARTED, "sudoku_check_from_current_position" );
    reset_cell_hints();

    if ( check_from_current_position( ) ) {
        SUDOKU_TRACE( SUDOKU_SOLVE_DEBUG, ("Solvable from that position!\n") );
        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_CHECK, 1 );
    } else {
        SUDOKU_TRACE( SUDOKU_SOLVE_DEBUG, ("Not Solvable from that position\n") );
        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_CHECK, 0 );
    }
}

extern void sudoku_solve_from_current_position( void *cntxt )
{
    int saved_states, sp = get_sp();
    SUDOKU_ASSERT( cntxt );

    reset_cell_hints();
    assert_game_state ( SUDOKU_STARTED, "sudoku_solve_from_current_position" );
    set_low_water_mark( sp );
    saved_states = get_redo_level( );

    if ( find_one_solution( saved_states ) ) {
        int last_state_in_stack = get_sp();
        SUDOKU_TRACE( SUDOKU_SOLVE_DEBUG, ("SOLVED!\n")) ;
        return_to_low_water_mark( sp );
        cancel_redo();

        push();
        copy_state( last_state_in_stack );
        update_edit_menu( cntxt );
    } else {
        SUDOKU_TRACE( SUDOKU_SOLVE_DEBUG, ( "Not Solvable from that position\n" ));
        SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_CHECK, 0 );
        return_to_low_water_mark( sp );
    }
    SUDOKU_REDRAW( cntxt );
}

static void update_entering_state( void *cntxt )
{
    int saved_states = get_redo_level( );

    reset_cell_hints();
    switch( check_current_game( saved_states) ) {
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

static void toggle_cell_symbol( int symbol, int row, int col )
{
    sudoku_cell_t *cell = get_cell( row, col );
    int mask = get_map_from_number(symbol - '1');

    if ( cell->symbol_map & mask ) {
        SUDOKU_TRACE( SUDOKU_UI_DEBUG, ( "Removing Symbol %d (0x%02x) remaining symbols %d\n",
                                         symbol, cell->symbol_map ^ mask, cell->n_symbols -1 ) );
        cell->n_symbols--;
    } else { /* symbol was not set */
        SUDOKU_TRACE( SUDOKU_UI_DEBUG, ( "Adding Symbol %d (0x%02x) total symbols %d\n",
                                         symbol, cell->symbol_map ^ mask,
                                         cell->n_symbols +1  ));
        cell->n_symbols++;
    }

    cell->symbol_map ^= mask;
    check_cell_integrity( cell );
}

static bool toggle_symbol( void *cntxt, int symbol )
{
    int col, row;

    SUDOKU_ASSERT( symbol > '0' && symbol <= '9' );
    get_selected_row_col( &row, &col );
    if ( -1 == row || -1 == col ) return false;

    SUDOKU_TRACE( SUDOKU_UI_DEBUG, ("toggle_symbol %c @ row %d col %d\n", symbol, row, col) );

    if ( -1 != push_state() ) {
        if ( is_game_in_entering_state( ) ) {
            set_cell_value( row, col, symbol-'1', false );
            update_entering_state( cntxt );
            cancel_redo();
            update_edit_menu( cntxt );
            SUDOKU_REDRAW( cntxt );
        } else {
            SUDOKU_TRACE( SUDOKU_UI_DEBUG, ("setting status text to empty\n") );
            SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_BLANK, 0 );

            SUDOKU_TRACE( SUDOKU_UI_DEBUG, ("toggle_symbol @stack = %d\n", get_sp() ) );
            toggle_cell_symbol( symbol, row, col );
            if ( show_conflict ) { update_grid_errors( row, col ); }
            cancel_redo();
            update_edit_menu( cntxt );
            SUDOKU_REDRAW( cntxt );

            if ( 1 == is_game_solved( ) ) {
                printf("SOLVED!\n");
                return true;
            }
            if ( auto_check ) {
                sudoku_check_from_current_position( cntxt );
            }
        }
    } else {
        SUDOKU_TRACE( SUDOKU_UI_DEBUG, ("No more room in stack\n") );
        SUDOKU_ASSERT(0);
    }
    return false;
}

static void force_no_game( void *cntxt )
{
    reset_stack( );
    init_state( );
    erase_all_bookmarks();
    SUDOKU_REDRAW( cntxt );
}

extern void sudoku_toggle_entering_new_game( void *cntxt )
{
    if ( is_game_in_entering_state( ) ) {
        // cancel entering mode
        SUDOKU_SET_ENTER_MODE( cntxt, SUDOKU_ENTER_GAME );
        init_game_menu( cntxt, SUDOKU_INIT );
    } else {
        // starts entering new game mode
        SUDOKU_SET_ENTER_MODE( cntxt, SUDOKU_CANCEL_GAME );
        init_game_menu( cntxt, SUDOKU_ENTER );
    }
    force_no_game( cntxt );
}

extern int sudoku_toggle_conflict_detection(  void *cntxt  )
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

extern int sudoku_toggle_auto_checking(  void *cntxt )
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

static void save_and_init_game( void )
{
    push();
    init_state( );  /* do not reset bookmarks or redo level yet */
}

static void forget_saved_game( void *cntxt )
{
    int new = get_sp();
    reset_stack();
    erase_all_bookmarks();
    copy_state( new );
    remove_selection( cntxt );
    reset_cell_hints();
    init_game_menu( cntxt, SUDOKU_STARTED );
}

static void restore_saved_game( void )
{
    pop();
}

static void start_game( void *cntxt, int game_number )
{
    make_game( game_number );
    erase_all_bookmarks();
    remove_selection( cntxt );
    reset_cell_hints();
    init_game_menu( cntxt, SUDOKU_STARTED );
}

extern void sudoku_undo( void *cntxt )
{
    int undo_status;
    SUDOKU_ASSERT( cntxt );

    reset_cell_hints();
    undo_status = undo();
    if ( undo_status > 0 ) {
        int cur_row, cur_col;
        get_selected_row_col( &cur_row, &cur_col );

        SUDOKU_TRACE( SUDOKU_UI_DEBUG, ("Undo\n"));
        if ( is_game_in_entering_state( ) ) {
            update_entering_state( cntxt );       // updates the status message if needed
        } else {
            SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_BLANK, 0 );
        }

        if ( 2 == undo_status ) {
            SUDOKU_SET_BACK_LEVEL( cntxt, get_bookmark( ) );
        }
        update_edit_menu( cntxt );
        set_current_selection( cntxt, cur_row, cur_col, true ); // forces a redaw
    } else {
        SUDOKU_TRACE( SUDOKU_UI_DEBUG, ("Nothing to undo\n"));
    }
}

extern void sudoku_redo( void *cntxt )
{
    int redo_status;
    SUDOKU_ASSERT( cntxt );

    reset_cell_hints();

    redo_status = redo();
    if ( redo_status > 0 ) {
        if ( is_game_in_entering_state( ) ) {
            update_entering_state( cntxt ); /* updates the status message if needed */
        } else {
            SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_BLANK, 0 );
        }

        if ( 2 == redo_status ) {
            SUDOKU_SET_BACK_LEVEL( cntxt, get_bookmark( ) );
        }
        update_edit_menu( cntxt );
        SUDOKU_REDRAW( cntxt );
    }
}

extern void sudoku_erase_selection( void *cntxt )
{
    SUDOKU_ASSERT( cntxt );

    int row, col;
    get_selected_row_col( &row, &col );

    if ( -1 != row && -1 != col ) {
        if ( -1 != push_state( ) ) {
            SUDOKU_TRACE( SUDOKU_UI_DEBUG, ("Erase\n"));

            reset_cell_hints();
            SUDOKU_SET_STATUS( cntxt, SUDOKU_STATUS_BLANK, 0 );

            erase_cell( row, col );
            cancel_redo();
            update_edit_menu( cntxt );
            SUDOKU_REDRAW( cntxt );

            if ( is_game_in_entering_state( ) ) {
                update_entering_state( cntxt ); /* updates the status message if needed */
            }
        }
    }
}

static void new_or_what( void *cntxt );

extern void sudoku_enter_symbol( void *cntxt, int symbol )
{
    if ( ! sudoku_is_selection_possible() ) return;

    reset_cell_hints();

    if ( symbol > '0' && symbol <= '9' ) {
#if SUDOKU_GRAPHIC_DEBUG
        printf("toggle key code %d\n", symbol );
#endif
        if ( toggle_symbol( cntxt, symbol ) ) new_or_what(cntxt);
    }
}

void set_game_time( unsigned long duration )
{
    time( &play_started );
    already_played = duration;
}

unsigned long get_game_duration( void )
{
    double diff;
    unsigned long duration;
    time_t end_time;

    time ( &end_time );
    diff = difftime( end_time, play_started );
    duration = (unsigned long)diff + already_played;
    return duration;
}

extern void sudoku_commit_game( void *cntxt, const char *game_name )
{
    int last_state_in_stack;
    SUDOKU_ASSERT ( SUDOKU_ENTER == sudoku_state );

    make_cells_given();
    last_state_in_stack = get_sp();

    reset_stack( );
    copy_state( last_state_in_stack );
    erase_all_bookmarks();

    SUDOKU_SET_ENTER_MODE( cntxt, SUDOKU_ENTER_GAME );
    SUDOKU_SET_WINDOW_NAME( cntxt, game_name );
    SUDOKU_SET_BACK_LEVEL( cntxt, 0 );
    set_game_time( 0 );
    SUDOKU_REDRAW( cntxt );
    init_game_menu( cntxt, SUDOKU_STARTED );
}

static void do_game( void *cntxt, int game_number )
{
    static char name_buffer[ 16 ];

    sprintf( name_buffer, "s%d", game_number );

    start_game( cntxt, game_number );
    SUDOKU_SET_WINDOW_NAME( cntxt, name_buffer );
    SUDOKU_SET_BACK_LEVEL( cntxt, 0 );
    set_game_time( 0 );
    SUDOKU_REDRAW( cntxt );
    init_game_menu( cntxt, SUDOKU_STARTED );
}

extern void sudoku_pick_game(void *cntxt, const char *number_string )
{
    int game_number;

    if ( number_string ) {
        game_number = atoi( number_string );
        if ( ( game_number >= MIN_SUDOKU_GAME_NUMBER ) &&
            ( game_number <= MAX_SUDOKU_GAME_NUMBER ) ) {
            do_game( cntxt, game_number );
        }
    }
}

extern void sudoku_random_game( void *cntxt )
{
    /* randomly choose a game number */
    int nb = random_value( MIN_SUDOKU_GAME_NUMBER, MAX_SUDOKU_GAME_NUMBER );
    do_game( cntxt, nb );
}

extern int sudoku_open_file( void *cntxt, const char *path )
{
    save_and_init_game(  );
    if ( load_file( path ) ) {
        size_t len;
        const char *name;
        forget_saved_game( cntxt );
        init_game_menu( cntxt, SUDOKU_STARTED );
        len = strlen(path);
        name = path + len;
        while ( --name >= path)
            if ( PATH_SEPARATOR == *name ) break;
        name++;

        SUDOKU_SET_WINDOW_NAME( cntxt, name );
        SUDOKU_REDRAW( cntxt );
        return 1;
    }
    restore_saved_game();
    return 0;
}

#define SEC_IN_MIN  60
#define SEC_IN_HOUR (60*60)

static void break_duration( unsigned long duration, sudoku_duration_t *dhms )
{
    SUDOKU_ASSERT( dhms );

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
        unsigned long time = get_game_duration( );
        break_duration( time, duration_hms );
        return true;
    }
    return false;
}

void new_or_what( void *cntxt )
{
    unsigned long duration = get_game_duration();
    init_game_menu( cntxt, SUDOKU_OVER );
    sudoku_duration_t duration_hms;
    break_duration( duration, &duration_hms );
    printf("         in %d hours, %d min, %d sec\n",
    duration_hms.hours, duration_hms.minutes, duration_hms.seconds);
    SUDOKU_SUCCESS_DIALOG( cntxt, &duration_hms );
}

typedef struct {
    char *file_name;
    int  start_game;
} args_t;

static int parse_args( int argc, char **argv, args_t *args )
{
    SUDOKU_ASSERT( args );
    args->file_name = NULL;
    args->start_game = -1;

    char **pp = &argv[1];

#if SUDOKU_FILE_DEBUG
    printf("argc %d, argv[0] %s\n", argc, argv[0]);
#endif /* SUDOKU_FILE_DEBUG */

    while (--argc) {
        char *s = *pp;
#if SUDOKU_FILE_DEBUG
        printf( "%s\n", *pp);
#endif /* SUDOKU_FILE_DEBUG */

        if (*s++ == '-') {
            switch ( *s++ ) {
            case 'g': case 'G':
                if ( *s ) {
                    args->start_game = atoi( s );
                    break;
                } else if ( --argc ) {
                    s = *++pp;
                    args->start_game = atoi( s );
	                break;
	            }
            	/* falls through */
            default:
                printf("sudoku: error option not recognized\n");
                /* falls through */

            case 'H': case 'h': /* help and stop */
                printf("     sudoku [-g]number [-h] [file]\n" );
	            printf("     -g    start with the following game number\n");
                printf("     -h    display this help message and exits\n");
                printf("     file  is the file name describing the sudoku game to play\n");
                return 1;
            }
        } else {
            if ( NULL != args->file_name ) {
                printf("Too many file names, aborting\n" );
                exit( 1 );
            }
            args->file_name = *pp;
        }
        pp++;
    }
    return 0;
}

static bool new_game = false;

extern int sudoku_game_init( void *instance, int argc, char **argv,
                             sudoku_ui_table_t *fcts )
{
    SUDOKU_ASSERT( instance );
    SUDOKU_ASSERT( argv );
    SUDOKU_ASSERT( fcts );

    args_t args;
    if ( parse_args( argc, argv, &args ) )
        return -1;

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

    if ( ( args.file_name ) && load_file( args.file_name ) ) {
        SUDOKU_SET_WINDOW_NAME( instance, args.file_name );
        new_game = true;
    } else {
        SUDOKU_SET_WINDOW_NAME( instance, SUDOKU_DEFAULT_WINDOW_NAME );
    }
    init_state( );
    init_game_menu( instance, SUDOKU_INIT );
    if ( -1 != args.start_game ) {
        printf("sudoku: starting game %d\n", args.start_game );
        start_game( instance, args.start_game );
    }

    return 0;
}
