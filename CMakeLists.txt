cmake_minimum_required(VERSION 3.16)
project(BlockModel VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find vcpkg toolchain if available
if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# Try to find TBB
find_package(TBB QUIET)

# Try to find DirectXMath
find_package(directxmath QUIET)

# Source files
set(SOURCES
    src/main.cpp
    src/block.cpp
    src/block_growth.cpp
    src/block_model.cpp
)

# Include directories
include_directories(include)

# Create executable
add_executable(block_model ${SOURCES})

# Compiler-specific optimizations
if(MSVC)
    target_compile_options(block_model PRIVATE 
        /O2 /Oi /Ot /Oy /GL /arch:AVX2
    )
    target_link_options(block_model PRIVATE /LTCG)
    message(STATUS "Using MSVC optimizations")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(block_model PRIVATE 
        -O3 -march=native -flto -funroll-loops -ffast-math
    )
    target_link_options(block_model PRIVATE -flto)
    message(STATUS "Using GCC optimizations")
endif()

# TBB integration
if(TBB_FOUND)
    target_link_libraries(block_model PRIVATE TBB::tbb)
    target_compile_definitions(block_model PRIVATE USE_TBB)
    message(STATUS "TBB found - enabling parallel processing")
    
    # For static linking on Windows
    if(WIN32 AND MSVC)
        set_property(TARGET block_model PROPERTY 
            MSVC_RUNTIME_LIBRARY "MultiThreaded")
    endif()
else()
    message(STATUS "TBB not found - using single-threaded fallback")
endif()

# DirectXMath integration
if(directxmath_FOUND)
    target_link_libraries(block_model PRIVATE Microsoft::DirectXMath)
    target_compile_definitions(block_model PRIVATE USE_DIRECTXMATH)
    message(STATUS "DirectXMath found - enabling SIMD optimizations")
else()
    message(STATUS "DirectXMath not found - using standard optimizations")
endif()

# Windows-specific settings
if(WIN32)
    # Static runtime for standalone executable
    if(MSVC)
        foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            if(${flag_var} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif()
        endforeach()
    endif()
    
    # Set output name
    set_target_properties(block_model PROPERTIES OUTPUT_NAME "block_model")
endif()

# Installation (for packaging)
install(TARGETS block_model 
    RUNTIME DESTINATION bin
)

# Print configuration summary
message(STATUS "========================================")
message(STATUS "Block Model Build Configuration")
message(STATUS "========================================")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "TBB Support: ${TBB_FOUND}")
message(STATUS "DirectXMath SIMD: ${directxmath_FOUND}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Target Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "========================================")
