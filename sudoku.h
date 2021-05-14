
/** @file sudoku.h
    @brief Suduku game - Interface declarations
*/

#ifndef __SUDOKU_H__
#define __SUDOKU_H__

#include <stdint.h>
#include <stdbool.h>

/** @mainpage sudoku

@section intro_section Introduction

    This sudoku package provides a backend for playing sudoku. 
    As is, it does not implement any user interface: it must be linked with
    a frontend providing all user interaction, graphics, texts and menus.

    The frontend implements the main entry point, implements an event loop,
    and calls the package for executing the game and changing the game state.

    Backend functions should be called from a single thread as internal data
    structures are not protected against concurrent modifications.

    The game package calls back some user Interface functions, for instance a
    redraw function for refreshing the game window, and provides helper
    functions.

    Note that in addition to dialog boxes, the front end needs only to provide
    basic window and graphic functions such as draw lines, fill rectangles and
    render characters.
   
    This document defines functions that are implemented in the game backend
    and those that are called by it, which must be implemented in the UI frontend.

    The UI frontend is described in @ref frontend "User Interface Requirements".
    The interface is described in @ref api "Sudoku Game Interface".
*/

/** @page frontend

@section user_section User Interface Requirements

    Since the frontend is responsible for imprementing the whole user interface,
    the descriptions below are just what the game backend expects. The actual
    presentation is up to the ui frontend. In particular the frontend is responsible
    for displaying the UI in multiple languages.

    However, the grid layout in the game window is expected to follow the classic
    SUDOKU definitions, as described below.

    The main window should display the sudoku game grid. The grid is made of
    81 (9 x 9) cells or squares in sudoku lingua.

    Initially each cell is either empty, or with a single given symbol between
    1 and 9. Given symbols should be marked with a special color to indicate
    they cannot be modified.

    An empty cell can be populated with one or more (up to 9) symbols. A non-given
    symbol is a symbol that is entered at that location by the player.

    A single non-given symbol indicates what the player believes is a solution.
    Multiple non-given symbols can also be entered, and are treated as potential
    solutions (penciled candidates). Multiple symbols should be displayed in
    smaller fonts in order to fit in the same square.

    The current selection should be marked with a special color, either as a
    square filled with a background color or with an outline stroke. The current
    selection indicates where character input and commands will be directed at.

    The user interface is responsible for reacting to menu selection, i.e. for
    asking the backend to execute the associated actions and to update its
    internal state. The current selection can be moved with the mouse by calling
    @ref sudoku_set_selection or with the arrow keys by calling @ref sudoku_move_selection.

    As the selection moves and symbols are entered the game updates its internal
    representation and requests a refresh of the user interface. If conflict
    detection is on, it indicates the cells in conflict with the current selection.
    As a result, a special mark indicates the cells that are potentially in error
    (sharing a symbol with the selected cell and in the same set, row, column or
    block).

    Optionally, that grid may have headers for row and colum numbers (from 1 to 9). 

    The backend expects that all game features are accessible from a menu. Typically
    a menu bar could be displayed at the top of the game window, although a tablet UI
    may choose to use a very different interface. 

    Finally, the backend assumes a status area for displaying messages in the UI.

    A desktop layout might look like the following:

@verbatim
   |---------------------------------------|
   |             Window name         v ^ x |
   |---------------------------------------|
   |[ menu bar                            ]|
   |---------------------------------------|
   ||   |   |   ||   |   |   ||   |   |   ||
   |---------------------------------------|
   ||   |   |   ||   |   |   ||   |   |   ||
   |---------------------------------------|
   ||   |   |   ||   |   |   ||   |   |   ||
   |---------------------------------------|
   |---------------------------------------|
   ||   |   |   ||   |   |   ||   |   |   ||
   |---------------------------------------|
   ||   |   |   ||   |   |   ||   |   |   ||
   |---------------------------------------|
   ||   |   |   ||   |   |   ||   |   |   ||
   |---------------------------------------|
   |---------------------------------------|
   ||   |   |   ||   |   |   ||   |   |   ||
   |---------------------------------------|
   ||   |   |   ||   |   |   ||   |   |   ||
   |---------------------------------------|
   ||   |   |   ||   |   |   ||   |   |   ||
   |---------------------------------------|
   |---------------------------------------|
   |[ status bar                          ]|
   |---------------------------------------|
@endverbatim

    Where double vertical or horizontal lines indicate a larger or more
    visible line in order to show the groups of cells constituting a box.

    A box, or block, is a square of 3x3 cells. There are 9 boxes in the game.

    The window name and the status bar are modified by the back end to
    reflect the current game and its status.

@section menu_section Game Menus

   The menus are expected to be logically similar to:
@verbatim
   File                Edit              Tools                       Help
   ----                ----              -----                       ----
   New                 Undo       uU     Check                   cC  Content
   Pick                Redo       rR     Hint                    hH  search
   Open                Erase      zZ     Fill selection          fF  -------
   Enter your game     -----             Fill all                    About Sudoku
   Save as         sS  Mark       mM     Do Solve                dD
   ----                Back       bB     -------         
   print setup                       [x] Conflict detection
   print                             [ ] Check after each change
   ----                                  -------
   Quit            qQxX                  Options
@endverbatim

   Notes:

    Some of the menu items should have a shortcut.
    u or U, for instance, can indicate a shortcut key for a menu item (Undo).

    [x] indicates a check mark. It is recommended that "Conflict detection" 
    be checked by default, and that "Check after each change" be not.

    The items print, print setup, options, and the whole help menu are
    completely out of the scope of the game itself, and their implementation,
    if any, depends on the windowing system and widget library available on
    the platform. Those menu items may be removed without affecting the game.

    Many game commands are also optional and a minimum implementation may
    not give users access to them.

    The game backend enables or disables a whole menu or menu items,
    according to the current game state. It specifies a menu by its index,
    and a menu item by its index in its menu.

    The menu item "Enter your game" can also be modified by the backend to
    show an alternate text, for example "Cancel game" or "Commit game", by
    a frontend provided function @ref set_enter_mode_fct_t. Similarly, the
    menu item "back" is also modified by the backend to indicate the previous
    mark the command will go back when called.

    Depending on the language the above menus and shortcuts may be different.
    However, the game assumes that the menu layouts are as defined here. If the
    frontend shows different menus or no menu at all, the actual UI implementation
    will have to translate the logical layout expected by the bakend into the
    actual one (see @ref sudoku_menu_t)

@subsection menu_item Menu item definitions

    In the following descriptions, a menu item is identified by its menu name
    followed by colon and the item name (Menu:Item)

@subsubsection file_new File:New

    As a result of selecting this menu item, a random game should be generated
    and started. The front end must call @ref sudoku_random_game to initialize
    a new random game.

    No user input is required. However, the frontend should first check whether
    a game is currently being played by calling @ref sudoku_is_game_on_going,
    and in that case show the @ref warning_start_stop "Warning" dialog to warn
    the user that the current game will be discarded if the action is not cancelled.

@subsubsection file_open File:Open

   The frontend can use the file open dialog box from the supporting window
   manager in order to choose a sudoku game that was previously saved. The game
   name must be passed to @ref sudoku_open_file. If a game was already started
   the user should be asked to confirm whether the current game can be discarded
   (see @ref warning_start_stop "warning")

   It is recommended that sudoku games use the default .sdk extension, altough
   any file name and extension can be used.

@subsubsection file_pick File:Pick

   If a game is already ongoing the user should be asked to confirm whether
   the current game can be discarded (see @ref warning_start_stop "warning").

   The front end should display small dialog box, for entering the game number. 
   (see @ref pick_dialog "pick").

@subsubsection file_enter File:Enter

@anchor enter_mode
   The menu item "Enter your own game" is always available, as new, open,
   pick and quit are, except when it has been selected. It turns then into
   something like "Cancel this game" and all other menus, except quit are
   disabled.

   When the user selects "Enter your own game", the frontend should call the
   backend function @ref sudoku_toggle_entering_new_game, to switch to the
   mode when a game is being created instead of being played.

   In that mode, if the user wants to stop creating the game, it is possible
   to select the quit menu item or to select the "cancel this game" menu
   item. If the user selects 'quit' the frontend should warn that the game
   has not been yet created by showing the @ref warning_discard_entering dialog,
   then, if confirmed, close all windows and exit the program. If instead the
   user selects 'cancel', the frontend should simply call back @ref
   sudoku_toggle_entering_new_game to swich back to the normal mode.

   Initially, when the game is created it has multiple solutions. Once the game
   has one and only one solution, the backend turns the same item into
   "Accept this game" by calling the frontend provided function @ref set_enter_mode_fct_t.
   The user can still make changes to the current game and the backend may change the
   item back to "Cancel this game" if after modifications the game has no or
   more than one solution.

   @see @ref menu_operation

   Only once the game has been either cancelled or accepted this particular
   mode is exited.

@subsubsection file_save File:Save as

   When the user selects 'save as', the frontend should invoke the file save
   dialog box from the supporting window manager in order to choose new name
   or select a file that was previously saved (a new file will be created or
   the existing file will be overwritten). If the user does not cancel, the
   front end should then call @ref sudoku_save_file with the chosen file name.

   It is recommended that files storing sudoku game use the default .sdk extension,
   altough any file name and extension, or no exrtension, can be used.

@subsubsection file_print_setup File:Print setup
@subsubsection file_print  File:Print

   File print and print setup are windowing system dependent.
   The back end code does not do anything about those items: it is up to
   the graphic sub-system to do whatever is required to print the current
   grid...

@subsubsection file_quit   File:Quit

    This terminates the application, the same as clicking the exit button [x]
    on the main window. If a game is ongoing (started and not completed) the
    frontend should show the @ref save_before_quitting_dialog "save" dialog
    box. If a new game is being entered and not committed yet, the frontend
    should show instead the @ref warning_discard_entering "discard" dialog box.


@subsubsection edit_undo Edit:Undo
    This causes the game to go back to the previous state (without the latest entry
    or selection). No dialog box is necessary, and the undo item is disabled if there
    is nothing to undo.

@subsubsection edit_redo Edit:Redo
    This causes the game to return to the next state in the undo stack. No dialog box
    is necessary, and the undo item is disabled if there is nothing to redo.

@subsubsection edit_erase Edit:Erase
    This causes the currently selected cell to be erased (erasing all penciled numbers
    or the entered symbol). No dialog box is necessary and the erase item is disabled if
    the cell does not have any number.

@subsubsection edit_mark Edit:Mark
    This causes the current undo stack level to be saved at the point the player is making
    a bold guess. This allows returning quickly to the position just before the guess was
    made. Marks are put in a stack, allowing to return to multiple levels of guessing. 
    The level is indicated in the 'back' item. @see @ref menu_operation

@subsubsection edit_back Edit:Back
    This causes the game to return to the previous mark. If multiple marks have been saved,
    back indicates the level where it will back. @see @ref menu_operation

@subsubsection tools_check Tools:Check
    This causes the game to check if the game, as currently set, has a solution. The result
    is indicated in the status bar.

@subsubsection tools_hint Tools:Hint
    This causes the game to look for hints, given the cuurent game. The result is indiacated
    in the status bar and on the grid with an area indicating the region related to the hint.
    The selection is also moved to the cell where the hint is meaningful,

@subsubsection tools_fill Tools:Fill
    This causes the game to pencil in the currently selected cell with all symbols that
    are still possible, given all other cells in the game.

@subsubsection tools_fill_all Tools:FillAll
    This causes the game to pencil in all cells with all symbols that are still possible,
    given all other cells in the game.

@subsubsection tools_solve Tools:Solve
    This causes the game to calculate the solution and show the result in the grid. It is
    to undo it and return to the previous state,

@subsubsection tools_conflict Tools:Conflict
    This cause the game to check for conflict with all cells in relation with the selection
    each time a new symbol is entered (that is cells in the same row, colum or box as the
    selection). This can be disabled in the menu.

@subsubsection tools_auto Tools:Auto
    This causes the game to automatically check if the game has a solution each time a symbol
    is entered, and to indicate in the status bar if it is still possible to solve it.

@subsubsection tools_options Tools:Options
    This is not managed by the backend. The front end may provide its own UI settings.

    Finally the help menu is completely dependent on the front end implementation
    It is just expected that there is at least a help item and an About item.

@section Dialogs
    The following dialogs must be presented to warn the user when something will
    be discarded as a result of their request, in order to allow the user
    to change his mind and cancel the request.

@anchor warning_start_stop 
    When the user wants to start a new game, it is possible to check whether
    a game is already ongoing by calling the function @ref sudoku_is_game_on_going.
    In that case, the user should be asked to confirm whether the current game
    can be discarded.

    An alert window such as the following should be displayed:

@verbatim
   -------------------------------------
   [          Stop this game?         x]
   -------------------------------------
   |                                   |
   |      You have started a game.     |
   |                                   |
   | Do you really want to stop it now |
   |       and start a new game?       |
   |                                   |
   |                [Oh NO!]  [ Yes ]  |
   -------------------------------------
@endverbatim

    Note that the gnome guidelines for cancel/ok locations are used here
    (cancel  on the left side and ok/action on the right side) which is the
    reverse on Windows and some other systems. Depending on the port you may
    choose a different layout.

@anchor warning_stop_entering
    Similarly, it is possible to check whether a game is being entered by
    calling the function @ref sudoku_is_entering_game_on_going. In that case, the
    user should be asked to confirm whether the game currently being enterered
    should be discarded.

    An alert window such as the following should be displayed:
@verbatim
   -------------------------------------
   [         Discard this game?       x]
   -------------------------------------
   |                                   |
   |      You are entering a game.     |
   |                                   |
   | Do you really want to lose it now |
   |       and start a new game?       |
   |                                   |
   |                [Oh NO!]  [ Yes ]  |
   -------------------------------------
@endverbatim

@anchor accept_dialog
   When the menu item "Enter a new game" has been selected and once the game
   has one and only one solution, the backend calls the frontend
   @ref set_enter_mode_fct_t with SUDOKU_COMMIT_GAME. The frontend should then
   modify the item text to show something like "Accept this game". Selecting the
   item should display a dialog requesting the user to give a game name:
@verbatim
   -------------------------------
   [      Accept that game?     x]
   -------------------------------
   | --------------------------- |
   | |      choose a name:     | |
   | |                         | |
   | |       [foo     ]        | |
   | --------------------------- |
   | [give up] [cancel] [enter ] |
   -------------------------------
@endverbatim

   "Cancel" returns to the previous state, without notification to backend, letting
   the user make other changes to the current game.

   "Give up" should notify the backend through @ref sudoku_toggle_entering_new_game
   to actually exit the entering mode and return to the normal mode (other menu items
   become selectable again).

   "Enter" commits the current game with the given name, by calling back the backend
   @ref sudoku_commit_game. As for "Give up", the backend goes back to its normal
   mode, but as if the entered game had been generated by new or previously saved and
   re-opened. Note that even though the game has a name, it is not saved by default.

   The current mode is indicated by changing the game mode with @ref set_enter_mode_fct_t.
   Initially the mode is @ref SUDOKU_ENTER_GAME by default. The actual text and language
   is of course left to the user interface.

@anchor game_over_dialog
    when a game has been completed the User Interface should congratulate
    the player and offer to play again. The result is either a call to
    sudoku_random_game() or nothing. The dialog box might look like:

@verbatim
   -------------------------------------
   [        Start a new game?         x]
   -------------------------------------
   |  Game Over                        |
   |                                   |
   |     Congratulations, you won!     |
   |        (in 43 min 14 sec)         |
   |    Do you want to play again?     |
   |                                   |
   |             [No thanks]  [ Yes ]  |
   -------------------------------------
@endverbatim

   Game over is detected by the back end and results in calling the interface function
   @ref success_dialog_fct_t. The time in second passed to the function can be
   used to show how long it took to solve the game (this may depend on optional
   settings). Internally all games are timed, and time spent in the game is always
   passed to this function.

@anchor pick_dialog
@verbatim
   -------------------------------
   [         Pick a game ?      x]
   -------------------------------
   | --------------------------- |
   | | Enter the game number:  | |
   | |                         | |
   | |         [ 121]          | |
   | --------------------------- |
   |          [cancel]  [ pick ] |
   -------------------------------
@endverbatim
   The game number is passed to @ref sudoku_pick_game to initialize a
   particular game. Note that the number is expected to be in ascii
   string format.

@anchor save_before_quitting_dialog
    The frontend shows the following dialog when the user has requested to quit a game
    that is not completed yet.

@verbatim
   --------------------------------------
   [        Save before qutting?       x]
   --------------------------------------
   |                                    |
   |      You have started a game.      |
   |                                    |
   | You can save first and then quit,  |
   |  you can cancel and keep playing,  |
   |    or you can quit without saving. |
   |                                    |
   |       [Save&Quit] [Cancel] [Quit]  |
   --------------------------------------
@endverbatim

@anchor warning_discard_entering
   The frontend shows the following dialog when the user is entering a game that
   has not been committed yet and the quit menu item is selected, or the main
   window exit button is pressed.

@verbatim
   -------------------------------------
   [        Discard this game?        x]
   -------------------------------------
   |                                   |
   |      You are entering a game.     |
   |                                   |
   | Do you really want to discard it  |
   |           now and exit?           |
   |                                   |
   |                [Oh NO!]  [ Yes ]  |
   -------------------------------------
@endverbatim

@subsection menu_operation Operations on menus

     - Change "Back" and "mark" to show the mark stack level (see below)
     - Change "Enter your game" into "Cancel this game" or "Accept this game".

    Back should show the level as explained below.
     - Initially Back does not show any level and is disabled (by @ref
       disable_menu_fct_t called by the game back end).
     - After a first mark, back is set to level 1 (by @ref set_back_level_fct_t 
       called by the back end), and both mark and back are disabled (again by 
       @ref disable_menu_fct_t called by the game back end).
     - After any operation changing the state, mark and back are enabled (by the game
       back end  @ref enable_menu_fct_t).
     - Each new mark increases the current back level (by @ref set_back_level_fct_t 
       called by the game back end).
     - Each back decreases the current back level (again by @ref set_back_level_fct_t 
       called by the game back end).
     - Once the back level is decremented to 0, it is disabled (by @ref 
       disable_menu_fct_t called by the game back end).

    The game backend calls @ref set_enter_mode_fct_t to switch the text of "Enter your
    game" menu item, to "Cancel this game" as soon as "Enter your game" has been
    selected and it switches to "Accept this game" once the proposed game has only 
    one solution. Since the text can be written in many languages, Sudoku does
    not specify the actual text to display. Instead it indicates the logical action
    (ENTER or CANCEL or COMMIT), and let the user interface deal with the actual
    text display. Similarly for Mark and Back, only the level is passed to the
    user interface, not the actual text.
*/

/** @page api

@section api_section Sudoku Game Interface

   The API is broken down into the following groups (from the back-end perspective):

@ref interface
   for user interface functions that the back-end needs to call as it processes
   changes (e.g. redraw, change status line, enable or disable menu items).
   This includes all graphic and User Interface functions required by the
   Sudoku game library.

   Those functions must be implemented in the UI front end, otherwise the
   game will fail to initialize.

@ref operation
   for game functions that can be called by the front end at any time, but
   usually as a result of a user action (typically activating a menu, moving
   the selected cell or ebtering a symbol). Most of those functions, if not
   all, should be called at some point in the game in order to run the game.

@ref helper
   for simple helper functions provided by the Sudoku game backend, for
   querying the game status or game information. 

@ref setup
   for functions provided by the sudoku game library, which must be
   called when the user interface is ready to initialize or start the game.

   At some point in its initialization the front end must call the function
   @ref sudoku_game_init and pass a pointer to the array of functions it
   provides for implementing the sudoku interface.
*/

/** @addtogroup interface

   The following functions must be provided by the window system and passed
   to the game as an array with sudoku_game_init. They are called by the game
   during user action processing.
   @{
*/

/** redraw game
   @param[in] cntxt  The graphic/UI context passed back an forth between UI front end and game backend
   @remark  This function is called at each modification that may impact the visual presentation.
*/
typedef void (*redraw_fct_t)( void *cntxt );

/** set window name
   @param[in] cntxt  The graphic/UI context passed back an forth between UI front end and game backend
   @param[in] name   The name to set for the main game window
   @remark  This function is called initially and when a new game is commited, generated or opened.
*/
typedef void (*set_window_name_fct_t)( void *cntxt, const char *name );

/** sudoku_status_t
   Depending on the current action the game might desire to display some
   status, error or condition. In order to work in any language, this is
   done through a status and an optional value, leaving the actual text
   to display up to the user interface.
*/
typedef enum {
  SUDOKU_STATUS_BLANK,              /**< erase previous status line */
  SUDOKU_STATUS_DUPLICATE,          /**< Duplicate symbol */
  SUDOKU_STATUS_MARK,               /**< Mark #%d, value */
  SUDOKU_STATUS_BACK,               /**< Back to Mark #%d, value */
  SUDOKU_STATUS_CHECK,              /**< Possible/Impossible) */
  SUDOKU_STATUS_HINT,               /**< see @ref sudoku_hint_type */

  SUDOKU_STATUS_NO_SOLUTION,        /**< No solution */
  SUDOKU_STATUS_ONE_SOLUTION_ONLY,  /**< Only ONE solution */
  SUDOKU_STATUS_SEVERAL_SOLUTIONS   /**< More than one solution */

} sudoku_status_t;

/** sudoku_hint_type
    As a result of calling sudoku_hint, the back end may display a hint message
    by passing to the function set_status_line a SUDOKU_STATUS_HINT status with
    a sudoku_hint_type.
*/
typedef enum {
  NO_HINT = 0,
  NO_SOLUTION,                      /**< No solution, undo first */
  NAKED_SINGLE,                     /**< Naked single */
  HIDDEN_SINGLE,                    /**< Hidden single */
  LOCKED_CANDIDATE,                 /**< Locked candidates */
  NAKED_SUBSET,                     /**< Naked subset (pair, triplet) */
  HIDDEN_SUBSET,                    /**< Hidden subset (pair, triplet) */
  XWING,                            /**< X-Wing */
  SWORDFISH,                        /**< SWORDFISH */
  JELLYFISH,                        /**< Jellyfish */
  XY_WING,                          /**< XY-Wing */
  CHAIN                             /**< Coloring or forbidding chain */
} sudoku_hint_type;

/** set_status_fct_t
   @param[in] cntxt   The graphic/UI context passed back an forth between UI front end and game backend
   @param[in] status  The game status to show
   @param[in] value   An additional information to be used with some status values.
   @remark This function is called to change the game status line, according to the status value:
     - @ref SUDOKU_STATUS_BLANK for erasing the status line (value is not used)
     - @ref SUDOKU_STATUS_DUPLICATE for indicating a duplicate symbol error (no value)
     - @ref SUDOKU_STATUS_MARK for indicating a new mark (value is the new  mark level)
     - @ref SUDOKU_STATUS_BACK for indicating we are back to the previous mark (value is the mark level)
     - @ref SUDOKU_STATUS_CHECK for indicating whether the game is solvable (value is TRUE or FALSE)
     - @ref SUDOKU_STATUS_HINT for indicating a hint at the selection (value is the hint type)
     - @ref SUDOKU_STATUS_NO_SOLUTION for indicating the current configuration has no solution when entering a game manually (no value).  In this case the game cannot be accepted.
     - @ref SUDOKU_STATUS_ONE_SOLUTION_ONLY for indicating the current configuration has a single solution when entering a game manually (no value). In this case the game can be accepted.
     - @ref SUDOKU_STATUS_SEVERAL_SOLUTIONS fo indicating the current configuration has more than one solution when entering a game manually (no value). In this case the game cannot be accepted.
*/
typedef void (*set_status_fct_t)( void *cntxt, sudoku_status_t status, int value );

/** set_back_level_fct_t
   @param[in] cntxt   The graphic/UI context passed back an forth between UI front end and game backend
   @param[in] level   The mark level to show in menu item (see below).
   @remark  Marks are managed as a stack, pushing a new mark at each call to
            @ref sudoku_mark_state and popping up the last mark at each call to the function
            @ref sudoku_back_to_mark. It is suggested to visually indicate the stack as
            follows: the menu item "Back" should indicate the level except in case
            the level is 0 where it should not indicate any level (it is disabled anyway).
   In summary:

@verbatim
    initially      1st mark       2nd mark       1st back      2nd back
     level 0        level 1        level 2        level 1       level 0
    back (greyed)  back to 1      back to 2      back to 1     back (greyed)
@endverbatim
*/
typedef void (*set_back_level_fct_t)( void *cntxt, int level );

/** sudoku_mode_t
    Game entering modes: actively giving symbols, cancelling the entry or committing the new game
    (when it has one and only one solution).
 */
typedef enum {
  SUDOKU_ENTER_GAME,             /**< Keep entering symbols */
  SUDOKU_CANCEL_GAME,            /**< Cancelling the current game */
  SUDOKU_COMMIT_GAME             /**< Committing the current game */
} sudoku_mode_t;

/** set_enter_mode_fct_t
   @param[in] cntxt   The graphic/UI context passed back an forth between UI front end and game backend
   @param[in] mode    The entering mode (see @ref sudoku_mode_t).
   @remark            The current mode is indicated by changing the menu item text. Initially the
                      mode is @ref SUDOKU_ENTER_GAME by default. As soon as one cell has a symbol,
                      the backend calls with @ref SUDOKU_CANCEL_GAME. When the game has a single
                      solution, the backend calls with @ref SUDOKU_COMMIT_GAME,

                      The actual text and language are left to the user interface
                      (see @ref enter_mode "Enter your own game").
*/
typedef void (*set_enter_mode_fct_t)( void *cntxt, sudoku_mode_t mode );

/** Top level menu index  */
typedef enum {
  SUDOKU_MENU_START = 0,
  SUDOKU_FILE_MENU = SUDOKU_MENU_START, /**< File Menu is first */
  SUDOKU_EDIT_MENU,                     /**< Edit Menu is second */
  SUDOKU_TOOL_MENU,                     /**< Tool Menu is third */
  SUDOKU_HELP_MENU,                     /**< Help Menu is last (not managed by the backend) */
  SUDOKU_MENU_BEYOND
} sudoku_menu_t;

/** enable_menu_fct_t
   @param[in] cntxt   The graphic/UI context passed back an forth between UI front end and game backend
   @param[in] which   The menu index (see @ref sudoku_menu_t).
   @remark The whole menu corresponding to the passed index (which) must be enabled (selectable).
*/
typedef void (*enable_menu_fct_t)( void *cntxt, sudoku_menu_t which );

/** disable_menu_fct_t
   @param[in] cntxt   The graphic/UI context passed back an forth between UI front end and game backend
   @param[in] which   The menu index (see @ref sudoku_menu_t).
   @remark The whole menu corresponding to the passed index (which) must be disabled (grayed).
*/
typedef void (*disable_menu_fct_t)( void *cntxt, sudoku_menu_t which );

/** File menu items

   Note that only ENTER item can be changed (to CANCEL or ACCEPT).
   NEW, PICK, OPEN, ENTER, SAVE, PRINT, PRINT_SETUP and EXIT items can be enabled/disabled.
*/
typedef enum {
  SUDOKU_FILE_START = 0,
  SUDOKU_NEW_ITEM = SUDOKU_FILE_START,   /**< new random game */
  SUDOKU_PICK_ITEM,                      /**< select a new game */
  SUDOKU_OPEN_ITEM,                      /**< open a saved game */
  SUDOKU_ENTER_ITEM,                     /**< create your own game */
  SUDOKU_SAVE_ITEM,                      /**< save current game */
  SUDOKU_PRINT_ITEM,                     /**< print game */
  SUDOKU_PRINT_SETUP_ITEM,               /**< prepare for printing */
  SUDOKU_EXIT_ITEM,                      /**< quit game */
  SUDOKU_FILE_BEYOND
} sudoku_file_items;

/** Edit Menu items

   UNDO, REDO, ERASE, MARK or BACK items can be enabled/disabled
*/
typedef enum {
  SUDOKU_EDIT_START = 0,
  SUDOKU_UNDO_ITEM = SUDOKU_EDIT_START,  /**< undo last move */
  SUDOKU_REDO_ITEM,                      /**< redo (undo undo) */
  SUDOKU_ERASE_ITEM,                     /**< erase selected cell */
  SUDOKU_MARK_ITEM,                      /**< mark current state */
  SUDOKU_BACK_ITEM,                      /**< return to marked state */
  SUDOKU_EDIT_BEYOND
} sudoku_edit_items;

/** Tool Menu items

  CHECK, HINT, FILL, FILL_ALL and SOLVE can be enabled/disabled
*/
typedef enum {
  SUDOKU_TOOL_START = 0,
  SUDOKU_CHECK_ITEM = SUDOKU_TOOL_START, /**< Check if a solution is possible */
  SUDOKU_HINT_ITEM,                      /**< Hint about candidates that may be reduced */
  SUDOKU_FILL_SEL_ITEM,                  /**< fill up selected cell with all symbols */
  SUDOKU_FILL_ALL_ITEM,                  /**< fill up all cells with all symbols */
  SUDOKU_SOLVE_ITEM,                     /**< Show the solution */
  SUDOKU_DETECT_ITEM,                    /**< check box: toggle check when activated - conflict */
  SUDOKU_AUTO_ITEM,                      /**< check box: toggle check when activated - no solution */
  SUDOKU_OPTION_ITEM,
  SUDOKU_TOOL_BEYOND
} sudoku_tool_items;

/* Note that the complete Help menu is not managed within the sudoku game */

/** enable_menu_item_fct_t
   @param[in] cntxt      The graphic/UI context passed back an forth between UI front
                         end and game backend
   @param[in] which_menu The menu index (see @ref sudoku_menu_t).
   @param[in] which_item The item index inside which_menu (see @ref sudoku_file_items,
                         @ref sudoku_edit_items or @ref sudoku_tool_items).
   @remark               Enable the menu item corresponding to the passed menu
                         (which_menu) and item in that menu (which_item).
*/
typedef void (*enable_menu_item_fct_t)( void *cntxt,
                                        sudoku_menu_t which_menu, 
                                        int which_item );

/** disable_menu_item_fct_t
   @param[in] cntxt      The graphic/UI context passed back an forth between UI front
                         end and game backend
   @param[in] which_menu The menu index (see @ref sudoku_menu_t).
   @param[in] which_item The item index inside which_menu (see @ref sudoku_file_items,
                         @ref sudoku_edit_items or @ref sudoku_tool_items).
   @remark               Disable the menu item corresponding to the passed menu
                         (which_menu) and item in that menu (which_item).
*/
typedef void (*disable_menu_item_fct_t)( void *cntxt, 
                                         sudoku_menu_t which_menu, 
                                         int which_item );

/** sudoku_duration_t
    Time the game has been played so far. Passed to success_dailog function and
    returned by @ref sudoku_how_long_playing */
typedef struct {
  unsigned int hours,    /**< hours elapsed since the game was started */
               minutes,  /**< minutes elapsed since the game was started */
               seconds;  /**< seconds elapsed since the game was started */
} sudoku_duration_t;

/** success_dialog_fct_t
   @param[in] cntxt      The graphic/UI context passed back an forth between UI front
                         end and game backend
   @param[in] dhms       The time spent on that game(see @ref sudoku_duration_t).
   @remark   This function is called when a game has been completed. The User Interface
    should congratulate the player and offer to play again. The result is either
    a call to sudoku_random_game() or nothing (see @ref game_over_dialog).

    If the answer is Yes, then the user interface should call
    @ref sudoku_random_game before returning.
*/
typedef void (*success_dialog_fct_t)( void *cntxt, sudoku_duration_t *dhms );

/** @} */

/** @addtogroup setup
   The following functions are provided by the game to the user interface in order to
   setup the game.
   @{
*/


/** sudoku_ui_table_t
    function table provided by the UI front end */
typedef struct {
  redraw_fct_t            redraw;            /**< how to force a redraw */
  set_window_name_fct_t   set_window_name;   /**< how to update window name */
  set_status_fct_t        set_status;        /**< how to set status */
  set_back_level_fct_t    set_back_level;    /**< how to indicate back level */
  set_enter_mode_fct_t    set_enter_mode;    /**< how to indicate enter mode */
  enable_menu_fct_t       enable_menu;       /**< how to enable a whole menu */
  disable_menu_fct_t      disable_menu;      /**< how to disable a whole menu */
  enable_menu_item_fct_t  enable_menu_item;  /**< how to enable a menu item */
  disable_menu_item_fct_t disable_menu_item; /**< how to disable a menu item */
  success_dialog_fct_t    success_dialog;    /**< how to end a successful game */

} sudoku_ui_table_t;

/** sudoku_game_init
   @param[in] instance   The graphic/UI context passed back an forth between UI front
                         end and game backend
   @param[in] argc       The number of arguments in the following argv array
   @param[in] argv       The argument array.
   @param[in] fcts       The interface function table.
   @remark   This function initializes the game from the windowing system main funtion.
             This must be called by the front end before creating the game window.
             The argument instance indicates an opaque window structure or handle that
             is always passed back when calling the window/graphic system. The arguments
             argc and argv are the standard arguments to the main function. The interface
             function table gives the front-end callbacks that the back-end uses to drive
             the user interface.
*/
extern int sudoku_game_init( void *instance, int argc, char **argv,
                             sudoku_ui_table_t *fcts );

/** @} */

/** @addtogroup helper

   The following three functions are provided by the game as a help to implement
   stateless user interfaces.
   @{
*/

/** sudoku_is_game_on_going
   @remark  This function should be called from enter, open, pick, and new menu dialogs,
            in order to check if a game is already on going, and warn about loosing a
            started game (See @ref warning_start_stop).
   @remark  Note that another function can be used to prevent selecting cells or entering
            symbols when a game is not started or not being manually entered (see @ref
            sudoku_is_selection_possible)
*/
extern bool sudoku_is_game_on_going( void );

/** sudoku_is_entering_game_on_going
   @remark This function should be called from enter, open, pick, and new menu dialogs,
           in order to check if a game is being entered, and warn about loosing that
           game (see @ref warning_stop_entering).
*/
extern bool sudoku_is_entering_game_on_going( void );

/** sudoku_is_entering_valid_game
   @remark If the new entered game is ready to be validated, and the user selects
           the menu item enter/accept, then the user interface should display the
           enter game name dialog box and commit the new game. If on the contrary
           the user is still in the process of entering a new game but this one
           is not ready (it has more than one solution or no solution), the user
           interface should simply cancel by toggling the mode (see @ref
           sudoku_toggle_entering_new_game).
*/
extern bool sudoku_is_entering_valid_game( void );

/** sudoku_is_selection_possible
   @remark This is used by the user interface to check whether a selection is possible,
           that is when a game is in process of being created, ready or ongoing.
*/
extern bool sudoku_is_selection_possible( void );

/** sudoku_how_long_playing
   @param duration  The time already spent playing the current game.
   @remark This function returns true if the game was ongoing, and in that
    case it also gives the time elapsed playing the current game. It can be
    called at any time, when the user has selected to display the elapsed time.
*/
extern bool sudoku_how_long_playing( sudoku_duration_t *duration );

/** @} */

/** @addtogroup operation
   The following functions are provided by the game to the user interface in order
   to allow playing a game. They should be called in response to menu activation.
   @{
*/

/** sudoku_random_game
   @param[in] cntxt         The graphic/UI context passed back an forth between UI front
                            end and game backend
   @remark   This function should be called by the front end as result of selecting
             the menu item File:New. It is the front-end responsibility to display first
             a confirmation menu (see @ref warning_start_stop) if a game is already ongoing
             or a game is being entered (see @ref sudoku_is_game_on_going). The function
             generates a random number and initializes the corresponding game without
             any user input.
*/
extern void sudoku_random_game( void *cntxt  );

/** sudoku_pick_game
   @param[in] cntxt         The graphic/UI context passed back an forth between UI front
                            end and game backend
   @param[in] number_string The number passed as an ASCII string.
   @remark   This function should be called by the front end as a result of selecting
             the menu item File:Pick. It is the front-end responsibility to display first
             a confirmation menu (see @ref warning_start_stop) if a game is already ongoing
             or a game is being entered (see @ref sudoku_is_game_on_going). A small
             dialog box (see @ref pick_dialog "pick") allows entering the game number.
             That game number is passed to initialize a particular game. Note that the
             number is expected to be in ascii string format.
*/
extern void sudoku_pick_game(void *cntxt, const char *number_string );

/** sudoku_open_file
   @param[in] cntxt      The graphic/UI context passed back an forth between UI front
                         end and game backend
   @param[in] path       The absolute file path name.
   @remark   This function should be called by the front end as a result of selecting
             the menu item File:Open. It is the front-end responsibility to display first
             a confirmation menu (see @ref warning_start_stop) if a game is already ongoing
             or a game is being entered (see @ref sudoku_is_game_on_going). It is expected
             that the front end use the file chooser dialog box from the supporting window
             manager in order to select a sudoku game that was previously saved. It is
             recommended that sudoku games use the default .sdk extension, but any file
             name can be used.
*/
extern int sudoku_open_file( void *cntxt, const char *path );

/** sudoku_toggle_entering_new_game
   @param[in] cntxt      The graphic/UI context passed back an forth between UI front
                         end and game backend
   @remark   This function should be called by the front end as result of selecting
             the menu item Enter your game/Cancel this game (depending on the current
             state set by the backend with @ref set_enter_mode_fct_t). */
extern void sudoku_toggle_entering_new_game( void *cntxt );

/** sudoku_commit_game
   @param[in] cntxt      The graphic/UI context passed back an forth between UI front
                         end and game backend
   @param[in] game_name  The new game name.
   @remark   This function should be called by the front end as a result of selecting
             the menu item Accept this game (set by the game backend in place of
             "Enter your game" or "Cancel your game" when the new game as exactly one solution).
             At this point the user cannot cancel anymore using the menu, unless she
             makes some change to the game so that it has no solution or more than
             one solution. The UI front end should display a dialog box (@ref accept_dialog
             "Accept") allowing the used to accept or cancel that game. Once the user
	         accepts the front-end should call this function.
*/
extern void sudoku_commit_game( void *cntxt, const char *game_name );

/** sudoku_save_file
   @param[in] cntxt      The graphic/UI context passed back an forth between UI front
                         end and game backend
   @param[in] path       The absolute game path name.
   @remark   This function should be called by the front end as a result of selecting
             the menu itemFile:Save. As for sudoku_file_open, it is expected that the
             front end use the file chooser dialog box from the supporting window
             manager in order to select a game name and its location. If the file exists
             it is overwriten.
*/
extern int sudoku_save_file(  void *cntxt, const char *path );

/** sudoku_undo
   @param[in] cntxt      The graphic/UI context passed back an forth between UI front
                         end and game backend.
   @remark   This function should be called by the front end as a result of selecting
             the menu Edit:Undo. The function just undoes the last operation.
*/
extern void sudoku_undo( void *cntxt );


/** sudoku_redo
   @param[in] cntxt      The graphic/UI context passed back an forth between UI front
                         end and game backend.
   @remark   This function should be called by the front end as a result of selecting
             the menu Edit:Redo. The function redoes what has just been undone.
*/
extern void sudoku_redo( void *cntxt );

/** sudoku_erase_selection
   @param[in] cntxt      The graphic/UI context passed back an forth between UI front
                         end and game backend.
   @remark   This function should be called by the front end as a result of selecting
             the menu Edit:Erase. The function just erases the values at the current
             selection.
*/
extern void sudoku_erase_selection( void *cntxt );

/** sudoku_mark_state
   @param[in] cntxt      The graphic/UI context passed back an forth between UI front
                         end and game backend.
   @remark   This function should be called by the front end as a result of selecting
             the menu Edit:Mark. The function just marks the current state in order
             to return quickly to it if needed (see @ref sudoku_back_to_mark).
*/
extern void sudoku_mark_state( void *cntxt );

/** sudoku_back_to_mark
   @param[in] cntxt      The graphic/UI context passed back an forth between UI front
                         end and game backend.
   @remark   This function should be called by the front end as a result of selecting
             the menu Edit:Back. The function just goes back quickly to the last marked
             state, as if the user had undone all operations in between. Note that
             returning to the last marked stated CANNOT be undone, although it is still
             possble to redo all operations one by one. However the last mark is removed
             and it will not be possible to return to it (unless the same state is marked
             again).
*/
extern void sudoku_back_to_mark( void *cntxt );

/** sudoku_check_from_current_position
   @param[in] cntxt      The graphic/UI context passed back an forth between UI front
                         end and game backend.
   @remark   This function should be called by the front end as a result of selecting
             the menu Tools:Check, The function just checks whether there is a solution
             from the current state. The solver takes in account all symbols already
             entered (including multiple symbols in a single square).
*/
extern void sudoku_check_from_current_position( void *cntxt );

/** sudoku_hint
   @param[in] cntxt      The graphic/UI context passed back an forth between UI front
                         end and game backend.
   @remark  This function should be called by the front end as a result of selecting
            the menu Tools:Hint. The function just moves the selection to a square
            that can be found by simple logic (if any). Hint may fail if the grid is
            to difficult to solve by simple logic only (i.e. if it needs back tracking).
*/
extern void sudoku_hint( void *cntxt );

/** sudoku_fill
   @param[in] cntxt       The graphic/UI context passed back an forth between UI front
                          end and game backend.
   @param[in] no_conflict Indicates whether symbols conflicting with other squares
                          should be automatically removed when filling up the square.
   @remark  This function should be called by the front end as a result of selecting
            the menu Tools:Fill Selection. The function just fills up the currently
            selected cell with all symbols. The argument no_conflict can be used to
            remove automatically the symbols that would create conflict otherwise.
*/
extern void sudoku_fill( void *cntxt, bool no_conflict );

/** sudoku_fill_all
   @param[in] cntxt       The graphic/UI context passed back an forth between UI front
                          end and game backend.
   @param[in] no_conflict Indicates whether symbols conflicting with other squares
                          should be automatically removed when filling up the square.
   @remark  This function should be called by the front end as a result of selecting
            the menu Tools:Fill All. The function just fills up all the cells that are
            still empty with all symbols. BEWARE: if no_conflict if FALSE and conflict
            detection is on (see @ref sudoku_toggle_conflict_detection) then it may
            display zillions of conflicts as the selection is moved. If, on the contray,
            no_conflict is TRUE, then it may completley solve the game in extremely
            simple cases.
*/
extern void sudoku_fill_all( void *cntxt, bool no_conflict );

/** sudoku_solve_from_current_position
   @param[in] cntxt       The graphic/UI context passed back an forth between UI front
                          end and game backend.
   @remark  This function should be called by the front end as a result of selecting
            the menu Tools:Solve. The function just solves the game from the current
            state if possible. If the game has no solution this is indicated by a
            message in the status bar. If the game has a solution, that solution
            is shown. It is still possible to undo the action and go back to solving
            the game by hand.
*/
extern void sudoku_solve_from_current_position( void *cntxt );

/** sudoku_toggle_conflict_detection
   @param[in] cntxt       The graphic/UI context passed back an forth between UI front
                          end and game backend.
   @remark  This function should be called by the front end as a result of selecting
            the menu Tools:Conflict.  This is a check box. Conflict detection is on
            by default when the game is initialized. The function toggles conflict
            detection in the game. It returns the previous state before toggling the
            state.
*/
extern int sudoku_toggle_conflict_detection( void *cntxt );

/** sudoku_toggle_auto_checking
   @param[in] cntxt       The graphic/UI context passed back an forth between UI front
                          end and game backend.
   @remark  This function should be called by the front end as a result of selecting
            the menu Tools:AutoCheck. This is a check box. Auto checking (checking
            automatically after each move) is off by default when the game is initialized.
            The function is called to toggle auto checking in the game. It returns the
            previous state before toggling the state. If the game is on this will force
            a redraw.
*/
extern int sudoku_toggle_auto_checking( void *cntxt );

/** sudoku_key_t
    Codes indicating how to move the current selection */
typedef enum {
    SUDOKU_NO_KEY,          /**< no effect */
    SUDOKU_UP_ARROW,        /**< move up */
    SUDOKU_DOWN_ARROW,      /**< move down */
    SUDOKU_LEFT_ARROW,      /**< move left */
    SUDOKU_RIGHT_ARROW,     /**< move right */
    SUDOKU_PAGE_UP,         /**< move to the top */
    SUDOKU_PAGE_DOWN,       /**< move to the bottom */
    SUDOKU_HOME_KEY,        /**< move to the left edge */
    SUDOKU_END_KEY          /**< move to the rugth edge */
} sudoku_key_t;

/** sudoku_move_selection
   @param[in] cntxt        The graphic/UI context passed back an forth between UI front
                           end and game backend.
   @param[in] how          Key entered, indicating how the selection should be moved
   @remark                 This is used by the user interface to inform the game that
                           the user wants to move the selection with a keypress.
   @remark                 Note that if the user presses the keys DEL or BACK SPACE,
                           the function @ref sudoku_erase_selection should be called
                           instead.
*/
extern void sudoku_move_selection( void *cntxt, sudoku_key_t how );

/** sudoku_set_selection
   @param[in] cntxt        The graphic/UI context passed back an forth between UI front
                           end and game backend.
   @param[in] row          row in which a selection is made.
   @param[in] col          column in which a selection is made.
   @remark                 This is used by the user interface to inform the game that the
                           user selected a cell with a mouse click.
*/
extern void sudoku_set_selection( void *cntxt, int row, int col );

#define SUDOKU_N_ROWS           9 /**< Rows are numbered from 0 to 8 */
#define SUDOKU_N_COLS           9 /**< Columns are numbered from 0 to 8 */
#define SUDOKU_N_BOXES          9 /**< Boxes are numbered from 0 to 8 */

#define SUDOKU_PENCILED_PER_ROW 3 /**< 3 smaller characters per row in a cell */
#define SUDOKU_PENCILED_ROWS    3 /**< 3 rows of penciled candidates in a cell */

/** sudoku_cell_state_t
    Lists all attributes a cell can have, which defines the cell rendering state */
typedef enum {
    SUDOKU_CANDIDATE = 0,           /**< cell is open to modifications */
    SUDOKU_GIVEN = 1,               /**< cell is given, non modifiable */
    SUDOKU_SELECTED = 2,            /**< cell is currently selected */
    SUDOKU_HINT = 4,                /**< cell is a hint (after sudoku_hint has been called) */
    SUDOKU_CHAIN_HEAD = 8,          /**< cell is head of a chain (after sudoku_hint has been called) */
    SUDOKU_IN_ERROR = 16,           /**< cell is in error with regard to current selection */
    SUDOKU_WEAK_TRIGGER = 32,       /**< cell is a weak trigger (after sudoku_hint has been called) */
    SUDOKU_TRIGGER = 64,            /**< cell is a trigger (after sudoku_hint has been called) */
    SUDOKU_ALTERNATE_TRIGGER = 128  /**< cell is an alternate trigger (after sudoku_hint has been called) */
} sudoku_cell_state_t;

/** sudoku_cell_t
   a cell is defined by:
    - the number of symbols it contains, 0 (unknown), 1 (given or entered) or up to 9 penciled candidates (1 < n <= 9)
    - the symbol or candidates are given in the symbol_map as 1 bit per possible symbol (bit 0 for symbol 1, up to bit 8 for symbol 9).
    - its cell state (sudoku_cell_state_t)


    @remark     It is expected that the user interface shows a given symbol as different
                a symbol entered by the user, the selected cell with some visible highlight,
                cells in error with a very distinctive mark (e.g. red background), as well
                as hints and various types of triggers showing the reason for a hint.

    It is suggested to present the penciled symbols, from 2 up to 9 symbols as
    3 rows of 3 symbols, for example the list { 1, 3, 5, 6, 7, 9 } as :
@verbatim
     ---|-------|---
        | 1   3 |
        |   5 6 |
        | 7   9 |
     ---|-------|---
@endverbatim
*/
typedef struct {
  sudoku_cell_state_t state;      /**< Cell rendering state */
  uint16_t            symbol_map; /**< bitmap, symbol 1 -> bit 0 .., symbol 9 -> bit 8 */
  uint8_t             n_symbols;  /**< from 0 to 9 (2..9 for penciled candidates) */
} sudoku_cell_t;

#define SUDOKU_N_SYMBOLS        9 /**< 1, 2, 3, 4, 5, 6, 7, 8, 9 */
#define SUDOKU_SYMBOL_MASK 0x01ff /**< 9 bits are used (2^0..2^8) */

#define SUDOKU_IS_CELL_GIVEN( _s )              ((bool)((_s) & SUDOKU_GIVEN))
#define SUDOKU_IS_CELL_SELECTED( _s )           ((bool)((_s) & SUDOKU_SELECTED))
#define SUDOKU_IS_CELL_HINT( _s )               ((bool)((_s) & SUDOKU_HINT))
#define SUDOKU_IS_CELL_CHAIN_HEAD( _s )         ((bool)((_s) & SUDOKU_CHAIN_HEAD))
#define SUDOKU_IS_CELL_IN_ERROR( _s )           ((bool)((_s) & SUDOKU_IN_ERROR))
#define SUDOKU_IS_CELL_WEAK_TRIGGER( _s )       ((bool)((_s) & SUDOKU_WEAK_TRIGGER))
#define SUDOKU_IS_CELL_TRIGGER( _s )            ((bool)((_s) & SUDOKU_TRIGGER))
#define SUDOKU_IS_CELL_ALTERNATE_TRIGGER( _s )  ((bool)((_s) & SUDOKU_ALTERNATE_TRIGGER))

/** sudoku_get_cell_definition
   @param[in]  row         row of the cell that is requested.
   @param[in]  col         column of the cell that is requested.
   @param[out] cell        A pointer to a cell definition that the fuction should
                           update if the call is successful.
   @remark                 This is used by the user interface code in order to get a
                           cell definition, given its row and column. If the row or
                           column is invalid, the function returns false, otherwise
                           it updates the cell definition and returns true.
*/
extern bool sudoku_get_cell_definition( int row, int col, sudoku_cell_t *cell );

/** sudoku_get_symbol

    If the cell contains a single symbol, sudoku_get_symbol returns the symbol
    as an ASCII character, otherwise it returns a space ' '.
*/
extern char sudoku_get_symbol( sudoku_cell_t *cell );

/** sudoku_enter_symbol
   @param[in] cntxt       The graphic/UI context passed back an forth between UI front
                          end and game backend.
   @param[in] symbol      The key ascii code of the symbol
   @remark                The 9 characters correspondings to symbols can be entered to
                          toggle that symbol in the current selection:
                          '1', '2', '3', '4', '5', '6', '7', '8', '9'.
   @remark                This will endup redrawing the game.

   Non-symbol keys should not be passed to the function. Instead, they should be processed as follows:

   - Backspace and delete keys should be processed by the user interface as
   similar to activating the menu item 'erase', that is by calling the 
   function @ref sudoku_erase_selection.

   - Special movement keys like @ref SUDOKU_UP_ARROW, @ref SUDOKU_DOWN_ARROW, @ref SUDOKU_LEFT_ARROW,
    @ref SUDOKU_RIGHT_ARROW, @ref SUDOKU_PAGE_UP, @ref SUDOKU_PAGE_DOWN, @ref SUDOKU_HOME_KEY or
    @ref SUDOKU_END_KEY should be processed by calling @ref sudoku_move_selection.

   - Finally, shortcuts to menus are supposed to be handled directly by the user
   interface, so that various langages can be used, and result in calling the
   function corresponding to the same menu items:

   For instance in english the following characters may be used to invoke the
   following functions (also accessible through menus):

   's', 'S' for file/Save should call @ref sudoku_save_file
   'x', 'X', 'q', 'Q' for file/Quit can simply exit the program, after confimation.

   The shortcuts below can be very useful and should probably be implemented for a
   good user experience:

   'u', 'U' for Undoing an action should call @ref sudoku_undo().
   'r', 'R' for Redoing undone action(s) should call @ref sudoku_redo()
   'r', 'R' for eRasing the current selection should call @ref sudoku_erase_selection.
   'm', 'M' for Marking the current game should call @ref sudoku_mark_state.
   'b', 'B' for backing off to the previous marked game should call @ref sudoku_back_to_mark.
   'c', 'C' for Checking the game for possible solution should call @ref sudoku_check_from_current_position.
   'h', 'H' for Hinting at a possible input should call @ref sudoku_hint.
   'f', 'F' for Filling the selected square should call @ref sudoku_fill.
   'd', 'D' for Doing (solving) the game should call @ref sudoku_solve_from_current_position.

*/
extern void sudoku_enter_symbol( void *cntxt, int symbol );

/** @} */
#endif /* __SUDOKU_H__ */
