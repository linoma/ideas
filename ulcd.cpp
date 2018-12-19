#include "gpu.h"
#include "lds.h"
#include "resource.h"
#include "dsmem.h"
#include "language.h"
#include "cheats.h"

//---------------------------------------------------------------------------
LUpLcd::LUpLcd() : LGPU()
{
	imageListDebug[0] = imageListDebug[1] = NULL;
   imLed = NULL;
   m_hWndTB = NULL;
   bLockWindows = TRUE;
   type = LCD_SUB;
   nStateLed = 1;
   uiTimerID = 0;
}
//---------------------------------------------------------------------------
LUpLcd::~LUpLcd()
{
   if(m_hWndTB != NULL)
       ::DestroyWindow(m_hWndTB);
   if(uiTimerID != 0)
       KillTimer(NULL,uiTimerID);
   uiTimerID = 0;
   for(int i = 0;i<2;i++){
       if(imageListDebug[i] != 0)
           ImageList_Destroy(imageListDebug[i]);
   }
   if(imLed != NULL)
       ImageList_Destroy(imLed);
#ifndef __WIN32__
   if(GetWindowLong(m_hWnd,GWL_MENU) != NULL)
       gtk_widget_destroy((GtkWidget *)GetWindowLong(m_hWnd,GWL_MENU));
#endif
}
//---------------------------------------------------------------------------
BOOL LUpLcd::CreateWnd()
{
	RECT rc,rc1;
	HBITMAP bit;
   TBBUTTON tbb[12]={0};
#ifndef __WIN32__
	GdkColor color={0,0,0,255};
	int d;          

   if(!CreateEx(WS_EX_ACCEPTFILES,"IDEAS","iDeaS Emulator",WS_CLIPCHILDREN|WS_OVERLAPPED|WS_SYSMENU|WS_MINIMIZEBOX	,
  				CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
               NULL,(HMENU)LoadMenuBar(langManager.get_CurrentLib(),MAKEINTRESOURCE(IDR_FILE_MENU)),hInst,NULL))
   	return FALSE;

	gdk_window_set_background(m_hWnd->window,&color);
	gtk_window_set_icon((GtkWindow *)m_hWnd,LoadIcon(hInst,MAKEINTRESOURCE(2)));
#else
   if(!CreateEx(WS_EX_ACCEPTFILES,"IDEAS","iDeaS Emulator",WS_CLIPCHILDREN|WS_OVERLAPPED|WS_SYSMENU|WS_MINIMIZEBOX	,
  				CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
               NULL,(HMENU)LoadMenu(langManager.get_CurrentLib(),MAKEINTRESOURCE(IDR_FILE_MENU)),hInst,NULL))
   	return FALSE;
#endif
   GetWindowRect(&rc);
   GetClientRect(&rc1);
   rc1.right = ((rc.right - rc.left) - rc1.right) + 256*fScale;
   rc1.bottom = ((rc.bottom - rc.top) - rc1.bottom) + 192*fScale;
   m_hWndTB = CreateWindowEx(0, TOOLBARCLASSNAME, (LPSTR) NULL,
       WS_CHILD |WS_VISIBLE|CCS_TOP|TBSTYLE_FLAT|TBSTYLE_TRANSPARENT|TBSTYLE_TOOLTIPS|TBSTYLE_CUSTOMERASE,
       	0, 0, 0, 0, m_hWnd, (HMENU) IDM_TOOLBAR,hInst, NULL);
   if(m_hWndTB == NULL)
       return FALSE;
   ::SendMessage(m_hWndTB, TB_BUTTONSTRUCTSIZE,(WPARAM) sizeof(TBBUTTON),0);
   imageListDebug[0] = ImageList_Create(16,16,ILC_COLOR16|ILC_MASK,8,8);
   if(imageListDebug[0] == NULL)
       return FALSE;
   bit = LoadBitmap(hInst,MAKEINTRESOURCE(IDB_TOOLBAR));
   if(bit == NULL)
       return FALSE;
   ImageList_AddMasked(imageListDebug[0],bit,RGB(255,0,255));
   ::SendMessage(m_hWndTB,TB_SETIMAGELIST,0,(LPARAM)imageListDebug[0]);
   ::DeleteObject(bit);

   tbb[0].idCommand = ID_FILE_OPEN;
   tbb[0].fsState = TBSTATE_ENABLED;
   tbb[0].fsStyle = TBSTYLE_BUTTON;
   tbb[0].iString = -1;

   tbb[1].fsState = TBSTATE_ENABLED;
   tbb[1].fsStyle = TBSTYLE_SEP;
   tbb[1].iString = -1;

   tbb[2].iBitmap = 1;
   tbb[2].idCommand = ID_FILE_START;
   tbb[2].fsStyle = TBSTYLE_BUTTON;
   tbb[2].iString = -1;

   tbb[3].iBitmap = 2;
   tbb[3].idCommand = ID_FILE_STOP;
   tbb[3].fsStyle = TBSTYLE_BUTTON;
   tbb[3].iString = -1;

   tbb[4].iBitmap = 3;
   tbb[4].idCommand = ID_TRIGGER_HING;
   tbb[4].fsState = TBSTATE_ENABLED;
   tbb[4].fsStyle = TBSTYLE_CHECK;
   tbb[4].iString = -1;

   tbb[5].fsState = TBSTATE_ENABLED;
   tbb[5].fsStyle = TBSTYLE_SEP;
   tbb[5].iString = -1;

   tbb[6].iBitmap = 4;
   tbb[6].idCommand = ID_DEBUG_DEBUGGER;
#ifndef _DEBUG
   EnableMenuItem(GetMenu(),ID_DEBUG_DEBUGGER,MF_BYCOMMAND|MF_GRAYED);
#else
   tbb[6].fsState = TBSTATE_ENABLED;
#endif
   tbb[6].fsStyle = TBSTYLE_BUTTON;
   tbb[6].iString = -1;

   tbb[7].iBitmap = 5;
   tbb[7].idCommand = ID_FILE_PROPERTIES;
   tbb[7].fsState = TBSTATE_ENABLED;
   tbb[7].fsStyle = TBSTYLE_BUTTON;
   tbb[7].iString = -1;

   tbb[8].iBitmap = 6;
   tbb[8].idCommand = ID_OPTIONS_LOCKWINDOW;
   tbb[8].fsState = TBSTATE_ENABLED|TBSTATE_CHECKED;
   tbb[8].fsStyle = TBSTYLE_CHECK;
   tbb[8].iString = -1;

   tbb[9].iBitmap = 7;
   tbb[9].idCommand = ID_INFO_INFO;
   tbb[9].fsState = TBSTATE_ENABLED;
   tbb[9].fsStyle = TBSTYLE_BUTTON;
   tbb[9].iString = -1;

   tbb[10].fsState = TBSTATE_ENABLED;
   tbb[10].fsStyle = TBSTYLE_SEP;
   tbb[10].iString = -1;

   tbb[11].iBitmap = 8;
   tbb[11].idCommand = ID_FILE_EXIT;
   tbb[11].fsState = TBSTATE_ENABLED;
   tbb[11].fsStyle = TBSTYLE_BUTTON;
   tbb[11].iString = -1;

   ::SendMessage(m_hWndTB, TB_ADDBUTTONS, (WPARAM)sizeof(tbb) / sizeof(TBBUTTON),(LPARAM)&tbb);
   imageListDebug[1] = ImageList_LoadImage(hInst,MAKEINTRESOURCE(IDB_TOOLBAR_DISABLED),16,8,RGB(255,0,255),IMAGE_BITMAP,LR_DEFAULTCOLOR);
   if(imageListDebug[1] == NULL)
       return 0;
    ::SendMessage(m_hWndTB,TB_SETDISABLEDIMAGELIST,0,(LPARAM)imageListDebug[1]);
#ifdef __WIN32__
	::GetWindowRect(m_hWndTB,&rc);
   rc1.bottom += rc.bottom - rc.top;
#else
  	gtk_box_pack_start(GTK_BOX (vbox),m_hWndTB,FALSE,FALSE,0);
	::GetWindowRect((HWND)GetWindowLong(m_hWnd,GWL_MENU),&rc);
	rc1.bottom += d = (rc.bottom - rc.top);
	::GetWindowRect((HWND)m_hWndTB,&rc);
	rc1.bottom += (rc.bottom -= rc.top)+1;
	rc.top = 0;
	rc.bottom += d + 1;
#endif
   bit = LoadBitmap(hInst,MAKEINTRESOURCE(100));
   if(bit == NULL)
       return FALSE;
   ::AppendMenu(GetMenu(),MF_OWNERDRAW,1,(LPCTSTR)"W");
   imLed = ImageList_Create(17,17,ILC_COLOR16|ILC_MASK,2,0);
   if(imLed == NULL)
       return FALSE;
	ImageList_AddMasked(imLed,bit,RGB(194,194,194));
   ::DeleteObject(bit);
	langManager.InsertMenu(GetSubMenu(GetMenu(),0),ID_FILE_EXIT);
	Resize(10,10,rc1.right,rc1.bottom);
	if(GetDC(m_hWnd))
		SetOrg(0,(rc.bottom - rc.top));
  	return TRUE;
}
//---------------------------------------------------------------------------
void LUpLcd::UpdateToolBarState(BOOL bLoad,BOOL bStart,BOOL bHinge)
{
   IMenu *m;
   
   if(!bLoad)
       bStart = FALSE;
   m = ds.CoCreateInstance_Menu(GetMenu());
   if(m != NULL){
       m->EnableMenuItem(ID_FILE_START,MF_BYCOMMAND|(bLoad && !bStart ? MF_ENABLED : MF_GRAYED));
       m->EnableMenuItem(ID_FILE_STOP,MF_BYCOMMAND|(bStart ? MF_ENABLED : MF_GRAYED));
       m->EnableMenuItem(ID_FILE_PAUSE,MF_BYCOMMAND|(bStart ? MF_ENABLED : MF_GRAYED));
       m->EnableMenuItem(ID_FILE_RESET,MF_BYCOMMAND|(bStart ? MF_ENABLED : MF_GRAYED));
       m->EnableMenuItem(ID_FILE_STATE_SAVE,MF_BYCOMMAND|(bStart ? MF_ENABLED : MF_GRAYED));
       m->EnableMenuItem(ID_ROM_INFO,MF_BYCOMMAND|(bLoad ? MF_ENABLED : MF_GRAYED));       
       m->CheckMenuItem(ID_TRIGGER_HING,MF_BYCOMMAND|(bHinge ? MF_CHECKED : 0));
       m->Release();
   }
   ::SendMessage(m_hWndTB,TB_ENABLEBUTTON,ID_FILE_START,MAKELPARAM(bLoad && !bStart,0));
   ::SendMessage(m_hWndTB,TB_ENABLEBUTTON,ID_FILE_STOP,MAKELPARAM(bStart,0));
   //::SendMessage(m_hWndTB,TB_ENABLEBUTTON,ID_FILE_PAUSE,MAKELPARAM(bStart,0));
   ::SendMessage(m_hWndTB,TB_CHECKBUTTON,ID_TRIGGER_HING,MAKELONG(bHinge,0));
}
//---------------------------------------------------------------------------
void LUpLcd::Reset()
{
	IMenu *menu;
	int i;
	BOOL bLoad,bStart;

   set_PowerMode();
	LGPU::Reset();
  	menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
	if(menu != NULL){
        for(i = 0;i<5;i++)
            menu->CheckMenuItem(i+ID_LAYER_UP_0,MF_BYCOMMAND|(layers[i].Visible ? MF_CHECKED : MF_UNCHECKED));
        for(i=0;i<2;i++)
            menu->CheckMenuItem(i+ID_LAYER_UP_WINDOW0,MF_BYCOMMAND|(win[i].Visible ? MF_CHECKED : MF_UNCHECKED));
        menu->Release();
	}
   ds.get_RomStatus(&bLoad,&bStart);
   UpdateToolBarState(bLoad,bStart);
   set_Caption("iDeaS Emulator");
 	set_StatusLed(0);
}
//---------------------------------------------------------------------------
void LUpLcd::set_PowerMode()
{
   if(ds.get_EmuMode() == 0){
       reg = io_mem;
       vram_obj = video_mem + 0x60000;
   }
   else{
       reg = io_mem7;
       vram_obj = video_mem + 0x10000;
   }
   reg += 0x1000;
	pal_bg = pal_mem + 0x400;
  	pal_obj = pal_bg + 0x200;
   obj = obj_mem + 0x400;
}
//---------------------------------------------------------------------------
void LUpLcd::OnMenuSelect(WORD wID)
{
   LONG l,lParam;
   RECT rc,rc1,rcDown;
   IMenu *m;

   switch(wID){
       case ID_OPTIONS_LOCKWINDOW:
           l = GetWindowLong(downLcd.Handle(),GWL_STYLE);
           if((bLockWindows = !bLockWindows)){
               l &= ~(WS_BORDER|WS_CAPTION);
               l |= WS_DLGFRAME;
           }
           else{
               l &= ~WS_DLGFRAME;
               l |= WS_BORDER|WS_CAPTION;
           }
           get_Rect(rc);
           ::SetRect(&rc1,0,0,rc.right*fScale,rc.bottom*fScale);
           
           downLcd.SetWindowStyle(l);
           GetWindowRect(&rc);
           downLcd.GetWindowRect(&rcDown);
           downLcd.AdjustWindowRect(&rc1);
           rc1.bottom += abs(rc1.top);
           rc1.right += abs(rc1.left);
           if(nRotate == ID_WINDOW_ROTATE90 || nRotate == ID_WINDOW_ROTATE270){
               rc.left = rc.right;
               rc.bottom -= rc1.bottom;
           }
			else{
				rc.left += (((rc.right - rc.left) - (rc1.right)) / 2);
			}
           ::SetWindowPos(downLcd.Handle(),m_hWnd,rc.left,rc.bottom,rc1.right,rc1.bottom,
               SWP_DRAWFRAME|SWP_FRAMECHANGED|SWP_NOSENDCHANGING|SWP_NOACTIVATE);
           ::CheckMenuItem(GetMenu(),wID,MF_BYCOMMAND|(bLockWindows ? MF_CHECKED : 0));
           ::SendMessage(m_hWndTB,TB_CHECKBUTTON,ID_OPTIONS_LOCKWINDOW,MAKELONG(bLockWindows,0));
       break;
       case ID_LAYER_UP_0:
       case ID_LAYER_UP_1:
       case ID_LAYER_UP_2:
       case ID_LAYER_UP_3:
       case ID_LAYER_UP_OAM:
//           layers[wID-ID_LAYER_UP_0].Visible ^= 1;
//           FillListBackground();

           m = ds.CoCreateInstance_Menu(GetMenu());
           l = !m->IsCheckedMenuItem(wID);
           m->CheckMenuItem(wID,MF_BYCOMMAND|(l ? MF_CHECKED : MF_UNCHECKED));
			for(lParam = l = 0;l<7;l++){
               if(m->IsCheckedMenuItem(l+ID_LAYER_UP_0))
                   lParam |= 1 << l;
           }
           m->Release();
           lParam |= IID_UPLCD << 16;
           pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_SHOWLAYERS,PIS_NOTIFYMASK,(LPARAM)&lParam);
       break;
       case ID_LAYER_UP_WINDOW0:
       case ID_LAYER_UP_WINDOW1:
//           win[wID-ID_LAYER_UP_WINDOW0].Visible ^= 1;
           m = ds.CoCreateInstance_Menu(GetMenu());
           l = !m->IsCheckedMenuItem(wID);
           m->CheckMenuItem(wID,MF_BYCOMMAND|(l ? MF_CHECKED : MF_UNCHECKED));
			for(lParam = l = 0;l<7;l++){
               if(m->IsCheckedMenuItem(l+ID_LAYER_UP_0))
                   lParam |= 1 << l;
           }
           m->Release();
           lParam |= IID_UPLCD << 16;
           pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_SHOWLAYERS,PIS_NOTIFYMASK,(LPARAM)&lParam);
       break;
       case ID_LAYER_UP_ALL:
           m = ds.CoCreateInstance_Menu(GetMenu());
			for(lParam = l = 0;l<7;l++){
               if(m->IsCheckedMenuItem(l+ID_LAYER_UP_0))
                   lParam |= 1 << l;
           }
			lParam = lParam == 0 ? MF_CHECKED : MF_UNCHECKED;
           for(l=0;l<5;l++)
               m->CheckMenuItem(l+ID_LAYER_UP_0,MF_BYCOMMAND|lParam);
           for(l=0;l<2;l++)
               m->CheckMenuItem(l+ID_LAYER_UP_WINDOW0,MF_BYCOMMAND|lParam);
			for(lParam = l = 0;l < 7;l++){
               if(m->IsCheckedMenuItem(l+ID_LAYER_UP_0))
                   lParam |= 1 << l;
           }
           m->Release();
           lParam |= IID_UPLCD << 16;
           pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_SHOWLAYERS,PIS_NOTIFYMASK,(LPARAM)&lParam);
       break;
       default:
           if(!downLcd.OnMenuSelect(wID))
               ds.OnMenuSelect(wID);
       break;
   }
}
//---------------------------------------------------------------------------
BOOL LUpLcd::AdjustWindowRect(LPRECT lpRect)
{
   RECT rcTemp;

   if(!LGPU::AdjustWindowRect(lpRect))
       return FALSE;
   if(GetMenu()) {
        CopyRect(&rcTemp,lpRect);
        rcTemp.bottom = 0x7FFF;
        SendMessage(WM_NCCALCSIZE,FALSE,(LPARAM)&rcTemp);
        lpRect->bottom += rcTemp.top;
   }
   if(m_hWndTB != NULL && ::IsWindow(m_hWndTB)){
       ::GetWindowRect(m_hWndTB,&rcTemp);
       lpRect->bottom += rcTemp.bottom  - rcTemp.top;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
void LUpLcd::OnChangeStatusLed(int nValue)
{
   if(nValue > 1)
   	nStateLed ^= 1;
   else
		nStateLed = nValue;
   if(reg == NULL)
   	return;
   DrawMenuBar(m_hWnd);
   ds.ProcessMessages();
}
//---------------------------------------------------------------------------
static VOID CALLBACK TimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
	upLcd.OnChangeStatusLed(2);
}
//---------------------------------------------------------------------------
void LUpLcd::set_StatusLed(int nValue)
{
	if(uiTimerID != 0)
      	KillTimer(NULL,uiTimerID);
   uiTimerID = 0;
	switch(nValue){
   	case 0:
       case 2:
       	OnChangeStatusLed(1);
       break;
		case 1:
       case 3:
       	uiTimerID = SetTimer(NULL,0,nValue == 3 ? 200 : 500,TimerProc);
       break;
   }
}
//---------------------------------------------------------------------------
LRESULT LUpLcd::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   RECT rc,rcTemp;
   MENUITEMINFO mii={0};
   char c[200];
	int x,y;

#ifdef __WIN32__
	LPNMTBCUSTOMDRAW p;
#endif

	switch(uMsg){
		case WM_NOTIFY:
			switch(wParam){
       case IDM_TOOLBAR:
#ifdef __WIN32__
           if(((LPNMHDR)lParam)->code == NM_CUSTOMDRAW){
               if((p = ((LPNMTBCUSTOMDRAW)lParam))->nmcd.dwDrawStage == CDDS_PREERASE){
                   ::GetClientRect(((LPNMHDR)lParam)->hwndFrom,&rc);
                   FillRect(p->nmcd.hdc,&rc,(HBRUSH)GetSysColorBrush(COLOR_BTNFACE));
                   return CDRF_SKIPDEFAULT;
               }
               return CDRF_DODEFAULT;
           }
           else if(((LPNMHDR)lParam)->code == TBN_GETINFOTIPA){
#else
               if(((LPNMHDR)lParam)->code == TBN_GETINFOTIPA){
#endif
                   mii.cbSize = sizeof(MENUITEMINFO);
                   mii.fMask = MIIM_TYPE;
                   mii.fType = MFT_STRING;
                   mii.dwTypeData = c;
                   mii.cch = 200;
                   GetMenuItemInfo(GetMenu(),((LPNMTBGETINFOTIP)lParam)->iItem,FALSE,&mii);
                   lstrcpy(((LPNMTBGETINFOTIP)lParam)->pszText,c);
               }
           break;
           case IDC_TREE1:
               return cheatsManager.OnWindowProc(WM_NOTIFY,wParam,lParam);
           }
       break;
       case WM_MEASUREITEM:
           if(((MEASUREITEMSTRUCT *)lParam)->itemID == 1){
               ((MEASUREITEMSTRUCT *)lParam)->itemHeight = 17;
               ((MEASUREITEMSTRUCT *)lParam)->itemWidth = 17;
               return TRUE;
           }
       break;
       case WM_DRAWITEM:
           if(((DRAWITEMSTRUCT *)lParam)->itemID == 1){
               CopyRect(&rc,&((DRAWITEMSTRUCT *)lParam)->rcItem);
               rc.left += ((rc.right - rc.left) - 17) >> 1;
               rc.top += ((rc.bottom - rc.top) - 17) >> 1;
               if(imLed != NULL)
                   ImageList_Draw(imLed,nStateLed,((DRAWITEMSTRUCT *)lParam)->hDC,rc.left,rc.top,ILD_TRANSPARENT);
               return TRUE;
           }
       break;
       case WM_CLOSE:
           ds.PostQuitMessage();
       break;
       case WM_COMMAND:
           if(HIWORD(wParam) < 2)
               OnMenuSelect(LOWORD(wParam));
       break;
       case WM_INITMENUPOPUP:
           ds.OnInitMenuPopup((HMENU)wParam,(UINT)LOWORD(lParam),(BOOL)HIWORD(lParam));
       break;
       case WM_ENTERMENULOOP:
       case WM_EXITMENULOOP:
           ds.OnMenuLoop((BOOL)(uMsg == WM_ENTERMENULOOP ? TRUE : FALSE));
       break;
       case WM_MOVE:
       case WM_SIZE:
           if(bLockWindows){
               GetWindowRect(&rc);
               downLcd.GetWindowRect(&rcTemp);
               if(nRotate == ID_WINDOW_ROTATE0 || nRotate == ID_WINDOW_ROTATE180){
                   y = rc.bottom;			
                   x = (rc.right - rc.left);
                   x = (x - rcTemp.right + rcTemp.left);
                   x = rc.left + (x/2);					
               }
               else{
                   x = rc.right;
                   y = rc.bottom - (rcTemp.bottom - rcTemp.top);
               }
               downLcd.MoveTo(x,y,m_hWnd);
           }
       break;
   }
   return LGPU::OnWindowProc(uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
void LUpLcd::Release()
{
}
//---------------------------------------------------------------------------
BOOL LUpLcd::Create()
{
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL LUpLcd::Map()
{
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL LUpLcd::Load(LStream *pFile,int ver)
{
   VideoPlugList *list;
   PlugIn *plugin;
   HMENU hMenu;
   LPARAM lParam;
   int l;
   
   if(!LGPU::Load(pFile,ver))
       return FALSE;
   hMenu = upLcd.GetMenu();
   if(hMenu == NULL)
       return FALSE;
   list = (VideoPlugList *)pPlugInContainer->get_PlugInList(PIL_VIDEO);
   if(list == NULL)
   	return FALSE;
   plugin = list->get_Active2DPlugin();
   if(plugin == NULL)
   	return FALSE;
   lParam = (IID_UPLCD << 16) | 0xFFFF;
   plugin->NotifyState(PNMV_SHOWLAYERS,PIS_NOTIFYMASK,(LPARAM)&lParam);
   for(l=0;l<5;l++)
      	CheckMenuItem(hMenu,l+ID_LAYER_UP_0,MF_BYCOMMAND|(lParam & (1 << l)) ? MF_CHECKED : MF_UNCHECKED);
   for(l=0;l<2;l++)
      	CheckMenuItem(hMenu,l+ID_LAYER_UP_WINDOW0,MF_BYCOMMAND|(lParam & (1 << (5 + l))) ? MF_CHECKED : MF_UNCHECKED);
   return TRUE;
}

