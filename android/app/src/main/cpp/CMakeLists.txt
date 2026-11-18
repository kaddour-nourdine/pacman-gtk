cmake_minimum_required(VERSION 3.22.1)

project("pacman")

add_library(${CMAKE_PROJECT_NAME} SHARED
    native-lib.cpp
    ../../../../../gamemodel.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ../../../../../
)

find_library(log-lib log)

target_link_libraries(${CMAKE_PROJECT_NAME}
    ${log-lib}
)
