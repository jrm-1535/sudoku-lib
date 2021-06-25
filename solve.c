/*
  Sudoku solver and generator
*/
#include <stdlib.h>
#include <string.h>
#include <time.h>   /* for performance measurements */

#include "sudoku.h"
//#include "stack.h"
#include "grdstk.h"
#include "grid.h"
#include "solve.h"
#include "hint.h"
#include "rand.h"

#define DLX_DEBUG 0

/*
    Implements Knuth's DLX (Dancing Links XCOVER algorithm for SUDOKU
*/

/* One node represents a specific constraint for each candidate.
   Each node in in two double linked circular lists,
    - one for multiple constraints to satisfy,
    - one for each candidates to examine */
typedef struct _node {
    struct _node    *up, *down,     // up down in node list attached to header (see below)
                    *left, *right;  // left, right in an entry node list
    struct _header  *header;        // quick link back to constraint header
} node_t;

/* One header represents a constraint, and points to the first node of that constraint.
   The number of nodes in the list is n_items, the root node included in the header does
   not count: it just avoids special cases when handling the first and last node. The
   number of items allows quick search of the constraint with the shortest number of
   nodes (O(n_constraints) instead of O(n_constraints * n_nodes)). */
typedef struct _header {
    struct _node    root;           // embedded root node (header.root->down is first)
    char            *name;          // this is for debugging purpose.
    int             n_items;        // number of nodes attached to root
    struct _header  *left, *right;  // header list
} header_t;

/* For sudoku, the constraints are:
    - for each cell, 1 symbol exactly (SUDOKU_N_ROWS*SUDOKU_N_COLS headers)
    - for each row, 1 instance of each symbol exactly (SUDOKU_N_ROWS*SUDOKU_N_SYMBOLS headers)
    - for each col, 1 instance of each symbol exactly (SUDOKU_N_COLS*SUDOKU_N_SYMBOLS headers)
    - for each box, 1 instance of each symbol exactly (SUDOKU_N_BOXES*SUDOKU_N_SYMBOLS headers)
*/
#define     N_CELL_CONSTRAINTS (SUDOKU_N_ROWS*SUDOKU_N_COLS)
#define     N_ROW_CONSTRAINTS  (SUDOKU_N_ROWS*SUDOKU_N_SYMBOLS)
#define     N_COL_CONSTRAINTS  (SUDOKU_N_COLS*SUDOKU_N_SYMBOLS)
#define     N_BOX_CONSTRAINTS  (SUDOKU_N_BOXES*SUDOKU_N_SYMBOLS)

#define     N_CONSTRAINTS   (N_CELL_CONSTRAINTS+N_ROW_CONSTRAINTS+N_COL_CONSTRAINTS+N_BOX_CONSTRAINTS)
header_t    constraints[N_CONSTRAINTS]; // static allocation for 324 headers

// constraint header locations
#define FIRST_CELL_HEADER           0
#define FIRST_ROW_SYMBOL_HEADER     (FIRST_CELL_HEADER+N_CELL_CONSTRAINTS)
#define FIRST_COL_SYMBOL_HEADER     (FIRST_ROW_SYMBOL_HEADER+N_ROW_CONSTRAINTS)
#define FIRST_BOX_SYMBOL_HEADER     (FIRST_COL_SYMBOL_HEADER+N_COL_CONSTRAINTS)

// the first header is pointed to by root
header_t root;

char *names[N_CONSTRAINTS] = {
    "r0c0", "r0c1", "r0c2", "r0c3", "r0c4", "r0c5", "r0c6", "r0c7", "r0c8",
    "r1c0", "r1c1", "r1c2", "r1c3", "r1c4", "r1c5", "r1c6", "r1c7", "r1c8",
    "r2c0", "r2c1", "r2c2", "r2c3", "r2c4", "r2c5", "r2c6", "r2c7", "r2c8",
    "r3c0", "r3c1", "r3c2", "r3c3", "r3c4", "r3c5", "r3c6", "r3c7", "r3c8",
    "r4c0", "r4c1", "r4c2", "r4c3", "r4c4", "r4c5", "r4c6", "r4c7", "r4c8",
    "r5c0", "r5c1", "r5c2", "r5c3", "r5c4", "r5c5", "r5c6", "r5c7", "r5c8",
    "r6c0", "r6c1", "r6c2", "r6c3", "r6c4", "r6c5", "r6c6", "r6c7", "r6c8",
    "r7c0", "r7c1", "r7c2", "r7c3", "r7c4", "r7c5", "r7c6", "r7c7", "r7c8",
    "r8c0", "r8c1", "r8c2", "r8c3", "r8c4", "r8c5", "r8c6", "r8c7", "r8c8",

    "r0s1", "r0s2", "r0s3", "r0s4", "r0s5", "r0s6", "r0s7", "r0s8", "r0s9",
    "r1s1", "r1s2", "r1s3", "r1s4", "r1s5", "r1s6", "r1s7", "r1s8", "r1s9",
    "r2s1", "r2s2", "r2s3", "r2s4", "r2s5", "r2s6", "r2s7", "r2s8", "r2s9",
    "r3s1", "r3s2", "r3s3", "r3s4", "r3s5", "r3s6", "r3s7", "r3s8", "r3s9",
    "r4s1", "r4s2", "r4s3", "r4s4", "r4s5", "r4s6", "r4s7", "r4s8", "r4s9",
    "r5s1", "r5s2", "r5s3", "r5s4", "r5s5", "r5s6", "r5s7", "r5s8", "r5s9",
    "r6s1", "r6s2", "r6s3", "r6s4", "r6s5", "r6s6", "r6s7", "r6s8", "r6s9",
    "r7s1", "r7s2", "r7s3", "r7s4", "r7s5", "r7s6", "r7s7", "r7s8", "r7s9",
    "r8s1", "r8s2", "r8s3", "r8s4", "r8s5", "r8s6", "r8s7", "r8s8", "r8s9",

    "c0s1", "c0s2", "c0s3", "c0s4", "c0s5", "c0s6", "c0s7", "c0s8", "c0s9",
    "c1s1", "c1s2", "c1s3", "c1s4", "c1s5", "c1s6", "c1s7", "c1s8", "c1s9",
    "c2s1", "c2s2", "c2s3", "c2s4", "c2s5", "c2s6", "c2s7", "c2s8", "c2s9",
    "c3s1", "c3s2", "c3s3", "c3s4", "c3s5", "c3s6", "c3s7", "c3s8", "c3s9",
    "c4s1", "c4s2", "c4s3", "c4s4", "c4s5", "c4s6", "c4s7", "c4s8", "c4s9",
    "c5s1", "c5s2", "c5s3", "c5s4", "c5s5", "c5s6", "c5s7", "c5s8", "c5s9",
    "c6s1", "c6s2", "c6s3", "c6s4", "c6s5", "c6s6", "c6s7", "c6s8", "c6s9",
    "c7s1", "c7s2", "c7s3", "c7s4", "c7s5", "c7s6", "c7s7", "c7s8", "c7s9",
    "c8s1", "c8s2", "c8s3", "c8s4", "c8s5", "c8s6", "c8s7", "c8s8", "c8s9",

    "b0s1", "b0s2", "b0s3", "b0s4", "b0s5", "b0s6", "b0s7", "b0s8", "b0s9",
    "b1s1", "b1s2", "b1s3", "b1s4", "b1s5", "b1s6", "b1s7", "b1s8", "b1s9",
    "b2s1", "b2s2", "b2s3", "b2s4", "b2s5", "b2s6", "b2s7", "b2s8", "b2s9",
    "b3s1", "b3s2", "b3s3", "b3s4", "b3s5", "b3s6", "b3s7", "b3s8", "b3s9",
    "b4s1", "b4s2", "b4s3", "b4s4", "b4s5", "b4s6", "b4s7", "b4s8", "b4s9",
    "b5s1", "b5s2", "b5s3", "b5s4", "b5s5", "b5s6", "b5s7", "b5s8", "b5s9",
    "b6s1", "b6s2", "b6s3", "b6s4", "b6s5", "b6s6", "b6s7", "b6s8", "b6s9",
    "b7s1", "b7s2", "b7s3", "b7s4", "b7s5", "b7s6", "b7s7", "b7s8", "b7s9",
    "b8s1", "b8s2", "b8s3", "b8s4", "b8s5", "b8s6", "b8s7", "b8s8", "b8s9"
};

/*
    Those headers define how nodes are woven in the constraint matrix.

    The matrix is seen as a table with contraints as columms and a specific
    symbol in a specific sudoku grid row and a specific sudoku grid column
    as a row. To avoid confusion between constraint matrix rows and columns
    and sudoku grid rows and columns, the matrix rows are called horizontal
    lists and matrix columns are called vertical lists. Here we use rows
    and colums only for sudoku grid rows and columns, whereas we use headers
    to identify contraint columns, and entries to identify a matrix row, a
    combination of sudoku row, column and symbol. Entries are listed in
    sudoku row, column and symbol order, as if they were entries in an
    array[rows][columns][symbols] (symbols varying faster than columns, and
    columns faster than rows):

    Entry #000 corresponds to sudoku row 0, col 0 with symbol 1
           ...
    Entry #008 corresponds to sudoku row 0, col 0 with symbol 9
    Entry #009 corresponds to sudoku row 0, col 1 with symbol 1
           ...
           ...
    Entry #080 corresponds to sudoku row 0, col 8 with symbol 9
    Entry #081 corresponds to sudoku row 1, col 0 with symbol 1
           ...
           ...
           ...
    Entry #728 corresponds to sudoku row 8, col 8 with symbol 9 

    The matrix is actually sparse, with many nodes missing in each matrix
    headers and entries: each entry is linked to only 4 constraints among
    324, and each header is linked to only 9 nodes (9*9*9=729 entries,
    9*9*4=324 headers, that's a (9*9*9)*(9*9*4)=236196 element matrix,
    whereas only 729*4=2916 (or 324*9=2916) nodes are needed.

    For example entry #009 (r0c1s1) is a list of 4 nodes, the first one is
    the first node in the r0c1 header list, the second one is the second node
    in the r0s1 header list (the first one is used for r0c0s1), the third one
    is the first node in the c1s1 header list and the last one is the second
    node in the b0s1 header list (the first one is used for r0c0s1).

    The headers bnsp correspond to the box contraints, boxes being labeled
    from 0 to 8, box 0 being the intersection of the first 3 rows and 3 cols,
    box 1 the intersection of the first 3 rows and the next 3 cols, etc.
*/

#define N_ENTRIES (9*9*9)

/* To identify a particular symbol in a particular cell, Sudoku
   requires 9*9*9 == 729 entries, associated with 9*9*9*4 = 2916
   nodes.

    Example:
        headers    r0c0 r0c1 ... r0s1 r0s2 ... c0s1 c0s2 .. c1s1 c1s2 ... b0s1 b0s2 ...
    entries         |    |        |    |        |    |       |    |        |    |
    #000/r0c0s1 -->[]----------->[]----------->[]------------------------>[]    |
    #000/r0c0s2 -->[]---------------->[]----------->[]------------------------->[]
    ............    |    |        |    |        |    |       |    |        |    |
    #009/r0c1s1 ------->[]------>[]------------------------>[]----------->[]    |
    #010/r0c1s2 ------->[]----------->[]------------------------->[]----------->[]

    for each entry #xxx, 
        (xxx % 9) is the symbol value -1
        (xxx / 9) is the cell # (r, c)
        ((xxx / 9) % 9) is the col # (c)
        (xxx / 81) is the row # (r)
        (r / 3) * 3 + c / 3 is the box # (b) */

#define SYMBOL(_entry)  (_entry % SUDOKU_N_SYMBOLS)
#define CELL(_entry)    (_entry / SUDOKU_N_SYMBOLS)
#define ROW(_entry)     (_entry / (SUDOKU_N_SYMBOLS * SUDOKU_N_COLS))
#define COL(_entry)     ((_entry / SUDOKU_N_SYMBOLS) % SUDOKU_N_COLS)
#define BOX(_entry)     ((_entry / (SUDOKU_N_SYMBOLS * SUDOKU_N_COLS * 3 )) * 3 +\
                         ((_entry / SUDOKU_N_SYMBOLS) % SUDOKU_N_COLS) / 3)

/* To link all nodes corresponding to each entry,
        find header by cell: header(cell) = headers[FIRST_CELL_HEADER+cell]
        find header by row, symbol: header(row, symbol) = constraints[FIRST_ROW_SYMBOL_HEADER+row*9+symbol]
        find header by col, symbol: header(col, symbol) = constraints[FIRST_COL_SYMBOL_HEADER+col*9+symbol]
        find header by box, symbol: header(box, symbol) = constraints[FIRST_BOX_SYMBOL_HEADER+box*9+symvol] */

#define CELL_HEADER(_cell)              constraints[FIRST_CELL_HEADER+(_cell)]
#define ROW_SYMBOL_HEADER(_row,_symbol) constraints[FIRST_ROW_SYMBOL_HEADER+(_row)*9+(_symbol)]
#define COL_SYMBOL_HEADER(_col,_symbol) constraints[FIRST_COL_SYMBOL_HEADER+(_col)*9+(_symbol)]
#define BOX_SYMBOL_HEADER(_box,_symbol) constraints[FIRST_BOX_SYMBOL_HEADER+(_box)*9+(_symbol)]

#define N_NODES     (N_CONSTRAINTS * SUDOKU_N_SYMBOLS)
node_t nodes[N_NODES];                  // all links are initially NULL

typedef struct {
    int row, col, symbol;
} rcs_t;

static void get_rcs_from_node( node_t *node, rcs_t *rcs )
{
    int hn = node->header - constraints, in = (node - nodes) % 9;

    if ( hn < FIRST_ROW_SYMBOL_HEADER ) {
        rcs->row = hn / 9;
        rcs->col = hn % 9; 
        rcs->symbol = in;
    } else if ( hn < FIRST_COL_SYMBOL_HEADER ) {
        int rsh = hn - FIRST_ROW_SYMBOL_HEADER;
        rcs->row = rsh / 9;
        rcs->col = in;
        rcs->symbol = rsh % 9;
    } else if ( hn < FIRST_BOX_SYMBOL_HEADER ) {
        int csh = hn - FIRST_COL_SYMBOL_HEADER;
        rcs->col = csh / 9;
        rcs->row = in;
        rcs->symbol = csh % 9;
    } else {
        int bsh = hn - FIRST_BOX_SYMBOL_HEADER;
        int box = bsh / 9;
        // order is box #b r=(b/3) *3 c=b%3
        rcs->row = (box / 3 ) * 3 + in / 3;
        rcs->col = (box % 3 ) * 3 + in % 3;
        rcs->symbol = bsh % 9;
    }
}

#if DLX_DEBUG
static char *get_entry_name( node_t *node )
{
    // rebuilding the entry number from each header # and item #
    rcs_t  rcs;
    get_rcs_from_node( node, &rcs );

    static char buffer[] = "r0c0s1";
    buffer[1] = '0' + rcs.row;
    buffer[3] = '0' + rcs.col;
    buffer[5] = '1' + rcs.symbol;
    return buffer;
}

static void print_constraint( long int which )
{
    assert( which >= 0 && which < N_CONSTRAINTS );
    printf("Constraint %03ld: %s <- %s -> %s, %d nodes [",
            which, constraints[which].left->name,
            constraints[which].name, constraints[which].right->name,
            constraints[which].n_items );

    for ( node_t *node = constraints[which].root.down; node != &constraints[which].root;
                                                                         node = node->down ) {
        printf( " ^%s", get_entry_name( node ) );
    }
    printf( " ]\n" );
}

static void print_entry( node_t *entry )
{
    node_t *first = entry;

    while ( first->header >= &constraints[ FIRST_ROW_SYMBOL_HEADER ] ) {
        first = first->left;
    }

    printf( "In entry: " );
    node_t *node = first;
    do {
        if ( node == entry ) {
            printf( " < %s >", node->header->name );
        } else {
            printf(" %s", node->header->name);
        }
        node = node->right;
    } while ( node != first );

    int k = 1;
    for( node = entry->header->root.down; node != entry; node = node->down ) ++k;
    printf(" node #%d of %d\n", k, entry->header->n_items );
}

static void print_all_constraints( void )
{
    for ( header_t *header = root.right; header != &root; header = header->right ) {
        print_constraint( header - constraints );
    }
}
#endif

static void sudoku_set_dlx_header_list( void )
{
    memset( nodes, 0, sizeof(nodes) );
    node_t *node = nodes;                       // nodes are allocated from the array

    for ( int i = 0; i < N_CONSTRAINTS; ++i ) { // first, constraint headers
        constraints[i].name = names[i];
        constraints[i].n_items = SUDOKU_N_SYMBOLS;

        constraints[i].root.header = &constraints[i];
        constraints[i].root.down = node;        // first in node list

        node->header = &constraints[i];
        node->up = &constraints[i].root;
        node->down = node + 1;

        for ( int j = 0; j < SUDOKU_N_SYMBOLS-1; ++j ) {    // following 8 nodes point to each other
            ++node;
            node->header = &constraints[i];
            node->up = node - 1;
            node->down = node + 1;
        }   // header.first <-> node 0 <-> node 1 <-> ... <-> node 7 <-> &header.first
        node->down = &constraints[i].root;    // fix & close circular link
        constraints[i].root.up = node;

        if ( i > 0 ) {
            constraints[i-1].right = &constraints[i];
            constraints[i].left = &constraints[i-1];
        }
        ++node;                                 // ready for next header
    }
    root.right = constraints;
    constraints[0].left = &root;

    root.left = &constraints[N_CONSTRAINTS-1];
    constraints[N_CONSTRAINTS-1].right = &root;
    root.name = "root";
}

static void sudoku_set_dlx_entry_lists( void )
{
    for ( int i = 0; i < N_ENTRIES; ++i ) {
        int symbol = SYMBOL(i);
        int cell = CELL(i);
        int row = ROW(i);
        int col = COL(i);
        int box = BOX(i);

        header_t *header = &CELL_HEADER(cell);
        node_t *cell_node;

//        printf("#%03d", i);
//        int n = 0;  // debug only
        for ( cell_node = header->root.down; ; cell_node = cell_node->down ) {
            if ( NULL == cell_node->right ) {
//                printf(" in %s#%d", header->name, n);
                break;
            }
//            ++n;
            assert ( cell_node->down != &header->root );
        }
        header = &ROW_SYMBOL_HEADER(row, symbol);
        node_t *row_node;
//        n = 0;  // debug only
        for ( row_node = header->root.down; ; row_node = row_node->down ) {
            if ( NULL == row_node->right ) {
//                printf(" in %s#%d", header->name, n);
                break;
            }
//            ++n;
            assert ( row_node->down != &header->root );
        }
        row_node->left = cell_node;
        cell_node->right = row_node;

        header = &COL_SYMBOL_HEADER(col, symbol);
        node_t *col_node;
//        n = 0;  // debug only
        for ( col_node = header->root.down; ; col_node = col_node->down ) {
            if ( NULL == col_node->right ) {
//                printf(" in %s#%d", header->name, n);
                break;
            }
//            ++n;
            assert ( col_node->down != &header->root );
        }
        col_node->left = row_node;
        row_node->right = col_node;

        header = &BOX_SYMBOL_HEADER(box, symbol);
        node_t *box_node;
//        n = 0;  // debug only
        for ( box_node = header->root.down; ; box_node = box_node->down ) {
            if ( NULL == box_node->right ) {
//                printf(" in %s#%d\n", header->name, n);
                break;
            }
//            ++n;
            assert ( box_node->down != &header->root );
        }
        box_node->left = col_node;
        col_node->right = box_node;

        cell_node->left = box_node;
        box_node->right = cell_node;
    }
}

static void cover( header_t *header )
{
    header_t *left = header->left, *right = header->right;
    left->right = right;
    right->left = left;

    // cover nodes from top to bottom, right to left.
    for ( node_t *node = header->root.down; node != &header->root ; node = node->down ) {
        for ( node_t *next = node->right; next != node; next = next->right ) {
            assert( next->header->n_items > 0 );
            node_t *up = next->up, *down = next->down;
            up->down = down;
            down->up = up;
            --next->header->n_items;
        }
    }
}

static void uncover( header_t *header )
{
    /* To take advantage of the dangling pointers still in the covered nodes,
       uncover in exact reverse order. i.e. bottom to top, left to rignt. */
    for ( node_t *node = header->root.up; node != &header->root; node = node->up ) {
        for ( node_t *prev = node->left; prev != node; prev = prev->left ) {
            node_t *up = prev->up;
            node_t *down = prev->down;
            up->down = down->up = prev;
            ++prev->header->n_items;
        }
    }
    header_t *left = header->left, *right = header->right;
    left->right = right->left = header;
}

static bool set_given( int row, int col, int symbol )
{
//printf( "set_given: row %d col %d symbol %d\n", row, col, 1 + symbol );

    header_t *rc_header = &CELL_HEADER( row * SUDOKU_N_COLS + col ),
             *rs_header = &ROW_SYMBOL_HEADER( row, symbol ),
             *cs_header = &COL_SYMBOL_HEADER( col, symbol ),
             *bs_header = &BOX_SYMBOL_HEADER( (row / 3) * 3 + col / 3, symbol );

//printf( "rc header: %s, rs_header %s, cs_header %s, bs_header %s\n",
//        rc_header->name, rs_header->name, cs_header->name, bs_header->name );

    if ( rc_header->left->right != rc_header ) {
//        printf("rc_header %s already covered, exiting\n", rc_header->name );
        return false;
    }
    cover( rc_header );

    if ( rs_header->left->right != rs_header ) {
//        printf("rs_header %s already covered, exiting\n", rs_header->name );
        return false;
    }
    cover( rs_header );     // a constraint may get down to 0 node

    if ( cs_header->left->right != cs_header ) {
//        printf("cs_header %s already covered, exiting\n", cs_header->name );
        return false;
    }
    cover( cs_header );     // the error will be seen by the solver

    if ( bs_header->left->right != bs_header ) {
//        printf("bs_header %s already covered, exiting\n", bs_header->name );
        return false;
    }
    cover( bs_header );
    return true;
}

static bool sudoku_set_dlx_constraints( void )
{
    sudoku_set_dlx_header_list( );    // first header list
    sudoku_set_dlx_entry_lists( );    // then the entry lists

    for ( int r = 0; r < SUDOKU_N_ROWS; ++r ) {
        for ( int c = 0; c < SUDOKU_N_COLS; ++c ) {
            sudoku_cell_t *cell = get_cell( r, c );
            if ( 1 == cell->n_symbols &&
                 ! set_given( r, c, get_number_from_map( cell->symbol_map ) ) )
                return false;
        }
    }
//    print_grid( );
    return true;
}

static void store_solution( node_t **solution, int n )
{
    while ( n-- ) {
        rcs_t  rcs;
        get_rcs_from_node( *solution, &rcs );
        set_cell_symbol( rcs.row, rcs.col, rcs.symbol, false );
        ++solution;
    }
}

static int solve( int n_solutions ) // returns number of solutions, up to n_solutions.
{
    int level = 0, count = 0;
    node_t *solution[SUDOKU_N_SYMBOLS*SUDOKU_N_SYMBOLS]; // list of currently chosen candidates

    while ( true ) {                        // forward loop: deterministically select constraint

        int min_items = SUDOKU_N_SYMBOLS + 1;
        header_t *best_header = NULL;

        // find header with the lowest number of items, in O(number of headers).
        for ( header_t *header = root.right; header != &root; header = header->right ) {
            if ( header->n_items < min_items ) {
                best_header = header;
                min_items = header->n_items;
            }
        }
        assert( best_header );  // can be removed (and best_header not initialized) after testing
        assert( best_header->n_items >= 0 );
//        printf( "Selected header %s n_items %d\n", best_header->name, best_header->n_items );

        // remove that header and set its first node in the expected solution stack
        cover( best_header );
        node_t *node = solution[level] = best_header->root.down;

        while ( true ) {                    // advance loop: cover other constraints for same entry
            if ( node != &best_header->root ) {
                for ( node_t *right = node->right; right != node; right = right->right ) {
                    cover( right->header );         // cover all neighboring node's headers
                }

                if ( root.right != &root ) {        // more constraints exist, matrix is not empty
                    ++level;
                    break;                  // break from advance loop, next forward
                }

                if ( 0 == count ) store_solution( solution, 1 + level );
                if ( ++count == n_solutions )       // empty matrix: one more solution
                    return n_solutions;             // enough to stop now
                
            } else {                                // no more constraint constraint, backup
                uncover( best_header );
                if ( 0 == level ) {
                    return count;                   // return without additional solution
                }

                --level;
                node = solution[level];
                best_header = node->header;
            }
                                                    // recover: uncover all neighboring node's headers
            for ( node_t *left = node->left; left != node; left = left->left ) {
                uncover( left->header );
            }
            node = solution[level] = node->down;    // and try another node
        }                                           // next advance loop
    }
}

static int solve_grid( bool multiple )
/* return 0, 1 or 2 according to the following table

    multiple n_solutions returned
      false       0         0
      false       >0        1

       true       0         0
       true       1         1
       true       >1        2 */
{
    game_new_grid();
    if ( ! sudoku_set_dlx_constraints() ) return 0;
    return solve( ( multiple ) ? 2 : 1 );   // solved grid, if any, is on top of stack
}

extern bool find_one_solution( void )
{
    if (is_game_solved()) return true;
    return (bool)solve_grid( false );
}

extern int check_current_grid( void )
{
    int sp = get_sp();
 
    SUDOKU_SOLVE_TRACE( ("\n#### Checking current grid for solutions @level %d\n", sp) );

    print_grid_pencils();
    int res = solve_grid( true );
#if SUDOKU_SOLVE_DEBUG
    if ( 2 == res ) {
        printf("More than one solution!\n");
    } else if ( 0 == res ) {
        printf("Not even a single solution!\n");
    } else {
        printf("One solution only!\n");
    }
#endif

    set_sp( sp );
    return res;
}

#define MAX_TRIALS  1000        // safety measure

static bool solve_random_cell_array( unsigned int seed )
{
    if ( 0 != seed ) set_random_seed( seed );

    reset_game();
    int n_trials = 0;

//                solve_count = true;

    while ( true ) {
        int col = random_value( 0, SUDOKU_N_COLS-1 );
        int row = random_value( 0, SUDOKU_N_ROWS-1 );

        sudoku_cell_t *cell = get_cell( row, col );
        if ( 1 != cell->n_symbols ) {
            int symbol = random_value( 0, SUDOKU_N_SYMBOLS-1 );

//            printf("Given symbol %c @random row %d, col %d\n", '1'+symbol, row, col );
            cell->state = SUDOKU_GIVEN;
            cell->symbol_map = get_map_from_number( symbol );
            cell->n_symbols = 1;

            int res = solve_grid( true );
            SUDOKU_SOLVE_TRACE( ("solve_random_cell_array: solve_grid returned %d\n", res) );
            reset_stack();

            if ( 1 == res ) break;  // exactly one solution, stop
            if ( 0 == res ) {       // no solution, remove symbol and keep trying
                cell->state = 0;
                cell->symbol_map = 0;
                cell->n_symbols = 0;
            }                       // else multiple solutions, keep trying
            if ( ++n_trials > MAX_TRIALS ) return false;
        }
    }
    SUDOKU_SOLVE_TRACE( ("Generated unique solution grid with %d symbols @level %d\n",
                         count_single_symbol_cells(), get_sp()) );
#if SUDOKU_SOLVE_DEBUG
    print_grid_pencils();
#endif
    return true;
}

typedef struct {
    int n_naked_singles, n_hidden_singles;
    int n_locked_candidates;
    int n_naked_subsets, n_hidden_subsets;
    int n_fishes, n_xy_wings, n_chains;
} hint_stats_t;

static void print_hint_stats( hint_stats_t *hstats )
{
    printf( "#Level determination:\n" );
    printf( "  naked singles: %d\n", hstats->n_naked_singles );
    printf( "  hidden singles: %d\n", hstats->n_hidden_singles );
    printf( "  locked candidates: %d\n", hstats->n_locked_candidates );
    printf( "  naked subsets: %d\n", hstats->n_naked_subsets );
    printf( "  hidden subsets: %d\n", hstats->n_hidden_subsets );
    printf( "  X-wings, fishes: %d\n", hstats->n_fishes );
    printf( "  XY-wings: %d\n", hstats->n_xy_wings );
    printf( "  chains: %d\n", hstats->n_chains );
}

static sudoku_level_t assess_hint_stats( hint_stats_t *hstats )
{
    print_hint_stats( hstats );

    if ( hstats->n_chains || hstats->n_fishes || hstats->n_xy_wings ) return DIFFICULT;

    if ( hstats->n_hidden_subsets ) return MODERATE;

    if ( hstats->n_naked_subsets || hstats->n_locked_candidates ) return SIMPLE;

    return EASY;
}

static sudoku_level_t evaluate_level( void )
{
//    print_grid_pencils(); // before
    game_new_filled_grid();
//    print_grid_pencils(); // after

    hint_stats_t hstats = { 0 };
    hint_desc_t hdesc;
    while ( true ) {
        if ( ! get_hint( &hdesc ) ) break;  // no hint
#if 0
        printf("@act_on_hint:\n");
        printf("  hint_type %d\n", hdesc.hint_type);
        printf("  n_hints %d\n", hdesc.n_hints);
        printf("  n_triggers %d\n", hdesc.n_triggers);
        printf("  action %d n_symbols=%d, map=0x%03x\n",
               hdesc.action, hdesc.n_symbols, hdesc.symbol_map);
#endif
        switch( hdesc.hint_type ) {
        case NO_HINT: case NO_SOLUTION:
            SUDOKU_ASSERT( 0 );
        case NAKED_SINGLE:
            ++hstats.n_naked_singles;
            break;
        case HIDDEN_SINGLE:
            ++hstats.n_hidden_singles;
            break;
        case LOCKED_CANDIDATE:
            ++hstats.n_locked_candidates;
            break;
        case NAKED_SUBSET:
            ++hstats.n_naked_subsets;
            break;
        case HIDDEN_SUBSET:
            ++hstats.n_hidden_subsets;
            break;
        case XWING: case SWORDFISH: case JELLYFISH:
            ++hstats.n_fishes;
            break;
        case XY_WING:
            ++hstats.n_xy_wings;
            break;
        case CHAIN:
            ++hstats.n_chains;
            break;
        }
        if ( act_on_hint( &hdesc ) ) return assess_hint_stats( &hstats );
    }
    print_hint_stats( &hstats );
    printf( "Stopped at NO_HINT\n" );
    return DIFFICULT;
}

extern sudoku_level_t make_game( int game_nb )
{
#ifdef TIME_MEASURE
    time_t start_time, end_time;

    if ( -1 == time( &start_time ) ) exit(1);
#endif /* TIME_MREASURE */

    printf("SUDOKU game_nb %d\n", game_nb );
    if ( ! solve_random_cell_array( game_nb ) ) {
        printf("solve_random_cell_array did not find a unique solution!\n");
        exit(1);
    }
printf("SUDOKU game nb %d solved\n", game_nb );
//    reduce_n_given();

#ifdef TIME_MEASURE
    if( -1 == time ( &end_time ) ) {
        printf("Unable to get end time\n");
    } else {
        printf("It took %g seconds to find this grid\n", difftime(end_time, start_time) );
    }
#endif /* TIME_MEASURE */

    reset_stack( );
    sudoku_level_t level = evaluate_level( );
    printf("Difficulty level %d\n", level );
    reset_stack();
    return level;
}
