#include "ideastypes.h"
#include "dstype.h"
#include "ldlg.h"

//---------------------------------------------------------------------------
#ifndef __tileviewerH__
#define __tileviewerH__
//---------------------------------------------------------------------------
#ifdef _DEBPRO5
//---------------------------------------------------------------------------
class LTileViewer : public LDlg
{
public:
   LTileViewer();
   ~LTileViewer();
   BOOL Show(HWND parent = NULL);
   BOOL Destroy();   
protected:
   LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   void EraseBackGround(HDC hdc);
   int DrawTile();
   int DrawTileInternal();
   int DrawTileBitmap(HDC hdc,u8 eraseBk);
   int DrawTileInternal(int tile,u16 *buffer);
   void OnLButtonDown(WPARAM fwKey,int x,int y);
   void SetMaxTiles();
   void UpdatePanel();
   void DrawPanel(HDC hdc,BOOL bErase = TRUE);
   void UpdateTile(HDC hdc);
   void EraseBkGndPanel(HDC hdc);
   void DrawSelectedBorder(BOOL bErase = TRUE);
   RECT rcTile,rcPanel;
   SIZE szBorder;
   HBITMAP hBit,hBitmap,hbPanel;
   BITMAPINFO BmpInfo,biPanel;
   u16 *pBuffer,*pbPanel,wMaxTile;
   int yScroll,xScroll,iScale,TileIndex,xScrollPanel;
   HDC hdcBitmap;
};

#endif

#endif

