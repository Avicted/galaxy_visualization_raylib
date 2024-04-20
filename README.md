# Galaxy Visualization

Data taken from the course GPU programming at: https://studiehandboken.abo.fi/sv/kurs/IT00CG19/19162

Visualization of 100k real galaxies in blue and 100k red randomly distributed galxies.


# Prerequisites

```bash
base-devel cmake clang git
```

# Build and run

Linux:

```bash
./build.sh
```

Windows:

```cmd
build.bat
```

# Demo

![demo](demo.gif "demo.gif")


# Patch notes

-   Space pauses the program
-   Added automated setup support for both Clang and GCC (build.sh)
-   Added automated setup support for Windows through build.bat and the MSVC compiler

---

![screen](screenshot.png "screenshot.png")