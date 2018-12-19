#include "ideastypes.h"
//---------------------------------------------------------------------------
#ifndef __LCANVASH__
#define __LCANVASH__
//---------------------------------------------------------------------------
class LCanvas
{
public:
   LCanvas();
   virtual ~LCanvas();
   inline HBITMAP Bitmap(){return hBitmap;};
   int Init(HWND hwnd);
   inline HDC DC(){return hDC;};
   inline HDC I_DC(){return hDC1;};
   inline char* get_OutBuffer(){return tempBuffer;};
   inline char* get_Buffer(){return screen;};
   void Reset();
   void set_OutBuffer(char *buf = NULL);
	virtual int BitBlt(HDC hdc = NULL,char *buffer = NULL);
	void Destroy();
	void SetOrg(int x,int y);
   void set_WorkArea(RECT rc,SIZE sz);
   virtual void get_WorkArea(RECT &rc,SIZE &sz);
   inline int get_Width(){return rcDisplay.right;};
   inline int get_Height(){return rcDisplay.bottom;};
   void set_Width(int value){rcDisplay.right = value;rcDisplay.left = (256 - value) >> 1;};
   void set_Height(int value){rcDisplay.bottom = value;rcDisplay.top = (192 - value) >> 1;};
   void set_Size(int x,int y){set_Height(y);set_Width(x);};
protected:
	int GetDC(HWND hwnd);
	void ReleaseDC();
	HWND g_hWnd;
	HBITMAP hBitmap;
	HDC hDC,hDC1;
	char *screen,*tempBuffer;
   bool bStretch;
   RECT rcDest,rcDisplay;
   SIZE szSource;
#ifdef __WIN32__
	BITMAPINFO bminfo;
   LPVOID graphics,image;
#else
	POINT ptOrg;
#endif	
};
#endif

