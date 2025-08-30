cmake_minimum_required(VERSION 3.16)

# Project configuration
project(SEP-UG-33-BlockModel
    VERSION 1.0.0
    DESCRIPTION "3D Block Model Compression Algorithm"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable compile_commands.json for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(MSVC)
    # MSVC-specific options
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /RTC1")
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob2 /DNDEBUG")
else()
    # GCC/Clang options
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
    
    # Additional warnings for better code quality
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow -Wformat=2 -Wcast-align")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion -Wsign-conversion")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnull-dereference -Wdouble-promotion")
endif()

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    if(MINGW)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libstdc++ -static-libgcc")
    endif()
elseif(UNIX)
    # Unix-specific settings (Linux, macOS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

# vcpkg integration (if available)
if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# Find packages (will be managed by vcpkg when available)
# Currently this project uses standard library only, but ready for future dependencies

# Include directories
include_directories(include)

# Source files
set(CORE_SOURCES
    src/block.cpp
    src/block_growth.cpp
    src/block_model.cpp
)

set(MAIN_SOURCE src/main.cpp)

# Create a library for core functionality (useful for testing)
add_library(block_model_core STATIC ${CORE_SOURCES})
target_include_directories(block_model_core PUBLIC include)

# Main executable
add_executable(block_model ${MAIN_SOURCE})
target_link_libraries(block_model PRIVATE block_model_core)

# Test executables
enable_testing()

# Compression test
add_executable(compression_test tests/compression_test.cpp)
target_link_libraries(compression_test PRIVATE block_model_core)
target_include_directories(compression_test PRIVATE include)

# Validation test
add_executable(validate_test tests/validate_test.cpp)
target_include_directories(validate_test PRIVATE include)

# Test data files
file(COPY tests/data DESTINATION ${CMAKE_BINARY_DIR}/tests)

# CTest integration
add_test(NAME CompressionTests COMMAND compression_test)

# ValidationTests - Skip standalone test since it requires input
# The validation functionality is tested via integration tests

# Integration tests that pipe main program output to validation
add_test(NAME IntegrationTestCase1 
    COMMAND ${CMAKE_COMMAND} -E env
    bash -c "$<TARGET_FILE:block_model> < ${CMAKE_BINARY_DIR}/tests/data/case1.txt | $<TARGET_FILE:validate_test>"
)

add_test(NAME IntegrationTestCase2 
    COMMAND ${CMAKE_COMMAND} -E env
    bash -c "$<TARGET_FILE:block_model> < ${CMAKE_BINARY_DIR}/tests/data/case2.txt | $<TARGET_FILE:validate_test>"
)

# Custom targets for convenience (similar to Makefile targets)
add_custom_target(run-case1
    COMMAND ${CMAKE_COMMAND} -E env
    bash -c "$<TARGET_FILE:block_model> < ${CMAKE_SOURCE_DIR}/tests/data/case1.txt"
    DEPENDS block_model
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running block_model with case1.txt"
)

add_custom_target(run-case2
    COMMAND ${CMAKE_COMMAND} -E env
    bash -c "$<TARGET_FILE:block_model> < ${CMAKE_SOURCE_DIR}/tests/data/case2.txt"
    DEPENDS block_model
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running block_model with case2.txt"
)

add_custom_target(test-all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS compression_test validate_test block_model
    COMMENT "Running all tests"
)

# Installation rules
install(TARGETS block_model 
    RUNTIME DESTINATION bin
)

install(TARGETS block_model_core
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Package configuration
set(CPACK_PACKAGE_NAME "SEP-UG-33-BlockModel")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "SEP-UG-33 Team")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)

# Development tools
add_custom_target(format
    COMMAND find src include tests -name "*.cpp" -o -name "*.h" | xargs clang-format -i
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Formatting source code with clang-format"
)

add_custom_target(lint
    COMMAND find src include tests -name "*.cpp" -o -name "*.h" | xargs clang-tidy
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-tidy static analysis"
)

# Print configuration summary
message(STATUS "=== Build Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()
message(STATUS "===========================")
