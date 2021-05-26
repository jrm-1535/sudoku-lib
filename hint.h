/*
  sudoku hint.h

  Suduku game: hint related declarations
*/

#ifndef __HINT_H__
#define __HINT_H__

#include "sudoku.h"
#include "game.h"

typedef enum {
    NONE, SET, REMOVE, ADD
} hint_action_t;

typedef struct {
    sudoku_hint_type    hint_type;
    int                 n_hints;            // where symbols can be set or removed
    int                 n_triggers;         // where other symbols trigger hints/candidates
    int                 n_candidates;       // where symbol could be placed (locked candidates)

    bool                hint_pencil;        // whether to show penciled symbols in hint cells
    
    cell_ref_t          candidates[ SUDOKU_N_SYMBOLS ];     // by default with pencils

    cell_ref_t          hints[ SUDOKU_N_ROWS*2 + SUDOKU_N_COLS*2 + SUDOKU_N_BOXES*2 ];

    cell_ref_t          triggers[ SUDOKU_N_ROWS*2 + SUDOKU_N_COLS*2 + SUDOKU_N_BOXES*2 ];
    cell_attrb_t        flavors[ SUDOKU_N_ROWS*2 + SUDOKU_N_COLS*2 + SUDOKU_N_BOXES*2 ];

    cell_ref_t          selection;          // -1,-1 if no selection

    hint_action_t       action;
    int                 n_symbols;
    int                 symbol_map;
} hint_desc_t;

extern bool get_hint( hint_desc_t *hdp );
extern bool act_on_hint( hint_desc_t *hdesc );

// return 0 if could not step, 1 if it did a step, 2 if the game is solved
extern int solve_step( void );
extern sudoku_hint_type find_hint( int *row_hint, int *col_hint );

#endif /* __HINT_H__ */
