#!/bin/bash

# SEP-UG-33 Development Environment Setup Script
# Supports Ubuntu/Debian, macOS, and basic Windows (WSL)

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
            sudo apt update
            sudo apt install -y \
                build-essential \
                cmake \
                ninja-build \
                clang \
                clang-format \
                clang-tidy \
                mingw-w64 \
                git \
                curl \
                zip \
                unzip \
                tar
            ;;
        "macos")
            if ! command -v brew &> /dev/null; then
                log_error "Homebrew not found. Please install Homebrew first."
                log_info "Visit: https://brew.sh/"
                exit 1
            fi
            
            brew install cmake ninja clang-format git curl
            ;;
        "rhel")
            sudo yum groupinstall -y "Development Tools"
            sudo yum install -y cmake ninja-build clang git curl
            ;;
        *)
            log_warn "Unsupported OS: $OS. Please install dependencies manually."
            log_info "Required: cmake, ninja-build, clang, clang-format, git"
            ;;
    esac
}

# Setup vcpkg
setup_vcpkg() {
    local vcpkg_dir="$1"
    
    if [[ -z "$vcpkg_dir" ]]; then
        vcpkg_dir="../vcpkg"
    fi
    
    log_info "Setting up vcpkg at $vcpkg_dir..."
    
    if [[ ! -d "$vcpkg_dir" ]]; then
        log_info "Cloning vcpkg..."
        git clone https://github.com/Microsoft/vcpkg.git "$vcpkg_dir"
    else
        log_info "vcpkg directory already exists. Updating..."
        cd "$vcpkg_dir"
        git pull
        cd -
    fi
    
    # Bootstrap vcpkg
    cd "$vcpkg_dir"
    if [[ "$OS" == "windows" ]]; then
        ./bootstrap-vcpkg.bat
    else
        ./bootstrap-vcpkg.sh
    fi
    cd -
    
    # Set environment variable
    export VCPKG_ROOT="$(realpath "$vcpkg_dir")"
    
    # Add to shell profiles
    local shell_profiles=("$HOME/.bashrc" "$HOME/.zshrc" "$HOME/.profile")
    for profile in "${shell_profiles[@]}"; do
        if [[ -f "$profile" ]]; then
            if ! grep -q "VCPKG_ROOT" "$profile"; then
                echo "export VCPKG_ROOT=\"$VCPKG_ROOT\"" >> "$profile"
                log_info "Added VCPKG_ROOT to $profile"
            fi
        fi
    done
    
    log_info "vcpkg setup complete. VCPKG_ROOT=$VCPKG_ROOT"
}

# Test build
test_build() {
    log_info "Testing CMake build..."
    
    # Clean previous builds
    rm -rf build/test-setup
    
    # Configure
    cmake -B build/test-setup -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
    
    # Build
    cmake --build build/test-setup
    
    # Test
    if [[ -f "build/test-setup/block_model" ]]; then
        log_info "Build successful! Testing with sample data..."
        echo "2,2,2,1,1,1" | build/test-setup/block_model > /dev/null
        log_info "Basic execution test passed!"
    else
        log_error "Build failed - executable not found"
        exit 1
    fi
}

# Setup IDE configuration
setup_ide() {
    log_info "Setting up IDE configurations..."
    
    # Create .vscode directory if it doesn't exist
    mkdir -p .vscode
    
    # VSCode settings
    cat > .vscode/settings.json << 'EOF'
{
    "cmake.configureOnOpen": true,
    "cmake.preferredGenerators": ["Ninja"],
    "cmake.buildDirectory": "${workspaceFolder}/build/${buildType}",
    "clangd.arguments": [
        "--compile-commands-dir=${workspaceFolder}/build",
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
        "ms-vscode.cmake-tools",
        "llvm-vs-code-extensions.vscode-clangd"
    ]
}
EOF

    log_info "IDE configuration complete"
}

# Generate compile commands
generate_compile_commands() {
    log_info "Generating compile_commands.json..."
    
    cmake -B build/compile-commands -S . -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    
    if [[ -f "build/compile-commands/compile_commands.json" ]]; then
        cp build/compile-commands/compile_commands.json .
        log_info "compile_commands.json generated successfully"
    fi
}

# Main setup function
main() {
    log_info "Starting SEP-UG-33 development environment setup..."
    
    # Parse command line arguments
    SETUP_VCPKG=true
    VCPKG_DIR="../vcpkg"
    SKIP_DEPS=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --skip-vcpkg)
                SETUP_VCPKG=false
                shift
                ;;
            --vcpkg-dir)
                VCPKG_DIR="$2"
                shift 2
                ;;
            --skip-deps)
                SKIP_DEPS=true
                shift
                ;;
            -h|--help)
                echo "Usage: $0 [OPTIONS]"
                echo "Options:"
                echo "  --skip-vcpkg     Skip vcpkg setup"
                echo "  --vcpkg-dir DIR  vcpkg installation directory (default: ../vcpkg)"
                echo "  --skip-deps      Skip system dependency installation"
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
    
    # Setup vcpkg
    if [[ "$SETUP_VCPKG" == true ]]; then
        setup_vcpkg "$VCPKG_DIR"
    else
        log_info "Skipping vcpkg setup"
    fi
    
    # Setup IDE
    setup_ide
    
    # Generate compile commands
    generate_compile_commands
    
    # Test build
    test_build
    
    log_info "Development environment setup complete!"
    log_info ""
    log_info "Next steps:"
    log_info "1. Restart your shell or run: source ~/.bashrc"
    log_info "2. Open project in your IDE"
    log_info "3. Start developing!"
    log_info ""
    log_info "Quick commands:"
    log_info "  cmake --preset release    # Configure release build"
    log_info "  cmake --build build/release  # Build project"
    log_info "  ctest --test-dir build/release  # Run tests"
}

# Run main function
main "$@"
