MesoGen
=======

*The official implementation of [MesoGen: Designing Procedural On-Surface Stranded Mesostructures](https://eliemichel.github.io/MesoGen) (SIGGRAPH 2023).*

![Screenshot](doc/screenshot.jpg)

Table of Contents
-----------------

 - [What is this?](#what-is-this)
 - [Usage](#usage)
   + [Download](#download)
   + [Instruction](#instructions)
 - [Building from source](#buliding-from-source)
   + [Download sources](#download-source)
   + [Dependencies](#dependencies)
   + [Building](#building)
   + [Running](#running)
   + [Sharing](#sharing)
 - [License](#license)

What is this?
-------------

This is a research prototype, meant to demonstrate the ideas presented in the paper [*MesoGen*](https://eliemichel.github.io/MesoGen) and provide a reference implementation to compare others with.

It is important to note that it is not meant to be full featured production tool, so the user experience could be better. It is rather a tool used internally while developing the paper to iterate on ideas and compare various options.

Nevertheless, it is usable as it is, and we are open to adding new features meant to make its use easier by non-technicians.

Usage
-----

### Download

Precompiled binaries are only available for Windows, see the [last release](https://github.com/eliemichel/MesoGen/releases/latest). On any other OS, you need to build from source.

**NB** If you happen to create a build for another OS, you are welcome to share it with us so that we can add it to the release downloads!

### Instruction

TODO

Building from source
--------------------

### Download

You can either use the [Download](https://github.com/eliemichel/MesoGen/archive/refs/heads/main.zip) button or use `git` to get the source code:

```
git clone https://github.com/eliemichel/MesoGen.git
cd MesoGen
```

### Dependencies

This projects includes most of its batteries, all you need besides the source code is:

 - [CMake](https://cmake.org/) for configuration.
 - [Python](https://www.python.org/) to generate glad headers.
 - The `xorg-dev` package if on linux (this is the Debian name of the package, something similar exists for other distributions).

**NB** In theory the build should work on either Windows, macOS and linux, although it's been mostly tested on Windows so some minor compiler-specific fixes may be needed.

### Building

This is a standard CMake project. Building it consits in running:

```bash
# Configure (create a 'build/' directory)
cmake -B build

# Build (create the executables)
cmake --build build --config Release
```

You can chose which compiler to use in the first call to `cmake`, using the `-G` option. See for instance [`build-msvc16.bat`](build-msvc16.bat) to build for Visual Studio 15 (2017), or the other build files for `mingw` or `gcc`. More easily, you can also just run one of those scripts, it will even get git submodules.

**NB** It is recommended to build in **Release mode** because the tiling library is slow in debug mode.

### Running

The executable file is created within the `build` directory, in `src/MesostructureGenerator` or `src/MesostructureGenerator/Debug` or `src/MesostructureGenerator/Release` or something similar depending on your compiler.

**NB** Run from the `share/samples` directory in order to easily load example scenes (using default values in the UI).

### Sharing

**By default**, the build is configured in **development mode**, which has the path to shaders be hardcoded in order to enable easy reload from source tree (by hitting F5).

This is nice **except when you wen to share the build with other people**. When so, turn the dev mode off upon using CMake:

```
cmake -B build-shared -DDEV_MODE=OFF
```

Make sure to copy the `share/` directory in the release zip, next to `MesostructureGenerator.exe`. You may also copy the content of the `share/samples` directory to enable easy loading from the default values.

License
-------

This repository as a whole is shared under the terms of the MIT license. For more information, see [`LICENSE`](LICENSE). It includes dependencies that are individually licensed under similar licenses, see each directory's LICENSE file.
