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
	if ($dsl -notmatch 'IsShotBlockedByOtherTower') {
		throw "$Path : tower targeting must skip enemies whose shot line is blocked by another tower."
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

	if ($dsl -notmatch 'GetFireCooldown') {
		throw "$Path : TryFire must apply FireCooldown when the tower authored a fire delay."
	}
}

function Test-FireRateBeginPlayDsl {
	param([string]$Path)

	$dsl = Get-Content -Raw $Path

	if ($dsl -notmatch 'GetFireCooldown') {
		throw "$Path : BeginPlay must prefer authored FireCooldown over AttackSpeed when setting FireInterval."
	}
	if ($dsl -notmatch 'SetFireInterval \(Variables\|Combat\|GetFireCooldown\)') {
		throw "$Path : BeginPlay must copy FireCooldown into FireInterval when FireCooldown is greater than 0."
	}
}

function Test-UpdateAoEBehaviorDsl {
	param([string]$Path)

	$dsl = Get-Content -Raw $Path

	foreach ($required in @(
		'GetPulseMode',
		'GetPulseTimer',
		'SetPulseTimer',
		'FireAoEBurst',
		'GetPulseInterval'
	)) {
		if ($dsl -notmatch [regex]::Escape($required)) {
			throw "$Path : UpdateAoEBehavior is missing required behavior: $required"
		}
	}

	if ($dsl -match 'GetFireCooldown') {
		throw "$Path : UpdateAoEBehavior must keep pulse cadence on PulseInterval, not FireCooldown."
	}
	if ($dsl -notmatch 'SetPulseTimer \(Variables\|Default\|GetPulseInterval\)') {
		throw "$Path : PulseMode reset must use PulseInterval."
	}
}

$root = Join-Path $PSScriptRoot '..\Content\TD'
Test-SelectVisibleTargetDsl (Join-Path $root 'Tower_SelectVisibleTarget.dsl.txt')
Test-TryFireDsl (Join-Path $root 'Tower_TryFire.dsl.txt')
Test-FireRateBeginPlayDsl (Join-Path $root 'Tower_EventGraph.dsl.txt')
Test-UpdateAoEBehaviorDsl (Join-Path $root 'Tower_UpdateAoEBehavior.dsl.txt')

foreach ($extra in $ExtraDslFiles) {
	$name = Split-Path $extra -Leaf
	if ($name -match 'TryFire') {
		Test-TryFireDsl $extra
	}
	elseif ($name -match 'UpdateAoEBehavior') {
		Test-UpdateAoEBehaviorDsl $extra
	}
	elseif ($name -match 'EventGraph') {
		Test-FireRateBeginPlayDsl $extra
	}
	else {
		Test-SelectVisibleTargetDsl $extra
	}
}

Write-Output 'Tower targeting regression test passed.'
