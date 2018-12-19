#ifdef __WIN32__
	#include <windows.h>
#else
	#include "ideastypes.h"
#endif

//---------------------------------------------------------------------------
#ifndef lstringH
#define lstringH
//---------------------------------------------------------------------------
#ifdef __WIN32__
	#define DPC_PATH '\\'
	#define DPS_PATH "\\"
	#define DPC_PATH_INVERSE '/'
	#define DPS_PATH_INVERSE "/"
#else
	#define DPC_PATH '/'
	#define DPS_PATH "/"
	#define DPC_PATH_INVERSE '\\'
	#define DPS_PATH_INVERSE "\\"
#endif

class LString
{
public:
   LString();
   LString(const int len);
   LString(const char *pString);
   LString(const LString& string);
   ~LString();
   const LString &AddEXT(char *ext);
   const BOOL IsEmpty();
   void Empty();
   const BOOL LoadString(UINT uID,HINSTANCE hInst = NULL);
   const LString& AllTrim();
   const LString& RemoveAllChar(const BYTE c);
   const LString& LeftTrim(const BYTE ch = 32);
   const LString& RightTrim(const BYTE ch = 32);
   const LString& Capacity(const int len);
   const LString &operator =(const char *pString);
   const LString &operator =(const int value);
   const LString &operator +(const LString& string);
   const LString &operator +(const char *pString);
   const LString &operator +(const char car);
   const LString &operator +=(const char car);
   const LString &operator +=(const LString& string);
   const LString &operator =(const LString& string);
   const LString &operator +=(const char *pString);
   const LString &Add(const char *pString);
   const LString &operator +=(const int value);
   LString Path();
   LString FileName();
   BOOL operator ==(const char *pString);
   BOOL operator <(const char *pString);
   BOOL operator >(const char *pString);
   char operator[](const int index);
   inline char *c_str(){return pData;};
   int Length() const;
   int Length(int len);
   const LString &UpperCase();
   const LString &LowerCase();
   int Pos(char *Search);
   int Pos(const char Search);
   LString SubString(int index,int count) const;
   int ToInt();
   LString NextToken(BYTE c);
   inline void ResetToken(){pToken = pData;};
   LString Ext();
   BOOL BuildFileName(const LPSTR fileName,const LPSTR fileExt = NULL,int ID = 0);   
   void Copy(const char *pSrc);
protected:
   char *pData,*pToken;
   int maxLen;
private:
   void AllocBuffer(int len);
   void CopyString(char *pSrc,int len);
   void CatString(char *pSrc,int len);
};
#endif


