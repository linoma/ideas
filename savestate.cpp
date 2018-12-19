//---------------------------------------------------------------------------
#include "savestate.h"
#include "lstring.h"
#include "cpu.h"
#include "lds.h"
#include "card.h"
#include "pxi.h"
#include "dma.h"
#include "timers.h"
#include "pluginctn.h"
#include "io.h"

#define VER_SAVESTATE  49
//---------------------------------------------------------------------------
LSaveState::LSaveState()
{
	Index = 0;
   IndexMax = 1;
	bUseFile = FALSE;
   pMemoryFile = NULL;
}
//---------------------------------------------------------------------------
LSaveState::~LSaveState()
{
   if(pMemoryFile != NULL)
   	delete pMemoryFile;
   pMemoryFile = NULL;
}
//---------------------------------------------------------------------------
void LSaveState::Reset()
{
   if(pMemoryFile != NULL)
   	delete pMemoryFile;
	Index = 0;
   pMemoryFile = NULL;
}
//---------------------------------------------------------------------------
BOOL LSaveState::set_File(const char *fileName)
{
   zipFile.Close();
   zipFile.SetFileStream(NULL);
	if(fileName == NULL)
   	bUseFile = FALSE;
   else{
   	lstrcpy(cFileName,fileName);
   	bUseFile = TRUE;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LSaveState::Init(BOOL bOpenAlways)
{
	if(bUseFile){
   	if(!zipFile.Open(cFileName,bOpenAlways))
       	return FALSE;
		return TRUE;
   }
   if(pMemoryFile == NULL){
       if((pMemoryFile = new LMemoryFile(65536)) == NULL)
           return FALSE;
   }
   if(!pMemoryFile->IsOpen()){
       if(!pMemoryFile->Open())
           return FALSE;
       Index = 0;
   }
   zipFile.SetFileStream(pMemoryFile);
   if(!zipFile.Open(""))
       return FALSE;
	return TRUE;
}
//---------------------------------------------------------------------------
LPVOID LSaveState::Save(LPSAVESTATECALLBACK pCallBack,int iLevel,int index)
{
   LString s;
   int i;
   LPZIPFILEHEADER res;

	if(!Init(TRUE))
   	return NULL;
   s = (int)(index != -1 ? index : Index + 1);
   s += ".sgf";
   res = NULL;
   if(zipFile.AddZipFile(s.c_str(),iLevel)){
       i = MAKELONG(VER_SAVESTATE,ds.get_EmuMode());
       zipFile.WriteZipFile(&i,sizeof(int));
       if(pCallBack) pCallBack(1);
       arm9.Save(&zipFile);
       if(pCallBack) pCallBack(2);
       arm7.Save(&zipFile);
       if(pCallBack) pCallBack(3);
       syscnt_Save(&zipFile);
       if(pCallBack) pCallBack(4);
       ds.Save(&zipFile);
       if(pCallBack) pCallBack(5);
/*       upLcd.Save(&zipFile);
       if(pCallBack) pCallBack(6);
       downLcd.Save(&zipFile);
       if(pCallBack) pCallBack(7);*/
       EEPROM_Save(&zipFile);
       if(pCallBack) pCallBack(8);
       PXI_Save(&zipFile);
       if(pCallBack) pCallBack(9);
       DMA_Save(&zipFile);
       if(pCallBack) pCallBack(10);
       TIMER_Save(&zipFile);
       if(pCallBack) pCallBack(11);
       pPlugInContainer->SaveState(&zipFile);
		if(pCallBack) pCallBack(12);
       POWER_Save(&zipFile);
       if(pCallBack) pCallBack(13);
       res = zipFile.GetCurrentZipFileHeader();
       if(index == -1){
       	Index++;
       	if(Index > IndexMax){
           	zipFile.Close();
           	zipFile.Open("");
           	zipFile.DeleteZipFile(1);
       	}
       }
   }
   zipFile.Close();
   return res;
}
//---------------------------------------------------------------------------
BOOL LSaveState::Load(LPSAVESTATECALLBACK pCallBack,UINT index)
{
   int ver;
   BOOL res;

	if(!Init())
   	return FALSE;
   res = FALSE;
   if(zipFile.OpenZipFile((WORD)index) != NULL){
       zipFile.ReadZipFile(&ver,sizeof(int));
       ver = LOWORD(ver);
       if(ver >= 33){
           if(pCallBack) pCallBack(1);
           arm9.Load(&zipFile,ver);
           if(pCallBack) pCallBack(2);
           arm7.Load(&zipFile,ver);
           if(pCallBack) pCallBack(3);
           syscnt_Load(&zipFile,ver);
           if(pCallBack) pCallBack(4);
           ds.Load(&zipFile,ver);
           if(pCallBack) pCallBack(5);
			if(ver < 38){
               upLcd.Load(&zipFile,ver);
               if(pCallBack) pCallBack(6);
               downLcd.Load(&zipFile,ver);
               if(pCallBack) pCallBack(7);
           }
           EEPROM_Load(&zipFile,ver);
           if(pCallBack) pCallBack(8);
           PXI_Load(&zipFile,ver);
           if(pCallBack) pCallBack(9);
           DMA_Load(&zipFile,ver);
           if(pCallBack) pCallBack(10);
           TIMER_Load(&zipFile,ver);
           if(pCallBack) pCallBack(11);
       	pPlugInContainer->LoadState(&zipFile,ver);
			if(pCallBack) pCallBack(12);
           POWER_Load(&zipFile,ver);
           if(pCallBack) pCallBack(13);
#ifdef _DEBPRO
           EEPROM_Load();
#endif
           res = TRUE;
       }
       else
           MessageBox(ds.Handle(),"SaveState info\r\n------------------------------\r\nUn error occuring when load the savestate.\r\nThe version isn't correct.","iDeaS Emulator",MB_OK|MB_ICONERROR);
   }
   return res;
}
//---------------------------------------------------------------------------
LSaveStateList::LSaveStateList() : LList()
{
   iLoadIndex = iLoadRecentIndex = -1;
   bLoadOnReset = bLoadRecent = FALSE;
   bLoadAccel = FALSE;
   ZeroMemory(cFileName,sizeof(cFileName));
}
//---------------------------------------------------------------------------
LSaveStateList::~LSaveStateList()
{
   Clear();
}
//---------------------------------------------------------------------------
void LSaveStateList::DeleteElem(LPVOID ele)
{
   if(ele != NULL)
       delete (SSFILE *)ele;
}
//---------------------------------------------------------------------------
BOOL LSaveStateList::set_LoadIndex(int index)
{
	LPSSFILE p;

	p = (LPSSFILE)GetItem(index+1);
   if(p == NULL)
   	return FALSE;
	iLoadIndex = index;
}
//---------------------------------------------------------------------------
int LSaveStateList::SortFunc(LPVOID ele0,LPVOID ele1)
{
	LPSSFILE a,b;
   char s[50],s1[50];

   a = (LPSSFILE)ele0;
   b = (LPSSFILE)ele1;
	wsprintf(s,"%04d%02d%02d%02d%02d%02d",a->time.wYear,a->time.wMonth,a->time.wDay,
   	a->time.wHour,a->time.wMinute,a->time.wSecond);
	wsprintf(s1,"%04d%02d%02d%02d%02d%02d",b->time.wYear,b->time.wMonth,b->time.wDay,
   	b->time.wHour,b->time.wMinute,b->time.wSecond);
   return lstrcmpi(s,s1) * -1;
}
//---------------------------------------------------------------------------
BOOL LSaveStateList::RebuildAccelerator()
{
   LString s,s1;
   int i,i1,i3;
   LPSSFILE p;
   DWORD dwPos;
   LPACCEL accelBuffer;
   HACCEL temphAccel;

   Sort(0,Count(),SortFunc);
   bLoadAccel = FALSE;
   iLoadRecentIndex = -1;
	p = (LPSSFILE)GetFirstItem(&dwPos);
	if(p != NULL){
       iLoadRecentIndex = 0;
   }
	if(ds.accel != NULL)
		DestroyAcceleratorTable(ds.accel);
   if((ds.accel = LoadAccelerators(hInst,MAKEINTRESOURCE(111))) == NULL || p == NULL)
       return FALSE;
   if((i = CopyAcceleratorTable(ds.accel,NULL,0)) < 1)
       return FALSE;
   i1 = (int)Count();
	if(i1 > 10)
   	i1 = 10;
   if((accelBuffer = (LPACCEL)GlobalAlloc(GPTR,sizeof(ACCEL) * (i+i1+1))) == NULL)
       return FALSE;
   CopyAcceleratorTable(ds.accel,accelBuffer,i);
   accelBuffer[i].fVirt = FVIRTKEY|FNOINVERT|FSHIFT;
   accelBuffer[i].key = VK_F7;
   accelBuffer[i++].cmd = (WORD)(ID_FILE_STATE_START);
   s.Capacity(200);
	for(i3 = i1 = 0;p != NULL && i3 < 10;i1++){
       if(i1){
           wsprintf(s.c_str(),"%04d%02d%02d%02d%02d%02d",p->time.wYear,p->time.wMonth,p->time.wDay,
               p->time.wHour,p->time.wMinute,p->time.wSecond);
           accelBuffer[i].fVirt = FVIRTKEY|FNOINVERT|FCONTROL;
           accelBuffer[i].key = (WORD)(VK_F1+i3++);
           accelBuffer[i++].cmd = (WORD)(ID_FILE_STATE_START+i3);
       }
       p = (LPSSFILE)GetNextItem(&dwPos);
   }
   temphAccel = CreateAcceleratorTable(accelBuffer,i);
   if(temphAccel != NULL){
		DestroyAcceleratorTable(ds.accel);
		ds.accel = temphAccel;
       bLoadAccel = TRUE;
   }
   GlobalFree(accelBuffer);
   return TRUE;
}
//22-23   Time (5/6/5 bits, for hour/minutes/doubleseconds)
//24-25   Date (7/4/5 bits, for year-since-1980/month/day)
//---------------------------------------------------------------------------
BOOL LSaveStateList::Save(int index)
{
   LString nameFile;
   LPZIPFILEHEADER p1;
   LPSSFILE p;
   FILETIME fileTime;
   IMenu *menu,*menu1,*menu2;
   LSaveState saveState;

//	if(!nameFile.BuildFileName((char *)ds.get_FileName(),"sgs"))
//       return FALSE;
	nameFile.Length(MAX_PATH+1);
   if(!ds.BuildFileName(PT_SAVESTATE,nameFile.c_str(),MAX_PATH))
   	return FALSE;
   p = NULL;
   if(index == -1)
       index = (int)(Count() + 1);
   else{
       if((p = (LPSSFILE)GetItem(++index)) == NULL)
           index = (int)(Count() + 1);
   }
   if(index > 15)
       index = DeleteLast();
   saveState.set_File(nameFile.c_str());
   if((p1 = (LPZIPFILEHEADER)saveState.Save(NULL,9,index)) != NULL){
       if(p == NULL){
           p = new SSFILE[1];
           Add((LPVOID)p);
           p->index = index;
       }
       lstrcpy(p->fileName,p1->nameFile);
       DosDateTimeToFileTime(p1->m_uModDate,p1->m_uModTime,&fileTime);
       FileTimeToSystemTime(&fileTime,&p->time);
       menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
       if(menu && (menu1 = menu->GetSubMenu(0))){
           if((menu2 = menu1->GetSubMenu(POS_MENU_SAVESTATE))){
               menu2->EnableMenuItem(6,MF_BYPOSITION|MF_ENABLED);
               menu2->EnableMenuItem(7,MF_BYPOSITION|MF_ENABLED);
               menu2->EnableMenuItem(ID_FILE_STATE_RESET,MF_ENABLED);
               menu2->Release();
           }
           menu1->Release();
       }
       if(menu)
           menu->Release();
   }
   RebuildAccelerator();
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LSaveStateList::Load(int index)
{
   LString nameFile;
	LSaveState saveState;
   LPSSFILE p;

   if(index > 14)
       return FALSE;
   if((p = (LPSSFILE)GetItem(index+1)) == NULL)
       return FALSE;
   nameFile.Length(MAX_PATH+1);
   if(!ds.BuildFileName(PT_SAVESTATE,nameFile.c_str(),MAX_PATH))
   	return FALSE;
//   if(!nameFile.BuildFileName((char *)ds.get_FileName(),"sgs"))
//   	return FALSE;
   saveState.set_File(nameFile.c_str());
	if(!saveState.Load(NULL,p->index))
   	return FALSE;
   iLoadIndex = index;
   return TRUE;
}
//---------------------------------------------------------------------------
void LSaveStateList::Reset()
{
   LString nameFile;
   IMenu *menu,*menu1,*menu2;

   menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
   if(menu && (menu1 = menu->GetSubMenu(0))){
       if((menu2 = menu1->GetSubMenu(POS_MENU_SAVESTATE))){
           menu2->EnableMenuItem(6,MF_BYPOSITION|MF_GRAYED);
           menu2->EnableMenuItem(7,MF_BYPOSITION|MF_GRAYED);
           menu2->EnableMenuItem(ID_FILE_STATE_RESET,MF_GRAYED);
           menu2->Release();
       }
       menu1->Release();
   }
   if(menu)
       menu->Release();
	nameFile.Length(MAX_PATH+1);
   if(!ds.BuildFileName(PT_SAVESTATE,nameFile.c_str(),MAX_PATH))
   	return;
//   if(!nameFile.BuildFileName((char *)ds.get_FileName(),"sgs"))
//       return;
	iLoadIndex = -1;
   ZeroMemory(cFileName,sizeof(cFileName));
   DeleteFile(nameFile.c_str());
   RebuildAccelerator();
   Clear();
}
//---------------------------------------------------------------------------
BOOL LSaveStateList::OnLoadRom()
{
   LString nameFile,s1;
   int i;
   LPSSFILE p;
   IMenu *menu,*menu1,*menu2;
   LZipFile zipFile;
   LPZIPFILEHEADER p1;
   FILETIME fileTime;

   bLoadAccel = FALSE;
   iLoadRecentIndex = -1;
   menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
   if(menu && (menu1 = menu->GetSubMenu(0))){
       if((menu2 = menu1->GetSubMenu(POS_MENU_SAVESTATE))){
           menu2->EnableMenuItem(6,MF_BYPOSITION|MF_GRAYED);
           menu2->EnableMenuItem(7,MF_BYPOSITION|MF_GRAYED);
           menu2->EnableMenuItem(ID_FILE_STATE_RESET,MF_GRAYED);
           menu2->Release();
       }
       menu1->Release();
   }
   if(menu)
       menu->Release();
   Clear();
	nameFile.Length(MAX_PATH+1);
   if(!ds.BuildFileName(PT_SAVESTATE,nameFile.c_str(),MAX_PATH))
   	return FALSE;

//   if(!nameFile.BuildFileName((char *)ds.get_FileName(),"sgs"))
//       return;
   if(!zipFile.Open(nameFile.c_str(),FALSE))
       return FALSE;
   zipFile.Rebuild();
   zipFile.Close();
   if(!zipFile.Open(nameFile.c_str(),FALSE))
       return FALSE;
   for(i=0;i< (int)zipFile.Count();i++){
       p1 = zipFile.GetZipFileHeader(i+1);
       s1 = p1->nameFile;
       s1.LowerCase();
       if(s1.Pos(".sgf") > 0){
           if((p = new SSFILE[1]) != NULL){
               lstrcpyn(p->fileName,s1.c_str(),12);
               DosDateTimeToFileTime(p1->m_uModDate,p1->m_uModTime,&fileTime);
               FileTimeToSystemTime(&fileTime,&p->time);
               p->index = i+1;
               Add((LPVOID)p);
           }
       }
   }
   zipFile.Close();
   if(Count() > 0){
       menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
       if(menu && (menu1 = menu->GetSubMenu(0))){
           if((menu2 = menu1->GetSubMenu(POS_MENU_SAVESTATE))){
               menu2->EnableMenuItem(6,MF_BYPOSITION|MF_ENABLED);
               menu2->EnableMenuItem(7,MF_BYPOSITION|MF_ENABLED);
               menu2->EnableMenuItem(ID_FILE_STATE_RESET,MF_ENABLED);
               menu2->Release();
           }
           menu1->Release();
       }
       if(menu)
           menu->Release();
   }
	if(lstrcmpi(cFileName,nameFile.c_str()))
   	iLoadIndex = -1;
   lstrcpy(cFileName,nameFile.c_str());
   RebuildAccelerator();
   if(iLoadRecentIndex < 0){
       iLoadIndex = -1;
       return TRUE;
   }
   i = -1;
   if(bLoadRecent && iLoadIndex == -1)
       i = iLoadRecentIndex;
   if(iLoadIndex != -1)
       i = iLoadIndex;
   if(i < 0)
       return TRUE;
   if(!Load(i))
   	return FALSE;
   if(!bLoadOnReset)
   	iLoadIndex = -1;
   return TRUE;
}
//---------------------------------------------------------------------------
int LSaveStateList::DeleteLast()
{
   LString s,s1;
   LPSSFILE p;
   DWORD dwPos;

   if(Count() < 1)
       return 1;
   p = (LPSSFILE)GetLastItem(&dwPos);
   Delete(Count());
   return atoi(p->fileName);
}
//---------------------------------------------------------------------------
BOOL LSaveStateList::OnInitMenu(IMenu *menu,BOOL bSave)
{
   int i,i1;
   MENUITEMINFO mii;
   DWORD dwPos;
   char s[50],s1[50];
   LPSSFILE p;
   WORD wID;

   i = menu->GetMenuItemCount()-1;
   for(;i>=0;i--)
       menu->DeleteMenu(i,MF_BYPOSITION);
   i = 0;
   ZeroMemory(&mii,sizeof(MENUITEMINFO));
   mii.cbSize = sizeof(MENUITEMINFO);
   if(Count() < 1)
       return FALSE;
   p = (LPSSFILE)GetFirstItem(&dwPos);
   wID = (WORD)(ID_FILE_STATE_START + (bSave ? 0xF : 0));
   i1 = 1;
   while(p != NULL && wID <= ID_FILE_STATE_END){
       mii.fMask = MIIM_TYPE|MIIM_STATE|MIIM_ID;
       mii.fType = MFT_STRING;
       mii.fState = MFS_ENABLED;
       mii.dwItemData = i;
       GetDateFormat(LOCALE_SYSTEM_DEFAULT,DATE_SHORTDATE,&p->time,NULL,s1,30);
       lstrcpy(s,s1);
       lstrcat(s,"  ");
       GetTimeFormat(LOCALE_SYSTEM_DEFAULT,LOCALE_NOUSEROVERRIDE,&p->time,NULL,s1,30);
       lstrcat(s,s1);
       if(bLoadAccel && !bSave){
           if(iLoadRecentIndex == i)
               lstrcat(s,"\tShift+F7");
           else if(i < 11){
				wsprintf(s1,"\tCtrl+F%d",i1++);
               lstrcat(s,s1);
           }
       }
       mii.dwTypeData = s;
       mii.wID = wID++;
       menu->InsertMenuItem(i++,TRUE,&mii);
       p = (LPSSFILE)GetNextItem(&dwPos);
   }
   return TRUE;
}

