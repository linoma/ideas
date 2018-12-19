#include "ideastypes.h"
#include "plugin.h"

//---------------------------------------------------------------------------
#ifndef __audioplugH__
#define __audioplugH__
//---------------------------------------------------------------------------
class AudioPlug : public PlugIn
{
public:
   AudioPlug();
   virtual ~AudioPlug(){};
   BOOL InitSetPlugInInfo(LPSETPLUGININFO p,DWORD dwState,DWORD dwStateMask);
   virtual DWORD Run(BOOL bSyncro)
   {
   	if(!isComplex){
       	return ((DWORD I_FASTCALL (*)(BOOL))pRunFunc)(bSyncro);
       }
       else{
       	RUNPARAM param;

           param.type = dwType;
   		param.index = index;
           param.audio.syncro = (int)bSyncro;
   		return ((LPDEFAULTPLUGINRUN)pRunFunc)(&param);
       }
   };
   int ResetTable();
   void EnableHook(BOOL bFlag);
   u32 CallOutputFunc(u32 adr,u8 am);
   void CallInputFunc(u32 adr,u32 data,u8 am);
	virtual BOOL SaveState(LStream *p){return TRUE;};
	virtual BOOL LoadState(LStream *p,int ver){return TRUE;};
   int WriteTable(DWORD,LPVOID,LPVOID);
   virtual BOOL OnEnableComplex();
protected:
	SOUNDPARAM sp;
};
//---------------------------------------------------------------------------
class AudioPlugList : public PlugInList
{
public:
   AudioPlugList();
   virtual ~AudioPlugList();
   AudioPlug *BuildPlugIn(char *path);
   BOOL PreLoad(WORD *wID);
   virtual BOOL CheckTypePlugIn(DWORD dwPlugInType,DWORD Type);
   virtual PlugIn *SetActivePlugIn(PlugIn *ele,DWORD dwType=0);
   PlugIn *get_ActivePlugIn(DWORD type = 0);   
   BOOL CheckIsComplex(PlugIn *p);
   inline PlugIn *get_ActiveMicPlugin(){return pActiveMicPlugIn;};
   void LoadSetConfig();
protected:
	PlugIn *pActiveMicPlugIn;
};
//---------------------------------------------------------------------------
class DSAudioPlugIn : public AudioPlug
{
public:
	DSAudioPlugIn();
   virtual ~DSAudioPlugIn();
   BOOL Enable(BOOL bFlag);
   BOOL Reset();
   BOOL Destroy();
   BOOL SetProperty(LPSETPROPPLUGIN p);
   DWORD Run(BOOL bSyncro);
	BOOL SaveState(LStream *p);
	BOOL LoadState(LStream *p,int ver);
   BOOL NotifyState(DWORD dwState,DWORD dwStateMask = 0,LPARAM lParam = (LPARAM)-1);
   void Stop(unsigned long value);
protected:
	static BOOL CALLBACK DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};
//---------------------------------------------------------------------------
class DSMicAudioPlugin : public AudioPlug
{
public:
   DSMicAudioPlugin();
   virtual ~DSMicAudioPlugin();
   BOOL Enable(BOOL bFlag);
   BOOL Reset();
   BOOL Destroy();
   DWORD Run(BOOL bSyncro);
   BOOL NotifyState(DWORD dwState,DWORD dwStateMask = 0,LPARAM lParam = (LPARAM)-1);
	BOOL SaveState(LStream *p);
	BOOL LoadState(LStream *p,int ver);
   BOOL SetProperty(LPSETPROPPLUGIN p);   
protected:
   static BOOL CALLBACK DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};

#endif

