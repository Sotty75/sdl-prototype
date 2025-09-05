# Minimum version required for cmake
cmake_minimum_required(VERSION 3.15)

# Project definition: name and language
project(sdl-prototype LANGUAGES C)

# C Version Requried
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# SDL3 library path
set(SDL3_DIR "F:\\Coding\\libraries\\SDL3\\cmake")

# Get SDL library package
find_package(SDL3 REQUIRED CONFIG)

# Define executable and entry point for the application
add_executable(sdl-prototype main.c)

# Link the SDL library to the target executable
target_link_libraries(sdl-prototype PRIVATE SDL3::SDL3)