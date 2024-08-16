# Galaxy Visualization

This project visualizes 100,000 real galaxies in blue and 100,000 randomly distributed galaxies in red. The data is sourced from the GPU programming course at [Åbo Akademi University](https://studiehandboken.abo.fi/sv/kurs/IT00CG19/19162?period=2024-2027).

## Prerequisites

This project can be built using Meson, CMake, or Make. Below are the installation instructions for the required dependencies:

### Dependencies

- **Raylib**
- **Base Development Tools** (e.g., `build-essential` on Ubuntu)
- **Meson**
- **CMake**
- **Git**
- **Clang** (or GCC)

### Installation Commands

#### Arch Linux

```bash
sudo pacman -S raylib base-devel meson git cmake clang
```

#### Ubuntu

```bash
sudo apt-get install -y raylib build-essential meson git cmake clang
```

### Build and Run the Project

You can choose from one of the following build systems:

```bash
meson setup build --buildtype=release

meson compile -C build && ./build/galaxy_visualization_raylib
```

```bash
make

make run

make clean
```

#### Windows

```powershell
build.bat
```


##  Demo

![demo](demo.gif "demo.gif")

---

![screen](screenshot.png "screenshot.png")

## Patch Notes

- Spacebar: Pauses the program.
- Added automated setup support for both Clang and GCC (build.sh).
- Added automated setup support for Windows through build.bat with CMake.
- Added Makefile for Linux.
- Added Meson build system for Linux.
- The Meson build now links against the system-installed Raylib.
