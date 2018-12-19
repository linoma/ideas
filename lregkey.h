#include "ideastypes.h"
#include "lstring.h"
#include "fstream.h"
#include <stdio.h>
//---------------------------------------------------------------------------
#ifndef lregkeyH
#define lregkeyH

#ifndef REG_QWORD
#define REG_QWORD 11
#endif

//---------------------------------------------------------------------------
class LRegKey
{
public:
   LRegKey(HKEY h = HKEY_CURRENT_USER);
   ~LRegKey();
   BOOL set_KeyRoot(HKEY h = HKEY_CURRENT_USER);
   BOOL Open(const char *key,BOOL bCreateAlways = TRUE);
   BOOL Close();
   BOOL Write(const char *key,LPBYTE lpData,DWORD cbData,DWORD type = REG_SZ);
   BOOL Read(const char *key,LPBYTE lpData,LPDWORD lpType,LPDWORD lpLen);
   DWORD ReadLong(char *key,DWORD def = 0);
   ULONGLONG ReadQword(char *key,ULONGLONG def = 0);
   LString ReadString(char *key,char *def);
   BOOL WriteLong(char *key,DWORD value){ return Write(key,(LPBYTE)&value,sizeof(DWORD),REG_DWORD);};
   BOOL WriteQword(char *key,ULONGLONG value){ return Write(key,(LPBYTE)&value,sizeof(ULONGLONG),REG_QWORD);};
   BOOL WriteString(char *key,char *value){
       if(value == NULL)
           return FALSE;
		return Write(key,(LPBYTE)value,lstrlen(value),REG_SZ);
	};
   BOOL WriteFloat(char *key,float value);
   float ReadFloat(char *key,float def = 0);
   BOOL WriteBinaryData(char *key,char *value,DWORD len){return Write(key,(LPBYTE)value,len,REG_BINARY);};
   DWORD ReadBinaryData(char *key,char *buffer,DWORD len);
#ifdef __USEREGISTRY__
   inline HKEY Handle(){return hKey;};
#else
	inline LFile *Handle(){return hKey;};
#endif
protected:
#ifdef __USEREGISTRY__
	HKEY hKey,hKeyRoot;
#else
	BOOL seek_ToSection(const char *sec);
	BOOL seek_ToKey(const char *key,BOOL bWrite = FALSE);
	LFile *hKey;
   unsigned long ulPosSection;
#endif
};

#endif

