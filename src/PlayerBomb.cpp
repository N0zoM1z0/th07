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
    void RegisterBombCallbacks(u8 shotType);
    void BeginBomb(i32 focused);
    void UpdateBomb();
    void DrawBomb();
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

void PlayerBombOverlay::RegisterBombCallbacks(u8 shotType)
{
    PlayerBombState *bomb = BombState();

    bomb->calculateUnfocused = g_PlayerBombCallbacks[shotType][0];
    bomb->drawUnfocused = g_PlayerBombCallbacks[shotType][1];
    bomb->calculateFocused = g_PlayerBombCallbacks[shotType][2];
    bomb->drawFocused = g_PlayerBombCallbacks[shotType][3];
    bomb->active = 0;
}

void PlayerBombOverlay::BeginBomb(i32 focused)
{
    PlayerBombState *bomb = BombState();

    bomb->capturedFocus = focused;
    bomb->active = 1;
    bomb->unknown08 = 999;
    bomb->tick.previous = -999;
    bomb->tick.fraction = 0.0f;
    bomb->tick.current = 0;

    if (bomb->capturedFocus) {
        bomb->calculateFocused(this);
    } else {
        bomb->calculateUnfocused(this);
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

void PlayerBombOverlay::DrawBomb()
{
    const PlayerBombState *bomb = BombState();

    if (!bomb->active) {
        return;
    }

    if (bomb->capturedFocus) {
        bomb->drawFocused(this);
    } else {
        bomb->drawUnfocused(this);
    }
}

} // namespace th07
