# Top-level CMakeLists.txt

cmake_minimum_required(VERSION 3.27.7)
project(CustomMallocImpl LANGUAGES CXX)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/entry.cpp
    src/malloc.cpp
)

# Add the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Testing configuration
enable_testing()

# Use CMake's FetchContent to include Google Test
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.11.0
)

FetchContent_MakeAvailable(googletest)

# Create a library
add_library(custom_malloc_impl_library ${SOURCES})

# Include directories for the library
target_include_directories(custom_malloc_impl_library PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Add the test subdirectory
add_subdirectory(test)
