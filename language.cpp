#include "language.h"
#include "lregkey.h"

LLanguageManager langManager;
//---------------------------------------------------------------------------
LLanguageItem::LLanguageItem()
{
	pathLibrary = "";
   langName = "";
   hLib = NULL;
   isReload = FALSE;
   wLanguageId = 0;
   wCodePage = 0;
}
//---------------------------------------------------------------------------
LLanguageItem::~LLanguageItem()
{
	Unload();
}
//---------------------------------------------------------------------------
BOOL LLanguageItem::Enable(BOOL bFlag)
{
	return TRUE;
}
//---------------------------------------------------------------------------
static BOOL CALLBACK EnumResLangProc(HANDLE hModule,LPCTSTR lpszType,LPCTSTR lpszName,WORD wIDLanguage,LONG lParam)
{
	if(lParam != NULL)
		((LPWORD)lParam)[0] = wIDLanguage;
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LLanguageItem::Load(const char *dll)
{
#ifdef __WIN32__
/*   DWORD dwLen,lpPointer;
   LPBYTE lpBuf;
   UINT dwByte;
	BOOL res;*/
	WORD wId,wCp;
	char *p;
	LString name;
	int len;

   if(hLib != NULL)
       return TRUE;
/*   if(wLanguageId == 0){
		if(dll == NULL || lstrlen(dll) == 0)
   		return FALSE;
   	if((dwLen = GetFileVersionInfoSize((LPTSTR)dll,&dwLen)) < 1)
       	return FALSE;
   	if((lpBuf = new BYTE[dwLen+10]) == NULL)
       	return FALSE;
       res = FALSE;
   	if(GetFileVersionInfo((LPTSTR)dll,0,dwLen,(LPVOID)lpBuf)){
       	if(VerQueryValue((LPVOID)lpBuf,"\\VarFileInfo\\Translation",(LPVOID *)&lpPointer,&dwByte)){
           	wId = ((LPWORD)lpPointer)[0];
               wCp = ((LPWORD)lpPointer)[1];
           	name.Length(251);
           	if(VerLanguageName(wId,name.c_str(),250))
           		res = TRUE;
       	}
   	}
  		delete []lpBuf;

		if(!res)
   		return FALSE;
   }*/
   if(dll == NULL || lstrlen(dll) == 0){
   	if(pathLibrary.IsEmpty())
   		return FALSE;
   	p = pathLibrary.c_str();
   }
   else
   	p = (char *)dll;
   hLib = GetModuleHandle(p);
   if(hLib != NULL)
       isReload = TRUE;
   else{
   	isReload = FALSE;
   	hLib = LoadLibrary(p);
   	if(hLib == NULL)
   		return FALSE;
   }
//ex_LLanguageItem:
   if(wLanguageId == 0){
		if(!EnumResourceLanguages(hLib,RT_MENU,MAKEINTRESOURCE(1000),(ENUMRESLANGPROC)EnumResLangProc,(LONG)&wId)){
       	Unload();
           return FALSE;
       }
   	len = GetLocaleInfo(MAKELCID(wId,SORT_DEFAULT),LOCALE_NOUSEROVERRIDE|LOCALE_SLANGUAGE,(LPTSTR)name.c_str(),0);
		if(len != 0){
       	name.Length(len + 1);
			GetLocaleInfo(MAKELCID(wId,SORT_DEFAULT),LOCALE_NOUSEROVERRIDE|LOCALE_SLANGUAGE,(LPTSTR)name.c_str(),len);
       }
       else
       	name = "Uknown";
   	langName = name;
   	wLanguageId = wId;
   	wCodePage = wCp;
   }
	pathLibrary = p;
	return TRUE;
#else
	return FALSE;
#endif	
}
//---------------------------------------------------------------------------
BOOL LLanguageItem::Unload()
{
   if(!isReload){
       if(hLib != NULL)
           FreeLibrary(hLib);
   }
	hLib = NULL;
   isReload = FALSE;
	return TRUE;
}
//---------------------------------------------------------------------------
LLanguageManager::LLanguageManager() : LList()
{
	pActiveLanguageItem = NULL;
}
//---------------------------------------------------------------------------
LLanguageManager::~LLanguageManager()
{
	LRegKey key;
   DWORD dw;

   if(pActiveLanguageItem != NULL && key.Open("Software\\iDeaS")){
   	dw = MAKELPARAM(pActiveLanguageItem->get_Id(),0);
   	key.WriteLong("Language",dw);
   	key.Close();
   }
	Clear();
}
//---------------------------------------------------------------------------
BOOL LLanguageManager::Init()
{
#ifdef __WIN32__
   LString curDir,prgDir,c,cName;
	WIN32_FIND_DATA wf;
	HANDLE handle;
  	LLanguageItem *p;
   BOOL bFlag;
	WORD wId;
 	LRegKey key;

	c.Length(MAX_PATH+1);
   GetModuleFileName(NULL,c.c_str(),MAX_PATH);
   c = c.Path();
   if(c[c.Length()] != DPC_PATH)
   	c += DPC_PATH;
   c += "Language";
   ::CreateDirectory(c.c_str(),NULL);
   Clear();
	pActiveLanguageItem = NULL;
   curDir.Capacity(MAX_PATH+1);
   prgDir.Capacity(MAX_PATH+1);
   ::GetModuleFileName(NULL,prgDir.c_str(),MAX_PATH);
   if((p = new LLanguageItem()) != NULL){
   	if(p->Load(prgDir.c_str())){
   		if(!Add(p))
   			delete p;
   	}
   }
   prgDir = prgDir.Path();
   prgDir += DPC_PATH;
   prgDir += "Language";
   ::GetCurrentDirectory(MAX_PATH,curDir.c_str());
   ::SetCurrentDirectory(prgDir.c_str());
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
       if((p = new LLanguageItem()) != NULL){
       	if(p->Load(c.c_str())){
       		if(!Add(p))
           		delete p;
           }
           else
           	delete p;
       }
       bFlag = ::FindNextFile(handle,&wf);
   }
   if(handle != INVALID_HANDLE_VALUE)
       FindClose(handle);
   SetCurrentDirectory(curDir.c_str());
   wId = 0;
   if(key.Open("Software\\iDeaS")){
   	if((wId = (WORD)key.ReadLong("Language",0)) != 0){
			if(Find(wId) == NULL && Find(PRIMARYLANGID(wId)) == NULL)
           	wId = 0;
       }
   	key.Close();
   }
	if(wId == 0)
		wId = LANGIDFROMLCID(GetSystemDefaultLCID());
   if((p = Find(wId)) != NULL || (p = Find(PRIMARYLANGID(wId))) != NULL)
   	pActiveLanguageItem = p;
   else
		pActiveLanguageItem = (LLanguageItem *)GetItem(1);
	::SetThreadLocale(MAKELANGID(pActiveLanguageItem->get_Id(),SORT_DEFAULT));
#endif
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LLanguageManager::InsertMenu(HMENU menu,WORD wId)
{
	MENUITEMINFO mii={0};
   HMENU hSubMenu;

	if(menu == NULL)
  		return FALSE;
	mii.cbSize = sizeof(MENUITEMINFO);
	if(Count() > 1){
  		hSubMenu = CreatePopupMenu();
		if(hSubMenu != NULL){
   		mii.fMask = MIIM_SUBMENU|MIIM_TYPE|MIIM_DATA;
   		mii.fType = MFT_STRING;
			mii.hSubMenu = hSubMenu;
   		mii.dwTypeData = "Language";
   		mii.dwItemData = 0x3333;
   		InsertMenuItem(menu,wId,FALSE,&mii);
       }
   }
   mii.fMask = MIIM_TYPE;
   mii.fType = MFT_SEPARATOR;
	mii.hSubMenu = NULL;
   InsertMenuItem(menu,wId,FALSE,&mii);
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LLanguageManager::Add(LLanguageItem *p)
{
   if(Find(p->get_Id()) != NULL)
   	return FALSE;
	return LList::Add((LPVOID)p);
}
//---------------------------------------------------------------------------
void LLanguageManager::DeleteElem(LPVOID ele)
{
	if(ele != NULL)
   	delete ((LLanguageItem *)ele);
}
//---------------------------------------------------------------------------
BOOL LLanguageManager::is_DefaultLanguage(WORD wId)
{
	LLanguageItem *p;

   p = (LLanguageItem  *)GetItem(1);
   if(p == NULL)
   	return FALSE;
   if(p->get_Id() == wId)
   	return TRUE;
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL LLanguageManager::OnInitMenu(HMENU menu)
{
	DWORD dwPos;
	LLanguageItem *p;
	MENUITEMINFO mii;
	int i;

   i = GetMenuItemCount(menu);
   for(;i>0;i--)
   	DeleteMenu(menu,0,MF_BYPOSITION);
	i = 1;
   p = (LLanguageItem *)GetFirstItem(&dwPos);
   while(p != NULL){
		memset(&mii,0,sizeof(MENUITEMINFO));
       mii.cbSize = sizeof(MENUITEMINFO);
       mii.fMask = MIIM_TYPE|MIIM_ID|MIIM_DATA;
       mii.fType = MFT_STRING;
       mii.wID = i++;
       mii.dwTypeData = (char *)p->get_Name();
       mii.dwItemData = p->get_Id();
		if(pActiveLanguageItem == p){
       	mii.fMask |= MIIM_STATE;
           mii.fState = MFS_CHECKED;
       }
       InsertMenuItem(menu,mii.wID,FALSE,&mii);
		p = (LLanguageItem *)GetNextItem(&dwPos);
   }
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LLanguageManager::OnEnable(DWORD dwData)
{
	LLanguageItem *p;

   if((p = Find(dwData)) == NULL)
   	return FALSE;
   if(!p->Load(p->get_LibraryName()))
   	return FALSE;
	pActiveLanguageItem = p;
   MessageBox(NULL,"You must to restart the emulator to apply the changes.","iDeaS Emulator",MB_ICONINFORMATION|MB_OK);
	return TRUE;
}
//---------------------------------------------------------------------------
LLanguageItem *LLanguageManager::Find(DWORD dwData)
{
	DWORD dwPos;
	LLanguageItem *p;

   p = (LLanguageItem *)GetFirstItem(&dwPos);
   while(p != NULL){
   	if(p->get_Id() == dwData)
       	return p;
		p = (LLanguageItem *)GetNextItem(&dwPos);
   }
   return NULL;
}
//---------------------------------------------------------------------------
HINSTANCE LLanguageManager::get_CurrentLib()
{
#ifdef __WIN32__
	HINSTANCE h;

	if(pActiveLanguageItem != NULL && (h = pActiveLanguageItem->get_Lib()) != NULL)
   	return h;
   return ::GetModuleHandle(NULL);
#else
	return NULL;
#endif
}
//---------------------------------------------------------------------------
HINSTANCE LLanguageManager::get_LibFromResource(WORD wID,LPCTSTR lpType)
{
#ifdef __WIN32__
	HINSTANCE h;

	if(pActiveLanguageItem != NULL && (h = pActiveLanguageItem->get_Lib()) != NULL){
   	if(::FindResource(h,MAKEINTRESOURCE(wID),lpType) != NULL)
       	return h;
	}
   return GetModuleHandle(NULL);
#else
	return NULL;
#endif
}
