#include "stdafx.h"
#include "CIFaceButton.h"
#include "CIFaceElement.h"
#include "CInterface.h"
#include "MatrixObjectCannon.hpp"
#include "MatrixRenderPipeline.hpp"
#include "MatrixFlyer.hpp"
#include "3g.hpp"
#include "CAnimation.h"
#include "MatrixHint.hpp"

CIFaceElement::CIFaceElement(void):m_Flags(0)
{
	m_nId = 0;
	m_nGroup = 0;
	m_xPos = m_yPos = m_zPos = m_xSize = m_ySize = 0;
	m_NextElement = m_PrevElement = NULL;
	m_Param1 = m_Param2 = 0;
	SETFLAG(m_Flags,IFEF_VISIBLE);
    m_PosElInX = 0; 
    m_PosElInY = 0;
    m_VisibleAlpha = IS_VISIBLEA;
    m_CurState = IFACE_NORMAL;
    m_DefState = IFACE_NORMAL;

    ZeroMemory(m_Actions, sizeof(m_Actions));

    m_Animation = NULL;
    m_iParam = 0;


    
}

CIFaceElement::~CIFaceElement()
{
    if(m_Animation){
        HDelete(CAnimation, m_Animation, g_MatrixHeap);
    }
}
	
void CIFaceElement::ElementGeomInit(void *pObj, bool full_size)
{ 
	CIFaceElement* pElement = (CIFaceElement*)pObj;

	for(int nC = 0; nC < MAX_STATES;nC++){
		if(pElement->m_StateImages[nC].Set)
        {
            SVert_V4_UV *v = pElement->m_StateImages[nC].m_Geom;

            float litx = (1 / ((pElement->m_StateImages[nC].TexWidth)*2 ));
            float lity = (1 / ((pElement->m_StateImages[nC].TexHeight)*2 ));

            float u0,u1, v0,v1;


			u0 = (float)((pElement->m_StateImages[nC].xTexPos  / pElement->m_StateImages[nC].TexWidth) + litx);
			v0 = (float)((pElement->m_StateImages[nC].yTexPos  / pElement->m_StateImages[nC].TexHeight) + lity);

			if(!full_size && pElement->m_strName != L"MiniMap")
            {
				u1 = (float)(((pElement->m_StateImages[nC].xTexPos + pElement->m_xSize ) / pElement->m_StateImages[nC].TexWidth) + litx);
				v1 = (float)(((pElement->m_StateImages[nC].yTexPos + pElement->m_ySize) / pElement->m_StateImages[nC].TexHeight) + lity);

            } else
            {
				u1 = (float)(((pElement->m_StateImages[nC].xTexPos + pElement->m_StateImages[nC].TexWidth ) / pElement->m_StateImages[nC].TexWidth)	+ litx);
				v1 = (float)(((pElement->m_StateImages[nC].yTexPos + pElement->m_StateImages[nC].TexHeight) / pElement->m_StateImages[nC].TexHeight) + lity);
            }


            (v+0)->p.x = 0;                 (v+0)->p.y = pElement->m_ySize; (v+0)->p.z = 0; (v+0)->p.w = 1; (v+0)->tu = u0; (v+0)->tv = v1;
            (v+1)->p.x = 0;                 (v+1)->p.y = 0;                 (v+1)->p.z = 0; (v+1)->p.w = 1; (v+1)->tu = u0; (v+1)->tv = v0;
            (v+2)->p.x = pElement->m_xSize; (v+2)->p.y = pElement->m_ySize; (v+2)->p.z = 0; (v+2)->p.w = 1; (v+2)->tu = u1; (v+2)->tv = v1;
            (v+3)->p.x = pElement->m_xSize; (v+3)->p.y = 0;                 (v+3)->p.z = 0; (v+3)->p.w = 1; (v+3)->tu = u1; (v+3)->tv = v0;
		}
	}
}

void CIFaceElement::CheckGroupReset(CIFaceElement *pFirstElement, CIFaceElement* pButton){
	CIFaceButton *pObjectsList = (CIFaceButton*)pFirstElement; 
	while(pObjectsList != NULL){
		if(pObjectsList->m_nGroup == pButton->m_nGroup && pObjectsList != (CIFaceButton*)pButton &&
			(pObjectsList->m_Type == IFACE_CHECK_BUTTON || pObjectsList->m_Type == IFACE_CHECK_BUTTON_SPECIAL || pObjectsList->m_Type == IFACE_CHECK_PUSH_BUTTON)){
			pObjectsList->SetState(IFACE_NORMAL);
			pObjectsList->m_DefState = IFACE_NORMAL;
            if(pObjectsList->m_Type == IFACE_CHECK_BUTTON || pObjectsList->m_Type == IFACE_CHECK_BUTTON_SPECIAL){
                pObjectsList->Action(ON_UN_PRESS);
            }
		}
		pObjectsList = (CIFaceButton*)pObjectsList->m_NextElement;
	}
}

bool CIFaceElement::SetStateImage(IFaceElementState State, CTextureManaged *pImage, float x, float y, float width, float height)
{
    pImage->MipmapOff();
	m_StateImages[State].pImage = pImage;
    if (!m_StateImages[State].pImage->IsLoaded())
    {
        m_StateImages[State].pImage->Load();
    }
	m_StateImages[State].xTexPos = x;
	m_StateImages[State].yTexPos = y;
	m_StateImages[State].TexWidth = width;
	m_StateImages[State].TexHeight = height;
	m_StateImages[State].Set = TRUE;
	return TRUE;
}
LPDIRECT3DTEXTURE9 CIFaceElement::GetStateImage(IFaceElementState State)
{
	m_StateImages[State].pImage->Prepare();
	return m_StateImages[State].pImage->Tex();
}

bool CIFaceElement::SetState(IFaceElementState State)
{
	m_CurState = State;
	return TRUE;
}

IFaceElementState CIFaceElement::GetState()
{
	return m_CurState;
}

void CIFaceElement::BeforeRender(void)
{
    m_StateImages[m_CurState].pImage->Preload();

    if (HasClearRect())
    {
        D3DRECT r;
        
        int x = Float2Int(m_StateImages[m_CurState].m_Geom[1].p.x);
        int y = Float2Int(m_StateImages[m_CurState].m_Geom[1].p.y);
        r.x1 = m_ClearRect.left + x;
        r.y1 = m_ClearRect.left + y;
        r.x2 = m_ClearRect.right + x;
        r.y2 = m_ClearRect.bottom + y;
        CInterface::ClearRects_Add(r);

    }
}

void CIFaceElement::Render(BYTE alpha)
{
	DTRACE();
    CMatrixSideUnit* player_side = g_MatrixMap->GetPlayerSide();

    DCP();

    ASSERT_DX(g_D3DD->SetSamplerState(0,D3DSAMP_MAGFILTER,			D3DTEXF_POINT));
	ASSERT_DX(g_D3DD->SetSamplerState(0,D3DSAMP_MINFILTER,			D3DTEXF_POINT));
	ASSERT_DX(g_D3DD->SetSamplerState(0,D3DSAMP_MIPFILTER,			D3DTEXF_NONE));

    ASSERT_DX(g_D3DD->SetRenderState( D3DRS_ALPHATESTENABLE,   TRUE ));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHAREF, 0));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZENABLE,				D3DZB_FALSE ));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE,		FALSE));
    ASSERT_DX(g_D3DD->SetRenderState( D3DRS_ALPHABLENDENABLE,   TRUE ));
    SetColorOpSelect(0, D3DTA_TEXTURE);
    SetAlphaOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_TFACTOR);
    SetColorOpDisable(1);

    ASSERT_DX(g_D3DD->SetRenderState( D3DRS_TEXTUREFACTOR,     ((DWORD)alpha << 24) ));

    CInstDraw::BeginDraw(IDFVF_V4_UV);

    DCP();

    if(m_nId == IF_CALLHELL_ID && GetState() == IFACE_DISABLED)
    {
    DCP();

        m_StateImages[GetState()].pImage->Prepare();
    DCP();

        SVert_V4_UV geom[8];
        memcpy(geom, m_StateImages[GetState()].m_Geom, sizeof(SVert_V4_UV) * 4);
        memcpy(geom + 4, m_StateImages[GetState()].m_Geom, sizeof(SVert_V4_UV) * 4);

        float t=0;
        if(!g_MatrixMap->MaintenanceDisabled())  t=g_MatrixMap->BeforMaintenanceTimeT();
        
        float hg = geom[0].p.y - geom[1].p.y;
        float ht = geom[0].tv - geom[1].tv;
        float wg = geom[3].p.x - geom[1].p.x;

        geom[0].p.y -= hg * t;
        geom[2].p.y -= hg * t;
        geom[0].tv -= ht * t;
        geom[2].tv -= ht * t;

        geom[1+4].p.y = geom[0+4].p.y - hg * t;
        geom[3+4].p.y = geom[2+4].p.y - hg * t;
        geom[1+4].tv = geom[0+4].tv - ht * t;
        geom[3+4].tv = geom[2+4].tv - ht * t;

    DCP();
        float d = wg / float(m_StateImages[GetState()].pImage->GetSizeX());
    DCP();
        for (int i=4;i<8;++i)
        {
            geom[i].tu += d;
        }

        CInstDraw::AddVerts(geom, 
                            m_StateImages[GetState()].pImage);

        CInstDraw::AddVerts(geom + 4, 
                            m_StateImages[GetState()].pImage);
    DCP();

    } else
    {
    DCP();

#if defined _TRACE || defined _DEBUG
    DCP();
    int st = GetState();
    DCP();
    if (st < 0 || st >= 5) _asm int 3
    DCP();
    //if (CheckValidPtr(m_StateImages[st].pImage) < 0) _asm int 3
    DCP();
#endif

#if defined _TRACE || defined _DEBUG
    try
    {
#endif
        m_StateImages[GetState()].pImage->Prepare();
#if defined _TRACE || defined _DEBUG
    } catch (...)
    {
        ERROR_S(L"Вылет в интерфейсе: <" + m_strName + L">,<" + m_nId + L">,<" + m_iParam + L">");
    }
#endif
    DCP();

        CInstDraw::AddVerts(m_StateImages[GetState()].m_Geom, 
                            m_StateImages[GetState()].pImage);
    DCP();

        if(m_Animation){
    DCP();
            m_Animation->GetCurrentFrame()->m_StateImages[IFACE_NORMAL].pImage->Prepare();
    DCP();

    DCP();
            CInstDraw::AddVerts(m_Animation->GetCurrentFrame()->m_StateImages[IFACE_NORMAL].m_Geom, 
                                m_Animation->GetCurrentFrame()->m_StateImages[IFACE_NORMAL].pImage);
    DCP();
        }
    }
    DCP();
    CInstDraw::ActualDraw();

    DCP();


    ASSERT_DX(g_D3DD->SetRenderState( D3DRS_ALPHABLENDENABLE,       FALSE ));

    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZENABLE,				D3DZB_TRUE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE,		TRUE));

    ASSERT_DX(g_D3DD->SetRenderState( D3DRS_ALPHATESTENABLE,   FALSE ));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHAREF,			0x08 ));

    if(m_iParam == IF_MAP_PANELI)
    {
    DCP();
        g_MatrixMap->m_Minimap.Draw();
    DCP();
    }
    else if(m_iParam == IF_RADAR_PNI)
    {
    DCP();
        g_MatrixMap->m_Minimap.DrawRadar(float(g_IFaceList->m_IFRadarPosX+90), float(g_IFaceList->m_IFRadarPosY+93), RADAR_RADIUS);//90,92
    DCP();
    }


    

    
    return;
    //if(m_strName == IF_HELI_SHTUCHKA  || m_strName == IF_WEAPON_PANEL || m_strName == IF_FACTORY_PANEL1){
	   // D3DXMATRIX matWorld, matView, matProj, OldMatWorld, OldMatView, OldMatProj, RotMat;
	   // D3DVIEWPORT9 ViewPort, OldViewPort;
	   // //static float za = 0;

	   // ZeroMemory(&OldViewPort, sizeof(D3DVIEWPORT9));
	   // ZeroMemory(&ViewPort, sizeof(D3DVIEWPORT9));

    //    if(m_strName == IF_HELI_SHTUCHKA){
    //        ViewPort.X      = Float2Int(m_PosElInX + m_xSize+5);//600;
	   //     ViewPort.Y      = Float2Int(m_PosElInY + 10);//400;
	   //     ViewPort.Width  = g_ScreenX - ViewPort.X;
	   //     ViewPort.Height = g_ScreenY - ViewPort.Y;
    //    }else if(m_strName == IF_WEAPON_PANEL){
    //        ViewPort.X      = Float2Int(m_PosElInX + m_xSize);//600;
	   //     ViewPort.Y      = Float2Int(m_PosElInY + 25);//400;
	   //     ViewPort.Width  = g_ScreenX - ViewPort.X;
	   //     ViewPort.Height = g_ScreenY - ViewPort.Y;
    //    }else if(m_strName == IF_FACTORY_PANEL1){
    //        ViewPort.X      = Float2Int(m_PosElInX + m_xSize);//600;
	   //     ViewPort.Y      = Float2Int(m_PosElInY + 25);//400;
	   //     ViewPort.Width  = g_ScreenX - ViewPort.X;
	   //     ViewPort.Height = g_ScreenY - ViewPort.Y;
    //    }

	   // ViewPort.MinZ   = 0.0f;
	   // ViewPort.MaxZ   = 1.0f;
    ////Save old
	   // ASSERT_DX(g_D3DD->GetViewport(&OldViewPort));
	   // ASSERT_DX(g_D3DD->GetTransform( D3DTS_WORLD, &OldMatWorld));
	   // ASSERT_DX(g_D3DD->GetTransform( D3DTS_VIEW, &OldMatView));
	   // ASSERT_DX(g_D3DD->GetTransform( D3DTS_PROJECTION, &OldMatProj));

    ////Set new
	   // ASSERT_DX(g_D3DD->SetViewport(&ViewPort));
    //    if (FLAG(g_Flags,GFLAG_STENCILAVAILABLE))
    //    {
    //        ASSERT_DX(g_D3DD->Clear( 0, NULL, D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL/*|D3DCLEAR_TARGET*/, D3DCOLOR_XRGB(255,0,0), 1.0f, 0 ));
    //    } else
    //    {
	   //     ASSERT_DX(g_D3DD->Clear( 0, NULL, D3DCLEAR_ZBUFFER/*|D3DCLEAR_TARGET*/, D3DCOLOR_XRGB(255,0,0), 1.0f, 0 ));
    //    }

	   // //Render
    //    CVectorObject::DrawBegin();
    //    if(m_strName == IF_HELI_SHTUCHKA){
	   //     D3DXMatrixIdentity(&matWorld);
	   //     D3DXMatrixLookAtLH (&matView, &D3DXVECTOR3 (20, 20, 20),
		  //      &D3DXVECTOR3 (0.0f, 0.0f, 0.0f),
		  //      &D3DXVECTOR3 (0.0f, 0.0f, 1.0f));

    //        D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4, 1.0f, 1.0f, 300.0f);
	   //     ASSERT_DX(g_D3DD->SetTransform( D3DTS_WORLD, &matWorld));
	   //     ASSERT_DX(g_D3DD->SetTransform( D3DTS_VIEW, &matView ));
	   //     ASSERT_DX(g_D3DD->SetTransform( D3DTS_PROJECTION, &matProj));

    //        g_D3DD->SetRenderState(D3DRS_TEXTUREFACTOR, 0xFFFFFFFF);
    //        //g_MatrixMap->GetPlayerSide()->GetFlyer()->Draw();
    //    }else if(m_strName == IF_WEAPON_PANEL && g_MatrixMap->GetPlayerSide()->m_ActiveObject){
    //        CMatrixRobotAI* bot = ((CMatrixRobotAI*)g_MatrixMap->GetPlayerSide()->m_ActiveObject);
    //        
    //        D3DXVECTOR3 rob_pos(bot->m_PosX, bot->m_PosY, 0);

	   //     D3DXMatrixIdentity(&matWorld);	        
    //        D3DXMatrixLookAtLH (&matView, &D3DXVECTOR3 (rob_pos.x + 0, rob_pos.y + 100, rob_pos.z + 70),
    //            &rob_pos,
		  //      &D3DXVECTOR3 (0.0f, 0.0f, 1.0f));
    //        D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4, 1.0f, 1.0f, 300.0f);
	   //     ASSERT_DX(g_D3DD->SetTransform( D3DTS_WORLD, &matWorld));
	   //     ASSERT_DX(g_D3DD->SetTransform( D3DTS_VIEW, &matView ));
	   //     ASSERT_DX(g_D3DD->SetTransform( D3DTS_PROJECTION, &matProj));

    //        bot->Draw();

    //    }else if(m_strName == IF_FACTORY_PANEL1 && g_MatrixMap->GetPlayerSide()->m_ActiveObject){

    //        CMatrixBuilding* building = ((CMatrixBuilding*)g_MatrixMap->GetPlayerSide()->m_ActiveObject);
    //        
    //        D3DXVECTOR3 b_pos(building->m_Pos.x, building->m_Pos.y, 0);

	   //     D3DXMatrixIdentity(&matWorld);	        
    //        D3DXMatrixLookAtLH (&matView, &D3DXVECTOR3 (/*building->m_Matrix._21 **/ (b_pos.x + 100), /*building->m_Matrix._22 * */(b_pos.y + 150), (b_pos.z + 150)),
    //            &b_pos,
		  //      &D3DXVECTOR3 (0.0f, 0.0f, 1.0f));
    //        D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4, 1.0f, 1.0f, 300.0f);
	   //     ASSERT_DX(g_D3DD->SetTransform( D3DTS_WORLD, &matWorld));
	   //     ASSERT_DX(g_D3DD->SetTransform( D3DTS_VIEW, &matView ));
	   //     ASSERT_DX(g_D3DD->SetTransform( D3DTS_PROJECTION, &matProj));

    //        building->Draw();

    //    }
	   // CVectorObject::DrawEnd();
    //    g_Render->m_ObjClear();

    ////Return old
	   // ASSERT_DX(g_D3DD->SetTransform( D3DTS_WORLD, &OldMatWorld));
	   // ASSERT_DX(g_D3DD->SetTransform( D3DTS_VIEW, &OldMatView ));
	   // ASSERT_DX(g_D3DD->SetTransform( D3DTS_PROJECTION, &OldMatProj));
	   // ASSERT_DX(g_D3DD->SetViewport(&OldViewPort));
    //}


}

bool CIFaceElement::ElementCatch(CPoint mouse)
{
		if(
		PointLineCatch(
			D3DXVECTOR2(	m_StateImages[GetState()].m_Geom[0].p.x,
							m_StateImages[GetState()].m_Geom[0].p.y),
			D3DXVECTOR2(	m_StateImages[GetState()].m_Geom[1].p.x,
							m_StateImages[GetState()].m_Geom[1].p.y),
			D3DXVECTOR2(	(float)mouse.x, (float)mouse.y)) 
		&& 
		PointLineCatch(
			D3DXVECTOR2(	m_StateImages[GetState()].m_Geom[1].p.x,
							m_StateImages[GetState()].m_Geom[1].p.y),
			D3DXVECTOR2(	m_StateImages[GetState()].m_Geom[3].p.x,
							m_StateImages[GetState()].m_Geom[3].p.y),
			D3DXVECTOR2(	(float)mouse.x, (float)mouse.y)) 
		&&
		PointLineCatch(
			D3DXVECTOR2(	m_StateImages[GetState()].m_Geom[3].p.x,
							m_StateImages[GetState()].m_Geom[3].p.y),
			D3DXVECTOR2(	m_StateImages[GetState()].m_Geom[2].p.x,
							m_StateImages[GetState()].m_Geom[2].p.y),
			D3DXVECTOR2(	(float)mouse.x, (float)mouse.y)) 
		&&
		PointLineCatch(
			D3DXVECTOR2(	m_StateImages[GetState()].m_Geom[2].p.x,
							m_StateImages[GetState()].m_Geom[2].p.y),
			D3DXVECTOR2(	m_StateImages[GetState()].m_Geom[0].p.x,
							m_StateImages[GetState()].m_Geom[0].p.y),
			D3DXVECTOR2(	(float)mouse.x, (float)mouse.y))){
						return true;
	}
	
	return false;
}

void CIFaceElement::Reset()
{
    if(GetState() != IFACE_DISABLED){
        SetState(m_DefState);
        //if(m_strName == IF_BASE_PILON_HULL || m_strName == IF_BASE_PILON_CHASSIS || m_strName == IF_BASE_PILON_HEAD || m_strName == IF_BASE_PILON1 || m_strName == IF_BASE_PILON2 || m_strName == IF_BASE_PILON3 || m_strName == IF_BASE_PILON4 || m_strName == IF_BASE_PILON5 || m_strName == IF_BASE_HEAD_EMPTY || m_strName == IF_BASE_WEAPON_EMPTY){
            Action(ON_UN_FOCUS);
        //}
    }
}

bool CIFaceElement::ElementAlpha(CPoint mouse)
{
    int x = Float2Int(mouse.x - (m_PosElInX));
    int y = Float2Int(mouse.y - (m_PosElInY));
    DWORD color = m_StateImages[GetState()].pImage->GetPixelColor(Float2Int(m_StateImages[GetState()].xTexPos) + x, Float2Int(m_StateImages[GetState()].yTexPos) + y);
    if((color & (0xff000000)))
        return true;
    return false;
}

void CIFaceElement::Action(EActions action)
{ 
    if(m_Actions[action].m_function){
        if(m_strName == IF_HINTS_OK || m_strName == IF_HINTS_CANCEL || m_strName == IF_HINTS_CANCEL_MENU || m_strName == IF_HINTS_CONTINUE || m_strName == IF_HINTS_EXIT || m_strName == IF_HINTS_RESET || m_strName == IF_HINTS_SURRENDER || m_strName == IF_HINTS_HELP){
            DialogButtonHandler(m_Actions[action].m_function)();
            //SetVisibility(false);
        }else{
            void* a = (void*)(&m_Actions[action]);
            FCALLFROMCLASS2(a);
        }
    }
     
}

void CIFaceElement::LogicTakt(int ms)
{
    if(m_Animation) m_Animation->LogicTakt(ms);

    if(m_nId == GROUP_SELECTOR_ID){
        CMatrixSideUnit* player_side = g_MatrixMap->GetPlayerSide();
        int i = player_side->GetCurSelNum();
        float pos = (i+1.0f) / 3.0f;

        float x,y;
        if(pos <= 1){
            x = (float)(223 + 48*i);
            y = 45;
        }else if(pos > 1 && pos <= 2){
            x = (float)((223 + 48*i)-48*3);
            y = 48*2;
        }else if(pos > 2){
            x = (float)((223 + 48*i)-48*6);
            y = 48*3;
        }
        RecalcPos(x,y,false);
    }
}

void CIFaceElement::RecalcPos(const float &x, const float &y, bool ichanged)
{
    
    if(m_Animation && ichanged){
        m_Animation->RecalcPos(x, y);
    }
    
    float i_x = x;
    float i_y = y;
    if(!ichanged){
        i_x = m_StateImages[IFACE_NORMAL].m_Geom[1].p.x - m_xPos;
        i_y = m_StateImages[IFACE_NORMAL].m_Geom[1].p.y - m_yPos;
        m_xPos = x;
        m_yPos = y;
    }
    int nC;
    for(nC = 0; nC < MAX_STATES;nC++)
    {
        if(m_StateImages[nC].Set)
        {

            m_StateImages[nC].m_Geom[0].p = D3DXVECTOR4(0, m_ySize, 0,1);
            m_StateImages[nC].m_Geom[1].p = D3DXVECTOR4(0, 0, 0, 1);
		    m_StateImages[nC].m_Geom[2].p = D3DXVECTOR4(m_xSize, m_ySize, 0, 1);
		    m_StateImages[nC].m_Geom[3].p = D3DXVECTOR4(m_xSize, 0, 0, 1);
        }

    }


    D3DXVECTOR3 dp(m_xPos + i_x, m_yPos + i_y, m_zPos);

    nC = 0;
	while(m_StateImages[nC].Set && nC < MAX_STATES) {
		for(int i = 0; i < 4; i++)
        {
			m_StateImages[nC].m_Geom[i].p.x += dp.x;
            m_StateImages[nC].m_Geom[i].p.y += dp.y;
            m_StateImages[nC].m_Geom[i].p.z += dp.z;
            m_PosElInX = dp.x;
            m_PosElInY = dp.y;
		}
    	nC++;
	}
}


//void    CIFaceElement::GenerateClearRect(void)
//{
//    
//    D3DSURFACE_DESC desc;
//    m_StateImages[IFACE_NORMAL].pImage->Tex()->GetLevelDesc(0,&desc);
//
//    int x = Float2Int(m_StateImages[IFACE_NORMAL].xTexPos);
//    int y = Float2Int(m_StateImages[IFACE_NORMAL].yTexPos);
//    int szx = Float2Int(this->m_xSize);
//    int szy = Float2Int(this->m_ySize);
//
//    //if (desc.Format != D3DFMT_A8R8G8B8)
//    {
//        m_ClearRect.left = 0;
//        m_ClearRect.top = 0;
//        m_ClearRect.right = szx;
//        m_ClearRect.bottom = szy;
//
//        return;
//    }
//
//
//    CBitmap bmp(g_CacheHeap);
//    bmp.CreateGrayscale(szx,szy);
//
//    D3DLOCKED_RECT lr;
//    m_StateImages[IFACE_NORMAL].pImage->LockRect(lr, D3DLOCK_READONLY);
//
//    DWORD *src = (DWORD *)((BYTE *)lr.pBits + y * lr.Pitch + x * sizeof(DWORD));
//    BYTE  *dst = (BYTE *)bmp.Data();
//    for (int j = 0; j<szy; ++j, src = (DWORD *)(((BYTE *)src) + lr.Pitch), dst += bmp.Pitch() )
//    {
//        for (int i = 0; i<szx; ++i)
//        {
//            dst[i] = ((src[i] & 0xFF000000) == 0xFF000000) ? 255 : 0;
//        }
//    }
//
//    //static int cnt = 0;
//    //bmp.SaveInPNG(m_StateImages[IFACE_NORMAL].pImage->m_Name + L"_" + (cnt++) + L".bmp");
//
//
//    m_StateImages[IFACE_NORMAL].pImage->UnlockRect();
//    
//
//    m_ClearRect.top = 1;
//    m_ClearRect.left = 2;
//    m_ClearRect.right = 3;
//    m_ClearRect.bottom = 4;
//
//}

void CIFaceElement::SetVisibility(bool visible)
{
    INITFLAG(m_Flags,IFEF_VISIBLE,visible); 
    //if(!visible && g_IFaceList && g_IFaceList->m_CurrentHintControlName == m_strName){
    //    g_IFaceList->m_CurrentHint->Release(); 
    //    g_IFaceList->m_CurrentHint = NULL; 
    //    g_IFaceList->m_CurrentHintControlName = L"";
    //}
}