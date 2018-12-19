#include "spriteviewer.h"
#include "resource.h"
#include "lstring.h"
#include "util.h"

#ifdef _DEBPRO4
//---------------------------------------------------------------------------
LSpriteViewer::LSpriteViewer() : LDlg()
{
   hBit = hBitmap = NULL;
   hdcBitmap = NULL;
   iScale = 0;
}
//---------------------------------------------------------------------------
LSpriteViewer::~LSpriteViewer()
{
   Destroy();
}
//---------------------------------------------------------------------------
LRESULT LSpriteViewer::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   u32 i;
   RECT rc;
   PAINTSTRUCT ps;
   BOOL res;
   POINT pt;
   HWND hwnd;
   LString fileName;
   WORD wID;
   
   res = FALSE;
   switch(uMsg){
       case WM_INITDIALOG:
           ::SetFocus(GetDlgItem(m_hWnd,IDC_EDIT1));
           ::SendDlgItemMessage(m_hWnd,IDC_SPIN1,UDM_SETRANGE,0,MAKELPARAM(127,0));
           ::SendDlgItemMessage(m_hWnd,IDC_SPIN1,UDM_SETPOS,0,0);
           ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"Main");
           ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"Sub");
           ::SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_SETCURSEL,0,0);
           pGpu = &downLcd;
           ::GetWindowRect(GetDlgItem(m_hWnd,IDC_SPRITE),&rc);
           ::DestroyWindow(GetDlgItem(m_hWnd,IDC_SPRITE));
           pt.x = rc.left;
           pt.y = rc.top;
           ::ScreenToClient(m_hWnd,&pt);
           ::SetRect(&rcSprite,pt.x,pt.y,pt.x+128,pt.y+128);
           pt.x = ::GetSystemMetrics(SM_CXEDGE);
           pt.y = ::GetSystemMetrics(SM_CYEDGE);
           rcSprite.right += pt.x;
           rcSprite.bottom +=pt.y;
           rcSprite.left -= pt.x;
           rcSprite.top -= pt.y;
           szBorder.cx = pt.x;
           szBorder.cy = pt.y;
           hwnd = GetDlgItem(m_hWnd,IDC_VSBSPR);
           ::GetWindowRect(hwnd,&rc);
           i = rc.right - rc.left;
           ::SetWindowPos(hwnd,NULL,rcSprite.right + 2,rcSprite.top - pt.y,i,
               rcSprite.bottom - rcSprite.top + pt.y,SWP_NOREPOSITION|SWP_NOSENDCHANGING|SWP_HIDEWINDOW);
           hwnd = GetDlgItem(m_hWnd,IDC_HSBSPR);
           ::GetWindowRect(hwnd,&rc);
           ::SetWindowPos(hwnd,NULL,rcSprite.left - pt.x,rcSprite.bottom + 2,rcSprite.right - rcSprite.left + pt.x,
               i,SWP_NOREPOSITION|SWP_NOSENDCHANGING|SWP_HIDEWINDOW);
           i += rcSprite.bottom + 3;
           hwnd = GetDlgItem(m_hWnd,IDC_TRACK1);
           ::GetWindowRect(hwnd,&rc);
           pt.x = rcSprite.left + (((rcSprite.right - rcSprite.left) - (rc.right - rc.left)) >> 1);
           ::SetWindowPos(hwnd,NULL,pt.x,i,0,0,SWP_NOREPOSITION|SWP_NOSIZE|SWP_NOSENDCHANGING);
           ::SendMessage(hwnd,TBM_SETRANGE,0,MAKELPARAM(0,10));
           ::SendMessage(hwnd,TBM_SETPOS,TRUE,0);
           IndexSprite = 0;
           for(i = IDC_SPRITE_ENA;i <= IDC_SPRITE_ATTR2;i++){
               ::GetWindowRect(GetDlgItem(m_hWnd,i),&rc);
               pt.x = rc.right;
               pt.y = rc.top;
               ScreenToClient(&pt);
               pt.x += 3;
               rcControl[i - IDC_SPRITE_ENA].left = pt.x;
               rcControl[i - IDC_SPRITE_ENA].top = pt.y;
               rcControl[i - IDC_SPRITE_ENA].right = pt.x + 60;
               rcControl[i - IDC_SPRITE_ENA].bottom = pt.y + rc.bottom - rc.top;
           }
//           RedrawWindow(m_hWnd,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE);
       break;
       case WM_VSCROLL:
           wID = (WORD)GetDlgCtrlID((HWND)lParam);
           switch(wID){
               case IDC_SPIN1:
                   switch(LOWORD(wParam)){
                       case SB_THUMBPOSITION:
                           IndexSprite = HIWORD(wParam);
                           if(IndexSprite != 0){
                               DrawSprite((u8)(IndexSprite - 1));
                               UpdateSprite(NULL);
                           }
                       break;
                   }
               break;
               case IDC_VSBSPR:
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
                       DrawSpriteBitmap(NULL,FALSE);
                   }
               break;
           }
       break;
       case WM_HSCROLL:
           wID = (WORD)GetDlgCtrlID((HWND)lParam);
           switch(wID){
               case IDC_TRACK1:
                   DrawSpriteBitmap(NULL,FALSE);
               break;
               case IDC_HSBSPR:
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
                       DrawSpriteBitmap(NULL,FALSE);
                   }
               break;
           }
       break;
       case WM_COMMAND:
           switch(LOWORD(wParam)){
               case IDC_COMBOBOX1:
                   if(HIWORD(wParam) == CBN_SELENDOK){
                       i = ::SendMessage((HWND)lParam,CB_GETCURSEL,0,0);
                       if(i != CB_ERR){
                           pGpu = i == 0 ? (LGPU *)&downLcd : (LGPU *)&upLcd;
                           DrawSprite((u8)(IndexSprite-1));
                           UpdateSprite(NULL);
                       }
                   }
               break;
               case IDC_EDIT1:
               break;
               case IDOK:
                   Destroy();
               break;
               case IDCANCEL:
                   UpdateSprite(NULL);
               break;
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
           }
       break;
       case WM_PAINT:
           ::BeginPaint(m_hWnd,&ps);
           DrawSpriteBitmap(ps.hdc,TRUE);              
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
BOOL LSpriteViewer::Destroy()
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
   LDlg::Destroy();
}
//---------------------------------------------------------------------------
BOOL LSpriteViewer::Show(HWND parent)
{
   if(m_hWnd == NULL){
       if(!LDlg::Create(hInst,MAKEINTRESOURCE(IDD_DIALOG8),parent))
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
void LSpriteViewer::EraseBackGround(HDC hdc)
{
   RECT rc;

   ::CopyRect(&rc,&rcSprite);
   InflateRect(&rc,szBorder.cx,szBorder.cy);
   DrawEdge(hdc,&rc,EDGE_SUNKEN,BF_RECT);
   InflateRect(&rc,-szBorder.cx,-szBorder.cy);
   FillRect(hdc,&rc,(HBRUSH)GetStockObject(BLACK_BRUSH));
}
//---------------------------------------------------------------------------
int LSpriteViewer::DrawSpriteBitmap(HDC hdc,u8 eraseBk)
{
   RECT rc,rc1,rc2;
   LPSPRITE pSprite;
   int i,sx,sy;
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
   pSprite = IndexSprite > 0 ? pGpu->get_Sprite((int)(IndexSprite-1)) : NULL;
   ::CopyRect(&rc,&rcSprite);
   if(hdcBitmap == NULL || hBitmap == NULL || pSprite == NULL){
       res = 0;
       EraseBackGround(hdc);
       goto Ex_DrawSpriteBitmap;
   }
   sx = pSprite->SizeX;
   sy = pSprite->SizeY;
   if(pSprite->bDouble != 0 && pSprite->bRot){
       sx <<= 1;
       sy <<= 1;
   }
   ::SetRect(&rc1,0,0,sx,sy);
   sz.cx = rc1.right;
   sz.cy = rc1.bottom;
   i = SendDlgItemMessage(m_hWnd,IDC_TRACK1,TBM_GETPOS,0,0);
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
   hwnd = GetDlgItem(m_hWnd,IDC_VSBSPR);
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
   hwnd = GetDlgItem(m_hWnd,IDC_HSBSPR);
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
Ex_DrawSpriteBitmap:
   if(flag != 0)
       ReleaseDC(m_hWnd,hdc);
   return res;
}
//---------------------------------------------------------------------------
void LSpriteViewer::UpdateSprite(HDC hdc)
{
   u8 flag;
   RECT rc,rc1,rc2;
   HFONT hFont;
   int i,sx,sy;
   char s[30];
   LPSPRITE pSprite;
   HBRUSH hBrush;

   ::EnableWindow(GetDlgItem(m_hWnd,IDC_BUTTON2),FALSE);
   if(hdc == NULL){
       hdc = GetDC(m_hWnd);
       flag = 1;
   }
   else
       flag = 0;
   hFont = (HFONT)SendDlgItemMessage(m_hWnd,IDC_EDIT1,WM_GETFONT,0,0);
   SelectObject(hdc,hFont);
   hBrush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
   for(i = IDC_SPRITE_ENA;i <= IDC_SPRITE_ATTR2;i++)
       FillRect(hdc,&rcControl[i - IDC_SPRITE_ENA],hBrush);
   ::DeleteObject(hBrush);
   CopyRect(&rc,&rcSprite);
   EraseBackGround(hdc);
   if(IndexSprite != 0 && hBit != NULL){
       xScroll = yScroll = 0;
       pSprite = pGpu->get_Sprite((IndexSprite-1));
       sx = pSprite->SizeX;
       sy = pSprite->SizeY;
       if(pSprite->bDouble != 0 && pSprite->bRot){
           sx <<= 1;
           sy <<= 1;
       }
       ::SetRect(&rc1,0,0,sx,sy);
       SendDlgItemMessage(m_hWnd,IDC_TRACK1,TBM_SETPOS,TRUE,0);
       iScale = 10;
       rc2.left = rc.left + ((rc.right-rc.left - rc2.right) >> 1);
       rc2.top = rc.top + ((rc.bottom - rc.top - rc2.bottom) >> 1);
       if(hdcBitmap != NULL){
           if(hBitmap != NULL)
               DeleteObject(hBitmap);
           hBitmap = NULL;
           ::DeleteDC(hdcBitmap);
           hdcBitmap = NULL;
       }
       if((hdcBitmap = CreateCompatibleDC(hdc)) != NULL){
           i = SendDlgItemMessage(m_hWnd,IDC_TRACK1,TBM_GETRANGEMAX,0,0);
           if((hBitmap = CreateCompatibleBitmap(hdc,sx * i,sy * i)) != NULL)
               DrawSpriteBitmap(hdc,FALSE);
       }
       SetBkMode(hdc,TRANSPARENT);
       DrawText(hdc,": Enabled",-1,&rcControl[0],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,": %1d",pSprite->Priority);
       DrawText(hdc,s,-1,&rcControl[1],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,": %3d,%3d",pSprite->xPos,pSprite->yPos);
       DrawText(hdc,s,-1,&rcControl[2],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,": %s",pSprite->bPalette ? "16" : "256");
       DrawText(hdc,s,-1,&rcControl[3],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,": %02d",pSprite->iPalette >> 4);
       DrawText(hdc,s,-1,&rcControl[4],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,": %02d,%02d",pSprite->SizeX,pSprite->SizeY);
       DrawText(hdc,s,-1,&rcControl[5],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,": %s",pSprite->bRot ? "Yes" : "No");
       DrawText(hdc,s,-1,&rcControl[6],DT_SINGLELINE|DT_LEFT);
       switch(pSprite->VisibleMode){
           case 0:
               lstrcpy(s,": Normal");
           break;
           case 1:
               lstrcpy(s,": Semi-Transparent");
           break;
           case 2:
               lstrcpy(s,": OBJ Window");
           break;
           case 3:
               lstrcpy(s,": OBJ Bitmap");
           break;
       }
       DrawText(hdc,s,-1,&rcControl[7],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,": 0x%04X",pSprite->a0);
       DrawText(hdc,s,-1,&rcControl[8],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,": 0x%04X",pSprite->a1);
       DrawText(hdc,s,-1,&rcControl[9],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,": 0x%04X",pSprite->a2);
       DrawText(hdc,s,-1,&rcControl[10],DT_SINGLELINE|DT_LEFT);
       ::EnableWindow(GetDlgItem(m_hWnd,IDC_BUTTON2),TRUE);
   }
   if(flag != 0)
       ReleaseDC(m_hWnd,hdc);
}
//---------------------------------------------------------------------------
int LSpriteViewer::DrawSpriteInternal(u8 nSprite,u16 *buffer,int inc)
{
   LPSPRITE pSprite;
   s16 sx,sy;
   u8 *p1,b;
   u16 x8,y8,x,*pal;
   u32 i,y1;
	u16 iColor;
   int xc,yc,y2,y3,x3,y,xc2,yTile,x2PA,x2PC;
   u32 tileBaseNumber,PA,PC;
   u8 xTile,xSubTile,ySubTile,xNbTile,yNbTile;

   if(nSprite > 127 || (pSprite = pGpu->get_Sprite(nSprite)) == NULL)
       return 0;
   if(!pSprite->bRot && pSprite->bDouble)
       return 0;
   sy = pSprite->SizeY;
   sx = pSprite->SizeX;
   if(pSprite->bRot)
       goto DrawDebugSprite_Rot;
   for(y1=0;y1<sy;y1++){
       y8 = (u16)y1;
       if(pSprite->bMosaic != 0 && pGpu->spryMosaic)
           y8 = (u16)(y8 - (y8 % pGpu->spryMosaic));
       y8 = (u16)(pSprite->vFlip ? sy - 1 - y8 : y8);
       if(pSprite->VisibleMode != 3){
           if(!pSprite->bPalette){
          	    p1 = &pGpu->vram_obj[(y8 & 0x7) << 3];
               pal = (b = (u8)(pGpu->reg[3] & 0x80)) != 0 ? &pGpu->texpal_obj[pSprite->iPalette<<4] : pGpu->tpal_obj;
           }
           else{
               p1 = &pGpu->vram_obj[(y8 & 0x7) << 2];
               pal = pGpu->tpal_obj;
           }
           p1 += (((pSprite->tileBaseNumber << ((pGpu->reg[2] & 0x30) >> 4)) + ((y8 >> 3) << pSprite->tileNumberYIncr)) << 5);
       }
       else{
           switch(pGpu->reg[0] & 0x60){
               case 0:
                   i = ((pSprite->tileBaseNumber & 0xF) << 4) + ((pSprite->tileBaseNumber & ~0xF) << 7) + (y8 << 8);
               break;
               case 0x20:
                   i = ((pSprite->tileBaseNumber & 0x1F) << 4) + ((pSprite->tileBaseNumber & ~0x1F) << 7) + (y8 << 9);
               break;
               case 0x40:
                   i = 7 + ((pGpu->reg[2] & 0x40) >> 6);
                   i = (pSprite->tileBaseNumber<<i) + (y8 << pSprite->tileNumberYIncr);
               break;
           }
           p1 = &pGpu->vram_obj[i];
       }
       for(x=0;x < sx;x++,buffer++){
           if(pSprite->bMosaic != 0 && pGpu->sprxMosaic)
               x8 = (u16)(x - (x % pGpu->sprxMosaic));
           else
               x8 = x;
           x8 = (u16)((y8 = (u16)(pSprite->hFlip ? sx - 1 - x8 : x8)) & 0x7);
           if(pSprite->VisibleMode == 3){
               y8 = ((u16 *)p1)[y8];
               if(!(y8 & 0x8000))
                   continue;
               y8 = BGR(y8);
           }
           else if(!pSprite->bPalette){
			    if((y8 = p1[((y8 >> 3) << 6) + x8]) == 0)
              	    continue;
          	    y8 = b ? BGR(pal[y8]) : pal[y8];
           }
           else{
               y8 = p1[((y8 >> 3) << 5) + (x8 >> 1)];
               if((y8 = (u16)((y8 >> ((x8 & 0x1) << 2)) & 0xF)) == 0)
                   continue;
               y8 = pal[y8 + (u16)pSprite->iPalette];
           }
           *buffer = y8;
       }
   }
   return 1;
DrawDebugSprite_Rot:
   xNbTile = sx;
   yNbTile = sy;
   xc = sx >> 1;
   yc = sy >> 1;
   if(pSprite->bDouble != 0){
       sx <<= 1;
       sy <<= 1;
   }
   if(pSprite->VisibleMode == 3){
       switch(pGpu->reg[0] & 0x60){
           case 0:
               tileBaseNumber = ((pSprite->tileBaseNumber & 0xF) << 4) + ((pSprite->tileBaseNumber & ~0xF) << 7);
               i = 7;
           break;
           case 0x20:
               tileBaseNumber = ((pSprite->tileBaseNumber & 0x1F) << 4) + ((pSprite->tileBaseNumber & ~0x1F) << 7);
				i = 8;
           break;
           case 0x40:
               tileBaseNumber = pSprite->tileBaseNumber << ((pGpu->reg[2] & 0x40) ? 8 : 7);
               i = pSprite->tileNumberYIncr;
           break;
       }
   }
   else
       tileBaseNumber = pSprite->tileBaseNumber << ((pGpu->reg[2] & 0x30) >> 4);
   xc2 = (0 - (sx >> 1));
   if(pSprite->rotMatrix == NULL)
       pSprite->rotMatrix = pGpu->rotMatrix;
   if(!pSprite->bPalette)
 	    pal = (b = (u8)(pGpu->reg[3] & 0x80)) != 0 ? &pGpu->texpal_obj[pSprite->iPalette<<4] : pGpu->tpal_obj;
   else
  	    pal = pGpu->tpal_obj;
   for(y=0;y<sy;y++){
       y2 = (y - (sy >> 1)) << 8;
       x2PA = xc2 * (PA = pSprite->rotMatrix->PA << 8) + (y2 * pSprite->rotMatrix->PB);
       x2PC = xc2 * (PC = pSprite->rotMatrix->PC << 8) + (y2 * pSprite->rotMatrix->PD);
       for(x = 0; x < sx;x++,x2PA += PA,x2PC += PC,buffer++){
           x3 = (x2PA >> 16) + xc;
		    y3 = (x2PC >> 16) + yc;
           if(!(x3 >= 0 && x3 < xNbTile && y3 >= 0 && y3 < yNbTile))
               continue;
           if(pSprite->VisibleMode == 3){
               iColor = *((u16 *)&pGpu->vram_obj[tileBaseNumber + ((x3 + (y3 << i)) << 1)]);
               if(!(iColor & 0x8000))
                   continue;
               iColor = BGR(iColor);
           }
           else{
           	xTile = (u8)(x3 >> 3);
		    	yTile = (y3 >> 3) << pSprite->tileNumberYIncr;
           	xSubTile = (u8)(x3 & 0x7);
		    	ySubTile = (u8)(y3 & 0x7);
           	if (!pSprite->bPalette){
               	if((iColor = pGpu->vram_obj[((tileBaseNumber + (xTile << 1) + yTile) << 5) + xSubTile + (ySubTile << 3)]) == 0)
                   	continue;
               	iColor = b ? BGR(pal[iColor]) : pal[iColor];
           	}
           	else{
               	iColor = pGpu->vram_obj[((tileBaseNumber + xTile + yTile) << 5) + (xSubTile >> 1) + (ySubTile << 2)];
               	if((iColor = (u16)((iColor >> ((xSubTile & 0x1) << 2)) & 0xf)) == 0)
                   	continue;
               	iColor = pal[iColor + pSprite->iPalette];
           	}
           }
           *buffer = iColor;
       }
   }
   return 1;
}
//---------------------------------------------------------------------------
int LSpriteViewer::DrawSprite(u8 nSprite)
{
   LPSPRITE pSprite;
   int sx,sy;

   if(hBit != NULL)
       ::DeleteObject(hBit);
   hBit = NULL;
   if((pSprite = pGpu->get_Sprite(nSprite)) == NULL)
       return -1;
   if(!pSprite->bRot && pSprite->bDouble || !pSprite->Enable)
       return -1;
   sx = pSprite->SizeX;
   sy = pSprite->SizeY;
   if(pSprite->bDouble != 0 && pSprite->bRot){
       sx <<= 1;
       sy <<= 1;
   }
   ZeroMemory(&BmpInfo,sizeof(BITMAPINFO));
	BmpInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	BmpInfo.bmiHeader.biBitCount    = 16;
	BmpInfo.bmiHeader.biWidth       = sx;
	BmpInfo.bmiHeader.biHeight      = -sy;
	BmpInfo.bmiHeader.biPlanes      = 1;
	BmpInfo.bmiHeader.biCompression = BI_RGB;
   hBit = ::CreateDIBSection(NULL,&BmpInfo,DIB_RGB_COLORS,(VOID**)&pBuffer,NULL,0);
   if(hBit == NULL)
       return -1;
   ZeroMemory(pBuffer,sy*sx * (BmpInfo.bmiHeader.biBitCount >> 3));
   return DrawSpriteInternal(nSprite,pBuffer,0);
}
#endif
