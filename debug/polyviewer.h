#include "ideastypes.h"
#include "ldlg.h"
#include "dstype.h"

#ifdef _DEBPRO
//---------------------------------------------------------------------------
#ifndef __polyviewerH__
#define __polyviewerH__
//---------------------------------------------------------------------------
class LPolygonViewer : public LDlg
{
public:
   LPolygonViewer();
   ~LPolygonViewer();
   BOOL Show(HWND parent = NULL);
   void Reset();
   LRESULT OnWindowProcPolygonViewer(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
protected:
   LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   void OnNotifyTreeView1(LPNM_TREEVIEW hdr);
   static LRESULT WindowProcPolygonViewer(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void OnPolygonViewerMouseMove(WORD wKeys,int x,int y);
	void OnPolygonViewerMouseWheel(WORD wKeys,short wWheel,int x,int y);
   void OnInitDialog();
   void UpdatePolygon();
   void DrawPolygon();
   void UpdateTransformation(int mode = -1);
   void UpdateScaleTransMultiplier();
   BOOL Destroy();
   HGLRC hRC;
   HDC hdcRender,hdcZoom,hdcWindow;
   HBITMAP hbRender,hbZoom;
   DWORD *pBuffer;
   u64 dwOptions;
   WORD wIndex;
   BITMAPINFO bmi;
   int yScroll,xScroll,iScale,nSwap,nScale[3],nTrans[3],nPos[3],nRotate[3],nMouseMode;
   WNDPROC lpPrevPolyViewerProc;
   float fTrans[3],fScale[3],fRotate[3];
   HIMAGELIST imageLists[2];
};
#endif


#endif //_DEBPRO

