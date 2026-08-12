param(
    [int]$DurationSeconds = 60,
    [int]$IntervalMilliseconds = 1000,
    [string]$ProcessName = "Xplayer",
    [string]$OutputPath = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $OutputPath = Join-Path (Join-Path (Get-Location) "artifacts\profiling") `
        "xplayer-process-$timestamp.csv"
}

$outputDirectory = Split-Path -Parent $OutputPath
if ($outputDirectory) {
    New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
}

$process = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue |
    Select-Object -First 1
if (-not $process) {
    throw "Process '$ProcessName' was not found."
}

$rows = [System.Collections.Generic.List[object]]::new()
$stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

while ($stopwatch.Elapsed.TotalSeconds -lt $DurationSeconds) {
    $process = Get-Process -Id $process.Id -ErrorAction SilentlyContinue
    if (-not $process) {
        break
    }

    $rows.Add([pscustomobject]@{
        timestamp = Get-Date -Format o
        pid = $process.Id
        cpu_seconds = [double]$process.CPU
        working_set_mb = [math]::Round($process.WorkingSet64 / 1MB, 3)
        private_mb = [math]::Round($process.PrivateMemorySize64 / 1MB, 3)
        virtual_mb = [math]::Round($process.VirtualMemorySize64 / 1MB, 3)
        handles = $process.HandleCount
        threads = $process.Threads.Count
    })

    Start-Sleep -Milliseconds $IntervalMilliseconds
}

$rows | Export-Csv -Path $OutputPath -NoTypeInformation -Encoding utf8
$rows | Select-Object -First 1
$rows | Select-Object -Last 1
