#include "wifiplugin.h"
#include "lds.h"
#include "io.h"
#ifdef __USEWIFI
#include "wifi.h"
#include "guid.h"
#endif

extern void nullIOFunc(u32,u32,u8);
//---------------------------------------------------------------------------
static void I_STDCALL OnWifiIRQ(void)
{
   ds.set_IRQ((1<<24),FALSE,2);
}
//---------------------------------------------------------------------------
static void I_STDCALL WriteTable(u32 adr,void *I,void *O)
{
   if((adr & 0x0FF00000) != 0x04800000)
       return;
   adr &= 0x7FFF;
   if(I != NULL)
       i_func7[0x3000+adr] = (LPIFUNC)I;
   o_func7[0x3000+adr] = (LPOFUNC)O;
}
//---------------------------------------------------------------------------
LWFPlugIn::LWFPlugIn() : PlugIn()
{
   dwType = PIT_WIFI;
}
//---------------------------------------------------------------------------
LWFPlugIn::~LWFPlugIn()
{
}
//---------------------------------------------------------------------------
BOOL LWFPlugIn::InitSetPlugInInfo(LPSETPLUGININFO p,DWORD dwState,DWORD dwStateMask)
{
   if(!PlugIn::InitSetPlugInInfo(p,dwState,dwStateMask))
       return FALSE;
   p->lParam = (LPARAM)&wf;
   wf.io_mem = io_mem7+0x3000;
   wf.pfn_writetable = ::WriteTable;
   wf.pfn_onirq = OnWifiIRQ;
   return TRUE;
}
//---------------------------------------------------------------------------
int LWFPlugIn::WriteTable(DWORD adr,LPVOID I,LPVOID O)
{
   if((adr & 0x0FF00000) != 0x04800000)
       return E_FAIL;
   adr &= 0x7FFF;
   if(I != NULL)
       i_func7[0x3000+adr] = (LPIFUNC)I;
   o_func7[0x3000+adr] = (LPOFUNC)O;
   return S_OK;
}
//---------------------------------------------------------------------------
int LWFPlugIn::ResetTable()
{
   int i;

   for(i=0x3000;i<0xB000;i++){
       i_func7[i] = nullIOFunc;
       o_func7[i] = NULL;
   }
   return S_OK;
}
//---------------------------------------------------------------------------
int LWFPlugIn::TriggerIRQ()
{
	ds.set_IRQ((1<<24),FALSE,2);
   return S_OK;
}
//---------------------------------------------------------------------------
LWFPlugList::LWFPlugList() : PlugInList("WiFi")
{
}
//---------------------------------------------------------------------------
LWFPlugIn *LWFPlugList::BuildPlugIn(char *path)
{
   LWFPlugIn *p;

   if(path == NULL || (p = new LWFPlugIn()) == NULL)
       return NULL;
   p->SetLibraryPath(path);
   return p;
}
//---------------------------------------------------------------------------
BOOL LWFPlugList::PreLoad(WORD *wID)
{
#ifdef __USEWIFI
	LWFPlugIn *pWFPlug;

   *wID = ID_PLUGIN_WIFI_START;
   pWFPlug = new WiFiPlugIn();
   if(pWFPlug != NULL){
       pWFPlug->SetMenuID(*wID);
   	(*wID)++;
		if(!Add(pWFPlug)){
           (*wID)--;
           delete pWFPlug;
       }
   }
#endif
	return TRUE;
}
#ifdef __USEWIFI
//---------------------------------------------------------------------------
static u32 I_STDCALL SetInfoFunc(LPSETPLUGININFO p)
{
   if(p == NULL)
       return FALSE;
   if(p->dwStateMask & PIS_NOTIFYMASK)
       wifiPlugin->NotifyState((p->dwState & PIS_NOTIFYMASK),p->lParam);
   else if(p->dwStateMask & PIS_ENABLEMASK){
       if(PLUGINISENABLE(p))
           return wifiPlugin->Init(p);
   }
   return TRUE;
}
//---------------------------------------------------------------------------
WiFiPlugIn::WiFiPlugIn() : LWFPlugIn()
{
   guid = WFGUID;
   name = "WiFi PlugIn";
	dwType = PIT_WIFI;
   dwFlags = PIT_DYNAMIC;
   pSetInfoFunc = SetInfoFunc;
	wifiPlugin = new LWiFiPlugIn();
}
//---------------------------------------------------------------------------
WiFiPlugIn::~WiFiPlugIn()
{
}
//---------------------------------------------------------------------------
/*BOOL WiFiPlugIn::Enable(BOOL bFlag)
{
	pSetInfoFunc = SetInfoFunc;
   if(!PlugIn::Enable(bFlag))
   	return FALSE;
   if(!bFlag)
       return TRUE;
   return wifiPlugin->Init(&wf);
}*/
//---------------------------------------------------------------------------
BOOL WiFiPlugIn::Reset()
{
   wifiPlugin->Reset();
  	pSetInfoFunc = SetInfoFunc;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL WiFiPlugIn::Destroy()
{
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL WiFiPlugIn::SetProperty(LPSETPROPPLUGIN p)
{
   return wifiPlugin->SetProperty(p);
}
#endif

