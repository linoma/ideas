#include "lstring.h"

#define CHECK_POINTER(p) if(p == NULL) return;
#define CHECK_POINTERR(p) if(p == NULL) return *this;

//---------------------------------------------------------------------------
LString::LString()
{
   pToken = pData = NULL;
   maxLen = 0;
}
//---------------------------------------------------------------------------
LString::LString(const LString &string)
{
   pData = NULL;
   maxLen = 0;
   CopyString((char *)string.pData,lstrlen(string.pData));
}
//---------------------------------------------------------------------------
LString::LString(const int len)
{
   pData = NULL;
   maxLen = 0;
   AllocBuffer(len);
}
//---------------------------------------------------------------------------
LString::~LString()
{
   if(pData != NULL)
       ::GlobalFree((HGLOBAL)pData);
   pData = NULL;
}
//---------------------------------------------------------------------------
LString::LString(const char *pString)
{
   maxLen = 0;
   pData = NULL;
   CHECK_POINTER(pString);
   CopyString((char *)pString,lstrlen(pString));
}
//---------------------------------------------------------------------------
const BOOL LString::IsEmpty()
{
	if(pData == NULL || lstrlen(pData) == 0)
		return TRUE;
	return FALSE;
};
//---------------------------------------------------------------------------
const LString &LString::AddEXT(char *ext)
{
	char *p,*p1;

	if(Length()){
   	if((p = strrchr(pData,'.')) != NULL){
           p1 = p;
           while(*p1 != DPC_PATH && *p1 != 0) p1++;
           if(*p1 == 0)
   			Length((int)(p-pData));
       }
		CatString(ext,lstrlen(ext));
   }
   return *this;
}
//---------------------------------------------------------------------------
LString LString::Path()
{
   LString c;
   char *p;

   c = "";
   if(Length()){
   	p = strrchr(pData,DPC_PATH);
   	if(p != NULL)
       	c = SubString(1,p-pData);
   }
   return c;
}
//---------------------------------------------------------------------------
LString LString::Ext()
{
   LString c;
   char *p;

	c = "";
   if(Length() && (p = strrchr(pData,'.')) != NULL)
       c = p;
   return c;
}
//---------------------------------------------------------------------------
LString LString::FileName()
{
   LString c;
   char *p,*p1;

	c = "";
   if(Length()){
   	c = pData;
   	if((p = strrchr(pData,DPC_PATH)) == NULL)
       	p = strrchr(pData,'/');
   	if(p != NULL)
       	c = ++p;
   	else
       	p = pData;
   	if((p1 = strrchr(p,'.')) != NULL)
       	c.Length((p1 - p));
   }
   return c;
}
//---------------------------------------------------------------------------
BOOL LString::BuildFileName(const LPSTR fileName,const LPSTR fileExt,int ID)
{
   LString s,nameFile;

   if(fileName == NULL)
       return FALSE;
   s = fileName;
   if(s.Length() == 0)
       return FALSE;
   nameFile = s.Path() + DPC_PATH;
   nameFile += s.FileName().AllTrim();
   if(ID != 0)
       nameFile += (int)ID;
   if(fileExt != NULL){
       nameFile += ".";
       nameFile += fileExt;
   }
   CopyString(nameFile.c_str(),nameFile.Length());
   return (BOOL)(nameFile.Length() > 0 ? TRUE : FALSE);
}
//---------------------------------------------------------------------------
LString LString::NextToken(BYTE c)
{
   char *p;
   LString s;
   int i;

   s = "";
   if(pToken == NULL)
       return s;
	p = strchr(pToken,c);
   if(p != NULL){
       i = p - pToken;
       s.Capacity(i+1);
       lstrcpyn(s.c_str(),pToken,i+1);
       pToken = p + 1;
       if(pToken[0] == 0)
           pToken = NULL;
   }
   else if(pToken != pData){
       i = lstrlen(pToken);
       s.Capacity(i+1);
       lstrcpyn(s.c_str(),pToken,i+1);
       pToken = NULL;
   }
   return s;
}
//---------------------------------------------------------------------------
const BOOL LString::LoadString(UINT uID,HINSTANCE hInst)
{
#ifdef __WIN32__
   int i;

   if(hInst == NULL)
       hInst = GetModuleHandle(NULL);
   AllocBuffer(100);
   if(pData == NULL)
       return FALSE;
   i = ::LoadString(hInst,uID,pData,100);
   if(i < 100 && i > 0)
       Length(i);
   return i > 0 ? TRUE : FALSE;
#else
	return FALSE;
#endif
}
//---------------------------------------------------------------------------
LString LString::SubString(int index,int count) const
{
   LString c;

   c.CopyString(&pData[index-1],count);
   return c;
}
//---------------------------------------------------------------------------
const LString& LString::RemoveAllChar(const BYTE c)
{
	LeftTrim(c);
   RightTrim(c);
   return *this;
}
//---------------------------------------------------------------------------
const LString& LString::AllTrim()
{
   return RemoveAllChar(32);
}
//---------------------------------------------------------------------------
const LString& LString::LeftTrim(const BYTE ch)
{
   LString c;
   int i,iLen;

	iLen = Length();
   for(i=0;i<iLen;i++){
       if(pData[i] != ch)
           break;
   }
   i++;
	c = SubString(i,iLen - i+1);
   CopyString(c.c_str(),c.Length());
   return *this;
}
//---------------------------------------------------------------------------
const LString& LString::RightTrim(const BYTE ch)
{
   LString c;
   int i;

   for(i=Length();i > 0;i--){
       if(pData[i-1] != ch)
           break;
   }
	c = SubString(1,i);
   CopyString(c.c_str(),c.Length());
   return *this;
}
//---------------------------------------------------------------------------
const LString& LString::operator =(const LString& string)
{
   CopyString(string.pData,string.Length());
   return *this;
}
//---------------------------------------------------------------------------
const LString &LString::operator =(const char *pString)
{
   CHECK_POINTERR(pString);
   CopyString((char *)pString,lstrlen(pString));
   pToken = pData;
   return *this;
}
//---------------------------------------------------------------------------
int LString::Length() const
{
    return pData != NULL ? lstrlen(pData) : 0;
}
//---------------------------------------------------------------------------
int LString::Length(int len)
{
   LString s;

   if(pData == NULL){
       AllocBuffer(len);
       return Length();
   }
   s = pData;
   CopyString(s.c_str(),len);
   return Length();
}
//---------------------------------------------------------------------------
int LString::ToInt()
{
   return pData != NULL ? atoi(pData) : 0;
}
//---------------------------------------------------------------------------
const LString &LString::operator +(const LString& string)
{
   if(string.pData)
       CatString(string.pData,string.Length());
   return *this;
}
//---------------------------------------------------------------------------
const LString& LString::operator +=(const LString& string)
{
   CatString(string.pData,string.Length());
   return *this;
}
//---------------------------------------------------------------------------
BOOL LString::operator <(const char *pString)
{
   if(pData == NULL || pString == NULL)
       return FALSE;
   return lstrcmp(pData,pString) < 0 ? TRUE : FALSE;
}
//---------------------------------------------------------------------------
BOOL LString::operator >(const char *pString)
{
   if(pData == NULL || pString == NULL)
       return FALSE;
   return lstrcmp(pData,pString) > 0 ? TRUE : FALSE;
}
//---------------------------------------------------------------------------
BOOL LString::operator ==(const char *pString)
{
   if(pData == NULL || pString == NULL)
       return 0;
   return lstrcmp(pData,pString) == 0 ? TRUE : FALSE;
}
//---------------------------------------------------------------------------
const LString &LString::operator +(const char car)
{
   pData[Length()] = car;
   return *this;
}
//---------------------------------------------------------------------------
const LString &LString::operator +=(const int value)
{
   char s[30];

   wsprintf(s,"%d",value);
   CatString(s,lstrlen(s));
   return *this;
}
//---------------------------------------------------------------------------
const LString &LString::operator +=(const char car)
{
   char s[4];

   *((long *)s) = 0;
   s[0] = car;
   CatString(s,1);
   return *this;
}
//---------------------------------------------------------------------------
const LString &LString::operator +(const char *pString)
{
   CHECK_POINTERR(pString);
   CatString((char *)pString,lstrlen(pString));
   return *this;
}
//---------------------------------------------------------------------------
char LString::operator[](const int index)
{
   if(pData == NULL || index > Length())
       return 0;
   return pData[index-1];
}
//---------------------------------------------------------------------------
const LString &LString::UpperCase()
{
   CHECK_POINTERR(pData);
#ifdef __WIN32__
   CharUpperBuff(pData,Length());
#else
	for(int i = 0;i < strlen(pData);i++)
		pData[i] = toupper(pData[i]);
#endif
   return *this;
}
//---------------------------------------------------------------------------
const LString &LString::LowerCase()
{
	CHECK_POINTERR(pData);
#ifdef __WIN32__
   CharLowerBuff(pData,Length());
#else
	for(int i = 0;i < strlen(pData);i++)
		pData[i] = tolower(pData[i]);
#endif
  	return *this;
}
//---------------------------------------------------------------------------
int LString::Pos(const char Search)
{
   char c[2];

   c[0] = Search;
   c[1] = 0;
   return Pos(c);
}
//---------------------------------------------------------------------------
int LString::Pos(char *Search)
{
   int len,i,length,i1;
   char *p,*p1,*p2;

   if(Search == NULL || (length = Length()) == 0)
       return 0;
   len = lstrlen(Search);
   if(len > length)
       return 0;
   p = pData;
   for(i = 0;i<length;i++){
       if(*p++ != *Search)
       	continue;
       p1 = p;
       p2 = Search + 1;
       for(i1=1;i1<len;i1++){
       	if(*p1++ != *p2++)
           	break;
       }
       if(i1 == len)
       	break;
   }
   return i == length ? -1 : i + 1;
}
//---------------------------------------------------------------------------
const LString& LString::operator +=(const char *pString)
{
	return Add(pString);
}
//---------------------------------------------------------------------------
const LString& LString::Add(const char *pString)
{
   CatString((char *)pString,lstrlen(pString));
   return *this;
}
//---------------------------------------------------------------------------
const LString& LString::operator =(const int value)
{
   AllocBuffer(20);
   wsprintf(pData,"%d",value);
   return *this;
}
//---------------------------------------------------------------------------
const LString& LString::Capacity(const int len)
{
    LString s;

    if(pData != NULL)
        s = pData;
    AllocBuffer(len);
    if(s.Length() > 0)
    	lstrcpyn(pData,s.c_str(),len);
    return *this;
}
//---------------------------------------------------------------------------
void LString::Empty()
{
	if(pData != NULL)
		*pData = 0;
}
//---------------------------------------------------------------------------
void LString::Copy(const char *pSrc)
{
	if(pSrc != NULL)
		CopyString((char *)pSrc,strlen(pSrc));
}
//---------------------------------------------------------------------------
void LString::AllocBuffer(int len)
{
	len = ((len >> 7) + 1) << 7;
	if(len > maxLen){
       if(pData)
       	::GlobalFree((HGLOBAL)pData);
   	if((pData = (char *)GlobalAlloc(GMEM_FIXED,len)) == NULL)
       	return;
	}
	ZeroMemory(pData,len);
	pToken = pData;
   maxLen = len;
}
//---------------------------------------------------------------------------
void LString::CopyString(char *pSrc,int len)
{
	AllocBuffer(len+1);
   if(pData == NULL || len < 1)
   	return;
	lstrcpyn(pData,pSrc,len+1);
}
//---------------------------------------------------------------------------
void LString::CatString(char *pSrc,int len)
{
   LString l(pData);

   AllocBuffer(len+l.Length());
   if(l.pData != NULL)
       lstrcpy(pData,l.pData);
    if(pSrc != NULL)
        lstrcat(pData,pSrc);
}

