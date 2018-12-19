#include "ideastypes.h"
#include "resource.h"
#include "lds.h"
#include "guitar_grip.h"
#include "language.h"

static WNDPROC oldWindowProc;
static HWND hwndDialog;
static char keySelected = -1;
extern u8 keyGrip[4];
static u16 keys[4];
static u8 keyValue;
//---------------------------------------------------------------------------
static LRESULT CALLBACK EditWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
static void ChangeEditKeyName(LPARAM lParam);
//---------------------------------------------------------------------------
static void SetKeyConfig(u8 *src,u8 mode)
{
   int i;

   if(src == NULL || *src == 0)
       return;
	switch(mode){
       case 0:
           for(i=0;i<4;i++)
#ifdef __WIN32__
               keys[i] = (u16)MapVirtualKey(src[i],0);
#else
				keys[i] = (u16)src[i];
#endif
       break;
       case 1:
           for(i=0;i<4;i++)
#ifdef __WIN32__
               keyGrip[i] = (u8)MapVirtualKey(((u16 *)src)[i],1);
#else
				keyGrip[i] = (u8)((u16 *)src)[i];
#endif
       break;
   }
}
//---------------------------------------------------------------------------
static void OnChangeKeyState(HWND hwndDlg,WORD wID)
{
   int i;

	if(SendMessage(GetDlgItem(hwndDlg,wID),BM_GETCHECK,0,0) == BST_UNCHECKED){
       SetWindowText(GetDlgItem(hwndDlg,IDC_EDIT1),"");
       keySelected = -1;
       return;
   }
   for(i=0;i<4;i++)
       SendDlgItemMessage(hwndDlg,IDC_CHECK2+i,BM_SETCHECK,wID != IDC_CHECK2+i ? BST_UNCHECKED : BST_CHECKED,0);
   keySelected = (u8)(wID - IDC_CHECK2);
   ChangeEditKeyName(keys[keySelected]<<16);
   SetFocus(GetDlgItem(hwndDlg,IDC_EDIT1));
}
//---------------------------------------------------------------------------
static BOOL CALLBACK KeyDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   WORD wID;
   int i;

	switch(uMsg){
		case WM_INITDIALOG:
           hwndDialog = hwndDlg;
           oldWindowProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_EDIT1),GWL_WNDPROC,(LONG)EditWndProc);
       break;
   	case WM_COMMAND:
       	switch(HIWORD(wParam)){
           	case BN_CLICKED:
                   wID = LOWORD(wParam);
               	switch(wID){
                   	case IDOK:
                       case IDCANCEL:
                       	EndDialog(hwndDlg,LOWORD(wParam));
                       break;
                       case IDC_CHECK2:
                       case IDC_CHECK3:
                       case IDC_CHECK4:
                       case IDC_CHECK5:
                       	OnChangeKeyState(hwndDlg,wID);
						break;
                       case IDC_BUTTON1:
                           for(i=0;i<4;i++)
                               SendDlgItemMessage(hwndDlg,IDC_CHECK2+i,BM_SETCHECK,BST_UNCHECKED,0);
                           SetWindowText(GetDlgItem(hwndDlg,IDC_EDIT1),"");
                           SetKeyConfig(keyGrip,0);
                       break;
                   }
               break;
           }
       break;
       case WM_CLOSE:
           EndDialog(hwndDlg,IDCANCEL);
       break;
   }
	return FALSE;
}
//---------------------------------------------------------------------------
void OnGuitarGripKeyConfig()
{
   SetKeyConfig(keyGrip,0);
   if(!DialogBox(langManager.get_CurrentLib(),MAKEINTRESOURCE(IDD_DIALOG23),ds.Handle(),KeyDlgProc))
       return;
   SetKeyConfig((u8 *)keys,1);
}
//---------------------------------------------------------------------------
static LRESULT CALLBACK EditWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg){
       case WM_CHAR:
       case WM_KEYDOWN:
           if(wParam != VK_SPACE && !(lParam & 0x40000000))
               ChangeEditKeyName(lParam);
           return 0;
   	case WM_GETDLGCODE:
           return DLGC_WANTALLKEYS;
   }
#ifdef __WIN32__
   HideCaret(hwnd);
#endif
   return CallWindowProc(oldWindowProc,hwnd,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
static void ChangeEditKeyName(LPARAM lParam)
{
   char szText[100];
   int i;
   u16 k;
   HWND hwnd;

   if(keySelected == -1)
       return;
   k = (u16)((u8)(lParam >> 16));
   for(i=0;i<4;i++){
       if(keySelected == i)
           continue;
       if(k == keys[i]){
           MessageBeep(MB_ICONHAND);
           hwnd = GetDlgItem(hwndDialog,IDC_CHECK2+i);
           for(i=0;i<4;i++){
           	EnableWindow(hwnd,(i & 1) ? TRUE : FALSE);
#ifndef __WIN32__
				gtk_main_iteration_do(false);
#endif
           	SleepEx(60,FALSE);
           }
           break;
       }
   }
   GetKeyNameText(lParam,szText,100);
   SetWindowText(GetDlgItem(hwndDialog,IDC_EDIT1),szText);
   keyValue = keys[keySelected] = k;
}



