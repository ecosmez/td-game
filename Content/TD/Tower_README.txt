Tower construction + combat stats (BP_Tower)

Construction flow
1. BP_TowerPad.TryPlaceTower destroys previous tower on pad (if any), spawns BP_Tower, calls StartConstruction.
2. StartConstruction shows HoloMesh (M_TowerHolo full silhouette). TowerMesh / HoloMesh keep their authored transforms (no runtime scale or location).
3. Tick → UpdateConstruction while IsConstructing: progress += dt / ConstructionTime.
4. At progress ≥ 1: IsBuilt=true, hide holo ("Tower ready!"), then TryFire each tick.

Editable stats on BP_Tower (Details panel)
- ConstructionTime  (seconds, default 3)
- AttackSpeed       (shots/sec; BeginPlay sets FireInterval = 1/AttackSpeed when > 0)
- FireInterval      (seconds between shots; shared interval used as each fire point's delay)
- DamagePerShot     (passed into BP_Projectile InitProjectile)
- ShotsPerVolley    (projectiles spawned per fire-point shot; SpawnVolleyAt loops this many times, min 1)
- Range             (uu from the fire point)
- ProjectileMesh    (StaticMesh for shots; child types override, else inherit base Sphere)
- TowerTypeName     (label)
- IsAoe             (routes EventTick to UpdateAoEBehavior instead of TryFire — see below)
- AoeRadius         (uu; detect + blast radius for IsAoe towers, reuses DamagePerShot as blast damage)
- PulseMode         (IsAoe only: false = one-shot then self-destruct, true = repeat every PulseInterval)
- PulseInterval     (seconds between pulses when PulseMode is true)

Fire positions (1–8)
- Place child Scene Components on the tower (or child tower BPs). Tag each with FirePoint.
- Base ships with FirePoint_0 at relative (120, 0, 180) so existing towers still shoot.
- BeginPlay → GatherFirePoints: clears FirePoints / FirePointCooldowns, collects tagged SceneComponents (max 8), one cooldown slot per point (starts at 0).
- Fire points can sit on the mesh. Tag each component FirePoint (name alone is not enough).

Combat fire (TryFire)
1. Gate on CanAttack + AttackSpeed > 0.
2. For each fire point index i:
   a. FirePointCooldowns[i] -= DeltaSeconds; skip if still > 0.
   b. SelectVisibleTarget from that point's world location:
      - Scan BP_Enemy actors; keep those within Range of the fire point.
      - Prefer nearest. Do not require Visibility LoS — rocky pads would block every shot.
   c. If HasFireTarget: set cooldown[i] = FireInterval, SpawnVolleyAt from that fire point (ShotsPerVolley, InitProjectile on BestFireTarget).

Helpers on BP_Tower
- GatherFirePoints
- SelectVisibleTarget(Origin)
- SpawnVolleyAt(Origin)
- TryFire (tick when IsBuilt and not IsAoe)
- UpdateAoEBehavior(DeltaSeconds) / HasEnemyInRange / FireAoEBurst (tick when IsBuilt and IsAoe;
  see Tower_UpdateAoEBehavior.dsl.txt — used by BP_Tower_Mine)

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

Ghost preview (BuildManager)
- SpawnGhostTower loads SelectedTowerClassPath and spawns that child class (fallback BP_Tower).
- ConfigureGhostTower: collision/tick off, IsGhost=true, ShowRangePreview, then UpdateGhostTower follows cursor/pads.
- ShowRangePreview shows RangeRing (cylinder + MI_AbilityRange) scaled XY = Range/50 so the disc matches combat Range.

Materials
- /Game/TD/Materials/M_TowerHolo — translucent cyan full silhouette during build

Projectile display
- Tower.ProjectileMesh → projectile.DisplayMesh → projectile StaticMesh (if DisplayMesh valid; else BP_Projectile default)
