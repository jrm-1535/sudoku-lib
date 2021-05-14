
#ifndef __GRDSTK_H__
#define __GRDSTK_H__

/*
    Sudoku grid storage in stack
*/
#include "stack.h"

// make the grid at index d empty
extern void empty_grid( stack_index_t d );

// replace current grid with the one pointed to by from
extern void copy_grid( stack_index_t d, stack_index_t s );

// copy grid s to d and fill empty cells in the copied grid with all candidates
extern void copy_fill_grid( stack_index_t d, stack_index_t s );


#endif /* __GRDSTK_H__ */
