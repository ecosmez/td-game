$ErrorActionPreference = 'Stop'

$sourcePath = Join-Path $PSScriptRoot '..\Source\TD\WorldFogOfWarComponent.cpp'
$source = Get-Content -Raw $sourcePath

if ($source -notmatch '#if WITH_EDITOR\s+HostActor->SetActorLabel\(TEXT\("WorldFOW_Host"\)\);\s+#endif') {
    throw 'Editor-only SetActorLabel must be excluded from standalone game builds.'
}

Write-Host 'Runtime build guard regression checks passed.'
