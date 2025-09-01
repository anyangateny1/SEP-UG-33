Package: hwloc:x64-mingw-dynamic@2.11.2

**Host Environment**

- Host: x64-linux
- Compiler: GNU 13.0.0
-    vcpkg-tool version: 2025-04-16-f9b6c6917b23c1ccf16c1a9f015ebabf8f615045
    vcpkg-scripts version: 96d5fb3de1 2025-04-26 (4 months ago)

**To Reproduce**

`vcpkg install `

**Failure logs**

```
Downloading https://github.com/open-mpi/hwloc/archive/hwloc-2.11.2.tar.gz -> open-mpi-hwloc-hwloc-2.11.2.tar.gz
Successfully downloaded open-mpi-hwloc-hwloc-2.11.2.tar.gz
-- Extracting source /home/anyangateny/packages/vcpkg/downloads/open-mpi-hwloc-hwloc-2.11.2.tar.gz
-- Applying patch fix_shared_win_build.patch
-- Applying patch stdout_fileno.patch
-- Using source at /home/anyangateny/packages/vcpkg/buildtrees/hwloc/src/loc-2.11.2-a8fd08dd1a.clean
-- Getting CMake variables for x64-mingw-dynamic-dbg
-- Getting CMake variables for x64-mingw-dynamic-rel
-- Generating configure for x64-mingw-dynamic
-- Finished generating configure for x64-mingw-dynamic
-- Configuring x64-mingw-dynamic-dbg
-- Configuring x64-mingw-dynamic-rel
-- Building x64-mingw-dynamic-dbg
-- Installing x64-mingw-dynamic-dbg
-- Building x64-mingw-dynamic-rel
-- Installing x64-mingw-dynamic-rel
-- Fixing pkgconfig file: /home/anyangateny/packages/vcpkg/packages/hwloc_x64-mingw-dynamic/lib/pkgconfig/hwloc.pc
-- Fixing pkgconfig file: /home/anyangateny/packages/vcpkg/packages/hwloc_x64-mingw-dynamic/debug/lib/pkgconfig/hwloc.pc
CMake Error at scripts/cmake/vcpkg_copy_tool_dependencies.cmake:31 (message):
  Could not find PowerShell Core; please open an issue to report this.
Call Stack (most recent call first):
  buildtrees/versioning_/versions/hwloc/4b14fa65f32abe3d816a08d5d37aaf183372338c/portfile.cmake:42 (vcpkg_copy_tool_dependencies)
  scripts/ports.cmake:206 (include)



```

**Additional context**

<details><summary>vcpkg.json</summary>

```
{
  "name": "sep-ug-33-block-model",
  "version": "1.0.0",
  "description": "3D Block Model Compression Algorithm",
  "homepage": "https://github.com/anyangateny1/Code-Catalyst-Block-SEP",
  "documentation": "hhttps://github.com/anyangateny1/Code-Catalyst-Block-SEP/blob/main/README.md",
  "license": "MIT",
  "dependencies": [
    "tbb",
    "xsimd"
  ],
  "features": {
    "testing": {
      "description": "Build with testing framework support",
      "dependencies": []
    },
    "benchmarking": {
      "description": "Build with benchmarking support",
      "dependencies": []
    }
  },
  "builtin-baseline": "96d5fb3de135b86d7222c53f2352ca92827a156b"
}

```
</details>
