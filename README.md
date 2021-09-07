# ufes_so2021_project1

First project for the Operational System class `Vaccine Shell`


### Usage

#### Make

To run it is necessary to have some `c` compiler available and also `makefile` installed on your computer, 
by default it will use the `clang` compiler. Basically you can run the command `make all` to compile the project
that will generate a `target` directory that will contain the binary `vsh` and then you can just execute it.

#### CMake

To build the project using CMake you can execute these commands to generate `build` directory and compile the project
with the binary named `vsh` into this `build` directory

```bash
mkdir -p build && cd build && cmake build ../ && cmake --build .
```

After this you can just execute it.