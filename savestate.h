#include "ideastypes.h"
#include "fstream.h"
#include "zipfile.h"
#include "llist.h"
#include "lmenu.h"

//---------------------------------------------------------------------------
#ifndef __savestateH__
#define __savestateH__

#define POS_MENU_SAVESTATE 11

typedef void WINAPI (*LPSAVESTATECALLBACK)(int);
//---------------------------------------------------------------------------
typedef struct{
   SYSTEMTIME time;
   char fileName[12];
   int index;
} SSFILE,*LPSSFILE;
//---------------------------------------------------------------------------
class LSaveState
{
public:
	LSaveState();
   ~LSaveState();
   LPVOID Save(LPSAVESTATECALLBACK pCallBack = NULL,int iLevel = 2,int index = -1);
   BOOL Load(LPSAVESTATECALLBACK pCallBack,UINT index);
   void set_IndexMax(UINT u){IndexMax = u;};
   UINT get_IndexMax(){return IndexMax;};
   void set_CurrentIndex(UINT u){Index = u;};
	UINT get_CurrentIndex(){return Index;};
   void Reset();
   UINT Count(){return zipFile.Count();};
   BOOL set_File(const char *fileName);
protected:
//---------------------------------------------------------------------------
	BOOL Init(BOOL bOpenAlways = FALSE);
//---------------------------------------------------------------------------
	LZipFile zipFile;
   LMemoryFile *pMemoryFile;
   UINT Index,IndexMax;
   BOOL bUseFile;
   char cFileName[MAX_PATH+1];
};
//---------------------------------------------------------------------------
class LSaveStateList : public LList
{
public:
   LSaveStateList();
   ~LSaveStateList();
   void Reset();
   BOOL Load(int index);
   BOOL Save(int index);
   BOOL OnInitMenu(IMenu *menu,BOOL bSave);
   BOOL OnLoadRom();
   inline BOOL get_LoadRecent(){return bLoadRecent;};
   inline BOOL get_LoadOnReset(){return bLoadOnReset;};
   inline void set_LoadOnReset(BOOL a)
   {
   	if(a != bLoadOnReset){
       	bLoadOnReset = a;
           iLoadIndex = -1;
       };
   }
   inline void set_LoadRecent(BOOL a){bLoadRecent = a;};
  	BOOL set_LoadIndex(int index);
   static int SortFunc(LPVOID ele0,LPVOID ele1);
protected:
   BOOL RebuildAccelerator();
   virtual void DeleteElem(LPVOID ele);
   int DeleteLast();
   int iLoadRecentIndex,iLoadIndex;
   BOOL bLoadRecent,bLoadAccel,bLoadOnReset;
   char cFileName[MAX_PATH+1];
};

#endif











