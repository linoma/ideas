#include "ideastypes.h"
#include "plugin.h"

//---------------------------------------------------------------------------
#ifndef __videoplugH__
#define __videoplugH__
//---------------------------------------------------------------------------
class VideoPlug : public PlugIn
{
public:
   VideoPlug();
   virtual ~VideoPlug(){};
   BOOL InitSetPlugInInfo(LPSETPLUGININFO p,DWORD dwState,DWORD dwStateMask);
   virtual inline unsigned long I_FASTCALL Run(unsigned long x,unsigned long y){
   	if(!isComplex){
#ifdef __BORLANDC__
   		_ECX = x;
#endif
       	return ((unsigned long I_FASTCALL (*)(unsigned long,unsigned long))pRunFunc)(x,y);
       }
       else{
       	RUNPARAM param;

           param.type = dwType;
   		param.index = index;
           param.render.x = x;
           param.render.y = y;
   		return ((LPDEFAULTPLUGINRUN)pRunFunc)(&param);
       }
   };
   int ResetTable();
   void EnableHook(BOOL bFlag);
   u32 CallOutputFunc(u32 adr,u8 am);
   void CallInputFunc(u32 adr,u32 data,u8 am);
   int TriggerIRQ();
   int WriteTable(DWORD,LPVOID,LPVOID);
   virtual BOOL OnEnableComplex();
   BOOL NotifyState(DWORD dwState,DWORD dwStateMask = 0,LPARAM lParam = (LPARAM)-1);   
protected:
	VIDEO3DPARAM v3d;
};
//---------------------------------------------------------------------------
class VideoPlugList : public PlugInList
{
public:
   VideoPlugList();
   virtual ~VideoPlugList();
   VideoPlug *BuildPlugIn(char *path);
   BOOL PreLoad(WORD *wID);
   virtual void LoadSetConfig();
   virtual BOOL CheckTypePlugIn(DWORD dwPlugInType,DWORD Type);
   virtual PlugIn *SetActivePlugIn(PlugIn *ele,DWORD type = 0);
   inline PlugIn *get_Active2DPlugin(){return p2DActivePlugIn;};
   PlugIn *get_ActivePlugIn(DWORD type = 0);
   BOOL CheckIsComplex(PlugIn *p);
protected:
	PlugIn *p2DActivePlugIn;
};
//---------------------------------------------------------------------------
class Render2DPlugIn : public VideoPlug
{
public:
	Render2DPlugIn();
   ~Render2DPlugIn();
	u32 Run(LPRUNPARAM param);
   BOOL Enable(BOOL bFlag);
   BOOL Reset();
   BOOL Destroy();
	BOOL SaveState(LStream *p);
	BOOL LoadState(LStream *p,int ver);
//   BOOL NotifyState(DWORD dwState,DWORD dwStateMask = 0,LPARAM lParam = (LPARAM)-1);
	DWORD SetInfo(LPSETPLUGININFO p);
protected:
   static DWORD WINAPI ThreadFunc_01(LPVOID arg);
   DWORD OnThread_01();
   BOOL EnableMultiCores(BOOL bFlag);
   
   HANDLE hThread,hEvents[2];
   int nCores,nDraw_Req;
   BOOL bQuit;
   unsigned short *video_status;
};
//---------------------------------------------------------------------------
class OpenGLPlugIn : public VideoPlug
{
public:
	OpenGLPlugIn();
   ~OpenGLPlugIn();
	unsigned long I_FASTCALL Run(unsigned long x,unsigned long y);
   BOOL Enable(BOOL bFlag);
   BOOL Reset();
   BOOL Destroy();
   BOOL SetProperty(LPSETPROPPLUGIN p);
	BOOL SaveState(LStream *p);
	BOOL LoadState(LStream *p,int ver);
	DWORD SetInfo(LPSETPLUGININFO p);
//   BOOL NotifyState(DWORD dwState,DWORD dwStateMask = 0,LPARAM lParam = (LPARAM)-1);
protected:
	static BOOL CALLBACK DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
   void OnChangeConfig();
};
//---------------------------------------------------------------------------
class D3DPlugIn : public VideoPlug
{
public:
	D3DPlugIn();
   ~D3DPlugIn();
	inline unsigned long I_FASTCALL Run(unsigned long x,unsigned long y);
   BOOL Enable(BOOL bFlag);
   BOOL Reset();
   BOOL Destroy();
   BOOL SetProperty(LPSETPROPPLUGIN p);
	BOOL SaveState(LStream *p);
	BOOL LoadState(LStream *p);
   BOOL NotifyState(DWORD dwState,DWORD dwStateMask = 0,LPARAM lParam = (LPARAM)-1);
protected:
	static BOOL CALLBACK DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};
//---------------------------------------------------------------------------
class WebCamPlugIn : public PlugIn
{
public:
	WebCamPlugIn();
   virtual ~WebCamPlugIn();
protected:
};
//---------------------------------------------------------------------------
class WebCamPlugList : public PlugInList
{
public:
   WebCamPlugList();
   virtual ~WebCamPlugList();
   WebCamPlugIn *BuildPlugIn(char *path);
   BOOL PreLoad(WORD *wID);
   virtual void LoadSetConfig();
   virtual BOOL CheckTypePlugIn(DWORD dwPlugInType,DWORD Type);
   BOOL CheckIsComplex(PlugIn *p);
protected:
};

#endif
