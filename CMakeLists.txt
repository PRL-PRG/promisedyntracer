cmake_minimum_required(VERSION 3.7)
project(promisedyntracer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES
        src/Context.cpp
        src/Context.h
        src/helpers.cpp
        src/init.c
        src/probes.cpp
        src/probes.h
        src/recorder.cpp
        src/recorder.h
        src/shell.c
        src/sqlite3ext.h
        src/sqlite3.c
        src/SqlSerializer.cpp
        src/SqlSerializer.h
        src/State.cpp
        src/State.h
        src/stdlibs.h
        src/tracer.cpp
        src/tracer.h
        src/utilities.cpp
        src/utilities.h)

include_directories(../R-dyntrace/include ../R-dyntrace/src/include)

add_executable(promisedyntracer ${SOURCE_FILES})