# Tabbed Tower Store Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add All, Attack, Defense, and Support tabs that filter the existing tower-store catalog without changing purchase or placement behavior.

**Architecture:** Give every `FTowerStoreEntryDef` one typed gameplay category and keep `All` as widget filter state. Build a small tab row above the existing horizontal cards; switching filters rebuilds the visible cards from the canonical catalog and refreshes the existing hover and affordability state.

**Tech Stack:** Unreal Engine C++, UMG (`UUserWidget`, `UButton`, `UHorizontalBox`), Unreal Header Tool reflection.

**Spec:** `docs/superpowers/specs/2026-08-23-tabbed-tower-store-design.md`

## Global Constraints

- Each catalog entry has exactly one of `Attack`, `Defense`, or `Support` as its primary category.
- `All` is a view and must not be stored as an entry category.
- Preserve existing BuildManager selection functions, purchasing, affordability, hover preview, stats, and placement behavior.
- Preserve the active filter while the same widget instance is closed and reopened.
- Do not modify unrelated Unreal assets or existing worktree changes.

---

### Task 1: Typed Catalog Categories and Filter State

**Files:**
- Modify: `Source/TD/TowerStoreWidget.h`
- Modify: `Source/TD/TowerStoreWidget.cpp`

**Interfaces:**
- Produces: `ETowerStoreCategory { Attack, Defense, Support }`
- Produces: `FTowerStoreEntryDef::Category`
- Produces: `UTowerStoreWidget::DoesEntryMatchActiveFilter(const FTowerStoreEntryDef&) const -> bool`
- Produces: canonical category assignments used by card rebuilding in Task 2.

- [ ] **Step 1: Add a compile-time category mapping check**

Add a development-only automation test at the bottom of `TowerStoreWidget.cpp`. Construct lightweight entry definitions and verify that the filter predicate accepts all entries in All mode, accepts the matching category, and rejects the other two categories. Guard the test and its include so shipping builds are unchanged:

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTowerStoreCategoryFilterTest,
	"TD.UI.TowerStore.CategoryFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
#endif
```

Because the test needs a pure seam, add this public static function to `UTowerStoreWidget`:

```cpp
static bool DoesCategoryMatchFilter(
	ETowerStoreCategory EntryCategory,
	bool bShowAll,
	ETowerStoreCategory ActiveCategory);
```

The test calls only this pure function; it does not construct a widget or world.

- [ ] **Step 2: Build to verify the test seam is absent**

Run the project’s Unreal Editor build command for target `TDEditor Win64 Development`.

Expected: compilation fails because `ETowerStoreCategory` and `DoesCategoryMatchFilter` do not exist yet.

- [ ] **Step 3: Add the typed category model and pure predicate**

In `TowerStoreWidget.h`, define the enum immediately before `FTowerStoreEntryDef`:

```cpp
UENUM(BlueprintType)
enum class ETowerStoreCategory : uint8
{
	Attack,
	Defense,
	Support
};
```

Add this field to `FTowerStoreEntryDef`:

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store")
ETowerStoreCategory Category = ETowerStoreCategory::Attack;
```

Add widget state:

```cpp
bool bShowAllCategories = true;
ETowerStoreCategory ActiveCategory = ETowerStoreCategory::Attack;
```

Implement the pure predicate as:

```cpp
return bShowAll || EntryCategory == ActiveCategory;
```

Update the `BuildDefaultCatalog` helper to accept a category and assign it to the entry. Map Arrow, Cannon, Sniper, Magic, and Mine to Attack; Wall and all Slow/Root/Freeze traps to Defense; Buff and Economy to Support.

- [ ] **Step 4: Run the automation test and compile**

Run the `TDEditor Win64 Development` build, then run Unreal Automation test `TD.UI.TowerStore.CategoryFilter`.

Expected: the target compiles and the category-filter test passes.

- [ ] **Step 5: Commit the category model**

```bash
git add Source/TD/TowerStoreWidget.h Source/TD/TowerStoreWidget.cpp
git commit -m "Add typed tower store categories"
```

### Task 2: Category Tabs and Filtered Card Rebuilding

**Files:**
- Modify: `Source/TD/TowerStoreWidget.h`
- Modify: `Source/TD/TowerStoreWidget.cpp`
- Modify: `Content/TD/Store_README.txt`

**Interfaces:**
- Consumes: `FTowerStoreEntryDef::Category` and `DoesCategoryMatchFilter(...)` from Task 1.
- Produces: `BuildCategoryTabs(UVerticalBox*)`, `SetCategoryFilter(bool, ETowerStoreCategory)`, and `RefreshCategoryTabVisuals()`.
- Produces: a tab binder that maps each button click to All or one typed category.

- [ ] **Step 1: Extend the automation test for visible-list behavior**

Create a local array containing one entry from each category. For All mode assert three matches; for Attack, Defense, and Support assert one match each by counting entries through `DoesCategoryMatchFilter`.

- [ ] **Step 2: Run the test and confirm the UI behavior is not implemented**

Run `TD.UI.TowerStore.CategoryFilter`.

Expected: predicate tests pass, while source inspection confirms there are no category-tab widgets or filter-triggered calls to `BuildCards` yet.

- [ ] **Step 3: Build tab controls and filtered cards**

Add a `UTowerStoreCategoryClickBinder` UObject patterned after the existing card binder. Store `bShowAll`, `Category`, and `Store`, and call:

```cpp
Store->SetCategoryFilter(bShowAll, Category);
```

In `BuildDefaultUI`, insert `BuildCategoryTabs(PanelV)` after the header and before `CardScroll`. Create four buttons labeled `All`, `Attack`, `Defense`, and `Support` and retain their button/text handles for selected styling.

Implement `SetCategoryFilter` so it returns early for the already-active filter; otherwise it hides the hover panel, resets `HoveredCardIndex`, destroys the current hover tower actor, updates filter state, calls `BuildCards`, refreshes tab visuals, and refreshes affordability from the current BuildManager resource.

Update `BuildCards` to clear `CardRow`, `Cards`, and `CardClickBinders`, then iterate the canonical `Catalog` and call `BuildCard` only when `DoesCategoryMatchFilter` returns true. Card indexes must be assigned from `Cards.Num()` so every binder indexes the visible array correctly.

Use the existing blue store frame palette for the active tab and a dim neutral-blue background for inactive tabs. Do not change card, hover, resource, or dock styling.

- [ ] **Step 4: Document and verify in PIE**

Update `Content/TD/Store_README.txt` to list the four tabs and their category mappings.

Build `TDEditor Win64 Development`, run `TD.UI.TowerStore.CategoryFilter`, then verify in PIE:

1. Store opens on All with 14 entries in catalog order.
2. Attack shows Arrow, Cannon, Sniper, Magic, and Mine.
3. Defense shows Wall and the six Slow/Root/Freeze trap variants.
4. Support shows Buff and Economy.
5. Switching tabs clears the floating hover preview.
6. Unaffordable cards remain dimmed and affordable cards remain normal.
7. Clicking one card in every category starts the existing tower drag/selection flow.
8. Closing and reopening the store retains the selected tab.

- [ ] **Step 5: Commit the tab UI**

```bash
git add Source/TD/TowerStoreWidget.h Source/TD/TowerStoreWidget.cpp Content/TD/Store_README.txt
git commit -m "Add category tabs to tower store"
```

### Task 3: Final Regression Verification

**Files:**
- Verify: `Source/TD/TowerStoreWidget.h`
- Verify: `Source/TD/TowerStoreWidget.cpp`
- Verify: `Content/TD/Store_README.txt`

**Interfaces:**
- Consumes: the completed typed catalog and category-tab UI.
- Produces: verified build and interaction results with no additional production interface.

- [ ] **Step 1: Run the focused automated test**

Run Unreal Automation test `TD.UI.TowerStore.CategoryFilter`.

Expected: PASS with no warnings from the test.

- [ ] **Step 2: Run a clean editor-target build**

Build `TDEditor Win64 Development` from the current project.

Expected: build succeeds with Unreal Header Tool accepting the new enum and reflected property.

- [ ] **Step 3: Review only scoped changes**

```bash
git diff HEAD~2 -- Source/TD/TowerStoreWidget.h Source/TD/TowerStoreWidget.cpp Content/TD/Store_README.txt
git status --short --untracked-files=all
```

Expected: the feature diff contains only the category model, tabs, filtering, automation test, and README update. Existing unrelated modifications remain present and untouched.
