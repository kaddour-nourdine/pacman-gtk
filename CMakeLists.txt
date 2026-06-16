cmake_minimum_required(VERSION 3.16)
project(pacman-gtk)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 gtk4 REQUIRED)

add_executable(pacman-gtk
    main_gtk.cpp
    gamemodel.cpp
    gamemodel.h
)
target_include_directories(pacman-gtk PRIVATE ${GTK4_INCLUDE_DIRS})
target_link_libraries(pacman-gtk PRIVATE ${GTK4_LIBRARIES})
target_compile_options(pacman-gtk PRIVATE ${GTK4_CFLAGS_OTHER})

# Add unit tests
add_executable(pacman-tests
    tests/test_gamemodel.cpp
    gamemodel.cpp
    gamemodel.h
)
target_include_directories(pacman-tests PRIVATE ${CMAKE_SOURCE_DIR})
