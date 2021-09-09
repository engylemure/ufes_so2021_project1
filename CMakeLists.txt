cmake_minimum_required(VERSION 3.20)
project(vsh C)

set(CMAKE_C_STANDARD 11)
set(GCC_COVERAGE_COMPILE_FLAGS "-pthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )

add_executable(vsh src/main.c src/lib/lib.c src/lib/lib.h src/lib/handlers.c src/lib/handlers.h src/lib/vec/vec.c src/lib/vec/vec.h)
