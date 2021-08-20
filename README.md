
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
separately (or perhaps download them from libretro's build bot at
https://buildbot.libretro.com/nightly/).

It is possible to run a statically built core too (that is, core.a sort of
file) by compiling it with miniretro. To do that you will need to add the
`-DSTATIC_CORE` flag to CXXFLAGS and the .a file to the LDFLAGS. You will
get a binary that ships the core.

To build miniretro for other architectures you will need a cross-toolchain
and then do something like:

```shell
  make CXX=/path/toolchains/bin/arm-linux-g++
```

Running
-------

Some example commandline:

```shell
./miniretro -c somecore.so -r  somegame.bin -f 3600 --dump-frames-every 60
```

This runs a ROM using a the given core for 3600 frames (that's 1 minute if
the core runs at 60 fps) and dumps an image every 60 frames (every second).


Regression testing
------------------

This was built with the intent to automatically test cores against a testing
suite. There's a python script provided for that purpose already, here some
examples on how can one use it for testing:

Run a bunch of ROMs for 5 minutes, in a parallel fashion:

```shell
  ./regression.py --core /path/mycore.so --system /path/systemdir \
  --input /path/dir-full-with-roms/ --output report.html  --threads=10 \
  --frames=18000
```

Run an ARM built core using qemu, like above. For this to work you will need
to build miniretro for ARM too, and have a toolchain that provides some
minimal sysroot (where libc and the dynamic linker as well as other
dependencies live).

```shell
  ./regression.py --core /path/mycore.so --system /path/systemdir \
  --input /path/dir-full-with-roms/ --output report.html  --threads=10 \
  --frames=18000 --driver "qemu-arm -L /path/to/sysroot/ ./miniretro.arm.bin
```

The tool report.py can help you generate an HTML report, and comparison reports
(this is still pretty barebones!)


