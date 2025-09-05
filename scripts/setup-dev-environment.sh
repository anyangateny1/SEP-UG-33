#!/bin/bash

# SEP-UG-33 Development Environment Setup Script
# Simple Makefile-based setup for Ubuntu/Debian, macOS, and WSL

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Detect OS
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if command -v apt &> /dev/null; then
            OS="ubuntu"
        elif command -v yum &> /dev/null; then
            OS="rhel"
        else
            OS="linux"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macos"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        OS="windows"
    else
        OS="unknown"
    fi
}

# Install dependencies based on OS
install_dependencies() {
    log_info "Installing dependencies for $OS..."
    
    case $OS in
        "ubuntu")
            log_info "Using Makefile for dependency installation..."
            make install-deps
            ;;
        "macos")
            if ! command -v brew &> /dev/null; then
                log_error "Homebrew not found. Please install Homebrew first."
                log_info "Visit: https://brew.sh/"
                exit 1
            fi
            
            brew install clang-format git curl
            log_info "Basic tools installed. Cross-compilation not supported on macOS."
            ;;
        "rhel")
            sudo yum groupinstall -y "Development Tools"
            sudo yum install -y git curl
            log_warn "Manual MinGW installation may be needed for Windows cross-compilation"
            ;;
        *)
            log_warn "Unsupported OS: $OS. Please install dependencies manually."
            log_info "Required: build-essential, mingw-w64 (for cross-compilation)"
            ;;
    esac
}

# Setup IDE integration
setup_ide_integration() {
    log_info "Setting up IDE integration..."
    
    # Install bear for compile_commands.json generation (Ubuntu only)
    if [[ "$OS" == "ubuntu" ]]; then
        if ! command -v bear &> /dev/null; then
            log_info "Installing bear for IDE support..."
            sudo apt install -y bear
        fi
    fi
    
    # Generate compile_commands.json
    if command -v bear &> /dev/null; then
        log_info "Generating compile_commands.json..."
        make compile-commands
    else
        log_warn "bear not available. IDE IntelliSense may be limited."
        log_info "On Ubuntu: sudo apt install bear"
    fi
}

# Test build
test_build() {
    log_info "Testing Makefile build..."
    
    # Clean previous builds
    make clean
    
    # Configure
    cmake -B build/test-setup -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
    
    # Build
    cmake --build build/test-setup
    
    # Test
    if [[ -f "build/test-setup/block_model" ]]; then
        log_info "Build successful! Testing with sample data..."
        if [[ -f "tests/data/case1.txt" ]]; then
            build/test-setup/block_model < tests/data/case1.txt > /dev/null
            log_info "Basic execution test passed!"
        else
            log_warn "Test data not found, skipping execution test"
        fi
    else
        log_error "Build or tests failed"
        exit 1
    fi
}

# Setup IDE configuration
setup_ide() {
    log_info "Setting up IDE configurations..."
    
    # Create .vscode directory if it doesn't exist
    mkdir -p .vscode
    
    # VSCode settings for Makefile project
    cat > .vscode/settings.json << 'EOF'
{
    "clangd.arguments": [
        "--compile-commands-dir=${workspaceFolder}",
        "--header-insertion=never"
    ],
    "files.associations": {
        "*.h": "cpp",
        "*.hpp": "cpp"
    },
    "editor.formatOnSave": true,
    "C_Cpp.formatting": "clangFormat",
    "C_Cpp.clang_format_style": "file"
}
EOF

    # VSCode extensions recommendations
    cat > .vscode/extensions.json << 'EOF'
{
    "recommendations": [
        "ms-vscode.cpptools-extension-pack",
        "llvm-vs-code-extensions.vscode-clangd"
    ]
}
EOF

    log_info "IDE configuration complete"
}

# Generate compile commands (handled by setup_ide_integration now)
generate_compile_commands() {
    setup_ide_integration
}

# Main setup function
main() {
    log_info "Starting SEP-UG-33 development environment setup..."
    
    # Parse command line arguments
    SKIP_DEPS=false
    SKIP_IDE=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --skip-deps)
                SKIP_DEPS=true
                shift
                ;;
            --skip-ide)
                SKIP_IDE=true
                shift
                ;;
            -h|--help)
                echo "Usage: $0 [OPTIONS]"
                echo "Options:"
                echo "  --skip-deps      Skip system dependency installation"
                echo "  --skip-ide       Skip IDE configuration setup"
                echo "  -h, --help       Show this help message"
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    # Detect OS
    detect_os
    log_info "Detected OS: $OS"
    
    # Install dependencies
    if [[ "$SKIP_DEPS" == false ]]; then
        install_dependencies
    else
        log_info "Skipping dependency installation"
    fi
    
    # Setup IDE
    if [[ "$SKIP_IDE" == false ]]; then
        setup_ide
        generate_compile_commands
    else
        log_info "Skipping IDE setup"
    fi
    
    # Test build
    test_build
    
    log_info "Development environment setup complete!"
    log_info ""
    log_info "Next steps:"
    log_info "1. Open project in your IDE"
    log_info "2. Start developing!"
    log_info ""
    log_info "Quick commands:"
    log_info "  make test-all              # Build and test everything"
    log_info "  make run-case1             # Test with sample data"
    log_info "  make windows-package       # Create Windows submission"
    log_info "  make compile-commands      # Regenerate IDE support"
    log_info "  make help                  # Show all available targets"
}

# Run main function
main "$@"
