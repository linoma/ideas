#include "paletteviewer.h"
#include "resource.h"
#include "lds.h"
#include "util.h"
#include "language.h"

#ifdef _DEBUG
//---------------------------------------------------------------------------
LPaletteViewer::LPaletteViewer() : LDlg()
{
	lpPalette = NULL;
}
//---------------------------------------------------------------------------
LPaletteViewer::~LPaletteViewer()
{
}
//---------------------------------------------------------------------------
BOOL LPaletteViewer::Show(HWND parent)
{
   if(m_hWnd == NULL){
       if(!LDlg::Create(langManager.get_CurrentLib(),MAKEINTRESOURCE(IDD_DIALOG4),parent))
           return FALSE;
   }
   else{
       ::BringWindowToTop(m_hWnd);
       ::InvalidateRect(m_hWnd,NULL,TRUE);
       ::UpdateWindow(m_hWnd);
   }
   return TRUE;
}
//---------------------------------------------------------------------------
void LPaletteViewer::OnChangeSelComboBox1()
{
   int iSel2,iSel1;

   iSel1 = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_GETCURSEL,0,0);
   iSel2 = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
   if(iSel1 == 0){
       if(iSel2 == 0)
           lpPalette = (u16 *)pal_mem;
       else if(iSel2 == 1)
           lpPalette = (u16  *)(pal_mem + 0x200);
   }
   else if(iSel1 == 1){
       if(iSel2 == 0)
           lpPalette = (u16  *)(pal_mem + 0x400);
       else if(iSel2 == 1)
           lpPalette = (u16  *)(pal_mem + 0x600);
   }
   DrawPalette(NULL);
}
//---------------------------------------------------------------------------
void LPaletteViewer::OnChangeSelComboBox3()
{
	int i,i1;

   i = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
   switch(i){
   	case 3:
			i = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_GETCURSEL,0,0);
			switch(i){
           	case 0://Bank E
               	i1 = 128;
               break;
				default:
               	i1 = 32;
               break;
           }
           SendDlgItemMessage(m_hWnd,IDC_UPDOWN1,UDM_SETRANGE,0,MAKELPARAM(i1,0));
           SendDlgItemMessage(m_hWnd,IDC_UPDOWN1,UDM_SETPOS,0,0);
       break;
       default:
       break;
	}
   OnChangePalettes();
}
//---------------------------------------------------------------------------
void LPaletteViewer::OnChangeSelComboBox4()
{
   OnChangePalettes();
}
//---------------------------------------------------------------------------
void LPaletteViewer::OnChangeSelComboBox2()
{
   int i;

   i = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
   switch(i){
       case 0:
           bConvert = FALSE;
           EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX1),TRUE);
           EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX3),FALSE);
           EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX4),FALSE);
           EnableWindow(GetDlgItem(m_hWnd,IDC_EDIT2),FALSE);
           EnableWindow(GetDlgItem(m_hWnd,IDC_UPDOWN1),FALSE);
       break;
       case 1:
           bConvert = FALSE;
           EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX1),TRUE);
           EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX3),FALSE);
           EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX4),FALSE);
           EnableWindow(GetDlgItem(m_hWnd,IDC_EDIT2),FALSE);
           EnableWindow(GetDlgItem(m_hWnd,IDC_UPDOWN1),FALSE);
       break;
       case 2://Extended
   		::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_RESETCONTENT,0,0);
   		::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Bank E");
   		::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Bank F");
   		::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Bank G");
   		::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Bank H");
   		::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Bank I");
   		::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_SETCURSEL,0,0);

			::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_RESETCONTENT,0,0);
   		::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_ADDSTRING,0,(LPARAM)"Slot 0");
   		::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_ADDSTRING,0,(LPARAM)"Slot 1");
   		::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_ADDSTRING,0,(LPARAM)"Slot 2");
   		::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_ADDSTRING,0,(LPARAM)"Slot 3");
           ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_SETCURSEL,0,0);
           bConvert = FALSE;
           EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX1),FALSE);
           EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX3),TRUE);
           EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX4),TRUE);
           EnableWindow(GetDlgItem(m_hWnd,IDC_EDIT2),FALSE);
           EnableWindow(GetDlgItem(m_hWnd,IDC_UPDOWN1),FALSE);
       break;
       case 3://Textures
   			::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_RESETCONTENT,0,0);
   			::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Bank E");
   			::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Bank F");
   			::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Bank G");
			::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_SETCURSEL,0,0);

           ::SendDlgItemMessage(m_hWnd,IDC_UPDOWN1,UDM_SETRANGE,0,MAKELPARAM(128,0));
           ::SendDlgItemMessage(m_hWnd,IDC_UPDOWN1,UDM_SETPOS,0,0);

           EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX1),FALSE);
           EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX3),TRUE);
           EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX4),FALSE);
           EnableWindow(GetDlgItem(m_hWnd,IDC_EDIT2),TRUE);
           EnableWindow(GetDlgItem(m_hWnd,IDC_UPDOWN1),TRUE);
           bConvert = FALSE;
       break;
   }
   OnChangePalettes();
}
//---------------------------------------------------------------------------
void LPaletteViewer::OnChangePalettes()
{
   int iSel1,iSel2;

   iSel2 = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
   iSel1 = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_GETCURSEL,0,0);
   switch(iSel2){
       case 0:
           bConvert = FALSE;
           if(iSel1 == 0)
               lpPalette = (u16 *)pal_mem;
           else if(iSel1 == 1)
               lpPalette = (u16 *)(pal_mem + 0x400);
       break;
       case 1:
           bConvert = FALSE;
           if(iSel1 == 0)
               lpPalette = (u16 *)(pal_mem + 0x200);
           else if(iSel1 == 1)
               lpPalette = (u16 *)(pal_mem + 0x600);
       break;
       case 2://Extended
           iSel1 = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_GETCURSEL,0,0);
           switch(iSel1){
               case 0:
                   lpPalette = (u16 *)(video_mem + 0x80000);
               break;
               case 1:
                   lpPalette = (u16 *)(video_mem + 0x90000);
               break;
               case 2:
                   lpPalette = (u16 *)(video_mem + 0x94000);
               break;
               case 3:
                   lpPalette = (u16 *)(video_mem + 0x98000);
               break;
               case 4:
                   lpPalette = (u16 *)(video_mem + 0xA0000);
               break;
           }
           iSel1 = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_GETCURSEL,0,0);
           lpPalette += iSel1*0x1000;
       break;
       case 3://Textures
           iSel1 = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_GETCURSEL,0,0);
           switch(iSel1){
               case 0:
                   lpPalette = (u16 *)(video_mem + 0x80000);
               break;
               case 1:
                   lpPalette = (u16 *)(video_mem + 0x90000);
               break;
               case 2:
                   lpPalette = (u16 *)(video_mem + 0x94000);
               break;
           }
           iSel2 = SendDlgItemMessage(m_hWnd,IDC_UPDOWN1,UDM_GETPOS,0,0);
           lpPalette += iSel2*0x100;

       break;
   }
   DrawPalette(NULL);
}
//---------------------------------------------------------------------------
void LPaletteViewer::OnMouseMove(int x,int y,WPARAM wParam)
{
   char s[20];
   POINT pt;
   HDC hdc;
   RECT rc;
   HFONT hFont;
   SIZE sz;

   if((hdc = GetDC(m_hWnd)) == NULL)
       return;
   pt.x=x;pt.y=y;
   *((int *)s) = 0;
   if(PtInRect(&rcColorDraw,pt)){
       int index;

       pt.x -= rcColorDraw.left;
       pt.y -= rcColorDraw.top;
       index = (pt.x >> 4) + ((pt.y >> 4) << 4);
       sprintf(s,"%d",index);
   }
   CopyRect(&rc,&rcColorPicker);
   rc.right = rc.left-10;
   rc.left = rcColorDraw.left;
   hFont = (HFONT)SendMessage(WM_GETFONT,0,0);
   SelectObject(hdc,hFont);

//   ::SetBkMode(hdc,TRANSPARENT);
   SelectObject(hdc,GetSysColorBrush(COLOR_BTNFACE));
   SetBkColor(hdc,GetSysColor(COLOR_BTNFACE));
   GetTextExtentPoint32(hdc,"X",1,&sz);
   ::ExtTextOut(hdc,rc.left,rc.top,ETO_OPAQUE,&rc,s,strlen(s),NULL);
   ::ReleaseDC(m_hWnd,hdc);
}
//---------------------------------------------------------------------------
LRESULT LPaletteViewer::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LRESULT res;
   PAINTSTRUCT ps;
   POINT pt;

   res = FALSE;
   switch(uMsg){
       case WM_INITDIALOG:
           res = OnInitDialog();
       break;
       case WM_MOUSEMOVE:
           OnMouseMove((int)(short)LOWORD(lParam),(int)(short)HIWORD(lParam),wParam);
       break;
       case WM_LBUTTONDOWN:
           pt.x = LOWORD(lParam);
           pt.y = HIWORD(lParam);
           if(PtInRect(&rcColorDraw,pt)){
               pt.x -= rcColorDraw.left;
               pt.y -= rcColorDraw.top;
               pt.x >>= 4;
               pt.y = (pt.y >> 4) << 4;
               colorPicker = lpPalette[pt.y + pt.x];
               DrawColorPicker();
               res = TRUE;
           }
       break;
       case WM_COMMAND:
           switch(LOWORD(wParam)){
               case IDC_COMBOBOX1:
                   if(HIWORD(wParam) == CBN_SELENDOK)
                       OnChangeSelComboBox1();
               break;
               case IDC_COMBOBOX2:
                   if(HIWORD(wParam) == CBN_SELENDOK)
                       OnChangeSelComboBox2();
               break;
               case IDC_COMBOBOX3://Banks
                   if(HIWORD(wParam) == CBN_SELENDOK)
                       OnChangeSelComboBox3();
               break;
               case IDC_COMBOBOX4://Slots
                   if(HIWORD(wParam) == CBN_SELENDOK)
                       OnChangeSelComboBox4();
               break;
               case IDOK:
                   Destroy();
               break;
#ifdef _DEBPRO3
				case IDC_BUTTON2:
					SavePalette();
               break;
#endif
           }
       break;
       case WM_VSCROLL:
           if(GetDlgCtrlID((HWND)lParam) == IDC_UPDOWN1){
               OnChangePalettes();
       		res  = 0;
           }
       break;
       case WM_PAINT:
           ::BeginPaint(m_hWnd,&ps);
           DrawPalette(ps.hdc);
           DrawColorPicker(ps.hdc);
           ::EndPaint(m_hWnd,&ps);
           res = TRUE;
       break;
       case WM_CLOSE:
           Destroy();
       break;
   }
   return res;
}
//---------------------------------------------------------------------------
#ifdef _DEBPRO3
BOOL LPaletteViewer::SavePalette()
{
	LFile *pFile;
	char s[501];
   u8 i,i1,x,y;
   u16 x1,y1,x2;
   u16 color;
   u16 *palette;

	*((int *)s) = 0;
   if(!ShowSaveDialog(NULL,s,"Palette Files (*.pal)\0*.pal\0All files (*.*)\0*.*\0\0\0\0\0",NULL))
   	return FALSE;
   if(lstrlen(s) == 0)
   	return FALSE;
	pFile = new LFile(LString(s).AddEXT(".pal").c_str());
   if(pFile == NULL)
   	return FALSE;
   if(!pFile->Open(GENERIC_WRITE,CREATE_ALWAYS)){
   	delete pFile;
       return FALSE;
   }
	lstrcpy(s,"JASC-PAL\r\n0100\r\n");
	pFile->Write(s,lstrlen(s));
   wsprintf(s,"%04d\r\n",256);
	pFile->Write(s,lstrlen(s));
	palette = lpPalette;
   for(i=0;i<16;i++){
       x1 = x2;
       for(i1=0;i1<16;i1++){
           color = (u16)(*palette++);
           if(!bConvert)
               color = (u16)(((color & 0x1F) << 10) | (color & 0x3e0) | ((color >> 10) & 0x1F));
           wsprintf(s,"%d %d %d\r\n",(int)(((color >> 10) & 0x1F) * 8.22f),
           	(int)(((color >> 5) & 0x1F) * 8.22f),(int)((color & 0x1F) * 8.22f));
           pFile->Write(s,lstrlen(s));
           x1 += x;
       }
       y1 += y;
   }
  	delete pFile;
   return TRUE;
}
#endif
//---------------------------------------------------------------------------
BOOL LPaletteViewer::OnInitDialog()
{
   POINT pt;
   LONG baseUnits;
   u8 x,y;
   int i;
   RECT rc;

   colorPicker = -1;
   bConvert = FALSE;
   baseUnits = GetDialogBaseUnits();
   GetClientRect(&rc);
   x = (u8)((LOWORD(baseUnits) >> 2) << 3);
   i = rc.right - (x << 4) - 5;
   y = (u8)((HIWORD(baseUnits) >> 3) << 3);
   ::SetRect(&rcColorDraw,i,10,i+(x << 4),10 + (y << 4));
   pt.x = (((rcColorDraw.right - rcColorDraw.left) - 60) >> 1) + rcColorDraw.left;
   pt.y = rcColorDraw.bottom + 10;
   ::SetRect(&rcColorPicker,pt.x,pt.y,pt.x+60,pt.y+40);
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"Main");
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"Sub");
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_SETCURSEL,0,0);
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"Background");
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"Sprite");
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"Extended");
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"Textures");
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_SETCURSEL,0,0);
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Bank E");
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Bank F");
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Bank G");
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Bank H");
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Bank I");
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_SETCURSEL,0,-1);
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_ADDSTRING,0,(LPARAM)"Slot 0");
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_ADDSTRING,0,(LPARAM)"Slot 1");
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_ADDSTRING,0,(LPARAM)"Slot 2");
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_ADDSTRING,0,(LPARAM)"Slot 3");
   ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_SETCURSEL,0,-1);
   lpPalette = (u16 *)pal_mem;
   return TRUE;
}
//---------------------------------------------------------------------------
void LPaletteViewer::DrawColorPicker(HDC hdc)
{
   RECT rc;
   HBRUSH hBrush;
   COLORREF col;
   char s[50];
   HFONT hFont;
   SIZE sz;
   int y,x;
   u8 flag;

   if(hdc == NULL){
       hdc = GetDC(m_hWnd);
       flag = 1;
   }
   else
       flag = 0;
   CopyRect(&rc,&rcColorPicker);
   InflateRect(&rc,GetSystemMetrics(SM_CXBORDER) << 1,GetSystemMetrics(SM_CYBORDER) << 1);
   DrawEdge(hdc,&rc,EDGE_SUNKEN,BF_RECT);
   OffsetRect(&rc,rc.right-rc.left+5,0);
   FillRect(hdc,&rc,(HBRUSH)GetSysColorBrush(COLOR_3DFACE));
   if(colorPicker == -1)
   	col = 0;
   else{
       bConvert = FALSE;
       if(!bConvert)
           col = ((colorPicker & 0x1F) << 10) | (colorPicker & 0x3e0) | ((colorPicker >> 10) & 0x1F);
       else
           col  = colorPicker;
       col = RGB(((col >> 10) & 0x1F) * 8.22f,((col >> 5) & 0x1F) * 8.22f,(col & 0x1F) * 8.22f);
   }
   CopyRect(&rc,&rcColorPicker);
   if((hBrush = CreateSolidBrush(col)) != NULL){
       FillRect(hdc,&rc,hBrush);
       ::DeleteObject(hBrush);
   }
   if(colorPicker == -1)
       goto Ex_DrawColorPicker;
   hFont = (HFONT)SendMessage(WM_GETFONT,0,0);
   SelectObject(hdc,hFont);
   ::SetBkMode(hdc,TRANSPARENT);
   GetTextExtentPoint32(hdc,"X",1,&sz);
   x = rc.right + 10;
	y = rc.top;// + sz.cy * 2;
   wsprintf(s,"B : 0x%02X",GetBValue(col));
   TextOut(hdc,x,y,s,lstrlen(s));
	y += sz.cy;
   wsprintf(s,"G : 0x%02X",GetGValue(col));
   TextOut(hdc,x,y,s,lstrlen(s));
	y += sz.cy;
   wsprintf(s,"R : 0x%02X",GetRValue(col));
   TextOut(hdc,x,y,s,lstrlen(s));
Ex_DrawColorPicker:
   if(flag != 0)
       ::ReleaseDC(m_hWnd,hdc);
}
//---------------------------------------------------------------------------
void LPaletteViewer::DrawPalette(HDC hdc)
{
   u8 i,i1,x,y;
   u16 x1,y1,x2;
   HBRUSH hBrush;
   COLORREF rgb;
   u16 color;
   RECT rc;
   LONG baseUnits;
   u16 *palette;
   u8 flag;

   if(lpPalette == NULL)
		return;
	if(hdc == NULL){
       hdc = GetDC(m_hWnd);
       flag = 1;
   }
   else
       flag = 0;
	baseUnits = GetDialogBaseUnits();
   x = (u8)((LOWORD(baseUnits) >> 2) << 3);
   y = (u8)((HIWORD(baseUnits) >> 3) << 3);
   ::CopyRect(&rc,&rcColorDraw);
   y1 = (u16)rc.top;
   x2 = (u16)rc.left;
   InflateRect(&rc,GetSystemMetrics(SM_CXBORDER)<<1,GetSystemMetrics(SM_CYBORDER)<<1);
   DrawEdge(hdc,&rc,EDGE_SUNKEN,BF_RECT);
   palette = lpPalette;
   for(i=0;i<16;i++){
       x1 = x2;
       for(i1=0;i1<16;i1++){
           color = (u16)(*palette++);
           if(!bConvert)
               color = (u16)(((color & 0x1F) << 10) | (color & 0x3e0) | ((color >> 10) & 0x1F));
           rgb = RGB(((color >> 10) & 0x1F) * 8.22f,((color >> 5) & 0x1F) * 8.22f,(color & 0x1F) * 8.22f);
           hBrush = ::CreateSolidBrush(rgb);
           ::SetRect(&rc,x1,y1,x1 + x,y1+y);
           ::FillRect(hdc,&rc,hBrush);
           ::DeleteObject(hBrush);
           x1 += x;
       }
       y1 += y;
   }
   if(flag && hdc != NULL)
       ReleaseDC(m_hWnd,hdc);
}
#endif
