
/* Undo/Redo stack manipulation for sudoku operations */

#include "common.h"
#include "stack.h"

/*
  undo/redo stack:

  In order to allow undoing and redoing multiple times, a stack of previous
  grid states is managed.

  bottom_stack is the bottom of Stack.
  stack_pointer is the theoretical stack pointer.

  Note that stack_pointer and bottom_stack are always incrementing as if the 
  stack was infinite. The actual stack index is stack_pointer modulo the
  stack size.
*/
#define index( _sp ) ( stack_pointer % MAX_DEPTH )

/*
  stack is empty if ( stack_pointer == bottom_stack + 1 ) modulo N
  stack is full if ( stack_pointer == bottom_stack ) modulo N
*/
#define IS_STACK_EMPTY() ((stack_pointer % MAX_DEPTH) ==    \
                          ((bottom_stack+1) % MAX_DEPTH))
#define IS_STACK_FULL()  ((stack_pointer % MAX_DEPTH) == (bottom_stack % MAX_DEPTH))

/*
  If stack is full, then the bottom of stack bS is automatically incremented
  to make room for one more new state, and as a consequence the oldest state
  saved in stack is lost (that is if we do not cross the low water mark - see
  more below).

  initially stack is empty:

  bottom_stack = 0
  |stack_ptr = 1
  ||
  vv
  [------------------[
  0                  N = MAX_DEPTH

  The functioning condtions are that:
  - bottom_stack is always less than or equal to stack_pointer.
  - the stack is never full (bottom_stack == stack_pointer).

  Which can be summarized as bottom_stack < stack_pointer;
*/
static int stack_pointer = 1, bottom_stack = 0;

#define STACK_IS_OK() (bottom_stack < stack_pointer)

/*
   Low water mark makes sure we do not remove that value from the stack
   when wrapping around and removing old entries in case of overflow.
   If that situation occurs, the push operation fails and the game stops.
   With a large stack, this occurence is extremely unlikely.
 */
static int low_water_mark = -1;

void set_low_water_mark( int mark )
{
    if ( ( -1 == low_water_mark ) || ( low_water_mark > mark ) )
        low_water_mark = mark;
}

int return_to_low_water_mark( int mark )
{
    SUDOKU_ASSERT ( -1 != low_water_mark );

    if ( low_water_mark == mark ) {
        low_water_mark = -1;
    }
    stack_pointer = mark;
    return stack_pointer % MAX_DEPTH;
}

int reset_stack( void )
{
    stack_pointer = 1;
    bottom_stack = 0;
    low_water_mark = -1;
    return stack_pointer % MAX_DEPTH;
}

bool is_stack_empty( void )
{
    return IS_STACK_EMPTY();
}

/*
   push returns new stack index [0..N[ for storing states and operating
   or -1 in case of overflow. Note that overflow can only happen if a
   low water mark is specified. If the stack is properly sized this should
   never happen.
*/
int push( void )
{
    SUDOKU_ASSERT( STACK_IS_OK() );

    stack_pointer = stack_pointer + 1;
    if ( IS_STACK_FULL() ) {
        if ( ( -1 != low_water_mark ) && ( low_water_mark <= bottom_stack ) ) {
            return -1;
        }
        bottom_stack ++;
    }
    SUDOKU_ASSERT( STACK_IS_OK() );
    return stack_pointer % MAX_DEPTH;
}

/*
   pop returns previous stack index [0..N[ if it was in stack, or -1 in
   case of underflow
*/
int pop( void )
{
    SUDOKU_ASSERT( STACK_IS_OK() );
    if ( IS_STACK_EMPTY() ) { return -1; }

    stack_pointer = stack_pointer - 1;
    SUDOKU_ASSERT( STACK_IS_OK() );
    return stack_pointer % MAX_DEPTH;
}

/*
   pushn calls n times push to reserve n slots in the stack.
*/
int pushn( int nb )
{
    for ( int i = 0; i < nb ; i++ ) {
        if ( -1 == push () ) {
            while ( i-- ) pop ();  /* failed, undo pushn */
            return -1;
        }
    }
    return stack_pointer % MAX_DEPTH;
}

/*
   The bottom stack is never directly modified outside the stack management.
   Only stack_pointer can be changed or queried.
*/

int get_sp( void )
{
    return stack_pointer;
}

int set_sp( int sp )
{
    SUDOKU_ASSERT( ( sp > bottom_stack ) && 
                   ( -1 == low_water_mark || sp >= low_water_mark ) );
    stack_pointer = sp;
    return stack_pointer % MAX_DEPTH;
}

int get_current_stack_index( void )
{
    return stack_pointer % MAX_DEPTH;
}

int get_stack_index( int sp )
{
    if ( -1 != sp ) {
        return sp % MAX_DEPTH;
    }
    return -1;
}
