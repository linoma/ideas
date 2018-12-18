#include "ideastypes.h"
#include "plugin.h"

//---------------------------------------------------------------------------
#ifndef __inputplugH__
#define __inputplugH__
//---------------------------------------------------------------------------
class LInputPlug : public PlugIn
{
public:
	LInputPlug();
   virtual ~LInputPlug();
   BOOL InitSetPlugInInfo(LPSETPLUGININFO p,DWORD dwState,DWORD dwStateMask);
   virtual u32 Run(u32 *value);
   BOOL OnEnableComplex();
};
//---------------------------------------------------------------------------
class LInputPlugList : public PlugInList
{
public:
	LInputPlugList();
   ~LInputPlugList();
   LInputPlug *BuildPlugIn(char *path);
   BOOL PreLoad(WORD *wID);
   BOOL CheckIsComplex(PlugIn *p);
};
//---------------------------------------------------------------------------
class DIInputPlug : public LInputPlug
{
public:
	DIInputPlug();
   virtual ~DIInputPlug();
   BOOL Enable(BOOL bFlag);
   BOOL Reset();
   BOOL Destroy();
   BOOL SetProperty(LPSETPROPPLUGIN p);
   u32 Run(u32 *value);
   BOOL NotifyState(DWORD dwState,DWORD dwStateMask = 0,LPARAM lParam = (LPARAM)-1);
};
#endif
