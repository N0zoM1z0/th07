#include "inttypes.hpp"

#include <string.h>

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
    u8 unknown0000[0x16A20];
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
    i32 Load(const char *name);
};

struct PlayerBombAnmManager {
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
};

struct PlayerBombDrawState {
    void SetDrawColor(i32 color);
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
extern void *g_PlayerAnmScripts[];
extern i16 g_PlayerBombHudActive;
extern i32 g_PlayerBombHudTimer;
extern i32 g_BombEffectState0;
extern i32 g_BombEffectState1;
extern i32 g_BombEffectState2;
extern i32 g_BombEffectState3;

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

    PlayerBombAnmManager *anmManager = g_PlayerBombAnmManager;
    *reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(this) + 0x1D8) = 1024;
    anmManager->SetAndExecute(this, g_PlayerAnmScripts[1024]);

    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x930) = g_PlayerInitialX / 2.0f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x934) = g_PlayerInitialY - 64.0f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x938) = 0.5f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x9BC) = 0.5f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x9C8) = 0.5f;

    {
        i32 damageRegionIndex = 0;
        while (damageRegionIndex < 128) {
            *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x9E8 + damageRegionIndex * 0x20) = 0;
            ++damageRegionIndex;
        }
    }

    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x990) = g_PlayerBombScreen->width / 2.0f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x994) = *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x990);
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x998) = 5.0f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x99C) = g_PlayerBombScreen->height / 2.0f;
    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x9A0) = *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(this) + 0x99C);
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

    PlayerBombAnmVm *firstSecondaryVm = reinterpret_cast<PlayerBombAnmVm *>(reinterpret_cast<u8 *>(this) + 0x24C);
    PlayerBombAnmManager *firstSecondaryAnmManager = g_PlayerBombAnmManager;
    firstSecondaryVm->scriptId = 1152;
    firstSecondaryAnmManager->SetAndExecute(firstSecondaryVm, g_PlayerAnmScripts[1152]);
    PlayerBombAnmVm *secondSecondaryVm = reinterpret_cast<PlayerBombAnmVm *>(reinterpret_cast<u8 *>(this) + 0x498);
    PlayerBombAnmManager *secondSecondaryAnmManager = g_PlayerBombAnmManager;
    secondSecondaryVm->scriptId = 1153;
    secondSecondaryAnmManager->SetAndExecute(secondSecondaryVm, g_PlayerAnmScripts[1153]);

    {
        u8 *bullet = reinterpret_cast<u8 *>(this) + 0x2444;
        i32 bulletCount = 0;
        while (bulletCount < 96) {
            *reinterpret_cast<i16 *>(bullet + 0x34A) = 0;
            ++bulletCount;
            bullet += 0x364;
        }
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

    if (g_BombScore >= g_PlayerBombResources->scoreFloor + 50000) {
        g_BombScore = g_PlayerBombResources->scoreFloor + 50000;
        FinishDeathBomb();
    }
    return 0;
}

#pragma var_order(activeBombTimer, scoreCost, startingBombTimer)
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
        if (activeBombTimer->current != activeBombTimer->previous) {
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

    if (g_PlayerBombGrazeState626270.IsBombStartBlocked()
        || g_PlayerBombGuiState49FBF0.IsBombInputBlocked()
        || !*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x23F8)
        || static_cast<i32>(g_PlayerBombResources->bombStock) <= 0
        || *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x23FC)
        || !(g_PlayerInputButtons & 2)) {
        *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x23DC) = 0;
        return;
    }

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
            if (!!(g_BombDrawTimer >= 510)) {
                i32 flashTimer = g_BombDrawTimer;
                color.blue = 0x80 - 80 * (540 - flashTimer) / 30;
                color.green = color.blue;
                color.red = color.green;
            } else if (!!(g_BombDrawTimer < 30)) {
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
