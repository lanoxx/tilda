#ifndef TILDA_OPTIONS_H
#define TILDA_OPTIONS_H

#include <glib.h>

typedef struct tilda_cli_options tilda_cli_options;

struct tilda_cli_options {
    gchar *background_color;
    gchar *command;
    gchar *font;
    gchar *working_dir;
    gint back_alpha;
    gint lines;
    gint x_pos;
    gint y_pos;
    gboolean scrollbar;
    gboolean show_config;
    gboolean version;
    gboolean hidden;
    gboolean enable_dbus;
};

/**
 * Creates a new tilda_cli_option instance.
 *
 * @return returns
 * a pointer to a newly allocated tilda_cli_options structure, we
 * should be freed when no longer needed.
 */
tilda_cli_options *tilda_cli_options_new (void);

/**
 * Parse all of the Command-Line Options given to tilda.
 * This can modify argv and argc, and will set values in the config.
 *
 * @param cli_options pointer to a struct to store command-line options into
 * @param argc argc from main
 * @param argv argv from main
 * @param config_file pointer which will be used to store the config file path if it was specified via command-line
 * @return TRUE if we should show the configuration wizard, FALSE otherwise
 */
gboolean           tilda_cli_options_parse_options (tilda_cli_options *options,
                                                    gint argc,
                                                    gchar **argv,
                                                    gchar **config_file);

#endif //TILDA_OPTIONS_H
