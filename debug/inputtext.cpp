#include "ideastypes.h"
#include "inputtext.h"

#ifdef _DEBUG
//---------------------------------------------------------------------------
static WNDPROC oldWndProc;
static int bExit;
static LONG lStyle;
extern HINSTANCE hInst;
//---------------------------------------------------------------------------
static LRESULT CALLBACK WindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	BOOL bFlag;
	LRESULT res;

   bFlag = FALSE;
   switch(uMsg){
   	case WM_KEYDOWN:
       	if(wParam == VK_RETURN && !(lStyle & ES_MULTILINE)){
               bExit = 0;
               res = 0;
               bFlag = TRUE;
           }
           else if(wParam == VK_ESCAPE){
           	res = 0;
               bExit = -1;
               bFlag = TRUE;
           }
           else if(wParam == VK_TAB){
               if(bExit > 0)
       		    bExit = 0;
               res = 0;
               bFlag = TRUE;
           }
       break;
       case WM_KILLFOCUS:
           if(bExit > 0)
               bExit = -1;
       break;
   }
	if(!bFlag)
   	res = CallWindowProc(oldWndProc,hwnd,uMsg,wParam,lParam);
   return res;
}
//---------------------------------------------------------------------------
BOOL InputCombo(HWND parent,LPRECT rcPos,DWORD dwStyle,char *string,int maxlen)
{
	HWND hwnd,hwnd1;
   HFONT hFont;
   HDC hdc;
   SIZE sz;
	MSG msg;
   char *s;
	int imaxlen,i,item,i1;

   rcPos->bottom -= rcPos->top;
   rcPos->right -= rcPos->left;
   lStyle = dwStyle;
   hwnd = CreateWindowEx(0,"COMBOBOX","",
       dwStyle|WS_BORDER|WS_CHILD|CBS_DROPDOWNLIST | WS_VSCROLL | CBS_AUTOHSCROLL | WS_HSCROLL,
       rcPos->left,rcPos->top,rcPos->right,rcPos->bottom+70,parent,NULL,hInst,NULL);
   if(hwnd == NULL)
       return FALSE;
   imaxlen = 0;
   if((s = string) != NULL){
   	i1 = 0;
   	while(*s != 0){
			SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)s);
       	s += (i = lstrlen(s)) + 1;
           if(i > imaxlen){
           	imaxlen = i;
               item = i1;
           }
           i1++;
   	}
       s++;
       if(*s != 0)
       	SendMessage(hwnd,CB_SELECTSTRING,-1,(LPARAM)s);
   }
	bExit = 1;
   oldWndProc = (WNDPROC)::SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WindowProc);
   hFont = (HFONT)SendMessage(parent,WM_GETFONT,0,0);
   SendMessage(hwnd,WM_SETFONT,(WPARAM)hFont,0);
   if((hdc = GetDC(hwnd)) != NULL){
       GetTextExtentPoint32(hdc,"X",1,&sz);
       ::ReleaseDC(hwnd,hdc);
       sz.cy += (GetSystemMetrics(SM_CYEDGE) * 2);
       if(sz.cy >= rcPos->bottom)
           MoveWindow(hwnd,rcPos->left,rcPos->top,rcPos->right,sz.cy,FALSE);
		if(imaxlen){
       	imaxlen *= sz.cx;
           if(SendMessage(hwnd,CB_GETDROPPEDWIDTH,0,0) < imaxlen)
           	SendMessage(hwnd,CB_SETDROPPEDWIDTH,imaxlen,0);
       }
   }
   ShowWindow(hwnd,SW_SHOW);
   SetFocus(hwnd);
   while(bExit > 0){
		GetMessage(&msg,NULL,0,0);
/*       if(msg.message == WM_LBUTTONDOWN){
           hwnd1 = msg.hwnd;
           while(hwnd1 != NULL){
               if(hwnd1 == hwnd)
                   break;
               hwnd1 = ::GetParent(hwnd1);
           }
           if(hwnd1 != hwnd){
               if(bExit > 0)
                   bExit = -1;
           }
       }*/
#ifdef __WIN32__
       TranslateMessage(&msg);
       DispatchMessage(&msg);
#endif
   }
	if(bExit == 0 && string){
   	*((int *)string) = SendMessage(hwnd,CB_GETCURSEL,0,0);
       GetWindowText(hwnd,&string[4],maxlen-4);
   }
   ::DestroyWindow(hwnd);
   if(bExit == 0)
   	return TRUE;
 	return FALSE;
}
//---------------------------------------------------------------------------
BOOL InputText(HWND parent,LPRECT rcPos,DWORD dwStyle,char *string,int maxlen)
{
	HWND hwnd;
   HFONT hFont;
   HDC hdc;
   SIZE sz;
	MSG msg;
                                                  
   rcPos->bottom -= rcPos->top;
   rcPos->right -= rcPos->left;
   lStyle = dwStyle;
#ifdef __WIN32__
   hwnd = CreateWindowEx(0,"EDIT",string,
       dwStyle|WS_BORDER|WS_CHILD|ES_LEFT|ES_AUTOHSCROLL|ES_MULTILINE,
       rcPos->left,rcPos->top,rcPos->right,rcPos->bottom,parent,NULL,hInst,NULL);
   if(hwnd == NULL)
       return FALSE;

#else
	
	hwnd = gtk_entry_new();
   if(hwnd == NULL)
       return FALSE;
	gtk_widget_set_size_request(hwnd,rcPos->right,rcPos->bottom);
	HWND hwnd1;
	RECT rc;

	if(GTK_IS_FIXED(parent))
		hwnd1 = parent;
	else{
		hwnd1 = (HWND)GetWindowLong(parent,GWL_FIXED);
	}	
	if(hwnd1 != NULL){
		gtk_fixed_put((GtkFixed *)hwnd1,hwnd,rcPos->left,rcPos->top);
	}
	if(dwStyle & WS_VISIBLE)
		gtk_widget_show(hwnd);
	if(dwStyle & WS_DISABLED)
		gtk_widget_set_sensitive(hwnd,false);
	gtk_entry_set_text((GtkEntry *)hwnd,string);
#endif
	bExit = 1;
   oldWndProc = (WNDPROC)::SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WindowProc);
   hFont = (HFONT)SendMessage(parent,WM_GETFONT,0,0);
   SendMessage(hwnd,WM_SETFONT,(WPARAM)hFont,0);
   if((hdc = GetDC(hwnd)) != NULL){
       GetTextExtentPoint32(hdc,"X",1,&sz);
       ::ReleaseDC(hwnd,hdc);
       sz.cy += (GetSystemMetrics(SM_CYBORDER) * 2);
       if(sz.cy >= rcPos->bottom)
           MoveWindow(hwnd,rcPos->left,rcPos->top,rcPos->right,sz.cy,FALSE);
   }
   ShowWindow(hwnd,SW_SHOW);
   SetFocus(hwnd);
   SendMessage(hwnd,EM_SETSEL,0,-1);
   while(bExit > 0){
		GetMessage(&msg,NULL,0,0);
#ifdef __WIN32__
       if(msg.message == WM_LBUTTONDOWN && msg.hwnd != hwnd){
           if(bExit > 0)
               bExit = -1;
       }
       TranslateMessage(&msg);
       DispatchMessage(&msg);
#endif
   }
	if(bExit == 0)
   	GetWindowText(hwnd,string,maxlen);
   ::DestroyWindow(hwnd);
   if(bExit == 0)
   	return TRUE;
 	return FALSE;
}

#endif
