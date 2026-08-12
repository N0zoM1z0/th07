#include "inttypes.hpp"

#include <d3dx8math.h>
#include <math.h>
#include <string.h>

#pragma intrinsic(sin, cos)

namespace th07 {

/*
 * This is deliberately a source-private view of Player's bomb tail.  Player.hpp
 * currently exposes only the collision-facing prefix through 0x16A24, whereas
 * the original constructor clears and initializes the full 0xB7E78 object.
 */

struct PlayerBombOverlay;

struct PlayerBombPoint {
    f32 x;
    f32 y;
    f32 z;

    void Initialize()
    {
    }
};

struct PlayerBombTimer {
    i32 previous;
    f32 fraction;
    i32 current;
};

struct PlayerBombDamageRegion {
    u8 unknown00[0x20];

    void Initialize()
    {
    }
};

/*
 * Target 0x441800/0x4418B0 populate this 96-entry pool.  A nonzero sizeX
 * selects the rectangular collision path; otherwise radius selects the
 * circular path in Player::CheckAuxProjectileCollision.
 */
struct PlayerBombProjectile {
    f32 centerX;
    f32 centerY;
    f32 sizeX;
    f32 sizeY;
    f32 radius;
    f32 unknown14;
    u32 unknown18;
    u32 collisionValue;
};

struct PlayerBombAnmVm {
    u8 unknown00[0x1D4];
    i16 scriptId;
    u8 unknown1D6[0x76];
};

/* The four target table slots are raw 32-bit code pointers invoked with ECX. */
typedef void (__fastcall *PlayerBombCallback)(PlayerBombOverlay *player);

struct PlayerBombWorkItem {
    u8 unknown00[0x20];
    PlayerBombPoint points[32];
    u8 unknown1A0[0x18];
    PlayerBombAnmVm sprites[8];
    u8 unknown1418[4];
    PlayerBombTimer timer;

    PlayerBombWorkItem *Initialize();
};

struct PlayerBombState {
    i32 active;
    i32 capturedFocus;
    i32 unknown08;
    i32 unknown0C;
    PlayerBombTimer tick;
    PlayerBombCallback calculateUnfocused;
    PlayerBombCallback drawUnfocused;
    PlayerBombCallback calculateFocused;
    PlayerBombCallback drawFocused;
    PlayerBombWorkItem workItems[128];

    PlayerBombState *Initialize();
};

struct PlayerBombOverlay {
    u8 unknown0000[0x17DC];
    PlayerBombProjectile bombProjectiles[96];
    u8 unknown23DC[0x14644];
    u8 bombStorage[0xA142C];
    u8 unknownB7E4C[0x0C];
    f32 rotationAngle;
    u8 unknownB7E5C[0x0C];
    void *bombVisual0;
    void *bombVisual1;
    void *shotDataUnfocused;
    void *shotDataFocused;

    PlayerBombState *BombState()
    {
        return reinterpret_cast<PlayerBombState *>(bombStorage);
    }

    const PlayerBombState *BombState() const
    {
        return reinterpret_cast<const PlayerBombState *>(bombStorage);
    }

    PlayerBombOverlay *Initialize();
    i32 RegisterBombCallbacks();
    void BeginBomb();
    void UpdateBomb();
    i32 DrawBomb();
    PlayerBombProjectile *AddBombProjectileRectangle(const PlayerBombPoint *center, f32 sizeX, f32 sizeY,
                                                      u32 collisionValue);
    PlayerBombProjectile *AddBombProjectileCircle(const PlayerBombPoint *center, f32 radius, f32 unknown14,
                                                   u32 unknown18, u32 collisionValue);

    /* Target calls resolved by the Player/ANM integration unit. */
    void StartDeathBomb(i32 deathBomb);
    void FinishDeathBomb();
    i32 IsBombStartBlocked();
    i32 IsBombInputBlocked();
    void MarkBombStart(i32 enabled);
    void SetBombItemState(i32 state);
    void SetBombEffectTimer(i32 timer);
    void DrawPlayerBullets();
};

typedef char PlayerBombAnmVmSizeMustMatch[(sizeof(PlayerBombAnmVm) == 0x24C) ? 1 : -1];
typedef char PlayerBombWorkItemSizeMustMatch[(sizeof(PlayerBombWorkItem) == 0x1428) ? 1 : -1];
typedef char PlayerBombStateSizeMustMatch[(sizeof(PlayerBombState) == 0xA142C) ? 1 : -1];
typedef char PlayerBombOverlaySizeMustMatch[(sizeof(PlayerBombOverlay) == 0xB7E78) ? 1 : -1];
typedef char PlayerBombCallbackSizeMustMatch[(sizeof(PlayerBombCallback) == 4) ? 1 : -1];
typedef char PlayerBombProjectileSizeMustMatch[(sizeof(PlayerBombProjectile) == 0x20) ? 1 : -1];

/*
 * 0x4011B0 and 0x401170 are target constructors called before the following
 * zeroing passes.  They remain imports here until the shared ANM ABI is named.
 */
extern void __fastcall InitializeAnmVm(void *vm);
extern void __fastcall InitializeSecondaryAnmVm(void *vm);
extern void __fastcall InitializePlayerBullet(void *bullet);

/* 0x49EC50: four adjacent callback entries per character/shot selector. */
extern PlayerBombCallback g_PlayerBombCallbacks[6][4];

/* Source-private global views used by the high-level bomb update and draw path. */
extern u16 g_PlayerInputButtons;

struct PlayerBombInputState {
    u8 unknown00[0xD6];
    u16 flags;
};

/* The shared collision-state global is a target pointer; this module needs its input flag tail. */
extern i32 g_PlayerCollisionFlags;
extern i32 g_PlayerFlags;
extern i32 g_BombScore;
extern i32 g_BombDrawTimer;
extern f32 g_PlayerDrawOffsetX;
extern f32 g_PlayerDrawOffsetY;
extern u8 g_HidePlayerDraw;

struct PlayerBombResourceState {
    u8 unknown00[0x68];
    f32 bombStock;
    u8 unknown6C[0x1C];
    i32 scoreFloor;
    i32 bombCapacity;
};

extern PlayerBombResourceState *g_PlayerBombResources;

struct PlayerBombScreenState {
    u8 unknown00[0x08];
    i32 bombCapacity;
    f32 width;
    f32 height;
};

struct PlayerBombDrawColor {
    u8 blue;
    u8 green;
    u8 red;
    u8 alpha;
};

struct PlayerBombShtSlot {
    i32 __fastcall Load(const char *name);
};

struct PlayerBombAnmManager {
    u8 unknown00[0x28EF0];
    void *scripts[];

    i32 LoadFile(i32 slot, const char *name, i32 scriptBase);
    void SetAndExecute(void *vm, void *script);
    void DrawPlayer(void *player);
    void DrawShadow(void *vm);
};

struct PlayerBombItemManager {
    void RemoveAllItems();
};

struct PlayerBombGrazeState {
    i32 IsBombStartBlocked();
    void MarkBombStart(i32 enabled);
    void SetBombItemState(i32 state);
    void SetBombEffectTimer(i32 timer);
};

struct PlayerBombGuiState {
    i32 IsBombInputBlocked();
    void StartDeathBombTransition(i32 first, i32 second);
};

struct PlayerBombDrawState {
    void SetDrawColor(i32 color);
};

/* The death-bomb paths only access these target-observed effect offsets. */
struct PlayerBombEffect {
    u8 unknown00[0x14];
    f32 positionZ;
    u8 unknown18[0x48];
    PlayerBombTimer timer60;
    u8 unknown6C[0x0C];
    PlayerBombTimer timer78;
    u8 unknown84[0x18];
    PlayerBombTimer timer9C;
    u8 unknownA8[0x0C];
    PlayerBombTimer timerB4;
    u8 unknownC0[2];
    u8 enabledC2;
    u8 unknownC3;
    u8 enabledC4;
    u8 unknownC5[3];
    i32 timerC8;
    u8 unknownCC[0xEF];
    u8 inheritedByte1BB;
    u8 unknown1BC[0x5C];
    f32 scaleX;
    f32 scaleY;
    f32 speedX;
    f32 speedY;
    u8 unknown228[3];
    u8 inheritedByte22B;
    u8 unknown22C[3];
    u8 state22F;
    u8 unknown230[0x64];
    f32 radialCos;
    f32 radialSin;
};

struct EffectManager {
    void *SpawnParticles(i32 effect, D3DXVECTOR3 *position, i32 count, i32 color);
    void *SpawnParticlesColored(i32 effect, D3DXVECTOR3 *position, i32 count, i32 blendMode, i32 color);
};

struct GameManager {
    u8 unknown00[0x88];
    i32 bombScore;
};

struct SoundPlayer {
    i32 PlaySoundByIdx(i32 sound, i32 param);
};

extern PlayerBombScreenState *g_PlayerBombScreen;
extern PlayerBombAnmManager *g_PlayerBombAnmManager;
extern PlayerBombItemManager g_PlayerBombItemManager575C70;
extern PlayerBombGrazeState g_PlayerBombGrazeState626270;
extern PlayerBombGuiState g_PlayerBombGuiState49FBF0;
extern PlayerBombDrawState g_PlayerBombDrawState1347B00;
extern u8 g_PlayerShotType;
extern i32 g_PlayerGameMode;
extern f32 g_PlayerInitialX;
extern f32 g_PlayerInitialY;
extern const char *g_PlayerUnfocusedShtFiles[6];
extern const char *g_PlayerFocusedShtFiles[6];
extern i16 g_TargetBossUi134DB5A[];
extern i16 g_PlayerBombHudActive;
extern i32 g_PlayerBombHudTimer;
extern i32 g_BombEffectState0;
extern i32 g_BombEffectState1;
extern i32 g_BombEffectState2;
extern i32 g_BombEffectState3;
extern EffectManager g_EffectManager;
extern GameManager *g_GameManager;
extern SoundPlayer g_SoundPlayer;

#pragma var_order(pointCount, pointStride, point, pointEnd, spriteBegin, spriteCount, spriteStride, sprite, workTimer)
PlayerBombWorkItem *PlayerBombWorkItem::Initialize()
{
    PlayerBombPoint *pointEnd;
    PlayerBombAnmVm *spriteBegin;
    i32 pointCount = 32;
    i32 pointStride = sizeof(PlayerBombPoint);
    PlayerBombPoint *point = points;

    while (--pointCount >= 0) {
        point->Initialize();
        point = reinterpret_cast<PlayerBombPoint *>(reinterpret_cast<u8 *>(point) + pointStride);
    }

    i32 spriteCount = 8;
    i32 spriteStride = sizeof(PlayerBombAnmVm);
    PlayerBombAnmVm *sprite = sprites;

    while (--spriteCount >= 0) {
        InitializeAnmVm(sprite);
        memset(sprite, 0, sizeof(PlayerBombAnmVm));
        sprite->scriptId = -1;
        sprite = reinterpret_cast<PlayerBombAnmVm *>(reinterpret_cast<u8 *>(sprite) + spriteStride);
    }

    PlayerBombTimer *workTimer = &timer;
    workTimer->current = 0;
    workTimer->previous = -999;
    workTimer->fraction = 0.0f;
    return this;
}

#pragma var_order(bombTimer, constructedWorkItem, workCount, workStride, workItem)
PlayerBombState *PlayerBombState::Initialize()
{
    PlayerBombWorkItem *constructedWorkItem;
    PlayerBombTimer *bombTimer = &tick;

    bombTimer->current = 0;
    bombTimer->previous = -999;
    bombTimer->fraction = 0.0f;

    i32 workCount = 128;
    i32 workStride = sizeof(PlayerBombWorkItem);
    PlayerBombWorkItem *workItem = workItems;

    while (--workCount >= 0) {
        workItem->Initialize();
        workItem = reinterpret_cast<PlayerBombWorkItem *>(reinterpret_cast<u8 *>(workItem) + workStride);
    }

    return this;
}

#pragma var_order(secondaryVmCount, secondaryVmStride, secondaryVm, orbitPointCount, orbitPointStride, orbitPoint, damageRegionCount, damageRegionStride, damageRegion, playerTimer, constructedBullet, bulletCount, bulletStride, bullet, timerCount, timerStride, timer, lastTimer, lastEnemyTimer, bombTimer, constructedBombState, bombStateEnd, playerBulletEnd, damageRegionEnd, secondaryVmEnd)
PlayerBombOverlay *PlayerBombOverlay::Initialize()
{
    u8 *constructedBullet;
    PlayerBombState *constructedBombState;
    PlayerBombState *bombStateEnd;
    u8 *playerBulletEnd;
    PlayerBombDamageRegion *damageRegionEnd;
    PlayerBombAnmVm *secondaryVmEnd;
    InitializeAnmVm(this);
    memset(this, 0, 0x24C);
    *reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(this) + 0x1D4) = -1;

    i32 secondaryVmCount = 3;
    i32 secondaryVmStride = sizeof(PlayerBombAnmVm);
    PlayerBombAnmVm *secondaryVm = reinterpret_cast<PlayerBombAnmVm *>(reinterpret_cast<u8 *>(this) + 0x24C);

    while (--secondaryVmCount >= 0) {
        InitializeSecondaryAnmVm(secondaryVm);
        secondaryVm = reinterpret_cast<PlayerBombAnmVm *>(reinterpret_cast<u8 *>(secondaryVm) + secondaryVmStride);
    }

    i32 orbitPointCount = 2;
    i32 orbitPointStride = sizeof(PlayerBombPoint);
    PlayerBombPoint *orbitPoint = reinterpret_cast<PlayerBombPoint *>(reinterpret_cast<u8 *>(this) + 0x9B4);

    while (--orbitPointCount >= 0) {
        orbitPoint->Initialize();
        orbitPoint = reinterpret_cast<PlayerBombPoint *>(reinterpret_cast<u8 *>(orbitPoint) + orbitPointStride);
    }

    i32 damageRegionCount = 112;
    i32 damageRegionStride = sizeof(PlayerBombDamageRegion);
    PlayerBombDamageRegion *damageRegion = reinterpret_cast<PlayerBombDamageRegion *>(reinterpret_cast<u8 *>(this) + 0x9DC);

    while (--damageRegionCount >= 0) {
        damageRegion->Initialize();
        damageRegion = reinterpret_cast<PlayerBombDamageRegion *>(reinterpret_cast<u8 *>(damageRegion) + damageRegionStride);
    }

    PlayerBombTimer *playerTimer = reinterpret_cast<PlayerBombTimer *>(reinterpret_cast<u8 *>(this) + 0x2410);
    playerTimer->current = 0;
    playerTimer->previous = -999;
    playerTimer->fraction = 0.0f;

    i32 bulletCount = 96;
    i32 bulletStride = 0x364;
    u8 *bullet = reinterpret_cast<u8 *>(this) + 0x2444;

    while (--bulletCount >= 0) {
        InitializePlayerBullet(bullet);
        bullet += bulletStride;
    }

    i32 timerCount = 3;
    i32 timerStride = 0x10;
    PlayerBombTimer *timer = reinterpret_cast<PlayerBombTimer *>(reinterpret_cast<u8 *>(this) + 0x169C4);

    while (--timerCount >= 0) {
        timer->current = 0;
        timer->previous = -999;
        timer->fraction = 0.0f;
        timer = reinterpret_cast<PlayerBombTimer *>(reinterpret_cast<u8 *>(timer) + timerStride);
    }

    PlayerBombTimer *lastTimer = reinterpret_cast<PlayerBombTimer *>(reinterpret_cast<u8 *>(this) + 0x169F4);
    lastTimer->current = 0;
    lastTimer->previous = -999;
    lastTimer->fraction = 0.0f;

    PlayerBombTimer *lastEnemyTimer = reinterpret_cast<PlayerBombTimer *>(reinterpret_cast<u8 *>(this) + 0x16A00);
    lastEnemyTimer->current = 0;
    lastEnemyTimer->previous = -999;
    lastEnemyTimer->fraction = 0.0f;

    PlayerBombTimer *bombTimer = reinterpret_cast<PlayerBombTimer *>(reinterpret_cast<u8 *>(this) + 0x16A0C);
    bombTimer->current = 0;
    bombTimer->previous = -999;
    bombTimer->fraction = 0.0f;

    BombState()->Initialize();
    return this;
}

#pragma var_order(index, bullet, anmManager, playerTimer, firstSecondaryVm, firstSecondaryAnmManager, secondSecondaryVm, secondSecondaryAnmManager, bombCooldown)
i32 PlayerBombOverlay::RegisterBombCallbacks()
{
    if (reinterpret_cast<PlayerBombShtSlot *>(reinterpret_cast<u8 *>(this) + 0xB7E70)
            ->Load(g_PlayerUnfocusedShtFiles[g_PlayerShotType])) {
        return -1;
    }
    if (reinterpret_cast<PlayerBombShtSlot *>(reinterpret_cast<u8 *>(this) + 0xB7E74)
            ->Load(g_PlayerFocusedShtFiles[g_PlayerShotType])) {
        return -1;
    }

    {
        i32 loadPlayerAnm;
        if (g_PlayerGameMode != 3 && g_PlayerGameMode != 11 && g_PlayerGameMode != 12) {
            loadPlayerAnm = 1;
        } else {
            loadPlayerAnm = 0;
        }
        if (loadPlayerAnm) {
            u8 selectedShot = g_PlayerShotType;

            if (selectedShot == 0) {
                if (g_PlayerBombAnmManager->LoadFile(10, "data/player00.anm", 1024)) {
                    return -1;
                }
            } else if (selectedShot == 1) {
                if (g_PlayerBombAnmManager->LoadFile(10, "data/player01.anm", 1024)) {
                    return -1;
                }
            } else if (selectedShot == 2) {
                if (g_PlayerBombAnmManager->LoadFile(10, "data/player02.anm", 1024)) {
                    return -1;
                }
            }
        }
    }

    PlayerBombAnmManager *anmManager = g_PlayerBombAnmManager;
    *reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(this) + 0x1D8) = 1024;
    anmManager->SetAndExecute(this, anmManager->scripts[1024]);

    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x930) = g_PlayerInitialX / 2.0f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x934) = g_PlayerInitialY - 64.0f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x938) = 0.49f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x9BC) = 0.49f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x9C8) = 0.49f;

    i32 index;
    for (index = 0; index < 128; ++index) {
        *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x9E8 + index * 0x20) = 0;
    }

    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x994) = g_PlayerBombScreen->width / 2.0f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x990) = *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x994);
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x998) = 5.0f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x9A0) = g_PlayerBombScreen->height / 2.0f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x99C) = *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x9A0);
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x9A4) = 5.0f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x9A8) = 12.0f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x9AC) = 12.0f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x9B0) = 5.0f;

    *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x241C) = 0;
    *reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x2408) = 1;
    PlayerBombTimer *playerTimer = reinterpret_cast<PlayerBombTimer *>(reinterpret_cast<u8 *>(this) + 0x16A00);
    playerTimer->current = 120;
    playerTimer->fraction = 0.0f;
    playerTimer->previous = -999;
    *reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x240A) = 1;

    u8 *firstSecondaryVm = reinterpret_cast<u8 *>(this) + 0x24C;
    PlayerBombAnmManager *firstSecondaryAnmManager = g_PlayerBombAnmManager;
    *reinterpret_cast<i16 *>(firstSecondaryVm + 0x1D8) = 1152;
    firstSecondaryAnmManager->SetAndExecute(firstSecondaryVm, firstSecondaryAnmManager->scripts[1152]);
    u8 *secondSecondaryVm = reinterpret_cast<u8 *>(this) + 0x498;
    PlayerBombAnmManager *secondSecondaryAnmManager = g_PlayerBombAnmManager;
    *reinterpret_cast<i16 *>(secondSecondaryVm + 0x1D8) = 1153;
    secondSecondaryAnmManager->SetAndExecute(secondSecondaryVm, secondSecondaryAnmManager->scripts[1153]);

    u8 *bullet = reinterpret_cast<u8 *>(this) + 0x2444;
    for (index = 0; index < 96; ++index, bullet += 0x364) {
        *reinterpret_cast<i16 *>(bullet + 0x34A) = 0;
    }

    PlayerBombTimer *bombCooldown = reinterpret_cast<PlayerBombTimer *>(reinterpret_cast<u8 *>(this) + 0x169F4);
    bombCooldown->current = -1;
    bombCooldown->fraction = 0.0f;
    bombCooldown->previous = -999;

    BombState()->calculateUnfocused = g_PlayerBombCallbacks[g_PlayerShotType][0];
    BombState()->drawUnfocused = g_PlayerBombCallbacks[g_PlayerShotType][1];
    BombState()->calculateFocused = g_PlayerBombCallbacks[g_PlayerShotType][2];
    BombState()->drawFocused = g_PlayerBombCallbacks[g_PlayerShotType][3];
    BombState()->active = 0;
    rotationAngle = -1.57079637f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x23F4) = 1.0f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x23F0) = 1.0f;
    *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x23F8) = g_PlayerBombScreen->bombCapacity;

    {
        i32 showBombHud;
        if (g_PlayerGameMode != 3 && g_PlayerGameMode != 11 && g_PlayerGameMode != 12) {
            showBombHud = 1;
        } else {
            showBombHud = 0;
        }
        if (showBombHud) {
            g_PlayerBombHudActive = 1;
            g_PlayerBombHudTimer = 1;
        }
    }

    g_TargetBossUi134DB5A[0] = 2;
    g_TargetBossUi134DB5A[294] = 2;
    g_TargetBossUi134DB5A[588] = 2;

    if (g_BombScore >= g_PlayerBombResources->scoreFloor + 50000) {
        g_BombScore = g_PlayerBombResources->scoreFloor + 50000;
        FinishDeathBomb();
    }
    return 0;
}

#pragma var_order(effect, lastEnemyTimer, effectTimerA, lastEnemyZ, effectTimerB, secondLastEnemyZ)
void PlayerBombOverlay::FinishDeathBomb()
{
    i32 secondLastEnemyZ;
    PlayerBombTimer *effectTimerB;
    i32 lastEnemyZ;
    PlayerBombTimer *effectTimerA;
    PlayerBombTimer *lastEnemyTimer;
    PlayerBombEffect *effect;

    if (*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x16A20)
        || g_PlayerBombGuiState49FBF0.IsBombInputBlocked()) {
        *reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x240D) = 2;
        return;
    }

    {
        switch (*reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x2408)) {
        case 1:
        case 3:
            *reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x240D) = 2;
            return;
        case 2:
            if (*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x23F8)) {
                StartDeathBomb(0);
                return;
            } else {
                *reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x240D) = 2;
                return;
            }
        default:
            break;
        }
    }

    lastEnemyTimer = reinterpret_cast<PlayerBombTimer *>(reinterpret_cast<u8 *>(this) + 0x16A00);
    lastEnemyTimer->current = 540;
    lastEnemyTimer->fraction = 0.0f;
    lastEnemyTimer->previous = -999;
    *reinterpret_cast<PlayerBombTimer *>(reinterpret_cast<u8 *>(this) + 0x16A0C)
        = *reinterpret_cast<PlayerBombTimer *>(reinterpret_cast<u8 *>(this) + 0x16A00);
    *reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x240D) = 1;
    *reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x2408) = 4;

    if (bombVisual1) {
        reinterpret_cast<u8 *>(bombVisual1)[0x2CC] = 0;
    }
    if (bombVisual0) {
        reinterpret_cast<u8 *>(bombVisual0)[0x2CC] = 0;
        bombVisual0 = 0;
    }

    effect = reinterpret_cast<PlayerBombEffect *>(g_EffectManager.SpawnParticlesColored(
        28, reinterpret_cast<D3DXVECTOR3 *>(reinterpret_cast<u8 *>(this) + 0x930), 4, 1, -1));
    effectTimerA = &effect->timer78;
    effectTimerA->current = 0;
    effectTimerA->fraction = 0.0f;
    effectTimerA->previous = -999;
    lastEnemyZ = *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x16A08);
    effectTimerB = &effect->timerB4;
    effectTimerB->current = lastEnemyZ;
    effectTimerB->fraction = 0.0f;
    effectTimerB->previous = -999;
    effect->enabledC4 = 0;
    effect->scaleY = 1.0f;
    effect->scaleX = 1.0f;
    effect->speedX = 0.25f;
    effect->speedY = 0.25f;
    secondLastEnemyZ = *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x16A08);
    effect->timerC8 = secondLastEnemyZ;
    effect->positionZ *= -1.0f;
    bombVisual1 = effect;

    g_PlayerBombGuiState49FBF0.StartDeathBombTransition(0, 2);
    g_SoundPlayer.PlaySoundByIdx(32, 0);
    g_SoundPlayer.PlaySoundByIdx(36, 0);
    reinterpret_cast<PlayerBombInputState *>(g_PlayerCollisionFlags)->flags |= 8;
}

#pragma var_order(playerTimer, effectTimer9C, effectTimer60, effectTimerB, effectTimer78, angle, index, effect)
void PlayerBombOverlay::StartDeathBomb(i32)
{
    PlayerBombTimer *playerTimer;
    PlayerBombTimer *effectTimer9C;
    PlayerBombTimer *effectTimer60;
    PlayerBombTimer *effectTimerB;
    PlayerBombTimer *effectTimer78;
    f32 angle;
    i32 index;
    PlayerBombEffect *effect;

    if (bombVisual1) {
        reinterpret_cast<u8 *>(bombVisual1)[0x2CC] = 0;
        bombVisual1 = 0;
    }

    effect = reinterpret_cast<PlayerBombEffect *>(g_EffectManager.SpawnParticlesColored(
        28, reinterpret_cast<D3DXVECTOR3 *>(reinterpret_cast<u8 *>(this) + 0x930), 4, 1, -1));
    effectTimer78 = &effect->timer78;
    effectTimer78->current = 0;
    effectTimer78->fraction = 0.0f;
    effectTimer78->previous = -999;
    effectTimerB = &effect->timerB4;
    effectTimerB->current = 30;
    effectTimerB->fraction = 0.0f;
    effectTimerB->previous = -999;
    effect->enabledC4 = 0;
    effect->speedX = 0.0625f;
    effect->speedY = 0.0625f;
    effect->scaleX = 1.3f;
    effect->scaleY = 1.3f;
    effectTimer60 = &effect->timer60;
    effectTimer60->current = 0;
    effectTimer60->fraction = 0.0f;
    effectTimer60->previous = -999;
    effectTimer9C = &effect->timer9C;
    effectTimer9C->current = 30;
    effectTimer9C->fraction = 0.0f;
    effectTimer9C->previous = -999;
    effect->enabledC2 = 1;
    effect->inheritedByte22B = effect->inheritedByte1BB;
    effect->state22F = 0;
    effect->timerC8 = 30;
    bombVisual1 = effect;

    g_BombEffectState0 = 0;
    g_BombEffectState1 = 0;
    *reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x240D) = 0;
    *reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x2408) = 3;
    playerTimer = reinterpret_cast<PlayerBombTimer *>(reinterpret_cast<u8 *>(this) + 0x16A00);
    playerTimer->current = 40;
    playerTimer->fraction = 0.0f;
    playerTimer->previous = -999;
    *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x23FC) = 40;
    g_BombScore = g_GameManager->bombScore;

    AddBombProjectileCircle(reinterpret_cast<PlayerBombPoint *>(reinterpret_cast<u8 *>(this) + 0x930), 32.0f, 16.0f,
                            50, 8);
    angle = -3.14159274f;
    for (index = 0; index < 32; ++index, angle += 0.196349546f) {
        f32 radialCos;
        f32 radialSin;

        effect = reinterpret_cast<PlayerBombEffect *>(g_EffectManager.SpawnParticles(
            29, reinterpret_cast<D3DXVECTOR3 *>(reinterpret_cast<u8 *>(this) + 0x930), 1, -1));
        radialCos = (f32)cos(angle);
        effect->radialCos = radialCos;
        radialSin = (f32)sin(angle);
        effect->radialSin = radialSin;
    }

    g_SoundPlayer.PlaySoundByIdx(7, 0);
    g_SoundPlayer.PlaySoundByIdx(33, 0);
    reinterpret_cast<PlayerBombInputState *>(g_PlayerCollisionFlags)->flags |= 16;
}

PlayerBombProjectile *PlayerBombOverlay::AddBombProjectileRectangle(const PlayerBombPoint *center, f32 sizeX,
                                                                    f32 sizeY, u32 collisionValue)
{
    PlayerBombProjectile *projectile = bombProjectiles;
    i32 index;

    for (index = 0; index < 95; ++index, ++projectile) {
        if (projectile->sizeX == 0.0f && projectile->radius == 0.0f) {
            break;
        }
    }
    projectile->centerX = center->x;
    projectile->centerY = center->y;
    projectile->sizeX = sizeX;
    projectile->sizeY = sizeY;
    projectile->unknown18 = 0;
    projectile->collisionValue = collisionValue;
    return projectile;
}

PlayerBombProjectile *PlayerBombOverlay::AddBombProjectileCircle(const PlayerBombPoint *center, f32 radius,
                                                                 f32 unknown14, u32 unknown18,
                                                                 u32 collisionValue)
{
    PlayerBombProjectile *projectile = bombProjectiles;
    i32 index;

    for (index = 0; index < 95; ++index, ++projectile) {
        if (projectile->sizeX == 0.0f && projectile->radius == 0.0f) {
            break;
        }
    }
    projectile->centerX = center->x;
    projectile->centerY = center->y;
    projectile->radius = radius;
    projectile->unknown14 = unknown14;
    projectile->unknown18 = unknown18;
    projectile->collisionValue = collisionValue;
    return projectile;
}

#pragma var_order(startingBombTimer, scoreCost, activeBombTimer)
void PlayerBombOverlay::BeginBomb()
{
    if (*reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x240D)
        && !*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x16A20)
        && (g_PlayerInputButtons & 2)) {
        StartDeathBomb(1);
        *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x23DC) = 0;
        g_PlayerBombItemManager575C70.RemoveAllItems();
        return;
    }

    if (*reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x240D) == 2) {
        FinishDeathBomb();
    }

    if (*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x23FC)) {
        --*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x23FC);
    }

    if (*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x16A20)) {
        PlayerBombTimer *activeBombTimer = reinterpret_cast<PlayerBombTimer *>(reinterpret_cast<u8 *>(this) + 0x16A30);
        if (activeBombTimer->current != activeBombTimer->previous ? 1 : 0) {
            i32 scoreCost = *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x16A2C);

            if (g_BombScore - g_PlayerBombResources->scoreFloor >= scoreCost) {
                g_BombScore -= scoreCost;
            } else {
                g_BombScore = g_PlayerBombResources->scoreFloor;
            }
            g_PlayerFlags = (g_PlayerFlags & 0xFFFFFCFF) | 0x200;
        }

        if (!*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x16A24)) {
            (*reinterpret_cast<PlayerBombCallback *>(reinterpret_cast<u8 *>(this) + 0x16A3C))(this);
        } else {
            (*reinterpret_cast<PlayerBombCallback *>(reinterpret_cast<u8 *>(this) + 0x16A44))(this);
        }
        return;
    }

    if (!g_PlayerBombGrazeState626270.IsBombStartBlocked()
        && !g_PlayerBombGuiState49FBF0.IsBombInputBlocked()
        && *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x23F8)
        && static_cast<i32>(g_PlayerBombResources->bombStock) > 0
        && !*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x23FC)
        && (g_PlayerInputButtons & 2)) {
        reinterpret_cast<PlayerBombInputState *>(g_PlayerCollisionFlags)->flags |= 1;
        g_PlayerBombGrazeState626270.MarkBombStart(1);
        g_PlayerBombGrazeState626270.SetBombItemState(-1);
        g_PlayerFlags = (g_PlayerFlags & 0xFFFFFFF3) | 8;

        *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x16A24) = *reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x240B);
        *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x16A20) = 1;
        *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x23DC) = 1;
        PlayerBombTimer *startingBombTimer = reinterpret_cast<PlayerBombTimer *>(reinterpret_cast<u8 *>(this) + 0x16A30);
        startingBombTimer->current = 0;
        startingBombTimer->fraction = 0.0f;
        startingBombTimer->previous = -999;
        *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x16A28) = 999;

        if (!*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x16A24)) {
            (*reinterpret_cast<PlayerBombCallback *>(reinterpret_cast<u8 *>(this) + 0x16A3C))(this);
        } else {
            (*reinterpret_cast<PlayerBombCallback *>(reinterpret_cast<u8 *>(this) + 0x16A44))(this);
        }

        g_BombEffectState0 = 0;
        g_BombEffectState1 = 0;
        g_PlayerBombGrazeState626270.SetBombEffectTimer(200);
        g_BombEffectState2 = g_BombEffectState3;

        *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x23F8) += 6;
        if (*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x23F8) > g_PlayerBombScreen->bombCapacity) {
            *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x23F8) = g_PlayerBombScreen->bombCapacity;
        }
    } else {
        *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x23DC) = 0;
    }
}

void PlayerBombOverlay::UpdateBomb()
{
    PlayerBombState *bomb = BombState();

    if (!bomb->active) {
        return;
    }

    if (bomb->capturedFocus) {
        bomb->calculateFocused(this);
    } else {
        bomb->calculateUnfocused(this);
    }
}

#pragma var_order(color, bombTimer, shadowTimer, flashTimer)
i32 PlayerBombOverlay::DrawBomb()
{
    DrawPlayerBullets();

    if (*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x16A20)) {
        if (!*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x16A24)) {
            (*reinterpret_cast<PlayerBombCallback *>(reinterpret_cast<u8 *>(this) + 0x16A40))(this);
        } else {
            (*reinterpret_cast<PlayerBombCallback *>(reinterpret_cast<u8 *>(this) + 0x16A48))(this);
        }
    }

    if (!g_HidePlayerDraw) {
        *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x1C8) = g_PlayerDrawOffsetX + *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x930);
        *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x1CC) = g_PlayerDrawOffsetY + *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x934);
        *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x1D0) = 0;
        g_PlayerBombAnmManager->DrawPlayer(this);

        if (*reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x240A)
            && (!*reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x2408)
                || *reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x2408) == 4
                || *reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x2408) == 3)) {
            *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x414) = g_PlayerDrawOffsetX + *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x9B4);
            *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x418) = g_PlayerDrawOffsetY + *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x9B8);
            *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x41C) = 0;
            *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x660) = g_PlayerDrawOffsetX + *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x9C0);
            *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x664) = g_PlayerDrawOffsetY + *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x9C4);
            *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x668) = 0;
            g_PlayerBombAnmManager->DrawShadow(reinterpret_cast<u8 *>(this) + 0x24C);
            g_PlayerBombAnmManager->DrawShadow(reinterpret_cast<u8 *>(this) + 0x498);
        }
    }

    PlayerBombDrawColor color;
    i32 bombTimer;
    if (*reinterpret_cast<i8 *>(reinterpret_cast<u8 *>(this) + 0x2408) == 4) {
        bombTimer = *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x16A08);
        if (bombTimer > 0) {
            i32 shadowTimer = *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x16A08);

            if (shadowTimer % 4 < 2) {
                *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x1B8) = -65536;
            } else {
                *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x1B8) = -1;
            }

            color.alpha = 0x80;
            if (g_BombDrawTimer >= 510 ? 1 : 0) {
                i32 flashTimer = g_BombDrawTimer;
                color.blue = 0x80 - 80 * (540 - flashTimer) / 30;
                color.green = color.blue;
                color.red = color.green;
            } else if (g_BombDrawTimer < 30 ? 1 : 0) {
                i32 flashTimer = g_BombDrawTimer;
                color.blue = 0x80 - 80 * flashTimer / 30;
                color.green = color.blue;
                color.red = color.green;
                } else {
                color.blue = 0x30;
                color.green = color.blue;
                color.red = color.green;
            }
            g_PlayerBombDrawState1347B00.SetDrawColor(*reinterpret_cast<i32 *>(&color));
        }
    }
    return 1;
}

} // namespace th07
