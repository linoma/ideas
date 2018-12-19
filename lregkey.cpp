#include "lregkey.h"
#include "lapp.h"
#include <ctype.h>
#include "lds.h"

//extern LApp ds;
//---------------------------------------------------------------------------
LRegKey::LRegKey(HKEY h)
{
   hKey = NULL;
#ifdef __USEREGISTRY__
   hKeyRoot = h;
#else
   ulPosSection = 0;
#endif
}
//---------------------------------------------------------------------------
LRegKey::~LRegKey()
{
   Close();
}
//---------------------------------------------------------------------------
BOOL LRegKey::set_KeyRoot(HKEY h)
{
	if(hKey != NULL)
       return FALSE;
#ifdef __USEREGISTRY__
   hKeyRoot = h;
#endif
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LRegKey::Open(const char *keyString,BOOL bCreateAlways)
{
	if(!Close())
       return FALSE;
#ifdef __USEREGISTRY__
   DWORD KeyWord;

   if(bCreateAlways){
   	if(RegCreateKeyEx(hKeyRoot,keyString,0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,&KeyWord) != ERROR_SUCCESS)
           return FALSE;
       return TRUE;
   }
   else{
       if(RegOpenKeyEx(hKeyRoot,keyString,0,KEY_ALL_ACCESS,&hKey) != ERROR_SUCCESS)
           return FALSE;
       return TRUE;
   }
#else
	char *fileName,*key;
   int i,i1;
   LString s,s1,s2;

   s = keyString;
   key = s.c_str();
	i1 = s.Length();
   for(i=0;i<i1;i++){
       if(key[i] != '\\')
			continue;
       key[i] = '/';
   }
   fileName = NULL;
   if(key[i1-1] == '/')
       key[--i1] = 0;
   for(i=0;i<i1;i++){
       if(key[i] != '/')
           continue;
       if(fileName == NULL){
           fileName = key;
           fileName[i] = 0;
       }
       else
           key[i] = '_';
   }
	if(fileName == NULL){
		fileName = key;
		key = NULL;
	}
	else
		key += strlen(fileName) + 1;
   s2.Capacity(MAX_PATH+10);
#ifdef __WIN32__
   GetModuleFileName(NULL,s2.c_str(),MAX_PATH);
#else
	s2 = getenv("HOME");
	s2 += DPS_PATH;
#endif
  	s1 = s2.Path();
	s1 += DPS_PATH;
   s1 += fileName;
   s1 += ".ini";
   if((hKey = new LFile(s1.c_str())) == NULL)
   	return FALSE;
	if(!hKey->Open(GENERIC_READ|GENERIC_WRITE,OPEN_EXISTING)){
       if(bCreateAlways)
           hKey->Open(GENERIC_READ|GENERIC_WRITE,OPEN_ALWAYS);
   }
	if(!hKey->IsOpen())
		return FALSE;
	if(key != NULL && !seek_ToSection(key)){
		hKey->SeekToEnd();
		hKey->WriteF("[%s]\r\n",key);
       ulPosSection = hKey->GetCurrentPosition();
	}
 	return TRUE;
#endif
}
//---------------------------------------------------------------------------
BOOL LRegKey::Close()
{
#ifdef __USEREGISTRY__
   LONG res;

   res = ERROR_SUCCESS;
   if(hKey != NULL)
       res = ::RegCloseKey(hKey);
   if(res != ERROR_SUCCESS)
       return FALSE;
   else{
       hKey = NULL;
       return TRUE;
   }
#else
	if(hKey != NULL)
  		delete hKey;
	hKey = NULL;
	return TRUE;
#endif
}
//---------------------------------------------------------------------------
BOOL LRegKey::Write(const char *key,LPBYTE lpData,DWORD cbData,DWORD type)
{
   LONG res;

  	if(hKey == NULL)
		return FALSE;
#ifdef __USEREGISTRY__
  	res = ::RegSetValueEx(hKey,key,0,type,lpData,cbData);
#else
   DWORD i,dwSize,dwPos;
	char c,*p;
   BOOL r;

#ifdef _DEBPRO
   if(lstrcmp(key,"Name") == 0){
       if(lstrcmp(lpData,"Lino") != 0)
           c = c;
   }
#endif
	p = NULL;
	res = ERROR_SUCCESS;
	r = seek_ToKey(key,TRUE);
   dwPos = hKey->GetCurrentPosition();
   for(i=dwPos;;i++){
       if(!hKey->Read(&c,1) || c == '[')
			break;
       if(c == '\n'){
         	i++;
           break;
       }
   }
   hKey->SeekToEnd();
   dwSize = hKey->GetCurrentPosition() - i;
   if(dwSize){
       if((p = (char *)LocalAlloc(LPTR,dwSize)) == NULL)
     	    return FALSE;
       hKey->Seek(i,SEEK_SET);
	    hKey->Read(p,dwSize);
   }
   if(!hKey->SetEndOfFile(dwPos))
       MessageBox(NULL,"Un error occuring in SetEndOfFile","iDeaS Emulator",MB_OK|MB_ICONERROR);
	if(!r)
   	hKey->WriteF("%s=",key);
   res = ERROR_SUCCESS;
	hKey->WriteF(",%d,",type);
	switch(type){
		case REG_SZ:
           hKey->Write(lpData,cbData);
   		hKey->WriteF("\r\n");
		break;
		case REG_DWORD:
			hKey->WriteF("%08X\r\n",*((unsigned long *)lpData));
		break;
       case REG_QWORD:
			for(i=0;i<16;i++){
				dwPos = (DWORD)(*((ULONGLONG *)lpData) >> ((64 - ((i+1)*4))) & 0xF);
				hKey->WriteF("%1X",dwPos);
           }
           hKey->WriteF("\r\n");
       break;
		case REG_BINARY:
			for(i = 0;i<cbData;i++)
				hKey->WriteF("%02X",lpData[i]);
			hKey->WriteF("\r\n");
		break;
 	}
#endif
   if(p != NULL){
		hKey->Write(p,dwSize);
       LocalFree(p);
   }
	hKey->Flush();
  	return res != ERROR_SUCCESS ? FALSE : TRUE;
}
//---------------------------------------------------------------------------
BOOL LRegKey::Read(const char *key,LPBYTE lpData,LPDWORD lpType,LPDWORD lpLen)
{
   LONG res;

   if(hKey == NULL)
   	return FALSE;
#ifdef __USEREGISTRY__
    res = ::RegQueryValueEx(hKey,key,0,lpType,lpData,lpLen);
#else
	unsigned long ulPos,ulLen,i,ulType;
	unsigned char c,c1[20];

	res = !ERROR_SUCCESS;
	if(seek_ToKey(key)){
       ulType = (unsigned long)-1;
       for(i = 0;;i++){
			if(!hKey->Read(&c,1) || c == '\n')
				break;
           if(ulType == (unsigned long)-1){
               if(c == ',')
                   ulType = 0;
           }
           else{
               if(c == ',')
                   break;
               else if(isdigit(c))
                   ulType += (c-48)*(i > 1 ? 10 : 1);
           }
       }
       if(lpType != NULL)
           *lpType = ulType;
		ulPos = hKey->GetCurrentPosition();
		for(ulLen = 0;;ulLen++){
			if(!hKey->Read(&c,1) || c == '\r')
				break;
  		}
		switch(ulType){
       	case REG_DWORD:
           	ulLen = sizeof(DWORD);
           break;
           case REG_BINARY:
           	ulLen >>=1;
           break;
           case REG_QWORD:
           	ulLen = sizeof(ULONGLONG);
           break;
       }
       hKey->Seek(ulPos,SEEK_SET);
 		if(lpData != NULL){
  			switch(ulType){
				case REG_SZ:
					if(hKey->Read(lpData,ulLen) == ulLen)
                       res = ERROR_SUCCESS;
				break;
               case REG_QWORD:
					if(hKey->Read(&c1,16) == 16){
                       *((ULONGLONG *)lpData) = 0;
                       for(i=0;i<16;i++){
                       	if(i != 0)
                           	*((ULONGLONG *)lpData) = *((ULONGLONG *)lpData) << 4;
                           sscanf((char *)&c1[i],"%1X",&ulType);
							*((ULONGLONG *)lpData) |= ulType;

                       }
                       res = ERROR_SUCCESS;
                   }
               break;
				case REG_DWORD:
					if(hKey->Read(&c1,8) == 8){
                       sscanf((char *)c1,"%08X",(LPDWORD)lpData);
                       res = ERROR_SUCCESS;
                   }
				break;
				case REG_BINARY:
					for(i = 0;i<ulLen;i++){
      					if(hKey->Read(&c1,2) != 2)
                           break;
                       sscanf((char *)c1,"%02X",&ulPos);
                       lpData[i] = (char)ulPos;
                   }
                   if(i == ulLen)
                       res = ERROR_SUCCESS;
				break;
 			}
			if(ulLen < *lpLen)
				ulLen = *lpLen;
		}
		else
			res = ERROR_SUCCESS;
  		if(lpLen != NULL)
  			*lpLen = ulLen;
	}
	else{
		if(lpLen != NULL)
			*lpLen = 0;
	}
#endif
	return res != ERROR_SUCCESS ? FALSE : TRUE;
}
//---------------------------------------------------------------------------
DWORD LRegKey::ReadBinaryData(char *key,char *buffer,DWORD len)
{
   DWORD dwType,dwLen,res;

   res = (DWORD)-1;
   if(Read(key,NULL,&dwType,&dwLen)){
       if(dwType == REG_BINARY && dwLen <= len){
           Read(key,(LPBYTE)buffer,&dwType,&dwLen);
           res = dwLen;
       }
   }
   return res;
}
//---------------------------------------------------------------------------
LString LRegKey::ReadString(char *key,char *def)
{
   DWORD dwType,dwLen;
   LString res;

   res = def;
   if(Read(key,NULL,&dwType,&dwLen)){
       if(dwType == REG_SZ && dwLen){
           res.Length(dwLen + 1);
           Read(key,(LPBYTE)res.c_str(),&dwType,&dwLen);
       }
   }
   return res;
}
//---------------------------------------------------------------------------
ULONGLONG LRegKey::ReadQword(char *key,ULONGLONG def)
{
   DWORD dwType,dwLen;
   ULONGLONG res;

   res = def;
   if(Read(key,NULL,&dwType,&dwLen)){
       if(dwLen == 8 && dwType == REG_QWORD)
           Read(key,(LPBYTE)&res,&dwType,&dwLen);
   }
   return res;
}
//---------------------------------------------------------------------------
DWORD LRegKey::ReadLong(char *key,DWORD def)
{
   DWORD res,dwType,dwLen;

   res = def;
   if(Read(key,NULL,&dwType,&dwLen)){
       if(dwLen == 4 && dwType == REG_DWORD)
           Read(key,(LPBYTE)&res,&dwType,&dwLen);
   }
   return res;
}
//---------------------------------------------------------------------------
BOOL LRegKey::WriteFloat(char *key,float value)
{
   char s[50];

   sprintf(s,"%f",value);
   return WriteString(key,s);
}
//---------------------------------------------------------------------------
float LRegKey::ReadFloat(char *key,float def)
{
   char s[50];
   float res;
   DWORD dwType,dwLen;

   res = def;
   if(Read(key,NULL,&dwType,&dwLen)){
       if(dwType == REG_SZ){
           Read(key,(LPBYTE)s,&dwType,&dwLen);
           sscanf(s,"%f",&res);
       }
   }
   return res;
}
#ifndef __USEREGISTRY__
//---------------------------------------------------------------------------
BOOL LRegKey::seek_ToSection(const char *sec)
{
	char *c,*c1,*c2;
	int i,len;
   BOOL res;

 	if(hKey == NULL || sec == NULL || (len = strlen(sec)) == 0)
		return FALSE;
   if((c = (char *)LocalAlloc(LPTR,1000 + (len + 10) * 2)) == NULL)
       return FALSE;
   c1 = c + 500;
   c2 = c1 + 500;
   for(i = 0;i < len;i++)
       c2[i] = (char)tolower(sec[i]);
   c2[i] = 0;
   res = FALSE;
	hKey->SeekToBegin();
	while(1){
       *((long *)c) = 0;
		for(i=0;;i++){
       	if(!hKey->Read(&c[i],1))
           	break;
           if(c[i] == '\n')
           	break;
       }
       if(c[0] == 0)
       	break;
		if(c[0] != '[')
			continue;
		for(i=1;i<=len && c[i] != 0 && c[i] != ']';i++)
			c1[i-1] = (char)tolower(c[i]);
       if(c[i] != ']')
           continue;
		c1[--i] = 0;
		if(strcmp(c1,c2) == 0){
			res = TRUE;
           ulPosSection = hKey->GetCurrentPosition();
           break;
       }
	}
   if(c != NULL)
       LocalFree(c);
	return res;
}
//---------------------------------------------------------------------------
BOOL LRegKey::seek_ToKey(const char *key,BOOL bWrite)
{
	char *c,*c1,*c2;
	int i,len,i1;
	unsigned long ulPos;
   BOOL res;                               
                            
 	if(hKey == NULL || key == NULL || (len = lstrlen(key)) == 0)
		return FALSE;               
   if((c = (char *)LocalAlloc(LPTR,1000+(len + 10) * 2)) == NULL)
       return FALSE;
   c1 = c + 500;
   c2 = c1 + 500;
   for(i = 0;i < len;i++)
       c2[i] = (char)tolower(key[i]);
   c2[i] = 0;
   res = FALSE;
   hKey->Seek(ulPosSection,SEEK_SET);
	while(1){
		ulPos = hKey->GetCurrentPosition();
       *((long *)c) = 0;
       for(i1=0;;i1++){
       	if(!hKey->Read(&c[i1],1))
           	break;
           if(c[i1] == '\n')
           	break;
       }
       c[--i1] = 0;
       if(c[0] == 0)
       	break;
		if(c[0] == '[')
			break;
		for(i = 0;c[i] != 0 && c[i] != '=';i++)
			c1[i] = (char)tolower(c[i]);
		c1[i] = 0;
		if(lstrcmp(c1,c2) != 0)
			continue;
       ulPos += lstrlen(key) + 1;
  		res = TRUE;
       break;
	}
   hKey->Seek(ulPos,SEEK_SET);
   if(c != NULL)
       LocalFree(c);
	return res;
}

#endif


