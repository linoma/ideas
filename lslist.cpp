#include "ideastypes.h"
#include "lslist.h"
#include "lregkey.h"
//---------------------------------------------------------------------------
LStringList::LStringList(DWORD dw) : LList(dw)
{
}
//---------------------------------------------------------------------------
LStringList::~LStringList()
{
	Clear();
}
//---------------------------------------------------------------------------
void LStringList::DeleteElem(LPVOID ele)
{
	if(ele != NULL)
   	delete (LString *)ele;
}
//---------------------------------------------------------------------------
BOOL LStringList::Save(char *k,BOOL bRefill,BOOL bKey)
{
	LRegKey key;
	DWORD dwPos;
	LString *s;
	int i;
	char ss[30];

	if(bKey){
   	if(!key.Open((char *)k))
       	return FALSE;
   	s = (LString *)GetFirstItem(&dwPos);
       i = 0;
       while(s != NULL){
           wsprintf(ss,"File%d",i++);
       	key.WriteString(ss,(char *)s->c_str());
       	s = (LString *)GetNextItem(&dwPos);
       }
       if(!bRefill || maxElem == 0xFFFFFFFF)
       	return TRUE;
       for(;i<maxElem;i++){
           wsprintf(ss,"File%d",i);
       	key.WriteString(ss,"");
       }
       return TRUE;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL LStringList::Load(char *k,BOOL bKey)
{
	LRegKey key;
	LString s;
	int i;
	char ss[30];

	if(bKey){
   	Clear();
   	if(!key.Open((char *)k),FALSE)
       	return FALSE;
       i = 0;
       while(1){
           wsprintf(ss,"File%d",i++);
       	s = key.ReadString(ss,"");
			if(s.IsEmpty())
				break;
           if(Find((char *)s.c_str()) == NULL)
           	Add(s);
       }
       return TRUE;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
const char *LStringList::Find(char *str)
{
   LString *s;
	DWORD dwPos;
   
	s = (LString *)GetFirstItem(&dwPos);
   while(s != NULL){
      	if(lstrcmp(s->c_str(),str) == 0)
       	return s->c_str();
     	s = (LString *)GetNextItem(&dwPos);
   }
   return NULL;
}
//---------------------------------------------------------------------------
BOOL LStringList::Add(LString s)
{
	if(s.IsEmpty())
   	return FALSE;
   return Add((char *)s.c_str());
}
//---------------------------------------------------------------------------
BOOL LStringList::Add(char *c)
{
	LString *s;

	if(c == NULL || *c == 0)
   	return FALSE;
   s = new LString(c);
   if(s == NULL)
   	return FALSE;
   return LList::Add((LPVOID)s);
}

