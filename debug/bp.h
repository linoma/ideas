#include "ideastypes.h"
#include "llist.h"
#include "dsmem.h"
#include "fstream.h"
#include "lwnd.h"
#include "lstring.h"
#include "cpu_0.h"
//---------------------------------------------------------------------------
#ifndef __bpH__
#define __bpH__

#define BT_PROGRAM     	0
#define BT_MEMORY			1

#define CC_EQ      1
#define CC_NE      2
#define CC_GT      3
#define CC_GE      4
#define CC_LT      5
#define CC_LE      6

#if defined(_DEBPRO2)
//---------------------------------------------------------------------------
class LBreakPoint
{
public:
	LBreakPoint();
   ~LBreakPoint();
   inline BOOL is_Enable(){return Enable;};
   inline unsigned long get_Address(){return Address;};
   inline int get_Type(){return Type;};
   void get_Condition(char *c){CopyMemory(c,Condition,sizeof(Condition));};
   void set_Condition(char *c){CopyMemory(Condition,c,sizeof(Condition));};
   void get_Description(char *c){lstrcpy(c,Description);};
   void set_Description(char *c){lstrcpy(Description,c);};
   void set_Type(int type){Type = (unsigned char )type;};
   BOOL Read(LFile *pFile,int ver = 0);
   BOOL Write(LFile *pFile);
   inline BOOL is_Write(){return Condition[0] & 1 ? TRUE : FALSE;};
   inline BOOL is_Read(){return Condition[0] & 2 ? TRUE : FALSE;};
	inline BOOL is_Modify(){return Condition[0] & 4 ? TRUE : FALSE;};
   inline BOOL is_Break(){return Condition[0] & 8 ? TRUE : FALSE;};
   inline int get_Access(){return Condition[0] >> 4;};
	void set_Write(BOOL b = TRUE){if(b) Condition[0] |= 1; else Condition[0] &= ~1;};
	void set_Read(BOOL b = TRUE){if(b) Condition[0] |= 2; else Condition[0] &= ~2;};
	void set_Modify(BOOL b = TRUE){if(b) Condition[0] |= 4; else Condition[0] &= ~4;};
	void set_Break(BOOL b = TRUE){if(b) Condition[0] |= 8; else Condition[0] &= ~8;};
	void set_Access(char f){Condition[0] &= (char)~0xF0;Condition[0] |= (char)(f << 4);};
   void set_Flags(char f){Condition[0] = f;};
   inline void set_Address(unsigned long l){Address = l;};
   inline void set_Address2(unsigned long l){*((unsigned long *)(Condition+4)) = l;Condition[1] |= 0x80;};
   inline unsigned long get_Address2(){return *((unsigned long *)(Condition+4));};
   inline void set_Enable(BOOL b = TRUE){Enable = (unsigned char)b;};
   inline BOOL has_Range(){if(Type == BT_MEMORY && (Condition[1] & 0x80)) return TRUE; return FALSE;};
   inline unsigned long get_PassCount(){return PassCount;};
   inline unsigned long get_InternalPassCount(){return int_PassCount2;};
   inline void set_InternalPassCount(){int_PassCount2 = 0;};   
   inline void set_Processor(int value){Condition[1] &= (char)~0xF;Condition[1] |= (char)(value & 0xF);};
   inline int get_Processor(){return (Condition[1] & 0xF);};
   void set_PassCount(unsigned long p){PassCount = p;int_PassCount = 0;};
   BOOL Check(unsigned long a0,int accessMode,IARM7 *cpu,unsigned long data = 0);
	LString ConditionToString(unsigned char type);
   void StringToCondition(char *sm,unsigned char type = BT_PROGRAM);
   inline void Reset(){int_PassCount = int_PassCount2 = 0;};
protected:
	u8 ConditionToValue(int *rd,u8 *cond,u32 *value,unsigned char type = BT_PROGRAM);
	unsigned char Type,Enable;
   unsigned long Address,PassCount,int_PassCount,int_PassCount2;
	char Condition[30],Description[100];
};
//---------------------------------------------------------------------------
class LListBreakPoint : public LList,public LWnd
{
public:
	LListBreakPoint();
   ~LListBreakPoint();
   LBreakPoint *Add(unsigned long address,int type = BT_PROGRAM);
   BOOL Delete(DWORD item,BOOL bFlag = TRUE);
   BOOL Add(LBreakPoint *p);
   LBreakPoint *Check(unsigned long address,int accessMode,IARM7 *cpu,unsigned long data = 0);
   LBreakPoint *Find(unsigned long address,int type = BT_PROGRAM);
   void Reset();
   BOOL Save(const char *lpFileName,BOOL bForce = FALSE);
   BOOL Load(const char *lpFileName);
   BOOL OnShowList(HWND hwndLV,int type = BT_PROGRAM);
   LONG OnNotify(NM_LISTVIEW *p);
   LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
 	BOOL UpdateList(HWND hwndLV,int type);
   void Select(HWND hwndLV,LBreakPoint *p,BOOL bShow = TRUE);
   void Clear();
   inline BOOL is_Modified(){return bModified;};
protected:
	void DeleteElem(LPVOID p);
   BOOL bModified;
   int currentType;
   HMENU hMenu;
   LString fileName;
};
#endif

#endif
