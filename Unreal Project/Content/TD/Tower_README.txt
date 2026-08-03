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
- Range             (uu)
- ProjectileMesh    (StaticMesh look for shots; applied via SetDisplayMesh → InitProjectile SetStaticMesh)
- TowerTypeName     (label for multi-type later)

Flags: IsConstructing, IsBuilt, ConstructionProgress, MeshMinZ

Ghost preview (BuildManager) leaves tick off so construction does not run until real place.

Materials
- /Game/TD/Materials/M_TowerHolo — translucent cyan full silhouette during build

Projectile display
- Tower.ProjectileMesh → projectile.DisplayMesh → projectile StaticMesh component (if DisplayMesh valid)

Next: child BP / DataAsset per tower type + UI selection of type class.
