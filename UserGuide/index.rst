
.. title:: bgswitch User Guide

.. toctree::
   :maxdepth: 2
   :hidden:

.. contents:: User Guide Contents
   :depth: 3
   :local:
   :backlinks: none


Description
-----------

A small command line tool to change the desktop background settings on `Haiku <https://www.haiku-os.org>`_.
It is possible to change the wallpaper, placement mode, offset/position, text outline, and background color.
These settings can be changed for each individual workspace or all workspaces at once. This allows creating
scripts that will rotate the background.


Help Output
-----------

In addition to the main help output from ``bgswitch -h`` you can also get help from the individual subcommands,
by running ``bgswitch set -h`` for example.  ``set`` is the only subcommand with extra options at the moment.


Main help output
^^^^^^^^^^^^^^^^

.. code-block:: none

   ~> bgswitch -h
   Usage: bgswitch [--help] [--all] [--workspace VAR] [--verbose] [--debug] {clear,list,reset,set}
   
   Get/Set workspace backgrounds
   
   Optional arguments:
     -h, --help       shows help message and exits
     -a, --all        Modify all workspaces at once
     -w, --workspace  The workspace # to modify, otherwise use the current workspace
     -v, --verbose    Print extra output to screen
     -d, --debug      Print debugging output to screen
   
   Subcommands:
     clear           Make background empty (same effect as: set -f "")
     list            List background information
     reset           Reset background to global default
     set             Set workspace background options


``set`` command help output
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: none

   ~> bgswitch set -h
   Usage: set [--help] [--file VAR] [--mode VAR] [--text] [--notext] [--offset VAR...] [--color VAR...]
   
   Set workspace background options
   
   Optional arguments:
     -h, --help    shows help message and exits
     -f, --file    Path to the image file (ex. -f /path/to/file.jpg)
     -m, --mode    Placement mode 1=Manual/2=Center/3=Scale/4=Tile (ex. -m 3)
     -t, --text    Enable text outline
     -n, --notext  Disable text outline
     -o, --offset  X/Y offset in manual placement mode, separated by a space (ex. -o 200 400)
     -c, --color   Set background RGB color, separated by a space (ex. -c 20 100 255)
   
   Specify one or more of the file/mode/text/offset options


Examples
--------


Changing all workspaces
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: none

   bgswitch -a clear
   bgswitch -a list
   bgswitch -a reset
   
   # setting a file
   bgswitch -a set -f /path/to/file.jpg

   # setting a file with scaled mode and text outline
   bgswitch -a set -m 3 -t -f /path/to/file.jpg

   # setting only the text outline mode on
   bgswitch -a set -t


Changing a specific workspace (#2 in this case)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: none

   bgswitch -w 2 clear
   bgswitch -w 2 list
   bgswitch -w 2 reset
   
   # setting a file
   bgswitch -w 2 set -f /path/to/file.jpg

   # setting a file with manual mode, no text outline, and offset
   bgswitch -w 2 set -m 1 -n -o 600 400 -f /path/to/file.jpg

   # setting only tiled placement mode
   bgswitch -w 2 set -m 4


Changing the current workspace
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: none

   bgswitch clear
   bgswitch list
   bgswitch reset
   
   # setting a file
   bgswitch set -f /path/to/file.jpg

   # setting a file in centered mode with text outline
   bgswitch set -m 2 -t -f /path/to/file.jpg


Changing workspace 0
^^^^^^^^^^^^^^^^^^^^

Workspace 0 holds the global default settings.  Any other workspaces which have not been customized will follow these defaults.
Workspace 0 does not show up in the ``bgswitch -a list`` output unless the ``-v`` option is used.  Specifying ``-w 0`` is the only way
to change these global defaults as they are not changed when using the ``-a`` option.

.. code-block:: none

   bgswitch -w 0 clear
   bgswitch -w 0 list
   bgswitch -v -w 0 list
   bgswitch -v -a list
   
   # setting a file
   bgswitch -w 0 set -f /path/to/file.jpg

   # setting manual placement mode with an offset of 0 0
   bgswitch -w 0 set -m 1 -o 0 0


.. note::
   Changing even one thing of a regular workspace will cause it to stop following the global defaults and keep its own settings
   until it is ``reset``. (Color is handled differently by the system and not included in this)
