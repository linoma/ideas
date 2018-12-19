#include "ideastypes.h"
#include "llist.h"
#include "lstring.h"

//---------------------------------------------------------------------------
#ifndef __languageH__
#define __languageH__
//---------------------------------------------------------------------------
class LLanguageItem
{
public:
	LLanguageItem();
   ~LLanguageItem();
   BOOL Enable(BOOL bFlag);
   BOOL Load(const char *dll);
   BOOL Unload();
   inline const char* get_Name(){return langName.c_str();};
   inline WORD get_CodePage(){return wCodePage;};
   inline WORD get_Id(){return wLanguageId;};
   inline const char *get_LibraryName(){return pathLibrary.c_str();};
   inline HINSTANCE get_Lib(){return hLib;};
protected:
	LString pathLibrary,langName;
   HINSTANCE hLib;
   char isReload;
   WORD wLanguageId,wCodePage;
};
//---------------------------------------------------------------------------
class LLanguageManager : public LList
{
public:
	LLanguageManager();
   ~LLanguageManager();
   BOOL Add(LLanguageItem *p);
   BOOL OnInitMenu(HMENU menu);
   BOOL OnEnable(DWORD dwData);
   BOOL Init();
   BOOL InsertMenu(HMENU menu,WORD wId);
   LLanguageItem *Find(DWORD dwData);
   HINSTANCE get_CurrentLib();
   WORD get_CurrentId(){if(pActiveLanguageItem != NULL) return pActiveLanguageItem->get_Id();return 0x0409;};
   BOOL is_DefaultLanguage(WORD wId);
   HINSTANCE get_LibFromResource(WORD wID,LPCTSTR lpType);   
protected:
	void DeleteElem(LPVOID ele);
   LLanguageItem *pActiveLanguageItem;
};

extern LLanguageManager langManager;
#endif
