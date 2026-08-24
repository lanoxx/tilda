/*
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "window-position.h"

typedef struct {
    GdkRectangle workarea;
    gint window_width;
    gint window_height;
    gint requested_x;
    gint requested_y;
    gint expected_x;
    gint expected_y;
} PositionCase;

static void
test_position_case (gconstpointer user_data)
{
    const PositionCase *test = user_data;
    gint x = test->requested_x;
    gint y = test->requested_y;

    tilda_window_constrain_position (&test->workarea,
                                     test->window_width,
                                     test->window_height,
                                     &x,
                                     &y);

    g_assert_cmpint (x, ==, test->expected_x);
    g_assert_cmpint (y, ==, test->expected_y);
}

int
main (int argc, char **argv)
{
    static const PositionCase cases[] = {
        {
            .workarea = { .x = 0, .y = 32, .width = 3840, .height = 2128 },
            .window_width = 1535,
            .window_height = 2106,
            .requested_x = 2300,
            .requested_y = 37,
            .expected_x = 2300,
            .expected_y = 37,
        },
        {
            .workarea = { .x = 0, .y = 32, .width = 3840, .height = 2128 },
            .window_width = 1535,
            .window_height = 2128,
            .requested_x = 2300,
            .requested_y = 37,
            .expected_x = 2300,
            .expected_y = 32,
        },
        {
            .workarea = { .x = 100, .y = 200, .width = 800, .height = 600 },
            .window_width = 400,
            .window_height = 300,
            .requested_x = 0,
            .requested_y = 900,
            .expected_x = 100,
            .expected_y = 500,
        },
        {
            .workarea = { .x = 100, .y = 200, .width = 800, .height = 600 },
            .window_width = 400,
            .window_height = 300,
            .requested_x = 1000,
            .requested_y = 0,
            .expected_x = 500,
            .expected_y = 200,
        },
        {
            .workarea = { .x = -1920, .y = 24, .width = 1920, .height = 1056 },
            .window_width = 1535,
            .window_height = 900,
            .requested_x = -500,
            .requested_y = 100,
            .expected_x = -1535,
            .expected_y = 100,
        },
        {
            .workarea = { .x = 1920, .y = 24, .width = 1920, .height = 1056 },
            .window_width = 2000,
            .window_height = 1200,
            .requested_x = 2400,
            .requested_y = 100,
            .expected_x = 1920,
            .expected_y = 24,
        },
        {
            .workarea = { .x = -800, .y = -600, .width = 800, .height = 600 },
            .window_width = 800,
            .window_height = 600,
            .requested_x = -700,
            .requested_y = -500,
            .expected_x = -800,
            .expected_y = -600,
        },
    };
    static const gchar *names[] = {
        "/window-position/preserves-valid-position",
        "/window-position/full-height-aligns-to-workarea",
        "/window-position/constrains-left-and-bottom",
        "/window-position/constrains-right-and-top",
        "/window-position/handles-negative-origin",
        "/window-position/aligns-oversized-window",
        "/window-position/aligns-exact-size",
    };

    g_test_init (&argc, &argv, NULL);

    for (guint i = 0; i < G_N_ELEMENTS (cases); i++)
        g_test_add_data_func (names[i], &cases[i], test_position_case);

    return g_test_run ();
}
