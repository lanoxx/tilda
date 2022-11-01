# Sessions

Current support for sessions is rather proof-of-concept than a proper solution, nonetheless - it works.

## Usage

In order to use Tilda with saving sessions - session file must be prepared manually. Currently:

- it supports comments on lines starting with '#'
- expects format of `tab N { dir = ... txt = ... cmd = ... args = ... }`
- strings with spaces inside must be quoted
- it should be possible to omit any of arguments

Such file can be loaded to Tilda:

- either from the command line, with `-e` or `--session-file` switch
- or via `config_{N}` where it looks for "session_file" key


## Functionality

Currently, on a successfull load, Tilda will open N tabs, change directory to each `dir`, try to set each label to `txt` and run cmd with args.

Design was initially intended to be quite small. It might make more sense to consider either:

- copy of config mechanisms (rather big code bloat)
- adjustments in config, separation of common file ops, for config and session
- poisioning of either config unit (with tilda_window header) or tilda.c with config header, types, iteration

Currently, tilda.c passes function pointer callback to session file reader, which calls it on each discovered tab.


## Known bugs, limitations

- Label is not set, when a cmd is empty or is a plain shell.
- commands, after exiting - close tab, so they don't drop to a shell ( which could be a bug or a feature ;) )
- parsing paths is limited, relative / full paths should work though

- there is no save/load session with a key/mouse from inside Tilda.
  While this could be probably added, it would require some redesign - session file would need to be guarded with mutex,
  and some ux thinking - the file probably should not simply "save" itself periodically, or on each adding/removing tab.
