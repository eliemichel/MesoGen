MesoGen
=======

*The official implementation of [MesoGen: Designing Procedural On-Surface Stranded Mesostructures](https://eliemichel.github.io/MesoGen) (SIGGRAPH 2023).*

![Screenshot](doc/screenshot.jpg)

### Downloading

You can either use the [Download](https://github.com/eliemichel/MesoGen/archive/refs/heads/main.zip) button or use git to get the source code:

```
git clone https://github.com/eliemichel/MesoGen.git
cd MesoGen
```

### Dependencies

To build this project, you need:

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

### Usage

TODO
