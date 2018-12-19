#include "ideastypes.h"
#include "ldlg.h"
#include "dstype.h"
#include "llist.h"
#include "elf.h"
#include "lstring.h"

#ifdef _DEBPRO
//---------------------------------------------------------------------------
#ifndef __inspectorH__
#define __inspectorH__
//---------------------------------------------------------------------------
struct IARM7;
class LInspectorList;

typedef struct {
   Type *type;
   u32 location;
} ITEM,*LPITEM;
//---------------------------------------------------------------------------
class LInspectorDlg : public LDlg
{
public:
   LInspectorDlg(const char *n,elf_Object *t,Symbol *s,Function *f,IARM7 *p,LInspectorList *p1);
   LInspectorDlg(const char *n,Type *t,u32 adr,Function *f,IARM7 *p,LInspectorList *p1);
   ~LInspectorDlg();
   BOOL Show(HWND parent = NULL);
   LRESULT OnWindowProcListView1(UINT uMsg,WPARAM wParam,LPARAM lParam);
   void Update();
protected:
   LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   static LRESULT CALLBACK WindowProcListView1(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
   void OnSize(WPARAM wParam,LPARAM lParam);
   void RecalWindowSize();
   BOOL OnInitDialog();
   void OnListView1DrawItem(LPDRAWITEMSTRUCT p);
   void OnListView1Notify(NM_LISTVIEW *p);
   void OnButton2Click();
   void ShowEditButton();
   LString get_Value(ITEM *item);
   LString get_Type(ITEM *item);
   u32 get_Location(ELFBlock *location);
   RECT rcClient,rcWin;
   Symbol *sym;
   Type *type;
   elf_Object *obj;
   Function *func;
   SIZE szFont;
   WNDPROC oldListView1WndProc;
   IARM7 *cpu;
   LInspectorList *parent;
   u32 baseAdr,minRange,maxRange;
   LString name;
   BOOL bExternal,bUseReg;
};
//---------------------------------------------------------------------------
class LInspectorList : public LList
{
public:
   LInspectorList();
   ~LInspectorList();
   BOOL Add(const char *name,u32 adr,IARM7 *cpu);
   BOOL IsDialogMessage(LPMSG p);
   BOOL Add(const char *name,Type *t,u32 adr,Function *f,IARM7 *cpu);
   void Update();
   void Reset();
protected:
   void DeleteElem(LPVOID ele);
};

#endif 

#endif //_DEBPRO


