#pragma once

#include "resource.h"

class CMatrixMapLogic;
class CIFaceList;
class CRenderPipeline;
class Base::CBlockPar;
class Base::CHeap;
class Base::CPoint;
class CLoadProgress;
class CHistory;
struct SMenuItemText;

extern Base::CHeap *        g_MatrixHeap;
extern Base::CBlockPar *    g_MatrixData;
extern CMatrixMapLogic *    g_MatrixMap;
extern CIFaceList *         g_IFaceList;
extern CRenderPipeline *    g_Render;
extern CLoadProgress *      g_LoadProgress;
extern SMenuItemText *      g_PopupHead;
extern SMenuItemText *      g_PopupWeaponNormal;
extern SMenuItemText *      g_PopupWeaponExtern;
extern SMenuItemText *      g_PopupHull;
extern SMenuItemText *      g_PopupChassis;
extern CHistory *           g_ConfigHistory;

struct SRobotsSettings;

void MatrixGameInit(HINSTANCE hInstance,HWND wnd,wchar * map=NULL,SRobotsSettings * set=NULL,wchar * txt_start=NULL,wchar * txt_win=NULL,wchar * txt_loss=NULL, wchar *planet=NULL);
void MatrixGameDeinit(void);
