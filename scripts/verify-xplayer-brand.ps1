$ErrorActionPreference = 'Stop'

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
Set-Location -LiteralPath $projectRoot

function Assert-FileContains {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$Message
    )

    $content = Get-Content -LiteralPath $Path -Raw
    if ($content -notmatch $Pattern) {
        throw $Message
    }
}

function Assert-FileDoesNotContain {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$Message
    )

    $content = Get-Content -LiteralPath $Path -Raw
    if ($content -match $Pattern) {
        throw $Message
    }
}

Assert-FileContains -Path 'CMakeLists.txt' `
    -Pattern 'project\s*\(\s*Xplayer\s+VERSION\s+\d+\.\d+\.\d+\s+LANGUAGES\s+CXX\s*\)' `
    -Message 'Root CMake project must be named Xplayer and expose a semantic version.'

Assert-FileContains -Path 'src/XplayerApp/CMakeLists.txt' `
    -Pattern 'set\s*\(\s*XPLAYER_APP_TARGET\s+Xplayer\s*\)' `
    -Message 'The app executable target must be centralized as XPLAYER_APP_TARGET Xplayer.'

Assert-FileContains -Path 'src/XplayerApp/CMakeLists.txt' `
    -Pattern 'qt_add_executable\s*\(\s*\$\{XPLAYER_APP_TARGET\}' `
    -Message 'qt_add_executable must create the Xplayer app target.'

Assert-FileContains -Path 'src/XplayerApp/main.cpp' `
    -Pattern 'setDesktopFileName\s*\(\s*QStringLiteral\("xplayer"\)\s*\)' `
    -Message 'Desktop file id must be xplayer.'

Assert-FileContains -Path 'src/XplayerApp/main.cpp' `
    -Pattern 'setOrganizationDomain\s*\(\s*"local\.xplayer"\s*\)' `
    -Message 'Organization domain must be local.xplayer for the forked app identity.'

Assert-FileContains -Path 'src/XplayerApp/main.cpp' `
    -Pattern 'kOrganizationName\s*=\s*"Godking"' `
    -Message 'Application organization must use the Godking brand.'

Assert-FileContains -Path 'LICENSE' `
    -Pattern 'Copyright \(c\) 2026 Godking' `
    -Message 'The project license must identify Godking as the copyright holder.'

Assert-FileDoesNotContain -Path 'src/XplayerApp/main.cpp' `
    -Pattern 'LegacyOrganization|migrateLegacyOrganizationStorage' `
    -Message 'Application startup must not contain obsolete organization migration code.'

Assert-FileContains -Path 'src/XplayerApp/views/settings/pageabout.cpp' `
    -Pattern 'm_appNameLabel\s*=\s*new QLabel\s*\(\s*qApp->applicationName\(\)' `
    -Message 'About page app name must come from QApplication::applicationName().'

Assert-FileContains -Path 'src/XplayerApp/managers/traymanager.cpp' `
    -Pattern 'tr\s*\(\s*"Show Xplayer"\s*\)' `
    -Message 'Tray show action must use the Xplayer name.'

Write-Host 'Xplayer brand verification passed.'
