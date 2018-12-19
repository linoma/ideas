#include "debug.h"
#include "resource.h"
#include "dsmem.h"
#include "util.h"
#include "fstream.h"
#include "inputtext.h"
#include "lds.h"
#include "io.h"
#include "elf.h"
#include "inspector.h"
#include "mtxviewer.h"
#include "polyviewer.h"
#include "language.h"

#ifdef _DEBPRO
LInspectorList inspector;
LPolygonViewer polyViewer;
#endif
//---------------------------------------------------------------------------
#ifdef _DEBUG
//---------------------------------------------------------------------------
LDebugDlg::LDebugDlg() : LWnd()
{
	pFileDis = pFileIO = NULL;
	m_hWnd = NULL;
	bDebug = FALSE;
   dwCheckAddress = 0;
   imageListDebug[0] = imageListDebug[1] = NULL;
   dwBreakIRQ[0] = dwBreakIRQ[1] = 0;
#if defined(_DEBPRO2)
   RecentFiles.set_MaxElem(10);
   RecentFiles.Load("Software\\iDeaS\\Settings\\Debugger\\RecentFile");
#endif
   cLinoSearch[0] = 0;
}
//---------------------------------------------------------------------------
LDebugDlg::~LDebugDlg()
{
	Destroy();
}
//---------------------------------------------------------------------------
BOOL LDebugDlg::Destroy()
{
#ifdef _LWYLIN
	if(pFileOut7 != NULL){
   	delete pFileOut7;
       pFileOut7 = NULL;
   }
   if(pFileOut9 == NULL){
		delete pFileOut9;
       pFileOut9 = NULL;
   }
#endif
	palViewer.Destroy();
   mtxViewer.Destroy();
#ifdef _DEBPRO4
   spriteViewer.Destroy();
#endif
#ifdef _DEBPRO5
	tileViewer.Destroy();
#endif
#ifdef _DEBPRO
   textureViewer.Destroy();
   ioViewer.Destroy();
#endif
	if(pFileDis != NULL)
   	delete pFileDis;
   pFileDis = NULL;
	if(pFileIO != NULL)
   	delete pFileIO;
   pFileIO = NULL;
#if defined(_DEBPRO2)
	prgBP.Save(NULL,TRUE);
   RecentFiles.Save("Software\\iDeaS\\Settings\\Debugger\\RecentFile");
#endif
	for(int i = 0;i<2;i++){
		if(imageListDebug[i] != 0)
   		ImageList_Destroy(imageListDebug[i]);
       imageListDebug[i] = NULL;
   }
	return LWnd::Destroy();
}
//---------------------------------------------------------------------------
BOOL LDebugDlg::Modeless(LPVOID p)
{
	parent = p;
	if(m_hWnd != NULL && ::IsWindow(m_hWnd))
       return ::BringWindowToTop(m_hWnd);
	m_hWnd = ::CreateDialogParam(langManager.get_CurrentLib(),MAKEINTRESOURCE(IDD_DIALOG1),((LDS *)p)->Handle(),DialogProc,(LPARAM)this);
   return (m_hWnd != NULL ? TRUE : FALSE);
}
//---------------------------------------------------------------------------
void LDebugDlg::EnterDebugMode(BOOL bArm7)
{
   if(!Modeless(&ds))
       return;
	bDebug = TRUE;
   bPass = FALSE;
	UpdateToolBar();
   WaitDebugger(bArm7);
}
//---------------------------------------------------------------------------
void LDebugDlg::UpdateToolBar()
{
   HMENU menu;
	BOOL bLoad,bStart,b;

	if(m_hWnd == NULL || !::IsWindow(m_hWnd) || parent == NULL || (menu = GetMenu()) == NULL)
   	return;
	((LDS *)parent)->get_RomStatus(&bLoad,&bStart);
	if(!bLoad)
   	bStart = FALSE;
	::EnableMenuItem(menu,ID_DEBUG_RESET,MF_BYCOMMAND|(bLoad ? MF_ENABLED : MF_GRAYED));
   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_ENABLEBUTTON,ID_DEBUG_RESET,(LPARAM)MAKELONG(bLoad,0));
   b = (bLoad && (!bStart || !bPass));
	::EnableMenuItem(menu,ID_DEBUG_RUN,MF_BYCOMMAND|(b ? MF_ENABLED : MF_GRAYED));
   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_ENABLEBUTTON,ID_DEBUG_RUN,(LPARAM)MAKELONG(b,0));
   if(bLoad && bStart){
       if(bPass){
           SendDlgItemMessage(m_hWnd,IDC_DEBUG_SB1,SB_SETTEXT,0,(LPARAM)"Running");
			pPlugInContainer->NotifyState(-1,PIS_RUN,PIS_RUNMASK);
       }
       else{
           SendDlgItemMessage(m_hWnd,IDC_DEBUG_SB1,SB_SETTEXT,0,(LPARAM)"Stopped");
           pPlugInContainer->NotifyState(-1,PIS_PAUSE,PIS_RUNMASK);
       }
   }
   else
       SendDlgItemMessage(m_hWnd,IDC_DEBUG_SB1,SB_SETTEXT,0,(LPARAM)"");
#ifdef _DEBPRO2
	::EnableMenuItem(menu,ID_DEBUG_BREAKPOINT,MF_BYCOMMAND|(b ? MF_ENABLED : MF_GRAYED));
   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_ENABLEBUTTON,ID_DEBUG_BREAKPOINT,(LPARAM)MAKELONG(b,0));
#endif
   b = (bLoad && bStart && !bPass);
#ifdef _DEBPRO2
	::EnableMenuItem(menu,ID_DEBUG_STEPOVER,MF_BYCOMMAND|(b ? MF_ENABLED : MF_GRAYED));
   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_ENABLEBUTTON,ID_DEBUG_STEPOVER,(LPARAM)MAKELONG(b,0));
#endif
#ifdef _DEBPRO
	::EnableMenuItem(menu,ID_DEBUG_VIEW_REFRESHLCD,MF_BYCOMMAND|(b ? MF_ENABLED : MF_GRAYED));
   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_ENABLEBUTTON,ID_DEBUG_VIEW_REFRESHLCD,(LPARAM)MAKELONG(b,0));
#endif
	::EnableMenuItem(menu,ID_DEBUG_TRACEINTO,MF_BYCOMMAND|(b ? MF_ENABLED : MF_GRAYED));
   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_ENABLEBUTTON,ID_DEBUG_TRACEINTO,(LPARAM)MAKELONG(b,0));
	::EnableMenuItem(menu,ID_DEBUG_RUNCURSOR,MF_BYCOMMAND|(b ? MF_ENABLED : MF_GRAYED));
   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_ENABLEBUTTON,ID_DEBUG_RUNCURSOR,(LPARAM)MAKELONG(b,0));
#ifdef _DEBPRO
	::EnableMenuItem(menu,ID_DEBUG_INSPECTOR,MF_BYCOMMAND|(b ? MF_ENABLED : MF_GRAYED));
   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_ENABLEBUTTON,ID_DEBUG_INSPECTOR,(LPARAM)MAKELONG(b,0));
#endif
#ifdef _DEBPRO2
   b = prgBP.is_Modified();
	::EnableMenuItem(menu,ID_DEBUG_BREAKPOINT_SAVE,MF_BYCOMMAND|(b ? MF_ENABLED : MF_GRAYED));
   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_ENABLEBUTTON,ID_DEBUG_BREAKPOINT_SAVE,(LPARAM)MAKELONG(b,0));
   b = prgBP.Count() > 0;
   ::EnableMenuItem(menu,ID_DEBUG2_DELALLBRKPOINT,MF_BYCOMMAND|(b ? MF_ENABLED : MF_GRAYED));
#endif
   b = (bLoad && bStart && bPass);
	::EnableMenuItem(menu,ID_DEBUG_PAUSE,MF_BYCOMMAND|(b ? MF_ENABLED : MF_GRAYED));
   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_ENABLEBUTTON,ID_DEBUG_PAUSE,(LPARAM)MAKELONG(b,0));
#ifndef __WIN32__
	::EnableMenuItem(menu,ID_DEBUG_TOOLSWINDOW_TOFLOAT,MF_BYCOMMAND|MF_GRAYED);
   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_ENABLEBUTTON,ID_DEBUG_TOOLSWINDOW_TOFLOAT,FALSE);
#endif
}
//---------------------------------------------------------------------------
void LDebugDlg::OnInitDialog(HWND hwnd)
{
   int i,item,st_parts[]={150,300};
   HBITMAP bit;
   TBBUTTON tbb[16];

   accel = LoadAccelerators(hInst,MAKEINTRESOURCE(112));
   if(accel == NULL)
       return;
   Attack(hwnd,FALSE);

   SendDlgItemMessage(hwnd,IDC_DEBUG_SB1,SB_SETPARTS,2,(LPARAM)st_parts);

   SendDlgItemMessage(hwnd,IDC_DEBUG_CPU,CB_ADDSTRING,0,(LPARAM)"Arm 9");
   SendDlgItemMessage(hwnd,IDC_DEBUG_CPU,CB_ADDSTRING,0,(LPARAM)"Arm 7");

   SendDlgItemMessage(hwnd,IDC_DEBUG_CPUMODE,CB_ADDSTRING,0,(LPARAM)"Arm");
   SendDlgItemMessage(hwnd,IDC_DEBUG_CPUMODE,CB_ADDSTRING,0,(LPARAM)"Thumb");
   SendDlgItemMessage(hwnd,IDC_DEBUG_CPUMODE,CB_ADDSTRING,0,(LPARAM)"Auto");

   SendDlgItemMessage(hwnd,IDC_DEBUG_CPU,CB_SETCURSEL,(nCpu = 0),0);
   SendDlgItemMessage(hwnd,IDC_DEBUG_CPUMODE,CB_SETCURSEL,(nMode = 2),0);

   for(i=0;i<get_CurrentCPU()->r_memmapsize();i++){
       if(get_CurrentCPU()->r_memmap()[i].bFlags & 1){
           item = SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEM,CB_ADDSTRING,0,(LPARAM)get_CurrentCPU()->r_memmap()[i].Name);
           SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEM,CB_SETITEMDATA,item,i);
       }
   }

   SendDlgItemMessage(hwnd,IDC_DEBUG_ARMMODE,CB_ADDSTRING,0,(LPARAM)"User");
   SendDlgItemMessage(hwnd,IDC_DEBUG_ARMMODE,CB_ADDSTRING,0,(LPARAM)"FIQ");
   SendDlgItemMessage(hwnd,IDC_DEBUG_ARMMODE,CB_ADDSTRING,0,(LPARAM)"IRQ");
   SendDlgItemMessage(hwnd,IDC_DEBUG_ARMMODE,CB_ADDSTRING,0,(LPARAM)"Supervisor");
   SendDlgItemMessage(hwnd,IDC_DEBUG_ARMMODE,CB_ADDSTRING,0,(LPARAM)"Abort");
   SendDlgItemMessage(hwnd,IDC_DEBUG_ARMMODE,CB_ADDSTRING,0,(LPARAM)"Undefined");
   SendDlgItemMessage(hwnd,IDC_DEBUG_ARMMODE,CB_ADDSTRING,0,(LPARAM)"System");

   SendDlgItemMessage(hwnd,IDC_DEBUG_ARMMODE,CB_SETITEMDATA,0,(LPARAM)USER_MODE);
   SendDlgItemMessage(hwnd,IDC_DEBUG_ARMMODE,CB_SETITEMDATA,1,(LPARAM)FIQ_MODE);
   SendDlgItemMessage(hwnd,IDC_DEBUG_ARMMODE,CB_SETITEMDATA,2,(LPARAM)IRQ_MODE);
   SendDlgItemMessage(hwnd,IDC_DEBUG_ARMMODE,CB_SETITEMDATA,3,(LPARAM)SUPERVISOR_MODE);
   SendDlgItemMessage(hwnd,IDC_DEBUG_ARMMODE,CB_SETITEMDATA,4,(LPARAM)ABORT_MODE);
   SendDlgItemMessage(hwnd,IDC_DEBUG_ARMMODE,CB_SETITEMDATA,5,(LPARAM)UNDEFINED_MODE);
   SendDlgItemMessage(hwnd,IDC_DEBUG_ARMMODE,CB_SETITEMDATA,6,(LPARAM)SYSTEM_MODE);

   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_BUTTONSTRUCTSIZE,(WPARAM)sizeof(TBBUTTON),0);
   imageListDebug[0] = ImageList_Create(16,16,ILC_COLOR16|ILC_MASK,12,12);
   if(imageListDebug[0] != NULL){
       bit = LoadBitmap(hInst,MAKEINTRESOURCE(IDB_TOOLBAR_DEBUG));
       if(bit != NULL){
           ImageList_AddMasked(imageListDebug[0],bit,RGB(255,0,255));
           ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_SETIMAGELIST,0,(LPARAM)imageListDebug[0]);
           ::DeleteObject(bit);
       }
   }
   imageListDebug[1] = ImageList_LoadImage(hInst,MAKEINTRESOURCE(IDB_TOOLBAR_DEBUG_DISABLED),16,11,RGB(255,0,255),IMAGE_BITMAP,LR_DEFAULTCOLOR);
   if(imageListDebug[1] != NULL)
       ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_SETDISABLEDIMAGELIST,0,(LPARAM)imageListDebug[1]);
   i = 0;
#ifdef _DEBPRO2
   tbb[i].iBitmap = 0;
   tbb[i].idCommand = ID_DEBUG_BREAKPOINT_OPEN;
   tbb[i].fsState = TBSTATE_ENABLED;
   tbb[i].fsStyle = TBSTYLE_BUTTON;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;
   tbb[i].iBitmap = 1;
   tbb[i].idCommand = ID_DEBUG_BREAKPOINT_SAVE;
   tbb[i].fsState = 0;
   tbb[i].fsStyle = TBSTYLE_BUTTON;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;
   tbb[i].iBitmap = -1;
   tbb[i].idCommand = 0;
   tbb[i].fsState = 0;
   tbb[i].fsStyle = TBSTYLE_SEP;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;
#endif
   tbb[i].iBitmap = 2;
   tbb[i].idCommand = ID_DEBUG_ADDMEMORYPAGE;
   tbb[i].fsState = TBSTATE_ENABLED;
   tbb[i].fsStyle = TBSTYLE_BUTTON;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;

   tbb[i].iBitmap = 3;
   tbb[i].idCommand = ID_DEBUG_TOOLSWINDOW_TOFLOAT;
   tbb[i].fsState = TBSTATE_ENABLED;
   tbb[i].fsStyle = TBSTYLE_BUTTON;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;

   tbb[i].iBitmap = -1;
   tbb[i].idCommand = 0;
   tbb[i].fsState = 0;
   tbb[i].fsStyle = TBSTYLE_SEP;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;

   tbb[i].iBitmap = 4;
   tbb[i].idCommand = ID_DEBUG_RUN;
   tbb[i].fsState = TBSTATE_ENABLED;
   tbb[i].fsStyle = TBSTYLE_BUTTON;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;

   tbb[i].iBitmap = 5;
   tbb[i].idCommand = ID_DEBUG_PAUSE;
   tbb[i].fsState = TBSTATE_ENABLED;
   tbb[i].fsStyle = TBSTYLE_BUTTON;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;
#ifdef _DEBPRO2
   tbb[i].iBitmap = 6;
   tbb[i].idCommand = ID_DEBUG_STEPOVER;
   tbb[i].fsState = TBSTATE_ENABLED;
   tbb[i].fsStyle = TBSTYLE_BUTTON;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;
#endif
   tbb[i].iBitmap = 7;
   tbb[i].idCommand = ID_DEBUG_RUNCURSOR;
   tbb[i].fsState = TBSTATE_ENABLED;
   tbb[i].fsStyle = TBSTYLE_BUTTON;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;
#ifdef _DEBPRO2
   tbb[i].iBitmap = 8;
   tbb[i].idCommand = ID_DEBUG_BREAKPOINT;
   tbb[i].fsState = 0;
   tbb[i].fsStyle = TBSTYLE_BUTTON;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;
#endif
   tbb[i].iBitmap = 9;
   tbb[i].idCommand = ID_DEBUG_RESET;
   tbb[i].fsState = TBSTATE_ENABLED;
   tbb[i].fsStyle = TBSTYLE_BUTTON;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;
#ifdef _DEBPRO
   tbb[i].iBitmap = -1;
   tbb[i].idCommand = 0;
   tbb[i].fsState = 0;
   tbb[i].fsStyle = TBSTYLE_SEP;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;

   tbb[i].iBitmap = 11;
   tbb[i].idCommand = ID_DEBUG_VIEW_REFRESHLCD;
   tbb[i].fsState = TBSTATE_ENABLED;
   tbb[i].fsStyle = TBSTYLE_BUTTON;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;
#endif
   tbb[i].iBitmap = -1;
   tbb[i].idCommand = 0;
   tbb[i].fsState = 0;
   tbb[i].fsStyle = TBSTYLE_SEP;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;

   tbb[i].iBitmap = 10;
   tbb[i].idCommand = ID_DEBUG2_EXIT;
   tbb[i].fsState = TBSTATE_ENABLED;
   tbb[i].fsStyle = TBSTYLE_BUTTON;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;

   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_ADDBUTTONS,(WPARAM)i,(LPARAM)&tbb);

   floatDlg.Create();
#ifdef __WIN32__
   SetClassLong(m_hWnd,GCL_HICON,(LONG)LoadIcon(hInst,MAKEINTRESOURCE(2)));
   floatDlg.toFloat(FALSE);
#else
   RECT rc;
   GList *pList;
   HWND hwndCtl,fixed;
   int x,y;

   fixed = (HWND)GetWindowLong(m_hWnd,GWL_FIXED);
   pList = gtk_container_get_children((GtkContainer *)fixed);
   if(pList != NULL){
       ::GetWindowRect(GetDlgItem(m_hWnd,IDC_DEBUG_TOOLBAR),&rc);
       rc.bottom -= rc.top;
       i = g_list_length(pList);
       for(item=0;item<i;item++){
           hwndCtl = (HWND)g_list_nth_data(pList,item);
           gtk_widget_translate_coordinates(hwndCtl,(HWND)fixed,0,0,&x,&y);
           y -= rc.bottom;
           //gtk_fixed_move((GtkFixed *)fixed,hwndCtl,x,y);
       }
   }
   floatDlg.toFloat(TRUE);
   GetWindowRect(&rc);
   SetWindowPos(NULL,0,0,rc.right - rc.left,rc.bottom - rc.top - 150,SWP_NOMOVE|SWP_NOREPOSITION|SWP_SHOWWINDOW);
   ShowWindow(floatDlg.get_ConsolleView()->Handle(),SW_HIDE);
#endif
   Init_Debugger();
   Reset_Debugger();
   GetClientRect(&rcClient);
   GetWindowRect(&rcWin);
}
//---------------------------------------------------------------------------
void LDebugDlg::OnEnterIRQ(u32 value,int index)
{
#ifdef _DEBPRO
   DWORD dw,dwBit;

	if(get_CurrentCPU()->r_index() != index)
   	return;
   for(dwBit = 0;dwBit < 32;dwBit++){
   	if((1 << dwBit) & value)
       	break;
   }
   if(dwBit == 32)
   	return;
   dw = (index == 7 ? dwBreakIRQ[1] : dwBreakIRQ[0]);
   if(!((1 << dwBit) & dw))
   	return;
   if(!Modeless(&ds))
       return;
	bDebug = TRUE;
   bPass = FALSE;
#endif
}
//---------------------------------------------------------------------------
void LDebugDlg::Init_Debugger()
{
#if defined(_DEBPRO2)
	prgBP.Attack(GetDlgItem(floatDlg.Handle(),IDC_DEBUG_LV1));
#endif
   disLB.Attack(GetDlgItem(m_hWnd,IDC_DEBUG_DIS));
   regLB.Attack(GetDlgItem(m_hWnd,IDC_DEBUG_REG));
}
//---------------------------------------------------------------------------
void LDebugDlg::Reset_Debugger()
{
	RECT rc;
   HWND hwnd;
   DWORD dw;
 	int index;

#ifdef _LWYLIN
	if(pFileOut7 != NULL){
   	delete pFileOut7;
       pFileOut7 = NULL;
   }
	if(pFileOut9 != NULL){
   	delete pFileOut9;
       pFileOut9 = NULL;
   }
#endif
   if(m_hWnd == NULL || !::IsWindow(m_hWnd))
   	return;
   ZeroMemory(&dbg.views[DBV_RUN],sizeof(DISVIEW));
   ZeroMemory(&dbg.views[DBV_VIEW],sizeof(DISVIEW));
   dbg.yScroll = 0;
   dbg.bTrackMode = 2;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_DIS);
   ::GetWindowRect(hwnd,&rc);
   dbg.iMaxItem = (u8)((rc.bottom - rc.top) / ::SendMessage(hwnd,LB_GETITEMHEIGHT,0,0));
	index = (signed char)GetCurrentAddressPipe(&dw,TRUE,TRUE);
	UpdateAddressBar(dw,TRUE);
// 	UpdateVertScrollBarDisDump(dw,index);
	bDebug = TRUE;
   bPass = FALSE;
   dwCheckAddress = 0;
	Update_Debugger();
#if defined(_DEBPRO)
   inspector.Reset();
#endif
#ifdef _DEBPRO2
   prgBP.Reset();
#endif
   floatDlg.Reset();
   UpdateToolBar();
}
//---------------------------------------------------------------------------
void LDebugDlg::Update_Debugger()
{
	FillListDiss(NULL,DBV_RUN,0);
   Update_Reg();
}
//---------------------------------------------------------------------------
void LDebugDlg::Update_Reg()
{
	char s1[30];
   IARM7 *cpu;
   int i;

   cpu = get_CurrentCPU();
	wsprintf(s1,"%02X",((u16 *)(io_mem + 6))[0]);
	SendDlgItemMessage(m_hWnd,IDC_LINE,WM_SETTEXT,0,(LPARAM)s1);
#ifdef _DEBPRO
	wsprintf(s1,"%d",ds.get_CurrentCycles());
	SendDlgItemMessage(m_hWnd,IDC_CYCLES,WM_SETTEXT,0,(LPARAM)s1);
#endif
   SendDlgItemMessage(m_hWnd,IDC_DEBUG_C,BM_SETCHECK,cpu->r_cf() ? BST_CHECKED : BST_UNCHECKED,0);
   SendDlgItemMessage(m_hWnd,IDC_DEBUG_N,BM_SETCHECK,cpu->r_nf() ? BST_CHECKED : BST_UNCHECKED,0);
   SendDlgItemMessage(m_hWnd,IDC_DEBUG_V,BM_SETCHECK,cpu->r_vf() ? BST_CHECKED : BST_UNCHECKED,0);
   SendDlgItemMessage(m_hWnd,IDC_DEBUG_Z,BM_SETCHECK,cpu->r_zf() ? BST_CHECKED : BST_UNCHECKED,0);
	SendDlgItemMessage(m_hWnd,IDC_DEBUG_I,BM_SETCHECK,cpu->r_cpsr() & IRQ_BIT ? BST_CHECKED : BST_UNCHECKED,0);
	SendDlgItemMessage(m_hWnd,IDC_DEBUG_T,BM_SETCHECK,cpu->r_cpsr() & T_BIT ? BST_CHECKED : BST_UNCHECKED,0);
	SendDlgItemMessage(m_hWnd,IDC_DEBUG_S,BM_SETCHECK,cpu->r_stop() ? BST_CHECKED : BST_UNCHECKED,0);
	SendDlgItemMessage(m_hWnd,IDC_DEBUG_Q,BM_SETCHECK,nCpu == 0 && arm9.r_qf() ? BST_CHECKED : BST_UNCHECKED,0);
	SendDlgItemMessage(m_hWnd,IDC_DEBUG_REG,LB_RESETCONTENT,0,0);
   for(i=0;i<17;i++){
		sprintf(s1,"R%02d  0x%08X",i,cpu->r_gpreg(i));
       SendDlgItemMessage(m_hWnd,IDC_DEBUG_REG,LB_ADDSTRING,0,(LPARAM)s1);
   }
	i = SendDlgItemMessage(m_hWnd,IDC_DEBUG_ARMMODE,CB_GETCOUNT,0,0) - 1;
   for(;i>=0;i--){
   	if((cpu->r_cpsr() & 0x1F) == SendDlgItemMessage(m_hWnd,IDC_DEBUG_ARMMODE,CB_GETITEMDATA,i,0)){
       	SendDlgItemMessage(m_hWnd,IDC_DEBUG_ARMMODE,CB_SETCURSEL,i,0);
           break;
		}
   }
}
//---------------------------------------------------------------------------
void LDebugDlg::OnMenuSelect(WORD wID)
{
	BOOL b;
	RECT rc,rc1;
	char s[MAX_PATH];
   DWORD dw;
   IARM7 *arm;
#ifdef _DEBPRO
	int i;
	CompileUnit *unit;
   Function *func;
   LInspectDlg *dlg;
   MENUITEMINFO mii;
#endif

   if(floatDlg.OnMenuSelect(wID))
   	return;
#if defined(_DEBPRO2)
	if(wID > ID_DEBUG_RECENT_RESET && wID <= ID_DEBUG_RECENT_RESET + RecentFiles.get_MaxElem()){
       if(prgBP.Load(((char *)((LString *)RecentFiles.GetItem(wID - ID_DEBUG_RECENT_RESET))->c_str()))){
           if(floatDlg.get_ActivePageType() == PG_BREAKPOINT)
               prgBP.UpdateList(GetDlgItem(floatDlg.Handle(),IDC_DEBUG_LV1),BT_PROGRAM);
           else if(floatDlg.get_ActivePageType() == PG_MEMBREAKPOINT)
               prgBP.UpdateList(GetDlgItem(floatDlg.Handle(),IDC_DEBUG_LV1),BT_MEMORY);
       }
       else
       	RecentFiles.Delete(wID - ID_DEBUG_RECENT_RESET);
   	return;
   }
#if defined(_DEBPRO)
   else if(wID >= ID_DEBUG_ENTERIRQ_START && wID < ID_DEBUG_ENTERIRQ_END){
   	if(wID - ID_DEBUG_ENTERIRQ_START > 31){
			dw = 1 << (wID - ID_DEBUG_ENTERIRQ_START - 32);
           dwBreakIRQ[1] ^= dw;
			dw &= dwBreakIRQ[1];
       }
       else{
			dw = 1 << (wID - ID_DEBUG_ENTERIRQ_START);
           dwBreakIRQ[0] ^= dw;
			dw &= dwBreakIRQ[0];
       }
		CheckMenuItem(GetMenu(),wID,MF_BYCOMMAND|(dw != 0 ? MF_CHECKED : MF_UNCHECKED));
       return;
   }
#endif

#endif
	switch(wID){
#ifdef _DEBPRO5
       case ID_DEBUG_VIEW_TILES:
           bPass = FALSE;
           UpdateToolBar();
           tileViewer.Show(m_hWnd);
       break;
#endif

#ifdef _DEBPRO4
       case ID_DEBUG_VIEW_OAM:
           bPass = FALSE;
           UpdateToolBar();
           spriteViewer.Show(m_hWnd);
       break;
#endif

#if defined(_DEBPRO)
		case ID_DEBUG_VIEW_REFRESHLCD:
       	ds.RefreshLCDs();
       break;
       case ID_DEBUG_START_LOG3DFIFO:
           mii.cbSize = sizeof(MENUITEMINFO);
           mii.fMask = MIIM_STATE;
           mii.wID = ID_DEBUG_START_LOG3DFIFO;
           GetMenuItemInfo(GetMenu(),ID_DEBUG_START_LOG3DFIFO,FALSE,&mii);
           b = (BOOL)(mii.fState & MFS_CHECKED ? TRUE : FALSE);
           pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_ENABLELOG,PIS_NOTIFYMASK,b ? 3 : 4);
           mii.fState = b ? MFS_UNCHECKED : MFS_CHECKED;
           SetMenuItemInfo(GetMenu(),ID_DEBUG_START_LOG3DFIFO,FALSE,&mii);
       break;
       case ID_DEBUG_DELETE_LOG3DFIFO:
           pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_ENABLELOG,PIS_NOTIFYMASK,5);
       break;
       case ID_DEBUG_START_LOG3DENGINE:
           mii.cbSize = sizeof(MENUITEMINFO);
           mii.fMask = MIIM_STATE;
           mii.wID = ID_DEBUG_START_LOG3DENGINE;
           GetMenuItemInfo(GetMenu(),ID_DEBUG_START_LOG3DENGINE,FALSE,&mii);
           b = (BOOL)(mii.fState & MFS_CHECKED ? TRUE : FALSE);
           pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_ENABLELOG,PIS_NOTIFYMASK,b ? 0 : 1);
           mii.fState = b ? MFS_UNCHECKED : MFS_CHECKED;
           SetMenuItemInfo(GetMenu(),ID_DEBUG_START_LOG3DENGINE,FALSE,&mii);
       break;
       case ID_DEBUG_DELETE_LOG3DENGINE:
           pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_ENABLELOG,PIS_NOTIFYMASK,2);
       break;
		case ID_DEBUG2_WRITEONFILE:
       	if(pFileDis == NULL){
           	if((pFileDis = new LFile("dis.s")) != NULL){
               	pFileDis->Open(GENERIC_WRITE,CREATE_ALWAYS);
                   for(i=0;i<0x8000;i+=4){
                   	arm7.decode_ins(0,0|i,s);
                       pFileDis->WriteF("%s\n",&s[18]);
                   }
               }
           }
           else{
				delete pFileDis;
               pFileDis = NULL;
           }
       break;
       case ID_DEBUG_DIS_FIND:
           SendDlgItemMessage(m_hWnd,IDC_DEBUG_EDITGO,WM_GETTEXT,(WPARAM)MAX_PATH,(LPARAM)s);
           if(ds.get_RomReader() != NULL){
               if(ds.get_RomReader()->GetFunctionFromName(s,(void **)&func,(void **)&unit,get_CurrentCPU()->r_index()) > 0){
                   dw = func->lowPC;
                   UpdateAddressBar(dw,TRUE);
                   FillListDiss(&dw,DBV_RUN,0);
               }
           }
       break;
       case ID_DEBUG_VIEW_POLYGON:
           bPass = FALSE;
           UpdateToolBar();
			polyViewer.Show(m_hWnd);
       break;
       case ID_DEBUG_INSPECTOR:
           bPass = FALSE;
           UpdateToolBar();
           GetCurrentAddressPipe(&dw,0,0);
           dlg = new LInspectDlg();
           if(dlg->CreateModal(hInst,MAKEINTRESOURCE(IDD_DIALOG14),m_hWnd))
               inspector.Add(dlg->get_Text().c_str(),dw,get_CurrentCPU());
           delete dlg;
       break;
       case ID_DEBUG_VIEW_IO:
           bPass = FALSE;
           UpdateToolBar();
           ioViewer.Show(m_hWnd);
       break;
       case ID_DEBUG_VIEW_TEXTURE:
           bPass = FALSE;
           UpdateToolBar();
           textureViewer.Show(m_hWnd);
       break;
       case ID_DEBUG_CLEARTEXTURESBUFFER:
       	if(ds.get_ActiveVideoPlugIn() != NULL)
           	ds.get_ActiveVideoPlugIn()->NotifyState(PNMV_CLEARTEXTUREBUFFER,PIS_NOTIFYMASK,0);
       break;
       case ID_DEBUG_CAPTURETEXTURES:
       	if(ds.get_ActiveVideoPlugIn() != NULL){
               i = -1;
               ds.get_ActiveVideoPlugIn()->NotifyState(PNMV_CAPTURETEXTURES,PIS_NOTIFYMASK,(LPARAM)&i);
               i = (i & 1) ^ 1;
               ds.get_ActiveVideoPlugIn()->NotifyState(PNMV_CAPTURETEXTURES,PIS_NOTIFYMASK,(LPARAM)&i);
				CheckMenuItem(GetMenu(),ID_DEBUG_CAPTURETEXTURES,MF_BYCOMMAND|(i != 0 ? MF_CHECKED : MF_UNCHECKED));
           }
       break;
#endif
#ifdef _DEBPRO2
       case ID_DEBUG_RECENT_RESET:
           RecentFiles.Clear();
       break;
       case ID_DEBUG_BREAKPOINT_OPEN:
           *((int *)s) = 0;
       	if(ShowOpenDialog(NULL,"BreakPoint files(*.lst)\0*.lst;\0\0\0\0\0\0",m_hWnd,s,MAX_PATH)){
           	if(prgBP.Load(s)){
					if(RecentFiles.Find(s) == NULL)
   					RecentFiles.Add(s);
					if(floatDlg.get_ActivePageType() == PG_BREAKPOINT)
   					prgBP.UpdateList(GetDlgItem(floatDlg.Handle(),IDC_DEBUG_LV1),BT_PROGRAM);
                   else if(floatDlg.get_ActivePageType() == PG_MEMBREAKPOINT)
						prgBP.UpdateList(GetDlgItem(floatDlg.Handle(),IDC_DEBUG_LV1),BT_MEMORY);
               }
			}
       break;
       case ID_DEBUG_BREAKPOINT_SAVE:
       	prgBP.Save(NULL,TRUE);
           UpdateToolBar();
       break;
		case ID_DEBUG2_DELALLBRKPOINT:
           prgBP.Clear();
           if(floatDlg.get_ActivePageType() == PG_BREAKPOINT)
   			prgBP.UpdateList(GetDlgItem(floatDlg.Handle(),IDC_DEBUG_LV1),BT_PROGRAM);
           else if(floatDlg.get_ActivePageType() == PG_MEMBREAKPOINT)
				prgBP.UpdateList(GetDlgItem(floatDlg.Handle(),IDC_DEBUG_LV1),BT_MEMORY);
       break;
       case ID_DEBUG2_PAUSE:
       	bPass = FALSE;
       break;
       case ID_DEBUG_BREAKPOINT:
         	if(BreakpointFromListBox(NULL,&dw)){
          		InsertBreakPoint(dw);
               UpdateToolBar();
           }
       break;
       case ID_DEBUG_STEPOVER:
           arm = get_CurrentCPU();
           dwBreak = arm->gp_regs[15];
           if(!(arm->status & T_BIT))
               dwBreak -= 4;
           nBreakMode = 1;
           bPass = TRUE;
           UpdateToolBar();
       break;
#endif
       case ID_DEBUG_VIEW_PALETTE:
           bPass = FALSE;
           UpdateToolBar();
           palViewer.Show(m_hWnd);
       break;
       case ID_DEBUG_VIEW_MATRIX:
           bPass = FALSE;
           UpdateToolBar();
           mtxViewer.Show(m_hWnd);
       break;
   	case ID_DEBUG_TOOLSWINDOW_TOFLOAT:
#ifdef __WIN32__
           b = !floatDlg.is_Float();
           ::GetWindowRect(m_hWnd,&rc);
           ::GetClientRect(floatDlg.Handle(),&rc1);
           rc.bottom -= rc.top + (160 * (b ? 1 : -1));
           floatDlg.toFloat(b);
			Resize(rc.left,rc.top,rc.right-rc.left,rc.bottom);
           ::RedrawWindow(m_hWnd,NULL,NULL,RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ERASENOW|RDW_ALLCHILDREN);
           ::CheckMenuItem(::GetMenu(m_hWnd),wID,MF_BYCOMMAND|(b ? MF_CHECKED : MF_UNCHECKED));
#endif
       break;
       case ID_DEBUG_TRACEINTO:
           bFlag = FALSE;
           bPass = FALSE;
           UpdateToolBar();
       break;
       case ID_DEBUG_RUNCURSOR:
           if(BreakpointFromListBox(NULL,&dwBreak)){
           	nBreakMode = 1;
           	bPass = TRUE;
               UpdateToolBar();
           }
       break;
       case ID_DEBUG2_EXIT:
           OnClose();
       break;
       case ID_DEBUG_RESET:
           ::SendMessage(((LDS *)parent)->Handle(),WM_COMMAND,MAKEWPARAM(ID_FILE_RESET,0),0);
       break;
       case ID_DEBUG_RUN:
          	bPass = TRUE;
           nBreakMode = 0;
           UpdateToolBar();
       break;
       case ID_DEBUG_PAUSE:
           bPass = FALSE;
           UpdateToolBar();
       break;
   }
}
//---------------------------------------------------------------------------
void LDebugDlg::OnCommand(WORD wNotifyCode,WORD wID,HWND hwnd)
{
	char s[MAX_PATH+1],s1[MAX_PATH+1],*p;
   DWORD dw;
	int i;
   IARM7 *cpu;

	switch(wID){
       case IDC_DEBUG_MEM:
       	switch(wNotifyCode){
           	case CBN_SELENDOK:
                   if((i = ::SendMessage(hwnd,CB_GETCURSEL,0,0)) != CB_ERR){
                       i = ::SendMessage(hwnd,CB_GETITEMDATA,i,0);
                       dw = get_CurrentCPU()->r_memmap()[i].Address;
                       UpdateVertScrollBarDisDump(dw,i);
                       SendDlgItemMessage(m_hWnd,IDC_DEBUG_CPUMODE,CB_SETCURSEL,(WPARAM)dbg.bTrackMode,0);
                       FillListDiss(&dw,DBV_RUN,0);
                   }
               break;
           }
       break;
       case IDC_DEBUG_ARMMODE:
       	switch(wNotifyCode){
           	case CBN_SELENDOK:
                   i = ::SendMessage(hwnd,CB_GETCURSEL,0,0);
                   dw = (DWORD)::SendMessage(hwnd,CB_GETITEMDATA,i,0);
                   SendDlgItemMessage(m_hWnd,IDC_DEBUG_REG,LB_RESETCONTENT,0,0);
                   cpu = get_CurrentCPU();
                   for(i=0;i<17;i++){
		                sprintf(s,"R%02d  0x%08X",i,cpu->r_modereg(dw,i));
                       SendDlgItemMessage(m_hWnd,IDC_DEBUG_REG,LB_ADDSTRING,0,(LPARAM)s);
                   }
               break;
           }
       break;
   	case IDC_DEBUG_CPU:
       	switch(wNotifyCode){
           	case CBN_SELENDOK:
                   nCpu = ::SendMessage(hwnd,CB_GETCURSEL,0,0);
                 	::EnableWindow(GetDlgItem(m_hWnd,IDC_DEBUG_Q),nCpu == 0 ? TRUE : FALSE);
                   SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEM,CB_RESETCONTENT,0,0);
                   for(i=0;i<get_CurrentCPU()->r_memmapsize();i++){
                       if(get_CurrentCPU()->r_memmap()[i].bFlags & 1){
   					    dw = (DWORD)SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEM,CB_ADDSTRING,0,(LPARAM)get_CurrentCPU()->r_memmap()[i].Name);
                           SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEM,CB_SETITEMDATA,dw,i);
                       }
                   }
                   floatDlg.OnChangeCPU();
                   Update_Debugger();
               break;
           }
       break;
       case IDC_DEBUG_GO:
       	switch(wNotifyCode){
           	case BN_CLICKED:
					SendDlgItemMessage(m_hWnd,IDC_DEBUG_EDITGO,WM_GETTEXT,(WPARAM)30,(LPARAM)s);
                   strlwr(s);
#ifdef _DEBPRO
                   ((u32 *)s1)[0] = 0;
                   if((p = strchr(s,'r')) != NULL){
                   	sscanf(p+1,"%2d",&i);
                       wsprintf(s1,"0x%08X",get_CurrentCPU()->gp_regs[i]);
                   	if((p = strpbrk(p+1,"+-*/")) != NULL)
                   		strcat(s1,p);
                   	strcpy(s,s1);
                   }
#endif
					dw = StrToHex(s);
                   UpdateAddressBar(dw,TRUE);
                   FillListDiss(&dw,DBV_RUN,0);
               break;
           }
       break;
       case IDC_DEBUG_C:
       	dw = ::SendMessage(hwnd,BM_GETCHECK,0,0) == BST_CHECKED ? 1 : 0;
           get_CurrentCPU()->w_cf((u8)dw);
       break;
       case IDC_DEBUG_V:
       	dw = ::SendMessage(hwnd,BM_GETCHECK,0,0) == BST_CHECKED ? 1 : 0;
           get_CurrentCPU()->w_vf((u8)dw);
       break;
       case IDC_DEBUG_Z:
       	dw = ::SendMessage(hwnd,BM_GETCHECK,0,0) == BST_CHECKED ? 1 : 0;
           get_CurrentCPU()->w_zf((u8)dw);
       break;
       case IDC_DEBUG_N:
       	dw = ::SendMessage(hwnd,BM_GETCHECK,0,0) == BST_CHECKED ? 1 : 0;
           get_CurrentCPU()->w_nf((u8)dw);
       break;
		case IDC_DEBUG_Q:
       	dw = ::SendMessage(hwnd,BM_GETCHECK,0,0) == BST_CHECKED ? 1 : 0;
			((_arm9 *)get_CurrentCPU())->w_qf((u8)dw);
       break;
       case IDC_DEBUG_CPUMODE:
       	dbg.bTrackMode = (u8)::SendMessage(hwnd,CB_GETCURSEL,0,0);
			dw = dbg.views[dbg.iCurrentView].StartAddress;
           FillListDiss(&dw,DBV_VIEW,1);
           UpdateAddressBar(dw,1);
       break;
		case IDC_DEBUG_REG:
			switch(wNotifyCode){
           	case LBN_DBLCLK:
                   ChangeRegister(CRT_CHANGE);
               break;
           }
       break;
#if defined(_DEBPRO)
       case IDC_DEBUG_CONSOLE:
       	switch(wNotifyCode){
           	case LBN_DBLCLK:
                   dw = ::SendMessage(hwnd,LB_GETCURSEL,0,0);
                   if(dw != (DWORD)-1){
                   	::SendMessage(hwnd,LB_GETTEXT,dw,(LPARAM)s);
                       s[10] = 0;
						::SendDlgItemMessage(m_hWnd,IDC_DEBUG_EDITGO,
                       	WM_SETTEXT,(WPARAM)0,(LPARAM)s);
                       ::SendMessage(m_hWnd,WM_COMMAND,MAKEWPARAM(IDC_DEBUG_GO,BN_CLICKED),
                       	(LPARAM)GetDlgItem(m_hWnd,IDC_DEBUG_GO));
                   }
               break;
           }
       break;
#endif
   }
}
//---------------------------------------------------------------------------
void LDebugDlg::ChangeRegister(CHANGEREGTYPE mode)
{
   int index;
   RECT rc;
   char s[50];
   BOOL bChange;
   u32 value;

   if((index = ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_REG,LB_GETCURSEL,0,0)) == LB_ERR)
       return;
   bChange = FALSE;
   switch(mode){
       case CRT_ZERO:
           bChange = TRUE;
           value = 0;
       break;
       case CRT_INCR:
           value = get_CurrentCPU()->r_gpreg(index) + 1;
           bChange = TRUE;
       break;
       case CRT_DECR:
           value = get_CurrentCPU()->r_gpreg(index) - 1;
           bChange = TRUE;
       break;
       case CRT_CHANGE:
	        ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_REG,LB_GETITEMRECT,index,(LPARAM)&rc);
	        MapWindowPoints(GetDlgItem(m_hWnd,IDC_DEBUG_REG),m_hWnd,(LPPOINT)&rc,2);
		   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_REG,LB_GETTEXT,index,(LPARAM)s);
	        if(InputText(m_hWnd,&rc,0,&s[5],20)){
		        value = StrToHex(&s[5]);
               bChange = TRUE;
           }
       break;
   }
   if(!bChange)
       return;
   get_CurrentCPU()->w_gpreg(index,value);
   wsprintf(s,"R%02d  0x%08X",index,get_CurrentCPU()->r_gpreg(index));
   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_REG,LB_DELETESTRING,index,0);
   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_REG,LB_INSERTSTRING,index,(LPARAM)s);
}
//---------------------------------------------------------------------------
BOOL LDebugDlg::OnDrawItem(WORD wID,LPDRAWITEMSTRUCT lpDIS)
{
	char string[200];
   RECT rc;
	DWORD dwPos,dw;
   HBRUSH hBrush,hOldBrush;
   HPEN hOldPen;
	BOOL bString;
   int i,i1,i2;
	SIZE sz;
#if defined(_DEBPRO2)
	LBreakPoint *p2;
#endif

	switch(wID){
   	case IDC_DEBUG_DIS:
       	if(lpDIS->itemID == -1)
           	return TRUE;
           *(&rc) = *(&lpDIS->rcItem);
			SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_GETTEXT,(WPARAM)lpDIS->itemID,(LPARAM)string);
			if(*string == 1)
           	bString = TRUE;
           else{
          		dwPos = StrToHex(string);
               bString = FALSE;
               GetCurrentAddressPipe(&dw,0,0,!bPass);
           }
#if defined(_DEBUG)
           if((lpDIS->itemState & ODS_SELECTED) && !bString){
              	if((lpDIS->itemState & ODS_FOCUS) && dw == dwPos){
                 	SetBkColor(lpDIS->hDC,GetSysColor(COLOR_HIGHLIGHT));
                   SetTextColor(lpDIS->hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
                   FillRect(lpDIS->hDC,&rc,GetSysColorBrush(COLOR_HIGHLIGHT));
               }                                           //2000dba
               else{
          			SetBkColor(lpDIS->hDC,GetSysColor(COLOR_WINDOW));
               		SetTextColor(lpDIS->hDC,GetSysColor(COLOR_WINDOWTEXT));
              		FillRect(lpDIS->hDC,&rc,GetSysColorBrush(COLOR_WINDOW));
                   DrawFocusRect(lpDIS->hDC,&rc);
               }
           }
           else{
          		SetBkColor(lpDIS->hDC,GetSysColor(COLOR_WINDOW));
               if(bString)
               	SetTextColor(lpDIS->hDC,RGB(0,0,255));
               else
               	SetTextColor(lpDIS->hDC,GetSysColor(COLOR_WINDOWTEXT));
              	FillRect(lpDIS->hDC,&rc,GetSysColorBrush(COLOR_WINDOW));
           }
#endif
           if(!bString){
                get_CurrentCPU()->decode_ins((u8)(GetCurrentIncPipe(dbg.iCurrentView) == 2 ? 1 : 0),dwPos,string);
#if defined(_DEBPRO)
                    if(cLinoSearch[0] != 0 && strstr(&string[9],cLinoSearch) != NULL){
                    bLino = FALSE;
                    SetBkColor(lpDIS->hDC,GetSysColor(COLOR_MENU));
                    SetTextColor(lpDIS->hDC,GetSysColor(COLOR_INFOTEXT));
                    FillRect(lpDIS->hDC,&rc,GetSysColorBrush(COLOR_MENU));
                }
#endif
#ifdef _DEBPRO2
                if((p2 = prgBP.Find(dwPos)) != NULL){
                    *(&rc) = *(&lpDIS->rcItem);
                    rc.left += 5;
                    rc.right = rc.left + 7;
                    rc.top = rc.top + (((rc.bottom - rc.top) - 6) >> 1);
                    rc.bottom = rc.top + 7;
                    hBrush = p2->is_Enable() ? CreateSolidBrush(RGB(255,0,0)) :
                        CreateSolidBrush(RGB(0,255,0));
                    hOldBrush = (HBRUSH)SelectObject(lpDIS->hDC,hBrush);
                    hOldPen = (HPEN)SelectObject(lpDIS->hDC,GetStockObject(BLACK_PEN));
                    Ellipse(lpDIS->hDC,rc.left,rc.top,rc.right,rc.bottom);
                    ::SelectObject(lpDIS->hDC,hOldBrush);
                    ::SelectObject(lpDIS->hDC,hOldPen);
                    ::DeleteObject(hBrush);
                }
#endif
                GetTextExtentPoint32(lpDIS->hDC,"A",1,&sz);
                *(&rc) = *(&lpDIS->rcItem);
                SetBkMode(lpDIS->hDC,TRANSPARENT);
                for(i=0;string[i] != 0;){
                    if(string[i++] == 32)
                        break;
                }
                rc.left += 18;
                ::DrawText(lpDIS->hDC,string,i,&rc,DT_NOCLIP|DT_LEFT|DT_SINGLELINE);
                rc.left += sz.cx*i;
                for(i2=i,i1=0;string[i] != 0;i1++){
                    if(string[i++] == 32)
                        break;
                }
                ::DrawText(lpDIS->hDC,&string[i2],i1,&rc,DT_NOCLIP|DT_LEFT|DT_SINGLELINE);
                rc.left += sz.cx*i1;
                ::DrawText(lpDIS->hDC,&string[i],-1,&rc,DT_NOCLIP|DT_LEFT|DT_SINGLELINE);
			}
           else{
               *(&rc) = *(&lpDIS->rcItem);
               rc.left += 18;
               SetBkMode(lpDIS->hDC,TRANSPARENT);
               ::DrawText(lpDIS->hDC,string+1,-1,&rc,DT_NOCLIP|DT_LEFT|DT_SINGLELINE|DT_EXPANDTABS|DT_NOPREFIX);
           }
       	return TRUE;
       default:
       	return FALSE;
   }
}
//---------------------------------------------------------------------------
LONG LDebugDlg::OnNotify(LPNMHDR p)
{
   MENUITEMINFO mii;
   char c[200];

	switch(p->idFrom){
#if defined(_DEBPRO2)
       case IDC_DEBUG_LV1:
			return prgBP.OnNotify((NM_LISTVIEW *)p);
#endif
       case IDC_DEBUG_TOOLBAR:
           switch(p->code){
               case TBN_GETINFOTIPA:
                   mii.cbSize = sizeof(MENUITEMINFO);
                   mii.fMask = MIIM_TYPE;
                   mii.fType = MFT_STRING;
                   mii.dwTypeData = c;
                   mii.cch = 200;
                   GetMenuItemInfo(GetMenu(),((LPNMTBGETINFOTIP)p)->iItem,FALSE,&mii);
                   lstrcpy(((LPNMTBGETINFOTIP)p)->pszText,c);
               break;
           }
       break;
   }
   return 0;
}
//---------------------------------------------------------------------------
BOOL LDebugDlg::IsDialogMessage(LPMSG lpMsg)
{
   if(m_hWnd == NULL || !::IsWindow(m_hWnd))
       return FALSE;
   if(TranslateAccelerator(m_hWnd,accel,lpMsg))
       return TRUE;
   if(::IsDialogMessage(m_hWnd,lpMsg))
       return TRUE;
   if(::IsDialogMessage(palViewer.Handle(),lpMsg))
       return TRUE;
   if(::IsDialogMessage(mtxViewer.Handle(),lpMsg))
       return TRUE;
#ifdef _DEBPRO
   if(::IsDialogMessage(spriteViewer.Handle(),lpMsg))
       return TRUE;
   if(::IsDialogMessage(textureViewer.Handle(),lpMsg))
       return TRUE;
   if(::IsDialogMessage(ioViewer.Handle(),lpMsg))
       return TRUE;
   if(inspector.IsDialogMessage(lpMsg))
       return TRUE;
   if(::IsDialogMessage(tileViewer.Handle(),lpMsg))
       return TRUE;
   if(::IsDialogMessage(polyViewer.Handle(),lpMsg))
       return TRUE;
#endif
   return FALSE;
}
//---------------------------------------------------------------------------
void LDebugDlg::OnClose()
{
	bDebug = FALSE;
   bPass = TRUE;
   floatDlg.Destroy();
   Destroy();
}
//---------------------------------------------------------------------------
void LDebugDlg::OnInitMenuPopup(HMENU hMenu,UINT uPos,BOOL bSM)
{
#if defined(_DEBPRO2)
	MENUITEMINFO mi;
   int i,i1;
#endif

#if defined(_DEBPRO2)
	if(hMenu != ::GetSubMenu(::GetSubMenu(GetMenu(),0),4))
   	return;
   i1 = RecentFiles.Count();
   i = GetMenuItemCount(hMenu) - 1;
   for(;i>0;i--)
   	DeleteMenu(hMenu,i,MF_BYPOSITION);
   if(i1 == 0)
   	return;
	ZeroMemory(&mi,sizeof(mi));
   mi.cbSize = sizeof(mi);
   mi.fMask = MIIM_TYPE;
   mi.fType = MFT_SEPARATOR;
  	InsertMenuItem(hMenu,1,TRUE,&mi);
   for(i=i1;i>0;i--){
		ZeroMemory(&mi,sizeof(mi));
       mi.cbSize = sizeof(mi);
       mi.fMask = MIIM_ID|MIIM_TYPE;
       mi.fType = MFT_STRING;
       mi.wID = ID_DEBUG_RECENT_RESET + i;
       mi.dwTypeData = (char *)((LString *)RecentFiles.GetItem(i))->c_str();
       InsertMenuItem(hMenu,mi.wID,FALSE,&mi);
   }
#endif
}
//---------------------------------------------------------------------------
BOOL CALLBACK LDebugDlg::DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	LDebugDlg *pDlg;

  	pDlg = (LDebugDlg *)GetWindowLong(hwndDlg,GWL_USERDATA);
	switch(uMsg){
       case WM_INITMENUPOPUP:
#if defined(_DEBPRO2)
       	pDlg->OnInitMenuPopup((HMENU)wParam,(UINT)LOWORD(lParam),(BOOL)HIWORD(lParam));
#endif
       break;
		case WM_SIZE:
           pDlg->OnSize(wParam,lParam);
       break;
   	case WM_NOTIFY:
          pDlg->OnNotify((LPNMHDR)lParam);
       break;
       case WM_GETMINMAXINFO:
           pDlg->OnGetMinMaxInfo((LPMINMAXINFO)lParam);
       break;
   	case WM_INITDIALOG:
       	::SetWindowLong(hwndDlg,GWL_USERDATA,(LONG)lParam);
           ((LDebugDlg *)lParam)->OnInitDialog(hwndDlg);
       break;
       case WM_VSCROLL:
  			if(pDlg != NULL)
           	pDlg->OnVScroll(wParam,lParam);
       break;
       case WM_DRAWITEM:
       	if(pDlg != NULL)
           	return pDlg->OnDrawItem((WORD)wParam,(LPDRAWITEMSTRUCT)lParam);
		break;
       case WM_CLOSE:
           if(pDlg != NULL)
           	pDlg->OnClose();
       break;
       case WM_DESTROY:
           return TRUE;
       case WM_CTLCOLORSTATIC:
       	switch(GetDlgCtrlID((HWND)lParam)){
           	case IDC_DEBUG_REG:
               case IDC_DEBUG_MEMVIEW:
               case IDC_LINE:
               case IDC_CYCLES:
//               	SetWindowLong(hwndDlg,DWL_MSGRESULT,
//                   	(LONG)GetSysColorBrush(COLOR_WINDOW));
					SetBkColor((HDC)wParam,GetSysColor(COLOR_WINDOW));
                   SetTextColor((HDC)wParam,GetSysColor(COLOR_WINDOWTEXT));
                   return (BOOL)GetSysColorBrush(COLOR_WINDOW);
           }
       break;
       case WM_MEASUREITEM:
           switch(wParam){
               case IDC_DEBUG_DIS:
                   ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = 14;
               break;
           }
           return TRUE;
       case WM_COMMAND:
       	if(lParam < 2 || lParam == (LPARAM)GetDlgItem(hwndDlg,IDC_DEBUG_TOOLBAR))
               pDlg->OnMenuSelect(LOWORD(wParam));
           else
           	pDlg->OnCommand(HIWORD(wParam),LOWORD(wParam),(HWND)lParam);
       break;
   }
	return FALSE;
}
//---------------------------------------------------------------------------
void LDebugDlg::UpdateVertScrollBarDisDump(DWORD address,int index)
{
   char s[31];
   SCROLLINFO si={0};
   HWND hwnd;
   u8 i;

   if(index == -1){
       wsprintf(s,"%d",address);                //20ac354
       index = MemoryStringToIndex(s);
   }
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_SB);
   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_ALL;
   si.nMax = get_CurrentCPU()->r_memmap()[index].Size-1;
   if(si.nMax != 0){
       i = (u8)GetCurrentIncPipe(dbg.iCurrentView);
       si.nPage = (dbg.iMaxItem * i);
       dbg.yScroll = (((address - get_CurrentCPU()->r_memmap()[index].Address) / i) * i) & (get_CurrentCPU()->r_memmap()[index].Size -1);
   }
   else{
       si.nPage = 1;
       dbg.yScroll = 0;
   }
   SetScrollInfo(hwnd,SB_CTL,&si,FALSE);
   SetScrollPos(hwnd,SB_CTL,dbg.yScroll,TRUE);
}
#if defined(_DEBPRO2)
//---------------------------------------------------------------------------
void LDebugDlg::OnGoBP(LBreakPoint *p)
{
   char s[30];

	if(p == NULL)
   	return;
   wsprintf(s,"0x%08X",p->get_Address());
   if(p->get_Type() == BT_PROGRAM){
       SetWindowText(GetDlgItem(m_hWnd,IDC_DEBUG_EDITGO),s);
       ::SendMessage(m_hWnd,WM_COMMAND,MAKEWPARAM(IDC_DEBUG_GO,BN_CLICKED),
   	    (LPARAM)GetDlgItem(m_hWnd,IDC_DEBUG_GO));
   }
   else{
       SetWindowText(GetDlgItem(floatDlg.Handle(),IDC_DEBUG_MEMVIEW_CB1),s);
       SendDlgItemMessage(floatDlg.Handle(),IDC_DEBUG_MEMVIEW_CB1,WM_KEYDOWN,VK_RETURN,0);
   }
}
#endif
//---------------------------------------------------------------------------
void LDebugDlg::UpdateAddressBar(u32 dwAddress,u8 forceUpdate)
{
   int index,i;
   char s[20];

   wsprintf(s,"0x%08X",dwAddress);
   index = MemoryStringToIndex(s);
   i = SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEM,CB_GETCURSEL,0,0);
   i = SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEM,CB_GETITEMDATA,i,0);
   if(forceUpdate != 0 || i != index){
       i = SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEM,CB_GETCOUNT,0,0);
       for(i--;i>=0;i--){
           if(SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEM,CB_GETITEMDATA,i,0) == index){
               SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEM,CB_SETCURSEL,(WPARAM)i,0);
               break;
           }
       }

       UpdateVertScrollBarDisDump(dwAddress,index);
   }
   else{
       dwAddress -= get_CurrentCPU()->r_memmap()[index].Address;
       dbg.yScroll = dwAddress & (get_CurrentCPU()->r_memmap()[index].Size - 1);
       SetScrollPos(GetDlgItem(m_hWnd,IDC_DEBUG_SB),SB_CTL,dbg.yScroll,TRUE);
   }
   SendDlgItemMessage(m_hWnd,IDC_DEBUG_CPUMODE,CB_SETCURSEL,(WPARAM)dbg.bTrackMode,0);
}
//---------------------------------------------------------------------------
void LDebugDlg::OnVScroll(WPARAM wParam,LPARAM lParam)
{
   SCROLLINFO si={0};
   HWND hwnd;
   int i;
   u8 inc;
   DWORD dw,dw1;
   LPDISVIEW p1;

   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_ALL;
   if(!GetScrollInfo((hwnd = (HWND)lParam),SB_CTL,&si))
       return;
   inc = GetCurrentIncPipe(DBV_VIEW);
   switch(LOWORD(wParam)){
       case SB_TOP:
           i = 0;
       break;
       case SB_BOTTOM:
           i = (si.nMax - si.nPage) + 1;
       break;
       case SB_PAGEDOWN:
           i = dbg.yScroll + inc * dbg.iMaxItem;
           if(i > (int)((si.nMax - si.nPage)+1))
               i = (si.nMax - si.nPage) + 1;
       break;
       case SB_PAGEUP:
           if((i = dbg.yScroll - inc * dbg.iMaxItem) < 0)
               i = 0;
       break;
       case SB_LINEUP:
           if((i = dbg.yScroll - inc) < 0)
               i = 0;
       break;
       case SB_LINEDOWN:
           i = dbg.yScroll + inc;
           if(i > (int)((si.nMax - si.nPage)+1))
               i = (si.nMax - si.nPage) + 1;
       break;
       case SB_THUMBPOSITION:
           i = si.nPos;
       break;
       case SB_THUMBTRACK:
           i = si.nTrackPos;
       break;
       default:
           i = dbg.yScroll;
       break;
   }
   i = (i / inc) * inc;
   if(i == (int)dbg.yScroll)
       return;
   dbg.yScroll = i;
   ::SetScrollPos(hwnd,SB_CTL,i,TRUE);
   dw = SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEM,CB_GETCURSEL,0,0);
   dw = SendDlgItemMessage(m_hWnd,IDC_DEBUG_MEM,CB_GETITEMDATA,dw,0);
   p1 = &dbg.views[dbg.iCurrentView];
   dw1 = p1->StartAddress;
   dw = dw1 & ~(get_CurrentCPU()->r_memmap()[dw].Size - 1);
   dw |= i;
   FillListDiss(&dw,DBV_VIEW,0);
}
//---------------------------------------------------------------------------
u8 LDebugDlg::GetCurrentAddressPipe(u32 *Address,u8 withindex,u8 remap,BOOL bFromPC)
{
   char s[20];
   u32 d,i,mask;
   IARM7 *arm;
   LPDISVIEW p;

   arm = get_CurrentCPU();
//   if(bThreadRun){
	if(bFromPC)
       d = arm->get_CurrentPC();
   else{
		p = &dbg.views[dbg.iCurrentView];
       d = p->StartAddress;
   }
/*   }
   else
       d = MemoryAddress[7].Address;*/

   if(remap && (i = MemoryAddressToIndex(d)) != -1){
       mask = 1 << (int)(log2(arm->r_memmap()[i].Size - 1) + 1);
       d = (d & ~(mask - 1)) | (d & (mask-1));
   }
   *Address = d;
   if(!withindex)
       return 0;
   wsprintf(s,"%d",d);
   return (u8)MemoryStringToIndex(s);
}
//---------------------------------------------------------------------------
int LDebugDlg::MemoryAddressToIndex(DWORD dwAddress)
{
   int i,best_index;
   IARM7 *arm;
   u64 end;
   s64 best_end;

   arm = get_CurrentCPU();
   best_end = -1;
   best_index = -1;
   for(i=0;i < (int)arm->r_memmapsize();i++){
       if(dwAddress >= arm->r_memmap()[i].Address){
           end = (u64)arm->r_memmap()[i].Address + (u64)((arm->r_memmap()[i].vSize == (u32)-1) ? arm->r_memmap()[i].Size : arm->r_memmap()[i].vSize);
           if(dwAddress < end){
               if(best_end == -1 || best_end  > end){
                   best_end = end;
                   best_index = i;
               }
           }
       }
/*       else if(dwAddress < arm->r_memmap()[i].Address){
           i--;
           break;
       }*/
   }
   return best_index;
}
//---------------------------------------------------------------------------
int LDebugDlg::MemoryStringToIndex(char *string)
{
   return MemoryAddressToIndex(StrToHex(string));
}
//---------------------------------------------------------------------------
u8 LDebugDlg::GetCurrentIncPipe(u8 indexView)
{
   u8 inc;
   IARM7 *p;

   p = get_CurrentCPU();
   if(dbg.bTrackMode != 2 && indexView != DBV_RUN)
       inc = (u8)(1 << (2 - dbg.bTrackMode));
   else
       inc = (u8)((p->r_cpsr() & T_BIT) ? 2 : 4);
   return inc;
}
//---------------------------------------------------------------------------
int LDebugDlg::FillListDiss(u32 *p,u8 indexView,u8 nItem)
{
	DWORD dwAddress;
   u8 inc;
   int i,i1;
   LPDISVIEW p1;
   char s[30];
	CompileUnit *unit;
   Function *func;
	LineInfoItem *table;
	LFile *pFile;
	LString s1,s2;

   if(p != NULL)
       dwAddress = *p;
   else
       GetCurrentAddressPipe(&dwAddress,0,0);
   p1 = &dbg.views[(dbg.iCurrentView = indexView)];
   inc = GetCurrentIncPipe(indexView);
   if(nItem != 0){
       if((dwAddress - p1->EndAddress) >= inc || !dwAddress){//Se super il contenuto della view
           p1->StartAddress = dwAddress;
           i = dbg.iMaxItem;
           SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_RESETCONTENT,0,0);
       }
       else{
           i = 1;
           SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_DELETESTRING,0,0);
           if(indexView == DBV_RUN)
               p1->StartAddress += inc;
       }
   }
   else{
       p1->StartAddress = dwAddress;
       i = dbg.iMaxItem;
      	SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_RESETCONTENT,0,0);
   }
   for(;i > 0;i--){
   	if(ds.get_RomReader() != NULL){
   		if(ds.get_RomReader()->GetCurrentFunction(dwAddress,(void **)&func,(void **)&unit,(void **)&table) != -1){
               s1 = "";
               if(table->file){
               	if((i1 = unit->lineInfoTable->files[table->file-1].dir))
                     	s1 = unit->lineInfoTable->dirs[i1-1];
                   if(s1.IsEmpty() || s1.Pos(":") < 0){
                       s1 = unit->compdir;
               		s1 += DPC_PATH;
                   	if(i1)
                       	s1 += unit->lineInfoTable->dirs[i1-1];
                   }
                   if(s1[s1.Length()] != DPC_PATH)
                   	s1 += DPC_PATH;
                   s1 += unit->lineInfoTable->files[table->file-1].name;
               }
//               s1.c_str()[0]='e';
           	if((pFile = new LFile(s1.c_str())) != NULL && pFile->Open()){
               	s1.Length(5000);
                  	if(i > 0 && pFile->ReadLine(func->line,s1.c_str(),5000)){
                   	s2 = "\1";
                       s2 += unit->lineInfoTable->files[table->file-1].name;
                       s2 += ".";
                       s2 += func->line;
                       s2 += ": ";
                       s1.LeftTrim();
                       s1.LeftTrim(9);
                       s2 += s1;
                      	SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_INSERTSTRING,(WPARAM)-1,(LPARAM)s2.c_str());
                       i--;
                   }
               }
               if(pFile != NULL)
  	               	delete pFile;
       	}
       }
       if(i > 0){
			wsprintf(s,"%08X",dwAddress);
           SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_INSERTSTRING,(WPARAM)-1,(LPARAM)s);
           dwAddress += inc;
       }
   }
   p1->EndAddress = dwAddress - inc;
   if(indexView == DBV_RUN)
		*(&dbg.views[DBV_VIEW]) = *p1;
   return 1;
}
//---------------------------------------------------------------------------
BOOL LDebugDlg::OnKeyDown(WPARAM wParam,LPARAM lParam)
{
   IARM7 *arm;
   DWORD dw;

   if(!bDebug || m_hWnd == NULL || !::IsWindow(m_hWnd))
   	return FALSE;
   switch(wParam){
       case VK_F4:
   	    bPass = FALSE;
           UpdateToolBar();
       break;
       case VK_F5:
           bPass = TRUE;
           nBreakMode = 0;
           UpdateToolBar();
       break;
       case VK_F8:
           if(!bPass)
               bFlag = FALSE;
           UpdateToolBar();
       break;
       case VK_F6:
           if(BreakpointFromListBox(NULL,&dwBreak)){
           	nBreakMode = 1;
              	bPass = TRUE;
               UpdateToolBar();
           }
       break;
#if defined(_DEBPRO2)
       case VK_F11:
           arm = get_CurrentCPU();
           dwBreak = arm->gp_regs[15];
           if(!(arm->status & T_BIT))
               dwBreak -= 4;
           nBreakMode = 1;
           bPass = TRUE;
       break;
       case VK_F9:
         	if(BreakpointFromListBox(NULL,&dw)){
          		InsertBreakPoint(dw);
               UpdateToolBar();
           }
       break;
#endif
#if defined(_DEBPRO)
       case VK_F2:
           if(!bLino){
               SendDlgItemMessage(m_hWnd,IDC_DEBUG_EDITGO,WM_GETTEXT,(WPARAM)19,(LPARAM)cLinoSearch);
//               strupr(cLinoSearch);
				bLino = TRUE;
           }
           else
               bLino = FALSE;
       break;
       case VK_F7:
/*   			fp = fopen("c:\\windows\\desktop\\arm9.bin","wb");
               for(i=0;i<0x344e;i++){
                   dw = read_word(0x20099c0+(i*4));
                   fwrite(&dw,sizeof(char),4,fp);
               }
               fclose(fp);
/*   			fp = fopen("c:\\windows\\desktop\\arm7.bin","wb");
               for(i=0;i<0x10000;i++){
                   dw = read_word(0x2380000+(i*4));
                   fwrite(&dw,sizeof(char),4,fp);
               }
               fclose(fp);*/
       break;
#endif
   }
	return FALSE;
}
//---------------------------------------------------------------------------
void LDebugDlg::WaitDebugger(BOOL bArm7)
{
   int i,item;
   DWORD dw;
   LPDISVIEW p;
   u8 inc;
	MSG msg;
	char s[300];
#ifdef _LWYLIN
   u32 d,mask;
   IARM7 *arm;
   char out[300];
#endif

#ifdef _LWYLIN
	if(pFileOut7 == NULL || pFileOut9 == NULL){
       GetModuleFileName(NULL,s,MAX_PATH);
       lstrcpy(s,LString(s).Path().c_str());
		i = lstrlen(s);
       if(s[i-1] != DPC_PATH)
       	lstrcat(s,DPS_PATH);
   }
   if(pFileOut7 == NULL){
		lstrcpy(out,s);
       lstrcat(out,"outarm7.txt");
       pFileOut7 = new LFile(out);
       pFileOut7->Open(GENERIC_WRITE,CREATE_ALWAYS);
   }
   if(pFileOut9 == NULL){
		lstrcpy(out,s);
       lstrcat(out,"outarm9.txt");
       pFileOut9 = new LFile(out);
       pFileOut9->Open(GENERIC_WRITE,CREATE_ALWAYS);
   }
	if(bArm7 && pFileOut7 != NULL){
       arm = (IARM7 *)&arm7;
       d = arm->get_CurrentPC();
       if((i = MemoryAddressToIndex(d)) != -1){
           mask = 1 << (int)(log2(arm->r_memmap()[i].Size - 1) + 1);
           d = (d & ~(mask - 1)) | (d & (mask-1));
       }
		arm->decode_ins((arm->r_cpsr() & T_BIT) ? 1 : 0,d,out);
		lstrcat(out,"\r\n");
       pFileOut7->Write(out,lstrlen(out));
   }
   if(pFileOut9 != NULL){
       arm = (IARM7 *)&arm9;
       d = arm->get_CurrentPC();
       if((i = MemoryAddressToIndex(d)) != -1){
           mask = 1 << (int)(log2(arm->r_memmap()[i].Size - 1) + 1);
           d = (d & ~(mask - 1)) | (d & (mask-1));
       }
		arm->decode_ins((arm->r_cpsr() & T_BIT) ? 1 : 0,d,out);
		lstrcat(out,"\r\n");
       pFileOut9->Write(out,lstrlen(out));
   }
#endif
	if(!bArm7 && get_CurrentCPU()->r_index() == 7)
   	return;
   if(!bDebug || (bPass && !ControlBP()))
		return;
   bFlag = FALSE;
   p = &dbg.views[DBV_RUN];
   GetCurrentAddressPipe(&dw,FALSE,TRUE);
   inc = GetCurrentIncPipe(DBV_RUN);
   if(dbg.bTrackMode != 2 || inc != dbg.bCurrentTrackMode){
       dbg.bTrackMode = 2;
       dbg.bCurrentTrackMode = inc;
       bFlag = TRUE;
   }
   else if(p->StartAddress == 0)
       bFlag = TRUE;
   else if(dbg.iCurrentView != DBV_RUN){
/*       i = ((int)p->StartAddress - (int)dbg.views[DBV_VIEW].StartAddress);
       if(abs(i) < (dbg.iMaxItem >> 1)){
           if(i < 0){
               CopyMemory(p,&dbg.views[DBV_VIEW],sizeof(DISVIEW));*/
               dbg.iCurrentView = DBV_RUN;
/*           }
       }
       else*/
           bFlag = TRUE;
   }
   else if(dw < p->StartAddress || dw > p->EndAddress)
       bFlag = TRUE;
   if(bFlag)
       FillListDiss(NULL,DBV_RUN,1);
   Update_Reg();
	floatDlg.Update();
   mtxViewer.Update();
#ifdef _DEBPRO
   ioViewer.Update();
   inspector.Update();
#endif
   SetFocus(disLB.Handle());
   item = SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_GETCOUNT,0,0);
   for(i=0;i<item;i++){
		SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_GETTEXT,i,(LPARAM)s);
		if(s[0] == 1)
       	continue;
       if(StrToHex(s) == dw){
       	SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_SETCURSEL,i,0);
           break;
       }
   }
   UpdateAddressBar(p->StartAddress,0);
   bFlag = TRUE;
   while(!bPass && bFlag && !ds.CanRun()){
#if defined(_DEBPRO)
		if(bLino){
       	if(SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_GETCURSEL,0,0) == dbg.iMaxItem-1){
           	::SendMessage(m_hWnd,WM_VSCROLL,MAKEWPARAM(SB_LINEDOWN,0),
              		(LPARAM)GetDlgItem(m_hWnd,IDC_DEBUG_SB));
               SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_SETCURSEL,dbg.iMaxItem-1,0);
       	}
       }
#endif
      	((LDS *)parent)->GetMessage(&msg);
		((LDS *)parent)->DispatchMessage(&msg);
#ifdef __WIN32__
       if(msg.message == WM_QUIT){
//         	bQuit = FALSE;
           return;
       }
		else if(msg.message != WM_KEYDOWN)
       	continue;
#ifdef _LWYLIN
		if(pFileOut9 != NULL)
       	pFileOut9->Flush();
       if(pFileOut7 != NULL)
       	pFileOut7->Flush();
#endif
       OnKeyDown(msg.wParam,msg.lParam);
#else
		if(msg.type != GDK_KEY_PRESS)
			continue;
		OnKeyDown(KeyCodeToVK((GdkEventKey *)&msg),0);
#endif
   }
}
//---------------------------------------------------------------------------
s8 LDebugDlg::InsertBreakPoint(u32 adress)
{
#if defined(_DEBPRO2)
   LBreakPoint *p;

   if((p = prgBP.Find(adress,BT_PROGRAM)) != NULL)
		p->set_Enable(!p->is_Enable());
   else if(prgBP.Add(adress) == NULL)
   	return FALSE;
	RedrawWindow(GetDlgItem(m_hWnd,IDC_DEBUG_DIS),NULL,NULL,RDW_ERASE|RDW_INVALIDATE|RDW_ERASENOW);
	if(floatDlg.get_ActivePageType() == PG_BREAKPOINT)
   	prgBP.UpdateList(GetDlgItem(floatDlg.Handle(),IDC_DEBUG_LV1),BT_PROGRAM);
   return TRUE;
#else
   return FALSE;
#endif
}
#ifdef _DEBPRO2
//---------------------------------------------------------------------------
BOOL LDebugDlg::InsertBreakpointFromIndex(int index)
{
   u32 value;

   if(!BreakpointFromIndex(index,NULL,&value))
       return FALSE;
   return InsertBreakPoint(value);
}
#endif
//---------------------------------------------------------------------------
BOOL LDebugDlg::BreakpointFromIndex(int index,char *p,u32 *adr)
{
   int len;
	LString s;

   if(p != NULL)
       *p = 0;
	len = SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_GETTEXTLEN,index,0);
   if(len == LB_ERR)
       return FALSE;
	s.Capacity(len + 1);
   SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_GETTEXT,index,(LPARAM)s.c_str());
   if(s[1] == 1)
   	return FALSE;
   if(p != NULL)
       lstrcpy(p,s.c_str());
   *adr = StrToHex(s.c_str());
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LDebugDlg::BreakpointFromListBox(char *p,u32 *adr)
{
   int index;
	LString s;

   if(p != NULL)
       *p = 0;
   if((index = SendDlgItemMessage(m_hWnd,IDC_DEBUG_DIS,LB_GETCURSEL,0,0)) == LB_ERR)
       return FALSE;
   return BreakpointFromIndex(index,p,adr);
}
//---------------------------------------------------------------------------
BOOL LDebugDlg::ControlMemoryBP(u32 address,u32 access,u32 value)
{
#if defined(_DEBPRO2)
	LBreakPoint *p;
   int index;
   IARM7 *arm;

	if(!bDebug || dwCheckAddress)
   	return FALSE;
/*	if(pFileIO == NULL){
   	if((pFileIO = new LFile("io.txt")) != NULL){
   		if(!pFileIO->Open(GENERIC_WRITE,CREATE_ALWAYS)){
				delete pFileIO;
           	pFileIO = NULL;
           }
       }
   }
   if(pFileIO != NULL && (address >> 24) == 4){
   	if((access & AMM_READ)){
			if(access & AMM_BYTE)
           	value = read_byte(address);
           else if(access & AMM_HWORD)
           	value = read_hword(address);
           else if(access & AMM_WORD)
           	value = read_word(address);
       }
   	pFileIO->WriteF("%c%c : %08X %08X\r\n",access & AMM_READ ? 'R' : 'W',
       	(access & AMM_BYTE ? 'B' : (access & AMM_WORD ? 'W' : 'H')),
       	address,value);
   }*/
   dwCheckAddress++;
#ifdef _DEBPRO
	floatDlg.set_CurrentAccess(address,access);
#endif
	if((p = prgBP.Check(address,(u8)access,(HIWORD(access) == 9 ? (IARM7 *)&arm9 : (IARM7 *)&arm7),value)) == NULL){
       dwCheckAddress--;
       return FALSE;
   }
   if(p->is_Break()){
   	bPass = FALSE;
       UpdateToolBar();
       MessageBeep(MB_ICONHAND);
   }
#endif
#ifdef _DEBPRO
   index = LOBYTE(HIWORD(access));
   arm = index == 9 ? (IARM7 *)&arm9 : (IARM7 *)&arm7;
   if(access & AMM_READ)
   	value = arm->read_mem(address,(u8)(access & 0xF));
   dwCheckAddress--;
	return floatDlg.get_ConsolleView()->WriteMessageConsole(arm,"%cArm%i %s to 0x%08X - 0x%08X",MSGT_MEMBRK,
       index,(access & AMM_READ ? "Reading" : "Writing"),address,value);
#else
   dwCheckAddress--;
   return TRUE;
#endif
}
//---------------------------------------------------------------------------
BOOL LDebugDlg::ControlBP()
{
   u32 dwCurrentAddress;

#if defined(_DEBPRO2)
   LBreakPoint *p;
#endif
   if(!bPass)
       return FALSE;
   GetCurrentAddressPipe(&dwCurrentAddress,FALSE,FALSE);
   if(nBreakMode == 1 && dwCurrentAddress == dwBreak){
   	dwBreak = 0;
       nBreakMode = 0;
       bPass = FALSE;
       UpdateToolBar();
       return TRUE;
   }
#if defined(_DEBPRO2)
	if(!(p = prgBP.Check(dwCurrentAddress,0,get_CurrentCPU())))
   	return FALSE;
//   if(floatDlg.get_ActivePageType() == PG_BREAKPOINT){
//       floatDlg.SelectPage(PG_BREAKPOINT);
       prgBP.Select(floatDlg.get_BreakPointListView(),p,TRUE);
//   }
   bPass = FALSE;
   UpdateToolBar();
   MessageBeep(MB_ICONHAND);
   return TRUE;
#else
   return FALSE;
#endif
}
//---------------------------------------------------------------------------
void LDebugDlg::OnGetMinMaxInfo(LPMINMAXINFO lParam)
{
   lParam->ptMinTrackSize.x = rcWin.right - rcWin.left;
   lParam->ptMinTrackSize.y = (rcWin.bottom - rcWin.top) - (!floatDlg.is_Float() ? 0 : 200);
}
//---------------------------------------------------------------------------
void LDebugDlg::OnSize(WPARAM wParam,LPARAM lParam)
{
   RECT rc,rc1,rc2,rc3;
   int cx,cy,i,i1;
   HWND hwnd;
   POINT pt;
   HDWP hdwp;
   SCROLLINFO si={0};
#ifndef __WIN32__
	GValue o;
#endif

	::SendDlgItemMessage(m_hWnd,IDC_DEBUG_SB1,WM_SIZE,wParam,lParam);
   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,WM_SIZE,wParam,lParam);
   cx = LOWORD(lParam);
   cy = HIWORD(lParam);
   CopyRect(&rc1,&rcClient);
	SetRect(&rcClient,0,0,cx,cy);
	if((hdwp = BeginDeferWindowPos(11)) == NULL)
       return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_C);
   ::GetWindowRect(hwnd,&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
   pt.x = cx - (rc1.right - rc.right) - (rc.right - rc.left);
   hdwp = DeferWindowPos(hdwp,hwnd,0,pt.x,rc.top,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
	if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_N);
   ::GetWindowRect(hwnd,&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
   pt.x = cx - (rc1.right - rc.right) - (rc.right - rc.left);
   hdwp = DeferWindowPos(hdwp,hwnd,0,pt.x,rc.top,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_V);
   ::GetWindowRect(hwnd,&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
   pt.x = cx - (rc1.right - rc.right) - (rc.right - rc.left);
   hdwp = DeferWindowPos(hdwp,hwnd,0,pt.x,rc.top,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_Z);
   ::GetWindowRect(hwnd,&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
   pt.x = cx - (rc1.right - rc.right) - (rc.right - rc.left);
   hdwp = DeferWindowPos(hdwp,hwnd,0,pt.x,rc.top,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_Q);
   ::GetWindowRect(hwnd,&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
   pt.x = cx - (rc1.right - rc.right) - (rc.right - rc.left);
   hdwp = DeferWindowPos(hdwp,hwnd,0,pt.x,rc.top,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_I);
   ::GetWindowRect(hwnd,&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
   pt.x = cx - (rc1.right - rc.right) - (rc.right - rc.left);
   hdwp = DeferWindowPos(hdwp,hwnd,0,pt.x,rc.top,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_F);
   ::GetWindowRect(hwnd,&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
   pt.x = cx - (rc1.right - rc.right) - (rc.right - rc.left);
   hdwp = DeferWindowPos(hdwp,hwnd,0,pt.x,rc.top,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_T);
   ::GetWindowRect(hwnd,&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
   pt.x = cx - (rc1.right - rc.right) - (rc.right - rc.left);
   hdwp = DeferWindowPos(hdwp,hwnd,0,pt.x,rc.top,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_S);
   ::GetWindowRect(hwnd,&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
   pt.x = cx - (rc1.right - rc.right) - (rc.right - rc.left);
   hdwp = DeferWindowPos(hdwp,hwnd,0,pt.x,rc.top,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_REG);
  	::GetWindowRect(hwnd,&rc);
   i1 = cx - 10;
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
   pt.x = cx - (rc1.right - rc.right) - (rc.right - rc.left);
   hdwp = DeferWindowPos(hdwp,hwnd,0,pt.x,rc.top,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_CPU);
   ::GetWindowRect(hwnd,&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
   pt.x = cx - (rc1.right - rc.right) - (rc.right - rc.left);
   hdwp = DeferWindowPos(hdwp,hwnd,0,pt.x,rc.top,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_LABEL_MODE);
   ::GetWindowRect(hwnd,&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
   hdwp = DeferWindowPos(hdwp,hwnd,0,pt.x-(rc.right - rc.left),rc.top,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_CPUMODE);
   ::GetWindowRect(hwnd,&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
   pt.x = cx - (rc1.right - rc.right) - (rc.right - rc.left);
   hdwp = DeferWindowPos(hdwp,hwnd,0,pt.x,rc.top,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_LABEL_CPU);
   ::GetWindowRect(hwnd,&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
   hdwp = DeferWindowPos(hdwp,hwnd,0,pt.x-(rc.right - rc.left),rc.top,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   if(hdwp == NULL) return;
   ::GetWindowRect(GetDlgItem(m_hWnd,IDC_DEBUG_SB1),&rc2);
   if(!floatDlg.is_Float()){
       rc2.top = (rc2.bottom - rc2.top) + 4;
		pt.y = cy - rc2.top;
		hwnd = floatDlg.Handle();
       ::GetWindowRect(hwnd,&rc);
       ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
       pt.y -= (rc.bottom - rc.top);
       hdwp = DeferWindowPos(hdwp,hwnd,0,0,pt.y,cx,(rc.bottom-rc.top),SWP_FRAMECHANGED|SWP_NOREPOSITION);
       if(hdwp == NULL) return;
   }
   else{
       pt.y = cy - (rc2.bottom - rc2.top) - 8;
       ::GetWindowRect(GetDlgItem(m_hWnd,IDC_DEBUG_ARMMODE),&rc);
       pt.y -= rc.bottom - rc.top;
       ::GetWindowRect(GetDlgItem(m_hWnd,IDC_DEBUG_LABEL_LINE),&rc);
       pt.y -= rc.bottom - rc.top;
   }
#ifndef __WIN32__
	::GetWindowRect(GetDlgItem(m_hWnd,IDC_DEBUG_TOOLBAR),&rc);
	pt.y -= (rc.bottom - rc.top);
	::GetWindowRect((HWND)::GetMenu(m_hWnd),&rc);
	pt.y -= (rc.bottom - rc.top);
	::GetWindowRect(((GtkDialog *)m_hWnd)->action_area,&rc);
	pt.y -= (rc.bottom - rc.top);
	o.g_type = G_TYPE_INT;
	gtk_widget_style_get_property(m_hWnd,"action-area-border",&o);
	pt.y -= o.data[0].v_int;
#endif
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_SB);
   ::GetWindowRect(hwnd,&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
   pt.x = cx - (rc1.right - rc.right) - (rc.right - rc.left);
   hdwp = DeferWindowPos(hdwp,hwnd,0,pt.x,rc.top,(rc.right-rc.left),pt.y-rc.top,SWP_FRAMECHANGED|SWP_NOREPOSITION);
   if(hdwp == NULL) return;

   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_DIS);
   ::GetWindowRect(hwnd,&rc);
   ::GetWindowRect(GetDlgItem(m_hWnd,IDC_LINE),&rc3);
   rc3.top -= rc.bottom;
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
   i = pt.y-rc.top;
	dbg.iMaxItem = (u8)(i / ::SendMessage(hwnd,LB_GETITEMHEIGHT,0,0));
	i = rc3.top+rc.top;
	pt.y -= rc.top;
   hdwp = DeferWindowPos(hdwp,hwnd,0,0,0,pt.x-rc.left,pt.y,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOMOVE);
   if(hdwp == NULL) return;

   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_PAGE;
   si.nPage = (dbg.iMaxItem * GetCurrentIncPipe(dbg.iCurrentView));
   SetScrollInfo(GetDlgItem(m_hWnd,IDC_DEBUG_SB),SB_CTL,&si,FALSE);
#ifdef _DEBPRO
	hwnd = GetDlgItem(m_hWnd,IDC_CYCLES);
   ::GetWindowRect(hwnd,&rc);
	hdwp = DeferWindowPos(hdwp,hwnd,0,i1-(rc.right-rc.left),pt.y + i,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   i1 -= rc.right-rc.left + 3;
   if(hdwp == NULL) return;
#endif
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_ARMMODE);
   ::GetWindowRect(hwnd,&rc);
   hdwp = DeferWindowPos(hdwp,hwnd,0,i1-(rc.right-rc.left),pt.y + i,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   i1 -= rc.right-rc.left + 3;
   if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_LABEL_CPUMODE);
   ::GetWindowRect(hwnd,&rc);
   hdwp = DeferWindowPos(hdwp,hwnd,0,i1,pt.y+i-(rc.bottom-rc.top),0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_LINE);
   ::GetWindowRect(hwnd,&rc);
   hdwp = DeferWindowPos(hdwp,hwnd,0,i1 -= rc.right-rc.left,pt.y + i,0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_DEBUG_LABEL_LINE);
   ::GetWindowRect(hwnd,&rc);
   hdwp = DeferWindowPos(hdwp,hwnd,0,i1,pt.y+i-(rc.bottom-rc.top),0,0,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSIZE);
   if(hdwp == NULL) return;
   EndDeferWindowPos(hdwp);
}
//---------------------------------------------------------------------------
LRESULT LDisListBox::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	HWND hwnd;
   int i;
   DWORD dw;
   HMENU h;
   POINT pt;
   RECT rc;

	switch(uMsg){
       case WM_MOUSEWHEEL:
           i = (int)(short)HIWORD(wParam);
           i /= WHEEL_DELTA;
#ifdef __WIN32__
			hwnd = GetParent(m_hWnd);
#else
			hwnd = (HWND)GetWindowLong(m_hWnd,GWL_NOTIFYPARENT);
#endif
           dw = (i < 0) ? SB_LINEDOWN : SB_LINEUP;
           for(i = abs(i);i > 0;i--){
               ::SendMessage(hwnd,WM_VSCROLL,MAKEWPARAM(dw,0),
                       (LPARAM)GetDlgItem(hwnd,IDC_DEBUG_SB));
           }
      		return 0;
       break;
       case WM_SIZE:
			dw = debugDlg.get_CurrentView()->StartAddress;
           debugDlg.FillListDiss(NULL,DBV_RUN,1);
           debugDlg.UpdateAddressBar(dw,1);
       break;
#ifdef _DEBPRO2
       case WM_RBUTTONDOWN:
           if((h = LoadMenu(langManager.get_CurrentLib(),MAKEINTRESOURCE(IDR_DEBUG_DIS_MENU))) != NULL){
               GetCursorPos(&pt);
          	 	TrackPopupMenu(GetSubMenu(h,0),TPM_LEFTALIGN,pt.x,pt.y,0,GetParent(m_hWnd),NULL);
           	DestroyMenu(h);
               return 0;
           }
       break;
       case WM_LBUTTONDBLCLK:
             	rc.left = 1;
               rc.right = rc.left + 18;
               pt.x = (signed short)LOWORD(lParam);
               pt.y = rc.top = (signed short)HIWORD(lParam);
               rc.bottom = rc.top + 2;
               if(PtInRect(&rc,pt)){
                   i = pt.y / SendMessage(LB_GETITEMHEIGHT,0,0);
                   debugDlg.InsertBreakpointFromIndex(i);
               }
       break;
#endif
   	case WM_KEYDOWN:
			switch(wParam){
       		case VK_NEXT:
#ifdef __WIN32__
				    hwnd = GetParent(m_hWnd);
#else
				    hwnd = (HWND)GetWindowLong(m_hWnd,GWL_NOTIFYPARENT);
#endif

           		::SendMessage(hwnd,WM_VSCROLL,MAKEWPARAM(SB_PAGEDOWN,0),
               			(LPARAM)GetDlgItem(hwnd,IDC_DEBUG_SB));
           		return 0;
           	case VK_PRIOR:
#ifdef __WIN32__
				hwnd = GetParent(m_hWnd);
#else
				hwnd = (HWND)GetWindowLong(m_hWnd,GWL_NOTIFYPARENT);
#endif
           		::SendMessage(hwnd,WM_VSCROLL,MAKEWPARAM(SB_PAGEUP,0),
               			(LPARAM)GetDlgItem(hwnd,IDC_DEBUG_SB));
           		return 0;
               case VK_UP:
               	if(::SendMessage(m_hWnd,LB_GETCURSEL,0,0) == 0){
#ifdef __WIN32__
					hwnd = GetParent(m_hWnd);
#else
					hwnd = (HWND)GetWindowLong(m_hWnd,GWL_NOTIFYPARENT);
#endif
           			::SendMessage(hwnd,WM_VSCROLL,MAKEWPARAM(SB_LINEUP,0),
               			(LPARAM)GetDlgItem(hwnd,IDC_DEBUG_SB));
					   	::SendMessage(m_hWnd,LB_SETCURSEL,0,0);
               		return 0;
                   }
               break;
       		case VK_DOWN:
#ifdef __WIN32__
				hwnd = GetParent(m_hWnd);
#else
				hwnd = (HWND)GetWindowLong(m_hWnd,GWL_NOTIFYPARENT);
#endif
                i = ::SendMessage(m_hWnd,LB_GETCOUNT,0,0);
               	if(::SendMessage(m_hWnd,LB_GETCURSEL,0,0) == i-1){
           			::SendMessage(hwnd,WM_VSCROLL,MAKEWPARAM(SB_LINEDOWN,0),
               			(LPARAM)GetDlgItem(hwnd,IDC_DEBUG_SB));
					   	::SendMessage(m_hWnd,LB_SETCURSEL,i-1,0);
                       return 0;
               	}
               break;
           }
       break;
   }
	return LWnd::OnWindowProc(uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LRegListBox::LRegListBox()
{
}
//---------------------------------------------------------------------------
LRegListBox::~LRegListBox()
{
}
//---------------------------------------------------------------------------
LRESULT LRegListBox::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   HMENU h;
   POINT pt,pt1;

   switch(uMsg){
   	case WM_RBUTTONDOWN:
           if((h = LoadMenu(langManager.get_CurrentLib(),MAKEINTRESOURCE(IDR_DEBUG_REGISTER))) != NULL){
           	GetCursorPos(&pt);
               pt1 = pt;
               ::MapWindowPoints(NULL,m_hWnd,&pt1,1);
               SendMessage(LB_SETCURSEL, pt1.y / SendMessage(LB_GETITEMHEIGHT,0,0),0);
          	 	TrackPopupMenu(GetSubMenu(h,0),TPM_LEFTALIGN,pt.x,pt.y,0,m_hWnd,NULL);
           		DestroyMenu(h);
				return 0;
           }
       break;
       case WM_COMMAND:
           if(HIWORD(wParam) == 0){
               switch(LOWORD(wParam)){
                   case ID_DEBUG_REGISTER_ZERO:
                       debugDlg.ChangeRegister(CRT_ZERO);
                   break;
                   case ID_DEBUG_REGISTER_DECREMENT:
                       debugDlg.ChangeRegister(CRT_DECR);
                   break;
                   case ID_DEBUG_REGISTER_INCREMENT:
                       debugDlg.ChangeRegister(CRT_INCR);
                   break;
                   case ID_DEBUG_REGISTER_CHANGE:
                       debugDlg.ChangeRegister(CRT_CHANGE);
                   break;
               }
           }
       break;
   }
   return LWnd::OnWindowProc(uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
#ifdef _DEBPRO
LInspectDlg::LInspectDlg() : LDlg()
{
   text = "";
}
//---------------------------------------------------------------------------
LInspectDlg::~LInspectDlg()
{
}
//---------------------------------------------------------------------------
LRESULT LInspectDlg::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   switch(uMsg){
       case WM_COMMAND:
           switch(LOWORD(wParam)){
               case IDOK:
                   if(HIWORD(wParam) == BN_CLICKED){
                       text.Length(300);
                       GetWindowText(GetDlgItem(m_hWnd,IDC_COMBOBOX1),text.c_str(),300);
                       EndDialog(m_hWnd,1);
                   }
               break;
               case IDCANCEL:
                   if(HIWORD(wParam) == BN_CLICKED)
                       EndDialog(m_hWnd,0);
               break;
           }
       break;
       case WM_CLOSE:
           EndDialog(m_hWnd,0);
       break;
   }
   return FALSE;
}
#endif
#endif






