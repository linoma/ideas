#include "ideastypes.h"
#include "ldlg.h"
#include "dstype.h"

//---------------------------------------------------------------------------
#ifndef __paletteviewerH__
#define __paletteviewerH__
//---------------------------------------------------------------------------
class LPaletteViewer : public LDlg
{
public:
   LPaletteViewer();
   ~LPaletteViewer();
   BOOL Show(HWND parent = NULL);
#ifdef _DEBPRO3
   BOOL SavePalette();
#endif
protected:
   LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   void DrawPalette(HDC hdc = NULL);
   void DrawColorPicker(HDC hdc = NULL);
   void OnChangeSelComboBox2();
   void OnChangeSelComboBox1();
   void OnChangeSelComboBox3();
   void OnChangeSelComboBox4();
   void OnChangePalettes();
   void OnMouseMove(int x,int y,WPARAM wParam);
   BOOL OnInitDialog();
   int colorPicker;
   RECT rcColorDraw,rcColorPicker;
   BOOL bConvert;
   u16 *lpPalette;
};

#endif


