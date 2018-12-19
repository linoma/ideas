#define INITGUID
//---------------------------------------------------------------------------
#include "ideastypes.h"
#include "license.h"
#include "resource.h"
//---------------------------------------------------------------------------
DEFINE_GUID(CLSID_Scripting,0x0D43FE01,0xF093,0x11CF,0x89,0x40,0x00,0xA0,0xC9,0x05,0x42,0x28);
DEFINE_GUID(IID_IFileSystem,0x0AB5A3D0,0xE5B6,0x11D0,0xAB,0xF5,0x00,0xA0,0xC9,0x0F,0xFF,0xC0);
//---------------------------------------------------------------------------
#ifdef _LICENSE
BOOL GetSerialNumber(LPDWORD p)
{
   HRESULT hr;
   IFileSystem *pFS;
   IDrive *pD;
   BOOL res;
   char dir[MAX_PATH+1];
   BSTR bstr;

   res = FALSE;
   if(!GetWindowsDirectory(dir,MAX_PATH))
       return FALSE;
   hr = CoCreateInstance(CLSID_Scripting,NULL,CLSCTX_ALL,IID_IFileSystem,(LPVOID *)&pFS);
   if(hr != S_OK || pFS == NULL)
       return res;
//   dir[1] = 0;
   if((bstr = SysAllocStringByteLen(dir,lstrlen(dir))) != NULL){
       MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,dir,1,bstr,2);
       bstr[1] = 0;
       hr = pFS->GetDrive(bstr,&pD);
       if(hr == S_OK && pD != NULL){
           hr = pD->SerialNumber((long *)p);
           if(hr == S_OK)
               res = TRUE;
           pD->Release();
       }
       SysFreeString(bstr);
   }
   pFS->Release();
   return res;
}
//---------------------------------------------------------------------------
BOOL get_ProductGuid(LPGUID guid)
{
   BOOL res;
   DWORD dwType,dwLen;
   HKEY hKey;

   res = FALSE;
   if(::RegOpenKey(HKEY_CURRENT_USER,"Software\\iDeaS\\Settings",&hKey) == ERROR_SUCCESS){
       dwLen = sizeof(GUID);
       dwType = REG_BINARY;
       if(::RegQueryValueEx(hKey,"ProductKey",NULL,&dwType,(LPBYTE)guid,&dwLen) == ERROR_SUCCESS)
           res = TRUE;
       ::RegCloseKey(hKey);
   }
   return res;
}
//---------------------------------------------------------------------------
BOOL get_LicenseGuid(LPGUID guid)
{
   BOOL res;
   DWORD dwType,dwLen;
   char s1[MAX_PATH*2];
   HKEY hKey;

   res = FALSE;
   if(::RegOpenKey(HKEY_CURRENT_USER,"Software\\iDeaS\\Settings",&hKey) == ERROR_SUCCESS){
       dwLen = MAX_PATH*2;
       dwType = REG_BINARY;
       if(::RegQueryValueEx(hKey,"License",NULL,&dwType,(LPBYTE)s1,&dwLen) == ERROR_SUCCESS){
           if(dwLen == sizeof(GUID)){
               CopyMemory(guid,s1,sizeof(GUID));
               res = TRUE;
           }
       }
       ::RegCloseKey(hKey);
   }
   return res;
}
//---------------------------------------------------------------------------
LString LicenseToString(LPGUID p)
{
   char s[100];

   if(p == NULL)
       return "";
   wsprintf(s,"%04X-%04X-%04X-%04X-%04X-%04X-%04X-%04X",
       (unsigned short)((p->Data1 >> 16)),
       (unsigned short)((unsigned short)p->Data1),
       (unsigned short)(p->Data2),
       (unsigned short)(p->Data3),
       (unsigned short)(((unsigned short *)p->Data4)[0]),
       (unsigned short)(((unsigned short *)p->Data4)[1]),
       (unsigned short)(((unsigned short *)p->Data4)[2]),
       (unsigned short)(((unsigned short *)p->Data4)[3]));
   return (LString)s;
}
//---------------------------------------------------------------------------
/*void GetLicenseControlCode(LPGUID p)
{
   char s[200];
   GUID g;

   CopyMemory(&g,p,sizeof(GUID));

   ((unsigned short *)&g.Data1)[0] ^= 0x5F31;
   ((unsigned short *)&g.Data1)[1] ^= 0x5F31;
   g.Data2 ^= 0x5F31;
   g.Data3 ^= 0x5F31;
   ((unsigned short *)g.Data4)[0] ^= 0x5F31;
   ((unsigned short *)g.Data4)[1] ^= 0x5F31;
   ((unsigned short *)g.Data4)[2] ^= 0x5F31;
   ((unsigned short *)g.Data4)[3] ^= 0x5F31;

   ((unsigned short *)&g.Data1)[0] ^= 0x315F;
   ((unsigned short *)&g.Data1)[1] ^= 0x315F;
   g.Data2 ^= 0x315F;
   g.Data3 ^= 0x315F;
   ((unsigned short *)g.Data4)[0] ^= 0x315F;
   ((unsigned short *)g.Data4)[1] ^= 0x315F;
   ((unsigned short *)g.Data4)[2] ^= 0x315F;
   ((unsigned short *)g.Data4)[3] ^= 0x315F;

   wsprintf(s,"%04X-%04X-%04X-%04X-%04X-%04X-%04X-%04X",
       (unsigned short)((g.Data1 >> 16)),
       (unsigned short)((unsigned short)g.Data1),
       (unsigned short)(g.Data2),
       (unsigned short)(g.Data3),
       (unsigned short)(((unsigned short *)g.Data4)[0]),
       (unsigned short)(((unsigned short *)g.Data4)[1]),
       (unsigned short)(((unsigned short *)g.Data4)[2]),
       (unsigned short)(((unsigned short *)g.Data4)[3]));           //2E8E-B632-3062-7791-22DE-6A4C-0DB6-375E
}*/
//---------------------------------------------------------------------------
BOOL LicenseKeyToGUID(LPGUID p,BOOL isProductKey)
{
   DWORD dw;
   WORD wXOR;
   
   if(p == NULL || !GetSerialNumber(&dw))
       return FALSE;
   /*   wXOR = (WORD)(isProductKey ? 0x5F31 : 0x315F);
   
   ((unsigned short *)&p->Data1)[0] ^= wXOR++;
   ((unsigned short *)&p->Data1)[1] ^= wXOR++;
   p->Data2 ^= wXOR++;
   p->Data3 ^= wXOR++;
   ((unsigned short *)p->Data4)[0] ^= wXOR++;
   ((unsigned short *)p->Data4)[1] ^= wXOR++;
   ((unsigned short *)p->Data4)[2] ^= wXOR++;
   ((unsigned short *)p->Data4)[3] ^= wXOR;*/
   if(isProductKey){
       wXOR = dw & 0xF;
       wXOR |= ((dw & 0xF00) >> 4);
       wXOR |= ((dw & 0xF0) << 4);
       wXOR |= ((dw & 0xF000));
       ((unsigned short *)&p->Data1)[0] ^= wXOR;
       ((unsigned short *)&p->Data1)[1] ^= wXOR;
       p->Data2 ^= wXOR;
       p->Data3 ^= wXOR;
       ((unsigned short *)p->Data4)[0] ^= wXOR;
       ((unsigned short *)p->Data4)[1] ^= wXOR;
       ((unsigned short *)p->Data4)[2] ^= wXOR;
       ((unsigned short *)p->Data4)[3] ^= wXOR;
   }
   else{
       wXOR = (dw & 0xF) << 12;
       wXOR |= ((dw & 0xF00) >> 4);
       wXOR |= ((dw & 0xF0) << 4);
       wXOR |= ((dw & 0xF000) >> 12);
       ((unsigned short *)&p->Data1)[0] ^= wXOR;
       wXOR = (WORD)((wXOR >> 1) | ((wXOR & 1) << 15));
       ((unsigned short *)&p->Data1)[1] ^= wXOR;
       wXOR = (WORD)((wXOR >> 1) | ((wXOR & 1) << 15));
       p->Data2 ^= wXOR;
       wXOR = (WORD)((wXOR >> 1) | ((wXOR & 1) << 15));
       p->Data3 ^= wXOR;
       wXOR = (WORD)((wXOR >> 1) | ((wXOR & 1) << 15));
       ((unsigned short *)p->Data4)[0] ^= wXOR;
       wXOR = (WORD)((wXOR >> 1) | ((wXOR & 1) << 15));
       ((unsigned short *)p->Data4)[1] ^= wXOR;
       wXOR = (WORD)((wXOR >> 1) | ((wXOR & 1) << 15));
       ((unsigned short *)p->Data4)[2] ^= wXOR;
       wXOR = (WORD)((wXOR >> 1) | ((wXOR & 1) << 15));
       ((unsigned short *)p->Data4)[3] ^= wXOR;
   }

   return TRUE;
}
//---------------------------------------------------------------------------
BOOL CheckLicenseKey(LPGUID p,BOOL isProductKey)
{
   GUID g;
   DWORD dw;
   BYTE c[4];

   if(p == NULL || !GetSerialNumber(&dw))
       return FALSE;
   if(((LPDWORD)p)[0] == 0 && ((LPDWORD)p)[1] == 0 && ((LPDWORD)p)[2] == 0 && ((LPDWORD)p)[3] == 0)
       return FALSE;
   CopyMemory(&g,p,sizeof(GUID));
   if(!LicenseKeyToGUID(&g,isProductKey))
       return FALSE;
   c[0] = (BYTE)((((unsigned char *)&g)[5] & 0xF) | ((((unsigned char *)&g)[6] & 0xF) << 4));
   c[1] = (BYTE)((((unsigned char *)&g)[9] & 0xF) | ((((unsigned char *)&g)[10] & 0xF) << 4));
   c[2] = (BYTE)((((unsigned char *)&g)[13] & 0xF) | ((((unsigned char *)&g)[14] & 0xF) << 4));
   c[3] = (BYTE)((((unsigned char *)&g)[1] & 0xF) | ((((unsigned char *)&g)[2] & 0xF) << 4));
   if(c[0] != ((unsigned char *)&dw)[0] ||
       c[1] != ((unsigned char *)&dw)[2] ||
       c[2] != ((unsigned char *)&dw)[1] ||
       c[3] != ((unsigned char *)&dw)[3])
           return FALSE;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL VerifyLicense(LPGUID p,LPGUID p1)
{
   GUID g,g1;

   if(!CheckLicenseKey(p1,TRUE))
       return FALSE;
   if(!CheckLicenseKey(p,FALSE))
       return FALSE;
   CopyMemory(&g,p,sizeof(GUID));
   if(!LicenseKeyToGUID(&g,FALSE))
       return FALSE;
   CopyMemory(&g1,p1,sizeof(GUID));
   if(!LicenseKeyToGUID(&g1,TRUE))
       return FALSE;
   if(g.Data1 != g1.Data1)
       return FALSE;
   if(g.Data2 != g1.Data2)
       return FALSE;
   if(g.Data3 != g1.Data3)
       return FALSE;
   if(((unsigned short *)g.Data4)[0] != ((unsigned short *)g1.Data4)[0])
       return FALSE;
   if(((unsigned short *)g.Data4)[1] != ((unsigned short *)g1.Data4)[1])
       return FALSE;
   if(((unsigned short *)g.Data4)[2] != ((unsigned short *)g1.Data4)[2])
       return FALSE;
   if(((unsigned short *)g.Data4)[3] != ((unsigned short *)g1.Data4)[3])
       return FALSE;
   return TRUE;
}
//---------------------------------------------------------------------------
static BOOL CALLBACK DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   GUID g,g1;
   int i;
   char s[MAX_PATH];
   HKEY hKey;
   
   switch(uMsg){
       case WM_COMMAND:
           if(LOWORD(wParam) == IDOK && HIWORD(wParam) == BN_CLICKED){
               i = 0;
               if(IsWindowEnabled(GetDlgItem(hwndDlg,IDC_EDIT2))){
                   if(GetWindowText(GetDlgItem(hwndDlg,IDC_EDIT2),s,MAX_PATH-1) == 39){
                       sscanf(s,"%04hX-%04hX-%04hX-%04hX-%04hX-%04hX-%04hX-%04hX",
                           &((unsigned char *)&g.Data1)[2],
                           &((unsigned char *)&g.Data1)[0],
                           &g.Data2,&g.Data3,((unsigned short *)g.Data4),
                           &((unsigned short *)g.Data4)[1],
                           &((unsigned short *)g.Data4)[2],
                           &((unsigned short *)g.Data4)[3]);
                       if(get_ProductGuid(&g1) && VerifyLicense(&g,&g1)){
                           if(::RegOpenKey(HKEY_CURRENT_USER,"Software\\iDeaS\\Settings",&hKey) == ERROR_SUCCESS){
                               if(::RegSetValueEx(hKey,"License",0,REG_BINARY,(const BYTE *)&g,sizeof(GUID)) == ERROR_SUCCESS)
                                   i = 1;
                               ::RegCloseKey(hKey);
                           }
                       }
                   }
               }
               else
                   i = 1;
               EndDialog(hwndDlg,i);
           }
       break;
       case WM_INITDIALOG:
           if(get_ProductGuid(&g))
               SendDlgItemMessage(hwndDlg,IDC_EDIT1,WM_SETTEXT,0,(LPARAM)LicenseToString(&g).c_str());
           if(get_LicenseGuid(&g)){
               SendDlgItemMessage(hwndDlg,IDC_EDIT2,WM_SETTEXT,0,(LPARAM)LicenseToString(&g).c_str());
               EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT2),FALSE);
           }
           else
               SetFocus(GetDlgItem(hwndDlg,IDC_EDIT2));
       break;
       case WM_CLOSE:
           EndDialog(hwndDlg,0);
       break;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL InsertLicense()
{
   if(::DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(500),NULL,DialogProc))
       return TRUE;
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL CreateLicenseKey()
{
   HKEY hKey;
   DWORD KeyWord,dwSerial;
   WORD w_xor;
	GUID guid,licGUID,proGUID;
	int i,i1;
	BOOL res;
   char s1[MAX_PATH*2];

	res = FALSE;
   i = get_LicenseGuid(&licGUID) ? 3 : 1;
   if(get_ProductGuid(&proGUID))
       i |= 4;
   if(i == 7)
       return VerifyLicense(&licGUID,&proGUID);
   else if(i == 5)
       return InsertLicense();
	if(::RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\iDeaS\\Settings",0,
       NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,&KeyWord) == ERROR_SUCCESS){
		for(i = 0;i<MAX_PATH;i++){
           ZeroMemory(&guid,sizeof(GUID));
			CoCreateGuid(&guid);
			if((i1 = MAX_PATH - i) > sizeof(GUID))
				i1 = sizeof(GUID);
			CopyMemory(&s1[i],&guid,i1);
			i += i1;
		}
		i = (int)(unsigned char)GetTickCount();
		if(i > 15 && i < 17 || (i > 67 && i < 69))
			i += 2;
		s1[15] = (char)i;
		s1[67] = (char)i;
		*((LPWORD)&s1[i]) = 0;
		::RegSetValueEx(hKey,"License",0,REG_BINARY,(const BYTE *)s1,MAX_PATH + 10);
       ::RegCloseKey(hKey);
       ::RegFlushKey(HKEY_CURRENT_USER);
	}
	if(CoCreateGuid(&guid) != S_OK)
       goto ex_CreateLicenseKey;
   if(!GetSerialNumber(&dwSerial))
		goto ex_CreateLicenseKey;
	CopyMemory(s1,&guid,sizeof(GUID));
   s1[5] = (char)((s1[5] & ~0xF) | (((unsigned char *)&dwSerial)[0] & 0xF));
   s1[6] = (char)((s1[6] & ~0xF) | ((((unsigned char *)&dwSerial)[0] & 0xF0) >> 4));
   s1[9] = (char)((s1[9] & ~0xF) | (((unsigned char *)&dwSerial)[2] & 0xF));
   s1[10] = (char)((s1[10] & ~0xF) | ((((unsigned char *)&dwSerial)[2] & 0xF0) >> 4));
   s1[13] = (char)((s1[13] & ~0xF) | (((unsigned char *)&dwSerial)[1] & 0xF));
   s1[14] = (char)((s1[14] & ~0xF) | ((((unsigned char *)&dwSerial)[1] & 0xF0) >> 4));
   s1[1] = (char)((s1[1] & ~0xF) | (((unsigned char *)&dwSerial)[3] & 0xF));
   s1[2] = (char)((s1[2] & ~0xF) | ((((unsigned char *)&dwSerial)[3] & 0xF0) >> 4));

   w_xor = dwSerial & 0xF;
   w_xor |= ((dwSerial & 0xF00) >> 4);
   w_xor |= ((dwSerial & 0xF0) << 4);
   w_xor |= ((dwSerial & 0xF000));
//   w_xor ^= 0x5f31;
   
   dwSerial = ((1 & 3) << 22)|((15 & 0x3FFF) << 7) | ((3 & 0x7) << 4)|((8 & 0xF));//14 + 7 + 2
    			//Product		//Cliente				//Version
//	dwSerial = 0xFFFFFFFF;
   s1[3] = (char)(((s1[3] & ~0xF) | (((unsigned char *)&dwSerial)[0] & 0xF)));
   s1[7] = (char)(((s1[7] & ~0xF0) | ((((unsigned char *)&dwSerial)[0] & 0xF0))));
   s1[8] = (char)(((s1[8] & ~0xF) | (((unsigned char *)&dwSerial)[2] & 0xF)));
   s1[12] = (char)(((s1[12] & ~0xF0) | ((((unsigned char *)&dwSerial)[2] & 0xF0))));
   s1[11] = (char)(((s1[11] & ~0xF) | (((unsigned char *)&dwSerial)[1] & 0xF)));
   s1[15] = (char)(((s1[15] & ~0xF0) | ((((unsigned char *)&dwSerial)[1] & 0xF0))));

/*   dwSerial = 0x0101;
   s1[5] = (char)(((s1[5] & 0xF) | (((unsigned char *)&dwSerial)[0] & 0xF) << 4));
   s1[6] = (char)(((s1[6] & 0xF) | (((unsigned char *)&dwSerial)[0] & 0xF0)));
   s1[9] = (char)(((s1[9] & 0xF) | (((unsigned char *)&dwSerial)[1] & 0xF) << 4));
   s1[10] = (char)(((s1[10] & 0xF) | (((unsigned char *)&dwSerial)[1] & 0xF0)));*/

   CopyMemory(&guid,s1,sizeof(GUID));

   ((unsigned short *)&guid.Data1)[0] ^= w_xor;
   ((unsigned short *)&guid.Data1)[1] ^= w_xor;
   guid.Data2 ^= w_xor;
   guid.Data3 ^= w_xor;
   ((unsigned short *)guid.Data4)[0] ^= w_xor;
   ((unsigned short *)guid.Data4)[1] ^= w_xor;
   ((unsigned short *)guid.Data4)[2] ^= w_xor;
   ((unsigned short *)guid.Data4)[3] ^= w_xor;
	if(::RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\iDeaS\\Settings",0,
        NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,&KeyWord) != ERROR_SUCCESS)
			goto ex_CreateLicenseKey;
	::RegSetValueEx(hKey,"ProductKey",0,REG_BINARY,(const BYTE *)&guid,sizeof(GUID));
	::RegCloseKey(hKey);
ex_CreateLicenseKey:
	return res;
}
#endif

