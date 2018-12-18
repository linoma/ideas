#include "ideastypes.h"
#include "dstype.h"
#include "llist.h"
#include "ldlg.h"
#include "lstring.h"

//---------------------------------------------------------------------------
#ifndef __cheatsH__
#define __cheatsH__
//---------------------------------------------------------------------------
#define MAX_CHEATNAME  30
#define CHEAT_VERSION	11

enum cheatType{null = 0, CodeBreaker,ActionReplay,CodeBreakerEncrypt};

struct _cheatHeader;

typedef struct _cheatHeader * (*LPCHEATEVALUATEFUNC)(struct _cheatHeader *,u32,u8);
typedef void (*LPENUMITEMFUNC)(HWND,HTREEITEM,LPARAM);

typedef struct _cheatHeader{
   char descr[MAX_CHEATNAME + 1];
   char codeString[20];
   u8 Enable,code,skip,sort;
   cheatType type;
   u32 adr,value,adrEnd;
   LPCHEATEVALUATEFUNC pEvaluateFunc;
   LList *pList;
   struct _cheatHeader *pNext;
} CHEATHEADER,*LPCHEATHEADER;

typedef CHEATHEADER CODEBREAK;
typedef LPCHEATHEADER LPCODEBREAK;

typedef CHEATHEADER GAMESHARK;
typedef LPCHEATHEADER LPGAMESHARK;

typedef CHEATHEADER ACTIONREPLAY;
typedef LPCHEATHEADER LPACTIONREPLAY;

//---------------------------------------------------------------------------
class LCheatList : public LList
{
public:
   LCheatList(BOOL clone = TRUE);
   virtual ~LCheatList();
   void EvaluateCheat(u32 address,u8 accessMode);
   BOOL Add(LPCHEATHEADER ele);
   void Clear();
   BOOL Save(LStream *pFile);
   BOOL Load(LStream *pFile);
protected:
	void DeleteElem(LPVOID ele);
   u32 maxAdr,minAdr;
   BOOL bClone;
};
//---------------------------------------------------------------------------
class LCheatsManager : public LDlg,public LCheatList
{
public:
	LCheatsManager();
	virtual ~LCheatsManager();
   void Show(HWND p);
   BOOL Destroy();
	LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   LPCHEATHEADER AddActionReplay(const char *lpCode,const char *lpDescr,BOOL bCheck = TRUE);
   LPCHEATHEADER AddCodeBreaker(const char *lpCode,const char *lpDescr,LPCHEATHEADER prev = NULL,cheatType typeDef = CodeBreaker);
   void Enable(BOOL b){bEnable = b;ResetCheatSystem();Apply();};
   inline BOOL is_Enable(){return bEnable;};
   inline BOOL is_AutoLoad(){return bAutoLoad;};
   inline void AutoLoad(BOOL b){bAutoLoad = b;};
   BOOL Load(const char *fileName = NULL);
   void ResetCheatSystem();
   void write_hword(u32 address,u32 value);
   BOOL OnLoadRom();
   BOOL IsDialogMessage(LPMSG p);
	DWORD inline get_GameID(){return crc_value;};
#ifndef __WIN32__
	void OnDestroyParent();
#endif
protected:
   BOOL DecryptCodeBreaker(u32 *adr,u32 *val);
   s64 ror(s64 a,s64 b);
   s64 key(s64 a);
   BOOL scramble();
	static BOOL EnumCheatsReset(LPVOID ele,DWORD index,LPARAM lParam);
   static int sortCheatList(LPVOID ele,LPVOID ele1);
   void Apply();
   BOOL CreateTreeView();
	BOOL OnInitDialog(LPARAM lParam);
   BOOL OnSize(WPARAM wParam,int nWidth,int nHeight);
   BOOL Open(BOOL bFlag);
	void OnCommand(WORD wID,WORD wNotifyCode,HWND hwndFrom);
   HTREEITEM InsertNewCheat(const char *lpCode,const char *lpDescr,cheatType type,HTREEITEM hNewItem = NULL);
	void OnMenuSelect(WORD wID);
   void OnEnumItem(HWND hwnd,HTREEITEM hStart,LPENUMITEMFUNC lpfnc,LPARAM lParam,BOOL bEnumChild = TRUE);
   static void OnEnumItemEnableChanged(HWND hwnd,HTREEITEM hItem,LPARAM lParam);
   static void OnEnumItemSearchCheat(HWND hwnd,HTREEITEM hItem,LPARAM lParam);
   static void OnEnumItemSaveCheat(HWND hwnd,HTREEITEM hItem,LPARAM lParam);
   static void OnEnumItemSortCheat(HWND hwnd,HTREEITEM hItem,LPARAM lParam);
   void SortTreeView();
   BOOL OnGetMinxMax(LPMINMAXINFO p);
   void OnClickButton1();
	LString SearchCheatCode(const char *lpCode,int *len);
	void OnNewCheats(cheatType type);
   BOOL CheckCheatCode(u32 *x,u32 *y,const char *lpCode,cheatType type);
   void OnNotify(int id,LPNMHDR lParam);
   void DeleteCheat();
   void DeleteAllCheats();
   void OnSaveCheats();
	void OnMouseMove(WPARAM wParam,int xPos,int yPos);
	void OnLButtonUp(WPARAM wParam,int xPos,int yPos);
	enum cheatEditMode{null = 0, Insert,Edit};
   BOOL bModified,bEnable,bAutoLoad,bOpen,bDrag;
   u32 Key0,Key1,Key2,Key3,game_code,EncryptedArea[0x200],crc_value;
   RECT  rcClient,rcWin;
   cheatType newType;
   cheatEditMode editMode;
   HWND m_hTV;
   HIMAGELIST hilState,hilDrag;
   HTREEITEM hItemDrop,hItemDrag;
};

extern LCheatsManager cheatsManager;
#endif
