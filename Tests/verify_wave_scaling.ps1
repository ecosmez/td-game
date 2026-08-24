$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$waveDsl = Get-Content -Raw (Join-Path $projectRoot 'Content/TD/AnnounceWave.dsl.txt')
$waveCpp = Get-Content -Raw (Join-Path $projectRoot 'Source/TD/TDEnemyPathLibrary.cpp')

$expectedDsl = 'SetNormalEnemiesThisWave (+ 12 (* (Variables|Default|GetWaveNumber) 6))'
$expectedCpp = 'EnemyCount = 12 + WaveNumber * 6;'

if (-not $waveDsl.Contains($expectedDsl)) {
    throw 'Blueprint wave scaling must be 12 + WaveNumber * 6.'
}

if (-not $waveCpp.Contains($expectedCpp)) {
    throw 'C++ fallback wave scaling must match the Blueprint formula: 12 + WaveNumber * 6.'
}

Write-Output 'Wave scaling is consistent across Blueprint and C++ fallback paths.'
