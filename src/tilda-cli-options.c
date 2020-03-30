#include "tilda-cli-options.h"

#include "config.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>

#include "debug.h"

gboolean tilda_cli_options_parse_options (tilda_cli_options *cli_options,
                                          gint argc,
                                          gchar *argv[],
                                          gchar **config_file)
{
    DEBUG_FUNCTION ("parse_cli");
    DEBUG_ASSERT (argc != 0);
    DEBUG_ASSERT (argv != NULL);
    // *config_file must be non-null only if a configuration file path has been parsed
    DEBUG_ASSERT (*config_file == NULL);

    /* All of the various command-line options */
    GOptionEntry cl_opts[] = {
            { "background-color",   'b', 0, G_OPTION_ARG_STRING,    &(cli_options->background_color),  N_("Set the background color"), NULL },
            { "command",            'c', 0, G_OPTION_ARG_STRING,    &(cli_options->command),           N_("Run a command at startup"), NULL },
            { "hidden",             'h', 0, G_OPTION_ARG_NONE,      &(cli_options->hidden),            N_("Start Tilda hidden"), NULL },
            { "font",               'f', 0, G_OPTION_ARG_STRING,    &(cli_options->font),              N_("Set the font to the following string"), NULL },
            { "config-file",        'g', 0, G_OPTION_ARG_STRING,    config_file,                       N_("Configuration file"), NULL },
            { "lines",              'l', 0, G_OPTION_ARG_INT,       &(cli_options->lines),             N_("Scrollback Lines"), NULL },
            { "scrollbar",          's', 0, G_OPTION_ARG_NONE,      &(cli_options->scrollbar),         N_("Use Scrollbar"), NULL },
            { "version",            'v', 0, G_OPTION_ARG_NONE,      &(cli_options->version),           N_("Print the version, then exit"), NULL },
            { "working-dir",        'w', 0, G_OPTION_ARG_STRING,    &(cli_options->working_dir),       N_("Set Initial Working Directory"), NULL },
            { "x-pos",              'x', 0, G_OPTION_ARG_INT,       &(cli_options->x_pos),             N_("X Position"), NULL },
            { "y-pos",              'y', 0, G_OPTION_ARG_INT,       &(cli_options->y_pos),             N_("Y Position"), NULL },
            { "background-alpha",   't', 0, G_OPTION_ARG_INT,       &(cli_options->back_alpha),        N_("Opaqueness: 0-100%"), NULL },
            { "config",             'C', 0, G_OPTION_ARG_NONE,      &(cli_options->show_config),       N_("Show Configuration Wizard"), NULL },
            { NULL }
    };

    /* Set up the command-line parser */
    GError *error = NULL;
    GOptionContext *context = g_option_context_new (NULL);
    g_option_context_add_main_entries (context, cl_opts, NULL);
    g_option_context_add_group (context, gtk_get_option_group (FALSE));
    g_option_context_parse (context, &argc, &argv, &error);
    g_option_context_free (context);

    /* Check for unknown options, and give a nice message if there are some */
    if (error)
    {
        g_printerr (_("Error parsing command-line options. Try \"tilda --help\"\nto see all possible options.\n\nError message: %s\n"),
                    error->message);

        exit (EXIT_FAILURE);
    }

    /* If we need to show the version, show it then exit normally */
    if (cli_options->version)
    {
        g_print ("%s\n\n", PACKAGE_STRING);

        g_print ("Copyright (c) 2012-2020 Sebastian Geiger (lanoxx@gmx.net)\n");
        g_print ("Copyright (c) 2005-2009 Tristan Sloughter (sloutri@iit.edu)\n");
        g_print ("Copyright (c) 2005-2009 Ira W. Snyder (tilda@irasnyder.com)\n\n");

        g_print ("General Information: https://github.com/lanoxx/tilda\n");
        g_print ("Bug Reports: https://github.com/lanoxx/tilda/issues?state=open\n\n");

        g_print ("This program comes with ABSOLUTELY NO WARRANTY.\n");
        g_print ("This is free software, and you are welcome to redistribute it\n");
        g_print ("under certain conditions. See the file COPYING for details.\n");

        exit (EXIT_SUCCESS);
    }

    /* TRUE if we should show the config wizard, FALSE otherwize */
    return cli_options->show_config;
}

tilda_cli_options *tilda_cli_options_new ()
{
    tilda_cli_options *options = g_malloc0(sizeof(tilda_cli_options));
    if (!options)
    {
        g_printerr (_("Error allocating memory for a new tilda_cli_options structure.\n"));
        exit (EXIT_FAILURE);
    }

    return options;
}
