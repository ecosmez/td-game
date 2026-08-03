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

- /Game/TD/UI/WBP_TowerStore

- Shown by BuildManager.ShowBuildHUD

- Live resource readout

- Tower buttons only → Select*Tower (sets cost + name + BeginTowerDrag)



Prices:

  Trap 40 | Wall 35 | Arrow 60 | Economy 90 | Buff 80

  Cannon 100 | Sniper 140 | Magic 200 | Basic 50



Flow:

  Click tower in store → must afford → drag ghost → drop on pad

  → charge SelectedTowerCost → spawn tower → construction



Note: placement currently spawns BP_Tower base class after charge;

  per-type class spawn (child BPs) can be wired next via SelectedTowerName switch.

