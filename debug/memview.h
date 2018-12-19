#include "ideastypes.h"
#include "dstype.h"
#include "lwnd.h"
#include "llist.h"

//---------------------------------------------------------------------------
#ifndef memviewH
#define memviewH

#define MAX_BOOKMARKMEM	20
#ifdef _DEBUG

//---------------------------------------------------------------------------
class LMemPage
{
public:
	LMemPage();
   ~LMemPage();
   inline int get_ScrollPos(){return yScroll;};
   inline int get_Item(){return nItem;};
   inline void set_ScrollPos(int i){yScroll = i;};
   void set_Item(int i);
   inline int get_Mode(){return nMode;};
   inline void set_Mode(int i){nMode = i;};
   void del_BookMark(int index);
   inline BOOL AddBookMark(u32 value){
   	if(nBMMem < MAX_BOOKMARKMEM){
       	bookmarkmem[nBMMem++] = value;
           return TRUE;
       }
       return FALSE;
   }
   BOOL set_Sel(u32 value);
   BOOL get_Sel(u32 *u0,u32 *u1);
   inline BOOL get_Freeze(){return bFreeze;};
   inline void set_Freeze(BOOL value){bFreeze = value;};
	inline int get_BookMarkCount(){return nBMMem;};
   u32 get_BookMark(int index){if(index < nBMMem) return bookmarkmem[index]; return 0;};
   void set_FollowAddress(char *string = NULL);
   inline BOOL get_FollowMe(){return bFollowMe;};
	inline char *get_FollowAddress(){return cFollowMe;};
protected:
   u32 bookmarkmem[MAX_BOOKMARKMEM],selStart,selEnd;
	int nMode,yScroll,nItem,nBMMem;
   POINT ptMenu;
   BOOL bFreeze,bFollowMe;
   char cFollowMe[10];
};
//---------------------------------------------------------------------------
class LMemPageList : public LList
{
public:
	LMemPageList();
   ~LMemPageList();
   BOOL Init();
	LMemPage *get_ActivePage();
   BOOL set_ActivePage(int index);
	void DeleteElem(LPVOID p){if(p != NULL) delete (LMemPage *)p;};
protected:
   int nActivePage;
};
//---------------------------------------------------------------------------
class LMemCBView : public LCBView
{
public:
	LMemCBView();
   ~LMemCBView();
	LRESULT OnWindowProcEdit(UINT uMsg,WPARAM wParam,LPARAM lParam);
	LRESULT OnWindowProcLB(UINT uMsg,WPARAM wParam,LPARAM lParam);
};
//---------------------------------------------------------------------------
class LMemoryView : public LWnd
{
public:
	LMemoryView();
   ~LMemoryView();
   void Init(LPVOID p);
   BOOL Destroy();
   void Reset();
	BOOL NewPage();
	LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   void OnVScroll(WPARAM wParam,LPARAM lParam);
   void Update(BOOL bInvalidate = TRUE);
   void OnCommand(WORD wNotifyCode,WORD wID,HWND hwnd);
   void UpdateScrollbar(BOOL bRepos);
	void OnGotoAddress(DWORD dw,BOOL bAdd = TRUE,BOOL bRedraw = TRUE);
   void OnDeleteBookmarkMemory(int index);
	void set_CurrentAccess(u32 address,u32 access);
   void ReSize(int x,int y,int w,int h);
   BOOL get_MemoryBookmark(int index,u32 *p){
   	LMemPage *pPage;
   	if(p == NULL || (pPage = memPages.get_ActivePage()) == NULL)
       	return FALSE;
       if(index >= pPage->get_BookMarkCount())
       	return FALSE;
		*p = pPage->get_BookMark(index);
       return TRUE;
   }
   BOOL set_ActivePage(int index);
   LMemPage *get_ActivePage(){return memPages.get_ActivePage();};
   void OnChangeCPU();
protected:
	BOOL PointToAddress(POINT &pt,u32 *address);
   void EnterEditMemory(POINT &pt);
   void DoScrolling(POINT pt,RECT rc);
   void OnTimer(WPARAM wParam,LPARAM lParam);
	BOOL get_ItemsPage(LPSIZE s);
   BOOL get_ItemSize(LPSIZE s);
   BOOL get_FontSize();
   void OnPaint(HDC hdc,RECT &rcClip);
   void OnKeyDown(WPARAM wParam,LPARAM lParam);
   void OnMenuSelect(WORD wID);
   LMemCBView memCBView;
	LMemPageList memPages;
   WNDPROC oldWndProcCB;
   LPVOID parent;
   HWND m_hWndCB,m_hWndEditCB;
   HFONT hFont;
   SIZE sz,szLetterFont;
   char *lpszText;
   u32 editAddress,crAddress[10][3],dwInterval;
   u8 editBits;
   BOOL bEditMemory;
   POINT ptPosCaret,ptMenu;
   HDC hDC;
   HBITMAP hBitmap;
};
#endif

#endif
