#include <stdio.h>
#include <stdlib.h>

#ifndef __WIN32__
	#include <X11/Xlib.h>
	#include <X11/Xutil.h>
	#include <X11/Xatom.h>
	#include <X11/keysym.h>
	#include <GL/gl.h>
	#include <GL/glx.h>
	#include <GL/glext.h>
	#include <gtk-2.0/gtk/gtk.h>
	#include <gtk-2.0/gdk/gdkx.h>
	#include <cairo/cairo.h>
	#include <sys/time.h>
	#include <sys/stat.h>
	#include <pthread.h>
	#include <unistd.h>
	#include <stdarg.h>
	#include <string.h>
	#include <ctype.h>
	#include <dlfcn.h>
	#include <dirent.h>
	#include "linux/mwm.h"
	#include "linux/lgdiobj.h"
	#include "linux/lwindow.h"
	#include "linux/lwindow_base.h"
	#include "linux/dialog.h"
	#include "linux/menu.h"
	#include "linux/bitmap.h"
	#include "linux/lcommctrl.h"
	#include "linux/ltreeview.h"
	#include "linux/ltabcontrol.h"
	#include "linux/cell_render_ownerdraw.h"
	#include "linux/ltimer.h"
	#include "linux/lcaret.h"
#else
	#include <windows.h>
	#include <commctrl.h>
	#include <shlobj.h>
	#include <gl\gl.h>
	#include <gl\glext.h>
	#include <gl\wglext.h>
#endif

#ifndef __ideastypesh__
#define __ideastypesh__

#ifdef _DEBPRO
#define _DEBPRO2
#define _DEBPRO3
#define _DEBPRO4
#define _DEBPRO5
#define _DEBPRO6
#endif
//------------------------------------------------------------------------------------------
struct IMessageBox
{
public:
	virtual int Show(HWND parent,LPCTSTR lpMessage,LPCTSTR lpCaption,DWORD dwStyle) PURE;
	virtual int ShowIndirect(LPMSGBOXPARAMS lpMsgBoxParams) PURE;
   virtual int Show(HWND parent,WORD wres,LPCTSTR lpCaption,DWORD dwStyle) PURE;
};
//------------------------------------------------------------------------------------------
struct IMenu
{
public:
	virtual BOOL GetMenuItemInfo(UINT uItem,BOOL fByPosition,LPMENUITEMINFO lpmii) PURE;
 	virtual BOOL InsertMenuItem(UINT uItem,BOOL fByPosition,LPMENUITEMINFO lpmii) PURE;
	virtual BOOL DeleteMenu(UINT uPosition,UINT uFlags) PURE;
  	virtual BOOL EnableMenuItem(UINT uIDEnableItem,UINT uEnable) PURE;
	virtual BOOL AppendMenu(UINT uFlags,UINT uIDNewItem,LPCTSTR lpNewItem) PURE;
  	virtual BOOL Attack(HMENU) PURE;
  	virtual BOOL Destroy() PURE;
  	virtual void Release() PURE;
  	virtual DWORD CheckMenuItem(UINT uIDCheckItem,UINT uCheck) PURE;
   virtual BOOL IsCheckedMenuItem(UINT uIDCheckItem) PURE;
  	virtual int GetMenuItemCount() PURE;
  	virtual IMenu *GetSubMenu(int nPos) PURE;
   virtual HMENU Handle() PURE;
};
//------------------------------------------------------------------------------------------
#ifndef __LStream__
#define __LStream__
struct LStream
{
public:
	virtual BOOL Open(DWORD dwStyle = GENERIC_READ,DWORD dwCreation = OPEN_EXISTING,DWORD dwFlags = 0) PURE;
   virtual void Close() PURE;
   virtual DWORD Read(LPVOID lpBuffer,DWORD dwBytes) PURE;
   virtual DWORD Write(LPVOID lpBuffer,DWORD dwBytes) PURE;
   virtual DWORD Seek(LONG dwDistanceToMove = 0,DWORD dwMoveMethod = FILE_BEGIN) PURE;
   virtual BOOL SeekToBegin() PURE;
   virtual BOOL SeekToEnd() PURE;
   virtual DWORD Size(LPDWORD lpHigh = NULL) PURE;
   virtual BOOL SetEndOfFile(DWORD dw) PURE;
   virtual DWORD GetCurrentPosition() PURE;
	virtual BOOL IsOpen() PURE;
   virtual void Release() PURE;
};
#endif
//------------------------------------------------------------------------------------------
#ifndef __LCompressedFile__
#define __LCompressedFile__
//---------------------------------------------------------------------------
typedef enum {NORMAL=0,TEMP,AUTO} COMPRESSEDMODE;
//---------------------------------------------------------------------------
typedef struct{
   char fileName[MAX_PATH];
   DWORD dwSize,dwSizeCompressed,dwFlags;
} COMPRESSEDFILEINFO,*LPCOMPRESSEDFILEINFO;
//---------------------------------------------------------------------------
struct LCompressedFile : public LStream
{
	virtual void SetFileStream(LStream *pStream) PURE;
	virtual BOOL AddCompressedFile(const char *lpFileName,const int iLevel = 9) PURE;
   virtual BOOL DeleteCompressedFile(DWORD index) PURE;
   virtual DWORD ReadCompressedFile(LPVOID buf,DWORD dwByte) PURE;
   virtual BOOL OpenCompressedFile(WORD uIndex,COMPRESSEDMODE mode=NORMAL) PURE;
   virtual DWORD WriteCompressedFile(LPVOID buf,DWORD dwByte) PURE;
   virtual void Rebuild() PURE;
   virtual BOOL get_FileCompressedInfo(DWORD index,LPCOMPRESSEDFILEINFO p) PURE;
   virtual DWORD Count() PURE;
};
#endif
//------------------------------------------------------------------------------------------
struct LBase
{
public:
   virtual BOOL Save(LStream *) PURE;
   virtual BOOL Load(LStream *,int) PURE;
};
//------------------------------------------------------------------------------------------
#ifndef __IWnd__
#define __IWnd__

#define IID_DOWNLCD   1
#define IID_UPLCD     2

struct IWnd
{
public:
	virtual HDC DC() PURE;
   virtual HWND Handle() PURE;
   virtual void Release() PURE;
   virtual BOOL Create() PURE;
   virtual BOOL Map() PURE;
};
#endif
//------------------------------------------------------------------------------------------
#ifndef __IFat__
#define __IFat__

#define IID_IFAT   10
struct IFat
{
public:
	virtual DWORD Read(DWORD,BYTE) PURE;
   virtual BOOL Seek(DWORD,BYTE) PURE;
   virtual IFat *Create(const char *) PURE;
   virtual void Release() PURE;
   virtual int get_Path(char *,int) PURE;
};
#endif


#ifndef __IPlugInInterface__
#define __IPlugInInterface__

#define OID_IO_MEMORY9		    1
#define OID_IO_MEMORY7		    2
#define OID_VRAM_MEMORY	    3
#define OID_PALETTE_MEMORY	    4
#define OID_OAM_MEMORY		    5
#define OID_EXTERN_WRAM	    6
#define OID_FIRMWARE_MEMORY    7
#define OID_READ_BYTE		    10
#define OID_READ_HWORD		    11
#define OID_READ_WORD		    12
#define OID_RUNFUNC		    13
#define OID_POWERMNG           14

struct IPlugInInterface
{
public:
   virtual int get_Format(DWORD,LPVOID,LPDWORD) PURE;
   virtual int WriteTable(DWORD,LPVOID,LPVOID) PURE;
   virtual int ResetTable() PURE;
   virtual int TriggerIRQ() PURE;
   virtual int get_Object(int,LPVOID *) PURE;
};

#endif


#ifndef __IPlugInManager__
#define __IPlugInManager__

#define IID_IPLUGINMANAGER     3

typedef struct
{
   DWORD type;
   USHORT index;
   union{
       struct {
           void *mem1;
           DWORD len1;
           void *mem2;
           DWORD len2;
           int syncro;
       } audio;
       struct {
           USHORT *mem1;
           USHORT *mem2;
           int render;
           int lcd_mask;
       } graphics;
       struct {
           DWORD x;
           DWORD y;
   	} render;
       struct {
           DWORD *value;
       } pad;
   };
} NOTIFYCONTENT,*LPNOTIFYCONTENT;


struct IPlugInManager
{
public:
   virtual int NotifyState(DWORD,LPVOID) PURE;
   virtual int get_PlugInFormat(DWORD,LPVOID,LPDWORD) PURE;
   virtual int get_PlugInInterface(GUID *,LPVOID *) PURE;
   virtual int get_ActivePlugIn(int,LPVOID *) PURE;
};

#endif

#ifndef __INDS__
#define __INDS__

#ifndef PT_SAVEGAME

#define PT_SAVEGAME	1
#define PT_SAVESTATE	2
#define PT_SCREENSHOT	3
#define PT_CHEAT		4
#define PT_GBA_SG_EEPROM	5
#define PT_GBA_SG_FLASH	6
#endif

struct INDS
{
public:
   virtual int QueryInterface(int,LPVOID *) PURE;
   virtual int get_FramesCount(LPDWORD) PURE;
   virtual int get_CurrentFileName(char *,int *) PURE;
   virtual int CreateFileName(int,char *,int *,char *,int) PURE;
   virtual int WriteConsole(char *) PURE;
   virtual int get_IsHomebrew(int *) PURE;
   virtual int get_EmulatorMode(int *) PURE;
   virtual int get_NumberOfCores(LPDWORD) PURE;
   virtual int get_DutyCycles(LPDWORD) PURE;
};

#endif

#ifndef __INDSMemory__
#define __INDSMemory__

#define IID_IMEMORY    8624

struct INDSMemory
{
public:
    virtual int get_VideoMemoryStatus(DWORD,DWORD,LPDWORD) PURE;
    virtual int set_VideoMemoryStatus(DWORD adr,DWORD size,DWORD status) PURE;
};
#endif


#ifdef __WIN32__
   typedef struct
   {
       float left;
       float top;
       float right;
       float bottom;
   } RECTF,*LPRECTF;

   typedef struct
   {
       UINT Width;
       UINT Height;
       INT Stride;
       INT PixelFormat;
       LPVOID Scan0;
       UINT Reserved;
   } BITMAPDATA,*LPBITMAPDATA;

   typedef struct
   {
       UINT GdiplusVersion;
       LPVOID DebugEventCallback;
       BOOL SuppressBackgroundThread;
       BOOL SuppressExternalCodecs;
   } GDIPSTARTUPINPUT,*LPGDIPSTARTUPINPUT;

   typedef int __stdcall (*LPGDIPBITMAPLOCKBITS)(LPVOID,LPRECT,UINT,INT,LPBITMAPDATA);
   typedef int __stdcall (*LPGDIPBITMAPUNLOCKBITS)(LPVOID,LPBITMAPDATA);
   typedef int __stdcall (*LPGDIPDRAWIMAGERECT)(LPVOID,LPVOID,float,float,float,float);
   typedef int __stdcall (*LPGDIPDRAWIMAGERECTI)(LPVOID,LPVOID,INT,INT,INT,INT);
   typedef int __stdcall (*LPGDIPCREATEBITMAPFROMGDIDIB)(BITMAPINFO*,LPVOID,LPVOID *);
   typedef int __stdcall (*LPGDIPCREATEBITMAPFROMHBITMAP)(HBITMAP,HPALETTE,LPVOID *);
   typedef int __stdcall (*LPGDIPDISPOSEIMAGE)(LPVOID);
   typedef int __stdcall (*LPGDIPRELEASEDC)(LPVOID,HDC);
   typedef int __stdcall (*LPGDIPSHUTDOWN)(ULONG);
   typedef int __stdcall (*LPGDIPSTARTUP)(ULONG *,LPGDIPSTARTUPINPUT,LPVOID);
   typedef int __stdcall (*LPGDIPCREATEFROMHDC)(HDC,LPVOID *);
   typedef int __stdcall (*LPGDIPDRAWIMAGERECTRECTI)(LPVOID,LPVOID,INT,INT,INT,INT,INT,INT,INT,INT,INT,LPVOID,LPVOID,LPVOID);
   typedef int __stdcall (*LPGDIPDELETEGRAPHICS)(LPVOID);

   extern LPGDIPBITMAPLOCKBITS pfnGdipBitmapLockBits;
   extern LPGDIPBITMAPUNLOCKBITS pfnGdipBitmapUnlockBits;
   extern LPGDIPDRAWIMAGERECTI pfnGdipDrawImageRectI;
   extern LPGDIPCREATEBITMAPFROMHBITMAP pfnGdipCreateBitmapFromHBITMAP;
   extern LPGDIPRELEASEDC pfnGdipReleaseDC;
   extern LPGDIPDISPOSEIMAGE pfnGdipDisposeImage;
   extern LPGDIPCREATEFROMHDC pfnGdipCreateFromHDC;
   extern LPGDIPCREATEBITMAPFROMGDIDIB pfnGdipCreateBitmapFromGDIDIB;
   extern LPGDIPDELETEGRAPHICS pfnGdipDeleteGraphics;

#endif

#endif


