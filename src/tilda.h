#include <gtk/gtk.h>

int wizard (int argc, char **argv);
int write_key_bindings (char wm[], char key[]);
void popup (char *message, char *b1_message, char *b2_message, void (*func1)(),void (*func2)());
void redo_wizard (GtkWidget *widget, gpointer data);
void add_anyway (GtkWidget *widget, gpointer data);
