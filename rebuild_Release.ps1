# PowerShell script to rebuild Wols_CA_PasswordGenerator in Release mode
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "  Rebuilding Wols_CA_PasswordGenerator (Release)  " -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

# Locate VsDevCmd.bat
$vsDevCmd = $null
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($vsPath -and (Test-Path "$vsPath\Common7\Tools\VsDevCmd.bat")) {
        $vsDevCmd = "$vsPath\Common7\Tools\VsDevCmd.bat"
    }
}

if (-not $vsDevCmd) {
    $fallbackPaths = @(
        "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
    )
    foreach ($p in $fallbackPaths) {
        if (Test-Path $p) {
            $vsDevCmd = $p
            break
        }
    }
}

if (-not $vsDevCmd) {
    Write-Error "VsDevCmd.bat not found. Please ensure Visual Studio C++ is installed."
    exit 1
}

Write-Host "Using Visual Studio Dev Command: $vsDevCmd" -ForegroundColor Gray

# Locate Qt directory
$qtDir = "C:/Qt/6.11.1/msvc2022_64"
if (-not (Test-Path $qtDir)) {
    $qtDir = "C:/Qt/6.11.2/msvc2022_64"
}

$buildDir = "cmake-build-release"

# Clean existing build directory if requested or recreate
if (Test-Path $buildDir) {
    Write-Host "Cleaning existing release build..." -ForegroundColor Yellow
    try {
        Remove-Item -Recurse -Force $buildDir -ErrorAction SilentlyContinue
    } catch {
        # Continue if file is temporarily locked
    }
}

Write-Host "Configuring CMake (Release)..." -ForegroundColor Green
$cmdConfigure = "call `"$vsDevCmd`" -arch=x64 && cmake -B `"$buildDir`" -DCMAKE_PREFIX_PATH=`"$qtDir`" -DCMAKE_BUILD_TYPE=Release -G Ninja"
cmd /c $cmdConfigure
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed with exit code $LASTEXITCODE."
    exit $LASTEXITCODE
}

Write-Host "Building project in Release mode..." -ForegroundColor Green
$cmdBuild = "call `"$vsDevCmd`" -arch=x64 && cmake --build `"$buildDir`" --config Release"
cmd /c $cmdBuild
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed with exit code $LASTEXITCODE."
    exit $LASTEXITCODE
}

# Run windeployqt if available
$windeployqt = "$qtDir/bin/windeployqt.exe"
if (Test-Path $windeployqt) {
    Write-Host "Deploying Qt runtime dependencies..." -ForegroundColor Green
    & $windeployqt --no-translations --compiler-runtime "$buildDir/Wols_CA_PasswordGenerator.exe"
}

Write-Host "==================================================" -ForegroundColor Green
Write-Host "  Release build succeeded!                       " -ForegroundColor Green
Write-Host "  Executable: $buildDir\Wols_CA_PasswordGenerator.exe" -ForegroundColor Green
Write-Host "==================================================" -ForegroundColor Green
