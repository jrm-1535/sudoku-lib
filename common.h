/*
  Common sudoku definitions
*/

#ifndef __COMMON_H__
#define __COMMON_H__

/** path separator for different systems */
#ifndef DOS_STYLE_SEPARATOR
#define PATH_SEPARATOR        '/'
#else
#define PATH_SEPARATOR        '\\'
#endif

#define MIN_SUDOKU_GAME_NUMBER 1     /**< Min game number in random selection */
#define MAX_SUDOKU_GAME_NUMBER 10000 /**< Max game number in random selection */

#ifndef SUDOKU_GRAPHIC_DEBUG
#define SUDOKU_GRAPHIC_DEBUG   0    /**< Turn on graphic debugging */
#endif

#ifndef SUDOKU_UI_DEBUG
#define SUDOKU_UI_DEBUG        0     /**< Turn on ui debugging */
#endif

#ifndef SUDOKU_SOLVE_DEBUG
#define SUDOKU_SOLVE_DEBUG     0    /**< Turn on solver debugging */
#endif

#ifndef SUDOKU_FILE_DEBUG
#define SUDOKU_FILE_DEBUG      0    /**< Turn on file (save/load) debugging */
#endif

#define SUDOKU_SPECIAL_DEBUG   0   /**< Turn on internal grid debugging */
#define SUDOKU_PRETTY_PRINT    1   /**< Turn on internal grid print */

#define STMT( _s )   do { _s; } while(0)   /**< used in trace macro */

/** Simple assert macro - may be enriched if needed */
#ifdef DEBUG
#define SUDOKU_ASSERT( _a )  assert( _a )
#else
#define SUDOKU_ASSERT( _a )
#endif

/** simple trace macro - may be enriched if needed */
#ifdef DEBUG
#define SUDOKU_TRACE( _l, _args )  STMT( if (_l) { printf _args; } )
#else
#define SUDOKU_TRACE( _l, _args )
#endif

/** Default window name when no game is on going */
#define SUDOKU_DEFAULT_WINDOW_NAME "Sudoku"

#define SUDOKU_SUCCESS 0    /**< For functions returning success */
#define SUDOKU_FAILURE (-1) /**< For functions returning some failure */
/*
  The sudoku game can be seen as a 2 dimension array ( 9 rows * 9 columns )
  of cells. Each cell contains possible symbols (from 0 up to 9), each
  symbol being a number from 0 to 8.

  The array of cells is called here a grid. 
  Each cell belongs to one row, one column and one box:

    | 0 | 1 | 2| 3| 4 | 5 | 6 | 7 | 8 |
  --|---------------------------------|
  0 |          |          |           |
  1 |   box 0  |   box 1  |   box 2   |
  2 |          |          |           |
  -------------------------------------
  3 |          |          |           |
  4 |   box 3  |   box 4  |   box 5   |
  5 |          |          |           |
  -------------------------------------
  6 |          |          |           |
  7 |   box 6  |   box 7  |   box 8   |
  8 |          |          |           |
  -------------------------------------
*/

#define SUDOKU_N_BOXES 9
/*
#define N_CELLS_PER_ROW NB_SYMBOLS
#define N_CELLS_PER_COL NB_SYMBOLS
#define N_CELLS_PER_BOX SUDOKU_N_SYMBOLS
*/
#endif /* __COMMON_H__ */
