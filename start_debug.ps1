# PowerShell script to start Wols_CA_PasswordGenerator (Debug version)
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

$qtDir = "C:\Qt\6.11.1\msvc2022_64\bin"
if (Test-Path $qtDir -and -not ($env:PATH -split ';' -contains $qtDir)) {
    $env:PATH = "$qtDir;$env:PATH"
}

$exePath = Join-Path $scriptDir "cmake-build-debug\Wols_CA_PasswordGenerator.exe"

if (-not (Test-Path $exePath)) {
    Write-Host "Debug executable not found at: $exePath" -ForegroundColor Yellow
    Write-Host "Please build the debug configuration first." -ForegroundColor Yellow
    exit 1
}

Write-Host "Starting Debug version: $exePath" -ForegroundColor Green
Start-Process -FilePath $exePath
