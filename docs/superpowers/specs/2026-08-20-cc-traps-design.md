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

**Revised during planning**: the Unreal MCP tooling available in this project
has no tool to create a new Blueprint *enum* asset, and editing per-subclass
CDO defaults is unconfirmed/risky. The codebase already has a proven
alternative for "one class, many configurations" — `BP_BuildManager`'s
`SpawnAndStartTower` (see `Content/TD/Pad_SpawnAndStartTower.dsl.txt`)
dispatches by **string name** and configures each freshly-spawned actor with
direct instance-property writes, the same way C++
`TDEnemyPathLibrary::SpawnNextWaveEnemy` configures a freshly-spawned enemy.
Traps follow that same pattern instead of using enums or subclasses:

- One Blueprint `BP_Trap_Base` (Actor, no C++ parent — matches how towers
  are pure Blueprint today, e.g. `BP_Tower_Arrow`). Components: default
  scene root and a mesh for the visual. No collision/overlap component —
  see detection note below.
- **Detection**: instead of a physical overlap volume, mirrors the Mine
  tower's own proven `HasEnemyInRange`/`FireAoEBurst` pattern exactly
  (`Content/TD/Tower_UpdateAoEBehavior.dsl.txt`): a `Radius` float
  variable, `Actor|GetAllActorsOfClass` on `BP_Enemy`, and a distance
  check against the trap's own location. Functionally equivalent to "who
  overlaps the trigger volume" without needing new collision-component
  setup — same query shape already compiling and running in this project.
- Instance variables (all configured post-spawn by `SpawnAndStartTower`,
  not baked into subclasses):
  - `EffectType` (String: `"Slow"` / `"Root"` / `"Stun"`) — dispatched with
    a DSL `switch string` in the trap's own pulse/trigger logic.
  - `bPulseMode` (Bool) — mirrors the Mine tower's existing `PulseMode`
    flag naming exactly: `true` = permanent pulsing field, `false` =
    one-shot snare.
  - `Magnitude` (float — `SlowFactor` for Slow, ignored for Root/Stun)
  - `EffectDuration` (float — passed to `ApplySlowTimed`/`ApplyRoot`/
    `ApplyStun` on each application)
  - `PulseInterval` (float — used only when `bPulseMode` is true)
  - `Radius` (float — detection radius, see above)
- **Pulse mode** (`bPulseMode = true`): `EventTick` counts `PulseInterval`
  down every frame (mirrors Mine's `PulseTimer` countdown exactly); when it
  reaches zero, apply the effect to every `BP_Enemy` within `Radius` and
  reset the countdown to `PulseInterval`. Because durations refresh-if-
  longer, an enemy standing inside just keeps getting refreshed for as
  long as it stays — no manual un-apply needed. Pulse traps never expire
  (permanent until sold/destroyed, per user decision — same lifetime model
  as towers).
- **OneShot mode** (`bPulseMode = false`): `EventTick` checks each frame
  whether any `BP_Enemy` is within `Radius`; the first frame one is, apply
  the effect to every enemy within `Radius` and destroy the trap in the
  same branch (both statements execute together, exactly mirroring Mine's
  proven one-shot branch) — consumed after one application.
- 6 store cards spawn the **same** `BP_Trap_Base` class with 6 different
  `(EffectType, bPulseMode)` configurations — no per-effect subclasses:
  - Slow Field (`"Slow"`, pulse) / Slow Snare (`"Slow"`, one-shot)
  - Root Field (`"Root"`, pulse) / Root Snare (`"Root"`, one-shot)
  - Freeze Field (`"Stun"`, pulse) / Freeze Snare (`"Stun"`, one-shot)

## Store / BuildManager integration

- 6 new entries in `UTowerStoreWidget::BuildDefaultCatalog()` (C++), all
  sharing the same `TowerClassPath` (`BP_Trap_Base`) but each with a
  distinct `SelectFunctionName` (`SelectSlowFieldTrap`,
  `SelectSlowSnareTrap`, `SelectRootFieldTrap`, `SelectRootSnareTrap`,
  `SelectFreezeFieldTrap`, `SelectFreezeSnareTrap`), display name,
  cost/build-time/stats, and an `ExtraNote` describing the effect (e.g.
  "Pulses slow • permanent" vs. "One-shot snare • consumed"). Each entry
  may use its own `MeshPath`/material for a visually distinct store
  preview even though the underlying class is shared.
- 6 matching `SelectXTrap` functions added to `BP_BuildManager`, each
  mirroring the existing `SelectArrowTower`-style pattern (sets the
  selected-tower name, enters placement/ghost mode) — the same mechanism
  already used for all 8 existing tower types.
- 6 new `elif` branches added to the `SpawnAndStartTower` DSL function
  (keyed on the same selected-tower-name string set by the `SelectXTrap`
  functions above), each spawning `BP_Trap_Base` and then writing its
  `EffectType`/`bPulseMode`/`Magnitude`/`EffectDuration`/`PulseInterval`
  instance variables to the values for that trap, before
  `StartConstruction` — mirrors every existing branch in that function.
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
- All Blueprint work (`BP_Enemy` new vars/functions/Tick decay, the single
  `BP_Trap_Base` actor, 6 new `BP_BuildManager` Select functions, 6 new
  `SpawnAndStartTower` branches) happens afterward through Unreal MCP
  editor automation against the relaunched editor, following the patterns
  already in [[unreal-mcp-editor-automation]]. Blueprint function/event
  bodies are written via the `BlueprintTools.write_graph_dsl` tool — for
  any graph that already has logic (e.g. `BP_Enemy.EventTick`,
  `SpawnAndStartTower`), the workflow is read the full existing DSL via
  `read_graph_dsl`, edit the complete text, and write the whole script
  back (it replaces, not appends).
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
