Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Invoke-CheckedCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Command,

        [Parameter()]
        [string[]]$Arguments = @()
    )

    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $Command $($Arguments -join ' ')"
    }
}

Write-Host "Checking for Python installation..."

$PythonCommand = $null
$PythonPrefixArguments = @()

if (Get-Command python -ErrorAction SilentlyContinue) {
    $PythonCommand = (Get-Command python).Source
}
elseif (Get-Command py -ErrorAction SilentlyContinue) {
    $PythonCommand = (Get-Command py).Source
    $PythonPrefixArguments = @("-3")
}
elseif (Get-Command python3 -ErrorAction SilentlyContinue) {
    $PythonCommand = (Get-Command python3).Source
}
else {
    Write-Error "Python does not appear to be installed or available on PATH. Please install Python 3.12 or higher and run this installer again."
}

# overseer_model uses f-string syntax introduced in Python 3.12 (PEP 701)
& $PythonCommand @PythonPrefixArguments -c "import sys; sys.exit(0 if sys.version_info >= (3, 12) else 1)"
if ($LASTEXITCODE -ne 0) {
    $FoundVersion = & $PythonCommand @PythonPrefixArguments --version
    Write-Error "Found $FoundVersion at $PythonCommand, but Python 3.12 or higher is required. Please install it and run this installer again."
}

Write-Host "Checking for make..."

if (Get-Command make -ErrorAction SilentlyContinue) {
    $MakeCommand = (Get-Command make).Source
}
elseif (Get-Command mingw32-make -ErrorAction SilentlyContinue) {
    $MakeCommand = (Get-Command mingw32-make).Source
}
else {
    Write-Error "GNU make does not appear to be installed or available on PATH. Please install make and a compatible C++ compiler, then run this installer again."
}

Write-Host "All checks passed. Proceeding to installation."

$ScriptDir = $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($ScriptDir)) {
    $ScriptDir = (Get-Location).Path
}
$ScriptDir = (Resolve-Path $ScriptDir).Path
Set-Location $ScriptDir

$VenvDir = Join-Path $ScriptDir "venv"
$VenvPython = Join-Path $VenvDir "Scripts\python.exe"
$OverseerDir = Join-Path $ScriptDir "Overseer"

Write-Host "Creating virtual environment..."
Invoke-CheckedCommand -Command $PythonCommand -Arguments ($PythonPrefixArguments + @("-m", "venv", $VenvDir))

if (-not (Test-Path $VenvPython -PathType Leaf)) {
    throw "The virtual environment was created, but its Python executable was not found at: $VenvPython"
}

Write-Host "Installing Overseer..."
Invoke-CheckedCommand -Command $VenvPython -Arguments @("-m", "pip", "install", "-e", $OverseerDir)
Write-Host "Successfully installed Overseer!"
Write-Host

Write-Host "Building simulation binary..."
Invoke-CheckedCommand -Command $MakeCommand
Write-Host "Successfully created binary!"
Write-Host

Write-Host "Configuring files..."
$UserConfigDir = (& $VenvPython -c "from platformdirs import user_config_dir; print(user_config_dir('Overseer', False, roaming=True))").Trim()
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($UserConfigDir)) {
    throw "Could not determine the Overseer configuration directory."
}

$UserConfigFile = Join-Path $UserConfigDir "config.yml"
$UserModelsDir = $ScriptDir
$OverseerModelDir = Join-Path $UserModelsDir "overseer_model"
$UserLogsDir = Join-Path $ScriptDir "logs"

New-Item -ItemType Directory -Path $UserConfigDir -Force | Out-Null
New-Item -ItemType Directory -Path $UserLogsDir -Force | Out-Null

$WriteConfig = $true
if (Test-Path $UserConfigFile) {
    Write-Host
    Write-Host "An Overseer configuration file already exists:"
    Write-Host "   $UserConfigFile"
    Write-Host "If we replaced it, you wouldn't lose much, but just in case..."

    $Response = Read-Host "Replace it with the Labor-Time-Economy configuration? [y/N]"
    if ($Response -notmatch '^(y|yes)$') {
        Write-Host "Skipping configuration step."
        $WriteConfig = $false
    }
}

if ($WriteConfig) {
    # Forward slashes avoid YAML escape-sequence problems in quoted Windows paths.
    $YamlScriptDir = $ScriptDir.Replace('\', '/')
    $YamlModelsDir = $UserModelsDir.Replace('\', '/')
    $YamlLogsDir = $UserLogsDir.Replace('\', '/')

    $ConfigYaml = @"
global_settings:
    default_save_dir: "$YamlScriptDir"
    user_models_dir: "$YamlModelsDir"
    user_logs_dir: "$YamlLogsDir"
    run_on_startup: true
"@

    Set-Content -Path $UserConfigFile -Value $ConfigYaml -Encoding utf8
}

$SimPath = Join-Path $ScriptDir "bin\sim.exe"
if (-not (Test-Path $SimPath -PathType Leaf)) {
    $AlternateSimPath = Join-Path $ScriptDir "bin\sim"
    if (Test-Path $AlternateSimPath -PathType Leaf) {
        $SimPath = $AlternateSimPath
    }
    else {
        throw "The simulation binary was not found at '$SimPath' or '$AlternateSimPath'."
    }
}

$EnvironmentVariableNames = @("BC_DEMO_FILE", "SIM_PATH", "MODEL_PATH")
$PreviousEnvironment = @{}
foreach ($Name in $EnvironmentVariableNames) {
    $PreviousEnvironment[$Name] = [Environment]::GetEnvironmentVariable($Name, "Process")
}

try {
    $env:BC_DEMO_FILE = Join-Path $OverseerModelDir "bc_lte_demo.yml"
    $env:SIM_PATH = $SimPath
    $env:MODEL_PATH = $OverseerModelDir

    Invoke-CheckedCommand -Command $VenvPython -Arguments @(
        (Join-Path $OverseerModelDir "prepare_demo_and_params.py")
    )
}
finally {
    foreach ($Name in $EnvironmentVariableNames) {
        [Environment]::SetEnvironmentVariable(
            $Name,
            $PreviousEnvironment[$Name],
            "Process"
        )
    }
}

$LauncherFile = Join-Path $ScriptDir "run.ps1"
$LauncherContents = @'
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$ScriptDir = $PSScriptRoot
Set-Location $ScriptDir

$VenvPython = Join-Path $ScriptDir "venv\Scripts\python.exe"
& $VenvPython -m overseer
exit $LASTEXITCODE
'@

Set-Content -Path $LauncherFile -Value $LauncherContents -Encoding utf8

Write-Host
Write-Host "Setup complete."
Write-Host "To launch the application, run:"
Write-Host
Write-Host "    .\run.ps1"
Write-Host
Write-Host "Additional details which might be relevant to you:"

$AdditionalDetails = @(
    "If you want the rest of the models that come packaged with Overseer,"
    "go into the settings and change your 'User models directory' to a"
    "directory where you want your user data to go. Make sure that the"
    "exact folder does not exist yet. The models will be created there"
    "the next time that you launch Overseer. You can then move the"
    "overseer_model directory there and have everything in one place."
) -join " "

Write-Host $AdditionalDetails
