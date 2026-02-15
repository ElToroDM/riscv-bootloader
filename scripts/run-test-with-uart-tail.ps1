param(
    [string]$LogFile = "uart_live.log",
    [switch]$LiveOnly,
    [switch]$NoTail,
    [switch]$DemoMode
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

function Write-Header {
    param([string]$Text)
    Write-Host ""
    Write-Host "════════════════════════════════════════════════════════════" -ForegroundColor DarkCyan
    Write-Host "  $Text" -ForegroundColor Cyan
    Write-Host "════════════════════════════════════════════════════════════" -ForegroundColor DarkCyan
}

function Write-Item {
    param(
        [string]$Label,
        [string]$Value,
        [string]$Color = "Gray"
    )
    Write-Host ("  {0,-18}: " -f $Label) -NoNewline -ForegroundColor DarkGray
    Write-Host $Value -ForegroundColor $Color
}

function Write-Step {
    param(
        [string]$Message,
        [string]$Color = "Cyan"
    )
    Write-Host "▶ $Message" -ForegroundColor $Color
}

function ConvertTo-EscapedSingleQuote {
    param([string]$Text)
    return $Text.Replace("'", "''")
}

$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

$logPath = Join-Path $repoRoot $LogFile
$displayLogPath = Resolve-Path -LiteralPath $repoRoot | ForEach-Object { Join-Path $_.Path $LogFile }

Write-Header "RISC-V Bootloader Live Validation"
Write-Item "Repository" $repoRoot "White"
Write-Item "UART mirror" $displayLogPath "White"
Write-Item "Tail window" ($(if ($NoTail) { "disabled" } else { "enabled" })) "White"
Write-Item "Live-only log" ($(if ($LiveOnly) { "enabled" } else { "disabled" })) "White"
Write-Item "Demo mode" ($(if ($DemoMode) { "enabled" } else { "disabled" })) "White"

if (Test-Path $logPath) {
    Remove-Item $logPath -Force
}

$escapedRepoRoot = ConvertTo-EscapedSingleQuote $repoRoot
$escapedLogPath = ConvertTo-EscapedSingleQuote $logPath

$tailProcess = $null
if (-not $NoTail) {
    Write-Step "Starting UART tail window"

    $tailCommand = @(
        "Set-Location '$escapedRepoRoot'",
        "`$Host.UI.RawUI.WindowTitle = 'UART Live Feed'",
        "try { `$PSStyle.OutputRendering = 'Ansi' } catch {}",
        "if (-not (Test-Path '$escapedLogPath')) { New-Item -ItemType File -Path '$escapedLogPath' | Out-Null }",
        "Clear-Host",
        "Write-Host 'UART live stream - waiting for data...' -ForegroundColor Cyan",
        "Write-Host 'Press Ctrl+C in this window to stop tailing.' -ForegroundColor DarkGray",
        "Write-Host ''",
        "Get-Content '$escapedLogPath' -Wait"
    ) -join "; "

    $tailProcess = Start-Process -FilePath "pwsh" -ArgumentList @(
        "-NoExit",
        "-Command",
        $tailCommand
    ) -PassThru
}

$pythonLauncher = $null
if (Get-Command py -ErrorAction SilentlyContinue) {
    $pythonLauncher = "py"
} elseif (Get-Command python -ErrorAction SilentlyContinue) {
    $pythonLauncher = "python"
} else {
    throw "Neither 'py' nor 'python' was found in PATH."
}

$testArgs = @(".\\test_validator.py", "--uart-mirror-file", ".\\$LogFile")
if ($LiveOnly) {
    $testArgs += "--uart-live-only"
}
if ($DemoMode) {
    $testArgs += @("--demo-step-delay", "0.9", "--demo-byte-delay", "0.0010")
}

$testExitCode = 0
$startTime = Get-Date

Write-Step "Running protocol validation"
Write-Item "Command" "$pythonLauncher $($testArgs -join ' ')" "DarkGray"

if ($DemoMode) {
    Write-Host ""
    Write-Host "Demo capture tips:" -ForegroundColor Cyan
    Write-Host "  1) Keep this window on the left and UART tail on the right." -ForegroundColor DarkGray
    Write-Host "  2) Use a dark terminal theme with 120x36 layout." -ForegroundColor DarkGray
    Write-Host "  3) Record 20-30s around upload + CRC + reboot." -ForegroundColor DarkGray
    Write-Host ""
}

try {
    & $pythonLauncher @testArgs
    $testExitCode = $LASTEXITCODE
}
finally {
    if ($LiveOnly -and $NoTail) {
        for ($i = 0; $i -lt 10; $i++) {
            if (-not (Test-Path $logPath)) {
                break
            }

            try {
                Remove-Item $logPath -Force -ErrorAction Stop
                break
            }
            catch {
                Start-Sleep -Milliseconds 100
            }
        }
    }
}

$elapsed = (Get-Date) - $startTime
Write-Header "Execution Summary"
Write-Item "Exit code" $testExitCode ($(if ($testExitCode -eq 0) { "Green" } else { "Red" }))
Write-Item "Duration" ("{0:mm\:ss}" -f $elapsed) "White"
Write-Item "Tail process" ($(if ($tailProcess -and -not $tailProcess.HasExited) { "running" } elseif ($NoTail) { "not started" } else { "closed" })) "White"

if ($testExitCode -eq 0) {
    Write-Host ""
    Write-Host "✓ Live validation completed successfully." -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "✗ Validation failed. Review the output above." -ForegroundColor Red
}

exit $testExitCode
