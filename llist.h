//---------------------------------------------------------------------------
#ifndef __llistH__
#define __llistH__
//---------------------------------------------------------------------------
#define LE_ALLOCMEM 1
//---------------------------------------------------------------------------
typedef int (*LPLISTSORTFNC)(LPVOID ele1,LPVOID ele2);
typedef BOOL (*LPLISTENUMFNC)(LPVOID ele,DWORD index,LPARAM lParam);

class LList
{
protected:
   struct elem_list
   {
       LPVOID Ele;
       elem_list *Next,*Last;
   public:
       elem_list(LPVOID ele = NULL,LPVOID next=NULL,LPVOID last=NULL);
   };
   virtual void DeleteElem(LPVOID ele);
   elem_list *CercaItem(DWORD item);
   elem_list *First,*Last;
   DWORD nCount,iErrore,maxElem;
public:
   LList(DWORD dw = 0xFFFFFFFF);
   virtual ~LList();
   virtual inline DWORD Count(){return nCount;};
   virtual void Clear();
   virtual BOOL Add(LPVOID ele);
   inline DWORD GetLastError(){return iErrore;};
   inline void SetLastError(DWORD err){iErrore = err;};
   virtual LPVOID GetItem(DWORD item);
   virtual BOOL Delete(DWORD item,BOOL bFlag = TRUE);
   virtual BOOL Delete(LPVOID ele,BOOL bFlag = TRUE);
   elem_list *Remove(DWORD item);
   BOOL SetItem(DWORD item,LPVOID ele);
   virtual LPVOID GetFirstItem(LPDWORD pos);
   virtual LPVOID GetNextItem(LPDWORD pos);
   virtual LPVOID GetLastItem(LPDWORD pos);
   virtual LPVOID GetPrevItem(LPDWORD pos);
   DWORD IndexFromEle(LPVOID ele);
   void Sort(DWORD start,DWORD end,LPLISTSORTFNC sortFunction);
   virtual BOOL Enum(LPLISTENUMFNC fnc,LPARAM lParam);
   BOOL RemoveInternal(LPVOID p,BOOL bFlag = FALSE);
   inline void set_MaxElem(DWORD dw){maxElem = dw;};
   inline DWORD get_MaxElem(){return maxElem;};
};
#endif
