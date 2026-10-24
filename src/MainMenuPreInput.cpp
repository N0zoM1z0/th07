// Keep this loop in its own native VC7 compilation unit. The constructor's
// noinline declaration hint prevents eight unused speculative inline slots;
// its body, layout and x86 calling convention remain shared with other TUs.
#define TH07_ANM_CTOR_NOINLINE
// Preserve the size profile of the common diagnostic stubs before parsing
// constructor headers with the ordinary /Od /Oi native profile.
#pragma optimize("s", on)
#include "Global.hpp"
#pragma optimize("s", off)
#include "MainMenu.hpp"

#include <direct.h>
#include <stdio.h>

#include "AnmManager.hpp"
#include "AsciiManager.hpp"

#include "GameManager.hpp"
#include "Global.hpp"
#include "ReplayManager.hpp"
#include "ScreenEffect.hpp"
#include "SoundPlayer.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"
#include "i18n.hpp"
#include "utils.hpp"


extern const char *g_MainMenuStrings[8];
extern const char *g_DemoReplayPaths[3];
// The target calls strcpy and otherwise uses the menu size profile.
#pragma function(strcpy)
#pragma optimize("s", on)
// FUNCTION: TH07 0x004555dd
u32 MainMenu::OnUpdatePreInput()
{
    i32 i;

    switch (this->menuSubState)
    {
    case MENU_SUBSTATE_PREINPUT_INIT:
        if (this->prevMenuState == MENU_STATE_PRE_INPUT &&
            g_Supervisor.prevState != SUPERVISOR_STATE_RESULTSCREEN)
        {
            g_Supervisor.PlayLoadedAudio(8);
        }
        if ((this->prevMenuState == MENU_STATE_PRE_INPUT ||
             this->prevMenuState == MENU_STATE_NORMAL_SELECT_DIFFICULTY ||
             this->prevMenuState == MENU_STATE_SELECT_REPLAY ||
             (this->prevMenuState == MENU_STATE_PRACTICE_SELECT_DIFFICULTY ||
              this->prevMenuState == MENU_STATE_EXTRA_SELECT_DIFFICULTY)) &&
            g_AnmManager->LoadSurface(0, "data/title/title00.jpg") != ZUN_SUCCESS)
        {
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
        }
        if (this->vmCount == 0)
        {
            this->vmCount = 164;
            this->vms = ZUN_NEW_ARRAY(AnmVm, this->vmCount, "SprtInf");
            g_AnmManager->ExecuteVmsAnms(this->vms, ANM_OFFSET_TITLE, this->vmCount);
        }
        g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 2);
        for (i = 0; i < ARRAY_SIZE_SIGNED(g_MainMenuStrings); i++)
        {
            g_AnmManager->SetActiveSprite(&this->vms[i + 1],
                                          this->vms[i + 1].baseSpriteIdx + 1);
        }
        g_AnmManager->SetActiveSprite(
            &this->vms[this->cursor + 1],
            (i32)this->vms[this->cursor + 1].baseSpriteIdx);
        this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
        this->inputDelayTimer = 0;
        this->selected = -1;
        this->menuSubState = MENU_SUBSTATE_PREINPUT_INPUT;
        this->demoFramesCount = 0;
        if (g_GameManager.replay)
        {
            this->prevMenuState = this->menuState;
            this->menuState = MENU_STATE_SELECT_REPLAY;
            this->inputDelayTimer = 0;
            this->stateTimer = 0;
            this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
            this->idleFrames = 0;
            g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 13);
            this->curDescriptionVm->SetInterrupt(2);
            g_GameManager.SetIsReplay(FALSE);
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        if (this->isPracticeMode)
        {
            this->prevMenuState = this->menuState;
            this->menuState = MENU_STATE_PRACTICE_SELECT_DIFFICULTY;
            this->inputDelayTimer = 0;
            this->stateTimer = 0;
            this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
            this->idleFrames = 0;
            g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 5);
            this->curDescriptionVm->SetInterrupt(2);
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        for (i = 0; i < ARRAY_SIZE(g_MainMenuStrings); i++)
        {
            g_AnmManager->DrawStringFormat2(&this->descriptionVms[i], 0xfff0e0, 0x300000,
                                            g_MainMenuStrings[i]);
        }
    case MENU_SUBSTATE_PREINPUT_INPUT: {
        i = MoveCursorVertical(ARRAY_SIZE_SIGNED(g_MainMenuStrings));
        if (i != 0)
        {
            while (g_GameManager.HasReachedMaxClearsAnyShotType() == 0 &&
                   this->cursor == 1)
            {
                this->cursor += i;
            }
            for (i = 0; i < ARRAY_SIZE_SIGNED(g_MainMenuStrings); i++)
            {
                g_AnmManager->SetActiveSprite(&this->vms[i + 1],
                                              this->vms[i + 1].baseSpriteIdx + 1);
            }
            g_AnmManager->SetActiveSprite(
                &this->vms[this->cursor + 1],
                (i32)this->vms[this->cursor + 1].baseSpriteIdx);
        }
        this->demoFramesCount++;
        if (g_CurFrameRawInput != 0)
        {
            this->demoFramesCount = 0;
        }
        if (this->demoFramesCount > 900)
        {
            g_GameManager.demoIdx++;
            g_GameManager.demoIdx %= 3;
            strcpy(g_GameManager.replayFilename,
                   g_DemoReplayPaths[g_GameManager.demoIdx]);
            this->currentReplay = (ReplayFile *)FileSystem::OpenFile(
                g_GameManager.replayFilename, 0);
            this->currentReplay =
                ReplayManager::ValidateReplayData(this->currentReplay, g_LastFileSize);
            if (!this->currentReplay)
            {
                utils::DebugPrint2("error : Demo Play is not ready\r\n");
                this->demoFramesCount = 0;
            }
            else
            {
                g_GameManager.SetIsReplay(TRUE);
                g_GameManager.demo = 1;
                g_GameManager.demoFrames = 0;
                g_GameManager.difficulty = this->currentReplay->data.difficulty;
                g_GameManager.character = this->currentReplay->data.shotType / 2;
                g_GameManager.shotType = this->currentReplay->data.shotType % 2;
                g_GameManager.shotTypeAndCharacter = this->currentReplay->data.shotType;
                i = 0;
                while (!this->currentReplay->head.stageReplayData[i].data)
                {
                    i++;
                }

                g_GameManager.currentStage = i;
                ZUN_FREE(this->currentReplay);
                this->currentReplay = NULL;
                g_Supervisor.curState = SUPERVISOR_STATE_GAMEMANAGER;
                g_GameManager.replayStage = 0;
                return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
            }
        }
        if (this->selected != this->cursor)
        {
            this->curDescriptionVm = &this->descriptionVms[this->cursor];
            this->curDescriptionVm->SetInterrupt(1);
        }
        this->selected = this->cursor;
        if (this->stateTimer < 10)
        {
            break;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            g_SoundPlayer.ProcessQueues();
            switch (this->cursor)
            {
            case MENU_CURSOR_PREINPUT_START:
                g_GameManager.practice = 0;
                this->cursor = g_Supervisor.cfg.defaultDifficulty;
                if (this->cursor >= 4)
                {
                    this->cursor = 2;
                }
                this->prevMenuState = this->menuState;
                this->menuState = MENU_STATE_NORMAL_SELECT_DIFFICULTY;
                this->inputDelayTimer = 0;
                this->stateTimer = 0;
                this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
                this->idleFrames = 0;
                g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 5);
                this->curDescriptionVm->SetInterrupt(2);
                return CHAIN_CALLBACK_RESULT_CONTINUE;
            case MENU_CURSOR_PREINPUT_PRACTICE_START:
                g_GameManager.practice = 1;
                this->cursor = g_Supervisor.cfg.defaultDifficulty;
                if (this->cursor >= 4)
                {
                    this->cursor = 2;
                }
                this->prevMenuState = this->menuState;
                this->menuState = MENU_STATE_PRACTICE_SELECT_DIFFICULTY;
                this->inputDelayTimer = 0;
                this->stateTimer = 0;
                this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
                this->idleFrames = 0;
                g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 5);
                this->curDescriptionVm->SetInterrupt(2);
                return CHAIN_CALLBACK_RESULT_CONTINUE;
            case MENU_CURSOR_PREINPUT_EXTRA_START:
                if (g_GameManager.HasReachedMaxClearsAnyShotType())
                {
                    g_GameManager.practice = 0;
                    this->cursor = g_Supervisor.cfg.defaultDifficulty == 5;
                    this->prevMenuState = this->menuState;
                    this->menuState = MENU_STATE_EXTRA_SELECT_DIFFICULTY;
                    this->inputDelayTimer = 0;
                    this->stateTimer = 0;
                    this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
                    this->idleFrames = 0;
                    g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 5);
                    this->curDescriptionVm->SetInterrupt(2);
                    return CHAIN_CALLBACK_RESULT_CONTINUE;
                }
            case MENU_CURSOR_PREINPUT_REPLAY:
                g_GameManager.practice = 0;
                this->prevMenuState = this->menuState;
                this->menuState = MENU_STATE_SELECT_REPLAY;
                this->inputDelayTimer = 0;
                this->stateTimer = 0;
                this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
                this->idleFrames = 0;
                g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 13);
                this->curDescriptionVm->SetInterrupt(2);
                return CHAIN_CALLBACK_RESULT_CONTINUE;
            case MENU_CURSOR_PREINPUT_MUSICROOM:
                g_Supervisor.curState = SUPERVISOR_STATE_MUSICROOM;
                this->curDescriptionVm->SetInterrupt(2);
                return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
            case MENU_CURSOR_PREINPUT_RESULTS:
                g_Supervisor.curState = SUPERVISOR_STATE_RESULTSCREEN;
                this->curDescriptionVm->SetInterrupt(2);
                return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
            case MENU_CURSOR_PREINPUT_OPTIONS:
                this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
                this->cursor = 0;
                this->stateTimer = 0;
                this->inputDelayTimer = 0;
                this->menuSubState = 3;
                this->inputDelayTimer = 0;
                OnUpdateOptionsMenu();
                this->cursor = 0;
                break;
            case MENU_CURSOR_PREINPUT_EXIT:
                this->menuSubState = MENU_SUBSTATE_PREINPUT_EXIT;
                this->inputDelayTimer = 0;
                g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 1);
                if (g_Supervisor.cfg.musicMode == 2)
                {
                    g_Supervisor.midiOutput->PlayLoaded(30);
                }
                break;
            }
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            g_AnmManager->SetActiveSprite(
                &this->vms[this->cursor + 1],
                this->vms[this->cursor + 1].baseSpriteIdx + 1);
            this->cursor = 7;
            g_AnmManager->SetActiveSprite(
                &this->vms[this->cursor + 1],
                (i32)this->vms[this->cursor + 1].baseSpriteIdx);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            g_SoundPlayer.ProcessQueues();
        }
        break;
    }
    case MENU_SUBSTATE_PREINPUT_EXIT:
        if (this->inputDelayTimer >= 60)
        {
            delete[] this->vms;
            this->vms = NULL;
            this->vms = NULL;
            this->vmCount = 0;
            this->stateTimer = 0;
            g_Supervisor.curState = SUPERVISOR_STATE_EXIT;
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
        }
        break;
    case MENU_SUBSTATE_PREINPUT_OPTIONS:
        if (this->inputDelayTimer >= 30)
        {
            this->prevMenuState = this->menuState;
            this->menuState = MENU_STATE_OPTIONS;
            this->inputDelayTimer = 0;
            this->stateTimer = 0;
            this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
            this->idleFrames = 0;
            this->cursor = 0;
            this->cfg = g_Supervisor.cfg;
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        break;
    }
    this->idleFrames++;
    this->inputDelayTimer++;
    this->stateTimer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}
