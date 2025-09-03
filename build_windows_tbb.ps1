# PowerShell script for Windows TBB build
# This script can be run in a Windows Docker container or native Windows

param(
    [string]$BuildType = "Release",
    [switch]$Clean = $false
)

Write-Host "=== Block Model Windows TBB Build ===" -ForegroundColor Green

# Check if we're in the right directory
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Error "CMakeLists.txt not found. Run this script from the project root."
    exit 1
}

# Clean build directory if requested
if ($Clean -and (Test-Path "build_windows")) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "build_windows"
}

# Create build directory
if (-not (Test-Path "build_windows")) {
    New-Item -ItemType Directory -Path "build_windows"
}

Set-Location "build_windows"

try {
    # Configure with vcpkg toolchain
    Write-Host "Configuring CMake..." -ForegroundColor Yellow
    
    $vcpkgToolchain = ""
    if ($env:VCPKG_ROOT) {
        $vcpkgToolchain = "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
        Write-Host "Using vcpkg from: $env:VCPKG_ROOT" -ForegroundColor Cyan
    }
    
    $cmakeArgs = @(
        "..",
        "-DCMAKE_BUILD_TYPE=$BuildType",
        "-DVCPKG_TARGET_TRIPLET=x64-windows-static"
    )
    
    if ($vcpkgToolchain) {
        $cmakeArgs += $vcpkgToolchain
    }
    
    & cmake @cmakeArgs
    
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }
    
    # Build the project
    Write-Host "Building project..." -ForegroundColor Yellow
    & cmake --build . --config $BuildType --parallel
    
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed"
    }
    
    # Find the executable
    $exePath = ""
    if (Test-Path "$BuildType/block_model.exe") {
        $exePath = "$BuildType/block_model.exe"
    } elseif (Test-Path "block_model.exe") {
        $exePath = "block_model.exe"
    }
    
    if (-not $exePath) {
        throw "Executable not found after build"
    }
    
    # Create package
    Write-Host "Creating package..." -ForegroundColor Yellow
    $packageName = "block_model_tbb_$(Get-Date -Format 'yyyyMMdd_HHmmss').exe.zip"
    
    Compress-Archive -Path $exePath -DestinationPath $packageName -Force
    
    $fileSize = (Get-Item $exePath).Length
    $packageSize = (Get-Item $packageName).Length
    
    Write-Host "=== Build Successful ===" -ForegroundColor Green
    Write-Host "Executable: $exePath ($([math]::Round($fileSize/1KB)) KB)" -ForegroundColor Cyan
    Write-Host "Package: $packageName ($([math]::Round($packageSize/1KB)) KB)" -ForegroundColor Cyan
    
    # Test executable (basic check)
    Write-Host "Testing executable..." -ForegroundColor Yellow
    & "./$exePath" --help 2>$null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Executable runs successfully" -ForegroundColor Green
    } else {
        Write-Host "⚠ Executable test failed (this may be normal if no --help option)" -ForegroundColor Yellow
    }
    
} catch {
    Write-Error "Build failed: $_"
    exit 1
} finally {
    Set-Location ".."
}

Write-Host "Build completed. Package ready for submission." -ForegroundColor Green
