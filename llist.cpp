#include "ideastypes.h"
#include "llist.h"
//---------------------------------------------------------------------------
LList::LList(DWORD dw)
{
   nCount = 0;
   First = Last = NULL;
   iErrore = 0;
   maxElem = dw;
}
//---------------------------------------------------------------------------
LList::~LList()
{
   Clear();
}
//---------------------------------------------------------------------------
LList::elem_list::elem_list(LPVOID ele,LPVOID next,LPVOID last)
{
   Next = (elem_list *)next;
   Last = (elem_list *)last;
   Ele = ele;
}
//---------------------------------------------------------------------------
LPVOID LList::GetItem(DWORD item)
{
   elem_list *tmp;

   tmp = CercaItem(item);
   return tmp != NULL ? tmp->Ele : NULL;
}
//---------------------------------------------------------------------------
LList::elem_list * LList::CercaItem(DWORD item)
{
   elem_list *tmp;
   DWORD i;

   if(nCount == 0 || First == NULL || Last == NULL || item > nCount || item < 1)
       return NULL;
   tmp = First;
   for(i=1;tmp != NULL && i < item;i++)
       tmp = tmp->Next;
   return tmp;
}
//---------------------------------------------------------------------------
void LList::DeleteElem(LPVOID ele)
{
   if(ele != NULL)
       delete []((char *)ele);
}
//---------------------------------------------------------------------------
void LList::Clear()
{
   elem_list *tmp,*tmp1;

   tmp = First;
   while(tmp){
       tmp1 = tmp->Next;
       DeleteElem(tmp->Ele);
       delete tmp;
       tmp = tmp1;                                 
   }
   nCount = 0;
   First = Last = NULL;
}
//---------------------------------------------------------------------------
DWORD LList::IndexFromEle(LPVOID ele)
{
   DWORD i;
   elem_list *tmp;

   tmp = First;
   i = 1;
   while(tmp != NULL){
       if(tmp->Ele == ele)
           break;
       tmp = tmp->Next;
       i++;
   }
	return tmp == NULL ? (DWORD)-1 : i;
}
//---------------------------------------------------------------------------
BOOL LList::SetItem(DWORD item,LPVOID ele)
{
   elem_list *tmp;

   tmp = CercaItem(item);
   if(tmp ==  NULL)
       return FALSE;
   tmp->Ele = ele;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LList::RemoveInternal(LPVOID p,BOOL bFlag)
{
   LList::elem_list *tmp;

   if(p == NULL)
       return FALSE;
   tmp = (LList::elem_list *)p;
   if(tmp->Last)
       tmp->Last->Next = tmp->Next;
   if(tmp->Next)
       tmp->Next->Last = tmp->Last;
   if(tmp == First)
       First = tmp->Next;
   if(tmp == Last)
       Last = tmp->Last;
   nCount--;
   if(bFlag){
       DeleteElem(tmp->Ele);
       delete tmp;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
LList::elem_list *LList::Remove(DWORD item)
{
   elem_list *tmp;

   tmp = CercaItem(item);
   RemoveInternal(tmp);
   return tmp;
}
//---------------------------------------------------------------------------
BOOL LList::Delete(LPVOID ele,BOOL bFlag)
{
   DWORD item;

   item = IndexFromEle(ele);
   if(item == (DWORD)-1)
       return FALSE;
   return Delete(item,bFlag);
}
//---------------------------------------------------------------------------
BOOL LList::Delete(DWORD item,BOOL bFlag)
{
   elem_list *tmp;

   tmp = Remove(item);
   if(tmp == NULL)
       return FALSE;
   if(bFlag){
       DeleteElem(tmp->Ele);
       delete tmp;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LList::Add(LPVOID ele)
{
   elem_list *tmp;

   tmp = new elem_list(ele,NULL,Last);
   if(tmp == NULL){
       iErrore = LE_ALLOCMEM;
       return FALSE;
   }
   if(maxElem != (DWORD)-1 && nCount == maxElem)
       Delete(1);
   if(Last)
       Last->Next = tmp;
   Last = tmp;
   if(First == NULL)
       First = tmp;
   nCount++;
   return TRUE;
}
//---------------------------------------------------------------------------
LPVOID LList::GetLastItem(LPDWORD pos)
{
   elem_list *tmp;

   tmp = Last;
   if(tmp){
       *pos = (DWORD)tmp;
       return tmp->Ele;
   }
   else{
       *pos = 0;
       return NULL;
   }
}
//---------------------------------------------------------------------------
LPVOID LList::GetPrevItem(LPDWORD pos)
{
   LPVOID res;

   if(pos == NULL || *pos == 0)
       return NULL;
   *pos = (DWORD)((elem_list *)*pos)->Last;
   if(*pos)
       res = ((elem_list *)*pos)->Ele;
   else
       res = NULL;
   return res;
}
//---------------------------------------------------------------------------
LPVOID LList::GetFirstItem(LPDWORD pos)
{
   elem_list *tmp;

   tmp = First;
   if(tmp){
       *pos = (DWORD)tmp;
       return tmp->Ele;
   }
   else{
       *pos = 0;
       return NULL;
   }
}
//---------------------------------------------------------------------------
LPVOID LList::GetNextItem(LPDWORD pos)
{
   LPVOID res;

   if(pos == NULL || *pos == 0)
       return NULL;
   *pos = (DWORD)((elem_list *)*pos)->Next;
   if(*pos)
       res = ((elem_list *)*pos)->Ele;
   else
       res = NULL;
   return res;
}
//---------------------------------------------------------------------------
BOOL LList::Enum(LPLISTENUMFNC fnc,LPARAM lParam)
{
   DWORD i;
   elem_list *tmp;
   
   if(fnc == NULL)
       return FALSE;
   if(nCount < 1)
       return TRUE;
   tmp = First;
   i = 1;
   while(tmp != NULL){
       if(!fnc(tmp->Ele,i++,lParam))
           break;
       tmp = tmp->Next;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
void LList::Sort(DWORD start,DWORD end,LPLISTSORTFNC sortFunction)
{
   LPVOID p2;
   DWORD i,i1,i2;
   BOOL flag;
   elem_list *p,*p1;

   i1 = nCount;
   flag = TRUE;
   while(flag){
       flag = FALSE;
       for(i=1;i < i1;i++){
           p = CercaItem(i);
           p1 = CercaItem(i+1);
           if(sortFunction(p->Ele,p1->Ele) <= 0)
               continue;
           p2 = p->Ele;
           p->Ele = p1->Ele;
           p1->Ele = p2;
//           SetItem(i,p1);
//           SetItem(i+1,p);
           flag = TRUE;
           i2 = i;
       }
       i1 = i2;
   }
}

