#include "stdafx.h"
#include "CBitmap.hpp"
#include "..\..\FilePNG\Src\FilePNG.hpp"
#include <malloc.h>
#include <ddraw.h>

#ifndef BASE_MATH_DEFINED
__forceinline int TruncFloat( float x )
{
    return int(x);
}
#pragma warning (disable: 4035)
__forceinline  int Float2Int( float x )
{
    _asm 
    { 
        fld x
        push eax
        fistp dword ptr [esp]
        pop eax
    }
}
__forceinline  int Double2Int( double x )
{
    _asm 
    { 
        fld x
        push eax
        fistp dword ptr [esp]
        pop eax
    }
}
#pragma warning (default: 4035)

__forceinline DWORD LIC(DWORD c0, DWORD c1, float t)
{
    DWORD c = 0;

    c |= 0xFF & Float2Int((0xFF & c0) + (int(0xFF & c1) - int(0xFF & c0)) * t);

    c0 >>= 8; c1 >>= 8;
    c |= (0xFF & Float2Int((0xFF & c0) + (int(0xFF & c1) - int(0xFF & c0)) * t)) << 8;

    c0 >>= 8; c1 >>= 8;
    c |= (0xFF & Float2Int((0xFF & c0) + (int(0xFF & c1) - int(0xFF & c0)) * t)) << 16;

    c0 >>= 8; c1 >>= 8;
    c |= (0xFF & Float2Int((0xFF & c0) + (int(0xFF & c1) - int(0xFF & c0)) * t)) << 24;
    return c;
}
#endif BASE_MATH_DEFINED

using namespace Base;


CBitmap::CBitmap(Base::CHeap * heap) : CMain()
{
	m_Heap=heap;

	m_Pos.x=0; m_Pos.y=0;
	m_Size.x=0; m_Size.y=0;

	m_Format=BMF_USER;
	m_BytePP=0;
	m_BitPP=0;
	m_MColor[0]=0; m_MColor[1]=0; m_MColor[2]=0; m_MColor[3]=0;

	m_Data=NULL;
	m_Pitch=0;
	m_DataExt=false;

	m_AddData[0]=0; m_AddData[1]=0; m_AddData[2]=0; m_AddData[3]=0;
	m_AddDataExt[0]=false; m_AddDataExt[1]=false; m_AddDataExt[2]=false; m_AddDataExt[3]=false;
	m_AddDataVal[0]=0; m_AddDataVal[1]=0; m_AddDataVal[2]=0; m_AddDataVal[3]=0;

	m_WindowBitmap=0;
	m_WindowDC=0;

}

CBitmap::~CBitmap()
{
	Clear();
	WBM_Clear();
}

void CBitmap::Clear()
{
	m_Pos.x=0; m_Pos.y=0;
	m_Size.x=0; m_Size.y=0;

	m_Format=BMF_USER;
	m_BytePP=0;
	m_BitPP=0;
	m_MColor[0]=0; m_MColor[1]=0; m_MColor[2]=0; m_MColor[3]=0;

	if(m_Data!=NULL && !m_DataExt) HFree(m_Data,m_Heap);
	m_Data=NULL;
	m_Pitch=0;
	m_DataExt=false;

	for(int i=0;i<4;i++) {
		if(m_AddData[i]!=NULL && !m_AddDataExt[i]) HFree(m_AddData[i],m_Heap);
		m_AddData[i]=NULL;
		m_AddDataExt[i]=false;
		m_AddDataVal[i]=0;
	}
}

void CBitmap::CreatePalate(int lenx,int leny,int palcnt)
{
	Clear();

	m_Size.x=lenx;
	m_Size.y=leny;

	m_Format=BMF_PALATE;
	m_BytePP=1;
	m_BitPP=8;
	m_MColor[0]=0x0ff;
	m_Pitch=lenx;
	m_Data=HAlloc(m_Pitch*m_Size.y,m_Heap);
	m_AddData[0]=HAlloc(palcnt*4,m_Heap);
	m_AddDataVal[0]=palcnt;
}

void CBitmap::CreateGrayscale(int lenx,int leny)
{
	Clear();

	m_Size.x=lenx;
	m_Size.y=leny;

	m_Format=BMF_FLAT;
	m_BytePP=1;
	m_BitPP=8;
	m_MColor[0]=0x0ff;
	m_Pitch=lenx;
	m_Data=HAlloc(m_Pitch*m_Size.y,m_Heap);
}

void CBitmap::Recrate(int lenx,int leny,int pitch)
{
    m_Size.x=lenx;
    m_Size.y=leny;
    m_Pitch=pitch;
    m_Data=HAllocEx(m_Data,m_Pitch*m_Size.y,m_Heap);
}

void CBitmap::AllocData()
{
	m_Data=HAllocEx(m_Data,m_Pitch*m_Size.y,m_Heap);
}

void CBitmap::BitmapDuplicate(CBitmap & des)
{
	des.Clear();

	des.m_Pos=m_Pos;
	des.m_Size=m_Size;
	des.m_Format=m_Format;
	des.m_BytePP=m_BytePP;
	des.m_BitPP=m_BitPP;
	des.m_MColor[0]=m_MColor[0]; des.m_MColor[1]=m_MColor[1]; des.m_MColor[2]=m_MColor[2]; des.m_MColor[3]=m_MColor[3];

	des.m_Pitch=m_Pitch;

	des.m_UserData=m_UserData;

	if(des.m_Size.x>0 && des.m_Size.y>0 && des.m_Pitch>0 && m_Data) {
		des.m_Data=HAlloc(des.m_Pitch*des.m_Size.y,des.m_Heap);
		CopyMemory(des.m_Data,m_Data,des.m_Pitch*des.m_Size.y);
	}
}

void BuildByMask(DWORD m,DWORD * s,DWORD * cb,DWORD * c)
{
	*s=0; // ��� �� ������
	*cb=0;// ��� � �����
	*c=0; // ���-�� ������
	if(m) {
		for (; !(m & 1); (*s)++, m >>= 1);
		for (; m & 1; (*cb)++, m >>= 1);
		*c=1<<(*cb);
	}
}

void CBitmap::Convert32To16(void)
{
    ASSERT(m_BitPP == 32);

    DWORD *src = (DWORD *)m_Data;
    WORD  *dst = (WORD *)m_Data;
    int cnt = m_Pitch * m_Size.y / m_BytePP;

    while (cnt-- > 0)
    {

        DWORD s = *(src);
        *dst = WORD( ((s >> 8) & 0xF800) |
                     ((s >> 5) & 0x07E0) |
                     ((s >> 3) & 0x001F) );

        ++src;
        ++dst;
    }
    m_BitPP = 16;
    m_BytePP = 2;
    m_Pitch >>= 1;

}

bool CBitmap::ConvertFrom(CBitmap & bm)
{
	if(m_Size.x<=0 || m_Size.y<=0) return false;
	if(m_Size.x!=bm.m_Size.x || m_Size.y!=bm.m_Size.y) return false;

	if(m_BytePP>=1 && m_BytePP<=4 && bm.m_BytePP>=1 && bm.m_BytePP<=4 && m_Format==BMF_FLAT && bm.m_Format==BMF_FLAT) {
		DWORD sR,cbR,cR;
		DWORD sG,cbG,cG;
		DWORD sB,cbB,cB;
		DWORD sA,cbA,cA;
		DWORD _sR,_cbR,_cR;
		DWORD _sG,_cbG,_cG;
		DWORD _sB,_cbB,_cB;
		DWORD _sA,_cbA,_cA;
		DWORD color,r,g,b,a;

		BuildByMask(bm.m_MColor[0],&sR,&cbR,&cR); int ssR=8-cbR;
		BuildByMask(bm.m_MColor[1],&sG,&cbG,&cG); int ssG=8-cbG;
		BuildByMask(bm.m_MColor[2],&sB,&cbB,&cB); int ssB=8-cbB;
		BuildByMask(bm.m_MColor[3],&sA,&cbA,&cA); int ssA=8-cbA;

		BuildByMask(m_MColor[0],&_sR,&_cbR,&_cR); int _ssR=8-_cbR;
		BuildByMask(m_MColor[1],&_sG,&_cbG,&_cG); int _ssG=8-_cbG;
		BuildByMask(m_MColor[2],&_sB,&_cbB,&_cB); int _ssB=8-_cbB;
		BuildByMask(m_MColor[3],&_sA,&_cbA,&_cA); int _ssA=8-_cbA;

		byte * sou=(byte *)bm.Data();
		byte * des=(byte *)Data();
		for(int y=0;y<m_Size.y;y++,sou+=bm.m_Pitch-bm.m_Size.x*bm.m_BytePP,des+=m_Pitch-m_Size.x*m_BytePP) {
			for(int x=0;x<m_Size.x;x++,sou+=bm.m_BytePP,des+=m_BytePP) {
				switch(bm.m_BytePP) {
					case 1: color=*(BYTE *)sou; break;
					case 2: color=*(WORD *)sou; break;
					case 3: color=*(WORD *)sou+((*(BYTE *)(sou+2)) << 16); break;
					case 4: color=*(DWORD *)sou; break;
				}

				r=BYTE((color>>sR)<<ssR);
				g=BYTE((color>>sG)<<ssG);
				b=BYTE((color>>sB)<<ssB);
				a=BYTE((color>>sA)<<ssA);
				color=DWORD(((r>>(_ssR))<<_sR)|((g>>(_ssG))<<_sG)|((b>>(_ssB))<<_sB)|((a>>(_ssA))<<_sA));

				switch(m_BytePP) {
					case 1: *(BYTE *)des=(BYTE)color; break;
					case 2: *(WORD *)des=(WORD)color; break;
					case 3: *(WORD *)des=WORD(color); *(BYTE *)(des+2)=BYTE(color >> 16);  break;
					case 4: *(DWORD *)des=(DWORD)color; break;
				}
			}
		}

		return true;
	}

	return false;
}


DWORD CBitmap::ARGB(float x, float y) // get interpolated ARGB for specified coordinate (0.0 - 1.0)
                                // 0.0,0.0 - center of left-top pixel
                                // 1.0,1.0 - center of right-bottom pixel
{
    float xf = x * (m_Size.x - 1) + 0.5f;
    int x0 = TruncFloat(xf);
    int x1 = x0 +1;
    float yf = y * (m_Size.y - 1) + 0.5f;
    int y0 = TruncFloat(yf);
    int y1 = y0 +1;

    DWORD cl = LIC( ARGBPixel(x0,y0),ARGBPixel(x0,y1), yf-y0 );
    DWORD cr = LIC( ARGBPixel(x1,y0),ARGBPixel(x1,y1), yf-y0 );
    return LIC(cl, cr, xf - x0);
}

#pragma warning (disable : 4731)
void CBitmap::Make2xSmaller(void)
{
    DTRACE();
    //void *d = HAlloc(m_Pitch*m_Size.y / 4,m_Heap);

	//byte * des=(byte *)d;
    byte * des=(byte *)m_Data;
	byte * sou=(byte *)m_Data;

    //int add = m_BytePP;
    //int addl = m_Pitch/2;

    m_Size.x /=2;
    m_Size.y /=2;

    if(m_BytePP==1)
    {
        int y = m_Size.y;
        do
        {
            int x = m_Size.x;
            do
            {
                int c = *sou;
                c += *(sou + 1);
                c += *(sou + m_Pitch);
                c += *(sou + m_Pitch + 1);
                sou += 2;
                *des++ = (BYTE)(c >> 2);
            }
		    while (--x > 0);
            sou += m_Pitch;
        }
        while (--y > 0);

    } else
    if(m_BytePP==2)
    {
        ERROR_E;
    } else
    if(m_BytePP==3)
    {
        int y = m_Size.y;
        do
        {
            int x = m_Size.x;
            do
            {
                int b0 = *(sou + 0);
                int b1 = *(sou + 1);
                int b2 = *(sou + 2);

                b0 += *(sou + 3);
                b1 += *(sou + 4);
                b2 += *(sou + 5);

                b0 += *(sou + 0 + m_Pitch);
                b1 += *(sou + 1 + m_Pitch);
                b2 += *(sou + 2 + m_Pitch);

                b0 += *(sou + 3 + m_Pitch);
                b1 += *(sou + 4 + m_Pitch);
                b2 += *(sou + 5 + m_Pitch);

                *(des + 0) = (byte)(b0 >> 2);
                *(des + 1) = (byte)(b1 >> 2);
                *(des + 2) = (byte)(b2 >> 2);
                sou += 6;
                des += 3;
            }
		    while (--x > 0);
            sou += m_Pitch;
        }
        while (--y > 0);
    } else
    if(m_BytePP==4)
    {

        _asm
        {

            mov esi, sou
            mov edi, des
            mov ebx, this

            mov eax, [ebx + m_Size + 4]

            mov ebx, [ebx + m_Pitch]

loopy:
            push eax
            push ebp

            mov eax, this
            mov eax, [eax + m_Size]
loopx:
            push eax

            xor ebp,ebp
            xor ecx,ecx

            mov ebp,[esi]
            mov ecx,ebp
            and ebp,0x00FF00FF
            and ecx,0xFF00FF00
            
            mov eax,[esi+4]
            mov edx,eax
            and eax,0x00FF00FF
            and edx,0xFF00FF00
            add ebp,eax
            add ecx,edx

            mov eax,[esi+ebx]
            adc ecx,0

            mov edx,eax
            and eax,0x00FF00FF
            and edx,0xFF00FF00
            add ebp,eax
            add ecx,edx

            mov eax,[esi+ebx+4]
            adc ecx,0
            mov edx,eax
            and eax,0x00FF00FF
            and edx,0xFF00FF00
            add eax, ebp
            add edx, ecx
            adc edx,0

            ror eax,2
            ror edx,2
            and eax,0x00FF00FF
            and edx,0xFF00FF00
            or  eax,edx

            mov [edi],eax

            add esi, 8
            add edi, 4

            pop eax
            dec eax
            jnz loopx

            pop ebp

            add esi, ebx

            pop eax
            dec eax
            jnz loopy

        };

    }

    m_Pitch /= 2;
    m_Data = HReAlloc(m_Data, m_Pitch * m_Size.y ,m_Heap);
    //HFree(m_Data,m_Heap);

    //m_Data = d;

}

void CBitmap::Make2xSmaller(const Base::CPoint & lu, const Base::CPoint & size, CBitmap & des_bitmap) const 
{
    DTRACE();

	if(m_BytePP==2) ERROR_E;

	int newx=size.x/2;
	int newy=size.y/2;

	if(des_bitmap.SizeX()!=newx || des_bitmap.SizeY()!=newy || des_bitmap.m_BytePP!=m_BytePP) {
		if(m_BytePP==1) des_bitmap.CreateGrayscale(newx,newy);
        else if(m_BytePP==3) des_bitmap.CreateRGB(newx,newy);
		else if(m_BytePP==4) des_bitmap.CreateRGBA(newx,newy);
	}

	byte * des=(byte *)des_bitmap.m_Data;
	byte * sou=(byte *)m_Data + lu.x * m_BytePP + m_Pitch*lu.y;


	int desnl=des_bitmap.m_Pitch-newx*m_BytePP;
	int sounl=m_Pitch-size.x*m_BytePP;

    if(m_BytePP==1)
    {
        for(int y=lu.y;y<(lu.y+size.y);y+=2)
        {
		    for(int x=lu.x;x<(lu.x+size.x);x+=2,++des)
            {
                int b0 = *(sou + x + 0);

                b0 += *(sou + x + 1);

                b0 += *(sou + x + 0 + m_Pitch);

                b0 += *(sou + x + 1 + m_Pitch);

                *(des + 0) = (byte)(b0 / 4);
            }			

            sou += m_Pitch * 2;
	    }
    } else if(m_BytePP==3) {
        for(int y=0;y<newy;y++,sou+=m_Pitch+sounl,des+=desnl) {
		    for(int x=0;x<newx;x++,des+=3,sou+=3+3) {

                int b0 = *(sou + 0);
                int b1 = *(sou + 1);
                int b2 = *(sou + 2);

                b0 += *(sou + 3);
                b1 += *(sou + 4);
                b2 += *(sou + 5);

                b0 += *(sou + 0 + m_Pitch);
                b1 += *(sou + 1 + m_Pitch);
                b2 += *(sou + 2 + m_Pitch);

                b0 += *(sou + 3 + m_Pitch);
                b1 += *(sou + 4 + m_Pitch);
                b2 += *(sou + 5 + m_Pitch);

                *(des + 0) = (byte)(b0 >> 2);
                *(des + 1) = (byte)(b1 >> 2);
                *(des + 2) = (byte)(b2 >> 2);
            }			
	    }
    } else if(m_BytePP==4)
    {
        _asm
        {

            mov esi, sou
            mov edi, des
            mov ebx, this
            mov ebx, [ebx + m_Pitch]

            mov eax, newy
loopy:
            push eax
            push ebp

            mov eax, newx
loopx:
            push eax

            xor ebp,ebp
            xor ecx,ecx

            mov ebp,[esi]
            mov ecx,ebp
            and ebp,0x00FF00FF
            and ecx,0xFF00FF00
            
            mov eax,[esi+4]
            mov edx,eax
            and eax,0x00FF00FF
            and edx,0xFF00FF00
            add ebp,eax
            add ecx,edx

            mov eax,[esi+ebx]
            adc ecx,0
            mov edx,eax
            and eax,0x00FF00FF
            and edx,0xFF00FF00
            add ebp,eax
            add ecx,edx

            mov eax,[esi+ebx+4]
            adc ecx,0
            mov edx,eax
            and eax,0x00FF00FF
            and edx,0xFF00FF00
            add eax, ebp
            add edx, ecx
            adc edx,0

            ror eax,2
            ror edx,2
            and eax,0x00FF00FF
            and edx,0xFF00FF00
            or  eax,edx

            mov [edi],eax

            add esi, 8
            add edi, 4

            pop eax
            dec eax
            jnz loopx

            pop ebp

            mov eax, sounl
            add esi, ebx
            add esi, eax
            add edi, desnl

            pop eax
            dec eax
            jnz loopy

        };

    }
}
#pragma warning (default : 4731)

void CBitmap::MakeLarger(int factor)
{
    void *d = HAlloc(m_Pitch*m_Size.y*factor*factor,m_Heap);

	byte * des=(byte *)d;
	byte * sou=(byte *)m_Data;

    int add = m_BytePP*factor;
    int addl = m_Pitch*factor;

    if(m_BytePP==1)
    {
        for(int y=0;y<m_Size.y;++y)
        {
		    for(int x=0;x<m_Size.x;++x,des+=add)
            {
                memset(des,*((byte *)sou + x), factor);
		    }

            for (int k = 0; k<(factor-1); ++k)
            {
                memcpy(des,des - addl, addl);
                des += addl;
            }

            sou += m_Pitch;
	    }
    } else
    if(m_BytePP==2)
    {
        for(int y=0;y<m_Size.y;++y)
        {
		    for(int x=0;x<m_Size.x;++x,des+=add)
            {
                word w = *(word *)(sou + x * 2);
                for (int k = 0; k<factor; ++k)
                {
                    *(((word *)des) + k) = w;
                }
		    }			
            for (int k = 0; k<(factor-1); ++k)
            {
                memcpy(des,des - addl, addl);
                des += addl;
            }

            sou += m_Pitch;
	    }
    } else
    if(m_BytePP==3)
    {
        for(int y=0;y<m_Size.y;++y)
        {
		    for(int x=0;x<m_Size.x;++x,des+=add)
            {
                word w = *(word *)(sou + x * 3);
                byte b = *(byte *)(sou + x * 3 + 2);
                for (int k = 0; k<factor; ++k)
                {
                    *((word *)(des + k*3)) = w;
                    *((byte *)(des + k*3 + 2)) = b;
                }
		    }			
            for (int k = 0; k<(factor-1); ++k)
            {
                memcpy(des,des - addl, addl);
                des += addl;
            }

            sou += m_Pitch;
	    }
    } else
    if(m_BytePP==4)
    {
        for(int y=0;y<m_Size.y;++y)
        {
		    for(int x=0;x<m_Size.x;++x,des+=add)
            {
                dword dw = *(dword *)(sou + x * 4);
                for (int k = 0; k<factor; ++k)
                {
                    *(((dword *)des) + k) = dw;
                }
            }
            for (int k = 0; k<(factor-1); ++k)
            {
                memcpy(des,des - addl, addl);
                des += addl;
            }

            sou += m_Pitch;
        }
    }

    
    HFree(m_Data,m_Heap);

    m_Data = d;
    m_Pitch *= factor;
    m_Size.x *=factor;
    m_Size.y *=factor;

}

void CBitmap::MakeLarger(int factor, CBitmap &out) const
{
	int newx=m_Size.x*factor;
	int newy=m_Size.y*factor;

    if(out.SizeX()!=newx || out.SizeY()!=newy || out.m_BytePP!=m_BytePP)
    {
        if(m_BytePP==1) out.CreateGrayscale(newx,newy);
        else if(m_BytePP==2) out.Create16(newx,newy);
		else if(m_BytePP==3) out.CreateRGB(newx,newy);
		else if(m_BytePP==4) out.CreateRGBA(newx,newy);
	}

	byte * des=(byte *)out.m_Data;;
	byte * sou=(byte *)m_Data;

    int add = m_BytePP*factor;
    int addl = m_Pitch*factor;

    if(m_BytePP==1)
    {
        for(int y=0;y<m_Size.y;++y)
        {
		    for(int x=0;x<m_Size.x;++x,des+=add)
            {
                memset(des,*((byte *)sou + x), factor);
		    }

            for (int k = 0; k<(factor-1); ++k)
            {
                memcpy(des,des - addl, addl);
                des += addl;
            }

            sou += m_Pitch;
	    }
    } else
    if(m_BytePP==2)
    {
        for(int y=0;y<m_Size.y;++y)
        {
		    for(int x=0;x<m_Size.x;++x,des+=add)
            {
                word w = *(word *)(sou + x * 2);
                for (int k = 0; k<factor; ++k)
                {
                    *(((word *)des) + k) = w;
                }
		    }			
            for (int k = 0; k<(factor-1); ++k)
            {
                memcpy(des,des - addl, addl);
                des += addl;
            }

            sou += m_Pitch;
	    }
    } else
    if(m_BytePP==3)
    {
        for(int y=0;y<m_Size.y;++y)
        {
		    for(int x=0;x<m_Size.x;++x,des+=add)
            {
                word w = *(word *)(sou + x * 3);
                byte b = *(byte *)(sou + x * 3 + 2);
                for (int k = 0; k<factor; ++k)
                {
                    *((word *)(des + k*3)) = w;
                    *((byte *)(des + k*3 + 2)) = b;
                }
		    }			
            for (int k = 0; k<(factor-1); ++k)
            {
                memcpy(des,des - addl, addl);
                des += addl;
            }

            sou += m_Pitch;
	    }
    } else
    if(m_BytePP==4)
    {
        for(int y=0;y<m_Size.y;++y)
        {
		    for(int x=0;x<m_Size.x;++x,des+=add)
            {
                dword dw = *(dword *)(sou + x * 4);
                for (int k = 0; k<factor; ++k)
                {
                    *(((dword *)des) + k) = dw;
                }
            }
            for (int k = 0; k<(factor-1); ++k)
            {
                memcpy(des,des - addl, addl);
                des += addl;
            }

            sou += m_Pitch;
        }
    }
}




DWORD CBitmap::Hash(void)
{
    return CalcCRC32(m_Data, m_Pitch*m_Size.y);
}


EAreaType CBitmap::GetAreaType(const Base::CPoint & p,const Base::CPoint & size)
{
    if (m_BytePP!=4) return AREA_SOLID;
	DWORD * ptr=(DWORD *)((byte *)m_Data+4*p.x+m_Pitch*p.y);

    bool empty_found = false;
    bool solid_found = false;

    for (int j = 0; j<size.y; ++j)
    {
        for (int i = 0; i<size.x; ++i)
        {
            DWORD ap = (m_MColor[3] & (*(ptr + i)));
            bool t_empty_found = (ap == 0x00000000);
            bool t_solid_found = (ap == 0xFF000000);

            if ( !t_empty_found && !t_solid_found  ) return AREA_SEMITRANSPARENT;

            empty_found |= t_empty_found;
            solid_found |= t_solid_found;

            if (empty_found && solid_found) return AREA_SEMITRANSPARENT;
        }
        ptr = (DWORD *)(((byte *)ptr) + m_Pitch);
    }
    return empty_found?AREA_TRANSPARENT:AREA_SOLID;
}

void CBitmap::Tile(const Base::CPoint & pdes,const Base::CPoint & desize, const CBitmap & bmsou,const Base::CPoint & spsou, const Base::CPoint & szsou)
{

    Copy(pdes, szsou, bmsou, spsou);

    // filling by x

    int cx = szsou.x;
    while (cx < desize.x)
    {
        int w = min((desize.x-cx), cx);
        Copy(CPoint(pdes.x + cx, pdes.y),CPoint(w,szsou.y), *this, pdes);
        cx += w;
    }

    // filling by y

    int cy = szsou.y;
    while (cy < desize.y)
    {
        int h = min((desize.y-cy), cy);
        Copy(CPoint(pdes.x, pdes.y + cy),CPoint(desize.x,h), *this, pdes);
        cy += h;
    }
}


void CBitmap::Copy(const Base::CPoint & pdes,const Base::CPoint & size,const CBitmap & bmsou,const Base::CPoint & spsou)
{
	if(spsou.x<0 || spsou.y<0) return;
	if((spsou.x+size.x)>bmsou.m_Size.x || (spsou.y+size.y)>bmsou.m_Size.y) return;

	if(pdes.x<0 || pdes.y<0) return;
	if((pdes.x+size.x)>m_Size.x || (pdes.y+size.y)>m_Size.y) return;

	byte * des=(byte *)m_Data+m_BytePP*pdes.x+m_Pitch*pdes.y;
	byte * sou=(byte *)bmsou.m_Data+bmsou.m_BytePP*spsou.x+bmsou.m_Pitch*spsou.y;

	if(bmsou.m_Format!=m_Format) return;
	//if(bmsou.m_BytePP!=m_BytePP) return;
    //if(bmsou.m_BitPP!=m_BitPP) return;


    if (bmsou.m_BytePP == m_BytePP)
    {
        int len = size.x * m_BytePP;
	    for(int y=0;y<size.y;y++,des+=m_Pitch,sou+=bmsou.m_Pitch)
        {
            memcpy(des, sou, len);
	    }
    } else
        // fix this brunch for non 3 or 4 bytePP images
    {
        DWORD alpha = bmsou.m_BytePP==3?0xFF000000:0;
	    for(int y=0;y<size.y;y++,des+=m_Pitch,sou+=bmsou.m_Pitch)
        {
            BYTE *des1 = des;
            BYTE *sou1 = sou;
            for (int i = 0; i<(size.x-1); ++i, sou1 += bmsou.m_BytePP, des1 += m_BytePP)
            {
                DWORD color = *((DWORD *)sou1) | alpha;
                *((DWORD *)des1) = color;
            }
            int c = min(bmsou.m_BytePP, m_BytePP);
            while (c--)
            {
                *des1++ = *sou1++;
            }
            if (alpha) *des1 = 0xFF;
        }
    }
}

void CBitmap::Fill(const Base::CPoint & pdes,const Base::CPoint & size, dword color)
{
	if(pdes.x<0 || pdes.y<0) return;
	if((pdes.x+size.x)>m_Size.x || (pdes.y+size.y)>m_Size.y) return;

	byte * des=(byte *)m_Data+m_BytePP*pdes.x+m_Pitch*pdes.y;
	int desnl=m_Pitch-size.x*m_BytePP;
	int desnp=m_BytePP;

	if(m_BytePP==1) {
		for(int y=0;y<size.y;y++,des+=desnl) {
			for(int x=0;x<size.x;x++,des+=desnp) {
				*(byte *)des=(byte)color;
			}			
		}
	} else if(m_BytePP==2) {
		for(int y=0;y<size.y;y++,des+=desnl) {
			for(int x=0;x<size.x;x++,des+=desnp) {
				*(word *)des=(word)color;
			}			
		}
	} else if(m_BytePP==3) {
		for(int y=0;y<size.y;y++,des+=desnl) {
			for(int x=0;x<size.x;x++,des+=desnp) {
				*(word *)des=(word)color;
				*(byte *)(des+2)=(byte)(color>>16);
			}			
		}
	} else if(m_BytePP==4) {
		for(int y=0;y<size.y;y++,des+=desnl) {
			for(int x=0;x<size.x;x++,des+=desnp) {
				*(dword *)des=color;
			}			
		}
	}
}

void CBitmap::Rotate90(const Base::CPoint & pdes,const Base::CPoint & sizesou,const CBitmap & bmsou,const Base::CPoint & spsou)
{
	if(spsou.x<0 || spsou.y<0) return;
	if((spsou.x+sizesou.x)>bmsou.m_Size.x || (spsou.y+sizesou.y)>bmsou.m_Size.y) return;

	if(pdes.x<0 || pdes.y<0) return;
	if((pdes.x+sizesou.y)>m_Size.x || (pdes.y+sizesou.x)>m_Size.y) return;

	byte * des=(byte *)m_Data+m_BytePP*pdes.x+m_Pitch*pdes.y;
	int desnl=m_Pitch-sizesou.x*m_BytePP;
	int desnp=m_BytePP;

	byte * sou=(byte *)bmsou.m_Data+bmsou.m_BytePP*spsou.x+bmsou.m_Pitch*spsou.y;
	//int sounl=bmsou.m_Pitch-sizesou.x*bmsou.m_BytePP;
	int soull=bmsou.m_Pitch;
	int sounp=bmsou.m_BytePP;

	sou=sou+(sizesou.x-1)*sounp;

	if(bmsou.m_Format!=m_Format) return;
	if(bmsou.m_BytePP!=m_BytePP) return;
	if(bmsou.m_BitPP!=m_BitPP) return;

	for(int y=0;y<sizesou.x;y++,des+=desnl,sou=sou-soull*sizesou.y-sounp) {
		for(int x=0;x<sizesou.y;x++,des+=desnp,sou+=soull) {
			switch(m_BytePP) {
				case 1: *(byte *)des=*(byte *)sou; break;
				case 2: *(word *)des=*(word *)sou; break;
				case 3: *(word *)des=*(word *)sou; *(byte *)(des+2)=*(byte *)(sou+2); break;
				case 4: *(dword *)des=*(dword *)sou;break;
			}
		}			
	}
}

void CBitmap::Rotate180(const Base::CPoint & pdes,const Base::CPoint & sizesou, const CBitmap & bmsou,const Base::CPoint & spsou)
{
	if(spsou.x<0 || spsou.y<0) return;
	if((spsou.x+sizesou.x)>bmsou.m_Size.x || (spsou.y+sizesou.y)>bmsou.m_Size.y) return;

	if(pdes.x<0 || pdes.y<0) return;
	if((pdes.x+sizesou.x)>m_Size.x || (pdes.y+sizesou.y)>m_Size.y) return;

	byte * des=(byte *)m_Data+m_BytePP*pdes.x+m_Pitch*pdes.y;
	int desnl=m_Pitch-sizesou.x*m_BytePP;
	int desnp=m_BytePP;

	byte * sou=(byte *)bmsou.m_Data+bmsou.m_BytePP*spsou.x+bmsou.m_Pitch*spsou.y;
	int sounl=bmsou.m_Pitch-sizesou.x*bmsou.m_BytePP;
	int soull=bmsou.m_Pitch;
	int sounp=bmsou.m_BytePP;

	sou=sou+(sizesou.x-1)*sounp+(sizesou.y-1)*soull;

	if(bmsou.m_Format!=m_Format) return;
	if(bmsou.m_BytePP!=m_BytePP) return;
	if(bmsou.m_BitPP!=m_BitPP) return;

	for(int y=0;y<sizesou.y;y++,des+=desnl,sou=sou-sounl) {
		for(int x=0;x<sizesou.x;x++,des+=desnp,sou-=sounp) {
			switch(m_BytePP) {
				case 1: *(byte *)des=*(byte *)sou; break;
				case 2: *(word *)des=*(word *)sou; break;
				case 3: *(word *)des=*(word *)sou; *(byte *)(des+2)=*(byte *)(sou+2); break;
				case 4: *(dword *)des=*(dword *)sou;break;
			}
		}			
	}
}

void CBitmap::Rotate270(const Base::CPoint & pdes,const Base::CPoint & sizesou, const CBitmap & bmsou,const Base::CPoint & spsou)
{
	if(spsou.x<0 || spsou.y<0) return;
	if((spsou.x+sizesou.x)>bmsou.m_Size.x || (spsou.y+sizesou.y)>bmsou.m_Size.y) return;

	if(pdes.x<0 || pdes.y<0) return;
	if((pdes.x+sizesou.y)>m_Size.x || (pdes.y+sizesou.x)>m_Size.y) return;

	byte * des=(byte *)m_Data+m_BytePP*pdes.x+m_Pitch*pdes.y;
	int desnl=m_Pitch-sizesou.x*m_BytePP;
	int desnp=m_BytePP;

	byte * sou=(byte *)bmsou.m_Data+bmsou.m_BytePP*spsou.x+bmsou.m_Pitch*spsou.y;
	//int sounl=bmsou.m_Pitch-sizesou.x*bmsou.m_BytePP;
	int soull=bmsou.m_Pitch;
	int sounp=bmsou.m_BytePP;

	sou=sou+(sizesou.y-1)*soull;

	if(bmsou.m_Format!=m_Format) return;
	if(bmsou.m_BytePP!=m_BytePP) return;
	if(bmsou.m_BitPP!=m_BitPP) return;

	for(int y=0;y<sizesou.x;y++,des+=desnl,sou=sou+soull*sizesou.y+sounp) {
		for(int x=0;x<sizesou.y;x++,des+=desnp,sou-=soull) {
			switch(m_BytePP) {
				case 1: *(byte *)des=*(byte *)sou; break;
				case 2: *(word *)des=*(word *)sou; break;
				case 3: *(word *)des=*(word *)sou; *(byte *)(des+2)=*(byte *)(sou+2); break;
				case 4: *(dword *)des=*(dword *)sou;break;
			}
		}			
	}
}

void CBitmap::FlipX(const Base::CPoint & pdes,const Base::CPoint & size, const CBitmap & bmsou,const Base::CPoint & spsou)
{
	if(spsou.x<0 || spsou.y<0) return;
	if((spsou.x+size.x)>bmsou.m_Size.x || (spsou.y+size.y)>bmsou.m_Size.y) return;

	if(pdes.x<0 || pdes.y<0) return;
	if((pdes.x+size.x)>m_Size.x || (pdes.y+size.y)>m_Size.y) return;

	byte * des=(byte *)m_Data+m_BytePP*pdes.x+m_Pitch*pdes.y;
	int desnl=m_Pitch-size.x*m_BytePP;
	int desnp=m_BytePP;

	byte * sou=(byte *)bmsou.m_Data+bmsou.m_BytePP*spsou.x+bmsou.m_Pitch*spsou.y;
//	int sounl=bmsou.m_Pitch-size.x*bmsou.m_BytePP;
	int soull=bmsou.m_Pitch;
	int sounp=bmsou.m_BytePP;

	sou=sou+(size.x-1)*sounp;

	if(bmsou.m_Format!=m_Format) return;
	if(bmsou.m_BytePP!=m_BytePP) return;
	if(bmsou.m_BitPP!=m_BitPP) return;

	for(int y=0;y<size.y;y++,des+=desnl,sou=sou+soull+(size.x)*sounp) {
		for(int x=0;x<size.x;x++,des+=desnp,sou-=sounp) {
			switch(m_BytePP) {
				case 1: *(byte *)des=*(byte *)sou; break;
				case 2: *(word *)des=*(word *)sou; break;
				case 3: *(word *)des=*(word *)sou; *(byte *)(des+2)=*(byte *)(sou+2); break;
				case 4: *(dword *)des=*(dword *)sou;break;
			}
		}			
	}
}

void CBitmap::FlipY(void)
{
    byte *l1 = (BYTE *)m_Data;
    byte *l2 = (BYTE *)m_Data + m_Pitch * (m_Size.y-1);
    
    while (l1<l2)
    {
        int cnt = m_Pitch;
        byte *p0 = l1;
        byte *p1 = l2;
        while (cnt >= 4)
        {
            DWORD temp = *(DWORD *)p0;
            *(DWORD *)p0 = *(DWORD *)p1;
            *(DWORD *)p1 = temp;
            p0 += 4;
            p1 += 4;
            cnt -= 4;
        }
        while (cnt > 0)
        {
            BYTE temp = *p0;
            *p0 = *p1;
            *p1 = temp;
            ++p0;
            ++p1;
            --cnt;
        }

        l1+=m_Pitch;
        l2-=m_Pitch;
    }

}

void CBitmap::FlipY(const Base::CPoint & pdes,const Base::CPoint & size, const CBitmap & bmsou,const Base::CPoint & spsou)
{
	if(spsou.x<0 || spsou.y<0) return;
	if((spsou.x+size.x)>bmsou.m_Size.x || (spsou.y+size.y)>bmsou.m_Size.y) return;

	if(pdes.x<0 || pdes.y<0) return;
	if((pdes.x+size.x)>m_Size.x || (pdes.y+size.y)>m_Size.y) return;

	byte * des=(byte *)m_Data+m_BytePP*pdes.x+m_Pitch*pdes.y;
	int desnl=m_Pitch-size.x*m_BytePP;
	int desnp=m_BytePP;

	byte * sou=(byte *)bmsou.m_Data+bmsou.m_BytePP*spsou.x+bmsou.m_Pitch*spsou.y;
	int sounl=bmsou.m_Pitch-size.x*bmsou.m_BytePP;
	int soull=bmsou.m_Pitch;
	int sounp=bmsou.m_BytePP;

	sou=sou+(size.y-1)*soull;

	if(bmsou.m_Format!=m_Format) return;
	if(bmsou.m_BytePP!=m_BytePP) return;
	if(bmsou.m_BitPP!=m_BitPP) return;

	for(int y=0;y<size.y;y++,des+=desnl,sou=sou+sounl-soull*2) {
		for(int x=0;x<size.x;x++,des+=desnp,sou+=sounp) {
			switch(m_BytePP) {
				case 1: *(byte *)des=*(byte *)sou; break;
				case 2: *(word *)des=*(word *)sou; break;
				case 3: *(word *)des=*(word *)sou; *(byte *)(des+2)=*(byte *)(sou+2); break;
				case 4: *(dword *)des=*(dword *)sou;break;
			}
		}			
	}
}

void CBitmap::FlipY(const Base::CPoint & pdes,const Base::CPoint & size)
{
	if(pdes.x<0 || pdes.y<0) return;
	if((pdes.x+size.x)>m_Size.x || (pdes.y+size.y)>m_Size.y) return;

	byte * des=(byte *)m_Data+m_BytePP*pdes.x+m_Pitch*pdes.y;
	int desnl=m_Pitch-size.x*m_BytePP;
	int desnp=m_BytePP;

	byte * sou=(byte *)m_Data+m_BytePP*pdes.x+m_Pitch*pdes.y;
	int sounl=m_Pitch-size.x*m_BytePP;
	int soull=m_Pitch;
	int sounp=m_BytePP;

	sou=sou+(size.y-1)*soull;

	DWORD t;

	for(int y=0;y<(size.y >> 1);y++,des+=desnl,sou=sou+sounl-soull*2) {
		for(int x=0;x<size.x;x++,des+=desnp,sou+=sounp) {
			switch(m_BytePP) {
				case 1: 
					t=*(byte *)des;
					*(byte *)des=*(byte *)sou; 
					*(byte *)sou=BYTE(t);
					break;
				case 2: 
					t=*(word *)des;
					*(word *)des=*(word *)sou; 
					*(word *)sou=WORD(t);
					break;
				case 3: 
					t=*(word *)des;
					*(word *)des=*(word *)sou; 
					*(word *)sou=WORD(t);

					t=*(byte *)(des+2);
					*(byte *)(des+2)=*(byte *)(sou+2); 
					*(byte *)(sou+2)=BYTE(t);
					break;
				case 4: 
					t=*(dword *)des;
					*(dword *)des=*(dword *)sou;
					*(dword *)sou=t;
					break;
			}
		}			
	}

}

bool CBitmap::Equals(const CBitmap & bm) const
{
    if (bm.SizeX() != SizeX()) return false;
    if (bm.SizeY() != SizeY()) return false;

	if (bm.m_Format!=m_Format) return false;
	if (bm.m_BytePP!=m_BytePP) return false;
	if (bm.m_BitPP!=m_BitPP) return false;

    int sz = m_Pitch * SizeY();
    if (memcmp(m_Data, bm.m_Data, sz)) return false;
    return true;
}

void CBitmap::MergeByMask(	const Base::CPoint & pdes,const Base::CPoint & size,
							const CBitmap & bm1,const Base::CPoint & sp1,
							const CBitmap & bm2,const Base::CPoint & sp2,
							const CBitmap & mask,const Base::CPoint & spm)
{
	if(sp1.x<0 || sp1.y<0) return;
	if((sp1.x+size.x)>bm1.m_Size.x || (sp1.y+size.y)>bm1.m_Size.y) return;

	if(sp2.x<0 || sp2.y<0) return;
	if((sp2.x+size.x)>bm2.m_Size.x || (sp2.y+size.y)>bm2.m_Size.y) return;

	if(pdes.x<0 || pdes.y<0) return;
	if((pdes.x+size.x)>m_Size.x || (pdes.y+size.y)>m_Size.y) return;

	if(bm1.m_Format!=bm2.m_Format) return;
	if(bm1.m_BytePP!=bm2.m_BytePP) return;
	if(bm1.m_BitPP!=bm2.m_BitPP) return;

	if(bm1.m_Format!=m_Format) return;
	if(bm1.m_BytePP!=m_BytePP) return;
	if(bm1.m_BitPP!=m_BitPP) return;

	if(bm1.m_Format!=BMF_FLAT) return;
	if(mask.m_BitPP<8) return;

	byte * des=(byte *)m_Data+m_BytePP*pdes.x+m_Pitch*pdes.y;
	int desnl=m_Pitch-size.x*m_BytePP;

	byte * sou1=(byte *)bm1.m_Data+bm1.m_BytePP*sp1.x+bm1.m_Pitch*sp1.y;
	int sou1nl=bm1.m_Pitch-size.x*bm1.m_BytePP;

	byte * sou2=(byte *)bm2.m_Data+bm2.m_BytePP*sp2.x+bm2.m_Pitch*sp2.y;
	int sou2nl=bm2.m_Pitch-size.x*bm2.m_BytePP;

	byte * m=(byte *)mask.m_Data+mask.m_BytePP*spm.x+mask.m_Pitch*spm.y;
	int mnl=mask.m_Pitch-size.x*mask.m_BytePP;
	int mnp=mask.m_BytePP;

	for(int y=0;y<size.y;y++,des+=desnl,sou1+=sou1nl,sou2+=sou2nl,m+=mnl) {
		for(int x=0;x<size.x;x++,m+=mnp) {
			for(int i=0;i<bm1.m_BytePP;i++,des++,sou1++,sou2++) {
				if(*m==0) *des=*sou2;
				else if(*m==255) *des=*sou1;
				else *des=   byte((DWORD(*sou1)*(DWORD(*m)<<8))>>16)
							+byte((DWORD(*sou2)*(DWORD(255-*m)<<8))>>16); // �� ������ ������ ������� =(

			}
		}
	}
}

void CBitmap::SwapByte(const Base::CPoint & pos,const Base::CPoint & size,int n1,int n2)
{
	if(n1==n2) return;
	if(n1<0 || n1>=m_BytePP) return;
	if(n2<0 || n2>=m_BytePP) return;
	if(m_BytePP<=1) return;

	byte * buf=(byte *)m_Data+m_BytePP*pos.x+m_Pitch*pos.y;
	int bufnl=m_Pitch-size.x*m_BytePP;
	int bufnp=m_BytePP;

	byte zn;

	for(int y=0;y<size.y;y++,buf+=bufnl) {
		for(int x=0;x<size.x;x++,buf+=bufnp) {
			zn=*(buf+n1); *(buf+n1)=*(buf+n2); *(buf+n2)=zn;
		}
	}
}

void CBitmap::MergeWithAlpha(const Base::CPoint & pdes,const Base::CPoint & size, const CBitmap & bmsou,const Base::CPoint & spsou)
{
	if(spsou.x<0 || spsou.y<0) return;
	if((spsou.x+size.x)>bmsou.m_Size.x || (spsou.y+size.y)>bmsou.m_Size.y) return;

	if(pdes.x<0 || pdes.y<0) return;
	if((pdes.x+size.x)>m_Size.x || (pdes.y+size.y)>m_Size.y) return;

	byte * des=(byte *)m_Data+m_BytePP*pdes.x+m_Pitch*pdes.y;
	int desnl=m_Pitch-size.x*m_BytePP;
	int desnp=m_BytePP;

	byte * sou=(byte *)bmsou.m_Data+bmsou.m_BytePP*spsou.x+bmsou.m_Pitch*spsou.y;
	int sounl=bmsou.m_Pitch-size.x*bmsou.m_BytePP;
	int sounp=bmsou.m_BytePP;

	if(bmsou.m_Format!=m_Format) return;
//	if(bmsou.m_BytePP!=m_BytePP) return;
//	if(bmsou.m_BitPP!=m_BitPP) return;
	if(m_BytePP!=3 && m_BytePP!=4) return;

	if(bmsou.m_Format!=BMF_FLAT) return;
	if(bmsou.m_BytePP!=4) return;

    if(m_BytePP==3) {

	    for(int y=0;y<size.y;y++,des+=desnl,sou+=sounl)
        {
		    for(int x=0;x<size.x;x++,des+=desnp,sou+=sounp)
            {
			    dword color=*(dword *)sou;
                dword ocolor = dword(*(word *)des) | (dword(*((byte *)des+2))<<16);

			    byte alpha = byte(color>>24);
                byte R = byte((color>>16) & 0xFF);
                byte G = byte((color>>8) & 0xFF);
                byte B = byte((color) & 0xFF);

                byte oR = byte((ocolor>>16) & 0xFF);
                byte oG = byte((ocolor>>8) & 0xFF);
                byte oB = byte((ocolor) & 0xFF);

                if (alpha==0) continue;
			    else
                if(alpha==255)
                {
                    ocolor = color;
                } else
                {
                    float A = float(alpha) / 255.0f;
                    float nA = 1.0f - A;
                    
                    
                    int oiB = Float2Int(float(B) * A + float(oB) * nA);
                    int oiG = Float2Int(float(G) * A + float(oG) * nA);
                    int oiR = Float2Int(float(R) * A + float(oR) * nA);

                    ocolor  = ((oiB>255)?255:oiB) | (((oiG>255)?255:oiG)<<8) | (((oiR>255)?255:oiR)<<16);

                }
                
                *des = byte(ocolor & 0xFF);
                *(des+1) = byte((ocolor>>8) & 0xFF);
                *(des+2) = byte((ocolor>>16) & 0xFF);
		    }			
	    }
    } else {

	    for(int y=0;y<size.y;y++,des+=desnl,sou+=sounl)
        {
		    for(int x=0;x<size.x;x++,des+=desnp,sou+=sounp)
            {
			    dword color=*(dword *)sou;
                dword ocolor = *(dword *)des;
			    byte alpha = byte(color>>24);
                byte R = byte((color>>16) & 0xFF);
                byte G = byte((color>>8) & 0xFF);
                byte B = byte((color) & 0xFF);

			    byte oalpha = byte(ocolor>>24);
                byte oR = byte((ocolor>>16) & 0xFF);
                byte oG = byte((ocolor>>8) & 0xFF);
                byte oB = byte((ocolor) & 0xFF);

                if (alpha==0) continue;
			    else
                if(alpha==255)
                {
                    ocolor = color;
                } else
                {


                    float A = float(alpha) / 255.0f;
                    float nA = 1.0f - A;
                    
                    
                    int oiB = Float2Int(float(B) * A + float(oB) * nA);
                    int oiG = Float2Int(float(G) * A + float(oG) * nA);
                    int oiR = Float2Int(float(R) * A + float(oR) * nA);

                    int oiA = Float2Int(oalpha + float(255-oalpha) * A);

                    ocolor  = ((oiB>255)?255:oiB) | (((oiG>255)?255:oiG)<<8) | (((oiR>255)?255:oiR)<<16) | (((oiA>255)?255:oiA)<<24);
                    
                    

                    /*
                    ocolor = byte ( float(B)*A + float(oB)*nA ) |
                            (byte ( float(G)*A + float(oG)*nA ) << 8) |
                            (byte ( float(R)*A + float(oR)*nA ) << 16) |
                            (( oalpha + byte(((255-oalpha)*(DWORD(alpha)<<8))>>16) ) << 24);
                    */
                }
                
                
                if (m_BytePP == 3)
                {
                    *des = byte(ocolor);
                    *(des+1) = byte(ocolor>>8);
                    *(des+2) = byte(ocolor>>16);
                } else
                {
                    *(dword *)des = ocolor;
                }
                
                

		    }			
	    }
    }
}

void CBitmap::MergeWithAlphaPM(const Base::CPoint & pdes,const Base::CPoint & size, const CBitmap & bmsou,const Base::CPoint & spsou)
{
    DTRACE();
    /*
	if(spsou.x<0 || spsou.y<0) return;
	if((spsou.x+size.x)>bmsou.m_Size.x || (spsou.y+size.y)>bmsou.m_Size.y) return;

	if(pdes.x<0 || pdes.y<0) return;
	if((pdes.x+size.x)>m_Size.x || (pdes.y+size.y)>m_Size.y) return;

	byte * des=(byte *)m_Data+m_BytePP*pdes.x+m_Pitch*pdes.y;
	int desnl=m_Pitch-size.x*m_BytePP;
	int desnp=m_BytePP;

	byte * sou=(byte *)bmsou.m_Data+bmsou.m_BytePP*spsou.x+bmsou.m_Pitch*spsou.y;
	int sounl=bmsou.m_Pitch-size.x*bmsou.m_BytePP;
	int sounp=bmsou.m_BytePP;

	if(bmsou.m_Format!=m_Format) return;
	if(bmsou.m_BytePP!=m_BytePP) return;
	if(bmsou.m_BitPP!=m_BitPP) return;

	if(m_Format!=BMF_FLAT) return;
	if(m_BytePP!=4) return;

	for(int y=0;y<size.y;y++,des+=desnl,sou+=sounl) {
		for(int x=0;x<size.x;x++,des+=desnp,sou+=sounp) {
			dword color=*(dword *)sou;
			byte alpha=byte(color>>24);
			if(alpha==0) continue;
			else if(alpha==255) { *(dword *)des=color; continue; }

			color=   dword(*sou)
						+byte((DWORD(*des)*(DWORD(255-alpha)<<8))>>16); // �� ������ ������ ������� =(
			if(color<255) *des=byte(color); else *des=255;
			sou++; des++;
			color=   dword(*sou)
						+byte((DWORD(*des)*(DWORD(255-alpha)<<8))>>16); // �� ������ ������ ������� =(
			if(color<255) *des=byte(color); else *des=255;
			sou++; des++;
			color=   dword(*sou)
						+byte((DWORD(*des)*(DWORD(255-alpha)<<8))>>16); // �� ������ ������ ������� =(
			if(color<255) *des=byte(color); else *des=255;
			sou++; des++;

			*des+=byte(((255-*des)*(DWORD(alpha)<<8))>>16); // �� ������ ������ ������� =(

//			if(dword(*des)+dword(alpha)>255) *des=255;
//			else *des+=alpha;

			sou-=3; des-=3;
		}			
	}
    */

	if(spsou.x<0 || spsou.y<0) return;
	if((spsou.x+size.x)>bmsou.m_Size.x || (spsou.y+size.y)>bmsou.m_Size.y) return;

	if(pdes.x<0 || pdes.y<0) return;
	if((pdes.x+size.x)>m_Size.x || (pdes.y+size.y)>m_Size.y) return;

	byte * des=(byte *)m_Data+m_BytePP*pdes.x+m_Pitch*pdes.y;
	int desnl=m_Pitch-size.x*m_BytePP;
	int desnp=m_BytePP;

	byte * sou=(byte *)bmsou.m_Data+bmsou.m_BytePP*spsou.x+bmsou.m_Pitch*spsou.y;
	int sounl=bmsou.m_Pitch-size.x*bmsou.m_BytePP;
	int sounp=bmsou.m_BytePP;

	if(bmsou.m_Format!=m_Format) return;
	//if(bmsou.m_BytePP!=m_BytePP) return;
	//if(bmsou.m_BitPP!=m_BitPP) return;

	if(bmsou.m_Format!=BMF_FLAT) return;
	if(bmsou.m_BytePP!=4) return;

	for(int y=0;y<size.y;y++,des+=desnl,sou+=sounl)
    {
        for(int x=0;x<size.x;x++,des+=desnp,sou+=sounp)
        {
			dword color=*(dword *)sou;
            dword ocolor = *des + (*(des+1) << 8) + (*(des+2) << 16);
			byte alpha = byte(color>>24);
            byte R = byte(color>>16);
            byte G = byte(color>>8);
            byte B = byte(color);

			byte oalpha = byte(color>>24);
            byte oR = byte(ocolor>>16);
            byte oG = byte(ocolor>>8);
            byte oB = byte(ocolor);

            if (alpha==0) continue;
			else
            if(alpha==255)
            {
                ocolor = color;
            } else
            {
                float A = alpha / 255.0f;

			    //*des=   byte((DWORD(B)*(DWORD(alpha)<<8))>>16)+byte((DWORD(oB)*(DWORD(255-alpha)<<8))>>16); // �� ������ ������ ������� =(

                int oiB = Float2Int(float(B) + float(oB) * (1.0f - A));
                int oiG = Float2Int(float(G) + float(oG) * (1.0f - A));
                int oiR = Float2Int(float(R) + float(oR) * (1.0f - A));

                int oiA = Float2Int(float(255-oalpha) * A);

                ocolor  = (oiB>255?255:oiB) | ((oiG>255?255:oiG)<<8) | ((oiR>255?255:oiR)<<16) | ((oiA>255?255:oiA)<<24);

                
                /*
                *des=   byte((DWORD(*sou)*(DWORD(alpha)<<8))>>16)
						    +byte((DWORD(*des)*(DWORD(255-alpha)<<8))>>16); // �� ������ ������ ������� =(
			    sou++; des++;
			    *des=   byte((DWORD(*sou)*(DWORD(alpha)<<8))>>16)
						    +byte((DWORD(*des)*(DWORD(255-alpha)<<8))>>16); // �� ������ ������ ������� =(
			    sou++; des++;

			    *des+=byte(((255-*des)*(DWORD(alpha)<<8))>>16);
                */
            }
            if (m_BytePP == 3)
            {
                *des = byte(ocolor);
                *(des+1) = byte(ocolor>>8);
                *(des+2) = byte(ocolor>>16);
            } else
            {

                *(dword *)des = ocolor;
            }

		}			
	}

}

void CBitmap::MergeGrayscaleWithAlpha(const Base::CPoint & pdes,const Base::CPoint & size, const CBitmap * data_src,const Base::CPoint & data_src_point, const CBitmap * alpha_src, const Base::CPoint & alpha_src_point)
{
    DTRACE();
    bool no_data = false;

    byte * dsou;
    int dsounl;

	if(data_src == NULL)
    {
        no_data = true;
    } else
    {
        if (data_src->m_Format!=BMF_FLAT || data_src->m_BytePP != 1 || data_src->m_BitPP != 8) ERROR_E;
	    dsou = (byte *)data_src->m_Data + data_src_point.x + data_src->m_Pitch*data_src_point.y;
	    dsounl=data_src->m_Pitch-size.x;
    }
    if(alpha_src == NULL) ERROR_E;

    if (m_Format!=BMF_FLAT || m_BytePP != 1 || m_BitPP != 8) ERROR_E;
    if (alpha_src->m_Format!=BMF_FLAT || alpha_src->m_BytePP != 4 || alpha_src->m_BitPP != 32) return;


	byte * des=(byte *)m_Data+m_BytePP*pdes.x+m_Pitch*pdes.y;
	int desnl=m_Pitch-size.x;

	byte * asou=(byte *)alpha_src->m_Data + alpha_src_point.x * 4 + alpha_src->m_Pitch*alpha_src_point.y + 3;
	int asounl=alpha_src->m_Pitch-size.x*4;

    if (no_data)
    {
	    for(int y=0;y<size.y;y++,des+=desnl,asou+=asounl)
        {
		    for(int x=0;x<size.x;x++,++des, asou+=4)
            {
			    byte alpha = *asou;
                byte odata;

                if (alpha==0) continue;
			    else
                if(alpha==255)
                {
                    odata = 0;
                } else
                {
                    odata = byte(float(255-alpha)*float(*des)*(1.0/255.0));
                }
                
                *des = odata;

		    }			
	    }
        return;
    }

	for(int y=0;y<size.y;y++,des+=desnl,asou+=asounl, dsou+=dsounl)
    {
		for(int x=0;x<size.x;x++,++des,++dsou, asou+=4)
        {
			byte alpha = *asou;
            byte odata;

            if (alpha==0) continue;
			else
            if(alpha==255)
            {
                odata = *dsou;
            } else
            {
                odata = byte((float(255-alpha)*float(*des) + float(alpha)*float(*dsou))*(1.0/255.0));
            }
            
            *des = odata;

		}			
	}

}

void CBitmap::MergeGrayscaleWithAlphaPM(const Base::CPoint & pdes,const Base::CPoint & size, const CBitmap * data_src,const Base::CPoint & data_src_point, const CBitmap * alpha_src, const Base::CPoint & alpha_src_point)
{
    DTRACE();
    bool no_data = false;

    byte * dsou;
    int dsounl;

	if(data_src == NULL)
    {
        no_data = true;
    } else
    {
        if (data_src->m_Format!=BMF_FLAT || data_src->m_BytePP != 1 || data_src->m_BitPP != 8) ERROR_E;
	    dsou = (byte *)data_src->m_Data + data_src_point.x + data_src->m_Pitch*data_src_point.y;
	    dsounl=data_src->m_Pitch-size.x;
    }
    if(alpha_src == NULL) ERROR_E;

    if (m_Format!=BMF_FLAT || m_BytePP != 1 || m_BitPP != 8) ERROR_E;
    if (alpha_src->m_Format!=BMF_FLAT || alpha_src->m_BytePP != 4 || alpha_src->m_BitPP != 32) return;


	byte * des=(byte *)m_Data+m_BytePP*pdes.x+m_Pitch*pdes.y;
	int desnl=m_Pitch-size.x;

	byte * asou=(byte *)alpha_src->m_Data + alpha_src_point.x * 4 + alpha_src->m_Pitch*alpha_src_point.y + 3;
	int asounl=alpha_src->m_Pitch-size.x*4;

    if (no_data)
    {
	    for(int y=0;y<size.y;y++,des+=desnl,asou+=asounl)
        {
		    for(int x=0;x<size.x;x++,++des, asou+=4)
            {
			    byte alpha = *asou;
                byte odata;

                if (alpha==0) continue;
			    else
                if(alpha==255)
                {
                    odata = 0;
                } else
                {
                    odata = byte(float(255-alpha)*float(*des)*(1.0/255.0));
                }
                
                *des = odata;

		    }			
	    }
        return;
    }

	for(int y=0;y<size.y;y++,des+=desnl,asou+=asounl, dsou+=dsounl)
    {
		for(int x=0;x<size.x;x++,++des,++dsou, asou+=4)
        {
            *des = byte((float(255 - *asou)*float(*des) + 255.0*float(*dsou))*(1.0/255.0));
		}			
	}

}


void CBitmap::ModulateGrayscaleWithAlpha(const Base::CPoint & pdes,const Base::CPoint & size, const CBitmap * alpha_src, const Base::CPoint & alpha_src_point)
{
    DTRACE();

    if(alpha_src == NULL) ERROR_E;

    if (m_Format!=BMF_FLAT || m_BytePP != 1 || m_BitPP != 8) ERROR_E;
    if (alpha_src->m_Format!=BMF_FLAT || alpha_src->m_BytePP != 4 || alpha_src->m_BitPP != 32) return;


	byte * des=(byte *)m_Data+m_BytePP*pdes.x+m_Pitch*pdes.y;
	int desnl=m_Pitch-size.x;

	byte * asou=(byte *)alpha_src->m_Data + alpha_src_point.x * 4 + alpha_src->m_Pitch*alpha_src_point.y + 3;
	int asounl=alpha_src->m_Pitch-size.x*4;

	for(int y=0;y<size.y;y++,des+=desnl,asou+=asounl)
    {
		for(int x=0;x<size.x;x++,++des, asou+=4)
        {
			byte alpha = *asou;

            if (alpha==255) continue;
			else
            {
                *des = byte((float(alpha)*float(*des))*(1.0/255.0));
            }

		}			
	}
}

void sharpen_run(CBitmap &obm, CBitmap &ibm, int lv);

void CBitmap::Sharpen(int lv)
{
    CBitmap bb(m_Heap);
    bb.CreateRGBA(SizeX(), SizeY());

    sharpen_run(bb, *this, lv);

    void *temp = m_Data;

    m_Data = bb.m_Data;

    bb.m_Data = temp;
}


void CBitmap::WBM_Clear()
{
	if(m_WindowDC) { DeleteDC(m_WindowDC); m_WindowDC=0; }
	if(m_WindowBitmap) { DeleteObject(m_WindowBitmap); m_WindowBitmap=0; }
}

void CBitmap::WBM_Init()
{
	WBM_Clear();

	if(m_Format!=BMF_FLAT) return;

	BITMAPV4HEADER bmi;

	ZeroMemory(&bmi,sizeof(bmi));
	bmi.bV4Size = sizeof(bmi);
	bmi.bV4Width = SizeX();
    bmi.bV4Height = SizeY();
	bmi.bV4Planes = 1;
	bmi.bV4BitCount = (WORD)BitPP();
	if(m_BytePP>=3)
    {
        bmi.bV4V4Compression = BI_RGB;
        bmi.bV4SizeImage = 0;
    }
	else
    {
		bmi.bV4V4Compression = BI_BITFIELDS;
		bmi.bV4RedMask=0x0000f800;
		bmi.bV4GreenMask=0x000007e0;
		bmi.bV4BlueMask=0x0000001f;
		bmi.bV4AlphaMask=0;
        bmi.bV4SizeImage = SizeX()*SizeY()*BytePP();
	}

	HDC tdc=GetDC(0);

    void * pvBits;

	m_WindowBitmap=CreateDIBSection(tdc, (BITMAPINFO *)&bmi, DIB_RGB_COLORS, &pvBits, 0, 0);
    if(m_WindowBitmap==0) ERROR_E;

//	DWORD lenLine=DWORD(SizeX()*BytePP());

	if((m_BytePP==3 || m_BytePP==4) && m_MColor[0]==0x0ff && m_MColor[1]==0x0ff00 && m_MColor[2]==0x0ff0000) SwapByte(CPoint(0,0),m_Size,0,2);

	DWORD ll=DWORD(SizeX()*BytePP());
    DWORD lls=(ll+3) & (~3);
	BYTE * bdes=(BYTE *)(pvBits)+lls*DWORD(SizeY()-1);
	BYTE * bsou=(BYTE *)(Data());
	for(int y=0;y<SizeY();y++) {
		CopyMemory(bdes,bsou,SizeX()*BytePP());
        bsou=bsou+ll;
        bdes=bdes-lls;
	}

	if((m_BytePP==3 || m_BytePP==4) && m_MColor[0]==0x0ff && m_MColor[1]==0x0ff00 && m_MColor[2]==0x0ff0000) SwapByte(CPoint(0,0),m_Size,0,2);

	m_WindowDC=CreateCompatibleDC(tdc);
	if(m_WindowDC==0) ERROR_E;

	if(SelectObject(m_WindowDC,m_WindowBitmap)==0) ERROR_E;
	
	ReleaseDC(0,tdc);
}

void CBitmap::WBM_Save(bool save16as32)
{
	if(!m_WindowDC || !m_WindowBitmap) return;

	struct {
		struct {
			BITMAPV4HEADER bmiHeader;
		} bmi;
		DWORD pal[256];
	} b;

	ZeroMemory(&b,sizeof(b));
	b.bmi.bmiHeader.bV4Size=sizeof(BITMAPINFOHEADER);
	if(GetDIBits(m_WindowDC,m_WindowBitmap,0,0,NULL,(LPBITMAPINFO)&b.bmi,DIB_RGB_COLORS)==0) return;

	if(b.bmi.bmiHeader.bV4BitCount!=16 && b.bmi.bmiHeader.bV4BitCount!=24 && b.bmi.bmiHeader.bV4BitCount!=32) return;

	if(save16as32 && b.bmi.bmiHeader.bV4BitCount==16)
    {
        b.bmi.bmiHeader.bV4BitCount = 32;
    }
	
    if (b.bmi.bmiHeader.bV4BitCount==16) Create16(b.bmi.bmiHeader.bV4Width,b.bmi.bmiHeader.bV4Height);
    else if(b.bmi.bmiHeader.bV4BitCount==24) CreateRGB(b.bmi.bmiHeader.bV4Width,b.bmi.bmiHeader.bV4Height);
	else if(b.bmi.bmiHeader.bV4BitCount==32) CreateRGBA(b.bmi.bmiHeader.bV4Width,b.bmi.bmiHeader.bV4Height);
	Fill(CPoint(0,0),Size(),0);

	if(GetDIBits(m_WindowDC,m_WindowBitmap,0,b.bmi.bmiHeader.bV4Height,Data(),(LPBITMAPINFO)&b.bmi,DIB_RGB_COLORS)==0) return;

	FlipY();
	if(b.bmi.bmiHeader.bV4BitCount==24 || b.bmi.bmiHeader.bV4BitCount==32) SwapByte(CPoint(0,0),Size(),0,2);

}

void CBitmap::SaveInBMP(Base::CBuf & buf) const
{
	BITMAPFILEHEADER bmFileHeader;
	BITMAPINFOHEADER bmInfoHeader;

	if(m_Format!=BMF_FLAT && m_Format!=BMF_PALATE) return;
	if(m_Format==BMF_PALATE && m_BytePP!=1) return;
	if(m_BytePP!=1 && m_BytePP!=3 && m_BytePP!=4) return;
	if((m_BitPP>>3)!=m_BytePP) return;

	int lPitch=((m_BytePP*m_Size.x-1)/4+1)*4;

	bmFileHeader.bfType=0x4D42;
	bmFileHeader.bfSize=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+lPitch*m_Size.y;
	if(m_Format==BMF_PALATE) bmFileHeader.bfSize+=m_AddDataVal[0]*4;
	bmFileHeader.bfOffBits=bmFileHeader.bfSize-lPitch*m_Size.y;
	buf.BufAdd(&bmFileHeader, sizeof(BITMAPFILEHEADER));

	memset(&bmInfoHeader, 0, sizeof(BITMAPINFOHEADER));
	bmInfoHeader.biSize   = sizeof(BITMAPINFOHEADER);
	bmInfoHeader.biWidth  = m_Size.x;
	bmInfoHeader.biHeight = m_Size.y;
	bmInfoHeader.biPlanes = 1;
	bmInfoHeader.biBitCount=(WORD)m_BitPP;
	bmInfoHeader.biCompression=0;
	bmInfoHeader.biSizeImage=lPitch*m_Size.y;
	buf.BufAdd(&bmInfoHeader, sizeof(BITMAPINFOHEADER));

	if(m_Format==BMF_PALATE) {
		buf.BufAdd(m_AddData[0],m_AddDataVal[0]*4);
	}

	BYTE * sou=(byte *)m_Data+(m_Size.y-1)*m_Pitch;

	//if((m_BytePP==3 || m_BytePP==4) && m_MColor[0]==0x0ff && m_MColor[1]==0x0ff00 && m_MColor[2]==0x0ff0000) SwapByte(CPoint(0,0),m_Size,0,2);

	int len=m_BytePP*m_Size.x;
    if(m_MColor[0]==0x0ff && m_MColor[1]==0x0ff00 && m_MColor[2]==0x0ff0000)
    {
        if (m_BytePP==3)
        {
	        for(int y=0;y<m_Size.y;y++,sou-=m_Pitch)
            {
                BYTE *sb = (BYTE *)sou;
                buf.TestAdd(len);
                BYTE *db = ((BYTE *)buf.Get()) + buf.Pointer();
                buf.Pointer(buf.Pointer()+len);
                BYTE *de = db + len;

                for (;db < de; db += 3, sb += 3)
                {
                    *(db + 2) = *(sb + 0);
                    *(db + 1) = *(sb + 1);
                    *(db + 0) = *(sb + 2);
                }
		    
                if(len<lPitch) buf.ByteLoop(0,lPitch-len);
            }
        } else if (m_BytePP==4)
        {
	        for(int y=0;y<m_Size.y;y++,sou-=m_Pitch)
            {
                BYTE *sb = (BYTE *)sou;
                buf.TestAdd(len);
                BYTE *db = ((BYTE *)buf.Get()) + buf.Pointer();
                buf.Pointer(buf.Pointer()+len);
                BYTE *de = db + len;

                for (;db < de; db += 4, sb += 4)
                {
                    *(db + 2) = *(sb + 0);
                    *(db + 1) = *(sb + 1);
                    *(db + 0) = *(sb + 2);
                    *(db + 3) = *(sb + 3);
                }
		    
                if(len<lPitch) buf.ByteLoop(0,lPitch-len);
            }
        } else goto oformat;

    } else
    {
oformat:
	    for(int y=0;y<m_Size.y;y++,sou-=m_Pitch)
        {
		    buf.BufAdd(sou,len);
		    if(len<lPitch) buf.ByteLoop(0,lPitch-len);
	    }
    }

	//if((m_BytePP==3 || m_BytePP==4) && m_MColor[0]==0x0ff && m_MColor[1]==0x0ff00 && m_MColor[2]==0x0ff0000) SwapByte(CPoint(0,0),m_Size,0,2);
}

void CBitmap::SaveInBMP(const wchar * filename, int filenamelen) const
{
	CBuf buf(m_Heap);
	SaveInBMP(buf);
	buf.SaveInFile(filename,filenamelen);
}

#pragma warning (disable : 4731)
void CBitmap::SaveInDDSUncompressed(Base::CBuf & buf) const
{
    if (m_BytePP != 3 && m_BytePP != 4)
    {
        ERROR_S(L"Unsupported format to convert");
    }

    int sz = m_Size.x * m_Size.y * m_BytePP;

    buf.Clear();
    buf.Expand(sz + sizeof(DDSURFACEDESC2) + sizeof(DWORD));

    DWORD *dds = (DWORD *)buf.Get();
    *dds = 0x20534444; // "DDS "

    DDSURFACEDESC2 * ddsp = (DDSURFACEDESC2 *)(dds + 1);
    memset(ddsp, 0, sizeof(DDSURFACEDESC2));
    
    ddsp->dwSize = sizeof(DDSURFACEDESC2);
    ddsp->dwWidth = m_Size.x;
    ddsp->dwHeight = m_Size.y;
    ddsp->dwFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE;

    ddsp->dwLinearSize = sz;
    ddsp->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    ddsp->ddpfPixelFormat.dwFlags = DDPF_RGB | ((m_BytePP == 4)?DDPF_ALPHAPIXELS:0);
    ddsp->ddpfPixelFormat.dwRGBBitCount = m_BitPP;
    ddsp->ddpfPixelFormat.dwRBitMask = 0x00FF0000;
    ddsp->ddpfPixelFormat.dwGBitMask = 0x0000FF00;
    ddsp->ddpfPixelFormat.dwBBitMask = 0x000000FF;
    if (m_BytePP == 4) ddsp->ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;

    ddsp->ddsCaps.dwCaps = DDSCAPS_TEXTURE;

    if (m_Size.x < 4)
    {
        int cnt = m_Size.x * m_Size.y;
        BYTE *des = (BYTE *)(ddsp + 1);
        BYTE *sou = (BYTE *)m_Data;
        if (m_BytePP == 3)
        {
            while (cnt-- > 0)
            {
                *des = *(sou + 2);
                *(des+1) = *(sou + 1);
                *(des+2) = *(sou + 0);
                sou += 3;
                des += 3;
            }
        } else
        {
            while (cnt-- > 0)
            {
                *des = *(sou + 2);
                *(des+1) = *(sou + 1);
                *(des+2) = *(sou + 0);
                *(des+3) = *(sou + 3);
                sou += 3;
                des += 3;
            }

        }

    } else
    {

    //DWORD *des = (DWORD *)(ddsp + 1);
    //DWORD *sou = (DWORD *)m_Data;
    //for (int i=0; i<1000; ++i)
    //{
    //    DWORD s = *sou;
    //    *des = (s & 0xFF00FF00) | ((s >> 16) & 0xFF) | ((s << 16) & 0xFF0000);
    //    ++sou;
    //    ++des;
    //}

        _asm
        {
            // dest
            mov     edi, ddsp
            add     edi, TYPE DDSURFACEDESC2

            // source
            mov     esi, this
            mov     ecx, [esi + m_Size]
            mov     ebx, [esi + m_BytePP]
            shr     ecx, 2
            mov     eax, [esi + m_Size + 4]
            mul     ecx
            mov     esi, [esi + m_Data]
            mov     ecx, eax

            cmp     ebx, 3
            jz      bpp3

    loop1:
            mov     eax, [esi]
            mov     edx, [esi + 4]

            mov     ebx, eax
            and     eax, 0xFF00FF00
            ror     ebx, 16
            add     edi, 16
            and     ebx, 0x00FF00FF
            or      eax, ebx
            mov     ebx, edx
            and     edx, 0xFF00FF00
            ror     ebx, 16
            add     esi, 16
            and     ebx, 0x00FF00FF
            or      edx, ebx

            mov     [edi - 16], eax
            mov     [edi - 12], edx

            mov     eax, [esi - 8]
            mov     edx, [esi - 4]

            mov     ebx, eax
            and     eax, 0xFF00FF00
            ror     ebx, 16
            and     ebx, 0x00FF00FF
            or      eax, ebx
            mov     ebx, edx
            and     edx, 0xFF00FF00
            ror     ebx, 16
            and     ebx, 0x00FF00FF
            or      edx, ebx

            mov     [edi - 8], eax
            mov     [edi - 4], edx

            dec     ecx
            jnz     loop1

            jmp     end
    bpp3:
            push    ebp
loop2:
            // converts every 4 pixels (3 DWORD's)

            mov     eax, [esi]
            mov     edx, [esi + 4]
            mov     ebx, eax
            and     eax, 0x0000FF00     // XXXXG0XX
            ror     ebx, 16
            mov     ebp, ebx
            and     ebx, 0x00FF00FF     // XXR0XXG0
            or      eax, ebx
            mov     ebx, edx
            and     ebx, 0x0000FF00     // XXXXR1XX
            and     ebp, 0x0000FF00     // XXXXR0XX
            shl     ebx, 16
            or      eax, ebx
            mov     ebx, edx            // store B2
            mov     [edi], eax
            and     edx, 0xFF0000FF     // G2XXXXG1
            shr     ebx, 16
            or      edx, ebp
            mov     eax, [esi + 8]
            and     ebx, 0x000000FF
            mov     ebp, eax
            and     eax, 0x00FF0000
            ror     ebp, 16
            or      eax, ebx
            mov     ebx, ebp
            and     ebp, 0x00FF0000     // XXR2XXXX
            or      edx, ebp
            mov     [edi + 4], edx
            and     ebx, 0xFF00FF00
            or      eax, ebx
            mov     [edi + 8], eax

            add     esi, 12
            add     edi, 12

            dec     ecx
            jnz     loop2


            pop     ebp
    end:

        };

    }
    //memcpy(des,sou,sz);
}
#pragma warning (default : 4731)

void CBitmap::SaveInDDSUncompressed(const wchar * filename, int filenamelen) const
{
	CBuf buf(m_Heap);
	SaveInDDSUncompressed(buf);
	buf.SaveInFile(filename,filenamelen);
}


#ifdef USE_PNGLIB
bool CBitmap::LoadFromPNG(void * buf, int buflen)
{
	Clear();

	DWORD lenx,leny,countcolor,format;

	DWORD id=FilePNG_ReadStart_Buf(buf,buflen,&lenx,&leny,&countcolor,&format);
	if(id==0) return false;

	if(format==1) {
		CreateGrayscale(lenx,leny);
	} else if(format==2) {
		CreateRGB(lenx,leny);
	} else if(format==3) {
		CreateRGBA(lenx,leny);
	} else if(format==4) {
		CreatePalate(lenx,leny,countcolor);
	} else ERROR_E;

	if(!FilePNG_Read(id,m_Data,m_Pitch,(DWORD *)m_AddData[0])) {
		Clear();
		return false;
	}

	return true;
}

bool CBitmap::LoadFromPNG(const wchar * filename)
{
	CFile file(filename,m_Heap);
	file.OpenRead();
	int size=file.Size();
	if(size<0) return false;
	void * buf;
    if (size <= 32768)
    {
        buf = _alloca(size);
    } else
    {
        buf = HAlloc(size,m_Heap);
    }
	try {
		file.Read(buf,size);
		LoadFromPNG(buf,size);
		if (size > 32768) HFree(buf,m_Heap);
		return true;
	} catch(...) {
		if (size > 32768) HFree(buf,m_Heap);
		throw;
	}
}

int CBitmap::SaveInPNG(void * buf, int buflen)
{
	if(m_Format!=BMF_FLAT) return 0;
	if(m_Size.x<=0 || m_Size.y<=0) return 0;

	return FilePNG_Write(buf,buflen,m_Data,m_Pitch,m_Size.x,m_Size.y,m_BytePP,0);
}

bool CBitmap::SaveInPNG(CBuf & buf)
{
    int len = m_Pitch*m_Size.y+100;
	buf.Len(len);
	len=SaveInPNG(buf.Get(),buf.Len());
	if(len>buf.Len()) {
		buf.Len(len);
		len=SaveInPNG(buf.Get(),buf.Len());
	} else buf.Len(len);
	return len>0;
}

bool CBitmap::SaveInPNG(const wchar * filename, int filenamelen)
{
	CBuf buf(m_Heap);
	if(!SaveInPNG(buf)) return false;

	CFile file(filename,filenamelen,m_Heap);
	file.Create();
	file.Write(buf.Get(),buf.Len());
	file.Close();

	return true;
}

#endif
