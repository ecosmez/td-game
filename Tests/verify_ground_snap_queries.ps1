$ErrorActionPreference = 'Stop'

$enemyPath = Join-Path $PSScriptRoot '..\Source\TD\TDEnemyPathLibrary.cpp'
$playerPath = Join-Path $PSScriptRoot '..\Source\TD\MobaPlayerController.cpp'

$enemySource = Get-Content -Raw $enemyPath
$playerSource = Get-Content -Raw $playerPath

if ($enemySource -notmatch 'LineTraceMultiByObjectType') {
    throw 'Enemy ground snapping must query WorldStatic geometry by object type.'
}

if ($enemySource -notmatch 'ECC_WorldStatic') {
    throw 'Enemy ground object queries must include ECC_WorldStatic.'
}

if ($enemySource -notmatch 'Location\s*=\s*SnapToGround\(World,\s*Location,\s*GroundOffset,\s*Enemy,\s*PrevLoc\.Z') {
    throw 'Each enemy movement step must be snapped to walkable terrain after steering.'
}

if ($playerSource -match 'SnapGroundedChampionToTerrain\(\)' -or
    $playerSource -match 'TDChampionGroundSnap') {
    throw 'Grounded champions must rely on CharacterMovement floor following, not a per-frame vertical trace/teleport.'
}

Write-Host 'Ground snapping object-query regression checks passed.'
