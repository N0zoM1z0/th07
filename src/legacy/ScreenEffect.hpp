#pragma once

#include "inttypes.hpp"

#include <d3d8.h>
#include <stddef.h>

namespace th07
{
struct ZunRect
{
    f32 left;
    f32 top;
    f32 right;
    f32 bottom;
};

// The four routines in this probe only use the effect's fade alpha and RGB
// parameter.  Their target offsets establish this prefix; the remaining
// fields belong to the wider screen-effect lifecycle.
class ScreenEffect
{
public:
    u8 unknown0[0x10];
    i32 fadeAlpha;
    i32 effectLength;
    i32 effectParam;

    static void __fastcall Clear(DWORD color);
    static void __fastcall SetViewport(DWORD clearColor);
    static i32 __fastcall DrawFullFade(ScreenEffect *screenEffect);
    static i32 __fastcall DrawArcadeFade(ScreenEffect *screenEffect);
    static void __fastcall DrawSquare(ZunRect *rect, DWORD color);
};

typedef char ScreenEffectFadeAlphaOffsetMustBe0x10[(offsetof(ScreenEffect, fadeAlpha) == 0x10) ? 1 : -1];
typedef char ScreenEffectColorOffsetMustBe0x18[(offsetof(ScreenEffect, effectParam) == 0x18) ? 1 : -1];

// The target invokes this state owner before updating a viewport or drawing a
// full-screen fade.  Its concrete engine type and method name are outside the
// four assigned function bounds, so this declaration preserves only the
// observed call ABI.
struct ScreenEffectRenderState
{
    void __fastcall PrepareScreenEffect();
};

extern IDirect3DDevice8 *g_ScreenEffectDevice;
extern D3DVIEWPORT8 g_ScreenEffectViewport;
extern D3DPRESENT_PARAMETERS g_ScreenEffectPresentParameters;
extern ScreenEffectRenderState *g_ScreenEffectRenderState;
} // namespace th07
