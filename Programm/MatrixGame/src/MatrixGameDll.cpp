#include "stdafx.h"
#include "MatrixGameDll.hpp"
#include "MatrixGame.h"
#include "MatrixMap.hpp"
#include "MatrixFormGame.hpp"
#include "CInterface.h"

#include <stdio.h>
#include <time.h>

SMGDRobotInterface g_RobotInterface;
SMGDRangersInterface * g_RangersInterface=NULL;
int g_ExitState=0;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

void __stdcall Init(SMGDRangersInterface * ri)
{
    g_RangersInterface=ri;
}

void __stdcall Deinit()
{
    g_RangersInterface=NULL;
}

int __stdcall Support()
{

    //g_D3D = Direct3DCreate9(D3D_SDK_VERSION);
    g_D3D = Direct3DCreate9(31);
    
    if (g_D3D == NULL) return SUPE_DIRECTX;


    D3DCAPS9 caps;
    if (D3D_OK != g_D3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps))
    {
        g_D3D->Release();
        g_D3D = NULL;
        return SUPE_DIRECTX;
    }

    if (caps.MaxSimultaneousTextures < 2) return SUPE_VIDEOHARDWARE;

    if (caps.MaxTextureWidth < 2048) return SUPE_VIDEOHARDWARE;
    if (caps.MaxTextureHeight < 2048) return SUPE_VIDEOHARDWARE;
    



    if (caps.MaxStreams == 0) return SUPE_VIDEODRIVER;
    g_D3D->Release();
    g_D3D = NULL;
    return SUPE_OK;
}

// 0-exit to windows (terminate)
// 1-exit to main menu
// 2-loss
// 3-win
int __stdcall Run(HINSTANCE hinst,HWND hwnd,wchar * map,SRobotsSettings * set,wchar * txt_start,wchar * txt_win,wchar * txt_loss, wchar *planet, SRobotGameState *rgs)
{
    
    try {
        srand((unsigned)time( NULL ));

        MatrixGameInit(hinst,hwnd,map,set,txt_start,txt_win,txt_loss,planet);

        CFormMatrixGame * formgame=HNew(NULL) CFormMatrixGame();
        FormChange(formgame);

        timeBeginPeriod(1);

        SETFLAG(g_Flags, GFLAG_APPACTIVE);
        RESETFLAG(g_Flags, GFLAG_EXITLOOP); 

        L3GRun();

        rgs->m_Time=g_MatrixMap->GetPlayerSide()->GetStatValue(STAT_TIME);
        rgs->m_BuildRobot=g_MatrixMap->GetPlayerSide()->GetStatValue(STAT_ROBOT_BUILD);
        rgs->m_KillRobot=g_MatrixMap->GetPlayerSide()->GetStatValue(STAT_ROBOT_KILL);
        rgs->m_BuildTurret=g_MatrixMap->GetPlayerSide()->GetStatValue(STAT_TURRET_BUILD);
        rgs->m_KillTurret=g_MatrixMap->GetPlayerSide()->GetStatValue(STAT_TURRET_KILL);
        rgs->m_KillBuilding=g_MatrixMap->GetPlayerSide()->GetStatValue(STAT_BUILDING_KILL);

        timeEndPeriod(1);

        MatrixGameDeinit();

        FormChange(NULL);
        HDelete(CFormMatrixGame, formgame, NULL);

        g_Cache->Clear();
        L3GDeinit();
        CacheDeinit();

#ifdef MEM_SPY_ENABLE
        CMain::BaseDeInit();
#endif

    }
    catch(CException * ex)
    {
#ifdef ENABLE_HISTORY
        CDebugTracer::SaveHistory();
#endif
        g_Cache->Clear();
        L3GDeinit();

        CStr exs(ex->Info());
        CBuf exb;
        exb.StrNZ(exs);
        exb.SaveInFile(L"exception.txt");


        MessageBox(NULL,exs.Get(),"Exception:",MB_OK);

        delete ex;
    }
    catch(...)
    {
        ClipCursor(NULL);
#ifdef ENABLE_HISTORY
        MessageBox(NULL,"Unknown bug (history has saved) :(","Exception:",MB_OK);
        CDebugTracer::SaveHistory();
#else
        MessageBox(NULL,"Dont panic! This is just unknown bug happens."
            "\nCode date: " __DATE__
            
            ,"Game crashed :(",MB_OK);
#endif
    }

    ClipCursor(NULL);

    if(FLAG(g_Flags, GFLAG_EXITLOOP)) return g_ExitState;
    else return 0;
}

MATRIXGAMEDLL_API SMGDRobotInterface * __cdecl GetRobotInterface(void)
{
    ZeroMemory(&g_RobotInterface,sizeof(SMGDRobotInterface));
    g_RobotInterface.m_Init=&Init;
    g_RobotInterface.m_Deinit=&Deinit;
    g_RobotInterface.m_Support=&Support;
    g_RobotInterface.m_Run=&Run;
    return &g_RobotInterface;
}
