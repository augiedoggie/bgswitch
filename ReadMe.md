## bgswitch

A small command line tool to change the desktop background on [Haiku](https://haiku-os.org).  This allows creating scripts that will rotate the background.


### Building

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
- Documentation and examples
- Allow setting a wallpaper if the workspace doesn't already have one set.
- Allow changing the scaling mode, offset, and text outline
