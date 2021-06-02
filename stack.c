
/* Undo/Redo stack manipulation for sudoku operations */

#include "stack.h"

/*
  undo/redo stack:

  In order to allow undoing and redoing multiple times, a stack of previous
  grids is managed.

  bottom_stack is the bottom of Stack.
  stack_pointer is the theoretical stack pointer.

  Note that stack_pointer and bottom_stack are always incrementing as if the 
  stack was infinite. The actual stack index is stack_pointer modulo the
  stack size.
*/
#define STACK_INDEX(_sp)  ((int)((_sp) % MAX_DEPTH))
/*
  stack is empty if ( stack_pointer == bottom_stack + 1 ) modulo N
  stack is full if ( stack_pointer == bottom_stack ) modulo N
*/
#define IS_STACK_EMPTY() ((stack_pointer % MAX_DEPTH) ==    \
                          ((bottom_stack+1) % MAX_DEPTH))
#define IS_STACK_FULL()  ((stack_pointer % MAX_DEPTH) == (bottom_stack % MAX_DEPTH))

/*
  initially stack is empty:

  bottom_stack = 0
  |stack_pointer = 1
  ||
  vv
  [------------------[
  0                  N = MAX_DEPTH

  The functioning condtions are that:
  - bottom_stack is always less than or equal to stack_pointer.
  - the stack is never full (bottom_stack == stack_pointer).

  Which can be summarized as bottom_stack < stack_pointer;
*/
static stack_pointer_t stack_pointer = 1, bottom_stack = 0;

#define STACK_IS_OK() (bottom_stack < stack_pointer)

/*
  If stack is full, then the bottom of stack bS is automatically incremented
  (modulo N) to make room for one more new state, and as a consequence the
  oldest state saved in stack is lost (that is if we do not cross the low
  water mark - see more below).

          sp   | wall protecting entries above low water mark
           |bs |
           ||  |
           vv  |
  [########-#########[
               ^
               |
        low water mark modulo N

   Low water mark makes sure we do not remove that value from the stack
   when wrapping around and removing old entries in case of overflow.
   If that situation occurs, the push operation fails and the game stops.
   With a large stack, this occurence is extremely unlikely.
 */
static stack_pointer_t low_water_mark = 0;

extern void set_low_water_mark( stack_pointer_t mark )
{
    if ( ( 0 == low_water_mark ) || ( low_water_mark > mark ) )
        low_water_mark = mark;
}

extern stack_pointer_t get_low_water_mark( void )
{
    return low_water_mark;
}

extern stack_index_t reset_stack( void )
{
    stack_pointer = 1;
    bottom_stack = 0;
    low_water_mark = 0;
    return STACK_INDEX(stack_pointer);
}

extern bool is_stack_empty( void )
{
    return IS_STACK_EMPTY();
}

/*
   push returns new stack index [0..N[ for storing states. It asserts in
   case of overflow . Note that overflow can only happen if a low water
   mark is specified. If the stack is properly sized this should never happen.
*/
extern stack_index_t push( void )
{
    SUDOKU_ASSERT( STACK_IS_OK() );

    stack_pointer = stack_pointer + 1;
    if ( IS_STACK_FULL() ) {
        SUDOKU_ASSERT( 0 == low_water_mark || low_water_mark > bottom_stack );
        bottom_stack ++;
    }
    SUDOKU_ASSERT( STACK_IS_OK() );
    return STACK_INDEX(stack_pointer);
}

/*
   pop returns previous stack index [0..N[ if it was in stack, or -1 in
   case of underflow
*/
extern stack_index_t pop( void )
{
    SUDOKU_ASSERT( STACK_IS_OK() );
    if ( IS_STACK_EMPTY() ) { return -1; }

    stack_pointer = stack_pointer - 1;
    SUDOKU_ASSERT( STACK_IS_OK() );
    return STACK_INDEX(stack_pointer);
}

/*
   pushn reserves n slots in the stack and moves the stack pointer.
   It returns the new stack index
*/
extern stack_index_t pushn( unsigned int nb )
{
    SUDOKU_ASSERT( STACK_IS_OK() );

    stack_pointer_t nsp = stack_pointer + nb;
    if ( STACK_INDEX( stack_pointer ) < STACK_INDEX( bottom_stack ) &&
         STACK_INDEX( nsp ) >= STACK_INDEX( bottom_stack ) ) {
        unsigned int nbs = nsp + 1;
        SUDOKU_ASSERT( 0 == low_water_mark || low_water_mark > nbs );
        bottom_stack = nbs;
    }
    stack_pointer = nsp;
    return STACK_INDEX(stack_pointer);
}

/*
   The bottom stack is never directly modified outside the stack management.
   Only stack_pointer can be changed or queried.
*/
extern stack_pointer_t get_sp( void )
{
    return STACK_INDEX(stack_pointer);
}

extern stack_index_t set_sp( stack_pointer_t sp )
{
    SUDOKU_ASSERT( ( sp > bottom_stack ) && 
                   ( 0 == low_water_mark || sp >= low_water_mark ) );
    stack_pointer = sp;
    return STACK_INDEX(stack_pointer);
}

extern stack_index_t get_current_stack_index( void )
{
    return STACK_INDEX(stack_pointer);
}

extern stack_index_t get_stack_index( stack_pointer_t sp )
{
    return STACK_INDEX(sp);
}
