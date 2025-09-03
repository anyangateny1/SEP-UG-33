# Local Windows TBB Build Script
# Enhanced version for local development with multiple fallback options

param(
    [string]$BuildType = "Release",
    [switch]$Clean = $false,
    [switch]$UseConan = $false,
    [string]$TbbPath = ""
)

Write-Host "=== Block Model Local Windows TBB Build ===" -ForegroundColor Green

# Check if we're in the right directory
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Error "CMakeLists.txt not found. Run this script from the project root."
    exit 1
}

# Clean build directory if requested
if ($Clean -and (Test-Path "build_local")) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "build_local"
}

# Create build directory
if (-not (Test-Path "build_local")) {
    New-Item -ItemType Directory -Path "build_local"
}

Set-Location "build_local"

try {
    Write-Host "Configuring CMake..." -ForegroundColor Yellow
    
    $cmakeArgs = @(
        "..",
        "-DCMAKE_BUILD_TYPE=$BuildType",
        "-DUSE_TBB=ON"
    )
    
    # Method 1: Try vcpkg first
    if (-not $UseConan -and -not $TbbPath) {
        Write-Host "Attempting vcpkg build..." -ForegroundColor Cyan
        
        # Look for vcpkg in common locations
        $vcpkgPaths = @(
            "C:\vcpkg\scripts\buildsystems\vcpkg.cmake",
            "$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake",
            "..\vcpkg_installed\vcpkg\scripts\buildsystems\vcpkg.cmake"
        )
        
        $vcpkgToolchain = $null
        foreach ($path in $vcpkgPaths) {
            if (Test-Path $path) {
                $vcpkgToolchain = $path
                Write-Host "Found vcpkg at: $path" -ForegroundColor Green
                break
            }
        }
        
        if ($vcpkgToolchain) {
            $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain"
            $cmakeArgs += "-DVCPKG_TARGET_TRIPLET=x64-windows-static"
        } else {
            Write-Host "vcpkg not found, trying other methods..." -ForegroundColor Yellow
        }
    }
    
    # Method 2: Use Conan if requested
    if ($UseConan) {
        Write-Host "Using Conan build..." -ForegroundColor Cyan
        
        # Check if conan is installed
        try {
            $conanVersion = & conan --version 2>$null
            Write-Host "Found Conan: $conanVersion" -ForegroundColor Green
            
            # Install dependencies
            Set-Location ".."
            & conan install . --build=missing -s build_type=$BuildType
            Set-Location "build_local"
            
            # Use conan-generated toolchain
            if (Test-Path "../conan_toolchain.cmake") {
                $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=../conan_toolchain.cmake"
            }
            
        } catch {
            Write-Error "Conan not found. Install with: pip install conan"
            exit 1
        }
    }
    
    # Method 3: Use manual TBB path
    if ($TbbPath) {
        Write-Host "Using manual TBB path: $TbbPath" -ForegroundColor Cyan
        if (-not (Test-Path $TbbPath)) {
            Write-Error "TBB path not found: $TbbPath"
            exit 1
        }
        $cmakeArgs += "-DTBB_ROOT=$TbbPath"
        $cmakeArgs += "-DCMAKE_PREFIX_PATH=$TbbPath"
    }
    
    # Configure
    Write-Host "Running cmake configure..." -ForegroundColor Yellow
    Write-Host "Arguments: $($cmakeArgs -join ' ')" -ForegroundColor Gray
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
    $searchPaths = @(
        "$BuildType/block_model.exe",
        "block_model.exe",
        "Release/block_model.exe",
        "Debug/block_model.exe"
    )
    
    foreach ($path in $searchPaths) {
        if (Test-Path $path) {
            $exePath = $path
            break
        }
    }
    
    if (-not $exePath) {
        throw "Executable not found after build"
    }
    
    # Create package
    Write-Host "Creating package..." -ForegroundColor Yellow
    $timestamp = Get-Date -Format 'yyyyMMdd_HHmmss'
    $packageName = "block_model_tbb_local_$timestamp.exe.zip"
    
    Compress-Archive -Path $exePath -DestinationPath $packageName -Force
    
    $fileSize = (Get-Item $exePath).Length
    $packageSize = (Get-Item $packageName).Length
    
    Write-Host "=== Build Successful ===" -ForegroundColor Green
    Write-Host "Executable: $exePath ($([math]::Round($fileSize/1KB)) KB)" -ForegroundColor Cyan
    Write-Host "Package: $packageName ($([math]::Round($packageSize/1KB)) KB)" -ForegroundColor Cyan
    
    # Basic functionality test
    Write-Host "Testing executable..." -ForegroundColor Yellow
    
    # Create a simple test input
    $testInput = @"
4,4,1,2,2,1
S, sea
SSSS
"@
    
    try {
        $testOutput = $testInput | & "./$exePath" 2>$null
        if ($testOutput) {
            Write-Host "✓ Executable runs and produces output" -ForegroundColor Green
            Write-Host "Sample output: $($testOutput.Split("`n")[0])" -ForegroundColor Gray
        } else {
            Write-Host "⚠ Executable runs but no output detected" -ForegroundColor Yellow
        }
    } catch {
        Write-Host "⚠ Executable test failed: $($_.Exception.Message)" -ForegroundColor Yellow
    }
    
    # Copy package to parent directory for easy access
    Copy-Item $packageName ".." -Force
    Write-Host "Package copied to: ..\$packageName" -ForegroundColor Cyan
    
} catch {
    Write-Error "Build failed: $_"
    exit 1
} finally {
    Set-Location ".."
}

Write-Host ""
Write-Host "=== Build Complete ===" -ForegroundColor Green
Write-Host "Your submission-ready package: $packageName" -ForegroundColor Cyan
Write-Host "Expected performance: ~0.2-0.3s (vs ~0.7s single-threaded)" -ForegroundColor Green
Write-Host ""
Write-Host "Usage examples:" -ForegroundColor Yellow
Write-Host "  Expand-Archive $packageName -DestinationPath test" -ForegroundColor Gray
Write-Host "  cd test" -ForegroundColor Gray
Write-Host "  echo '4,4,1,2,2,1`nS, sea`nSSSS' | .\block_model.exe" -ForegroundColor Gray
