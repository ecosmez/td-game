function Test-RequiresEditorBuild {
    param([string[]] $ChangedFiles)

    foreach ($file in $ChangedFiles) {
        $normalized = $file.Replace('\', '/')
        if (
            $normalized -match '(^|/)Source/' -or
            $normalized -match '\.uproject$' -or
            $normalized -match '\.uplugin$' -or
            $normalized -match '\.(Build|Target)\.cs$'
        ) {
            return $true
        }
    }

    return $false
}

function Find-GitExecutable {
    $command = Get-Command git -ErrorAction SilentlyContinue
    if ($null -eq $command) {
        return $null
    }

    return $command.Source
}

function Test-IsStandardGitLfsPostMerge {
    param([AllowEmptyString()] [string] $Content)

    $hasPostMerge = $false
    foreach ($line in ($Content -split "`r?`n")) {
        $trimmed = $line.Trim()
        if (-not $trimmed -or $trimmed.StartsWith('#')) {
            continue
        }
        if ($trimmed -match '^command -v git-lfs .*This repository is configured for Git LFS.*$') {
            continue
        }
        if ($trimmed -match '^git lfs post-merge "\$@"$') {
            $hasPostMerge = $true
            continue
        }
        return $false
    }

    return $hasPostMerge
}

function Find-UnrealEngineRoot {
    param([Parameter(Mandatory)] [string] $ProjectFile)

    if ($env:TD_UNREAL_ENGINE_ROOT) {
        return $env:TD_UNREAL_ENGINE_ROOT
    }

    $project = Get-Content -Raw -LiteralPath $ProjectFile | ConvertFrom-Json
    $association = [string] $project.EngineAssociation
    if ([string]::IsNullOrWhiteSpace($association)) {
        return $null
    }

    $customBuildsKey = 'HKCU:\Software\Epic Games\Unreal Engine\Builds'
    try {
        $customRoot = Get-ItemPropertyValue -Path $customBuildsKey -Name $association -ErrorAction Stop
        if ($customRoot) {
            return [string] $customRoot
        }
    }
    catch {
        # The project may use an Epic Launcher version instead of a registered custom build.
    }

    $launcherKeys = @(
        "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$association",
        "HKLM:\SOFTWARE\WOW6432Node\EpicGames\Unreal Engine\$association"
    )
    foreach ($key in $launcherKeys) {
        try {
            $launcherRoot = Get-ItemPropertyValue -Path $key -Name InstalledDirectory -ErrorAction Stop
            if ($launcherRoot) {
                return [string] $launcherRoot
            }
        }
        catch {
            continue
        }
    }

    return $null
}
