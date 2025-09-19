# Galaxy Visualization

## The Course

This project visualizes 100,000 real galaxies in blue and 100,000 randomly distributed galaxies in red. The data is sourced from the GPU programming course at: [Åbo Akademi University](https://studiehandboken.abo.fi/en/course/IT00CG19/19162?period=2024-2027)

The assignment is to use parallel programming with CUDA (or HIP) to calculate 10 billion angles between galaxies and prove they are not randomly distributed. 
The students must leverage the GPU for these calculations on their own using a smaller compute cluster or supercomputer.

### Note:
- The students have to prove this on their own.
- The expected runtime for the calculation is approximately 3 seconds.
    - It is possible to optimize the runtime to 0.85 seconds on a single GPU (my own solution tested with one AMD RX 6900 XT).
- **This program is a visualization of the data, not the solution to the assignment.**


## Prerequisites

This project can be built using clang through build.sh (raylib is the only dependency) or use Meson as the build system.


## Dependencies

- **Raylib** (Tested with v5.5-1)
- **Base Development Tools** (e.g., `build-essential` on Ubuntu)
- **Meson**
- **Git**
- **Clang** (or GCC)
- **gdb** (optional)

## Installation Commands

### Arch Linux

```bash
sudo pacman -S raylib base-devel meson git clang
```

### Ubuntu

```bash
sudo apt-get install -y raylib build-essential meson git clang
```

### Build and Run the Project

You can choose from one of the following build systems:
- Meson (F5 in VSCode)
- build.sh


### Meson
```bash
# Setup
meson setup build --buildtype=release 

# Build
meson compile -C build

# Run
./build/galaxy_visualization_raylib

# Clean
meson compile -C build --clean
```

### Bash
```bash
# Build
chmod +x ./build.sh

# Run
./build/galaxy_visualization_raylib
```

##  Demo

![demo](assets/images/galaxy_demo.gif "galaxy_demo.gif")

---

![screen](assets/images/screenshot.png "screenshot.png")

## Data sources
- Added earth model: https://science.nasa.gov/resource/earth-3d-model/ | Credit: NASA Visualization Technology Applications and Development (VTAD).
- Redshift data: https://lweb.cfa.harvard.edu/~dfabricant/huchra/zcat/seyfert.dat
