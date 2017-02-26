# Feature Requests / Improvements

 * Dragable bar at the bottom to change Tilda's height
 * Ability to disable keyboard accelerators (close tab, new tab, etc)
 * Session support, so Tilda will load with the same number of tabs that it had
   upon closing.
 * Ability to rename a tab manually

# Future Plans

 * DBus-ize Tilda
   This will make Tilda more of a long-running daemon than a terminal. What I
   envision is one Tilda process per user, with one config file, managing all of
   the Tilda windows (individual terminals) that the user wants. Also, expose all
   of the terminal's properties over DBus, so a completely seperate config
   program can be written. Also, things like "open a new tab in terminal 3, and
   launch XYZ in it" should be possible, and easy.

   Unfortunately, to support this, I am thinking about moving away from
   libConfuse, and moving to the built-in GLib key-value parser (INI-like). This
   should allow a config file like:

   [global]
   setting1 = value1
   setting2 = value2

   [terminal1]
   setting1 = value3

   Which means (to me) that all terminals should have the settings from the
   global section, but terminal1 will have setting1 overridden to a different
   value. This could be used to give different backgrounds or different fonts
   in each terminal, for example, but keep all other properties the same.

   Of course, the DBus stuff should probably be optional, so that you don't have
   to have it installed, nor running. You should be able to start a Tilda which
   is configured with DBus, but not have DBus running. It would be nice if Tilda
   didn't die when DBus dies out from under us.

# Possible Translation Problems

Change "Animation Delay" to "Animation Duration"
"Notebook" should be changed to "Window", probably
"tab_pos" error msg (in a switch stmt) should be changed.

