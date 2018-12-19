#include "lds.h"
#include "resource.h"
#include "util.h"
#include "io.h"
#include "timers.h"
#include "dma.h"
#include "lregkey.h"
#include "card.h"
#include "pxi.h"
#include "rtc.h"
#include "fw.h"
#include "spi.h"
#include "cpm.h"
#include "keyconfig.h"
#include "ndsreader.h"
#include "arm7bin.h"
#include "cheats.h"
#include "license.h"
#include "language.h"
#include "lzari.h"
#include "guitar_grip.h"
#include "gbabackup.h"
#include "rominfo.h"
#include <math.h>

#define POS_MENU_RECENT 12
/*
   emulate cartridge latency

       if(card.cycles){\
           card.cycles--;\
           if(card.cycles == 0)\
               StartDMA(5);\
       }\
*/

#define EXEC9\
   wCycles += exec9();\
   if((((u32 *)(io_mem + 0x208))[0] & 1)){\
       if(((u32 *)(io_mem + 0x210))[0] & nIRQ9)\
           arm9irq();\
   }

#define EXEC7\
	if((bDouble ^= bArm7)){\
       x1 = exec7();\
       if((((u32 *)(io_mem7 + 0x208))[0] & 1)){\
           if(((u32 *)(io_mem7 + 0x210))[0] & nIRQ7)\
               arm7irq();\
       }\
   }

#define EXEC7_GBA\
	x1 = exec7();\
   if((((u16 *)(io_mem7 + 0x200))[0] & nIRQ7)){\
   	if((((u16 *)(io_mem7 + 0x208))[0] & 1))\
       	arm7irq();\
   }


//---------------------------------------------------------------------------
static HWND hwndPropertySheet;
u8 keyBoard[]={'Z','A','X',VK_RETURN,VK_RIGHT,VK_LEFT,VK_UP,VK_DOWN,'S',VK_TAB,'Q','W'};
u8 keyGrip[]={VK_F1,VK_F2,VK_F3,VK_F4};
//---------------------------------------------------------------------------
#ifndef __WIN32__
static guint key_snooper;
//---------------------------------------------------------------------------
static gint OnKeyEvent(GtkWidget *grab_widget,GdkEventKey *event,gpointer func_data)
{
	if(event->type == GDK_KEY_PRESS){
       if(TranslateAccelerator(upLcd.Handle(),ds.accel,(LPMSG)event))
           return TRUE;
       ds.OnKeyDown(KeyCodeToVK(event),0);
   }
   else if(event->type == GDK_KEY_RELEASE)
       ds.OnKeyUp(KeyCodeToVK(event),0);//KeyCodeToVK((XKeyEvent *)&msg)
   return FALSE;
}
#endif
//---------------------------------------------------------------------------
LDS::LDS() : LCPU(), LMMU(), LApp()
{
   pPlugInContainer = NULL;
   pActiveAudioPlugIn = NULL;
   p3DPlugIn = NULL;
#ifdef __WIN32__
	classId = 0;
#endif
	dwOptions = NDSO_LOAD_DEFAULT_ARM7|NDSO_OPTIMIZE_CPU|NDSO_OPTIMIZE_LOOP;
#ifdef _DEBPRO
   pLog = NULL;
#endif
   bAutoIncreaseSpeed = 0;
	nCommand = -1;
   pfnLoop = &LDS::OnLoop;
   nCores = -1;
}
//---------------------------------------------------------------------------
LDS::~LDS()
{
   LRegKey key;
   int i;

   for(i=0;i<sizeof(hEvents)/sizeof(HANDLE);i++){
       if(hEvents[i] != NULL)
           ::SetEvent(hEvents[i]);
   }
   if(hEvents[1] != NULL)
       WaitForSingleObject(hEvents[1],INFINITE);
   if(pPlugInContainer != NULL){
       delete pPlugInContainer;
       pPlugInContainer = NULL;
   }
   RecentFiles.Save("Software\\iDeaS\\Settings\\RecentFile");
   if(key.Open("Software\\iDeaS\\Settings")){
       key.WriteString("LastDirectory",lastPath.c_str());
       key.WriteString("SaveGamePath",savegamePath.c_str());
       key.WriteString("SaveStatePath",savestatePath.c_str());
       key.WriteString("ScreenShotsPath",screenshotPath.c_str());
       key.WriteString("CheatsPath",cheatPath.c_str());
       key.WriteLong("EEPROMID",get_EEPROMID());
       key.WriteLong("TouchSettings",nTouchSettings);
       key.WriteLong("Language",FirmWare_get_LanguageSettings());
       key.WriteLong("Settings",dwOptions);
       if(dwOptions & NDSO_FRAMES_AUTO_SKIP)
           nSkipCount = -1;
       key.WriteLong("SkipFrames",nSkipCount);
       key.WriteLong("KEY_A",keyBoard[0]);
       key.WriteLong("KEY_B",keyBoard[1]);
       key.WriteLong("KEY_SELECT",keyBoard[2]);
       key.WriteLong("KEY_START",keyBoard[3]);
       key.WriteLong("KEY_RIGHT",keyBoard[4]);
       key.WriteLong("KEY_LEFT",keyBoard[5]);
       key.WriteLong("KEY_UP",keyBoard[6]);
       key.WriteLong("KEY_DOWN",keyBoard[7]);
       key.WriteLong("KEY_R",keyBoard[8]);
       key.WriteLong("KEY_L",keyBoard[9]);
       key.WriteLong("KEY_X",keyBoard[10]);
       key.WriteLong("KEY_Y",keyBoard[11]);
       key.WriteString("Name",FirmWare_get_UserName());
       key.WriteLong("ExRam",get_ExpansionRam());
       key.WriteLong("LoadMostRecentSaveState",SaveStateList.get_LoadRecent());
       key.WriteLong("EnableSSE",hasSSE());
       key.WriteFloat("Zoom",upLcd.get_Scale());
       key.WriteLong("BackupAutoSave",(dwOptions & NDSO_BACKUP_AUTO_SAVE) ? TRUE : FALSE);
       key.WriteLong("UseTempFile",nUseTempFile);
       if(bAutoIncreaseSpeed)
           nIncreaseSpeed = -1;
       key.WriteLong("CpuSpeed",nIncreaseSpeed);
       key.WriteLong("FramesSkipCapture",dwOptions & NDSO_FRAMES_SKIP_CAPTURE ? TRUE : FALSE);
       key.WriteLong("GBAGamePackID",(LONG)GetGamePackID());
       key.WriteLong("EnableCheatList",(LONG)cheatsManager.is_Enable());
       key.Close();
   }
#ifdef __WIN32__
   if(classId != 0)
   	UnregisterClass("IDEAS",hInst);
#else
	if(key_snooper != 0)
		gtk_key_snooper_remove(key_snooper);
	key_snooper = 0;
#endif
#ifdef _DEBPRO
   if(pLog != NULL)
       delete pLog;
#endif
}
//---------------------------------------------------------------------------
BOOL LDS::Init(int argc,char **argv)
{
	LRegKey key;
   WORD wID;
   IMenu *menu;
   LString s;
   double d;
   DWORD dw;
   int i;

   if(!LApp::Init(argc,argv))
       return FALSE;
#ifdef _LICENSE
   if(!CreateLicenseKey())
       return FALSE;
#endif
	RecentFiles.set_MaxElem(10);
   RecentFiles.Load("Software\\iDeaS\\Settings\\RecentFile");
   langManager.Init();
#ifdef __WIN32__
	WNDCLASS wc = {0};

	wc.style 			= CS_VREDRAW|CS_HREDRAW|CS_OWNDC;
   wc.lpfnWndProc 		= DefWindowProc;
   wc.hInstance 		= hInst;
   wc.hCursor 			= ::LoadCursor(NULL,IDC_ARROW);
   wc.lpszClassName 	= "IDEAS";
   wc.hbrBackground	= (HBRUSH)::GetStockObject(BLACK_BRUSH);
   wc.hIcon			= ::LoadIcon(hInst,MAKEINTRESOURCE(2));
   if((classId = RegisterClass(&wc)) == 0)
   	return FALSE;
#else
	key_snooper = gtk_key_snooper_install(OnKeyEvent,NULL);
#endif
	accel = LoadAccelerators(hInst,MAKEINTRESOURCE(111));
	if(accel == NULL)
		return FALSE;
   if(!LMMU::Init()){
       MessageBox(NULL,"An error occur during the LMMU creation.","iDeaS Emulator",MB_OK|MB_ICONERROR);
   	return FALSE;
   }
   if(!LCPU::Init()){
       MessageBox(NULL,"An error occur during the LCPU creation.","iDeaS Emulator",MB_OK|MB_ICONERROR);
   	return FALSE;   }
   if(!upLcd.Init()){
   	MessageBox(NULL,"An error occur during the upLcd creation.","iDeaS Emulator",MB_OK|MB_ICONERROR);
   	return FALSE;
   }
   if(!downLcd.Init()){
   	MessageBox(NULL,"An error occur during the downLcd creation.","iDeaS Emulator",MB_OK|MB_ICONERROR);
   	return FALSE;
   }
   setIOTable();
   if((pPlugInContainer = new PlugInContainer()) == NULL)
   	return FALSE;
   upLcd.Show(SW_SHOW);
   downLcd.Show(SW_SHOW);
   if(!pPlugInContainer->Create(-1))
   	return FALSE;
   dw = -1;
	if(!pPlugInContainer->Init(dw)){
        s = "An error occur during the plugins list creation.\r\n";
        s += pPlugInContainer->GetLastError();
        MessageBox(NULL,s.c_str(),"iDeaS Emulator",MB_OK|MB_ICONERROR);
        return FALSE;
	}
   upLcd.SetActive();
   {
       SYSTEM_INFO si;

       GetSystemInfo(&si);
       i = si.dwNumberOfProcessors;
       if(key.Open("Software\\iDeaS\\Settings")){
           i = (int)key.ReadLong("MultiCore",i);
           key.Close();
       }
   }
   set_MultiCore(i);

	Reset();
   if(key.Open("Software\\iDeaS\\Settings")){
       menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
       dwOptions = key.ReadLong("Settings",NDSO_LOAD_DEFAULT_ARM7|NDSO_OPTIMIZE_CPU|NDSO_OPTIMIZE_LOOP|NDSO_USE_DMA_LATENCY);
		menu->CheckMenuItem(ID_SKIP_OPTIMIZE_CPU,MF_BYCOMMAND|((dwOptions & NDSO_OPTIMIZE_CPU) ? MF_CHECKED : MF_UNCHECKED));
       menu->CheckMenuItem(ID_SKIP_OPTIMIZE_LOOPS,MF_BYCOMMAND|((dwOptions & NDSO_OPTIMIZE_LOOP) ? MF_CHECKED : MF_UNCHECKED));
       menu->CheckMenuItem(ID_USE_DMA_LATENCY,MF_BYCOMMAND|((dwOptions & NDSO_USE_DMA_LATENCY) ? MF_CHECKED : MF_UNCHECKED));

       wID = (WORD)key.ReadLong("EEPROMID",ID_EEPROM_AUTO);
       OnSelectEEPROM(wID);
       menu->CheckMenuItem(wID,MF_BYCOMMAND|MF_CHECKED);
       lastPath = key.ReadString("LastDirectory","");
       savegamePath = key.ReadString("SaveGamePath","");
       savestatePath = key.ReadString("SaveStatePath","");
       screenshotPath = key.ReadString("ScreenShotsPath","");
       cheatPath = key.ReadString("CheatsPath","");
		nTouchSettings = key.ReadLong("TouchSettings",1);
       if((nSkipCount = key.ReadLong("SkipFrames",-1)) < 0){
           nSkipCount = 0;
           dwOptions |= NDSO_FRAMES_AUTO_SKIP;
       }
		FirmWare_set_LanguageSettings(key.ReadLong("Language",4));
       s = key.ReadString("Name","Lino");
#ifdef _DEBPRO
       if(!(s == "Lino") || nTouchSettings != 1 || FirmWare_get_LanguageSettings() != 4)
           d = d;
#endif
       FirmWare_set_UserName(s.c_str());
		keyBoard[0] = (u8)key.ReadLong("KEY_A",keyBoard[0]);
		keyBoard[1] = (u8)key.ReadLong("KEY_B",keyBoard[1]);
       keyBoard[2] = (u8)key.ReadLong("KEY_SELECT",keyBoard[2]);
       keyBoard[3] = (u8)key.ReadLong("KEY_START",keyBoard[3]);
       keyBoard[4] = (u8)key.ReadLong("KEY_RIGHT",keyBoard[4]);
       keyBoard[5] = (u8)key.ReadLong("KEY_LEFT",keyBoard[5]);
       keyBoard[6] = (u8)key.ReadLong("KEY_UP",keyBoard[6]);
       keyBoard[7] = (u8)key.ReadLong("KEY_DOWN",keyBoard[7]);
       keyBoard[8] = (u8)key.ReadLong("KEY_R",keyBoard[8]);
       keyBoard[9] = (u8)key.ReadLong("KEY_L",keyBoard[9]);
       keyBoard[10] = (u8)key.ReadLong("KEY_X",keyBoard[10]);
       keyBoard[11] = (u8)key.ReadLong("KEY_Y",keyBoard[11]);
       set_ExpansionRam((u8)key.ReadLong("ExRam",0));
       set_SSE((BOOL)key.ReadLong("EnableSSE",TRUE));
       SaveStateList.set_LoadRecent((BOOL)key.ReadLong("LoadMostRecentSaveState",FALSE));
       menu->CheckMenuItem(ID_FILE_STATE_AUTOLOADMOSTRCNT,MF_BYCOMMAND|SaveStateList.get_LoadRecent() ? MF_CHECKED : MF_UNCHECKED);
       menu->CheckMenuItem(ID_SSE_EXTENSION,MF_BYCOMMAND|(hasSSE() ? MF_CHECKED : MF_UNCHECKED));
       menu->EnableMenuItem(ID_SSE_EXTENSION,(CheckSSE() ? MF_ENABLED : MF_GRAYED));
       d = key.ReadFloat("Zoom",1);
       downLcd.set_Scale(d);
       upLcd.set_Scale(d);
       menu->CheckMenuItem(ID_WINDOW_SIZE0,MF_BYCOMMAND|MF_UNCHECKED);
       menu->CheckMenuItem(ID_WINDOW_SIZE15,MF_BYCOMMAND|MF_UNCHECKED);
       menu->CheckMenuItem(ID_WINDOW_SIZE20,MF_BYCOMMAND|MF_UNCHECKED);
       if(d == 1)
           menu->CheckMenuItem(ID_WINDOW_SIZE0,MF_BYCOMMAND|MF_CHECKED);
       else if(d == 1.5)
           menu->CheckMenuItem(ID_WINDOW_SIZE15,MF_BYCOMMAND|MF_CHECKED);
       else if(d == 2.0)
			menu->CheckMenuItem(ID_WINDOW_SIZE20,MF_BYCOMMAND|MF_CHECKED);
       dwOptions |= key.ReadLong("BackupAutoSave",0) ? NDSO_BACKUP_AUTO_SAVE : 0;
       menu->CheckMenuItem(ID_EEPROM_AUTOSAVE,MF_BYCOMMAND|((dwOptions & NDSO_BACKUP_AUTO_SAVE) ? MF_CHECKED : MF_UNCHECKED));
       nUseTempFile = key.ReadLong("UseTempFile",TEMP);
       nIncreaseSpeed = key.ReadLong("CpuSpeed",0);
       if(nIncreaseSpeed == -1){
           bAutoIncreaseSpeed = 0;
           nIncreaseSpeed = 0;
           wID = ID_SKIP_AUTO;
       }
       else{
           bAutoIncreaseSpeed = 0;
           switch(nIncreaseSpeed){
               case 0:
                   wID = ID_SKIP_CPUNONE;
               break;
               case 25:
                   wID = ID_SKIP_CPU25;
               break;
               case 50:
                   wID = ID_SKIP_CPU50;
               break;
               case 75:
                   wID = ID_SKIP_CPU75;
               break;
               case 100:
                   wID = ID_SKIP_CPU100;
               break;
               case 125:
                   wID = ID_SKIP_CPU125;
               break;
               case 150:
                   wID = ID_SKIP_CPU150;
               break;
               default:
                   wID = 0xFFFF;
               break;
           }
       }
       dwOptions |= key.ReadLong("FramesSkipCapture",FALSE) ? NDSO_FRAMES_SKIP_CAPTURE : 0;
		menu->CheckMenuItem(ID_SKIP_FRAME_CAPTURE,MF_BYCOMMAND|((dwOptions & NDSO_FRAMES_SKIP_CAPTURE) ? MF_CHECKED : MF_UNCHECKED));
		SetGamePackID((WORD)key.ReadLong("GBAGamePackID",ID_EEPROM_32K));
       cheatsManager.Enable((BOOL)key.ReadLong("EnableCheatList",TRUE));
		menu->CheckMenuItem(ID_FILE_CHEAT_DISABLELIST,MF_BYCOMMAND|(!cheatsManager.is_Enable() ? MF_CHECKED : MF_UNCHECKED));
       key.Close();
       menu->Release();
   }
   OnMenuSelect(wID);
   OnMenuSelect((WORD)(dwOptions & NDSO_FRAMES_AUTO_SKIP ? ID_SKIP_FRAMEAUTO : ID_SKIP_FRAMENONE+nSkipCount));
   OnMenuSelect(GetGamePackID());

   bQuit = FALSE;
#ifdef __WIN32__
   s = *argv;
#else
	if(argc > 1)
		s = argv[1];
	else
		s = "";
#endif
   if(!s.IsEmpty()){
       s.AllTrim();
       s.RemoveAllChar(34);
       if(!s.IsEmpty()){
           if(OpenRom(s.c_str()))
               upLcd.PostMessage(WM_COMMAND,MAKEWPARAM(ID_FILE_START,0),0);
       }
   }
	return TRUE;
}
//---------------------------------------------------------------------------
void LDS::PostQuitMessage()
{
	LApp::PostQuitMessage();
   wCycles = 10000;
   VCOUNT = 263;
#ifdef _DEBUG
   debugDlg.StopDebugger();
#endif
}
//---------------------------------------------------------------------------
void LDS::Reset(BOOL bAll)
{
	if(bAll){
		bRom = FALSE;
		bRun = FALSE;
       bPause = FALSE;
   	bArm7 = FALSE;
		bArm9 = FALSE;
		bDouble = FALSE;
		dwOptions &= ~NDSO_HINGE;
   }
  	bFreeRunning = FALSE;
   nCommand = -1;
	wCycles = 0;
   wStopCycles = 0;
   dwLastTick = dwFrame = 0;
   dwDispCap = 0;
   dwDispSwap = 0;
   CloseRom();
   set_EmuMode(EMUM_NDS);
	LMMU::Reset();
   LCPU::Reset();
	ResetTimer();
	ResetDMA();
   CARD_reset();
   EEPROM_reset();
   PXI_reset();
   RTC_reset();
   Power_Reset();
   *((u16 *)&io_mem[0x130]) = 0x3FF;
   *((u16 *)&io_mem7[0x130]) = 0x3FF;
   *((u16 *)&io_mem[0x204]) = 0xE880;
   *((u16 *)&io_mem[0x304]) = 0x1;
   *((u16 *)&io_mem7[0x204]) = 0xE880;
   *((u16 *)&io_mem7[0x136]) = 0x43;
   *((u32 *)&io_mem7[0x304]) = 0x1;
   *((u32 *)&io_mem7[0x504]) = 0x200;
   *((u32 *)&io_mem[0x600]) = 0x06000000;

   nGripKeys = 0xFF;

	upLcd.Reset();
   downLcd.Reset();
   FirmWare_Reset();
	pPlugInContainer->Reset();
#ifdef _DEBPRO
   if(pLog)
       delete pLog;
#endif
}
//---------------------------------------------------------------------------
void LDS::set_IRQ(u32 value,BOOL bEnter,u8 c)
{
   if(c & 1){
       nIRQ9 |= value;
       ((u32 *)&io_mem[0x214])[0] = nIRQ9;
       if(bEnter){
           if((((u32 *)(io_mem + 0x208))[0] & 1)){
               if(((u32 *)(io_mem + 0x210))[0] & nIRQ9)
                   arm9irq();
           }
       }
   }
   if(c & 2){
       nIRQ7 |= value;
       if(get_EmuMode() != EMUM_GBA){
           ((u32 *)&io_mem7[0x214])[0] = nIRQ7;
           if(bEnter){
               if((((u32 *)(io_mem7 + 0x208))[0] & 1)){
                   if(((u32 *)(io_mem7 + 0x210))[0] & nIRQ7)
                       arm7irq();
               }
           }
       }
       else{
       	((u16 *)&io_mem7[0x202])[0] = (u16)nIRQ7;
           if(bEnter && (((u16 *)(io_mem7 + 0x208))[0] & 1)){
               if(((u16 *)(io_mem7 + 0x200))[0] & nIRQ7)
                   arm7irq();
           }
       }
   }
}
//---------------------------------------------------------------------------
LONG LDS::DispatchMessage(LPMSG p)
{
#ifdef __WIN32__
   if(!TranslateAccelerator(upLcd.Handle(),accel,p)){
#endif
       if(!cheatsManager.IsDialogMessage(p)
#ifdef _DEBUG
			&& !debugDlg.IsDialogMessage(p) && !floatDlg.IsDialogMessage(p)
#endif
       )
       {
           if(!TranslateMessage(p))
               LApp::DispatchMessage(p);
       }
#ifdef __WIN32__
   }
#endif
   return 0;
}
//---------------------------------------------------------------------------
#ifdef _DEBPRO
void LDS::RefreshLCDs()
{
	u16 wVCOUNT;

	wVCOUNT = VCOUNT;
   upLcd.ResetFrame();
   downLcd.ResetFrame();
   for(VCOUNT = 0;VCOUNT < 192;(VCOUNT)++){
   	upLcd.RenderLine();
   	downLcd.RenderLine();
   }
   upLcd.BitBlt();
   downLcd.BitBlt();
   upLcd.ResetFrame();
   downLcd.ResetFrame();
   VCOUNT = wVCOUNT;
}
#endif
//---------------------------------------------------------------------------
void LDS::MainLoop()
{
   MSG msg;
   DWORD dw,dw1;
   char s[60];

   skip = 0;
   dwStatus = 0;
   dwFrame = 0;
   VCOUNT7 = VCOUNT = 0;
   nElapse = 15;
	while(!bQuit){
       if(bRun){
           dw1 = (this->*pfnLoop)();
           dwFrame++;
           dw = GetTickCount();
           if(!dw1){
               if(dwOptions & NDSO_FRAMES_AUTO_SKIP){
                   nElapse += (int)(dw - dwLastTick1);
                   if(dwFrame & 1){
                       if(nElapse >= 0){
                           if(nElapse <= 33){
                               if(!bFreeRunning)
                                   Sleep(33-nElapse);
                               nElapse = 0;
                           }
                           else
                               nElapse -= 33;
                       }
                   }
                   dwLastTick1 = dw = GetTickCount();
               }
           }
           if((dw - dwLastTick) >= 1000){
               wsprintf(s,"iDeaS Emulator %d fps",(int)dwFrame);
               upLcd.set_Caption(s);
               if(dwOptions & NDSO_FRAMES_AUTO_SKIP){
                   if(dwFrame < 58){
                       if(nSkipCount < (ID_SKIP_FRAME9 - ID_SKIP_FRAMENONE))
                           nSkipCount++;
                   }
                   else if(nSkipCount > 0 && dwFrame > 62)
                       nSkipCount--;
               }
               pPlugInContainer->NotifyState(-1,PNM_COUNTFRAMES,PIS_NOTIFYMASK,dwFrame);
               dwFrame = 0;
               dwLastTick = dw;
               if((dwOptions & NDSO_BACKUP_AUTO_SAVE) && EEPROM_IsModified())
                   EEPROM_AutoSave(FALSE);
               nElapse = 0;
           }
       }
       else if(!bQuit)
           WaitMessage();
       while(PeekMessage(&msg)){
           DispatchMessage(&msg);
#ifdef __WIN32__
           switch(msg.message){
               case WM_KEYDOWN:
                   OnKeyDown(msg.wParam,msg.lParam);
               break;
               case WM_KEYUP:
                   OnKeyUp(msg.wParam,msg.lParam);
               break;
           }
#endif
       }
   }
	ProcessMessages();
}
//---------------------------------------------------------------------------
u32 LDS::OnLoopGBA()
{
   RUNPARAM rp={0};
   int x1;
   DWORD dw;

   pPlugInContainer->NotifyState(-1,PNM_STARTFRAME,PIS_NOTIFYMASK,0);
   rp.type = PIT_VIDEO;
   rp.graphics.mem1 = (u16 *)upLcd.get_OutBuffer();
   rp.graphics.mem2 = (u16 *)downLcd.get_OutBuffer();
   DISPSTAT7 &= ~7;
   dwStatus &= ~7;
   wCycles = 0;
   nTotCycles = 0;

   for(;VCOUNT < 161;(VCOUNT)++){
       VCOUNT7 = VCOUNT;
       DISPSTAT7 &= ~6;
       dwStatus &= ~6;
       if(DISPSTAT7 & 0x20){
           if(VCOUNT7 == ((DISPSTAT7 >> 8))){
               set_IRQ(4,TRUE,2);
               DISPSTAT7 |= 4;
               dwStatus |= 4;
           }
       }
       for(;wCycles < 960;){
#ifdef _DEBUG
           if(debugDlg.is_Active())
               debugDlg.WaitDebugger(TRUE);
#endif
           EXEC7_GBA
           wCycles += (unsigned short)x1;
       }
       DISPSTAT7 |= 2;
       dwStatus |= 2;
       if(DISPSTAT7 & 0x10)
           set_IRQ(2,TRUE,2);
       if(nCores > 1 && VCOUNT == 0)
           WaitForSingleObject(hEvents[1],INFINITE);
       if(!skip){
           skip = 0;
           if(p2DPlugIn != NULL){
               rp.graphics.render = 1;
               p2DPlugIn->Run(&rp);
           }
       }
       else{
           if(p2DPlugIn != NULL){
               rp.graphics.render = 0;
               p2DPlugIn->Run(&rp);
           }
       }
       StartDMA(2);
       for(;wCycles < 1232;){
#ifdef _DEBUG
           if(debugDlg.is_Active())
               debugDlg.WaitDebugger(TRUE);
#endif
           EXEC7_GBA
           wCycles += (unsigned short)x1;
       }
       pPlugInContainer->NotifyState(PIL_AUDIO|PIL_VIDEO,PNM_ENDLINE,PIS_NOTIFYMASK,wCycles);
       RenderTimer((u16)wCycles);
       nTotCycles += wCycles;
       wCycles = 0;
   }
   if(bAutoIncreaseSpeed) ResetCpuSpeed();
   pPlugInContainer->NotifyState(PIL_VIDEO,PNM_ENTERVBLANK,PIS_NOTIFYMASK,MAKELPARAM(MAKEWORD(skip,x1),nSkipCount));
   DISPSTAT7 &= ~6;
   dwStatus &= ~6;
   for(;wCycles < 50;){
#ifdef _DEBUG
       if(debugDlg.is_Active())
           debugDlg.WaitDebugger(TRUE);
#endif
       EXEC7_GBA
       wCycles += (unsigned short)x1;
   }
   StartDMA(1);
   DISPSTAT7 |= 1;
   dwStatus |= 1;
   if((DISPSTAT7 & 0x8))
       set_IRQ(1,TRUE,2);
   if(DISPSTAT7 & 0x20){
       if(VCOUNT7 == ((DISPSTAT7 >> 8))){
           set_IRQ(4,TRUE,2);
           DISPSTAT7 |= 4;
           dwStatus |= 4;
       }
   }
   for(;wCycles < 960;){
#ifdef _DEBUG
       if(debugDlg.is_Active())
           debugDlg.WaitDebugger(TRUE);
#endif
       EXEC7_GBA
       wCycles += (unsigned short)x1;
   }
   DISPSTAT7 |= 2;
   dwStatus |= 2;
   for(;wCycles < 1232;){
#ifdef _DEBUG
       if(debugDlg.is_Active())
           debugDlg.WaitDebugger(TRUE);
#endif
       EXEC7_GBA
       wCycles += (unsigned short)x1;
   }
   pPlugInContainer->NotifyState(PIL_AUDIO|PIL_VIDEO,PNM_ENDLINE,PIS_NOTIFYMASK,wCycles);
   RenderTimer((u16)wCycles);
   (VCOUNT)++;
   VCOUNT7 = VCOUNT;
   nTotCycles += wCycles;
   wCycles = 0;
   if(bAutoIncreaseSpeed)
       IncreaseCpuSpeed(nIncreaseSpeed);
   //remainder line
   for(;VCOUNT < 228;(VCOUNT)++){
       VCOUNT7 = VCOUNT;
       DISPSTAT7 &= ~6;
       dwStatus &= ~6;
       if(DISPSTAT7 & 0x20){
           if(VCOUNT7 == ((DISPSTAT7>> 8))){
               set_IRQ(4,TRUE,2);
               DISPSTAT7 |= 4;
               dwStatus |= 4;
           }
       }
       for(;wCycles < 960;){
#ifdef _DEBUG
           if(debugDlg.is_Active())
               debugDlg.WaitDebugger(TRUE);
#endif
           EXEC7_GBA
           wCycles += (unsigned short)x1;
       }
       DISPSTAT7 |= 2;
       dwStatus |= 2;
       for(;wCycles < 1232;){
#ifdef _DEBUG
           if(debugDlg.is_Active())
               debugDlg.WaitDebugger(TRUE);
#endif
           EXEC7_GBA
           wCycles += (unsigned short)x1;
       }
       pPlugInContainer->NotifyState(PIL_AUDIO|PIL_VIDEO,PNM_ENDLINE,PIS_NOTIFYMASK,wCycles);
       RenderTimer((u16)wCycles);
       nTotCycles += wCycles;
       wCycles = 0;
   }
   VCOUNT = 0;
   if(nCores < 2){
       ((GraphicsPlugList *)pPlugInContainer->get_PlugInList(PIL_GRAPHICS))->Run(upLcd.get_Buffer(),downLcd.get_Buffer());
       if(!skip){
           upLcd.BitBlt();
           downLcd.BitBlt();
       }
   }
   else{
       skip_old = skip;
       SetEvent(hEvents[0]);
   }
  	if(bFreeRunning){
       x1 = 50;
       if(nIncreaseSpeed != x1)
           IncreaseCpuSpeed(x1);
   }
   pPlugInContainer->NotifyState(PIL_VIDEO,PNM_ENDFRAME,PIS_NOTIFYMASK,0);

   if(!bFreeRunning){
       if(++skip > nSkipCount)
           skip = 0;
       if(nIncreaseSpeed != 0)
           IncreaseCpuSpeed(0);
   }
   else{
       if(++skip > (ID_SKIP_FRAME9 - ID_SKIP_FRAMENONE))
           skip = 0;
   }
   if(nCores < 2){
       x1 = (int)pPlugInContainer->get_PlugInList(PIL_INPUT)->get_ActivePlugIn();
       if(x1){
           dw = MAKELONG(((u16 *)(io_mem + 0x130))[0],0);
           if(((LInputPlug *)x1)->Run(&dw)){
               ((u16 *)(io_mem + 0x130))[0] = ((u16 *)(io_mem7 + 0x130))[0] = (u16)dw;
               //                  if(!((dw >> 16) & 2))
                   //                       dw = dw;
               cheatsManager.write_hword(0x04000130,(u16)dw);
           }
       }
   }
   if(pActiveAudioPlugIn == NULL)
       return 0;
   return pActiveAudioPlugIn->Run(bFreeRunning);
}
//---------------------------------------------------------------------------
u32 LDS::OnLoop()
{
   DWORD dw;
   unsigned short x1;
   RUNPARAM rp={0};

   pPlugInContainer->NotifyState(-1,PNM_STARTFRAME,PIS_NOTIFYMASK,0);
   rp.type = PIT_VIDEO;
   rp.graphics.mem1 = (u16 *)upLcd.get_OutBuffer();
   rp.graphics.mem2 = (u16 *)downLcd.get_OutBuffer();
   DISPSTAT &= ~7;
   DISPSTAT7 &= ~7;
   dwStatus &= ~7;
   wCycles = 0;
   nTotCycles = 0;

   for(VCOUNT = 0;VCOUNT < 192;(VCOUNT)++){
       DISPSTAT &= ~6;
       DISPSTAT7 &= ~7;
       dwStatus &= ~6;
       VCOUNT7 = VCOUNT;
       DISPSTAT7 |= (u16)(DISPSTAT & 3);
       if(DISPSTAT & 0x20){
           if(VCOUNT == ((DISPSTAT >> 8) | ((DISPSTAT & 0x80) << 1))){
               set_IRQ(4,TRUE,1);
               DISPSTAT |= 4;
               dwStatus |= 4;
           }
       }
       if(DISPSTAT7 & 0x20){
           if(VCOUNT == ((DISPSTAT7 >> 8) | ((DISPSTAT7 & 0x80) << 1))){
               set_IRQ(4,TRUE,2);
               DISPSTAT7 |= 4;
           }
       }
       set_StopCycles(0);
       for(;wCycles < LINE_CYCLES_VISIBLE*2;){
#ifdef _DEBUG
           if(debugDlg.is_Active())
               debugDlg.WaitDebugger(bDouble);
#endif
           EXEC7
           EXEC9
       }
       DISPSTAT |= 2;
       dwStatus |= 2;
       DISPSTAT7 &= ~3;
       DISPSTAT7 |= (u16)(DISPSTAT & 3);
       if(DISPSTAT & 0x10)
           set_IRQ(2,TRUE,1);
       if(DISPSTAT7 & 0x10)
           set_IRQ(2,TRUE,2);
       if(!skip || (dwDispCap & 0x8000)){
           skip = 0;
           if(nTotCycles < LINE_CYCLES*2 && nCores > 1)
               WaitForSingleObject(hEvents[1],INFINITE);
           if(p2DPlugIn != NULL){
               rp.graphics.render = 1;
               p2DPlugIn->Run(&rp);
           }
       }
       else{
           if(p2DPlugIn != NULL){
               rp.graphics.render = 0;
               p2DPlugIn->Run(&rp);
           }
       }
       StartDMA(2);
       for(x1=0;x1<32;x1++)
           StartDMA(4);
       for(;wCycles < LINE_CYCLES*2;){
#ifdef _DEBUG
           if(debugDlg.is_Active())
               debugDlg.WaitDebugger(bDouble);
#endif
           EXEC7
           EXEC9
       }
       pPlugInContainer->NotifyState(PIL_WIFI|PIL_AUDIO|PIL_VIDEO,PNM_ENDLINE,PIS_NOTIFYMASK,wCycles);
       RenderTimer((u16)(wCycles>>1));
       nTotCycles += wCycles;
       wCycles = 0;
   }
   if(bAutoIncreaseSpeed) ResetCpuSpeed();
   x1 = 0;
   if(dwDispCap & 0x8000){
       DisplayCapture();
       dwDispCap &= (u32)~0x8000;
       x1 = 1;
   }
   //          downLcd.SwapMainMemory();
   DISPCAPCNT &= ~0x80000000;
   if(nCores < 2){
       ((GraphicsPlugList *)pPlugInContainer->get_PlugInList(PIL_GRAPHICS))->Run(upLcd.get_Buffer(),downLcd.get_Buffer());
       if(!skip){
           upLcd.BitBlt();
           downLcd.BitBlt();
       }
   }
   else{
       skip_old = skip;
       SetEvent(hEvents[0]);
   }
   pPlugInContainer->NotifyState(PIL_VIDEO,PNM_ENTERVBLANK,PIS_NOTIFYMASK,MAKELPARAM(MAKEWORD(skip,x1),nSkipCount));
   /*		if(dwDispCap & 0x80000000){
       dwDispCap |= 0x8000;
       dwDispCap &= ~0x80000000;
   }
   if(dwDispSwap & 0x80000000)
       DisplaySwap();*/

   DISPSTAT &= ~6;
   DISPSTAT7 &= ~7;
   dwStatus &= ~6;
   VCOUNT7 = VCOUNT;
   DISPSTAT7 |= (u16)(DISPSTAT & 3);
   set_StopCycles(0);
   for(;wCycles < 50;){
#ifdef _DEBUG
       if(debugDlg.is_Active())
           debugDlg.WaitDebugger(bDouble);
#endif
       EXEC7
       EXEC9
   }
   StartDMA(7);
   StartDMA(1);
   DISPSTAT |= 1;
   dwStatus |= 1;
   if(DISPSTAT & 0x8)
       set_IRQ(1,TRUE,1);
   if(DISPSTAT7 & 0x8)
       set_IRQ(1,TRUE,2);
   if(DISPSTAT & 0x20){
       if(VCOUNT == ((DISPSTAT >> 8) | ((DISPSTAT & 0x80) << 1))){
           set_IRQ(4,TRUE,1);
           DISPSTAT |= 4;
           dwStatus |= 4;
       }
   }
   if(DISPSTAT7 & 0x20){
       if(VCOUNT == ((DISPSTAT7 >> 8) | ((DISPSTAT7 & 0x80) << 1))){
           set_IRQ(4,TRUE,2);
           DISPSTAT7 |= 4;
       }
   }
   for(;wCycles < 650;){
#ifdef _DEBUG
       if(debugDlg.is_Active())
           debugDlg.WaitDebugger(bDouble);
#endif
       EXEC7
       EXEC9
   }
   pPlugInContainer->NotifyState(PIL_VIDEO,0xC00,PIS_NOTIFYMASK,wCycles);
   set_StopCycles(0);
   for(;wCycles < LINE_CYCLES_VISIBLE*2;){
#ifdef _DEBUG
       if(debugDlg.is_Active())
           debugDlg.WaitDebugger(bDouble);
#endif
       EXEC7
       EXEC9
   }
   DISPSTAT |= 2;
   dwStatus |= 2;
   DISPSTAT7 &= ~3;
   DISPSTAT7 |= (u16)(DISPSTAT & 3);
   for(;wCycles < LINE_CYCLES*2;){
#ifdef _DEBUG
       if(debugDlg.is_Active())
           debugDlg.WaitDebugger(bDouble);
#endif
       EXEC7
       EXEC9
   }
   pPlugInContainer->NotifyState(PIL_WIFI|PIL_AUDIO|PIL_VIDEO,PNM_ENDLINE,PIS_NOTIFYMASK,wCycles);
   RenderTimer((u16)(wCycles>>1));
   nTotCycles += wCycles;
   wCycles = 0;
   if(bAutoIncreaseSpeed)
       IncreaseCpuSpeed(nIncreaseSpeed);
   //remainder line
   for((VCOUNT)++;VCOUNT < 263;(VCOUNT)++){
       DISPSTAT &= ~6;
       DISPSTAT7 &= ~7;
       dwStatus &= ~6;
       VCOUNT7 = VCOUNT;
       DISPSTAT7 |= (u16)(DISPSTAT & 3);
       if(DISPSTAT & 0x20){
           if(VCOUNT == ((DISPSTAT >> 8) | ((DISPSTAT & 0x80) << 1))){
               set_IRQ(4,TRUE,1);
               DISPSTAT |= 4;
               dwStatus |= 4;
           }
       }
       if(DISPSTAT7 & 0x20){
           if(VCOUNT == ((DISPSTAT7 >> 8) | ((DISPSTAT7 & 0x80) << 1))){
               set_IRQ(4,TRUE,2);
               DISPSTAT7 |= 4;
           }
       }
       set_StopCycles(0);
       for(;wCycles < LINE_CYCLES_VISIBLE*2;){
#ifdef _DEBUG
           if(debugDlg.is_Active())
               debugDlg.WaitDebugger(bDouble);
#endif
           EXEC7
           EXEC9
       }
       DISPSTAT |= 2;
       dwStatus |= 2;
       DISPSTAT7 &= ~3;
       DISPSTAT7 |= (u16)(DISPSTAT & 3);
       for(;wCycles < LINE_CYCLES*2;){
#ifdef _DEBUG
           if(debugDlg.is_Active())
               debugDlg.WaitDebugger(bDouble);
#endif
           EXEC7
           EXEC9
       }
       pPlugInContainer->NotifyState(PIL_WIFI|PIL_AUDIO|PIL_VIDEO,PNM_ENDLINE,PIS_NOTIFYMASK,wCycles);
       RenderTimer((u16)(wCycles>>1));
       nTotCycles += wCycles;
       wCycles = 0;
   }
   if(bAutoIncreaseSpeed || bFreeRunning){
       dw = 0;
       if(!(DISPSTAT & 0x10) && !(DISPSTAT7 & 0x10)){
           dw = arm9.get_Stopped();
           if(dw > arm7.get_Stopped())
               dw = arm7.get_Stopped();
           if(dw > LINE_CYCLES*2*180 || bFreeRunning)
               dw = 250;
           else if(dw > LINE_CYCLES*2*130)
               dw = 200;
           else if(dw > LINE_CYCLES*2*100)
               dw = 160;
           else if(dw > LINE_CYCLES*2*70)
               dw = 140;
           else if(dw > LINE_CYCLES*2*40)
               dw = 90;
           else if(dw > LINE_CYCLES*2*20)
               dw = 60;
           else
               dw = 0;
       }
       else if(bFreeRunning)
           dw = 50;
       if(nIncreaseSpeed != dw)
           IncreaseCpuSpeed(dw);
       arm9.set_Stopped();
       arm7.set_Stopped();
   }
   pPlugInContainer->NotifyState(PIL_WIFI|PIL_VIDEO|PIL_AUDIO,PNM_ENDFRAME,PIS_NOTIFYMASK,0);
   if(!bFreeRunning){
       if(++skip > nSkipCount)
           skip = 0;
       if(nIncreaseSpeed != 0)
           IncreaseCpuSpeed(0);
   }
   else{
       if(++skip > (ID_SKIP_FRAME9 - ID_SKIP_FRAMENONE))
           skip = 0;
   }
   if(nCores < 2){
       nTotCycles = (int)pPlugInContainer->get_PlugInList(PIL_INPUT)->get_ActivePlugIn();
       if(nTotCycles){
           dw = MAKELONG(((u16 *)(io_mem + 0x130))[0],((u16 *)(io_mem7 + 0x136))[0]);
           if(((LInputPlug *)nTotCycles)->Run(&dw)){
               ((u16 *)(io_mem + 0x130))[0] = ((u16 *)(io_mem7 + 0x130))[0] = (u16)dw;
               ((u16 *)(io_mem7 + 0x136))[0] &= ~0xB;
               ((u16 *)(io_mem7 + 0x136))[0] |= (u16)((dw >> 16) & 0xB);
               //if(!((dw >> 16) & 2))
                   //dw = dw;
               cheatsManager.write_hword(0x04000130,(u16)dw);
           }
       }
   }
   if(pActiveAudioPlugIn == NULL)
       return 0;
   return pActiveAudioPlugIn->Run(bFreeRunning);
}
//---------------------------------------------------------------------------
void LDS::set_SSE(BOOL bFlag)
{
   if(bFlag){
       if(CheckSSE())
           dwOptions |= NDSO_CPU_SUPPORT_SSE;
       else
           dwOptions &= ~NDSO_CPU_SUPPORT_SSE;
   }
   else
       dwOptions &= ~NDSO_CPU_SUPPORT_SSE;
	pPlugInContainer->NotifyState(-1,PNM_CHANGECONFIG,PIS_NOTIFYMASK,0);
}
//---------------------------------------------------------------------------
void LDS::OnInitMenuPopup(HMENU hMenu,UINT uPos,BOOL bSM)
{
	MENUITEMINFO mi={0};
   int i,i1;
   IMenu *menu,*menu1,*menu2,*menu3;

	menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
   menu2 = menu3 = NULL;
	if((menu1 = menu->GetSubMenu(0)) != NULL){
   	mi.cbSize = sizeof(MENUITEMINFO);
       mi.fMask = MIIM_DATA|MIIM_SUBMENU;
       menu1->GetMenuItemInfo(uPos,TRUE,&mi);
       if(mi.dwItemData == 0x3333 && mi.hSubMenu == hMenu){
       	langManager.OnInitMenu(hMenu);
           goto ex_OnInitMenuPopup;
		}
   	menu1->Release();
   }
   menu1 = menu->GetSubMenu(2);
   if(menu1 != NULL){
       menu2 = menu1->GetSubMenu(0);
       if(menu2 != NULL){
           menu3 = menu2->GetSubMenu(0);
           if(menu3->Handle() == hMenu){
               pPlugInContainer->get_PlugInList(PIL_VIDEO)->OnInitMenu(hMenu);
               goto ex_OnInitMenuPopup;
           }
       }
   }
   if(menu3 != NULL)
       menu3->Release();
   if(menu2 != NULL)
       menu2->Release();
   menu2 = menu3 = NULL;
   if(menu1 != NULL){
       menu2 = menu1->GetSubMenu(0);
       if(menu2 != NULL){
           menu3 = menu2->GetSubMenu(1);
           if(menu3->Handle() == hMenu){
               pPlugInContainer->get_PlugInList(PIL_GRAPHICS)->OnInitMenu(hMenu);
               goto ex_OnInitMenuPopup;
           }
       }
   }
   if(menu3 != NULL)
       menu3->Release();
   if(menu2 != NULL)
       menu2->Release();
   menu2 = menu3 = NULL;
   if(menu1 != NULL){
       menu2 = menu1->GetSubMenu(0);
       if(menu2 != NULL){
           menu3 = menu2->GetSubMenu(2);
           if(menu3->Handle() == hMenu){
               pPlugInContainer->get_PlugInList(PIL_AUDIO)->OnInitMenu(hMenu);
               goto ex_OnInitMenuPopup;
           }
       }
   }
   if(menu3 != NULL)
       menu3->Release();
   if(menu2 != NULL)
       menu2->Release();
   menu2 = menu3 = NULL;
   if(menu1 != NULL){
       menu2 = menu1->GetSubMenu(0);
       if(menu2 != NULL){
           menu3 = menu2->GetSubMenu(3);
           if(menu3->Handle() == hMenu){
               pPlugInContainer->get_PlugInList(PIL_WIFI)->OnInitMenu(hMenu);
               goto ex_OnInitMenuPopup;
           }
       }
   }
   if(menu3 != NULL)
       menu3->Release();
   if(menu2 != NULL)
       menu2->Release();
   menu2 = menu3 = NULL;
   if(menu1 != NULL){
       menu2 = menu1->GetSubMenu(0);
       if(menu2 != NULL){
           menu3 = menu2->GetSubMenu(4);
           if(menu3 != NULL && menu3->Handle() == hMenu){
               pPlugInContainer->get_PlugInList(PIL_INPUT)->OnInitMenu(hMenu);
               goto ex_OnInitMenuPopup;
           }
       }
   }
   if(menu1 != NULL){
       menu2 = menu1->GetSubMenu(0);
       if(menu2 != NULL){
           menu3 = menu2->GetSubMenu(5);
           if(menu3->Handle() == hMenu){
               pPlugInContainer->get_PlugInList(PIL_WEBCAM)->OnInitMenu(hMenu);
               goto ex_OnInitMenuPopup;
           }
       }
   }
   if(menu3 != NULL)
       menu3->Release();
   if(menu2 != NULL)
       menu2->Release();
   menu2 = menu3 = NULL;

   if(menu1 != NULL)
       menu1->Release();
   menu2 = menu3 = NULL;
   menu1 = menu->GetSubMenu(0);
   if(menu1 != NULL){
       menu2 = menu1->GetSubMenu(POS_MENU_RECENT);
       if(menu2 != NULL && menu2->Handle() == hMenu){
           i1 = RecentFiles.Count();
           i = menu2->GetMenuItemCount() - 1;
           for(;i>0;i--)
   	        menu2->DeleteMenu(i,MF_BYPOSITION);
           if(i1 != 0){
               mi.cbSize = sizeof(mi);
               mi.fMask = MIIM_TYPE;
               mi.fType = MFT_SEPARATOR;
  	            menu2->InsertMenuItem(1,TRUE,&mi);
               for(i=i1;i>0;i--){
                   mi.cbSize = sizeof(mi);
                   mi.fMask = MIIM_ID|MIIM_TYPE;
                   mi.fType = MFT_STRING;
                   mi.wID = ID_FILE_RECENT_RESET + i+1;
                   mi.dwTypeData = (char *)((LString *)RecentFiles.GetItem(i))->c_str();
                   menu2->InsertMenuItem(mi.wID,FALSE,&mi);
               }
           }
       }
       else{
           menu2 = menu1->GetSubMenu(POS_MENU_SAVESTATE);
           if(menu2 != NULL){
               if((menu3 = menu2->GetSubMenu(6))){
                   if(menu3->Handle() == hMenu)
                       SaveStateList.OnInitMenu(menu3,FALSE);
                   menu3->Release();
                   menu3 = NULL;
               }
               if((menu3 = menu2->GetSubMenu(7))){
                   if(menu3->Handle() == hMenu)
                       SaveStateList.OnInitMenu(menu3,TRUE);
                   menu3->Release();
                   menu3 = NULL;
               }
           }
       }
   }
ex_OnInitMenuPopup:
   if(menu3 != NULL)
       menu3->Release();
   if(menu2 != NULL)
       menu2->Release();
   if(menu1 != NULL)
       menu1->Release();
   if(menu != NULL)
       menu->Release();
}
//---------------------------------------------------------------------------
BOOL LDS::OpenRom(char *lpFileName)
{
#ifdef _DEBPRO
	char ext[] = {"NDS files (*.bin,*.nds,*.srl,*.pme,*.ds.gba,*.sc.nds,*.zip,*.elf,*.7z)\0*.bin;*.nds;*.srl;*.pme;*.ds.gba;*.zip;*.elf;*.7z;*.gba;\0All files (*.*)\0*.*\0\0\0\0\0"};
#else
	char ext[] = {"NDS files (*.bin,*.nds,*.pme,*.ds.gba,*.sc.nds,*.zip,*.elf,*.7z,*.gba)\0*.bin;*.nds;*.pme;*.ds.gba;*.zip;*.elf;*.7z;*.gba;\0All files (*.*)\0*.*\0\0\0\0\0"};
#endif
   LString s;
   LZ lz;
   LRegKey key;
	char *c;
	BOOL res,b7,b9,bb7,bb9;
   DWORD dwBoot,dw;
#ifdef _LICENSE
   GUID licGUID,proGUID;
#endif

	Reset();
   res = b7 = b9 = bb7 = bb9 = FALSE;
  	if((c = (char *)LocalAlloc(LPTR,5000)) == NULL)
      	goto ex_OpenRom;
   if(lpFileName == NULL || lstrlen(lpFileName) == 0){
   	*((int *)c) = 0;
		if(!lastPath.IsEmpty())
       	lstrcpy(c,lastPath.c_str());
       res = ShowOpenDialog(NULL,ext,upLcd.Handle(),c,5000,OFN_ALLOWMULTISELECT|0x00800000);
		if(!res || lstrlen(c) == 0)
       	goto ex_OpenRom_1;
       res = FALSE;
   }
   else
   	lstrcpy(c,lpFileName);
   if(Open(c))
   	res = TRUE;
ex_OpenRom:
#ifdef _LICENSE
   dw = get_LicenseGuid(&licGUID) ? 3 : 1;
   if(get_ProductGuid(&proGUID))
       dw |= 4;
   if(dw != 7 || !VerifyLicense(&licGUID,&proGUID))
       res = FALSE;
#endif
	if(!res){
       s = "Unable to read ";
       s += c;
		MessageBox(upLcd.Handle(),s.c_str(),"iDeaS Emulator",MB_OK|MB_ICONERROR);
   }
   else{
       if(dwOptions & NDSO_LOAD_BIOS){
           if(key.Open("Software\\iDeaS\\Settings")){
               s = key.ReadString("bios9","");
               key.Close();
           }
       }
       else
           s = "";
       if(!LoadBios(s.c_str(),9)){//"f:\\ideas\\bios\\arm9.bin"
           MessageBox(upLcd.Handle(),"Unable to read bios9.\r\nLoading default bios9...","iDeaS Emulator",MB_OK|MB_ICONERROR);
           LoadBios("",9);
       }
       else if(!s.IsEmpty())
           bb9 = TRUE;
       if(bb9 && (dwOptions & NDSO_LOAD_BIOS)){
           if(key.Open("Software\\iDeaS\\Settings")){
               s = key.ReadString("bios7","");
               key.Close();
           }
       }
       else
           s = "";
       if(!LoadBios(s.c_str(),(u8)(get_RomType() == 10 ? 8 : 7))){//"h:\\ideas\\bios\\arm7.bin"
           MessageBox(upLcd.Handle(),"Unable to read bios7.\r\nLoading default bios7...","iDeaS Emulator",MB_OK|MB_ICONERROR);
           LoadBios("",7);
       }
       else if(!s.IsEmpty())
           bb7 = TRUE;
       //    LoadBios("/mnt/win_e/ideas/vsh.c",13);

       if((dwOptions & NDSO_BOOT_FROM_BIOSES) && bb7 && bb9){
           arm7.InitPipeLine(0);
           arm9.InitPipeLine(0xFFFF0000);
           b9 = b7 = TRUE;
       }
       else if((dwBoot = LMMU::Load(c))){
           b9 = HIWORD(dwBoot);
           if(!(b7 = LOWORD(dwBoot)) && get_LoadDefARM7()){
               //CopyMemory(int_mem,arm7bin,sizeof(arm7bin));
               lz.ScompattaFile((char *)arm7bin,(int)sizeof(arm7bin),(char *)int_mem);
               arm7.InitPipeLine(0x37F8000);
               b7 = TRUE;
           }
           io_mem[0x300] = 1;
           io_mem7[0x300] = 1;
       }
       if(RecentFiles.Find(c) == NULL)
           RecentFiles.Add(c);
       lastFileName = c;
       lastPath = lastFileName.Path();
       bArm7 = b7;
       bArm9 = b9;
       bRom = TRUE;
       dw = get_RomType();
       if(dw == 10)
           dw = EMUM_GBA;                                      //2072498
       else if(dw == 4)
           dw = EMUM_DSI;
       else
           dw = EMUM_NDS;
       set_EmuMode(dw);
       EEPROM_Load();
       SaveStateList.OnLoadRom();
       DisplaySwap();
       fat.Init(c);
       cheatsManager.OnLoadRom();
       pPlugInContainer->NotifyState(-1,PNM_OPENFILE,PIS_NOTIFYMASK,(LPARAM)c);
       if(get_EmuMode() != EMUM_GBA){
       	if(get_ExpansionRam() == 3){
               *((int *)c) = 0;
               if(!lastPath.IsEmpty())
                   lstrcpy(c,lastPath.c_str());
               res = ShowOpenDialog(NULL,"GBA files (*.gba,*.zip,*.7z)\0*.gba;*.zip;*.7z;\0\0\0\0\0\0\0",upLcd.Handle(),c,5000,OFN_ALLOWMULTISELECT|0x00800000);
				if(res){
                   res = FALSE;
                   dwBoot = arm7.r_gpreg(-1);
                   if(Open(c,1) && LMMU::Load(c,1)){
                       res = TRUE;
                       arm7.InitPipeLine(dwBoot);
                   }
                   if(!res){
                       s = "Unable to read ";
                       s += c;
                       MessageBox(upLcd.Handle(),s.c_str(),"iDeaS Emulator",MB_OK|MB_ICONERROR);
                   }
               }
           }
       }
       //       OnApplyIPSPatch();
   }
ex_OpenRom_1:
   upLcd.UpdateToolBarState(bRom);
   if(c != NULL)
   	LocalFree(c);
#ifdef _DEBUG
	debugDlg.Reset_Debugger();
#endif
   return bRom;
}
//---------------------------------------------------------------------------
BOOL LDS::MakeScreenShot(int mode)
{
	LString s;
	HIMAGELIST hi;
   HBITMAP bit;
   SIZE sz;
   RECT rc;
   int res;

   if(!bRom || (!bRun && !bPause))
       return FALSE;
   s.Length(MAX_PATH+1);
   res = FALSE;
   bit = NULL;
#ifdef __WIN32__
   HDC dc;

   dc = ::CreateCompatibleDC(NULL);
   if(dc == NULL)
       return FALSE;
   upLcd.get_WorkArea(rc,sz);
   if(mode == -1){
       BuildFileName(PT_SCREENSHOT,s.c_str(),MAX_PATH,"bmp",(int)GetTickCount());
       bit = ::CreateBitmap(rc.right-rc.left,(rc.bottom-rc.top)*2,GetDeviceCaps(dc,PLANES),GetDeviceCaps(dc,BITSPIXEL),NULL);
       if(bit == NULL)
           goto ex_MakeScreenShot;
       ::SelectObject(dc,bit);
       ::BitBlt(dc,0,0,rc.right-rc.left,rc.bottom-rc.top,upLcd.I_DC(),0,0,SRCCOPY);
       downLcd.get_WorkArea(rc,sz);
       ::BitBlt(dc,0,rc.bottom-rc.top,rc.right-rc.left,rc.bottom-rc.top,downLcd.I_DC(),0,0,SRCCOPY);
       //::DrawIconEx(dc,256-32,384-32,::LoadIcon(hInst,MAKEINTRESOURCE(2)),32,32,0,0,DI_NORMAL);
       //   hi = ImageList_LoadImage(hInst,MAKEINTRESOURCE(2),16,1,CLR_NONE,IMAGE_ICON,LR_DEFAULTCOLOR);
       hi = ImageList_Create(32,32,ILC_MASK|ILC_COLOR16,1,0);
       if(hi != NULL){
           ImageList_AddIcon(hi,::LoadIcon(hInst,MAKEINTRESOURCE(2)));
           ImageList_DrawEx(hi,0,dc,rc.right-rc.left-32,(rc.bottom-rc.top) * 2 - 32,32,32,CLR_NONE,CLR_NONE,ILD_BLEND25);
           ImageList_Destroy(hi);
       }
       else
           ::DrawIconEx(dc,256-32,384-32,::LoadIcon(hInst,MAKEINTRESOURCE(2)),32,32,0,0,DI_NORMAL);
       //   s.BuildFileName((char *)lastFileName.c_str(),"bmp",(int)GetTickCount());
       SaveBitmap(dc,bit,s.c_str());
       res = TRUE;
       goto ex_MakeScreenShot;
   }
   bit = ::CreateBitmap(rc.right-rc.left,(rc.bottom-rc.top),GetDeviceCaps(dc,PLANES),GetDeviceCaps(dc,BITSPIXEL),NULL);
   if(bit == NULL)
       goto ex_MakeScreenShot;
   ::SelectObject(dc,bit);
   if(mode & 1){
       BuildFileName(PT_SCREENSHOT,s.c_str(),MAX_PATH,"0.bmp",(int)GetTickCount());
       ::BitBlt(dc,0,0,rc.right-rc.left,rc.bottom-rc.top,upLcd.I_DC(),0,0,SRCCOPY);
       SaveBitmap(dc,bit,s.c_str());
       res = TRUE;
   }
   if(mode & 2){
       BuildFileName(PT_SCREENSHOT,s.c_str(),MAX_PATH,"1.bmp",(int)GetTickCount());
       ::BitBlt(dc,0,0,rc.right-rc.left,rc.bottom-rc.top,downLcd.I_DC(),0,0,SRCCOPY);
       SaveBitmap(dc,bit,s.c_str());
       res = TRUE;
   }
ex_MakeScreenShot:
   if(dc != NULL)
       ::DeleteDC(dc);
#else
	int w,h;
	GdkPixmap *map;
	HBITMAP bit1;

   BuildFileName(PT_SCREENSHOT,s.c_str(),MAX_PATH,"bmp",(int)GetTickCount());
	w = gdk_pixbuf_get_width(upLcd.Bitmap());
	h = gdk_pixbuf_get_height(upLcd.Bitmap());
	map = gdk_pixmap_new(NULL,w,h*2,24);
	gdk_pixbuf_render_to_drawable(upLcd.Bitmap(),map,NULL,0,0,0,0,w,h,GDK_RGB_DITHER_NONE,0,0);
	gdk_pixbuf_render_to_drawable(downLcd.Bitmap(),map,NULL,0,0,0,h,w,h,GDK_RGB_DITHER_NONE,0,0);
	hi = ImageList_Create(32,32,ILC_MASK|ILC_COLOR16,1,0);
	if(hi != NULL){
       ImageList_AddIcon(hi,::LoadIcon(hInst,MAKEINTRESOURCE(2),32,32,0));
   	ImageList_DrawEx(hi,0,(HDC)map,256-32,384-32,32,32,CLR_NONE,CLR_NONE,ILD_BLEND25);
   	ImageList_Destroy(hi);
   }
	bit1 = gdk_pixbuf_get_from_drawable(NULL,map,NULL,0,0,0,0,w,h*2);
	bit = gdk_pixbuf_flip(bit1,false);
	SaveBitmap(NULL,bit,s.c_str());
	DeleteObject(bit1);
	DeleteObject(map);
   res = TRUE;
#endif
   if(bit != NULL)
	    DeleteObject(bit);
   return res;
}
//---------------------------------------------------------------------------
void LDS::OnMenuSelect(WORD wID)
{
   MSGBOXPARAMS mp={0};
	IMenu *menu;
   LString s;
	MENUITEMINFO mii;

   if(wID > 0 && wID < 100){
       memset(&mii,0,sizeof(MENUITEMINFO));
       mii.cbSize = sizeof(MENUITEMINFO);
       mii.fMask = MIIM_DATA;
		menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
       if(menu != NULL){
			menu->GetMenuItemInfo(wID,FALSE,&mii);
   		langManager.OnEnable(mii.dwItemData);
       	menu->Release();
       }
   }
	if(wID > ID_FILE_RECENT_RESET && wID < ID_FILE_RECENT_RESET + 12){
       OpenRom((char *)((LString *)RecentFiles.GetItem(wID - ID_FILE_RECENT_RESET-1))->c_str());
   	return;
   }
   if(wID >= ID_PLUGIN_GRAPHICS_START && wID <= ID_PLUGIN_GRAPHICS_END){
		pPlugInContainer->get_PlugInList(PIL_VIDEO)->OnEnablePlug(wID);
   	return;
   }
   if(wID >= ID_PLUGIN_AUDIO_START && wID <= ID_PLUGIN_AUDIO_END){
		pPlugInContainer->get_PlugInList(PIL_AUDIO)->OnEnablePlug(wID);
   	return;
   }
   if(wID >= ID_PLUGIN_RENDER_START && wID <= ID_PLUGIN_RENDER_END){
		pPlugInContainer->get_PlugInList(PIL_GRAPHICS)->OnEnablePlug(wID);
   	return;
   }
   if(wID >= ID_PLUGIN_WIFI_START && wID <= ID_PLUGIN_WIFI_END){
		pPlugInContainer->get_PlugInList(PIL_WIFI)->OnEnablePlug(wID);
   	return;
   }
   if(wID >= ID_PLUGIN_INPUT_START && wID <= ID_PLUGIN_INPUT_END){
		pPlugInContainer->get_PlugInList(PIL_INPUT)->OnEnablePlug(wID);
       return;
   }
   if(wID >= ID_FILE_STATE_START && wID <= ID_FILE_STATE_END){
       wID -= (WORD)ID_FILE_STATE_START;
       if(wID < 15){
//       	Reset(FALSE);
           SaveStateList.set_LoadIndex(wID);
           SendMessage(ds.Handle(),WM_COMMAND,ID_FILE_RESET,0);
//           SaveStateList.Load(wID);
//           cheatsManager.OnLoadRom();
       }
       else
           SaveStateList.Save(wID - 15);
       return;
   }
	if(wID >= ID_SRAM_START && wID <= ID_EEPROM_END){
   	SetGamePackID((WORD)wID);
       return;
   }
	switch(wID){
		case ID_ROM_INFO:
			OnShowRomInfo();
       break;
       case ID_EEPROM_AUTOSAVE:
           dwOptions ^= NDSO_BACKUP_AUTO_SAVE;
           menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
		    menu->CheckMenuItem(wID,MF_BYCOMMAND|((dwOptions & NDSO_BACKUP_AUTO_SAVE) ? MF_CHECKED : MF_UNCHECKED));
           menu->Release();
           if(dwOptions & NDSO_BACKUP_AUTO_SAVE)
               EEPROM_AutoSave(TRUE);
       break;
       case ID_SSE_EXTENSION:
           set_SSE((dwOptions & NDSO_CPU_SUPPORT_SSE) ? FALSE : TRUE);
           menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
		    menu->CheckMenuItem(wID,MF_BYCOMMAND|(dwOptions & NDSO_CPU_SUPPORT_SSE ? MF_CHECKED : MF_UNCHECKED));
           menu->Release();
       break;
   	case ID_FILE_CHEAT_LIST:
           cheatsManager.Show(Handle());
       break;
       case ID_FILE_CHEAT_LOAD:
       	cheatsManager.Load();
       break;
       case ID_FILE_CHEAT_AUTOLOAD:
       	cheatsManager.AutoLoad(!cheatsManager.is_AutoLoad());
			menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
		    menu->CheckMenuItem(wID,MF_BYCOMMAND|cheatsManager.is_AutoLoad() ? MF_CHECKED : MF_UNCHECKED);
           menu->Release();
       break;
       case ID_FILE_CHEAT_DISABLELIST:
       	cheatsManager.Enable(!cheatsManager.is_Enable());
			menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
		    menu->CheckMenuItem(wID,MF_BYCOMMAND|cheatsManager.is_Enable() ? MF_UNCHECKED : MF_CHECKED);
           menu->Release();
       break;
       case ID_FILE_STATE_AUTOLOADONRESET:
           SaveStateList.set_LoadOnReset(!SaveStateList.get_LoadOnReset());
           menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
		    menu->CheckMenuItem(ID_FILE_STATE_AUTOLOADONRESET,MF_BYCOMMAND|SaveStateList.get_LoadOnReset() ? MF_CHECKED : MF_UNCHECKED);
           menu->Release();
       break;
       case ID_FILE_STATE_AUTOLOADMOSTRCNT:
           SaveStateList.set_LoadRecent(!SaveStateList.get_LoadRecent());
           menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
		    menu->CheckMenuItem(ID_FILE_STATE_AUTOLOADMOSTRCNT,MF_BYCOMMAND|SaveStateList.get_LoadRecent() ? MF_CHECKED : MF_UNCHECKED);
           menu->Release();
       break;
       case ID_FILE_STATE_RESET:
           SaveStateList.Reset();
       break;
       case ID_FILE_STATE_SAVE:
           SaveStateList.Save(-1);
       break;
       case ID_WINDOW_SIZE0:
           downLcd.set_Scale(0);
           upLcd.set_Scale(0);
           menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
		    menu->CheckMenuItem(ID_WINDOW_SIZE0,MF_BYCOMMAND|MF_CHECKED);
           menu->CheckMenuItem(ID_WINDOW_SIZE15,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_WINDOW_SIZE20,MF_BYCOMMAND|MF_UNCHECKED);
           menu->Release();
       break;
       case ID_WINDOW_SIZE15:
           downLcd.set_Scale(1.5);
           upLcd.set_Scale(1.5);
           menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
		    menu->CheckMenuItem(ID_WINDOW_SIZE0,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_WINDOW_SIZE15,MF_BYCOMMAND|MF_CHECKED);
           menu->CheckMenuItem(ID_WINDOW_SIZE20,MF_BYCOMMAND|MF_UNCHECKED);
           menu->Release();
       break;
       case ID_WINDOW_SIZE20:
           downLcd.set_Scale(2.0);
           upLcd.set_Scale(2.0);
           menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
		    menu->CheckMenuItem(ID_WINDOW_SIZE0,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_WINDOW_SIZE20,MF_BYCOMMAND|MF_CHECKED);
           menu->CheckMenuItem(ID_WINDOW_SIZE15,MF_BYCOMMAND|MF_UNCHECKED);
           menu->Release();
       break;
       case ID_WINDOW_ROTATE0:
       case ID_WINDOW_ROTATE90:
       case ID_WINDOW_ROTATE180:
       case ID_WINDOW_ROTATE270:
           downLcd.Rotate(wID);
           upLcd.Rotate(wID);
           menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
		    menu->CheckMenuItem(ID_WINDOW_ROTATE0,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_WINDOW_ROTATE90,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_WINDOW_ROTATE180,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_WINDOW_ROTATE270,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(wID,MF_BYCOMMAND|MF_CHECKED);
           menu->Release();
       break;
       case ID_LAYERS_ALL:
           upLcd.SendMessage(WM_COMMAND,MAKEWPARAM(ID_LAYER_UP_ALL,0),0);
           upLcd.SendMessage(WM_COMMAND,MAKEWPARAM(ID_LAYER_DOWN_ALL,0),0);
       break;
       case ID_KEY_CONFIG:
           OnKeyConfig();
       break;
       case ID_GUITARGRIPKEY_CONFIG:
       	OnGuitarGripKeyConfig();
       break;
       case ID_SKIP_FRAMENONE:
       case ID_SKIP_FRAME1:
       case ID_SKIP_FRAME2:
       case ID_SKIP_FRAME3:
       case ID_SKIP_FRAME4:
       case ID_SKIP_FRAME5:
       case ID_SKIP_FRAME6:
       case ID_SKIP_FRAME7:
       case ID_SKIP_FRAME8:
       case ID_SKIP_FRAME9:
           nSkipCount = wID - ID_SKIP_FRAMENONE;
          	dwOptions &= ~NDSO_FRAMES_AUTO_SKIP;
			menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
			if(menu != NULL){
                menu->CheckMenuItem(ID_SKIP_FRAMENONE,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME1,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME2,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME3,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME4,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME5,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME6,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME7,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME8,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME9,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAMEAUTO,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(wID,MF_BYCOMMAND|MF_CHECKED);
                menu->Release();
			}
       break;
       case ID_SKIP_FRAMEAUTO:
           menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
			if(menu != NULL){
                if(!menu->IsCheckedMenuItem(ID_SKIP_FRAMEAUTO))
                    dwOptions |= NDSO_FRAMES_AUTO_SKIP;
                else
                    dwOptions &= ~NDSO_FRAMES_AUTO_SKIP;
                menu->CheckMenuItem(ID_SKIP_FRAMENONE,MF_BYCOMMAND|(dwOptions & NDSO_FRAMES_AUTO_SKIP ? MF_UNCHECKED : MF_CHECKED));
                menu->CheckMenuItem(ID_SKIP_FRAME1,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME2,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME3,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME4,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME5,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME6,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME7,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME8,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAME9,MF_BYCOMMAND|MF_UNCHECKED);
                menu->CheckMenuItem(ID_SKIP_FRAMEAUTO,MF_BYCOMMAND|(dwOptions & NDSO_FRAMES_AUTO_SKIP ? MF_CHECKED : MF_UNCHECKED));
                menu->Release();
			}
       break;
       case ID_SKIP_FRAME_CAPTURE:
           menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
           if(menu != NULL){
                dwOptions ^= NDSO_FRAMES_SKIP_CAPTURE;
                menu->CheckMenuItem(ID_SKIP_FRAME_CAPTURE,MF_BYCOMMAND|(dwOptions & NDSO_FRAMES_SKIP_CAPTURE ? MF_CHECKED : MF_UNCHECKED));
                menu->Release();
			}
       break;
       case ID_USE_DMA_LATENCY:
       	menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
           if(menu != NULL){
               dwOptions ^= NDSO_USE_DMA_LATENCY;
               menu->CheckMenuItem(ID_USE_DMA_LATENCY,MF_BYCOMMAND|((dwOptions & NDSO_USE_DMA_LATENCY) ? MF_CHECKED : MF_UNCHECKED));
               menu->Release();
           }
       break;
       case ID_SKIP_OPTIMIZE_CPU:
       	menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
           if(menu != NULL){
               dwOptions ^= NDSO_OPTIMIZE_CPU;
               menu->CheckMenuItem(ID_SKIP_OPTIMIZE_CPU,MF_BYCOMMAND|((dwOptions & NDSO_OPTIMIZE_CPU) ? MF_CHECKED : MF_UNCHECKED));
               menu->Release();
           }
       break;
       case ID_SKIP_OPTIMIZE_LOOPS:
       	menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
           dwOptions ^= NDSO_OPTIMIZE_LOOP;
			menu->CheckMenuItem(ID_SKIP_OPTIMIZE_LOOPS,MF_BYCOMMAND|((dwOptions & NDSO_OPTIMIZE_LOOP) ? MF_CHECKED : MF_UNCHECKED));
			menu->Release();
       break;
       case ID_SKIP_CPU25:
       case ID_SKIP_CPU50:
       case ID_SKIP_CPU75:
       case ID_SKIP_CPU100:
       case ID_SKIP_CPU125:
       case ID_SKIP_CPU150:
           bAutoIncreaseSpeed = 0;
           menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
			menu->CheckMenuItem(ID_SKIP_CPU25,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_SKIP_CPU50,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_SKIP_CPU75,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_SKIP_CPU100,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_SKIP_CPU125,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_SKIP_CPU150,MF_BYCOMMAND|MF_UNCHECKED);
  			menu->CheckMenuItem(ID_SKIP_CPUNONE,MF_BYCOMMAND|MF_UNCHECKED);
	        menu->CheckMenuItem(ID_SKIP_AUTO,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(wID,MF_BYCOMMAND|MF_CHECKED);
		    menu->Release();
           IncreaseCpuSpeed((wID - ID_SKIP_CPU25 + 1) * 25);
       break;
       case ID_SKIP_AUTO:
           bAutoIncreaseSpeed = !bAutoIncreaseSpeed;
           menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
			menu->CheckMenuItem(ID_SKIP_CPU25,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_SKIP_CPU50,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_SKIP_CPU75,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_SKIP_CPU100,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_SKIP_CPU125,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_SKIP_CPU150,MF_BYCOMMAND|MF_UNCHECKED);
           if(bAutoIncreaseSpeed){
			    menu->CheckMenuItem(ID_SKIP_AUTO,MF_BYCOMMAND|MF_CHECKED);
  			    menu->CheckMenuItem(ID_SKIP_CPUNONE,MF_BYCOMMAND|MF_UNCHECKED);
           }
           else{
			    menu->CheckMenuItem(ID_SKIP_AUTO,MF_BYCOMMAND|MF_UNCHECKED);
  			    menu->CheckMenuItem(ID_SKIP_CPUNONE,MF_BYCOMMAND|MF_CHECKED);
           }
		    menu->Release();
           IncreaseCpuSpeed(0);
       break;
       case ID_SKIP_CPUNONE:
           bAutoIncreaseSpeed = 0;
           menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
           menu->CheckMenuItem(ID_SKIP_AUTO,MF_BYCOMMAND|MF_UNCHECKED);
			menu->CheckMenuItem(ID_SKIP_CPU25,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_SKIP_CPU50,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_SKIP_CPU75,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_SKIP_CPU100,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_SKIP_CPUNONE,MF_BYCOMMAND|MF_CHECKED);
           menu->CheckMenuItem(ID_SKIP_CPU125,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_SKIP_CPU150,MF_BYCOMMAND|MF_UNCHECKED);
           IncreaseCpuSpeed(0);
       break;
       case ID_EEPROM_M3SAVE:
           s.Capacity(5000);
           ShowOpenDialog(NULL,"M3 (*.dat)\0*.dat\0\0\0\0\0",upLcd.Handle(),s.c_str(),5000,0);
           EEPROM_Import(s.c_str(),wID);
       break;
       case ID_EEPROM_SCFSAVE:
       case ID_EEPROM_SCONESAVE:
           s.Capacity(5000);
           ShowOpenDialog(NULL,"Supercard CF (*.sav)\0*.sav\0\0\0\0\0",upLcd.Handle(),s.c_str(),5000,0);
           EEPROM_Import(s.c_str(),wID);
       break;
       case ID_TRIGGER_HING:
			*((u16 *)&io_mem7[0x136]) ^= 0x80;
           dwOptions ^= NDSO_HINGE;
			upLcd.UpdateToolBarState(bRom,bRun,(dwOptions & NDSO_HINGE ? TRUE : FALSE));
//           set_IRQ(128,FALSE,2);
       break;
       case ID_EEPROM_4KBIT:
       case ID_EEPROM_64KBIT:
       case ID_FLASH_2MBIT:
       case ID_EEPROM_512KBIT:
       case ID_FLASH_4MBIT:
       case ID_FLASH_8MBIT:
       case ID_EEPROM_AUTO:
       case ID_EEPROM_NONE:
           OnSelectEEPROM(wID);
           menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
			menu->CheckMenuItem(ID_EEPROM_4KBIT,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_EEPROM_64KBIT,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_EEPROM_512KBIT,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_FLASH_2MBIT,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_FLASH_4MBIT,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_FLASH_8MBIT,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_EEPROM_AUTO,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(ID_EEPROM_NONE,MF_BYCOMMAND|MF_UNCHECKED);
           menu->CheckMenuItem(wID,MF_BYCOMMAND|MF_CHECKED);
		    menu->Release();
       break;
		case ID_FILE_OPEN_BIOS:
       	dwOptions ^= NDSO_LOAD_BIOS;
		    menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
			menu->EnableMenuItem(ID_FILE_OPEN_BIOS,MF_BYCOMMAND|((dwOptions & NDSO_LOAD_BIOS) ? MF_ENABLED : MF_GRAYED));
		    menu->Release();
       break;
   	case ID_FILE_START:
       	if(bRom){
               bRun = TRUE;
               bPause = FALSE;
           }
			upLcd.UpdateToolBarState(bRom,bRun,(dwOptions & NDSO_HINGE ? TRUE : FALSE));
			pPlugInContainer->NotifyState(-1,PIS_RUN,PIS_RUNMASK);
       break;
       case ID_FILE_PAUSE:
       	bRun = FALSE;
           bPause = TRUE;
			pPlugInContainer->NotifyState(-1,PIS_PAUSE,PIS_RUNMASK);
			upLcd.UpdateToolBarState(bRom,bRun,(dwOptions & NDSO_HINGE ? TRUE : FALSE));
       break;
       case ID_FILE_STOP:
       	bRun = bRom = FALSE;
			pPlugInContainer->NotifyState(-1,0,PIS_RUNMASK);
			upLcd.UpdateToolBarState(bRom,bRun,(dwOptions & NDSO_HINGE ? TRUE : FALSE));
       break;
		case ID_FILE_RECENT_RESET:
       	RecentFiles.Clear();
       break;
       case ID_FILE_PROPERTIES:
			pPlugInContainer->NotifyState(-1,PIS_PAUSE,PIS_RUNMASK);
       	OnShowProperties();
			pPlugInContainer->NotifyState(-1,0,PIS_RUNMASK);
       break;
       case ID_FILE_SCREENSHOTS:
           MakeScreenShot();
       break;
       case ID_FILE_SCREENSHOTS_TOP:
       case ID_FILE_SCREENSHOTS_BOTTOM:
           MakeScreenShot(1 << (wID - ID_FILE_SCREENSHOTS_TOP));
       break;
       case ID_FILE_SCREENSHOTS_BOTH:
           MakeScreenShot(3);
       break;
#ifdef _LICENSE
       case ID_INFO_LICENSE:
           InsertLicense();
       break;
#endif
		case ID_INFO_SITE:
       	ShellExecute(ds.Handle(),"open","http://www.ideasemu.biz",NULL,NULL,SW_MAXIMIZE);
       break;
		case ID_INFO_INFO:
           mp.cbSize = sizeof(MSGBOXPARAMS);
           mp.hwndOwner = upLcd.Handle();
           mp.hInstance = hInst;
           mp.lpszText = new char[100];
			lstrcpy((char *)mp.lpszText,"iDeaS Emulator 1.0.4.0 Multicores\r\n");
           lstrcat((char *)mp.lpszText,"\tby Actarus");
           mp.lpszCaption = "iDeaS Emulator";
           mp.lpszIcon = MAKEINTRESOURCE(2);
           mp.dwStyle = MB_USERICON|MB_OK|MB_APPLMODAL;
  			pPlugInContainer->NotifyState(-1,PIS_PAUSE,PIS_RUNMASK);
      		::MessageBoxIndirect(&mp);
			delete []mp.lpszText;
			pPlugInContainer->NotifyState(-1,PIS_RUN,PIS_RUNMASK);
       break;
   	case ID_FILE_EXIT:
			PostQuitMessage();
       break;
       case ID_FILE_OPEN:
			OpenRom(NULL);
       break;
       case ID_FILE_RESET:
       	OpenRom((char *)get_FileName());
           upLcd.SendMessage(WM_COMMAND,MAKEWPARAM(ID_FILE_START,0),0);
       break;
       case ID_DEBUG_DEBUGGER:
#ifdef _DEBUG
       	debugDlg.Modeless(this);
#endif
       break;
   }
}
//---------------------------------------------------------------------------
BOOL LDS::OnDlgProc22(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   int i;
	char s[MAX_PATH+10];
   BROWSEINFO bi;
   LPITEMIDLIST id;

	switch(uMsg){
       case WM_INITDIALOG:
           if(!savegamePath.IsEmpty())
				SetDlgItemText(hwnd,IDC_LABEL1,savegamePath.c_str());
           if(!savestatePath.IsEmpty())
				SetDlgItemText(hwnd,IDC_LABEL2,savestatePath.c_str());
       	if(!screenshotPath.IsEmpty())
           	SetDlgItemText(hwnd,IDC_LABEL3,screenshotPath.c_str());
       	if(!cheatPath.IsEmpty())
           	SetDlgItemText(hwnd,IDC_LABEL4,cheatPath.c_str());
           SendDlgItemMessage(hwnd,IDC_CHECK1,CB_ADDSTRING,0,(LPARAM)"None");
           SendDlgItemMessage(hwnd,IDC_CHECK1,CB_ADDSTRING,0,(LPARAM)"Temp");
           SendDlgItemMessage(hwnd,IDC_CHECK1,CB_ADDSTRING,0,(LPARAM)"Auto");
           SendDlgItemMessage(hwnd,IDC_CHECK1,CB_SETCURSEL,nUseTempFile,0);
       break;
       case WM_NOTIFY:
           if(((LPNMHDR)lParam)->code == PSN_APPLY){
				savegamePath.Length(MAX_PATH+10);
               GetDlgItemText(hwnd,IDC_LABEL1,savegamePath.c_str(),MAX_PATH+1);
   			savestatePath.Length(MAX_PATH+10);
               GetDlgItemText(hwnd,IDC_LABEL2,savestatePath.c_str(),MAX_PATH+1);
   			screenshotPath.Length(MAX_PATH+10);
               GetDlgItemText(hwnd,IDC_LABEL3,screenshotPath.c_str(),MAX_PATH+1);
   			cheatPath.Length(MAX_PATH+10);
               GetDlgItemText(hwnd,IDC_LABEL4,cheatPath.c_str(),MAX_PATH+1);
               nUseTempFile = SendDlgItemMessage(hwnd,IDC_CHECK1,CB_GETCURSEL,0,0);
           }
       break;
       case WM_CTLCOLORSTATIC:
       case WM_CTLCOLOREDIT:
#ifdef __WIN32__
       	switch(GetDlgCtrlID((HWND)lParam)){
           	case IDC_LABEL1:
               case IDC_LABEL2:
               case IDC_LABEL3:
               case IDC_LABEL4:
       			SetTextColor((HDC)wParam,::GetSysColor(COLOR_WINDOWTEXT));
	           		return (BOOL)::GetSysColorBrush(COLOR_WINDOW);
       	}
#endif
       break;
       case WM_COMMAND:
       	switch(HIWORD(wParam)){
           	case BN_CLICKED:
               	switch(LOWORD(wParam)){
                   	case IDC_BUTTON1:
                       	lstrcpy(s,savegamePath.c_str());
                           i = IDC_LABEL1;
                       break;
                       case IDC_BUTTON2:
                       	i = IDC_LABEL2;
							lstrcpy(s,savestatePath.c_str());
                       break;
                       case IDC_BUTTON3:
                       	lstrcpy(s,screenshotPath.c_str());
                           i = IDC_LABEL3;
                       break;
                       case IDC_BUTTON4:
                       	lstrcpy(s,cheatPath.c_str());
                           i = IDC_LABEL4;
                       break;
                       default:
                       	i = 0;
                       break;
                   }
                   if(i){
                   	memset(&bi,0,sizeof(BROWSEINFO));
   					bi.ulFlags = BIF_RETURNONLYFSDIRS;
   					bi.hwndOwner = hwnd;
   					bi.pszDisplayName = s;
   					if((id = SHBrowseForFolder(&bi)) != NULL){
                           if(SHGetPathFromIDList(id,s)){
                               if(s[lstrlen(s)-1] != DPC_PATH)
                                   lstrcat(s,DPS_PATH);
                           }
                       }
                       SetDlgItemText(hwnd,i,s);
                   }
               break;
           }
       break;
   }
	return FALSE;
}
//---------------------------------------------------------------------------
BOOL CALLBACK LDS::DlgProc16(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   PlugIn *p;
   SETPROPPLUGIN p1;
   int i,i1;
   PlugInList *pPlugList;
	char s[MAX_PATH+10];

	switch(uMsg){
       case WM_INITDIALOG:
         	SendDlgItemMessage(hwnd,IDC_CHECK6,BM_SETCHECK,ds.get_Optimize() & NDSO_LOAD_FIRMWARE ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK1,BM_SETCHECK,ds.get_LoadDefARM7() ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK5,BM_SETCHECK,ds.get_Optimize() & NDSO_LOAD_BIOS ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK8,BM_SETCHECK,ds.get_Optimize() & NDSO_BOOT_FROM_BIOSES ? BST_CHECKED : BST_UNCHECKED,0);
			SendDlgItemMessage(hwnd,IDC_TOUCH_SETTINGS,CB_ADDSTRING,0,(LPARAM)"Far Corners");
			SendDlgItemMessage(hwnd,IDC_TOUCH_SETTINGS,CB_ADDSTRING,0,(LPARAM)"Normal");
           SendDlgItemMessage(hwnd,IDC_TOUCH_SETTINGS,CB_ADDSTRING,0,(LPARAM)"Around 16x16");
           SendDlgItemMessage(hwnd,IDC_TOUCH_SETTINGS,CB_SETCURSEL,(WPARAM)ds.get_TouchScreenSettings(),0);
  			SendDlgItemMessage(hwnd,IDC_LANGUAGE_SETTINGS,CB_ADDSTRING,0,(LPARAM)"Japanese");
			SendDlgItemMessage(hwnd,IDC_LANGUAGE_SETTINGS,CB_ADDSTRING,0,(LPARAM)"English");
           SendDlgItemMessage(hwnd,IDC_LANGUAGE_SETTINGS,CB_ADDSTRING,0,(LPARAM)"French");
           SendDlgItemMessage(hwnd,IDC_LANGUAGE_SETTINGS,CB_ADDSTRING,0,(LPARAM)"German");
           SendDlgItemMessage(hwnd,IDC_LANGUAGE_SETTINGS,CB_ADDSTRING,0,(LPARAM)"Italian");
           SendDlgItemMessage(hwnd,IDC_LANGUAGE_SETTINGS,CB_ADDSTRING,0,(LPARAM)"Spanish");
           SendDlgItemMessage(hwnd,IDC_LANGUAGE_SETTINGS,CB_SETCURSEL,(WPARAM)FirmWare_get_LanguageSettings(),0);
           SendDlgItemMessage(hwnd,IDC_DLDI_SETTINGS,CB_ADDSTRING,0,(LPARAM)"None");
           {
               SYSTEM_INFO si;

               GetSystemInfo(&si);
               ::EnableWindow(GetDlgItem(hwnd,IDC_EDIT2),si.dwNumberOfProcessors > 1 ? TRUE : FALSE);
               wsprintf(s,"%d",ds.get_MultiCore());
               SetDlgItemText(hwnd,IDC_EDIT2,s);
           }
			pPlugList = pPlugInContainer->get_PlugInList(PIL_DLDI);
   		for(i=0;i<(signed)pPlugList->Count();i++){
       		p = (PlugIn *)pPlugList->GetItem(i+1);
       		p->InitGetPlugInInfo(&p1.info);
       		p1.info.pszText = (LPWSTR)s;
       		p1.info.cchTextMax = MAX_PATH;
       		p->GetInfo(&p1.info,TRUE);
				SendDlgItemMessage(hwnd,IDC_DLDI_SETTINGS,CB_ADDSTRING,0,(LPARAM)s);
   		}
           if(ds.get_UseFATxEFS()){
				i = 0;
               EnableWindow(GetDlgItem(hwnd,IDC_DLDI_SETTINGS),FALSE);
               SendDlgItemMessage(hwnd,IDC_CHECK7,BM_SETCHECK,BST_CHECKED,0);
           }
           else{
                SendDlgItemMessage(hwnd,IDC_CHECK7,BM_SETCHECK,BST_UNCHECKED,0);
                EnableWindow(GetDlgItem(hwnd,IDC_DLDI_SETTINGS),TRUE);
                if((p = pPlugList->get_ActivePlugIn()) == NULL)
                    i = 0;
                else
                    i = pPlugList->IndexFromEle(p);
			}
           SendDlgItemMessage(hwnd,IDC_DLDI_SETTINGS,CB_SETCURSEL,(WPARAM)i,0);
           SendDlgItemMessage(hwnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"None");
           SendDlgItemMessage(hwnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Supercard CF/SD");
           SendDlgItemMessage(hwnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Guitar Hero Grip");
			SendDlgItemMessage(hwnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"GameBoy Advance Cartridges");

           SendDlgItemMessage(hwnd,IDC_COMBOBOX3,CB_SETCURSEL,(WPARAM)ds.get_ExpansionRam(),0);
           SendDlgItemMessage(hwnd,IDC_EDIT1,WM_SETTEXT,0,(LPARAM)FirmWare_get_UserName());
           SendDlgItemMessage(hwnd,IDC_EDIT1,EM_LIMITTEXT,10,0);
   		p1.hwndOwner = hwndPropertySheet;
   		pPlugList = pPlugInContainer->get_PlugInList(PIL_VIDEO);
   		for(i=0;i<(signed)pPlugList->Count();i++,i1++){
       		p = (PlugIn *)pPlugList->GetItem(i+1);
               if(!p->IsEnable())
                   continue;
       		p->InitGetPlugInInfo(&p1.info);
       		p1.info.pszText = (LPWSTR)s;
       		p1.info.cchTextMax = MAX_PATH;
       		p->GetInfo(&p1.info,TRUE);
       		p->SetProperty(&p1);
   		}
   		pPlugList = pPlugInContainer->get_PlugInList(PIL_AUDIO);
   		for(i=0;i<(signed)pPlugList->Count();i++,i1++){
       		p = (PlugIn *)pPlugList->GetItem(i+1);
               if(!p->IsEnable())
                   continue;
       		p->InitGetPlugInInfo(&p1.info);
       		p1.info.pszText = (LPWSTR)s;
       		p1.info.cchTextMax = MAX_PATH;
       		p->GetInfo(&p1.info,TRUE);
				p->SetProperty(&p1);
   		}
   		pPlugList = pPlugInContainer->get_PlugInList(PIL_GRAPHICS);
   		for(i=0;i<(signed)pPlugList->Count();i++,i1++){
       		p = (PlugIn *)pPlugList->GetItem(i+1);
               if(!p->IsEnable())
                   continue;
       		p->InitGetPlugInInfo(&p1.info);
       		p1.info.pszText = (LPWSTR)s;
       		p1.info.cchTextMax = MAX_PATH;
       		p->GetInfo(&p1.info,TRUE);
       		p->SetProperty(&p1);
   		}
   		pPlugList = pPlugInContainer->get_PlugInList(PIL_WIFI);
   		for(i=0;i<(signed)pPlugList->Count();i++,i1++){
       		p = (PlugIn *)pPlugList->GetItem(i+1);
               if(!p->IsEnable())
                   continue;
       		p->InitGetPlugInInfo(&p1.info);
       		p1.info.pszText = (LPWSTR)s;
       		p1.info.cchTextMax = MAX_PATH;
       		p->GetInfo(&p1.info,TRUE);
       		p->SetProperty(&p1);
   		}
   		pPlugList = pPlugInContainer->get_PlugInList(PIL_INPUT);
   		for(i=0;i<(signed)pPlugList->Count();i++,i1++){
       		p = (PlugIn *)pPlugList->GetItem(i+1);
               if(!p->IsEnable())
                   continue;
       		p->InitGetPlugInInfo(&p1.info);
       		p1.info.pszText = (LPWSTR)s;
       		p1.info.cchTextMax = MAX_PATH;
       		p->GetInfo(&p1.info,TRUE);
       		p->SetProperty(&p1);
   		}
			pPlugList = pPlugInContainer->get_PlugInList(PIL_DLDI);
   		for(i=0;i<(signed)pPlugList->Count();i++){
       		p = (PlugIn *)pPlugList->GetItem(i+1);
       		p->InitGetPlugInInfo(&p1.info);
       		p1.info.pszText = (LPWSTR)s;
       		p1.info.cchTextMax = MAX_PATH;
       		p->GetInfo(&p1.info,TRUE);
				p->SetProperty(&p1);
   		}
       break;
       case WM_NOTIFY:
           if(((LPNMHDR)lParam)->code == PSN_APPLY){
               i = ds.get_Optimize() & ~(NDSO_BOOT_FROM_BIOSES|NDSO_LOAD_BIOS|NDSO_LOAD_FIRMWARE);
           	if(SendDlgItemMessage(hwnd,IDC_CHECK8,BM_GETCHECK,0,0) == BST_CHECKED)
					i |= NDSO_BOOT_FROM_BIOSES;
           	if(SendDlgItemMessage(hwnd,IDC_CHECK5,BM_GETCHECK,0,0) == BST_CHECKED)
					i |= NDSO_LOAD_BIOS;
           	if(SendDlgItemMessage(hwnd,IDC_CHECK6,BM_GETCHECK,0,0) == BST_CHECKED)
					i |= NDSO_LOAD_FIRMWARE;
				ds.set_Optimize(i);
               ds.set_LoadDefARM7(SendDlgItemMessage(hwnd,IDC_CHECK1,BM_GETCHECK,0,0) == BST_CHECKED ? TRUE : FALSE);
				ds.set_TouchScreenSettings(SendDlgItemMessage(hwnd,IDC_TOUCH_SETTINGS,CB_GETCURSEL,0,0));
				FirmWare_set_LanguageSettings(SendDlgItemMessage(hwnd,IDC_LANGUAGE_SETTINGS,CB_GETCURSEL,0,0));
               SendDlgItemMessage(hwnd,IDC_EDIT1,WM_GETTEXT,MAX_PATH,(LPARAM)s);
               SendDlgItemMessage(hwnd,IDC_EDIT1,WM_GETTEXT,MAX_PATH,(LPARAM)s);
               FirmWare_set_UserName(s);
               if(SendDlgItemMessage(hwnd,IDC_CHECK7,BM_GETCHECK,0,0) == BST_CHECKED){
					ds.set_UseFATxEFS(TRUE);
                   i = ID_DLDI_NONE;
               }
               else{
                   ds.set_UseFATxEFS(FALSE);
                   if((i = SendDlgItemMessage(hwnd,IDC_DLDI_SETTINGS,CB_GETCURSEL,0,0)) != CB_ERR){
                       if(i == 0)
                           i = ID_DLDI_NONE;
                       else
                           i += ID_PLUGIN_DLDI_START - 1;
                   }
               }
               pPlugInContainer->get_PlugInList(PIL_DLDI)->OnEnablePlug((WORD)i);
               if((i = SendDlgItemMessage(hwnd,IDC_COMBOBOX3,CB_GETCURSEL,0,0)) != CB_ERR)
                   ds.set_ExpansionRam((u8)i);
               FirmWare_Reset();
               if(::IsWindowEnabled(GetDlgItem(hwnd,IDC_EDIT2))){
                   LRegKey key;

                   GetDlgItemText(hwnd,IDC_EDIT2,s,MAX_PATH);
                   i = atoi(s);
                   ds.set_MultiCore(i);
                   if(key.Open("Software\\iDeaS\\Settings")){
                       key.WriteLong("MultiCore",i);
                       key.Close();
                   }
               }
           }
       break;
       case WM_COMMAND:
       	switch(LOWORD(wParam)){
           	case IDC_CHECK7:
               	switch(HIWORD(wParam)){
                   	case BN_CLICKED:
                       	EnableWindow(GetDlgItem(hwnd,IDC_DLDI_SETTINGS),::SendMessage((HWND)lParam,BM_GETCHECK,0,0) == BST_CHECKED ? FALSE : TRUE);
                       break;
                   }
               break;
           }
       break;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL CALLBACK LDS::DlgProc22(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LDS *p;

   switch(uMsg){
   	case WM_INITDIALOG:
       	SetWindowLong(hwnd,GWL_USERDATA,((LPPROPSHEETPAGE)lParam)->lParam);
       default:
       	if((p = (LDS *)GetWindowLong(hwnd,GWL_USERDATA)) != NULL)
          		return p->OnDlgProc22(hwnd,uMsg,wParam,lParam);
       break;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL CALLBACK LDS::DlgProc17(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg){
   	case WM_INITDIALOG:
			SendDlgItemMessage(hwnd,IDC_RADIO1,BM_SETCHECK,BST_CHECKED,0);
       break;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL LDS::OnDlgProc23(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg){
   }
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL CALLBACK LDS::DlgProc23(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LDS *p;

   switch(uMsg){
   	case WM_INITDIALOG:
       	SetWindowLong(hwnd,GWL_USERDATA,((LPPROPSHEETPAGE)lParam)->lParam);
       default:
       	if((p = (LDS *)GetWindowLong(hwnd,GWL_USERDATA)) != NULL)
          		return p->OnDlgProc23(hwnd,uMsg,wParam,lParam);
       break;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
static int CALLBACK PropSheetProc(HWND hwndDlg,UINT uMsg,LPARAM lParam)
{
   if(uMsg == PSCB_INITIALIZED)
       hwndPropertySheet = hwndDlg;
   return 0;
}
//---------------------------------------------------------------------------
void LDS::OnShowProperties()
{
	PROPSHEETPAGE psp[2]={0};
   PROPSHEETHEADER psh={0};
	int i;

   i = 0;
   psp[i].dwSize = sizeof(PROPSHEETPAGE);
   psp[i].dwFlags = PSP_DEFAULT;
   psp[i].hInstance = langManager.get_CurrentLib();
   psp[i].pszTemplate = MAKEINTRESOURCE(IDD_DIALOG16);
   psp[i].pfnDlgProc = (DLGPROC)DlgProc16;
   psp[i++].lParam = (LPARAM)this;

   psp[i].dwSize = sizeof(PROPSHEETPAGE);
   psp[i].dwFlags = PSP_DEFAULT;
   psp[i].hInstance = langManager.get_CurrentLib();
   psp[i].pszTemplate = MAKEINTRESOURCE(IDD_DIALOG22);
   psp[i].pfnDlgProc = (DLGPROC)DlgProc22;
   psp[i++].lParam = (LPARAM)this;

   psh.dwSize = sizeof(PROPSHEETHEADER);
   psh.dwFlags = PSH_NOAPPLYNOW|PSH_USEICONID|PSH_PROPSHEETPAGE|PSH_DEFAULT|PSH_USECALLBACK;
   psh.pfnCallback = PropSheetProc;
   psh.pszCaption = "Properties...";
   psh.pszIcon = MAKEINTRESOURCE(2);
   psh.hInstance = hInst;
   psh.hwndParent = upLcd.Handle();
   psh.nPages = i;
   psh.ppsp = psp;
   if(PropertySheet(&psh) == -1)
       return;
}
//---------------------------------------------------------------------------
BOOL LDS::OnKeyDown(WPARAM wParam,LPARAM lParam)
{
	u16 key;

#ifdef _DEBUG
   debugDlg.OnKeyDown(wParam,lParam);
#endif
	key = ((u16 *)(io_mem + 0x130))[0];
  	if(wParam == keyBoard[0])//Z -> A
   	key &= ~1;
   else if(wParam == keyBoard[1])//A -> B
  	    key &= ~2;
   else if(wParam == keyBoard[2])//X -> Select
       key &= ~4;
   else if(wParam == keyBoard[3])//VK_RETURN -> Start
       key &= ~8;
   else if(wParam == keyBoard[4])//RIGHT
       key &= ~16;
   else if(wParam == keyBoard[5])//LEFT
       key &= ~32;
   else if(wParam == keyBoard[6])//UP
       key &= ~64;
   else if(wParam == keyBoard[7])//down
       key &= ~128;
   else if(wParam == keyBoard[8])//S -> R
       key &= ~256;
   else if(wParam == keyBoard[9])//VK_TAB -> L
       key &= ~512;
   else if(wParam == keyBoard[10])//Q -> X
		((u16 *)(io_mem7 + 0x136))[0] &= ~0x1;
   else if(wParam == keyBoard[11])//W -> Y
		((u16 *)(io_mem7 + 0x136))[0] &= ~0x2;
	((u16 *)&(io_mem[0x130]))[0] = key;
	((u16 *)(io_mem7 + 0x130))[0] = key;
  	cheatsManager.write_hword(0x04000130,key);
   if(wParam == keyGrip[3])
       nGripKeys &= ~0x40;//Green
   else if(wParam == keyGrip[1])
       nGripKeys &= ~0x20;//Yellow
   else if(wParam == keyGrip[2])
       nGripKeys &= ~0x10;//Red
   else if(wParam == keyGrip[0])
       nGripKeys &= ~0x8;//Blue
   if(wParam == VK_SPACE && (GetAsyncKeyState(VK_CONTROL) & 0x8000))
   	bFreeRunning = TRUE;
	return FALSE;
}
//---------------------------------------------------------------------------
BOOL LDS::OnKeyUp(WPARAM wParam,LPARAM lParam)
{
	u16 key;

	key = ((u16 *)(io_mem + 0x130))[0];
  	if(wParam == keyBoard[0])//Z
   	key |= 1;
   else if(wParam == keyBoard[1])//A
  	    key |= 2;
   else if(wParam == keyBoard[2])//X
       key |= 4;
   else if(wParam == keyBoard[3])//VK_RETURN
       key |= 8;
   else if(wParam == keyBoard[4])//RIGHT
       key |= 16;
   else if(wParam == keyBoard[5])//LEFT
       key |= 32;
   else if(wParam == keyBoard[6])//UP
       key |= 64;
   else if(wParam == keyBoard[7])//down
       key |= 128;
   else if(wParam == keyBoard[8])//S
       key |= 256;
   else if(wParam == keyBoard[9])//VK_TAB
       key |= 512;
   else if(wParam == keyBoard[10])//Q
		((u16 *)(io_mem7 + 0x136))[0] |= 0x1;
   else if(wParam == keyBoard[11])//W
		((u16 *)(io_mem7 + 0x136))[0] |= 0x2;
	((u16 *)&(io_mem[0x130]))[0] = key;
	((u16 *)(io_mem7 + 0x130))[0] = key;
	cheatsManager.write_hword(0x04000130,key);
   if(wParam == keyGrip[3])
       nGripKeys |= 0x40;//Green
   else if(wParam == keyGrip[1])
       nGripKeys |= 0x20;//Yellow
   else if(wParam == keyGrip[2])
       nGripKeys |= 0x10;//Red
   else if(wParam == keyGrip[0])
       nGripKeys |= 0x8;//Blue
   if(wParam == VK_SPACE)
   	bFreeRunning = FALSE;
	return FALSE;
}
//---------------------------------------------------------------------------
void LDS::OnDisplayCapture(DWORD data)
{
   int i;
   u32 old;

   old = dwDispCap;
   if(nSkipCount != 0 && (dwDispCap & 0x4000) && (dwOptions & NDSO_FRAMES_SKIP_CAPTURE)){
       dwDispCap |= (u16)(((data >> 24) & 1) + 1);
       if(dwDispCap & 0x20){
           i = nSkipCount & ~1;
           if(dwDispCap & 0x40)
               i = 2;
           else
               nSkipCount & ~1;
       }
       else
           i = 2;
       if((dwDispSwap & 0x3FFF) > i){
           dwDispSwap &= 0xC000;
           dwDispCap |= 0x8020;
           if((dwDispCap & 7) == 2)
               dwDispCap |= 0x40;
           else
               dwDispCap &= ~0x40;
           dwDispCap &= ~7;
       }
       else
           dwDispCap &= (u16)~0xC000;
   }
   else
       dwDispCap = 0x8000;
   dwDispCap &= (u32)~0x4000;
/*  	if(VCOUNT < 192){
       dwDispCap |= (dwDispCap & 0x8000) << 16;
   	if(!(old & 0x8000)){
           dwDispCap &= ~0x8000;
       }
   }*/
}
//---------------------------------------------------------------------------
void LDS::OnDisplaySwap(u16 value)
{
   u32 i;

   if((dwDispSwap & 0x8000) == (value & 0x8000))
       return;
   i = ((dwDispSwap & 0x7FFF) + 1) & 0x7FFF;
   dwDispSwap = (u32)((value & 0x8000) | i);
   dwDispCap |= 0x4000;
   dwDispSwap |= 0x80000000;
//   if(VCOUNT > 191)
   	DisplaySwap();
}
//---------------------------------------------------------------------------
void LDS::DisplaySwap()
{
   char *h;

   if((dwDispSwap & 0x8000)){
       h = downLcd.get_Buffer();
       downLcd.set_OutBuffer(upLcd.get_Buffer());
       upLcd.set_OutBuffer(h);
   }
   else{
       upLcd.set_OutBuffer();
       downLcd.set_OutBuffer();
   }
   dwDispSwap &= ~0x80000000;
}
//---------------------------------------------------------------------------
void LDS::DisplayCapture()
{
   u8 bank,size,source,eva,evb;
	u32 col32,x,y,rA,bA,gA,rB,bB,gB,data,srcOfs;
   u16 *dst,color,dstOfs;
	void *srcA,*srcB;
   short sz[][2]={{128,128},{256,64},{256,128},{256,192}};

   data = DISPCAPCNT;
   bank = (u8)((data >> 16) & 3);
   if(io_mem[0x240+bank] != 0x80)
   	return;
	size = (u8)((data >> 20) & 3);
   source = (u8)((data >> 24) & 1);
   switch(source){
   	case 0:
           srcB = downLcd.get_OutBuffer();
       break;
       case 1:
       	if(p3DPlugIn == NULL)
           	return;
           p3DPlugIn->NotifyState(PNMV_GETBUFFER,PIS_NOTIFYMASK,(LPARAM)&srcB);
           if(srcB == NULL)
               return;
       break;
   }
   switch(((data >> 25) & 1)){
   	case 0:
           srcA = video_mem;
       break;
       case 1:
       	srcA = downLcd.get_MainMemoryAddress();
       break;
   }
	dstOfs = (u16)(((data >> 18) & 3) * 0x4000);
	dst = (u16 *)(&video_mem[bank * 0x20000]);
   switch((data >> 29) & 3){
       case 2:
   	case 3:
   		if((eva = (u8)((data & 0x1F))) > 16)
       		eva = 16;
   		if((evb = (u8)((data >> 8) & 0x1F)) > 16)
       		evb = 16;
           bank = (u8)((io_mem[2] >> 2) & 3);
			srcA = (void *)&((u8 *)srcA)[bank * 0x20000];
   		if(io_mem[0x240+bank] != 0x80)
   			evb = 0;
   		if((io_mem[2] & 3) != 2){
           	srcOfs = ((data >> 26) & 3) * 0x8000;
   			srcA = (void *)(((u8 *)srcA)+srcOfs);
           }
			switch(source){
           	case 0:
       			for(y=sz[size][1];y>0;y--){
           			for(x=sz[size][0];x>0;x--){
                           color = *(((u16 *)srcB));
							srcB = ((u8 *)srcB) + 2;
                           bA = ((color & 31));
                           rA = (color >> 10);
                           gA = ((color >> 5) & 31);
                           color = *(((u16 *)srcA));
                           srcA = ((u8 *)srcA) + 2;
                           rB = ((color & 31));
                           bB = ((color >> 10) & 31);
                           gB = ((color >> 5) & 31);
							if((rA = (rA * eva + rB * evb) >> 4) > 31)
                           	rA = 31;
							if((gA = (gA * eva + gB * evb) >> 4) > 31)
                           	gA = 31;
                           if((bA = (bA * eva + bB * evb) >> 4) > 31)
                           	bA = 31;
							dst[dstOfs++] = (u16)(0x8000| rA | (gA << 5) | (bA << 10));
                       }
                       dstOfs += (u16)(256 - sz[size][0]);
               	}
				break;
               case 1:
       			for(y=0;y<sz[size][1];y++){
           			for(x=0;x<sz[size][0];x++){
                           col32 = ((u32 *)srcB)[((191-y)<<8)+x];
                           bA = ((u8)col32) >> 3;
							gA = ((u8)(col32 >> 8)) >> 3;
                           rA = ((u8)(col32 >> 16)) >> 3;
                           color = *(((u16 *)srcA));
                           srcA = ((u8*)srcA) + 2;
                           rB = ((color & 31));
                           bB = (color >> 10);
                           gB = ((color >> 5) & 31);
							if((rA = (rA * eva + rB * evb) >> 4) > 31)
                           	rA = 31;
							if((gA = (gA * eva + gB * evb) >> 4) > 31)
                           	gA = 31;
                           if((bA = (bA * eva + bB * evb) >> 4) > 31)
                           	bA = 31;
							dst[dstOfs++] = (u16)(0x8000| rA | (gA << 5) | (bA << 10));
                       }
                       dstOfs += (u16)(256 - sz[size][0]);
               	}
               break;
			}
       break;
   	default:
			switch(source){
           	case 0:
       			for(y=sz[size][1];y>0;y--){
           			for(x=sz[size][0];x>0;x--){
                           color = *(((u16 *)srcB));
							srcB = ((u8*)srcB) + 2;
					        dst[dstOfs++] = (u16)(0x8000|((color & 0x1F) << 10) | (color & 0x3E0) | (color >> 10));
                       }
                       dstOfs += (u16)(256 - sz[size][0]);
               	}
				break;
       		case 1:
               	for(y=0;y< sz[size][1];y++){
           			for(x=0;x<sz[size][0];x++){
                           col32 = ((u32 *)srcB)[((191-y)<<8)+x];
                           bA = (u8)col32;
							gA = (u8)(col32 >> 8);
                           rA = (u8)(col32 >> 16);
							dst[dstOfs++] = (u16)(0x8000|(rA >> 3) | ((gA >> 3) << 5) | ((bA >> 3) << 10));
                   	}
               		dstOfs += (u16)(256 - sz[size][0]);
           		}
       		break;
   		}
		break;
   }
}
//---------------------------------------------------------------------------
void LDS::OnShowRomInfo()
{
	LRomInfo dlg;

   dlg.Show(Handle());
}
//---------------------------------------------------------------------------
void LDS::OnMenuLoop(BOOL bEnter)
{
   if(!bRom || dwFrame == 0)
       return;
   if(bRun && bEnter){
       pPlugInContainer->NotifyState(-1,PIS_PAUSE,PIS_RUNMASK);
       bRun = FALSE;
   }
   else if(!bEnter && !bRun){
   	if(!bPause){
           pPlugInContainer->NotifyState(-1,PIS_RUN,PIS_RUNMASK);
           bRun = TRUE;
       }
   	*((u16 *)&io_mem[0x130]) = 0x3FF;
   	*((u16 *)&io_mem7[0x130]) = 0x3FF;
   	nGripKeys = 0xFF;
   }
}
//---------------------------------------------------------------------------
void LDS::OnChangeDISPSTAT(int index,u8 *mem,u32 data)
{
   if((data & 0x10) && bAutoIncreaseSpeed)
       IncreaseCpuSpeed(0);
}
//---------------------------------------------------------------------------
BOOL LDS::Load(LStream *pFile,int ver)
{
   if(!LMMU::Load(pFile,ver))
   	return FALSE;
   dwDispSwap = *((u16 *)&io_mem[0x304]) & 0x8000;
   return TRUE;
}
//---------------------------------------------------------------------------
void LDS::set_EmuMode(int mode)
{
	u16 w,h;
	int i;

	if(mode == EMUM_GBA){
		nEmuMode = EMUM_GBA;
       pfnLoop = &LDS::OnLoopGBA;
       w = 240;
       h = 160;
       for(i = 0;i<250;i++)
           VRAMmap[i] = (u8)(i % 6);
   }
   else
   {
		nEmuMode = EMUM_NDS;
       pfnLoop = &LDS::OnLoop;
       w = 256;
       h = 192;
   }
	upLcd.set_PowerMode();
   downLcd.set_PowerMode();
   upLcd.set_Size(w,h);
   downLcd.set_Size(w,h);
   arm7.InitTable(i_func,i_func7);
   if(pPlugInContainer != NULL)
   	pPlugInContainer->NotifyState(-1,PNM_INITTABLE,PIS_NOTIFYMASK,get_RomType());
}
//---------------------------------------------------------------------------
int LDS::set_VideoMemoryStatus(DWORD adr,DWORD size,DWORD status)
{
    if(adr < 0x067FFFFF || adr > 0x068A4000)
        return E_FAIL;
    if(status)
        status = 1;
    if(adr+size > 0x68A4000)
    	size = 0x68A4000 - adr;
    for(;size > 0 && (adr & 31) != 0;size--,adr++)
       video_cache[(adr & 0xFFFFF) >> 5] &= ~(1 << (adr & 0x1F));
    adr = (adr & 0xFFFFF) >> 5;
    for(;size > 31;size -= 32,adr++)
        video_cache[adr] = 0;
    for(;size > 0;size--)
        video_cache[adr] &= ~(1 << (size & 0x1F));
    return S_OK;
}
//---------------------------------------------------------------------------
int LDS::get_VideoMemoryStatus(DWORD adr,DWORD size,LPDWORD status)
{
   if(status == NULL)
       return E_FAIL;
   *status = 0;
   switch(adr >> 20){
       case 0x68:
//           if(adr > 0x067FFFFF){
               for(;size > 0 & (adr & 31) != 0;size--,adr++){
                   if(video_cache[(adr & 0xFFFFF) >> 5] & (1 << (adr & 31))){
                       *status = 1;
                       return S_OK;
                   }
               }
               adr = (adr & 0xFFFFF) >> 5;
               for(;size > 31;size -= 32,adr++){
                   if(video_cache[adr]){
                       *status = 1;
                       return S_OK;
                   }
               }
               for(;size > 0;size--){
                   if(video_cache[adr] & (1 << (size & 31))){
                       *status = 1;
                       return S_OK;
                   }
               }
               return S_OK;
//           }
       break;
   }
   return E_FAIL;
}
//---------------------------------------------------------------------------
int LDS::get_FramesCount(LPDWORD value)
{
   if(value == NULL)
       return E_FAIL;
   *value = Frames();
   return S_OK;
}
//---------------------------------------------------------------------------
int LDS::QueryInterface(int iid,LPVOID *ppvObject)
{
   if(ppvObject == NULL)
       return E_FAIL;
   *ppvObject = NULL;
   if(iid == IID_IMEMORY){
       *ppvObject = (LPVOID)(INDSMemory *)this;
       return S_OK;
   }
   if(iid == IID_IPLUGINMANAGER){
       *ppvObject = (LPVOID)(IPlugInManager *)pPlugInContainer;
       return S_OK;
   }
   if(iid == IID_UPLCD){
       *ppvObject = (LPVOID)(IWnd *)&upLcd;
       return S_OK;
   }
   if(iid == IID_DOWNLCD){
       *ppvObject = (LPVOID)(IWnd *)&downLcd;
       return S_OK;
   }
   if(iid == IID_IFAT){
       *ppvObject = (LPVOID)(IFat *)get_FatInterface();
       return S_OK;
   }
   return E_FAIL;
}
//---------------------------------------------------------------------------
int LDS::get_CurrentFileName(char *buffer,int *maxLength)
{
   LString s;
   int len;

   if(maxLength == NULL || buffer == NULL)
       return E_INVALIDARG;
   s = get_FileName();
   if(*maxLength <= (len = s.Length())){
       *maxLength = s.Length()+1;
       return E_FAIL;
   }
   *maxLength = ++len;
   lstrcpyn(buffer,s.c_str(),len);
   return S_OK;
}
//---------------------------------------------------------------------------
int LDS::CreateFileName(int type,char *buffer,int *maxLength,char *ext,int ID)
{
   LString s;
   int len;

   if(maxLength == NULL || buffer == NULL)
       return E_INVALIDARG;
   s.Capacity(MAX_PATH+1);
   if(!BuildFileName(type,s.c_str(),MAX_PATH,ext,ID))
       return E_FAIL;
   if(*maxLength <= (len = s.Length())){
       *maxLength = s.Length()+1;
       return E_FAIL;
   }
   *maxLength = ++len;
   lstrcpyn(buffer,s.c_str(),len);
   return S_OK;
}
//---------------------------------------------------------------------------
int LDS::WriteConsole(char *msg)
{
//	IARM7 *arm;

	if(msg == NULL || *msg == 0)
   	return E_FAIL;
/*   switch(index){
       case 7:
           arm = (IARM7 *)&arm7;
       break;
       case 9:
           arm = (IARM7 *)&arm9;
       break;
       default:
       	return E_FAIL;
   }*/
	WriteMsgConsolle(NULL,"%c%s",MSGT_OUTONCONSOLLE,msg);
	return S_OK;
}
//---------------------------------------------------------------------------
int LDS::get_IsHomebrew(int *res)
{
	if(res == NULL || !bRom)
   	return E_FAIL;
   if(get_RomType() != 0 && get_RomType() != 3)
   	*res = 1;
   else
   	*res = 0;
   return S_OK;
}
//---------------------------------------------------------------------------
int LDS::get_EmulatorMode(int *res)
{
	if(res == NULL)
   	return E_FAIL;
   *res = nEmuMode;
	return S_OK;
}
//---------------------------------------------------------------------------
int LDS::get_NumberOfCores(LPDWORD res)
{
	if(res == NULL)
   	return E_FAIL;
   *res = get_MultiCore();
	return S_OK;
}
//---------------------------------------------------------------------------
int LDS::get_DutyCycles(LPDWORD res)
{
	if(res == NULL)
   	return E_FAIL;
   *res = (DWORD)get_StopCycles();
	return S_OK;
}
//---------------------------------------------------------------------------
BOOL LDS::EnableMultiCore(BOOL bFlag)
{
   int i;
   LString s;
   DWORD dw;

   if(!bFlag){
       BOOL b;

       b = bQuit;
       bQuit = TRUE;
       for(i=0;i<sizeof(hEvents)/sizeof(HANDLE);i++){
           if(hEvents[i] != NULL)
               ::SetEvent(hEvents[i]);
       }
       if(hThread != NULL){
           WaitForSingleObject(hThread,INFINITE);
           CloseHandle(hThread);
           hThread = NULL;
       }
       for(i=0;i<sizeof(hEvents)/sizeof(HANDLE);i++){
           if(hEvents[i] != NULL)
               CloseHandle(hEvents[i]);
       }
       bQuit = b;
   }
   else if(hThread == NULL){
       if(nCores > 1){
           for(i=0;i<2;i++){
               s = "iDeaS_Event_";
               s += i;
               hEvents[i] = CreateEvent(NULL,FALSE,FALSE,s.c_str());
               if(hEvents[i] == NULL){
                   nCores = 1;
                   break;
               }
           }
       }
       if(nCores > 1){
           hThread = CreateThread(NULL,0,ThreadFunc_01,(LPVOID)this,0,&dw);
           if(hThread == NULL)
               nCores = 1;
           SetEvent(hEvents[1]);
       }
   }
   return TRUE;
}
//---------------------------------------------------------------------------
void LDS::set_MultiCore(int value)
{
   SYSTEM_INFO si;

   GetSystemInfo(&si);
   if(value > si.dwNumberOfProcessors)
       value = si.dwNumberOfProcessors;
   if(nCores == value)
       return;
   nCores = value;
   EnableMultiCore(nCores > 1);
   pPlugInContainer->NotifyState(-1,PNM_CHANGECONFIG,PIS_NOTIFYMASK,1);
}
//---------------------------------------------------------------------------
DWORD LDS::OnThread_01()
{
   int i;

   SetThreadIdealProcessor(GetCurrentThread(),((1 << nCores) -1));
//   SetThreadAffinityMask(GetCurrentThread(),2);
   while(!bQuit){
       WaitForSingleObject(hEvents[0],INFINITE);
       if(bQuit)
           break;
       ((GraphicsPlugList *)pPlugInContainer->get_PlugInList(PIL_GRAPHICS))->Run(upLcd.get_Buffer(),downLcd.get_Buffer());
       if(!skip_old){
#ifndef __WIN32__
           XLockDisplay(GDK_DISPLAY());
#endif
           upLcd.BitBlt();
           downLcd.BitBlt();
#ifndef __WIN32__
           XUnlockDisplay(GDK_DISPLAY());
#endif
       }
       i = (int)pPlugInContainer->get_PlugInList(PIL_INPUT)->get_ActivePlugIn();
       if(i){
           DWORD dw;

           dw = MAKELONG(((u16 *)(io_mem + 0x130))[0],((u16 *)(io_mem7 + 0x136))[0]);
           if(((LInputPlug *)i)->Run(&dw)){
               ((u16 *)(io_mem + 0x130))[0] = ((u16 *)(io_mem7 + 0x130))[0] = (u16)dw;
               ((u16 *)(io_mem7 + 0x136))[0] &= ~0xB;
               ((u16 *)(io_mem7 + 0x136))[0] |= (u16)((dw >> 16) & 0xB);
               cheatsManager.write_hword(0x04000130,(u16)dw);
           }
       }
       SetEvent(hEvents[1]);
   }
   return 0;
}
//---------------------------------------------------------------------------
DWORD WINAPI LDS::ThreadFunc_01(LPVOID arg)
{
   return ((LDS *)arg)->OnThread_01();
}
//---------------------------------------------------------------------------
static void OnApplyIPSPatch()
{
   char szFile[MAX_PATH];
   LFile *pFile;
   DWORD ofs;
   WORD bts;
   BYTE b;
   LString nameFile;

   *((LPDWORD)szFile) = 0;
   if(!nameFile.BuildFileName((char *)ds.get_FileName(),"ips"))
       return;
   pFile = new LFile(nameFile.c_str());
   if(pFile != NULL && pFile->Open())
       goto OnApplyIPSPatch_2;
   lstrcpy(szFile,nameFile.c_str());
   if(!ShowOpenDialog(NULL,"IPS Files(*.ips)\0*.ips\0\0\0\0\0",upLcd.Handle(),szFile))
       goto ex_OnApplyIPSPatch;
   if(pFile != NULL)
       delete pFile;
   pFile = new LFile(szFile);
   if(pFile == NULL || !pFile->Open())
       return;
   OnApplyIPSPatch_2:
   ((LPDWORD)szFile)[0] = ((LPDWORD)szFile)[1] = 0;
   pFile->Read(szFile,5);
   if(lstrcmpi(szFile,"PATCH") != 0)
       goto ex_OnApplyIPSPatch;
   do{
       ofs = 0;
       if(pFile->Read(&ofs,3) != 3)
           break;
       ofs = ((BYTE)ofs << 16) | (ofs & 0xFF00) | ((BYTE)(ofs >> 16));
       if(pFile->Read(&bts,2) != 2)
           break;
       bts = MAKEWORD(HIBYTE(bts),LOBYTE(bts));
       if(ofs == 0x454F46 && bts == 0x7FAB)
           break;
       if(!bts){
           if(pFile->Read(&bts,2) != 2)
               goto ex_OnApplyIPSPatch;
           bts = MAKEWORD(HIBYTE(bts),LOBYTE(bts));
           if(pFile->Read(&b,1) != 1)
               goto ex_OnApplyIPSPatch;
           for(;bts != 0;ofs++,bts--){
               /*               if((ofs >> 16) > bin.maxIndex){
                   if((rom_pages_u8[(ofs >> 16)] = (unsigned char *)GlobalAlloc(GPTR,0x10000)) == NULL)
                       return;
                   bin.maxIndex++;
                   bin.rom_size_u8 += bts;
               }
               rom_pages_u8[(ofs >> 16)][(u16)ofs] = b;*/
           }
       }
       else{
           for(;bts != 0;ofs++,bts--){
               if(pFile->Read(&b,1) != 1)
                   goto ex_OnApplyIPSPatch;
               /*               if((ofs >> 16) > bin.maxIndex){
                   if((rom_pages_u8[(ofs >> 16)] = (unsigned char *)GlobalAlloc(GPTR,0x10000)) == NULL)
                       return;
                   bin.maxIndex++;
                   bin.rom_size_u8 += bts;
               }
               rom_pages_u8[(ofs >> 16)][(u16)ofs] = b;*/
           }
       }
   }while(1);
ex_OnApplyIPSPatch:
   if(pFile != NULL)
       delete pFile;
}
//---------------------------------------------------------------------------
/*void LDS::EnableIRQ(int index,BOOL value)
{
   DWORD i,i1,i2,dwSizeCode;
   unsigned char s_b[] = {0xFF,0x15,0,0,0,0},*p1,*p;
   unsigned long vfn;

   if(!use_switch_irq)
       return;
   lino_cazzo.buf = buf;
   p = (unsigned char *)(&(this->pfnLoop));
   p = (unsigned char *)*((unsigned long *)p);
   if(lino_cazzo.hPr == NULL){
       DWORD dw,dw1;

       lino_cazzo.hPr = OpenProcess(PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_QUERY_INFORMATION, FALSE,GetCurrentProcessId());
       if(lino_cazzo.hPr == NULL)
           return;
       ReadProcessMemory(lino_cazzo.hPr,(LPVOID)p,lino_cazzo.buf,sizeof(buf)/2,&lino_cazzo.dwSizeCode);
       dwSizeCode = lino_cazzo.dwSizeCode;
       for(dw1=0;dw1<2;dw1++){
           switch(dw1){
               case 0:
                   vfn = (unsigned long)&pfn_arm9irq;
               break;
               case 1:
                   vfn = (unsigned long)&pfn_arm7irq;
               break;
               case 2:
                   vfn = (unsigned long)&exec9;
               break;
               case 3:
                   vfn = (unsigned long)&exec7;
               break;
           }
           *((unsigned long *)&s_b[2]) = vfn;
           dw = 0;
           for(i=0;i<lino_cazzo.dwSizeCode;i++){
               p1 = &buf[i];
               for(i1 = 0;i1 <sizeof(s_b);i1++){
                   if(*p1++ != s_b[i1])
                       break;
               }
               if(i1 == sizeof(s_b)){
                   if(dw1 == 0)
                       lino_cazzo.arm9.pointer[lino_cazzo.arm9.count++].code_p = i;
                   else
                       lino_cazzo.arm7.pointer[lino_cazzo.arm7.count++].code_p = i;
                   i += (i1-1);
               }
           }
       }
       dw = 0;
       dw1 = 0xFFFFFFFF;
       for(i=0;i<lino_cazzo.arm9.count;i++){
           if(lino_cazzo.arm9.pointer[i].code_p > dw)
               dw = lino_cazzo.arm9.pointer[i].code_p + sizeof(s_b);
           if(lino_cazzo.arm9.pointer[i].code_p < dw1)
               dw1 = lino_cazzo.arm9.pointer[i].code_p;
       }
       for(i=0;i<lino_cazzo.arm7.count;i++){
           if(lino_cazzo.arm7.pointer[i].code_p > dw)
               dw = lino_cazzo.arm7.pointer[i].code_p + sizeof(s_b);
           if(lino_cazzo.arm7.pointer[i].code_p < dw1)
               dw1 = lino_cazzo.arm7.pointer[i].code_p;
       }
       lino_cazzo.dwSizeCode = dw;
       lino_cazzo.dwOffset = dw1;

   }
   if(!value){
       if(index == 9){
           if(buf[lino_cazzo.arm9.pointer[0].code_p] == 0x90)
               return;
           for(i=0;i<lino_cazzo.arm9.count;i++){
               memset(&buf[lino_cazzo.arm9.pointer[i].code_p],0x90,sizeof(s_b));
           }
           use_irq[0]++;
           use_irq[3] = 0;
       }
       else if(index == 7){
           if(buf[lino_cazzo.arm7.pointer[0].code_p] == 0x90)
               return;
           for(i=0;i<lino_cazzo.arm7.count;i++){
               memset(&buf[lino_cazzo.arm7.pointer[i].code_p],0x90,sizeof(s_b));
           }
       }
   }
   else{
       if(index == 9){
           vfn = (unsigned long)&pfn_arm9irq;
           *((unsigned long *)&s_b[2]) = vfn;
           if(buf[lino_cazzo.arm9.pointer[0].code_p] == s_b[0])
               return;
           for(i=0;i<lino_cazzo.arm9.count;i++){
               memcpy(&buf[lino_cazzo.arm9.pointer[i].code_p],s_b,sizeof(s_b));
           }
           use_irq[1]++;
           use_irq[3] = 1;
       }
       else if(index == 7){
           vfn = (unsigned long)&pfn_arm7irq;
           *((unsigned long *)&s_b[2]) = vfn;
           if(buf[lino_cazzo.arm7.pointer[0].code_p] == s_b[0])
               return;
           for(i=0;i<lino_cazzo.arm7.count;i++){
               memcpy(&buf[lino_cazzo.arm7.pointer[i].code_p],s_b,sizeof(s_b));
           }
       }

   }
   WriteProcessMemory(lino_cazzo.hPr,(LPVOID)&p[lino_cazzo.dwOffset],&buf[lino_cazzo.dwOffset],lino_cazzo.dwSizeCode,&i1);
}*/


