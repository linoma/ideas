#include "tileviewer.h"
#include "resource.h"
#include "lds.h"
#include "util.h"
#include <math.h>

#ifdef _DEBPRO5
static u16 bankTiles[] = {4096,4096,4096,4096,2048,512,512,1024,512};
//---------------------------------------------------------------------------
LTileViewer::LTileViewer() : LDlg()
{
   hbPanel = hBitmap = NULL;
   hdcBitmap = NULL;
   hBit = NULL;
   TileIndex = 0;
   wMaxTile = 64;
   xScrollPanel = 0;
   ZeroMemory(&biPanel,sizeof(BITMAPINFO));
}
//---------------------------------------------------------------------------
LTileViewer::~LTileViewer()
{
   Destroy();
}
//---------------------------------------------------------------------------
BOOL LTileViewer::Show(HWND parent)
{
   if(m_hWnd == NULL){
       if(!LDlg::Create(hInst,MAKEINTRESOURCE(IDD_DIALOG18),parent))
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
void LTileViewer::SetMaxTiles()
{
   if(SendDlgItemMessage(m_hWnd,IDC_RADIO2,BM_GETCHECK,0,0) == BST_CHECKED)
       wMaxTile = 128;
   else if(SendDlgItemMessage(m_hWnd,IDC_RADIO1,BM_GETCHECK,0,0) == BST_CHECKED)
       wMaxTile = bankTiles[SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_GETCURSEL,0,0)];
   if(SendDlgItemMessage(m_hWnd,IDC_RADIO3,BM_GETCHECK,0,0) != BST_CHECKED)
       wMaxTile >>= 1;
}
//---------------------------------------------------------------------------
void LTileViewer::UpdatePanel()
{
   int cx,cy,i,x,y,tile,x1,y1,i1;
   HWND hwnd;
   SCROLLINFO si={0};
   u16 buffer[64],*p;
   RECT rc;
   BITMAP bm;

   i = wMaxTile >> 1;
   if(i > 256){
       cx = 256;
       cy = 128;
   }
   else if(i < 128){
       cy = i;
       i = cx = 128;
   }
   else{
       cx = i;
       cy = 128;
   }
   i += (i >> 3) + 1;
   cx += (cx >> 3) + 1;
   cy += (cy >> 3) + 1;
   rcPanel.right = rcPanel.left + cx;
   rcPanel.bottom = rcPanel.top + cy;
   if(hbPanel == NULL || i != biPanel.bmiHeader.biWidth || cy != abs(biPanel.bmiHeader.biHeight)){
       ZeroMemory(&biPanel,sizeof(BITMAPINFO));
	    biPanel.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	    biPanel.bmiHeader.biBitCount    = 16;
	    biPanel.bmiHeader.biWidth       = i;
	    biPanel.bmiHeader.biHeight      = -cy;
	    biPanel.bmiHeader.biPlanes      = 1;
	    biPanel.bmiHeader.biCompression = BI_RGB;
       hbPanel = ::CreateDIBSection(NULL,&biPanel,DIB_RGB_COLORS,(VOID**)&pbPanel,NULL,0);
       if(hbPanel == NULL)
           return;
       xScrollPanel = 0;
       TileIndex = 0;
   }
   GetObject(hbPanel,sizeof(BITMAP),&bm);
   ZeroMemory(pbPanel,bm.bmWidthBytes*cy);
   for(tile = y = 0;y < cy-1;y += 9){
       for(x=0;x < i-1;x += 9){
           DrawTileInternal(tile++,buffer);
           for(i1=0,y1=1;y1 < 9;y1++){
               p = pbPanel + (((y + y1) * bm.bmWidthBytes >> 1) + x + 1);
               for(x1=1;x1 < 9;x1++,i1++)
                   *p++ = buffer[i1];
           }
       }
   }
   GetWindowRect(&rc);
   y = rc.bottom - rc.top;
   ::GetWindowRect(GetDlgItem(m_hWnd,IDC_LABEL_TILE),&rc);
   MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,2);
   if(rcPanel.right > rc.right)
       x = rcPanel.right + 15;
   else
       x = rc.right + 15;
   GetWindowRect(&rc);
   if(x != (rc.right - rc.left)){
       if(x < (rc.right - rc.left))
           InvalidateRect(m_hWnd,NULL,TRUE);
       ::SetWindowPos(m_hWnd,0,0,0,x,y,SWP_NOREPOSITION|SWP_NOMOVE|SWP_FRAMECHANGED|SWP_DRAWFRAME);
   }
   if(xScrollPanel == 0){
       if(i > cx){
           hwnd = GetDlgItem(m_hWnd,IDC_SCROLLBAR4);
           si.cbSize = sizeof(SCROLLINFO);
           si.fMask = SIF_ALL;
           si.nMax = i-1;
           si.nPage = cx;
           si.nPos = 0;
           SetScrollInfo(hwnd,SB_CTL,&si,FALSE);
           ::GetWindowRect(hwnd,&rc);
           MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,2);
           ::SetWindowPos(hwnd,0,rcPanel.left-szBorder.cx,rcPanel.bottom+szBorder.cy,rcPanel.right-rcPanel.left+szBorder.cx*2,rc.bottom-rc.top,SWP_NOREPOSITION|SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_DRAWFRAME);
       }
       else
           ShowScrollBar(GetDlgItem(m_hWnd,IDC_SCROLLBAR4),SB_CTL,FALSE);
   }
   DrawTile();
   DrawSelectedBorder(FALSE);
   ::SetRect(&rc,rcPanel.left,rcPanel.top,rcPanel.left+256,rcPanel.top+128);
   InflateRect(&rc,szBorder.cx,szBorder.cy);   
   InvalidateRect(m_hWnd,&rc,TRUE);
}
//---------------------------------------------------------------------------
void LTileViewer::DrawPanel(HDC hdc,BOOL bErase)
{
   BOOL bFlag;

   if(hbPanel == NULL)
       return;
   if(hdc == NULL){
       bFlag = TRUE;
       if((hdc = GetDC(m_hWnd)) == NULL)
           return;
   }
   else
       bFlag = FALSE;
   if(bErase)
       EraseBkGndPanel(hdc);
   ::StretchDIBits(hdc,rcPanel.left,rcPanel.top,(rcPanel.right - rcPanel.left),(rcPanel.bottom-rcPanel.top)
       ,xScrollPanel,0,(rcPanel.right - rcPanel.left),(rcPanel.bottom-rcPanel.top),
       pbPanel,&biPanel,DIB_RGB_COLORS,SRCCOPY);
   if(bFlag)
       ReleaseDC(m_hWnd,hdc);
}
//---------------------------------------------------------------------------
void LTileViewer::OnLButtonDown(WPARAM fwKey,int x,int y)
{
   POINT pt={x,y};

   if(!PtInRect(&rcPanel,pt))
       return;
   y -= rcPanel.top + 1;
   x -= rcPanel.left + 1;
   y = (y / 9) * ((biPanel.bmiHeader.biWidth - 1) / 9);
   TileIndex = y + ((x+xScrollPanel)/9);
   DrawSelectedBorder();
   DrawTile();
   UpdateTile(NULL);
}
//---------------------------------------------------------------------------
void LTileViewer::UpdateTile(HDC hdc)
{
   u8 flag;
   RECT rc,rc1,rc2;
   int i;

   if(hdc == NULL){
       hdc = GetDC(m_hWnd);
       flag = 1;
   }
   else
       flag = 0;
   ::EnableWindow(GetDlgItem(m_hWnd,IDC_BUTTON2),FALSE);
   CopyRect(&rc,&rcTile);
   EraseBackGround(hdc);
   if(hBit != NULL){
       xScroll = yScroll = 0;
       ::SetRect(&rc1,0,0,8,8);
       SendDlgItemMessage(m_hWnd,IDC_TRACK2,TBM_SETPOS,TRUE,0);
       iScale = SendDlgItemMessage(m_hWnd,IDC_TRACK2,TBM_GETRANGEMAX,0,0);
       rc2.left = rc.left + ((rc.right-rc.left - rc2.right) >> 1);
       rc2.top = rc.top + ((rc.bottom - rc.top - rc2.bottom) >> 1);
       if(hdcBitmap != NULL)
           ::DeleteDC(hdcBitmap);
       hdcBitmap = NULL;
       if(hBitmap != NULL)
           DeleteObject(hBitmap);
       hBitmap = NULL;
       if((hdcBitmap = CreateCompatibleDC(hdc)) != NULL){
           i = iScale;
           if((hBitmap = CreateCompatibleBitmap(hdc,8 * i,8 * i)) != NULL){
               DrawTileBitmap(hdc,FALSE);
               ::EnableWindow(GetDlgItem(m_hWnd,IDC_BUTTON2),TRUE);
           }
       }
   }
   if(flag != 0)
       ReleaseDC(m_hWnd,hdc);
}
//---------------------------------------------------------------------------
int LTileViewer::DrawTileBitmap(HDC hdc,u8 eraseBk)
{
   RECT rc,rc1,rc2;
   int i;
   u8 flag;
   s8 flagScale;
   HWND hwnd;
   SCROLLINFO si;
   u8 res;
   SIZE sz;

   res = 1;
   if(hdc == NULL){
       hdc = GetDC(m_hWnd);
       flag = 1;
   }
   else
       flag = 0;
   ::CopyRect(&rc,&rcTile);
   if(hdcBitmap == NULL || hBitmap == NULL){
       res = 0;
       EraseBackGround(hdc);
       goto Ex_DrawTileBitmap;
   }
   ::SetRect(&rc1,0,0,8,8);
   sz.cx = rc1.right;
   sz.cy = rc1.bottom;
   i = SendDlgItemMessage(m_hWnd,IDC_TRACK2,TBM_GETPOS,0,0);
   if(i != iScale){
       flagScale = (u8)(iScale > i ? -1 : 1);
       iScale = i;
   }
   else
       flagScale = 0;
   if(i > 0){
       rc1.right *= i;
       rc1.bottom *= i;
   }
   CopyRect(&rc2,&rc1);
   hwnd = GetDlgItem(m_hWnd,IDC_SCROLLBAR1);
   if(rc1.bottom > (i = rc.bottom - rc.top)){
       if(flagScale != 0){
           ZeroMemory(&si,sizeof(SCROLLINFO));
           si.cbSize = sizeof(SCROLLINFO);
           si.fMask = SIF_ALL;
           si.nMax = rc2.bottom-1;
           si.nPage = i;
           if(flagScale < 0)
               yScroll = 0;
           si.nPos = yScroll;
           ShowScrollBar(hwnd,SB_CTL,TRUE);
           SetScrollInfo(hwnd,SB_CTL,&si,TRUE);
       }
       rc2.bottom = i;
   }
   else if(flagScale != 0)
       ShowScrollBar(hwnd,SB_CTL,FALSE);
   hwnd = GetDlgItem(m_hWnd,IDC_SCROLLBAR2);
   if(rc1.right > (i = rc.right - rc.left)){
       if(flagScale != 0){
           ZeroMemory(&si,sizeof(SCROLLINFO));
           si.cbSize = sizeof(SCROLLINFO);
           si.fMask = SIF_ALL;
           si.nMax = rc2.right-1;
           si.nPage = i;
           if(flagScale < 0)
               xScroll = 0;
           si.nPos = xScroll;
           ShowScrollBar(hwnd,SB_CTL,TRUE);
           SetScrollInfo(hwnd,SB_CTL,&si,TRUE);
       }
       rc2.right = i;
   }
   else if(flagScale != 0)
       ShowScrollBar(hwnd,SB_CTL,FALSE);
   rc2.left = rc.left + ((rc.right-rc.left - rc2.right) >> 1);
   rc2.top = rc.top + ((rc.bottom - rc.top - rc2.bottom) >> 1);
   if(flagScale != 0 || eraseBk != 0)
       EraseBackGround(hdc);
   ::SelectObject(hdcBitmap,hBitmap);
   ::StretchDIBits(hdcBitmap,0,0,rc1.right,rc1.bottom,0,0,sz.cx,sz.cy,pBuffer,&BmpInfo,DIB_RGB_COLORS,SRCCOPY);
   ::BitBlt(hdc,rc2.left,rc2.top,rc2.right,rc2.bottom,hdcBitmap,xScroll,yScroll,SRCCOPY);
Ex_DrawTileBitmap:
   if(flag != 0)
       ReleaseDC(m_hWnd,hdc);
   return res;
}
//---------------------------------------------------------------------------
int LTileViewer::DrawTileInternal(int tile,u16 *buffer)
{
   u16 *palData;
   u8 *tileSrc,palMode,col;
   u32 offset;
   int i;
   BOOL bTransparent;

   bTransparent = SendDlgItemMessage(m_hWnd,IDC_CHECK1,BM_GETCHECK,0,0) == BST_CHECKED ? TRUE : FALSE;
   if(SendDlgItemMessage(m_hWnd,IDC_RADIO3,BM_GETCHECK,0,0) == BST_CHECKED){
       palMode = 0;
       switch(SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_GETCURSEL,0,0)){
           case 0:
               palData = downLcd.tpal_bg;
           break;
           case 1:
               palData = downLcd.tpal_obj;
           break;
           case 2:
               palData = upLcd.tpal_bg;
           break;
           case 3:
               palData = upLcd.tpal_obj;
           break;
       }
       palData += SendDlgItemMessage(m_hWnd,IDC_COMBOBOX8,CB_GETCURSEL,0,0) << 4;
   }
   else if(SendDlgItemMessage(m_hWnd,IDC_RADIO4,BM_GETCHECK,0,0) == BST_CHECKED){
       palMode = 1;
       switch(SendDlgItemMessage(m_hWnd,IDC_COMBOBOX5,CB_GETCURSEL,0,0)){
           case 0:
               palData = downLcd.tpal_bg;
           break;
           case 1:
               palData = downLcd.tpal_obj;
           break;
           case 2:
               palData = upLcd.tpal_bg;
           break;
           case 3:
               palData = upLcd.tpal_obj;
           break;
       }
   }
   else if(SendDlgItemMessage(m_hWnd,IDC_RADIO5,BM_GETCHECK,0,0) == BST_CHECKED){
       palMode = 2;
       switch(SendDlgItemMessage(m_hWnd,IDC_COMBOBOX6,CB_GETCURSEL,0,0)){
           case 0:
               palData = (u16 *)(video_mem + 0x80000);
           break;
           case 1:
               palData = (u16 *)(video_mem + 0x90000);
           break;
           case 2:
               palData = (u16 *)(video_mem + 0x94000);
           break;
           case 3:
               palData = (u16 *)(video_mem + 0x98000);
           break;
       }
       palData += SendDlgItemMessage(m_hWnd,IDC_COMBOBOX9,CB_GETCURSEL,0,0) << 12;
       palData += SendDlgItemMessage(m_hWnd,IDC_COMBOBOX7,CB_GETCURSEL,0,0) << 8;
   }
   if(SendDlgItemMessage(m_hWnd,IDC_RADIO2,BM_GETCHECK,0,0) == BST_CHECKED){
       offset = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_GETCURSEL,0,0) << 16;
       offset += SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_GETCURSEL,0,0) << 12;
   }
   else if(SendDlgItemMessage(m_hWnd,IDC_RADIO1,BM_GETCHECK,0,0) == BST_CHECKED){
       switch((i = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_GETCURSEL,0,0))){
           default:
               offset = i << 17;
           break;
           case 5:
               offset = (4 << 17) + 0x10000;
           break;
           case 6:
               offset = (4 << 17) + 0x14000;
           break;
           case 7:
               offset = (4 << 17) + 0x18000;
           break;
           case 8:
               offset = (4 << 17) + 0x1C000;
           break;
           case 9:
               offset = (4 << 17) + 0x1C000 + 0x8000;
           break;
       }
   }
   tileSrc = video_mem + offset;
   switch(palMode){
       case 0:
           tileSrc += tile << 5;
           for(i=0;i<32;i++){
               if((col = (u8)(tileSrc[i]& 0xF)) == 0 && bTransparent)
                   buffer[i+i] = 0;
               else
                   buffer[i+i] = palData[col];
               if((col = (u8)((tileSrc[i]>>4)&0xF)) == 0 && bTransparent)
                   buffer[i+i+1] = 0;
               else
				    buffer[i+i+1] = palData[col];
           }
       break;
       case 1:
           tileSrc += tile << 6;
           for(i=0;i<64;i++){
               if((col = tileSrc[i]) == 0 && bTransparent)
                   buffer[i] = 0;
               else
		            buffer[i] = palData[col];
           }
       break;
       case 2:
           tileSrc += tile << 6;
           for(i=0;i<64;i++){
               if((col = tileSrc[i]) == 0 && bTransparent)
                   buffer[i] = 0;
               else
		            buffer[i] = BGR(palData[col]);
           }
       break;
   }
   return 1;
}
//---------------------------------------------------------------------------
void LTileViewer::DrawSelectedBorder(BOOL bErase)
{
   int x,y,i;
   u16 *p;
   BITMAP bm;

   ::GetObject(hbPanel,sizeof(BITMAP),&bm);
   i = biPanel.bmiHeader.biWidth - 1;
   y = (TileIndex * 9) / i;
   x = TileIndex - (y * i / 9);
   if(bErase)
       UpdatePanel();
   p = pbPanel + x * 9 + y * 9 * (bm.bmWidthBytes >> 1);
   for(i = 0;i<9;i++)
       p[i] = 32767;
   for(i = 0;i < 9*(bm.bmWidthBytes >> 1);i += (bm.bmWidthBytes >> 1))
       p[i] = 32767;
   for(i = 0;i<9*(bm.bmWidthBytes >> 1);i+=(bm.bmWidthBytes >> 1))
       p[i+9] = 32767;
   for(i = 9*(bm.bmWidthBytes >> 1);i< 9*(bm.bmWidthBytes >> 1)+10;i++)
       p[i] = 32767;
   DrawPanel(NULL);
}
//---------------------------------------------------------------------------
int LTileViewer::DrawTile()
{
   char s[10];

   if(hBit == NULL){
       ZeroMemory(&BmpInfo,sizeof(BITMAPINFO));
	    BmpInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	    BmpInfo.bmiHeader.biBitCount    = 16;
	    BmpInfo.bmiHeader.biWidth       = 8;
	    BmpInfo.bmiHeader.biHeight      = -8;
	    BmpInfo.bmiHeader.biPlanes      = 1;
	    BmpInfo.bmiHeader.biCompression = BI_RGB;
       hBit = ::CreateDIBSection(NULL,&BmpInfo,DIB_RGB_COLORS,(VOID**)&pBuffer,NULL,0);
       if(hBit == NULL)
           return -1;
   }
   ZeroMemory(pBuffer,64 * (BmpInfo.bmiHeader.biBitCount >> 3));
   if(!DrawTileInternal(TileIndex,pBuffer))
       return 0;
   wsprintf(s,"%d",TileIndex);
   SendDlgItemMessage(m_hWnd,IDC_LABEL_TILE,WM_SETTEXT,0,(LPARAM)s);
}
//---------------------------------------------------------------------------
LRESULT LTileViewer::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   BOOL res;
   RECT rc;
   POINT pt;
   HWND hwnd;
   int i;
   PAINTSTRUCT ps;
   char s[10];
   LString fileName;

   res = FALSE;
   switch(uMsg){
       case WM_CTLCOLORSTATIC:
   		if(GetDlgCtrlID((HWND)lParam) == IDC_LABEL_TILE){
       		SetTextColor((HDC)wParam,GetSysColor(COLOR_WINDOWTEXT));
	           	return (BOOL)GetSysColorBrush(COLOR_WINDOW);
       	}
       break;
       case WM_INITDIALOG:
           ::GetWindowRect(GetDlgItem(m_hWnd,IDC_TILE),&rc);
           ::DestroyWindow(GetDlgItem(m_hWnd,IDC_TILE));
           pt.x = rc.left;
           pt.y = rc.top;
           ::ScreenToClient(m_hWnd,&pt);
           ::SetRect(&rcTile,pt.x,pt.y,pt.x+64,pt.y+64);
           pt.x = ::GetSystemMetrics(SM_CXEDGE);
           pt.y = ::GetSystemMetrics(SM_CYEDGE);
           rcTile.right += pt.x;
           rcTile.bottom +=pt.y;
           rcTile.left -= pt.x;
           rcTile.top -= pt.y;
           szBorder.cx = pt.x;
           szBorder.cy = pt.y;
           ::GetWindowRect(GetDlgItem(m_hWnd,IDC_PANEL),&rc);
           ::DestroyWindow(GetDlgItem(m_hWnd,IDC_PANEL));
           MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,1);
           ::SetRect(&rcPanel,rc.left,rc.top,0,0);
           hwnd = GetDlgItem(m_hWnd,IDC_SCROLLBAR1);
           ::GetWindowRect(hwnd,&rc);
           i = rc.right - rc.left;
           ::SetWindowPos(hwnd,NULL,rcTile.right + 2,rcTile.top - pt.y,i,
               rcTile.bottom - rcTile.top + pt.y,SWP_NOREPOSITION|SWP_NOSENDCHANGING|SWP_HIDEWINDOW);
           hwnd = GetDlgItem(m_hWnd,IDC_SCROLLBAR2);
           ::GetWindowRect(hwnd,&rc);
           ::SetWindowPos(hwnd,NULL,rcTile.left - pt.x,rcTile.bottom + 2,rcTile.right - rcTile.left + pt.x,
               i,SWP_NOREPOSITION|SWP_NOSENDCHANGING|SWP_HIDEWINDOW);
           i += rcTile.bottom + 3;
           hwnd = GetDlgItem(m_hWnd,IDC_TRACK2);
           ::GetWindowRect(hwnd,&rc);
           pt.x = rcTile.left + (((rcTile.right - rcTile.left) - (rc.right - rc.left)) >> 1);
           ::SetWindowPos(hwnd,NULL,pt.x,i,0,0,SWP_NOREPOSITION|SWP_NOSIZE|SWP_NOSENDCHANGING);
           ::SendMessage(hwnd,TBM_SETRANGE,0,MAKELPARAM(0,20));
           ::SendMessage(hwnd,TBM_SETPOS,TRUE,0);
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"A");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"B");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"C");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"D");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"E");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"F");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"G");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"H");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"I");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_SETCURSEL,0,0);
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"0");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"0x10000");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"0x20000");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"0x30000");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"0x40000");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"0x50000");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"0x60000");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"0x70000");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"0x80000");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"0x90000");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_SETCURSEL,0,0);
           for(i=0;i<16;i++){
               wsprintf(s,"%d",i);
               SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)s);
               if(i < 4)
                   SendDlgItemMessage(m_hWnd,IDC_COMBOBOX9,CB_ADDSTRING,0,(LPARAM)s);
               wsprintf(s,"Palette %d",i);
               SendDlgItemMessage(m_hWnd,IDC_COMBOBOX8,CB_ADDSTRING,0,(LPARAM)s);
               SendDlgItemMessage(m_hWnd,IDC_COMBOBOX7,CB_ADDSTRING,0,(LPARAM)s);
           }
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX3,CB_SETCURSEL,0,0);
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX9,CB_SETCURSEL,0,0);
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX8,CB_SETCURSEL,0,0);
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX7,CB_SETCURSEL,0,0);
           SendDlgItemMessage(m_hWnd,IDC_RADIO2,BM_SETCHECK,BST_CHECKED,0);
           SendDlgItemMessage(m_hWnd,IDC_RADIO4,BM_SETCHECK,BST_CHECKED,0);
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX6,CB_ADDSTRING,0,(LPARAM)"Bank E");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX6,CB_ADDSTRING,0,(LPARAM)"Bank F");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX6,CB_ADDSTRING,0,(LPARAM)"Bank G");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX6,CB_ADDSTRING,0,(LPARAM)"Bank H");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX6,CB_SETCURSEL,0,0);
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_ADDSTRING,0,(LPARAM)"Main BG");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_ADDSTRING,0,(LPARAM)"Main OAM");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_ADDSTRING,0,(LPARAM)"Sub BG");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_ADDSTRING,0,(LPARAM)"Sub OAM");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX4,CB_SETCURSEL,0,0);
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX5,CB_ADDSTRING,0,(LPARAM)"Main BG");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX5,CB_ADDSTRING,0,(LPARAM)"Main OAM");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX5,CB_ADDSTRING,0,(LPARAM)"Sub BG");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX5,CB_ADDSTRING,0,(LPARAM)"Sub OAM");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX5,CB_SETCURSEL,0,0);
           UpdatePanel();
       break;
       case WM_HSCROLL:
           switch(GetDlgCtrlID((HWND)lParam)){
               case IDC_SCROLLBAR4:
                   switch(LOWORD(wParam)){
                       case SB_LINEUP:
                           i = xScrollPanel - 1;
                       break;
                       case SB_LINEDOWN:
                           i = xScrollPanel + 1;
                       break;
                       case SB_THUMBPOSITION:
                       case SB_THUMBTRACK:
                           i = HIWORD(wParam);
                       break;
                       default:
                           i = xScrollPanel;
                       break;
                   }
                   if(i != xScrollPanel){
                       xScrollPanel = i;
                       SetScrollPos((HWND)lParam,SB_CTL,i,TRUE);
                       DrawPanel(NULL);
                   }
               break;
               case IDC_TRACK2:
                   DrawTileBitmap(NULL,FALSE);
               break;
               case IDC_SCROLLBAR2:
                   switch(LOWORD(wParam)){
                       case SB_LINEUP:
                           i = xScroll - 1;
                       break;
                       case SB_LINEDOWN:
                           i = xScroll + 1;
                       break;
                       case SB_THUMBPOSITION:
                       case SB_THUMBTRACK:
                           i = HIWORD(wParam);
                       break;
                       default:
                           i = xScroll;
                       break;
                   }
                   if(i != xScroll){
                       xScroll = i;
                       SetScrollPos((HWND)lParam,SB_CTL,xScroll,TRUE);
                       DrawTileBitmap(NULL,FALSE);
                   }
               break;
           }
       break;
       case WM_VSCROLL:
           switch(GetDlgCtrlID((HWND)lParam)){
               case IDC_SCROLLBAR3:
               break;
               case IDC_SCROLLBAR1:
                   switch(LOWORD(wParam)){
                       case SB_LINEUP:
                           i = yScroll - 1;
                       break;
                       case SB_LINEDOWN:
                           i = yScroll + 1;
                       break;
                       case SB_THUMBPOSITION:
                       case SB_THUMBTRACK:
                           i = HIWORD(wParam);
                       break;
                       default:
                           i = yScroll;
                       break;
                   }
                   if(i != yScroll){
                       yScroll = i;
                       SetScrollPos((HWND)lParam,SB_CTL,yScroll,TRUE);
                       DrawTileBitmap(NULL,FALSE);
                   }
               break;
           }
       break;
       case WM_COMMAND:
           switch(LOWORD(wParam)){
               case IDC_BUTTON2:
                   fileName.Capacity(MAX_PATH+1);
                   GetTempPath(MAX_PATH,fileName.c_str());
                   fileName += "image";
                   fileName.BuildFileName(fileName.c_str(),"bmp",GetTickCount());
   				if(ShowSaveDialog(NULL,fileName.c_str(),"Bitmap Files (*.bmp)\0*.bmp\0All files (*.*)\0*.*\0\0\0\0\0",NULL)){
                       fileName.AddEXT(".bmp");
                       ::SelectObject(hdcBitmap,hBit);
                       ::StretchDIBits(hdcBitmap,0,0,BmpInfo.bmiHeader.biWidth,BmpInfo.bmiHeader.biHeight
                       	,0,0,BmpInfo.bmiHeader.biWidth,BmpInfo.bmiHeader.biHeight,pBuffer,&BmpInfo,DIB_RGB_COLORS,SRCCOPY);
                       SaveBitmap(hdcBitmap,hBit,fileName.c_str());
                       ::SelectObject(hdcBitmap,hBitmap);
                   }
               break;
               case IDOK:
                   if(HIWORD(wParam) == BN_CLICKED)
                       Destroy();
               break;
               case IDC_RADIO3:
                   if(HIWORD(wParam) == BN_CLICKED){
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX4),TRUE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX8),TRUE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX5),FALSE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX6),FALSE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX7),FALSE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX9),FALSE);
                       SetMaxTiles();
                       UpdatePanel();
                   }
               break;
               case IDC_RADIO4:
                   if(HIWORD(wParam) == BN_CLICKED){
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX4),FALSE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX8),FALSE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX5),TRUE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX6),FALSE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX7),FALSE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX9),FALSE);
                       SetMaxTiles();
                       UpdatePanel();
                   }
               break;
               case IDC_RADIO5:
                   if(HIWORD(wParam) == BN_CLICKED){
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX4),FALSE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX8),FALSE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX5),FALSE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX6),TRUE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX7),TRUE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX9),TRUE);
                       SetMaxTiles();
                       UpdatePanel();
                   }
               break;
               case IDC_RADIO1:
                   if(HIWORD(wParam) == BN_CLICKED){
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX1),TRUE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX2),FALSE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX3),FALSE);
                       SetMaxTiles();
                       UpdatePanel();
                   }
               break;
               case IDC_RADIO2:
                   if(HIWORD(wParam) == BN_CLICKED){
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX1),FALSE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX2),TRUE);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_COMBOBOX3),TRUE);
                       SetMaxTiles();
                       UpdatePanel();
                   }
               break;
               case IDC_COMBOBOX1:
                   if(HIWORD(wParam) == CBN_SELENDOK){
                       SetMaxTiles();
                       UpdatePanel();
                   }
               break;
               case IDC_COMBOBOX2:
               case IDC_COMBOBOX3:
               case IDC_COMBOBOX4:
               case IDC_COMBOBOX5:
               case IDC_COMBOBOX6:
               case IDC_COMBOBOX7:
               case IDC_COMBOBOX8:
               case IDC_COMBOBOX9:
                   if(HIWORD(wParam) == CBN_SELENDOK)
                       UpdatePanel();
               break;
               case IDC_CHECK1:
                   UpdatePanel();
               break;
           }
       break;
       case WM_LBUTTONDOWN:
           OnLButtonDown(wParam,(int)LOWORD(lParam),(int)HIWORD(lParam));
       break;
       case WM_PAINT:
           ::BeginPaint(m_hWnd,&ps);
           DrawPanel(ps.hdc);
           DrawTileBitmap(ps.hdc,TRUE);
           ::EndPaint(m_hWnd,&ps);
       break;
       case WM_CLOSE:
           Destroy();
       break;
   }
   return res;
}
//---------------------------------------------------------------------------
BOOL LTileViewer::Destroy()
{
   if(hBitmap != NULL)
       ::DeleteObject(hBitmap);
   hBitmap = NULL;
   if(hdcBitmap != NULL)
       ::DeleteDC(hdcBitmap);
   hdcBitmap = NULL;
   if(hBit != NULL)
       ::DeleteObject(hBit);
   hBit = NULL;
   if(hbPanel != NULL)
       ::DeleteObject(hbPanel);
   hbPanel = NULL;
   return LDlg::Destroy();
}
//---------------------------------------------------------------------------
void LTileViewer::EraseBackGround(HDC hdc)
{
   RECT rc;

   ::CopyRect(&rc,&rcTile);
   InflateRect(&rc,szBorder.cx,szBorder.cy);
   DrawEdge(hdc,&rc,EDGE_SUNKEN,BF_RECT);
   InflateRect(&rc,-szBorder.cx,-szBorder.cy);
   FillRect(hdc,&rc,(HBRUSH)GetStockObject(BLACK_BRUSH));
}
//---------------------------------------------------------------------------
void LTileViewer::EraseBkGndPanel(HDC hdc)
{
   RECT rc;

   ::CopyRect(&rc,&rcPanel);
   InflateRect(&rc,szBorder.cx,szBorder.cy);
   DrawEdge(hdc,&rc,EDGE_SUNKEN,BF_RECT);
   InflateRect(&rc,-szBorder.cx,-szBorder.cy);
   FillRect(hdc,&rc,(HBRUSH)GetStockObject(BLACK_BRUSH));
}

#endif
