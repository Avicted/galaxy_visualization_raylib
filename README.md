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
- Additional redshift data from various surveys has been included for visualization purposes.

## Prerequisites

This project can be built using clang/gcc through build.sh (raylib is the only dependency) or use Meson as the build system.


## Dependencies

- **Git**
- **Base Development Tools** (e.g. `build-essential` on Ubuntu)
- **Raylib** (Tested with v5.5-1)
- **Clang**  (or GCC)
- **Meson**  (optional)
- **gdb**    (optional)

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
./build.sh

# Run
./build/galaxy_visualization_raylib
```

## Examples

![real_data](assets/images/real_data.png "real_data.png")
![real_data](assets/images/redshift_data.png "redshift_data.png")

---

## Data Sources & Attribution

This repository contains **code and derived galaxy redshift data**.  
All original data are the property of their respective surveys and authors.  
This project **does not claim credit for the original data**.

### CfA Redshift Survey

- **Original source:** ZCAT, Harvard–Smithsonian Center for Astrophysics  
- **URL:** [https://lweb.cfa.harvard.edu/~dfabricant/huchra/zcat/](https://lweb.cfa.harvard.edu/~dfabricant/huchra/zcat/)  
- **Modifications:** Only essential fields retained (e.g., galaxy ID and redshift); extraneous data removed for this project.

**Reference:**  

> Huchra, J., Davis, M., Latham, D., & Tonry, J. (1983).  
> *A survey of galaxy redshifts. I.*  
> **Astrophysical Journal Supplement Series**, 52, 89.  
> [ADS link](https://ui.adsabs.harvard.edu/abs/1983ApJS...52...89H)

---

### SAGA Survey

- **Original source:** SAGA Survey III — Satellite Systems around Milky Way–mass Galaxies  
- **URL / DOI:** [DOI:10.3847/1538-4357/ad64c4](https://doi.org/10.3847/1538-4357/ad64c4)  
- **Modifications:** Only published redshifts relevant to this project are included; all other data excluded.

**Reference:**  

> Mao, Y.-Y., Geha, M., Wechsler, R. H., Asali, Y., Wang, Y., Kado-Fong, E., Kallivayalil, N., Nadler, E. O., Tollerud, E. J., Weiner, B., de los Reyes, M. A. C., & Wu, J. F. (2024).  
> *The SAGA Survey. III. A Census of 101 Satellite Systems around Milky Way–mass Galaxies.*  
> **The Astrophysical Journal**, 976(1), 117.  
> [ADS link](https://ui.adsabs.harvard.edu/abs/2024ApJ...976..117M) | [arXiv:2404.14498](https://arxiv.org/abs/2404.14498)

---

### Usage & Citation

- This repository contains **code and derived datasets only**.  
- **Original surveys must always be cited** when using the included redshift data.  
- If you use this repository for code or its cleaned dataset, you may cite it via the [`CITATION.cff`](CITATION.cff) file—but this does **not replace citing the original surveys**.


## License
MIT License - see the [LICENSE](LICENSE) file for details