#include "ideastypes.h"
#include "dstype.h"
#include "lstring.h"
#include "llist.h"
#include "plugin/pluginmain.h"

//---------------------------------------------------------------------------
#ifndef __pluginH__
#define __pluginH__
//-----------------------------------------------------------------------
class PlugIn : public IPlugInInterface
{
public:
   PlugIn();
   virtual ~PlugIn();
   virtual BOOL Load();
   void Unload(BOOL bForced = FALSE);
   virtual BOOL Reset();
   BOOL CanRun(){if(pRunFunc == NULL || (dwFlags & PIT_NORUN) != 0) return FALSE;return TRUE;};
   virtual BOOL InitSetPlugInInfo(LPSETPLUGININFO p,DWORD dwState = 0,DWORD dwStateMask = 0);
   BOOL InitGetPlugInInfo(LPGETPLUGININFO p);
   virtual int GetInfo(LPGETPLUGININFO p,BOOL bReloadAll = FALSE);
   virtual DWORD SetInfo(LPSETPLUGININFO p);
   virtual BOOL NotifyState(DWORD dwState,DWORD dwStateMask = 0,LPARAM lParam = (LPARAM)-1);
   virtual inline BOOL Destroy(){if(pDeleteFunc != NULL) return pDeleteFunc();return TRUE;};
   virtual BOOL SetProperty(LPSETPROPPLUGIN p);
	virtual inline BOOL SaveState(LStream *p){if(pSaveStateFunc != NULL) return pSaveStateFunc(p); return TRUE;};
	virtual inline BOOL LoadState(LStream *p,int ver){if(pLoadStateFunc != NULL) return pLoadStateFunc(p,ver); return TRUE;};
   inline BOOL IsEnable(){return bEnable;};
   inline BOOL IsLicensed(){return isLicensed;};
   inline BOOL IsExclusive(){return bExclusive;};
   inline BOOL IsComplex(){return isComplex;};
   virtual BOOL OnEnableComplex(){return TRUE;};
   inline void SetComplex(BOOL value){isComplex = (char)value;};
   inline FARPROC GetRunFunc(){return pRunFunc;};
   inline void SetRunFunc(FARPROC p){pRunFunc = p;};
   inline LPPLUGINGETINFO GetGetInfoFunc(){return pGetInfoFunc;};
   virtual int GetFileInfo(char *pOut);
   virtual BOOL Enable(BOOL bFlag);
   inline void SetLibraryPath(char *path){pathLibrary = path;};
   inline void GetGuid(LPGUID p){if(p != NULL) CopyMemory(p,&guid,sizeof(GUID));};
   inline void SetGuid(LPGUID p){if(p != NULL) CopyMemory(&guid,p,sizeof(GUID));};
   inline void SetIndex(char val){index = val;};
   inline WORD GetIndex(){return index;};
   inline void SetUsed(){if(isLoad == 1) isLoad = 2;};
   inline DWORD GetType(DWORD mask = 0xF){return (DWORD)(dwType & mask);};
   void SetType(DWORD value);
   inline DWORD IsAttribute(DWORD value){return (dwFlags & value);};
   inline void OrAttribute(DWORD value){dwFlags |= value;};
   inline WORD GetMenuID(){return wID;};
   inline void SetMenuID(WORD id){wID = id;};
   virtual int ResetTable(){return 0;};
   virtual void EnableHook(BOOL bFlag){};
   void SetNode(PlugIn *last = (PlugIn *)0xFFFFFFFF,PlugIn *next = (PlugIn *)0xFFFFFFFF);
   void GetNode(PlugIn **last=NULL,PlugIn **next=NULL);
   inline void set_NextHook(PlugIn *p){pNextHookPlugIn = p;};
   inline PlugIn *get_NextHook(){return pNextHookPlugIn;};
   virtual void CallInputFunc(u32 adr,u32 data,u8 am){};
   virtual u32 CallOutputFunc(u32 adr,u8 am){return 0;};
   virtual u32 Run(LPRUNPARAM param);
   virtual int get_Format(DWORD,LPVOID,LPDWORD);
   virtual int WriteTable(DWORD,LPVOID,LPVOID);
   virtual int TriggerIRQ();
   virtual int get_Object(int,LPVOID *);
protected:
   char bEnable,isLoad,bDynamic,bExclusive,isReload,isLicensed,index,isComplex;
   WORD wID;
   DWORD dwType,dwFlags,dwThreadId;
   GUID guid;
   LString pathLibrary,name;
   FARPROC pRunFunc;
   LPPLUGINRESET pResetFunc;
   LPPLUGINDELETE pDeleteFunc;
   LPPLUGINGETINFO pGetInfoFunc;
   LPPLUGINSETINFO pSetInfoFunc;
   LPPLUGINSETPROPERTY pSetPropertyFunc;
   LPPLUGINSAVESTATE pSaveStateFunc;
   LPPLUGINLOADSTATE pLoadStateFunc;
   HINSTANCE hLib;
   LPIFUNC *pInputFunc;
   LPOFUNC *pOutputFunc;
   PlugIn *pNextHookPlugIn,*pNext,*pLast;
};
//---------------------------------------------------------------------------
class PlugInList : public LList
{
public:
   PlugInList(char *p);
   virtual ~PlugInList();
   virtual PlugIn *GetItemFromGUID(LPGUID p);
   virtual int Reset();
   virtual BOOL PreLoad(WORD *wID);
   virtual PlugIn *BuildPlugIn(char *path);
   virtual void Clear();
   int Load(WORD wID,DWORD Type);
   virtual int OnInitMenu(HMENU menu,BOOL bDelete = TRUE,int startPos = 0);
   virtual int NotifyState(DWORD dwState,DWORD dwStateMask = 0,LPARAM lParam = (LPARAM)-1);
   virtual BOOL Run(LPRUNPARAM param,DWORD dwAttribute=(DWORD)-1);
   virtual BOOL Add(PlugIn *ele);
   BOOL GetSelectedGUID(LPGUID p);
   virtual void LoadSetConfig();
   virtual void SaveSetConfig();
   virtual BOOL CheckTypePlugIn(DWORD dwPlugInType,DWORD Type);
   virtual BOOL CheckIsComplex(PlugIn *p);
   virtual BOOL Enable(LPGUID p,BOOL bEnable);
   virtual BOOL Enable(DWORD item,BOOL bEnable);
   virtual BOOL OnEnablePlug(WORD wID);
   virtual inline PlugIn *get_ActivePlugIn(DWORD type = 0){return pActivePlugIn;};
   virtual PlugIn *SetActivePlugIn(PlugIn *ele,DWORD type = 0);
   virtual BOOL SaveState(LStream *p);
   virtual BOOL LoadState(LStream *p,int ver);
   inline PlugInList* GetNext(){return next;};
protected:
   virtual void DeleteElem(LPVOID ele);
   LString name;
   PlugIn *pActivePlugIn;
   PlugInList *next;
   LPVOID container;
   DWORD dwType;
};

#define EXEC_INPUTHOOK \
   PlugIn *p;\
   u32 I_STDCALL (*pfn)(u32,u32,u32 *,u8);\
   u32 bCall,res;\
   \
   p = pFirstHookPlugIn;\
   bCall = 1;\
   while(p != NULL){\
       if(p->IsEnable() && p->IsAttribute(PIT_HOOKBEFORE) && p->CanRun()){\
           pfn = (u32 I_STDCALL (*)(u32,u32,u32*,u8))p->GetRunFunc();\
           res = pfn(1,adr,&data,am);\
           if(bCall && HIWORD(res))\
               bCall = 0;\
           if(!LOWORD(res))\
               break;\
       }\
       p = p->get_NextHook();\
   }\
   if(bCall)\
       p->CallInputFunc(adr,data,am);\
   p = pFirstHookPlugIn;\
   while(p != NULL){\
       if(p->IsEnable() && p->IsAttribute(PIT_HOOKAFTER) && p->CanRun()){\
           pfn = (u32 I_STDCALL (*)(u32,u32,u32*,u8))p->GetRunFunc();\
           if(!LOWORD(pfn(1,adr,&data,am)))\
               break;\
       }\
       p = p->get_NextHook();\
   }


#define EXEC_OUTPUTHOOK\
   PlugIn *p;\
   u32 I_STDCALL (*pfn)(u32,u32,u32*,u8);\
   u32 value,res,bCall;\
\
   p = pFirstHookPlugIn;\
   bCall = 1;\
   while(p != NULL){\
       if(p->IsEnable() && p->IsAttribute(PIT_HOOKBEFORE) && p->CanRun()){\
           pfn = (u32 I_STDCALL (*)(u32,u32,u32*,u8))p->GetRunFunc();\
           res = pfn(0x10001,adr,0,am);\
           if(bCall && HIWORD(res))\
               bCall = 0;\
           if(!LOWORD(res))\
               break;\
       }\
       p = p->get_NextHook();\
   }\
   if(bCall)\
       value = p->CallOutputFunc(adr,am);\
   p = pFirstHookPlugIn;\
   while(p != NULL){\
       if(p->IsEnable() && p->IsAttribute(PIT_HOOKAFTER) && p->CanRun()){\
           pfn = (u32 I_STDCALL (*)(u32,u32,u32*,u8))p->GetRunFunc();\
           if(!LOWORD(pfn(0x10001,adr,&value,am)))\
               break;\
       }\
       p = p->get_NextHook();\
   }\
   return value;

#endif


