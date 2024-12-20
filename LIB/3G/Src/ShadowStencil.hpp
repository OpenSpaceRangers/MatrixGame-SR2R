#ifndef SHADOW_STENCIL_INCLUDE
#define SHADOW_STENCIL_INCLUDE

struct SVOShadowStencilVertex
{
    D3DXVECTOR3 v;

    static const DWORD FVF = D3DFVF_XYZ;
};


class CVectorObject;

class CVOShadowStencil : public CMain
{
    DWORD                                   m_DirtyDX;
    D3D_VB                                  m_VB;
    D3D_IB                                  m_IB;

    int                                     m_IBSize; // in bytes
    int                                     m_VBSize;

    static  CVOShadowStencil *m_First;
    static  CVOShadowStencil *m_Last;

    CVOShadowStencil *m_Prev;
    CVOShadowStencil *m_Next;

    CVectorObject *m_vo;

    int             m_FrameFor;

    struct  SSSFrameData
    {
        SVOShadowStencilVertex                 *m_preVerts;
        int                                     m_preVertsAllocated;
        int                                     m_preVertsSize;

        WORD                                   *m_preInds;
        int                                     m_preIndsAllocated;
        int                                     m_preIndsSize;

        SVONormal      m_light;
        float          m_len;
    };



    CHeap *m_Heap;


    // calculated for

    SSSFrameData   *m_Frames;
    int             m_FramesCnt;


	public:

        static void StaticInit(void)
        {
            m_First = NULL;
            m_Last = NULL;

        }
        static void BeforeRenderAll(void)
        {
            ASSERT_DX(g_D3DD->SetFVF( SVOShadowStencilVertex::FVF ));
        }

        static void MarkAllBuffersNoNeed(void);

		CVOShadowStencil(CHeap * heap);
		~CVOShadowStencil(void);

        bool        IsReady(void) const {return m_Frames != NULL;}
        void        DX_Prepare(void);
        void        DX_Free(void)
        {
            if (IS_VB(m_VB)) DESTROY_VB(m_VB);
            if (IS_VB(m_IB)) DESTROY_VB(m_IB);

            m_DirtyDX = true;
        }

        void Build(CVectorObject & obj, int frame, const D3DXVECTOR3 & vLight, float len, bool invert);

        void BeforeRender(void) { DX_Prepare(); }
		void Render(const D3DXMATRIX &objma);
};



#endif