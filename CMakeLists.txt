cmake_minimum_required(VERSION 3.30)
project(t)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-lncurses")

add_executable(
                    t
                    simulation.h
                    simulation.cpp
                    main.h
                    main.cpp
        TUI.cpp
        TUI.h

)
