#include "ideastypes.h"
#include "plugin.h"
#include "lregkey.h"
#include "pluginctn.h"
#include "lds.h"
#include "language.h"

//---------------------------------------------------------------------------
PlugIn::PlugIn()
{
   wID = 0xFFFF;
   dwType = 0;
   ZeroMemory(&guid,sizeof(GUID));
   isComplex = isLicensed = isReload = isLoad = bEnable = 0;
   index = bExclusive = 1;
   bDynamic = 0;
   pResetFunc = NULL;
   pDeleteFunc = NULL;
   pGetInfoFunc = NULL;
   pSetInfoFunc = NULL;
   pRunFunc = NULL;
   pSetPropertyFunc = NULL;
   pSaveStateFunc = NULL;
   pLoadStateFunc = NULL;
   hLib = NULL;
   pathLibrary = "";
   name = "";
   dwFlags = 0;
   pNext = pLast = pNextHookPlugIn = NULL;
   pInputFunc = NULL;
   pOutputFunc = NULL;
}
//---------------------------------------------------------------------------
PlugIn::~PlugIn()
{
   Unload(TRUE);
   if(pInputFunc != NULL)
       LocalFree(pInputFunc);
   pInputFunc = NULL;
   if(pOutputFunc != NULL)
       LocalFree(pOutputFunc);
   pOutputFunc = NULL;
}
//---------------------------------------------------------------------------
BOOL PlugIn::Reset()
{
	if(pResetFunc != NULL)
   	return pResetFunc();
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL PlugIn::NotifyState(DWORD dwState,DWORD dwStateMask,LPARAM lParam)
{
   SETPLUGININFO pi;

   if(!InitSetPlugInInfo(&pi,dwState,dwStateMask))
       return FALSE;
   if(lParam != (LPARAM)-1)
   	pi.lParam = lParam;
 	return SetInfo(&pi);
}
//---------------------------------------------------------------------------
void PlugIn::SetNode(PlugIn *last,PlugIn *next)
{
   if(last != (PlugIn *)0xFFFFFFFF)
       pLast = last;
   if(next != (PlugIn *)0xFFFFFFFF)
       pNext = next;
}
//---------------------------------------------------------------------------
void PlugIn::GetNode(PlugIn **last,PlugIn **next)
{
   if(last != NULL)
       *last = pLast;
   if(next != NULL)
       *next = pNext;
}
//---------------------------------------------------------------------------
u32 PlugIn::Run(LPRUNPARAM param)
{
   param->type = dwType;
   param->index = index;
   return ((LPDEFAULTPLUGINRUN)pRunFunc)(param);
}
//---------------------------------------------------------------------------
BOOL PlugIn::SetProperty(LPSETPROPPLUGIN p)
{
    if(pSetPropertyFunc == NULL)
        return FALSE;
#ifdef __WIN32__
    return pSetPropertyFunc(p);
#else
    LPPROPSHEETPAGE psp;
    HPROPSHEETPAGE hpsp;
    BOOL res;

    psp = (LPPROPSHEETPAGE)pSetPropertyFunc(p);
    if(psp == NULL)
        return FALSE;
    psp->hInstance = hLib;
    hpsp = CreatePropertySheetPage(psp);
    if(hpsp != NULL)
        res = PropSheet_AddPage((HWND)p->hwndOwner,hpsp);
    else
        res = FALSE;
    delete psp;
    return res;
#endif
}
//---------------------------------------------------------------------------
DWORD PlugIn::SetInfo(LPSETPLUGININFO p)
{
   if(p == NULL)
       return FALSE;
//   wID = p->wID;
   if(pSetInfoFunc != NULL)
       return pSetInfoFunc(p);
   return TRUE;
}
//---------------------------------------------------------------------------
int PlugIn::GetInfo(LPGETPLUGININFO p,BOOL bReloadAll)
{
   LString s;
   int res;

   if(p == NULL)
       return FALSE;
   if(bReloadAll){
       if(!Load())
           return FALSE;
       if(pGetInfoFunc != NULL)
           res = pGetInfoFunc(p);
       else{
           res = TRUE;
           if(p->pszText != NULL){
               s.Capacity(p->cchTextMax);
//               TranslateLoadString(&s,wID);
               lstrcpyn((LPSTR)p->pszText,name.c_str(),p->cchTextMax);
           }
           memcpy(&p->guidID,&guid,sizeof(GUID));
       }
       if(res){
           if(!p->guidID.Data1 || !p->guidID.Data2)
               res = FALSE;
           else{
               dwType = (DWORD)(BYTE)p->dwType;
               if((dwType & 0xF) == PIT_VIDEO || (dwType & 0xF) == PIT_AUDIO ||
               	(dwType & 0xF) == PIT_GRAPHICS || (dwType & 0xF) == PIT_WIFI ||
                   (dwType & 0xF) == PIT_PAD || (dwType & 0xF) == PIT_DLDI){
                   	memcpy(&guid,&p->guidID,sizeof(GUID));
                   	name = (LPSTR)p->pszText;
                    SetType(p->dwType);
               }
               else{
                   dwType = 0;
                   res = FALSE;
               }
           }
       }
   }
   else{
       p->dwType = dwType;
       memcpy(&p->guidID,&guid,sizeof(GUID));
  		if(p->pszText != NULL && p->cchTextMax > 0)
      		lstrcpyn((LPSTR)p->pszText,name.c_str(),p->cchTextMax);
       res = TRUE;
   }
   return res;
}
//---------------------------------------------------------------------------
void PlugIn::SetType(DWORD value)
{
   dwType = (DWORD)(BYTE)value;
   if((dwType & 0xF) == PIT_GRAPHICS)
       value |= PIT_ISFILTER;
   dwFlags = (DWORD)value;
   bDynamic = (char)((value & PIT_DYNAMIC) >> 8);
   bExclusive = !((value & PIT_NOEXCLUSIVE) >> 9);
   if(value & (PIT_NOMENU|PIT_ISFILTER))
       bExclusive = 0;
   isLicensed = (value >> 4) == ((guid.Data3 >> 4) & 0xF);
}
//---------------------------------------------------------------------------
BOOL PlugIn::Load()
{
   BOOL res;

   if(isLoad)
       return TRUE;
   Unload();
   res = FALSE;
   if(pathLibrary.IsEmpty())
       return TRUE;
   hLib = GetModuleHandle(pathLibrary.c_str());
   if(hLib != NULL)
       isReload = TRUE;
   else
       hLib = LoadLibrary(pathLibrary.c_str());
   if(hLib == NULL)
       return FALSE;
   pResetFunc = (LPPLUGINRESET)GetProcAddress(hLib,"ResetFunc");
   pDeleteFunc = (LPPLUGINDELETE)GetProcAddress(hLib,"DeleteFunc");
   pGetInfoFunc = (LPPLUGINGETINFO)GetProcAddress(hLib,"GetInfoFunc");
   pSetInfoFunc = (LPPLUGINSETINFO)GetProcAddress(hLib,"SetInfoFunc");
   pSetPropertyFunc = (LPPLUGINSETPROPERTY)GetProcAddress(hLib,"SetPropertyFunc");
   pSaveStateFunc = (LPPLUGINSAVESTATE)GetProcAddress(hLib,"SaveStateFunc");
   pLoadStateFunc = (LPPLUGINLOADSTATE)GetProcAddress(hLib,"LoadStateFunc");
   pRunFunc = GetProcAddress(hLib,"RunFunc");
   if(pGetInfoFunc == NULL || pSetInfoFunc == NULL || pRunFunc == NULL)
       Unload();
   else{
       res = TRUE;
       isLoad = 1;
   }
   return res;
}
//---------------------------------------------------------------------------
int PlugIn::GetFileInfo(char *pOut)
{
#ifdef __WIN32__
   DWORD dwLen,lpPointer;
   LPBYTE lpBuf;
   UINT dwByte;
   WORD lID,slID;
   char string[100];
#endif
   int i;

   *((LPDWORD)pOut) = 0;
   i = -1;
   if(pathLibrary.IsEmpty())
       return i;
#ifdef __WIN32__
   if((dwLen = GetFileVersionInfoSize(pathLibrary.c_str(),&dwLen)) < 1)
       return i;
   if((lpBuf = new BYTE[dwLen+10]) == NULL)
       return i;
   if(GetFileVersionInfo(pathLibrary.c_str(),0,dwLen,(LPVOID)lpBuf)){
       if(VerQueryValue((LPVOID)lpBuf,"\\VarFileInfo\\Translation",(LPVOID *)&lpPointer,&dwByte)){
           lID =  ((LPWORD)lpPointer)[0];
           slID = ((LPWORD)lpPointer)[1];
           wsprintf(string,"\\StringFileInfo\\%04x%04x\\CompanyName",lID,slID);
           if(VerQueryValue((LPVOID)lpBuf,string,(LPVOID *)&lpPointer,&dwByte))
               lstrcpy(pOut,(LPSTR)lpPointer);
           else{
               lstrcpy(pOut,"Unknown");
               dwByte = 8;
           }
           pOut += dwByte;
           *pOut++ = 0;
           i = dwByte + 1;
           wsprintf(string,"\\StringFileInfo\\%04x%04x\\ProductVersion",lID,slID);
           if(VerQueryValue((LPVOID)lpBuf,string,(LPVOID *)&lpPointer,&dwByte))
               lstrcpy(pOut,(LPSTR)lpPointer);
           else{
               lstrcpy(pOut,"Unknown");
               dwByte = 8;
           }
           pOut += dwByte;
           *((LPWORD)pOut) = 0;
           i += dwByte + 2;
       }
   }
  	delete []lpBuf;
#else
	HINSTANCE lib;
	DWORD (*pfnFunc)(char *);

   lib = dlopen(pathLibrary.c_str(),RTLD_LAZY);
   if(lib != NULL){
       pfnFunc = (DWORD (*)(char *))dlsym(lib,"GetFileVersionInfo");
       if(pfnFunc != NULL)
       	i = (int)pfnFunc(pOut);
   	dlclose(lib);
   }
#endif
   return i;
}
//---------------------------------------------------------------------------
BOOL PlugIn::InitGetPlugInInfo(LPGETPLUGININFO p)
{
   if(p == NULL)
       return FALSE;
   ZeroMemory(p,sizeof(GETPLUGININFO));
   p->cbSize = sizeof(GETPLUGININFO);
   p->wIndex = (WORD)index;
   p->dwType = (DWORD)((dwType & 0xFF0F) | dwFlags);
   memcpy(&p->guidID,&guid,sizeof(GUID));
   if(!langManager.is_DefaultLanguage(langManager.get_CurrentId())){
       p->lParam = (u32)langManager.get_CurrentLib();
       p->dwLanguageID = langManager.get_CurrentId();
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL PlugIn::InitSetPlugInInfo(LPSETPLUGININFO p,DWORD dwState,DWORD dwStateMask)
{
   if(p == NULL)
       return FALSE;
//   ZeroMemory(p,sizeof(SETPLUGININFO));
   p->cbSize = sizeof(SETPLUGININFO);
//   p->wID = wID;
   p->wIndex = (WORD)index;
//   p->dwType = (DWORD)(dwType & 0xFF0F);
   p->dwState = dwState;
   p->dwStateMask = dwStateMask;
   if((dwState & PIS_ENABLEMASK))
       p->dwStateMask |= PIS_ENABLEMASK;
   if((dwState & PIS_RUNMASK))
       p->dwStateMask |= PIS_RUNMASK;
//   *(&p->guidID) = *(&guid);
//   p->dwLanguageID = MAKELONG(0x410,0x410);
   p->lParam = 0;
   p->lpNDS = (INDS*)&ds;
   return TRUE;
}
//---------------------------------------------------------------------------
void PlugIn::Unload(BOOL bForced)
{
   PlugIn *p,*p1;
   BOOL bFree;

   if(!bForced && !bDynamic)
       return;
   if(!isReload){
       if(isLoad == 2 && pDeleteFunc != NULL)
           pDeleteFunc();
       if(hLib != NULL){
           bFree = TRUE;
           p = pLast;
           while(p != NULL){
               p->GetNode(&p1);
               if(p1 == NULL)
                   break;
               p = p1;
           }
           if(p == NULL)
               p = pNext;
           while(p != NULL){
               if(p->IsEnable()){
                   bFree = FALSE;
                   break;
               }
               p->GetNode(NULL,&p);
           }
           if(bFree)
               FreeLibrary(hLib);
       }
   }
   hLib = NULL;
   pRunFunc = NULL;
   pResetFunc = NULL;
   pDeleteFunc = NULL;
   pGetInfoFunc = NULL;
   pSetInfoFunc = NULL;
   pSetPropertyFunc = NULL;
   pSaveStateFunc = NULL;
   pLoadStateFunc = NULL;
   isLoad = 0;
}
//---------------------------------------------------------------------------
BOOL PlugIn::Enable(BOOL bFlag)
{
   BOOL res;

   if(bEnable == bFlag)
       return TRUE;
   if(IsAttribute(PIT_HOOKAFTER|PIT_HOOKBEFORE))
       EnableHook(bFlag);
   res = TRUE;
   if((bEnable = (char)(bFlag & 1)) == 0){
       NotifyState(0,PIS_ENABLEMASK,0);
       ResetTable();
       Unload();
   }
   else{
       if((res = Load()) != 0){
           SetUsed();
           res = NotifyState((bEnable & 1),PIS_RUNMASK|PIS_ENABLEMASK);
           if(res)
           	NotifyState(PNM_INITTABLE,PIS_NOTIFYMASK,ds.LMMU::get_EmuMode());
       }
   }
   if(res && pNext != NULL && OnEnableComplex())
       pNext->Enable(bFlag);
   return res;
}
//---------------------------------------------------------------------------
int PlugIn::get_Format(DWORD,LPVOID object,LPDWORD dwSize)
{
   GETPLUGININFO gpi;
   int res;

   if(object == NULL || dwSize == NULL || pGetInfoFunc == NULL)
       return E_FAIL;
   InitGetPlugInInfo(&gpi);               
   gpi.wType = PIR_FORMAT;
   res = (int)pGetInfoFunc(&gpi);
   if(!(res & 0xF0000000))
       return E_FAIL;
   *dwSize = abs(res);
   InitGetPlugInInfo(&gpi);
   gpi.wType = PIR_FORMAT;
   gpi.lParam = (LPARAM)object;
   res = (int)pGetInfoFunc(&gpi);
   if(!(res & 0xF0000000))
       return E_FAIL;
   return S_OK;
}
//---------------------------------------------------------------------------
int PlugIn::WriteTable(DWORD,LPVOID,LPVOID)
{
   return E_FAIL;
}
//---------------------------------------------------------------------------
int PlugIn::TriggerIRQ()
{
   return E_FAIL;
}
//---------------------------------------------------------------------------
int PlugIn::get_Object(int iid,LPVOID *obj)
{
	if(obj == NULL)
   	return E_FAIL;
   *obj = NULL;
   switch(iid){
   	case OID_IO_MEMORY9:
       	*obj = io_mem;
       	return S_OK;
   	case OID_IO_MEMORY7:
       	*obj = io_mem7;
       	return S_OK;
		case OID_VRAM_MEMORY:
       	*obj = video_mem;
       	return S_OK;
       case OID_PALETTE_MEMORY:
       	*obj = pal_mem;
           return S_OK;
       case OID_OAM_MEMORY:
       	*obj = obj_mem;
           return S_OK;
       case OID_EXTERN_WRAM:
       	*obj = int_mem;
           return S_OK;
		case OID_READ_BYTE:
       	*obj = (LPVOID)read_byte;
       	return S_OK;
		case OID_READ_HWORD:
       	*obj = (LPVOID)read_hword;
       	return S_OK;
		case OID_READ_WORD:
       	*obj = (LPVOID)read_word;
       	return S_OK;
       case OID_RUNFUNC:
       	*obj = (LPVOID)pRunFunc;
           return S_OK;
       case OID_FIRMWARE_MEMORY:
           *obj = (LPVOID)fw_mem;
           return S_OK;
       case OID_POWERMNG:
           extern POWERMNG pwr;

           memcpy(obj,&pwr,sizeof(POWERMNG));
           return S_OK;
   	default:
       	return E_FAIL;
   }
}
//---------------------------------------------------------------------------
PlugInList::PlugInList(char *p) : LList()
{
   name = p;
   pActivePlugIn = NULL;
	next = NULL;
   container = NULL;
}
//---------------------------------------------------------------------------
PlugInList::~PlugInList()
{
   SaveSetConfig();
   Clear();
}
//---------------------------------------------------------------------------
void PlugInList::Clear()
{
   elem_list *tmp,*tmp1;

   tmp = Last;
   while(tmp){
       tmp1 = tmp->Last;
       DeleteElem(tmp->Ele);
       delete tmp;
       tmp = tmp1;
   }
   nCount = 0;
   First = Last = NULL;
}
//---------------------------------------------------------------------------
void PlugInList::DeleteElem(LPVOID ele)
{
   if(ele == NULL)
       return;
   ((PlugIn *)ele)->Destroy();
   delete (PlugIn *)ele;
}
//---------------------------------------------------------------------------
void PlugInList::SaveSetConfig()
{
   LRegKey reg;
   GUID guid;

   GetSelectedGUID(&guid);
   reg.Open("Software\\iDeaS");
   reg.WriteBinaryData(name.c_str(),(char *)&guid,sizeof(GUID));
   reg.Close();
}
//---------------------------------------------------------------------------
BOOL PlugInList::Run(LPRUNPARAM param,DWORD dwAttribute)
{
   LList::elem_list *ele;
	PlugIn *pPlugIn;
   DWORD i;

   i = 0;
   ele = First;
   while(ele != NULL){
       pPlugIn = (PlugIn *)ele->Ele;
		if(pPlugIn->IsEnable() && pPlugIn->IsComplex() && pPlugIn->IsAttribute(dwAttribute)){
       	pPlugIn->Run(param);
           i++;
       }
       ele = ele->Next;
   }
   return i ? TRUE : FALSE;
}
//---------------------------------------------------------------------------
void PlugInList::LoadSetConfig()
{
   LRegKey reg;
   GUID guid;

   reg.Open("Software\\iDeaS");
   reg.ReadBinaryData(name.c_str(),(char *)&guid,sizeof(GUID));
   Enable(&guid,TRUE);
   reg.Close();
}
//---------------------------------------------------------------------------
PlugIn *PlugInList::BuildPlugIn(char *path)
{
   PlugIn *p;

   if(path == NULL || (p = new PlugIn()) == NULL)
       return NULL;
   p->SetLibraryPath(path);
   return p;
}
//---------------------------------------------------------------------------
PlugIn *PlugInList::GetItemFromGUID(LPGUID p)
{
   DWORD dwPos;
   PlugIn *pPlugIn,*res;
   GETPLUGININFO pi;

   res = NULL;
   if((pPlugIn = (PlugIn *)GetFirstItem(&dwPos)) ==  NULL)
       return res;
   do{
       ZeroMemory(&pi,sizeof(GETPLUGININFO));
       pi.cbSize = sizeof(GETPLUGININFO);
       if(!pPlugIn->GetInfo(&pi))
           continue;
       if(memcmp(&pi.guidID,p,sizeof(GUID)) == 0){
           res = pPlugIn;
           break;
       }
   }while((pPlugIn = (PlugIn *)GetNextItem(&dwPos)) != NULL);
   return res;
}
//---------------------------------------------------------------------------
BOOL PlugInList::PreLoad(WORD *wID)
{
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL PlugInList::Enable(DWORD item,BOOL bEnable)
{
	PlugIn *p1;
   GUID g;

   p1 = (PlugIn *)GetItem(item);
   if(p1 == NULL)
   	return FALSE;
   p1->GetGuid(&g);
	return Enable(&g,bEnable);
}
//---------------------------------------------------------------------------
PlugIn *PlugInList::SetActivePlugIn(PlugIn *ele,DWORD dwType)
{
   PlugIn *p;

   p = pActivePlugIn;
   pActivePlugIn = ele;
   return p;
}
//---------------------------------------------------------------------------
BOOL PlugInList::Enable(LPGUID p,BOOL bEnable)
{
   PlugIn *pPlugIn,*p1,*p2;
   DWORD dwPos;

   if(p == NULL || (pPlugIn = GetItemFromGUID(p)) == NULL)
       return FALSE;
   p2 = pActivePlugIn;
   if(pPlugIn->IsExclusive() && bEnable && !pPlugIn->IsAttribute(PIT_HOOKAFTER|PIT_HOOKBEFORE|PIT_NOMENU)){
       dwPos = 0;
       if((p1 = (PlugIn *)GetFirstItem(&dwPos)) !=  NULL){
           do{
               if(!p1->IsAttribute(PIT_HOOKAFTER|PIT_HOOKBEFORE|PIT_ISFILTER) && p1 != pPlugIn){
               	if(CheckTypePlugIn(p1->IsAttribute(0xFFFFFFFF),pPlugIn->IsAttribute(0xFFFFFFFF)))
                   	p1->Enable(FALSE);
               }
           }while((p1 = (PlugIn *)GetNextItem(&dwPos)) != NULL);
       }
   }
   if(!pPlugIn->Enable(bEnable) && bEnable){                        
       Delete(IndexFromEle((LPVOID)pPlugIn));
       if(p2 != NULL)
           p2->Enable(TRUE);
       return FALSE;
   }
   if(!pPlugIn->IsAttribute(PIT_HOOKAFTER|PIT_HOOKBEFORE|PIT_NOMENU)){
       if(!pPlugIn->IsAttribute(PIT_ISFILTER)){
           DWORD dw;

           dw = pPlugIn->IsAttribute(0xFFFFFFFF);
           if(!bEnable){
               if(get_ActivePlugIn(dw) == pPlugIn)
                   pPlugIn = NULL;
//               else
//                   return TRUE;
           }
         	SetActivePlugIn(pPlugIn,dw);
       }
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL PlugInList::Add(PlugIn *ele)
{
   GUID guid;
   elem_list *p;

   if(ele == NULL)
       return FALSE;
   ele->GetGuid(&guid);
   if(pPlugInContainer->get_PlugInFromGUID(&guid) != NULL)
       return FALSE;
   if(!LList::Add((LPVOID)ele))
       return FALSE;
   if(!ele->IsAttribute(PIT_HOOKAFTER|PIT_HOOKBEFORE) || nCount < 2)
       return TRUE;
   p = First;
   do{
       if(p != (LList::elem_list*)ele){
           if(((PlugIn *)p)->get_NextHook() ==NULL){
               ((PlugIn *)p)->set_NextHook(ele);
               break;
           }
       }
   }while((p = p->Next) != NULL);
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL PlugInList::GetSelectedGUID(LPGUID p)
{
   GETPLUGININFO pi;
   BOOL bres;

   if(p == NULL)
       return FALSE;
   ZeroMemory(p,sizeof(GUID));
   bres = FALSE;
/*   if((pPlugIn = (PlugIn *)GetFirstItem(&dwPos)) ==  NULL)
       return bres;
   do{
       if(!pPlugIn->IsEnable())
           continue;
       ZeroMemory(&pi,sizeof(GETPLUGININFO));
       pi.cbSize = sizeof(GETPLUGININFO);
       if(!pPlugIn->GetInfo(&pi))
           break;
       memcpy(p,&pi.guidID,sizeof(GUID));
       bres = TRUE;
   }while(!bres && (pPlugIn = (PlugIn *)GetNextItem(&dwPos)) != NULL);*/
   if(pActivePlugIn != NULL){
        ZeroMemory(&pi,sizeof(GETPLUGININFO));
        pi.cbSize = sizeof(GETPLUGININFO);
        if(pActivePlugIn->GetInfo(&pi)){
           memcpy(p,&pi.guidID,sizeof(GUID));
           bres = TRUE;
   	}
	}                     

   return bres;
}
//---------------------------------------------------------------------------
BOOL PlugInList::OnEnablePlug(WORD wID)
{
   BOOL res;
   PlugIn *p;
   MENUITEMINFO mi={0};
   GUID guid;
	IMenu *menu;
	DWORD dwItem;

   if(nCount == 0)
       return FALSE;
   res = FALSE;
   mi.cbSize = sizeof(MENUITEMINFO);
   mi.fMask = MIIM_DATA;
	menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
	menu->GetMenuItemInfo(wID,FALSE,&mi);
   dwItem = mi.dwItemData + 1;
   if((p = (PlugIn *)GetItem(dwItem)) != NULL){
       p->GetGuid(&guid);
       res = Enable(&guid,!p->IsEnable());
   }
	menu->Release();
   return res;
}
//---------------------------------------------------------------------------
int PlugInList::OnInitMenu(HMENU menu,BOOL bDelete,int startPos)
{
   int i,i1,i2,i3,res,changed,iTotAccel;
   MENUITEMINFO mii={0};
   PlugIn *p;
   DWORD dwPos;
   LString s,s1,s2;
   GETPLUGININFO pi;
   LPACCEL buffer;
   BYTE fVirt;
   HACCEL accel;

   buffer = NULL;
   iTotAccel = 0;
   mii.cbSize = sizeof(MENUITEMINFO);
   if(bDelete){
       i = CopyAcceleratorTable(ds.accel,NULL,0);
       buffer = (LPACCEL)LocalAlloc(LPTR,i*sizeof(ACCEL));
       i1 = CopyAcceleratorTable(ds.accel,buffer,i);
       i = GetMenuItemCount(menu) - 1;
       s.Capacity(250);
       changed = 0;	   
       for(;i >= 0;i--){
           mii.fMask = MIIM_TYPE;
           mii.fType = MFT_STRING;           
           mii.cch = 250;
			mii.dwTypeData = s.c_str();
           GetMenuItemInfo(menu,i,TRUE,&mii);
           if((i2 = s.Pos(9)) > 0){
               s1 = s.SubString(i2+1,s.Length()-2).AllTrim();
				s2 = s1.LowerCase();
               if(s2.Pos("ctrl") == 1)
                   fVirt = FCONTROL;
               else if(s2.Pos("shift") == 1)
                   fVirt = FSHIFT;
               else if(s2.Pos("alt") == 1)
                   fVirt = FALT;
               else
                   fVirt = 0;
               if(fVirt != 0 && (i2 = s.Pos('+')) > 0){
                   s1 = s.SubString(i2+1,s.Length() - i2).AllTrim();
                   for(i3=0;i3<i1;i3++){
                       if(buffer[i3].fVirt == fVirt && buffer[i3].key == s1[1]){
                           if(buffer[i3].cmd >= ID_PLUGIN_START && buffer[i3].cmd <= ID_PLUGIN_END){
                               buffer[i3].fVirt = 0;
                               buffer[i3].key = 0;
                               changed = 1;
                           }
                       }
                   }
               }
           }
           DeleteMenu(menu,i,MF_BYPOSITION);
       }
       if(changed){
           for(i2=i3=0;i3<i1;i3++){
               if(buffer[i3].fVirt == 0 && buffer[i3].key == 0)
                   continue;
               memcpy(&buffer[i2++],&buffer[i3],sizeof(ACCEL));
           }
           accel = CreateAcceleratorTable(buffer,i2);
           if(accel != NULL){
               DestroyAcceleratorTable(ds.accel);
               ds.accel = accel;
           }
       }
       LocalFree(buffer);
       buffer = NULL;
   }
   res = 0;
   if(nCount < 1)
       goto ex_OnInitMenu;
   p = (PlugIn *)GetFirstItem(&dwPos);
   i = startPos;
   do{
       if(p->IsAttribute(PIT_NOMENU))
           continue;
       mii.fMask = MIIM_TYPE|MIIM_STATE|MIIM_ID|MIIM_DATA;
       mii.fType = MFT_STRING;
       mii.fState = MFS_ENABLED;
       mii.dwItemData = (DWORD)IndexFromEle(p)-1;
       if(p->IsEnable() && !p->IsAttribute(PIT_ENABLERUN))
           mii.fState |= MFS_CHECKED;
       p->InitGetPlugInInfo(&pi);
       s.Capacity(250);
       pi.pszText = (LPWSTR)s.c_str();
       pi.cchTextMax = 249;
       p->GetInfo(&pi,TRUE);		
       mii.dwTypeData = s.c_str();
       mii.wID = p->GetMenuID();
       if((i2 = s.Pos(9)) > 0){
           if(buffer == NULL){
               iTotAccel = CopyAcceleratorTable(ds.accel,NULL,0);
               buffer = (LPACCEL)LocalAlloc(LPTR,(iTotAccel + i) * sizeof(ACCEL) * 2);
               iTotAccel = CopyAcceleratorTable(ds.accel,buffer,iTotAccel);
           }
           s1 = s.SubString(i2+1,s.Length()-2).AllTrim();
			s2 = s1.LowerCase();
           if(s2.Pos("ctrl") == 1){
               fVirt = FCONTROL;
               i2 = 4;
           }
           else if(s2.Pos("shift") == 1){
               fVirt = FSHIFT;
               i2 = 5;
           }
           else if(s2.Pos("alt") == 1){
               i2 = 3;
               fVirt = FALT;
           }
           else
               fVirt = 0;
           if(fVirt != 0 && (i2 = s.Pos('+')) > 0){
               s1 = s.SubString(i2+1,s.Length() - i2).AllTrim();
               buffer[iTotAccel].fVirt = (BYTE)(fVirt|FVIRTKEY);
               buffer[iTotAccel].key = s1[1];
               buffer[iTotAccel].cmd = (WORD)mii.wID;
               iTotAccel++;
           }
       }
       InsertMenuItem(menu,i++,TRUE,&mii);
       if(!p->IsEnable())
           p->Unload();
   }while((p = (PlugIn *)GetNextItem(&dwPos)) != NULL);
   if(i != startPos)
   	res = i;
   if(iTotAccel > 0){
       accel = CreateAcceleratorTable(buffer,iTotAccel);
       if(accel != NULL){
           DestroyAcceleratorTable(ds.accel);
           ds.accel = accel;
       }
   }
ex_OnInitMenu:
   if(buffer != NULL)
       LocalFree(buffer);
   return res;
}
//---------------------------------------------------------------------------
BOOL PlugInList::CheckTypePlugIn(DWORD dwPlugInType,DWORD Type)
{
   Type &= ~0xF;
   Type |= (dwType & 0xF);
	return (dwPlugInType & 0xF) == (Type & 0xF);
}
//---------------------------------------------------------------------------
BOOL PlugInList::CheckIsComplex(PlugIn *p)
{
   PlugInList *list;

	if(p == NULL || pPlugInContainer == NULL)
   	return FALSE;
   list = pPlugInContainer->get_PlugInList((DWORD)(1 << p->GetType()));
   if(list == NULL)
   	return FALSE;
	return list != this;
}
//---------------------------------------------------------------------------
int PlugInList::Load(WORD wID,DWORD Type)
{
   LString curDir,prgDir,c,cName;
   GETPLUGININFO pi;
   SETPLUGININFO pi2;
   WIN32_FIND_DATA wf;
   HANDLE handle;
   BOOL bFlag,res,bComplex;
   PlugIn *p,*p1,*p0;
   PlugInList *list;
   int i1,i;

	dwType = Type;
   wID += (WORD)Count();
   if(!PreLoad(&wID))
       return FALSE;
   curDir.Capacity(MAX_PATH+1);
   prgDir.Capacity(MAX_PATH+1);
   GetModuleFileName(NULL,prgDir.c_str(),MAX_PATH);
   prgDir = prgDir.Path();
   prgDir += DPC_PATH;
   prgDir += "PlugIn";
   GetCurrentDirectory(MAX_PATH,curDir.c_str());
   SetCurrentDirectory(prgDir.c_str());
#ifdef __WIN32__
   c = "*.dll";
#else
   c = "*.so";
#endif
   handle = FindFirstFile(c.c_str(),&wf);
   bFlag = handle != INVALID_HANDLE_VALUE ? TRUE : FALSE;
   while(bFlag){
       c = prgDir;
       c += DPC_PATH;
       cName = wf.cFileName;
       c += cName;
       if((p = BuildPlugIn(c.c_str())) != NULL){
           res = FALSE;
           if(p->Load()){
               p->InitGetPlugInInfo(&pi);
               cName.Length(256);
               pi.pszText = (wchar_t *)cName.c_str();
               pi.cchTextMax = 255;
               if((i = p->GetInfo(&pi,TRUE)) != 0 && CheckTypePlugIn(pi.dwType,0)){
                   p->InitSetPlugInInfo(&pi2);
                   //                   pi.wID = wID;
                   if(p->SetInfo(&pi2) && Add(p)){
                       p->SetUsed();
                       p->SetMenuID(wID++);
                       res = TRUE;
                       p0 = p;
                       bComplex = FALSE;
                       for(i1=2;i1<=i;i1++){
                           if((p1 = BuildPlugIn(c.c_str())) == NULL)
                               break;
                           p1->SetUsed();
                           p1->SetIndex((char)i1);
                           p1->InitGetPlugInInfo(&pi);
                           if(p1->GetInfo(&pi,TRUE)){
                               pPlugInContainer->InitSetPlugInInfo(p1,&pi2);
                               //pi.wID = wID;
                               if(p1->SetInfo(&pi2) && (list = pPlugInContainer->Add(p1))){
                                   p0->SetNode((PlugIn *)0xFFFFFFFF,p1);
                                   p1->SetNode(p0);
                                   p0 = p1;
                                   p1->SetMenuID(wID);
                                   if(!CheckIsComplex(p1))
                                       wID++;
                                   else{
                                       bComplex = TRUE;
                                       p1->SetComplex(TRUE);
                                   }
                                   p1->Unload();
                               }
                               else{
                                   delete p1;
                                   p1 = NULL;
                               }
                           }
                           else{
                               delete p1;
                               p1 = NULL;
                           }
                       }
                       if(bComplex && p1 != NULL){
                           p1 = p;
                           do{
                               p1->SetComplex(TRUE);
                               p1->GetNode(NULL,&p1);
                           }while(p1 != NULL);
                       }
                   }
               }
           }
           if(!res)
               delete p;
           else
               p->Unload();
       }
       bFlag = ::FindNextFile(handle,&wf);
   }
   if(handle != INVALID_HANDLE_VALUE)
       FindClose(handle);
   SetCurrentDirectory(curDir.c_str());
   LoadSetConfig();
   return (int)wID;
}
//---------------------------------------------------------------------------
int PlugInList::Reset()
{
   int i;
   elem_list *p;

   if(nCount == 0)
       return -1;
   for(p = First,i = 0;p != NULL;p = p->Next,i++)
       ((PlugIn *)p->Ele)->Reset();
   return i;
}
//---------------------------------------------------------------------------
int PlugInList::NotifyState(DWORD dwState,DWORD dwStateMask,LPARAM lParam)
{
   int i;
   elem_list *p;

	for(p = First,i = 0;p != NULL;p = p->Next,i++){
       if(((PlugIn *)p->Ele)->IsEnable())
		    ((PlugIn *)p->Ele)->NotifyState(dwState,dwStateMask,lParam);
   }
   return i;
}
//---------------------------------------------------------------------------
BOOL PlugInList::SaveState(LStream *p)
{
   GUID g;
	PlugIn *p1;
   DWORD dwPos;

   p1 = (PlugIn *)GetFirstItem(&dwPos);
   while(p1 != NULL){
   	if(p1->IsEnable()){
           p1->GetGuid(&g);
           p->Write(&g,sizeof(GUID));
           if(!p1->SaveState(p))
               return FALSE;
       }
   	p1 = (PlugIn *)GetNextItem(&dwPos);
   }
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL PlugInList::LoadState(LStream *p,int ver)
{
   GUID g;
   GETPLUGININFO pi;
   LString s;
   char s1[300];
   PlugIn *p1;
   DWORD dwPos;
   
/*	if(pActivePlugIn != NULL){
       p->Read(&g,sizeof(g));
       s.Length(256);
       pi.pszText = (wchar_t*)s1;
       pi.cchTextMax = 299;
       pActivePlugIn->GetInfo(&pi,FALSE);
       if(memcmp(&g,&pi.guidID,sizeof(GUID)) != 0){
           s = "Un error occuring when load the savestate. PlugIn : ";
           s += s1;
           s += "\r\nCheck your list of plugins";
           MessageBox(NULL,s.c_str(),"iDeaS Emulator",MB_ICONERROR|MB_OK);
           return FALSE;
       }
   	return pActivePlugIn->LoadState(p,ver);
   }*/
   p1 = (PlugIn *)GetFirstItem(&dwPos);
   while(p1 != NULL){
       if(p1->IsEnable()){
           p->Read(&g,sizeof(g));
           s.Length(256);
           pi.pszText = (wchar_t*)s1;
           pi.cchTextMax = 299;
           p1->GetInfo(&pi,FALSE);
           if(memcmp(&g,&pi.guidID,sizeof(GUID)) != 0){
               s = "SaveState info\r\n------------------------------\r\n";
               s += "Un error occuring when load the savestate. PlugIn : ";
               s += s1;
               s += "\r\nCheck your list of plugins";
               MessageBox(NULL,s.c_str(),"iDeaS Emulator",MB_ICONERROR|MB_OK);
               return FALSE;
           }
           if(!p1->LoadState(p,ver))
               return FALSE;
       }
       p1 = (PlugIn *)GetNextItem(&dwPos);
   }
	return TRUE;
}

