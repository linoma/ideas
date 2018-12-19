#include "ideastypes.h"
#include "cpu.h"
#include "dsreg.h"
#include "gpu.h"
#include "debug.h"
#include "lslist.h"
#include "pluginctn.h"
#include "lapp.h"
#include "lmenu.h"
#include "touch.h"
#include "audioplug.h"
#include "savestate.h"

//---------------------------------------------------------------------------
#ifndef __LDSH__
#define __LDSH__

#define NDSO_OPTIMIZE_CPU				0x01
#define NDSO_OPTIMIZE_LOOP				0x02
#define NDSO_LOAD_DEFAULT_ARM7			0x04
#define NDSO_LOAD_BIOS					0x08
#define NDSO_CPU_SUPPORT_SSE			0x10
#define NDSO_BACKUP_AUTO_SAVE			0x20
#define NDSO_FRAMES_AUTO_SKIP			0x40
#define NDSO_FRAMES_SKIP_CAPTURE		0x80
#define NDSO_HINGE						0x100
#define NDSO_USE_FAT_EFSLIB_EMULATION	0x200
#define NDSO_BOOT_FROM_BIOSES			0x400
#define NDSO_LOAD_FIRMWARE				0x800
#define NDSO_USE_DMA_LATENCY           0x1000

/*
struct{
   struct{
       struct{
           unsigned long code_p;
           unsigned long code_s;
           char *code_c;
       } pointer[20];
       unsigned long count;
   } arm7;
   struct{
       struct{
           unsigned long code_p;
           unsigned long code_s;
           char *code_c;
       } pointer[20];
       unsigned long count;
   } arm9;
   HANDLE hPr;
   DWORD dwSizeCode,dwOffset;
   unsigned char *buf;
} lino_cazzo;*/

//---------------------------------------------------------------------------
class LDS : public LMMU,public LCPU,public LApp,public INDS,public INDSMemory
{
public:
	LDS();
   ~LDS();
   BOOL Init(int argc,char **argv);
   void Reset(BOOL bAll = TRUE);
   u32 Loop(){return (this->*pfnLoop)();};
   void MainLoop();
   void OnMenuSelect(WORD wID);
   BOOL OnKeyDown(WPARAM wParam,LPARAM lParam);
   BOOL OnKeyUp(WPARAM wParam,LPARAM lParam);
   void set_IRQ(u32 value,BOOL bEnter = TRUE,u8 c = 0xFF);
   LONG DispatchMessage(LPMSG p);
   inline HWND Handle(){return upLcd.Handle();};
   void OnInitMenuPopup(HMENU hMenu,UINT uPos,BOOL bSM);
   BOOL OpenRom(char *lpFileName);
   void StartArm9(u32 data){
   	bArm9 = TRUE;
		arm9.InitPipeLine(data);
   }
//   void EnableIRQ(int index,BOOL value = TRUE);
	inline BOOL get_UseFATxEFS(){return dwOptions & NDSO_USE_FAT_EFSLIB_EMULATION ? TRUE : FALSE;};
   inline void set_UseFATxEFS(BOOL value){if(value) dwOptions |= NDSO_USE_FAT_EFSLIB_EMULATION;else dwOptions &= ~NDSO_USE_FAT_EFSLIB_EMULATION;};
   inline BOOL get_LoadDefARM7(){return (dwOptions & NDSO_LOAD_DEFAULT_ARM7) != 0 ? TRUE : FALSE;};
   inline void set_LoadDefARM7(BOOL f){if(f) dwOptions |= NDSO_LOAD_DEFAULT_ARM7; else dwOptions &= ~NDSO_LOAD_DEFAULT_ARM7;};
   inline void set_TouchScreenSettings(int value){nTouchSettings = value;Touch_Settings();};
   inline int get_TouchScreenSettings(){return nTouchSettings;};
   inline void set_ActiveAudioPlugIn(AudioPlug *p){
       if(p != NULL && p->IsAttribute(PIT_ISMICROPHONE))
       	p = NULL;
   	pActiveAudioPlugIn = p;
   };
   inline AudioPlug *get_ActiveAudioPlugIn(){return pActiveAudioPlugIn;};
   inline void set_ActiveVideoPlugIn(VideoPlug *p){
       if(p != NULL && !p->IsAttribute(PIT_IS3D))
       	p = NULL;
   	p3DPlugIn = p;
   };
   inline VideoPlug *get_ActiveVideoPlugIn(){return p3DPlugIn;};
   inline Render2DPlugIn *get_Active2DPlugIn(){return p2DPlugIn;};
   inline void set_Active2DPlugIn(Render2DPlugIn *p){p2DPlugIn = p;};

   inline BOOL is_Hinge(){return dwOptions & NDSO_HINGE ? TRUE : FALSE;};
   void OnDisplayCapture(DWORD data);
   void DisplayCapture();
   void OnDisplaySwap(u16 value);
   inline u32 get_CurrentStatus(){return dwStatus;};
   BOOL MakeScreenShot(int mode = -1);
   inline void get_RomStatus(BOOL *a,BOOL *b){if(a != NULL) *a = bRom;if(b!= NULL) *b = bRun;};
   inline BOOL hasSSE(){return (dwOptions & NDSO_CPU_SUPPORT_SSE) ? TRUE : FALSE;};
   void set_SSE(BOOL bFlag);
   void OnMenuLoop(BOOL bEnter);
   void PostQuitMessage();
   BOOL OnDlgProc22(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
   BOOL OnDlgProc23(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
   inline int get_CurrentCycles(){return nTotCycles + wCycles;};
   inline unsigned short get_CurrentLineCycles(){return wCycles;};
	inline int set_Optimize(DWORD value){dwOptions = value;return S_OK;};
   inline int get_Optimize(){if(bFreeRunning) return 3;return dwOptions;};
   inline int get_GripKeys(){return nGripKeys;};
   inline DWORD get_Options(){return dwOptions;};
   void OnChangeDISPSTAT(int index,u8 *mem,u32 data);
   BOOL is_FatEnable(int slot){return fat.is_Enable(slot);};
   inline DWORD Frames(){return dwFrame;};
   inline const int get_MultiCore(){return nCores;};
   void set_MultiCore(int value);
   inline const unsigned short get_StopCycles(){return wStopCycles;};
   inline void set_StopCycles(unsigned short value){wStopCycles = value;};
   DWORD OnThread_01();
   BOOL Load(LStream *pFile,int ver);
   void set_EmuMode(int mode);
   //INDS Methods
   int QueryInterface(int iid,LPVOID *ppvObject);
   int get_FramesCount(LPDWORD value);
   int get_CurrentFileName(char *buffer,int *maxLength);
   int CreateFileName(int type,char *buffer,int *maxLength,char *ext,int ID);
   int WriteConsole(char *msg);
	int get_IsHomebrew(int *res);
   int get_EmulatorMode(int *res);
   int get_NumberOfCores(LPDWORD value);
   int get_DutyCycles(LPDWORD);
   //INDSMemory
   int get_VideoMemoryStatus(DWORD adr,DWORD size,LPDWORD status);
   int set_VideoMemoryStatus(DWORD adr,DWORD size,DWORD status);
#ifdef _DEBPRO
	void RefreshLCDs();
#endif
	HACCEL accel;
   BOOL bDouble;
protected:
   BOOL EnableMultiCore(BOOL bFlag);
	void OnShowRomInfo();
	void OnShowProperties();
   u32 OnLoop();
//   u32 OnLoopEx();
   u32 OnLoopGBA();
   void DisplaySwap();
	static BOOL CALLBACK DlgProc15(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static BOOL CALLBACK DlgProc16(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static BOOL CALLBACK DlgProc17(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static BOOL CALLBACK DlgProc22(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static BOOL CALLBACK DlgProc23(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);   
   static DWORD WINAPI ThreadFunc_01(LPVOID arg);
#ifdef __WIN32__
   ATOM classId;
#endif
   BOOL bRom,bRun,bArm7,bArm9,bPause,bFreeRunning;
   LStringList RecentFiles;
   LString lastPath;
   DWORD dwFrame,dwLastTick,dwStatus,dwLastTick1,dwOptions;
   AudioPlug *pActiveAudioPlugIn;
   VideoPlug *p3DPlugIn;
   Render2DPlugIn *p2DPlugIn;
   int nTouchSettings,nSkipCount,nTotCycles,nCommand,nGripKeys,nCores,nElapse;
   unsigned short wCycles,wStopCycles;                              
   LSaveStateList SaveStateList;
   u32 dwDispCap,dwDispSwap;
   u32 (LDS::*pfnLoop)();
   HANDLE hThread,hEvents[2];
   u8 skip,skip_old;
#ifdef _DEBPRO
   LFile *pLog;
#endif
};

extern HINSTANCE hInst;
extern LDS ds;
#ifdef _DEBUG
extern LDebugDlg debugDlg;
extern LToolsDlg floatDlg;
#endif

#endif
