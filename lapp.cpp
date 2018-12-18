#include "lapp.h"
#include "lwnd.h"
#include "lmenu.h"

#ifdef __WIN32__
   LPGDIPBITMAPLOCKBITS pfnGdipBitmapLockBits;
   LPGDIPBITMAPUNLOCKBITS pfnGdipBitmapUnlockBits;
   LPGDIPDRAWIMAGERECTI pfnGdipDrawImageRectI;
   LPGDIPCREATEBITMAPFROMHBITMAP pfnGdipCreateBitmapFromHBITMAP;
   LPGDIPRELEASEDC pfnGdipReleaseDC;
   LPGDIPDISPOSEIMAGE pfnGdipDisposeImage;
   LPGDIPCREATEFROMHDC pfnGdipCreateFromHDC;
   LPGDIPCREATEBITMAPFROMGDIDIB pfnGdipCreateBitmapFromGDIDIB;
   LPGDIPDELETEGRAPHICS pfnGdipDeleteGraphics;
#endif
//---------------------------------------------------------------------------
LApp::LApp()
{
	bQuit = FALSE;
#ifdef __WIN32__
   hGdipLib = NULL;
   token = 0;
#endif
}
//---------------------------------------------------------------------------
LApp::~LApp()
{
	bQuit = TRUE;
#ifdef __WIN32__
   if(hGdipLib != NULL){
       if(token != 0){
           pfnGdipShutDown(token);
           token = 0;
       }
       FreeLibrary(hGdipLib);
       hGdipLib = NULL;
   }
   OleUninitialize();
#endif
}
//---------------------------------------------------------------------------
BOOL LApp::GetMessage(LPMSG lpMsg)
{
#ifndef __WIN32__
	LPMSG p;

	gtk_main_iteration ();
	p = gtk_get_current_event();
	if(p != NULL)
		memcpy(lpMsg,p,sizeof(MSG));
	return TRUE;
#else
   return ::GetMessage(lpMsg,NULL,0,0);
#endif
}
//---------------------------------------------------------------------------
BOOL LApp::PeekMessage(LPMSG lpMsg)
{
#ifndef __WIN32__
	HWND hwnd;

	gtk_main_iteration_do (false);
	hwnd = gtk_grab_get_current();
	if(hwnd != NULL){
		if(GTK_IS_MENU_SHELL(hwnd))
			return TRUE;
	}
	return gtk_events_pending();
#else
   return ::PeekMessage(lpMsg,NULL,0,0,PM_REMOVE);
#endif
}
//---------------------------------------------------------------------------
void LApp::ProcessMessages()
{
	MSG msg;

	while(PeekMessage(&msg))
       DispatchMessage(&msg);
}
//---------------------------------------------------------------------------
BOOL LApp::WaitMessage()
{
#ifndef __WIN32__
	gtk_main_iteration();
	return TRUE;
#else
   return ::WaitMessage();
#endif
}
//---------------------------------------------------------------------------
void LApp::PostQuitMessage()
{
	bQuit = TRUE;
}
//---------------------------------------------------------------------------
BOOL LApp::TranslateMessage(LPMSG lpMsg)
{
#ifdef __WIN32__
   return ::TranslateMessage(lpMsg);
#else
/*	int id;

	if(lpMsg->type == KeyPress){
		id = KeyCodeToVK((XKeyEvent *)lpMsg);
		if((id >= 'a' && id <= 'z'))
			id = id + 'A' - 'a';
		return TRUE;
	}*/
   return FALSE;
#endif
}
//---------------------------------------------------------------------------
LONG LApp::DispatchMessage(LPMSG lpMsg)
{
#ifndef __WIN32__
	//gtk_main_do_event(lpMsg);
 	gtk_main_iteration_do (false);
   return 0;
#else
   return ::DispatchMessage(lpMsg);
#endif
}
//---------------------------------------------------------------------------
IMenu *LApp::CoCreateInstance_Menu(HMENU menu)
{
 	LPopupMenu *pMenu;

 	if((pMenu = new LPopupMenu()) == NULL)
		return FALSE;
	pMenu->Attack(menu);
	return pMenu;
}
//---------------------------------------------------------------------------
BOOL LApp::Init(int argc,char **argv)
{
#ifndef __WIN32__
	window_init(argc,argv);
   {
       SYSTEM_INFO si;

       GetSystemInfo(&si);
       if(si.dwNumberOfProcessors > 1)
           XInitThreads();
   }
	gtk_set_locale();
  	gtk_init(&argc,&argv);
	bQuit = FALSE;
	return TRUE;
#else
   INITCOMMONCONTROLSEX ice;
   GDIPSTARTUPINPUT st={1,NULL,FALSE,FALSE};
   LPVOID output;

   OleInitialize(NULL);
   if(hGdipLib == NULL)
       hGdipLib = LoadLibrary("gdiplus.dll");
   if(hGdipLib != NULL){
       pfnGdipStartup = (LPGDIPSTARTUP)GetProcAddress(hGdipLib,"GdiplusStartup");
       pfnGdipStartup(&token,&st,output);
       pfnGdipShutDown = (LPGDIPSHUTDOWN)GetProcAddress(hGdipLib,"GdiplusShutdown");
       pfnGdipCreateFromHDC = (LPGDIPCREATEFROMHDC)
           GetProcAddress(hGdipLib,"GdipCreateFromHDC");
       pfnGdipCreateBitmapFromHBITMAP = (LPGDIPCREATEBITMAPFROMHBITMAP)
           GetProcAddress(hGdipLib,"GdipCreateBitmapFromHBITMAP");
       pfnGdipBitmapLockBits = (LPGDIPBITMAPLOCKBITS)
           GetProcAddress(hGdipLib,"GdipBitmapLockBits");
       pfnGdipBitmapUnlockBits = (LPGDIPBITMAPUNLOCKBITS)
           GetProcAddress(hGdipLib,"GdipBitmapUnlockBits");
       pfnGdipDrawImageRectI = (LPGDIPDRAWIMAGERECTI)
           GetProcAddress(hGdipLib,"GdipDrawImageRectI");
       pfnGdipDisposeImage = (LPGDIPDISPOSEIMAGE)
           GetProcAddress(hGdipLib,"GdipDisposeImage");
       pfnGdipReleaseDC = (LPGDIPRELEASEDC)
           GetProcAddress(hGdipLib,"GdipReleaseDC");
       pfnGdipCreateBitmapFromGDIDIB = (LPGDIPCREATEBITMAPFROMGDIDIB)
           GetProcAddress(hGdipLib,"GdipCreateBitmapFromGdiDib");
       pfnGdipDeleteGraphics = (LPGDIPDELETEGRAPHICS)
           GetProcAddress(hGdipLib,"GdipDeleteGraphics");
   }
   ZeroMemory(&ice,sizeof(INITCOMMONCONTROLSEX));
   ice.dwSize = sizeof(INITCOMMONCONTROLSEX);
   ice.dwICC = ICC_WIN95_CLASSES|ICC_INTERNET_CLASSES;
	return InitCommonControlsEx(&ice);
#endif
}
//---------------------------------------------------------------------------
void LApp::ReleaseCapture()
{
#ifndef __WIN32__

#else
   ::ReleaseCapture();
#endif
}
//---------------------------------------------------------------------------
void LApp::SetCapture(HWND hwnd)
{
#ifndef __WIN32__
	//	XGrabPointer(pDisplay,hwnd,TRUE,ButtonPressMask|ButtonReleaseMask|PointerMotionMask
	//,GrabModeAsync,GrabModeAsync,None,None,CurrentTime);
#else
   ::SetCapture(hwnd);
#endif
}

