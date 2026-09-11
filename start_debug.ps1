# PowerShell script to start Wols_CA_PasswordGenerator (Debug version)
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

$exePath = Join-Path $scriptDir "cmake-build-debug\Wols_CA_PasswordGenerator.exe"

if (-not (Test-Path $exePath)) {
    Write-Host "Debug executable not found at: $exePath" -ForegroundColor Yellow
    Write-Host "Please build the debug configuration first." -ForegroundColor Yellow
    exit 1
}

Write-Host "Starting Debug version: $exePath" -ForegroundColor Green
Start-Process -FilePath $exePath
