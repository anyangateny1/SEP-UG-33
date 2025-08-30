# SEP-UG-33 Block Model Project

A C++ implementation of a 3D block model compression algorithm that reads model specifications and performs block fitting and growth operations.

## Project Overview

This project implements a block-based 3D model compression system with multi-threading support:

1. **Input**: 3D model data with specifications, tag tables, and voxel grids
2. **Processing**: Block fitting and growth algorithm for optimal compression
3. **Output**: Compressed block format (`x,y,z,width,height,depth,label`)
4. **Validation**: Reconstruction verification to ensure data integrity
5. **Performance**: Multi-threaded processing for enhanced speed while preserving correctness

The system is designed with separation of concerns:
- **Core Algorithm** (`src/`): Block model processing and compression with threading support
- **Testing** (`tests/`): Comprehensive unit, integration, and threading tests
- **Build System** (`CMake`): Modern cross-platform compilation and automation
- **Documentation** (`docs/`): Detailed implementation and usage guides

## Project Structure

```
SEP-UG-33/
├── src/                    # Source files (.cpp)
│   ├── main.cpp           # Main entry point
│   ├── block.cpp          # Block class implementation
│   ├── block_growth.cpp   # Block growth algorithm
│   └── block_model.cpp    # Model reading and processing
├── include/               # Header files (.h)
│   ├── block.h
│   ├── block_growth.h
│   └── block_model.h
├── tests/                 # Test files and data
│   ├── validate_test.cpp  # 3D model validation test
│   ├── compression_test.cpp # Compression algorithm unit tests
│   └── data/             # Test case data
│       ├── case1.txt
│       └── case2.txt
├── docs/                  # Documentation
│   └── compileHelpNotes.txt
├── build/                 # Build output directory
├── Makefile              # Build configuration
└── README.md             # This file
```

## Multi-Threading Features

This implementation includes multi-threading support to improve compression performance:

### Usage

```bash
# Multi-threaded with auto thread detection (default)
./build/block_model < tests/data/case1.txt

# Multi-threaded with specific thread count
./build/block_model --threads 4 < tests/data/case1.txt

# Single-threaded mode (opt-in for testing/debugging)
./build/block_model --single-threaded < tests/data/case1.txt
```

### Performance Testing

```bash
# Run performance benchmarks
cmake --build build --target benchmark-performance

# Run threading correctness tests
cmake --build build --target test-threading
```

### Benefits

- **Parallel Processing**: Parent blocks are processed concurrently
- **Thread Safety**: No race conditions or deadlocks
- **Deterministic Output**: Results maintain exact ordering
- **Scalable Performance**: Typically 2x-6x speedup on multi-core systems

See [MULTITHREADING.md](docs/MULTITHREADING.md) for detailed documentation.

## Building the Project

### Modern Build System (CMake) - Recommended

The project now uses CMake as the primary build system for better cross-platform support and modern C++ toolchain integration.

#### Prerequisites

- CMake 3.16+
- C++17 compatible compiler (GCC 7+, Clang 7+, MSVC 2019+)
- Ninja build system (recommended) or Make
- Optional: vcpkg for dependency management

#### Quick Start

```bash
# Clone and enter the project
cd SEP-UG-33

# Set up development environment (automated)
./scripts/setup-dev-environment.sh

# Or manual setup:
# Configure and build
cmake --preset release
cmake --build build/release

# Run tests
ctest --test-dir build/release

# Run with sample data
cmake --build build/release --target run-case1
```

#### Build Commands

```bash
# Linux development builds
cmake --preset debug           # Debug build (Linux)
cmake --preset release         # Release build (Linux) 
cmake --build build/release    # Build Linux executable

# Windows submission (choose one method)
cmake --preset windows-mingw   # Windows cross-compilation (CMake)
cmake --build build/windows-mingw
# OR (easier)
make windows-package           # One-command Windows .exe.zip creation

# Testing
ctest --test-dir build/release --output-on-failure
cmake --build build/release --target test-all

# Utility targets
cmake --build build/release --target run-case1
cmake --build build/release --target run-case2
cmake --build build/release --target format  # Code formatting
cmake --build build/release --target lint    # Static analysis
```

### Legacy Build System (Makefile) - Still Supported

For compatibility, the original Makefile is still available:

#### Prerequisites

- g++ compiler with C++17 support
- make utility
- For Windows cross-compilation: mingw-w64

```bash
# Install dependencies on Ubuntu/Debian
make install-deps

# Build everything and run tests
make test-all

# Create Windows package for submission
make windows-package
```

### Windows Deployment

For easy Windows deployment and submission:

```bash
# One-command Windows packaging
make windows-package

# This will:
# 1. Check/install MinGW-w64 cross-compiler
# 2. Compile with: x86_64-w64-mingw32-g++ -std=c++17 -O2 -static -static-libstdc++ -static-libgcc
# 3. Create block_model.exe.zip with no folder structure
# 4. Ready for submission: build/block_model.exe.zip
```

### Running the Project

```bash
# Run with test data
make run-case1
make run-case2

# Or run directly
./build/block_model < tests/data/case1.txt
```

## Testing

This project includes a comprehensive test suite with two distinct test programs:

### Test Programs

1. **`compression_test.cpp`** - Unit tests for the compression algorithm
   - Tests BlockModel functionality directly
   - Validates compression with test case data
   - Counts generated blocks and output

2. **`validate_test.cpp`** - Integration tests for output validation
   - Takes compressed block output (format: `x,y,z,width,height,depth,label`)
   - Reconstructs 3D model from compressed blocks
   - Outputs visual representation to verify correctness

### Test Commands

#### Comprehensive Testing
```bash
make test-all              # Run all tests (unit + integration)
make test                  # Build both test executables
```

#### Unit Testing (Compression Algorithm)
```bash
make test-compression-unit # Test compression algorithm directly
make run-compression-test  # Same as above
```

#### Integration Testing (End-to-End Pipeline)
```bash
make test-integration      # Run compression → validation pipeline
make validate-case1        # Validate main program output with case1.txt
make validate-case2        # Validate main program output with case2.txt
```

#### Individual Test Programs
```bash
make run-validate-test     # Interactive validation test (stdin input)
make run-compression-test  # Run compression unit tests
```

### Test Data

Test data is located in `tests/data/`:
- `case1.txt` - Test case with smaller dimensions (64x8x5)
- `case2.txt` - Test case with larger dimensions (64x16x5)

### Example Test Output

**Compression Unit Tests:**
```bash
$ make test-compression-unit
=== Compression Test Suite ===
✓ Basic compression test passed
✓ Case1 compression test passed - generated 86 blocks
✓ Case2 compression test passed - output length: 2134 chars
All compression tests passed!
```

**Integration Tests:**
```bash
$ make test-integration
Running integration tests (compression + validation)...
✓ Case 1 integration passed
✓ Case 2 integration passed
All integration tests completed!
```

**Complete Test Suite:**
```bash
$ make test-all
# Runs compression unit tests + integration tests
# Shows comprehensive pass/fail status for all components
```

## Development Standards

This project follows comprehensive development standards to ensure code quality, consistency, and maintainability. See detailed documentation:

- **[Development Standards](docs/DEVELOPMENT_STANDARDS.md)** - Complete development guidelines
- **[Developer Setup](docs/DEVELOPER_SETUP.md)** - Environment setup instructions

### Key Standards

- **Build System**: CMake 3.16+ with vcpkg for dependencies
- **C++ Standard**: C++17 with strict compiler warnings
- **Code Style**: Google-based style with 4-space indentation
- **Testing**: Comprehensive unit and integration tests
- **CI/CD**: Automated testing across platforms

### Quick Development Setup

```bash
# 🚀 One-command setup (recommended)
./scripts/setup-dev-environment.sh

# 📖 Complete documentation  
# See docs/development-environment.pdf & docs/coding-standards.pdf

# ⚡ Manual verification
cmake --preset release
cmake --build build/release
ctest --test-dir build/release
```

## Development Workflow

### Git Workflow

**Work in separate branch pulled from main linked to a ticket, spaces in ticket name separated by hyphen.**
- Format: `UG33-ticketnumber-ticketname`
- Example: `UG33-19-initiate-group-kick-off-meeting`

**When trying to push to main, create a pull request.**
- Resolve merge conflicts by rebasing to main

**Commit according to conventional commits:** https://www.conventionalcommits.org/en/v1.0.0/

Common types:
- `feat`: new feature
- `fix`: bug fix
- `docs`: documentation only
- `refactor`: code change, no new feature or fix
- `chore`: maintenance tasks (build, dependencies)

Examples:
- `feat: add Google login support`
- `chore: update React to v18`

**Pull request must be approved by someone other than branch-owner before push to main.**

### Code Quality Standards

Before submitting a PR:

```bash
# 🎨 Format code (required)
cmake --build build --target format

# 🔍 Run static analysis
cmake --build build --target lint

# ✅ Run all tests
cmake --build build --target test-all

# 🪟 Verify cross-platform build
cmake --preset windows-mingw
cmake --build build/windows-mingw

# 📚 Full guides: docs/development-environment.tex & docs/coding-standards.tex
```

### Available Make Targets

Run `make help` to see all available commands:

#### Build Targets
- `all` - Build the main executable (default)
- `test` - Build both test executables
- `windows` - Cross-compile for Windows
- `windows-zip` - Create Windows executable zip file
- `windows-package` - Complete Windows build and packaging

#### Test Targets
- `test-all` - Run all tests (unit + integration)
- `test-compression-unit` - Run compression algorithm unit tests  
- `test-integration` - Run integration tests (compress + validate)
- `run-validate-test` - Run validation test (interactive)
- `run-compression-test` - Run compression unit tests
- `validate-case1` - Validate main program output with case1.txt
- `validate-case2` - Validate main program output with case2.txt

#### Run Targets
- `run-case1` - Run main program with case1.txt data
- `run-case2` - Run main program with case2.txt data

#### Utility Targets
- `clean` - Clean build artifacts
- `clean-all` - Clean everything in build directory
- `install-deps` - Install all required dependencies
- `install-mingw` - Install MinGW-w64 for Windows cross-compilation
- `help` - Show help message
