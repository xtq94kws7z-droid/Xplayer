$ErrorActionPreference = 'Stop'

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
Set-Location -LiteralPath $projectRoot

$qssPath = 'src/XplayerApp/resources/qss/light-style.qss'
$qss = Get-Content -LiteralPath $qssPath -Raw

function Assert-QssContains {
    param(
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$Message
    )

    if ($qss -notmatch $Pattern) {
        throw $Message
    }
}

Assert-QssContains -Pattern '#F4F3F7' `
    -Message 'Light theme must use the approved Xplayer app background #F4F3F7.'

Assert-QssContains -Pattern '#FAF8F4' `
    -Message 'Light theme must include the approved warm content background #FAF8F4.'

Assert-QssContains -Pattern '#2F80ED' `
    -Message 'Light theme must use the approved Xplayer accent blue #2F80ED.'

Assert-QssContains -Pattern 'QLineEdit#titlebar-search\s*\{[\s\S]*background-color:\s*rgba\(255,\s*255,\s*255,\s*0\.72\)' `
    -Message 'Titlebar search must use a glass-like translucent white background.'

Assert-QssContains -Pattern 'QLineEdit#titlebar-search\s*\{[\s\S]*border-radius:\s*18px' `
    -Message 'Titlebar search must use a more pill-like 18px radius.'

Write-Host 'Xplayer light theme verification passed.'
