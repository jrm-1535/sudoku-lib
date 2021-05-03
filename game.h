/*
  sudoku game.h

  Suduku game: game representation related declarations
*/

#ifndef __GAME_H__
#define __GAME_H__

#include "sudoku.h"

extern sudoku_ui_table_t sudoku_ui_fcts;

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

/* exported to file functions */
extern void set_game_time( unsigned long duration );
extern unsigned long get_game_duration( void );

#endif /* __GAME_H__ */
