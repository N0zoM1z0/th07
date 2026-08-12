#include "Controller.hpp"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <mmsystem.h>
#include <string.h>
#include <windows.h>

namespace th07
{
struct ControllerMapping
{
    i16 shotButton;
    i16 bombButton;
    i16 focusButton;
    i16 menuButton;
    i16 upButton;
    i16 downButton;
    i16 leftButton;
    i16 rightButton;
    i16 skipButton;
};

// These globals are direct absolute references in the target input cluster.
// Their definitions and final relocation mapping remain with their owning lanes.
extern LPDIRECTINPUTDEVICE8A g_Keyboard;
extern LPDIRECTINPUTDEVICE8A g_Controller;
extern ControllerMapping g_ControllerMapping;
extern i16 g_PadXAxis;
extern i16 g_PadYAxis;
extern u8 g_ShotSlowEnabled;
extern JOYCAPSA g_JoystickCaps;
extern u16 g_FocusButtonConflictState;
extern u8 g_ControllerData[128];

extern void __cdecl DebugPrint(const char *format, ...);
extern void __cdecl LogGameError(void *context, const char *message);
extern u8 g_GameErrorContext;
extern const char g_NoPadFoundError[];

#define JOYSTICK_MIDPOINT(min, max) ((min + max) / 2)
#define JOYSTICK_BUTTON_PRESSED(button, x, y) ((x) > (y) ? (button) : 0)
#define JOYSTICK_BUTTON_PRESSED_INVERT(button, x, y) ((x) < (y) ? (button) : 0)
#define KEYBOARD_KEY_PRESSED(button, x) (keyboardState[x] & 0x80 ? (button) : 0)

u16 Controller::GetJoystickCaps()
{
    JOYINFOEX pji;

    pji.dwSize = sizeof(JOYINFOEX);
    pji.dwFlags = JOY_RETURNALL;
    if (joyGetPosEx(0, &pji) != MMSYSERR_NOERROR)
    {
        LogGameError(&g_GameErrorContext, g_NoPadFoundError);
        return 1;
    }

    joyGetDevCapsA(0, &g_JoystickCaps, sizeof(g_JoystickCaps));
    return 0;
}

u32 Controller::SetButtonFromDirectInputJoystate(u16 *outButtons, i16 controllerButtonToTest,
                                                 enum TouhouButton touhouButton, u8 *inputButtons)
{
    if (controllerButtonToTest < 0)
    {
        return 0;
    }

    *outButtons |= inputButtons[controllerButtonToTest] & 0x80 ? (u16)touhouButton : 0;
    return inputButtons[controllerButtonToTest] & 0x80 ? (u16)touhouButton : 0;
}

u32 Controller::SetButtonFromControllerInputs(u16 *outButtons, i16 controllerButtonToTest, enum TouhouButton touhouButton,
                                              u32 inputButtons)
{
    u32 mask;

    if (controllerButtonToTest < 0)
    {
        return 0;
    }

    mask = 1 << controllerButtonToTest;
    *outButtons |= inputButtons & mask ? (u16)touhouButton : 0;
    return inputButtons & mask ? (u16)touhouButton : 0;
}

#pragma var_order(joyinfoex, axisDeadZone, joystickShotPressed, directInputShotPressed, dires, dijoystate2, buttons)
u16 Controller::GetControllerInput(u16 buttons)
{
    JOYINFOEX joyinfoex;
    u32 axisDeadZone;
    u32 joystickShotPressed;
    DIJOYSTATE2 dijoystate2;
    u32 directInputShotPressed;
    HRESULT dires;

    if (g_Controller == NULL)
    {
        memset(&joyinfoex, 0, sizeof(joyinfoex));
        joyinfoex.dwSize = sizeof(JOYINFOEX);
        joyinfoex.dwFlags = JOY_RETURNALL;
        if (joyGetPosEx(0, &joyinfoex) != MMSYSERR_NOERROR)
        {
            return buttons;
        }

        joystickShotPressed = SetButtonFromControllerInputs(&buttons, g_ControllerMapping.shotButton,
                                                            TH_BUTTON_SHOOT, joyinfoex.dwButtons);
        if (g_ShotSlowEnabled)
        {
            if (joystickShotPressed != 0)
            {
                if (g_FocusButtonConflictState < 20)
                {
                    g_FocusButtonConflictState++;
                }
                if (g_FocusButtonConflictState >= 10)
                {
                    buttons |= TH_BUTTON_FOCUS;
                }
            }
            else if (g_FocusButtonConflictState > 10)
            {
                g_FocusButtonConflictState -= 10;
                buttons |= TH_BUTTON_FOCUS;
            }
            else
            {
                g_FocusButtonConflictState = 0;
            }
        }

        SetButtonFromControllerInputs(&buttons, g_ControllerMapping.bombButton, TH_BUTTON_BOMB,
                                      joyinfoex.dwButtons);
        SetButtonFromControllerInputs(&buttons, g_ControllerMapping.focusButton, TH_BUTTON_FOCUS,
                                      joyinfoex.dwButtons);
        SetButtonFromControllerInputs(&buttons, g_ControllerMapping.menuButton, TH_BUTTON_MENU,
                                      joyinfoex.dwButtons);
        SetButtonFromControllerInputs(&buttons, g_ControllerMapping.upButton, TH_BUTTON_UP,
                                      joyinfoex.dwButtons);
        SetButtonFromControllerInputs(&buttons, g_ControllerMapping.downButton, TH_BUTTON_DOWN,
                                      joyinfoex.dwButtons);
        SetButtonFromControllerInputs(&buttons, g_ControllerMapping.leftButton, TH_BUTTON_LEFT,
                                      joyinfoex.dwButtons);
        SetButtonFromControllerInputs(&buttons, g_ControllerMapping.rightButton, TH_BUTTON_RIGHT,
                                      joyinfoex.dwButtons);
        SetButtonFromControllerInputs(&buttons, g_ControllerMapping.skipButton, TH_BUTTON_SKIP,
                                      joyinfoex.dwButtons);

        axisDeadZone = (g_JoystickCaps.wXmax - g_JoystickCaps.wXmin) / 2 / 2;
        buttons |= JOYSTICK_BUTTON_PRESSED(TH_BUTTON_RIGHT, joyinfoex.dwXpos,
                                           JOYSTICK_MIDPOINT(g_JoystickCaps.wXmin, g_JoystickCaps.wXmax) + axisDeadZone);
        buttons |= JOYSTICK_BUTTON_PRESSED(TH_BUTTON_LEFT,
                                           JOYSTICK_MIDPOINT(g_JoystickCaps.wXmin, g_JoystickCaps.wXmax) - axisDeadZone,
                                           joyinfoex.dwXpos);

        axisDeadZone = (g_JoystickCaps.wYmax - g_JoystickCaps.wYmin) / 2 / 2;
        buttons |= JOYSTICK_BUTTON_PRESSED(TH_BUTTON_DOWN, joyinfoex.dwYpos,
                                           JOYSTICK_MIDPOINT(g_JoystickCaps.wYmin, g_JoystickCaps.wYmax) + axisDeadZone);
        buttons |= JOYSTICK_BUTTON_PRESSED(TH_BUTTON_UP,
                                           JOYSTICK_MIDPOINT(g_JoystickCaps.wYmin, g_JoystickCaps.wYmax) - axisDeadZone,
                                           joyinfoex.dwYpos);
        return buttons;
    }

    dires = g_Controller->Poll();
    if (FAILED(dires))
    {
        i32 retryCount = 0;
        DebugPrint("error : DIERR_INPUTLOST\r\n");
        dires = g_Controller->Acquire();
        while (dires == DIERR_INPUTLOST)
        {
            dires = g_Controller->Acquire();
            DebugPrint("error : DIERR_INPUTLOST %d\r\n", retryCount);
            retryCount++;
            if (retryCount >= 400)
            {
                return buttons;
            }
        }
        return buttons;
    }

    memset(&dijoystate2, 0, sizeof(dijoystate2));
    dires = g_Controller->GetDeviceState(sizeof(dijoystate2), &dijoystate2);
    if (FAILED(dires))
    {
        return buttons;
    }

    directInputShotPressed = SetButtonFromDirectInputJoystate(&buttons, g_ControllerMapping.shotButton,
                                                              TH_BUTTON_SHOOT, dijoystate2.rgbButtons);
    if (g_ShotSlowEnabled)
    {
        if (directInputShotPressed != 0)
        {
            if (g_FocusButtonConflictState < 20)
            {
                g_FocusButtonConflictState++;
            }
            if (g_FocusButtonConflictState >= 10)
            {
                buttons |= TH_BUTTON_FOCUS;
            }
        }
        else if (g_FocusButtonConflictState > 10)
        {
            g_FocusButtonConflictState -= 10;
            buttons |= TH_BUTTON_FOCUS;
        }
        else
        {
            g_FocusButtonConflictState = 0;
        }
    }

    SetButtonFromDirectInputJoystate(&buttons, g_ControllerMapping.bombButton, TH_BUTTON_BOMB,
                                     dijoystate2.rgbButtons);
    SetButtonFromDirectInputJoystate(&buttons, g_ControllerMapping.focusButton, TH_BUTTON_FOCUS,
                                     dijoystate2.rgbButtons);
    SetButtonFromDirectInputJoystate(&buttons, g_ControllerMapping.menuButton, TH_BUTTON_MENU,
                                     dijoystate2.rgbButtons);
    SetButtonFromDirectInputJoystate(&buttons, g_ControllerMapping.upButton, TH_BUTTON_UP,
                                     dijoystate2.rgbButtons);
    SetButtonFromDirectInputJoystate(&buttons, g_ControllerMapping.downButton, TH_BUTTON_DOWN,
                                     dijoystate2.rgbButtons);
    SetButtonFromDirectInputJoystate(&buttons, g_ControllerMapping.leftButton, TH_BUTTON_LEFT,
                                     dijoystate2.rgbButtons);
    SetButtonFromDirectInputJoystate(&buttons, g_ControllerMapping.rightButton, TH_BUTTON_RIGHT,
                                     dijoystate2.rgbButtons);
    SetButtonFromDirectInputJoystate(&buttons, g_ControllerMapping.skipButton, TH_BUTTON_SKIP,
                                     dijoystate2.rgbButtons);
    SetButtonFromDirectInputJoystate(&buttons, 7, TH_BUTTON_D, dijoystate2.rgbButtons);

    buttons |= JOYSTICK_BUTTON_PRESSED(TH_BUTTON_RIGHT, dijoystate2.lX, g_PadXAxis);
    buttons |= JOYSTICK_BUTTON_PRESSED_INVERT(TH_BUTTON_LEFT, dijoystate2.lX, -g_PadXAxis);
    buttons |= JOYSTICK_BUTTON_PRESSED(TH_BUTTON_DOWN, dijoystate2.lY, g_PadYAxis);
    buttons |= JOYSTICK_BUTTON_PRESSED_INVERT(TH_BUTTON_UP, dijoystate2.lY, -g_PadYAxis);
    return buttons;
}

#pragma var_order(joyinfoex, joyButtonBit, joyButtonIndex, dires, dijoystate2, retryCount)
u8 *Controller::GetControllerState()
{
    JOYINFOEX joyinfoex;
    u32 joyButtonBit;
    u32 joyButtonIndex;
    HRESULT dires;
    DIJOYSTATE2 dijoystate2;
    i32 retryCount;

    memset(g_ControllerData, 0, sizeof(g_ControllerData));
    if (g_Controller == NULL)
    {
        memset(&joyinfoex, 0, sizeof(joyinfoex));
        joyinfoex.dwSize = sizeof(JOYINFOEX);
        joyinfoex.dwFlags = JOY_RETURNALL;
        if (joyGetPosEx(0, &joyinfoex) != MMSYSERR_NOERROR)
        {
            return g_ControllerData;
        }
        for (joyButtonBit = joyinfoex.dwButtons, joyButtonIndex = 0; joyButtonIndex < 32;
             joyButtonIndex++, joyButtonBit >>= 1)
        {
            if (joyButtonBit & 1)
            {
                g_ControllerData[joyButtonIndex] = 0x80;
            }
        }
        return g_ControllerData;
    }

    dires = g_Controller->Poll();
    if (FAILED(dires))
    {
        retryCount = 0;
        DebugPrint("error : DIERR_INPUTLOST\r\n");
        dires = g_Controller->Acquire();
        while (dires == DIERR_INPUTLOST)
        {
            dires = g_Controller->Acquire();
            retryCount++;
            if (retryCount >= 400)
            {
                DebugPrint("error : DIERR_INPUTLOST %d\r\n", retryCount);
                return g_ControllerData;
            }
        }
        return g_ControllerData;
    }

    g_Controller->GetDeviceState(sizeof(dijoystate2), &dijoystate2);
    if (FAILED(dires))
    {
        return g_ControllerData;
    }
    memcpy(g_ControllerData, dijoystate2.rgbButtons, sizeof(dijoystate2.rgbButtons));
    return g_ControllerData;
}

#pragma var_order(keyboardState, buttons, res)
u16 Controller::GetInput()
{
    HRESULT res;
    u16 buttons;
    u8 keyboardState[256];

    buttons = 0;
    if (g_Keyboard == NULL)
    {
        GetKeyboardState(keyboardState);

        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP, VK_UP);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN, VK_DOWN);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_LEFT, VK_LEFT);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_RIGHT, VK_RIGHT);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP, VK_NUMPAD8);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN, VK_NUMPAD2);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_LEFT, VK_NUMPAD4);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_RIGHT, VK_NUMPAD6);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP_LEFT, VK_NUMPAD7);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP_RIGHT, VK_NUMPAD9);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN_LEFT, VK_NUMPAD1);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN_RIGHT, VK_NUMPAD3);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_HOME, VK_HOME);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_D, 'D');
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_SHOOT, 'Z');
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_BOMB, 'X');
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_FOCUS, VK_SHIFT);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_MENU, VK_ESCAPE);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_SKIP, VK_CONTROL);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_Q, 'Q');
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_S, 'S');
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_RESET, 'R');
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_ENTER, VK_RETURN);
    }
    else
    {
        res = g_Keyboard->GetDeviceState(sizeof(keyboardState), keyboardState);
        buttons = 0;
        if (res == DIERR_INPUTLOST)
        {
            g_Keyboard->Acquire();
            return GetControllerInput(buttons);
        }

        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP, DIK_UP);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN, DIK_DOWN);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_LEFT, DIK_LEFT);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_RIGHT, DIK_RIGHT);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP, DIK_NUMPAD8);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN, DIK_NUMPAD2);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_LEFT, DIK_NUMPAD4);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_RIGHT, DIK_NUMPAD6);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP_LEFT, DIK_NUMPAD7);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP_RIGHT, DIK_NUMPAD9);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN_LEFT, DIK_NUMPAD1);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN_RIGHT, DIK_NUMPAD3);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_HOME, DIK_HOME);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_D, DIK_D);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_SHOOT, DIK_Z);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_BOMB, DIK_X);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_FOCUS, DIK_LSHIFT);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_FOCUS, DIK_RSHIFT);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_MENU, DIK_ESCAPE);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_SKIP, DIK_LCONTROL);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_SKIP, DIK_RCONTROL);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_Q, DIK_Q);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_S, DIK_S);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_ENTER, DIK_RETURN);
        buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_RESET, DIK_R);
    }

    return GetControllerInput(buttons);
}

void Controller::ResetKeyboard()
{
    u8 key_states[256];

    GetKeyboardState(key_states);
    for (i32 idx = 0; idx < 256; idx++)
    {
        *(key_states + idx) &= 0x7f;
    }
    SetKeyboardState(key_states);
}
}; // namespace th07
