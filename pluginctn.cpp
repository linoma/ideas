#include "ideastypes.h"
#include "pluginctn.h"
#include "resource.h"
#include "lds.h"


PlugInContainer *pPlugInContainer = NULL;
//---------------------------------------------------------------------------
PlugInContainer::PlugInContainer()
{
    iLastError = -1;
    pVideoPlugList = NULL;
    pAudioPlugList = NULL;
    pGraphicsPlugList = NULL;
    pWifiPlugList = NULL;
    pInputPlugList = NULL;
    pDLDIPlugList = NULL;
    pWebCamPlugList = NULL;
}
//---------------------------------------------------------------------------
PlugInContainer::~PlugInContainer()
{
    Destroy();
}
//---------------------------------------------------------------------------
void PlugInContainer::NotifyState(DWORD item,DWORD dwState,DWORD dwStateMask,LPARAM lParam)
{
   if(pVideoPlugList != NULL && (item & PIL_VIDEO))
   	pVideoPlugList->NotifyState(dwState,dwStateMask,lParam);
   if(pAudioPlugList != NULL && (item & PIL_AUDIO))
       pAudioPlugList->NotifyState(dwState,dwStateMask,lParam);
   if(pGraphicsPlugList != NULL && (item & PIL_GRAPHICS))
       pGraphicsPlugList->NotifyState(dwState,dwStateMask,lParam);
   if(pWifiPlugList != NULL && (item & PIL_WIFI))
       pWifiPlugList->NotifyState(dwState,dwStateMask,lParam);
   if(pInputPlugList != NULL && (item & PIL_INPUT))
       pInputPlugList->NotifyState(dwState,dwStateMask,lParam);
   if(pWebCamPlugList != NULL && (item & PIL_WEBCAM))
   	pWebCamPlugList->NotifyState(dwState,dwStateMask,lParam);
   if(pDLDIPlugList != NULL && (item & PIL_DLDI))
       pDLDIPlugList->NotifyState(dwState,dwStateMask,lParam);
}
//---------------------------------------------------------------------------
BOOL PlugInContainer::Create(DWORD flags)
{
   iLastError = -6;
   if((pGraphicsPlugList = new GraphicsPlugList()) == NULL)
       return FALSE;
   iLastError = -13;
   if((pDLDIPlugList = new DLDIPlugList()) == NULL)
       return FALSE;
   iLastError = -9;
   if((pWifiPlugList = new LWFPlugList()) == NULL)
       return FALSE;
   iLastError = -2;
   if((pVideoPlugList = new VideoPlugList()) == NULL)
       return FALSE;
   iLastError = -4;
   if((pAudioPlugList = new AudioPlugList()) == NULL)
       return FALSE;
   iLastError = -11;
   if((pInputPlugList = new LInputPlugList()) == NULL)
       return FALSE;
   iLastError = -20;
   if((pWebCamPlugList = new WebCamPlugList()) == NULL)
   	return FALSE;
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL PlugInContainer::Init(DWORD flags)
{
	PlugIn *p;
   DWORD dwPos;
	int i;

	if(flags & PIL_GRAPHICS){
       if(!pGraphicsPlugList->Load(ID_PLUGIN_RENDER_START,PIT_GRAPHICS)){
           iLastError = -7;
           return FALSE;
       }
   }
   if(flags & PIL_DLDI){
       if(!pDLDIPlugList->Load(ID_PLUGIN_DLDI_START,PIT_DLDI)){
           iLastError = -14;
           return FALSE;
       }
   }
   if(flags & PIL_VIDEO){
       if(!pVideoPlugList->Load(ID_PLUGIN_GRAPHICS_START,PIT_VIDEO)){
           iLastError = -3;
           return FALSE;
       }
       iLastError = -8;
       if(pVideoPlugList->get_ActivePlugIn(PIT_IS3D) == NULL){
           p = (PlugIn *)pVideoPlugList->GetFirstItem(&dwPos);
           for(i=1;p != NULL;i++){
               if(p->IsAttribute(PIT_IS3D)){
               	if(pVideoPlugList->Enable(i,TRUE))
                       break;
               }
           	p = (PlugIn *)pAudioPlugList->GetNextItem(&dwPos);
           }
           if(p == NULL)
               return FALSE;
       }
//       if(pVideoPlugList->get_ActivePlugIn(PIT_IS3D) == NULL)
//           return FALSE;
       ds.set_ActiveVideoPlugIn((VideoPlug *)pVideoPlugList->get_ActivePlugIn(PIT_IS3D));
       iLastError = -15;
       if(pVideoPlugList->get_Active2DPlugin() == NULL){
           if(!pVideoPlugList->Enable(2,TRUE))
               return FALSE;
       }
   }
   if(flags & PIL_AUDIO){
       if(!pAudioPlugList->Load(ID_PLUGIN_AUDIO_START,PIT_AUDIO)){
           iLastError = -5;
           return FALSE;
       }
       if(pAudioPlugList->get_ActivePlugIn() == NULL){
			iLastError = -16;
           p = (PlugIn *)pAudioPlugList->GetFirstItem(&dwPos);
           for(i=1;p != NULL;i++){
               if(p->IsAttribute(PIT_NOMENU|PIT_ISMICROPHONE) == 0){
               	if(!pAudioPlugList->Enable(i,TRUE))
                   	return FALSE;
                   break;
               }
           	p = (PlugIn *)pAudioPlugList->GetNextItem(&dwPos);
           }
//           if(p == NULL)
//           	return FALSE;
       }
       ds.set_ActiveAudioPlugIn((AudioPlug *)pAudioPlugList->get_ActivePlugIn());
   }
   if(flags & PIL_WIFI){
       if(!pWifiPlugList->Load(ID_PLUGIN_WIFI_START,PIT_WIFI)){
           iLastError = -10;
           return FALSE;
       }
   }
   if(flags & PIL_INPUT){
   	if(!pInputPlugList->Load(ID_PLUGIN_INPUT_START,PIT_PAD)){
           iLastError = -12;
           return FALSE;
       }
	}
   if(flags & PIL_WEBCAM){
       if(!pWebCamPlugList->Load(ID_PLUGIN_WEBCAM_START,PIT_WEBCAM)){
       	iLastError = -21;
           return FALSE;
       }
   }
   iLastError = 0;
   return TRUE;
}
//---------------------------------------------------------------------------
void PlugInContainer::Destroy()
{
   if(pVideoPlugList != NULL){
       delete pVideoPlugList;
       pVideoPlugList = NULL;
   }
   if(pAudioPlugList != NULL){
       delete pAudioPlugList;
       pAudioPlugList = NULL;
   }
   if(pGraphicsPlugList != NULL){
       delete pGraphicsPlugList;
       pGraphicsPlugList = NULL;
   }
   if(pWifiPlugList != NULL){
       delete pWifiPlugList;
       pWifiPlugList = NULL;
   }
   if(pInputPlugList != NULL){
       delete pInputPlugList;
       pInputPlugList = NULL;
   }
   if(pDLDIPlugList != NULL){
       delete pDLDIPlugList;
       pDLDIPlugList = NULL;
   }
   if(pWebCamPlugList != NULL){
       delete pWebCamPlugList;
       pWebCamPlugList = NULL;
   }
}
//---------------------------------------------------------------------------
PlugInList *PlugInContainer::Add(PlugIn *p)
{
   PlugInList *list;

   if(p == NULL)
       return NULL;
   if((list = get_PlugInList((DWORD)(1 << p->GetType()))) == NULL)
       return NULL;
   if(!list->Add(p))
       return NULL;
   return list;
}
//---------------------------------------------------------------------------
int PlugInContainer::get_PlugInFormat(DWORD type,LPVOID object,LPDWORD dwSize)
{
   PlugIn *p;

   if(object == NULL || dwSize == NULL)
       return E_FAIL;
   switch(type){
       case PIT_AUDIO:
           if(pAudioPlugList != NULL)
               p = pAudioPlugList->get_ActivePlugIn();
       break;
		default:
			p = NULL;
		break;
   }
	if(p == NULL)
		return E_FAIL;
   return p->get_Format(0,object,dwSize);
}
//---------------------------------------------------------------------------
PlugIn *PlugInContainer::get_PlugInFromGUID(GUID *guid)
{
   PlugIn *p;

   if(guid == NULL)
       return NULL;
   if(pVideoPlugList != NULL && (p = pVideoPlugList->GetItemFromGUID(guid)) != NULL)
       return p;
   if(pAudioPlugList != NULL && (p = pAudioPlugList->GetItemFromGUID(guid)) != NULL)
       return p;
   if(pGraphicsPlugList != NULL && (p = pGraphicsPlugList->GetItemFromGUID(guid)) != NULL)
       return p;
   if(pWifiPlugList != NULL && (p = pWifiPlugList->GetItemFromGUID(guid)) != NULL)
       return p;
   if(pInputPlugList != NULL && (p = pInputPlugList->GetItemFromGUID(guid)) != NULL)
       return p;
   if(pDLDIPlugList != NULL && (p = pDLDIPlugList->GetItemFromGUID(guid)) != NULL)
       return p;
   if(pWebCamPlugList != NULL && (p = pWebCamPlugList->GetItemFromGUID(guid)) != NULL)
   	return p;
   return NULL;
}
//---------------------------------------------------------------------------
int PlugInContainer::get_PlugInInterface(GUID *guid,LPVOID *ppvObject)
{
   PlugIn *p;

   if(guid == NULL || ppvObject == NULL)
       return E_FAIL;
   *ppvObject = NULL;
   p = get_PlugInFromGUID(guid);
   if(p != NULL){
       *ppvObject = (LPVOID)p;
       return S_OK;
   }
   return E_FAIL;
}
//---------------------------------------------------------------------------
int PlugInContainer::get_ActivePlugIn(int pid,LPVOID *ppvObject)
{
	PlugIn *p;

	if(ppvObject == NULL)
   	return E_FAIL;
	*ppvObject = NULL;
   switch(pid){
   	case PIT_VIDEO:
       	if(pVideoPlugList != NULL){
           	p = pVideoPlugList->get_ActivePlugIn();
               if(p != NULL){
               	if(!p->IsAttribute(PIT_IS3D))
                   	p = NULL;
               }
               *ppvObject = p;
               return S_OK;
           }
       break;
   }
   return E_FAIL;
}
//---------------------------------------------------------------------------
int PlugInContainer::NotifyState(DWORD items,LPVOID ns)
{
   int group;

   if(ns == NULL || (items & 0x8000) == 0)
       return E_FAIL;
   items >>= PIS_NOTIFYSHIFT;
   group = 1 << (items & 0xF);
   switch((items & 0x70) >> 4){
       case 0:
           if((group & PIL_AUDIO) && pAudioPlugList != NULL){
               pAudioPlugList->Run((LPNOTIFYCONTENT)ns,PIT_ISFILTER);
           }
       break;
       case 1:
           if(!(group & PIL_AUDIO) && pAudioPlugList != NULL){
               pAudioPlugList->NotifyState(items<<PIS_NOTIFYSHIFT,PIS_NOTIFYMASK,(LPARAM)ns);
           }
           if(!(group & PIL_GRAPHICS) && pGraphicsPlugList != NULL){
               pGraphicsPlugList->NotifyState(items<<PIS_NOTIFYSHIFT,PIS_NOTIFYMASK,(LPARAM)ns);
           }
       break;
       default:
           return E_FAIL;
   }
   return S_OK;
}
//---------------------------------------------------------------------------
BOOL PlugInContainer::InitSetPlugInInfo(PlugIn *plugin,LPSETPLUGININFO p,DWORD dwState,DWORD dwStateMask)
{
   PlugIn *p1;

   if(plugin == NULL || p == NULL)
       return FALSE;
   switch(plugin->GetType()){
       case PIT_AUDIO:
           p1 = (PlugIn *)new AudioPlug();
       break;
       case PIT_VIDEO:
           p1 = (PlugIn *)new VideoPlug();
       break;
       case PIT_GRAPHICS:
           p1 = (PlugIn *)new GraphicsPlug();
       break;
       case PIT_WIFI:
           p1 = (PlugIn *)new LWFPlugIn();
       break;
       case PIT_DLDI:
           p1 = (PlugIn *)new DLDIPlugIn();
       break;
       case PIT_WEBCAM:
       	p1 = (PlugIn *)new WebCamPlugIn();
       break;
       default:
           p1 = NULL;
       break;
   }
   if(p1 == NULL)
       return FALSE;
   p1->SetType(plugin->IsAttribute(0xFFFFFFFF));
   p1->InitSetPlugInInfo(p,dwState,dwStateMask);
   delete p1;
   return TRUE;
}
//---------------------------------------------------------------------------
PlugInList *PlugInContainer::get_PlugInList(DWORD type)
{
   switch(type){
		case PIL_VIDEO:
   		return pVideoPlugList;
       case PIL_AUDIO:
       	return pAudioPlugList;
		case PIL_GRAPHICS:
       	return pGraphicsPlugList;
       case PIL_WIFI:
       	return pWifiPlugList;
       case PIL_INPUT:
       	return pInputPlugList;
      	case PIL_DLDI:
       	return pDLDIPlugList;
		case PIL_WEBCAM:
       	return pWebCamPlugList;
       default:
   		return NULL;
   }
}
//---------------------------------------------------------------------------
void PlugInContainer::Reset()
{
   if(pVideoPlugList != NULL)
   	pVideoPlugList->Reset();
	if(pAudioPlugList != NULL)
   	pAudioPlugList->Reset();
   if(pGraphicsPlugList != NULL)
   	pGraphicsPlugList->Reset();
   if(pWifiPlugList != NULL)
   	pWifiPlugList->Reset();
   if(pInputPlugList != NULL)
   	pInputPlugList->Reset();
   if(pDLDIPlugList != NULL)
   	pDLDIPlugList->Reset();
   if(pWebCamPlugList != NULL)
   	pWebCamPlugList->Reset();
}
//---------------------------------------------------------------------------
BOOL PlugInContainer::SaveState(LStream *p)
{
	if(pVideoPlugList != NULL){
       if(!pVideoPlugList->SaveState(p))
           return FALSE;
   }
	if(pAudioPlugList != NULL){
   	if(!pAudioPlugList->SaveState(p))
       	return FALSE;
   }
   if(pGraphicsPlugList != NULL){
   	if(!pGraphicsPlugList->SaveState(p))
       	return FALSE;
   }
   if(pWifiPlugList != NULL){
   	if(!pWifiPlugList->SaveState(p))
       	return FALSE;
   }
   if(pInputPlugList != NULL){
   	if(!pInputPlugList->SaveState(p))
       	return FALSE;
   }
   if(pDLDIPlugList != NULL){
   	if(!pDLDIPlugList->SaveState(p))
       	return FALSE;
   }
	if(pWebCamPlugList != NULL){
   	if(!pWebCamPlugList->SaveState(p))
       	return FALSE;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL PlugInContainer::LoadState(LStream *p,int ver)
{
	if(pVideoPlugList != NULL){
        if(!pVideoPlugList->LoadState(p,ver))
            return FALSE;
	}
	if(pAudioPlugList != NULL){
   	if(!pAudioPlugList->LoadState(p,ver))
       	return FALSE;
   }
   if(pGraphicsPlugList != NULL){
   	if(!pGraphicsPlugList->LoadState(p,ver))
       	return FALSE;
   }
   if(pWifiPlugList != NULL){
   	if(!pWifiPlugList->LoadState(p,ver))
       	return FALSE;
   }
   if(pInputPlugList != NULL){
   	if(!pInputPlugList->LoadState(p,ver))
       	return FALSE;
   }
   if(pDLDIPlugList != NULL){
   	if(!pDLDIPlugList->LoadState(p,ver))
       	return FALSE;
   }
	if(pWebCamPlugList != NULL){
   	if(!pWebCamPlugList->LoadState(p,ver))
       	return FALSE;
   }
   return TRUE;
}

