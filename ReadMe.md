# bgswitch

A small command line tool to change the desktop background on [Haiku](https://haiku-os.org).  This allows creating scripts that will rotate the background.


## Running

The `help` output:
```
Usage: bgswitch [--help] [--all] [--workspace VAR] [--verbose] [--debug] {clear,list,set}

Get/Set workspace backgrounds.

Optional arguments:
  -h, --help       shows help message and exits
  -a, --all        Get/Set/Clear all workspaces at once
  -w, --workspace  The workspace # to get/set/clear, otherwise use the current workspace [default: -1]
  -v, --verbose    Print extra output to screen
  -d, --debug      Print debugging output to screen

Subcommands:
  clear           Clear workspace background
  list            List background information
  set             Set workspace background
```

### Getting/Setting/Clearing A Background

For all workspaces:
```
bgswitch -a list
bgswitch -a set /path/to/file.jpg
bgswitch -a clear
```

For one specific workspace (#2 in this case):
```
bgswitch -w 2 list
bgswitch -w 2 set /path/to/file.jpg
bgswitch -w 2 clear
```

For the current workspace:
```
bgswitch list
bgswitch set /path/to/file.jpg
bgswitch clear
```

*Any type of image file can be used if Haiku has a data translator installed(jpg,png,webp,...)*


## Building

- Using cmake.  This is the primary build system at the moment.
```
~> cd bgswitch
~/bgswitch> cmake .
~/bgswitch> make
```

- Using jam.  You must have the `jamfile_engine` package installed.
```
~> cd bgswitch
~/bgswitch> jam
```


### ToDo Items
- Allow setting a wallpaper if the workspace doesn't already have one set.
- Allow changing the scaling mode, offset, and text outline
