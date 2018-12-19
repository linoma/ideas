#include "videoplug.h"
#include "resource.h"
#include "dsmem.h"
#include "gpu.h"
#include "io.h"
#include "lds.h"
#include "lregkey.h"

extern void nullIOFunc(u32,u32,u8);
static PlugIn *pFirstHookPlugIn;
//---------------------------------------------------------------------------
static void VideoInputFunc(u32 adr,u32 data,u8 am)
{
   EXEC_INPUTHOOK
}
//---------------------------------------------------------------------------
static u32 VideoOutputFunc(u32 adr,u8 am)
{
   EXEC_OUTPUTHOOK
}
//---------------------------------------------------------------------------
static void I_STDCALL WriteTable(u32 adr,void *I,void *O)
{
   if((adr & 0x0FF00000) != 0x04000000)
       return;
   adr &= 0xFFF;
   if(adr < 0x60 || adr > 0x6F0 || (adr < 0x320 && adr > 0x61))
       return;
   if(I != NULL)
       i_func[adr] = (LPIFUNC)I;
   o_func[adr] = (LPOFUNC)O;
}
//---------------------------------------------------------------------------
static void I_STDCALL OnGXFIFOIRQ(void)
{
   ds.set_IRQ(0x200000,FALSE,1);
}
//---------------------------------------------------------------------------
VideoPlug::VideoPlug() : PlugIn()
{
   dwType = PIT_VIDEO;
   v3d.video_mem = video_mem;
   v3d.io_mem = io_mem;
   v3d.lpWnd = &downLcd;
   v3d.pfn_writetable = ::WriteTable;
   v3d.pfn_onirq = OnGXFIFOIRQ;
}
//---------------------------------------------------------------------------
u32 VideoPlug::CallOutputFunc(u32 adr,u8 am)
{
   int index;
   
   if(adr == 0x60 || adr == 0x61)
       index = adr - 0x60;
   else if(adr > 0x31F && adr < 0x6F0)
       index = adr - 0x31E;
   else
       return 0;
   if(pOutputFunc[index] != NULL)
       return pOutputFunc[index](adr,am);
   return 0;
}
//---------------------------------------------------------------------------
void VideoPlug::CallInputFunc(u32 adr,u32 data,u8 am)
{
   int index;

   if(adr == 0x60 || adr == 0x61)
       index = adr - 0x60;
   else if(adr > 0x31F && adr < 0x6F0)
       index = adr - 0x31E;
   else
       return;
   pInputFunc[index](adr,data,am);
}
//---------------------------------------------------------------------------
void VideoPlug::EnableHook(BOOL bFlag)
{
   int i;

   if(bFlag){
       if(pInputFunc == NULL){
           if((pInputFunc = (LPIFUNC *)LocalAlloc(LPTR,((0x6F0 - 0x320)+2) * sizeof(LPIFUNC))) == NULL)
               return;
       }
       if(pOutputFunc == NULL){
           if((pOutputFunc = (LPOFUNC *)LocalAlloc(LPTR,((0x6F0 - 0x320)+2) * sizeof(LPOFUNC))) == NULL)
               return;
       }
       if(pFirstHookPlugIn == NULL)
           pFirstHookPlugIn = this;
       pInputFunc[0] = i_func[0x60];
       pInputFunc[1] = i_func[0x61];
       for(i=0x320;i<0x6F0;i++)
           pInputFunc[(i-0x320)+2] = i_func[i];
       pOutputFunc[0] = o_func[0x60];
       pOutputFunc[1] = o_func[0x61];
       for(i=0x320;i<0x6F0;i++)
           pOutputFunc[(i-0x320)+2] = o_func[i];
       for(i=0x320;i<0x6F0;i++){
           if(i_func[i] != nullIOFunc)
               i_func[i] = VideoInputFunc;
           o_func[i] = VideoOutputFunc;
       }
       for(i=0x60;i<0x62;i++){
           if(i_func[i] != nullIOFunc)
               i_func[i] = VideoInputFunc;
           o_func[i] = VideoOutputFunc;
   	}
	}
   else{
       if(pInputFunc != NULL){
           i_func[0x60] = pInputFunc[0];
           i_func[0x61] = pInputFunc[1];
           for(i=0x320;i<0x6F0;i++)
               i_func[i] = pInputFunc[(i-0x320)+2];
           LocalFree(pInputFunc);
           pInputFunc = NULL;
       }
       if(pOutputFunc != NULL){
           o_func[0x60] = pOutputFunc[0];
           o_func[0x61] = pOutputFunc[1];
           for(i=0x320;i<0x6F0;i++)
               o_func[i] = pOutputFunc[(i-0x320)+2];
           LocalFree(pOutputFunc);
           pOutputFunc = NULL;
       }
   }
}
//---------------------------------------------------------------------------
int VideoPlug::ResetTable()
{
   int i;

   if(IsAttribute(PIT_IS3D)){
       for(i=0x320;i<0x6F0;i++){
           i_func[i] = nullIOFunc;
           o_func[i] = NULL;
       }
       for(i=0x60;i<0x62;i++){
           i_func[i] = nullIOFunc;
           o_func[i] = NULL;
       }
   }
   else{
       for(i=0;i<0x60;i++){
           i_func[i] = nullIOFunc;
           o_func[i] = NULL;
           i_func[0x1000+i] = nullIOFunc;
           o_func[0x1000+i] = NULL;
       }
   }
   return S_OK;
}
//---------------------------------------------------------------------------
int VideoPlug::WriteTable(DWORD adr,LPVOID I,LPVOID O)
{
   if((adr & 0x0FF00000) != 0x04000000)
       return E_FAIL;
   if(ds.LMMU::get_EmuMode() == 0){
       if(IsAttribute(PIT_IS3D)){
           adr &= 0xFFF;
           if(adr < 0x60 || adr > 0x6F0 || (adr < 0x320 && adr > 0x61))
               return E_FAIL;
       }
       else{
           adr &= 0xFFFF;
           if(adr > 0x319 && adr < 0x6F0 || adr > 0x1100 || (adr > 0x5F && adr < 0x62))
               return E_FAIL;
       }
       if(I != NULL)
           i_func[adr] = (LPIFUNC)I;
       o_func[adr] = (LPOFUNC)O;
   }
   else{
   	if(IsAttribute(PIT_IS3D))
       	return E_FAIL;
       adr &= 0xFFFF;
       if(adr > 0x5F)
       	return E_FAIL;
       if(I != NULL)
           i_func7[adr] = (LPIFUNC)I;
       o_func7[adr] = (LPOFUNC)O;
   }
	return S_OK;
}
//---------------------------------------------------------------------------
int VideoPlug::TriggerIRQ()
{
	if(IsAttribute(PIT_IS3D)){
       ds.set_IRQ(0x200000,FALSE,1);
       return S_OK;
   }
   return E_FAIL;
}
//---------------------------------------------------------------------------
BOOL VideoPlug::InitSetPlugInInfo(LPSETPLUGININFO p,DWORD dwState,DWORD dwStateMask)
{
   if(!PlugIn::InitSetPlugInInfo(p,dwState,dwStateMask))
       return FALSE;
   if(IsAttribute(PIT_IS3D))
   	p->lParam = (LPARAM)&v3d;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL VideoPlug::OnEnableComplex()
{
	return FALSE;
}
//---------------------------------------------------------------------------
BOOL VideoPlug::NotifyState(DWORD dwState,DWORD dwStateMask,LPARAM lParam)
{
	return PlugIn::NotifyState(dwState,dwStateMask,lParam);
}
//---------------------------------------------------------------------------
VideoPlugList::VideoPlugList() : PlugInList("Render3D")
{
   pFirstHookPlugIn = NULL;
   p2DActivePlugIn = NULL;
}
//---------------------------------------------------------------------------
VideoPlugList::~VideoPlugList()
{
   LRegKey reg;
   GUID guid={0};
   GETPLUGININFO pi;

	if(p2DActivePlugIn != NULL){
       ZeroMemory(&pi,sizeof(GETPLUGININFO));
       pi.cbSize = sizeof(GETPLUGININFO);
       if(p2DActivePlugIn->GetInfo(&pi))
       	memcpy(&guid,&pi.guidID,sizeof(GUID));
   }
   reg.Open("Software\\iDeaS");
   reg.WriteBinaryData("Render2D",(char *)&guid,sizeof(GUID));
   reg.Close();
}
//---------------------------------------------------------------------------
VideoPlug *VideoPlugList::BuildPlugIn(char *path)
{
   VideoPlug *p;

   if(path == NULL || (p = new VideoPlug()) == NULL)
       return NULL;
   p->SetLibraryPath(path);
   return p;
}
//---------------------------------------------------------------------------
BOOL VideoPlugList::PreLoad(WORD *wID)
{
   VideoPlug *pVideoPlug;
   Render2DPlugIn *p;

   *wID = ID_PLUGIN_GRAPHICS_START;
   pVideoPlug = new OpenGLPlugIn();
   if(pVideoPlug != NULL){
       pVideoPlug->SetMenuID(*wID);
   	(*wID)++;
		if(!Add(pVideoPlug)){
           (*wID)--;
           delete pVideoPlug;
       }
   }
/*   pVideoPlug = new D3DPlugIn();
   if(pVideoPlug != NULL){
       pVideoPlug->SetMenuID(*wID);
   	(*wID)++;
		if(!Add(pVideoPlug)){
           (*wID)--;
           delete pVideoPlug;
       }
   }*/

   p = new Render2DPlugIn();
   if(p != NULL){
       p->SetMenuID(*wID);
   	(*wID)++;
		if(!Add(p)){
           (*wID)--;
           delete p;
       }
   }
	return TRUE;
}
//---------------------------------------------------------------------------
PlugIn *VideoPlugList::get_ActivePlugIn(DWORD type)
{
   if(type & PIT_IS3D)
       return pActivePlugIn;
   return p2DActivePlugIn;
}
//---------------------------------------------------------------------------
PlugIn *VideoPlugList::SetActivePlugIn(PlugIn *ele,DWORD dwType)
{
   PlugIn *p;

	p = pActivePlugIn;
	PlugInList::SetActivePlugIn(ele);
   if((ele != NULL && ele->IsAttribute(PIT_IS3D)) || (ele == NULL && (dwType & PIT_IS3D)))
	   ds.set_ActiveVideoPlugIn((VideoPlug *)pActivePlugIn);
   else if((ele != NULL && !ele->IsAttribute(PIT_IS3D)) || (ele == NULL && !(dwType & PIT_IS3D))){
       p = p2DActivePlugIn;
       p2DActivePlugIn = pActivePlugIn;
       ds.set_Active2DPlugIn((Render2DPlugIn *)p2DActivePlugIn);
		pActivePlugIn = ds.get_ActiveVideoPlugIn();
   }
   return p;
}
//---------------------------------------------------------------------------
void VideoPlugList::LoadSetConfig()
{
   LRegKey reg;
   GUID guid;

	PlugInList::LoadSetConfig();
   reg.Open("Software\\iDeaS");
   reg.ReadBinaryData("Render2D",(char *)&guid,sizeof(GUID));
   Enable(&guid,TRUE);
   reg.Close();
}
//---------------------------------------------------------------------------
BOOL VideoPlugList::CheckTypePlugIn(DWORD dwPlugInType,DWORD Type)
{
	if(!PlugInList::CheckTypePlugIn(dwPlugInType,Type))
   	return FALSE;
   if(Type == 0)
   	return TRUE;
	return (BOOL)((dwPlugInType & PIT_IS3D) == (Type & PIT_IS3D));
}
//---------------------------------------------------------------------------
BOOL VideoPlugList::CheckIsComplex(PlugIn *p)
{
	return p != NULL;
}
//---------------------------------------------------------------------------
WebCamPlugList::WebCamPlugList() : PlugInList("WebCam")
{
}
//---------------------------------------------------------------------------
WebCamPlugList::~WebCamPlugList()
{
}
//---------------------------------------------------------------------------
WebCamPlugIn *WebCamPlugList::BuildPlugIn(char *path)
{
   WebCamPlugIn *p;

   if(path == NULL || (p = new WebCamPlugIn()) == NULL)
       return NULL;
   p->SetLibraryPath(path);
   return p;
}
//---------------------------------------------------------------------------
BOOL WebCamPlugList::PreLoad(WORD *wID)
{
	return TRUE;
}
//---------------------------------------------------------------------------
void WebCamPlugList::LoadSetConfig()
{
}
//---------------------------------------------------------------------------
BOOL WebCamPlugList::CheckTypePlugIn(DWORD dwPlugInType,DWORD Type)
{
	if(!PlugInList::CheckTypePlugIn(dwPlugInType,Type))
   		return FALSE;
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL WebCamPlugList::CheckIsComplex(PlugIn *p)
{
	return FALSE;
}
//---------------------------------------------------------------------------
WebCamPlugIn::WebCamPlugIn() : PlugIn()
{
}
//---------------------------------------------------------------------------
WebCamPlugIn::~WebCamPlugIn()
{
}

