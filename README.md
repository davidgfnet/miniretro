
miniretro
=========

This is a small (Linux only for now) CLI libretro frontend.

The aim is to allow for automated testing of libretro cores in a simpler way
(vs using the full fledged Retroarch frontend). Also it can serve as an example
on how to implement a libretro Frontend that's simple enough for beginners to
read and understand.

It should run on any POSIX compatible system (presumably) but might only
work for cores that do not use hardware acceleration.


Building
--------

Just do a "make" and you are good. If you want to cross-build the tool use
something like "make CXX=arm-linux-g++". You will need to build the cores
separatedly (or perhaps download them from libretro's build bot at
https://buildbot.libretro.com/nightly/).

It is possible to run a statically built core too (that is, core.a sort of
file) by compiling it with miniretro. To do that you will need to add the
`-DSTATIC_CORE` flag to CXXFLAGS and the .a file to the LDFLAGS. You will
get a binary that ships the core.


Running
-------

Some example commandline:

```shell
./miniretro -c somecore.so -r  somegame.bin -f 3600 --dump-frames-every 60
```

This runs a ROM using a the given core for 3600 frames (that's 1 minute if
the core runs at 60 fps) and dumps an image every 60 frames (every second).


