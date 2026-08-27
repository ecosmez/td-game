[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $scriptRoot 'GitHooks/AfterGitPull.Core.ps1')
$repoRoot = (& git rev-parse --show-toplevel 2>$null)
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($repoRoot)) {
    Write-Error 'Ejecuta este script dentro del repositorio TD.'
    exit 1
}
$repoRoot = $repoRoot.Trim()

$existingHooksPath = (& git config --local --get core.hooksPath 2>$null)
if ($LASTEXITCODE -eq 0 -and $existingHooksPath -and $existingHooksPath.Trim() -ne '.githooks') {
    Write-Error "Este repositorio ya usa core.hooksPath='$($existingHooksPath.Trim())'. No se reemplazó."
    exit 1
}

$gitDir = (& git rev-parse --git-dir).Trim()
if (-not [System.IO.Path]::IsPathRooted($gitDir)) {
    $gitDir = Join-Path $repoRoot $gitDir
}
$personalPostMerge = Join-Path $gitDir 'hooks/post-merge'
if (-not $existingHooksPath -and (Test-Path -LiteralPath $personalPostMerge)) {
    $existingHookContent = Get-Content -Raw -LiteralPath $personalPostMerge
    if (-not (Test-IsStandardGitLfsPostMerge $existingHookContent)) {
        Write-Error "Ya existe un hook personal en '$personalPostMerge'. No se reemplazó."
        exit 1
    }
}

& git config --local core.hooksPath .githooks
if ($LASTEXITCODE -ne 0) {
    Write-Error 'No se pudo configurar core.hooksPath.'
    exit 1
}

Write-Host 'Hooks instalados. Los próximos git pull ejecutarán LFS y compilarán C++ cuando sea necesario.'
