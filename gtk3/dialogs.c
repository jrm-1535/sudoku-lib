
/*
  Sudoku game - using GTK+

  Dialog boxes

*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "../sudoku.h"
#include "dialog.h"

extern GtkWidget* create_pick_dialog( GtkWindow *parent )
{
    GtkWidget *pick = gtk_dialog_new ();
    gtk_window_set_transient_for( GTK_WINDOW( pick ), parent );
    gtk_widget_set_size_request( pick, 250, -1 );
    gtk_window_set_position( GTK_WINDOW( pick ),
                             GTK_WIN_POS_CENTER_ON_PARENT
                           /* GTK_WIN_POS_MOUSE */ );
    gtk_window_set_title( GTK_WINDOW( pick ), "Pick a game?" );
    gtk_window_set_modal( GTK_WINDOW( pick ), TRUE );

    gtk_window_set_resizable( GTK_WINDOW( pick ), FALSE );
    gtk_window_set_type_hint( GTK_WINDOW( pick ), GDK_WINDOW_TYPE_HINT_DIALOG );
  
    GtkWidget *dialog_vbox = gtk_dialog_get_content_area( GTK_DIALOG( pick ) );

    GtkWidget *frame = gtk_frame_new( NULL );
    gtk_box_pack_start( GTK_BOX( dialog_vbox ), frame, FALSE, FALSE, 0 );
    gtk_widget_set_size_request( frame, 200, 100 );
    gtk_frame_set_label_align( GTK_FRAME( frame ), 0, 0 );
    gtk_frame_set_shadow_type( GTK_FRAME( frame ), GTK_SHADOW_NONE );

    GtkWidget *alignment = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 10 );
    gtk_container_add( GTK_CONTAINER( frame ), alignment );
    GtkWidget *entry = gtk_entry_new();
    gtk_box_pack_start( GTK_BOX(alignment), entry, TRUE, FALSE, 0 );

    gtk_entry_set_max_length (GTK_ENTRY (entry), 6 );
    gtk_entry_set_width_chars (GTK_ENTRY (entry), 6);

    GtkWidget *label = gtk_label_new( "<b>Choose the game number</b>");
    gtk_frame_set_label_widget( GTK_FRAME ( frame ), label );
    gtk_widget_set_size_request( label, 242, -1 );

    gtk_label_set_use_markup( GTK_LABEL( label ), TRUE );
    gtk_label_set_justify( GTK_LABEL( label ), GTK_JUSTIFY_CENTER );
    gtk_widget_set_margin_top( label, 15 );
    gtk_widget_set_margin_bottom( label, 15 );

    gtk_dialog_add_button( GTK_DIALOG( pick ), "CANCEL", GTK_RESPONSE_CANCEL );
    gtk_dialog_add_button( GTK_DIALOG( pick ), "OK", GTK_RESPONSE_OK );
    gtk_dialog_set_default_response( GTK_DIALOG( pick ), GTK_RESPONSE_CANCEL );

    gtk_widget_show( label );
    gtk_widget_show( entry );
    gtk_widget_show( alignment );
    gtk_widget_show( frame );
    gtk_widget_show(dialog_vbox);

    g_object_set_data (G_OBJECT (pick), "entry", entry);
    gtk_widget_grab_focus( entry );
    return pick;
}

extern GtkWidget* create_commit_dialog( GtkWindow *parent )
{
    GtkWidget *commit = gtk_dialog_new ();
    gtk_window_set_transient_for( GTK_WINDOW( commit ), parent );
    gtk_widget_set_size_request( commit, 250, -1 );
    gtk_window_set_position( GTK_WINDOW( commit ),
                             GTK_WIN_POS_CENTER_ON_PARENT
                           /* GTK_WIN_POS_MOUSE */ );
    gtk_window_set_title( GTK_WINDOW( commit ), "Accept that game?" );
    gtk_window_set_modal( GTK_WINDOW( commit ), TRUE );

    gtk_window_set_resizable( GTK_WINDOW( commit ), FALSE );
    gtk_window_set_type_hint( GTK_WINDOW( commit ), GDK_WINDOW_TYPE_HINT_DIALOG );
  
    GtkWidget *dialog_vbox = gtk_dialog_get_content_area( GTK_DIALOG( commit ) );

    GtkWidget *frame = gtk_frame_new( NULL );
    gtk_box_pack_start( GTK_BOX( dialog_vbox ), frame, FALSE, FALSE, 0 );
    gtk_widget_set_size_request( frame, 100, 100 );
    gtk_frame_set_label_align( GTK_FRAME( frame ), 0, 0 );
    gtk_frame_set_shadow_type( GTK_FRAME( frame ), GTK_SHADOW_NONE );

    GtkWidget *alignment = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 10 );
    gtk_container_add( GTK_CONTAINER( frame ), alignment );

    GtkWidget *entry = gtk_entry_new();
    gtk_box_pack_start( GTK_BOX(alignment), entry, TRUE, FALSE, 0 );
    gtk_entry_set_max_length (GTK_ENTRY (entry), 32 );
    gtk_entry_set_width_chars (GTK_ENTRY (entry), 24 );

    GtkWidget *label = gtk_label_new( "<b>Choose a name</b>");
    gtk_frame_set_label_widget( GTK_FRAME ( frame ), label );
    gtk_widget_set_size_request( label, 242, -1 );
    gtk_label_set_use_markup( GTK_LABEL( label ), TRUE );
    gtk_label_set_justify( GTK_LABEL( label ), GTK_JUSTIFY_CENTER );
    gtk_widget_set_margin_top( label, 15 );
    gtk_widget_set_margin_bottom( label, 15 );

    gtk_dialog_add_button( GTK_DIALOG( commit ), "CANCEL", GTK_RESPONSE_CANCEL );
    gtk_dialog_add_button( GTK_DIALOG( commit ), "OK", GTK_RESPONSE_OK );
    gtk_dialog_set_default_response( GTK_DIALOG( commit ), GTK_RESPONSE_CANCEL );

    gtk_widget_show( label );
    gtk_widget_show( entry );
    gtk_widget_show( alignment );
    gtk_widget_show( frame );
    gtk_widget_show(dialog_vbox);

    g_object_set_data (G_OBJECT (commit), "entry", entry);
    gtk_widget_grab_focus( entry );
    return commit;
}

extern GtkWidget* create_stop_dialog( GtkWindow *parent, game_state_t state )
{
    GtkWidget *stop = gtk_dialog_new ();
    gtk_window_set_transient_for( GTK_WINDOW( stop ), parent );
    gtk_widget_set_size_request( stop, 280, -1 );
    gtk_window_set_position( GTK_WINDOW( stop ),
                             GTK_WIN_POS_CENTER_ON_PARENT
                           /* GTK_WIN_POS_MOUSE */ );
    gtk_window_set_title( GTK_WINDOW( stop ), "Stop this game?" );
    gtk_window_set_modal( GTK_WINDOW( stop ), TRUE );

    gtk_window_set_resizable( GTK_WINDOW( stop ), FALSE );
    gtk_window_set_type_hint( GTK_WINDOW( stop ), GDK_WINDOW_TYPE_HINT_DIALOG );
  
    GtkWidget *dialog_vbox = gtk_dialog_get_content_area( GTK_DIALOG( stop ) );

    char *title, *text;
    if ( ENTERING == state ) {
        title = "You are entering a game";
        text = "<span foreground=\"red\"><b>Do you really want to lose it now\nand start a new game?</b></span>" ;
    } else {
        title = "You have started a game";
        text = "<span foreground=\"red\"><b>Do you really want to stop it now\nand start a new game?</b></span>" ;
    }

    GtkWidget *frame = gtk_frame_new( title );
    gtk_box_pack_start( GTK_BOX( dialog_vbox ), frame, FALSE, FALSE, 0 );
    /* gtk_widget_set_size_request( frame, 180, 80 ); */
    gtk_frame_set_label_align( GTK_FRAME( frame ), 0.1, 0.5 );
    gtk_frame_set_shadow_type( GTK_FRAME( frame ), GTK_SHADOW_OUT );

    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL (label), text );
    gtk_container_add( GTK_CONTAINER( frame ), label );

    gtk_widget_set_size_request( label, 260, -1 );
    gtk_label_set_justify( GTK_LABEL( label ), GTK_JUSTIFY_CENTER );
    gtk_widget_set_margin_top( label, 15 );
    gtk_widget_set_margin_bottom( label, 15 );

    gtk_dialog_add_button( GTK_DIALOG( stop ), "Oh NO!", GTK_RESPONSE_CANCEL );
    gtk_dialog_add_button( GTK_DIALOG( stop ), "Yes", GTK_RESPONSE_OK );
    gtk_dialog_set_default_response( GTK_DIALOG( stop ), GTK_RESPONSE_CANCEL );

    gtk_widget_show( label );
    gtk_widget_show( frame );
    gtk_widget_show(dialog_vbox);

    return stop;
}

extern GtkWidget* create_exit_dialog( GtkWindow *parent, game_state_t state )
{
    char *title, *text;

    GtkWidget *dialog_vbox;
    GtkWidget *frame;
    GtkWidget *label;

    GtkWidget *exit = gtk_dialog_new ();
    gtk_window_set_transient_for( GTK_WINDOW( exit ), parent );
    gtk_widget_set_size_request( exit, 320, -1 );
    gtk_window_set_position( GTK_WINDOW( exit ),
                            GTK_WIN_POS_CENTER_ON_PARENT
                           /* GTK_WIN_POS_MOUSE */ );
    gtk_window_set_title( GTK_WINDOW( exit ), "Exit this game?" );
    gtk_window_set_modal( GTK_WINDOW( exit ), TRUE );

    gtk_window_set_resizable( GTK_WINDOW( exit ), FALSE );
    gtk_window_set_type_hint( GTK_WINDOW( exit ), GDK_WINDOW_TYPE_HINT_DIALOG );
  
    dialog_vbox = gtk_dialog_get_content_area( GTK_DIALOG( exit ) );

    if ( ENTERING == state ) {
        title = "You are entering a game";
        text = "<span foreground=\"red\"><b>Do you really want to lose it now\nand exit?</b></span>" ;
    } else {
        title = "You have started a game";
        text = "<span foreground=\"red\"><b>You can save first and then quit,\nyou can cancel and keep playing,\nor you can quit without saving...\n\nWhat do you want to do?</b></span>";
    }

    frame = gtk_frame_new( title );
    gtk_box_pack_start( GTK_BOX( dialog_vbox ), frame, FALSE, FALSE, 0 );
    /* gtk_widget_set_size_request( frame, 180, 80 ); */
    gtk_frame_set_label_align( GTK_FRAME( frame ), 0.1, 0.5 );
    gtk_frame_set_shadow_type( GTK_FRAME( frame ), GTK_SHADOW_OUT );

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL (label), text );
    gtk_container_add( GTK_CONTAINER( frame ), label );

    gtk_widget_set_size_request( label, 260, -1 );
    gtk_label_set_justify( GTK_LABEL( label ), GTK_JUSTIFY_CENTER );
    gtk_widget_set_margin_top( label, 15 );
    gtk_widget_set_margin_bottom( label, 15 );

    if ( ENTERING == state ) {
        gtk_dialog_add_button( GTK_DIALOG( exit ), "Oh NO!", GTK_RESPONSE_CANCEL );
        gtk_dialog_add_button( GTK_DIALOG( exit ), "Yes", GTK_RESPONSE_OK );
        gtk_dialog_set_default_response( GTK_DIALOG( exit ), GTK_RESPONSE_CANCEL );

    } else {
        gtk_dialog_add_button( GTK_DIALOG( exit ), "Save&Exit", GTK_RESPONSE_CLOSE );
        gtk_dialog_add_button( GTK_DIALOG( exit ), "CANCEL", GTK_RESPONSE_CANCEL );
        gtk_dialog_add_button( GTK_DIALOG( exit ), "QUIT", GTK_RESPONSE_OK );
        gtk_dialog_set_default_response( GTK_DIALOG( exit ), GTK_RESPONSE_CLOSE );
    }

    gtk_widget_show( label );
    gtk_widget_show( frame );
    gtk_widget_show(dialog_vbox);

    return exit;
}

extern GtkWidget* create_restart_dialog( GtkWindow *parent, sudoku_duration_t *dhms )
{
    GtkWidget *start = gtk_dialog_new ();
    gtk_window_set_transient_for( GTK_WINDOW( start ), parent );
    gtk_widget_set_size_request( start, 280, -1 );
    gtk_window_set_position( GTK_WINDOW( start ),
                             GTK_WIN_POS_CENTER_ON_PARENT
                           /* GTK_WIN_POS_MOUSE */ );
    gtk_window_set_title( GTK_WINDOW( start ), "Start a new game?" );
    gtk_window_set_modal( GTK_WINDOW( start ), TRUE );

    gtk_window_set_resizable( GTK_WINDOW( start ), FALSE );
    gtk_window_set_type_hint( GTK_WINDOW( start ), GDK_WINDOW_TYPE_HINT_DIALOG );
  
    GtkWidget *dialog_vbox = gtk_dialog_get_content_area( GTK_DIALOG( start ) );

    GtkWidget *frame = gtk_frame_new( "Game over" );
    gtk_box_pack_start( GTK_BOX( dialog_vbox ), frame, FALSE, FALSE, 0 );
    /* gtk_widget_set_size_request( frame, 180, 80 ); */
    gtk_frame_set_label_align( GTK_FRAME( frame ), 0.1, 0.5 );
    gtk_frame_set_shadow_type( GTK_FRAME( frame ), GTK_SHADOW_OUT );

    GtkWidget *label = gtk_label_new(NULL);
    if ( dhms ) {
        char  buffer[256];
        if ( dhms->hours ) {
            sprintf( buffer, "<span foreground=\"DarkGreen\"><b>Congratulations, you WON!\n(in %d hours %d minutes %d seconds)\nDo you want to play again?</b></span>",
                     dhms->hours, dhms->minutes, dhms->seconds );
        } else {
            sprintf( buffer, "<span foreground=\"DarkGreen\"><b>Congratulations, you WON!\n(in %d minutes %d seconds)\nDo you want to play again?</b></span>",
                     dhms->minutes, dhms->seconds );
        }  
        gtk_label_set_markup(GTK_LABEL (label), buffer );
    } else {
        gtk_label_set_markup(GTK_LABEL (label), "<span foreground=\"DarkGreen\"><b>Congratulations, you WON!\nDo you want to play again?</b></span>" );
    }
    gtk_container_add( GTK_CONTAINER( frame ), label );

    gtk_widget_set_size_request( label, 260, -1 );
    gtk_label_set_justify( GTK_LABEL( label ), GTK_JUSTIFY_CENTER );
    gtk_widget_set_margin_top( label, 15 );
    gtk_widget_set_margin_bottom( label, 15 );

    gtk_dialog_add_button( GTK_DIALOG( start ), "No thanks", GTK_RESPONSE_CANCEL );
    gtk_dialog_add_button( GTK_DIALOG( start ), "Yes", GTK_RESPONSE_OK );
    gtk_dialog_set_default_response( GTK_DIALOG( start ), GTK_RESPONSE_OK );

    gtk_widget_show( label );
    gtk_widget_show( frame );
    gtk_widget_show(dialog_vbox);

    return start;
}

static void _time_toggle( GtkWidget *button, gpointer arg )
{
  game_option_t *opts = (game_option_t *)arg;
  if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button) ) ) {
    gtk_widget_set_sensitive( opts->display_time, TRUE );
  } else {
    gtk_widget_set_sensitive( opts->display_time, FALSE );
  }
}

extern GtkWidget * create_options_dialog( GtkWindow *parent, game_option_t *opts )
{
    GtkWidget *options = gtk_dialog_new ();
    gtk_window_set_transient_for( GTK_WINDOW( options ), parent );
    gtk_widget_set_size_request( options, 380, -1 );
    gtk_window_set_position( GTK_WINDOW( options ),
                             GTK_WIN_POS_CENTER_ON_PARENT
                           /* GTK_WIN_POS_MOUSE */ );
    gtk_window_set_title( GTK_WINDOW( options ), "Sudoku Options" );
    gtk_window_set_modal( GTK_WINDOW( options ), TRUE );

    gtk_window_set_resizable( GTK_WINDOW( options ), FALSE );
    gtk_window_set_type_hint( GTK_WINDOW( options ), GDK_WINDOW_TYPE_HINT_DIALOG );

    GtkWidget *dialog_vbox = gtk_dialog_get_content_area( GTK_DIALOG( options ) );

    GtkWidget *frame_color = gtk_frame_new( "Color Theme" );
    gtk_box_pack_start( GTK_BOX( dialog_vbox ), frame_color, FALSE, FALSE, 0 );
    gtk_frame_set_label_align( GTK_FRAME( frame_color ), 0.1, 0.5 );
    gtk_frame_set_shadow_type( GTK_FRAME( frame_color ), GTK_SHADOW_OUT );

    GtkWidget *radio_box = gtk_button_box_new( GTK_ORIENTATION_HORIZONTAL );
    GtkWidget *radio_b1 = gtk_radio_button_new_with_label( NULL, "Chalkboard" );
    GtkWidget *radio_b2 = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON( radio_b1 ), "Paper sheet" );
    GtkWidget *radio_b3 = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON( radio_b2 ), "Image" );

    opts->theme_1 = radio_b1;
    opts->theme_2 = radio_b2;
    opts->theme_3 = radio_b3;

    GtkWidget *pre_selected;
    switch( opts->theme_id ) {
    default:
        pre_selected = radio_b1;
        break;
    case 2:
        pre_selected = radio_b2;
        break;
    case 3:
        pre_selected = radio_b3;
        break;
    }

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( pre_selected ), TRUE );

    gtk_box_set_homogeneous( GTK_BOX( radio_box ), FALSE );
    gtk_box_pack_start( GTK_BOX( radio_box ), radio_b1, TRUE, TRUE, 20 );
    gtk_box_pack_end( GTK_BOX( radio_box ), radio_b2, TRUE, TRUE, 20 );
    gtk_box_pack_end( GTK_BOX( radio_box ), radio_b3, TRUE, TRUE, 20 );

    GtkWidget *translucent =  gtk_check_button_new_with_label( "Translucent background" );
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( translucent ), opts->translucent_state );
    opts->translucent = translucent;

    GtkWidget *frame_vbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
    gtk_box_pack_start( GTK_BOX( frame_vbox ), radio_box, TRUE, TRUE, 2 );
    gtk_box_pack_end( GTK_BOX( frame_vbox ), translucent, TRUE, TRUE, 2 );

    gtk_container_add( GTK_CONTAINER( frame_color ), frame_vbox );

    gtk_widget_show( frame_vbox );
    gtk_widget_show( translucent );

    gtk_widget_show( radio_b1 );
    gtk_widget_show( radio_b2 );
    gtk_widget_show( radio_b3 );
    gtk_widget_show( radio_box );
    gtk_widget_show( frame_color );

    GtkWidget *frame_time = gtk_frame_new( "Timed game" );
    gtk_box_pack_start( GTK_BOX( dialog_vbox ), frame_time, FALSE, FALSE, 0 );
    gtk_frame_set_label_align( GTK_FRAME( frame_time ), 0.1, 0.5 );
    gtk_frame_set_shadow_type( GTK_FRAME( frame_time ), GTK_SHADOW_OUT );

    GtkWidget *time_box = gtk_button_box_new( GTK_ORIENTATION_HORIZONTAL );
    GtkWidget *timed_game = gtk_check_button_new_with_label( "Keep track of time" );
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( timed_game ), opts->timed_game_state );
    g_signal_connect( timed_game, "toggled", G_CALLBACK( _time_toggle ), opts );
    opts->timed_game = timed_game;

    gtk_box_set_homogeneous( GTK_BOX( time_box ), FALSE );
    gtk_box_pack_start( GTK_BOX( time_box ), timed_game, TRUE, TRUE, 0 );

    GtkWidget *display_time = gtk_check_button_new_with_label( "Display time in status");
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( display_time ), opts->display_time_state );
    opts->display_time = display_time;

    gtk_box_pack_start( GTK_BOX( time_box ), display_time, TRUE, TRUE, 0 );

    gtk_container_add( GTK_CONTAINER( frame_time ), time_box );
    gtk_widget_show( timed_game );
    gtk_widget_show( display_time );
    gtk_widget_show( time_box );
    gtk_widget_show( frame_time );

    if ( ! gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( timed_game ) ) ) {
        gtk_widget_set_sensitive( display_time, FALSE );
    }

    GtkWidget *frame_fill = gtk_frame_new( "Fill option" );
    gtk_box_pack_start( GTK_BOX( dialog_vbox ), frame_fill, FALSE, FALSE, 0 );
    gtk_frame_set_label_align( GTK_FRAME( frame_fill ), 0.1, 0.5 );
    gtk_frame_set_shadow_type( GTK_FRAME( frame_fill ), GTK_SHADOW_OUT );

    GtkWidget *remove_fill = gtk_check_button_new_with_label( "Resolve conflicts automatically when filling up a cell");
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( remove_fill ), opts->remove_fill_state );
    opts->remove_fill = remove_fill;
    gtk_container_add( GTK_CONTAINER( frame_fill ), remove_fill );

    gtk_widget_show( remove_fill );
    gtk_widget_show( frame_fill );

    gtk_dialog_add_button( GTK_DIALOG( options ), "Cancel", GTK_RESPONSE_CANCEL );
    gtk_dialog_add_button( GTK_DIALOG( options ), "Ok", GTK_RESPONSE_OK );
    gtk_dialog_set_default_response( GTK_DIALOG( options ), GTK_RESPONSE_CANCEL );

    gtk_widget_show(dialog_vbox);
    return options;
}

extern const char * get_widget_entry( GtkWidget *wdg )
{
  GtkWidget *entry = g_object_get_data( G_OBJECT(wdg), "entry" );
  if ( entry ) {
    const char *text = gtk_entry_get_text( GTK_ENTRY( entry ) );
    if (text) {
      printf( "input=[%s]\n", text );
      return text;
    }
  }
  return NULL;
}
