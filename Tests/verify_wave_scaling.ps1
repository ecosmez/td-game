$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$waveDsl = Get-Content -Raw (Join-Path $projectRoot 'Content/TD/AnnounceWave.dsl.txt')
$waveCpp = Get-Content -Raw (Join-Path $projectRoot 'Source/TD/TDEnemyPathLibrary.cpp')
$beginPlayDsl = Get-Content -Raw (Join-Path $projectRoot 'Content/TD/spawnerbeginplay.dsl.txt')

$expectedDsl = 'SetNormalEnemiesThisWave (+ 12 (* (Variables|Default|GetWaveNumber) 8))'
$expectedCppPerSpawn = 'return 12 + SafeWave * 8;'
$expectedCppInterval = 'FMath::Max(0.30f, 0.55f - static_cast<float>(SafeWave - 1) * 0.05f)'
$expectedBeginPlayInterval = 'SetSpawnInterval 0.55'

if (-not $waveDsl.Contains($expectedDsl)) {
    throw 'Blueprint wave scaling must be 12 + WaveNumber * 8 per spawn.'
}

if (-not $waveCpp.Contains($expectedCppPerSpawn)) {
    throw 'C++ per-spawn scaling must be 12 + WaveNumber * 8.'
}

if (-not $waveCpp.Contains($expectedCppInterval)) {
    throw 'C++ spawn interval must start at 0.55s and floor at 0.30s.'
}

if ($waveCpp -match 'return Slot.RouteId == PrimaryRouteId') {
    throw 'Later waves must keep the primary spawn instead of excluding it.'
}

if (-not $beginPlayDsl.Contains($expectedBeginPlayInterval)) {
    throw 'Spawner begin play must default SpawnInterval to 0.55.'
}

Write-Output 'Wave scaling is consistent across Blueprint and C++ paths.'
