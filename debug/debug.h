#include "ideastypes.h"
#include "cpu.h"
#include "bp.h"
#include "memview.h"
#include "lslist.h"
#include "ldlg.h"
#include "spriteviewer.h"
#include "textureviewer.h"
#include "paletteviewer.h"
#include "ioviewer.h"
#include "tileviewer.h"
#include "mtxviewer.h"

//---------------------------------------------------------------------------
#ifndef __debugH__
#define __debugH__

typedef struct _dispointer{
   u32 StartAddress;
   u32 EndAddress;
} DISVIEW,*LPDISVIEW;
//---------------------------------------------------------------------------
typedef struct {
   u32 yScroll;
   DISVIEW views[2];
   u8 iMaxItem,bTrackMode,iCurrentView,bCurrentTrackMode;
} DEBUGGER;
//---------------------------------------------------------------------------
enum PAGETYPE {PG_NULL=-1,PG_MEMVIEW=1,PG_BREAKPOINT,PG_MEMBREAKPOINT,PG_SYSTEMCONTROL,PG_CALLSTACK,PG_CONSOLLE};
enum CHANGEREGTYPE {CRT_ZERO,CRT_INCR,CRT_DECR,CRT_CHANGE};
//---------------------------------------------------------------------------
#define DBV_RUN    		0
#define DBV_VIEW   		1
#define DBV_NULL   		0xFF

#define MSGT_CP			1
#define MSGT_BIOS			2
#define MSGT_SPI			3
#define MSGT_MEMBRK        4
#define MSGT_CARD          5
#define MSGT_PXI7          6
#define MSGT_PXI9          7
#define MSGT_POWER			8
#define MSGT_TOUCH			9
#define MSGT_DMA9          10
#define MSGT_DMA7          11
#define MSGT_OUTONCONSOLLE 12
#define MSGT_FIRMWARE      13
#define MSGT_FLASH         14

#if defined(_DEBUG)
//---------------------------------------------------------------------------
class LConsolleDlg : public LDlg
{
public:
	LConsolleDlg();
   ~LConsolleDlg();
   BOOL Destroy();
   BOOL Create(HWND hwnd);
   LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   BOOL Reset();
   void Update();
   void set_ActiveView(int index);
   void Resize(int x,int y,int w,int h);
   BOOL WriteMessageConsole(IARM7 *cpu,char *mes,...);
   void OnCommand(WORD wID,WORD wCode,HWND hwnd);
   LONG OnNotify(LPNMHDR p);
	static LRESULT CALLBACK ListBoxWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
   LRESULT OnListBoxWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
protected:
   void Clear();
   void Save();
	LStringList *pStringConsole;
   int nActiveView;
   HIMAGELIST imageListDebug[2];
   BYTE showMessage[20];
   BOOL bStop;
   WNDPROC oldListBoxWndProc;
};
//---------------------------------------------------------------------------
class LToolsDlg : public LDlg
{
public:
	LToolsDlg();
	~LToolsDlg();
	BOOL Destroy();
   BOOL Create();
   LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   LRESULT OnTabControlWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   BOOL Reset();
   void Update();
   BOOL OnMenuSelect(WORD wID);
   BOOL IsDialogMessage(LPMSG lpMsg);
#if defined(_DEBPRO)
   inline void set_CurrentAccess(u32 address,u32 access){memView.set_CurrentAccess(address,access);};
#endif
	BOOL toFloat(BOOL bFloat = TRUE);
   BOOL OnSize(int width,int height);
   BOOL is_Float();
   PAGETYPE get_ActivePageType(int iPage = -1);
   void OnChangeCPU();
#if defined(_DEBUG)
   inline LConsolleDlg *get_ConsolleView(){return &consolleView;};
#endif
   void UpdateCP(u8 reg,u8 r2,u8 op2);
   HWND get_BreakPointListView(){return GetDlgItem(m_hWnd,IDC_DEBUG_LV1);};
   BOOL SelectPage(PAGETYPE page);
   BOOL SelectPage(int index);
protected:
	static LRESULT CALLBACK TabControlWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void OnSelChangeTab1(LPNMHDR h);
   void OnCommand(WORD wNotifyCode,WORD wID,HWND hwnd);
   LONG OnNotify(LPNMHDR p);
   BOOL OnMeasureItem(UINT id,LPMEASUREITEMSTRUCT p);
   BOOL OnDrawItem(UINT id,LPDRAWITEMSTRUCT p);
   LConsolleDlg consolleView;
   LMemoryView memView;
	HIMAGELIST hil;
   WNDPROC oldTabControlWndProc;
   BOOL bPressed;
};
//---------------------------------------------------------------------------
class LDisListBox : public LWnd
{
public:
	LDisListBox(){hMenu = NULL;};
   ~LDisListBox(){if(hMenu != NULL) DestroyMenu(hMenu);};
   LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
protected:
	HMENU hMenu;
};
//---------------------------------------------------------------------------
class LRegListBox : public LWnd
{
public:
   LRegListBox();
   ~LRegListBox();
   LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
};
//---------------------------------------------------------------------------
class LInspectDlg : public LDlg
{
public:
   LInspectDlg();
   ~LInspectDlg();
   LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   inline LString get_Text(){return text;};
protected:
   LString text;
};
//---------------------------------------------------------------------------
class LDebugDlg : public LWnd
{
public:
   LDebugDlg();
   ~LDebugDlg();
   BOOL Destroy();
   BOOL Modeless(LPVOID p);
   void OnClose();
   BOOL OnDrawItem(WORD wID,LPDRAWITEMSTRUCT lpDIS);
   void OnInitDialog(HWND hwnd);
   void OnMenuSelect(WORD wID);
   void OnCommand(WORD wNotifyCode,WORD wID,HWND hwnd);
   void Update_Debugger();
   void Init_Debugger();
   void Reset_Debugger();
	void OnVScroll(WPARAM wParam,LPARAM lParam);
	int MemoryAddressToIndex(DWORD dwAddress);
	int MemoryStringToIndex(char *string);
   BOOL OnKeyDown(WPARAM wParam,LPARAM lParam);
   void WaitDebugger(BOOL bArm7);
   inline void StopDebugger(){bDebug = FALSE;bPass = TRUE;};
   void EnterDebugMode(BOOL bArm7);
   inline IARM7 *get_CurrentCPU(){return nCpu == 0 ? (IARM7 *)&arm9 : (IARM7 *)&arm7;};
   BOOL ControlMemoryBP(u32 address,u32 access,u32 value);
   LONG OnNotify(LPNMHDR p);
   inline BOOL is_Active(){return bDebug;};
   void OnSize(WPARAM wParam,LPARAM lParam);
   void OnGetMinMaxInfo(LPMINMAXINFO lParam);
   void ChangeRegister(CHANGEREGTYPE mode);
   int FillListDiss(u32 *p,u8 indexView,u8 nItem);
   inline int get_CurrentViewIndex(){return dbg.iCurrentView;};
   inline LPDISVIEW get_CurrentView(){return &dbg.views[dbg.iCurrentView];};
   inline BOOL is_Run(){return is_Active() && !bPass;};
	void UpdateAddressBar(u32 dwAddress,u8 forceUpdate);
   void UpdateToolBar();
	void OnEnterIRQ(u32 value,int index);
#if defined(_DEBPRO2)
   inline LListBreakPoint *get_BP(){return &prgBP;};
   void OnDeleteBP(LBreakPoint *p = (LBreakPoint *)-1);
   void OnGoBP(LBreakPoint *p);
   BOOL InsertBreakpointFromIndex(int index);
   LListBreakPoint prgBP;
#endif
	BOOL IsDialogMessage(LPMSG lpMsg);
protected:
   void OnInitMenuPopup(HMENU hMenu,UINT uPos,BOOL bSM);
	static BOOL CALLBACK DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
   u8 GetCurrentIncPipe(u8 indexView);
   u8 GetCurrentAddressPipe(u32 *Address,u8 withindex,u8 remap,BOOL bFromPC = TRUE);
   void UpdateVertScrollBarDisDump(DWORD address,int index);
   void Update_Reg();
	BOOL BreakpointFromListBox(char *p,u32 *adr);
   BOOL BreakpointFromIndex(int index,char *p,u32 *adr);
   BOOL ControlBP();
   s8 InsertBreakPoint(u32 adress);
//---------------------------------------------------------------------------
   int nMode,nCpu,yScroll;
   DEBUGGER dbg;
   u8 nBreakMode;
   DWORD dwBreak,dwBreakIRQ[2],dwCheckAddress;
   BOOL bStep,bDebug,bPass,bLino,bFlag;
   LPVOID parent;
   LFile *pFileIO,*pFileDis;
#ifdef _LWYLIN
	LFile *pFileOut7,*pFileOut9;
#endif
	LStringList RecentFiles;
   LDisListBox disLB;
   LRegListBox regLB;
   HACCEL accel;
   char cLinoSearch[20];
   RECT rcClient,rcWin;
   HIMAGELIST imageListDebug[2];
   LPaletteViewer palViewer;
   LMatrixViewer mtxViewer;
#ifdef _DEBPRO4
   LSpriteViewer spriteViewer;
#endif   
#ifdef _DEBPRO5
	LTileViewer tileViewer;
#endif

#ifdef _DEBPRO
   LTextureViewer textureViewer;
   LIOViewer ioViewer;
#endif
};
#endif

#endif
