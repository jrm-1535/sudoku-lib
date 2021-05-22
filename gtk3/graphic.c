
/*
  Sudoku game - using GTK+
*/

#include <assert.h>
#include <string.h>
#include "../sudoku.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "dialog.h"

#define SUDOKU_OPTIONS_VERSION     2
#define SUDOKU_OPTIONS_FILENAME    ".sudoku-options"

#define SUDOKU_BACKGROUND_NAME     "bg.png"
#define SUDOKU_DEFAULT_NAME        "Sudoku"

#define DEFAULT_THEME_ID           CHALKBOARD_THEME
#define DEFAULT_TRANSLUCENT_STATE  false
#define DEFAULT_REMOVE_FILL_STATE  false
#define DEFAULT_TIMED_GAME_STATE   false
#define DEFAULT_DISPLAY_TIME_STATE false
#define DEFAULT_SHOW_HEADLINES     false

#define SUDOKU_GRAPHIC_DEBUG       1

#define SUDOKU_MAX_NAME_LEN         255
#define SUDOKU_DIR_PATH_LEN         511

#define SUDOKU_MAX_ERROR_LEN        256
#define MAX_STATUS_MESSAGE_LEN      128

#define MAX_MARK_LEVEL_SIZE         32

// Drawing area minimum width
#define SUDOKU_GRID_WIDTH          330
#define SUDOKU_GRID_HEIGHT         330

static void exit_error( char *err )
{
    printf("sudoku: %s - aborting\n", err );
    exit(1);
}

#define GRAPHIC_DEBUG   0

#if GRAPHIC_DEBUG
#define GRAPHIC_TRACE( _args ) printf _args
#else
#define GRAPHIC_TRACE( _args )
#endif

#if 0 // SUDOKU_GRAPHIC_DEBUG
static void exit_error_with_str_arg( char *err, const char *arg )
{
    char buffer[ SUDOKU_MAX_ERROR_LEN ];

    sprintf( buffer, err, arg );
    exit_error( buffer );
}
#endif

static void exit_error_with_int_arg( char *err, int arg )
{
    char buffer[ SUDOKU_MAX_ERROR_LEN ];
  
    sprintf( buffer, err, arg );
    exit_error( buffer);
}

typedef struct {
    GtkWidget               *window, *canvas, *info, *status,
                            *file_items[SUDOKU_FILE_BEYOND],
                            *edit_items[SUDOKU_EDIT_BEYOND],
                            *tools_items[SUDOKU_TOOL_BEYOND];

    cairo_surface_t         *image;
    int                     image_width, image_height;

    theme_id_t              theme_id;
    bool                    translucent_state,
                            remove_fill_state,
                            timed_game_state,
                            display_time_state,
                            show_headlines;

    char                    home[1+SUDOKU_DIR_PATH_LEN];
    char                    background_path[1+SUDOKU_DIR_PATH_LEN];
    char                    wname[1+SUDOKU_MAX_NAME_LEN];

} game_cntxt_t;

static bool initialize_paths( game_cntxt_t *game_cx , char *command )
{
    /* find sudoku directory from the command */
    printf("Command %s\n", command );

    size_t len;
    if ( '/' == command[0] ) { /* absolute path name */
        char *end = strrchr( command, '/' );
        if ( end == command ) {  /* surprisingly root. Chroot? */
            strcpy ( game_cx->background_path, "/");
        } else {
            len = end-command;
            if ( len > SUDOKU_DIR_PATH_LEN ) return false;
            strncpy( game_cx->background_path, command, len );
            game_cx->background_path[len] = '\0';
        }
    } else {   /* relative path name */
        if ( NULL == getcwd( game_cx->background_path, SUDOKU_DIR_PATH_LEN ) ) return false;
    }
    printf("Sudoku is in %s\n", game_cx->background_path );

    len = strlen( game_cx->background_path );

    /* need room for: directory + '/' + name + '\0' */
    if ( len >= (SUDOKU_DIR_PATH_LEN - sizeof(SUDOKU_BACKGROUND_NAME) - 1 ) ) return false;

    strcpy( &game_cx->background_path[len], "/" SUDOKU_BACKGROUND_NAME );
    printf("Background is in %s\n", game_cx->background_path );

    if ( NULL == getcwd( game_cx->home, SUDOKU_DIR_PATH_LEN ) ) return false;
    printf("Home is in %s\n", game_cx->home );
    return true;
}

static bool initialize_background_image( game_cntxt_t *game_cx )
{
    game_cx->image = cairo_image_surface_create_from_png( game_cx->background_path );
    if ( NULL == game_cx->image ) {
        printf("Cannot create surface from png %s\n", game_cx->background_path );
        return false;
    }
    if ( CAIRO_STATUS_SUCCESS != cairo_surface_status( game_cx->image ) ) {
        printf("Invalid png surface %s\n", game_cx->background_path );
        return false;
    }

    printf("Background Image %p initialized\n", (void *)game_cx->image );
	game_cx->image_width = cairo_image_surface_get_width( game_cx->image );
	game_cx->image_height = cairo_image_surface_get_height( game_cx->image );
    return true;
}

static int update_options( game_cntxt_t *game_cntxt )
{
    FILE *options = fopen( SUDOKU_OPTIONS_FILENAME, "w" );

    if( NULL == options ) {
        printf("Unable to save options in " SUDOKU_OPTIONS_FILENAME "\n");
        return 1;
    }

    fprintf( options, "SUDOKU Siesta options version=%d:\n", SUDOKU_OPTIONS_VERSION );
    fprintf( options, "theme_id=%d\ntranslucent_state=%d\nremove_fill_state=%d\ntimed_game_state=%d\ndisplay_time_state=%d\nshow_headlines=%d\n", 
             game_cntxt->theme_id, game_cntxt->translucent_state, game_cntxt->remove_fill_state,
             game_cntxt->timed_game_state, game_cntxt->display_time_state, game_cntxt->show_headlines );

      fclose( options );
    return 0;
}

static int read_options( game_cntxt_t *game_cntxt )
{
    /* default values if we cannot read the file */
    game_cntxt->theme_id = DEFAULT_THEME_ID;
    game_cntxt->translucent_state = DEFAULT_TRANSLUCENT_STATE;
    game_cntxt->remove_fill_state = DEFAULT_REMOVE_FILL_STATE;
    game_cntxt->timed_game_state = DEFAULT_TIMED_GAME_STATE;
    game_cntxt->display_time_state = DEFAULT_DISPLAY_TIME_STATE;
    game_cntxt->show_headlines = DEFAULT_SHOW_HEADLINES;

    FILE *options = fopen( SUDOKU_OPTIONS_FILENAME, "r" );
    if( NULL == options ) return 1;         // no previous option file

    int version=0;
    if ( 1 != fscanf( options, "SUDOKU Siesta options version=%d: ", &version ) ) {
        printf( "Corrupted option file: cannot read option version\n" );
        return 1;
    }

    if ( version != SUDOKU_OPTIONS_VERSION ) {
        printf( "Invalid option file: unexpected option version %d\n", version );
        return 1;
    }

    int temp_theme_id, temp_translucent_state, temp_remove_fill_state, temp_timed_game_state, temp_display_time_state, temp_show_headlines;
    if ( 6 != fscanf( options, "theme_id=%d\ntranslucent_state=%d\nremove_fill_state=%d\ntimed_game_state=%d\ndisplay_time_state=%d\nshow_headlines=%d\n",
                      &temp_theme_id, &temp_translucent_state, &temp_remove_fill_state, &temp_timed_game_state, &temp_display_time_state, &temp_show_headlines ) ) {
        printf( "Corrupted option file: cannot read options\n" );
        return 1;
    }

    /* check validity in this version */
    switch ( temp_theme_id ) {
    case CHALKBOARD_THEME: case PAPER_THEME: case IMAGE_THEME: 
        break;
    default:
        printf( "Invalid option file: invalid theme (%d)\n", temp_theme_id );
        return 1;
    }

    if ( 0 > temp_translucent_state || 1 < temp_translucent_state ) {
        printf( "Invalid option file: invalid translucent state  (%d)\n", temp_translucent_state );
        return 1;
    }

    if ( 0 > temp_remove_fill_state || 1 < temp_remove_fill_state ) {
        printf( "Invalid option file: invalid translucent state  (%d)\n", temp_remove_fill_state );
        return 1;
    }

    if ( 0 > temp_timed_game_state || 1 < temp_timed_game_state ) {
        printf( "Invalid option file: invalid translucent state  (%d)\n", temp_timed_game_state );
        return 1;
    }

    if ( 0 > temp_display_time_state || 1 < temp_display_time_state ) {
        printf( "Invalid option file: invalid translucent state  (%d)\n", temp_display_time_state );
        return 1;
    }

    if ( 0 > temp_show_headlines || 1 < temp_show_headlines ) {
        printf( "Invalid option file: invalid translucent state  (%d)\n", temp_show_headlines );
        return 1;
    }

    game_cntxt->theme_id = temp_theme_id;
    game_cntxt->translucent_state = temp_translucent_state;
    game_cntxt->remove_fill_state = temp_remove_fill_state;
    game_cntxt->timed_game_state = temp_timed_game_state;
    game_cntxt->display_time_state = temp_display_time_state;
    game_cntxt->show_headlines = temp_show_headlines;

    printf( "Sucessfully loaded options version %d:\n", version );
    printf( "theme_id=%d, translucent_state=%d, remove_fill_state=%d, timed_game_state=%d, display_time_state=%d, show_headlines=%d\n", 
            game_cntxt->theme_id, game_cntxt->translucent_state, game_cntxt->remove_fill_state,
            game_cntxt->timed_game_state, game_cntxt->display_time_state, game_cntxt->show_headlines );

    return 0;
}

static gboolean timed_game_fct( gpointer data )
{
    game_cntxt_t *cntxt = (game_cntxt_t *)data;

    sudoku_duration_t duration;
    if ( cntxt->display_time_state && sudoku_how_long_playing( &duration ) ) {
        char info_buffer[64];
        sprintf( info_buffer, "time: %02d:%02d:%02d", duration.hours, duration.minutes, duration.seconds );
        gtk_label_set_text( GTK_LABEL(cntxt->info), (const char *)info_buffer );
        return TRUE;
    }
    return FALSE;
}

static void manage_displaying_time( game_cntxt_t *cntxt )
{
    if ( cntxt->display_time_state ) {
        sudoku_duration_t duration;
        if ( sudoku_how_long_playing( &duration ) ) {
            char info_buffer[64];
            sprintf( info_buffer, "time: %02d:%02d:%02d", duration.hours, duration.minutes, duration.seconds );
            gtk_label_set_text( GTK_LABEL(cntxt->info), (const char *)info_buffer );
            g_timeout_add_seconds ( 1, timed_game_fct, (gpointer) cntxt );
            return;
        }
    }
    gtk_label_set_text( GTK_LABEL(cntxt->info), "Siesta Productions" );
}

static void begin_print( GtkPrintOperation *operation,
                         GtkPrintContext   *context,
                         gpointer           user_data )
{
    (void)context;
    (void)user_data;
    GRAPHIC_TRACE( ("Print_game: begin print\n") );
    gtk_print_operation_set_n_pages( operation, 1 );
}

#define DEFAULT_LINE_WIDTH              1.0
// Alpha channel
#define TRANSLUCENT_ALPHA               0.75 //  75% opaque
#define OPAQUE_ALPHA                    1.0  // 100% opaque

// PRINTING_THEME
#define PRINTING_HEADER_BG              0.10, 0.10, 0.40, 1.0 // blue-ish
#define PRINTING_HEADER_FG              0.10, 0.10, 0.40, 1.0 // blue-ish

// background is not painted for printing, but a rectangle is drawn around the game
#define PRINTING_GAME_LINE_FG           0.0, 0.0, 0.0         // black, always opaque
#define PRINTING_GAME_LINE_WIDTH        3.0

#define PRINTING_GIVEN_FG               0.0, 0.45, 0.05, 1.0  // green
#define PRINTING_ENTERED_FG             0.40, 0.23, 0.0, 1.0  // orange
#define PRINTING_PENCIL_FG              0.38, 0.35, 0.0, 1.0  // dark yellow

#define PRINTING_SELECTED_FG            0.8, 0.8, 0.8, 1.0    // grey
#define PRINTING_HINT_FG                0.80, 0.70, 0.0, 1.0  // yellowish
#define PRINTING_CHAIN_HEAD_FG          1.0, 0.0, 0.5, 1.0    // dark red

#define PRINTING_ERROR_BG               0.5, 0.0, 0.0, 1.0    // red
#define PRINTING_WEAK_TRIGGER_BG        0.0, 0.7, 0.9, 1.0    // blueish
#define PRINTING_TRIGGER_BG             0.0, 0.5, 0.7, 1.0    // darker blueish 
#define PRINTING_ALTERNATE_TRIGGER_BG   0.0, 0.7, 0.5, 1.0    // darker greenish 

#define PRINTING_MAJOR_LINE_FG          0.0, 0.0, 0.0, 1.0    // back
#define PRINTING_MAJOR_LINE_WIDTH       2.0

#define PRINTING_MINOR_LINE_FG          0.15, 0.15, 0.15, 1.0    // dark grey
#define PRINTING_MINOR_LINE_WIDTH       1.0

// CHALKBOARD_THEME
#define CHALKBOARD_HEADER_BG            0.10, 0.10, 0.40, 1.0 // blue-ish
#define CHALKBOARD_HEADER_FG            0.50, 0.50, 1.0, 1.0  // light blue

#define CHALKBOARD_BG                   0.0, 0.0, 0.0         // black

#define CHALKBOARD_GIVEN_FG             0.0, 0.7, 0.1, 1.0    // bright green
#define CHALKBOARD_ENTERED_FG           0.75, 0.42, 0.0, 1.0  // orange
#define CHALKBOARD_PENCIL_FG            0.56, 0.46, 0.0, 1.0  // yellow

#define CHALKBOARD_SELECTED_FG          1.0, 0.2, 0.2, 1.0    // bright red
#define CHALKBOARD_HINT_FG              0.6, 0.8, 0.2, 1.0    // light green
#define CHALKBOARD_CHAIN_HEAD_FG        0.0, 0.8, 0.8, 1.0    // cyan

#define CHALKBOARD_ERROR_BG             0.5, 0.0, 0.0, 1.0    // dark red
#define CHALKBOARD_WEAK_TRIGGER_BG      0.0, 0.05, 0.2, 1.0   // darker blueish
#define CHALKBOARD_TRIGGER_BG           0.0, 0.15, 0.35, 1.0  // blueish
#define CHALKBOARD_ALTERNATE_TRIGGER_BG 0.0, 0.35, 0.15, 1.0  // darker greenish 

#define CHALKBOARD_MAJOR_LINE_FG        1.0, 1.0, 1.0, 1.0    // white
#define CHALKBOARD_MINOR_LINE_FG        0.3, 0.3, 0.3, 1.0    // grey

// PAPER_THEME
#define PAPER_HEADER_BG                 0.10, 0.10, 0.40, 1.0 // blue-ish
#define PAPER_HEADER_FG                 0.10, 0.10, 0.40, 1.0 // blue-ish

#define PAPER_BG                        0.75, 0.75, 0.75      // light grey

#define PAPER_GIVEN_FG                  0.0, 0.45, 0.05, 1.0  // green
#define PAPER_ENTERED_FG                0.40, 0.23, 0.0, 1.0  // orange
#define PAPER_PENCIL_FG                 0.28, 0.23, 0.0, 1.0  // dark yellow

#define PAPER_SELECTED_FG               0.6, 0.1, 0.1, 1.0    // red
#define PAPER_HINT_FG                   0.3, 0.3, 0.0, 1.0    // yellowish
#define PAPER_CHAIN_HEAD_FG             0.0, 0.3, 0.3, 1.0    // dark cyan

#define PAPER_ERROR_BG                  0.5, 0.0, 0.0, 1.0    // red
#define PAPER_WEAK_TRIGGER_BG           0.0, 0.8, 1.0, 1.0    // light blueish
#define PAPER_TRIGGER_BG                0.0, 0.6, 0.8, 1.0    // blueish
#define PAPER_ALTERNATE_TRIGGER_BG      0.0, 0.8, 0.6, 1.0    // greenish 

#define PAPER_MAJOR_LINE_FG             0.0, 0.0, 0.0, 1.0    // back
#define PAPER_MINOR_LINE_FG             0.95, 0.95, 0.95, 1.0 // dark grey

// IMAGE_THEME
#define IMAGE_HEADER_BG                 0.10, 0.10, 0.40, 1.0 // blue-ish
#define IMAGE_HEADER_FG                 0.10, 0.10, 0.40, 1.0 // blue-ish

// background is a png image (special case).

#define IMAGE_GIVEN_FG                  0.0, 0.40, 0.05, 1.0  // dark green
#define IMAGE_ENTERED_FG                0.40, 0.23, 0.0, 1.0  // orange
#define IMAGE_PENCIL_FG                 0.28, 0.23, 0.0, 1.0  // dark yellow

#define IMAGE_SELECTED_FG               0.6, 0.5, 0.5, 1.0    // light green
#define IMAGE_HINT_FG                   0.3, 0.3, 0.0, 1.0    // brownish
#define IMAGE_CHAIN_HEAD_FG             0.0, 0.3, 0.3, 1.0    // dark cyan

#define IMAGE_ERROR_BG                  0.5, 0.0, 0.0, 1.0    // red
#define IMAGE_WEAK_TRIGGER_BG           0.0, 0.8, 1.0, 1.0    // lighr blueish
#define IMAGE_TRIGGER_BG                0.0, 0.6, 0.8, 1.0    // blueish
#define IMAGE_ALTERNATE_TRIGGER_BG      0.0, 0.8, 0.6, 1.0    // greenish 

#define IMAGE_MAJOR_LINE_FG             0.0, 0.0, 0.0, 1.0    // back
#define IMAGE_MINOR_LINE_FG             0.95, 0.95, 0.95, 1.0 // dark grey

#define PRINT_FONT                      "Courier 10 Pitch"
#define CANVAS_FONT                     "Courier 10 Pitch"
#define SUDOKU_FONT_SLANT               CAIRO_FONT_SLANT_NORMAL
#define SUDOKU_FONT_WEIGHT              CAIRO_FONT_WEIGHT_NORMAL

static void set_source_line_width( game_cntxt_t *game_cx, cairo_t *cr, bool major )
{
    if ( PRINTING_THEME == game_cx->theme_id ) {
        if ( major ) {
            cairo_set_line_width( cr, PRINTING_MAJOR_LINE_WIDTH );
        } else {
            cairo_set_line_width( cr, PRINTING_MINOR_LINE_WIDTH );
        }
        return;
    }
    // all other cases use DEFAULT_LINE_WIDTH
    cairo_set_line_width( cr, DEFAULT_LINE_WIDTH );
}

// theme specific colors
typedef enum {
    HEADER_BG, HEADER_FG,
    GIVEN_FG, ENTERED_FG, PENCIL_FG,
    SELECTED_FG, HINT_FG, CHAIN_HEAD_FG,

    ERROR_BG, WEAK_TRIGGER_BG, TRIGGER_BG, ALTERNATE_TRIGGER_BG,
    MAJOR_LINE_FG, MINOR_LINE_FG
} color_type_t;

typedef struct {
    double r, g, b, a;
} rgba_t;

static rgba_t *get_theme_colors( game_cntxt_t *game_cx )
{
    static rgba_t printing_colors[] = {
        { PRINTING_HEADER_BG }, { PRINTING_HEADER_FG },
        { PRINTING_GIVEN_FG }, { PRINTING_ENTERED_FG }, { PRINTING_PENCIL_FG },
        { PRINTING_SELECTED_FG }, { PRINTING_HINT_FG }, { PRINTING_CHAIN_HEAD_FG },
        { PRINTING_ERROR_BG }, 
        { PRINTING_WEAK_TRIGGER_BG }, { PRINTING_TRIGGER_BG }, { PRINTING_ALTERNATE_TRIGGER_BG },
        { PRINTING_MAJOR_LINE_FG }, { PRINTING_MINOR_LINE_FG }
    };

    static rgba_t chalkboard_colors[] = {
        { CHALKBOARD_HEADER_BG }, { CHALKBOARD_HEADER_FG },
        { CHALKBOARD_GIVEN_FG }, { CHALKBOARD_ENTERED_FG }, { CHALKBOARD_PENCIL_FG },
        { CHALKBOARD_SELECTED_FG }, { CHALKBOARD_HINT_FG }, { CHALKBOARD_CHAIN_HEAD_FG },
        { CHALKBOARD_ERROR_BG },
        { CHALKBOARD_WEAK_TRIGGER_BG }, { CHALKBOARD_TRIGGER_BG }, { CHALKBOARD_ALTERNATE_TRIGGER_BG },
        { CHALKBOARD_MAJOR_LINE_FG }, { CHALKBOARD_MINOR_LINE_FG }
    };

    static rgba_t paper_colors[] = {
        { PAPER_HEADER_BG }, { PAPER_HEADER_FG },
        { PAPER_GIVEN_FG }, { PAPER_ENTERED_FG }, { PAPER_PENCIL_FG },
        { PAPER_SELECTED_FG }, { PAPER_HINT_FG }, { PAPER_CHAIN_HEAD_FG },
        { PAPER_ERROR_BG },
        { PAPER_WEAK_TRIGGER_BG }, { PAPER_TRIGGER_BG }, { PAPER_ALTERNATE_TRIGGER_BG },
        { PAPER_MAJOR_LINE_FG }, { PAPER_MINOR_LINE_FG }
    };

    static rgba_t image_colors[] = {
        { IMAGE_HEADER_BG }, { IMAGE_HEADER_FG },
        { IMAGE_GIVEN_FG }, { IMAGE_ENTERED_FG }, { IMAGE_PENCIL_FG },
        { IMAGE_SELECTED_FG }, { IMAGE_HINT_FG }, { IMAGE_CHAIN_HEAD_FG },
        { IMAGE_ERROR_BG },
        { IMAGE_WEAK_TRIGGER_BG }, { IMAGE_TRIGGER_BG }, { IMAGE_ALTERNATE_TRIGGER_BG },
        { IMAGE_MAJOR_LINE_FG }, { IMAGE_MINOR_LINE_FG }
    };

    rgba_t *colors;
    switch( game_cx->theme_id ) {
    default:
        assert( 0 );
    case PRINTING_THEME:    colors = printing_colors; break;
    case CHALKBOARD_THEME:  colors = chalkboard_colors; break;
    case PAPER_THEME:       colors = paper_colors; break;
    case IMAGE_THEME:       colors = image_colors; break;
    }
    return colors;
}

static void set_source_color( game_cntxt_t *game_cx, cairo_t *cr, color_type_t which )
{
    rgba_t *colors = get_theme_colors( game_cx );
    cairo_set_source_rgba( cr, colors[ which ].r, colors[ which ].g, colors[ which ].b, colors[ which ].a );
}

static void draw_area_background( game_cntxt_t *game_cx, cairo_t *cr, double width, double height )
{
    cairo_set_operator( cr, CAIRO_OPERATOR_SOURCE );
    double alpha = ( game_cx->translucent_state ) ? TRANSLUCENT_ALPHA : OPAQUE_ALPHA;

    switch( game_cx->theme_id ) {
    case PRINTING_THEME:    // game rectangle will be printed after all cells and lines
        return;

    case CHALKBOARD_THEME:
        cairo_set_source_rgba( cr, CHALKBOARD_BG, alpha );
        cairo_paint( cr );
        return;

    case PAPER_THEME:
        cairo_set_source_rgba( cr, PAPER_BG, alpha );
        cairo_paint( cr );
        return;

    case IMAGE_THEME:
//printf( "Use Background Image %p, width=%d, height=%d scaled to w=%g, h=%g\n",
//        (void *)game_cx->image, game_cx->image_width, game_cx->image_height,
//        width/(double)game_cx->image_width, height/(double)game_cx->image_height );
        cairo_save( cr );
	    cairo_scale( cr, width/(double)game_cx->image_width, height/(double)game_cx->image_height );
	    cairo_set_source_surface( cr, game_cx->image, 0.0, 0.0 );
        cairo_paint( cr );
        cairo_restore( cr );
        return;
    }
}

static void draw_game( game_cntxt_t *game_cx, cairo_t *cr, double width, double height )
{
    draw_area_background( game_cx, cr, width, height );

    int large_font_size = (( width > height ) ? height : width) / SUDOKU_N_COLS; // asuming same as SUDOKU_NB_ROWS
//    printf( "large font size=%d\n", large_font_size );

    cairo_text_extents_t extents;
    char buffer[2];
    buffer[1] = 0;

    double x = 0.0, y = 0.0;

    // draw header row and col
    if ( game_cx->show_headlines ) {
        set_source_color( game_cx, cr, HEADER_BG );
        cairo_rectangle( cr, 0.0, 0.0, width, height/(double)(SUDOKU_N_ROWS+1) );
        cairo_fill( cr );                               // fill with header background color
        cairo_rectangle( cr, 0.0, height/(double)(SUDOKU_N_ROWS+1),
                         width/(double)(SUDOKU_N_COLS+1), height - height/(double)(SUDOKU_N_ROWS+1) );
        cairo_fill( cr );

        set_source_color( game_cx, cr, HEADER_FG );
        cairo_select_font_face( cr, CANVAS_FONT, CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_WEIGHT_NORMAL );
        cairo_set_font_size( cr, large_font_size );
        cairo_text_extents( cr, "5", &extents );

        printf( "extents: x_baring=%g y_baring=%g width=%g height=%g x_advance=%g y_advance=%g\n",
                extents.x_bearing, extents.y_bearing, extents.width, extents.height,
                extents.x_advance, extents.y_advance );

        for ( int i = 1; i <= SUDOKU_N_COLS; ++i ) {
            buffer[0] = '0'+i;
            double cx = ( width / (double)(SUDOKU_N_ROWS+1) ) * ( (double)i + 0.5 ) -
                          extents.x_bearing - (extents.width / 2.0);
            double cy = ( height / (2.0 * (double)(SUDOKU_N_ROWS+1)) ) + (extents.height / 2.0);

            cairo_move_to( cr, cx, cy );
            cairo_show_text( cr, buffer );
        }
        
        for ( int i = 1; i <= SUDOKU_N_ROWS; ++i ) {
            buffer[0] = '0'+i;
            double cx = ( width / (2.0 * (double)(SUDOKU_N_COLS+1)) ) - extents.x_bearing - (extents.width / 2.0);
            double cy = ( height / (double)(SUDOKU_N_ROWS+1) ) * ( (double)i + 0.5 ) + (extents.height / 2.0);

            cairo_move_to( cr, cx, cy );
            cairo_show_text( cr, buffer );
        }
        x = width / (double)(SUDOKU_N_COLS+1);
        width -= x;
        y = height / (double)(SUDOKU_N_ROWS+1);
        height -= y;
    }

    // then, draw each cell
    cairo_select_font_face (cr, CANVAS_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    double cw = width / (double)SUDOKU_N_COLS;
    double ch = height / (double)SUDOKU_N_ROWS;
    for ( int r = 0; r < SUDOKU_N_ROWS; ++r ) {
        double cy = y + ch * r;
        for ( int c = 0; c < SUDOKU_N_COLS; ++c ) {
            sudoku_cell_t cell;
            sudoku_get_cell_definition( r, c, &cell );
//            printf( "Cell state 0x%03x\n", cell.state );
            double cx = x + (cw * c);
            if ( SUDOKU_IS_CELL_IN_ERROR( cell.state ) ) {
                set_source_color( game_cx, cr, ERROR_BG );   // error background
                cairo_rectangle( cr, cx, cy, cw, ch );
                cairo_fill( cr );                            // fill with background
            } else if ( SUDOKU_IS_CELL_WEAK_TRIGGER( cell.state ) ) {
                set_source_color( game_cx, cr, WEAK_TRIGGER_BG ); // weak trigger background
                cairo_rectangle( cr, cx, cy, cw, ch );
                cairo_fill( cr );                            // fill with background
            } else if ( SUDOKU_IS_CELL_TRIGGER( cell.state ) ) {
                set_source_color( game_cx, cr, TRIGGER_BG ); // main trigger background
                cairo_rectangle( cr, cx, cy, cw, ch );
                cairo_fill( cr );                            // fill with background
            } else if ( SUDOKU_IS_CELL_ALTERNATE_TRIGGER( cell.state ) ) {
                set_source_color( game_cx, cr, ALTERNATE_TRIGGER_BG ); // alternate trigger background
                cairo_rectangle( cr, cx, cy, cw, ch );
                cairo_fill( cr );                            // fill with background
            }
            if ( SUDOKU_IS_CELL_CHAIN_HEAD( cell.state ) ) {
                set_source_color( game_cx, cr, CHAIN_HEAD_FG );  // chain head outline
                cairo_rectangle( cr, cx + 3, cy + 3, cw - 6, ch - 6 );
                cairo_stroke(cr);
            }
            if ( SUDOKU_IS_CELL_HINT( cell.state ) ) {
                set_source_color( game_cx, cr, HINT_FG );    // hint outline
                double dash[] = { 10.0, 6.0 };
                cairo_set_dash( cr, dash, sizeof(dash)/sizeof(dash[0]), 5.0 );
                cairo_rectangle( cr, cx + 2, cy + 2, cw - 4, ch - 4 );
                cairo_stroke(cr);
                cairo_set_dash( cr, dash, 0, 0.0 );
            }
            if ( SUDOKU_IS_CELL_SELECTED( cell.state ) ) {
                set_source_color( game_cx, cr, SELECTED_FG ); // selected outline
                cairo_rectangle( cr, cx + 1, cy + 1, cw - 2, ch -2 );
                cairo_stroke(cr);
            }

            double lx, ty;
            if ( cell.n_symbols > 1 ) {
                set_source_color( game_cx, cr, PENCIL_FG ); // penciled symbols foreground
                // asuming SUDOKU_PENCILED_PER_ROW is equal to SUDOKU_PENCILED_ROWS
                int small_font_size = large_font_size / SUDOKU_PENCILED_PER_ROW;
//                printf( "small font size=%d\n", small_font_size );
                cairo_set_font_size (cr, small_font_size );

                buffer[0] = '5';
                cairo_text_extents (cr, buffer, &extents);

//    printf( "extents for '5' width=%g, height=%g, x bearing=%g, y bearing=%g, x adavance=%g, y advance=%g\n", 
//            extents.width, extents.height, extents.x_bearing, extents.y_bearing, extents.x_advance, extents.y_advance );

                /*
                    ---|---------------|---
                       | [1]  [2]  [3] |
                       |<---><---><--->|
                       |  ^    ^    ^  |
                       |<>|<-->|<-->|<>| <--> is 1/3 of cell width and <> is 1/6 of cell width
                       1rst  2nd  3rd

                    leftmost and topmost pencilled positions are therefore defined in a cell as:
                    left_most = cell_left + cell_width / ( SUDOKU_PENCILLED_PER_ROW * 2 )
                    top_most = cell_top + cell_height / ( SUDOKU_PENCILLED_ROWS * 2 )

                    The increment between 1st and 2nd, and between 2nd and 3rd are for
                    col: cell_width / SUDOKU_PENCILLED_PER_ROW
                    row: cell_height / SUDOKU_PENCILLED_ROWS
                */
                lx = cx + ( width / (double)(SUDOKU_N_COLS * SUDOKU_PENCILED_PER_ROW * 2 ) );
                ty = cy + ( height / (double)(SUDOKU_N_ROWS * SUDOKU_PENCILED_ROWS * 2 ) );
//                printf( "letfmost=%g, topmost=%g\n", lx, ty );

                for ( int i = 0; i < 9; ++i ) {
                    if ( cell.symbol_map & (1<<i) ) {
                        buffer[0] = '1'+i;
                        int j = i % 3, k = i / 3;
                        double zx = lx + ( width / (double)(SUDOKU_N_COLS * SUDOKU_PENCILED_PER_ROW) ) * j;
                        double zy = ty + ( height / (double)(SUDOKU_N_ROWS * SUDOKU_PENCILED_ROWS) ) * k;
                        cairo_move_to ( cr, zx - extents.x_bearing - (extents.width / 2.0),
                                            zy + (extents.height / 2.0) );
                        cairo_show_text( cr, buffer );
                    }
                }
//                continue;
            } else if ( cell.n_symbols == 1 ) {
                if ( SUDOKU_IS_CELL_GIVEN( cell.state ) ) {
                    set_source_color( game_cx, cr, GIVEN_FG );    // given symbol foreground
                } else {
                    set_source_color( game_cx, cr, ENTERED_FG );  // entered symbol foreground
                }
                cairo_set_font_size (cr, large_font_size );
                cairo_text_extents (cr, "5", &extents);

                lx = cx + ( width / (double)(SUDOKU_N_COLS * 2) );
                ty = cy + ( height / (double)(SUDOKU_N_ROWS * 2) );

                buffer[0] = sudoku_get_symbol( &cell );
                cairo_move_to( cr, lx - extents.x_bearing - (extents.width / 2.0),
                                   ty + (extents.height / 2.0) );
                cairo_show_text( cr, buffer );
            }
        }
    }

    // finally, draw lines - actual area is (x, y) to (width, height)    
    // minor lines first
    for ( int i = 1; i < SUDOKU_N_ROWS; ++i ) {               // draw horizontal lines
        cairo_move_to(cr, x, y + ( height / (double)SUDOKU_N_ROWS ) * i );
        if ( i % 3 ) {                                        // a minor line
            set_source_line_width( game_cx, cr, false );
            set_source_color( game_cx, cr, MINOR_LINE_FG );
            cairo_line_to(cr, x + width, y + ( height / (double)SUDOKU_N_ROWS ) * i  );
            cairo_stroke(cr);
        }
    }

    for ( int i = 1; i < SUDOKU_N_COLS; ++i ) {               // draw vertical lines
        cairo_move_to(cr, x + ( width / (double)SUDOKU_N_COLS ) * i, y );
        if ( i % 3 ) {                                        // a minor line
            set_source_line_width( game_cx, cr, false );
            set_source_color( game_cx, cr, MINOR_LINE_FG );
            cairo_line_to(cr, x + ( width / (double)SUDOKU_N_COLS ) * i, y + height );
            cairo_stroke(cr);
        }
    }
    // finally major lines only
    for ( int i = 1; i < SUDOKU_N_ROWS; ++i ) {               // draw horizontal lines
        cairo_move_to(cr, x, y + ( height / (double)SUDOKU_N_ROWS ) * i );
        if ( 0 == i % 3 ) {                                    // a major line
            set_source_line_width( game_cx, cr, true );
            set_source_color( game_cx, cr, MAJOR_LINE_FG );
            cairo_line_to(cr, x + width, y + ( height / (double)SUDOKU_N_ROWS ) * i  );
            cairo_stroke(cr);
        }
    }
    for ( int i = 1; i < SUDOKU_N_COLS; ++i ) {               // draw vertical lines
        cairo_move_to(cr, x + ( width / (double)SUDOKU_N_COLS ) * i, y );
        if ( 0 == i % 3 ) {                                    // a major line
            set_source_line_width( game_cx, cr, true );
            set_source_color( game_cx, cr, MAJOR_LINE_FG );
            cairo_line_to(cr, x + ( width / (double)SUDOKU_N_COLS ) * i, y + height );
            cairo_stroke(cr);
        }
    }
}

static gboolean draw_cb( GtkWidget *widget, cairo_t *cr, gpointer data )
{
    game_cntxt_t *game_cx = (game_cntxt_t *)data;

    double width = (double)gtk_widget_get_allocated_width( widget );    // draw area width
    double height = (double)gtk_widget_get_allocated_height( widget );  // draw area height
//printf( "Drawing canvas width %g, height %g\n", width, height );
    draw_game( game_cx, cr, width, height );
    return FALSE;
}

static void draw_page( GtkPrintOperation *operation,
                       GtkPrintContext   *context,
                       gint               page_nr,
                       gpointer           user_data )
{
    (void)operation;
    (void)page_nr;
    game_cntxt_t *game_cntxt = (game_cntxt_t *)user_data;
    cairo_t *cr = gtk_print_context_get_cairo_context( context );

    double page_width = gtk_print_context_get_width( context );
    double page_height = gtk_print_context_get_height( context );

    GRAPHIC_TRACE( ("drawing page witdh = %g height = %g\n", page_width, page_height) );

    double print_area_width = page_width - 100;
    double print_area_height = page_width - 100;

    GRAPHIC_TRACE( ("print  : game width %g height %g\n", print_area_width, print_area_height) );

    // center the game on page
    cairo_translate( cr, 50.0, (page_height - print_area_height)/2.0 );

    theme_id_t previous_theme_id = game_cntxt->theme_id;
    game_cntxt->theme_id = PRINTING_THEME;
    draw_game( game_cntxt, cr, print_area_width, print_area_height );

    cairo_set_line_width( cr, PRINTING_GAME_LINE_WIDTH );
    cairo_set_source_rgb( cr, PRINTING_GAME_LINE_FG );
    cairo_rectangle( cr, 0.0, 0.0, print_area_width, print_area_height );
    cairo_stroke( cr );

    cairo_select_font_face( cr, CANVAS_FONT, CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_WEIGHT_NORMAL );
    cairo_set_font_size( cr, 12.0 );

    char buffer[ SUDOKU_MAX_NAME_LEN + 32 ];
    sprintf( buffer, "(c)siesta productions - %s", game_cntxt->wname );
    cairo_text_extents_t extents;

    cairo_text_extents( cr, buffer, &extents );
    cairo_move_to( cr, 5.0, 5.0 + print_area_height + extents.height );
    cairo_show_text( cr, buffer );

    game_cntxt->theme_id = previous_theme_id;
}

void button_event( GtkWidget *widget, GdkEventButton *event, gpointer data )
{
    game_cntxt_t *game_cx = (game_cntxt_t *)data;
    double ex = (double)event->x, ey = (double)event->y;
    g_print ("Button pressed - x %g y %g\n", ex, ey );

    if ( ! sudoku_is_selection_possible() ) return;

    double x = 0.0, y = 0.0;
    double width = (double)gtk_widget_get_allocated_width( widget );    // draw area width
    double height = (double)gtk_widget_get_allocated_height( widget );  // draw area height

    if ( game_cx->show_headlines ) {
        x = width / (double)(SUDOKU_N_COLS+1);
        width -= x;
        y = height / (double)(SUDOKU_N_ROWS+1);
        height -= y;
    }

    ex -= x,
    ey -= y;

    int row = -1, col = -1;
    double cell_w = width/SUDOKU_N_COLS, cell_h = height/SUDOKU_N_ROWS;
    GRAPHIC_TRACE( ( "grid x: %g y: %g, w: %g, h: %g, event x: %g, y: %g cell_w: %g, cell_h: %g\n",
                     x, y, width, height, ex, ey, cell_w, cell_h ) );
    for ( int i = 0; i < SUDOKU_N_ROWS; i++ ) {
        if ( ey < (i+1) * cell_h && ey >= i * cell_h ) {
            row = i;
            break;
        }
    }
    if ( -1 == row ) return; // not in any row

    for ( int i = 0; i < SUDOKU_N_COLS; i++ ) {
        if ( ex < (i+1)*cell_w && ex >= i * cell_w ) {
            col = i;
            break;
        }
    }

    if ( row != -1 && col != -1 ) {
        GRAPHIC_TRACE( ("cell found @row %d col %d\n", row, col) );
        sudoku_set_selection( game_cx, row, col );
    }
}

static void choose_options( GtkWidget *wdg, gpointer data ); // forward references
static int ok_to_quit( void *data );
static int do_save_game( game_cntxt_t *game_cntxt );

static gint key_event( GtkWidget *widget, GdkEventKey *event, gpointer data )
{
    game_cntxt_t *game_cntxt = (game_cntxt_t *)data;
    char key_char = event->string[0];
    int key_val = event->keyval;
    sudoku_key_t sudoku_key = SUDOKU_NO_KEY;

#if 0
    char *str;
    switch( event->type ) {
    case GDK_KEY_PRESS: 
        str = "GDK_KEY_PRESS"; break;
    case GDK_KEY_RELEASE:
        str = "GDK_KEY_RELEASE"; break;
    default:
        str = "???????????"; break;
    }

    if ( key_char ) {
        printf( "Key event -  widget %p keytype %s keyval 0x%08x char %c data %p\n",
                (void *)widget, str, key_val, key_char, (void *)data );
    } else {
        printf( "Key event -  widget %p keytype %s keyval 0x%08x char <0> data %p\n",
                (void *)widget, str, key_val, (void *)data );
    }
#endif
    if( game_cntxt->canvas != widget ) return FALSE;
    //printf("widget is canvas\n");
    if ( /*GDK_KEY_PRESS*/GDK_KEY_RELEASE == event->type ) {
        switch( key_char ) {
        default:
            GRAPHIC_TRACE( ( "Got key released on window, Ascii code 0x%x (%c)\n",
                    key_char, key_char ) );
            if ( key_char >= 0x31 && key_char <= 0x39 )
                sudoku_enter_symbol( game_cntxt, key_char );
            return TRUE;

        case 0:
            switch( key_val ) {
            case GDK_KEY_Up:
                sudoku_key = SUDOKU_UP_ARROW; break;
            case GDK_KEY_Down:
                sudoku_key = SUDOKU_DOWN_ARROW; break;

            case GDK_KEY_Left:
                sudoku_key = SUDOKU_LEFT_ARROW; break;
            case GDK_KEY_Right:
                sudoku_key = SUDOKU_RIGHT_ARROW; break;

            case GDK_KEY_Page_Up:
                sudoku_key = SUDOKU_PAGE_UP; break;
            case GDK_KEY_Page_Down:
                sudoku_key = SUDOKU_PAGE_DOWN; break;

            case GDK_KEY_Home:
                sudoku_key = SUDOKU_HOME_KEY; break;
            case GDK_KEY_End:
                sudoku_key = SUDOKU_END_KEY; break;

            case GDK_KEY_Delete:
                sudoku_erase_selection( (void *)game_cntxt );
                return TRUE;
            default:
                sudoku_key = SUDOKU_NO_KEY; break;
            }
            sudoku_move_selection( (void *)game_cntxt, sudoku_key );
            return TRUE;

        /* English short cuts */
        case 's': case 'S':
#if SUDOKU_GRAPHIC_DEBUG
            printf("Save!\n");
#endif
            do_save_game( game_cntxt );
            return TRUE;
        case 'q': case 'Q': case 'x': case 'X':
#if SUDOKU_GRAPHIC_DEBUG
            printf("Quit!\n");
#endif
            if ( ok_to_quit( (void *)game_cntxt ) ) gtk_main_quit ();
            return TRUE;
        case 'u': case 'U':
#if SUDOKU_GRAPHIC_DEBUG
            printf("Undo!\n");
#endif
            sudoku_undo( (void *)game_cntxt );
            return TRUE;
        case 'm': case 'M':
#if SUDOKU_GRAPHIC_DEBUG
            printf("Mark!\n");
#endif
            sudoku_mark_state( (void *)game_cntxt );
            return TRUE;
        case 'b': case 'B':
#if SUDOKU_GRAPHIC_DEBUG
            printf("Back!\n");
#endif
            sudoku_back_to_mark( (void *)game_cntxt );
            return TRUE;
        case 'r': case 'R':
#if SUDOKU_GRAPHIC_DEBUG
            printf("Redo!\n");
#endif
            sudoku_redo( (void *)game_cntxt );
            return TRUE;
        case 't': case 'T':
#if 1 //SUDOKU_GRAPHIC_DEBUG
            printf( "Execute 1 step\n");
#endif
            sudoku_step( (void *)game_cntxt );
            return TRUE;
        case 'z': case 'Z':
#if SUDOKU_GRAPHIC_DEBUG
            printf("Erase!\n");
#endif
            sudoku_erase_selection( (void *)game_cntxt );
            return TRUE;
        case 'c': case 'C':
#if SUDOKU_GRAPHIC_DEBUG
            printf("Check!\n");
#endif
            sudoku_check_from_current_position( (void *)game_cntxt );
            return 0;
        case 'd': case 'D':
#if SUDOKU_GRAPHIC_DEBUG
            printf("Do solve it!\n");
#endif
            sudoku_solve_from_current_position( (void *)game_cntxt );
            return TRUE;
        case 'h': case 'H':
#if SUDOKU_GRAPHIC_DEBUG
            printf("Hint!\n");
#endif
            sudoku_hint( (void *)game_cntxt );
            return TRUE;
        case 'f': case 'F':
#if SUDOKU_GRAPHIC_DEBUG
            printf("Fill!\n");
#endif
            sudoku_fill( (void *)game_cntxt, game_cntxt->remove_fill_state );
            return TRUE;
    case 'o': case 'O':
#if SUDOKU_GRAPHIC_DEBUG
            printf("Options!\n");
#endif
            choose_options( game_cntxt->tools_items[SUDOKU_OPTION_ITEM], (void *)game_cntxt );
            return TRUE;
        }
    }
    return FALSE;
}

// ----- begin of ui functions (exported to sudoku thru function table)

void ui_redraw_game( void *cntxt )
{
    game_cntxt_t *game_cx = (game_cntxt_t *)cntxt;
    gtk_widget_queue_draw( game_cx->canvas );
}

static void ui_update_window_name( void *cntxt, const char *path )
{
    game_cntxt_t *game_cntxt = (game_cntxt_t *)cntxt;
    const char *last_slash;
    size_t len;

    assert(path);
    assert(game_cntxt);
//printf("sudoku_update_window_name: %s\n", path );
    last_slash = strrchr( path, '/' );
    if ( NULL == last_slash )
        last_slash = path;

    len = strlen( last_slash );
    assert(len < SUDOKU_MAX_NAME_LEN);

    strncpy( game_cntxt->wname, last_slash, SUDOKU_MAX_NAME_LEN );
    gtk_window_set_title( GTK_WINDOW(game_cntxt->window), last_slash );
}
/*
char *get_window_name( void *cntxt )
{
  game_cntxt_t *game_cntxt = (game_cntxt_t *)cntxt;
  return game_cntxt->wname;
}
*/
void ui_set_status_line( void *cntxt, sudoku_status_t status, int value )
{
    game_cntxt_t *game_cx = (game_cntxt_t *)cntxt;

    char status_buffer[ MAX_STATUS_MESSAGE_LEN ];
    if ( game_cx->status ) {

    switch( status ) {
    case SUDOKU_STATUS_BLANK:             /* erase previous status line */
      status_buffer[0] = '\0';
      break;
    case SUDOKU_STATUS_DUPLICATE:
      strcpy( status_buffer, "Duplicate symbol");
      break;
    case SUDOKU_STATUS_MARK:
      sprintf( status_buffer, "Mark #%d", value );
      break;
    case SUDOKU_STATUS_BACK:
      sprintf( status_buffer, "Back to Mark #%d", value );
      break;
    case SUDOKU_STATUS_CHECK:
      sprintf( status_buffer, "%sossible", (value) ? "P": "Imp");
      break;
    case SUDOKU_STATUS_HINT:
      switch( (sudoku_hint_type)value ) {
      case NO_HINT:             strcpy( status_buffer, "No hint"); break;
      case NO_SOLUTION:         strcpy( status_buffer, "No solution: undo first"); break;
      case NAKED_SINGLE:        strcpy( status_buffer, "Naked Single at selection"); break;
      case HIDDEN_SINGLE:       strcpy( status_buffer, "Hidden Single at selection");  break;
      case LOCKED_CANDIDATE:    strcpy( status_buffer, "locked candidate"); break;
      case NAKED_SUBSET:        strcpy( status_buffer, "Naked Subset"); break;
      case HIDDEN_SUBSET:       strcpy( status_buffer, "Hidden Subset"); break;
      case XWING:               strcpy( status_buffer, "X-Wing"); break;
      case SWORDFISH:           strcpy( status_buffer, "Swordfish"); break;
      case JELLYFISH:           strcpy( status_buffer, "Jellyfish"); break;
      case XY_WING:             strcpy( status_buffer, "XY-Wing"); break;
      case CHAIN:               strcpy( status_buffer, "Exclusion Chain"); break;
      }
      break;
    case SUDOKU_STATUS_NO_SOLUTION:       /* printf("No solution") */
      strcpy( status_buffer, "No solution");
      break;
    case SUDOKU_STATUS_ONE_SOLUTION_ONLY: /* printf("Only ONE solution") */
      strcpy( status_buffer, "Only One solution");
      break;
    case SUDOKU_STATUS_SEVERAL_SOLUTIONS: /* printf("More than one solution") */
      strcpy( status_buffer, "More than one solution");
      break;
    }
    GRAPHIC_TRACE( ("ready to set status text: %s\n", status_buffer) );

    /* GTK: set label text */
    gtk_label_set_text( GTK_LABEL(game_cx->status), (const char *)status_buffer );
  }
}

static void ui_set_mark_n_back_level( void *cntxt, int level )
{
    char buffer[MAX_MARK_LEVEL_SIZE];
    game_cntxt_t *game_cntxt = (game_cntxt_t *)cntxt;
    GtkWidget *back = game_cntxt->edit_items[SUDOKU_BACK_ITEM];
    GtkWidget *menu_label = gtk_bin_get_child( GTK_BIN(back) );

    printf("mark_n_back: level %d\n", level );
    sprintf( buffer, "Mark #%d", 1 + level );
    gtk_label_set_text(GTK_LABEL(menu_label), buffer);

    menu_label = gtk_bin_get_child(GTK_BIN(back));
    if ( 0 == level ) {
        gtk_label_set_text_with_mnemonic(GTK_LABEL(menu_label), "_Back");
    } else {
        sprintf( buffer, "_Back #%d", level );
        gtk_label_set_text_with_mnemonic(GTK_LABEL(menu_label), buffer);
  }
}

static void ui_change_enter_item( void *cntxt, sudoku_mode_t mode )
{
    char *item_name;
    game_cntxt_t *game_cntxt = (game_cntxt_t *)cntxt;
    GtkWidget *enter = game_cntxt->file_items[SUDOKU_ENTER_ITEM];
    GtkWidget *menu_label = gtk_bin_get_child( GTK_BIN(enter) );

    switch ( mode ) {
    case SUDOKU_ENTER_GAME:
        item_name = "_Enter your game";
        break;

    case SUDOKU_CANCEL_GAME:
        item_name = "_Cancel this game";
        break;

    case SUDOKU_COMMIT_GAME:
        item_name = "_Accept this game";
        break;

    default:
        assert(0);
    break;
    }

    GRAPHIC_TRACE( ("Enter mode: %s (%d)", item_name, mode ) );
    gtk_label_set_text_with_mnemonic(GTK_LABEL(menu_label), item_name );
}

static void set_menu_item( game_cntxt_t *game_cntxt, GtkWidget *menu_item,
                           sudoku_menu_t which_menu, int which_item )
{
    assert( which_menu >= SUDOKU_MENU_START && which_menu < SUDOKU_MENU_BEYOND );
    switch ( which_menu ) {
    case SUDOKU_FILE_MENU:
        assert( which_item >= SUDOKU_FILE_START && which_item < SUDOKU_FILE_BEYOND );
        GRAPHIC_TRACE( ("setting menu item %p @SUDOKU_FILE_MENU, %d\n", (void *)menu_item, which_item) );
        game_cntxt->file_items[which_item] = menu_item;
        break;
    case SUDOKU_EDIT_MENU:
        assert( which_item >= SUDOKU_EDIT_START && which_item < SUDOKU_EDIT_BEYOND );
        GRAPHIC_TRACE( ("setting menu item %p @SUDOKU_EDIT_MENU, %d\n", (void *)menu_item, which_item) );
        game_cntxt->edit_items[which_item] = menu_item;
        break;
    case SUDOKU_TOOL_MENU:
        assert( which_item >= SUDOKU_TOOL_START && which_item < SUDOKU_TOOL_BEYOND );
        GRAPHIC_TRACE( ("setting menu item %p @SUDOKU_TOOL_MENU, %d\n", (void *)menu_item, which_item) );
        game_cntxt->tools_items[which_item] = menu_item;
        break;
    default:
        printf("Bad menu %d\n", which_menu );
        exit(1);
    }
}

static GtkWidget *get_menu_item(game_cntxt_t *game_cntxt,
                                sudoku_menu_t which_menu, int which_item )
{
    assert( which_menu >= SUDOKU_MENU_START && which_menu < SUDOKU_MENU_BEYOND );
    switch ( which_menu ) {
    case SUDOKU_FILE_MENU:
        assert( which_item >= SUDOKU_FILE_START && which_item < SUDOKU_FILE_BEYOND );
        return game_cntxt->file_items[which_item];
        //break;
    case SUDOKU_EDIT_MENU:
        assert( which_item >= SUDOKU_EDIT_START && which_item < SUDOKU_EDIT_BEYOND );
        return game_cntxt->edit_items[which_item];
        //break;
    case SUDOKU_TOOL_MENU:
        assert( which_item >= SUDOKU_TOOL_START && which_item < SUDOKU_TOOL_BEYOND );
        return game_cntxt->tools_items[which_item];
        //break;
    default:
        printf("Bad menu %d\n", which_menu );
        exit(1);
    }
}

static void ui_disable_menu_item( void *cntxt, sudoku_menu_t which_menu, int which_item )
{
    game_cntxt_t *game_cntxt = (game_cntxt_t *)cntxt;
    GtkWidget *wdg = get_menu_item( game_cntxt, which_menu, which_item );
    GRAPHIC_TRACE( ("ui_disable_menu_item: menu %d, item %d, widget %p\n", which_menu, which_item, (void *)wdg) );
    gtk_widget_set_sensitive( wdg, FALSE );
}

static void ui_enable_menu_item( void *cntxt, sudoku_menu_t which_menu, int which_item )
{
    game_cntxt_t *game_cntxt = (game_cntxt_t *)cntxt;
    GtkWidget *wdg = get_menu_item( game_cntxt, which_menu, which_item );
    GRAPHIC_TRACE( ("ui_enable_menu_item: menu %d, item %d, widget %p\n", which_menu, which_item, (void *)wdg) );
    gtk_widget_set_sensitive( wdg, TRUE );
}

static void ui_disable_menu( void *cntxt, sudoku_menu_t which )
{
    assert( which >= SUDOKU_MENU_START && which < SUDOKU_MENU_BEYOND );

    game_cntxt_t *game_cx = (game_cntxt_t *)cntxt;
    switch ( which ) {
    case SUDOKU_FILE_MENU:
        GRAPHIC_TRACE( ("ui_disable_menu: menu SUDOKU_FILE_MENU, widget %p\n", (void *)game_cx->file_items[0]) );
        for ( int item = SUDOKU_FILE_START; item < SUDOKU_FILE_BEYOND; item ++ )
            gtk_widget_set_sensitive( game_cx->file_items[item], FALSE );
        break;
    case SUDOKU_EDIT_MENU:
        GRAPHIC_TRACE( ("ui_disable_menu: menu SUDOKU_EDIT_MENU, widget %p\n", (void *)game_cx->edit_items[0]) );
        for ( int item = SUDOKU_EDIT_START; item < SUDOKU_EDIT_BEYOND; item ++ )
            gtk_widget_set_sensitive( game_cx->edit_items[item], FALSE );
        break;
    case SUDOKU_TOOL_MENU:
        GRAPHIC_TRACE( ("ui_disable_menu: menu SUDOKU_TOOL_MENU, widget %p\n", (void *)game_cx->tools_items[0]) );
        for ( int item = SUDOKU_TOOL_START; item < SUDOKU_TOOL_BEYOND; item ++ )
            gtk_widget_set_sensitive( game_cx->tools_items[item], FALSE );
        break;
    default:
        exit_error_with_int_arg( "Bad menu while disabling", which );
  }
}

static void ui_enable_menu( void *cntxt, sudoku_menu_t which )
{
    assert( which >= SUDOKU_MENU_START && which < SUDOKU_MENU_BEYOND );

    game_cntxt_t *game_cx = (game_cntxt_t *)cntxt;
    switch ( which ) {
    case SUDOKU_FILE_MENU:
        GRAPHIC_TRACE( ("ui_enable_menu: menu SUDOKU_FILE_MENU, widget %p\n", (void *)game_cx->file_items[0]) );
        for ( int item = SUDOKU_FILE_START; item < SUDOKU_FILE_BEYOND; item ++ )
            gtk_widget_set_sensitive( game_cx->file_items[item], TRUE );
        break;
    case SUDOKU_EDIT_MENU:
        GRAPHIC_TRACE( ("ui_enable_menu: menu SUDOKU_EDIT_MENU, widget %p\n", (void *)game_cx->edit_items[0]) );
        for ( int item = SUDOKU_EDIT_START; item < SUDOKU_EDIT_BEYOND; item ++ )
            gtk_widget_set_sensitive( game_cx->edit_items[item], TRUE );
        break;
    case SUDOKU_TOOL_MENU:
        GRAPHIC_TRACE( ("ui_enable_menu: menu SUDOKU_TOOL_MENU, widget %p\n", (void *)game_cx->tools_items[0]) );
        for ( int item = SUDOKU_TOOL_START; item < SUDOKU_TOOL_BEYOND; item ++ )
            gtk_widget_set_sensitive( game_cx->tools_items[item], TRUE );
        break;
    default:
        exit_error_with_int_arg( "Bad menu while enabling", which );
    }
}

void ui_success_dialog( void *data, sudoku_duration_t *dhms )
{
    game_cntxt_t *game_cntxt = (game_cntxt_t *)data;
    GtkWidget *restart = create_restart_dialog( GTK_WINDOW(game_cntxt->window),
                                                (game_cntxt->timed_game_state) ? dhms : NULL );
    if ( restart ) {
        int result = gtk_dialog_run( GTK_DIALOG( restart ) );
        gtk_widget_destroy( restart );
        switch ( result ) {
        case GTK_RESPONSE_OK:
            sudoku_random_game( data );
            manage_displaying_time( game_cntxt );
        default:
            break;
        }
    }
}

// ----- end of ui functions

static int ok_to_stop_current_game( game_cntxt_t *game_cntxt )
{
    game_state_t state = STOPPED;

    if ( sudoku_is_game_on_going( ) || sudoku_is_entering_valid_game( ) ) {
        state = PLAYING;
    } else if ( sudoku_is_entering_game_on_going( ) ) {
        state = ENTERING;
    }

    if ( STOPPED != state ) {
        GtkWidget *stop = create_stop_dialog( GTK_WINDOW(game_cntxt->window), state );
        if ( stop ) {
            int result = gtk_dialog_run( GTK_DIALOG( stop ) );
            gtk_widget_destroy(stop);

            if ( GTK_RESPONSE_OK == result ) return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

static void new_game( GtkWidget *wdg, gpointer data )
{
    (void)wdg;

    GRAPHIC_TRACE( ("new_game - data %p\n", data) );
    game_cntxt_t *game_cntxt = (game_cntxt_t *)data;  
    if ( ok_to_stop_current_game( game_cntxt ) ) {
        sudoku_random_game( (void *)data );
        manage_displaying_time( game_cntxt );
    }
}

static void pick_game( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    game_cntxt_t *game_cntxt = (game_cntxt_t *)data;

    GRAPHIC_TRACE( ("pick_game - data %p\n", data) );
    if ( ok_to_stop_current_game( game_cntxt ) ) {
        GtkWidget *pick = create_pick_dialog( GTK_WINDOW( game_cntxt->window ) );
        if ( pick ) {
            const char *input;
            int result = gtk_dialog_run( GTK_DIALOG( pick ) );
            switch ( result ) {
            case GTK_RESPONSE_OK:
                input = get_widget_entry( pick );
                assert( input );
                GRAPHIC_TRACE( ("pick_game - returned ok game = %s\n", input) );
                sudoku_pick_game( (void *)data, input );
                manage_displaying_time( game_cntxt );
                break;
            default:
                GRAPHIC_TRACE( ("pick_game - cancelled\n") );
            break;
            }
            gtk_widget_destroy( pick );
        }
    }
}

static int was_a_game_saved_or_loaded = FALSE;

static void open_game( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    game_cntxt_t *game_cntxt = (game_cntxt_t *)data;

    GRAPHIC_TRACE( ("open_game - data %p\n", data) );
    if ( ok_to_stop_current_game( game_cntxt ) ) {
        GtkWidget *dialog = gtk_file_chooser_dialog_new ( "Open File",
                                                          GTK_WINDOW( game_cntxt->window ),
                                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                                          "_Cancel", GTK_RESPONSE_CANCEL,
                                                          "_Open", GTK_RESPONSE_ACCEPT,
                                                          NULL);
        if ( dialog ) {
            if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
                char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
                sudoku_open_file( game_cntxt, filename );
                was_a_game_saved_or_loaded = TRUE;
                g_free (filename);
            }
            gtk_widget_destroy (dialog);
        }
    }
}

static void enter_or_commit( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    game_cntxt_t *game_cntxt = (game_cntxt_t *)data;

    GRAPHIC_TRACE( ("make game - data %p\n", data) );
    if ( sudoku_is_entering_valid_game() ) {
        GtkWidget *commit = create_commit_dialog( GTK_WINDOW( game_cntxt->window ) );
        if ( commit ) {
            int result = gtk_dialog_run( GTK_DIALOG( commit ) );
            const char *input;
            switch ( result ) {
            case GTK_RESPONSE_OK:
                input = get_widget_entry( commit );
                assert( input );
                GRAPHIC_TRACE( ("pick_game - returned ok game = %s\n", input) );
                sudoku_commit_game( (void *)data, input );
                break;
            default:
                GRAPHIC_TRACE( ("commit_game - cancelled\n") );
                break;
            }
            gtk_widget_destroy( commit );
        }
    } else {
        sudoku_toggle_entering_new_game( (void *)data );
    }
}

static int do_save_game( game_cntxt_t *game_cntxt )
{
    int result = FALSE;
    GtkWidget *dialog = gtk_file_chooser_dialog_new( "Save Game",
                                                     GTK_WINDOW( game_cntxt->window ),
                                                     GTK_FILE_CHOOSER_ACTION_SAVE,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     "_Save", GTK_RESPONSE_ACCEPT,
                                                     NULL );
    if ( dialog ) {
        gtk_file_chooser_set_do_overwrite_confirmation( GTK_FILE_CHOOSER( dialog ), TRUE );
        gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER (dialog), game_cntxt->wname );

        if (gtk_dialog_run( GTK_DIALOG( dialog ) ) == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ) );
            sudoku_save_file( game_cntxt, filename );
            was_a_game_saved_or_loaded = result = TRUE;
            g_free (filename);
        }
        gtk_widget_destroy (dialog);
    }
    return result;
}

static void save_game( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    game_cntxt_t *game_cntxt = (game_cntxt_t *)data;

    GRAPHIC_TRACE( ("Saving in the current directory: './%s'\n", game_cntxt->wname) );
    do_save_game( game_cntxt );
}

static GtkPrintSettings *settings = NULL; // TODO: move to game_cntxt_t
static void print_game( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    game_cntxt_t *game_cntxt = (game_cntxt_t *)data;
    GtkPrintOperation *print;
    GtkPrintOperationResult res;

    GRAPHIC_TRACE( ("print game - data %p\n", data) );
    print = gtk_print_operation_new( );

    if (settings != NULL) 
        gtk_print_operation_set_print_settings( print, settings );

    g_signal_connect (print, "begin_print", G_CALLBACK (begin_print), (gpointer)game_cntxt );
    g_signal_connect (print, "draw_page", G_CALLBACK (draw_page), (gpointer)game_cntxt );

    res = gtk_print_operation_run( print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
                                   GTK_WINDOW( game_cntxt->window ), NULL );

    if ( res == GTK_PRINT_OPERATION_RESULT_APPLY ) {
        if ( settings != NULL )
            g_object_unref( settings );
        settings = g_object_ref( gtk_print_operation_get_print_settings (print) );
    }

    g_object_unref( print );
}

static void print_setup( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    printf("print setup - data %p\n", data );
    // TODO: implement...
}

static int ok_to_quit( void *data )
{
    game_cntxt_t *game_cntxt = (game_cntxt_t *)data;
    game_state_t state = STOPPED;

    if ( sudoku_is_game_on_going( ) || sudoku_is_entering_valid_game( ) ) {
        state = PLAYING;
    } else if ( sudoku_is_entering_game_on_going( ) ) {
        state = ENTERING;
    }

    if ( STOPPED != state ) {
        GtkWidget *exit= create_exit_dialog( GTK_WINDOW(game_cntxt->window), state );
        if ( exit ) {
            int result = gtk_dialog_run( GTK_DIALOG( exit ) );
            gtk_widget_destroy( exit );
            switch ( result ) {
            case GTK_RESPONSE_CLOSE:
                if ( ! do_save_game( game_cntxt ) )  break;
                // else falls through
            case GTK_RESPONSE_OK:
                return TRUE;
            default:
                break;
            }
        }
        return FALSE;
    }
    return TRUE;
}

static void quit_game( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    GRAPHIC_TRACE( ("quit_game - data %p\n", data) );
    if ( ok_to_quit( (void *)data ) ) gtk_main_quit ();
}

static gint delete_event( GtkWidget *widget, GdkEvent *event, gpointer data )
{
    (void)widget;
    (void)event;
    if ( ok_to_quit( (void *)data ) ) {
        gtk_main_quit ();
        return FALSE;
    }
    return TRUE;
}

static void undo_move( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    GRAPHIC_TRACE( ("undo_move - data %p\n", data ) );
    sudoku_undo( (void *)data );
}

static void redo_move( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    GRAPHIC_TRACE( ("redo_move - data %p\n", data ) );
    sudoku_redo( (void *)data );
}

static void erase_move( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    GRAPHIC_TRACE( ("erase_move - data %p\n", data ) );
    sudoku_erase_selection( (void *)data );
}

static void mark_state( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    GRAPHIC_TRACE( ("mark_state - data %p\n", data) );
    sudoku_mark_state( (void *)data );
}

static void back_to_state( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    GRAPHIC_TRACE( ("back_to_state - data %p\n", data) );
    sudoku_back_to_mark( (void *)data );
}

static void check_move( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    GRAPHIC_TRACE( ("check_move - data %p\n", data) );
    sudoku_check_from_current_position( (void *)data );
}

static void hint_next( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    GRAPHIC_TRACE( ("hint_next - data %p\n", data) );
    sudoku_hint( (void *)data );
}

static void fill_selection( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    GRAPHIC_TRACE( ("fill_selection - data %p\n", data) );
    game_cntxt_t *game_cntxt = ( game_cntxt_t *)data;
    sudoku_fill( (void *)data, game_cntxt->remove_fill_state );
}

static void fill_all( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    GRAPHIC_TRACE( ("fill_all - data %p\n", data) );
    game_cntxt_t *game_cntxt = ( game_cntxt_t *)data;
    sudoku_fill_all( (void *)data, game_cntxt->remove_fill_state );
}

static void solve_game( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    GRAPHIC_TRACE( ("solve_game - data %p\n", data) );
    sudoku_solve_from_current_position( (void *)data );
}

static void conflict_detect( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
#if GRAPHIC_DEBUG
    gboolean state = gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM( wdg ) );
    GRAPHIC_TRACE( ("conflict_detect - state %d data %p\n", state, data) );
#endif
    sudoku_toggle_conflict_detection( (void *)data );
}

static void auto_check( GtkWidget *wdg, gpointer data )
{
    (void)wdg;
    gboolean state = gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM( wdg ) );
    printf("auto_check - state %d data %p\n", state, data );
    sudoku_toggle_auto_checking( (void *)data );
}

static void display_time_if_needed( game_cntxt_t *cntxt, bool new_state )
{
    bool current_state = cntxt->display_time_state;
    if ( current_state == new_state )
        return;

    cntxt->display_time_state = new_state;
    manage_displaying_time( cntxt );
}

static void choose_options( GtkWidget *wdg, gpointer data )
{
    GRAPHIC_TRACE( ("choose_options - data %p\n", data) );

    (void)wdg;
    game_cntxt_t *cntxt = ( game_cntxt_t *)data;

    assert( 0 != cntxt->theme_id );
    game_option_t options;
    options.theme_id = cntxt->theme_id;
    options.translucent_state = cntxt->translucent_state;
    options.remove_fill_state = cntxt->remove_fill_state;
    options.timed_game_state = cntxt->timed_game_state;
    options.display_time_state = cntxt->display_time_state;

    GtkWidget *option_dialog = create_options_dialog( GTK_WINDOW(cntxt->window), &options );
    if ( option_dialog ) {
        int result = gtk_dialog_run( GTK_DIALOG( option_dialog ) );
        switch ( result ) {
        case GTK_RESPONSE_OK:
            GRAPHIC_TRACE( ("Options changed\n") );
            if ( 1 == gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( options.theme_2 ) ) ) {
                cntxt->theme_id = PAPER_THEME;
            } else if ( 1 == gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( options.theme_3 ) ) ) {
                cntxt->theme_id = IMAGE_THEME;
            } else {
            	cntxt->theme_id = CHALKBOARD_THEME;
            }
            cntxt->translucent_state = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( options.translucent ) );
            cntxt->remove_fill_state = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( options.remove_fill ) );
            cntxt->timed_game_state = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( options.timed_game ) );

            if ( cntxt->timed_game_state ) {
                display_time_if_needed( cntxt,
                                        gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( options.display_time ) ) );
            } else {
                display_time_if_needed( cntxt, FALSE );
            }
            ui_redraw_game( cntxt );

            /* save new options */
            update_options( cntxt );

        default:
            break;
        }
        gtk_widget_destroy( option_dialog );
    }
}

// ------ menu and window creation

typedef void (*action_t)( GtkWidget *wdg, gpointer data );

typedef enum {
    no_check_box = -1, unchecked, checked
} check_box_t;

typedef struct {
    char        *name;
    char        *short_cut;
    action_t    action;
    check_box_t check_box;
} item_t;

typedef struct {
    char        *name;
    int         n_items;
    item_t      *items;
} menu_t;

typedef struct {
    int         n_menus;
    menu_t      *menus;
} menus_t;

static GtkWidget * init_window_menus( game_cntxt_t *game_cx )
{
    // FIXME: short_cuts in items below are never used - replace with better support
    item_t file_items[] = {
        { "_New", "n", new_game, no_check_box },
        { "_Pick", "p", pick_game, no_check_box },
        { "_Open", "o", open_game, no_check_box },
        { "Make Your _Game", "g", enter_or_commit, no_check_box },
        { "_Save as", "s", save_game, no_check_box },
        { NULL, NULL, NULL, no_check_box },
        { "Print", NULL, print_game, no_check_box },
        { "Print Setup", NULL, print_setup, no_check_box },
        { NULL, NULL, NULL, no_check_box },
        { "_Quit", "q", quit_game, no_check_box } };

    item_t edit_items[] = {
        { "_Undo", "u", undo_move, no_check_box},
        { "_Redo", "r", redo_move, no_check_box},
        { "_Erase", "z", erase_move, no_check_box },  // TODO: check the mnemonic 'e'
        { NULL, NULL, NULL, no_check_box },
        { "_Mark", "m", mark_state, no_check_box },
        { "_Back", "b", back_to_state, no_check_box } };

    item_t tools_items[] = {
        { "_Check", "c", check_move, no_check_box },
        { "_Hint", "h", hint_next, no_check_box },
        { "_Fill", "f", fill_selection, no_check_box},
        { "Fill All", "", fill_all, no_check_box},
        { "_Do Solve", "d", solve_game, no_check_box },
        { NULL, NULL, NULL, no_check_box },
        { "Conflict detection", NULL, conflict_detect, checked },  // checked by default
        { "Check after each change", NULL, auto_check, unchecked}, // unchecked by default
        { "_options", "o", choose_options, no_check_box } } ;

    menu_t menus[] = { 
        { "_File", sizeof( file_items ) / sizeof( item_t ), file_items },
        { "_Edit", sizeof( edit_items ) / sizeof( item_t ), edit_items },
        { "_Tools", sizeof( tools_items ) / sizeof( item_t ), tools_items } };

    menus_t sudoku_menus = { sizeof( menus ) / sizeof( menu_t ), menus }; 

#define TOOLS_MENU_ID 2             // 3rd starting from 0
#define TOOLS_CONFLICT_ITEM_ID 3    // excluding empty separators

    GtkWidget *menu_bar = gtk_menu_bar_new();
    for ( int i = 0; i < sudoku_menus.n_menus; ++i ) {
        GtkWidget *mh = gtk_menu_item_new_with_mnemonic( sudoku_menus.menus[i].name );
        gtk_widget_show( mh );
        gtk_menu_shell_append(GTK_MENU_SHELL( menu_bar ), mh);
        GtkWidget *mn = gtk_menu_new( );
        gtk_menu_item_set_submenu( GTK_MENU_ITEM( mh ), mn );

        int k = 0;
        for ( int j = 0; j < sudoku_menus.menus[i].n_items; ++j ) {
            GtkWidget *mi;
            if ( NULL == sudoku_menus.menus[i].items[j].name ) {
                mi = gtk_separator_menu_item_new();
            } else if ( sudoku_menus.menus[i].items[j].check_box != no_check_box ) {
                mi = gtk_check_menu_item_new_with_label( sudoku_menus.menus[i].items[j].name );
                gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM( mi ),
                                                sudoku_menus.menus[i].items[j].check_box );
            } else {
                mi = gtk_menu_item_new_with_mnemonic( sudoku_menus.menus[i].items[j].name );
            }
            gtk_widget_show( mi );
            if ( NULL != sudoku_menus.menus[i].items[j].action ) {
                g_signal_connect( G_OBJECT( mi ), "activate",
                                  G_CALLBACK( sudoku_menus.menus[i].items[j].action ),
                                  (gpointer)game_cx );
                set_menu_item( game_cx, mi, (sudoku_menu_t)i, k++ );
            }
            gtk_menu_shell_append(GTK_MENU_SHELL(mn), mi);
        }
    }
    return menu_bar;
}

static void set_alpha_channel(GtkWidget *widget)
{
    /* To check if the display supports alpha channels, get the colormap */
  GdkScreen *screen = gtk_widget_get_screen(widget);
  GdkVisual *visual = gdk_screen_get_rgba_visual( screen );

  if (! visual) {
    printf("Screen does not support alpha channels!\n");
    visual = gdk_screen_get_system_visual( screen );
  }
  gtk_widget_set_visual(widget, visual);
}

static void setup_sudoku_window( void *instance )
{
    game_cntxt_t *game_cntxt = (game_cntxt_t *)instance;

    /* Sets the border width of the window. */
    gtk_container_set_border_width( GTK_CONTAINER(game_cntxt->window), 0 );

    /* We create a box to pack widgets into. */
    GtkWidget *box = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );

    /* Put the box into the main window. */
    gtk_container_add( GTK_CONTAINER(game_cntxt->window), box );

    /* create apps menus */
    GtkWidget *menu = init_window_menus( game_cntxt );
    if ( NULL == menu ) {
        exit_error(" Unable to create menu");
    }
    gtk_widget_show( menu );
    gtk_box_pack_start( GTK_BOX(box), menu, FALSE, FALSE, 0 );

//    gtk_widget_set_app_paintable( game_cntxt->window, TRUE );
    set_alpha_channel( game_cntxt->window );

    /* create canvas */
    game_cntxt->canvas = gtk_drawing_area_new();
    set_alpha_channel(game_cntxt->canvas);
    // to update content in canvas call gtk_widget_queue_draw(game_cntxt->canvas);

    /* set drawing area size */
    gtk_widget_set_size_request( game_cntxt->canvas,
                                 SUDOKU_GRID_WIDTH, SUDOKU_GRID_HEIGHT );

    g_signal_connect( G_OBJECT( game_cntxt->canvas ), "draw", G_CALLBACK(draw_cb), (void *)game_cntxt );
    gtk_widget_set_can_focus( game_cntxt->canvas, true );

    g_signal_connect( G_OBJECT( game_cntxt->canvas ), "button_press_event",
                      G_CALLBACK( button_event ), game_cntxt );
    g_signal_connect( G_OBJECT( game_cntxt->canvas ), "key_release_event",
                      G_CALLBACK( key_event ), game_cntxt );

    gtk_widget_set_events( game_cntxt->canvas, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK );

    gtk_box_pack_start (GTK_BOX(box), game_cntxt->canvas, TRUE, TRUE, 0);
    gtk_widget_show( game_cntxt->canvas );

    /* create info and status (bottom) */
    GtkWidget *hbox = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 0 );

    GtkWidget *frame = gtk_frame_new( NULL );
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_OUT );
    GtkWidget *info = gtk_label_new( "Siesta Productions" );
    gtk_label_set_xalign( GTK_LABEL(info), 0.0 );
    gtk_label_set_yalign( GTK_LABEL(info), 0.5 );
    gtk_container_add( GTK_CONTAINER( frame), info);
    game_cntxt->info = info;
    gtk_widget_show( info );

    gtk_box_pack_start( GTK_BOX(hbox), frame, TRUE, TRUE, 0 );
    gtk_widget_show( frame );

    frame = gtk_frame_new( NULL );
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_OUT );
    GtkWidget *status = gtk_label_new( " " );
    gtk_label_set_xalign( GTK_LABEL(status), 0.0 );
    gtk_label_set_yalign( GTK_LABEL(status), 0.5 );
    gtk_container_add( GTK_CONTAINER( frame), status);
    game_cntxt->status = status;
    gtk_widget_show( status );

    gtk_box_pack_start( GTK_BOX(hbox), frame, TRUE, TRUE, 0 );
    gtk_widget_show( frame );

    gtk_box_pack_start( GTK_BOX(box), hbox, FALSE, FALSE, 0 );
    gtk_widget_show( hbox );

    gtk_widget_show( box );

    /* center window */
    gtk_window_set_position( GTK_WINDOW (game_cntxt->window), GTK_WIN_POS_CENTER );

    /* Here we just set a handler for delete_event that immediately exits GTK. */
    g_signal_connect( G_OBJECT (game_cntxt->window), "delete_event",
                      G_CALLBACK (delete_event), game_cntxt );

    gtk_widget_show(game_cntxt->window);
}

static void *init_window_system( int *argcp, char **argv, char *app_name )
{
    static game_cntxt_t game_context;

    /* This is called in all GTK applications. Arguments are parsed
     * from the command line and are returned to the application. */

    gtk_init(argcp, &argv);

    /* create a new window */
    game_context.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    if ( NULL == game_context.window ) {
        printf("Sudoku:gtk: unable to create top level window - exiting\n");
        exit(1);
    }
    gtk_window_set_title( GTK_WINDOW (game_context.window), app_name );
    return (void *)(&game_context);
}

int main( int argc, char *argv[] )
{
    void *instance = init_window_system( &argc, argv, SUDOKU_DEFAULT_NAME );

    if ( 0 != read_options( instance ) ) {
        printf("sudoku: error reading options: use default\n");
    }

    game_cntxt_t *game_cx = (game_cntxt_t *)instance;
    if ( ! initialize_paths( game_cx, argv[0] ) || ! initialize_background_image( game_cx ) ) {
        exit(1);
    }

    setup_sudoku_window( instance );
    GRAPHIC_TRACE( ( "main.c: setup_window done\n") );

    static sudoku_ui_table_t ui_fcts = {
        .redraw = ui_redraw_game, 
        .set_window_name = ui_update_window_name,
        .set_status = ui_set_status_line,
        .set_back_level = ui_set_mark_n_back_level,
        .set_enter_mode = ui_change_enter_item,
        .enable_menu = ui_enable_menu,
        .disable_menu = ui_disable_menu,
        .enable_menu_item = ui_enable_menu_item,
        .disable_menu_item = ui_disable_menu_item,
        .success_dialog = ui_success_dialog
    };

    if ( sudoku_game_init( instance, argc, argv, &ui_fcts ) ) {
        exit(1);
    }
    /* All GTK applications must have a gtk_main(). Control ends here
     * and waits for an event to occur (like a key press or
     * mouse event). */
    gtk_main ();
    return 0;
}

/* -------------- end graphic.c -------------- */
