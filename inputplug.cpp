#include "inputplug.h"
#include "lds.h"

#ifdef __DLL__
#include "dinput.h"
#endif

//---------------------------------------------------------------------------
LInputPlug::LInputPlug() : PlugIn()
{
	dwType = PIT_PAD;
}
//---------------------------------------------------------------------------
LInputPlug::~LInputPlug()
{
}
//---------------------------------------------------------------------------
BOOL LInputPlug::InitSetPlugInInfo(LPSETPLUGININFO p,DWORD dwState,DWORD dwStateMask)
{
   if(!PlugIn::InitSetPlugInInfo(p,dwState,dwStateMask))
       return FALSE;
	p->lParam = (LPARAM)(IWnd *)&downLcd;
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LInputPlug::OnEnableComplex()
{
   return FALSE;
}
//---------------------------------------------------------------------------
u32 LInputPlug::Run(u32 *value)
{
   RUNPARAM rp;

   if(!isComplex)
       return ((u32 I_STDCALL (*)(u32 *))pRunFunc)(value);
   rp.pad.value = value;
   return PlugIn::Run(&rp);
}
//---------------------------------------------------------------------------
LInputPlugList::LInputPlugList() : PlugInList("Input")
{
}
//---------------------------------------------------------------------------
LInputPlugList::~LInputPlugList()
{
}
//---------------------------------------------------------------------------
LInputPlug *LInputPlugList::BuildPlugIn(char *path)
{
   LInputPlug *p;

   if(path == NULL || (p = new LInputPlug()) == NULL)
       return NULL;
   p->SetLibraryPath(path);
   return p;
}
//---------------------------------------------------------------------------
BOOL LInputPlugList::CheckIsComplex(PlugIn *p)
{
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LInputPlugList::PreLoad(WORD *wID)
{
/*   DIInputPlug *pPlug;

   *wID = ID_PLUGIN_INPUT_START;
   pPlug = new DIInputPlug();
   if(pPlug != NULL){
       pPlug->SetMenuID(*wID);
   	(*wID)++;
		if(!Add(pPlug)){
           (*wID)--;
           delete pPlug;
       }
   }*/
	return TRUE;
}

#if defined __DLL__
//---------------------------------------------------------------------------
static u32 I_STDCALL SetInfoFunc(LPSETPLUGININFO p)
{
   if(p == NULL)
	   return FALSE;
   if(PLUGINISENABLE(p))
   	   return DIInputPlugIn.Init(p);
    if(p->dwStateMask & PIS_NOTIFYMASK)
    	DIInputPlugIn.NotifyState((p->dwState & PIS_NOTIFYMASK),p->lParam);
   return TRUE;
}
//---------------------------------------------------------------------------
DIInputPlug::DIInputPlug() : LInputPlug()
{
   guid = DINPUT_GUID;
   name = "Gamepad PlugIn";
	dwType = PIT_PAD;
   dwFlags = PIT_DYNAMIC;
   pSetInfoFunc = SetInfoFunc;
}
//---------------------------------------------------------------------------
DIInputPlug::~DIInputPlug()
{
}
//---------------------------------------------------------------------------
BOOL DIInputPlug::Enable(BOOL bFlag)
{
   if(!PlugIn::Enable(bFlag))
   	return FALSE;
   if(!bFlag)
       return TRUE;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL DIInputPlug::Reset()
{
	return DIInputPlugIn.Reset();
}
//---------------------------------------------------------------------------
BOOL DIInputPlug::Destroy()
{
	return DIInputPlugIn.Destroy();
}
//---------------------------------------------------------------------------
BOOL DIInputPlug::SetProperty(LPSETPROPPLUGIN p)
{
	return DIInputPlugIn.SetProperty(p);
}
//---------------------------------------------------------------------------
u32 DIInputPlug::Run(u32 *value)
{
	return DIInputPlugIn.Run(value);
}
//---------------------------------------------------------------------------
BOOL DIInputPlug::NotifyState(DWORD dwState,DWORD dwStateMask,LPARAM lParam)
{
	DIInputPlugIn.NotifyState((dwState & PIS_NOTIFYMASK),lParam);
	return TRUE;
}
#endif

