#!/usr/bin/env python

#
# Copyright: Ira W. Snyder (tilda@irasnyder.com)
# License: GNU General Public License v2
#

import sys, os, re

def get_tilda_configs():
    """Get a list of the config file names"""

    config_dir = "~/.tilda"
    config_file_regex = re.compile("config_\d+")
    files = []

    # Move into the config directory
    os.chdir(os.path.abspath(os.path.expanduser(config_dir)))

    # Get a list of all the config file names
    for f in os.listdir(os.getcwd()):
        if config_file_regex.match(f):
            files.append(f)

    return files

def read_config_file(filename):
    """Returns a list of lines"""

    try:
        f = open(filename, 'r', 0)
        try:
            data = f.read()
        finally:
            f.close()
    except IOError:
        print 'Error opening %s for reading ... exiting!' % filename
        sys.exit()

    lines = [l for l in data.split('\n') if len(l) > 0]

    return lines

def write_config_file(filename, lines):
    """Write out all of the lines to filename"""

    try:
        f = open(filename, 'wb', 0)
        try:
            for l in lines:
                f.write(l)
        finally:
            f.close()
    except IOError:
        print 'Error opening %s for writing ... exiting!' % filename
        sys.exit()

def convert_v1_to_v2(filename):
    """Convert's a readconf-style file to a libConfuse style file"""

    string_keys = ('background', 'font', 'image', 'key', 'title', 'command')
    int_keys = ('max_height', 'max_width', 'min_height', 'min_width', 'transparency',
            'x_pos', 'y_pos', 'scrollback', 'tab_pos', 'backspace_key', 'delete_key',
            'd_set_title', 'command_exit', 'scheme', 'scrollbar_pos', 'back_red',
            'back_green', 'back_blue', 'text_red', 'text_green', 'text_blue')
    boolean_keys = ('scroll_on_output', 'scroll_background', 'notebook_border',
            'notaskbar', 'above', 'pinned', 'antialias', 'scrollbar', 'use_image',
            'grab_focus', 'down', 'bold', 'blinks', 'bell', 'run_command', 'scroll_on_key')

    # Read the original config
    lines = read_config_file(filename)

    newlines = []

    # Loop through every line and convert them
    for l in lines:

        # Attempt to get each key / value pair
        try:
            (key, val) = l.split(':')
            key = key.strip()
            val = val.strip()
        except ValueError:
            print "You've probably already converted %s" % filename
            return

        # Handle string keys
        if key in string_keys:
            if val == 'none':
                newlines.append('# %s = ""\n' % (key, ))
            else:
                newlines.append('%s = "%s"\n' % (key, val))

        # Handle int keys
        if key in int_keys:
            newlines.append('%s = %s\n' % (key, val))

        # Handle boolean keys
        if key in boolean_keys:
            if val == 'TRUE':
                newlines.append('%s = true\n' % (key, ))
            else:
                newlines.append('%s = false\n' % (key, ))

    # Write out the new file
    write_config_file(filename, newlines)

def main():
    files = get_tilda_configs()

    for f in files:
        print 'Attempting to convert %s' % f
        convert_v1_to_v2(f)

if __name__ == '__main__':
    main()
