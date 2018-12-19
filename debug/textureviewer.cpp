#include "textureviewer.h"
#include "3d.h"
#include "resource.h"
#include "lds.h"
#include "util.h"
#include "pluginctn.h"

#ifdef _DEBPRO
static char cFormat[][25]={"No Texture","A3I5 Translucent","4-Color Palette","16-Color Palette",
       "256-Color Palette","4x4-Texel Compressed","A5I3 Translucent","Direct Texture"};
//---------------------------------------------------------------------------
LTextureViewer::LTextureViewer() : LDlg()
{
   hBit=hBitmap = NULL;
   hdcBitmap = NULL;
   iScale = 0;
   texFormat = indexTex = -1;
   texData = texAddress = (DWORD)-1;
   dwOptions = 0;
}
//---------------------------------------------------------------------------
LTextureViewer::~LTextureViewer()
{
   Destroy();
}
//---------------------------------------------------------------------------
BOOL LTextureViewer::Show(HWND parent)
{
   if(m_hWnd == NULL){
       if(!LDlg::Create(hInst,MAKEINTRESOURCE(IDD_DIALOG9),parent))
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
BOOL LTextureViewer::Destroy()
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
   return LDlg::Destroy();
}
//---------------------------------------------------------------------------
LRESULT LTextureViewer::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LRESULT res;
   HWND hwnd;
   POINT pt;
   RECT rc;
   PAINTSTRUCT ps;
   WORD wID;
   int i,i1;
   LString fileName;
	char s[50];

   res = FALSE;
   switch(uMsg){
       case WM_INITDIALOG:
           ::SetFocus(GetDlgItem(m_hWnd,IDC_EDIT1));
           ::SendDlgItemMessage(m_hWnd,IDC_SPIN1,UDM_SETRANGE,0,MAKELPARAM(TEXTURE_ALLOCATED,0));
           ::SendDlgItemMessage(m_hWnd,IDC_SPIN1,UDM_SETPOS,0,0);
           ::GetWindowRect(GetDlgItem(m_hWnd,IDC_TEXTURE),&rc);
           ::DestroyWindow(GetDlgItem(m_hWnd,IDC_TEXTURE));
           pt.x = rc.left;
           pt.y = rc.top;
           ::ScreenToClient(m_hWnd,&pt);
           ::SetRect(&rcTexture,pt.x,pt.y,pt.x+256,pt.y+256);
           pt.x = ::GetSystemMetrics(SM_CXEDGE);
           pt.y = ::GetSystemMetrics(SM_CYEDGE);
           rcTexture.right += pt.x;
           rcTexture.bottom +=pt.y;
           rcTexture.left -= pt.x;
           rcTexture.top -= pt.y;
           szBorder.cx = pt.x;
           szBorder.cy = pt.y;
           hwnd = GetDlgItem(m_hWnd,IDC_VSBTEX);
           ::GetWindowRect(hwnd,&rc);
           i = rc.right - rc.left;
           ::SetWindowPos(hwnd,NULL,rcTexture.right + 2,rcTexture.top - pt.y,i,
               rcTexture.bottom - rcTexture.top + pt.y,SWP_NOREPOSITION|SWP_NOSENDCHANGING|SWP_HIDEWINDOW);
           hwnd = GetDlgItem(m_hWnd,IDC_HSBTEX);
           ::GetWindowRect(hwnd,&rc);
           ::SetWindowPos(hwnd,NULL,rcTexture.left - pt.x,rcTexture.bottom + 2,rcTexture.right - rcTexture.left + pt.x,
               i,SWP_NOREPOSITION|SWP_NOSENDCHANGING|SWP_HIDEWINDOW);
           i += rcTexture.bottom + 3;
           hwnd = GetDlgItem(m_hWnd,IDC_TRACK1);
           ::GetWindowRect(hwnd,&rc);
           pt.x = rcTexture.left + (((rcTexture.right - rcTexture.left) - (rc.right - rc.left)) >> 1);
           ::SetWindowPos(hwnd,NULL,pt.x,i,0,0,SWP_NOREPOSITION|SWP_NOSIZE|SWP_NOSENDCHANGING|SWP_HIDEWINDOW);
           ::SendMessage(hwnd,TBM_SETRANGE,0,MAKELPARAM(0,10));
           ::SendMessage(hwnd,TBM_SETPOS,TRUE,0);
           for(i1=0,i = IDC_TEX_SIZE;i <= IDC_TEX_PAL;i++,i1++){
               ::GetWindowRect(GetDlgItem(m_hWnd,i),&rc);
               pt.x = rc.right;
               pt.y = rc.top;
               ScreenToClient(&pt);
               pt.x += 3;
               rcControl[i - IDC_TEX_SIZE].left = pt.x;
               rcControl[i - IDC_TEX_SIZE].top = pt.y;
               rcControl[i - IDC_TEX_SIZE].right = rcTexture.left - 5;
               rcControl[i - IDC_TEX_SIZE].bottom = pt.y + rc.bottom - rc.top;
               if(i == IDC_TEX_OFFSET){
                   ::SetWindowPos(GetDlgItem(m_hWnd,IDC_EDIT2),NULL,pt.x,pt.y,
                   	rcControl[i1].right - rcControl[i1].left,rcControl[i1].bottom - rcControl[i1].top,SWP_NOREPOSITION|SWP_NOSENDCHANGING);
               }
               else if(i == IDC_TEX_FORMAT){
                   ::SetWindowPos(GetDlgItem(m_hWnd,IDC_COMBOBOX1),NULL,pt.x,pt.y,
                   	rcControl[i1].right - rcControl[i1].left,rcControl[i1].bottom - rcControl[i1].top,SWP_NOREPOSITION|SWP_NOSENDCHANGING);
               }
               else if(i == IDC_TEX_DATA){
                   ::SetWindowPos(GetDlgItem(m_hWnd,IDC_EDIT3),NULL,pt.x,pt.y,
                   	rcControl[i1].right - rcControl[i1].left,rcControl[i1].bottom - rcControl[i1].top,SWP_NOREPOSITION|SWP_NOSENDCHANGING);
               }
           }
           for(i=0;i < sizeof(cFormat) / 25;i++)
           	SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)&cFormat[i]);
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_SETCURSEL,0,0);
			::SetWindowLong(GetDlgItem(m_hWnd,IDC_EDIT2),GWL_USERDATA,(LONG)this);
			::SetWindowLong(GetDlgItem(m_hWnd,IDC_EDIT3),GWL_USERDATA,(LONG)this);
           oldEditWndProc2 = (WNDPROC)SetWindowLong(GetDlgItem(m_hWnd,IDC_EDIT2),GWL_WNDPROC,(LONG)EditWindowProc);
           oldEditWndProc3 = (WNDPROC)SetWindowLong(GetDlgItem(m_hWnd,IDC_EDIT3),GWL_WNDPROC,(LONG)EditWindowProc);
          	res = TRUE;
       break;
       case WM_CLOSE:
           Destroy();
       break;
       case WM_PAINT:
           ::BeginPaint(m_hWnd,&ps);
           DrawTextureBitmap(ps.hdc,TRUE);
           ::EndPaint(m_hWnd,&ps);
           res = TRUE;
       break;
       case WM_VSCROLL:
           wID = (WORD)GetDlgCtrlID((HWND)lParam);
           switch(wID){
               case IDC_SPIN1:
                   switch(LOWORD(wParam)){
                       case SB_THUMBPOSITION:
                           indexTex = HIWORD(wParam);
                           texData = texAddress = (DWORD)-1;
							texFormat = -1;
                           if(indexTex > 0){
                               DrawTexture(indexTex - 1);
                               UpdateTexture(NULL);
                           }
                       break;
                   }
               break;
               case IDC_VSBTEX:
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
                       DrawTextureBitmap(NULL,FALSE);
                   }
               break;
           }
       break;
       case WM_HSCROLL:
           wID = (WORD)GetDlgCtrlID((HWND)lParam);
           switch(wID){
               case IDC_TRACK1:
                   DrawTextureBitmap(NULL,FALSE);
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
                       DrawTextureBitmap(NULL,FALSE);
                   }
               break;
           }
       break;
       case WM_COMMAND:
           switch(LOWORD(wParam)){
           	case IDC_COMBOBOX1:
               	switch(HIWORD(wParam)){
                   	case CBN_SELENDOK:
                       	texFormat = ::SendMessage((HWND)lParam,CB_GETCURSEL,0,0);
                           if(indexTex > 0){
                               DrawTexture(indexTex - 1);
                               UpdateTexture(NULL);
                           }
                       break;
                   }
               break;
               case IDOK:
                   Destroy();
               break;
               case IDC_BUTTON1:
               break;
               case IDC_BUTTON2:
                   fileName.Capacity(MAX_PATH+1);
                   GetTempPath(MAX_PATH,fileName.c_str());
                   fileName += "image";
                   fileName.BuildFileName(fileName.c_str(),"bmp",GetTickCount());
   				if(ShowSaveDialog(NULL,fileName.c_str(),"Bitmap Files (*.bmp)\0*.bmp\0All files (*.*)\0*.*\0\0\0\0\0",NULL)){
                       ::SelectObject(hdcBitmap,hBit);
                       ::StretchDIBits(hdcBitmap,0,0,BmpInfo.bmiHeader.biWidth,BmpInfo.bmiHeader.biHeight
                       	,0,0,BmpInfo.bmiHeader.biWidth,BmpInfo.bmiHeader.biHeight,pBuffer,&BmpInfo,DIB_RGB_COLORS,SRCCOPY);
                       SaveBitmap(hdcBitmap,hBit,fileName.c_str());
                       ::SelectObject(hdcBitmap,hBitmap);
                   }
               break;
               case IDC_CHECK1:
               	if(::SendMessage((HWND)lParam,BM_GETCHECK,0,0) == BST_CHECKED)
                   	dwOptions |= 2;
                   else
                   	dwOptions &= ~2;
					InvalidateRect(m_hWnd,NULL,TRUE);
                   UpdateWindow(m_hWnd);
               break;
               case IDC_CHECK2:
               	if(::SendMessage((HWND)lParam,BM_GETCHECK,0,0) == BST_CHECKED)
                   	dwOptions |= 1;
                   else
                   	dwOptions &= ~1;
                   if(indexTex > 0){
                       DrawTexture(indexTex - 1);
                       UpdateTexture(NULL);
                   }
               break;
           }
		break;
       case WM_MOUSEMOVE:
       	*((int *)s) = 0;
       	if(hBitmap != NULL){
               pt.x = LOWORD(lParam);pt.y = HIWORD(lParam);
               if(PtInRect(&rcTexture,pt))
                   wsprintf(s,"(%d,%d)",pt.x-rcTexture.left,pt.y-rcTexture.top);
           }
			SetDlgItemText(m_hWnd,IDC_INSPECTOR_SB,s);
       break;
   }
   return res;
}
//---------------------------------------------------------------------------
int LTextureViewer::DrawTexture(int index)
{
   TEXTURE tex;
   int sx,sy,x,y,i,i1;
   BYTE r,color,alpha,base_color;
   PlugIn *p;
   void *p1[2];

   if(hBit != NULL)
       ::DeleteObject(hBit);
   hBit = NULL;
   if((p = pPlugInContainer->get_PlugInList(PIL_VIDEO)->get_ActivePlugIn(PIT_IS3D)) == NULL)
       return -1;
   if(texData != -1)
       p1[0] = (LPVOID)texData;
   else
       p1[0] = NULL;
   p1[1] = (void *)index;
   p->NotifyState(PNMV_GETTEXTUREINFO,PIS_NOTIFYMASK,(LPARAM)p1);
   if(p1[0] != NULL)
       memcpy(&tex,p1[0],sizeof(TEXTURE));
   if(p1[0] == NULL || tex.data == (u32)-1){
       InvalidateRect(m_hWnd,NULL,TRUE);
       UpdateWindow(m_hWnd);
       return -1;
   }
   sx = tex.h_size;
   sy = tex.v_size;
   ZeroMemory(&BmpInfo,sizeof(BITMAPINFO));
   BmpInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
   BmpInfo.bmiHeader.biBitCount    = 32;
   BmpInfo.bmiHeader.biWidth       = sx;
   BmpInfo.bmiHeader.biHeight      = -sy;
   BmpInfo.bmiHeader.biPlanes      = 1;
   BmpInfo.bmiHeader.biCompression = BI_RGB;
   hBit = ::CreateDIBSection(NULL,&BmpInfo,DIB_RGB_COLORS,(VOID**)&pBuffer,NULL,0);
   if(hBit == NULL)
       return -1;
   ZeroMemory(pBuffer,sy * sx * (BmpInfo.bmiHeader.biBitCount >> 3));
   p1[0] = (void *)pBuffer;
   p1[1] = (void *)&tex;
   if(texAddress != (DWORD)-1)
       tex.adr = texAddress;
   if(texFormat != -1)
       tex.format = (u8)texFormat;
   if(texData != -1)
       tex.data = texData;
   p->NotifyState(PNMV_GETTEXTURE,PIS_NOTIFYMASK,(LPARAM)p1);
   if(!p1[1])
       return -1;
   base_color = 0xC0;
	for(i=y=0;y<sy;y++,i+= sx){
   	if((y & 15) == 0)
       	base_color ^= 0x40;
       color = base_color;
   	for(x=0;x<sx;x++){
       	i1 = i*4+x*4;
           if(dwOptions & 1){
               if((x & 15) == 0)
                   color ^= 0x40;
               alpha = pBuffer[i1+3];
               r = pBuffer[i1];
               pBuffer[i1] = (pBuffer[i1+2] * alpha / 255.0f) + (color * (255-alpha) / 255.0f);
               pBuffer[i1+2] = (r * alpha / 255.0f) + (color * (255-alpha) / 255.0f);
               pBuffer[i1+1] = (pBuffer[i1+1] * alpha / 255.0f) + (color * (255-alpha) / 255.0f);
           }
           else{
               r = pBuffer[i1];
               pBuffer[i1] = pBuffer[i1+2];
               pBuffer[i1+2] = r;
           }
       }
   }
   return 1;
}
//---------------------------------------------------------------------------
void LTextureViewer::EraseBackGround(HDC hdc)
{
   RECT rc;
	int x,y,i;
	HBRUSH dk_brush,lg_brush;

   rc = rcTexture;
   ::InflateRect(&rc,szBorder.cx,szBorder.cy);
   ::DrawEdge(hdc,&rc,EDGE_SUNKEN,BF_RECT);
   ::InflateRect(&rc,-szBorder.cx,-szBorder.cy);
	if(dwOptions & 2){
       dk_brush = CreateSolidBrush(RGB(0x80,0x80,0x80));
       lg_brush = CreateSolidBrush(RGB(0xc0,0xc0,0xc0));
       for(i=0,y=rcTexture.top;y<rcTexture.bottom;y += 16){
           for(x=rcTexture.left;x<rcTexture.right;x +=16,i++){
               ::SetRect(&rc,x,y,x+16,y+16);
               if(rc.right > rcTexture.right)
                   rc.right = rcTexture.right;
               if(rc.bottom > rcTexture.bottom)
                   rc.bottom = rcTexture.bottom;
               if(i&1)
                   ::FillRect(hdc,&rc,dk_brush);
               else
                   ::FillRect(hdc,&rc,lg_brush);
           }
       }
       DeleteObject(dk_brush);
       DeleteObject(lg_brush);
   }
   else
   	::FillRect(hdc,&rc,(HBRUSH)GetStockObject(BLACK_BRUSH));
}
//---------------------------------------------------------------------------
int LTextureViewer::DrawTextureBitmap(HDC hdc,u8 eraseBk)
{
   SIZE sz;
   HFONT hFont;
   HBRUSH hBrush;
   RECT rc,rc1,rc2;
   TEXTURE tex;
   HWND hwnd;
   SCROLLINFO si;
   char s[30];
   char cCoordTra[][20]={": Do not transform",": TexCoord source",": Normal source",": Vertex source"};
   char cColor[][10]={": No Color",": 32",": 4",": 16",": 256",": 32767",": 8",": 32767"};
   int i;
   u8 flag,res;
   s8 flagScale;
   PlugIn *p;
   void *p1[2];

   res = 1;
   if(hdc == NULL){
       hdc = GetDC(m_hWnd);
       flag = 1;
   }
   else
       flag = 0;
   hFont = (HFONT)SendDlgItemMessage(m_hWnd,IDC_EDIT1,WM_GETFONT,0,0);
   SelectObject(hdc,hFont);
//   if(eraseBk){
       hBrush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
       for(i = IDC_TEX_SIZE;i <= IDC_TEX_PAL;i++)
           FillRect(hdc,&rcControl[i - IDC_TEX_SIZE],hBrush);
       ::DeleteObject(hBrush);
       ::SetWindowText(GetDlgItem(m_hWnd,IDC_EDIT2),"");
       ::SetWindowText(GetDlgItem(m_hWnd,IDC_EDIT3),"");
       SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_SETCURSEL,0,0);
//   }
   p1[1] = NULL;
	if(indexTex > 0){
       if((p = pPlugInContainer->get_PlugInList(PIL_VIDEO)->get_ActivePlugIn(PIT_IS3D)) != NULL){
			if(texData != -1)
           	p1[0] = (LPVOID)texData;
           else
           	p1[0] = NULL;
           p1[1] = (void *)(indexTex-1);
           p->NotifyState(PNMV_GETTEXTUREINFO,PIS_NOTIFYMASK,(LPARAM)p1);
           if(p1[0] != NULL)
				CopyMemory(&tex,p1[0],sizeof(TEXTURE));
           if(texAddress != (DWORD)-1)
       		tex.adr = texAddress;
       	if(texFormat != -1)
       		tex.format = (u8)texFormat;
           if(texData != -1)
       		tex.data = texData;
       }
   }
   if(hdcBitmap == NULL || hBitmap == NULL || p1[0] == NULL){
       res = 0;
       EraseBackGround(hdc);
       goto Ex_DrawTextureBitmap;
   }
   rc = rcTexture;
   ::SetRect(&rc1,0,0,tex.h_size,tex.v_size);
   sz.cx = rc1.right;
   sz.cy = rc1.bottom;
   i = SendDlgItemMessage(m_hWnd,IDC_TRACK1,TBM_GETPOS,0,0);
   if(i != iScale){
       flagScale = (u8)(iScale > i ? -1 : 1);
       iScale = i;
   }
   else{
       if(sz.cy > (rc.bottom - rc.top) || sz.cx > (rc.right - rc.left))
           flagScale = 1;
       else
           flagScale = 0;
   }
   if(i > 0){
       rc1.right *= i;
       rc1.bottom *= i;
   }
   rc2 = rc1;
   hwnd = GetDlgItem(m_hWnd,IDC_VSBTEX);
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
   else
       ShowScrollBar(hwnd,SB_CTL,FALSE);
   hwnd = GetDlgItem(m_hWnd,IDC_HSBTEX);
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
   else
       ShowScrollBar(hwnd,SB_CTL,FALSE);
   rc2.left = rc.left + ((rc.right-rc.left - rc2.right) >> 1);
   rc2.top = rc.top + ((rc.bottom - rc.top - rc2.bottom) >> 1);
//   if(flagScale != 0 || eraseBk != 0)
       EraseBackGround(hdc);
   ::SelectObject(hdcBitmap,hBitmap);
   ::StretchDIBits(hdcBitmap,0,0,rc1.right,rc1.bottom,0,0,sz.cx,sz.cy,pBuffer,&BmpInfo,DIB_RGB_COLORS,SRCCOPY);
   ::BitBlt(hdc,rc2.left,rc2.top,rc2.right,rc2.bottom,hdcBitmap,xScroll,yScroll,SRCCOPY);
   SetBkMode(hdc,TRANSPARENT);
   wsprintf(s,": %dx%d",tex.h_size,tex.v_size);
   DrawText(hdc,s,-1,&rcControl[0],DT_SINGLELINE|DT_LEFT);
//   DrawText(hdc,cFormat[tex.format],-1,&rcControl[1],DT_SINGLELINE|DT_LEFT);
	SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_SETCURSEL,tex.format,0);
   wsprintf(s,": %s",tex.ws ? "Yes" : "No");
   DrawText(hdc,s,-1,&rcControl[2],DT_SINGLELINE|DT_LEFT);
   wsprintf(s,": %s",tex.wt ? "Yes" : "No");
   DrawText(hdc,s,-1,&rcControl[3],DT_SINGLELINE|DT_LEFT);
   wsprintf(s,": %s",tex.fs ? "Yes" : "No");
   DrawText(hdc,s,-1,&rcControl[4],DT_SINGLELINE|DT_LEFT);
   wsprintf(s,": %s",tex.ft ? "Yes" : "No");
   DrawText(hdc,s,-1,&rcControl[5],DT_SINGLELINE|DT_LEFT);
   DrawText(hdc,cColor[tex.format],-1,&rcControl[6],DT_SINGLELINE|DT_LEFT);
   wsprintf(s,": %s",tex.tr ? "Yes" : "No");
   DrawText(hdc,s,-1,&rcControl[7],DT_SINGLELINE|DT_LEFT);
   DrawText(hdc,cCoordTra[tex.coordMode],-1,&rcControl[8],DT_SINGLELINE|DT_LEFT);
   wsprintf(s,"0x%08X",tex.adr);
//	::SetWindowPos(GetDlgItem(m_hWnd,IDC_EDIT2),NULL,rcControl[9].left,rcControl[9].top,
//   	rcControl[9].right - rcControl[9].left,rcControl[9].bottom - rcControl[9].top,SWP_NOREPOSITION);
	::SetWindowText(GetDlgItem(m_hWnd,IDC_EDIT2),s);
   wsprintf(s,"0x%08X",tex.data);
	::SetWindowText(GetDlgItem(m_hWnd,IDC_EDIT3),s);
   wsprintf(s,": 0x%08X",tex.palData);
   DrawText(hdc,s,-1,&rcControl[11],DT_SINGLELINE|DT_LEFT);
Ex_DrawTextureBitmap:
   if(flag != 0)
       ReleaseDC(m_hWnd,hdc);
   return res;
}
//---------------------------------------------------------------------------
void LTextureViewer::UpdateTexture(HDC hdc)
{
   u8 flag;
   RECT rc,rc1,rc2;
   TEXTURE tex;
   PlugIn *p;
   void *p1[2];

   ::EnableWindow(GetDlgItem(m_hWnd,IDC_BUTTON2),FALSE);
   if(hdc == NULL){
       hdc = GetDC(m_hWnd);
       flag = 1;
   }
   else
       flag = 0;
   rc = rcTexture;
   EraseBackGround(hdc);
   if(indexTex > 0 && hBit != NULL){
       xScroll = yScroll = 0;
       p1[0] = NULL;
       if((p = pPlugInContainer->get_PlugInList(PIL_VIDEO)->get_ActivePlugIn(PIT_IS3D)) != NULL){
       	if(texData != -1)
           	p1[0] = (LPVOID)texData;
           else
           	p1[0] = NULL;
           p1[1] = (void *)(indexTex - 1);
           p->NotifyState(PNMV_GETTEXTUREINFO,PIS_NOTIFYMASK,(LPARAM)p1);
           if(p1[0] != NULL)
           	memcpy(&tex,p1[0],sizeof(TEXTURE));
       }
       if(p1[0] != NULL){
           ::SetRect(&rc1,0,0,tex.h_size,tex.v_size);
           if(hdcBitmap != NULL){
               if(hBitmap != NULL)
                   DeleteObject(hBitmap);
               hBitmap = NULL;
               ::DeleteDC(hdcBitmap);
               hdcBitmap = NULL;
           }
           iScale = 0;
           if((hdcBitmap = CreateCompatibleDC(hdc)) != NULL){
   			for(iScale = 10;iScale > 1;iScale--){
					hBitmap = CreateCompatibleBitmap(hdc,tex.h_size * iScale,tex.v_size * iScale);
           		if(hBitmap != NULL)
           			break;
               }
               if(hBitmap != NULL){
                   DrawTextureBitmap(hdc,FALSE);
                   ::EnableWindow(GetDlgItem(m_hWnd,IDC_BUTTON2),TRUE);
               }
           }
           if(iScale < 2)
               ShowWindow(GetDlgItem(m_hWnd,IDC_TRACK1),SW_HIDE);
           else{
               SendDlgItemMessage(m_hWnd,IDC_TRACK1,TBM_SETRANGE,FALSE,MAKELPARAM(0,iScale));
               SendDlgItemMessage(m_hWnd,IDC_TRACK1,TBM_SETPOS,TRUE,0);
               ShowWindow(GetDlgItem(m_hWnd,IDC_TRACK1),SW_SHOW);
           }
           rc2.left = rc.left + ((rc.right-rc.left - rc2.right) >> 1);
           rc2.top = rc.top + ((rc.bottom - rc.top - rc2.bottom) >> 1);
       }
   }
   else{
       ShowScrollBar(GetDlgItem(m_hWnd,IDC_VSBTEX),SB_CTL,FALSE);
       ShowScrollBar(GetDlgItem(m_hWnd,IDC_HSBTEX),SB_CTL,FALSE);
   }
   if(flag != 0)
       ReleaseDC(m_hWnd,hdc);
}
//---------------------------------------------------------------------------
LRESULT LTextureViewer::OnEditWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	char s[31];

	switch(GetDlgCtrlID(hwnd)){
       case IDC_EDIT2:
			switch(uMsg){
               case WM_KEYDOWN:
                   if(wParam == VK_RETURN){
                       if(indexTex > 0){
                           GetWindowText(hwnd,s,30);
                           texAddress = StrToHex(s);
                           DrawTexture(indexTex - 1);
                           UpdateTexture(NULL);
                       }
                       return 0;
                   }
               break;
               case WM_GETDLGCODE:
                   return DLGC_WANTARROWS|DLGC_WANTTAB|DLGC_WANTMESSAGE|DLGC_HASSETSEL|DLGC_DEFPUSHBUTTON
                   	|DLGC_UNDEFPUSHBUTTON|DLGC_RADIOBUTTON|DLGC_WANTCHARS|DLGC_STATIC|DLGC_BUTTON;
       	}
           return ::CallWindowProc(oldEditWndProc2,hwnd,uMsg,wParam,lParam);
       break;
       case IDC_EDIT3:
			switch(uMsg){
               case WM_KEYDOWN:
                   if(wParam == VK_RETURN){
                       if(indexTex > 0){
                           GetWindowText(hwnd,s,30);
                           texData = StrToHex(s);
                           DrawTexture(indexTex - 1);
                           UpdateTexture(NULL);
                       }
                       return 0;
                   }
               break;
               case WM_GETDLGCODE:
                   return DLGC_WANTARROWS|DLGC_WANTTAB|DLGC_WANTMESSAGE|DLGC_HASSETSEL|DLGC_DEFPUSHBUTTON
                   	|DLGC_UNDEFPUSHBUTTON|DLGC_RADIOBUTTON|DLGC_WANTCHARS|DLGC_STATIC|DLGC_BUTTON;
       	}
           return ::CallWindowProc(oldEditWndProc3,hwnd,uMsg,wParam,lParam);
       break;
   }

}
//---------------------------------------------------------------------------
LRESULT CALLBACK LTextureViewer::EditWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	LTextureViewer *pView;

   pView = (LTextureViewer *)GetWindowLong(hwnd,GWL_USERDATA);
   if(pView == NULL)
   	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
   return pView->OnEditWindowProc(hwnd,uMsg,wParam,lParam);
}
#endif
