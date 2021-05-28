/*
  Sudoku stack.h

  Internal undo stack manipulation for sudoku
*/

#ifndef __STACK_H__
#define __STACK_H__

#include <stdbool.h>
#include "debug.h"

/*
   This is the undo/redo stack implementation.

   The actual stack is an array of games, with each game
   being an 9x9 array of cells (symbol map and state) plus
   the selection row and column and an array of NB_MARKS
   bookmarks.
*/
#define NB_MARKS     16
#define MAX_DEPTH    1000
/*
   The actual stack size depends on the CPU architecture
   (32 or 64 bits) and the compiler. At worst, a cell would
   take 24 bytes, a selection 16 bytes and a bookmark 8
   bytes. With 16 bookmarks an entry would require:
    9*9*24 + 16 + 16*8 = 1944 + 16 + 128 = 2088 bytes.
   At worst a stack of 1000 entries would then require:
    2088*1000 = 2MB.

   For a 32 bit architecture the entry would require only
    9*9*12 + 8 + 16*4 = 972 + 8 + 64 = 1044 bytes
   So the whole stack would take only 1MB.
*/

extern bool is_stack_empty( void );

/* The stack is seen as an infinite array (only limited by the
   address space). Stack pointers are indexes in this infinite
   array. However the real stack is finite and although the stack
   pointer keeps incrementing, real indexes wrap around when they
   reach MAX_DEPTH limit.

   The function get_sp returns the theoretical stack pointer not
   the actual physical array index. */

typedef unsigned int stack_pointer_t;
extern stack_pointer_t get_sp( void );

/* It is possible to go back to a theoretical stack pointer that
   was previously returned by get_sp() by calling set_sp(sp).
   All games that were pushed on the atack are dismissed and the
   current game is back to the game was current at the time
   get_sp() was called.

   The returned value is the real stack index. */
typedef int stack_index_t;
extern stack_index_t set_sp( stack_pointer_t sp );

/* Because the undo stack is limited, there is a possibility that
   undo cannot work. A special mark in the stack, low water mark
   guarantees it is always possible to undo from the top of stack
   to that mark. The mark value is a theoretical stack pointer.

   It is also possible to quickly go back to the low water mark,
   dismissing all previously stacked operations, by calling
   return_to_low_water_mark.
*/
extern void set_low_water_mark( stack_pointer_t mark );
extern stack_pointer_t get_low_water_mark( void );
extern void return_to_low_water_mark( stack_pointer_t mark );

/* The following functions return the current real stack index,
   which can be directly used to access the current game. */
extern stack_index_t reset_stack( void );
extern stack_index_t push( void );
extern stack_index_t pushn( unsigned int nb );
extern stack_index_t pop( void );

/* helper function returning a stack index from a theoretical
   stack pointer */
extern stack_index_t get_stack_index( stack_pointer_t sp );
extern stack_index_t get_current_stack_index( void );

#endif /* __STACK_H__ */
