#include <windows.h>
#pragma hdrstop
#include <condefs.h>
#include <stdio.h>
#include "pluginmain.h"

static int nEnable = 0;
static INDS *lpNDS;
static DWORD nFrame;
static FILE *fp;
//---------------------------------------------------------------------------
static void open_file();
//---------------------------------------------------------------------------
#ifdef __WIN32__
int WINAPI DllEntryPoint(HINSTANCE hinstDLL, unsigned long fdwReason, void* lpvReserved)
{
   switch(fdwReason){
       case DLL_PROCESS_ATTACH:
       break;
   }
   return 1;
}
#endif
//---------------------------------------------------------------------------
extern "C" DWORD I_EXPORT I_STDCALL GetInfoFunc(LPGETPLUGININFO p)
{
   GUID guid0 = {0x8229CDC4,0x89AB,0x45CA,{0xBA,0x01,0x3F,0xB8,0x7A,0xD9,0x17,0xE9}};
   GUID guid1 = {0x8229CDC5,0x89AC,0x45CA,{0xBA,0x01,0x3F,0xB8,0x7A,0xD9,0x17,0xE9}};

   p->dwType = PIT_PAD|PIT_DYNAMIC;
   switch(p->wIndex){
       case 1:
         	if(p->pszText != NULL)
               lstrcpyn((LPSTR)p->pszText,"Keys Player\tCtrl+B",p->cchTextMax);
           *(&p->guidID) = *(&guid0);
       break;
       case 2:
           if(p->pszText != NULL)
               lstrcpyn((LPSTR)p->pszText,"Keys Recorder\tCtrl+A",p->cchTextMax);
           *(&p->guidID) = *(&guid1);
       break;
   }
   return 2;
}
//---------------------------------------------------------------------------
extern "C" BOOL I_EXPORT I_STDCALL SetInfoFunc(LPSETPLUGININFO p)
{
	if(p ==	NULL)
		return FALSE;
	if(p->dwStateMask & PIS_ENABLEMASK){
       if(p->dwState & PIS_ENABLE)
           nEnable |= (1 << (p->wIndex - 1));
       else
           nEnable &= ~(1 << (p->wIndex - 1));
       lpNDS = p->lpNDS;
   }
   if(p->dwStateMask & PIS_NOTIFYMASK){
       switch(p->dwState){
           case PNM_STARTFRAME:
               nFrame++;
           break;
           case PNM_OPENFILE:
               if(p->lParam != 0){
                   if(fp != NULL){
                       fclose(fp);
                       fp = NULL;
                   }
                   open_file();
               }
           break;
           case PNM_ENDFRAME:
               p = p;
           break;
       }
   }
  	return TRUE;
}
//---------------------------------------------------------------------------
extern "C" BOOL I_EXPORT I_STDCALL ResetFunc()
{
   nFrame = 0;
   if(fp != NULL){
       fclose(fp);
       fp = NULL;
   }
 	return TRUE;
}
//---------------------------------------------------------------------------
extern "C" BOOL I_EXPORT I_STDCALL DeleteFunc()
{
   if(fp !=  NULL){
       fclose(fp);
       fp = NULL;
   }
	return TRUE;
}
//---------------------------------------------------------------------------
extern "C" u32 I_EXPORT I_STDCALL RunFunc(LPNOTIFYCONTENT p)
{
   int i,frame,keys;
   char s[300];

   //Player
   if(nEnable & 1){
       if(fp == NULL)
           return FALSE;
       for(i=0;i<300 && !feof(fp);i++){
           s[i] = (char)fgetc(fp);
           if(s[i] == 10)
               break;
       }
       if(i < 300 && s[i] == 10){
           s[i+1] = 0;
           sscanf(s,"%08X,%08X",&frame,&keys);
           *p->pad.value = keys;
           return TRUE;
       }
       return FALSE;
   }
   //Recorder
   if(nEnable & 2){
       if(fp == NULL)
           open_file();
       if(fp != NULL)
           fprintf(fp,"%08X,%08X\r\n",nFrame,*p->pad.value);
       return FALSE;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
extern "C" BOOL I_EXPORT I_STDCALL SetPropertyFunc(LPSETPROPPLUGIN p)
{
	return FALSE;
}
//---------------------------------------------------------------------------
static void open_file()
{
   char s[MAX_PATH+1];
   int i;

   if(fp != NULL)
       return;
   i = MAX_PATH;
   if(lpNDS->CreateFileName(PT_SAVESTATE,s,&i,"pat",-1) == S_OK){
       if(nEnable & 1)
           fp = fopen(s,"rb+");
       else if(nEnable & 2){
           remove(s);
           fp = fopen(s,"wb+");
       }
   }
}

