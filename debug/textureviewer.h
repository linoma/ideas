#include "ideastypes.h"
#include "ldlg.h"
#include "dstype.h"
//---------------------------------------------------------------------------
#ifndef __textureviewerH__
#define __textureviewerH__
//---------------------------------------------------------------------------
class LTextureViewer : public LDlg
{
public:
   LTextureViewer();
   ~LTextureViewer();
   BOOL Show(HWND parent = NULL);
   BOOL Destroy();   
protected:
   LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
	LRESULT OnEditWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
   static LRESULT CALLBACK EditWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
   int DrawTexture(int index);
   int DrawTextureBitmap(HDC hdc,u8 eraseBk);
   void UpdateTexture(HDC hdc);
   void EraseBackGround(HDC hdc);
   RECT rcTexture;
   SIZE szBorder;
   HBITMAP hBit,hBitmap;
   BITMAPINFO BmpInfo;
   BYTE *pBuffer;
   int yScroll,xScroll,iScale,indexTex,texFormat;
   HDC hdcBitmap;
   RECT rcControl[12];
   WNDPROC oldEditWndProc2,oldEditWndProc3;
   DWORD texAddress,texData,dwOptions;
};
#endif
