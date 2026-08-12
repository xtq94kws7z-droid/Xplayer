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

$serverManager = 'src/XplayerCore/services/manager/servermanager.cpp'
$proxyConfig = 'src/XplayerCore/models/profile/proxyconfig.cpp'
$proxyManager = 'src/XplayerCore/api/proxymanager.cpp'
$webSocket = 'src/XplayerCore/api/embywebsocket.cpp'

Assert-FileContains -Path $serverManager `
    -Pattern 'accessTokenCipher' `
    -Message 'ServerManager must persist server access tokens with an encrypted accessTokenCipher field.'

Assert-FileNotContains -Path $serverManager `
    -Pattern 'obj\["accessToken"\]\s*=\s*p\.accessToken' `
    -Message 'ServerManager must not save server accessToken as plaintext JSON.'

Assert-FileContains -Path $proxyConfig `
    -Pattern 'passwordCipher' `
    -Message 'ProxyConfig JSON persistence must use encrypted passwordCipher.'

Assert-FileNotContains -Path $proxyConfig `
    -Pattern 'obj\["password"\]\s*=\s*password' `
    -Message 'ProxyConfig must not save proxy password as plaintext JSON.'

Assert-FileContains -Path $proxyManager `
    -Pattern 'ProxyPasswordCipher' `
    -Message 'Global proxy password persistence must use ConfigKeys::ProxyPasswordCipher.'

Assert-FileNotContains -Path $proxyManager `
    -Pattern 'store->set\(ConfigKeys::ProxyPassword,\s*cfg\.password\)' `
    -Message 'ProxyManager must not save global proxy password as plaintext config.'

Assert-FileContains -Path $webSocket `
    -Pattern 'safeWebSocketUrlForLog' `
    -Message 'EmbyWebSocket logs must redact token-bearing websocket URLs.'

Assert-FileNotContains -Path $webSocket `
    -Pattern 'Connecting to:"\s*<<\s*url' `
    -Message 'EmbyWebSocket must not log raw websocket URLs containing api_key.'

Assert-FileNotContains -Path $webSocket `
    -Pattern '\|\s*url:"\s*<<\s*buildWebSocketUrl\(\)' `
    -Message 'EmbyWebSocket TLS logs must not print raw websocket URLs containing api_key.'

Write-Host 'Xplayer secret persistence verification passed.'
