#include "inttypes.hpp"

#include <d3d8.h>

namespace th07
{

struct VertexDiffuseXyzrhw
{
    f32 x;
    f32 y;
    f32 z;
    f32 rhw;
    D3DCOLOR diffuse;
    f32 u;
    f32 v;
};

struct AnmTriangleFanSprite
{
    i32 textureIndex;
};

struct AnmTriangleFanVm
{
    u8 unknown000[0x1BB];
    u8 alpha;
    u8 unknown1BC[4];
    u32 visible : 1;
    u32 enabled : 1;
    u32 unknownFlags : 30;
    u8 unknown1C4[0x20];
    AnmTriangleFanSprite *sprite;
};

struct AnmTriangleFanManager
{
    u8 unknown00000[0x282AC];
    IDirect3DBaseTexture8 *textures[264];
    u8 unknown286CC[0x5E00];
    IDirect3DBaseTexture8 *currentTexture;
    u8 currentBlendMode;
    u8 currentColorOp;
    u8 currentVertexShader;
    u8 currentZWriteDisable;
    u8 unknown2E4D4[0x5C];
    i32 spritesToDraw;

    void FlushVertexBuffer();
    void SetRenderStateForVm(AnmTriangleFanVm *vm);
    i32 DrawTriangleFan(AnmTriangleFanVm *vm, VertexDiffuseXyzrhw *vertices, i32 vertexCount);
};

extern IDirect3DDevice8 *g_ScreenEffectDevice;

i32 AnmTriangleFanManager::DrawTriangleFan(AnmTriangleFanVm *vm,
                                            VertexDiffuseXyzrhw *vertices,
                                            i32 vertexCount)
{
    if (!vm->visible)
        return -1;
    if (!vm->enabled)
        return -1;
    if (!vm->alpha)
        return -1;

    if (spritesToDraw != 0)
        FlushVertexBuffer();

    if (currentTexture != textures[vm->sprite->textureIndex])
    {
        currentTexture = textures[vm->sprite->textureIndex];
        g_ScreenEffectDevice->SetTexture(0, currentTexture);
    }

    if (currentVertexShader != 3)
    {
        g_ScreenEffectDevice->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
        currentVertexShader = 3;
    }

    SetRenderStateForVm(vm);
    // The TH07 target's Direct3D 8 SDK encodes this fan path as primitive 5.
    g_ScreenEffectDevice->DrawPrimitiveUP(static_cast<D3DPRIMITIVETYPE>(5),
                                          vertexCount - 2, vertices,
                                          sizeof(VertexDiffuseXyzrhw));
    return 0;
}

} // namespace th07
