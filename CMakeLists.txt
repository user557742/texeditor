cmake_minimum_required(VERSION 3.16)
project(LIDE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# SDL core
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

# SDL_ttf
add_subdirectory(vendored/SDL_ttf EXCLUDE_FROM_ALL)

add_executable(
    LIDE WIN32
    src/main.cpp
)

target_link_libraries(LIDE PRIVATE
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
)
