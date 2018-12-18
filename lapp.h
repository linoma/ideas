#include "ideastypes.h"
#include "lslist.h"

#ifndef __lapph__
#define __lapph__
//----------------------------------------------------------------------------------
class LApp
{
public:
	LApp();
	virtual ~LApp();
	BOOL GetMessage(LPMSG lpMsg);
	BOOL PeekMessage(LPMSG lpMsg);
	BOOL WaitMessage();
    BOOL TranslateMessage(LPMSG lpMsg);
	virtual LONG DispatchMessage(LPMSG lpMsg);
	virtual BOOL Init(int argc,char **argv);
	void ReleaseCapture();
	void SetCapture(HWND hwnd);
	HWND WindowFromPoint(int x,int y);	
	void ProcessMessages();
	COLORREF GetSysColor(INT color);
	virtual void PostQuitMessage();
	IMenu *CoCreateInstance_Menu(HMENU menu);
   inline BOOL CanRun(){return bQuit;};
protected:
#ifdef __WIN32__
   LPGDIPSTARTUP pfnGdipStartup;
   LPGDIPSHUTDOWN pfnGdipShutDown;
   ULONG token;
   HINSTANCE hGdipLib;
#endif
   BOOL bQuit;
};
//----------------------------------------------------------------------------------
extern LApp *pApp;
//----------------------------------------------------------------------------------
#endif

