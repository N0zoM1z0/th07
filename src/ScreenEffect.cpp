#include "ScreenEffect.hpp"

namespace th07
{
void __fastcall ScreenEffect::Clear(DWORD color)
{
    g_ScreenEffectDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 1.0f, 0);
    if (g_ScreenEffectDevice->Present(NULL, NULL, NULL, NULL) < 0)
    {
        g_ScreenEffectDevice->Reset(&g_ScreenEffectPresentParameters);
    }
    g_ScreenEffectDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 1.0f, 0);
    if (g_ScreenEffectDevice->Present(NULL, NULL, NULL, NULL) < 0)
    {
        g_ScreenEffectDevice->Reset(&g_ScreenEffectPresentParameters);
    }
}

void __fastcall ScreenEffect::SetViewport(DWORD clearColor)
{
    if (g_ScreenEffectRenderState != NULL)
    {
        g_ScreenEffectRenderState->PrepareScreenEffect();
    }
    g_ScreenEffectViewport.X = 0;
    g_ScreenEffectViewport.Y = 0;
    g_ScreenEffectViewport.Width = 640;
    g_ScreenEffectViewport.Height = 480;
    g_ScreenEffectViewport.MinZ = 0.0f;
    g_ScreenEffectViewport.MaxZ = 1.0f;
    g_ScreenEffectDevice->SetViewport(&g_ScreenEffectViewport);
    ScreenEffect::Clear(clearColor);
}

i32 __fastcall ScreenEffect::DrawFullFade(ScreenEffect *screenEffect)
{
    ZunRect rect;

    rect.left = 0.0f;
    rect.top = 0.0f;
    rect.right = 640.0f;
    rect.bottom = 480.0f;
    g_ScreenEffectRenderState->PrepareScreenEffect();
    g_ScreenEffectViewport.X = 0;
    g_ScreenEffectViewport.Y = 0;
    g_ScreenEffectViewport.Width = 640;
    g_ScreenEffectViewport.Height = 480;
    g_ScreenEffectDevice->SetViewport(&g_ScreenEffectViewport);
    ScreenEffect::DrawSquare(&rect, (screenEffect->fadeAlpha << 24) | screenEffect->effectParam);
    return 1;
}

i32 __fastcall ScreenEffect::DrawArcadeFade(ScreenEffect *screenEffect)
{
    ZunRect rect;

    rect.left = 32.0f;
    rect.top = 16.0f;
    rect.right = 416.0f;
    rect.bottom = 464.0f;
    ScreenEffect::DrawSquare(&rect, (screenEffect->fadeAlpha << 24) | screenEffect->effectParam);
    return 1;
}
} // namespace th07
