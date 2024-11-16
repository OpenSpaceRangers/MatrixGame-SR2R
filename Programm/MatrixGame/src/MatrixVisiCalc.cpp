#include "stdafx.h"
#include "MatrixMap.hpp"

struct STempPoints
{
    DWORD invisible;    // 0 - visible // 1 - invisible
    DWORD invisible_current;    // 0 - visible // 1 - invisible
    D3DXVECTOR3 p;

};
struct STempCalcs
{
    SPlane pl1;
    SPlane pl2;
    D3DXVECTOR3 p[4];
};

struct SPotEdge
{
    D3DXVECTOR3 p1,p2;
    SPlane pl1;
    SPlane pl2;
};

struct SVisGroup
{
    int x,y;    // group coord
    DWORD id;

    LPDWORD invisible[(MAP_GROUP_SIZE+1)*(MAP_GROUP_SIZE+1)];
    int invcnt;


    D3DXVECTOR3 srcpts[(MAP_GROUP_SIZE+1)*4];
    int         srcpts_cnt;

    float minz;

    D3DXVECTOR2 my_pos;

    void BuildPts(STempPoints *pts)
    {
        minz = 0;
        id = x + y * 65536;
        invcnt = 0;
        CMatrixMapGroup *g = g_MatrixMap->GetGroupByIndex(x,y);
        int xp = (x*MAP_GROUP_SIZE);
        int yp = (y*MAP_GROUP_SIZE);
        
        my_pos = D3DXVECTOR2(float(xp * GLOBAL_SCALE), float(yp * GLOBAL_SCALE));


        if (g != NULL)
        {
            minz = 100000;
            // build dest pts

            int w = min(MAP_GROUP_SIZE, (g_MatrixMap->m_Size.x - xp));
            int h = min(MAP_GROUP_SIZE, (g_MatrixMap->m_Size.y - yp));

            ASSERT(w != 0 && h != 0);

            for (int yy = 0; yy <= h; ++yy)
            {
                for (int xx = 0; xx <= w; ++xx)
                {
                    
                    int ccx = xx + xp;
                    int ccy = yy + yp;
                    STempPoints *zzp = pts + ccx+ccy*(g_MatrixMap->m_Size.x+1);
                    invisible[invcnt++] = &zzp->invisible;
           

                    SMatrixMapPoint *mp = g_MatrixMap->PointGet(xx + xp, yy+yp);
                    float zz = mp->z;
                    if (zz < 0) zz = 0;
                    if (minz > zz) minz = zz;
                }
            }
        }

        srcpts_cnt = 0;

        SMatrixMapPoint *mp;
        for (int yy = 0; yy <= MAP_GROUP_SIZE; ++yy)
        {
            // left

            srcpts[srcpts_cnt].x = xp * GLOBAL_SCALE;
            srcpts[srcpts_cnt].y = (yy+yp) * GLOBAL_SCALE;

            mp = g_MatrixMap->PointGetTest(xp, yy + yp);
            if (mp)
            {
                srcpts[srcpts_cnt].z = mp->z>=0?mp->z:0;
            } else srcpts[srcpts_cnt].z = 0;
            ++srcpts_cnt;


            // rite

            srcpts[srcpts_cnt].x = (xp + MAP_GROUP_SIZE) * GLOBAL_SCALE;
            srcpts[srcpts_cnt].y = (yy+yp) * GLOBAL_SCALE;

            mp = g_MatrixMap->PointGetTest((xp + MAP_GROUP_SIZE), yy + yp);
            if (mp)
            {
                srcpts[srcpts_cnt].z = mp->z>=0?mp->z:0;
            } else srcpts[srcpts_cnt].z = 0;
            ++srcpts_cnt;

        }
        for (int xx = 1; xx < MAP_GROUP_SIZE; ++xx)
        {
            // up
            srcpts[srcpts_cnt].x = (xx+xp) * GLOBAL_SCALE;
            srcpts[srcpts_cnt].y = yp * GLOBAL_SCALE;
            mp = g_MatrixMap->PointGetTest((xx+xp), yp);
            if (mp)
            {
                srcpts[srcpts_cnt].z = mp->z>=0?mp->z:0;
            } else srcpts[srcpts_cnt].z = 0;
            ++srcpts_cnt;

            // down
            srcpts[srcpts_cnt].x = (xx+xp) * GLOBAL_SCALE;
            srcpts[srcpts_cnt].y = (yp+MAP_GROUP_SIZE) * GLOBAL_SCALE;
            mp = g_MatrixMap->PointGetTest((xx+xp), (yp+MAP_GROUP_SIZE));
            if (mp)
            {
                srcpts[srcpts_cnt].z = mp->z>=0?mp->z:0;
            } else srcpts[srcpts_cnt].z = 0;
            ++srcpts_cnt;
        }
    }



    void BuildShadowFor(const D3DXVECTOR3 &ptfrom, int ptcnt, STempPoints *pts, CBuf &pe)
    {

        for (int i=0;i<ptcnt;++i)
                pts[i].invisible_current = 0;


        SPotEdge *eb = pe.Buff<SPotEdge>();
        SPotEdge *ee = pe.BuffEnd<SPotEdge>();

        SPlane vpl;

        for (;eb < ee;++eb)
        {
            bool os1 = eb->pl1.IsOnSide(ptfrom);
            bool os2 = eb->pl2.IsOnSide(ptfrom);
            if (os1 ^ os2)
            {
            } else
            {
                continue;
            }


    #ifdef _DEBUG
            CHelper::Create(10000,888)->Cone(eb->p1, eb->p2, 2, 2, 0xFF00FF00, 0xFF00FF00, 3);

            //CHelper::Create(10000,888)->Cone(ptfrom, *pp0, 1, 1, 0xFF8080FF, 0xFF8080FF, 3);
            //CHelper::Create(10000,888)->Cone(ptfrom, *pp1, 1, 1, 0xFF8080FF, 0xFF8080FF, 3);
    #endif


            D3DXVECTOR2 p20(eb->p1.x, eb->p1.y);
            D3DXVECTOR2 p21(eb->p2.x, eb->p2.y);
            bool ppcam = PointLineCatch(p20, p21, D3DXVECTOR2(ptfrom.x,ptfrom.y));
            

            if (ppcam)
            {
                SPlane::BuildFromPoints(vpl, ptfrom, eb->p1, eb->p2);
            } else
            {
                SPlane::BuildFromPoints(vpl, ptfrom, eb->p2, eb->p1);
            }
    #ifdef _DEBUG

            //CHelper::Create(10000,888)->Cone(*pp0, *pp0 + vpl.norm * 10, 1, 1, 0xFF8080FF, 0xFF8080FF, 3);

    #endif

            STempPoints *tp = pts;

            for (int i=0;i<ptcnt;++i,++tp)
            {
                if (!tp->invisible) continue; // не нужно проверять видимые части
                if (tp->invisible_current) continue; // уже пометили как невидимую

                bool vv = PointLineCatch(p20, p21, D3DXVECTOR2(tp->p.x,tp->p.y));
                if (vv == ppcam) continue;
            
                if (!(((!ppcam) ^ PointLineCatch(D3DXVECTOR2(ptfrom.x,ptfrom.y), p20, D3DXVECTOR2(tp->p.x,tp->p.y))) &&
                ((!ppcam) ^ PointLineCatch(p21, D3DXVECTOR2(ptfrom.x,ptfrom.y), D3DXVECTOR2(tp->p.x,tp->p.y))))) continue;

                if (!vpl.IsOnSide(tp->p))
                {

                    tp->invisible_current = 1;
                }
            }
        }

        STempPoints *tp = pts;

        // реально не видно, бля
        for (int i=0;i<ptcnt;++i,++tp)
        {
            tp->invisible = tp->invisible & tp->invisible_current;
        }



    }

    void CalcVis(SVisGroup *vg, int gcnt, int ptcnt, STempPoints *pts, STempCalcs *calcs, CBuf &pe)
    {

        CBuf    levels_idxs(g_MatrixHeap);
        CBuf    groups(g_MatrixHeap);
       

        float z0 = minz + GLOBAL_SCALE + 1;


        // TEMP:
        z0 = g_MatrixMap->m_Camera.GetFrustumCenter().z;


        for (;;) // loop by levels
        {


            // building shadow...

            // set full shadow
            for (int i=0;i<ptcnt;++i)
                    pts[i].invisible = 1;



            for (int i=0;i<srcpts_cnt;++i)
            {
                if (z0 >= srcpts[i].z)
                {
                    BuildShadowFor(D3DXVECTOR3(srcpts[i].x, srcpts[i].y, z0), ptcnt, pts, pe);
                }
            }

            // TEMP:

            //CHelper::DestroyByGroup(888);
            for (int i=0;i<ptcnt;++i)
            {
                if (pts[i].invisible)
                {
#ifdef _DEBUG
                    CHelper::Create(1000,888)->Sphere(pts[i].p, 2, 3, 0xFF00FF00);
#endif
                }
            }

            return;
            // TEMP:


            bool was_invis = false;


            for (int i=0;i<gcnt;++i)
            {
                if (vg[i].invcnt == 0) continue; // always not visible

                if (D3DXVec2LengthSq(&(vg[i].my_pos - my_pos)) > MAX_VIEW_DISTANCE_SQ) continue;

                // check already in
                DWORD *g0 = groups.Buff<DWORD>();
                DWORD *g1 = groups.BuffEnd<DWORD>();
                for(;g0<g1;++g0)
                {
                    if (vg[i].id == *g0) break;
                }
                if (g0 < g1) continue; // alredy present

                int v = 0;
                for (; v < vg[i].invcnt; ++v)
                {
                    if (*vg[i].invisible[v] == 0) break;
                }
                if (v < vg[i].invcnt) continue; // visible, сука

                groups.Dword(vg[i].id);

                was_invis = true;
            }

            levels_idxs.Int(groups.Len() / sizeof(DWORD));

            z0 += GLOBAL_SCALE;
            if (!was_invis) break;



        }

        SGroupVisibility *gv = g_MatrixMap->m_GroupVis + x + y * g_MatrixMap->m_GroupSize.x;
        gv->z_from = minz + GLOBAL_SCALE + 1;

        gv->levels_cnt = levels_idxs.Len() / sizeof(int);
        gv->levels = (int *)HAlloc(levels_idxs.Len(), g_MatrixHeap);
        memcpy(gv->levels, levels_idxs.Get(), levels_idxs.Len());

        gv->vis_cnt = groups.Len() / sizeof(DWORD);
        gv->vis = (PCMatrixMapGroup *)HAlloc(gv->vis_cnt * sizeof(PCMatrixMapGroup), g_MatrixHeap);
        for (int t=0;t<gv->vis_cnt;++t)
        {
            DWORD id = groups.Buff<DWORD>()[t];
            gv->vis[t] = g_MatrixMap->GetGroupByIndex(id & 0x0000FFFF, (id >> 16));
        }

    }

};

void CMatrixMap::CalcVis(void)
//void CMatrixMap::CalcVisTemp(int from, int to, const D3DXVECTOR3 &ptfrom)
{
    DTRACE();

#ifdef _DEBUG
    CHelper::DestroyByGroup(888);
#endif


    //D3DXVECTOR3 ptfrom(g_MatrixMap->m_Camera.GetFrustumCenter());

    CBuf pe(g_MatrixHeap);
   
    int cnt_p = (g_MatrixMap->m_Size.y+1) * (g_MatrixMap->m_Size.x+1);

    STempPoints *aps = (STempPoints *)HAlloc(cnt_p * sizeof(STempPoints), g_MatrixHeap);
    STempCalcs *bla = (STempCalcs *)HAlloc(g_MatrixMap->m_Size.y * g_MatrixMap->m_Size.x * sizeof(STempCalcs), g_MatrixHeap);

    STempCalcs *tt = bla;
    STempPoints *tp = aps;

    for (int j=0;j<=g_MatrixMap->m_Size.y;++j)
    {
        for (int i=0;i<=g_MatrixMap->m_Size.x;++i,++tp)
        {
            tp->p.x = i * GLOBAL_SCALE;
            tp->p.y = j * GLOBAL_SCALE;
            tp->p.z = g_MatrixMap->PointGet(i,j)->z_land;
        }
    }

    for (int j=0;j<g_MatrixMap->m_Size.y;++j)
    {
        for (int i=0;i<g_MatrixMap->m_Size.x;++i,++tt)
        {
            SMatrixMapUnit *un = g_MatrixMap->UnitGet(i,j);
            if (!un->IsLand()) continue;
            SMatrixMapPoint *po = g_MatrixMap->PointGet(i,j);

            D3DXVECTOR3 p[4];

            p[0].x = i * GLOBAL_SCALE;
            p[0].y = (j+1) * GLOBAL_SCALE;
            p[0].z = (po+g_MatrixMap->m_Size.x+1)->z_land;
            if (p[0].z < 0) p[0].z = 0;

            p[1].x = i * GLOBAL_SCALE;
            p[1].y = j * GLOBAL_SCALE;
            p[1].z = po->z_land;
            if (p[1].z < 0) p[1].z = 0;

            p[2].x = (i+1) * GLOBAL_SCALE;
            p[2].y = (j+1) * GLOBAL_SCALE;
            p[2].z = (po+g_MatrixMap->m_Size.x+2)->z_land;
            if (p[2].z < 0) p[2].z = 0;

            p[3].x = (i+1) * GLOBAL_SCALE;
            p[3].y = j * GLOBAL_SCALE;
            p[3].z = (po+1)->z_land;
            if (p[3].z < 0) p[3].z = 0;

            memcpy(tt->p, p, sizeof(p));

            // 012
            SPlane::BuildFromPoints(tt->pl1, p[0],p[1],p[2]);

            // 132
            SPlane::BuildFromPoints(tt->pl2, p[1],p[3],p[2]);

        }
    }


    // calc potential edges
    {
        tt = bla;

        SPotEdge edg;

        for (int j=0;j<g_MatrixMap->m_Size.y;++j)
        {
            for (int i=0;i<g_MatrixMap->m_Size.x;++i,++tt)
            {

                if (!tt->pl1.IsOnSide(tt->p[3]))
                {
                    edg.p1 = tt->p[1];
                    edg.p2 = tt->p[2];
                    edg.pl1 = tt->pl1;
                    edg.pl2 = tt->pl2;

                    if (edg.pl1.norm != edg.pl2.norm)
                    {
                        pe.AnyStruct<SPotEdge>(edg);
                    }

                }
                if (j < (g_MatrixMap->m_Size.y-1))
                {
                    if (!tt->pl1.IsOnSide((tt+g_MatrixMap->m_Size.x)->p[2]))
                    {
                        edg.p1 = tt->p[0];
                        edg.p2 = tt->p[2];

                        edg.pl1 = tt->pl1;
                        edg.pl2 = (tt+g_MatrixMap->m_Size.x)->pl2;

                        if (edg.pl1.norm != edg.pl2.norm)
                        {
                            pe.AnyStruct<SPotEdge>(edg);
                        }
                    }
                }
                if (i < (g_MatrixMap->m_Size.x-1))
                {
                    if (!tt->pl2.IsOnSide((tt+1)->p[2]))
                    {
                        edg.p1 = tt->p[2];
                        edg.p2 = tt->p[3];

                        edg.pl1 = tt->pl2;
                        edg.pl2 = (tt+1)->pl1;

                        if (edg.pl1.norm != edg.pl2.norm)
                        {
                            pe.AnyStruct<SPotEdge>(edg);
                        }
                    }
                }
            }
        }

    }

 
    int gcnt = m_GroupSize.x * m_GroupSize.y;
    SVisGroup *vg = (SVisGroup *)HAllocClear(sizeof(SVisGroup) * gcnt, g_MatrixHeap);

    ClearGroupVis();
    m_GroupVis = (SGroupVisibility *)HAllocClear(sizeof(SGroupVisibility) * gcnt, g_MatrixHeap);

    for (int i=0;i<m_GroupSize.x;++i)
    {
        for (int j=0;j<m_GroupSize.y;++j)
        {
            SVisGroup *cvg = vg + i + j*m_GroupSize.x;
            cvg->x = i;
            cvg->y = j;
            cvg->BuildPts(aps);
        }
    }


    g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE,		FALSE);
    ASSERT_DX(g_D3DD->SetRenderState( D3DRS_ALPHATESTENABLE,   FALSE ));

	g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZENABLE,				D3DZB_FALSE));

	g_D3DD->SetSamplerState(0,D3DSAMP_ADDRESSU,			D3DTADDRESS_CLAMP);
	g_D3DD->SetSamplerState(0,D3DSAMP_ADDRESSV,			D3DTADDRESS_CLAMP);

    SetColorOpSelect(0, D3DTA_TFACTOR);
    SetAlphaOpDisable(0);
    SetColorOpDisable(1);


    g_D3DD->SetRenderState(D3DRS_TEXTUREFACTOR,0xFF003000);

    SVert_V4 v[4];
    v[0].p = D3DXVECTOR4(0,float(g_ScreenY),0,1);
    v[1].p = D3DXVECTOR4(0,0,0,1);
    v[2].p = D3DXVECTOR4(0,float(g_ScreenY),0,1);
    v[3].p = D3DXVECTOR4(0,0,0,1);

    // TEMP:
    int gx = TruncFloat(g_MatrixMap->m_Camera.GetFrustumCenter().x / MAP_GROUP_SIZE / GLOBAL_SCALE);
    int gy = TruncFloat(g_MatrixMap->m_Camera.GetFrustumCenter().y / MAP_GROUP_SIZE / GLOBAL_SCALE);
    vg[gx + gy * g_MatrixMap->m_GroupSize.x].CalcVis(vg,gcnt,cnt_p, aps, bla, pe);
    if (0)
    // TEMP:


    for (int i=0;i<gcnt;++i)
    {
        vg[i].CalcVis(vg,gcnt,cnt_p, aps, bla, pe);


        ASSERT_DX(g_D3DD->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0 ));
        g_D3DD->BeginScene();

    
        v[2].p.x = float(i * g_ScreenX / gcnt);
        v[3].p.x = v[2].p.x;
        CInstDraw::BeginDraw(IDFVF_V4);
        CInstDraw::AddVerts(v,NULL);
        CInstDraw::ActualDraw();


        g_D3DD->EndScene();
        g_D3DD->Present(NULL,NULL,NULL,NULL);
       
        bool esc=(GetAsyncKeyState(VK_ESCAPE) & 0x0001)==0x0001;

        if (esc) break;
    }


    //for (int i=0;i<gcnt;++i)
    //{
    //    vg[i].Release();
    //}
    HFree(vg, g_MatrixHeap);

    CStorage stor(g_MatrixHeap);
    stor.Load(MapName());

    stor.DelRecord(DATA_GROUPS_VIS);

    CStorageRecord sr(CWStr(DATA_GROUPS_VIS,g_MatrixHeap),g_MatrixHeap);
    sr.AddItem(CStorageRecordItem(CWStr(DATA_GROUPS_VIS_LEVELS,g_MatrixHeap), ST_INT32));
    sr.AddItem(CStorageRecordItem(CWStr(DATA_GROUPS_VIS_GROUPS,g_MatrixHeap), ST_INT32));
    sr.AddItem(CStorageRecordItem(CWStr(DATA_GROUPS_VIS_ZFROM,g_MatrixHeap), ST_FLOAT));
    stor.AddRecord(sr);
    
    CDataBuf *dbl = stor.GetBuf(DATA_GROUPS_VIS, DATA_GROUPS_VIS_LEVELS, ST_INT32);
    CDataBuf *dbg = stor.GetBuf(DATA_GROUPS_VIS, DATA_GROUPS_VIS_GROUPS, ST_INT32);
    CDataBuf *dbz = stor.GetBuf(DATA_GROUPS_VIS, DATA_GROUPS_VIS_ZFROM, ST_FLOAT);

    dbz->AddArray();

    for (int i=0;i<gcnt;++i)
    {
        int dbla = dbl->AddArray();
        int dbga = dbg->AddArray();

        SGroupVisibility *gv = m_GroupVis + i;

        dbz->AddToArray<float>(0,gv->z_from);

        dbl->AddToArray<int>(dbla, gv->levels, gv->levels_cnt);
        for (int j=0;j<gv->vis_cnt;++j)
        {
            for (int g=0;g<gcnt;++g)
            {
                if (m_Group[g] == gv->vis[j])
                {
                    dbg->AddToArray<int>(dbga, g);
                    break;
                }
            }
            
        }
    }

    //stor.Save(MapName());

    HFree(bla, g_MatrixHeap);
    HFree(aps, g_MatrixHeap);


}




// runtime



#define NPOS    4

struct CMatrixMap::SCalcVisRuntime
{
    D3DXVECTOR2       pos[4];
    PCMatrixMapGroup *vcmg;
    int i, j;

};

void CMatrixMap::CheckCandidate(SCalcVisRuntime &cvr, CMatrixMapGroup *cmg)
{
    if (cmg->IsPointIn(cvr.pos[0])) goto visible;
    if (cmg->IsPointIn(cvr.pos[1])) goto visible;
    if (cmg->IsPointIn(cvr.pos[2])) goto visible;
#if NPOS == 4
    if (cmg->IsPointIn(cvr.pos[3])) goto visible;
#endif

    int i0 = NPOS-1;
    int i1 = 0;
    while (i1<NPOS)
    {
        if (PointLineCatch(cvr.pos[i0],cvr.pos[i1],cmg->GetPos0()))
        {
            goto checknext;
        }
        if (PointLineCatch(cvr.pos[i0],cvr.pos[i1],cmg->GetPos1()))
        {
            goto checknext;
        }
        if (PointLineCatch(cvr.pos[i0],cvr.pos[i1],D3DXVECTOR2(cmg->GetPos0().x, cmg->GetPos1().y)))
        {
            goto checknext;
        }
        if (PointLineCatch(cvr.pos[i0],cvr.pos[i1],D3DXVECTOR2(cmg->GetPos1().x, cmg->GetPos0().y)))
        {
            goto checknext;
        }
        goto invisible;
checknext:
        i0 = i1++;

    }

    // last check : frustum
    if (cmg->IsInFrustum())
    {
visible:
        //cmg->SetVisible(true);
        (*cvr.vcmg) = cmg;
        ++m_VisibleGroupsCount;
        ++cvr.vcmg;
    } else
    {
invisible:
        //cmg->SetVisible(false);
        // so, map group is invisible.
        // if it is an edge of whole map, it's water can be visible
        if ((cvr.i == (m_GroupSize.x-1)) || (cvr.j == (m_GroupSize.y-1)))
        {
            D3DXVECTOR3 mins(cmg->GetPos0().x, cmg->GetPos0().y, WATER_LEVEL);
            D3DXVECTOR3 maxs(mins.x + float(MAP_GROUP_SIZE * GLOBAL_SCALE), mins.y + float(MAP_GROUP_SIZE * GLOBAL_SCALE), WATER_LEVEL);
            if (m_Camera.IsInFrustum(mins,maxs))
            {
                m_VisWater->AnyStruct<D3DXVECTOR2>(cmg->GetPos0());
            }
        }

    }

}

// visibility
void CMatrixMap::CalcMapGroupVisibility(void)
{
    DTRACE();

    SCalcVisRuntime cvr;

    D3DXVECTOR3 topfwd(m_Camera.GetFrustumLT() + m_Camera.GetFrustumRT());
    float cam_dir2 = D3DXVec2Length((D3DXVECTOR2 *)&topfwd);
    float k_cam = (float)fabs(topfwd.z) / cam_dir2;
    float k_etalon = m_Camera.GetFrustumCenter().z * INVERT(MAX_VIEW_DISTANCE);
    float k_etalon_fog = m_Camera.GetFrustumCenter().z * INVERT(MAX_VIEW_DISTANCE * FOG_NEAR_K);
    //float dist_naklon = (float)sqrt(dist + m_Camera.GetFrustumCenter().z * m_Camera.GetFrustumCenter().z);

    if (topfwd.z > 0 || k_cam < k_etalon_fog)
    {
        SETFLAG(m_Flags,MMFLAG_FOG_ON);

    } else
    {
        RESETFLAG(m_Flags,MMFLAG_FOG_ON);
    }

    if (k_cam < k_etalon || topfwd.z > 0)
    {
        //float d2 = dist * 2;
        float k;
        // right top <- left top
        if (m_Camera.GetFrustumLT().z >= (-0.001)) {k = MAX_VIEW_DISTANCE;} else
        {
            k = (m_minz - m_Camera.GetFrustumCenter().z) / m_Camera.GetFrustumLT().z;
            if (k > MAX_VIEW_DISTANCE) k = MAX_VIEW_DISTANCE;
        }
        cvr.pos[0].x = m_Camera.GetFrustumCenter().x + m_Camera.GetFrustumLT().x*k;
        cvr.pos[0].y = m_Camera.GetFrustumCenter().y + m_Camera.GetFrustumLT().y*k;

        if (m_Camera.GetFrustumRT().z >= (-0.001)) {k = MAX_VIEW_DISTANCE;} else
        {
            k = (m_minz - m_Camera.GetFrustumCenter().z) / m_Camera.GetFrustumRT().z;
            if (k > MAX_VIEW_DISTANCE) k = MAX_VIEW_DISTANCE;
        }
        cvr.pos[1].x = m_Camera.GetFrustumCenter().x + m_Camera.GetFrustumRT().x*k;
        cvr.pos[1].y = m_Camera.GetFrustumCenter().y + m_Camera.GetFrustumRT().y*k;

        D3DXVECTOR2 ex;
        D3DXVec2Normalize(&ex, &(cvr.pos[1]-cvr.pos[0]));
        ex *= GLOBAL_SCALE * MAP_GROUP_SIZE;
        cvr.pos[0] -= ex;
        cvr.pos[1] += ex;




        k = (m_minz - m_Camera.GetFrustumCenter().z) / m_Camera.GetFrustumRB().z;
        cvr.pos[2].x = m_Camera.GetFrustumCenter().x + m_Camera.GetFrustumRB().x*k;
        cvr.pos[2].y = m_Camera.GetFrustumCenter().y + m_Camera.GetFrustumRB().y*k;

        k = (m_minz - m_Camera.GetFrustumCenter().z) / m_Camera.GetFrustumLB().z;
        cvr.pos[3].x = m_Camera.GetFrustumCenter().x + m_Camera.GetFrustumLB().x*k;
        cvr.pos[3].y = m_Camera.GetFrustumCenter().y + m_Camera.GetFrustumLB().y*k;

        D3DXVECTOR2 disp(*(D3DXVECTOR2 *)&m_Camera.GetFrustumCenter() - (cvr.pos[2]+cvr.pos[3])*0.5f);
        cvr.pos[2] += disp;
        cvr.pos[3] += disp;

        // enable fog

        SETFLAG(m_Flags,MMFLAG_SKY_ON);

        
        {
            //float si, co;
            //SinCos(M_PI_MUL(0.5) - m_Camera.GetAngleX(), &si, &co);

            //D3DXVECTOR2 pt(MAX_VIEW_DISTANCE, -m_Camera.GetFrustumCenter().z);
            //D3DXVECTOR2 ptr;
            //ptr.x =  co * pt.x - si * pt.y;
            //ptr.y =  si * pt.x + co * pt.y;
            //ptr.y += m_Camera.GetFrustumCenter().z;

            //double c = (double(g_ScreenY) * 0.5) / tan( 0.5 * CAM_HFOV * double(g_ScreenY) * m_Camera.GetResXInversed() );

            //m_SkyHeight = float(g_ScreenY) * 0.5f - float(c * ptr.y / ptr.x);

            D3DXVECTOR3 p(m_Camera.GetDir());
            p.z = 0;
            float len = D3DXVec3Length(&p);
            if (len < 0.0001)
            {
                m_SkyHeight = -100;
            } else
            {
                p *= (1.0f / len);
                p = m_Camera.GetFrustumCenter() + p * MAX_VIEW_DISTANCE;
                p.z -= m_Camera.GetFrustumCenter().z * 1.5f;

                D3DXVECTOR2 ptr = m_Camera.Project(p, GetIdentityMatrix());
                m_SkyHeight = ptr.y;
            }

        }

        //m_DI.T(L"sh",CWStr(m_SkyHeight));

        //m_SkyHeight


    } else
    {
        RESETFLAG(m_Flags,MMFLAG_SKY_ON);

        float k;

        // right top <- left top
        k = (m_minz - m_Camera.GetFrustumCenter().z) / m_Camera.GetFrustumLT().z;
        cvr.pos[0].x = m_Camera.GetFrustumCenter().x + m_Camera.GetFrustumLT().x*k;
        cvr.pos[0].y = m_Camera.GetFrustumCenter().y + m_Camera.GetFrustumLT().y*k;

        k = (m_minz - m_Camera.GetFrustumCenter().z) / m_Camera.GetFrustumRT().z;
        cvr.pos[1].x = m_Camera.GetFrustumCenter().x + m_Camera.GetFrustumRT().x*k;
        cvr.pos[1].y = m_Camera.GetFrustumCenter().y + m_Camera.GetFrustumRT().y*k;

        k = (m_minz - m_Camera.GetFrustumCenter().z) / m_Camera.GetFrustumRB().z;
        cvr.pos[2].x = m_Camera.GetFrustumCenter().x + m_Camera.GetFrustumRB().x*k;
        cvr.pos[2].y = m_Camera.GetFrustumCenter().y + m_Camera.GetFrustumRB().y*k;

        k = (m_minz - m_Camera.GetFrustumCenter().z) / m_Camera.GetFrustumLB().z;
        cvr.pos[3].x = m_Camera.GetFrustumCenter().x + m_Camera.GetFrustumLB().x*k;
        cvr.pos[3].y = m_Camera.GetFrustumCenter().y + m_Camera.GetFrustumLB().y*k;

        if (m_Camera.GetFrustPlaneB().norm.z > 0)
        {
            D3DXVECTOR2 disp(*(D3DXVECTOR2 *)&m_Camera.GetFrustumCenter() - (cvr.pos[2]+cvr.pos[3])*0.5f);
            cvr.pos[2] += disp;
            cvr.pos[3] += disp;

        }
    }


    bool no_info_about_visibility = true;

    cvr.vcmg=m_VisibleGroups;
    m_VisibleGroupsCount = 0;

    if (m_GroupVis != NULL)
    {

        const D3DXVECTOR3 &cam = m_Camera.GetFrustumCenter();
        int gx = TruncDouble(cam.x * (1.0 / (GLOBAL_SCALE * MAP_GROUP_SIZE)));
        int gy = TruncDouble(cam.y * (1.0 / (GLOBAL_SCALE * MAP_GROUP_SIZE)));

        if (gx >= 0 && gx < m_GroupSize.x && gy >= 0 && gy < m_GroupSize.y)
        {
            SGroupVisibility *gv = m_GroupVis + gx + gy * m_GroupSize.x;

            int level = TruncFloat((cam.z - gv->z_from) / GLOBAL_SCALE);

            if (level >=0 && level < gv->levels_cnt)
            {

                int n = gv->levels[level];

                for (int i=0;i<n;++i)
                {
                    PCMatrixMapGroup g = gv->vis[i];

                    cvr.i = Float2Int(g->GetPos0().x) / int(MAP_GROUP_SIZE * GLOBAL_SCALE);
                    cvr.j = Float2Int(g->GetPos0().y) / int(MAP_GROUP_SIZE * GLOBAL_SCALE);

                    CheckCandidate(cvr,g);
                }


                no_info_about_visibility = false;
            }
            
        }
    }


    float minx = cvr.pos[0].x;
    float maxx = cvr.pos[0].x;
    float miny = cvr.pos[0].y;
    float maxy = cvr.pos[0].y;
    for (int k = 1; k<NPOS; ++k)
    {
        if (cvr.pos[k].x < minx) minx = cvr.pos[k].x;
        if (cvr.pos[k].x > maxx) maxx = cvr.pos[k].x;
        if (cvr.pos[k].y < miny) miny = cvr.pos[k].y;
        if (cvr.pos[k].y > maxy) maxy = cvr.pos[k].y;
    }

    int iminx = (int)floor(minx * (1.0/(MAP_GROUP_SIZE * GLOBAL_SCALE)))-1;
    int imaxx = (int)(maxx * (1.0/(MAP_GROUP_SIZE * GLOBAL_SCALE))) + 1;
    int iminy = (int)floor(miny * (1.0/(MAP_GROUP_SIZE * GLOBAL_SCALE)))-1;
    int imaxy = (int)(maxy * (1.0/(MAP_GROUP_SIZE * GLOBAL_SCALE))) + 1;

    m_VisWater->Clear();

    //DM("TIME", "TIME_STEP1 " + CStr(iminx) + " " + CStr(iminy)+ " " + CStr(imaxx)+ " " + CStr(imaxy));

    for (cvr.j = iminy; cvr.j<=imaxy /*m_GroupSize.y*/; ++cvr.j)
    {

        for (cvr.i = iminx; cvr.i<=imaxx /*m_GroupSize.x*/; ++cvr.i)
        {
            bool is_map = (cvr.i>=0) && (cvr.i<m_GroupSize.x) && (cvr.j>=0) && (cvr.j<m_GroupSize.y);
            if (is_map)
            {
                // calculate visibility of map group
	            CMatrixMapGroup * cmg = m_Group[cvr.j * m_GroupSize.x + cvr.i];

                if (cmg == NULL)
                {
                    goto water_calc;
                } else
                {
                    if (no_info_about_visibility)
                        CheckCandidate(cvr, cmg);

                }
            } else
            {
                // calculate visibility of free water
water_calc:

                D3DXVECTOR3 p0(float(cvr.i * (MAP_GROUP_SIZE * GLOBAL_SCALE)), float(cvr.j * (MAP_GROUP_SIZE * GLOBAL_SCALE)), WATER_LEVEL);
                D3DXVECTOR3 p1(p0.x + float(MAP_GROUP_SIZE * GLOBAL_SCALE), p0.y + float(MAP_GROUP_SIZE * GLOBAL_SCALE), WATER_LEVEL);

                int i0 = NPOS-1;
                int i1 = 0;
                while (i1 < NPOS)
                {
                    if (PointLineCatch(cvr.pos[i0],cvr.pos[i1],*(D3DXVECTOR2 *)&p0))
                    {
                        goto checknext_w;
                    }
                    if (PointLineCatch(cvr.pos[i0],cvr.pos[i1],*(D3DXVECTOR2 *)&p1))
                    {
                        goto checknext_w;
                    }
                    if (PointLineCatch(cvr.pos[i0],cvr.pos[i1],D3DXVECTOR2(p0.x, p1.y)))
                    {
                        goto checknext_w;
                    }
                    if (PointLineCatch(cvr.pos[i0],cvr.pos[i1],D3DXVECTOR2(p1.x, p0.y)))
                    {
                        goto checknext_w;
                    }
                    goto invisible_w;
checknext_w:
                    i0 = i1++;

                }

                if (m_Camera.IsInFrustum(p0,p1))
                {
                    m_VisWater->AnyStruct<D3DXVECTOR2>(*(D3DXVECTOR2 *)&p0);
                }
invisible_w:;

            }
        }
    }

    //DM("TIME", "TIME_END");
}


