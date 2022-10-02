# Kronos Triggered

Kronos Triggered is a modified version of the Kronos 2.3.1 build which contains modifications for lightgun support, as well as some other updates related to compiling (support for old versions of Visual C++) and OpenGL (optimized part of the code to run a little faster on my computer).

Kronos Triggered has been built and primarily tested in Windows.  Kindly let me know if you notice any bugs on Linux or other systems, and I will be happy to work together to fix them.

Everything is provided under the same GNU Public License as Yabause and Kronos, and is open source and available free of charge.

I don't have Patreon.  However, if you really enjoy my work, I would appreciate if you would consider donating any Sega Saturn hardware or software to help with this project, and any Super Nintendo hardware or software to help with some of my other projects.

Github: jamalzed		Tw: jamal_zedman		RH: Jamal

## Goals of the project

The current goals of the project are to:
- fix existing lightgun support in Yabause/Kronos (done)
- add support for using a lightgun in the 2nd player port (done)
- add support for using two lightguns at the same time, by using windows raw mouse (in progress) and linux/osx equivalents (not yet started)
- support all of the Sega Saturn lightgun games and fix any relevant bugs related to them
- fix other bugs and make other improvements as relevant

## Using the software
NOTE: the Gun/Mouse sensitivity is used to calibrate the lightgun, so this will need to be adjusted before you start.  For best results, change this value until the mouse cursor location matches with the shooting/bullet graphics.

The current status of the lightgun games tested are:
Area 51 - untested
Chaos Control Remix - untested
Crypt Killer - untested
Death Crimson - untested
Die Hard Trilogy - apparently works if you set the cartridge to "8Mbit DRAM" or "None" under Settings, have a Gamepad in the first player port and the Lightgun in the second player port, but untested
The House of the Dead (J) - works without issue so far, although has some graphical glitches on my computer (might be due to Kronos lacking support for older video cards).
Mighty Hits - untested
Maximum Force - untested
Mechanical Violator Hakaider - untested
Policenaughts - untested
Scud the Disposable Assassin - untested
Virtua Cop - untested
Virtua Cop 2 - untested

## Sega Saturn Emulated Hardware

You can read about the Sega Saturn Console and is components [here](https://www.copetti.org/writings/consoles/sega-saturn/).

## Building

### [CMake](https://cmake.org/)

The CMake system has been refined to build the full Yabause stack.
you have to download CMake and run it in the root of the first CMakeLists.txt. Currently in `/yabause/CMakeLists.txt`.

#### Platforms

Since CMake is a true cross platform build tool you can build on plenty of platforms and easily add new ones.

Currently used are:

- Windows
- Linux (Debian)

#### QT Version

You can set the CMake flag `YAB_USE_QT` to build a Standalone version which requires you to have QT5 installed

#### LibRetro - RetroArch Version

For this you need to se the CMake flag `KRONOS_LIBRETRO_CORE`. This will build a version of the core consuming LibRetro and there fore can be used with LibRetro compatible front-ends like RetroArch (which is what i test against)

##### Old make build script (obsolete)

The old build script is still there for reference. It contains custom build code for various platforms and is the base reference for newer entries in the CMake code. It is still though considered obsolete.

For retroarch core:

- mkdir build_retro; cd build_retro;
- make -j4 -C ../yabause/src/libretro/ generate-files
- make -j4 -C ../yabause/src/libretro/
- then execute retroarch -L ../yabause/src/libretro/kronos_libretro.so path_to_your_iso

## Platform Notes

### RetroArch

Is the Frontend which is used for manual tests

#### Windows

Works quite well on a modern Hardware.

#### Raspberry Pi 4 Raspberian OS

Has currently various problems running and is not usable.

- The Shaders are too complex and Mesa can not handle that. It will fail with a driver crash with Mesa 19.2
- Code is to demanding on the CPU by now even with frameskip 5 there is no acceptable framerate even on 2d games.

## Outdated Reference Documentation

- [Description](/yabause/blob/kronos-cmake_ci/yabause/README.DC)
- [Dreamcast](/yabause/blob/kronos-cmake_ci/yabause/README.DC)
- [Linux](/yabause/blob/kronos-cmake_ci/yabause/README.QT)
- [QT](/yabause/blob/kronos-cmake_ci/yabause/README.QT)
- [Windows](/yabause/blob/kronos-cmake_ci/yabause/README.WIN)

## Contributing

To generate a changelog, add in your commits the [ChangeLog] tag. Changelog will be extracted like this

  `git shortlog --grep=Changelog --since "01 Jan 2020"`
