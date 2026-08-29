# Capture bases

## Goal

Add designer-placed outposts around the map. The champion channels the site, then builds on 3 starter hex pads to fully capture it. A fully captured base grants crystal-style vision and reveals its extra pads. Enemies un-capture it by destroying those 3 starter towers: extra towers shut down until the champion channels again and rebuilds all 3.

Main-crystal pads and the existing hex-pad click/build path stay as they are. This feature only owns outpost bases.

## Existing infrastructure

- `BP_HexPad` / `BP_TowerPad`: click-to-place via `TryPlaceTower` and `BP_BuildManager`. Pads are not destroyed on place; a tower is spawned at the pad transform.
- Live fog: `UMapDiscoveryComponent::RegisterVisionSource(Actor, Radius)` plus `FTDFogVision`. The main crystal is already a live source at `CrystalVisionRadius` (8000 cm). There is no per-actor unregister yet — only `ClearVisionSources()`, which would also drop the crystal. This feature adds `UnregisterVisionSource(Actor)`.
- Towers (`BP_Tower` and children) already have combat flags such as `CanAttack` / `IsBuilt`. Extra-tower shutdown uses a powered flag the base sets; unpowered towers do not fire.
- Enemies are `BP_Enemy` and its children (`BP_RangedEnemy`, `BP_Boss`). Champion is the pawn from `AMobaPlayerController::GetControlledChampion()`.

## Capture state

Derived from two integers and two sticky flags. No overlapping modes.

| Input | Meaning |
| --- | --- |
| `StarterTowersAlive` | 0–3 living towers currently occupying the 3 starter pads |
| `ChannelProgress` | 0–1, paused (not reset) when fill conditions fail |
| `bExtraPadsUnlocked` | Sticky. Set true the first time `StarterTowersAlive` reaches 3. Never cleared. |
| `bHeld` | Set true when `StarterTowersAlive` reaches 3. Set false when it reaches 0. |

**Fill channel** when the champion is inside `CaptureRadius` and no living enemy is inside that same radius. Otherwise progress pauses where it is (champion leaving, or any enemy in the zone). Completing the channel (`ChannelProgress >= 1`) is required whenever `StarterTowersAlive == 0` before starter pads can be built on. Completing a channel with starters already alive is a no-op.

| Condition | Starter pads | Extra pads | Vision | Extra towers |
| --- | --- | --- | --- | --- |
| `StarterTowersAlive == 0` and channel not complete | Hidden, not buildable | Hidden until unlocked; after unlock they stay visible but not newly buildable | Off | Unpowered |
| Channel complete and `StarterTowersAlive < 3` and never held this life (`!bHeld`) | Empty ones visible and buildable | Hidden if not yet unlocked | Off | Unpowered |
| `bHeld && StarterTowersAlive >= 1` (includes all 3 up, and 1–2 remaining after a full capture) | Occupied ones hidden | Visible (unlock on first time reaching 3) | On | Powered |
| After `bHeld` drops because starters hit 0 | Hidden until a new channel completes | Stay visible | Off | Unpowered |

Reaching 3 starters in one life is **full capture**: set `bHeld`, set `bExtraPadsUnlocked`, register vision, show extra pads, power extra towers.

Destroying the last of the 3 starters is **loss**: clear `bHeld`, unregister vision, unpower extra towers, hide starter pads, reset `ChannelProgress` to 0. Extra pads and their tower actors stay in the world. Recapture is the same loop: channel, then rebuild all 3 starters.

First-visit 1–2 starter towers (never yet reached 3) do not grant vision, do not show extra pads, and do not power extra towers. After a full capture, dropping to 1–2 starters keeps vision and extra-tower power until the count hits 0. After a loss, placing only 1–2 rebuilt starters does **not** restore vision or extra power — all 3 must be up again.

## `ACaptureBase` actor

C++ actor `ACaptureBase` with Blueprint `BP_CaptureBase` for mesh, overlap, and channel bar.

Editor properties:

- `CaptureRadius` (default 1200 cm) — channel and contest cylinder, XY distance to the base location.
- `ChannelDuration` (default 5 s) — time to fill 0→1 while fill conditions hold.
- `VisionRadius` (default 8000 cm) — same as `CrystalVisionRadius`, tunable per base.
- `StarterPads` — exactly 3 `AActor` references (`BP_HexPad`).
- `ExtraPads` — 0 or more designer-placed `BP_HexPad` references. All appear together on first full capture.

Begin play: validate, then hide every assigned pad. A base with missing starter slots, a null extra pad, or the same pad listed twice logs an error and stays inert (no channel, no vision, pads left hidden).

The base tracks which tower occupies each starter pad (set when a tower is placed on that pad, cleared when that tower dies or is destroyed). Extra towers are whatever occupies extra pads; the base does not destroy them on loss.

## Channel

Overlap/tick counts:

- Champion in radius: controlled champion XY distance ≤ `CaptureRadius`.
- Enemy in radius: any living `BP_Enemy` (or child) with XY distance ≤ `CaptureRadius`.

Fill: `ChannelProgress += DeltaSeconds / ChannelDuration`, clamped to 1. Pause: do not change progress. No decay and no reset except on loss (`StarterTowersAlive` hits 0), which sets progress to 0 so the next claim is a full channel.

World widget on the base shows progress while `StarterTowersAlive == 0` and (`ChannelProgress > 0` or champion in radius). Hidden otherwise. Contested or champion-absent: bar frozen, not emptied.

## Pads and placement

Pads remain `BP_HexPad`. The base owns visibility and whether a pad currently accepts a tower:

- Occupied pad: not buildable (already has a tower). Occupied starter pads are hidden. Extra pads stay visible even while occupied.
- Empty starter pad: buildable only after the current channel has completed and `StarterTowersAlive < 3`.
- Empty extra pad: buildable only while `bHeld` (so only while the base still has at least one starter after a full capture, including all 3 up). After loss, extra pads reject new placement until the 3 starters are rebuilt.

`TryPlaceTower` / BuildManager consult `ACaptureBase::IsPadBuildable(Pad)` for pads that belong to a base. Unowned pads (main crystal, etc.) are unchanged.

Placing a tower does not destroy the pad. The pad stores (or the base stores) the occupying tower so destruction can update `StarterTowersAlive`.

## Towers

Placement, cost, construction, and combat stay on BuildManager / `BP_Tower`.

Extra-tower **power**:

- Powered while `bHeld`.
- Unpowered on loss. Unpowered extra towers stay in the world, remain visible, and do not attack. They do not count as destroyed.
- Power restored when `StarterTowersAlive` reaches 3 again.

Starter towers are never “shut down”; they are either alive on the pad or destroyed. Enemies un-capture only by destroying those 3, not by channeling the zone.

A `bPowered` (or equivalent) flag on `BP_Tower` is the gate. Default true so non-base towers are unaffected. The capture base sets it on extra towers it owns; combat tick skips fire when false.

## Vision

On full capture (`bHeld` becomes true), the base calls `RegisterVisionSource(this, VisionRadius)` on the local `UMapDiscoveryComponent`. On loss, it calls new `UnregisterVisionSource(this)`.

`UnregisterVisionSource` removes only that actor’s entry. It must not clear the crystal or champion explorer.

Captured-base vision is the same live circle as the crystal: fog and minion hide/show use `FTDFogVision::IsLocationVisible` over all current sources. It is not a one-way Diablo stamp.

## Editor placement

Level designers:

1. Place `BP_CaptureBase`.
2. Place 3 starter `BP_HexPad`s and any extra pads around it.
3. Assign the 3 starter references and the extra-pad list.

No procedural pad spawn. Pad count after full capture is whatever was assigned in the editor.

## Failure handling

- Inert base (invalid pad assignment): no gameplay, error log.
- Champion or enemy missing from overlap: treat as not present (channel pauses if the champion is gone).
- Extra tower destroyed by combat while the base is held: that pad becomes empty and is buildable again only while `bHeld`. It does not affect `bHeld` or vision.
- Starter tower destroyed: `StarterTowersAlive` drops. At 0, loss runs even if extra towers still exist.
- Fog component not ready yet: retry vision register on tick until it succeeds, while `bHeld` is true.

## Out of scope

- Changing main-crystal pads, drop location, or crystal vision radius.
- Enemy-channeled capture of a base.
- Hiding or despawning extra towers on loss.
- Multiple teams / PvP contest.
- Minimap-specific base markers (optional later; vision itself still affects fog on the minimap).

## Logic to test in C++

Pure helper `FTDCaptureBaseLogic` (same style as `FTDFogVision`): given champion-in-radius, enemy-in-radius, delta time, duration, current progress, starter count, previous `bHeld` / `bExtraPadsUnlocked`, it returns new progress, new flags, pad visibility, vision on/off, extra powered, and whether starter pads are buildable.

Cases:

- Fill while champion in and no enemies; pause when an enemy enters; resume from the same progress.
- Channel complete with 0 starters → starter pads buildable, vision off, extra hidden.
- 1–2 starters, never held → no vision, extra hidden.
- 3rd starter → `bHeld`, vision on, extra visible and powered.
- Drop from 3 to 2 → still held: vision on, extra powered.
- Drop from 1 to 0 → loss: vision off, extra unpowered, extra pads still “unlocked/visible”, channel progress 0, starter pads not buildable.
- After loss, channel again and rebuild 3 → vision and extra power restored.
- Extra pads not buildable while not held.

Also: `UnregisterVisionSource` removes one actor and leaves others registered.

## Verification

- Compile the TD editor target.
- Run the capture-base and vision-unregister automation tests.
- In PIE: approach a placed base, channel (pause with an enemy in radius), build 3 starters, confirm vision and extra pads, destroy the 3 starters, confirm extra towers stop firing and stay, recapture by channel + 3 rebuilds.
