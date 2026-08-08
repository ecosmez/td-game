TOWER STORE + RESOURCE ECONOMY

================================

Resource (crystal energy) lives on BP_BuildManager.

Defaults (Details panel on BuildManager):
- StartingResource = 150
- ResourcePerSecond = 5
- Resource = current balance (starts at StartingResource on BeginPlay)

Income:
- EventTick calls UpdateResource
- each frame: Resource += ResourcePerSecond * DeltaSeconds

Spending:
- Towers: cost charged on successful pad place (TrySpendResource)

Champion abilities:
- NOT in the tower store
- Use the Ability Bar (Q / W / E / R)

Store UI:
- /Game/TD/UI/WBP_TowerStore (C++ parent: TowerStoreWidget /Script/TD.TowerStoreWidget)
- Shown by BuildManager.ShowBuildHUD
- Always-visible PLUS button (bottom-left) toggles the store panel open/closed
- Panel closed by default; click + to open, − / X to close
- Live resource readout + afford greying
- Each tower type card:
    • Translucent projection of the real tower mesh (M_TowerHolo scene-capture)
    • Stats: build time, damage, attack speed, range, role notes
    • Click → Select*Tower on BuildManager (sets cost + name + BeginTowerDrag)

Prices:
  Trap 40 | Wall 35 | Arrow 60 | Economy 90 | Buff 80
  Cannon 100 | Sniper 140 | Magic 200

Flow:
  Click + → open store → click tower card → must afford → drag ghost → drop on pad
  → charge SelectedTowerCost → spawn tower → construction

C++ source:
  Source/TD/TowerStoreWidget.h / .cpp

Setup (after C++ compile):
  Content/Python/setup_tower_store_ui.py  (also auto via init_unreal.py)

Note: placement currently may still spawn base BP_Tower after charge depending pad wiring;
  per-type class spawn (child BPs) can be wired via SelectedTowerName switch.
