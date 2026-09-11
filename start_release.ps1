# PowerShell script to start Wols_CA_PasswordGenerator (Release version)
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

$exePath = Join-Path $scriptDir "cmake-build-release\Wols_CA_PasswordGenerator.exe"

if (-not (Test-Path $exePath)) {
    Write-Host "Release executable not found at: $exePath" -ForegroundColor Yellow
    Write-Host "Rebuilding in Release mode now..." -ForegroundColor Cyan
    & (Join-Path $scriptDir "rebuild_Release.ps1")
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $exePath)) {
        Write-Error "Failed to build Release executable."
        exit 1
    }
}

Write-Host "Starting Release version: $exePath" -ForegroundColor Green
Start-Process -FilePath $exePath
