#include "debug.h"
#include "lds.h"
#include "resource.h"
#include "util.h"
#include "language.h"

#if defined(_DEBUG)
//---------------------------------------------------------------------------
LToolsDlg::LToolsDlg() : LDlg()
{
	hil = NULL;
}
//---------------------------------------------------------------------------
LToolsDlg::~LToolsDlg()
{
   Destroy();
}
//---------------------------------------------------------------------------
BOOL LToolsDlg::OnMeasureItem(UINT id,LPMEASUREITEMSTRUCT p)
{
	TC_ITEM tci={0};


	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LToolsDlg::OnDrawItem(UINT id,LPDRAWITEMSTRUCT p)
{
#ifdef __WIN32__
	TC_ITEM tci={0};
	char s[100];
	RECT rc,rc1;
   UINT uFormat;

   tci.mask = TCIF_TEXT|TCIF_PARAM|TCIF_IMAGE;
   tci.pszText = s;
   tci.cchTextMax = 100;
   TabCtrl_GetItem(p->hwndItem,p->itemID,&tci);
   TabCtrl_GetItemRect(p->hwndItem,p->itemID,&rc);
	FillRect(p->hDC,&p->rcItem,GetSysColorBrush(COLOR_BTNFACE));
   uFormat = DT_VCENTER|DT_SINGLELINE|DT_CENTER|DT_NOCLIP;
   if(LOBYTE(tci.lParam) == PG_MEMVIEW && p->itemID > 0){
   	uFormat &= ~DT_CENTER;
       uFormat |= DT_LEFT;
       InflateRect(&rc,-4,0);
       rc1.right = 13;
		rc1.bottom = rc1.right;
       rc1.left = rc.right - rc1.right;
       rc1.right += rc1.left;
       rc1.top = rc.top + (((rc.bottom - rc.top) - rc1.bottom) >> 1);
       rc1.bottom += rc1.top;
       ImageList_Draw(hil,tci.iImage,p->hDC,rc1.left,rc1.top,ILD_TRANSPARENT);
   }
  	DrawText(p->hDC,s,-1,&rc,uFormat);
#endif
	return TRUE;
}
//---------------------------------------------------------------------------
LRESULT CALLBACK LToolsDlg::TabControlWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	return floatDlg.OnTabControlWindowProc(uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LRESULT LToolsDlg::OnTabControlWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	LRESULT res;
	int iPage;
	POINT pt;
	HWND hwndTab;
	TC_ITEM tci;
	RECT rc,rc1;
	LPARAM oldValue;

   hwndTab = GetDlgItem(m_hWnd,IDC_DEBUG_TAB1);
   res = ::CallWindowProc(oldTabControlWndProc,hwndTab,uMsg,wParam,lParam);
   switch(uMsg){
   	case WM_LBUTTONDOWN:
       	bPressed = FALSE;
          	iPage = ::SendMessage(hwndTab,TCM_GETCURSEL,0,0);
           memset(&tci,0,sizeof(TC_ITEM));
           tci.mask = TCIF_IMAGE|TCIF_PARAM;
           TabCtrl_GetItem(hwndTab,iPage,&tci);
           if((u8)tci.lParam == PG_MEMVIEW){
               TabCtrl_GetItemRect(hwndTab,iPage,&rc);
               rc1 = rc;
               pt.x = (signed short)LOWORD(lParam);
           	pt.y = (signed short)HIWORD(lParam);
       		InflateRect(&rc,-4,0);
               rc.left = rc.right - GetSystemMetrics(SM_CXSMSIZE);
       		rc.top += (((rc.bottom - rc.top) - GetSystemMetrics(SM_CXSMSIZE)) >> 1);
				if(PtInRect(&rc,pt))
               	tci.iImage = 1;
				else
               	tci.iImage = 0;
               TabCtrl_SetItem(hwndTab,iPage,&tci);
//               ::InvalidateRect(hwndTab,&rc,FALSE);
//               ::UpdateWindow(hwndTab);
               bPressed = TRUE;
               SetCapture(hwndTab);
           }
       break;
       case WM_MOUSEMOVE:
       	if(bPressed && (wParam & MK_LBUTTON)){
           	iPage = ::SendMessage(hwndTab,TCM_GETCURSEL,0,0);
               memset(&tci,0,sizeof(TC_ITEM));
               tci.mask = TCIF_IMAGE;
               TabCtrl_GetItem(hwndTab,iPage,&tci);
				TabCtrl_GetItemRect(hwndTab,iPage,&rc);
               rc1 = rc;
           	pt.x = (signed short)LOWORD(lParam);
           	pt.y = (signed short)HIWORD(lParam);
       		InflateRect(&rc,-4,0);
               rc.left = rc.right - GetSystemMetrics(SM_CXSMSIZE);
       		rc.top += (((rc.bottom - rc.top) - GetSystemMetrics(SM_CXSMSIZE)) >> 1);
               oldValue = tci.iImage;
				tci.iImage = PtInRect(&rc,pt) ? 1 : 0;
               if(oldValue != tci.iImage){
               	TabCtrl_SetItem(hwndTab,iPage,&tci);
//               	::InvalidateRect(hwndTab,&rc1,FALSE);
//               	::UpdateWindow(hwndTab);
               }
           }
       break;
       case WM_LBUTTONUP:
           if(bPressed){
           	iPage = ::SendMessage(hwndTab,TCM_GETCURSEL,0,0);
               memset(&tci,0,sizeof(TC_ITEM));
               tci.mask = TCIF_PARAM|TCIF_IMAGE;
               TabCtrl_GetItem(hwndTab,iPage,&tci);
				TabCtrl_GetItemRect(hwndTab,iPage,&rc);
               rc1 = rc;
           	pt.x = (signed short)LOWORD(lParam);
           	pt.y = (signed short)HIWORD(lParam);
       		InflateRect(&rc,-4,0);
               rc.left = rc.right - GetSystemMetrics(SM_CXSMSIZE);
       		rc.top += (((rc.bottom - rc.top) - GetSystemMetrics(SM_CXSMSIZE)) >> 1);
              	tci.iImage = 0;
				if(!PtInRect(&rc,pt))
                   bPressed = FALSE;
               TabCtrl_SetItem(hwndTab,iPage,&tci);
//               ::InvalidateRect(hwndTab,&rc1,FALSE);
//               ::UpdateWindow(hwndTab);
               if(bPressed)
               	::PostMessage(memView.Handle(),WM_COMMAND,ID_MEMVIEW_CLOSE,0);
               bPressed = FALSE;
       		ReleaseCapture();
           }
       break;
   }
	return res;
}
//---------------------------------------------------------------------------
LRESULT LToolsDlg::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   switch(uMsg){
   	case WM_MEASUREITEM:
       	return OnMeasureItem((UINT)wParam,(LPMEASUREITEMSTRUCT)lParam);
       case WM_DRAWITEM:
			return OnDrawItem((UINT)wParam,(LPDRAWITEMSTRUCT)lParam);
       case WM_GETMINMAXINFO:
           ((LPMINMAXINFO)lParam)->ptMinTrackSize.x = 487;
           ((LPMINMAXINFO)lParam)->ptMinTrackSize.y = 164;
       break;
#ifdef __WIN32__
       case WM_WINDOWPOSCHANGING:
           if(!is_Float()){
               ((LPWINDOWPOS)lParam)->hwndInsertAfter = HWND_BOTTOM;
               return 0;
           }
       break;
#endif
       case WM_NCLBUTTONDBLCLK:
       	if(wParam == HTCAPTION)
				::PostMessage(debugDlg.Handle(),WM_COMMAND,MAKEWPARAM(ID_DEBUG_TOOLSWINDOW_TOFLOAT,0),0);
       break;
   	case WM_SIZE:
           OnSize(LOWORD(lParam),HIWORD(lParam));
       break;
       case WM_CLOSE:
       	::PostMessage(debugDlg.Handle(),WM_COMMAND,MAKEWPARAM(ID_DEBUG_TOOLSWINDOW_TOFLOAT,0),0);
       break;
       case WM_VSCROLL:
       	memView.OnVScroll(wParam,lParam);
       break;
       case WM_MOUSEWHEEL:
           {
               TC_ITEM tci = {0};
               int iPage;

               iPage = SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_GETCURSEL,0,0);
               tci.mask = TCIF_PARAM;
               SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_GETITEM,iPage,(LPARAM)&tci);
               if(tci.lParam == PG_MEMVIEW){
                   for(int i=0;i<3;i++)
                       memView.OnVScroll((short)HIWORD(wParam) > 0 ? SB_LINEUP : SB_LINEDOWN,(LPARAM)GetDlgItem(m_hWnd,IDC_DEBUG_MEMVIEWSB));
               }
           }
       break;
       case WM_COMMAND:
       	OnCommand(HIWORD(wParam),LOWORD(wParam),(HWND)lParam);
       break;
       case WM_NOTIFY:
			return OnNotify((LPNMHDR)lParam);
       case WM_ACTIVATE:
           if(!is_Float())
               return 0;
       break;
   }
	return LDlg::OnWindowProc(uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
BOOL LToolsDlg::Create()
{
   TC_ITEM tci={TCIF_TEXT | TCIF_PARAM|TCIF_IMAGE,0,0,NULL,0,0,NULL};
   TV_INSERTSTRUCT tvi={TVI_ROOT,TVI_LAST,{TVIF_TEXT,NULL,0,0,NULL,0,0,0,0}};
   HTREEITEM h[15],hRoot;
   char s[30];
	int i,i1;

	if(!LDlg::Create(langManager.get_CurrentLib(),MAKEINTRESOURCE(IDD_DIALOG2),ds.Handle()))
   	return FALSE;
/*   hil = ImageList_Create(13,13,ILC_COLOR16|ILC_MASK,2,2);
   if(hil != NULL){
   	bit = LoadBitmap(hInst,MAKEINTRESOURCE(7783));
   	if(bit != NULL){
   		ImageList_AddMasked(hil,bit,RGB(255,0,255));
			SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_SETIMAGELIST,0,(LPARAM)hil);
           DeleteObject(bit);
       }
   }
   oldTabControlWndProc = (WNDPROC)SetWindowLong(GetDlgItem(m_hWnd,IDC_DEBUG_TAB1),GWL_WNDPROC,(LONG)TabControlWindowProc);*/
   tci.pszText = "Memory";
   tci.lParam = (LPARAM)PG_MEMVIEW;
	SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_INSERTITEM,(i1 = 0),(LPARAM)&tci);
#if defined(_DEBPRO2)
   tci.lParam = (LPARAM)PG_BREAKPOINT;
   tci.pszText = "Breakpoint";
	SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_INSERTITEM,++i1,(LPARAM)&tci);
   tci.lParam = (LPARAM)PG_MEMBREAKPOINT;
   tci.pszText = "Memory Breakpoint";
	SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_INSERTITEM,++i1,(LPARAM)&tci);
	ListView_SetExtendedListViewStyle(GetDlgItem(m_hWnd,IDC_DEBUG_LV1),LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT);
#endif
#ifdef _DEBPRO
   tci.pszText = "System Control";
   tci.lParam = (LPARAM)PG_SYSTEMCONTROL;
	SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_INSERTITEM,++i1,(LPARAM)&tci);

   tci.pszText = "Call Stack";
   tci.lParam = (LPARAM)PG_CALLSTACK;
	SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_INSERTITEM,++i1,(LPARAM)&tci);

   tvi.item.pszText = "Registers";
   hRoot = tvi.hParent = (HTREEITEM)SendDlgItemMessage(m_hWnd,IDC_DEBUG_SCP,TVM_INSERTITEM,0,(LPARAM)&tvi);
	tvi.item.mask |= TVIF_CHILDREN;
	for(i=0;i<16;i++){
       wsprintf(s,"Register %2d",i);
       tvi.item.cChildren = I_CHILDRENCALLBACK;
		tvi.item.pszText = s;
       tvi.item.mask |= TVIF_PARAM;
       switch(i){
       	case 1:
           	tvi.item.lParam = (LPARAM)syscnt_show_reg1;
           break;
           case 5:
           	tvi.item.lParam = (LPARAM)syscnt_show_reg5;
           break;
           case 6:
           	tvi.item.lParam = (LPARAM)syscnt_show_reg6;
           break;
           case 9:
           	tvi.item.lParam = (LPARAM)syscnt_show_reg9;
           break;
           default:
       		tvi.item.mask &= ~TVIF_PARAM;
				tvi.item.lParam = 0;
       	break;
       }
		h[i] = (HTREEITEM)SendDlgItemMessage(m_hWnd,IDC_DEBUG_SCP,TVM_INSERTITEM,0,(LPARAM)&tvi);
   }
#endif
#ifdef _DEBUG
   tci.pszText = "Console";
   tci.lParam = (LPARAM)PG_CONSOLLE;
	SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_INSERTITEM,++i1,(LPARAM)&tci);
	consolleView.Create(m_hWnd);
#endif
   memView.Init(this);
   memView.Update();
   OnChangeCPU();
   return ShowWindow(m_hWnd,SW_SHOW);
}
//---------------------------------------------------------------------------
BOOL LToolsDlg::IsDialogMessage(LPMSG lpMsg)
{
	if(m_hWnd == NULL || !::IsWindow(m_hWnd))
   	return FALSE;
   if(::IsDialogMessage(m_hWnd,lpMsg))
   	return TRUE;
   if(::IsDialogMessage(memView.Handle(),lpMsg))
   	return TRUE;
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL LToolsDlg::Destroy()
{
#ifdef _DEBUG
	memView.Destroy();
	consolleView.Destroy();
#endif
	if(hil != NULL)
   	ImageList_Destroy(hil);
	return LDlg::Destroy();
}
//---------------------------------------------------------------------------
void LToolsDlg::UpdateCP(u8 reg,u8 r2,u8 op2)
{
	WriteMsgConsolle(&arm9,"%cWrite System Control %d,%d,%d",MSGT_CP,reg,r2,op2);
   if(m_hWnd)
	    RedrawWindow(GetDlgItem(m_hWnd,IDC_DEBUG_SCP),NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
}
//---------------------------------------------------------------------------
void LToolsDlg::OnChangeCPU()
{
   IARM7 *cpu,*cpu2;
	int i,index;
   char s[100];

   index = SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEMVIEW_CB1,CB_GETCURSEL,0,0);
	cpu = debugDlg.get_CurrentCPU();
   if(index != -1)
       SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEMVIEW_CB1,CB_GETLBTEXT,index,(LPARAM)s);
   else{
       cpu2 = (cpu->r_index() == 7) ? (IARM7 *)&arm9 : (IARM7 *)&arm7;
       index = memView.get_ActivePage()->get_Item();
       lstrcpy(s,cpu2->r_memmap()[index].Name);
   }
   SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEMVIEW_CB1,CB_RESETCONTENT,0,0);
   index = -1;
   for(i=0;i<cpu->r_memmapsize();i++){
       if(!lstrcmpi(cpu->r_memmap()[i].Name,s))
           index = i;
		SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEMVIEW_CB1,CB_ADDSTRING,0,(LPARAM)cpu->r_memmap()[i].Name);
   }
   if(index != -1)
       SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEMVIEW_CB1,CB_SETCURSEL,index,0);
   else
       index = memView.get_ActivePage()->get_Item();
   if(index != -1)
       memView.get_ActivePage()->set_Item(index);
   memView.OnChangeCPU();
#ifdef _DEBUG
   consolleView.Update();
#endif
}
//---------------------------------------------------------------------------
void LToolsDlg::OnSelChangeTab1(LPNMHDR h)
{
	int iPage;
	BOOL b[4] = {0};
  	TC_ITEM tci = {0};

   iPage = SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_GETCURSEL,0,0);
   tci.mask = TCIF_PARAM;
	SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_GETITEM,iPage,(LPARAM)&tci);
	switch((u8)tci.lParam){
   	case PG_CONSOLLE:
   		b[2] = TRUE;
       	consolleView.set_ActiveView(PG_CONSOLLE);
		break;
#if defined(_DEBPRO)
       case PG_CALLSTACK:
			b[2] = TRUE;
      		consolleView.set_ActiveView(PG_CALLSTACK);
       break;
       case PG_SYSTEMCONTROL:
       	b[3] = TRUE;
 		break;
       case PG_MEMBREAKPOINT:
			b[1] = TRUE;
       	debugDlg.prgBP.OnShowList(GetDlgItem(m_hWnd,IDC_DEBUG_LV1),BT_MEMORY);
		break;
       case PG_BREAKPOINT:
			b[1] = TRUE;
       	debugDlg.prgBP.OnShowList(GetDlgItem(m_hWnd,IDC_DEBUG_LV1),BT_PROGRAM);
		break;
#elif defined(_DEBPRO2)
       case PG_MEMBREAKPOINT:
			b[1] = TRUE;
       	debugDlg.prgBP.OnShowList(GetDlgItem(m_hWnd,IDC_DEBUG_LV1),BT_MEMORY);
		break;
       case PG_BREAKPOINT:
			b[1] = TRUE;
       	debugDlg.prgBP.OnShowList(GetDlgItem(m_hWnd,IDC_DEBUG_LV1),BT_PROGRAM);
		break;
#endif
		default:
			consolleView.set_ActiveView(-1);
			memView.set_ActivePage(iPage+1);
       	b[0] = TRUE;
       break;
	}
#ifdef _DEBUG
  	::ShowWindow(GetDlgItem(m_hWnd,IDC_DEBUG_MEMVIEW),b[0]);
	::ShowWindow(GetDlgItem(m_hWnd,IDC_DEBUG_MEMVIEWSB),b[0]);
  	::ShowWindow(consolleView.Handle(),b[2]);
#endif

#if defined(_DEBPRO2)
	::ShowWindow(GetDlgItem(m_hWnd,IDC_DEBUG_LV1),b[1]);
#endif
#ifdef _DEBPRO
  	::ShowWindow(GetDlgItem(m_hWnd,IDC_DEBUG_SCP),b[3]);
#endif
}
//---------------------------------------------------------------------------
LONG LToolsDlg::OnNotify(LPNMHDR p)
{
   TV_DISPINFO *p1;
   NM_TREEVIEW *p2;

	switch(p->idFrom){
#if defined(_DEBPRO)
   	case IDC_DEBUG_SCP:
       	switch(p->code){
           	case TVN_GETDISPINFO:
           		p1 = (TV_DISPINFO *)p;
               	if((p1->item.mask & TVIF_CHILDREN)){
               		if(p1->item.lParam != NULL)
							((void (*)(TV_ITEM *,HWND))p1->item.lParam)(&p1->item,p->hwndFrom);
                   	else
                   		p1->item.cChildren = 0;
               	}
           		else if((p1->item.mask & TVIF_TEXT)){
						if(p1->item.lParam != NULL)
                   		((void (*)(TV_ITEM *,HWND))p1->item.lParam)(&p1->item,p->hwndFrom);
               	}
               break;
               case TVN_ITEMEXPANDING:
                   p2 = (NM_TREEVIEW *)p;
               	if(p2->itemNew.lParam != NULL)
						((void (*)(TV_ITEM *,HWND))p2->itemNew.lParam)(&p2->itemNew,p->hwndFrom);
               break;
           }
       break;
#endif
   	case IDC_DEBUG_TAB1:
       	switch(p->code){
           	case TCN_SELCHANGE:
           		OnSelChangeTab1(p);
               break;
           }
       break;
#if defined(_DEBPRO2)
       case IDC_DEBUG_LV1:
			return debugDlg.prgBP.OnNotify((NM_LISTVIEW *)p);
#endif
   }
   return 0;
}
//---------------------------------------------------------------------------
void LToolsDlg::OnCommand(WORD wNotifyCode,WORD wID,HWND hwnd)
{
	switch(wID){
       case IDC_DEBUG_MEMVIEW_CB1:
			memView.OnCommand(wNotifyCode,wID,hwnd);
       break;
   }
}
//---------------------------------------------------------------------------
BOOL LToolsDlg::Reset()
{
#ifdef _DEBUG
	consolleView.Reset();
	memView.Reset();
#endif
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LToolsDlg::OnMenuSelect(WORD wID)
{
   TC_ITEM tci={0};
	char s[30];
	HWND hwnd;
   int i;

	switch(wID){
   	case ID_DEBUG_ADDMEMORYPAGE:
       	if(memView.NewPage()){
               hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_TAB1);
#ifdef _DEBPRO
               wsprintf(s,"Memory %d",(i = TabCtrl_GetItemCount(hwnd) - 5));
#elif defined(_DEBPRO2)
               wsprintf(s,"Memory %d",(i = TabCtrl_GetItemCount(hwnd) - 3));
#else
               wsprintf(s,"Memory %d",(i = TabCtrl_GetItemCount(hwnd) - 1));
#endif
               tci.mask = TCIF_TEXT|TCIF_PARAM|TCIF_IMAGE;
   			tci.lParam = (LPARAM)PG_MEMVIEW;
               tci.pszText = s;
           	TabCtrl_InsertItem(hwnd,i,&tci);
               memView.set_ActivePage(i+1);
               TabCtrl_SetCurSel(hwnd,i);
				ShowWindow(GetDlgItem(m_hWnd,IDC_DEBUG_MEMVIEW),TRUE);
				ShowWindow(GetDlgItem(m_hWnd,IDC_DEBUG_MEMVIEWSB),TRUE);
               ShowWindow(GetDlgItem(m_hWnd,IDC_DEBUG_CONSOLE),FALSE);
               SetFocus(GetDlgItem(m_hWnd,IDC_DEBUG_MEMVIEW_CB1));
           }
           return TRUE;
		case ID_DEBUG_CLEARCONSOLLE:
//			if(pStringConsole != NULL)
//           	pStringConsole->Clear();
      		::SendMessage(consolleView.Handle(),WM_COMMAND,ID_DEBUG_CLEARCONSOLLE,0);
           return TRUE;
   }
	return FALSE;
}
//---------------------------------------------------------------------------
void LToolsDlg::Update()
{
#if defined(_DEBUG)
	consolleView.Update();
	memView.Update();
#endif
}
//---------------------------------------------------------------------------
BOOL LToolsDlg::is_Float()
{
   if(m_hWnd == NULL || !::IsWindow(m_hWnd))
   	return FALSE;
   return (BOOL)(GetWindowLong(m_hWnd,GWL_STYLE) & WS_CAPTION ? TRUE : FALSE);
}
//---------------------------------------------------------------------------
BOOL LToolsDlg::toFloat(BOOL bFloat)
{
	LONG l;
	RECT rc;
	POINT pt;

   if(m_hWnd == NULL || !::IsWindow(m_hWnd))
   	return FALSE;
	if(!bFloat){
   	l = GetWindowLong(m_hWnd,GWL_STYLE);
       l &= ~(WS_POPUP|WS_CAPTION|WS_BORDER|WS_SYSMENU|WS_SIZEBOX);
       l |= WS_CHILD|DS_CONTROL;
       SetWindowLong(m_hWnd,GWL_STYLE,l);
       l = GetWindowLong(m_hWnd,GWL_EXSTYLE);
       l &= ~WS_EX_TOOLWINDOW;
//       l |= WS_EX_TRANSPARENT;
       SetWindowLong(m_hWnd,GWL_EXSTYLE,l);
		SetParent(m_hWnd,debugDlg.Handle());
		::GetWindowRect(GetDlgItem(debugDlg.Handle(),IDC_DEBUG_DIS),&rc);
       MapWindowPoints(NULL,debugDlg.Handle(),(LPPOINT)&rc,2);
       pt.y = rc.bottom;
       pt.x = rc.left;
       ::GetClientRect(debugDlg.Handle(),&rc);
       ::SetWindowPos(m_hWnd,HWND_BOTTOM,0,pt.y,rc.right,200,SWP_FRAMECHANGED|SWP_NOACTIVATE);
   }
   else{
       SetParent(m_hWnd,NULL);
   	l = GetWindowLong(m_hWnd,GWL_STYLE);
       l &= ~(DS_CONTROL|WS_CHILD);
       l |= (WS_POPUP|WS_CAPTION|WS_BORDER|WS_SYSMENU|WS_SIZEBOX);
       SetWindowLong(m_hWnd,GWL_STYLE,l);
       l = GetWindowLong(m_hWnd,GWL_EXSTYLE);
       l &= ~WS_EX_TRANSPARENT;
       l |= WS_EX_TOOLWINDOW;
       SetWindowLong(m_hWnd,GWL_EXSTYLE,l);
       ::SetWindowPos(m_hWnd,0,0,0,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LToolsDlg::OnSize(int width,int height)
{
	HWND hwnd;
	POINT pt;
   RECT rc,rc1;
   HDWP hdwp;

	::GetWindowRect(GetDlgItem(m_hWnd,IDC_DEBUG_MEMVIEW_CB1),&rc);
   MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,2);
   pt.y = rc.bottom+5;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_TAB1);
	if((hdwp = BeginDeferWindowPos(6)) == NULL)
       return FALSE;
   hdwp = ::DeferWindowPos(hdwp,hwnd,HWND_BOTTOM,2,pt.y,width-4,height-pt.y,SWP_FRAMECHANGED|SWP_NOREPOSITION);
	if(hdwp == NULL) return FALSE;
   ::SetRect(&rc,2,pt.y,width-4,height-pt.y);
//   GetClientRect(hwnd,&rc);
   TabCtrl_GetItemRect(hwnd,0,&rc1);
	rc.left += 4;
   rc.right -= 8;
   rc.top += rc1.bottom + 4;
   rc.bottom -= 8 + rc1.bottom;
#if defined(_DEBPRO2)
   hdwp = DeferWindowPos(hdwp,GetDlgItem(m_hWnd,IDC_DEBUG_LV1),0,rc.left,rc.top,rc.right,rc.bottom,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOZORDER);
   if(hdwp == NULL) return FALSE;
#endif
#if defined(_DEBPRO)
   hdwp = DeferWindowPos(hdwp,GetDlgItem(m_hWnd,IDC_DEBUG_SCP),0,rc.left,rc.top,rc.right,rc.bottom,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOZORDER);
   if(hdwp == NULL) return FALSE;
#endif
	::GetWindowRect(hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_MEMVIEWSB),&rc1);
   pt.x = rc.right - (rc1.right - rc1.left) + 4;
   hdwp = DeferWindowPos(hdwp,hwnd,HWND_TOP,pt.x,rc.top,(rc1.right - rc1.left),rc.bottom,SWP_FRAMECHANGED|SWP_NOREPOSITION);
   if(hdwp == NULL) return FALSE;
   hdwp = DeferWindowPos(hdwp,memView.Handle(),0,rc.left,rc.top,rc.right - (rc1.right - rc1.left) - 2,rc.bottom,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOZORDER);
   if(hdwp == NULL) return FALSE;
#ifdef _DEBUG
   hdwp = DeferWindowPos(hdwp,consolleView.Handle(),0,rc.left,rc.top,rc.right,rc.bottom,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOZORDER);
   if(hdwp == NULL) return FALSE;
#endif
   EndDeferWindowPos(hdwp);
	memView.ReSize(rc.left,rc.top,rc.right - (rc1.right - rc1.left) - 2,rc.bottom);
   return TRUE;
}
//---------------------------------------------------------------------------
PAGETYPE LToolsDlg::get_ActivePageType(int iPage)
{
//	int itemCount;
   TC_ITEM tci={0};

   if(iPage == -1)
   	iPage = SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_GETCURSEL,0,0);
	tci.mask = TCIF_PARAM;
	SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_GETITEM,iPage,(LPARAM)&tci);
   return (PAGETYPE)(u8)tci.lParam;
}
//---------------------------------------------------------------------------
BOOL LToolsDlg::SelectPage(int page)
{
	int i;

	i = SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_GETCURSEL,0,0);
   if(i == page)
   	return TRUE;
   SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_SETCURSEL,page,0);
   OnSelChangeTab1(NULL);
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LToolsDlg::SelectPage(PAGETYPE page)
{
	int itemCount,iPage,i;

   i = SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_GETCURSEL,0,0);
   itemCount = SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_GETITEMCOUNT,0,0);
#if defined(DEBPRO)
   switch(page){
       case PG_CONSOLLE:
           iPage = itemCount - 1;
       break;
       case PG_CALLSTACK:
           iPage = itemCount - 4;
       break;
       case PG_SYSTEMCONTROL:
           iPage = itemCount - 3;
       break;
       case PG_MEMBREAKPOINT:
           iPage = itemCount - 4;
       break;
       case PG_BREAKPOINT:
           iPage = itemCount - 5;
       break;
       default:
           iPage = 0;
       break;
   }
#elif defined(_DEBPRO2)
   switch(page){
       case PG_MEMBREAKPOINT:
           iPage = itemCount - 1;
       break;
       case PG_BREAKPOINT:
           iPage = itemCount - 2;
       break;
       default:
           iPage = 0;
       break;
   }
#else
   switch(page){
       case PG_CONSOLLE:
           iPage = itemCount - 1;
       break;
       default:
           iPage = 0;
       break;
   }
#endif
   if(i == iPage)
       return TRUE;
   SendDlgItemMessage(m_hWnd,IDC_DEBUG_TAB1,TCM_SETCURSEL,iPage,0);
   OnSelChangeTab1(NULL);
   return TRUE;
}
#endif
