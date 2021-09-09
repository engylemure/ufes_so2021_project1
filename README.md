# ufes_so2021_project1

First project for the Operational System class `Vaccine Shell`


### Usage

To run it is necessary to have some `c` compiler available and also `makefile` installed on your computer.
#### Make

By default it will use the `clang` compiler if it isn't found into the system it will try to use `gcc` or the `cc` available compilers. 
Basically you can run the command `make all` to compile the project that will generate a `target` directory and will contain the binary 
`vsh` and then you can just execute it.

#### CMake

To build the project using CMake you can execute these commands to generate `build` directory and compile the project
with the binary named `vsh` into this `build` directory

```bash
mkdir -p build && cd build && cmake build ../ && cmake --build .
```

After this you can just execute it.


### Documentation

Some of the documentation related to allocation of objects are ommited since they seem unneceessary and are cumbersome,
other than that I've tried to add some minimum documentation to all function and objects into this project.

I've tried to initially mimic a basic behavior of the shell that I've used and after that I tried to follow each of the 
project requirements, I've made some improvements and refactoring along the way.