Tower construction + combat stats (BP_Tower)

Construction flow
1. BP_TowerPad.TryPlaceTower destroys previous tower on pad (if any), spawns BP_Tower, calls StartConstruction.
2. StartConstruction shows HoloMesh (M_TowerHolo full silhouette) and starts solid mesh near ground.
3. Tick → UpdateConstruction while IsConstructing: progress += dt / ConstructionTime.
4. Solid mesh Z-scale grows 0→2.5 with foot planted via MeshMinZ offset.
5. At progress ≥ 1: IsBuilt=true, hide holo, full mesh ("Tower ready!"), then TryFire each tick.

Editable stats on BP_Tower (Details panel)
- ConstructionTime  (seconds, default 3)
- AttackSpeed       (shots/sec; BeginPlay sets FireInterval = 1/AttackSpeed when > 0)
- FireInterval      (seconds between shots)
- DamagePerShot     (passed into BP_Projectile InitProjectile)
- ShotsPerVolley    (projectiles spawned per shot; TryFire loops this many times, min 1)
- Range             (uu)
- ProjectileMesh    (StaticMesh for shots; child types override, else inherit base Sphere)
- TowerTypeName     (label)

Combat fire (TryFire)
1. Gate on CanAttack + AttackSpeed > 0; tick down FireCooldown.
2. When ready and an enemy is in Range: reset cooldown to FireInterval.
3. Spawn ShotsPerVolley projectiles (slight X offset), each with tower ProjectileMesh + DamagePerShot.

Placement (BP_TowerPad.SpawnAndStartTower)
- Spawns the selected child class (Arrow/Cannon/…) from BuildManager.SelectedTowerName
  so that type’s CDO damage / AS / volley / mesh are used at runtime.
- Select*Tower also sets SelectedTowerClassPath for reference.

Example type defaults
- Arrow:  AS 3.0, DMG 6, volley 3, mesh Cylinder
- Cannon: AS 0.6, DMG 35, volley 1, mesh Cone

Flags: IsConstructing, IsBuilt, ConstructionProgress, MeshMinZ

Ghost preview (BuildManager) leaves tick off so construction does not run until real place.

Materials
- /Game/TD/Materials/M_TowerHolo — translucent cyan full silhouette during build

Projectile display
- Tower.ProjectileMesh → projectile.DisplayMesh → projectile StaticMesh (if DisplayMesh valid; else BP_Projectile default)
