param(
    [string]$BuildDir,
    [string]$OutputDir,
    [string]$IsccPath,
    [int]$Jobs = 4
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $PSScriptRoot 'xplayer-build-environment.ps1')
$BuildDir = if ($BuildDir) {
    ConvertTo-XplayerAbsolutePath -Path $BuildDir -BasePath $projectRoot
} else {
    Join-Path $projectRoot 'build-xplayer'
}
$OutputDir = Resolve-XplayerInstallerOutput -ProjectRoot $projectRoot -OutputDir $OutputDir

$appName = 'Xplayer'
$publisher = 'Godking'
$buildBin = Join-Path $BuildDir 'bin'
$exePath = Join-Path $buildBin 'Xplayer.exe'
$iconPath = Join-Path $projectRoot 'src\XplayerApp\resources\XplayerApp.ico'
$projectFile = Join-Path $projectRoot 'CMakeLists.txt'
$projectContent = Get-Content -LiteralPath $projectFile -Raw
$versionMatch = [regex]::Match(
    $projectContent,
    'project\s*\(\s*Xplayer\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)\s+LANGUAGES\s+CXX\s*\)')
if (-not $versionMatch.Success) {
    throw "Could not read the Xplayer version from $projectFile."
}
$appVersion = $versionMatch.Groups[1].Value
$outputBaseName = "$appName-$appVersion-Setup"
$installerPath = Join-Path $OutputDir ($outputBaseName + '.exe')
$staging = Join-Path ([IO.Path]::GetTempPath()) (
    'Xplayer-inno-' + [Guid]::NewGuid().ToString('N'))

function Resolve-IsccPath {
    param([string]$ExplicitPath)

    $programFilesX86 = [Environment]::GetFolderPath([Environment+SpecialFolder]::ProgramFilesX86)
    $programFiles = [Environment]::GetFolderPath([Environment+SpecialFolder]::ProgramFiles)
    $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    $pathCommand = if ($command) { $command.Source } else { $null }
    $candidates = @(
        $ExplicitPath,
        $env:XPLAYER_ISCC,
        'D:\Inno Setup 6\ISCC.exe',
        (Join-Path $programFilesX86 'Inno Setup 6\ISCC.exe'),
        (Join-Path $programFiles 'Inno Setup 6\ISCC.exe'),
        $pathCommand
    )
    $resolved = Get-XplayerFirstExistingFile $candidates
    if (-not $resolved) {
        throw 'Inno Setup 6 was not found. Set XPLAYER_ISCC to the full path of ISCC.exe.'
    }
    return $resolved
}

function Assert-ReleaseExecutable {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$ExpectedVersion
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Release executable is missing after the build: $Path"
    }
    $executable = Get-Item -LiteralPath $Path
    if ($executable.Length -lt 1MB) {
        throw "Release executable is unexpectedly small: $($executable.Length) bytes."
    }
    if ($executable.VersionInfo.FileVersion -notlike "$ExpectedVersion*" -or
        $executable.VersionInfo.ProductVersion -notlike "$ExpectedVersion*") {
        throw "Release executable version does not match CMakeLists.txt. Expected $ExpectedVersion, FileVersion='$($executable.VersionInfo.FileVersion)', ProductVersion='$($executable.VersionInfo.ProductVersion)'."
    }
}

if (-not (Test-Path -LiteralPath $iconPath -PathType Leaf)) {
    throw "Xplayer installer icon is missing: $iconPath"
}

$iscc = Resolve-IsccPath -ExplicitPath $IsccPath
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
if (Test-Path -LiteralPath $installerPath) {
    Remove-Item -LiteralPath $installerPath -Force
}

& (Join-Path $PSScriptRoot 'build-xplayer.ps1') -BuildDir $BuildDir -Configuration Release -Jobs $Jobs
Assert-ReleaseExecutable -Path $exePath -ExpectedVersion $appVersion
$toolchain = Resolve-XplayerToolchain -ProjectRoot $projectRoot
$ctest = Join-Path (Split-Path $toolchain.CMake -Parent) 'ctest.exe'
if (-not (Test-Path -LiteralPath $ctest -PathType Leaf)) {
    throw "CTest is missing beside CMake: $ctest"
}
& $ctest --test-dir $BuildDir --output-on-failure -C Release
if ($LASTEXITCODE -ne 0) {
    throw "Release tests failed with exit code $LASTEXITCODE."
}

New-Item -ItemType Directory -Path $staging -Force | Out-Null

try {
    $issTemplate = @'
#define AppName "@APP_NAME@"
#define AppVersion "@APP_VERSION@"
#define AppPublisher "@APP_PUBLISHER@"
#define BuildBin "@BUILD_BIN@"

[Setup]
AppId={{8E6B495D-458A-49A9-9AC8-2EF160820101}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
VersionInfoVersion={#AppVersion}
VersionInfoCompany={#AppPublisher}
VersionInfoDescription=Xplayer installer
DefaultDirName={localappdata}\Programs\Xplayer
DefaultGroupName=Xplayer
DisableProgramGroupPage=yes
OutputDir=@OUTPUT_DIR@
OutputBaseFilename=@OUTPUT_BASE_NAME@
SetupIconFile=@ICON_PATH@
UninstallDisplayIcon={app}\Xplayer.exe
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
CloseApplications=yes
CloseApplicationsFilter=*.exe,*.dll
RestartApplications=no
SetupLogging=yes

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#BuildBin}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs restartreplace; Excludes: "*.lnk,lib?EmbyCore.dll,Qt6Test.dll,*_test.exe,playedhistory_probe.exe"

[InstallDelete]
Type: files; Name: "{app}\Qt6Test.dll"
Type: files; Name: "{app}\*_test.exe"
Type: files; Name: "{app}\playedhistory_probe.exe"

[Icons]
Name: "{group}\Xplayer"; Filename: "{app}\Xplayer.exe"; WorkingDir: "{app}"; IconFilename: "{app}\Xplayer.exe"
Name: "{userdesktop}\Xplayer"; Filename: "{app}\Xplayer.exe"; WorkingDir: "{app}"; IconFilename: "{app}\Xplayer.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\Xplayer.exe"; Description: "运行 Xplayer"; Flags: nowait postinstall skipifsilent
'@

    $iss = $issTemplate
    $replacements = @{
        '@APP_NAME@' = $appName
        '@APP_VERSION@' = $appVersion
        '@APP_PUBLISHER@' = $publisher
        '@BUILD_BIN@' = $buildBin
        '@OUTPUT_DIR@' = $OutputDir
        '@OUTPUT_BASE_NAME@' = $outputBaseName
        '@ICON_PATH@' = $iconPath
    }
    foreach ($entry in $replacements.GetEnumerator()) {
        $iss = $iss.Replace($entry.Key, $entry.Value)
    }

    $issPath = Join-Path $staging 'xplayer-installer.iss'
    $utf8Bom = [Text.UTF8Encoding]::new($true)
    [IO.File]::WriteAllText($issPath, $iss, $utf8Bom)

    & $iscc $issPath
    if ($LASTEXITCODE -ne 0) {
        throw "Inno Setup failed with exit code $LASTEXITCODE."
    }
    if (-not (Test-Path -LiteralPath $installerPath -PathType Leaf)) {
        throw "Inno Setup did not create $installerPath."
    }

    $installer = Get-Item -LiteralPath $installerPath
    if ($installer.Length -lt 1MB -or
        $installer.VersionInfo.FileVersion -notlike "$appVersion*" -or
        $installer.VersionInfo.ProductVersion -notlike "$appVersion*") {
        throw "Generated installer failed size or version validation: $installerPath"
    }
    & (Join-Path $PSScriptRoot 'verify-xplayer-package.ps1') -OutputDir $OutputDir
    Write-Host "Xplayer installer created and verified: $installerPath"
}
finally {
    $resolvedStaging = [IO.Path]::GetFullPath($staging)
    $resolvedTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\')
    if ($resolvedStaging.StartsWith($resolvedTemp + '\', [StringComparison]::OrdinalIgnoreCase) -and
        (Test-Path -LiteralPath $resolvedStaging)) {
        Remove-Item -LiteralPath $resolvedStaging -Recurse -Force
    }
}
