#include "ideastypes.h"
#include "plugin.h"

//---------------------------------------------------------------------------
#ifndef __dldipluginH__
#define __dldipluginH__
//---------------------------------------------------------------------------
class DLDIPlugIn : public PlugIn
{
public:
	DLDIPlugIn();
   virtual ~DLDIPlugIn(){};
   BOOL InitSetPlugInInfo(LPSETPLUGININFO p,DWORD dwState,DWORD dwStateMask);
	virtual inline DWORD Run(DWORD adr,LPDWORD data,BYTE accessMode){return ((LPDLDIPLUGINRUN)pRunFunc)(index,adr,data,accessMode);};
protected:
	DLDIPARAM sp;
};
//---------------------------------------------------------------------------
class DLDIPlugList : public PlugInList
{
public:
   DLDIPlugList();
   DLDIPlugIn *BuildPlugIn(char *path);
   BOOL PreLoad(WORD *wID);
   BOOL OnEnablePlug(WORD wID);   
};
//---------------------------------------------------------------------------
class R4PlugIn : public DLDIPlugIn
{
public:
	R4PlugIn();
   virtual ~R4PlugIn(){};
   BOOL SetProperty(LPSETPROPPLUGIN p){return FALSE;};
	DWORD Run(DWORD adr,LPDWORD data,BYTE accessMode);
};
//---------------------------------------------------------------------------
class NJPlugIn : public DLDIPlugIn
{
public:
	NJPlugIn();
   virtual ~NJPlugIn(){};
   BOOL SetProperty(LPSETPROPPLUGIN p){return FALSE;};
	DWORD Run(DWORD adr,LPDWORD data,BYTE accessMode);
protected:
   u8 command;
};
//---------------------------------------------------------------------------
class AK2PlugIn : public DLDIPlugIn
{
public:
	AK2PlugIn();
   virtual ~AK2PlugIn(){};
   BOOL SetProperty(LPSETPROPPLUGIN p){return FALSE;};
	DWORD Run(DWORD adr,LPDWORD data,BYTE accessMode);
protected:
   u8 command;
};
#endif


