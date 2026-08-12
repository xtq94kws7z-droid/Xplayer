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

function Assert-FileNotContains {
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
    -Pattern 'project\s*\(\s*Xplayer\s+VERSION\s+[0-9]+\.[0-9]+\.[0-9]+\s+LANGUAGES\s+CXX\s*\)' `
    -Message 'Project must define a semantic Xplayer version.'

Assert-FileNotContains -Path 'src/XplayerApp/mainwindow.cpp' `
    -Pattern 'UpdateManager|UpdateDialog|m_updateButton|CheckForUpdates' `
    -Message 'MainWindow must not contain automatic update checks or update indicators.'

Assert-FileNotContains -Path 'src/XplayerApp/mainwindow.h' `
    -Pattern 'UpdateInfo|UpdateIndicatorButton|showUpdateDialog|m_updateButton|m_availableUpdate' `
    -Message 'MainWindow header must not expose update state.'

Assert-FileNotContains -Path 'src/XplayerApp/views/settings/pagegeneral.cpp' `
    -Pattern 'Check for Updates|CheckForUpdates|Automatically check GitHub' `
    -Message 'General settings must not expose automatic update controls.'

Assert-FileNotContains -Path 'src/XplayerApp/views/settings/pageabout.cpp' `
    -Pattern 'UpdateManager|UpdateDialog|checkForUpdates|Check for Updates' `
    -Message 'About page must not offer manual update checks.'

$removedFiles = @(
    'src/XplayerApp/managers/updatemanager.cpp',
    'src/XplayerApp/managers/updatemanager.h',
    'src/XplayerApp/managers/windowsupdatemanager.cpp',
    'src/XplayerApp/managers/windowsupdatemanager.h',
    'src/XplayerApp/components/updatedialog.cpp',
    'src/XplayerApp/components/updatedialog.h',
    'src/XplayerApp/components/updateprogressdialog.cpp',
    'src/XplayerApp/components/updateprogressdialog.h',
    'src/XplayerApp/components/updateindicatorbutton.cpp',
    'src/XplayerApp/components/updateindicatorbutton.h'
)

foreach ($path in $removedFiles) {
    if (Test-Path -LiteralPath $path) {
        throw "Update-related source file must be removed: $path"
    }
}

$scanRoots = @('src/XplayerApp', 'src/XplayerCore')
$forbidden = 'api\.github\.com/repos/[^/]+/Xplayer/releases/latest|UpdateManager|WindowsUpdateManager|UpdateDialog|UpdateProgressDialog|UpdateIndicatorButton'
foreach ($root in $scanRoots) {
    $matches = rg -n $forbidden $root -g '!resources/i18n/**' -g '!resources/qss/**' -g '!*.ts'
    if ($LASTEXITCODE -eq 0) {
        throw "Update implementation references remain:`n$matches"
    }
    if ($LASTEXITCODE -ne 1) {
        throw "Failed to scan $root for update references."
    }
}

Write-Host 'Xplayer no-updates verification passed.'
