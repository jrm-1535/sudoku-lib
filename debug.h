/*
  Sudoku debug settings
*/

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <assert.h>
#include <stdio.h>

#ifndef SUDOKU_INTERFACE_DEBUG
#define SUDOKU_INTERFACE_DEBUG  1   /**< Turn on interface debugging */
#endif

#ifndef SUDOKU_SOLVE_DEBUG
#define SUDOKU_SOLVE_DEBUG      0   /**< Turn on solver debugging */
#endif

#ifndef SUDOKU_HINT_DEBUG
#define SUDOKU_HINT_DEBUG       0   /**< Turn on hint debugging */
#endif

#ifndef SUDOKU_FILE_DEBUG
#define SUDOKU_FILE_DEBUG       0   /**< Turn on file (save/load) debugging */
#endif

// TODO: cleanup those PRINT macros
#define SUDOKU_SPECIAL_DEBUG   0    /**< Turn on internal grid debugging */
#define SUDOKU_PRETTY_PRINT    1    /**< Turn on internal grid print */

#define STMT( _s )   do { _s; } while(0)   /**< used in trace macro */

/** simple trace macro - may be enriched if needed */
#ifdef DEBUG
#define SUDOKU_TRACE( _l, _args )  STMT( if (_l) { printf _args; } )
#else
#define SUDOKU_TRACE( _l, _args )
#endif

/** Simple assert macro - may be enriched if needed */
#ifdef DEBUG
#define SUDOKU_ASSERT( _a )  assert( _a )
#else
#define SUDOKU_ASSERT( _a )
#endif

#define SUDOKU_SOLVE_TRACE( _args ) SUDOKU_TRACE( SUDOKU_SOLVE_DEBUG, _args )
#define SUDOKU_HINT_TRACE( _args )  SUDOKU_TRACE( SUDOKU_HINT_DEBUG, _args )

#endif /* __DEBUG_H__ */
