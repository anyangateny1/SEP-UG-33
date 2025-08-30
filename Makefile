# Makefile for SEP-UG-33 Block Model Project

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude
WINDOWS_CXX = x86_64-w64-mingw32-g++
WINDOWS_FLAGS = -std=c++17 -O2 -static -static-libstdc++ -static-libgcc -Iinclude

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
TEST_DIR = tests
DATA_DIR = tests/data

# Source files
SOURCES = $(SRC_DIR)/main.cpp $(SRC_DIR)/block.cpp $(SRC_DIR)/block_growth.cpp $(SRC_DIR)/block_model.cpp
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
# Library objects (without main.cpp)
LIB_OBJECTS = $(BUILD_DIR)/block.o $(BUILD_DIR)/block_growth.o $(BUILD_DIR)/block_model.o
TARGET = $(BUILD_DIR)/block_model
WINDOWS_TARGET = $(BUILD_DIR)/block_model.exe

# Test files
VALIDATE_TEST_SOURCES = $(TEST_DIR)/validate_test.cpp
VALIDATE_TEST_TARGET = $(BUILD_DIR)/validate_test
COMPRESSION_TEST_SOURCES = $(TEST_DIR)/compression_test.cpp
COMPRESSION_TEST_TARGET = $(BUILD_DIR)/compression_test

# Default target
all: $(TARGET)

# Main executable
$(TARGET): $(OBJECTS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Windows cross-compilation
windows: $(WINDOWS_TARGET)

$(WINDOWS_TARGET): $(SOURCES) | $(BUILD_DIR)
	$(WINDOWS_CXX) $(WINDOWS_FLAGS) -o $@ $^

# Windows zip target (as per original requirements)
windows-zip: $(WINDOWS_TARGET)
	cd $(BUILD_DIR) && zip -j block_model.exe.zip block_model.exe

# Windows package - complete build and packaging process
windows-package: install-mingw windows windows-zip
	@echo "Windows executable packaged successfully!"
	@echo "File: $(BUILD_DIR)/block_model.exe.zip"
	@ls -la $(BUILD_DIR)/block_model.exe.zip

# Test executables
test: $(VALIDATE_TEST_TARGET) $(COMPRESSION_TEST_TARGET)

# Validation test (reconstructs 3D model from compressed blocks)
$(VALIDATE_TEST_TARGET): $(VALIDATE_TEST_SOURCES) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compression test (tests the compression algorithm directly)
$(COMPRESSION_TEST_TARGET): $(COMPRESSION_TEST_SOURCES) $(LIB_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Run the main program with test data
run-case1: $(TARGET)
	./$(TARGET) < $(DATA_DIR)/case1.txt

run-case2: $(TARGET)
	./$(TARGET) < $(DATA_DIR)/case2.txt

# Run validation test
run-validate-test: $(VALIDATE_TEST_TARGET)
	./$(VALIDATE_TEST_TARGET)

# Run compression test
run-compression-test: $(COMPRESSION_TEST_TARGET)
	./$(COMPRESSION_TEST_TARGET)

# Run validate_test with case data (validate the main program output)
validate-case1: $(TARGET) $(VALIDATE_TEST_TARGET)
	./$(TARGET) < $(DATA_DIR)/case1.txt | ./$(VALIDATE_TEST_TARGET)

validate-case2: $(TARGET) $(VALIDATE_TEST_TARGET)
	./$(TARGET) < $(DATA_DIR)/case2.txt | ./$(VALIDATE_TEST_TARGET)

# Integration tests - compress and validate
test-integration: $(TARGET) $(VALIDATE_TEST_TARGET)
	@echo "Running integration tests (compression + validation)..."
	@echo "Testing case1.txt..."
	@./$(TARGET) < $(DATA_DIR)/case1.txt | ./$(VALIDATE_TEST_TARGET) > /dev/null && echo "✓ Case 1 integration passed" || echo "✗ Case 1 integration failed"
	@echo "Testing case2.txt..."
	@./$(TARGET) < $(DATA_DIR)/case2.txt | ./$(VALIDATE_TEST_TARGET) > /dev/null && echo "✓ Case 2 integration passed" || echo "✗ Case 2 integration failed"
	@echo "All integration tests completed!"

# Unit tests for compression algorithm
test-compression-unit: $(COMPRESSION_TEST_TARGET)
	@echo "Running compression unit tests..."
	@./$(COMPRESSION_TEST_TARGET)

# Run all tests
test-all: test test-compression-unit test-integration
	@echo "All tests completed!"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)/*

# Clean everything except source code and directories
clean-all:
	rm -rf $(BUILD_DIR)/*

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt update
	sudo apt install build-essential mingw-w64

# Install MinGW for Windows cross-compilation
install-mingw:
	@echo "Checking for MinGW-w64..."
	@which x86_64-w64-mingw32-g++ > /dev/null 2>&1 || \
		(echo "Installing MinGW-w64..." && \
		 sudo apt update && \
		 sudo apt install -y mingw-w64)
	@echo "MinGW-w64 is ready!"

# Help target
help:
	@echo "Available targets:"
	@echo "  all            - Build the main executable (default)"
	@echo "  windows        - Cross-compile for Windows"
	@echo "  windows-zip    - Create Windows executable zip file"
	@echo "  windows-package- Complete Windows build and packaging (installs MinGW if needed)"
	@echo "  test               - Build both test executables"
	@echo "  test-all           - Run all tests (unit + integration)"
	@echo "  test-compression-unit - Run compression algorithm unit tests"
	@echo "  test-integration   - Run integration tests (compress + validate)"
	@echo "  run-case1          - Run main program with case1.txt data"
	@echo "  run-case2          - Run main program with case2.txt data"
	@echo "  run-validate-test  - Run validation test (interactive)"
	@echo "  run-compression-test - Run compression unit tests"
	@echo "  validate-case1     - Validate main program output with case1.txt"
	@echo "  validate-case2     - Validate main program output with case2.txt"
	@echo "  clean          - Clean build artifacts"
	@echo "  clean-all      - Clean everything in build directory"
	@echo "  install-deps   - Install all required dependencies"
	@echo "  install-mingw  - Install MinGW-w64 for Windows cross-compilation"
	@echo "  help           - Show this help message"
	@echo ""
	@echo "Windows Packaging Workflow:"
	@echo "  1. make windows-package  # Installs MinGW, builds, and packages"
	@echo "  2. Submit build/block_model.exe.zip"

# Phony targets
.PHONY: all windows windows-zip windows-package test test-all test-compression-unit test-integration clean clean-all install-deps install-mingw run-case1 run-case2 run-validate-test run-compression-test validate-case1 validate-case2 help

# Dependencies (header files)
$(BUILD_DIR)/main.o: $(INCLUDE_DIR)/block_model.h
$(BUILD_DIR)/block.o: $(INCLUDE_DIR)/block.h
$(BUILD_DIR)/block_growth.o: $(INCLUDE_DIR)/block_growth.h $(INCLUDE_DIR)/block.h
$(BUILD_DIR)/block_model.o: $(INCLUDE_DIR)/block_model.h $(INCLUDE_DIR)/block.h $(INCLUDE_DIR)/block_growth.h
