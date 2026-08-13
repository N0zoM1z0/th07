#include "inttypes.hpp"

#include <math.h>

#pragma intrinsic(sin, cos)

namespace th07
{

/*
 * These overlays are local to 0x00422170.  They deliberately do not describe
 * the shared Enemy or ANM types: every offset here is directly read or written
 * by the target draw routine.
 */
struct EnemyRenderVec3
{
    f32 x;
    f32 y;
    f32 z;
};

struct EnemyRenderSprite
{
    u8 unknown00[0x1C];
    f32 uvLeft;
    f32 uvTop;
    f32 uvRight;
    f32 uvBottom;
    f32 trailUOffset;
    f32 trailVOffset;
};

struct EnemyRenderVm
{
    u8 unknown00[0x08];
    f32 rotationZ;
    u8 unknown0C[0x0C];
    f32 scaleX;
    f32 scaleY;
    u8 unknown20[8];
    f32 textureUOffset;
    f32 textureVOffset;
    u8 unknown30[0x188];
    u32 color;
    u8 unknown1BC[4];
    u32 flags;
    i16 rotationEnabled;
    u8 unknown1C6[2];
    EnemyRenderVec3 position;
    u8 unknown1D4[4];
    i16 scriptIndex;
    u8 unknown1DA[0x0A];
    EnemyRenderSprite *sprite;
    u8 unknown1E8[0x48];
    EnemyRenderVec3 positionOffset;
    u8 unknown23C[0x10];
};
typedef char EnemyRenderVm_size[(sizeof(EnemyRenderVm) == 0x24C) ? 1 : -1];

struct EnemyRenderTrailSample
{
    EnemyRenderVec3 position;
    u8 unknown0C[0x0C];
    f32 angle;
};
typedef char EnemyRenderTrailSample_size[(sizeof(EnemyRenderTrailSample) == 0x1C) ? 1 : -1];

struct VertexDiffuseXyzrhw
{
    f32 x;
    f32 y;
    f32 z;
    f32 rhw;
    u32 diffuse;
    f32 u;
    f32 v;
};
typedef char VertexDiffuseXyzrhw_size[(sizeof(VertexDiffuseXyzrhw) == 0x1C) ? 1 : -1];

struct EnemyRenderEnemy
{
    u8 raw[0x4F48];

    EnemyRenderVm *PrimaryVm()
    {
        return reinterpret_cast<EnemyRenderVm *>(raw);
    }

    EnemyRenderVm *AuxiliaryVm(i32 index)
    {
        return reinterpret_cast<EnemyRenderVm *>(raw + 0x24C + 0x24C * index);
    }

    EnemyRenderVec3 *Position()
    {
        return reinterpret_cast<EnemyRenderVec3 *>(raw + 0x2B0C);
    }

    f32 &Angle() { return *reinterpret_cast<f32 *>(raw + 0x2B54); }
    u8 &DeathFlags() { return *reinterpret_cast<u8 *>(raw + 0x2E2A); }
    u8 &UpdateFlags() { return *reinterpret_cast<u8 *>(raw + 0x2E2B); }
    EnemyRenderTrailSample *TrailSample(i32 index)
    {
        return reinterpret_cast<EnemyRenderTrailSample *>(raw + 0x2F78 + 0x1C * index);
    }
    VertexDiffuseXyzrhw *TrailVertices()
    {
        return reinterpret_cast<VertexDiffuseXyzrhw *>(raw + 0x39F8);
    }
    u8 &TrailFlags() { return *reinterpret_cast<u8 *>(raw + 0x4F30); }
    i16 &TrailHistoryCount() { return *reinterpret_cast<i16 *>(raw + 0x4F32); }
    i16 &TrailStep() { return *reinterpret_cast<i16 *>(raw + 0x4F36); }
    EnemyRenderEnemy *&DrawNext()
    {
        return *reinterpret_cast<EnemyRenderEnemy **>(raw + 0x4F44);
    }
};
typedef char EnemyRenderEnemy_size[(sizeof(EnemyRenderEnemy) == 0x4F48) ? 1 : -1];

struct EnemyRenderAnmManager
{
    void Draw3(EnemyRenderVm *vm);
    i32 DrawTriangleFan(EnemyRenderVm *vm, VertexDiffuseXyzrhw *vertices, i32 vertexCount);
};

extern EnemyRenderAnmManager *g_EnemyRenderAnmManager;
extern f32 g_EnemyRenderOffsetX;
extern f32 g_EnemyRenderOffsetY;

// Private overlay for the two target-confirmed EnemyManager draw-chain
// adapters.  The manager layout is not needed by either wrapper.
struct EnemyManagerRenderOverlay
{
    u8 unknown00[0x954598];
    void *bosses[8];

    i32 __fastcall DrawImpl(i32 drawGroup, i32 chainPriority);
    i32 OnDrawHighPriority();
    i32 OnDrawLowPriority();
    i32 HasActiveBoss();

    EnemyRenderEnemy **DrawHeads()
    {
        return reinterpret_cast<EnemyRenderEnemy **>(reinterpret_cast<u8 *>(this) + 0x954700);
    }
};

#pragma var_order(wrappedDistance, directDistance)
f32 __stdcall InterpolateWrappedAngle(f32 start, f32 end, f32 fraction)
{
    f32 directDistance;
    f32 wrappedDistance;

    if (start < end)
    {
        directDistance = end - start;
        wrappedDistance = start + 6.2831855f - end;
    }
    else
    {
        directDistance = start - end;
        wrappedDistance = end + 6.2831855f - start;
        start = end;
    }

    if (directDistance < wrappedDistance)
        return directDistance * fraction + start;
    return wrappedDistance * fraction + start;
}

i32 EnemyManagerRenderOverlay::OnDrawHighPriority()
{
    return DrawImpl(0, 2);
}

i32 EnemyManagerRenderOverlay::OnDrawLowPriority()
{
    return DrawImpl(2, 4);
}

#pragma var_order(vertexCount, widthFactor, u, previousAngle, color, oldScaleY, oldScaleX, sampleIndex, vmIndex, enemy, drawGroup)
i32 __fastcall EnemyManagerRenderOverlay::DrawImpl(i32 drawGroup, i32 groupEnd)
{
    EnemyRenderEnemy *enemy;
    i32 vmIndex;
    i32 sampleIndex;
    f32 oldScaleX;
    f32 oldScaleY;
    u32 color;
    f32 previousAngle;
    f32 u;
    f32 widthFactor;
    f32 halfWidth;
    i32 vertexCount;
    VertexDiffuseXyzrhw *vertex;

    for (; drawGroup < groupEnd; ++drawGroup)
    {
        enemy = DrawHeads()[drawGroup];
        while (enemy)
        {
            // The first auxiliary VM precedes the primary VM in the target.
            for (vmIndex = 0; vmIndex < 1; ++vmIndex)
            {
                EnemyRenderVm *vm = enemy->AuxiliaryVm(vmIndex);
                if (vm->scriptIndex >= 0)
                {
                    if (vm->rotationEnabled)
                    {
                        vm->rotationZ = enemy->Angle();
                        vm->flags |= 4;
                    }
                    vm->position.x = enemy->Position()->x + vm->positionOffset.x + g_EnemyRenderOffsetX;
                    vm->position.y = enemy->Position()->y + vm->positionOffset.y + g_EnemyRenderOffsetY;
                    vm->position.z = enemy->Position()->z + vm->positionOffset.z;
                    vm->position.z = 0.3f;
                    g_EnemyRenderAnmManager->Draw3(vm);
                }
            }

            if (enemy->DeathFlags() & 0x10)
            {
                enemy->PrimaryVm()->rotationZ = enemy->Angle();
                enemy->PrimaryVm()->flags |= 4;
            }

            enemy->PrimaryVm()->position.x =
                enemy->Position()->x + enemy->PrimaryVm()->positionOffset.x;
            enemy->PrimaryVm()->position.y =
                enemy->Position()->y + enemy->PrimaryVm()->positionOffset.y;
            enemy->PrimaryVm()->position.z = 0.29f;
            if ((enemy->TrailFlags() & 0x10) == 0 && (enemy->UpdateFlags() & 4) == 0)
            {
                enemy->PrimaryVm()->position.x += g_EnemyRenderOffsetX;
                enemy->PrimaryVm()->position.y += g_EnemyRenderOffsetY;
            }
            g_EnemyRenderAnmManager->Draw3(enemy->PrimaryVm());

            // The second auxiliary VM is rotated in the opposite direction.
            for (vmIndex = 1; vmIndex < 2; ++vmIndex)
            {
                EnemyRenderVm *vm = enemy->AuxiliaryVm(vmIndex);
                if (vm->scriptIndex >= 0)
                {
                    if (vm->rotationEnabled)
                    {
                        vm->rotationZ = -enemy->Angle();
                        vm->flags |= 4;
                    }
                    vm->position.x = enemy->Position()->x + vm->positionOffset.x + g_EnemyRenderOffsetX;
                    vm->position.y = enemy->Position()->y + vm->positionOffset.y + g_EnemyRenderOffsetY;
                    vm->position.z = enemy->Position()->z + vm->positionOffset.z;
                    vm->position.z = 0.3f;
                    g_EnemyRenderAnmManager->Draw3(vm);
                }
            }

            if (enemy->TrailFlags())
            {
                oldScaleX = enemy->PrimaryVm()->scaleX;
                oldScaleY = enemy->PrimaryVm()->scaleY;
                color = enemy->PrimaryVm()->color;

                if ((enemy->TrailFlags() & 8) == 0)
                {
                    for (sampleIndex = enemy->TrailStep(); sampleIndex < enemy->TrailHistoryCount();
                         sampleIndex += enemy->TrailStep())
                    {
                        EnemyRenderTrailSample *sample = enemy->TrailSample(sampleIndex);
                        if (sample->position.x < -990.0f)
                            break;

                        if (enemy->DeathFlags() & 0x10)
                        {
                            enemy->PrimaryVm()->rotationZ = sample->angle;
                            enemy->PrimaryVm()->flags |= 4;
                        }
                        if (enemy->TrailFlags() & 2)
                            enemy->PrimaryVm()->scaleX = oldScaleX -
                                oldScaleX * (f32)sampleIndex / (f32)enemy->TrailHistoryCount();
                        if (enemy->TrailFlags() & 4)
                        {
                            u8 alpha = static_cast<u8>(color >> 24);
                            alpha = static_cast<u8>(alpha -
                                alpha * sampleIndex / enemy->TrailHistoryCount());
                            enemy->PrimaryVm()->color = (color & 0x00FFFFFF) | (static_cast<u32>(alpha) << 24);
                        }

                        enemy->PrimaryVm()->position.x = sample->position.x +
                            enemy->PrimaryVm()->positionOffset.x + g_EnemyRenderOffsetX;
                        enemy->PrimaryVm()->position.y = sample->position.y +
                            enemy->PrimaryVm()->positionOffset.y + g_EnemyRenderOffsetY;
                        enemy->PrimaryVm()->position.z = 0.3f;
                        g_EnemyRenderAnmManager->Draw3(enemy->PrimaryVm());
                    }
                }
                else
                {
                    vertexCount = 0;
                    for (sampleIndex = 0; sampleIndex < enemy->TrailHistoryCount();
                         sampleIndex += enemy->TrailStep())
                    {
                        if (enemy->TrailSample(sampleIndex)->position.x < -990.0f)
                            break;
                        vertexCount += 2;
                    }

                    if (vertexCount > 2)
                    {
                        widthFactor = (vertexCount + 1) / 2 - 1;
                        widthFactor /= enemy->PrimaryVm()->sprite->uvRight -
                            enemy->PrimaryVm()->sprite->uvLeft;
                        halfWidth = enemy->PrimaryVm()->sprite->trailVOffset * oldScaleY / 2.0f;
                        u = enemy->PrimaryVm()->sprite->uvRight +
                            enemy->PrimaryVm()->textureUOffset;
                        vertex = enemy->TrailVertices();

                        for (sampleIndex = 0; sampleIndex < enemy->TrailHistoryCount();
                             sampleIndex += enemy->TrailStep(), u -= widthFactor)
                        {
                            EnemyRenderTrailSample *sample = enemy->TrailSample(sampleIndex);
                            f32 angle;
                            f32 sine;
                            f32 cosine;
                            f32 fade;

                            if (sample->position.x < -990.0f)
                                break;

                            if (sampleIndex == 0 || (enemy->TrailFlags() & 2) == 0)
                                angle = sample->angle;
                            else
                                angle = InterpolateWrappedAngle(previousAngle, sample->angle, 0.5f);
                            previousAngle = angle;

                            sine = static_cast<f32>(sin(angle));
                            cosine = static_cast<f32>(cos(angle));
                            fade = 1.0f;
                            if (enemy->TrailFlags() & 2)
                                fade = 1.0f - (f32)sampleIndex / (f32)enemy->TrailHistoryCount();

                            vertex[0].x = sample->position.x + 32.0f - sine * (halfWidth * fade);
                            vertex[0].y = sample->position.y + 16.0f + cosine * (halfWidth * fade);
                            vertex[0].z = 0.0f;
                            vertex[0].rhw = 1.0f;
                            vertex[0].diffuse = color;
                            vertex[0].u = u;
                            vertex[0].v = enemy->PrimaryVm()->sprite->uvTop +
                                enemy->PrimaryVm()->textureVOffset;

                            vertex[1].x = sample->position.x + 32.0f + sine * (halfWidth * fade);
                            vertex[1].y = sample->position.y + 16.0f - cosine * (halfWidth * fade);
                            vertex[1].z = 0.0f;
                            vertex[1].rhw = 1.0f;
                            vertex[1].diffuse = color;
                            vertex[1].u = u;
                            vertex[1].v = enemy->PrimaryVm()->sprite->uvBottom +
                                enemy->PrimaryVm()->textureVOffset;
                            vertex += 2;
                        }
                        g_EnemyRenderAnmManager->DrawTriangleFan(enemy->PrimaryVm(), enemy->TrailVertices(), vertexCount);
                    }
                }

                enemy->PrimaryVm()->scaleX = oldScaleX;
                enemy->PrimaryVm()->scaleY = oldScaleY;
                enemy->PrimaryVm()->color = color;
            }

            enemy = enemy->DrawNext();
        }
    }
    return 1;
}

i32 EnemyManagerRenderOverlay::HasActiveBoss()
{
    for (i32 i = 0; i < 8; ++i)
    {
        if (bosses[i])
            return 1;
    }
    return 0;
}

} // namespace th07
