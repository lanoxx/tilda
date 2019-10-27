#include "screen-size.h"

#include <gtk/gtk.h>

static gint
get_window_scaling_factor (void)
{
    GValue gvalue = G_VALUE_INIT;
    GdkScreen *screen;

    g_value_init (&gvalue, G_TYPE_INT);

    screen = gdk_screen_get_default ();
    if (gdk_screen_get_setting (screen, "gdk-window-scaling-factor", &gvalue))
        return g_value_get_int (&gvalue);

    return 1;
}

void
screen_size_get_dimensions (gint *width,
                            gint *height)
{
    GdkScreen *screen;
    GdkWindow *window;
    gint scale;

    scale = get_window_scaling_factor ();

    screen = gdk_screen_get_default ();
    window = gdk_screen_get_root_window (screen);

    *width = gdk_window_get_width (window) / scale;
    *height = gdk_window_get_height (window) / scale;
}
