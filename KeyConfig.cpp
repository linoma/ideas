//---------------------------------------------------------------------------
#include "ideastypes.h"
#include "resource.h"
#include "lds.h"
#include "KeyConfig.h"
#include "language.h"

extern u8 keyBoard[MAX_BUTTONS];
static u8 keyValue;
static u16 keys[MAX_BUTTONS];
static char keySelected = -1;
static WNDPROC oldWindowProc;
static HWND hwndDialog;          
//---------------------------------------------------------------------------
void SetKeyConfig(u8 *src,u8 mode)
{
   int i;

   if(src == NULL || *src == 0)               
       return;
	switch(mode){
       case 0:
           for(i=0;i<MAX_BUTTONS;i++)
#ifdef __WIN32__
               keys[i] = (u16)MapVirtualKey(src[i],0);
#else
				keys[i] = (u16)src[i];
#endif
       break;
       case 1:
           for(i=0;i<MAX_BUTTONS;i++)
#ifdef __WIN32__
               keyBoard[i] = (u8)MapVirtualKey(((u16 *)src)[i],1);
#else
				keyBoard[i] = (u8)((u16 *)src)[i];
#endif
       break;
   }
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
   for(i=0;i<MAX_BUTTONS;i++){
       if(keySelected == i)
           continue;
       if(k == keys[i]){
           MessageBeep(MB_ICONHAND);
           hwnd = GetDlgItem(hwndDialog,IDC_BUTTON_A+i);
           for(i=0;i<6;i++){
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
//---------------------------------------------------------------------------
static void OnChangeKeyState(HWND hwndDlg,WORD wID)
{
   int i;

	if(SendMessage(GetDlgItem(hwndDlg,wID),BM_GETCHECK,0,0) == BST_UNCHECKED){
       SetWindowText(GetDlgItem(hwndDlg,IDC_EDIT1),"");
       keySelected = -1;
       return;
   }
   for(i=0;i<MAX_BUTTONS;i++)
       SendMessage(GetDlgItem(hwndDlg,IDC_BUTTON_A+i),BM_SETCHECK,wID != IDC_BUTTON_A+i ? BST_UNCHECKED : BST_CHECKED,0);
   keySelected = (u8)(wID - IDC_BUTTON_A);
   ChangeEditKeyName(keys[keySelected]<<16);
   SetFocus(GetDlgItem(hwndDlg,IDC_EDIT1));
}
//---------------------------------------------------------------------------
static LRESULT CALLBACK EditWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg){
       case WM_CHAR:
       case WM_KEYDOWN:
           if(!(lParam & 0x40000000) && wParam != VK_SPACE)
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
static BOOL CALLBACK KeyDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	BOOL res;
	WORD wID;
   	int notifyCode;

   res = FALSE;
   switch(uMsg){
       case WM_INITDIALOG:
           SendDlgItemMessage(hwndDlg,IDC_BUTTON_RIGHT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)LoadBitmap(hInst,MAKEINTRESOURCE(IDI_BUTTON_RIGHT)));
           SendDlgItemMessage(hwndDlg,IDC_BUTTON_LEFT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)LoadBitmap(hInst,MAKEINTRESOURCE(IDI_BUTTON_LEFT)));
           SendDlgItemMessage(hwndDlg,IDC_BUTTON_UP,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)LoadBitmap(hInst,MAKEINTRESOURCE(IDI_BUTTON_UP)));
           SendDlgItemMessage(hwndDlg,IDC_BUTTON_DOWN,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)LoadBitmap(hInst,MAKEINTRESOURCE(IDI_BUTTON_DOWN)));
           hwndDialog = hwndDlg;
           oldWindowProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_EDIT1),GWL_WNDPROC,(LONG)EditWndProc);
       break;
       case WM_CLOSE:
           EndDialog(hwndDlg,0);
       break;
       case WM_COMMAND:
           wID = LOWORD(wParam);
           notifyCode = (int)HIWORD(wParam);
           switch(notifyCode){
               case BN_CLICKED:
                   switch(wID){
                       case IDC_BUTTON_RIGHT:
                       case IDC_BUTTON_LEFT:
                       case IDC_BUTTON_UP:
                       case IDC_BUTTON_DOWN:
                       case IDC_BUTTON_START:
                       case IDC_BUTTON_SELECT:
                       case IDC_BUTTON_A:
                       case IDC_BUTTON_B:
                       case IDC_BUTTON_L:
                       case IDC_BUTTON_R:
                       case IDC_BUTTON_X:
                       case IDC_BUTTON_Y:
                           OnChangeKeyState(hwndDlg,wID);
                       break;
                       case IDOK:
                           EndDialog(hwndDlg,1);
                       break;
                       case IDCANCEL:
                           EndDialog(hwndDlg,0);
                       break;
                       case IDC_BUTTON3:
                           for(int i=0;i<MAX_BUTTONS;i++)
                               SendMessage(GetDlgItem(hwndDlg,IDC_BUTTON_A+i),BM_SETCHECK,BST_UNCHECKED,0);
                           SetWindowText(GetDlgItem(hwndDlg,IDC_EDIT1),"");
                           SetKeyConfig(keyBoard,0);
                       break;
                   }
               break;
           }
       break;
   }
   return res;
}
//---------------------------------------------------------------------------
void OnKeyConfig()
{
   SetKeyConfig(keyBoard,0);
   if(!DialogBox(langManager.get_CurrentLib(),MAKEINTRESOURCE(IDD_DIALOG13),ds.Handle(),KeyDlgProc))
       return;
   SetKeyConfig((u8 *)keys,1);
}
//---------------------------------------------------------------------------





