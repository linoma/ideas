#include "lwindow.h"

#ifndef __BITMAPH__

#define __BITMAPH__

HBITMAP LoadBitmap(HINSTANCE instance,LPCTSTR name);
HBITMAP CreateBitmapMask(HBITMAP src,guint32 color0);
HICON LoadIcon(HINSTANCE instance,LPCSTR lpIconName);
HICON LoadIcon(HINSTANCE instance,LPCTSTR name, INT desiredx, INT desiredy, UINT loadflags);
#endif

