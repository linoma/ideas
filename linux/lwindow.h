#ifndef __WIN32__

#ifndef __LWINDOWH__
#define __LWINDOWH__
	typedef GtkWidget *     	HWND;
	typedef cairo_t*			HDC;
 	typedef void *				HDWP;
	typedef GdkPixbuf *			HBITMAP;
  	typedef GdkEvent			MSG;
	typedef HBITMAP				HICON;
	typedef void * 				HGDIOBJ;
	typedef void *				HACCEL;
	typedef void *				HPALETTE;
	typedef GdiObject *			HFONT;
	typedef void *				HRGN;
	typedef void *				HBRUSH;
	typedef void *				HPEN;
	typedef long 				LRESULT;
	typedef long 				LPARAM;
	typedef unsigned int 		WPARAM;
	typedef char * 				LPCTSTR;
	typedef char *				LPTSTR;
	typedef unsigned int 		BOOL;
	typedef BOOL *				LPBOOL;
	typedef void				VOID;
	typedef void *				LPVOID;
	typedef unsigned long 		DWORD;
	typedef unsigned short 		WORD;
	typedef signed long			LONG;
    typedef LONG *              LPLONG;
	typedef GtkMenuShell *    	HMENU;
	typedef void * 				HANDLE;
	typedef unsigned long *		LPDWORD;
	typedef MSG *				LPMSG;
	typedef char *				LPCSTR;
	typedef LPVOID			    HINSTANCE;
	typedef LPVOID				HMODULE;
	typedef unsigned short *	LPWORD;
	typedef unsigned char 		BYTE;
	typedef BYTE *				LPBYTE;
	typedef unsigned long		COLORREF;
	typedef signed int			INT;
	typedef unsigned int 		UINT;
	typedef UINT *  			UINT_PTR;
	typedef void *				HKEY;
	typedef char *				LPSTR;
	typedef unsigned long *     ULONG_PTR;
	typedef wchar_t *			LPWSTR;
	typedef unsigned long		HRESULT;
  	typedef HANDLE              HGLOBAL;
  	typedef HANDLE              HLOCAL;
  	typedef HANDLE              GLOBALHANDLE;
  	typedef HANDLE              LOCALHANDLE;
	typedef char				CHAR;
	typedef unsigned long		ULONG;
	typedef unsigned short		USHORT;
	typedef unsigned short		WCHAR;
	typedef WCHAR *	    		LPCWSTR;
	typedef unsigned short		SHORT;
	typedef GtkSelectionData *	HDROP;
	typedef unsigned long long  ULONGLONG;

	#define HIBYTE(b)			(BYTE)(b >> 8)
	#define LOBYTE(v)			(BYTE)v
	#define LOWORD(v) 			(WORD)v
	#define HIWORD(v)			(WORD)(((DWORD)v) >> 16)
	#define MAKELONG(l,h)		(LONG)(((h) << 16) | ((WORD)l))
	#define MAKEWORD(l,h)		(WORD)(((h) << 8) | ((BYTE)(l)))
	#define MAKEWPARAM(l,h)		(WPARAM)MAKELONG(l,h)
	#define MAKELPARAM(l,h)		(LPARAM)MAKELONG(l,h)
	#define MAKEINTRESOURCE(i) 	((char *)((WORD)(i)))
	#define MAKEINTRESOURCEW(i) ((WCHAR *)((WORD)(i)))

	#ifndef TRUE
		#define TRUE	1
	#endif
	#ifndef FALSE
		#define FALSE   0
	#endif

	#ifndef APIENTRY
	#define APIENTRY
	#endif

	#define WINAPI
	#define FAR
	#define NEAR
	#define _USERENTRY
	#define CALLBACK
	#define UNALIGNED
	#define CONST const
	#define PURE = 0
	#define MAX_PATH          				4096

	typedef int (FAR WINAPI *FARPROC)();
	typedef UINT APIENTRY (*LPOFNHOOKPROC)(HWND,UINT,WPARAM,LPARAM);
	typedef UINT (CALLBACK* DLGPROC)(HWND, UINT, WPARAM, LPARAM);
	typedef LRESULT (CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
	typedef VOID (CALLBACK* TIMERPROC)(HWND, UINT, UINT, DWORD);
	typedef DWORD LCID;
	typedef LPDWORD PLCID;
	typedef WORD LANGID;

	typedef struct _tagRECT{
		LONG left,top,right,bottom;
	} RECT,*LPRECT;

	typedef struct _tagPOINT{
		LONG x,y;
	} POINT,*LPPOINT;

	typedef struct _tagSIZE{
		LONG cx,cy;
	} SIZE,*LPSIZE;

	typedef struct tagDLGTEMPLATE{
    	DWORD style;
    	DWORD dwExtendedStyle;
    	WORD cdit;
    	short x;
    	short y;
    	short cx;
    	short cy;
	} DLGTEMPLATE;

	typedef DLGTEMPLATE *LPDLGTEMPLATE;
	typedef const DLGTEMPLATE *LPCDLGTEMPLATE;

	typedef struct{
    	DWORD      	style;
    	DWORD      	exStyle;
    	DWORD      	helpId;
    	short      	x;
    	short      	y;
    	short      	cx;
    	short      	cy;
    	DWORD   	id;
    	LPCWSTR    	className;
		DWORD		classId;
    	LPCWSTR    	windowName;
    	LPVOID    	data;
	} DLG_CONTROL_INFO;

   typedef struct {
       BOOL dialogEx;
       DWORD style;
       DWORD exstyle;
       LPCWSTR caption;
       LPCWSTR className;
       LPCWSTR menuName;
       WORD x;
       WORD y;
       WORD cx;
       WORD cy;
       WORD nItems;
       WORD pointSize;
       WORD weight;
       WORD italic;
       LPCWSTR faceName;
   } DLG_INFO, *LPDLG_INFO;

	typedef struct{
    	WORD   fVirt;
    	WORD   key;
    	WORD   cmd;
    	WORD   pad;
	} PE_ACCEL, *LPPE_ACCEL;

	typedef struct tagOFN { // ofn
    	DWORD         lStructSize;
    	HWND          hwndOwner;
    	HINSTANCE     hInstance;
    	LPCTSTR       lpstrFilter;
    	LPTSTR        lpstrCustomFilter;
    	DWORD         nMaxCustFilter;
    	DWORD         nFilterIndex;
    	LPTSTR        lpstrFile;
    	DWORD         nMaxFile;
    	LPTSTR        lpstrFileTitle;
    	DWORD         nMaxFileTitle;
    	LPCTSTR       lpstrInitialDir;
    	LPCTSTR       lpstrTitle;
    	DWORD         Flags;
    	WORD          nFileOffset;
    	WORD          nFileExtension;
    	LPCTSTR       lpstrDefExt;
    	DWORD         lCustData;
    	LPOFNHOOKPROC lpfnHook;
    	LPCTSTR       lpTemplateName;
	} OPENFILENAME,*LPOPENFILENAME;

	typedef  struct  _tagHELPINFO {
    	UINT        cbSize;
    	int         iContextType;
    	int         iCtrlId;
    	HANDLE      hItemHandle;
    	DWORD       dwContextId;
    	POINT       MousePos;
	} HELPINFO, *LPHELPINFO;

	typedef void CALLBACK (*MSGBOXCALLBACK)(LPHELPINFO);

	typedef struct {
    	UINT            cbSize;
    	HWND            hwndOwner;
    	HINSTANCE       hInstance;
    	LPCSTR          lpszText;
    	LPCSTR          lpszCaption;
    	DWORD           dwStyle;
    	LPCSTR          lpszIcon;
    	DWORD           dwContextHelpId;
    	MSGBOXCALLBACK  lpfnMsgBoxCallback;
    	DWORD           dwLanguageId;
	} MSGBOXPARAMS, *PMSGBOXPARAMS, *LPMSGBOXPARAMS;

	typedef struct _RES_HEADER{
		DWORD	    DataSize;
		DWORD	    HeaderSize;
		DWORD	    ResType;
		DWORD	    ResName;
		WORD	    MemoryFlags;
		WORD	    LanguageID;
		DWORD 	    Version;
		DWORD 	    Characteristics;
	} RES_HEADER,*LPRES_HEADER;

	typedef struct {
  		BYTE rgbBlue;
  		BYTE rgbGreen;
  		BYTE rgbRed;
  		BYTE rgbReserved;
	} RGBQUAD, *LPRGBQUAD;

	typedef struct {
  		BYTE rgbtBlue;
  		BYTE rgbtGreen;
  		BYTE rgbtRed;
	} RGBTRIPLE;

	typedef struct{
    	DWORD 	biSize;
    	LONG  	biWidth;
    	LONG  	biHeight;
    	WORD 	biPlanes;
    	WORD 	biBitCount;
    	DWORD 	biCompression;
    	DWORD 	biSizeImage;
    	LONG  	biXPelsPerMeter;
    	LONG  	biYPelsPerMeter;
    	DWORD 	biClrUsed;
    	DWORD 	biClrImportant;
	} BITMAPINFOHEADER, *PBITMAPINFOHEADER, *LPBITMAPINFOHEADER;

	typedef struct tagBITMAPINFO{
    	BITMAPINFOHEADER bmiHeader;
    	RGBQUAD          bmiColors[1];
	} BITMAPINFO, *PBITMAPINFO, *LPBITMAPINFO;

	typedef struct{
    	DWORD   bcSize;
    	WORD    bcWidth;
    	WORD    bcHeight;
    	WORD    bcPlanes;
    	WORD    bcBitCount;
	} BITMAPCOREHEADER, *PBITMAPCOREHEADER, *LPBITMAPCOREHEADER;

	typedef struct{
    	BITMAPCOREHEADER bmciHeader;
    	RGBTRIPLE        bmciColors[1];
	} BITMAPCOREINFO, *PBITMAPCOREINFO, *LPBITMAPCOREINFO;

	typedef struct tagBITMAP{
    	LONG        bmType;
    	LONG        bmWidth;
    	LONG        bmHeight;
    	LONG        bmWidthBytes;
    	WORD        bmPlanes;
    	WORD        bmBitsPixel;
    	LPVOID      bmBits;
  	} BITMAP, *LPBITMAP;

	typedef struct tagMENUITEMINFOA{
    	UINT      	cbSize;
    	UINT      	fMask;
    	UINT      	fType;         // used if MIIM_TYPE (4.0) or MIIM_FTYPE (>4.0)
    	UINT      	fState;        // used if MIIM_STATE
    	UINT      	wID;           // used if MIIM_ID
    	HMENU     	hSubMenu;      // used if MIIM_SUBMENU
    	HBITMAP   	hbmpChecked;   // used if MIIM_CHECKMARKS
    	HBITMAP  	 hbmpUnchecked; // used if MIIM_CHECKMARKS
    	DWORD 		dwItemData;    // used if MIIM_DATA
    	LPSTR     	dwTypeData;    // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
    	UINT      	cch;           // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
		UINT		empty;
	} MENUITEMINFO, *LPMENUITEMINFO;

	typedef struct tagMINMAXINFO {
    	POINT ptReserved;
    	POINT ptMaxSize;
    	POINT ptMaxPosition;
    	POINT ptMinTrackSize;
    	POINT ptMaxTrackSize;
	} MINMAXINFO, *PMINMAXINFO, *LPMINMAXINFO;

	typedef struct tagACCEL {
    	BYTE   fVirt;
    	WORD   key;
    	WORD   cmd;
	} ACCEL, *LPACCEL;

	typedef struct tagMEASUREITEMSTRUCT {
    	UINT       CtlType;
    	UINT       CtlID;
    	UINT       itemID;
    	UINT       itemWidth;
    	UINT       itemHeight;
    	ULONG_PTR  itemData;
	} MEASUREITEMSTRUCT, *LPMEASUREITEMSTRUCT;

	typedef struct tagDRAWITEMSTRUCT {
    	UINT        CtlType;
    	UINT        CtlID;
    	UINT        itemID;
    	UINT        itemAction;
    	UINT        itemState;
    	HWND        hwndItem;
    	HDC         hDC;
    	RECT        rcItem;
    	ULONG_PTR   itemData;
	} DRAWITEMSTRUCT, *LPDRAWITEMSTRUCT;

	typedef struct tWAVEFORMATEX {
		WORD wFormatTag;
		WORD nChannels;
		DWORD nSamplesPerSec;
		DWORD nAvgBytesPerSec;
		WORD nBlockAlign;
		WORD wBitsPerSample;
		WORD cbSize;
	} WAVEFORMATEX,*PWAVEFORMATEX,*LPWAVEFORMATEX;

  	typedef struct _GUID {
  		unsigned long Data1;
    	unsigned short Data2;
    	unsigned short Data3;
    	unsigned char Data4[8];
	} GUID, *LPGUID;

  	typedef size_t FILETIME,*LPFILETIME;
	//typedef FILETIME *LPFILETIME;

  	typedef struct _SYSTEMTIME {
       WORD wYear;
       WORD wMonth;
       WORD wDayOfWeek;
       WORD wDay;
       WORD wHour;
       WORD wMinute;
       WORD wSecond;
       WORD wMilliseconds;
  	} SYSTEMTIME, *PSYSTEMTIME, FAR *LPSYSTEMTIME;

    typedef struct _SYSTEM_INFO {
        union {
            DWORD dwOemId;
            struct {
                WORD wProcessorArchitecture;
                WORD wReserved;
            };
        };
        DWORD dwPageSize;
        LPVOID lpMinimumApplicationAddress;
        LPVOID lpMaximumApplicationAddress;
        LPDWORD dwActiveProcessorMask;
        DWORD dwNumberOfProcessors;
        DWORD dwProcessorType;
        DWORD dwAllocationGranularity;
        WORD wProcessorLevel;
        WORD wProcessorRevision;
    } SYSTEM_INFO, *LPSYSTEM_INFO;

  	typedef struct _WIN32_FIND_DATA {
       DWORD       dwFileAttributes;
       FILETIME    ftCreationTime;
       FILETIME    ftLastAccessTime;
       FILETIME    ftLastWriteTime;
       DWORD       nFileSizeHigh;
       DWORD       nFileSizeLow;
       DWORD       dwReserved0;
       DWORD       dwReserved1;
       CHAR        cFileName[ MAX_PATH ];
       CHAR        cAlternateFileName[ 16 ];
  	} WIN32_FIND_DATA, *PWIN32_FIND_DATA, *LPWIN32_FIND_DATA;

	typedef struct tagPAINTSTRUCT {
    	HDC         hdc;
    	BOOL        fErase;
    	RECT        rcPaint;
    	BOOL        fRestore;
    	BOOL        fIncUpdate;
    	BYTE        rgbReserved[32];
	} PAINTSTRUCT, *PPAINTSTRUCT, *NPPAINTSTRUCT, *LPPAINTSTRUCT;

	typedef struct _SHITEMID        // mkid
	{
    	USHORT      cb;             // Size of the ID (including cb itself)
    	BYTE        abID[1];        // The item ID (variable length)
	} SHITEMID;

	typedef UNALIGNED SHITEMID *LPSHITEMID;
	typedef const UNALIGNED SHITEMID *LPCSHITEMID;

	typedef struct _ITEMIDLIST      // idl
	{
    	SHITEMID    mkid;
	} ITEMIDLIST;

	typedef UNALIGNED ITEMIDLIST * LPITEMIDLIST;
	typedef const UNALIGNED ITEMIDLIST * LPCITEMIDLIST;
	typedef int (CALLBACK* BFFCALLBACK)(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
	typedef struct _browseinfo {
    	HWND        hwndOwner;
    	LPCITEMIDLIST pidlRoot;
    	LPSTR        pszDisplayName;	// Return display name of item selected.
    	LPCSTR       lpszTitle;			// text to go in the banner over the tree.
    	UINT         ulFlags;			// Flags that control the return stuff
    	BFFCALLBACK  lpfn;
    	LPARAM       lParam;			// extra info that's passed back in callbacks
    	int          iImage;			// output var: where to return the Image index.
	} BROWSEINFO, *PBROWSEINFO, *LPBROWSEINFO;

	typedef struct tagSCROLLINFO{
    	UINT    cbSize;
    	UINT    fMask;
    	int     nMin;
    	int     nMax;
    	UINT    nPage;
    	int     nPos;
   		int     nTrackPos;
	} SCROLLINFO, *LPSCROLLINFO;

	typedef SCROLLINFO CONST *LPCSCROLLINFO;

	typedef struct tagLOGFONT
	{
    	LONG      lfHeight;
    	LONG      lfWidth;
    	LONG      lfEscapement;
    	LONG      lfOrientation;
    	LONG      lfWeight;
    	BYTE      lfItalic;
    	BYTE      lfUnderline;
    	BYTE      lfStrikeOut;
    	BYTE      lfCharSet;
    	BYTE      lfOutPrecision;
    	BYTE      lfClipPrecision;
    	BYTE      lfQuality;
    	BYTE      lfPitchAndFamily;
    	CHAR      lfFaceName[256];
	} LOGFONT, *LPLOGFONT;

#pragma pack(1)
	typedef struct
	{
    	WORD    bfType;
    	DWORD   bfSize;
    	WORD    bfReserved1;
    	WORD    bfReserved2;
    	DWORD   bfOffBits;
	} BITMAPFILEHEADER, *PBITMAPFILEHEADER, *LPBITMAPFILEHEADER;

	typedef struct
	{
    	BYTE bWidth;
    	BYTE bHeight;
    	BYTE bColorCount;
    	BYTE bReserved;
	} ICONRESDIR;

	typedef struct
	{
    	WORD wWidth;
    	WORD wHeight;
	} CURSORDIR;

	typedef struct
	{
		union{
    		ICONRESDIR icon;
      		CURSORDIR  cursor;
    	} ResInfo;
    	WORD   wPlanes;
    	WORD   wBitCount;
    	DWORD  dwBytesInRes;
    	WORD   wResId;
	} CURSORICONDIRENTRY;

	typedef struct
	{
    	WORD                idReserved;
    	WORD                idType;
   	 	WORD                idCount;
    	CURSORICONDIRENTRY  idEntries[1];
	} CURSORICONDIR;
#pragma pack(4)

	#ifndef INITGUID
		#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
   		extern "C" const GUID name
	#else
		#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        	extern "C" const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
	#endif // INITGUID

	#define DIB_RGB_COLORS          0
	#define DIB_PAL_COLORS          1
	#define CBM_INIT                4

	#define CW_USEDEFAULT       	((int)0x80000000)

	#define WS_OVERLAPPED           0x00000000L
	#define WS_POPUP                0x80000000L
	#define WS_CHILD                0x40000000L
	#define WS_MINIMIZE             0x20000000L
	#define WS_VISIBLE              0x10000000L
	#define WS_DISABLED             0x08000000L
	#define WS_CLIPSIBLINGS         0x04000000L
	#define WS_CLIPCHILDREN         0x02000000L
	#define WS_MAXIMIZE             0x01000000L
	#define WS_CAPTION              0x00C00000L
	#define WS_BORDER               0x00800000L
	#define WS_DLGFRAME             0x00400000L
	#define WS_VSCROLL              0x00200000L
	#define WS_HSCROLL              0x00100000L
	#define WS_SYSMENU              0x00080000L
	#define WS_THICKFRAME           0x00040000L
	#define WS_GROUP                0x00020000L
	#define WS_TABSTOP              0x00010000L
	#define WS_MINIMIZEBOX          0x00020000L
	#define WS_MAXIMIZEBOX          0x00010000L
	#define WS_TILED                WS_OVERLAPPED
	#define WS_ICONIC               WS_MINIMIZE
	#define WS_SIZEBOX              WS_THICKFRAME
	#define WS_OVERLAPPEDWINDOW     (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME| WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
	#define WS_POPUPWINDOW          (WS_POPUP | WS_BORDER | WS_SYSMENU)
	#define WS_CHILDWINDOW          (WS_CHILD)
	#define WS_TILEDWINDOW          (WS_OVERLAPPEDWINDOW)

	/*
 	* Extended Window Styles
 	*/
	#define WS_EX_DLGMODALFRAME     0x00000001L
	#define WS_EX_NOPARENTNOTIFY    0x00000004L
	#define WS_EX_TOPMOST           0x00000008L
	#define WS_EX_ACCEPTFILES       0x00000010L
	#define WS_EX_TRANSPARENT       0x00000020L
	#define WS_EX_MDICHILD          0x00000040L
	#define WS_EX_TOOLWINDOW        0x00000080L
	#define WS_EX_WINDOWEDGE        0x00000100L
	#define WS_EX_CLIENTEDGE        0x00000200L
	#define WS_EX_CONTEXTHELP       0x00000400L
	#define WS_EX_RIGHT             0x00001000L
	#define WS_EX_LEFT              0x00000000L
	#define WS_EX_RTLREADING        0x00002000L
	#define WS_EX_LTRREADING        0x00000000L
	#define WS_EX_LEFTSCROLLBAR     0x00004000L
	#define WS_EX_RIGHTSCROLLBAR    0x00000000L
	#define WS_EX_CONTROLPARENT     0x00010000L
	#define WS_EX_STATICEDGE        0x00020000L
	#define WS_EX_APPWINDOW         0x00040000L
	#define WS_EX_OVERLAPPEDWINDOW  (WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE)
	#define WS_EX_PALETTEWINDOW     (WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST)
	#define WS_EX_LAYERED           0x00080000
	#define WS_EX_NOINHERITLAYOUT   0x00100000L // Disable inheritence of mirroring by children
	#define WS_EX_LAYOUTRTL         0x00400000L // Right to left mirroring
	#define WS_EX_NOACTIVATE        0x08000000L

	#define WM_NULL                 0x0000
	#define WM_CREATE               0x0001
	#define WM_DESTROY              0x0002
	#define WM_MOVE                 0x0003
	#define WM_SIZEWAIT             0x0004
	#define WM_SIZE                 0x0005
	#define WM_ACTIVATE             0x0006
	#define WM_SETFOCUS             0x0007
	#define WM_KILLFOCUS            0x0008
	#define WM_SETVISIBLE           0x0009
	#define WM_ENABLE               0x000a
	#define WM_SETREDRAW            0x000b
	#define WM_SETTEXT              0x000c
	#define WM_GETTEXT              0x000d
	#define WM_GETTEXTLENGTH        0x000e
	#define WM_PAINT                0x000f
	#define WM_CLOSE                0x0010
	#define WM_QUERYENDSESSION      0x0011
	#define WM_QUIT                 0x0012
	#define WM_QUERYOPEN            0x0013
	#define WM_ERASEBKGND           0x0014
	#define WM_SYSCOLORCHANGE       0x0015
	#define WM_ENDSESSION           0x0016
	#define WM_SYSTEMERROR          0x0017
	#define WM_SHOWWINDOW           0x0018
	#define WM_CTLCOLOR             0x0019
	#define WM_WININICHANGE         0x001a
	#define WM_SETTINGCHANGE        WM_WININICHANGE
	#define WM_DEVMODECHANGE        0x001b
	#define WM_ACTIVATEAPP          0x001c
	#define WM_FONTCHANGE           0x001d
	#define WM_TIMECHANGE           0x001e
	#define WM_CANCELMODE           0x001f
	#define WM_SETCURSOR            0x0020
	#define WM_MOUSEACTIVATE        0x0021
	#define WM_CHILDACTIVATE        0x0022
	#define WM_QUEUESYNC            0x0023
	#define WM_GETMINMAXINFO        0x0024
	#define WM_DROPFILES            0x0233
	#define WM_NOTIFY               0x004E
	#define WM_SHOWWINDOW           0x0018
	#define WM_HSCROLL              0x0114
	#define WM_VSCROLL              0x0115
	#define WM_CTLCOLORMSGBOX       0x0132
	#define WM_CTLCOLOREDIT         0x0133
	#define WM_CTLCOLORLISTBOX      0x0134
	#define WM_CTLCOLORBTN          0x0135
	#define WM_CTLCOLORDLG          0x0136
	#define WM_CTLCOLORSCROLLBAR    0x0137
	#define WM_CTLCOLORSTATIC       0x0138
	#define MK_LBUTTON	    		0x0001
	#define MK_RBUTTON	    		0x0002
	#define MK_SHIFT	    		0x0004
	#define MK_CONTROL	    		0x0008
	#define MK_MBUTTON	    		0x0010
	#define MK_XBUTTON1         	0x0020
	#define MK_XBUTTON2         	0x0040
	#define WM_MOUSEMOVE	    	0x0200
	#define WM_LBUTTONDOWN	    	0x0201
	#define WM_LBUTTONUP	    	0x0202
	#define WM_LBUTTONDBLCLK    	0x0203
	#define WM_RBUTTONDOWN	    	0x0204
	#define WM_RBUTTONUP	    	0x0205
	#define WM_RBUTTONDBLCLK    	0x0206
	#define WM_MBUTTONDOWN	    	0x0207
	#define WM_MBUTTONUP	    	0x0208
	#define WM_MBUTTONDBLCLK    	0x0209
	#define WM_MOUSEWHEEL       	0x020A
	#define WM_XBUTTONDOWN      	0x020B
	#define WM_XBUTTONUP        	0x020C
	#define WM_XBUTTONDBLCLK    	0x020D
	#define WM_SIZING	    		0x0214
	#define WM_CAPTURECHANGED   	0x0215
	#define WM_MOVING	    		0x0216
	#define WM_POWERBROADCAST   	0x0218
	#define WM_DEVICECHANGE     	0x0219
	#define WM_INITDIALOG       	0x0110
	#define WM_COMMAND          	0x0111
	#define WM_SYSCOMMAND       	0x0112
	#define WM_TIMER	    		0x0113
	#define WM_SYSTIMER	    		0x0118
	#define WM_INITMENU         	0x0116
	#define WM_INITMENUPOPUP    	0x0117
  	/* Keyboard messages */
	#define WM_KEYDOWN          	0x0100
	#define WM_KEYUP            	0x0101
	#define WM_CHAR             	0x0102
	#define WM_DEADCHAR         	0x0103
	#define WM_SYSKEYDOWN       	0x0104
	#define WM_SYSKEYUP         	0x0105
	#define WM_SYSCHAR          	0x0106
	#define WM_SYSDEADCHAR      	0x0107
	#define WM_KEYFIRST         	WM_KEYDOWN
	#define WM_KEYLAST          	0x0108
	#define WM_TIMER	    		0x0113
	#define WM_PAINTICON                    0x0026
	#define WM_ICONERASEBKGND               0x0027
	#define WM_NEXTDLGCTL                   0x0028
	#define WM_SPOOLERSTATUS                0x002A
	#define WM_DRAWITEM                     0x002B
	#define WM_MEASUREITEM                  0x002C
	#define WM_DELETEITEM                   0x002D
	#define WM_VKEYTOITEM                   0x002E
	#define WM_CHARTOITEM                   0x002F
	#define WM_SETFONT                      0x0030
	#define WM_GETFONT                      0x0031
	#define WM_SETHOTKEY                    0x0032
	#define WM_GETHOTKEY                    0x0033
	#define WM_QUERYDRAGICON                0x0037
	#define WM_COMPAREITEM                  0x0039
	#define WM_ENTERMENULOOP                0x0211
	#define WM_EXITMENULOOP                 0x0212
	#define WM_USER                         0x0400
	#define WM_GETDLGCODE                   0x0087
	#define WM_CONTEXTMENU                  0x007B
	#define WM_NCMOUSEMOVE                  0x00A0
	#define WM_NCLBUTTONDOWN                0x00A1
	#define WM_NCLBUTTONUP                  0x00A2
	#define WM_NCLBUTTONDBLCLK              0x00A3
	#define WM_NCRBUTTONDOWN                0x00A4
	#define WM_NCRBUTTONUP                  0x00A5
	#define WM_NCRBUTTONDBLCLK              0x00A6
	#define WM_NCMBUTTONDOWN                0x00A7
	#define WM_NCMBUTTONUP                  0x00A8
	#define WM_NCMBUTTONDBLCLK              0x00A9
	#define WM_NCCALCSIZE                   0x0083

	#define PSCB_INITIALIZED  1
	#define PSCB_PRECREATE    2

	#define PSP_DEFAULT                0x00000000
	#define PSP_DLGINDIRECT            0x00000001
	#define PSP_USEHICON               0x00000002
	#define PSP_USEICONID              0x00000004
	#define PSP_USETITLE               0x00000008
	#define PSP_RTLREADING             0x00000010
	#define PSP_HASHELP                0x00000020
	#define PSP_USEREFPARENT           0x00000040
	#define PSP_USECALLBACK            0x00000080
	#define PSP_PREMATURE              0x00000400
	#define PSP_HIDEHEADER             0x00000800
	#define PSP_USEHEADERTITLE         0x00001000
	#define PSP_USEHEADERSUBTITLE      0x00002000

	#define PSH_DEFAULT             0x00000000
	#define PSH_PROPTITLE           0x00000001
	#define PSH_USEHICON            0x00000002
	#define PSH_USEICONID           0x00000004
	#define PSH_PROPSHEETPAGE       0x00000008
	#define PSH_WIZARDHASFINISH     0x00000010
	#define PSH_WIZARD              0x00000020
	#define PSH_USEPSTARTPAGE       0x00000040
	#define PSH_NOAPPLYNOW          0x00000080
	#define PSH_USECALLBACK         0x00000100
	#define PSH_HASHELP             0x00000200
	#define PSH_MODELESS            0x00000400
	#define PSH_RTLREADING          0x00000800
	#define PSH_WIZARDCONTEXTHELP   0x00001000
	#define PSH_WATERMARK           0x00008000
	#define PSH_USEHBMWATERMARK     0x00010000
	#define PSH_USEHPLWATERMARK     0x00020000
	#define PSH_STRETCHWATERMARK    0x00040000
	#define PSH_HEADER              0x00080000
	#define PSH_USEHBMHEADER        0x00100000
	#define PSH_USEPAGELANG         0x00200000
	#define PSH_WIZARD_LITE         0x00400000
	#define PSH_NOCONTEXTHELP       0x02000000

	#define PSN_FIRST               (0U-200U)
	#define PSN_LAST                (0U-299U)
	#define PSN_SETACTIVE           (PSN_FIRST-0)
	#define PSN_KILLACTIVE          (PSN_FIRST-1)
	#define PSN_APPLY               (PSN_FIRST-2)
	#define PSN_RESET               (PSN_FIRST-3)
	#define PSN_HELP                (PSN_FIRST-5)
	#define PSN_WIZBACK             (PSN_FIRST-6)
	#define PSN_WIZNEXT             (PSN_FIRST-7)
	#define PSN_WIZFINISH           (PSN_FIRST-8)
	#define PSN_QUERYCANCEL         (PSN_FIRST-9)
	#define PSN_GETOBJECT           (PSN_FIRST-10)
	#define PSN_TRANSLATEACCELERATOR (PSN_FIRST-12)
	#define PSN_QUERYINITIALFOCUS   (PSN_FIRST-13)

	#define PSM_ADDPAGE             (WM_USER + 103)

	#define BN_CLICKED             	0
	#define BN_PAINT               	1
	#define BN_HILITE              	2
	#define BN_PUSHED              	BN_HILITE
	#define BN_UNHILITE            	3
	#define BN_UNPUSHED            	BN_UNHILITE
	#define BN_DISABLE             	4
	#define BN_DOUBLECLICKED       	5
	#define BN_DBLCLK              	BN_DOUBLECLICKED
	#define BN_SETFOCUS            	6
	#define BN_KILLFOCUS           	7

	#define BS_PUSHBUTTON       0x00000000L
	#define BS_DEFPUSHBUTTON    0x00000001L
	#define BS_CHECKBOX         0x00000002L
	#define BS_AUTOCHECKBOX     0x00000003L
	#define BS_RADIOBUTTON      0x00000004L
	#define BS_3STATE           0x00000005L
	#define BS_AUTO3STATE       0x00000006L
	#define BS_GROUPBOX         0x00000007L
	#define BS_USERBUTTON       0x00000008L
	#define BS_AUTORADIOBUTTON  0x00000009L
	#define BS_OWNERDRAW        0x0000000BL
	#define BS_LEFTTEXT         0x00000020L
	#define BS_TEXT             0x00000000L
	#define BS_ICON             0x00000040L
	#define BS_BITMAP           0x00000080L
	#define BS_LEFT             0x00000100L
	#define BS_RIGHT            0x00000200L
	#define BS_CENTER           0x00000300L
	#define BS_TOP              0x00000400L
	#define BS_BOTTOM           0x00000800L
	#define BS_VCENTER          0x00000C00L
	#define BS_PUSHLIKE         0x00001000L
	#define BS_MULTILINE        0x00002000L
	#define BS_NOTIFY           0x00004000L
	#define BS_FLAT             0x00008000L
	#define BS_RIGHTBUTTON      BS_LEFTTEXT

	#define BM_GETCHECK        0x00F0
	#define BM_SETCHECK        0x00F1
	#define BM_GETSTATE        0x00F2
	#define BM_SETSTATE        0x00F3
	#define BM_SETSTYLE        0x00F4
	#define BM_CLICK           0x00F5
	#define BM_GETIMAGE        0x00F6
	#define BM_SETIMAGE        0x00F7
	#define BST_UNCHECKED      0x0000
	#define BST_CHECKED        0x0001
	#define BST_INDETERMINATE  0x0002
	#define BST_PUSHED         0x0004
	#define BST_FOCUS          0x0008
	/*
 	* Edit Control Styles
 	*/
	#define ES_LEFT             0x0000L
	#define ES_CENTER           0x0001L
	#define ES_RIGHT            0x0002L
	#define ES_MULTILINE        0x0004L
	#define ES_UPPERCASE        0x0008L
	#define ES_LOWERCASE        0x0010L
	#define ES_PASSWORD         0x0020L
	#define ES_AUTOVSCROLL      0x0040L
	#define ES_AUTOHSCROLL      0x0080L
	#define ES_NOHIDESEL        0x0100L
	#define ES_OEMCONVERT       0x0400L
	#define ES_READONLY         0x0800L
	#define ES_WANTRETURN       0x1000L
	#define ES_NUMBER           0x2000L
	/*
 	* Edit Control Notification Codes
 	*/
	#define EN_SETFOCUS         0x0100
	#define EN_KILLFOCUS        0x0200
	#define EN_CHANGE           0x0300
	#define EN_UPDATE           0x0400
	#define EN_ERRSPACE         0x0500
	#define EN_MAXTEXT          0x0501
	#define EN_HSCROLL          0x0601
	#define EN_VSCROLL          0x0602
	#define EN_ALIGN_LTR_EC     0x0700
	#define EN_ALIGN_RTL_EC     0x0701
	/* Edit control EM_SETMARGIN parameters */
	#define EC_LEFTMARGIN       0x0001
	#define EC_RIGHTMARGIN      0x0002
	#define EC_USEFONTINFO      0xffff
	#define EMSIS_COMPOSITIONSTRING        0x0001
	/* lParam for EMSIS_COMPOSITIONSTRING  */
	#define EIMES_GETCOMPSTRATONCE         0x0001
	#define EIMES_CANCELCOMPSTRINFOCUS     0x0002
	#define EIMES_COMPLETECOMPSTRKILLFOCUS 0x0004
	/*
 	* Edit Control Messages
 	*/
	#define EM_GETSEL               0x00B0
	#define EM_SETSEL               0x00B1
	#define EM_GETRECT              0x00B2
	#define EM_SETRECT              0x00B3
	#define EM_SETRECTNP            0x00B4
	#define EM_SCROLL               0x00B5
	#define EM_LINESCROLL           0x00B6
	#define EM_SCROLLCARET          0x00B7
	#define EM_GETMODIFY            0x00B8
	#define EM_SETMODIFY            0x00B9
	#define EM_GETLINECOUNT         0x00BA
	#define EM_LINEINDEX            0x00BB
	#define EM_SETHANDLE            0x00BC
	#define EM_GETHANDLE            0x00BD
	#define EM_GETTHUMB             0x00BE
	#define EM_LINELENGTH           0x00C1
	#define EM_REPLACESEL           0x00C2
	#define EM_GETLINE              0x00C4
	#define EM_LIMITTEXT            0x00C5
	#define EM_CANUNDO              0x00C6
	#define EM_UNDO                 0x00C7
	#define EM_FMTLINES             0x00C8
	#define EM_LINEFROMCHAR         0x00C9
	#define EM_SETTABSTOPS          0x00CB
	#define EM_SETPASSWORDCHAR      0x00CC
	#define EM_EMPTYUNDOBUFFER      0x00CD
	#define EM_GETFIRSTVISIBLELINE  0x00CE
	#define EM_SETREADONLY          0x00CF
	#define EM_SETWORDBREAKPROC     0x00D0
	#define EM_GETWORDBREAKPROC     0x00D1
	#define EM_GETPASSWORDCHAR      0x00D2
	#define EM_SETMARGINS           0x00D3
	#define EM_GETMARGINS           0x00D4
	#define EM_SETLIMITTEXT         EM_LIMITTEXT   /* ;win40 Name change */
	#define EM_GETLIMITTEXT         0x00D5
	#define EM_POSFROMCHAR          0x00D6
	#define EM_CHARFROMPOS          0x00D7
	#define EM_SETIMESTATUS         0x00D8
	#define EM_GETIMESTATUS         0x00D9

	/*
 	* Listbox Return Values
 	*/
	#define LB_OKAY             0
	#define LB_ERR              (-1)
	#define LB_ERRSPACE         (-2)
	/*
	* Listbox Notification Codes
 	*/
	#define LBN_ERRSPACE        (-2)
	#define LBN_SELCHANGE       1
	#define LBN_DBLCLK          2
	#define LBN_SELCANCEL       3
	#define LBN_SETFOCUS        4
	#define LBN_KILLFOCUS       5
	/*
 	* Listbox messages
	*/
	#define LB_ADDSTRING            0x0180
	#define LB_INSERTSTRING         0x0181
	#define LB_DELETESTRING         0x0182
	#define LB_SELITEMRANGEEX       0x0183
	#define LB_RESETCONTENT         0x0184
	#define LB_SETSEL               0x0185
	#define LB_SETCURSEL            0x0186
	#define LB_GETSEL               0x0187
	#define LB_GETCURSEL            0x0188
	#define LB_GETTEXT              0x0189
	#define LB_GETTEXTLEN           0x018A
	#define LB_GETCOUNT             0x018B
	#define LB_SELECTSTRING         0x018C
	#define LB_DIR                  0x018D
	#define LB_GETTOPINDEX          0x018E
	#define LB_FINDSTRING           0x018F
	#define LB_GETSELCOUNT          0x0190
	#define LB_GETSELITEMS          0x0191
	#define LB_SETTABSTOPS          0x0192
	#define LB_GETHORIZONTALEXTENT  0x0193
	#define LB_SETHORIZONTALEXTENT  0x0194
	#define LB_SETCOLUMNWIDTH       0x0195
	#define LB_ADDFILE              0x0196
	#define LB_SETTOPINDEX          0x0197
	#define LB_GETITEMRECT          0x0198
	#define LB_GETITEMDATA          0x0199
	#define LB_SETITEMDATA          0x019A
	#define LB_SELITEMRANGE         0x019B
	#define LB_SETANCHORINDEX       0x019C
	#define LB_GETANCHORINDEX       0x019D
	#define LB_SETCARETINDEX        0x019E
	#define LB_GETCARETINDEX        0x019F
	#define LB_SETITEMHEIGHT        0x01A0
	#define LB_GETITEMHEIGHT        0x01A1
	#define LB_FINDSTRINGEXACT      0x01A2
	#define LB_SETLOCALE            0x01A5
	#define LB_GETLOCALE            0x01A6
	#define LB_SETCOUNT             0x01A7
	#define LB_INITSTORAGE          0x01A8
	#define LB_ITEMFROMPOINT        0x01A9
	#define LB_MULTIPLEADDSTRING    0x01B1
	/*
 	* Listbox Styles
 	*/
	#define LBS_NOTIFY            0x0001L
	#define LBS_SORT              0x0002L
	#define LBS_NOREDRAW          0x0004L
	#define LBS_MULTIPLESEL       0x0008L
	#define LBS_OWNERDRAWFIXED    0x0010L
	#define LBS_OWNERDRAWVARIABLE 0x0020L
	#define LBS_HASSTRINGS        0x0040L
	#define LBS_USETABSTOPS       0x0080L
	#define LBS_NOINTEGRALHEIGHT  0x0100L
	#define LBS_MULTICOLUMN       0x0200L
	#define LBS_WANTKEYBOARDINPUT 0x0400L
	#define LBS_EXTENDEDSEL       0x0800L
	#define LBS_DISABLENOSCROLL   0x1000L
	#define LBS_NODATA            0x2000L

	/*
 	* Combo Box return Values
 	*/
	#define CB_OKAY             0
	#define CB_ERR              (-1)
	#define CB_ERRSPACE         (-2)
	/*
 	* Combo Box Notification Codes
 	*/
	#define CBN_ERRSPACE        (-1)
	#define CBN_SELCHANGE       1
	#define CBN_DBLCLK          2
	#define CBN_SETFOCUS        3
	#define CBN_KILLFOCUS       4
	#define CBN_EDITCHANGE      5
	#define CBN_EDITUPDATE      6
	#define CBN_DROPDOWN        7
	#define CBN_CLOSEUP         8
	#define CBN_SELENDOK        9
	#define CBN_SELENDCANCEL    10
	/*
 	* Combo Box styles
 	*/
	#define CBS_SIMPLE            0x0001L
	#define CBS_DROPDOWN          0x0002L
	#define CBS_DROPDOWNLIST      0x0003L
	#define CBS_OWNERDRAWFIXED    0x0010L
	#define CBS_OWNERDRAWVARIABLE 0x0020L
	#define CBS_AUTOHSCROLL       0x0040L
	#define CBS_OEMCONVERT        0x0080L
	#define CBS_SORT              0x0100L
	#define CBS_HASSTRINGS        0x0200L
	#define CBS_NOINTEGRALHEIGHT  0x0400L
	#define CBS_DISABLENOSCROLL   0x0800L
	#define CBS_UPPERCASE           0x2000L
	#define CBS_LOWERCASE           0x4000L
	/*
 	* Combo Box messages
 	*/
	#define CB_GETEDITSEL               0x0140
	#define CB_LIMITTEXT                0x0141
	#define CB_SETEDITSEL               0x0142
	#define CB_ADDSTRING                0x0143
	#define CB_DELETESTRING             0x0144
	#define CB_DIR                      0x0145
	#define CB_GETCOUNT                 0x0146
	#define CB_GETCURSEL                0x0147
	#define CB_GETLBTEXT                0x0148
	#define CB_GETLBTEXTLEN             0x0149
	#define CB_INSERTSTRING             0x014A
	#define CB_RESETCONTENT             0x014B
	#define CB_FINDSTRING               0x014C
	#define CB_SELECTSTRING             0x014D
	#define CB_SETCURSEL                0x014E
	#define CB_SHOWDROPDOWN             0x014F
	#define CB_GETITEMDATA              0x0150
	#define CB_SETITEMDATA              0x0151
	#define CB_GETDROPPEDCONTROLRECT    0x0152
	#define CB_SETITEMHEIGHT            0x0153
	#define CB_GETITEMHEIGHT            0x0154
	#define CB_SETEXTENDEDUI            0x0155
	#define CB_GETEXTENDEDUI            0x0156
	#define CB_GETDROPPEDSTATE          0x0157
	#define CB_FINDSTRINGEXACT          0x0158
	#define CB_SETLOCALE                0x0159
	#define CB_GETLOCALE                0x015A
	#define CB_GETTOPINDEX              0x015b
	#define CB_SETTOPINDEX              0x015c
	#define CB_GETHORIZONTALEXTENT      0x015d
	#define CB_SETHORIZONTALEXTENT      0x015e
	#define CB_GETDROPPEDWIDTH          0x015f
	#define CB_SETDROPPEDWIDTH          0x0160
	#define CB_INITSTORAGE              0x0161
	#define CB_MULTIPLEADDSTRING        0x0163
	#define CB_MSGMAX                   0x0163
	/*
 	* Static Control Constants
 	*/
	#define SS_LEFT             0x00000000L
	#define SS_CENTER           0x00000001L
	#define SS_RIGHT            0x00000002L
	#define SS_ICON             0x00000003L
	#define SS_BLACKRECT        0x00000004L
	#define SS_GRAYRECT         0x00000005L
	#define SS_WHITERECT        0x00000006L
	#define SS_BLACKFRAME       0x00000007L
	#define SS_GRAYFRAME        0x00000008L
	#define SS_WHITEFRAME       0x00000009L
	#define SS_USERITEM         0x0000000AL
	#define SS_SIMPLE           0x0000000BL
	#define SS_LEFTNOWORDWRAP   0x0000000CL
	#define SS_OWNERDRAW        0x0000000DL
	#define SS_BITMAP           0x0000000EL
	#define SS_ENHMETAFILE      0x0000000FL
	#define SS_ETCHEDHORZ       0x00000010L
	#define SS_ETCHEDVERT       0x00000011L
	#define SS_ETCHEDFRAME      0x00000012L
	#define SS_TYPEMASK         0x0000001FL
	#define SS_NOPREFIX         0x00000080L /* Don't do "&" character translation */
	#define SS_NOTIFY           0x00000100L
	#define SS_CENTERIMAGE      0x00000200L
	#define SS_RIGHTJUST        0x00000400L
	#define SS_REALSIZEIMAGE    0x00000800L
	#define SS_SUNKEN           0x00001000L
	#define SS_ENDELLIPSIS      0x00004000L
	#define SS_PATHELLIPSIS     0x00008000L
	#define SS_WORDELLIPSIS     0x0000C000L
	#define SS_ELLIPSISMASK     0x0000C000L

	#define STM_SETICON         0x0170
	#define STM_GETICON         0x0171
	#define STM_SETIMAGE        0x0172
	#define STM_GETIMAGE        0x0173
	#define STN_CLICKED         0
	#define STN_DBLCLK          1
	#define STN_ENABLE          2
	#define STN_DISABLE         3
	/*
 	* Edit Control Styles
 	*/
	#define ES_LEFT             0x0000L
	#define ES_CENTER           0x0001L
	#define ES_RIGHT            0x0002L
	#define ES_MULTILINE        0x0004L
	#define ES_UPPERCASE        0x0008L
	#define ES_LOWERCASE        0x0010L
	#define ES_PASSWORD         0x0020L
	#define ES_AUTOVSCROLL      0x0040L
	#define ES_AUTOHSCROLL      0x0080L
	#define ES_NOHIDESEL        0x0100L
	#define ES_OEMCONVERT       0x0400L
	#define ES_READONLY         0x0800L
	#define ES_WANTRETURN       0x1000L
	#define ES_NUMBER           0x2000L
	/*
	* ListBox Control Style
	*/
	#define LB_ADDSTRING            0x0180
	#define LB_INSERTSTRING         0x0181
	#define LB_DELETESTRING         0x0182
	#define LB_SELITEMRANGEEX       0x0183
	#define LB_RESETCONTENT         0x0184
	#define LB_SETSEL               0x0185
	#define LB_SETCURSEL            0x0186
	#define LB_GETSEL               0x0187
	#define LB_GETCURSEL            0x0188
	#define LB_GETTEXT              0x0189
	#define LB_GETTEXTLEN           0x018A
	#define LB_GETCOUNT             0x018B
	#define LB_SELECTSTRING         0x018C
	#define LB_DIR                  0x018D
	#define LB_GETTOPINDEX          0x018E
	#define LB_FINDSTRING           0x018F
	#define LB_GETSELCOUNT          0x0190
	#define LB_GETSELITEMS          0x0191
	#define LB_SETTABSTOPS          0x0192
	#define LB_GETHORIZONTALEXTENT  0x0193
	#define LB_SETHORIZONTALEXTENT  0x0194
	#define LB_SETCOLUMNWIDTH       0x0195
	#define LB_ADDFILE              0x0196
	#define LB_SETTOPINDEX          0x0197
	#define LB_GETITEMRECT          0x0198
	#define LB_GETITEMDATA          0x0199
	#define LB_SETITEMDATA          0x019A
	#define LB_SELITEMRANGE         0x019B
	#define LB_SETANCHORINDEX       0x019C
	#define LB_GETANCHORINDEX       0x019D
	#define LB_SETCARETINDEX        0x019E
	#define LB_GETCARETINDEX        0x019F
	#define LB_SETITEMHEIGHT        0x01A0
	#define LB_GETITEMHEIGHT        0x01A1
	#define LB_FINDSTRINGEXACT      0x01A2
	#define LB_SETLOCALE            0x01A5
	#define LB_GETLOCALE            0x01A6
	#define LB_SETCOUNT             0x01A7

	#define LBN_ERRSPACE        (-2)
	#define LBN_SELCHANGE       1
	#define LBN_DBLCLK          2
	#define LBN_SELCANCEL       3
	#define LBN_SETFOCUS        4
	#define LBN_KILLFOCUS       5

	#define LB_OKAY             0
	#define LB_ERR              (-1)
	#define LB_ERRSPACE         (-2)

	/*
 	* Scroll Bar Constants
 	*/
	#define SB_HORZ             0
	#define SB_VERT             1
	#define SB_CTL              2
	#define SB_BOTH             3
	/*
 	* Scroll Bar Commands
 	*/
	#define SB_LINEUP           0
	#define SB_LINELEFT         0
	#define SB_LINEDOWN         1
	#define SB_LINERIGHT        1
	#define SB_PAGEUP           2
	#define SB_PAGELEFT         2
	#define SB_PAGEDOWN         3
	#define SB_PAGERIGHT        3
	#define SB_THUMBPOSITION    4
	#define SB_THUMBTRACK       5
	#define SB_TOP              6
	#define SB_LEFT             6
	#define SB_BOTTOM           7
	#define SB_RIGHT            7
	#define SB_ENDSCROLL        8

	#define SBS_HORZ                    0x0000L
	#define SBS_VERT                    0x0001L
	#define SBS_TOPALIGN                0x0002L
	#define SBS_LEFTALIGN               0x0002L
	#define SBS_BOTTOMALIGN             0x0004L
	#define SBS_RIGHTALIGN              0x0004L
	#define SBS_SIZEBOXTOPLEFTALIGN     0x0002L
	#define SBS_SIZEBOXBOTTOMRIGHTALIGN 0x0004L
	#define SBS_SIZEBOX                 0x0008L
	#define SBS_SIZEGRIP                0x0010L
	/*
 	* Scroll bar messages
 	*/
	#define SBM_SETPOS                  0x00E0 /*not in win3.1 */
	#define SBM_GETPOS                  0x00E1 /*not in win3.1 */
	#define SBM_SETRANGE                0x00E2 /*not in win3.1 */
	#define SBM_SETRANGEREDRAW          0x00E6 /*not in win3.1 */
	#define SBM_GETRANGE                0x00E3 /*not in win3.1 */
	#define SBM_ENABLE_ARROWS           0x00E4 /*not in win3.1 */
	#define SBM_SETSCROLLINFO           0x00E9
	#define SBM_GETSCROLLINFO           0x00EA

	#define SIF_RANGE           0x0001
	#define SIF_PAGE            0x0002
	#define SIF_POS             0x0004
	#define SIF_DISABLENOSCROLL 0x0008
	#define SIF_TRACKPOS        0x0010
	#define SIF_ALL             (SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS)

	/*
 	* Dialog Codes
 	*/
	#define DLGC_WANTARROWS     0x0001      /* Control wants arrow keys         */
	#define DLGC_WANTTAB        0x0002      /* Control wants tab keys           */
	#define DLGC_WANTALLKEYS    0x0004      /* Control wants all keys           */
	#define DLGC_WANTMESSAGE    0x0004      /* Pass message to control          */
	#define DLGC_HASSETSEL      0x0008      /* Understands EM_SETSEL message    */
	#define DLGC_DEFPUSHBUTTON  0x0010      /* Default pushbutton               */
	#define DLGC_UNDEFPUSHBUTTON 0x0020     /* Non-default pushbutton           */
	#define DLGC_RADIOBUTTON    0x0040      /* Radio button                     */
	#define DLGC_WANTCHARS      0x0080      /* Want WM_CHAR messages            */
	#define DLGC_STATIC         0x0100      /* Static item: don't include       */
	#define DLGC_BUTTON         0x2000      /* Button item: can be checked      */

	/*
 	* WM_NCHITTEST and MOUSEHOOKSTRUCT Mouse Position Codes
 	*/
	#define HTERROR             (-2)
	#define HTTRANSPARENT       (-1)
	#define HTNOWHERE           0
	#define HTCLIENT            1
	#define HTCAPTION           2
	#define HTSYSMENU           3
	#define HTGROWBOX           4
	#define HTSIZE              HTGROWBOX
	#define HTMENU              5
	#define HTHSCROLL           6
	#define HTVSCROLL           7
	#define HTMINBUTTON         8
	#define HTMAXBUTTON         9
	#define HTLEFT              10
	#define HTRIGHT             11
	#define HTTOP               12
	#define HTTOPLEFT           13
	#define HTTOPRIGHT          14
	#define HTBOTTOM            15
	#define HTBOTTOMLEFT        16
	#define HTBOTTOMRIGHT       17
	#define HTBORDER            18
	#define HTREDUCE            HTMINBUTTON
	#define HTZOOM              HTMAXBUTTON
	#define HTSIZEFIRST         HTLEFT
	#define HTSIZELAST          HTBOTTOMRIGHT

	#define IDOK                    1
	#define IDCANCEL                2
	#define IDABORT                 3
	#define IDRETRY                 4
	#define IDIGNORE                5
	#define IDYES                   6
	#define IDNO                    7
	#define IDCLOSE                 8
	#define IDHELP                  9

	/*
 	* Owner draw control types
 	*/
	#define ODT_MENU        1
	#define ODT_LISTBOX     2
	#define ODT_COMBOBOX    3
	#define ODT_BUTTON      4
	#define ODT_STATIC      5
	/*
 	* Owner draw actions
 	*/
	#define ODA_DRAWENTIRE  0x0001
	#define ODA_SELECT      0x0002
	#define ODA_FOCUS       0x0004
	/*
 	* Owner draw state
 	*/
	#define ODS_SELECTED    0x0001
	#define ODS_GRAYED      0x0002
	#define ODS_DISABLED    0x0004
	#define ODS_CHECKED     0x0008
	#define ODS_FOCUS       0x0010
	#define ODS_DEFAULT         0x0020
	#define ODS_COMBOBOXEDIT    0x1000
	#define ODS_HOTLIGHT        0x0040
	#define ODS_INACTIVE        0x0080
	#define ODS_NOACCEL         0x0100
	#define ODS_NOFOCUSRECT     0x0200

	#define MB_OK				    0x00000000
	#define MB_OKCANCEL			    0x00000001
	#define MB_ABORTRETRYIGNORE	    0x00000002
	#define MB_YESNOCANCEL		    0x00000003
	#define MB_YESNO			    0x00000004
	#define MB_RETRYCANCEL		    0x00000005
	#define MB_TYPEMASK			    0x0000000F

	#define MB_APPLMODAL            0x00000000L
	#define MB_SYSTEMMODAL          0x00001000L
	#define MB_TASKMODAL            0x00002000L

	#define MF_ENABLED         	    0x0000
	#define MF_GRAYED          	    0x0001
	#define MF_DISABLED        	    0x0002
	#define MF_STRING          	    0x0000
	#define MF_BITMAP          	    0x0004
	#define MF_UNCHECKED       	    0x0000
	#define MF_CHECKED         	    0x0008
	#define MF_POPUP           	    0x0010
	#define MF_MENUBARBREAK    	    0x0020
	#define MF_MENUBREAK       	    0x0040
	#define MF_UNHILITE        	    0x0000
	#define MF_HILITE          	    0x0080
	#define MF_OWNERDRAW       	    0x0100
	#define MF_USECHECKBITMAPS 	    0x0200
	#define MF_BYCOMMAND       	    0x0000
	#define MF_BYPOSITION      	    0x0400
	#define MF_SEPARATOR       	    0x0800
	#define MF_DEFAULT         	    0x1000
	#define MF_SYSMENU         	    0x2000
	#define MF_HELP            	    0x4000
	#define MF_RIGHTJUSTIFY    	    0x4000
	#define MF_MOUSESELECT     	    0x8000

	#define MFT_STRING          	MF_STRING
	#define MFT_BITMAP          	MF_BITMAP
	#define MFT_MENUBARBREAK    	MF_MENUBARBREAK
	#define MFT_MENUBREAK       	MF_MENUBREAK
	#define MFT_OWNERDRAW       	MF_OWNERDRAW
	#define MFT_RADIOCHECK      	0x00000200L
	#define MFT_SEPARATOR       	MF_SEPARATOR
	#define MFT_RIGHTORDER      	0x00002000L
	#define MFT_RIGHTJUSTIFY    	MF_RIGHTJUSTIFY

	/* Menu flags for Add/Check/EnableMenuItem() */
	#define MFS_GRAYED          	0x00000003L
	#define MFS_DISABLED        	MFS_GRAYED
	#define MFS_CHECKED         	MF_CHECKED
	#define MFS_HILITE          	MF_HILITE
	#define MFS_ENABLED         	MF_ENABLED
	#define MFS_UNCHECKED       	MF_UNCHECKED
	#define MFS_UNHILITE        	MF_UNHILITE
	#define MFS_DEFAULT         	MF_DEFAULT

	#define MIIM_STATE       		0x00000001
	#define MIIM_ID          		0x00000002
	#define MIIM_SUBMENU     		0x00000004
	#define MIIM_CHECKMARKS  		0x00000008
	#define MIIM_TYPE        		0x00000010
	#define MIIM_DATA        		0x00000020

	#define BIF_RETURNONLYFSDIRS   0x0001  // For finding a folder to start document searching

	#define OFN_READONLY                 0x00000001
	#define OFN_OVERWRITEPROMPT          0x00000002
	#define OFN_HIDEREADONLY             0x00000004
	#define OFN_NOCHANGEDIR              0x00000008
	#define OFN_SHOWHELP                 0x00000010
	#define OFN_ENABLEHOOK               0x00000020
	#define OFN_ENABLETEMPLATE           0x00000040
	#define OFN_ENABLETEMPLATEHANDLE     0x00000080
	#define OFN_NOVALIDATE               0x00000100
	#define OFN_ALLOWMULTISELECT         0x00000200
	#define OFN_EXTENSIONDIFFERENT       0x00000400
	#define OFN_PATHMUSTEXIST            0x00000800
	#define OFN_FILEMUSTEXIST            0x00001000
	#define OFN_CREATEPROMPT             0x00002000
	#define OFN_SHAREAWARE               0x00004000
	#define OFN_NOREADONLYRETURN         0x00008000
	#define OFN_NOTESTFILECREATE         0x00010000
	#define OFN_NONETWORKBUTTON          0x00020000
	#define OFN_NOLONGNAMES              0x00040000     // force
	#define OFN_EXPLORER                 0x00080000     // new look commdlg
	#define OFN_NODEREFERENCELINKS       0x00100000
	#define OFN_LONGNAMES                0x00200000     // force long names for 3.x modules
	#define OFN_ENABLEINCLUDENOTIFY      0x00400000     // send include message to callback
	#define OFN_ENABLESIZING             0x00800000

	#define MB_ICONHAND                 0x00000010L
	#define MB_ICONQUESTION             0x00000020L
	#define MB_ICONEXCLAMATION          0x00000030L
	#define MB_ICONASTERISK             0x00000040L
	#define MB_USERICON                 0x00000080L
	#define MB_ICONWARNING              MB_ICONEXCLAMATION
	#define MB_ICONERROR                MB_ICONHAND
	#define MB_ICONINFORMATION          MB_ICONASTERISK
	#define MB_ICONSTOP                 MB_ICONHAND

	#define RT_CURSOR         	    	MAKEINTRESOURCE(1)
	#define RT_BITMAP         	    	MAKEINTRESOURCE(2)
	#define RT_ICON           	    	MAKEINTRESOURCE(3)
	#define RT_MENU           	    	MAKEINTRESOURCE(4)
	#define RT_DIALOG         	    	MAKEINTRESOURCE(5)
	#define RT_STRING         	    	MAKEINTRESOURCE(6)
	#define RT_FONTDIR        	    	MAKEINTRESOURCE(7)
	#define RT_FONT           	    	MAKEINTRESOURCE(8)
	#define RT_ACCELERATOR    	    	MAKEINTRESOURCE(9)
	#define RT_RCDATA         	    	MAKEINTRESOURCE(10)
	#define RT_MESSAGETABLE   	    	MAKEINTRESOURCE(11)
	#define RT_GROUP_CURSOR   	    	MAKEINTRESOURCE(12)
	#define RT_GROUP_ICON     	    	MAKEINTRESOURCE(14)
	#define RT_VERSION        	    	MAKEINTRESOURCE(16)
	#define RT_DLGINCLUDE     	    	MAKEINTRESOURCE(17)
	#define RT_PLUGPLAY       	    	MAKEINTRESOURCE(19)
	#define RT_VXD            	    	MAKEINTRESOURCE(20)
	#define RT_ANICURSOR      	    	MAKEINTRESOURCE(21)
	#define RT_ANIICON        	    	MAKEINTRESOURCE(22)
	#define RT_HTML           	    	MAKEINTRESOURCE(23)

	#define VK_LBUTTON              	0x01
	#define VK_RBUTTON              	0x02
	#define VK_CANCEL               	0x03
	#define VK_MBUTTON              	0x04
	#define VK_XBUTTON1             	0x05
	#define VK_XBUTTON2             	0x06
	/*                          0x07  Undefined */
	#define VK_BACK                 0x08
	#define VK_TAB                  0x09
	/*                          0x0A-0x0B  Undefined */
	#define VK_CLEAR                0x0C
	#define VK_RETURN               0x0D
	/*                          0x0E-0x0F  Undefined */
	#define VK_SHIFT                0x10
	#define VK_CONTROL              0x11
	#define VK_MENU                 0x12
	#define VK_PAUSE                0x13
	#define VK_CAPITAL              0x14
	/*                          0x15-0x19  Reserved for Kanji systems */
	/*                          0x1A       Undefined */
	#define VK_ESCAPE               0x1B
	/*                          0x1C-0x1F  Reserved for Kanji systems */
	#define VK_SPACE                0x20
	#define VK_PRIOR                0x21
	#define VK_NEXT                 0x22
	#define VK_END                  0x23
	#define VK_HOME                 0x24
	#define VK_LEFT                 0x25
	#define VK_UP                   0x26
	#define VK_RIGHT                0x27
	#define VK_DOWN                 0x28
	#define VK_SELECT               0x29
	#define VK_PRINT                0x2A /* OEM specific in Windows 3.1 SDK */
	#define VK_EXECUTE              0x2B
	#define VK_SNAPSHOT             0x2C
	#define VK_INSERT               0x2D
	#define VK_DELETE               0x2E
	#define VK_HELP                 0x2F
	/* VK_0 - VK-9              0x30-0x39  Use ASCII instead */
	/*                          0x3A-0x40  Undefined */
	/* VK_A - VK_Z              0x41-0x5A  Use ASCII instead */
	#define VK_LWIN                 0x5B
	#define VK_RWIN                 0x5C
	#define VK_APPS                 0x5D
	/*                          0x5E-0x5F Unassigned */
	#define VK_NUMPAD0              0x60
	#define VK_NUMPAD1              0x61
	#define VK_NUMPAD2              0x62
	#define VK_NUMPAD3              0x63
	#define VK_NUMPAD4              0x64
	#define VK_NUMPAD5              0x65
	#define VK_NUMPAD6              0x66
	#define VK_NUMPAD7              0x67
	#define VK_NUMPAD8              0x68
	#define VK_NUMPAD9              0x69
	#define VK_MULTIPLY             0x6A
	#define VK_ADD                  0x6B
	#define VK_SEPARATOR            0x6C
	#define VK_SUBTRACT             0x6D
	#define VK_DECIMAL              0x6E
	#define VK_DIVIDE               0x6F
	#define VK_F1                   0x70
	#define VK_F2                   0x71
	#define VK_F3                   0x72
	#define VK_F4                   0x73
	#define VK_F5                   0x74
	#define VK_F6                   0x75
	#define VK_F7                   0x76
	#define VK_F8                   0x77
	#define VK_F9                   0x78
	#define VK_F10                  0x79
	#define VK_F11                  0x7A
	#define VK_F12                  0x7B
	#define VK_F13                  0x7C
	#define VK_F14                  0x7D
	#define VK_F15                  0x7E
	#define VK_F16                  0x7F
	#define VK_F17                  0x80
	#define VK_F18                  0x81
	#define VK_F19                  0x82
	#define VK_F20                  0x83
	#define VK_F21                  0x84
	#define VK_F22                  0x85
	#define VK_F23                  0x86
	#define VK_F24                  0x87
	/*                          0x88-0x8F  Unassigned */
	#define VK_NUMLOCK              0x90
	#define VK_SCROLL               0x91

	#define	FVIRTKEY			    0x01
	#define	FNOINVERT			    0x02
	#define	FSHIFT				    0x04
	#define	FCONTROL			    0x08
	#define	FALT				    0x10

	#define COLOR_SCROLLBAR		    		0
	#define COLOR_BACKGROUND	    		1
	#define COLOR_ACTIVECAPTION	    		2
	#define COLOR_INACTIVECAPTION	    	3
	#define COLOR_MENU		    			4
	#define COLOR_WINDOW		    		5
	#define COLOR_WINDOWFRAME	    		6
	#define COLOR_MENUTEXT		    		7
	#define COLOR_WINDOWTEXT	    		8
	#define COLOR_CAPTIONTEXT  	    		9
	#define COLOR_ACTIVEBORDER	   			10
	#define COLOR_INACTIVEBORDER	   		11
	#define COLOR_APPWORKSPACE	   			12
	#define COLOR_HIGHLIGHT		   			13
	#define COLOR_HIGHLIGHTTEXT	   			14
	#define COLOR_BTNFACE              		15
	#define COLOR_BTNSHADOW            		16
	#define COLOR_GRAYTEXT             		17
	#define COLOR_BTNTEXT		   			18
	#define COLOR_INACTIVECAPTIONTEXT  		19
	#define COLOR_BTNHIGHLIGHT         		20

	#define COLOR_3DDKSHADOW           		21
	#define COLOR_3DLIGHT              		22
	#define COLOR_INFOTEXT             		23
	#define COLOR_INFOBK               		24
	#define COLOR_DESKTOP              		COLOR_BACKGROUND
	#define COLOR_3DFACE               		COLOR_BTNFACE
	#define COLOR_3DSHADOW             		COLOR_BTNSHADOW
	#define COLOR_3DHIGHLIGHT          		COLOR_BTNHIGHLIGHT
	#define COLOR_3DHILIGHT            		COLOR_BTNHIGHLIGHT
	#define COLOR_BTNHILIGHT           		COLOR_BTNHIGHLIGHT

	#define COLOR_ALTERNATEBTNFACE      	25  /* undocumented, constant's name unknown */
	#define COLOR_HOTLIGHT              	26
	#define COLOR_GRADIENTACTIVECAPTION 	27
	#define COLOR_GRADIENTINACTIVECAPTION  	28

	#define COLOR_MENUHILIGHT              	29
	#define COLOR_MENUBAR                  	30

	#define REG_NONE                    	( 0 )   // No value type
	#define REG_SZ                      	( 1 )   // Unicode nul terminated string
	#define REG_EXPAND_SZ               	( 2 )   // Unicode nul terminated string
	#define REG_BINARY                  	( 3 )   // Free form binary
	#define REG_DWORD                   	( 4 )   // 32-bit number
	#define REG_DWORD_LITTLE_ENDIAN     	( 4 )   // 32-bit number (same as REG_DWORD)
	#define REG_DWORD_BIG_ENDIAN        	( 5 )   // 32-bit number
	#define REG_LINK                    	( 6 )   // Symbolic Link (unicode)
	#define REG_MULTI_SZ                	( 7 )   // Multiple Unicode strings
	#define REG_RESOURCE_LIST           	( 8 )   // Resource list in the resource map
	#define REG_FULL_RESOURCE_DESCRIPTOR 	( 9 )   // Resource list in the hardware description
	#define REG_RESOURCE_REQUIREMENTS_LIST 	( 10 )  //
	#define REG_QWORD                   	( 11 )  // 64-bit number
	#define REG_QWORD_LITTLE_ENDIAN     	( 11 )  // 64-bit number (same as REG_QWORD)

	#define SW_HIDE                         0
	#define SW_SHOWNORMAL                   1
	#define SW_NORMAL                       1
	#define SW_SHOWMINIMIZED                2
	#define SW_SHOWMAXIMIZED                3
	#define SW_MAXIMIZE                     3
	#define SW_SHOWNOACTIVATE               4
	#define SW_SHOW                         5
	#define SW_MINIMIZE                     6
	#define SW_SHOWMINNOACTIVE              7
	#define SW_SHOWNA                       8
	#define SW_RESTORE                      9
	#define SW_SHOWDEFAULT                  10
	#define SW_MAX                          10

	#define SWP_NOSIZE          0x0001
	#define SWP_NOMOVE          0x0002
	#define SWP_NOZORDER        0x0004
	#define SWP_NOREDRAW        0x0008
	#define SWP_NOACTIVATE      0x0010
	#define SWP_FRAMECHANGED    0x0020  /* The frame changed: send WM_NCCALCSIZE */
	#define SWP_SHOWWINDOW      0x0040
	#define SWP_HIDEWINDOW      0x0080
	#define SWP_NOCOPYBITS      0x0100
	#define SWP_NOOWNERZORDER   0x0200  /* Don't do owner Z ordering */
	#define SWP_NOSENDCHANGING  0x0400  /* Don't send WM_WINDOWPOSCHANGING */
	#define SWP_DRAWFRAME       SWP_FRAMECHANGED
	#define SWP_NOREPOSITION    SWP_NOOWNERZORDER

	#define HWND_TOP        ((HWND)0)
	#define HWND_BOTTOM     ((HWND)1)
	#define HWND_TOPMOST    ((HWND)-1)
	#define HWND_NOTOPMOST  ((HWND)-2)

	#define DT_TOP                      	0x00000000
	#define DT_LEFT                     	0x00000000
	#define DT_CENTER                   	0x00000001
	#define DT_RIGHT                    	0x00000002
	#define DT_VCENTER                  	0x00000004
	#define DT_BOTTOM                   	0x00000008
	#define DT_WORDBREAK                	0x00000010
	#define DT_SINGLELINE               	0x00000020
	#define DT_EXPANDTABS               	0x00000040
	#define DT_TABSTOP                  	0x00000080
	#define DT_NOCLIP                   	0x00000100
	#define DT_EXTERNALLEADING          	0x00000200
	#define DT_CALCRECT                 	0x00000400
	#define DT_NOPREFIX                 	0x00000800
	#define DT_INTERNAL                 	0x00001000

	#define DS_ABSALIGN         0x01L
	#define DS_SYSMODAL         0x02L
	#define DS_LOCALEDIT        0x20L   /* Edit items get Local storage. */
	#define DS_SETFONT          0x40L   /* User specified font for Dlg controls */
	#define DS_MODALFRAME       0x80L   /* Can be combined with WS_CAPTION  */
	#define DS_NOIDLEMSG        0x100L  /* WM_ENTERIDLE message will not be sent */
	#define DS_SETFOREGROUND    0x200L  /* not in win3.1 */
	#define DS_3DLOOK           0x0004L
	#define DS_FIXEDSYS         0x0008L
	#define DS_NOFAILCREATE     0x0010L
	#define DS_CONTROL          0x0400L
	#define DS_CENTER           0x0800L
	#define DS_CENTERMOUSE      0x1000L
	#define DS_CONTEXTHELP      0x2000L
	#define DS_SHELLFONT        (DS_SETFONT | DS_FIXEDSYS)

	#define ERROR_SUCCESS                   0L

	#define CREATE_NEW          	        1
	#define CREATE_ALWAYS       	        2
	#define OPEN_EXISTING       	        3
	#define OPEN_ALWAYS         	        4
	#define TRUNCATE_EXISTING   	        5

 	#define GENERIC_READ                     (0x80000000L)
	#define GENERIC_WRITE                    (0x40000000L)
	#define GENERIC_EXECUTE                  (0x20000000L)
	#define GENERIC_ALL                      (0x10000000L)

	#define INVALID_HANDLE_VALUE 			((HANDLE)-1)

	#define FILE_BEGIN           			SEEK_SET
	#define FILE_CURRENT         			SEEK_CUR
	#define FILE_END             			SEEK_END

	#define HKEY_CLASSES_ROOT               ((HKEY)0x80000000)
	#define HKEY_CURRENT_USER               ((HKEY)0x80000001)
	#define HKEY_LOCAL_MACHINE              ((HKEY)0x80000002)
	#define HKEY_USERS                      ((HKEY)0x80000003)
	#define HKEY_PERFORMANCE_DATA           ((HKEY)0x80000004)

	#define PROFILE_LINKED   				'LINK'
	#define PROFILE_EMBEDDED 				'MBED'

	#define BI_RGB           				0
	#define BI_RLE8          				1
	#define BI_RLE4          				2
	#define BI_BITFIELDS     				3

	#define WAVE_FORMAT_PCM     		   	1

   	#define LMEM_FIXED          0x0000
   	#define LMEM_MOVEABLE       0x0002
   	#define LMEM_NOCOMPACT      0x0010
   	#define LMEM_NODISCARD      0x0020
   	#define LMEM_ZEROINIT       0x0040
   	#define LMEM_MODIFY         0x0080
   	#define LMEM_DISCARDABLE    0x0F00
   	#define LMEM_VALID_FLAGS    0x0F72
   	#define LMEM_INVALID_HANDLE 0x8000

   	#define LHND                (LMEM_MOVEABLE | LMEM_ZEROINIT)
   	#define LPTR                (LMEM_FIXED | LMEM_ZEROINIT)

   	#define GMEM_FIXED          0x0000
   	#define GMEM_MOVEABLE       0x0002
   	#define GMEM_NOCOMPACT      0x0010
   	#define GMEM_NODISCARD      0x0020
   	#define GMEM_ZEROINIT       0x0040
   	#define GMEM_MODIFY         0x0080
   	#define GMEM_DISCARDABLE    0x0100
   	#define GMEM_NOT_BANKED     0x1000
   	#define GMEM_SHARE          0x2000
   	#define GMEM_DDESHARE       0x2000
   	#define GMEM_NOTIFY         0x4000
   	#define GMEM_LOWER          GMEM_NOT_BANKED
   	#define GMEM_VALID_FLAGS    0x7F72
   	#define GMEM_INVALID_HANDLE 0x8000
   	#define GHND                (GMEM_MOVEABLE | GMEM_ZEROINIT)
   	#define GPTR                (GMEM_FIXED | GMEM_ZEROINIT)

	#define FILE_SHARE_READ                 0x00000001
	#define FILE_SHARE_WRITE                0x00000002
	#define FILE_SHARE_DELETE               0x00000004
	#define FILE_ATTRIBUTE_READONLY             0x00000001
	#define FILE_ATTRIBUTE_HIDDEN               0x00000002
	#define FILE_ATTRIBUTE_SYSTEM               0x00000004
	#define FILE_ATTRIBUTE_DIRECTORY            0x00000010
	#define FILE_ATTRIBUTE_ARCHIVE              0x00000020
	#define FILE_ATTRIBUTE_DEVICE               0x00000040
	#define FILE_ATTRIBUTE_NORMAL               0x00000080
	#define FILE_ATTRIBUTE_TEMPORARY            0x00000100
	#define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200
	#define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400
	#define FILE_ATTRIBUTE_COMPRESSED           0x00000800
	#define FILE_ATTRIBUTE_OFFLINE              0x00001000
	#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000
	#define FILE_ATTRIBUTE_ENCRYPTED            0x00004000
	#define FILE_NOTIFY_CHANGE_FILE_NAME    0x00000001
	#define FILE_NOTIFY_CHANGE_DIR_NAME     0x00000002
	#define FILE_NOTIFY_CHANGE_ATTRIBUTES   0x00000004
	#define FILE_NOTIFY_CHANGE_SIZE         0x00000008
	#define FILE_NOTIFY_CHANGE_LAST_WRITE   0x00000010
	#define FILE_NOTIFY_CHANGE_LAST_ACCESS  0x00000020
	#define FILE_NOTIFY_CHANGE_CREATION     0x00000040
	#define FILE_NOTIFY_CHANGE_SECURITY     0x00000100
	#define FILE_ACTION_ADDED                   0x00000001
	#define FILE_ACTION_REMOVED                 0x00000002
	#define FILE_ACTION_MODIFIED                0x00000003
	#define FILE_ACTION_RENAMED_OLD_NAME        0x00000004
	#define FILE_ACTION_RENAMED_NEW_NAME        0x00000005
	#define MAILSLOT_NO_MESSAGE             ((DWORD)-1)
	#define MAILSLOT_WAIT_FOREVER           ((DWORD)-1)
	#define FILE_CASE_SENSITIVE_SEARCH      0x00000001
	#define FILE_CASE_PRESERVED_NAMES       0x00000002
	#define FILE_UNICODE_ON_DISK            0x00000004
	#define FILE_PERSISTENT_ACLS            0x00000008
	#define FILE_FILE_COMPRESSION           0x00000010
	#define FILE_VOLUME_QUOTAS              0x00000020
	#define FILE_SUPPORTS_SPARSE_FILES      0x00000040
	#define FILE_SUPPORTS_REPARSE_POINTS    0x00000080
	#define FILE_SUPPORTS_REMOTE_STORAGE    0x00000100
	#define FILE_VOLUME_IS_COMPRESSED       0x00008000
	#define FILE_SUPPORTS_OBJECT_IDS        0x00010000
	#define FILE_SUPPORTS_ENCRYPTION        0x00020000
	#define FILE_NAMED_STREAMS              0x00040000

	#define ERROR_NO_MORE_FILES     18

	#define GWL_ID              1
	#define GWL_WNDPROC         2
	#define GWL_HINSTANCE       3
	#define GWL_HWNDPARENT      4
	#define GWL_STYLE           5
	#define GWL_EXSTYLE         6
	#define GWL_USERDATA        7
	#define DWL_MSGRESULT		8
	#define GWL_NOTIFYPARENT	10
	#define GWL_LPARAM			11
	#define GWL_STATE			12
	#define GWL_FIXED			14
	#define GWL_MENU			15
	#define GWL_IMAGELIST		16
	#define GWL_TOOLTIPS		17
	#define GWL_ID2				18
	#define	GWL_SIGNAL			30
	#define GWL_DATA_INT0		97
	#define GWL_MESSAGES		98
	#define GWL_CARET			99

	/*
 	* GetSystemMetrics() codes
 	*/
	#define SM_CXSCREEN             0
	#define SM_CYSCREEN             1
	#define SM_CXVSCROLL            2
	#define SM_CYHSCROLL            3
	#define SM_CYCAPTION            4
	#define SM_CXBORDER             5
	#define SM_CYBORDER             6
	#define SM_CXDLGFRAME           7
	#define SM_CYDLGFRAME           8
	#define SM_CYVTHUMB             9
	#define SM_CXHTHUMB             10
	#define SM_CXICON               11
	#define SM_CYICON               12
	#define SM_CXCURSOR             13
	#define SM_CYCURSOR             14
	#define SM_CYMENU               15
	#define SM_CXFULLSCREEN         16
	#define SM_CYFULLSCREEN         17
	#define SM_CYKANJIWINDOW        18
	#define SM_MOUSEPRESENT         19
	#define SM_CYVSCROLL            20
	#define SM_CXHSCROLL            21
	#define SM_DEBUG                22
	#define SM_SWAPBUTTON           23
	#define SM_RESERVED1            24
	#define SM_RESERVED2            25
	#define SM_RESERVED3            26
	#define SM_RESERVED4            27
	#define SM_CXMIN                28
	#define SM_CYMIN                29
	#define SM_CXSIZE               30
	#define SM_CYSIZE               31
	#define SM_CXFRAME              32
	#define SM_CYFRAME              33
	#define SM_CXMINTRACK           34
	#define SM_CYMINTRACK           35
	#define SM_CXDOUBLECLK          36
	#define SM_CYDOUBLECLK          37
	#define SM_CXICONSPACING        38
	#define SM_CYICONSPACING        39
	#define SM_MENUDROPALIGNMENT    40
	#define SM_PENWINDOWS           41
	#define SM_DBCSENABLED          42
	#define SM_CMOUSEBUTTONS        43
	#define SM_CXFIXEDFRAME           SM_CXDLGFRAME  /* ;win40 name change */
	#define SM_CYFIXEDFRAME           SM_CYDLGFRAME  /* ;win40 name change */
	#define SM_CXSIZEFRAME            SM_CXFRAME     /* ;win40 name change */
	#define SM_CYSIZEFRAME            SM_CYFRAME     /* ;win40 name change */
	#define SM_SECURE               44
	#define SM_CXEDGE               45
	#define SM_CYEDGE               46
	#define SM_CXMINSPACING         47
	#define SM_CYMINSPACING         48
	#define SM_CXSMICON             49
	#define SM_CYSMICON             50
	#define SM_CYSMCAPTION          51
	#define SM_CXSMSIZE             52
	#define SM_CYSMSIZE             53
	#define SM_CXMENUSIZE           54
	#define SM_CYMENUSIZE           55
	#define SM_ARRANGE              56
	#define SM_CXMINIMIZED          57
	#define SM_CYMINIMIZED          58
	#define SM_CXMAXTRACK           59
	#define SM_CYMAXTRACK           60
	#define SM_CXMAXIMIZED          61
	#define SM_CYMAXIMIZED          62
	#define SM_NETWORK              63
	#define SM_CLEANBOOT            67
	#define SM_CXDRAG               68
	#define SM_CYDRAG               69
	#define SM_SHOWSOUNDS           70
	#define SM_CXMENUCHECK          71   /* Use instead of GetMenuCheckMarkDimensions()! */
	#define SM_CYMENUCHECK          72
	#define SM_SLOWMACHINE          73
	#define SM_MIDEASTENABLED       74
	#define SM_MOUSEWHEELPRESENT    75
	#define SM_XVIRTUALSCREEN       76
	#define SM_YVIRTUALSCREEN       77
	#define SM_CXVIRTUALSCREEN      78
	#define SM_CYVIRTUALSCREEN      79
	#define SM_CMONITORS            80
	#define SM_SAMEDISPLAYFORMAT    81
	#define SM_IMMENABLED           82
	#define SM_CMETRICS             83
	#define SM_REMOTESESSION        0x1000

	#define IMAGE_BITMAP        0
	#define IMAGE_ICON          1
	#define IMAGE_CURSOR        2

	#define LR_DEFAULTCOLOR     0x0000
	#define LR_MONOCHROME       0x0001
	#define LR_COLOR            0x0002
	#define LR_COPYRETURNORG    0x0004
	#define LR_COPYDELETEORG    0x0008
	#define LR_LOADFROMFILE     0x0010
	#define LR_LOADTRANSPARENT  0x0020
	#define LR_DEFAULTSIZE      0x0040
	#define LR_VGACOLOR         0x0080
	#define LR_LOADMAP3DCOLORS  0x1000
	#define LR_CREATEDIBSECTION 0x2000
	#define LR_COPYFROMRESOURCE			0x4000
	#define LR_SHARED           		0x8000

	#define SRCCOPY             		(DWORD)0x00CC0020 /* dest = source                   */
	#define SRCPAINT            		(DWORD)0x00EE0086 /* dest = source OR dest           */
	#define SRCAND             			(DWORD)0x008800C6 /* dest = source AND dest          */
	#define SRCINVERT           		(DWORD)0x00660046 /* dest = source XOR dest          */
	#define SRCERASE            		(DWORD)0x00440328 /* dest = source AND (NOT dest )   */
	#define NOTSRCCOPY          		(DWORD)0x00330008 /* dest = (NOT source)             */
	#define NOTSRCERASE        		 	(DWORD)0x001100A6 /* dest = (NOT src) AND (NOT dest) */
	#define MERGECOPY           		(DWORD)0x00C000CA /* dest = (source AND pattern)     */
	#define MERGEPAINT          		(DWORD)0x00BB0226 /* dest = (NOT source) OR dest     */
	#define PATCOPY             		(DWORD)0x00F00021 /* dest = pattern                  */
	#define PATPAINT           			(DWORD)0x00FB0A09 /* dest = DPSnoo                   */
	#define PATINVERT           		(DWORD)0x005A0049 /* dest = pattern XOR dest         */
	#define DSTINVERT           		(DWORD)0x00550009 /* dest = (NOT dest)               */
	#define BLACKNESS           		(DWORD)0x00000042 /* dest = BLACK                    */
	#define WHITENESS           		(DWORD)0x00FF0062 /* dest = WHITE                    */

	#define ETO_OPAQUE                   0x0002
	#define ETO_CLIPPED                  0x0004

	#define TA_NOUPDATECP                0
	#define TA_UPDATECP                  1
	#define TA_LEFT                      0
	#define TA_RIGHT                     2
	#define TA_CENTER                    6
	#define TA_TOP                       0
	#define TA_BOTTOM                    8

	#define TA_BASELINE                  24

	#define CF_TEXT             1
	#define CF_BITMAP           2
	#define CF_METAFILEPICT     3
	#define CF_SYLK             4
	#define CF_DIF              5
	#define CF_TIFF             6
	#define CF_OEMTEXT          7
	#define CF_DIB              8
	#define CF_PALETTE          9
	#define CF_PENDATA          10
	#define CF_RIFF             11
	#define CF_WAVE             12
	#define CF_UNICODETEXT      13
	#define CF_ENHMETAFILE      14
	/*
 	* Flags for TrackPopupMenu
 	*/
	#define TPM_LEFTBUTTON  0x0000L
	#define TPM_RIGHTBUTTON 0x0002L
	#define TPM_LEFTALIGN   0x0000L
	#define TPM_CENTERALIGN 0x0004L
	#define TPM_RIGHTALIGN  0x0008L
	#define TPM_TOPALIGN        0x0000L
	#define TPM_VCENTERALIGN    0x0010L
	#define TPM_BOTTOMALIGN     0x0020L
	#define TPM_HORIZONTAL      0x0000L     /* Horz alignment matters more */
	#define TPM_VERTICAL        0x0040L     /* Vert alignment matters more */
	#define TPM_NONOTIFY        0x0080L     /* Don't send any notification msgs */
	#define TPM_RETURNCMD       0x0100L
	/*
 	* RedrawWindow() flags
 	*/
	#define RDW_INVALIDATE          0x0001
	#define RDW_INTERNALPAINT       0x0002
	#define RDW_ERASE               0x0004
	#define RDW_VALIDATE            0x0008
	#define RDW_NOINTERNALPAINT     0x0010
	#define RDW_NOERASE             0x0020
	#define RDW_NOCHILDREN          0x0040
	#define RDW_ALLCHILDREN         0x0080
	#define RDW_UPDATENOW           0x0100
	#define RDW_ERASENOW            0x0200
	#define RDW_FRAME               0x0400
	#define RDW_NOFRAME             0x0800

	/* Background Modes */
	#define TRANSPARENT         1
	#define OPAQUE              2
	#define BKMODE_LAST         2
	#define SORT_DEFAULT                     0x0     // sorting default
	#define LANG_NEUTRAL                     0x00
	#define SUBLANG_NEUTRAL                  0x00    // language neutral
	#define SUBLANG_DEFAULT                  0x01    // user default
	#define SUBLANG_SYS_DEFAULT              0x02    // system default
	/* 3D border styles */
	#define BDR_RAISEDOUTER 			0x0001
	#define BDR_SUNKENOUTER 			0x0002
	#define BDR_RAISEDINNER 			0x0004
	#define BDR_SUNKENINNER 			0x0008
	#define BDR_OUTER       			(BDR_RAISEDOUTER | BDR_SUNKENOUTER)
	#define BDR_INNER       			(BDR_RAISEDINNER | BDR_SUNKENINNER)
	#define BDR_RAISED      			(BDR_RAISEDOUTER | BDR_RAISEDINNER)
	#define BDR_SUNKEN      			(BDR_SUNKENOUTER | BDR_SUNKENINNER)
	#define EDGE_RAISED     			(BDR_RAISEDOUTER | BDR_RAISEDINNER)
	#define EDGE_SUNKEN     			(BDR_SUNKENOUTER | BDR_SUNKENINNER)
	#define EDGE_ETCHED     			(BDR_SUNKENOUTER | BDR_RAISEDINNER)
	#define EDGE_BUMP       			(BDR_RAISEDOUTER | BDR_SUNKENINNER)
	/* Border flags */
	#define BF_LEFT         			0x0001
	#define BF_TOP          			0x0002
	#define BF_RIGHT        			0x0004
	#define BF_BOTTOM      				0x0008
	#define BF_TOPLEFT      			(BF_TOP | BF_LEFT)
	#define BF_TOPRIGHT     			(BF_TOP | BF_RIGHT)
	#define BF_BOTTOMLEFT   			(BF_BOTTOM | BF_LEFT)
	#define BF_BOTTOMRIGHT  			(BF_BOTTOM | BF_RIGHT)
	#define BF_RECT         			(BF_LEFT | BF_TOP | BF_RIGHT | BF_BOTTOM)
	#define BF_DIAGONAL     			0x0010
	// For diagonal lines, the BF_RECT flags specify the end point of the
	// vector bounded by the rectangle parameter.
	#define BF_DIAGONAL_ENDTOPRIGHT     (BF_DIAGONAL | BF_TOP | BF_RIGHT)
	#define BF_DIAGONAL_ENDTOPLEFT      (BF_DIAGONAL | BF_TOP | BF_LEFT)
	#define BF_DIAGONAL_ENDBOTTOMLEFT   (BF_DIAGONAL | BF_BOTTOM | BF_LEFT)
	#define BF_DIAGONAL_ENDBOTTOMRIGHT  (BF_DIAGONAL | BF_BOTTOM | BF_RIGHT)
	#define BF_MIDDLE       			0x0800  /* Fill in the middle */
	#define BF_SOFT        				0x1000  /* For softer buttons */
	#define BF_ADJUST       			0x2000  /* Calculate the space left over */
	#define BF_FLAT         			0x4000  /* For flat rather than 3D borders */
	#define BF_MONO         			0x8000  /* For monochrome borders */

	#define DIB_RGB_COLORS      0 /* color table in RGBs */
	#define DIB_PAL_COLORS      1 /* color table in palette indices */

	#define DRIVERVERSION 0     /* Device driver version                    */
	#define TECHNOLOGY    2     /* Device classification                    */
	#define HORZSIZE      4     /* Horizontal size in millimeters           */
	#define VERTSIZE      6     /* Vertical size in millimeters             */
	#define HORZRES       8     /* Horizontal width in pixels               */
	#define VERTRES       10    /* Vertical height in pixels                */
	#define BITSPIXEL     12    /* Number of bits per pixel                 */
	#define PLANES        14    /* Number of planes                         */
	#define NUMBRUSHES    16    /* Number of brushes the device has         */
	#define NUMPENS       18    /* Number of pens the device has            */
	#define NUMMARKERS    20    /* Number of markers the device has         */
	#define NUMFONTS      22    /* Number of fonts the device has           */
	#define NUMCOLORS     24    /* Number of colors the device supports     */
	#define PDEVICESIZE   26    /* Size required for device descriptor      */
	#define CURVECAPS     28    /* Curve capabilities                       */
	#define LINECAPS      30    /* Line capabilities                        */
	#define POLYGONALCAPS 32    /* Polygonal capabilities                   */
	#define TEXTCAPS      34    /* Text capabilities                        */
	#define CLIPCAPS      36    /* Clipping capabilities                    */
	#define RASTERCAPS    38    /* Bitblt capabilities                      */
	#define ASPECTX       40    /* Length of the X leg                      */
	#define ASPECTY       42    /* Length of the Y leg                      */
	#define ASPECTXY      44    /* Length of the hypotenuse                 */

	#define WHEEL_DELTA   120
	#define MAKELANGID(p, s)       ((((WORD  )(s)) << 10) | (WORD  )(p))
	#define PRIMARYLANGID(lgid)    ((WORD  )(lgid) & 0x3ff)
	#define SUBLANGID(lgid)        ((WORD  )(lgid) >> 10)
	#define MAKELCID(lgid, srtid)  ((DWORD)((((DWORD)((WORD  )(srtid))) << 16) |  \
                                   ((DWORD)((WORD  )(lgid)))))
	#define MAKESORTLCID(lgid, srtid, ver) \
                               ((DWORD)((MAKELCID(lgid, srtid)) | \
                               (((DWORD)((WORD  )(ver))) << 20)))
	#define LANGIDFROMLCID(lcid)  		((WORD  )(lcid))
	#define SORTIDFROMLCID(lcid)   		((WORD  )((((DWORD)(lcid)) >> 16) & 0xf))
	#define SORTVERSIONFROMLCID(lcid)  	((WORD  )((((DWORD)(lcid)) >> 20) & 0xf))
	//
	//  Default System and User IDs for language and locale.
	//
	#define LANG_SYSTEM_DEFAULT    (MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT))
	#define LANG_USER_DEFAULT      (MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))
	#define LOCALE_SYSTEM_DEFAULT  (MAKELCID(LANG_SYSTEM_DEFAULT, SORT_DEFAULT))
	#define LOCALE_USER_DEFAULT    (MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT))
	#define LOCALE_NEUTRAL         (MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT))

	#define DATE_SHORTDATE            				0x00000001  // use short date picture
	#define LOCALE_NOUSEROVERRIDE         			0x80000000   // do not use user overrides

	#define S_OK                              		((HRESULT)0x00000000L)
	#define S_FALSE                          		((HRESULT)0x00000001L)
	#define E_FAIL								    ((HRESULT)0x80004005L)
	#define E_INVALIDARG                       		((HRESULT)0x80000003L)
	#define E_NOINTERFACE                     		((HRESULT)0x80000004L)

	#define GetBValue(rgb)      ((BYTE)(rgb))
	#define GetGValue(rgb)      ((BYTE)(((WORD)(rgb)) >> 8))
	#define GetRValue(rgb)      ((BYTE)((rgb)>>16))

	#define ZeroMemory(a,b) 				memset(a,0,b)
	#define CopyMemory(a,b,c) 				memcpy(a,b,c)
	#define FillMemory(a,b,c)				memset(a,c,b)
	#define wsprintf					   	sprintf
  	#define lstrlen							strlen
  	#define lstrcpy             		    strcpy
  	#define lstrcpyn(a,b,c)		            strncpy(a,b,c-1)
  	#define lstrcmp				            strcmp
	#define lstrcmpi						strcasecmp
  	#define wvsprintf			            vsprintf
	#define lstrcat                         strcat
	#define RGB(r,g,b)          			((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

	#define PropSheet_AddPage(hDlg, hpage) \
        SendMessage(hDlg, PSM_ADDPAGE, 0, (LPARAM)hpage)

	DWORD GetFileAttributes(const char *file);
	DWORD GetTempPath(DWORD nBufferLength,LPSTR lpBuffer);
	DWORD GetLastError();
	BOOL FindClose(HANDLE hFindFile);
	HANDLE FindFirstFile(LPCSTR lpFileName,LPWIN32_FIND_DATA lpFindFileData);
	BOOL FindNextFile(HANDLE hFindFile,LPWIN32_FIND_DATA lpFindFileData);
	BOOL DeleteFile(LPCSTR lpFileName);
	UINT GetTempFileName(LPCSTR lpPathName,LPCSTR lpPrefixString,UINT uUnique,LPSTR lpTempFileName);
	void strlwr(char *s);

	void SetRect(LPRECT p,int x,int y,int width,int height);
	void CopyRect(LPRECT p,LPRECT p1);
	BOOL PtInRect(CONST RECT *lprc,POINT pt);
	BOOL InflateRect(LPRECT lprc,int dx,int dy);
    BOOL IsRectEmpty(const RECT *lprc);
	BOOL OffsetRect(LPRECT lprc,int dx,int dy);
	HWND CreateDialogParam(HINSTANCE hInstance,LPCSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
	int DialogBoxParam(HINSTANCE hInstance,LPCSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
	int DialogBox(HINSTANCE hInstance,LPCSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc);
	int WideCharToMultiByte(UINT CodePage,DWORD dwFlags,LPCWSTR lpWideCharStr,int cchWideChar,LPSTR lpMultiByteStr,int cbMultiByte,LPCSTR lpDefaultChar,LPBOOL lpUsedDefaultChar);
   	HINSTANCE LoadLibrary(LPCTSTR lpLibFileName);
	HINSTANCE GetModuleHandle(LPCTSTR lpLibFileName);
   	BOOL FreeLibrary(HMODULE hLibModule);
   	FARPROC GetProcAddress(HMODULE hModule,LPCSTR lpProcName);
	DWORD GetCurrentDirectory(DWORD nBufferLength,LPSTR lpBuffer);
	BOOL SetCurrentDirectory(LPCSTR lpPathName);
	SHORT GetAsyncKeyState(int vKey);

	HWND CreateWindowEx(DWORD dwExStyle,LPCSTR lpClassName,LPCSTR lpWindowName,DWORD dwStyle,int X,int Y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam);
	HWND CreateWindow(LPCSTR lpClassName,LPCSTR lpWindowName,DWORD dwStyle,int X,int Y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam);
	BOOL BringWindowToTop(HWND hWnd);
	HWND GetActiveWindow();
	BOOL SetForegroundWindow(HWND hWnd);

	BOOL IsDialogMessage(HWND hDlg,LPMSG lpMsg);

	long GetDialogBaseUnits();
	HWND GetNextDlgTabItem(HWND hDlg,HWND hCtl,BOOL bPrevious);
	HWND SetParent(HWND hWndChild,HWND hWndNewParent);
	HWND GetParent(HWND hWnd);
	BOOL EndDialog(HWND hDlg,INT nResult);
	BOOL DestroyWindow(HWND hwnd);
	LRESULT DefWindowProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
	LRESULT CallWindowProc(WNDPROC lpPrevWndFunc,HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);

	BOOL SetWindowPos(HWND hWnd,HWND hWndInsertAfter,int X,int Y,int cx,int cy,UINT uFlags);
	BOOL MoveWindow(HWND hWnd,int X,int Y,int nWidth,int nHeight,BOOL bRepaint);
	int GetSystemMetrics(int nIndex);
	BOOL AdjustWindowRect(LPRECT lpRect,DWORD dwStyle,BOOL bMenu);
	BOOL IsWindow(HWND hWnd);
	HDWP BeginDeferWindowPos(int nNumWindows);
	HDWP DeferWindowPos(HDWP hWinPosInfo,HWND hWnd,HWND hWndInsertAfter,int x,int y,int cx,int cy,UINT uFlags);
	BOOL EndDeferWindowPos(HDWP hWinPosInfo);
	BOOL InvalidateRect(HWND hWnd,CONST RECT *lpRect,BOOL bErase);
	BOOL UpdateWindow(HWND hWnd);
	BOOL RedrawWindow(HWND hWnd,CONST RECT *lprcUpdate,HRGN hrgnUpdate,UINT flags);

	HWND GetCapture();
	HWND SetCapture(HWND hWnd);
	BOOL ReleaseCapture();
	HDC BeginPaint(HWND hWnd,LPPAINTSTRUCT lpPaint);
	BOOL EndPaint(HWND hWnd,CONST PAINTSTRUCT *lpPaint);

	int GetKeyNameText(LONG lParam,LPSTR lpString,int nSize);
	UINT SetTimer(HWND hWnd,UINT nIDEvent,UINT uElapse,TIMERPROC lpTimerFunc);
	BOOL KillTimer(HWND hWnd,UINT uIDEvent);
	BOOL SHGetPathFromIDList(LPCITEMIDLIST pidl,LPSTR pszPath);
	LPITEMIDLIST SHBrowseForFolder(LPBROWSEINFO lpbi);
	BOOL GetCursorPos(LPPOINT lpPoint);
	VOID GetLocalTime(LPSYSTEMTIME lpSystemTime);
	BOOL DosDateTimeToFileTime(WORD wFatDate,WORD wFatTime,LPFILETIME lpFileTime);
	BOOL FileTimeToDosDateTime(CONST FILETIME *lpFileTime,LPWORD lpFatDate,LPWORD lpFatTime);
	BOOL FileTimeToLocalFileTime(FILETIME *lpFileTime,LPFILETIME lpLocalFileTime);
	BOOL FileTimeToSystemTime(FILETIME *lpFileTime,LPSYSTEMTIME lpSystemTime);
	BOOL SystemTimeToFileTime(CONST SYSTEMTIME *lpSystemTime,LPFILETIME lpFileTime);
	int GetTimeFormat(LCID Locale,DWORD dwFlags,SYSTEMTIME *lpTime,LPCSTR lpFormat,LPSTR lpTimeStr,int cchTime);
	int GetDateFormat(LCID Locale,DWORD dwFlags,SYSTEMTIME *lpDate,LPCSTR lpFormat,LPSTR lpDateStr,int cchDate);

	int CopyAcceleratorTable(HACCEL hAccelSrc,LPACCEL lpAccelDst,int cAccelEntries);
	int TranslateAccelerator(HWND hWnd,HACCEL hAccTable,LPMSG lpMsg);
	BOOL DestroyAcceleratorTable(HACCEL hAccel);
	HACCEL CreateAcceleratorTable(LPACCEL accel,int cAccelEntries);
	HACCEL LoadAccelerators(HINSTANCE hInstance,LPCSTR lpTableName);

	HDC GetDC(HWND hwnd);
	int ReleaseDC(HWND hWnd,HDC hDC);
	BOOL DeleteDC(HDC hDC);
	int GetDeviceCaps(HDC hDC,int nValue);
	HDC CreateCompatibleDC(HDC hDC);
	BOOL GetTextExtentPoint32(HDC hDC,LPCSTR lpszText,int cchTextMax,LPSIZE psz);
	int GetObject(void *obj,int buflen,LPVOID buf);
	HGDIOBJ SelectObject(HDC hDC,HGDIOBJ hObj);
	HBITMAP CreateBitmap(int,int,UINT,UINT,CONST VOID *);

	int StretchDIBits(HDC hdc,int XDest,int YDest,int nDestWidth,int nDestHeight,int XSrc,int YSrc,int nSrcWidth,int nSrcHeight,const VOID *lpBits,const BITMAPINFO *lpBitsInfo,UINT iUsage,DWORD dwRop);
	HBITMAP CreateDIBSection(HDC hdc,BITMAPINFO *pbmi,UINT iUsage,VOID **ppvBits,HANDLE hSection,DWORD dwOffset);

	void DeleteObject(void *obj);
	COLORREF SetBkColor(HDC hDC,COLORREF color);
	COLORREF SetTextColor(HDC hDC,COLORREF color);
	HBRUSH GetSysColorBrush(int nIndex);
	HBRUSH CreateSolidBrush(COLORREF color);
	DWORD GetSysColor(int nIndex);
	int FillRect(HDC hDC,CONST RECT *lprc,HBRUSH hbr);
	BOOL DrawFocusRect(HDC hDC,CONST RECT * lprc);
	int SetBkMode(HDC hDC,int mode);
	UINT SetTextAlign(HDC hdc,UINT fMode);
	int DrawText(HDC hDC,LPCSTR lpString,int nCount,LPRECT lpRect,UINT uFormat);
    BOOL ExtTextOut(HDC hDC,int x,int y,UINT flags,LPRECT lpRect,LPCSTR lpString,UINT nCount,INT *lpTabs);
	BOOL TextOut(HDC hDC,int x,int y,LPCSTR lpString,int nCount);
	BOOL BitBlt(HDC hdcDest,int nXDest,int nYDest,int nWidth,int nHeight,HDC hdcSrc,int nXSrc,int nYSrc,DWORD dwRop);
	BOOL DrawEdge(HDC hdc,LPRECT qrc,UINT edge,UINT grfFlags);
	int SetScrollInfo(HWND hWnd,int,LPCSCROLLINFO,BOOL);
	BOOL GetScrollInfo(HWND hWnd,int,LPSCROLLINFO);
	int SetScrollPos(HWND hWnd,int nBar,int nPos,BOOL bRedraw);
	int GetScrollPos(HWND hWnd,int nBar);

	BOOL OpenClipboard(HWND hWndNewOwner);
	BOOL CloseClipboard();
	BOOL EmptyClipboard();
	HANDLE SetClipboardData(UINT uFormat,HANDLE hMem);

	BOOL CreateCaret(HWND hWnd,HBITMAP hBitmap,int nWidth,int nHeight);
	BOOL DestroyCaret();
	BOOL HideCaret(HWND hWnd);
	BOOL ShowCaret(HWND hWnd);
	BOOL SetCaretPos(int X,int Y);

	HFONT CreateFontIndirect(LOGFONT *lf);

	UINT DragQueryFile(HDROP hDrop,UINT iFile,LPTSTR lpszFile,UINT cch);
	VOID DragFinish(HDROP hDrop);

	HFONT GetWindowFont(HWND hWnd);
	UINT ShellExecute(HWND hwnd,LPCSTR lpOperation,LPCSTR lpFile,LPCSTR lpParameters,LPCSTR lpDirectory,INT nShowCmd);

    void GetSystemInfo(LPSYSTEM_INFO lpSystemInfo);

	BOOL change_signal(HWND hWnd,int index,const gchar *detailed_signal,GCallback c_handler,gpointer gobject,BOOL bAfter);
	gboolean on_window_event(GtkWidget *widget,GdkEvent *event, gpointer user_data);
	void gtk_widget_subclass(GtkWidget *widget);
	//For internal use
	unsigned int strlenW( const WCHAR *str );
	BOOL CallWindowProc(GdkEvent *e,GtkWidget *w);
	int KeyCodeToVK(GdkEventKey *event);
	int VKToKeyCodeTo(int value);
#endif

#endif // __WIN32__

