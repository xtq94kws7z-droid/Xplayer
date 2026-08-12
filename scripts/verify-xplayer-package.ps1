param([string]$OutputDir)

$ErrorActionPreference = 'Stop'

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $PSScriptRoot 'xplayer-build-environment.ps1')
Set-Location -LiteralPath $projectRoot

$exePath = 'build-xplayer/bin/Xplayer.exe'
$rcTemplatePath = 'src/XplayerApp/XplayerApp.rc.in'
$packageScriptPath = 'scripts/package-xplayer.ps1'
$projectContent = Get-Content -LiteralPath 'CMakeLists.txt' -Raw
$versionMatch = [regex]::Match(
    $projectContent,
    'project\s*\(\s*Xplayer\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)\s+LANGUAGES\s+CXX\s*\)')
if (-not $versionMatch.Success) {
    throw 'Could not read the Xplayer version from CMakeLists.txt.'
}
$expectedVersion = $versionMatch.Groups[1].Value
$installerOutput = Resolve-XplayerInstallerOutput -ProjectRoot $projectRoot -OutputDir $OutputDir
$installerPath = Join-Path $installerOutput "Xplayer-$expectedVersion-Setup.exe"

if (-not (Test-Path -LiteralPath $exePath)) {
    throw 'Release Xplayer.exe is missing.'
}

$rcTemplate = Get-Content -LiteralPath $rcTemplatePath -Raw
if ($rcTemplate -notmatch 'resources/XplayerApp\.ico') {
    throw 'Windows native resource must use resources/XplayerApp.ico.'
}
if ($rcTemplate -notmatch 'VS_VERSION_INFO\s+VERSIONINFO') {
    throw 'Windows native resource must include VERSIONINFO metadata.'
}

$exeVersion = (Get-Item -LiteralPath $exePath).VersionInfo
if ($exeVersion.FileVersion -notlike "$expectedVersion*" -or
    $exeVersion.ProductVersion -notlike "$expectedVersion*") {
    throw "Xplayer.exe version metadata must be $expectedVersion. FileVersion='$($exeVersion.FileVersion)' ProductVersion='$($exeVersion.ProductVersion)'."
}

$packageScript = Get-Content -LiteralPath $packageScriptPath -Raw
if ($packageScript -notmatch 'Inno Setup|ISCC\.exe') {
    throw 'Packaging script must use Inno Setup.'
}
if ($packageScript -match 'IExpress|iexpress\.exe') {
    throw 'Packaging script must not use IExpress.'
}
if ($packageScript -notmatch 'AppId=\{\{8E6B495D-458A-49A9-9AC8-2EF160820101\}') {
    throw 'Installer AppId must remain stable so existing users update in place.'
}
if ($packageScript -notmatch 'DefaultDirName=\{localappdata\}\\Programs\\Xplayer') {
    throw 'Default install directory must remain the per-user Xplayer directory.'
}
if ($packageScript -notmatch 'PrivilegesRequired=lowest') {
    throw 'Installer must remain per-user and avoid elevation prompts.'
}
if ($packageScript -notmatch 'CloseApplications=yes') {
    throw 'Installer must close running applications before replacing loaded DLLs.'
}
if ($packageScript -notmatch 'CloseApplicationsFilter=\*\.exe,\*\.dll') {
    throw 'Installer must detect running EXE and DLL file locks during upgrades.'
}
if ($packageScript -notmatch 'RestartApplications=no') {
    throw 'Installer must not auto-restart old app instances during upgrades.'
}
if ($packageScript -notmatch 'Flags:\s*ignoreversion\s+recursesubdirs\s+createallsubdirs\s+restartreplace') {
    throw 'Installer files must use restartreplace as a fallback for locked files.'
}
if ($packageScript -match '\{commondesktop\}') {
    throw 'Per-user installer must not create shortcuts on the public desktop.'
}
if ($packageScript -notmatch '\{userdesktop\}\\Xplayer') {
    throw 'Desktop shortcut task must create a current-user desktop shortcut.'
}
if ($packageScript -notmatch '\*_test\.exe') {
    throw 'Installer must exclude test executables.'
}
if ($packageScript -notmatch 'Qt6Test\.dll') {
    throw 'Installer must exclude the Qt test runtime.'
}
if ($packageScript -notmatch 'playedhistory_probe\.exe') {
    throw 'Installer must exclude local probe utilities.'
}
if ($packageScript -notmatch '\[InstallDelete\][\s\S]*\{app\}\\Qt6Test\.dll') {
    throw 'Upgrades must remove a stale Qt test runtime from earlier installations.'
}

if (-not (Test-Path -LiteralPath $installerPath)) {
    throw 'Xplayer installer executable is missing.'
}

$installer = Get-Item -LiteralPath $installerPath
if ($installer.Length -lt 1MB) {
    throw "Xplayer installer is unexpectedly small: $($installer.Length) bytes."
}

$installerVersion = $installer.VersionInfo
if ($installerVersion.FileVersion -notlike "$expectedVersion*" -or
    $installerVersion.ProductVersion -notlike "$expectedVersion*") {
    throw "Installer version metadata must be $expectedVersion. FileVersion='$($installerVersion.FileVersion)' ProductVersion='$($installerVersion.ProductVersion)'."
}

Write-Host "Xplayer package verification passed: $installerPath"
