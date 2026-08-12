Set-StrictMode -Version Latest

function ConvertTo-XplayerAbsolutePath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$BasePath
    )

    if ([IO.Path]::IsPathRooted($Path)) {
        return [IO.Path]::GetFullPath($Path)
    }
    return [IO.Path]::GetFullPath((Join-Path $BasePath $Path))
}

function Resolve-XplayerInstallerOutput {
    param(
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [string]$OutputDir
    )

    if ($OutputDir) {
        return ConvertTo-XplayerAbsolutePath -Path $OutputDir -BasePath $ProjectRoot
    }
    if ($env:XPLAYER_INSTALLER_OUTPUT) {
        return ConvertTo-XplayerAbsolutePath -Path $env:XPLAYER_INSTALLER_OUTPUT -BasePath $ProjectRoot
    }
    if (Test-Path -LiteralPath 'D:\' -PathType Container) {
        return 'D:\anzhuang'
    }
    return Join-Path $ProjectRoot 'dist'
}

function Test-XplayerSourceDependencies {
    param([Parameter(Mandatory = $true)][string]$ProjectRoot)

    $projectRoot = [IO.Path]::GetFullPath($ProjectRoot)
    $requiredFiles = @(
        'libs\qwindowkit\CMakeLists.txt',
        'libs\qwindowkit\src\widgets\widgetwindowagent.cpp',
        'libs\qwindowkit\examples\shared\CMakeLists.txt'
    )
    foreach ($relativePath in $requiredFiles) {
        $absolutePath = Join-Path $projectRoot $relativePath
        if (-not (Test-Path -LiteralPath $absolutePath -PathType Leaf)) {
            return [pscustomobject]@{
                IsValid = $false
                Reason = "Required qwindowkit source file is missing: $relativePath. Restore the complete libs\qwindowkit directory before building."
            }
        }
    }

    return [pscustomobject]@{
        IsValid = $true
        Reason = 'Vendored source dependencies are complete.'
    }
}

function Get-XplayerFirstExistingFile {
    param([string[]]$Candidates)

    foreach ($candidate in $Candidates) {
        if (-not [string]::IsNullOrWhiteSpace($candidate)) {
            $resolved = [Environment]::ExpandEnvironmentVariables($candidate.Trim())
            if (Test-Path -LiteralPath $resolved -PathType Leaf) {
                return [IO.Path]::GetFullPath($resolved)
            }
        }
    }
    return $null
}

function Get-XplayerFirstValidRoot {
    param(
        [string[]]$Candidates,
        [Parameter(Mandatory = $true)][string]$RequiredRelativePath
    )

    foreach ($candidate in $Candidates) {
        if (-not [string]::IsNullOrWhiteSpace($candidate)) {
            $resolved = [IO.Path]::GetFullPath(
                [Environment]::ExpandEnvironmentVariables($candidate.Trim()))
            if (Test-Path -LiteralPath (Join-Path $resolved $RequiredRelativePath) -PathType Leaf) {
                return $resolved
            }
        }
    }
    return $null
}

function Get-XplayerSubdirectories {
    param([string[]]$Roots)

    foreach ($root in $Roots) {
        if ($root -and (Test-Path -LiteralPath $root -PathType Container)) {
            Get-ChildItem -LiteralPath $root -Directory -ErrorAction SilentlyContinue |
                Sort-Object Name -Descending |
                ForEach-Object { $_.FullName }
        }
    }
}

function Resolve-XplayerToolchain {
    param(
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [string]$CMakePath,
        [string]$QtRoot,
        [string]$MinGWRoot,
        [string]$NinjaRoot
    )

    $projectRoot = [IO.Path]::GetFullPath($ProjectRoot)
    $cmakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue
    $pathCMake = if ($cmakeCommand) { $cmakeCommand.Source } else { $null }
    $cmake = Get-XplayerFirstExistingFile @(
        $CMakePath,
        $env:XPLAYER_CMAKE,
        (Join-Path $projectRoot 'tools\cmake-4.4.2-windows-x86_64\bin\cmake.exe'),
        $pathCMake
    )

    $qtInstallRoots = @('D:\Qt', 'C:\Qt', (Join-Path $env:USERPROFILE 'Qt'))
    $qtVersionRoots = @(Get-XplayerSubdirectories $qtInstallRoots)
    $qtDiscoveredRoots = foreach ($versionRoot in $qtVersionRoots) {
        Get-XplayerSubdirectories @($versionRoot) |
            Where-Object { $_ -match 'mingw[^\\/]*$' }
    }
    $qt = Get-XplayerFirstValidRoot @(
        $QtRoot,
        $env:XPLAYER_QT_ROOT,
        'D:\Qt\6.11.1\mingw_64',
        'C:\Qt\6.11.1\mingw_64',
        $qtDiscoveredRoots
    ) 'lib\cmake\Qt6\Qt6Config.cmake'

    $toolRoots = @('D:\Qt\Tools', 'C:\Qt\Tools', (Join-Path $env:USERPROFILE 'Qt\Tools'))
    $toolSubdirectories = @(Get-XplayerSubdirectories $toolRoots)
    $mingw = Get-XplayerFirstValidRoot @(
        $MinGWRoot,
        $env:XPLAYER_MINGW_ROOT,
        'D:\Qt\Tools\mingw1310_64',
        'C:\Qt\Tools\mingw1310_64',
        ($toolSubdirectories | Where-Object { (Split-Path $_ -Leaf) -match '^mingw' })
    ) 'bin\c++.exe'
    $ninjaRootResolved = Get-XplayerFirstValidRoot @(
        $NinjaRoot,
        $env:XPLAYER_NINJA_ROOT,
        'D:\Qt\Tools\Ninja',
        'C:\Qt\Tools\Ninja',
        ($toolSubdirectories | Where-Object { (Split-Path $_ -Leaf) -eq 'Ninja' })
    ) 'ninja.exe'

    if (-not $cmake) {
        throw 'CMake was not found. Set XPLAYER_CMAKE to cmake.exe or install CMake.'
    }
    if (-not $qt) {
        throw 'Qt 6 MinGW was not found. Set XPLAYER_QT_ROOT to the Qt kit root containing lib\cmake\Qt6\Qt6Config.cmake.'
    }
    if (-not $mingw) {
        throw 'MinGW was not found. Set XPLAYER_MINGW_ROOT to the toolchain root containing bin\c++.exe.'
    }
    if (-not $ninjaRootResolved) {
        throw 'Ninja was not found. Set XPLAYER_NINJA_ROOT to the directory containing ninja.exe.'
    }

    $mingwBin = Join-Path $mingw 'bin'
    $cc1 = Get-ChildItem -LiteralPath (Join-Path $mingw 'libexec\gcc') -Filter cc1plus.exe -File -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if (-not $cc1) {
        throw "MinGW is incomplete: cc1plus.exe is missing below $(Join-Path $mingw 'libexec\gcc')."
    }

    [pscustomobject]@{
        ProjectRoot = $projectRoot
        CMake = $cmake
        QtRoot = $qt
        MinGWRoot = $mingw
        MinGWBin = $mingwBin
        CCompiler = Join-Path $mingwBin 'gcc.exe'
        CXXCompiler = Join-Path $mingwBin 'c++.exe'
        Ninja = Join-Path $ninjaRootResolved 'ninja.exe'
        NinjaRoot = $ninjaRootResolved
        QtBin = Join-Path $qt 'bin'
    }
}

function Set-XplayerToolchainPath {
    param([Parameter(Mandatory = $true)]$Toolchain)

    $requiredPaths = @($Toolchain.MinGWBin, $Toolchain.NinjaRoot, $Toolchain.QtBin)
    $existingPaths = @($env:PATH -split ';' | Where-Object { $_ })
    $env:PATH = (@($requiredPaths) + $existingPaths | Select-Object -Unique) -join ';'
}

function Read-XplayerCMakeCache {
    param([Parameter(Mandatory = $true)][string]$CachePath)

    $values = @{}
    foreach ($line in Get-Content -LiteralPath $CachePath) {
        if ($line -match '^([^#/][^:]*):[^=]*=(.*)$') {
            $values[$Matches[1]] = $Matches[2]
        }
    }
    return $values
}

function Test-XplayerEquivalentPath {
    param([string]$Actual, [string]$Expected)

    if ([string]::IsNullOrWhiteSpace($Actual) -or [string]::IsNullOrWhiteSpace($Expected)) {
        return $false
    }
    try {
        $actualPath = [IO.Path]::GetFullPath($Actual.Replace('/', '\')).TrimEnd('\')
        $expectedPath = [IO.Path]::GetFullPath($Expected.Replace('/', '\')).TrimEnd('\')
        return $actualPath.Equals($expectedPath, [StringComparison]::OrdinalIgnoreCase)
    }
    catch {
        return $false
    }
}

function Get-XplayerBuildCacheStatus {
    param(
        [Parameter(Mandatory = $true)][string]$BuildDir,
        [Parameter(Mandatory = $true)]$Toolchain,
        [Parameter(Mandatory = $true)][string]$Configuration
    )

    $cachePath = Join-Path $BuildDir 'CMakeCache.txt'
    $ninjaFile = Join-Path $BuildDir 'build.ninja'
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) {
        return [pscustomobject]@{ IsValid = $false; Reason = 'CMakeCache.txt is missing.' }
    }
    if (-not (Test-Path -LiteralPath $ninjaFile -PathType Leaf)) {
        return [pscustomobject]@{ IsValid = $false; Reason = 'build.ninja is missing.' }
    }

    $cache = Read-XplayerCMakeCache $cachePath
    $checks = @(
        @{ Key = 'CMAKE_HOME_DIRECTORY'; Expected = $Toolchain.ProjectRoot },
        @{ Key = 'CMAKE_C_COMPILER'; Expected = $Toolchain.CCompiler },
        @{ Key = 'CMAKE_CXX_COMPILER'; Expected = $Toolchain.CXXCompiler },
        @{ Key = 'CMAKE_MAKE_PROGRAM'; Expected = $Toolchain.Ninja },
        @{ Key = 'CMAKE_PREFIX_PATH'; Expected = $Toolchain.QtRoot },
        @{ Key = 'Qt6_DIR'; Expected = (Join-Path $Toolchain.QtRoot 'lib\cmake\Qt6') }
    )
    foreach ($check in $checks) {
        if (-not $cache.ContainsKey($check.Key)) {
            return [pscustomobject]@{
                IsValid = $false
                Reason = "$($check.Key) does not match the selected toolchain."
            }
        }
        $cacheValue = $cache[$check.Key]
        $matchesPath = if ($check.Key -eq 'CMAKE_PREFIX_PATH') {
            @($cacheValue -split ';' | ForEach-Object { $_.Trim() }) |
                Where-Object { Test-XplayerEquivalentPath $_ $check.Expected } |
                Select-Object -First 1
        } else {
            Test-XplayerEquivalentPath $cacheValue $check.Expected
        }
        if (-not $matchesPath) {
            return [pscustomobject]@{
                IsValid = $false
                Reason = "$($check.Key) does not match the selected toolchain."
            }
        }
    }
    if (-not $cache.ContainsKey('CMAKE_GENERATOR') -or $cache['CMAKE_GENERATOR'] -ne 'Ninja') {
        return [pscustomobject]@{
            IsValid = $false
            Reason = 'CMAKE_GENERATOR is not Ninja.'
        }
    }
    if (-not $cache.ContainsKey('CMAKE_BUILD_TYPE') -or $cache['CMAKE_BUILD_TYPE'] -ne $Configuration) {
        return [pscustomobject]@{
            IsValid = $false
            Reason = "CMAKE_BUILD_TYPE is not $Configuration."
        }
    }
    return [pscustomobject]@{ IsValid = $true; Reason = 'CMake cache matches the selected toolchain.' }
}
