# Specify the minimum version of CMake required
cmake_minimum_required(VERSION 3.10)

# Define the project name
project(ParserProject)

# Find the LLVM package and load its configuration
find_package(LLVM REQUIRED CONFIG)

# Print out the variables to understand what is set by LLVMConfig.cmake
message(STATUS "LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM_DEFINITIONS: ${LLVM_DEFINITIONS}")
message(STATUS "LLVM_LIBRARY_DIRS: ${LLVM_LIBRARY_DIRS}")
message(STATUS "LLVM_VERSION: ${LLVM_VERSION}")

# Include the directories provided by LLVM
include_directories(${LLVM_INCLUDE_DIRS})

# Add any definitions provided by LLVM
add_definitions(${LLVM_DEFINITIONS})

# Define the executable target
add_executable(parser parser.cc)

# Link the LLVM libraries to the target executable
target_link_libraries(parser LLVM)