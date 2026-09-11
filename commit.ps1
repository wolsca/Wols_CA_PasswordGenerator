# PowerShell script to commit and push changes using CHANGELOG.md
[CmdletBinding()]
param(
    [string]$Title = "",
    [string]$Message = "",
    [switch]$NoPush
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "       Git Commit Helper (CHANGELOG-driven)       " -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

# Check if git is installed
if (-not (Get-Command "git" -ErrorAction SilentlyContinue)) {
    Write-Error "Git command not found. Please ensure Git is installed and in your PATH."
    exit 1
}

# Check if inside a Git repository
$gitStatus = git status --porcelain 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Error "Current directory is not a Git repository."
    exit 1
}

# Check if there are any changes (modified, added, deleted, untracked)
if (-not $gitStatus -or $gitStatus.Trim() -eq "") {
    Write-Host "No changes detected in working tree (working directory clean)." -ForegroundColor Yellow
    exit 0
}

# Check for CHANGELOG.md
$changelogPath = Join-Path $scriptDir "CHANGELOG.md"
if (-not (Test-Path $changelogPath)) {
    Write-Error "CHANGELOG.md not found at '$changelogPath'."
    exit 1
}

# Extract description from CHANGELOG.md
$changelogContent = Get-Content -Path $changelogPath -Raw -Encoding UTF8
$unreleasedPattern = "(?s)##\s*\[Unreleased\]\s*\r?\n(.*?)(?=\r?\n##\s*\[|\r?\n---\s*\r?\n|\Z)"

$extractedNotes = ""
if ($changelogContent -match $unreleasedPattern) {
    $extractedNotes = $Matches[1].Trim()
}

if ([string]::IsNullOrWhiteSpace($extractedNotes) -and [string]::IsNullOrWhiteSpace($Message)) {
    Write-Host "Warning: No entries found under '## [Unreleased]' in CHANGELOG.md." -ForegroundColor Yellow
    Write-Host "Please describe your changes under '## [Unreleased]' in CHANGELOG.md before committing." -ForegroundColor Yellow
    Write-Host ""
    $userInput = Read-Host "Enter a commit message directly (or press Enter to cancel)"
    if ([string]::IsNullOrWhiteSpace($userInput)) {
        Write-Host "Commit aborted." -ForegroundColor Red
        exit 1
    }
    $commitMessage = $userInput
} elseif (-not [string]::IsNullOrWhiteSpace($Message)) {
    $commitMessage = $Message
} else {
    # Extract first bullet as title if not provided
    $lines = $extractedNotes -split "\r?\n" | Where-Object { $_.Trim() -ne "" }
    $firstBullet = ($lines | Where-Object { $_.Trim().StartsWith("-") } | Select-Object -First 1)

    if (-not [string]::IsNullOrWhiteSpace($Title)) {
        $subject = $Title
    } elseif ($firstBullet) {
        $subject = $firstBullet.TrimStart("- ").Trim()
        # Strip markdown bold or backticks if at ends
        $subject = $subject -replace '^[`\*]+|[`\*]+$', ''
    } else {
        $subject = "Changelog updates and project improvements"
    }

    # Construct formatted commit message
    $commitMessage = "$subject`n`nChanges from CHANGELOG.md:`n$extractedNotes"
}

Write-Host "Changes detected to be committed:" -ForegroundColor Green
git status --short
Write-Host ""

Write-Host "Commit message:" -ForegroundColor Magenta
Write-Host "--------------------------------------------------" -ForegroundColor DarkGray
Write-Host $commitMessage -ForegroundColor White
Write-Host "--------------------------------------------------" -ForegroundColor DarkGray
Write-Host ""

# Stage all changes
Write-Host "Staging changes (git add .)..." -ForegroundColor Green
git add .
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to stage changes with git add."
    exit $LASTEXITCODE
}

# Execute commit with trailer
Write-Host "Committing changes..." -ForegroundColor Green
$commitArgs = @("commit", "-m", $commitMessage, "--trailer", "Co-authored-by: Junie <junie@jetbrains.com>")
& git @commitArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "Git commit failed."
    exit $LASTEXITCODE
}

Write-Host "Commit successful!" -ForegroundColor Green

# Push if requested / default
if (-not $NoPush) {
    $remote = git remote
    if ($remote) {
        Write-Host "Pushing to remote repository (git push)..." -ForegroundColor Cyan
        git push
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Changes successfully pushed to remote repository!" -ForegroundColor Green
        } else {
            Write-Host "Push failed. You can manually run 'git push'." -ForegroundColor Yellow
        }
    } else {
        Write-Host "No remote configured. Skipping git push." -ForegroundColor Yellow
    }
} else {
    Write-Host "Skipping git push (-NoPush specified)." -ForegroundColor Yellow
}

Write-Host "==================================================" -ForegroundColor Green
Write-Host "                  Done!                           " -ForegroundColor Green
Write-Host "==================================================" -ForegroundColor Green
