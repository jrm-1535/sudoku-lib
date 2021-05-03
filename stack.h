/*
  Sudoku stack.h

  Internal undo stack manipulation for sudoku
*/

#ifndef __STACK_H__
#define __STACK_H__

#include "sudoku.h"

/*
   This is the undo/redo stack implementation.

   The actual stack is an array of games, with each game
   being an 9x9 array of cells (symbol map and state) plus
   the selection row and column and an array of NB_MARKS
   bookmarks.
*/
#define NB_MARKS     16
#define MAX_DEPTH    1000 /* 800 */
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
   address space). Stack pointers are an index in this infinite
   array. However the real stack is not infinite and wraps around
   when it reaches the MAX_DEPTH limit. Although the stack pointer
   keeps incrementing the current stack array index wraps around.
   The function get_sp returns the theoretical stack pointer not
   the actual physical array index.
*/
extern int get_sp( void );

/* Because the undo stack is limited, there is a possibility that
   undo evetually will not work. low water mark is a special mark
   in the stack that guarantee it is always possible to undo from
   the top of stack to that mark. The mark value is a theoretical
   stack pointer.

   It is also possible to quickly go back to the low water mark
   by calling return_to_low_water_mark.
*/
extern void set_low_water_mark( int mark );
extern int  return_to_low_water_mark( int mark );

/* The following functions return the current stack index,
   which can be directly used to access the current game. */

extern int reset_stack( void );
extern int push( void );
extern int pushn( int nb );
extern int pop( void );
extern int set_sp( int sp );

/* helper functions returning a stack index from a stack pointer */
extern int get_stack_index( int sp );
extern int get_current_stack_index( void );

#endif /* __STACK_H__ */
