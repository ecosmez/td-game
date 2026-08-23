# Tabbed Tower Store Design

## Goal

Make the tower store easier to browse by filtering its existing catalog into clear gameplay categories. Preserve the current purchase, affordability, hover-preview, resource, and placement behavior.

## Categories

The store exposes four exclusive tabs:

- **All:** every catalog entry, in catalog order.
- **Attack:** Arrow, Cannon, Sniper, Magic, and Mine.
- **Defense:** Wall plus all Slow, Root, and Freeze field/snare traps.
- **Support:** Buff and Economy.

Each catalog entry has exactly one primary category. `All` is a view, not an entry category.

## Data Model

Add a Blueprint-visible `ETowerStoreCategory` enum with `Attack`, `Defense`, and `Support` values. Add a `Category` property to `FTowerStoreEntryDef` and assign it when the default catalog is built.

The existing free-form `Role` remains descriptive text for hover details. It is not used for filtering because gameplay roles and navigation categories are different concepts.

## UI and Interaction

Add an `All / Attack / Defense / Support` tab row between the store header and the horizontal card strip. `All` is selected when the widget is first built.

Selecting a tab:

1. Updates the active-category state and selected-tab styling.
2. Clears the current hover selection and hides the floating preview.
3. Rebuilds the card row from catalog entries matching the selected category, or all entries for `All`.
4. Immediately refreshes affordability styling using the current resource value.

Clicking the already-active tab is harmless and leaves the current card list intact. Closing and reopening the store preserves the active tab for that widget instance.

## Existing Behavior Preserved

Filtered cards retain their complete `FTowerStoreEntryDef`, including selection function, class path, price, mesh, material, and stats. Card clicks therefore continue to call the existing BuildManager selection functions. No BuildManager, placement, economy, tower Blueprint, or trap Blueprint behavior changes.

Card event binders use indexes into the currently visible `Cards` array. Rebuilding clears old binders and cards before creating the filtered set, preventing stale indexes.

## Failure Handling

An unrecognized filter state falls back to showing all entries. A category with no entries displays an empty card row without affecting the store header or resource display. Missing BuildManager selection functions continue to follow the current click behavior.

## Verification

- Compile the TD editor target to validate Unreal reflection and C++ integration.
- Inspect the catalog mapping for all 14 entries.
- Verify each tab shows only its expected entries and `All` restores catalog order.
- Verify switching tabs clears hover preview state.
- Verify affordability refresh and card selection still operate on filtered entries.
- Confirm unrelated worktree changes remain untouched.
