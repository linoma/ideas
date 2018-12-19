#include "ideastypes.h"
#include "ldlg.h"
#include "gpu.h"

//---------------------------------------------------------------------------
#ifndef __spriteviewerH__
#define __spriteviewerH__

#ifdef _DEBPRO4
//---------------------------------------------------------------------------
class LSpriteViewer : public LDlg
{
public:
   LSpriteViewer();
   ~LSpriteViewer();
   BOOL Show(HWND parent = NULL);
   BOOL Destroy();   
protected:
   LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   int DrawSpriteBitmap(HDC hdc,u8 eraseBk);
   void EraseBackGround(HDC hdc);
   void UpdateSprite(HDC hdc);
   int DrawSprite(u8 nSprite);
   int DrawSpriteInternal(u8 nSprite,u16 *buffer,int inc);
   RECT rcSprite;
   SIZE szBorder;
   HBITMAP hBit,hBitmap;
   BYTE IndexSprite;
   BITMAPINFO BmpInfo;
   u16 *pBuffer;
   RECT rcControl[11];
   int yScroll,xScroll,iScale;
   HDC hdcBitmap;
   LGPU *pGpu;
};

#endif
#endif











