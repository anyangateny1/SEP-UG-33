#!/bin/bash
#
# Script to format all C++ code in the project using clang-format
#

set -e

echo "Formatting all C++ code in the project..."

# Check if clang-format is available
if ! command -v clang-format >/dev/null 2>&1; then
    echo "Error: clang-format not found. Please install clang-format to format C++ code."
    echo "You can install it with:"
    echo "  - macOS: brew install clang-format"
    echo "  - Ubuntu: sudo apt install clang-format"
    echo "  - Windows: choco install llvm"
    exit 1
fi

# Find all C++ files
CPP_FILES=$(find src include tests -name "*.cpp" -o -name "*.h" 2>/dev/null || true)

if [ -z "$CPP_FILES" ]; then
    echo "No C++ files found in src/, include/, or tests/ directories."
    exit 0
fi

echo "Found $(echo "$CPP_FILES" | wc -l | tr -d ' ') C++ files to format."

# Format each file
for file in $CPP_FILES; do
    if [ -f "$file" ]; then
        echo "Formatting $file..."
        clang-format -i "$file"
    fi
done

echo "✓ Code formatting complete!"
echo ""
echo "To check formatting without making changes, run:"
echo "  find src include tests -name \"*.cpp\" -o -name \"*.h\" | xargs clang-format --dry-run --Werror"
