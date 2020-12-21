/*
 * Copyright (C) 2016 Sebastian Geiger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "tilda-search-box.h"
#include "tilda-enum-types.h"

#define PCRE2_CODE_UNIT_WIDTH 0
#include <pcre2.h>

#define GRESOURCE "/org/tilda/"

struct _TildaSearchBox
{
  GtkBox                parent;

  GtkWidget            *label;
  GtkWidget            *entry;
  GtkWidget            *button_next;
  GtkWidget            *button_prev;
  GtkWidget            *check_match_case;
  GtkWidget            *check_regex;

  TildaSearchDirection  last_direction;
  gboolean              last_search_successful;
};

enum
{
  SIGNAL_SEARCH,
  SIGNAL_SEARCH_GREGEX,
  SIGNAL_FOCUS_OUT,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (TildaSearchBox, tilda_search_box, GTK_TYPE_BOX)

static void
search_vte_regex (TildaSearchBox       *search,
                  TildaSearchDirection  direction)
{
  GtkEntry *entry;
  GtkEntryBuffer *buffer;
  GtkToggleButton *toggle_button;

  guint32 compile_flags;
  gboolean wrap_on_search;
  gboolean is_regex;
  gboolean match_case;
  const gchar *text;
  gchar *pattern;
  size_t pattern_length;
  gboolean search_result;

  GError *error;
  VteRegex *regex;

  compile_flags = 0;
  wrap_on_search = FALSE;
  toggle_button = GTK_TOGGLE_BUTTON (search->check_regex);
  is_regex = gtk_toggle_button_get_active (toggle_button);
  entry = GTK_ENTRY (search->entry);
  buffer = GTK_ENTRY_BUFFER (gtk_entry_get_buffer (entry));
  text = gtk_entry_buffer_get_text (buffer);

  if (!search->last_search_successful)
    wrap_on_search = TRUE;

  if (is_regex)
    {
      compile_flags |= PCRE2_MULTILINE;
      pattern = g_strdup (text);
    }
  else
    pattern = g_regex_escape_string (text, -1);

  pattern_length = strlen (pattern);

  toggle_button = GTK_TOGGLE_BUTTON (search->check_match_case);
  match_case = gtk_toggle_button_get_active (toggle_button);

  if (!match_case)
  {
    compile_flags |= PCRE2_CASELESS;
  }

  compile_flags |= PCRE2_MULTILINE;

  error = NULL;

  regex = vte_regex_new_for_search (pattern, pattern_length, compile_flags, &error);

  g_free (pattern);

  if (error)
    {
      GtkLabel *label = GTK_LABEL (search->label);
      gtk_label_set_text (label, error->message);
      gtk_widget_set_visible (search->label, TRUE);
      g_error_free (error);
      return;
    }

  g_signal_emit (search, signals[SIGNAL_SEARCH], 0,
                 regex, direction, wrap_on_search, &search_result);

  search->last_direction = direction;

  gtk_widget_set_visible (search->label, !search_result);
  search->last_search_successful = search_result;

  vte_regex_unref (regex);
}

static void
search (TildaSearchBox       *search,
        TildaSearchDirection  direction)
{
    search_vte_regex (search, direction);
}

static gboolean
entry_key_press_cb (TildaSearchBox *box,
                    GdkEvent       *event,
                    GtkWidget      *widget)
{
  GdkEventKey *event_key;

  event_key = (GdkEventKey*) event;

  if (event_key->keyval == GDK_KEY_Return) {
    search (box, box->last_direction);
    return GDK_EVENT_STOP;
  }

  /* If the search entry has focus the user can hide the search bar by pressing escape. */
  if (event_key->keyval == GDK_KEY_Escape)
    {
      if (gtk_widget_has_focus (box->entry))
        {
          g_signal_emit (box, signals[SIGNAL_FOCUS_OUT], 0);
          gtk_widget_set_visible (GTK_WIDGET (box), FALSE);

          return GDK_EVENT_STOP;
        }
    }

  return GDK_EVENT_PROPAGATE;
}

static gboolean
entry_changed_cb (TildaSearchBox *box,
                  GtkEditable *editable)
{
  gtk_widget_hide (box->label);
  box->last_search_successful = TRUE;

  return GDK_EVENT_STOP;
}

static void
button_next_cb (TildaSearchBox *box,
                GtkWidget      *widget)
{
  /* The default is to search forward */
  search (box, SEARCH_FORWARD);
}

static void
button_prev_cb (TildaSearchBox *box,
                GtkWidget      *widget)
{
  search (box, SEARCH_BACKWARD);
}

void
tilda_search_box_toggle (TildaSearchBox *box)
{
  gboolean visible = !gtk_widget_get_visible(GTK_WIDGET (box));

  if (visible)
    gtk_widget_grab_focus (box->entry);
  else
    g_signal_emit (box, signals[SIGNAL_FOCUS_OUT], 0);

  gtk_widget_set_visible(GTK_WIDGET (box), visible);
}

static void
tilda_search_box_class_init (TildaSearchBoxClass *box_class)
{
  GtkWidgetClass *widget_class;
  const gchar *resource_name;

  widget_class = GTK_WIDGET_CLASS (box_class);

  /**
   * TildaSearchBox::search:
   * @widget: the widget that received the signal
   * @regex: the regular expression entered by the user
   * @direction: the direction for the search
   * @wrap_over: if the search should wrap over
   *
   * This signal is, emitted when the user performed a search action, such
   * as clicking on the prev or next buttons or hitting enter.
   *
   * This widget does not actually perform the search, but leaves it to the
   * users of this signal to perform the search based on the information
   * provided to the signal handler.
   *
   * The user function needs to return %TRUE or %FALSE depending on
   * whether the search was successful (i.e. a match was found).
   *
   * Returns: %TRUE if the search found a match, %FALSE otherwise.
   */
  signals[SIGNAL_SEARCH] =
    g_signal_new ("search", TILDA_TYPE_SEARCH_BOX, G_SIGNAL_RUN_LAST, 0,
                  NULL, NULL, NULL, G_TYPE_BOOLEAN,
                  3, VTE_TYPE_REGEX, TILDA_TYPE_SEARCH_DIRECTION, G_TYPE_BOOLEAN);

/**
   * TildaSearchBox::search-gregex:
   * @widget: the widget that received the signal
   * @regex: the regular expression entered by the user
   * @direction: the direction for the search
   * @wrap_over: if the search should wrap over
   *
   * This signal is, emitted when the user performed a search action, such
   * as clicking on the prev or next buttons or hitting enter.
   *
   * This widget does not actually perform the search, but leaves it to the
   * users of this signal to perform the search based on the information
   * provided to the signal handler.
   *
   * The user function needs to return %TRUE or %FALSE depending on
   * whether the search was successful (i.e. a match was found).
   *
   * Returns: %TRUE if the search found a match, %FALSE otherwise.
   */
    signals[SIGNAL_SEARCH_GREGEX] =
            g_signal_new ("search-gregex", TILDA_TYPE_SEARCH_BOX, G_SIGNAL_RUN_LAST, 0,
                          NULL, NULL, NULL, G_TYPE_BOOLEAN,
                          3, G_TYPE_REGEX, TILDA_TYPE_SEARCH_DIRECTION, G_TYPE_BOOLEAN);

  /**
   * TildaSearchBox::focus-out:
   * @widget: the widget that received the signal
   *
   * This signal is emitted, when the search bar is being toggled
   * and this would cause the search bar to hide. Hiding the search bar
   * means that it will loose focus and that some other widget should get
   * focused.
   *
   * This signal should be used to grab the focus of another widget.
   */
  signals[SIGNAL_FOCUS_OUT] =
    g_signal_new ("focus-out", TILDA_TYPE_SEARCH_BOX, G_SIGNAL_RUN_LAST, 0,
                  NULL, NULL, NULL, G_TYPE_NONE, 0);

  resource_name = GRESOURCE "tilda-search-box.ui";
  gtk_widget_class_set_template_from_resource (widget_class, resource_name);

  gtk_widget_class_bind_template_child (widget_class, TildaSearchBox,
                                        label);
  gtk_widget_class_bind_template_child (widget_class, TildaSearchBox,
                                        entry);
  gtk_widget_class_bind_template_child (widget_class, TildaSearchBox,
                                        button_next);
  gtk_widget_class_bind_template_child (widget_class, TildaSearchBox,
                                        button_prev);
  gtk_widget_class_bind_template_child (widget_class, TildaSearchBox,
                                        check_match_case);
  gtk_widget_class_bind_template_child (widget_class, TildaSearchBox,
                                        check_regex);
}

static void
tilda_search_box_init (TildaSearchBox *box)
{
  TildaSearchBox *search_box;

  gtk_widget_init_template (GTK_WIDGET (box));

  search_box = TILDA_SEARCH_BOX (box);

  search_box->last_direction = SEARCH_BACKWARD;

  /* Initialize to true to prevent search from
   * wrapping around on first search. */
  search_box->last_search_successful = TRUE;

  gtk_widget_set_name (GTK_WIDGET (search_box), "search");

  g_signal_connect_swapped (G_OBJECT(search_box->entry), "key-press-event",
                            G_CALLBACK(entry_key_press_cb), search_box);
  g_signal_connect_swapped (G_OBJECT (search_box->entry), "changed",
                            G_CALLBACK (entry_changed_cb), search_box);
  g_signal_connect_swapped (G_OBJECT (search_box->button_next), "clicked",
                            G_CALLBACK (button_next_cb), search_box);
  g_signal_connect_swapped (G_OBJECT (search_box->button_prev), "clicked",
                            G_CALLBACK (button_prev_cb), search_box);
}

GtkWidget*
tilda_search_box_new (void)
{
  return g_object_new (TILDA_TYPE_SEARCH_BOX,
                       "orientation", GTK_ORIENTATION_VERTICAL,
                       NULL);
}
