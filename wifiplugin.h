#include "ideastypes.h"
#include "plugin.h"

//---------------------------------------------------------------------------
#ifndef __wifipluginH__
#define __wifipluginH__
//---------------------------------------------------------------------------
class LWFPlugIn : public PlugIn
{
public:
   LWFPlugIn();
   virtual ~LWFPlugIn();
   BOOL InitSetPlugInInfo(LPSETPLUGININFO p,DWORD dwState,DWORD dwStateMask);
   int ResetTable();
   int TriggerIRQ();
   int WriteTable(DWORD,LPVOID,LPVOID);
protected:
   WIFIPARAM wf;
};
//---------------------------------------------------------------------------
class LWFPlugList : public PlugInList
{
public:
   LWFPlugList();
   LWFPlugIn *BuildPlugIn(char *path);
   BOOL PreLoad(WORD *wID);
};
//---------------------------------------------------------------------------
class WiFiPlugIn : public LWFPlugIn
{
public:
	WiFiPlugIn();
   ~WiFiPlugIn();
//   BOOL Enable(BOOL bFlag);
   BOOL Reset();
   BOOL Destroy();
   BOOL SetProperty(LPSETPROPPLUGIN p);
};

#endif
