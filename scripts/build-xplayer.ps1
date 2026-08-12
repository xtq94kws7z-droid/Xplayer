param(
    [string]$BuildDir,
    [string]$CMakePath,
    [string]$QtRoot,
    [string]$MinGWRoot,
    [string]$NinjaRoot,
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$Configuration = 'Release',
    [int]$Jobs = 4,
    [switch]$CleanConfigure,
    [switch]$ValidateOnly
)

$ErrorActionPreference = 'Stop'

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $PSScriptRoot 'xplayer-build-environment.ps1')
$BuildDir = if ($BuildDir) {
    ConvertTo-XplayerAbsolutePath -Path $BuildDir -BasePath $projectRoot
} else {
    Join-Path $projectRoot 'build-xplayer'
}
$toolchain = Resolve-XplayerToolchain -ProjectRoot $projectRoot -CMakePath $CMakePath -QtRoot $QtRoot -MinGWRoot $MinGWRoot -NinjaRoot $NinjaRoot
$dependencyStatus = Test-XplayerSourceDependencies -ProjectRoot $projectRoot
if (-not $dependencyStatus.IsValid) { throw $dependencyStatus.Reason }
$cmake = $toolchain.CMake
$mingwBin = $toolchain.MinGWBin
$ninja = $toolchain.Ninja
$winPthread = Join-Path $mingwBin 'libwinpthread-1.dll'
$compiler = $toolchain.CXXCompiler
$cCompiler = $toolchain.CCompiler
if (-not (Test-Path $winPthread -PathType Leaf)) { throw "MinGW runtime DLL is missing: $winPthread" }

Set-XplayerToolchainPath $toolchain
Set-Location -LiteralPath $projectRoot

$probeRoot = Join-Path ([IO.Path]::GetTempPath()) (
    'xplayer-build-probe-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $probeRoot -Force | Out-Null

try {
    Set-Content -LiteralPath (Join-Path $probeRoot 'probe.cpp') -Value 'int main(){return 0;}'
    Push-Location $probeRoot
    try {
        & $compiler -fsyntax-only probe.cpp
        if ($LASTEXITCODE -ne 0) {
            throw "MinGW compiler preflight failed with exit code $LASTEXITCODE."
        }
    }
    finally {
        Pop-Location
    }
}
finally {
    $resolvedProbe = [IO.Path]::GetFullPath($probeRoot)
    $resolvedTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    if ($resolvedProbe.StartsWith($resolvedTemp, [StringComparison]::OrdinalIgnoreCase) -and
        (Test-Path -LiteralPath $resolvedProbe)) {
        Remove-Item -LiteralPath $resolvedProbe -Recurse -Force
    }
}

$cacheStatus = Get-XplayerBuildCacheStatus -BuildDir $BuildDir -Toolchain $toolchain -Configuration $Configuration

if ($ValidateOnly) {
    Write-Host 'Xplayer build environment validated:'
    $toolchain | Format-List
    Write-Host "Build directory: $BuildDir"
    Write-Host "Build cache: $($cacheStatus.Reason)"
    exit 0
}

if ($CleanConfigure -or -not $cacheStatus.IsValid) {
    Write-Host "Reconfiguring build directory: $($cacheStatus.Reason)"
    $freshArgument = if (Test-Path -LiteralPath (Join-Path $BuildDir 'CMakeCache.txt')) {
        @('--fresh')
    } else {
        @()
    }
    & $cmake @freshArgument -S $projectRoot -B $BuildDir -G Ninja `
        "-DCMAKE_BUILD_TYPE=$Configuration" `
        "-DCMAKE_PREFIX_PATH=$($toolchain.QtRoot)" `
        "-DCMAKE_MAKE_PROGRAM=$ninja" `
        "-DCMAKE_C_COMPILER=$cCompiler" `
        "-DCMAKE_CXX_COMPILER=$compiler"
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed with exit code $LASTEXITCODE."
    }
}

& $cmake --build $BuildDir --config $Configuration -- -j $Jobs
if ($LASTEXITCODE -ne 0) {
    throw "Xplayer build failed with exit code $LASTEXITCODE."
}

Write-Host "Xplayer $Configuration build completed: $BuildDir"
