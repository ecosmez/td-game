$ErrorActionPreference = 'Stop'

$enemyPath = Join-Path $PSScriptRoot '..\Source\TD\TDEnemyPathLibrary.cpp'
$playerPath = Join-Path $PSScriptRoot '..\Source\TD\MobaPlayerController.cpp'

$enemySource = Get-Content -Raw $enemyPath
$playerSource = Get-Content -Raw $playerPath

if ($enemySource -notmatch 'LineTraceMultiByObjectType') {
    throw 'Enemy ground snapping must query WorldStatic geometry by object type.'
}

if ($playerSource -notmatch 'LineTraceSingleByObjectType') {
    throw 'Player landing snapping must query WorldStatic geometry by object type.'
}

if ($enemySource -notmatch 'ECC_WorldStatic' -or $playerSource -notmatch 'ECC_WorldStatic') {
    throw 'Ground object queries must include ECC_WorldStatic.'
}

Write-Host 'Ground snapping object-query regression checks passed.'
