# SEP-UG-33 Block Model Project

A C++ implementation of a 3D block model compression algorithm that reads model specifications and performs block fitting and growth operations.

## Project Overview

This project implements a block-based 3D model compression system:

1. **Input**: 3D model data with specifications, tag tables, and voxel grids
2. **Processing**: Block fitting and growth algorithm for optimal compression
3. **Output**: Compressed block format (`x,y,z,width,height,depth,label`)
4. **Validation**: Reconstruction verification to ensure data integrity

The system is designed with separation of concerns:
- **Core Algorithm** (`src/`): Block model processing and compression
- **Testing** (`tests/`): Comprehensive unit and integration tests
- **Build System** (`Makefile`): Cross-platform compilation and automation

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

## Building the Project

### Prerequisites

- g++ compiler with C++17 support
- make utility
- For Windows cross-compilation: mingw-w64

Install dependencies on Ubuntu/Debian:
```bash
make install-deps
```

### Quick Start

```bash
# Clone and enter the project
cd SEP-UG-33

# Build everything and run tests
make test-all

# Run with sample data
make run-case1

# Create Windows package for submission
make windows-package
```

### Build Commands

```bash
# Build the main executable
make

# Cross-compile for Windows
make windows

# Create Windows executable zip (as per original requirements)
make windows-zip

# Complete Windows packaging (installs MinGW if needed, builds, and packages)
make windows-package

# Build test executable
make test

# Clean build artifacts
make clean
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
