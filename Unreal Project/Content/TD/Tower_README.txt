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
- FireInterval      (seconds between shots; shared interval used as each fire point's delay)
- DamagePerShot     (passed into BP_Projectile InitProjectile)
- ShotsPerVolley    (projectiles spawned per fire-point shot; SpawnVolleyAt loops this many times, min 1)
- Range             (uu from the fire point)
- ProjectileMesh    (StaticMesh for shots; child types override, else inherit base Sphere)
- TowerTypeName     (label)

Fire positions (1–8)
- Place child Scene Components on the tower (or child tower BPs). Tag each with FirePoint.
- Base ships with FirePoint_0 at relative (120, 0, 180) so existing towers still shoot.
- BeginPlay → GatherFirePoints: clears FirePoints / FirePointCooldowns, collects tagged SceneComponents (max 8), one cooldown slot per point (starts at 0).
- Keep points outside TowerMesh collision so LoS traces do not start embedded.
  TowerBase local bounds are about X±272, Y±314, Z up to ~1177 (plus runtime scale).
  Place FirePoints beyond ~400 uu from the center on X/Y or LoS always hits the tower and never fires.

Combat fire (TryFire)
1. Gate on CanAttack + AttackSpeed > 0.
2. For each fire point index i:
   a. FirePointCooldowns[i] -= DeltaSeconds; skip if still > 0.
   b. SelectVisibleTarget from that point's world location:
      - Scan BP_Enemy actors; keep those within Range of the fire point.
      - Prefer nearest with clear Visibility line trace (start offset 10uu toward enemy; end at enemy +Z 50).
      - Trace ignores enemies (they do not block). bIgnoreSelf=false so the tower body blocks opposite-side points.
      - WorldStatic / blocking world geometry also blocks.
   c. If HasFireTarget: set cooldown[i] = FireInterval, SpawnVolleyAt from that fire point (ShotsPerVolley, InitProjectile on BestFireTarget).

Helpers on BP_Tower
- GatherFirePoints
- SelectVisibleTarget(Origin)
- SpawnVolleyAt(Origin)
- TryFire (tick when IsBuilt)

Collision notes
- TowerMesh: BlockAllDynamic / QueryAndPhysics — occludes LoS through the tower.
- HoloMesh: NoCollision so the silhouette never blocks combat traces.

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
