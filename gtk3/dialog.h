
/*
  Sudoku game - using X11+GTK+

  Functions manipulating dialog boxes.

*/

#ifndef __DIALOG_H__

extern GtkWidget * create_restart_dialog( GtkWindow *parent, sudoku_duration_t *dhms );

typedef enum {
  STOPPED  = 0,  /* Should not be called if already stopped */
  ENTERING = 1,  /* display dialog for interrupting entering */
  PLAYING  = 2   /* display dialog for interrupting playing */
} game_state_t;

extern GtkWidget * create_stop_dialog( GtkWindow *parent, game_state_t what );
extern GtkWidget * create_exit_dialog( GtkWindow *parent, game_state_t what );

extern GtkWidget * create_pick_dialog( GtkWindow *parent );
extern GtkWidget * create_commit_dialog( GtkWindow *parent );
//extern GtkWidget * create_enter_dialog( void );

typedef enum {
    PRINTING_THEME = 0, CHALKBOARD_THEME, PAPER_THEME, IMAGE_THEME
} theme_id_t;

typedef struct {
  GtkWidget   *theme_1, *theme_2, *theme_3, *translucent; /* OUT */
  GtkWidget   *remove_fill, *timed_game, *display_time;   /* OUT */
  theme_id_t  theme_id;             /* IN */
  bool        translucent_state,    /* IN */
              remove_fill_state,    /* IN */
              timed_game_state,     /* IN */
              display_time_state;   /* IN */
} game_option_t;

extern GtkWidget * create_options_dialog( GtkWindow *parent, game_option_t *opts );

extern const char * get_widget_entry( GtkWidget *wdg );

#endif /* __DIALOG_H__ */
