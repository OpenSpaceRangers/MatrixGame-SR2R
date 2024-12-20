// MatrixGame.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "MatrixGame.h"
#include "MatrixMap.hpp"
#include "MatrixFormGame.hpp"
#include "CInterface.h"
#include "MatrixRenderPipeline.hpp"
#include "MatrixGameDll.hpp"
#include "ShadowStencil.hpp"
#include "MatrixLoadProgress.hpp"
#include "MatrixFlyer.hpp"
#include "MatrixMapStatic.hpp"
#include "MatrixMultiSelection.hpp"
#include "MatrixTerSurface.hpp"
#include "MatrixSkinManager.hpp"
#include "MatrixSoundManager.hpp"
#include "CIFaceMenu.h"
#include "MatrixHint.hpp"
#include "CHistory.h"
#include "MatrixInstantDraw.hpp"



#include <stdio.h>
#include <time.h>

#include <ddraw.h>

////////////////////////////////////////////////////////////////////////////////
CHeap * g_MatrixHeap;
CBlockPar *g_MatrixData;
CMatrixMapLogic * g_MatrixMap;
CRenderPipeline   *g_Render;
CLoadProgress     *g_LoadProgress;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE ,
                     LPTSTR    ,
                     int       )
{


    //CBitmap b,bb;
    //b.LoadFromPNG(L"test.png");
    //bb.CreateRGB(b.SizeX(), b.SizeY());
    //bb.Copy(CPoint(0,0), b.Size(), b, CPoint(0,0));
    //bb.SaveInDDSUncompressed(L"out.dds");

    //CBuf bla;
    //bla.LoadFromFile(L"test.dds");
    //BYTE *data = (BYTE *)bla.Get();
    //DDSURFACEDESC2 *desc = (DDSURFACEDESC2 *)(data + 4);

    //int x = FP_NORM_TO_BYTE2(1.0f);
    //x = FP_NORM_TO_BYTE2(0.9999f);
    //x = FP_NORM_TO_BYTE2(0.5f);
    //x = FP_NORM_TO_BYTE2(0.1f);
    //x = FP_NORM_TO_BYTE2(0.0f);
    //x = FP_NORM_TO_BYTE2(2.0f);
    //x = FP_NORM_TO_BYTE2(200.6f);
    //x = FP_NORM_TO_BYTE2(255.6f);
    //x = FP_NORM_TO_BYTE2(256.6f);


    //float t,k;
    //k = 10.0f; FP_INV(t, k);
    //k = 20.0f; FP_INV(t, k);
    //k = 25.0f; FP_INV(t, k);

    const wchar *cmd = GetCommandLineW();

    int numarg;
    wchar **args = CommandLineToArgvW(cmd,&numarg);
    wchar *map = NULL;

    if (numarg > 1)
    {
        map = args[1];

    }

	try {

		srand((unsigned)time( NULL ));

		MatrixGameInit(hInstance, NULL, map);

		CFormMatrixGame * formgame=HNew(NULL) CFormMatrixGame();
		FormChange(formgame);

		timeBeginPeriod(1);

//DWORD * buf=(DWORD *)HAlloc(124,NULL);
//*(buf-1)=1;
//HFree(buf,NULL);

        if (map)
        {
            FILE *file;
            file = fopen("calcvis.log","a");
            CStr name(g_MatrixMap->MapName(), g_MatrixHeap);
            fwrite(name.Get(), name.Len(),1, file);
            fwrite(" ...", 4,1, file);
            fclose(file);

            g_MatrixMap->CalcVis();

            file = fopen("calcvis.log","a");
            fwrite("done\n", 5,1, file);
            fclose(file);

            
            

        } else
        {
		    L3GRun();
        }

		timeEndPeriod(1);

		MatrixGameDeinit();

        FormChange(NULL);
        HDelete(CFormMatrixGame, formgame, NULL);


        g_Cache->Clear();
        L3GDeinit();
		CacheDeinit();

        CMain::BaseDeInit();
        
    } catch(CException * ex)
    {
        ClipCursor(NULL);
#ifdef ENABLE_HISTORY
        CDebugTracer::SaveHistory();
#endif
        g_Cache->Clear();
		L3GDeinit();
        
		MessageBox(NULL,CStr(ex->Info()).Get(),"Exception:",MB_OK);

		delete ex;
	}
    catch(...)
    {
#ifdef ENABLE_HISTORY
        CDebugTracer::SaveHistory();
#endif
        MessageBox(NULL,"Unknown bug :(","Exception:",MB_OK);


    }
    
    ClipCursor(NULL);

	return 1;
}

static void static_init(void)
{

    // Base
    CMain::BaseInit();

    // 3G
#ifdef _DEBUG
    D3DResource::StaticInit();
#endif
#if (defined _DEBUG) &&  !(defined _RELDEBUG)
    CHelper::StaticInit();
#endif
    CCacheData::StaticInit();
    CVectorObject::StaticInit();
    CVOShadowProj::StaticInit();
    CVOShadowStencil::StaticInit();
    CBillboard::StaticInit();
    CBillboardLine::StaticInit();
    CForm::StaticInit();


    // Game
    CMatrixProgressBar::StaticInit();
    CMatrixMapStatic::StaticInit();
    CMatrixMapObject::StaticInit();
    CMatrixFlyer::StaticInit();
    CMatrixMapGroup::StaticInit();
    CInterface::StaticInit();
    CMultiSelection::StaticInit();
    CTerSurface::StaticInit();
    CBottomTextureUnion::StaticInit();
    CSkinManager::StaticInit();
    CMatrixHint::StaticInit();
    CInstDraw::StaticInit();
    SInshorewave::StaticInit();

    g_Flags = 0; //GFLAG_FORMACCESS;

}

void MatrixGameInit(HINSTANCE inst,HWND wnd,wchar * map,SRobotsSettings * set,wchar * txt_start,wchar * txt_win,wchar * txt_loss, wchar *planet)
{
    static_init();

    DTRACE();

    g_MatrixHeap = HNew(NULL) CHeap;

    CFile::AddPackFile(L"DATA\\robots.pkg", NULL);
    CFile::OpenPackFiles();

    CLoadProgress   lp;
    g_LoadProgress = &lp;
DCP();

    CStorage    stor_cfg(g_MatrixHeap);
    bool        stor_cfg_present = false;
    CWStr       stor_cfg_name(g_MatrixHeap);
    if (CFile::FileExist(stor_cfg_name, FILE_CONFIGURATION))
    {
        stor_cfg.Load(FILE_CONFIGURATION);
        stor_cfg_present = true;
    }


	g_MatrixData = HNew(g_MatrixHeap) CBlockPar(1, g_MatrixHeap);
    if (stor_cfg_present)
    {
        stor_cfg.RestoreBlockPar(L"da", *g_MatrixData);

        if (CFile::FileExist(stor_cfg_name, L"cfg\\robots\\cfg.txt"))
        {
            CBlockPar *bpc = g_MatrixData->BlockGet(L"Config");
            bpc->LoadFromTextFile(L"cfg\\robots\\cfg.txt");
        }

    } else
    {
        g_MatrixData->LoadFromTextFile(L"cfg\\robots\\data.txt");
    }


    {
        CBlockPar *repl = g_MatrixData->BlockGetAdd(PAR_REPLACE);

        // init menu replaces

        CBlockPar *rr = g_MatrixData->BlockGet(IF_LABELS_BLOCKPAR)->BlockGet(L"Replaces");
        int cnt = rr->ParCount();
        for (int i=0;i<cnt;++i)
        {
            repl->ParAdd(rr->ParGetName(i), rr->ParGet(i));
        }


        if (txt_start)
        {
            if (txt_start[0] >= '1' && txt_start[0] <= '6')
            {

                repl->ParSetAdd(PAR_REPLACE_BEGIN_ICON_RACE, CWStr(txt_start,1,g_MatrixHeap));
                repl->ParSetAdd(PAR_REPLACE_DIFFICULTY, CWStr(txt_start+1,2,g_MatrixHeap));
                repl->ParSetAdd(PAR_REPLACE_BEGIN_TEXT, txt_start + 3);
            } else
            {
                repl->ParSetAdd(PAR_REPLACE_BEGIN_ICON_RACE, L"1");
                repl->ParSetAdd(PAR_REPLACE_BEGIN_TEXT, txt_start);
            }

            
        } else
        {
            repl->ParSetAdd(PAR_REPLACE_BEGIN_ICON_RACE, L"1");
            repl->ParSetAdd(PAR_REPLACE_BEGIN_TEXT, L"Go! Go! Go!");
        }

        if (txt_win)
        {
            repl->ParSetAdd(PAR_REPLACE_END_TEXT_WIN, txt_win);

        } else
        {
            repl->ParSetAdd(PAR_REPLACE_END_TEXT_WIN, L"Good job man :)");
        }
        if (txt_loss)
        {
            repl->ParSetAdd(PAR_REPLACE_END_TEXT_LOOSE, txt_loss);
        } else
        {
            repl->ParSetAdd(PAR_REPLACE_END_TEXT_LOOSE, L"Sux :(");
        }
        if (planet)
        {
            repl->ParSetAdd(PAR_REPLACE_END_TEXT_PLANET, planet);
        } else
        {
            repl->ParSetAdd(PAR_REPLACE_END_TEXT_PLANET, L"Luna");
        }
    }
    


DCP();

	CacheInit();
DCP();


    if (g_RangersInterface)
    {
        // run from rangers
        // do not set fullscreen
        g_MatrixData->BlockGet(L"Config")->ParSet(L"FullScreen", L"0");
    }

	L3GInit(inst,*g_MatrixData->BlockGet(L"Config"),L"MatrixGame",L"Matrix Game",wnd);

    if (set)
    {
        g_ScreenX = set->m_ResolutionX;
        g_ScreenY = set->m_ResolutionY;
    }

    g_Render = HNew(g_MatrixHeap) CRenderPipeline;   // prepare pipelines

	ShowWindow(g_Wnd, SW_SHOWNORMAL);
	UpdateWindow(g_Wnd);







//#ifdef _DEBUG
//    CBitmap b,b1,br;
//    b1.LoadFromPNG(L"bla1.png");
//    b.CreateRGBA(b1.SizeX(),b1.SizeY());
//    b.Copy(CPoint(0,0), b1.Size(), b1, CPoint(0,0));
//
//
//    SHintElement he[] = 
//    {
//        {&b, HEM_CENTER},
//        {NULL, HEM_COPY},
//        {&b, HEM_CENTER_LEFT_5},
//        {NULL, HEM_COPY},
//        {&b, HEM_CENTER_RIGHT_5},
//        {NULL, HEM_LAST},
//    };
//
//
//    CMatrixHint *hint = CMatrixHint::Build(0,he);
//    hint->Show(150,180);
//#endif






DCP();

    g_Config.SetDefaults();
    g_Config.ReadParams();
    if (set) g_Config.ApplySettings(set);


DCP();
	

	g_MatrixMap =  HNew(g_MatrixHeap) CMatrixMapLogic;

	g_MatrixMap->LoadSide(*g_MatrixData->BlockGet(L"Side"));
	//g_MatrixMap->LoadTactics(*g_MatrixData->BlockGet(L"Tactics"));
	g_IFaceList = HNew(g_MatrixHeap) CIFaceList;

	// load new map
DCP();
    g_MatrixMap->RobotPreload();
    

    CStorage stor(g_CacheHeap);
DCP();

    CWStr mapname(g_CacheHeap);

    if(map)
    {
        mapname.Set(L"Matrix\\Map\\");
        mapname.Add(map);
    } else
    {
        mapname = g_MatrixData->BlockGet(L"Config")->Par(L"Map");
    }

	stor.Load(mapname);
DCP();
	
    if (0 > g_MatrixMap->PrepareMap(stor, mapname))
    {
        ERROR_S(L"Unable to load map. Error happens.");
    }

    CWStr mn(g_MatrixMap->MapName(), g_MatrixHeap);
    mn.LowerCase();
    if (mn.Find(L"demo") >= 0)
    {
        SETFLAG(g_MatrixMap->m_Flags, MMFLAG_AUTOMATIC_MODE|MMFLAG_FLYCAM|MMFLAG_FULLAUTO);
    }

    g_MatrixMap->CalcCannonPlace();
    SSpecialBot::LoadAIRobotType(*g_MatrixData->BlockGet(L"AIRobotType"));


    g_LoadProgress->SetCurLP(LP_PREPARININTERFACE);
    g_LoadProgress->InitCurLP(701);


    CBlockPar bpi(1, g_CacheHeap);
    if (stor_cfg_present)
    {
        stor_cfg.RestoreBlockPar(L"if", bpi);
    } else
    {
        bpi.LoadFromTextFile(IF_PATH);
    }


    g_ConfigHistory = HNew(g_MatrixHeap) CHistory;
    CInterface *pInterface = NULL;

    g_LoadProgress->SetCurLPPos(10);

    CMatrixHint::PreloadBitmaps();

    bool iface_save = false;

    g_PopupHead = (SMenuItemText*)HAlloc(sizeof(SMenuItemText)*MENU_HEAD_ITEMS, g_MatrixHeap);
    for(int i = 0; i < MENU_HEAD_ITEMS; i++){
        g_PopupHead[i].SMenuItemText::SMenuItemText(g_MatrixHeap);
    }

    g_PopupHull = (SMenuItemText*)HAlloc(sizeof(SMenuItemText)*MENU_HULL_ITEMS, g_MatrixHeap);
    for(int i = 0; i < MENU_HULL_ITEMS; i++){
        g_PopupHull[i].SMenuItemText::SMenuItemText(g_MatrixHeap);
    }

    g_PopupWeaponNormal = (SMenuItemText*)HAlloc(sizeof(SMenuItemText)*MENU_WEAPONNORM_ITEMS, g_MatrixHeap);
    for(int i = 0; i < MENU_WEAPONNORM_ITEMS; i++){
        g_PopupWeaponNormal[i].SMenuItemText::SMenuItemText(g_MatrixHeap);
    }

    g_PopupWeaponExtern = (SMenuItemText*)HAlloc(sizeof(SMenuItemText)*MENU_WEAPONEXTERN_ITEMS, g_MatrixHeap);
    for(int i = 0; i < MENU_WEAPONEXTERN_ITEMS; i++){
        g_PopupWeaponExtern[i].SMenuItemText::SMenuItemText(g_MatrixHeap);
    }

    g_PopupChassis = (SMenuItemText*)HAlloc(sizeof(SMenuItemText)*MENU_CHASSIS_ITEMS, g_MatrixHeap);
    for(int i = 0; i < MENU_CHASSIS_ITEMS; i++){
        g_PopupChassis[i].SMenuItemText::SMenuItemText(g_MatrixHeap);
    }
    
    CIFaceMenu::m_MenuGraphics = HNew(g_MatrixHeap) CInterface;

    g_PopupMenu = HNew(g_MatrixHeap) CIFaceMenu;

    pInterface = HNew(g_MatrixHeap) CInterface;
	iface_save |= pInterface->Load(bpi, IF_TOP);
	LIST_ADD(pInterface, g_IFaceList->m_First, g_IFaceList->m_Last, m_PrevInterface, m_NextInterface);

    g_LoadProgress->SetCurLPPos(100);

    pInterface = HNew(g_MatrixHeap) CInterface;
	iface_save |= pInterface->Load(bpi, IF_MINI_MAP);
	LIST_ADD(pInterface, g_IFaceList->m_First, g_IFaceList->m_Last, m_PrevInterface, m_NextInterface);

    g_LoadProgress->SetCurLPPos(200);

    pInterface = HNew(g_MatrixHeap) CInterface;
	iface_save |= pInterface->Load(bpi, IF_RADAR);
	LIST_ADD(pInterface, g_IFaceList->m_First, g_IFaceList->m_Last, m_PrevInterface, m_NextInterface);

    g_LoadProgress->SetCurLPPos(300);

    pInterface = HNew(g_MatrixHeap) CInterface;
	iface_save |= pInterface->Load(bpi, IF_BASE);
	LIST_ADD(pInterface, g_IFaceList->m_First, g_IFaceList->m_Last, m_PrevInterface, m_NextInterface);
    
    g_IFaceList->m_BaseX = Float2Int(pInterface->m_xPos);
    g_IFaceList->m_BaseY = Float2Int(pInterface->m_yPos);
    g_LoadProgress->SetCurLPPos(400);

    pInterface = HNew(g_MatrixHeap) CInterface;
	iface_save |= pInterface->Load(bpi, IF_MAIN);
    g_IFaceList->SetMainPos(pInterface->m_xPos, pInterface->m_yPos);
	LIST_ADD(pInterface, g_IFaceList->m_First, g_IFaceList->m_Last, m_PrevInterface, m_NextInterface);

    g_LoadProgress->SetCurLPPos(500);

    pInterface = HNew(g_MatrixHeap) CInterface;
	iface_save |= pInterface->Load(bpi, IF_HINTS);
	LIST_ADD(pInterface, g_IFaceList->m_First, g_IFaceList->m_Last, m_PrevInterface, m_NextInterface);
    g_IFaceList->m_Hints = pInterface;

    g_LoadProgress->SetCurLPPos(600);

    iface_save |= CIFaceMenu::LoadMenuGraphics(bpi);
	//LIST_ADD(CIFaceMenu::m_MenuGraphics, g_IFaceList->m_First, g_IFaceList->m_Last, m_PrevInterface, m_NextInterface);

    g_IFaceList->ConstructorButtonsInit();

    g_LoadProgress->SetCurLPPos(700);

    g_PopupWeaponExtern[0].text = g_MatrixData->BlockGet(IF_LABELS_BLOCKPAR)->BlockGet(L"Base")->ParGet(L"none");
    g_PopupWeaponNormal[0].text = g_MatrixData->BlockGet(IF_LABELS_BLOCKPAR)->BlockGet(L"Base")->ParGet(L"none");
    g_PopupHead[0].text = g_MatrixData->BlockGet(IF_LABELS_BLOCKPAR)->BlockGet(L"Base")->ParGet(L"none");


    if(g_RangersInterface)
    {
        g_MatrixMap->m_Transition.BuildTexture();
        g_RangersInterface->m_Begin();
    }

    if (set) set->ApplyVideoParams();

    g_MatrixMap->m_Transition.RenderToPrimaryScreen();

    CMatrixEffect::InitEffects(*g_MatrixData);
    g_MatrixMap->CreatePoolDefaultResources(true);
    g_MatrixMap->InitObjectsLights();

    g_MatrixMap->GetPlayerSide()->Select(BUILDING, g_MatrixMap->GetPlayerSide()->m_ActiveObject);
    g_MatrixMap->m_Cursor.Select(CURSOR_ARROW);

    if (!FLAG(g_MatrixMap->m_Flags, MMFLAG_FULLAUTO))
    {
        g_MatrixMap->EnterDialogMode(TEMPLATE_DIALOG_BEGIN);
    }

    // this code can be safetly removed from release : RELEASE_OFF

    //if (iface_save) bpi.SaveInTextFile(IF_PATH, true);


}

void SRobotsSettings::ApplyVideoParams(void)
{
    DTRACE();
    RESETFLAG(g_Flags, GFLAG_FULLSCREEN);
    
    int bpp;
	D3DDISPLAYMODE d3ddm;
	ASSERT_DX(g_D3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT,&d3ddm));
    if (d3ddm.Format == D3DFMT_X8R8G8B8 || d3ddm.Format == D3DFMT_A8R8G8B8)
    {
        d3ddm.Format=D3DFMT_A8R8G8B8;
        bpp = 32;
    } else
    {
        bpp = 16;
    }

    RECT rect;
    GetClientRect(g_Wnd, &rect);

    bool is_window_mode = (rect.right != d3ddm.Width || rect.bottom != d3ddm.Height);
    bool change_refresh_rate = m_RefreshRate != 0 && m_RefreshRate != d3ddm.RefreshRate;
    int  refresh_rate_required = change_refresh_rate ? m_RefreshRate : 0;

	ZeroMemory( &g_D3Dpp, sizeof(g_D3Dpp) );

    if (bpp != m_BPP || m_ResolutionX != rect.right || m_ResolutionY != rect.bottom || change_refresh_rate)
    {
        // reinitialization required

        if (is_window_mode)
        {
            if (bpp != m_BPP) goto full_screen_mode;
            if (change_refresh_rate) goto full_screen_mode;

            g_D3Dpp.Windowed = TRUE;

            RECT r1,r2;
            GetWindowRect(g_Wnd, &r1);
            GetClientRect(g_Wnd, &r2);
            SetWindowPos(g_Wnd, NULL, 0,0, m_ResolutionX + (r1.right-r1.left-r2.right), m_ResolutionY+(r1.bottom-r1.top-r2.bottom), SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOACTIVATE);



        } else
        {
full_screen_mode:

            SETFLAG(g_Flags, GFLAG_FULLSCREEN);
            g_D3Dpp.Windowed = FALSE;

            if (is_window_mode)
            {
                // transition from window mode
                SetWindowLong(g_Wnd, GWL_STYLE, WS_POPUP|WS_VISIBLE);
                MoveWindow(g_Wnd, 0, 0, m_ResolutionX, m_ResolutionY, false);
            }
        }

		g_D3Dpp.SwapEffect = D3DSWAPEFFECT_FLIP;
        g_D3Dpp.BackBufferFormat = (m_BPP==16)?D3DFMT_R5G6B5:D3DFMT_A8R8G8B8;
        g_D3Dpp.EnableAutoDepthStencil = TRUE;
		g_D3Dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
		g_D3Dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        g_D3Dpp.FullScreen_RefreshRateInHz = refresh_rate_required;
        g_D3Dpp.BackBufferWidth  = m_ResolutionX;
        g_D3Dpp.BackBufferHeight = m_ResolutionY;
        g_D3Dpp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
        g_D3Dpp.hDeviceWindow = g_Wnd;

        SETFLAG(g_Flags, GFLAG_STENCILAVAILABLE);


        HRESULT hr1 = D3DERR_DEVICELOST;
        HRESULT hr2 = D3DERR_DRIVERINTERNALERROR;
        HRESULT hr3 = D3DERR_INVALIDCALL;
        HRESULT hr4 = D3DERR_OUTOFVIDEOMEMORY;
        HRESULT hr5 = E_OUTOFMEMORY;

        HRESULT hr;
        if (D3D_OK != (hr = g_D3DD->Reset(&g_D3Dpp)))
        {
            if (bpp == 16)
            {
                g_D3Dpp.AutoDepthStencilFormat = D3DFMT_D16;
            } else
            {
                g_D3Dpp.AutoDepthStencilFormat = D3DFMT_D24X8;
            }
            if (D3D_OK != (hr = g_D3DD->Reset(&g_D3Dpp)))
            {
                ERROR_S(L"Sorry, unable to set this params.");
            }
            RESETFLAG(g_Flags, GFLAG_STENCILAVAILABLE);
        }

    }

	D3DVIEWPORT9 ViewPort;
	ZeroMemory(&ViewPort, sizeof(D3DVIEWPORT9));

    ViewPort.X      = 0;
	ViewPort.Y      = 0;
    ViewPort.Width  = m_ResolutionX;
	ViewPort.Height = m_ResolutionY;

	ViewPort.MinZ   = 0.0f;
	ViewPort.MaxZ   = 1.0f;

	
	ASSERT_DX(g_D3DD->SetViewport(&ViewPort));
   

	S3D_Default();
	D3DMATERIAL9 mtrl;
	ZeroMemory(&mtrl,sizeof(D3DMATERIAL9));
	mtrl.Diffuse.r = mtrl.Ambient.r = 1.0f;
	mtrl.Diffuse.g = mtrl.Ambient.g = 1.0f;
	mtrl.Diffuse.b = mtrl.Ambient.b = 1.0f;
	mtrl.Diffuse.a = mtrl.Ambient.a = 1.0f;
	mtrl.Specular.r = 0.5f;
	mtrl.Specular.g = 0.5f;
	mtrl.Specular.b = 0.5f;
	mtrl.Specular.a = 0.5f;
	g_D3DD->SetMaterial(&mtrl );
	g_D3DD->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);

	D3DXVECTOR3 vecDir;
	D3DLIGHT9 light;
	ZeroMemory( &light, sizeof(D3DLIGHT9));
	light.Type = D3DLIGHT_DIRECTIONAL;//D3DLIGHT_POINT;//D3DLIGHT_DIRECTIONAL;
	light.Diffuse.r  = GetColorR(g_MatrixMap->m_LightMainColorObj);
	light.Diffuse.g  = GetColorG(g_MatrixMap->m_LightMainColorObj);
	light.Diffuse.b  = GetColorB(g_MatrixMap->m_LightMainColorObj);
	light.Ambient.r  = 0.0f;
	light.Ambient.g  = 0.0f;
	light.Ambient.b  = 0.0f;
	light.Specular.r = GetColorR(g_MatrixMap->m_LightMainColorObj);
	light.Specular.g = GetColorG(g_MatrixMap->m_LightMainColorObj);
	light.Specular.b = GetColorB(g_MatrixMap->m_LightMainColorObj);
	//light.Range       = 0;
	light.Direction	= g_MatrixMap->m_LightMain;
//	light.Direction=D3DXVECTOR3(250.0f,-50.0f,-250.0f);
//	D3DXVec3Normalize((D3DXVECTOR3 *)(&(light.Direction)),(D3DXVECTOR3 *)(&(light.Direction)));
	ASSERT_DX(g_D3DD->SetLight(0,&light));
	ASSERT_DX(g_D3DD->LightEnable(0,TRUE));

}

void MatrixGameDeinit(void)
{
    DTRACE();

    SSpecialBot::ClearAIRobotType();

    g_Config.Clear();

    if (g_Render)
    {
        HDelete(CRenderPipeline, g_Render, g_MatrixHeap);
        g_Render = NULL;
    }

    CMatrixHint::ClearAll();
	
	if(g_MatrixMap) {
		ASSERT(g_MatrixHeap);

		HDelete(CMatrixMapLogic,g_MatrixMap,g_MatrixHeap);
		g_MatrixMap=NULL;
	}

    if(g_MatrixData){
        HDelete(CBlockPar,g_MatrixData,NULL);
        g_MatrixData = NULL;
    }

    CMatrixRobot::DestroyPneumaticData();

    if(g_IFaceList){
		ASSERT(g_MatrixHeap);
		HDelete(CIFaceList, g_IFaceList, g_MatrixHeap);
        g_IFaceList = NULL;
    }

    if(g_ConfigHistory){
		ASSERT(g_MatrixHeap);
		HDelete(CHistory, g_ConfigHistory, g_MatrixHeap);
        g_ConfigHistory = NULL;
    }
	
    if(CIFaceMenu::m_MenuGraphics){
        HDelete(CInterface, CIFaceMenu::m_MenuGraphics, g_MatrixHeap);
        CIFaceMenu::m_MenuGraphics = NULL;
    }

    if(g_PopupMenu){
        HDelete(CIFaceMenu, g_PopupMenu, g_MatrixHeap);
        g_PopupMenu = NULL;
    }
    
    if(g_PopupHead){
        for(int i = 0; i < MENU_HEAD_ITEMS; i++){
            g_PopupHead[i].text.CWStr::~CWStr(); 
        }
        
        HFree(g_PopupHead, g_MatrixHeap);
        g_PopupHead = NULL;
    }

    if(g_PopupHull){
        for(int i = 0; i < MENU_HULL_ITEMS; i++){
            g_PopupHull[i].text.CWStr::~CWStr();
        }
        
        HFree(g_PopupHull, g_MatrixHeap);
        g_PopupHull = NULL;
    }

    if(g_PopupWeaponNormal){
        for(int i = 0; i < MENU_WEAPONNORM_ITEMS; i++){
            g_PopupWeaponNormal[i].text.CWStr::~CWStr();
        }
        
        HFree(g_PopupWeaponNormal, g_MatrixHeap);
        g_PopupWeaponNormal = NULL;
    }

    if(g_PopupWeaponExtern){
        for(int i = 0; i < MENU_WEAPONEXTERN_ITEMS; i++){
            g_PopupWeaponExtern[i].text.CWStr::~CWStr();
        }
        
        HFree(g_PopupWeaponExtern, g_MatrixHeap);
        g_PopupWeaponExtern = NULL;
    }

    if(g_PopupChassis){
        for(int i = 0; i < MENU_CHASSIS_ITEMS; i++){
            g_PopupChassis[i].text.CWStr::~CWStr();
        }
        
        HFree(g_PopupChassis, g_MatrixHeap);
        g_PopupChassis = NULL;
    }

    if(g_Config.m_Labels){
        for(int i = 0;i < LABELS_LAST; i++){
            g_Config.m_Labels[i].CWStr::~CWStr();
        }
        HFree(g_Config.m_Labels, g_MatrixHeap);
        g_Config.m_Labels = NULL;
    }

    if(g_Config.m_Descriptions){
        for(int i = 0;i < DESCRIPTIONS_LAST; i++){
            g_Config.m_Descriptions[i].CWStr::~CWStr();
        }
        HFree(g_Config.m_Descriptions, g_MatrixHeap);
        g_Config.m_Descriptions = NULL;
    }

    CInstDraw::ClearAll();
  
    CFile::ReleasePackFiles();

    if(g_MatrixHeap) {
		HDelete(CHeap,g_MatrixHeap,NULL);
		g_MatrixHeap=NULL;
	}
}
