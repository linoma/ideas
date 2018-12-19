#include "ideastypes.h"
#include "lstring.h"
#include "llist.h"

//---------------------------------------------------------------------------
#ifndef lslistH
#define lslistH
//---------------------------------------------------------------------------
class LStringList : public LList
{
public:
	LStringList(DWORD dw = 0xFFFFFFFF);
   	~LStringList();
   	BOOL Add(char *c);
   	BOOL Add(LString s);
   	BOOL Save(char *k,BOOL bRefill = TRUE,BOOL bKey = TRUE);
   	BOOL Load(char *k,BOOL bKey = TRUE);
   	const char *Find(char *str);
protected:
	void DeleteElem(LPVOID p);
};
#endif


 
