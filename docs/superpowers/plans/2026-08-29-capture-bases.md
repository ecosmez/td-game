# Capture Bases Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Designer-placed outposts that the champion channels, then fully captures by building 3 starter towers, unlocking crystal-style vision and extra hex pads; enemies un-capture by destroying those 3.

**Architecture:** Pure `FTDCaptureBaseLogic` owns the state machine. `ACaptureBase` is a placeable C++ actor that assigns 3 starter pads + extra pads, ticks occupancy/contest, shows/hides pads (collision off when not buildable so existing `BP_HexPad` needs no graph edit), registers/unregisters live vision, and restores extra-tower `CanAttack`. `UMapDiscoveryComponent::UnregisterVisionSource` removes one live source without clearing the crystal.

**Tech Stack:** Unreal Engine C++, UMG world widget, existing `BP_HexPad` / `BP_Tower` / `BP_Enemy`, live fog `UMapDiscoveryComponent`.

**Spec:** `docs/superpowers/specs/2026-08-29-capture-bases-design.md`

## Global Constraints

- Channel fills only with champion in radius and zero living enemies in radius; otherwise pause (no decay except reset when the last starter tower dies).
- Vision and extra-tower power follow `bHeld` (on at 3 starters, stays on while at least one starter remains after a full capture, off at 0).
- Extra pads unlock once and stay visible; they reject new builds while not held.
- Recapture after loss requires a full channel, then all 3 starters rebuilt.
- Do not change main-crystal pads or unrelated worktree files.
- Unreal MCP is unavailable: pad gating is hide + collision, not HexPad graph edits.

## Files

- Create: `Source/TD/TDCaptureBaseLogic.h`, `Source/TD/TDCaptureBaseLogic.cpp`
- Create: `Source/TD/Tests/CaptureBaseTests.cpp`
- Create: `Source/TD/CaptureBase.h`, `Source/TD/CaptureBase.cpp`
- Create: `Source/TD/CaptureChannelWidget.h`, `Source/TD/CaptureChannelWidget.cpp`
- Modify: `Source/TD/MapDiscoveryComponent.h`, `Source/TD/MapDiscoveryComponent.cpp`
- Modify: `Source/TD/MapFogWorldSubsystem.h`

---

### Task 1: Capture state machine (`FTDCaptureBaseLogic`)

**Files:**
- Create: `Source/TD/TDCaptureBaseLogic.h`
- Create: `Source/TD/TDCaptureBaseLogic.cpp`
- Test: `Source/TD/Tests/CaptureBaseTests.cpp`

**Interfaces:**
- Produces: `FTDCaptureBaseInput`, `FTDCaptureBaseOutput`, `FTDCaptureBaseLogic::Step`, `ShouldFillChannel`, `AdvanceChannel`, `IsInsideRadius`, `IsPadBuildable`, `ArePadAssignmentsValid`
- Produces: `FTDVisionSourceList::Unregister(TArray<uint64>& Keys, uint64 ActorKey) -> int32`

- [ ] **Step 1: Write failing automation tests** covering fill/pause, first capture, hold-while-losing, loss, recapture, extra-pad build gate, pad assignment validation, vision-key unregister.
- [ ] **Step 2: Implement `FTDCaptureBaseLogic` and `FTDVisionSourceList` so those tests pass.**
- [ ] **Step 3: Compile TDEditor and run `TD.Capture` + `TD.Fog.Vision` automation tests.**

### Task 2: `UnregisterVisionSource`

**Files:**
- Modify: `Source/TD/MapDiscoveryComponent.h`
- Modify: `Source/TD/MapDiscoveryComponent.cpp`

**Interfaces:**
- Consumes: `FTDVisionSourceList::Unregister` pattern (pointer identity)
- Produces: `UMapDiscoveryComponent::UnregisterVisionSource(AActor* Actor) -> bool`

- [ ] **Step 1: Add `UnregisterVisionSource` that removes only that actor’s live source.**
- [ ] **Step 2: Expose `GetBoundDiscovery()` on `UMapFogWorldSubsystem`.**

### Task 3: `ACaptureBase` actor

**Files:**
- Create: `Source/TD/CaptureBase.h`, `Source/TD/CaptureBase.cpp`
- Create: `Source/TD/CaptureChannelWidget.h`, `Source/TD/CaptureChannelWidget.cpp`

**Interfaces:**
- Consumes: `FTDCaptureBaseLogic::Step`, `UnregisterVisionSource`, `RegisterVisionSource`
- Produces: placeable `ACaptureBase` with `StarterPads[3]`, `ExtraPads`, `CaptureRadius`, `ChannelDuration`, `VisionRadius`, `IsPadBuildable(Pad)`

Behavior:
- BeginPlay validates pad refs; inert on failure; hides all assigned pads.
- Tick: champion + living enemies in XY radius; occupancy = living `BP_Tower` within 180 cm of a pad; `Step`; apply pad hidden/collision; extra tower `CanAttack` save/restore; register/unregister vision with retry while held; channel bar while `bShowChannelBar`.

- [ ] **Step 1: Implement actor + world-space channel bar.**
- [ ] **Step 2: Compile TDEditor.**

## Verification

- TDEditor Win64 Development compiles.
- Automation: `TD.Capture.Base.*` and vision unregister helper tests pass.
- PIE (when editor available): channel, 3 starters, vision + extra pads, destroy 3, extra towers silent, recapture.
