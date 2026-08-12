$ErrorActionPreference = 'Stop'

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
Set-Location -LiteralPath $projectRoot

Add-Type -AssemblyName System.Drawing

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

$pngPath = 'src/XplayerApp/resources/images/xplayer_icon.png'
$icoPath = 'src/XplayerApp/resources/XplayerApp.ico'

if (-not (Test-Path -LiteralPath $pngPath)) {
    throw 'Cropped Xplayer icon PNG is missing.'
}
if (-not (Test-Path -LiteralPath $icoPath)) {
    throw 'Windows ICO resource is missing.'
}

$image = [System.Drawing.Image]::FromFile((Resolve-Path -LiteralPath $pngPath))
try {
    if ($image.Width -ne 1024 -or $image.Height -ne 1024) {
        throw "Cropped icon PNG must be 1024x1024, got $($image.Width)x$($image.Height)."
    }
} finally {
    $image.Dispose()
}

Assert-FileContains -Path 'src/XplayerApp/resources/resources.qrc' `
    -Pattern '<file>images/xplayer_icon\.png</file>' `
    -Message 'resources.qrc must include images/xplayer_icon.png.'

Assert-FileContains -Path 'src/XplayerApp/main.cpp' `
    -Pattern 'QIcon appIcon\s*\(\s*QStringLiteral\(":/images/xplayer_icon\.png"\)\s*\)' `
    -Message 'main.cpp must use the new Xplayer icon resource.'

Assert-FileContains -Path 'src/XplayerApp/managers/traymanager.cpp' `
    -Pattern 'QIcon\s*\(":/images/xplayer_icon\.png"\)' `
    -Message 'Tray icon must use the new Xplayer icon resource.'

Assert-FileContains -Path 'src/XplayerApp/views/settings/pageabout.cpp' `
    -Pattern 'QPixmap logoPixmap\s*\(":/images/xplayer_icon\.png"\)' `
    -Message 'About page logo must use the new Xplayer icon resource.'

Assert-FileContains -Path 'src/XplayerApp/resources/qss/light-style.qss' `
    -Pattern 'qproperty-iconNormal:\s*url\(":/images/xplayer_icon\.png"\)' `
    -Message 'Light titlebar icon must use the new Xplayer icon resource.'

Assert-FileContains -Path 'src/XplayerApp/resources/qss/dark-style.qss' `
    -Pattern 'qproperty-iconNormal:\s*url\(":/images/xplayer_icon\.png"\)' `
    -Message 'Dark titlebar icon must use the new Xplayer icon resource.'

Assert-FileNotContains -Path 'src/XplayerApp/main.cpp' `
    -Pattern 'xplayer_logo\.svg' `
    -Message 'main.cpp must not use the old xplayer_logo.svg icon.'

Assert-FileNotContains -Path 'README.md' `
    -Pattern 'xplayer_logo\.svg' `
    -Message 'README.md must use the current PNG application icon.'

Assert-FileNotContains -Path 'src/XplayerApp/resources/resources.qrc' `
    -Pattern 'xplayer_logo\.svg' `
    -Message 'The obsolete xplayer_logo.svg must not remain in application resources.'

Write-Host 'Xplayer icon verification passed.'
