[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $scriptRoot 'AfterGitPull.Core.ps1')

$repoRoot = (& git rev-parse --show-toplevel 2>$null)
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($repoRoot)) {
    Write-Error 'No se pudo localizar la raíz del repositorio.'
    exit 1
}
$repoRoot = $repoRoot.Trim()

Write-Host '[post-pull] Sincronizando archivos de Git LFS...'
& git lfs pull
if ($LASTEXITCODE -ne 0) {
    Write-Error 'git lfs pull falló. Revisa Git LFS y la conexión antes de abrir Unreal.'
    exit 1
}

$changedFiles = @(& git diff --name-only ORIG_HEAD HEAD 2>$null)
if ($LASTEXITCODE -ne 0) {
    Write-Warning '[post-pull] No fue posible comparar ORIG_HEAD; se omite la compilación automática.'
    exit 0
}

if (-not (Test-RequiresEditorBuild $changedFiles)) {
    Write-Host '[post-pull] No cambió código de Unreal; no es necesario recompilar.'
    exit 0
}

$runningEditor = Get-Process -Name UnrealEditor, UE4Editor -ErrorAction SilentlyContinue
if ($runningEditor) {
    Write-Warning '[post-pull] Cambió código C++, pero Unreal está abierto. Ciérralo y ejecuta Scripts/GitHooks/BuildEditor.ps1.'
    exit 0
}

& (Join-Path $scriptRoot 'BuildEditor.ps1')
exit $LASTEXITCODE
