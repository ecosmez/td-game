# Crowd-control traps (Slow / Root / Freeze / Stun)

## Goal

Add player-placeable traps that apply crowd-control effects to enemies: Slow,
Root, and Freeze/Stun. Traps are bought and placed through the same Tower
Store / `BP_BuildManager` flow as towers. Each effect can be placed as either
a permanent pulsing field (reapplies on an interval to whatever's inside) or
a one-shot snare (applies once to everyone currently inside, then the trap
is consumed).

## Existing infrastructure (confirmed in the live editor before designing)

- `BP_Enemy` already has `SlowFactor` (float, move-speed multiplier read by
  C++ `TDEnemyPathLibrary::AdvanceEnemyAlongPath` every tick) and
  `StunRemaining` (float countdown).
- `BP_Enemy.ApplyStun(Duration)`: refresh-if-longer
  (`if Duration > StunRemaining: StunRemaining = Duration`), then calls
  `UpdateStunVisual(0.0)`.
- `BP_Enemy.EventTick` already decays `StunRemaining` unconditionally every
  frame (`StunRemaining = Max(0, StunRemaining - DeltaSeconds)`), and calls
  `UpdateStunVisual` (toggles `StunVFXRoot` visibility + spins it while
  `StunRemaining > 0`).
- `BP_Enemy.ApplySlow(Factor)` is a **raw setter** with no duration or decay
  — `SlowFactor = Factor`, nothing else. Only consumer today is
  `BP_SlowZone` (the champion's E ability), which sets it on `BeginOverlap`
  and manually resets it to `1.0` on `EndOverlap`. Confirmed bug: if the zone
  is destroyed while an enemy is still overlapping, `EndOverlap` doesn't
  reliably fire and the enemy stays slowed forever. The new Slow trap must
  not inherit this.
- No Root-equivalent variable or function exists anywhere yet.
- `BP_RangedEnemy` and `BP_Boss` both have parent `BP_Enemy.BP_Enemy_C` —
  true child Blueprints, so anything added to `BP_Enemy` covers all three
  enemy types automatically.
- `TDEnemyPathLibrary::AdvanceEnemyAlongPath` (C++) gates attacking and
  movement on `StunRemaining`: if the enemy is already
  `bAttackingCrystal`/`bWallBlocked`, attack ticks are skipped while
  `StunRemaining > 0`; if the enemy is still traveling (neither flag set),
  `StunRemaining > 0` skips the movement-advance block entirely.
- The Tower Store (`UTowerStoreWidget::BuildDefaultCatalog`, C++) already has
  an unrelated card, currently mid-rename in the working tree from "Trap"
  (`BP_Tower_Trap`) to "Mine" (`BP_Tower_Mine`) — a burst-AoE damage tower
  (one-shot or pulse *damage* burst via `UpdateAoEBehavior`, see
  `Content/TD/Tower_UpdateAoEBehavior.dsl.txt`), unrelated to CC and left
  untouched by this feature. No mechanical collision, only a naming one;
  the new cards avoid the bare word "Trap" in their display names so they
  don't read as related to the Mine card either.

## Status-effect mechanics (on `BP_Enemy`)

- **Stun/Freeze**: no changes. Both effects call the existing
  `ApplyStun(Duration)`. Freeze is purely a different trap
  name/visual/duration tuning, not a different mechanic (confirmed with
  user: Freeze = Stun alias).
- **Slow**: add `SlowRemaining` (float) and a new function
  `ApplySlowTimed(Factor, Duration)`:
  - `SlowFactor = Factor`
  - `SlowRemaining = Max(SlowRemaining, Duration)` (refresh-if-longer, same
    rule as `ApplyStun`)
  - `EventTick` gets one more decay line: `SlowRemaining = Max(0,
    SlowRemaining - DeltaSeconds)`; when it reaches `0`, reset
    `SlowFactor = 1.0`.
  - This makes Slow self-healing by time instead of relying on
    `EndOverlap`, sidestepping the `BP_SlowZone` bug. The old raw
    `ApplySlow(Factor)` is untouched — `BP_SlowZone` keeps working exactly
    as today.
- **Root**: add `RootRemaining` (float) and `ApplyRoot(Duration)`, same
  refresh-if-longer rule, decayed in `EventTick` the same way as
  `SlowRemaining`.
  - Root blocks movement only — a rooted enemy that is already attacking
    (crystal or wall) keeps attacking. Root must **not** gate the
    `bAttackingCrystal`/`bWallBlocked` branches.
  - **C++ change** (`TDEnemyPathLibrary::AdvanceEnemyAlongPath`): the final
    movement-advance block's existing `if (StunRemaining > 0) return;` gate
    becomes `if (StunRemaining > 0 || RootRemaining > 0) return;`. The two
    earlier attack branches are untouched. Read `RootRemaining` via the
    existing `ReadFloatOr` reflection helper, matching how `StunRemaining`
    is already read.
- No VFX is required for Root in this pass (out of scope; can reuse the
  `StunVFXRoot` pattern later if desired).

## Trap actor architecture

- New Blueprint `BP_Trap_Base` (Actor, no C++ parent — matches how towers
  are pure Blueprint today, e.g. `BP_Tower_Arrow`). Components: root scene,
  a mesh/decal for the visual, and a `TriggerVolume` overlap component
  (box or sphere).
- Instance-editable config:
  - `EffectType` (enum: `Slow` / `Root` / `Stun`)
  - `ApplicationMode` (enum: `Pulse` / `OneShot`)
  - `Magnitude` (float — `SlowFactor` for Slow, ignored for Root/Stun)
  - `EffectDuration` (float — passed to `ApplySlowTimed`/`ApplyRoot`/
    `ApplyStun` on each application)
  - `PulseInterval` (float — Pulse mode only)
- **Pulse mode**: on placement, start a repeating timer at `PulseInterval`.
  Each firing, gather every actor currently overlapping `TriggerVolume`,
  cast to `BP_Enemy`, and call the matching `Apply*` function with
  `EffectDuration`. Because durations refresh-if-longer, an enemy standing
  inside just keeps getting refreshed for as long as it stays — no manual
  un-apply needed. Pulse traps never expire (permanent until sold/destroyed,
  per user decision — same lifetime model as towers).
- **OneShot mode**: on the trap's first `BeginOverlap`, apply the effect
  once to every actor currently overlapping the volume (not just the
  actor that triggered it — user decision: affects all enemies inside),
  then the trap destroys itself (consumed).
- 6 thin subclasses of `BP_Trap_Base`, each just setting different CDO
  defaults (no new logic per subclass):
  - `BP_Trap_Slow_Field` (Pulse) / `BP_Trap_Slow_Snare` (OneShot)
  - `BP_Trap_Root_Field` (Pulse) / `BP_Trap_Root_Snare` (OneShot)
  - `BP_Trap_Freeze_Field` (Pulse) / `BP_Trap_Freeze_Snare` (OneShot) —
    both use `EffectType = Stun` under the hood

## Store / BuildManager integration

- 6 new entries in `UTowerStoreWidget::BuildDefaultCatalog()` (C++), one per
  subclass, each with a distinct `SelectFunctionName` (`SelectSlowFieldTrap`,
  `SelectSlowSnareTrap`, `SelectRootFieldTrap`, `SelectRootSnareTrap`,
  `SelectFreezeFieldTrap`, `SelectFreezeSnareTrap`), its `TowerClassPath`,
  cost/build-time/stats, and an `ExtraNote` describing the effect (e.g.
  "Pulses slow • permanent" vs. "One-shot snare • consumed").
- 6 matching `SelectXTrap` functions added to `BP_BuildManager`, each
  mirroring the existing `SelectArrowTower`-style pattern (enters
  placement/ghost mode for that class) — the same mechanism already used
  for all 8 existing tower types, just pointed at the new trap classes.
- Display names use "Slow"/"Root"/"Freeze" rather than the bare word "Trap"
  (e.g. "Slow Field", "Slow Snare") to avoid confusion with the existing
  unrelated "Mine" card (burst-AoE damage, formerly named "Trap"). That
  card and its in-progress rename are untouched by this feature.
- Role tag: reuse `"Trap"` as the Role for all 6 new cards (the Mine card's
  Role is `"Mine"` after its rename, so there's no tag collision either).

## Build & verification plan

- Two C++ files change: `TDEnemyPathLibrary.cpp` (Root gating) and
  `TowerStoreWidget.cpp` (6 new catalog entries). Neither requires the new
  Blueprint variables/classes to exist yet to compile — reflection reads by
  name at runtime, and store entries reference soft class paths — so the
  C++ side can be built first, independently of the Blueprint work.
- Per project history ([[live-coding-crashes]]), this project's editor
  reliably crashes on a Live Coding patch after gameplay C++ changes. Build
  via a closed-editor `Build.bat` run instead of a Live Coding compile in
  the running editor.
- All Blueprint work (`BP_Enemy` new vars/functions/Tick decay, the 6
  `BP_Trap_*` actors, 6 new `BP_BuildManager` Select functions) happens
  afterward through Unreal MCP editor automation against the relaunched
  editor, following the patterns already in
  [[unreal-mcp-editor-automation]].
- Verification in PIE: place each of the 6 traps from the store and confirm:
  - Slow reduces enemy speed and self-heals back to normal after
    `EffectDuration` with no source object required to still exist.
  - A rooted enemy that is already attacking (crystal or wall) keeps
    attacking while stationary; a rooted enemy that is still traveling just
    stops.
  - Stun/Freeze fully locks an enemy (no movement, no attack) and its
    existing VFX toggles correctly.
  - Pulse ("Field") traps keep refreshing enemies that linger inside and
    never expire on their own.
  - OneShot ("Snare") traps apply once to everyone currently inside, then
    self-destroy.

## Out of scope for this pass

- Root/Slow-specific VFX (reuses no visual beyond the trap's own mesh;
  `StunVFXRoot` already covers Stun/Freeze).
- Effect stacking/priority rules beyond "refresh if longer" (no interaction
  between different effect types is defined here, e.g. Slow + Root
  simultaneously — they're independent variables so they simply compose
  additively as-is, which is acceptable for v1).
- Trap sell/refund UI, trap upgrade tiers.
- Flying/immune enemy types (not present in the project yet).
