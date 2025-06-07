cmake_minimum_required(VERSION 3.30)
project(t)

set(CMAKE_CXX_STANDARD 17)



include_directories(/home/linuxbrew/.linuxbrew/include)
link_directories(/home/linuxbrew/.linuxbrew/lib)


add_executable(
                    t
        simulation.h
        simulation.cpp
#                    main.h
        main.cpp
        TUI.cpp
        TUI.h

)


target_link_libraries(t ncurses)