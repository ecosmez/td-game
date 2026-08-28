param(
	[string[]]$ExtraDslFiles = @()
)

$ErrorActionPreference = 'Stop'

function Test-SelectVisibleTargetDsl {
	param([string]$Path)

	$dsl = Get-Content -Raw $Path

	if ($dsl -match 'LineTraceByChannel') {
		throw "$Path : tower targeting must not reject in-range enemies because terrain blocks Visibility."
	}
	if ($dsl -match 'ForEachLoop') {
		throw "$Path : SelectVisibleTarget must not use leftover ForEachLoop for enemy location."
	}

	foreach ($required in @(
		'GetAllActorsOfClass',
		'GetRange',
		'GetActorLocation',
		'SetBestFireTarget',
		'SetHasFireTarget true'
	)) {
		if ($dsl -notmatch [regex]::Escape($required)) {
			throw "$Path : tower targeting is missing required behavior: $required"
		}
	}
}

function Test-TryFireDsl {
	param([string]$Path)

	$dsl = Get-Content -Raw $Path

	if ($dsl -match 'FlowControl\|ForLoop') {
		throw "$Path : TryFire must not use leftover ForLoop to pick a fire point or cooldown."
	}

	foreach ($required in @(
		'GetCanAttack',
		'GetAttackSpeed',
		'GetFirePoints',
		'GetFirePointCooldowns',
		'GetWorldLocation',
		'SelectVisibleTarget',
		'SpawnVolleyAt'
	)) {
		if ($dsl -notmatch [regex]::Escape($required)) {
			throw "$Path : TryFire is missing required behavior: $required"
		}
	}
}

$root = Join-Path $PSScriptRoot '..\Content\TD'
Test-SelectVisibleTargetDsl (Join-Path $root 'Tower_SelectVisibleTarget.dsl.txt')
Test-TryFireDsl (Join-Path $root 'Tower_TryFire.dsl.txt')

foreach ($extra in $ExtraDslFiles) {
	$name = Split-Path $extra -Leaf
	if ($name -match 'TryFire') {
		Test-TryFireDsl $extra
	}
	else {
		Test-SelectVisibleTargetDsl $extra
	}
}

Write-Output 'Tower targeting regression test passed.'
