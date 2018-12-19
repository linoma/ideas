#include "ideastypes.h"
#include "memview.h"
#include "resource.h"
#include "debug.h"
#include "util.h"
#include "io.h"
#include "language.h"
#include <ctype.h>

#if defined(_DEBUG)
//---------------------------------------------------------------------------
LMemoryView::LMemoryView() : LWnd()
{
	lpszText = NULL;
	hFont = NULL;
	sz.cx = sz.cy = 0;
   bEditMemory = FALSE;
   hDC = NULL;
   hBitmap = NULL;
	Reset();
}
//---------------------------------------------------------------------------
LMemoryView::~LMemoryView()
{
	Destroy();
}
//---------------------------------------------------------------------------
BOOL LMemoryView::Destroy()
{
	KillTimer(m_hWnd,1);
   KillTimer(m_hWnd,2);
	if(hDC != NULL)
   	DeleteDC(hDC);
   hDC = NULL;
   if(hBitmap != NULL)
   	::DeleteObject(hBitmap);
   hBitmap = NULL;
	if(lpszText != NULL)
   	delete lpszText;
   lpszText = NULL;
	if(m_hWnd != NULL)
   	DestroyCaret();
#ifndef __WIN32__
	if(hFont != NULL)
		DeleteObject(hFont);
#endif
   hFont = NULL;
   memPages.Clear();
   return TRUE;
}
//---------------------------------------------------------------------------
void LMemoryView::Reset()
{
   LMemPage *p;
   DWORD dwPos;
   
   p = (LMemPage *)memPages.GetFirstItem(&dwPos);
   while(p != NULL){
       p->set_Sel(0);
       p = (LMemPage *)memPages.GetNextItem(&dwPos);
   }
   if(m_hWnd != NULL){
       KillTimer(m_hWnd,1);
       KillTimer(m_hWnd,2);
   }
   dwInterval = 0;
   ZeroMemory(crAddress,sizeof(crAddress));
   Update();
}
//---------------------------------------------------------------------------
void LMemoryView::Init(LPVOID p)
{
   if(!memPages.Init())
   	return;
	parent = p;
   Attack(GetDlgItem(((LWnd *)p)->Handle(),IDC_DEBUG_MEMVIEW));
	memCBView.Init(p,IDC_DEBUG_MEMVIEW_CB1,this);
   set_ActivePage(1);
}
//---------------------------------------------------------------------------
void LMemoryView::OnVScroll(WPARAM wParam,LPARAM lParam)
{
	SCROLLINFO si ={0};
   HWND hwnd;
	int x,inc;
	SIZE sz1;
   LMemPage *pPage;

   if((pPage = memPages.get_ActivePage()) == NULL)
   	return;
   hwnd = (HWND)lParam;
   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_ALL;
   if(GetScrollInfo(hwnd,SB_CTL,&si) == 0 || !get_ItemsPage(&sz1))
   	return;
	inc = (si.nPage / sz1.cy);
   switch(LOWORD(wParam)){
   	case SB_PAGEUP:
       	if((x = pPage->get_ScrollPos() - si.nPage) < 0)
           	x = 0;
       break;
       case SB_TOP:
         	x = 0;
       break;
       case SB_BOTTOM:
         	x = (si.nMax - si.nPage) + 1;
       break;
       case SB_PAGEDOWN:
         	x = pPage->get_ScrollPos() + si.nPage;
           if(x > (int)((si.nMax - si.nPage) + 1))
           	x = (si.nMax - si.nPage) + 1;
       break;
       case SB_LINEUP:
         	x = pPage->get_ScrollPos() - inc;
           if(x < 0)
             	x = 0;
       break;
       case SB_LINEDOWN:
         	x = pPage->get_ScrollPos() + inc;
           if(x > (int)((si.nMax - si.nPage) + 1))
             	x = (si.nMax - si.nPage) + 1;
       break;
       case SB_THUMBPOSITION:
         	x = si.nPos;
       break;
       case SB_THUMBTRACK:
         	x = si.nTrackPos;
       break;
       default:
         	x = pPage->get_ScrollPos();
       break;
	}
	x = (x / inc) * inc;
   if(x == pPage->get_ScrollPos())
   	return;
   pPage->set_ScrollPos(x);
//   ::SetScrollPos(hwnd,SB_CTL,x,TRUE);
	si.nPos = x;
   si.fMask = SIF_POS;
	SetScrollInfo(hwnd,SB_CTL,&si,TRUE);
	Update();
}
//---------------------------------------------------------------------------
void LMemoryView::OnCommand(WORD wNotifyCode,WORD wID,HWND hwnd)
{
	int i;
	LMemPage *pPage;

	switch(wID){
   	case IDC_DEBUG_MEMVIEW_CB1:
       	switch(wNotifyCode){
           	case CBN_SELENDOK:
                   if((i = ::SendMessage(hwnd,CB_GETCURSEL,0,0)) != CB_ERR){
                       if((pPage = memPages.get_ActivePage()) != NULL){
   						pPage->set_Item(i);
       					UpdateScrollbar(TRUE);
       					Update();
                       }
                   }
               break;
           }
       break;
   }
}
//---------------------------------------------------------------------------
void LMemoryView::OnDeleteBookmarkMemory(int index)
{
	LMemPage *pPage;

   if((pPage = memPages.get_ActivePage()) == NULL)
   	return;
   pPage->del_BookMark(index);
}
//---------------------------------------------------------------------------
void LMemoryView::OnGotoAddress(DWORD dw,BOOL bAdd,BOOL bRedraw)
{
   MEMORYINFO *p;
	int i;
	LMemPage *pPage;

   i = debugDlg.MemoryAddressToIndex(dw);
   if(i != -1){
   	p = &debugDlg.get_CurrentCPU()->r_memmap()[i];
       if((pPage = memPages.get_ActivePage()) != NULL){
           pPage->set_Item(i);
           pPage->set_ScrollPos((int)((dw /*- p->Address*/) % p->Size));
           i = memPages.IndexFromEle(pPage);
			((LToolsDlg *)parent)->SelectPage(i-1);
       	UpdateScrollbar(!bRedraw);
           if(bRedraw)
       		Update();
       	if(bAdd && bRedraw)
       		pPage->AddBookMark(dw);
       }
   }
   if(bRedraw)
		SetFocus(GetDlgItem(((LWnd *)parent)->Handle(),IDC_DEBUG_MEMVIEWSB));
}
//---------------------------------------------------------------------------
void LMemoryView::OnPaint(HDC hdc,RECT &rcClip)
{
	char *p,adr[30],*p1;
   int y,x,badr,bsel,btex,x1,x2;
	char c;
   RECT rc;
	HDC hdc1;
   SIZE sz1;

	hdc1 = hdc;
   if(hDC != NULL && hBitmap != NULL){
		SelectObject(hDC,hBitmap);
		hdc1 = hDC;
   }
   ::GetClientRect(m_hWnd,&rc);
	FillRect(hdc1,&rc,GetSysColorBrush(COLOR_WINDOW));
	if(lpszText == NULL)
   	return;
	x = y = 1;
   btex = 0;
   SelectObject(hdc1,hFont);
   SetTextAlign(hdc1,TA_NOUPDATECP);
	SetBkMode(hdc1,OPAQUE);
   get_ItemsPage(&sz1);
   x1 = sz1.cx;
   get_ItemSize(&sz1);
   x1 = x1 * sz1.cx + 11 * sz.cx + 5;
   x2 = sz.cx;
   for(p = lpszText,p1 = adr,badr = 0;*p != 0;p++){
   	c = *p;
   	if(c == 13){
       	*p++;
   		y += sz.cy;
           x = 1;
           x2 = sz.cx;
           badr = btex = 0;
           if(*p != 0)
				continue;
           else
           	break;
       }
       else if(c == 10){
           btex = 1;
           SetTextColor(hdc1,GetSysColor(COLOR_WINDOWTEXT));
   			SetBkColor(hdc1,GetSysColor(COLOR_WINDOW));
           x = x1;
           x2 = szLetterFont.cx;
           continue;
       }
       if(!badr){
           *p1++ = c;
           if(c == ':'){
           	badr = 1;
               *p1++ = *(++p);
               *p1 = 0;
               p1 = adr;
   			SetTextColor(hdc1,RGB(0,0,200));
   			SetBkColor(hdc1,GetSysColor(COLOR_WINDOW));
       		::SetRect(&rc,x,y,x+11*sz.cx,y+sz.cy);
       		ExtTextOut(hdc1,x,y,ETO_OPAQUE,&rc,p1,lstrlen(p1),NULL);
               x += 11 * x2;
               bsel = 0;
           }
       	continue;
       }
       if(!bsel && !btex){
           switch((c&0xf) - 1){
				case 1:
   				SetTextColor(hdc1,GetSysColor(COLOR_HIGHLIGHTTEXT));
   				SetBkColor(hdc1,GetSysColor(COLOR_HIGHLIGHT));
               break;
               case 2:
               	if((c >> 4)){
           			SetBkColor(hdc1,RGB(0x80,0,0));
           			SetTextColor(hdc1,GetSysColor(COLOR_WINDOW));
               	}
               	else{
           			SetBkColor(hdc1,GetSysColor(COLOR_WINDOW));
           			SetTextColor(hdc1,RGB(0xFF,0,0));
               	}
				break;
				case 3:
               	if((c >> 4)){
           			SetBkColor(hdc1,RGB(0,0x80,0));
           			SetTextColor(hdc1,GetSysColor(COLOR_WINDOW));
               	}
               	else{
           			SetBkColor(hdc1,GetSysColor(COLOR_WINDOW));
           			SetTextColor(hdc1,RGB(0,0xFF,0));
               	}
				break;
               default:
               	if(c > 1)
                   	c = c;
   				SetTextColor(hdc1,GetSysColor(COLOR_WINDOWTEXT));
   				SetBkColor(hdc1,GetSysColor(COLOR_WINDOW));
               break;
           }
           bsel = 1;
           continue;
       }
       if(c == 32)
       	bsel = 0;
       ::SetRect(&rc,x,y,x+sz.cx,y+sz.cy);
       ::DrawText(hdc1,p,1,&rc,DT_LEFT|DT_NOCLIP|DT_SINGLELINE|DT_TOP);
       x += x2;
   }
	if(hdc1 != hdc){
       if(IsRectEmpty(&rcClip))
           ::GetClientRect(m_hWnd,&rcClip);
       ::BitBlt(hdc,rcClip.left,rcClip.top,rcClip.right,rcClip.bottom,hdc1,rcClip.left,rcClip.top,SRCCOPY);
   }
}
//---------------------------------------------------------------------------
BOOL LMemoryView::get_FontSize()
{
   HDC hdc;
   
   if(hFont == NULL && m_hWnd != NULL){
#ifdef __WIN32__
       if((hFont = (HFONT)::SendMessage(m_hWnd,WM_GETFONT,0,0)) == NULL)
           return FALSE;
#else
       PangoContext *context;
       HFONT font;
       PangoFontDescription *font_desc;
       LOGFONT lf={0};
       char *s;
       
       context = gtk_widget_get_pango_context(m_hWnd);
       font_desc = pango_context_get_font_description(context);
       lstrcpy(lf.lfFaceName,pango_font_description_get_family(font_desc));
       lf.lfHeight = pango_font_description_get_size(font_desc) - 1 * PANGO_SCALE;
       hFont = CreateFontIndirect(&lf);
#endif
       if((hdc = GetDC(m_hWnd)) == NULL){
           hFont = NULL;
           return FALSE;
       }
       SelectObject(hdc,hFont);
       GetTextExtentPoint32(hdc,"0",1,&sz);
       GetTextExtentPoint32(hdc,"W",1,&szLetterFont);
		ReleaseDC(m_hWnd,hdc);
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LMemoryView::get_ItemSize(LPSIZE s)
{
   if(s == NULL || !get_FontSize())
   	return FALSE;
   switch(memPages.get_ActivePage()->get_Mode()){
   	case 0:
       	s->cx = 3*sz.cx;
       break;
       case 1:
       	s->cx = 5*sz.cx;
       break;
       case 2:
       	s->cx = 9*sz.cx;
       break;
   }
	s->cy = sz.cy;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LMemoryView::get_ItemsPage(LPSIZE s)
{
   RECT rc;
	SIZE sz1;
   int i;

   if(s == NULL || !get_ItemSize(&sz1))
   	return FALSE;
   ::GetClientRect(m_hWnd,&rc);
#ifndef __WIN32__
	rc.right -= 4;
	rc.bottom -= 4;
#endif
   rc.right -= 11 * sz.cx;
   s->cx = rc.right / sz1.cx;
   s->cy = rc.bottom / sz.cy;
   if(memPages.get_ActivePage()->get_Mode() == 0){
       for(;;){
           i = s->cx * szLetterFont.cx;
           if(rc.right - (s->cx*sz1.cx) < i)
               s->cx--;
           else
               break;
       }
   }
   return TRUE;
}
//---------------------------------------------------------------------------
void LMemoryView::OnChangeCPU()
{
   if(bEditMemory){
       DestroyCaret();
       bEditMemory = FALSE;
   }
   Update();
}
//---------------------------------------------------------------------------
void LMemoryView::Update(BOOL bInvalidate)
{
   u64 dw,dwMax;
	DWORD dw1,dw2,dwSelStart,dwSelEnd;                      //4ce08
   int i,i1,i3,x,nMode;
   char s[31],bRead,cr[]={13,10,0},c,s1[40],*p1;
	SIZE sz1;
	MEMORYINFO *p;
   LMemPage *pPage;
   IARM7 *cpu;
	BOOL bSel;

   if(m_hWnd == NULL)
		return;
   if((pPage = memPages.get_ActivePage()) == NULL)
   	return;
   if(pPage->get_Freeze())
   	goto ex_Update;
	if(lpszText == NULL && (lpszText = new char[20000]) == NULL)
 		return;
	((long *)lpszText)[0] = 0;
	((long *)lpszText)[1] = 0;
	((long *)lpszText)[2] = 0;
	if(!get_ItemsPage(&sz1))
   	return;
	bSel = pPage->get_Sel(&dwSelStart,&dwSelEnd);
   cpu = debugDlg.get_CurrentCPU();
#ifdef _DEBPRO
   if(pPage->get_FollowMe()){
		lstrcpy(s,pPage->get_FollowAddress());
		if((p1 = strchr(s,'r')) != NULL){
           sscanf(p1+1,"%2d",&i);
           wsprintf(s1,"0x%08X",cpu->gp_regs[i]);
           if((p1 = strpbrk(p1+1,"+-*/")) != NULL)
               strcat(s1,p1);
           strcpy(s,s1);
       }
  		dw1 = StrToHex(s);
		OnGotoAddress(dw1,FALSE,FALSE);
   }
#endif
   p = &cpu->r_memmap()[pPage->get_Item()];
   dw = p->Address;
   dwMax = dw;
   dw += pPage->get_ScrollPos();
	i3 = (p->vSize == -1 ? p->Size : p->vSize);
	dwMax += i3;
   i3 = sz1.cx;
   bRead = 1;
   nMode = memPages.get_ActivePage()->get_Mode();
   for(x=i=0;i<sz1.cy && dw < dwMax;i++){
       x += 11;
       wsprintf(s,"%08X : ",dw);
       lstrcat(lpszText,s);
       *((int *)s1) = 0;
       for(i1=0;i1<i3 && dw < dwMax;i1++){
       	dw2 = cpu->remap_address((u32)dw);
           switch(nMode){
               case 0:
                   if(bRead != 0)
                       dw1 = (DWORD)(u8)cpu->read_mem(dw2,AMM_BYTE);
                   else
                       dw1 = 0;
                   wsprintf(s+1,"%02X ",dw1);
                   if(isprint(dw1))
                       s1[i1] = (char)dw1;
                   else
                       s1[i1] = '.';
                   s1[i1+1] = 0;
                   x += 3;
                   dw++;
               break;
               case 1:
                   if(bRead != 0)
                       dw1 = (DWORD)(u16)cpu->read_mem(dw2,AMM_HWORD);
                   else
                       dw1 = 0;
                   dw += 2;
                   wsprintf(s+1,"%04X ",dw1);
                   x += 5;
               break;
               case 2:
                   if(bRead != 0)
                       dw1 = cpu->read_mem(dw2,AMM_WORD);
                   else
                       dw1 = 0;
                   dw += 4;
                   wsprintf(s+1,"%08X ",dw1);
                   x += 9;
               break;
           }
			if(crAddress[9][0] != 0 && dw2 >= crAddress[9][1] && dw2 < crAddress[9][2]){
               if(crAddress[9][0] & AMM_WRITE)
           		c = 0x13;
               else
					c = 3;
           }
           else if(crAddress[7][0] != 0 && dw2 >= crAddress[7][1] && dw2 < crAddress[7][2]){
               if(crAddress[7][0] & AMM_WRITE)
           		c = 0x14;
               else
					c = 4;
           }
           else{
				c = 1;
           	if(bSel){
					if(dw2 >= dwSelStart && dw2 < dwSelEnd)
               		c = 2;
           	}
           }
			s[0] = c;
           lstrcat(lpszText,s);
       }
       if(*s1){
           dw1 = 10;
           lstrcat(lpszText,(char *)&dw1);
           lstrcat(lpszText,s1);
       }
       lstrcat(lpszText,cr);
   }
ex_Update:
   if(bInvalidate)
   	InvalidateRect(m_hWnd,NULL,FALSE);
   UpdateWindow(m_hWnd);
}
//---------------------------------------------------------------------------
void LMemoryView::UpdateScrollbar(BOOL bRepos)
{
   SCROLLINFO si={0};
	DWORD dw;
   HWND hwnd;
	MEMORYINFO *p;
   SIZE sz1;
   LMemPage *pPage;

   if(m_hWnd == NULL || !get_ItemsPage(&sz1))
       return;
   if((pPage = memPages.get_ActivePage()) == NULL)
   	return;
	switch(pPage->get_Mode()){
   	case 1:
			sz1.cx *= 2;
       break;
       case 2:
       	sz1.cx *= 4;
       break;
   }
   p = &debugDlg.get_CurrentCPU()->r_memmap()[pPage->get_Item()];
   hwnd = GetDlgItem(((LWnd *)parent)->Handle(),IDC_DEBUG_MEMVIEWSB);
   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_ALL;
   dw = ((p->Size - 1) / sz1.cx);
   si.nMax = (dw + 1) * sz1.cx;
	si.nPage = si.nMax > 0 ? sz1.cx * sz1.cy : 1;
   SetScrollInfo(hwnd,SB_CTL,&si,FALSE);
   if(!bRepos)
   	return;
   SetScrollPos(hwnd,SB_CTL,pPage->get_ScrollPos(),TRUE);
}
//---------------------------------------------------------------------------
void LMemoryView::OnMenuSelect(WORD wID)
{
	DWORD dw,dw1,dw2;
	char s[60];
	LFile *pFile;
   MEMORYINFO *p;
   IARM7 *cpu;
   u8 b;
   int i;
	POINT pt;
   char *lptstrCopy;
   HGLOBAL hglbCopy;
#ifdef _DEBPRO2
	LBreakPoint *pbp;
   LMemPage *pPage;
#endif
   
#ifdef _DEBPRO
	if(wID >= ID_MVCB_BK_START && wID <= ID_MVCB_BK_END){
   	wID -= (WORD)ID_MVCB_BK_START;
		if(get_MemoryBookmark(wID,&dw))
       	OnGotoAddress(dw,FALSE);
       return;
   }
#endif
	switch(wID){
       case ID_MEMVIEW_CLOSE:
           i = memPages.IndexFromEle(memPages.get_ActivePage());
           if(i > 1 && set_ActivePage(i-1)){
               SendDlgItemMessage(((LWnd *)parent)->Handle(),IDC_DEBUG_TAB1,TCM_SETCURSEL,i-2,0);
               SendDlgItemMessage(((LWnd *)parent)->Handle(),IDC_DEBUG_TAB1,TCM_DELETEITEM,i-1,0);
               memPages.Delete(i);
           }
       break;
#ifdef _DEBPRO
       case ID_MEMVIEW_SAVEDUMP:
       	dw = dw1 = 0;
			cpu = debugDlg.get_CurrentCPU();
			if(!memPages.get_ActivePage()->get_Sel(&dw,&dw1)){
				p = &cpu->MemoryMap[memPages.get_ActivePage()->get_Item()];
               dw = p->Address;
               dw1 = p->Address + p->Size - 1;
           }
   		if((pFile = new LFile("memorydump.bin")) != NULL){
           	if(pFile->Open(GENERIC_WRITE,CREATE_ALWAYS)){
               	for(;dw <= dw1;dw++){
						b = (u8)cpu->read_mem(dw,AMM_BYTE);
                       pFile->Write(&b,1);
                   }
               }
           	delete pFile;
           }
       break;
       case ID_MEMVIEW_FIND:
           SendDlgItemMessage(((LToolsDlg *)parent)->Handle(),IDC_DEBUG_MEMVIEW_CB1,WM_GETTEXT,(WPARAM)60,(LPARAM)s);
   		if(lstrlen(s) == 0)
               return;
           p = &(cpu = debugDlg.get_CurrentCPU())->MemoryMap[memPages.get_ActivePage()->get_Item()];
           for(dw = 0;dw<p->Size;dw++){
               b = (u8)cpu->read_mem(dw+p->Address,AMM_BYTE);
               if(s[0] != b)
                   continue;
               for(i=1;i<lstrlen(s);i++){
                   if(s[i] != cpu->read_mem(dw+p->Address+i,AMM_BYTE))
                       break;
               }
               if(i == lstrlen(s)){
                   OnGotoAddress(dw+p->Address,FALSE);
                   break;
               }
           }
       break;
       case ID_MVCB_BK_DELALL:
       	OnDeleteBookmarkMemory(-1);
       break;
       case ID_MEMVIEW_FREEZE:
			pPage = memPages.get_ActivePage();
           if(pPage != NULL)
           	pPage->set_Freeze(!pPage->get_Freeze());
       break;
       case ID_MEMVIEW_FOLLOWME:
			pPage = memPages.get_ActivePage();
           if(pPage != NULL){
               b = !pPage->get_FollowMe();
               *((int *)s) = 0;
               if(b){
                   i = SendDlgItemMessage(floatDlg.Handle(),IDC_DEBUG_MEMVIEW_CB1,CB_GETCURSEL,0,0);
                   if(i == -1)
               		GetDlgItemText(floatDlg.Handle(),IDC_DEBUG_MEMVIEW_CB1,s,59);
               }
               pPage->set_FollowAddress(s);               
           }
       break;
#endif
#ifdef _DEBPRO2
       case ID_MEMVIEW_BREAKPOINT:
           pbp = NULL;
			if(!memPages.get_ActivePage()->get_Sel(&dw,&dw1)){
				pt = ptMenu;
           	ScreenToClient(&pt);
				if(PointToAddress(pt,&dw))
           		pbp = debugDlg.prgBP.Add(dw,BT_MEMORY);
           }
           else{
           	pbp = debugDlg.prgBP.Add(dw,BT_MEMORY);
              	pbp->set_Address2(dw1);
           }
           if(pbp != NULL){
               pbp->set_Enable();
           	pbp->set_Flags(0xB3);
				if(floatDlg.get_ActivePageType() == PG_MEMBREAKPOINT)
               	debugDlg.prgBP.UpdateList(GetDlgItem(floatDlg.Handle(),IDC_DEBUG_LV1),BT_MEMORY);
           }
       break;
#endif
   	case ID_MEMVIEW_GOTO:
			pt = ptMenu;
           ScreenToClient(&pt);
			if(PointToAddress(pt,&dw)){
               ::SendMessage(debugDlg.Handle(),WM_COMMAND,ID_DEBUG_ADDMEMORYPAGE,0);
               wsprintf(s,"0x%08X",dw);
               SetWindowText(memCBView.Handle(),s);
               OnGotoAddress(dw,FALSE);
			}
         	SetFocus(memCBView.Handle());
       break;
       case ID_MEMVIEW_DBYTES:
       case ID_MEMVIEW_DHWORDS:
       case ID_MEMVIEW_DWORDS:
       	memPages.get_ActivePage()->set_Mode((wID - ID_MEMVIEW_DBYTES));
           UpdateScrollbar(FALSE);
           Update();
       break;
       case ID_MEMVIEW_COPY:
			if(memPages.get_ActivePage()->get_Sel(&dw,&dw1)){
           	if(OpenClipboard(m_hWnd)){
					hglbCopy = GlobalAlloc(GMEM_ZEROINIT|GMEM_DDESHARE,((dw1-dw) + 1) * 3);
        			if(hglbCopy != NULL){
                   	cpu = debugDlg.get_CurrentCPU();
                       EmptyClipboard();
        				lptstrCopy = (char *)GlobalLock(hglbCopy);
						for(dw2=0;dw<dw1;dw++,dw2+=3){
							b = (u8)cpu->read_mem(dw,AMM_BYTE);
                           wsprintf(&lptstrCopy[dw2],"%02X ",b);
                       }
                       GlobalUnlock(hglbCopy);
        				SetClipboardData(CF_TEXT, hglbCopy);
               	}
              		CloseClipboard();
                   if(hglbCopy != NULL)
                   	GlobalFree(hglbCopy);
               }
           }
       break;
   }
}
//---------------------------------------------------------------------------
void LMemoryView::OnKeyDown(WPARAM wParam,LPARAM lParam)
{
	int mask,inc,nMode;
	u32 value;
	RECT rc;
	SIZE sz1,sz2;
   IARM7 *cpu;
   LPIFUNC *cI;
   LPOFUNC *cO;
   u8 *cIO_mem;

	if(!bEditMemory)
		return;
   nMode = memPages.get_ActivePage()->get_Mode();
   if((wParam > 47 && wParam < 58) || (wParam > 95 && wParam < 106) ||
   	(wParam > 64 && wParam < 71) || wParam == VK_LEFT || wParam == VK_RIGHT){
       inc = 1;
       if((wParam > 47 && wParam < 58))
           wParam -= 48;
       else if((wParam > 95 && wParam < 106))
           wParam -= 96;
       else if((wParam > 64 && wParam < 71))
           wParam -= 55;
       else if(wParam == VK_LEFT)
           inc = -1;
       if(wParam < 16){
           mask = (0xF << editBits);
           cpu = debugDlg.get_CurrentCPU();
           cI = crI;
           cIO_mem = crIO_mem;
           cO = crO;
           if(cpu->r_index() == 9){
               crI = i_func;
               crIO_mem = io_mem;
               crO = o_func;
           }
           else{
               crI = i_func7;
               crO = o_func7;
               crIO_mem = io_mem7;
           }
           switch(nMode){
               case 0:
                   value = cpu->read_mem(editAddress,AMM_BYTE);
               break;
               case 1:
                   value = cpu->read_mem(editAddress,AMM_HWORD);
               break;
               case 2:
                   value = cpu->read_mem(editAddress,AMM_WORD);
               break;
           }
           value &= ~mask;
           value |= (wParam << editBits);
           switch(nMode){
               case 0:
                   cpu->write_mem(editAddress,value,AMM_BYTE);
               break;
               case 1:
                   cpu->write_mem(editAddress,value,AMM_HWORD);
               break;
               case 2:
                   cpu->write_mem(editAddress,value,AMM_WORD);
               break;
           }
           crI = cI;
           crIO_mem = cIO_mem;
           crO = cO;
       }
       rc.right = rc.left = ptPosCaret.x;
       rc.bottom = (rc.top = ptPosCaret.y) + sz.cy;
       if(inc > 0)
           rc.right += sz.cx;
       else
           rc.left -= sz.cx;
       ptPosCaret.x += sz.cx*inc;
       if((editBits == 0 && inc > 0) || (inc < 0 && editBits == ((8 << nMode)-4))){
           editAddress += (1 << nMode) * inc;
           editBits = (u8)(inc < 0 ? 0 : ((8 << nMode)-4));
           ptPosCaret.x += sz.cx*inc;
       }
       else
           editBits -= (u8)(4*inc);
       get_ItemsPage(&sz1);
       get_ItemSize(&sz2);
       sz1.cx = sz1.cx * sz2.cx + (11*sz.cx);
       if(ptPosCaret.x >= sz1.cx){
           ptPosCaret.y += sz.cy;
           ptPosCaret.x = sz.cx * 11;
       }
       else if(ptPosCaret.x < sz.cx*11){
           ptPosCaret.y -= sz.cy;
           ptPosCaret.x = sz1.cx - sz.cx*2;
       }
       SetCaretPos(ptPosCaret.x,ptPosCaret.y);
       Update();
       InvalidateRect(m_hWnd,&rc,FALSE);
       Update(FALSE);
   }
   else if(wParam == VK_RETURN){
   	DestroyCaret();
       bEditMemory = FALSE;
   }
   else if(wParam == VK_TAB){
   	DestroyCaret();
       bEditMemory = FALSE;
       value = GetAsyncKeyState(VK_SHIFT) & 1 ? TRUE : FALSE;
       SetFocus(GetNextDlgTabItem(debugDlg.Handle(),m_hWnd,value));
   }
}
//---------------------------------------------------------------------------
void LMemoryView::set_CurrentAccess(u32 address,u32 access)
{
	u8 index;
   IARM7 *cpu;
  	MEMORYINFO *p;
                                                   //23c9b4c
   cpu = debugDlg.get_CurrentCPU();
   p = &cpu->r_memmap()[debugDlg.MemoryAddressToIndex(address)];
   address = (address & ~(p->Size-1)) | (address & (p->Size - 1));
   index = HIWORD(access);
	crAddress[index][1] = address;
	switch((crAddress[index][0] = (u32)(u8)access) & 0xF){
       case AMM_BYTE:
       	crAddress[index][2] = address + 1;
       break;
   	case AMM_HWORD:
       	crAddress[index][2] = address + 2;
       break;
		case AMM_WORD:
       	crAddress[index][2] = address + 4;
       break;
   }
}
//---------------------------------------------------------------------------
void LMemoryView::EnterEditMemory(POINT &pt)
{
	DWORD dw;

	if(!PointToAddress(pt,&dw))
   	return;
   editAddress = dw;
   switch(memPages.get_ActivePage()->get_Mode()){
       case 0:
   	    editBits = 4;
       break;
       case 1:
   	    editBits = 12;
       break;
       case 2:
   	    editBits = 28;
       break;
   }
   *(&ptPosCaret) = *(&pt);
   SetFocus(m_hWnd);
	if(!bEditMemory)
   	CreateCaret(m_hWnd,NULL,sz.cx,sz.cy);
   SetCaretPos(pt.x,pt.y);
	ShowCaret(m_hWnd);
   bEditMemory = TRUE;
}
//---------------------------------------------------------------------------
BOOL LMemoryView::PointToAddress(POINT &pt,u32 *address)
{
   SIZE s;
   int x,y,xi,yi;
	MEMORYINFO *p;
   LMemPage *pPage;

	if(address == NULL || !get_ItemsPage(&s))
   	return FALSE;
   if((pPage = memPages.get_ActivePage()) == NULL)
   	return FALSE;
   *address = 0;
   if(pt.x < 11 * sz.cx)
   	return FALSE;
   x = (pt.x - 11 * sz.cx) / sz.cx;
   y = pt.y / sz.cy;
	switch(memPages.get_ActivePage()->get_Mode()){
       case 0:
           xi = 3;
           yi = 1;
       break;
   	case 1:
           xi = 5;
           yi = 2;
       break;
       case 2:
       	xi = 9;
           yi = 4;
       break;
   }
   if(pt.x > (11 + (s.cx * xi)) * sz.cx)
       return FALSE;
   x /= xi;
   y *= yi * s.cx;
   p = &debugDlg.get_CurrentCPU()->r_memmap()[pPage->get_Item()];
   *address = p->Address + (x * yi) + y + pPage->get_ScrollPos();
   pt.x = 11 * sz.cx + x * xi * sz.cx;
   pt.y = (y / yi / s.cx) * sz.cy;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LMemoryView::set_ActivePage(int index)
{
	if(!memPages.set_ActivePage(index))
   	return FALSE;
   UpdateScrollbar(TRUE);
   Update();
   return TRUE;
}
//---------------------------------------------------------------------------
void LMemoryView::ReSize(int x,int y,int w,int h)
{
   HBITMAP hBit;
   
   if(hDC == NULL)
       hDC = CreateCompatibleDC(NULL);
#ifdef __WIN32__
   if(hDC != NULL){
       hBit = CreateBitmap(w,h,1,GetDeviceCaps(hDC,BITSPIXEL),NULL);
       SelectObject(hDC,hBit);
   }
   else
#endif
       hBit = NULL;
   if(hBitmap != NULL)
       ::DeleteObject(hBitmap);
   hBitmap = hBit;
   //	LWnd::ReSize(x,y,w,h);
   UpdateScrollbar(TRUE);
   Update();
}
//---------------------------------------------------------------------------
BOOL LMemoryView::NewPage()
{
	LMemPage *pPage;

   if((pPage = new LMemPage()) == NULL)
   	return FALSE;
   return memPages.Add(pPage);
}
//---------------------------------------------------------------------------
void LMemoryView::DoScrolling(POINT pt,RECT rc)
{
	DWORD dwInt;
	if(pt.y > rc.bottom){
      dwInt = 250 / (1 + ((pt.y - rc.bottom) / sz.cy));
      if(dwInt != dwInterval){
         dwInterval = dwInt;
         SetTimer(m_hWnd,2,dwInt,NULL);
         OnTimer(2,0);
      }
   }
   else if(pt.y < 0){
      dwInt = 250 / (1 + (abs(pt.y) / sz.cy));
      if(dwInt != dwInterval){
         dwInterval = dwInt;
         SetTimer(m_hWnd,1,dwInt,NULL);
         OnTimer(1,0);
      }
   }
   else{
		KillTimer(m_hWnd,1);
   	KillTimer(m_hWnd,2);
		dwInterval = 0;
   }
}
//---------------------------------------------------------------------------
void LMemoryView::OnTimer(WPARAM wParam,LPARAM lParam)
{
	LMemPage *pPage;
	SIZE s1;
	MEMORYINFO *p;
	DWORD dw,dwSelEnd;

	pPage = memPages.get_ActivePage();
   pPage->get_Sel(NULL,&dwSelEnd);
	get_ItemSize(&s1);
   p = &debugDlg.get_CurrentCPU()->r_memmap()[pPage->get_Item()];
	switch(wParam){
   	case 1:
           if((dwSelEnd - p->Address) > s1.cx)
           	dw = dwSelEnd + s1.cx;
           else
           	dw = p->Address;
			pPage->set_Sel(dw);
			OnVScroll(MAKEWPARAM(SB_LINEUP,0),(LPARAM)GetDlgItem(((LWnd *)parent)->Handle(),IDC_DEBUG_MEMVIEWSB));
       break;
       case 2:
           if((p->Address + p->Size) > (dwSelEnd + s1.cx))
           	dw = dwSelEnd + s1.cx;
           else
           	dw = p->Address + p->Size - 1;
			pPage->set_Sel(dw);
       	OnVScroll(MAKEWPARAM(SB_LINEDOWN,0),(LPARAM)GetDlgItem(((LWnd *)parent)->Handle(),IDC_DEBUG_MEMVIEWSB));
       break;
   }
}
//---------------------------------------------------------------------------
LRESULT LMemoryView::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   HMENU h,h1;
	POINT pt;
	PAINTSTRUCT ps;
	int i;
	u32 value;
	char s[30];
	LMemPage *pPage;
	RECT rc;
   MEMORYINFO *p;

   switch(uMsg){
       case WM_TIMER:
			OnTimer(wParam,lParam);
       break;
   	case WM_GETDLGCODE:
           return DLGC_WANTALLKEYS;
       case WM_LBUTTONUP:
       	ReleaseCapture();
			KillTimer(m_hWnd,1);
			KillTimer(m_hWnd,2);
       break;
   	case WM_LBUTTONDOWN:
       	if(!(wParam & MK_SHIFT)){
           	if((pPage = memPages.get_ActivePage()) != NULL){
               	pPage->set_Sel(0);
                   Update();
               }
           }
       	pt.x = (signed short)LOWORD(lParam);
           pt.y = (signed short)HIWORD(lParam);
			if(PointToAddress(pt,&value)){
               wsprintf(s,"0x%08X",value);
               SetWindowText(GetDlgItem(((LWnd *)parent)->Handle(),IDC_DEBUG_MEMVIEW_CB1),s);
               SetCapture(m_hWnd);
           }
           SetFocus(GetDlgItem(((LWnd *)parent)->Handle(),IDC_DEBUG_MEMVIEWSB));
       break;
       case WM_KEYDOWN:
       	OnKeyDown(wParam,lParam);
       break;
		case WM_KILLFOCUS:
       	bEditMemory = FALSE;
//       	HideCaret(m_hWnd);
       break;
   	case WM_LBUTTONDBLCLK:
       	pt.x = (signed short)LOWORD(lParam);
           pt.y = (signed short)HIWORD(lParam);
           EnterEditMemory(pt);
       break;
       case WM_ERASEBKGND:
			return TRUE;
   	case WM_PAINT:
       	::BeginPaint(m_hWnd,&ps);
           OnPaint(ps.hdc,ps.rcPaint);
           ::EndPaint(m_hWnd,&ps);
			return 0;
   	case WM_COMMAND:
       	if(HIWORD(wParam) < 2)
       		OnMenuSelect(LOWORD(wParam));
           else
           	OnCommand(HIWORD(wParam),LOWORD(wParam),(HWND)lParam);
       break;
   	case WM_RBUTTONDOWN:
           if((h = LoadMenu(langManager.get_CurrentLib(),MAKEINTRESOURCE(IDR_MEMVIEW_MENU))) != NULL){
               h1 = GetSubMenu(h,0);
               pPage = memPages.get_ActivePage();
               p = &debugDlg.get_CurrentCPU()->r_memmap()[pPage->get_Item()];
               if(!(p->AccessMode & AMM_BYTE))
                   EnableMenuItem(h1,ID_MEMVIEW_DBYTES,MF_GRAYED);
               if(!(p->AccessMode & AMM_HWORD))
                   EnableMenuItem(h1,ID_MEMVIEW_DHWORDS,MF_GRAYED);
               if(!(p->AccessMode & AMM_WORD))
                   EnableMenuItem(h1,ID_MEMVIEW_DWORDS,MF_GRAYED);
               EnableMenuItem(h1,ID_MEMVIEW_GOTO,pPage->get_Mode() == 2 ? MF_ENABLED : MF_GRAYED);
               CheckMenuRadioItem(h1,ID_MEMVIEW_DBYTES,ID_MEMVIEW_DWORDS,ID_MEMVIEW_DBYTES + pPage->get_Mode(),MF_BYCOMMAND);
               EnableMenuItem(h1,ID_MEMVIEW_CLOSE,(memPages.Count() > 1 && memPages.IndexFromEle(pPage) > 1) ? MF_ENABLED : MF_GRAYED);
				EnableMenuItem(h1,ID_MEMVIEW_COPY,(pPage->get_Sel(NULL,NULL)) ? MF_ENABLED : MF_GRAYED);
               CheckMenuItem(h1,ID_MEMVIEW_FREEZE,pPage->get_Freeze() ? MF_CHECKED : MF_UNCHECKED);
               CheckMenuItem(h1,ID_MEMVIEW_FOLLOWME,pPage->get_FollowMe() ? MF_CHECKED : MF_UNCHECKED);               
           	GetCursorPos(&pt);
#ifdef _DEBPRO
               i = 0;
           	while(get_MemoryBookmark(i,&value)){
           		if(!i)
						AppendMenu(GetSubMenu(h1,3),MF_SEPARATOR,0,NULL);
           		wsprintf(s,"0x%08X",value);
           		AppendMenu(GetSubMenu(h1,3),MF_STRING,i + ID_MVCB_BK_START,s);
               	i++;
           	}
#endif
               ptMenu = pt;
          	 	TrackPopupMenu(h1,TPM_LEFTALIGN,pt.x,pt.y,0,m_hWnd,NULL);
           	DestroyMenu(h);
				return 0;
           }
       break;
       case WM_MOUSEMOVE:
       	if(wParam & MK_LBUTTON){
       		pt.x = (signed short)LOWORD(lParam);
           	pt.y = (signed short)HIWORD(lParam);
              	GetClientRect(&rc);
               if(PtInRect(&rc,pt)){
               	if(PointToAddress(pt,&value)){
						if(memPages.get_ActivePage()->set_Sel(value))
                       	Update();
                   }
               }
               else
               	DoScrolling(pt,rc);
           }
       break;
   }
	return ::CallWindowProc(oldWndProc,m_hWnd,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LMemCBView::LMemCBView() : LCBView()
{
}
//---------------------------------------------------------------------------
LMemCBView::~LMemCBView()
{
}
//---------------------------------------------------------------------------
LRESULT LMemCBView::OnWindowProcEdit(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	char s[100],s1[20],*p;
	int i;
	DWORD dw;

	switch(uMsg){
   	case WM_GETDLGCODE:
           return DLGC_WANTALLKEYS;
   	case WM_KEYDOWN:
           if(wParam == VK_TAB){
               dw = GetAsyncKeyState(VK_SHIFT) & 1 ? TRUE : FALSE;
               SetFocus(GetNextDlgTabItem(debugDlg.Handle(),m_hWnd,dw));
           }
       	else if(wParam == VK_RETURN){
           	if(parent1 != NULL){
                   GetWindowText(m_hWnd,s,99);
#ifdef _DEBPRO
                   strlwr(s);
                   {
                       char s2[100],*p2;

                       p = s;
                       p2 = s2;
                       *p2 = 0;
                       for(;*p != 0;p++){
                           if(*p == 'r'){
                               sscanf(p+1,"%2d",&i);
                               wsprintf(s1,"0x%08X",debugDlg.get_CurrentCPU()->gp_regs[i]);
                               p++;
                               strcat(p2,s1);
                               p2 += 10;
                               continue;
                           }
                           *p2++ = *p;
                       }
                       *p2 = 0;
                       strcpy(s,s2);
                   }
#endif
					dw = StrToHex(s);
               	((LMemoryView *)parent1)->OnGotoAddress(dw);
               }
			}
       break;
   }
	return LCBView::OnWindowProcEdit(uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LRESULT LMemCBView::OnWindowProcLB(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg){
       case WM_INITMENUPOPUP:
       case WM_INITMENU:
       case WM_CONTEXTMENU:
       	wParam = wParam;
       break;
   	case WM_COMMAND:
           if(hMenu)
           	DestroyMenu(hMenu);
           hMenu = NULL;
       break;
       case WM_CAPTURECHANGED:
           if(hMenu != NULL)
           	return 0;
   }
	return LCBView::OnWindowProcLB(uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LMemPage::LMemPage()
{
	nMode = yScroll = nBMMem = 0;
   nItem = 0;
   selStart = selEnd = 0;
   bFreeze = bFollowMe = FALSE;
  	ZeroMemory(cFollowMe,sizeof(cFollowMe));
}
//---------------------------------------------------------------------------
LMemPage::~LMemPage()
{
}
//---------------------------------------------------------------------------
BOOL LMemPage::get_Sel(u32 *u0,u32 *u1)
{
	if(u0) *u0 = selStart;
   if(u1) *u1 = selEnd;
   if((selStart == selEnd && selStart == 0) || selEnd == 0)
   	return FALSE;
   return TRUE;
}
//---------------------------------------------------------------------------
void LMemPage::set_Item(int i)
{
   MEMORYINFO *p;

   yScroll = 0;
   nItem = i;
   p = &debugDlg.get_CurrentCPU()->r_memmap()[i];
   switch(nMode){
       case 0:
           i = AMM_BYTE;
       break;
       case 1:
           i = AMM_HWORD;
       break;
       case 2:
           i = AMM_WORD;
       break;
   }
   if((p->AccessMode & i))
       return;
   if(p->AccessMode & AMM_BYTE)
       nMode = 0;
   else if(p->AccessMode & AMM_HWORD)
       nMode = 1;
   else if(p->AccessMode & AMM_WORD)
       nMode = 2;
}
//---------------------------------------------------------------------------
BOOL LMemPage::set_Sel(u32 value)
{
	u32 old,old1;
   char s[100];

   old = selStart;
   old1 = selEnd;
	if(value == 0){
       if(selStart != selEnd && selEnd != 0){
       	if(abs(value - selStart) > 20){
				selStart = selEnd = 0;
               goto ex_set_Sel;
           }
       }
       else{
      		selStart = selEnd = 0;
           goto ex_set_Sel;
       }
	}
	if(selStart == selEnd && selStart == 0)
   	selStart = value;
	else if(value <= selStart){
   	if(value < selStart){
       	if(selEnd < selStart)
   			selEnd = selStart;
   		selStart = value;
       }
   }
   else
   	selEnd = value;
ex_set_Sel:
	if((int)(selEnd-selStart) > 0)
   	wsprintf(s,"Sel (byte): %d",(int)(selEnd-selStart));
   else
   	s[0] = 0;
   SendDlgItemMessage(debugDlg.Handle(),IDC_DEBUG_SB1,SB_SETTEXT,1,(LPARAM)s);
	if(selStart != old)
   	return TRUE;
   if(selEnd != old1)
   	return TRUE;
   return FALSE;
}
//---------------------------------------------------------------------------
void LMemPage::del_BookMark(int index)
{
	int i,i1;

	if(index == -1){
   	nBMMem = 0;
       return;
   }
	if(index < 0 || index > nBMMem)
   	return;
   for(i = i1 = 0;i<nBMMem;i++){
       if(i == index)
       	continue;
		*(&bookmarkmem[i1++]) = *(&bookmarkmem[i]);
   }
   nBMMem = i1;
}
//---------------------------------------------------------------------------
void LMemPage::set_FollowAddress(char *string)
{
	LString s;

	if(string == NULL || lstrlen(string) == 0){
   	bFollowMe = FALSE;
       return;
   }
   s = LString(string).AllTrim();
	lstrcpy(cFollowMe,s.c_str());
   bFollowMe = TRUE;
}
//---------------------------------------------------------------------------
LMemPageList::LMemPageList() : LList()
{
	nActivePage = -1;
}
//---------------------------------------------------------------------------
LMemPageList::~LMemPageList()
{
	Clear();
}
//---------------------------------------------------------------------------
LMemPage *LMemPageList::get_ActivePage()
{
	return (LMemPage *)GetItem(nActivePage);
}
//---------------------------------------------------------------------------
BOOL LMemPageList::set_ActivePage(int index)
{
	if(GetItem(index) == NULL)
   	return FALSE;
   nActivePage = index;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LMemPageList::Init()
{
	if(!Add(new LMemPage()))
   	return FALSE;
   nActivePage = 1;
   return TRUE;
}
#endif
