[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent (Split-Path -Parent $scriptRoot)
$projectFile = Join-Path $repoRoot 'TD.uproject'
. (Join-Path $scriptRoot 'AfterGitPull.Core.ps1')

$runningEditor = Get-Process -Name UnrealEditor, UE4Editor -ErrorAction SilentlyContinue
if ($runningEditor) {
    Write-Error 'Cierra Unreal Editor antes de compilar TD Editor.'
    exit 1
}

$engineRoot = Find-UnrealEngineRoot -ProjectFile $projectFile
if ([string]::IsNullOrWhiteSpace($engineRoot)) {
    Write-Error 'No se encontró la instalación de Unreal. Define TD_UNREAL_ENGINE_ROOT con la carpeta del Engine.'
    exit 1
}

$buildScript = Join-Path $engineRoot 'Engine/Build/BatchFiles/Build.bat'
if (-not (Test-Path -LiteralPath $buildScript)) {
    Write-Error "No existe Build.bat en: $buildScript"
    exit 1
}

Write-Host '[post-pull] Compilando TDEditor Win64 Development...'
& $buildScript TDEditor Win64 Development $projectFile -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) {
    Write-Error "La compilación de TDEditor falló con código $LASTEXITCODE."
    exit $LASTEXITCODE
}

Write-Host '[post-pull] Compilación terminada.'
