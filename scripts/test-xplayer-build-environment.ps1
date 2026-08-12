$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $PSScriptRoot 'xplayer-build-environment.ps1')
$toolchain = Resolve-XplayerToolchain -ProjectRoot $projectRoot
$testRoot = Join-Path ([IO.Path]::GetTempPath()) (
    'xplayer-build-environment-test-' + [Guid]::NewGuid().ToString('N'))

function ConvertTo-CMakePath {
    param([Parameter(Mandatory = $true)][string]$Path)
    return ([IO.Path]::GetFullPath($Path)).Replace('\', '/')
}

New-Item -ItemType Directory -Path $testRoot -Force | Out-Null
try {
    $cachePath = Join-Path $testRoot 'CMakeCache.txt'
    $cacheLines = @(
        "CMAKE_HOME_DIRECTORY:INTERNAL=$(ConvertTo-CMakePath $projectRoot)",
        "CMAKE_C_COMPILER:FILEPATH=$(ConvertTo-CMakePath $toolchain.CCompiler)",
        "CMAKE_CXX_COMPILER:STRING=$(ConvertTo-CMakePath $toolchain.CXXCompiler)",
        "CMAKE_MAKE_PROGRAM:UNINITIALIZED=$(ConvertTo-CMakePath $toolchain.Ninja)",
        "CMAKE_PREFIX_PATH:PATH=C:/unrelated;$(ConvertTo-CMakePath $toolchain.QtRoot)",
        "Qt6_DIR:PATH=$(ConvertTo-CMakePath (Join-Path $toolchain.QtRoot 'lib\cmake\Qt6'))",
        'CMAKE_GENERATOR:INTERNAL=Ninja',
        'CMAKE_BUILD_TYPE:STRING=Release'
    )
    Set-Content -LiteralPath $cachePath -Value $cacheLines
    Set-Content -LiteralPath (Join-Path $testRoot 'build.ninja') -Value '# synthetic cache test'

    $validStatus = Get-XplayerBuildCacheStatus -BuildDir $testRoot -Toolchain $toolchain -Configuration Release
    if (-not $validStatus.IsValid) {
        throw "A matching cache was rejected: $($validStatus.Reason)"
    }

    $cacheLines[2] = 'CMAKE_CXX_COMPILER:FILEPATH=C:/invalid/c++.exe'
    Set-Content -LiteralPath $cachePath -Value $cacheLines
    $invalidStatus = Get-XplayerBuildCacheStatus -BuildDir $testRoot -Toolchain $toolchain -Configuration Release
    if ($invalidStatus.IsValid -or $invalidStatus.Reason -notlike 'CMAKE_CXX_COMPILER*') {
        throw 'A stale compiler cache was not rejected with the expected reason.'
    }

    $relativeOutput = Resolve-XplayerInstallerOutput -ProjectRoot $projectRoot -OutputDir 'artifacts'
    if (-not (Test-XplayerEquivalentPath $relativeOutput (Join-Path $projectRoot 'artifacts'))) {
        throw 'Relative installer output was not resolved from the project root.'
    }

    $dependencyStatus = Test-XplayerSourceDependencies -ProjectRoot $projectRoot
    if (-not $dependencyStatus.IsValid) {
        throw "The checked-in source dependencies were rejected: $($dependencyStatus.Reason)"
    }

    $missingDependencyRoot = Join-Path $testRoot 'missing-source'
    New-Item -ItemType Directory -Path $missingDependencyRoot -Force | Out-Null
    $missingDependencyStatus = Test-XplayerSourceDependencies -ProjectRoot $missingDependencyRoot
    if ($missingDependencyStatus.IsValid -or
        $missingDependencyStatus.Reason -notlike '*qwindowkit*') {
        throw 'Missing vendored source dependencies were not rejected clearly.'
    }

    Write-Host 'Xplayer build environment regression tests passed.'
}
finally {
    $resolvedTestRoot = [IO.Path]::GetFullPath($testRoot)
    $resolvedTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\')
    if ($resolvedTestRoot.StartsWith($resolvedTemp + '\', [StringComparison]::OrdinalIgnoreCase) -and
        (Test-Path -LiteralPath $resolvedTestRoot)) {
        Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force
    }
}
