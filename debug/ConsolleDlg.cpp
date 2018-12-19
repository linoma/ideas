#include "debug.h"
#include "lds.h"
#include "resource.h"
#include "lregkey.h"
#include "language.h"
#include "util.h"

#if defined(_DEBUG)
//---------------------------------------------------------------------------
LConsolleDlg::LConsolleDlg() : LDlg()
{
   LRegKey key;

   bStop = FALSE;
	imageListDebug[0] = imageListDebug[1] = NULL;
	pStringConsole = NULL;
   nActiveView = -1;
   FillMemory(showMessage,sizeof(showMessage),1);
   if(key.Open("Software\\iDeaS\\Settings\\Debugger",FALSE))
       key.ReadBinaryData("ConsolleSM",(char *)showMessage,sizeof(showMessage));
}
//---------------------------------------------------------------------------
LConsolleDlg::~LConsolleDlg()
{
	Destroy();
}
//---------------------------------------------------------------------------
void LConsolleDlg::Clear()
{
	if(pStringConsole != NULL)
   	delete pStringConsole;
   pStringConsole = NULL;
	for(int i = 0;i<2;i++){
		if(imageListDebug[i] != 0)
   		ImageList_Destroy(imageListDebug[i]);
       imageListDebug[i] = NULL;
   }
}
//---------------------------------------------------------------------------
BOOL LConsolleDlg::Destroy()
{
   LRegKey key;

   if(key.Open("Software\\iDeaS\\Settings\\Debugger"))
       key.WriteBinaryData("ConsolleSM",(char *)showMessage,sizeof(showMessage));
   Clear();
   nActiveView = -1;
   return LDlg::Destroy();
}
//---------------------------------------------------------------------------
BOOL LConsolleDlg::Create(HWND hwnd)
{
   TBBUTTON tbb[3];
   HBITMAP bit;
	HWND m_hWndTB;

	if(!LDlg::Create(langManager.get_CurrentLib(),MAKEINTRESOURCE(IDD_DIALOG3),hwnd))
   	return FALSE;
  	m_hWndTB = GetDlgItem(m_hWnd,IDC_DEBUG_TB_CON);
   ::SendMessage(m_hWndTB, TB_BUTTONSTRUCTSIZE,(WPARAM) sizeof(TBBUTTON), 0);
   imageListDebug[0] = ImageList_Create(16,16,ILC_COLOR16|ILC_MASK,2,2);
   if(imageListDebug[0] == NULL)
       return FALSE;
   bit = LoadBitmap(hInst,MAKEINTRESOURCE(IDB_TOOLBAR_CONSOLLE));
   if(bit == NULL)
       return FALSE;                                                          
   ImageList_AddMasked(imageListDebug[0],bit,RGB(255,0,255));
   ::SendMessage(m_hWndTB,TB_SETIMAGELIST,0,(LPARAM)imageListDebug[0]);
   ::DeleteObject(bit);

   tbb[0].iBitmap = 0;
   tbb[0].idCommand = ID_DEBUG_CLEARCONSOLLE;
   tbb[0].fsState = TBSTATE_ENABLED;
   tbb[0].fsStyle = TBSTYLE_BUTTON;                         
   tbb[0].dwData = 0;
   tbb[0].iString = -1;                                       

   tbb[1].iBitmap = 1;
   tbb[1].idCommand = ID_FILE_START;
   tbb[1].fsState = TBSTATE_ENABLED;
   tbb[1].fsStyle = 0x80;
   tbb[1].dwData = 0;
   tbb[1].iString = -1;
  	::SendMessage(m_hWndTB, TB_SETEXTENDEDSTYLE, (WPARAM)0,(LPARAM)TBSTYLE_EX_DRAWDDARROWS);
   ::SendMessage(m_hWndTB, TB_ADDBUTTONS, (WPARAM)2,(LPARAM)&tbb);
   imageListDebug[1] = ImageList_LoadImage(hInst,MAKEINTRESOURCE(IDB_TOOLBAR_DISABLED_CONSOLLE),16,2,RGB(255,0,255),IMAGE_BITMAP,LR_DEFAULTCOLOR);
   if(imageListDebug[1] == NULL)
       return FALSE;                                     
   ::SendMessage(m_hWndTB,TB_SETDISABLEDIMAGELIST,0,(LPARAM)imageListDebug[1]);
#ifndef __WIN32__	
	SetWindowLong(m_hWnd,GWL_WNDPROC,(LONG)WindowProc);
	//SetWindowLong(m_hWnd,GWL_LPARAM,(LONG)dwInitParam);
	//SetWindowLong(m_hWnd,GWL_STYLE,(LONG)dlgInfo.style);
	g_signal_connect((gpointer)m_hWnd,"event",G_CALLBACK(on_window_event),NULL);	
	SetWindowLong(m_hWndTB,GWL_NOTIFYPARENT,(LONG)m_hWnd);
	//SetWindowLong(m_hWnd,GWL_NOTIFYPARENT,(LONG)hwnd);
	gtk_widget_subclass(GetDlgItem(m_hWnd,IDC_DEBUG_CONSOLE));
#endif
	return TRUE;
}
//---------------------------------------------------------------------------
LRESULT LConsolleDlg::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LConsolleDlg *p;
   int i;

   p = (LConsolleDlg *)GetWindowLong(m_hWnd,GWL_USERDATA);
	switch(uMsg){
       case WM_INITDIALOG:
           SetWindowLong(GetDlgItem(m_hWnd,IDC_DEBUG_CONSOLE),GWL_USERDATA,(LONG)this);
       	oldListBoxWndProc = (WNDPROC)SetWindowLong(GetDlgItem(m_hWnd,IDC_DEBUG_CONSOLE),GWL_WNDPROC,(LONG)ListBoxWindowProc);
       break;
       case WM_INITMENUPOPUP:
           if(nActiveView == PG_CONSOLLE){
               CheckMenuItem((HMENU)wParam,ID_CONSOLLE_STOP,MF_BYCOMMAND|(bStop ? MF_CHECKED : MF_UNCHECKED));
               for(i = ID_CONSOLLE_MSG_START;i <= ID_CONSOLLE_MSG_STOP;i++)
                   CheckMenuItem((HMENU)wParam,i,MF_BYCOMMAND|(showMessage[i - ID_CONSOLLE_MSG_START] ? MF_CHECKED : MF_UNCHECKED));
           }
       break;
       case WM_SIZE:
#ifndef __WIN32__
			gtk_widget_set_size_request(m_hWnd,LOWORD(lParam),HIWORD(lParam));
#endif
      		SendDlgItemMessage(m_hWnd,IDC_DEBUG_TB_CON,WM_SIZE,wParam,lParam);
           Resize(0,0,0,0);
       break;
       case WM_NOTIFY:
       	if(p != NULL)
           	return p->OnNotify((LPNMHDR)lParam);
       break;
		case WM_COMMAND:
			if(p != NULL)
           	p->OnCommand(LOWORD(wParam),HIWORD(wParam),(HWND)lParam);
       break;
       case WM_DESTROY:
           Clear();
       break;
   }
	return LDlg::OnWindowProc(uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LRESULT CALLBACK LConsolleDlg::ListBoxWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	LConsolleDlg *p;

   p = (LConsolleDlg *)GetWindowLong(hwnd,GWL_USERDATA);
 	if(p != NULL)
   	return p->OnListBoxWindowProc(hwnd,uMsg,wParam,lParam);
   return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LRESULT LConsolleDlg::OnListBoxWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   HMENU menu,submenu;
	POINT pt;
   DWORD dwPos,dw;
	int nCount,nSelCount,*items;
	char c;
	LString *s1;

   switch(uMsg){
   	case WM_COMMAND:
       	if(lParam == 0){
           	switch(LOWORD(wParam)){
                   case ID_CONSOLLE_DELETE_ALL_TYPE:
						nCount = ::SendMessage(hwnd,LB_GETCOUNT,0,0);
                       if(nCount > 0){
                           items = (int *)LocalAlloc(LMEM_FIXED,(nCount + 1) * sizeof(int));
                           ::SendMessage(hwnd,LB_GETSELITEMS,nCount,(LPARAM)items);
							s1 = (LString *)pStringConsole->GetItem(items[0] + 1);
                           c = s1->c_str()[0];
                           LocalFree(items);
                           ::SendMessage(hwnd,LB_RESETCONTENT,0,0);
   						s1 = (LString *)pStringConsole->GetFirstItem(&dwPos);
   						while(s1 != NULL){
								if(s1->c_str()[0] == c){
                                   dw = dwPos;
                               	s1 = (LString *)pStringConsole->GetNextItem(&dwPos);
                                   pStringConsole->RemoveInternal((LPVOID)dw,TRUE);
                               }
                               else{
                                   SendDlgItemMessage(m_hWnd,IDC_DEBUG_CONSOLE,LB_ADDSTRING,0,(LPARAM)(s1+1));
                                   s1 = (LString *)pStringConsole->GetNextItem(&dwPos);
                               }
                           }

                       }
                   break;
               	case ID_CONSOLLE_DESELECT_ALL:
                   	::SendMessage(hwnd,LB_SETSEL,FALSE,(LPARAM)-1);
                   break;
               	case ID_CONSOLLE_SELECT_ALL:
						nCount = ::SendMessage(hwnd,LB_GETCOUNT,0,0);
                       if(nCount > 0)
                       	::SendMessage(hwnd,LB_SELITEMRANGE,TRUE,MAKELPARAM(0,nCount - 1));
                   break;
                   case ID_CONSOLLE_DELETE_ALL:
                   	::SendMessage(hwnd,LB_RESETCONTENT,0,0);
                   break;
                   case ID_CONSOLLE_DELETE_ROW:
                   	nCount = ::SendMessage(hwnd,LB_GETCOUNT,0,0);
                       if(nCount > 0){
                           items = (int *)LocalAlloc(LMEM_FIXED,(nCount + 1) * sizeof(int));
                           ::SendMessage(hwnd,LB_GETSELITEMS,nCount,(LPARAM)items);
                           LocalFree(items);
                       }
                   break;
               }
           }
       break;
       case WM_INITMENUPOPUP:
       	nSelCount = ::SendMessage(hwnd,LB_GETSELCOUNT,0,0);
           nCount = ::SendMessage(hwnd,LB_GETCOUNT,0,0);
           EnableMenuItem((HMENU)wParam,ID_CONSOLLE_SELECT_ALL,MF_BYCOMMAND|(nCount != nSelCount ? MF_ENABLED : MF_GRAYED));
           EnableMenuItem((HMENU)wParam,ID_CONSOLLE_DELETE_ALL,MF_BYCOMMAND|(nSelCount > 0 ? MF_ENABLED : MF_GRAYED));
           EnableMenuItem((HMENU)wParam,ID_CONSOLLE_DELETE_ROW,MF_BYCOMMAND|(nSelCount > 0 ? MF_ENABLED : MF_GRAYED));
           EnableMenuItem((HMENU)wParam,ID_CONSOLLE_DESELECT_ALL,MF_BYCOMMAND|(nSelCount > 0 ? MF_ENABLED : MF_GRAYED));
           EnableMenuItem((HMENU)wParam,ID_CONSOLLE_DELETE_ALL_TYPE,MF_BYCOMMAND|(nSelCount > 0 ? MF_ENABLED : MF_GRAYED));
       break;
       case WM_RBUTTONUP:
       	if(::SendMessage(hwnd,LB_GETCOUNT,0,0) > 0){
               menu = LoadMenu(langManager.get_CurrentLib(),MAKEINTRESOURCE(IDR_CONSOLLE_MENU2));
               submenu = GetSubMenu(menu,0);
               GetCursorPos(&pt);
               TrackPopupMenu(submenu,TPM_LEFTALIGN,pt.x,pt.y,0,hwnd,NULL);
               DestroyMenu(submenu);
           }
       break;
   }
   return CallWindowProc(oldListBoxWndProc,hwnd,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LONG LConsolleDlg::OnNotify(LPNMHDR p)
{
	HMENU h1,h;
	POINT pt;
	RECT rc;

	switch(p->idFrom){
   	case IDC_DEBUG_TB_CON:
       	switch(p->code){
           	case TBN_DROPDOWN:
               	SetWindowLong(m_hWnd,DWL_MSGRESULT,1);
                   if(nActiveView == PG_CONSOLLE){
                       h1 = GetSubMenu(h = LoadMenu(langManager.get_CurrentLib(),MAKEINTRESOURCE(IDR_CONSOLLE_MENU)),0);
                       ::SendMessage(p->hwndFrom,TB_GETRECT,ID_FILE_START,(LPARAM)&rc);
                       pt.x = rc.left;
                       pt.y = rc.bottom;
                       MapWindowPoints(p->hwndFrom,NULL,&pt,1);
                       TrackPopupMenu(h1,TPM_LEFTALIGN,pt.x,pt.y,0,m_hWnd,NULL);
                       DestroyMenu(h);
                   }
               break;
           }
       break;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
void LConsolleDlg::Save()
{
   LFile *pFile;
   LString *s,s1;
   DWORD dwPos;

   if(!pStringConsole->Count())
       return;
   s1.Length(MAX_PATH+1);
   if(!ShowSaveDialog(NULL,s1.c_str(),"Txt Files (*.txt)\0*.txt\0All files (*.*)\0*.*\0\0\0\0\0",NULL))
       return;
   if(s1.IsEmpty())
       s1 = "Console";
   s1.AddEXT(".txt");
   pFile = new LFile(s1.c_str());
   if(pFile == NULL)
       return;
   if(!pFile->Open(GENERIC_WRITE,CREATE_ALWAYS)){
       delete pFile;
       return;
   }
   s = (LString *)pStringConsole->GetFirstItem(&dwPos);
   while(s != NULL){
       pFile->WriteF("%s\r\n",s->c_str()+1);
       s = (LString *)pStringConsole->GetNextItem(&dwPos);
   }
   delete pFile;
}
//---------------------------------------------------------------------------
void LConsolleDlg::OnCommand(WORD wID,WORD wCode,HWND hwnd)
{
   DWORD dw;
   char s[100],s1[11];

   if(wID >= ID_CONSOLLE_MSG_START && wID <= ID_CONSOLLE_MSG_STOP){
       showMessage[wID - ID_CONSOLLE_MSG_START] ^= 1;
       return;
   }
   switch(wID){
       case ID_CONSOLLE_STOP:
           bStop = !bStop;
       break;
       case ID_DEBUG_CLEARCONSOLLE:
           SendDlgItemMessage(m_hWnd,IDC_DEBUG_CONSOLE,LB_RESETCONTENT,0,0);
           if(pStringConsole != NULL)
               pStringConsole->Clear();
       break;
       case ID_CONSOLLE_SAVE:
           Save();
       break;
       case IDC_DEBUG_CONSOLE:
           switch(wCode){
               case LBN_DBLCLK:
                   dw = ::SendMessage(hwnd,LB_GETCURSEL,0,0);
                   if(dw != (DWORD)-1){
                       ::SendMessage(hwnd,LB_GETTEXT,dw,(LPARAM)s);
                       ZeroMemory(s1,11);
                       if(nActiveView == PG_CONSOLLE)
                           strncpy(s1,s,10);
                       else
                           strcpy(s1,s);
                       SendDlgItemMessage(debugDlg.Handle(),IDC_DEBUG_EDITGO,
                           WM_SETTEXT,(WPARAM)0,(LPARAM)s1);
                       ::SendMessage(debugDlg.Handle(),WM_COMMAND,MAKEWPARAM(IDC_DEBUG_GO,BN_CLICKED),
                           (LPARAM)GetDlgItem(debugDlg.Handle(),IDC_DEBUG_GO));
                   }
               break;
           }
       break;
   }
}
//---------------------------------------------------------------------------
void LConsolleDlg::Resize(int x,int y,int w,int h)
{
   HDWP hdwp;
   RECT rc,rc1;
   
   if(m_hWnd == NULL)
       return;
   ::GetWindowRect(GetDlgItem(m_hWnd,IDC_DEBUG_TB_CON),&rc1);
   GetClientRect(&rc);
   rc.left = 0;
   rc.top = 0;
   rc.right -= (rc1.right - rc1.left);
   rc.bottom -= 2;
   if((hdwp = BeginDeferWindowPos(2)) == NULL)
       return;
   hdwp = DeferWindowPos(hdwp,GetDlgItem(m_hWnd,IDC_DEBUG_CONSOLE),0,0,0,rc.right,rc.bottom,SWP_FRAMECHANGED|SWP_NOREPOSITION);
   if(hdwp == NULL)
       return;
   rc.left = rc.right;
   rc.right = w-4;
   hdwp = DeferWindowPos(hdwp,GetDlgItem(m_hWnd,IDC_DEBUG_TB_CON),0,rc.left,rc.top,32,rc.bottom,SWP_FRAMECHANGED|SWP_NOREPOSITION);
   if(hdwp == NULL)
       return;
   EndDeferWindowPos(hdwp);
}
//---------------------------------------------------------------------------
BOOL LConsolleDlg::Reset()
{
   if(pStringConsole == NULL)
   	pStringConsole = new LStringList();
	if(pStringConsole != NULL)
   	pStringConsole->Clear();
	SendDlgItemMessage(m_hWnd,IDC_DEBUG_CONSOLE,LB_RESETCONTENT,0,0);
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LConsolleDlg::WriteMessageConsole(IARM7 *cpu,char *mes,...)
{
	char s[2000];
	va_list ap;
	DWORD adr;

	if(pStringConsole == NULL || bStop)
   	return FALSE;
   va_start(ap, mes);
   if(cpu != NULL)
   	adr = cpu->r_gpreg(-1);
   else
   	adr = 0;
	wvsprintf(&s[1000],mes,ap);
   va_end(ap);
	s[0] = s[1000];
   wsprintf(&s[1],"0x%08X ",adr);
   lstrcpy(&s[12],&s[1001]);
   if(!showMessage[s[0]-1])
       return TRUE;
   if(nActiveView == PG_CONSOLLE)
   	SendDlgItemMessage(m_hWnd,IDC_DEBUG_CONSOLE,LB_ADDSTRING,0,(LPARAM)(s+1));
	return pStringConsole->Add(s);
}
//---------------------------------------------------------------------------
void LConsolleDlg::Update()
{
#ifdef _DEBPRO
	char s1[30];
   IARM7 *cpu;
	unsigned long i;

   if(nActiveView == PG_CALLSTACK){
   	cpu = debugDlg.get_CurrentCPU();
   	if(cpu != NULL){
           SendDlgItemMessage(m_hWnd,IDC_DEBUG_CONSOLE,LB_RESETCONTENT,0,0);
           for(i = cpu->CallStack->count(); i>0; i--){
               wsprintf(s1,"0x%08X",cpu->CallStack->items(i));
               SendDlgItemMessage(m_hWnd,IDC_DEBUG_CONSOLE,LB_ADDSTRING,0,(LPARAM)s1);
           }
       }
   }
#endif
}
//---------------------------------------------------------------------------
void LConsolleDlg::set_ActiveView(int index)
{
#ifdef _DEBPRO
   IARM7 *cpu;
   char s1[30];
#endif
   unsigned long ul;
   LString *pString;
   HWND hwnd;
   int nCmdShow;

   if(nActiveView == index)
       return;
   nCmdShow = SW_SHOW;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_CONSOLE);
   ::SendMessage(hwnd,LB_RESETCONTENT,0,0);
   switch((nActiveView = index)){
       case PG_CONSOLLE:
           pString = (LString *)pStringConsole->GetFirstItem(&ul);
           while(pString != NULL){
               ::SendMessage(hwnd,LB_ADDSTRING,0,(LPARAM)pString->c_str()+1);
               pString = (LString *)pStringConsole->GetNextItem(&ul);
           }
       break;
#ifdef _DEBPRO
       case PG_CALLSTACK:
           nCmdShow = SW_HIDE;
           cpu = debugDlg.get_CurrentCPU();
           if(cpu != NULL){
               for(ul = cpu->CallStack->count(); ul>0;ul--){
                   wsprintf(s1,"0x%08X",cpu->CallStack->items(ul));
                   ::SendMessage(hwnd,LB_ADDSTRING,0,(LPARAM)s1);
               }
           }
       break;
#endif
   }
   ShowWindow(GetDlgItem(m_hWnd,IDC_DEBUG_TB_CON),nCmdShow);
}

#endif

