#include <windows.h>
#pragma hdrstop
#include <condefs.h>
#include "ideastypes.h"
#include <math.h>
#include "pluginmain.h"
#include "audguid.h"
#include "sound.h"
#include "resource.h"
#include "lstring.h"

//---------------------------------------------------------------------------
#ifdef AUD_ALSA
#define NAME_PLUGIN "ALSA Sound PlugIn"
#elif defined(__WIN32__)
#define NAME_PLUGIN "DirectSound PlugIn"
#else
USEUNIT("aud\sound.cpp");
USEUNIT("aud\audguid.cpp");
USEUNIT("aud\FSTREAM.CPP");
USEUNIT("aud\LLIST.CPP");
USEUNIT("aud\lregkey.cpp");
USEUNIT("aud\LSTRING.CPP");
USERC("aud\aud.rc");
#//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
define NAME_PLUGIN "OSS Sound PlugIn"
#endif
//---------------------------------------------------------------------------
HINSTANCE hInstance,hLib = NULL;
static HMENU hMenu;
static HIMAGELIST hImageList = NULL;
static int nChannel = -1;
extern u32 nOptions;
extern int nResample;
extern WAVEFORMATEX wf;
extern LString *fileName;
extern BOOL bStartCapture,bWaitOnResample;
extern DSCHANNEL dschan[16];
#ifdef _DEBPRO
extern BOOL bReportReposition;
#endif

#ifdef __WIN32__

#ifdef __BORLANDC__
//---------------------------------------------------------------------------
#pragma argsused
int WINAPI DllEntryPoint(HINSTANCE hInst, unsigned long reason, void*)
#else
extern "C" BOOL APIENTRY DllMain(HINSTANCE hInst,DWORD reason,LPVOID reserved)
#endif
{
   switch (reason){
       case DLL_PROCESS_ATTACH:
           hInstance = hInst;
       break;
       case DLL_PROCESS_DETACH:
       break;
       case DLL_THREAD_ATTACH:
       break;
       case DLL_THREAD_DETACH:
       break;
   }
   return TRUE;}
#endif
//---------------------------------------------------------------------------
extern "C" DWORD I_EXPORT I_STDCALL GetInfoFunc(LPGETPLUGININFO p)
{
	if(p == NULL)
		return 0;
	switch(p->wType){
		default:
			p->dwType = PIT_AUDIO|PIT_DYNAMIC;
			if(p->pszText != NULL){
				lstrcpyn((LPSTR)p->pszText,NAME_PLUGIN,p->cchTextMax);
				if(p->dwLanguageID)
					hLib = (HINSTANCE)p->lParam;
				else
					hLib = NULL;
			}
			*(&p->guidID) = *(&AUDGUID);
			return 1;
		case PIR_FORMAT:
			if(p->lParam != 0)
				memcpy((LPVOID)p->lParam,&wf,sizeof(WAVEFORMATEX));
			return MAKE_PIRESULT(1,sizeof(WAVEFORMATEX));
	}
}
//---------------------------------------------------------------------------
extern "C" BOOL I_EXPORT I_STDCALL SetInfoFunc(LPSETPLUGININFO p)
{
   if(p ==	NULL)
		return FALSE;
	if(PLUGINISENABLE(p)){
		if(!InitSoundDevice(p)){
			return FALSE;
		}
		if(!EnableSound()){
			return FALSE;
		}
		InitSoundTable();
	}
   if(p->dwStateMask & PIS_RUNMASK){
       if((p->dwState & PIS_RUNMASK) == 4 || !(p->dwState & PIS_RUNMASK))
		   StopSoundSystem((BOOL)((p->dwState & PIS_RUNMASK) ? TRUE : FALSE));
   }
   if(p->dwStateMask & PIS_NOTIFYMASK){
	   if(p->dwState == PNM_ENDLINE)
           RenderDSChannel(p->lParam);
       else if(p->dwState == PNM_COUNTFRAMES){
           nResample = (int)(p->lParam >> 2) << 2;
			nResample = (int)(nResample / 59.98f * (float)SND_DECIMAL);
       }
	   else if(p->dwState == PNM_OPENFILE){
		if(fileName == NULL)
			fileName = new LString();
		if(p->lParam != 0)
		   fileName->Copy((char *)p->lParam);
		else
			fileName->Empty();
	   }
   }
   return TRUE;
}
//---------------------------------------------------------------------------
extern "C" BOOL I_EXPORT I_STDCALL ResetFunc()
{
	ResetSoundSystem();
 	return TRUE;
}
//---------------------------------------------------------------------------
extern "C" BOOL I_EXPORT I_STDCALL DeleteFunc()
{
	DestroySoundSystem();	
	return TRUE;
}
//---------------------------------------------------------------------------
extern "C" void I_EXPORT I_STDCALL RunFunc(void)
{
	UpdateDSChannel();
}
//---------------------------------------------------------------------------
static int ValueToDB(int val)
{
	return val - 100;
}
//---------------------------------------------------------------------------
void FlashTrackBar(HWND hwnd,UINT uCount)
{
	UINT i;
#ifdef __WIN32__
	MSG msg;
#endif
	for(i=0;i<uCount;i++){
		EnableWindow(hwnd,(i & 1) ? TRUE : FALSE);
#ifndef __WIN32__
		gtk_main_iteration_do(false);
#else
		while(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
			DispatchMessage(&msg);
#endif
		Sleep(150);
	}
}
//---------------------------------------------------------------------------
static BOOL CALLBACK DialogProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	char s[30];
   int iPos,i;
   TBBUTTON tbb[1];
	POINT pt;
	RECT rc;
	LPNMHDR p;
	HMENU h;
	HBITMAP bit;
	
   switch(uMsg){
	   case WM_COMMAND:
		   i = (int)HIWORD(wParam);
		   switch(i){
			   default:
				   switch(LOWORD(wParam)){
					   case IDC_SYNCRONIZE:
							switch(HIWORD(wParam)){
								case CBN_SELENDOK:
									EnableWindow(GetDlgItem(hwnd,IDC_CHECK1),SendMessage((HWND)lParam,CB_GETCURSEL,0,0) == 1);
								break;
							}
						break;
				   }
				break;
				case 0:
				case 1:					
					i = LOWORD(wParam);
					switch(i){
						case 50:
							nChannel = -1;
							SetDlgItemText(hwnd,IDC_GROUP_VOLUME,"Volume");
							SendDlgItemMessage(hwnd,IDC_TRACK1,TBM_SETRANGE,TRUE,MAKELPARAM(0,100));
							
							iPos = get_Volume();							
							SendDlgItemMessage(hwnd,IDC_TRACK1,TBM_SETPOS,(WPARAM)TRUE,iPos);
							wsprintf(s,"%d",ValueToDB(iPos));
							SendDlgItemMessage(hwnd,IDC_EDIT2,WM_SETTEXT,0,(LPARAM)s);
							FlashTrackBar(GetDlgItem(hwnd,IDC_TRACK1),8);
							SetFocus(GetDlgItem(hwnd,IDC_TRACK1));							
						break;
						default:
							if(i > 0 && i < 17){
								dschan[i-1].mix_enable ^= 1;								
							}
							else if(i > 50 && i < 67){
								nChannel = (i-51);
								wsprintf(s,"Volume Channel %d",nChannel);
								SetDlgItemText(hwnd,IDC_GROUP_VOLUME,s);								
								SendDlgItemMessage(hwnd,IDC_TRACK1,TBM_SETRANGE,TRUE,MAKELPARAM(0,200));
								iPos = dschan[i-51].mix_vol+100;
								SendDlgItemMessage(hwnd,IDC_TRACK1,TBM_SETPOS,(WPARAM)TRUE,iPos);								
								wsprintf(s,"%d",ValueToDB(iPos));
								SendDlgItemMessage(hwnd,IDC_EDIT2,WM_SETTEXT,0,(LPARAM)s);
								FlashTrackBar(GetDlgItem(hwnd,IDC_TRACK1),8);
								SetFocus(GetDlgItem(hwnd,IDC_TRACK1));
							}
						break;	
					}
				break;
		   }
		break;
	case WM_DESTROY:
			if(hImageList != NULL){
				ImageList_Destroy(hImageList);
				hImageList = NULL;
			}			
		break;
       case WM_INITDIALOG:
			SendDlgItemMessage(hwnd,IDC_TOOLBAR1,TB_BUTTONSTRUCTSIZE,(WPARAM) sizeof(TBBUTTON), 0);
			if(hImageList == NULL){
				hImageList = ImageList_Create(16,16,ILC_COLOR16|ILC_MASK,2,2);
			   bit = LoadBitmap(hInstance,MAKEINTRESOURCE(7777));
			   if(bit != NULL){
				   ImageList_AddMasked(hImageList,bit,RGB(255,0,255));
				   SendDlgItemMessage(hwnd,IDC_TOOLBAR1,TB_SETIMAGELIST,0,(LPARAM)hImageList);
				   ::DeleteObject(bit);
				}
			}
		   tbb[0].iBitmap = 0;
		   tbb[0].idCommand = 5555;
		   tbb[0].fsState = TBSTATE_ENABLED;
		   tbb[0].fsStyle = 0x80;
		   tbb[0].dwData = 0;
		   tbb[0].iString = -1;
		   
			SendDlgItemMessage(hwnd,IDC_TOOLBAR1,TB_SETEXTENDEDSTYLE, (WPARAM)0,(LPARAM)TBSTYLE_EX_DRAWDDARROWS);
		   SendDlgItemMessage(hwnd,IDC_TOOLBAR1,TB_ADDBUTTONS, (WPARAM)1,(LPARAM)&tbb);
#ifndef __WIN32__
			SetWindowPos(GetDlgItem(hwnd,IDC_TOOLBAR1),NULL,0,0,28,60,SWP_NOMOVE|SWP_NOREPOSITION);
#endif				   
		   if(!((nOptions >> 6) & 1))
			    SendDlgItemMessage(hwnd,IDC_RADIO1,BM_SETCHECK,BST_CHECKED,0);
			else
				SendDlgItemMessage(hwnd,IDC_RADIO2,BM_SETCHECK,BST_CHECKED,0);
            if(!((nOptions >> 6) & 2))
				SendDlgItemMessage(hwnd,IDC_RADIO4,BM_SETCHECK,BST_CHECKED,0);
            else
				SendDlgItemMessage(hwnd,IDC_RADIO3,BM_SETCHECK,BST_CHECKED,0);
           SendDlgItemMessage(hwnd,IDC_SYNCRONIZE,CB_ADDSTRING,0,(LPARAM)"No Syncronize (slow quality)");
           SendDlgItemMessage(hwnd,IDC_SYNCRONIZE,CB_ADDSTRING,0,(LPARAM)"Resampling (slow pc)");
			SendDlgItemMessage(hwnd,IDC_SYNCRONIZE,CB_ADDSTRING,0,(LPARAM)"Syncronize (best quality)");
			SendDlgItemMessage(hwnd,IDC_SYNCRONIZE,CB_SETCURSEL,(nOptions & 7),0);
			EnableWindow(GetDlgItem(hwnd,IDC_CHECK1),(nOptions & 7) == 1);
			SendDlgItemMessage(hwnd,IDC_TRACK1,TBM_SETRANGE,TRUE,MAKELPARAM(0,100));
			iPos = get_Volume();
            SendDlgItemMessage(hwnd,IDC_TRACK1,TBM_SETPOS,(WPARAM)TRUE,iPos);
            wsprintf(s,"%d",ValueToDB(iPos));
            SendDlgItemMessage(hwnd,IDC_EDIT1,WM_SETTEXT,0,(LPARAM)s);
          	SendDlgItemMessage(hwnd,IDC_TRACK2,TBM_SETRANGE,TRUE,MAKELPARAM(1,7));
            SendDlgItemMessage(hwnd,IDC_TRACK2,TBM_SETPOS,(WPARAM)TRUE,(iPos = (nOptions >> 3) & 7));
            wsprintf(s,"%d",iPos);
            SendDlgItemMessage(hwnd,IDC_EDIT2,WM_SETTEXT,0,(LPARAM)s);
			SendDlgItemMessage(hwnd,IDC_BUTTON1,BM_SETCHECK,bStartCapture ? BST_CHECKED :  BST_UNCHECKED,0);
			SendDlgItemMessage(hwnd,IDC_CHECK1,BM_SETCHECK,bWaitOnResample ? BST_CHECKED :  BST_UNCHECKED,0);			
#ifdef _DEBPRO
			SendDlgItemMessage(hwnd,IDC_BUTTON2,BM_SETCHECK,bReportReposition ? BST_CHECKED :  BST_UNCHECKED,0);
#endif			
		break;
#ifdef __WIN32__
		case WM_CTLCOLORSTATIC:
			if(GetDlgCtrlID((HWND)lParam) == IDC_EDIT1 || GetDlgCtrlID((HWND)lParam) == IDC_EDIT2){
				SetTextColor((HDC)wParam,GetSysColor(COLOR_WINDOWTEXT));
	           	return (BOOL)GetSysColorBrush(COLOR_WINDOW);
			}
        break;
#endif
        case WM_HSCROLL:
            switch(LOWORD(wParam)){
                case TB_THUMBTRACK:
                case TB_ENDTRACK:
                    switch(GetDlgCtrlID((HWND)lParam)){
                        case IDC_TRACK1:
							iPos = (int)SendMessage((HWND)lParam,TBM_GETPOS,0,0);
							wsprintf(s,"%d",ValueToDB(iPos));
							SendDlgItemMessage(hwnd,IDC_EDIT1,WM_SETTEXT,0,(LPARAM)s);
							if(nChannel >= 0){
								dschan[nChannel].mix_vol = iPos - 100;
							}
                        break;
                        case IDC_TRACK2:
							iPos = (int)SendMessage((HWND)lParam,TBM_GETPOS,0,0);
							wsprintf(s,"%d",iPos);
							SendDlgItemMessage(hwnd,IDC_EDIT2,WM_SETTEXT,0,(LPARAM)s);
                        break;
                    }
                break;
                default:
                break;
           }
        break;
		case WM_SIZE:
#ifndef __WIN32__
			gtk_widget_set_size_request(hwnd,LOWORD(lParam),HIWORD(lParam));
#endif

			SendDlgItemMessage(hwnd,IDC_TOOLBAR1,WM_SIZE,wParam,lParam);
		break;
		case WM_INITMENUPOPUP:
			for(i=0;i<16;i++)
				CheckMenuItem(hMenu,i+1,MF_BYCOMMAND|(dschan[i].mix_enable ? MF_CHECKED : MF_UNCHECKED));
		break;
        case WM_NOTIFY:
			p = (LPNMHDR)lParam;
            switch(p->code){
				case TBN_DROPDOWN:
					hMenu = GetSubMenu(h = LoadMenu(hInstance,MAKEINTRESOURCE(IDR_TOOLBAR_MENU)),0);
                    ::SendMessage(p->hwndFrom,TB_GETRECT,5555,(LPARAM)&rc);
                    pt.x = rc.left;
                    pt.y = rc.bottom;
                    MapWindowPoints(p->hwndFrom,NULL,&pt,1);
                    TrackPopupMenu(hMenu,TPM_LEFTALIGN,pt.x,pt.y,0,hwnd,NULL);
                    DestroyMenu(h);					
				break;
				case PSN_APPLY:
				   nOptions &= ~0x1FF;
				   nOptions |= (u32)SendDlgItemMessage(hwnd,IDC_SYNCRONIZE,CB_GETCURSEL,0,0);
				   nOptions |= (u32)(SendDlgItemMessage(hwnd,IDC_TRACK2,TBM_GETPOS,0,0) << 3);
				   if(SendDlgItemMessage(hwnd,IDC_RADIO2,BM_GETCHECK,0,0) == BST_CHECKED)
					   nOptions |= 0x40;
				   if(SendDlgItemMessage(hwnd,IDC_RADIO3,BM_GETCHECK,0,0) == BST_CHECKED)
					   nOptions |= 0x80;
				   if(SendDlgItemMessage(hwnd,IDC_CHECK1,BM_GETCHECK,0,0) != BST_CHECKED)
					   nOptions |= 0x100;
				   set_SoundOptions(nOptions);
				   if(nChannel == -1){
						iPos = (int)SendDlgItemMessage(hwnd,IDC_TRACK1,TBM_GETPOS,0,0);
						set_Volume(iPos);
				   }
					i = SendDlgItemMessage(hwnd,IDC_BUTTON1,BM_GETCHECK,0,0) == BST_CHECKED ? TRUE : FALSE;
                   if(bStartCapture != i && bStartCapture)
                       DestroyCaptureFile();
                   bStartCapture = (BOOL)i;
#ifdef _DEBPRO
					bReportReposition = SendDlgItemMessage(hwnd,IDC_BUTTON2,BM_GETCHECK,0,0) == BST_CHECKED ? TRUE : FALSE;				
#endif	
				break;
			}
       break;
	}
	return FALSE;}
//---------------------------------------------------------------------------
extern "C" BOOL I_EXPORT I_STDCALL SaveStateFunc(LStream *p)
{
	return SaveState(p);
}
//---------------------------------------------------------------------------
extern "C" BOOL I_EXPORT I_STDCALL LoadStateFunc(LStream *p,int ver)
{
	return LoadState(p,ver);
}
//---------------------------------------------------------------------------
extern "C" BOOL I_EXPORT I_STDCALL SetPropertyFunc(LPSETPROPPLUGIN p)
{
#ifdef __WIN32__
	PROPSHEETPAGE psp={0};
	HPROPSHEETPAGE hpsp;

   psp.dwSize = sizeof(PROPSHEETPAGE);
   psp.dwFlags = PSP_DEFAULT|PSP_USETITLE;
   psp.hInstance = hLib == NULL ? hInstance : hLib;
   psp.pfnDlgProc = (DLGPROC)DialogProc;
   psp.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG1);
   psp.pszTitle = NAME_PLUGIN;
   hpsp = CreatePropertySheetPage(&psp);
   if(hpsp == NULL)
		return FALSE;
   return PropSheet_AddPage((HWND)p->hwndOwner,hpsp);
#else
	LPPROPSHEETPAGE psp;

	psp = new PROPSHEETPAGE[1];
	if(psp == NULL)
		return NULL;
   psp->dwSize = sizeof(PROPSHEETPAGE);
   psp->dwFlags = PSP_DEFAULT|PSP_USETITLE;
   psp->pfnDlgProc = (DLGPROC)DialogProc;
   psp->pszTemplate = MAKEINTRESOURCE(IDD_DIALOG1);
   psp->pszTitle = NAME_PLUGIN;
	return (BOOL)psp;
#endif
}

