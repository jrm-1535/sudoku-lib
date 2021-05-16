/*
   files.c
   Sudoku game - load or save game.
*/

#include <string.h>
#include <time.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include "sudoku.h"
#include "grid.h"
#include "game.h"
#include "files.h"

#define SUDOKU_SUCCESS 0    /**< For functions returning success */
#define SUDOKU_FAILURE (-1) /**< For functions returning some failure */

/* file syntax :

  # comment - can start anywhere in a line, stops at end of line.
  T nnnnnn
  C c  R r  = v    x : v   x,y = v   x,y : v1, v2 , v3
  where 
  nnnnnn is an integer number of seconds
  c, r, x, y, v are 1 digit in [1..9]
  = v is given symbol, at location (r,c)
  x = v is given symbol v at location (r,x)
  x,y = v is given symbol v at location (x,y)
  x : v1, v2, v3 are (non given) symbols v1, v2 and v3 at location (r,x).
  etc.
  spaces, tab and new lines are not significant.

  grammar:

  file:                       <expression>* <eof>
  eof:                        EOF
  expression:                 <space> | <time> | <command> |
                              <fully-specified-assignment> |
                              <column-assignment> | <assignment>

  space:                      ' ' | '\t' | '\n' | '\r' | comment
  comment:                    '#' <any-char> <eol>
  any-char:                   any ASCII char other than '\n' or '\r'.
  eol:                        '\n' | '\r'

  time                        <time-prefix> <space-value>
  time-prefix                 'T' | 't'
  space-value                 <value-space> | ( <space> <value-space> )
  value-space                 <number> | ( <value-space> <space> )
  number                      <digit> | ( <digit> <number> )
  digit                       '0' | '1' | '2' | '3' | '4' |
                              '5' | '6' | '7' | '8' | '9'

  command:                    <prefix> <space-symbol>
  prefix:                     'C' | 'c' | 'R' | 'r'
  space-symbol:               <symbol-space> | ( <space> <space-symbol> )
  symbol-space:               <symbol> | ( <symbol-space> <space> )

  fully-specified-assignment: <space-symbol> ',' <space-symbol> <assignment>
  column-assignment:          <space-symbol> <assignment>
  assignment:                 ( '=' <space-symbol> ) | ( ':' <symbol-list> )

  symbol-list:                <space-symbol> | ( <symbol-list> ',' <space-symbol> )
  symbol:                     '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'
*/

static int get_digit( FILE *fd )
{
    int c = getc(fd);

    if (c <'0' || c > '9') {
        ungetc( c, fd );
        return SUDOKU_FAILURE;
    }
    return c - '0';
}

static int get_symbol( FILE *fd )
{
    int c = getc(fd);

    if (c <'1' || c > '9') {
        ungetc( c, fd );
        return SUDOKU_FAILURE;
    }
    return c - '1';
}

static void skip_comment( FILE *fd )
{
    int c;
    while ( (EOF != ( c = getc(fd) )) && (c != '\n') && (c != '\r') );
    if ( EOF != c )
        ungetc( c, fd );
}

static void skip_space( FILE *fd )
{
    int c;
    while ( EOF != (c = getc(fd) ) ) {
        switch (c) {
        case ' ': case '\t': case '\n': case '\r': /* just skip */
            break;
        case '#':
            skip_comment( fd );
            break;
        default:
            ungetc( c, fd );
            return;
        }
    }
}

static unsigned long parse_time( FILE *fd )
{
    unsigned long time = 0;
    int digit = get_digit( fd );

    if ( SUDOKU_FAILURE == digit ) {
        printf("Syntax error: expecting a digit >= '0' (T)\n");
        return 0;
    }

    while ( SUDOKU_FAILURE != digit ) {
        time = time * 10;
        time += digit;
        digit = get_digit( fd );
    }
    return time;
}

static int cur_row = -1, cur_col = -1;
static int parse_command( FILE *fd, int c )
{
    int v = get_symbol( fd );
    if ( SUDOKU_FAILURE == v ) {
        printf("Syntax error: expecting a digit > '0' (%c)\n", c);
        return SUDOKU_FAILURE;
    }
    // here c can only be 'C', 'c', 'R' or 'r'
    if ( 'C' == (c & 0x5f) ) {  // upper case
        cur_col = v;
#if SUDOKU_FILE_DEBUG
        printf("Set CurCol to %d\n", cur_col );
#endif /* SUDOKU_FILE_DEBUG */
    } else {
        cur_row = v;
#if SUDOKU_FILE_DEBUG
        printf("Set CurRow to %d\n", cur_row );
#endif /* SUDOKU_FILE_DEBUG */
    }
    return SUDOKU_SUCCESS;
}

static int parse_assignment( FILE *fd )
{
    /* assingment may start with
        nothing  (use cur_row and cur_col)
        col      (use cur_row and col)
        row, col (use row and col) */
    int row = cur_row, col = cur_col;

    int n = 0, c, val;
    while ( n < 2 ) {
        val = get_symbol( fd );
        if ( SUDOKU_FAILURE == val )
            break;     // simple assignment

        if ( ++n == 2 ) row = col;
        col = val;

        skip_space( fd );

        c = getc( fd );     // may be a row followed by a col ?
        if (',' != c ) {
            ungetc( c, fd );    // nope, it isn't
            break;
        }
        skip_space( fd );
    }

    // followed by '=' or ':'
    bool is_given;
    switch ( (c = getc(fd)) ) {
    case '=':   // it is a single given value
        is_given = true;
        break;
    case ':':   // it is a list of values
        is_given = false;
        break;
    default:
        return SUDOKU_FAILURE;
    }
 
    skip_space( fd );
    val = get_symbol( fd );
    if ( SUDOKU_FAILURE == val )  return SUDOKU_FAILURE;

    set_cell_symbol( row, col, val, is_given );
    if ( ! is_given ) {
        while ( true ) {
            skip_space( fd );
            c = getc( fd );
            if (',' != c ) {
                ungetc( c, fd );
                break;
            }
            skip_space( fd );
            val = get_symbol( fd );
            if ( SUDOKU_FAILURE == val ) return SUDOKU_FAILURE;
            add_cell_candidate( row, col, val );
        }
    }
    return SUDOKU_SUCCESS;
}

static int parse_file( FILE *fd )
{
    unsigned long time = 0;

    int c;
    while ( EOF != (c = getc( fd )) ) {
        // printf("parsing %c\n", c );
        switch ( c ) {
        case 'C': case 'c': case 'R': case 'r':
            skip_space( fd );
            if ( SUDOKU_FAILURE == parse_command( fd, c ) )
                return SUDOKU_FAILURE;
            break;

        case 'T': case 't':
            if ( 0 != time )
                return SUDOKU_FAILURE;

            skip_space( fd );
            time = parse_time( fd );
            if ( 0 == time )
                return SUDOKU_FAILURE;
            break;

        case '#': case ' ': case '\t': case '\n': case '\r': /* just skip */
            ungetc( c, fd );
            skip_space( fd );
            break;

        default:  /* should be assignements */
            ungetc( c, fd );
            if ( SUDOKU_FAILURE == parse_assignment(fd) )
                return SUDOKU_FAILURE;
            break;
        }
    }

    set_game_time( time );
    return SUDOKU_SUCCESS;
}

extern int load_file( const char *name )
{
    FILE *fd;

    fd = fopen( name, "r" );
    if ( NULL == fd ) {
        printf("File %s does not exist\n", name);
        return 0;
    }

    if ( parse_file( fd) ) {
        printf("Error parsing file %s\n", name);
        return 0;
    }
    fclose(fd);
    return 1;
}

static int write_file( FILE *fd, const char *name )
{
    int r, c;

    fprintf(fd, "# Saved as %s\r\n\r\n", name );
    fprintf(fd, "T %lu \r\n", get_game_duration() );

    for ( r = 0; r < SUDOKU_N_ROWS; r ++ ) {
        bool row_set = false;
        for ( c = 0; c < SUDOKU_N_COLS; c++ ) {
            uint8_t ns;
            int     map;
            bool is_given = get_cell_type_n_map( r, c, &ns, &map );

            if ( ns > 0 ) {
                if ( ! row_set ) {
                    fprintf( fd, "r %c\r\n", '1' + r );
                    row_set = true;
                }
                fprintf(fd, " %c ", '1' + c );

                if ( is_given ) {
                    fputc( '=', fd );
                } else {
                    fputc( ':', fd );
                }
                int symbol = extract_bit_from_map( &map );
                SUDOKU_ASSERT(0 != symbol);
                fprintf(fd, " %d", 1 + symbol );

                while( true ) {
                    symbol = extract_bit_from_map( &map );
                    if ( 0 == symbol ) break;

                    fprintf(fd, ", %d", 1 + symbol );
                }
                fputs( "\r\n", fd );
            }
        }
    }
    return 0;
}

extern int sudoku_save_file( void *context, const char *name )
{
    (void)context;      // suppress unused paramater warning

    FILE *fd = fopen( name, "w" );
    if ( NULL == fd ) {
        printf("Cannot open File %s\n", name);
        return 0;
    }

    if ( write_file( fd, name ) ) {
        printf("Error writing file %s, aborting\n", name);
        fclose(fd);
        return 0;
    }
    fclose(fd);
    return 1;
}

