$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $repoRoot 'Scripts/GitHooks/AfterGitPull.Core.ps1')

$failures = [System.Collections.Generic.List[string]]::new()

function Assert-Equal {
    param(
        [AllowNull()] $Expected,
        [AllowNull()] $Actual,
        [Parameter(Mandatory)] [string] $Name
    )

    if ($Expected -ne $Actual) {
        $failures.Add("$Name`: expected '$Expected', got '$Actual'")
    }
}

Assert-Equal $true (Test-RequiresEditorBuild @('Source/TD/TDEnemy.cpp')) 'C++ source triggers build'
Assert-Equal $true (Test-RequiresEditorBuild @('Source/TD/TD.Build.cs')) 'Build.cs triggers build'
Assert-Equal $true (Test-RequiresEditorBuild @('TD.uproject')) 'uproject triggers build'
Assert-Equal $true (Test-RequiresEditorBuild @('Plugins/MyPlugin/Source/MyPlugin.cpp')) 'plugin source triggers build'
Assert-Equal $false (Test-RequiresEditorBuild @('Content/TD/BP_Tower.uasset')) 'Blueprint alone skips build'
Assert-Equal $false (Test-RequiresEditorBuild @('README.md', 'Config/DefaultGame.ini')) 'unrelated files skip build'
Assert-Equal $true (Test-RequiresEditorBuild @('Content/A.uasset', 'Source/TD/NewClass.h')) 'mixed changes trigger build'
Assert-Equal $false (Test-RequiresEditorBuild @()) 'empty change list skips build'
Assert-Equal $true (Test-IsStandardGitLfsPostMerge "#!/bin/sh`ngit lfs post-merge `"`$@`"") 'standard LFS hook may be migrated'
Assert-Equal $false (Test-IsStandardGitLfsPostMerge "#!/bin/sh`necho custom") 'personal hook is preserved'

$originalPath = $env:PATH
try {
    $env:PATH = ''
    Assert-Equal $null (Find-GitExecutable) 'missing git is reported'
}
finally {
    $env:PATH = $originalPath
}

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Host 'Git pull automation tests passed.'
