#include "lcanvas.h"
#include "lapp.h"

//---------------------------------------------------------------------------
LCanvas::LCanvas()
{
   hBitmap = NULL;
   hDC = hDC1 = NULL;
   screen = NULL;
   bStretch = false;
   ::SetRect(&rcDest,0,0,256,192);
   ::SetRect(&rcDisplay,0,0,256,192);
   szSource.cx = 256;
   szSource.cy = 192;
#ifndef __WIN32__
	ptOrg.x = ptOrg.y = 0;
#else
   graphics = NULL;
   image = NULL;
   pfnGdipBitmapLockBits = NULL;
   pfnGdipBitmapUnlockBits = NULL;
   pfnGdipDrawImageRectI = NULL;
#endif
}
//---------------------------------------------------------------------------
LCanvas::~LCanvas()
{
	Destroy();
}
//---------------------------------------------------------------------------
void LCanvas::set_WorkArea(RECT rc,SIZE sz)
{
   rcDest = rc;
   szSource = sz;
   bStretch = (rc.right != 256 || rc.bottom != 192) ? true : false;
#ifdef __WIN32__
   bminfo.bmiHeader.biWidth = sz.cx;
   bminfo.bmiHeader.biHeight = -sz.cy;
   if(bStretch){
       if(image != NULL){
           pfnGdipDisposeImage(image);
           image = NULL;
       }
       if(graphics != NULL){
           pfnGdipDeleteGraphics(graphics);
           graphics = NULL;
       }
   }
#else
	if(hBitmap != NULL)
		DeleteObject(hBitmap);
	hBitmap = gdk_pixbuf_new(GDK_COLORSPACE_RGB,false,8,sz.cx,sz.cy);
#endif
}
//---------------------------------------------------------------------------
void LCanvas::get_WorkArea(RECT &rc,SIZE &sz)
{
   rc = rcDest;
   sz = szSource;
}
//---------------------------------------------------------------------------
/*void LCanvas::SetRect(int right,int bottom)
{
   bStretch = false;
   rcDest.right = right;
   if(right != 256)
       bStretch = true;
   rcDest.bottom = bottom;
   if(bottom != 192)
       bStretch = true;
}
//---------------------------------------------------------------------------
void LCanvas::SetRect(int left,int top,int right,int bottom)
{
   SetRect(right,bottom);
   rcDest.left = left;
   rcDest.top = top;
}*/
//---------------------------------------------------------------------------
void LCanvas::Destroy()
{
	if(hDC1 != NULL)
		::DeleteDC(hDC1);
   hDC1 = NULL;
	ReleaseDC();
   if(hBitmap != NULL)
		::DeleteObject(hBitmap);
#ifndef __WIN32__
	if(screen != NULL)
		free(screen);
#endif
	screen = NULL;
   hBitmap = NULL;
}
//---------------------------------------------------------------------------
void LCanvas::ReleaseDC()
{
	if(hDC == NULL)
		return;
#ifdef __WIN32__
   if(graphics != NULL){
       if(image != NULL){
           pfnGdipDisposeImage(image);
           image = NULL;
       }
       pfnGdipDeleteGraphics(graphics);
       graphics = NULL;
   }
	::ReleaseDC(g_hWnd,hDC);
#endif
	hDC = NULL;
}
//---------------------------------------------------------------------------
int LCanvas::GetDC(HWND hwnd)
{
	if(hDC != NULL)
		ReleaseDC();
	g_hWnd = hwnd;
#ifdef __WIN32__
	hDC = ::GetDC(g_hWnd);
	if(hDC == NULL)
		return FALSE;
#endif
	return TRUE;
}
//---------------------------------------------------------------------------
int LCanvas::Init(HWND hwnd)
{
	if(hDC == NULL && !GetDC(hwnd))
       return FALSE;
#ifdef __WIN32__
	ZeroMemory(&bminfo,sizeof(BITMAPINFO));
   bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bminfo.bmiHeader.biWidth = 256;
   bminfo.bmiHeader.biHeight = -192;
   bminfo.bmiHeader.biPlanes = 1;
   bminfo.bmiHeader.biBitCount = 16;
   bminfo.bmiHeader.biCompression = BI_RGB;
   hBitmap = CreateDIBSection(hDC,&bminfo,DIB_RGB_COLORS,(LPVOID *)&screen,NULL,0);
	if(hBitmap == NULL)
   	return FALSE;
   hDC1 = CreateCompatibleDC(NULL);
   if(hDC1 == NULL)
   	return FALSE;
   ::SelectObject(hDC1,hBitmap);
//	::GetClientRect(hwnd,&rcDest);
#else
	Reset();
	hBitmap = gdk_pixbuf_new(GDK_COLORSPACE_RGB,false,8,256,192);
	if(hBitmap == NULL)
		return FALSE;
	screen = (char *)malloc(256*192*sizeof(unsigned short));
	if(screen == NULL)
		return FALSE;
#endif
   //::GetClientRect(hwnd,&rcDest);
  	return TRUE;
}
//---------------------------------------------------------------------------
void LCanvas::Reset()
{
 	set_OutBuffer();
	if(screen != NULL)
       memset(screen,0,256*192*sizeof(short));
}
//---------------------------------------------------------------------------
void LCanvas::set_OutBuffer(char *buf)
{
   if(buf == NULL)
       buf = screen;
   tempBuffer = buf;
}
//---------------------------------------------------------------------------
int LCanvas::BitBlt(HDC hdc,char *buffer)
{
#ifdef __WIN32__
   BITMAPDATA bmd;
#endif

	if(hdc == NULL)
       hdc = hDC;
#ifdef __WIN32__
   if(bStretch || buffer != NULL){
       if(buffer == NULL)
           buffer = screen;
       if(pfnGdipCreateBitmapFromGDIDIB != NULL){
           if(image == NULL)
               pfnGdipCreateBitmapFromGDIDIB(&bminfo,buffer,&image);
           if(pfnGdipBitmapLockBits(image,NULL,2,(5 | (16 << 8)|0x00020000),&bmd) == 0){
               memcpy(bmd.Scan0,buffer,256*192*2);
               pfnGdipBitmapUnlockBits(image,&bmd);
           }
           if(graphics == NULL)
               pfnGdipCreateFromHDC(hDC,&graphics);
           pfnGdipDrawImageRectI(graphics,image,rcDest.left,rcDest.top,rcDest.right,rcDest.bottom);
//           pfnGdipDrawImageRectRectI(graphics,image,rcDest.left,rcDest.top,rcDest.right,rcDest.bottom,
//               0,0,szSource.cx,szSource.cy,2,NULL,NULL,NULL);
           return TRUE;
       }
       return (int)(StretchDIBits(hdc,rcDest.left,rcDest.top,rcDest.right,rcDest.bottom,0,0,szSource.cx,szSource.cy,buffer,&bminfo,DIB_RGB_COLORS,SRCCOPY) == GDI_ERROR ? FALSE : TRUE);
   }
	return (int)::BitBlt(hdc,rcDisplay.left,rcDisplay.top,rcDisplay.right,rcDisplay.bottom,hDC1,0,0,SRCCOPY);
#else
	char *p,*p2;
	unsigned short *p1,c;
	int x,y,bytes_perline;
	HBITMAP hBitScale;

   if(buffer == NULL)
       buffer = screen;
	p1 = (unsigned short *)buffer;
	if((p = (char *)gdk_pixbuf_get_pixels(hBitmap)) == NULL)
		return FALSE;
	bytes_perline = gdk_pixbuf_get_rowstride(hBitmap);
	for(y=0;y<szSource.cy;y++){
		p2 = p;
		for(x=0;x<szSource.cx;x++){
			c = *p1++;
			*p2++ = (((c >> 10) & 0x1F) << 3);
			*p2++ = (((c >> 5) & 0x1F) << 3);
			*p2++ = ((c & 0x1F) << 3);
		}
		p += bytes_perline;
  	}
	if(bStretch){
		hBitScale = gdk_pixbuf_scale_simple(hBitmap,rcDest.right,rcDest.bottom,GDK_INTERP_BILINEAR);
		gdk_draw_pixbuf(g_hWnd->window,NULL,hBitScale,0,0,ptOrg.x+rcDest.left,ptOrg.y+rcDest.top,-1,-1,GDK_RGB_DITHER_NONE,0,0);
       DeleteObject(hBitScale);
	}
	else
		gdk_draw_pixbuf(g_hWnd->window,NULL,hBitmap,0,0,ptOrg.x+rcDisplay.left,ptOrg.y+rcDisplay.top,rcDisplay.right,rcDisplay.bottom,GDK_RGB_DITHER_NONE,0,0);
	return TRUE;
#endif
}

//---------------------------------------------------------------------------
void LCanvas::SetOrg(int x,int y)
{
#ifdef __WIN32__
	SetWindowOrgEx(hDC,x,-y,NULL);
#else
	ptOrg.x = x;
	ptOrg.y = y;
#endif
}

