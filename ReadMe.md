# bgswitch

A small command line tool to change the desktop background settings on [Haiku](https://www.haiku-os.org).
This allows creating scripts that will rotate the background.


# Help Output

### The main `help` output (i.e. `bgswitch -h`):
```console
Usage: bgswitch [--help] [--all] [--workspace VAR] [--verbose] [--debug] {clear,list,reset,set}

Get/Set workspace backgrounds.

Optional arguments:
  -h, --help       shows help message and exits
  -a, --all        Modify all workspaces at once
  -w, --workspace  The workspace # to modify, otherwise use the current workspace [default: -1]
  -v, --verbose    Print extra output to screen
  -d, --debug      Print debugging output to screen

Subcommands:
  clear           Make background empty (same effect as: set "")
  list            List background information
  reset           Reset background to global default
  set             Set background
```

### The `help` output for the `set` command (i.e. `bgswitch set -h`):
```console
Usage: set [--help] [--file VAR] [--mode VAR] [--text] [--notext] [--offset VAR...]

Set workspace background options

Optional arguments:
  -h, --help    shows help message and exits
  -f, --file    Path to the image file (ex. -f /path/to/file.jpg) [default: ""]
  -m, --mode    Placement mode 1=Manual/2=Center/3=Scale/4=Tile (ex. -m 3)
  -t, --text    Enable text outline
  -n, --notext  Disable text outline
  -o, --offset  X/Y offset in manual placement mode, separated by a space (ex. -o 200 400) [nargs: 2]

Specify one or more of the file/mode/text/offset options
```


# Examples

## Changing all workspaces:
```console
bgswitch -a clear
bgswitch -a list
bgswitch -a reset

# setting a file
bgswitch -a set -f /path/to/file.jpg
# setting a file with scaled mode and text outline
bgswitch -a set -m 3 -t -f /path/to/file.jpg
# setting only the text outline mode on
bgswitch -a set -t
```


## Changing a specific workspace (#2 in this case):
```console
bgswitch -w 2 clear
bgswitch -w 2 list
bgswitch -w 2 reset

# setting a file
bgswitch -w 2 set -f /path/to/file.jpg
# setting a file with manual mode, no text outline, and offset
bgswitch -w 2 set -m 1 -n -o 600 400 -f /path/to/file.jpg
# setting only the placement mode
bgswitch -w 2 set -m 3
```


## Changing the current workspace:
```console
bgswitch clear
bgswitch list
bgswitch reset

# setting a file
bgswitch set -f /path/to/file.jpg
# setting a file in centered mode with text outline
bgswitch set -m 2 -t -f /path/to/file.jpg
```


## Changing workspace 0:
Workspace 0 holds the global default settings.  Any other workspaces which have not been customized will follow these defaults.
Workspace 0 does not show up in the `bgswitch -a list` output unless the `-v` option is used.  Specifying `-w 0` is the only way
to change these global defaults as they are not changed when using the `-a` option.
```console
bgswitch -v -a list
bgswitch -v -w 0 list
bgswitch -w 0 list

# setting a file
bgswitch -w 0 set -f /path/to/file.jpg
# setting manual placement mode with an offset of 0 0
bgswitch -w 0 set -m 1 -o 0 0
```
*Note: Changing even one thing of a regular workspace will cause it to stop following the global defaults and keep its own settings
until it is `reset`.*


# Building

## Using cmake

This is the primary build system at the moment.

```console
~> cd bgswitch
~/bgswitch> cmake .
~/bgswitch> make
```


## Using jam

You must have the `jamfile_engine` package installed.

```console
~> cd bgswitch
~/bgswitch> jam
```
