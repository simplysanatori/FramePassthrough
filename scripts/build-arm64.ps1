param(
    [Parameter(Mandatory = $true)][string]$ZigPath,
    [string]$BuildDirectory = 'build-arm64'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$compiler = (Resolve-Path -LiteralPath $ZigPath).Path
$build = [IO.Path]::GetFullPath((Join-Path $projectRoot $BuildDirectory))
New-Item -ItemType Directory -Force -Path (Join-Path $build 'bin') | Out-Null
$oldGlobalCache = $env:ZIG_GLOBAL_CACHE_DIR
$oldLocalCache = $env:ZIG_LOCAL_CACHE_DIR
try {
    $env:ZIG_GLOBAL_CACHE_DIR = Join-Path $build 'zig-global-cache'
    $env:ZIG_LOCAL_CACHE_DIR = Join-Path $build 'zig-local-cache'
    $sources = @(
        'src/main.cpp', 'src/config.cpp', 'src/controller.cpp', 'src/openvr_camera_source.cpp',
        'third_party/openvr/src/openvr_api_public.cpp',
        'third_party/openvr/src/jsoncpp.cpp',
        'third_party/openvr/src/vrcore/dirtools_public.cpp',
        'third_party/openvr/src/vrcore/envvartools_public.cpp',
        'third_party/openvr/src/vrcore/pathtools_public.cpp',
        'third_party/openvr/src/vrcore/sharedlibtools_public.cpp',
        'third_party/openvr/src/vrcore/hmderrors_public.cpp',
        'third_party/openvr/src/vrcore/vrpathregistry_public.cpp',
        'third_party/openvr/src/vrcore/strtools_public.cpp'
    ) | ForEach-Object { Join-Path $projectRoot $_ }
    $includes = @('src', 'third_party/json', 'third_party/openvr/headers', 'third_party/openvr/src', 'third_party/openvr/src/vrcore') |
        ForEach-Object { '-I' + (Join-Path $projectRoot $_) }
    # Tested with Zig 0.13.0. glibc stays dynamic for SteamVR's dlopen loader;
    # Zig links libc++ and its C++ ABI implementation statically.
    & $compiler c++ -target aarch64-linux-gnu.2.31 -std=c++17 -O2 -fvisibility=hidden -Wno-deprecated-declarations `
        -DLINUX -DPOSIX -DLINUXARM64 -DVR_API_PUBLIC -DVRCORE_NO_PLATFORM -DOPENVR_BUILD_STATIC `
        @includes @sources -ldl -lpthread -o (Join-Path $build 'bin/frame-passthrough-shortcuts')
    if ($LASTEXITCODE -ne 0) { throw "ARM64 compilation failed with exit code $LASTEXITCODE" }
    Copy-Item -LiteralPath (Join-Path $build 'bin/frame-passthrough-shortcuts') `
        -Destination (Join-Path $build 'bin/frame-passthrough-control') -Force
    Copy-Item -Path (Join-Path $projectRoot 'assets/*') -Destination (Join-Path $build 'bin') -Recurse -Force
    Write-Output "ARM64 build ready: $build/bin/frame-passthrough-shortcuts"
} finally {
    $env:ZIG_GLOBAL_CACHE_DIR = $oldGlobalCache
    $env:ZIG_LOCAL_CACHE_DIR = $oldLocalCache
}
