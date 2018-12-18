#include "lwindow.h"
#include "llist.h"

#ifndef __WIN32__

#ifndef __LCOMMCTRLH__
#define __LCOMMCTRLH__

	#define TOOLBARCLASSNAME       "ToolbarWindow32"

	#define NM_FIRST                (0U-  0U)       // generic to all controls
	#define NM_CLICK                (NM_FIRST-2)    // uses NMCLICK struct
	#define NM_DBLCLK               (NM_FIRST-3)
	#define NM_RCLICK               (NM_FIRST-5)    // uses NMCLICK struct

	#define UDN_FIRST               (0U-721)        // updown
	#define UDN_LAST                (0U-740)
	
	typedef struct _UDACCEL {
    	UINT nSec;
    	UINT nInc;
	} UDACCEL, FAR *LPUDACCEL;

	#define UD_MAXVAL               0x7fff
	#define UD_MINVAL               (-UD_MAXVAL)

	#define UDS_WRAP                0x0001
	#define UDS_SETBUDDYINT         0x0002
	#define UDS_ALIGNRIGHT          0x0004
	#define UDS_ALIGNLEFT           0x0008
	#define UDS_AUTOBUDDY           0x0010
	#define UDS_ARROWKEYS           0x0020
	#define UDS_HORZ                0x0040
	#define UDS_NOTHOUSANDS         0x0080
	#define UDS_HOTTRACK            0x0100

	#define UDM_SETRANGE            (WM_USER+101)
	#define UDM_GETRANGE            (WM_USER+102)
	#define UDM_SETPOS              (WM_USER+103)
	#define UDM_GETPOS              (WM_USER+104)
	#define UDM_SETBUDDY            (WM_USER+105)
	#define UDM_GETBUDDY            (WM_USER+106)
	#define UDM_SETACCEL            (WM_USER+107)
	#define UDM_GETACCEL            (WM_USER+108)
	#define UDM_SETBASE             (WM_USER+109)
	#define UDM_GETBASE             (WM_USER+110)
	#define UDM_SETRANGE32          (WM_USER+111)
	#define UDM_GETRANGE32          (WM_USER+112) // wParam & lParam are LPINT
	#define UDM_SETUNICODEFORMAT    CCM_SETUNICODEFORMAT
	#define UDM_GETUNICODEFORMAT    CCM_GETUNICODEFORMAT
	#define UDM_SETPOS32            (WM_USER+113)
	#define UDM_GETPOS32            (WM_USER+114)

	HWND CreateUpDownControl(DWORD dwStyle);

	
	#define UDN_DELTAPOS            (UDN_FIRST - 1)

	#define TBN_FIRST               (0U-700U)
	#define TBN_LAST                (0U-720U)

	#define TCN_FIRST               (0U-550U)       // tab control
	#define TCN_LAST                (0U-580U)

	#define TCM_FIRST               0x1300

	#define CCS_TOP                 0x00000001L
	#define CCS_NOMOVEY             0x00000002L
	#define CCS_BOTTOM              0x00000003L
	#define CCS_NORESIZE            0x00000004L
	#define CCS_NOPARENTALIGN       0x00000008L
	#define CCS_ADJUSTABLE          0x00000020L
	#define CCS_NODIVIDER           0x00000040L
	
	#define TB_ADDBUTTONS           (WM_USER + 20)
	#define TB_BUTTONSTRUCTSIZE     (WM_USER + 30)
	#define TB_SETINDENT            (WM_USER + 47)
	#define TB_SETIMAGELIST         (WM_USER + 48)
	#define TB_GETIMAGELIST         (WM_USER + 49)
	#define TB_LOADIMAGES           (WM_USER + 50)
	#define TB_GETRECT              (WM_USER + 51)
	#define TB_SETHOTIMAGELIST      (WM_USER + 52)
	#define TB_GETHOTIMAGELIST      (WM_USER + 53)
	#define TB_SETDISABLEDIMAGELIST (WM_USER + 54)
	#define TB_GETDISABLEDIMAGELIST (WM_USER + 55)
	#define TB_SETSTYLE             (WM_USER + 56)
	#define TB_GETSTYLE             (WM_USER + 57)
	#define TB_GETBUTTONSIZE        (WM_USER + 58)
	#define TB_SETBUTTONWIDTH       (WM_USER + 59)
	#define TB_SETMAXTEXTROWS       (WM_USER + 60)
	#define TB_GETTEXTROWS          (WM_USER + 61)
	#define TB_SETEXTENDEDSTYLE     (WM_USER + 84)

	#define TB_ENABLEBUTTON         (WM_USER + 1)
	#define TB_CHECKBUTTON          (WM_USER + 2)
	#define TB_PRESSBUTTON          (WM_USER + 3)
	#define TB_HIDEBUTTON           (WM_USER + 4)
	#define TB_INDETERMINATE        (WM_USER + 5)

	#define TBS_AUTOTICKS           0x0001
	#define TBS_VERT                0x0002
	#define TBS_HORZ                0x0000
	#define TBS_TOP                 0x0004
	#define TBS_BOTTOM              0x0000
	#define TBS_LEFT                0x0004
	#define TBS_RIGHT               0x0000
	#define TBS_BOTH                0x0008
	#define TBS_NOTICKS             0x0010
	#define TBS_ENABLESELRANGE      0x0020
	#define TBS_FIXEDLENGTH         0x0040
	#define TBS_NOTHUMB             0x0080
	#define TBS_TOOLTIPS            0x0100
	#define TBS_REVERSED            0x0200  // Accessibility hint: the smaller number (usually the min value) means "high" and the larger number (usually the max value) means "low"

	#define TBM_GETPOS              (WM_USER)
	#define TBM_GETRANGEMIN         (WM_USER+1)
	#define TBM_GETRANGEMAX         (WM_USER+2)
	#define TBM_GETTIC              (WM_USER+3)
	#define TBM_SETTIC              (WM_USER+4)
	#define TBM_SETPOS              (WM_USER+5)
	#define TBM_SETRANGE            (WM_USER+6)
	#define TBM_SETRANGEMIN         (WM_USER+7)
	#define TBM_SETRANGEMAX         (WM_USER+8)
	#define TBM_CLEARTICS           (WM_USER+9)
	#define TBM_SETSEL              (WM_USER+10)
	#define TBM_SETSELSTART         (WM_USER+11)
	#define TBM_SETSELEND           (WM_USER+12)
	#define TBM_GETPTICS            (WM_USER+14)
	#define TBM_GETTICPOS           (WM_USER+15)
	#define TBM_GETNUMTICS          (WM_USER+16)
	#define TBM_GETSELSTART         (WM_USER+17)
	#define TBM_GETSELEND           (WM_USER+18)
	#define TBM_CLEARSEL            (WM_USER+19)
	#define TBM_SETTICFREQ          (WM_USER+20)
	#define TBM_SETPAGESIZE         (WM_USER+21)
	#define TBM_GETPAGESIZE         (WM_USER+22)
	#define TBM_SETLINESIZE         (WM_USER+23)
	#define TBM_GETLINESIZE         (WM_USER+24)
	#define TBM_GETTHUMBRECT        (WM_USER+25)
	#define TBM_GETCHANNELRECT      (WM_USER+26)
	#define TBM_SETTHUMBLENGTH      (WM_USER+27)
	#define TBM_GETTHUMBLENGTH      (WM_USER+28)
	#define TBM_SETTOOLTIPS         (WM_USER+29)
	#define TBM_GETTOOLTIPS         (WM_USER+30)
	#define TBM_SETTIPSIDE          (WM_USER+31)

	// TrackBar Tip Side flags
	#define TBTS_TOP                0
	#define TBTS_LEFT               1
	#define TBTS_BOTTOM             2
	#define TBTS_RIGHT              3
	#define TBM_SETBUDDY            (WM_USER+32) // wparam = BOOL fLeft; (or right)
	#define TBM_GETBUDDY            (WM_USER+33) // wparam = BOOL fLeft; (or right)
	#define TB_LINEUP               0
	#define TB_LINEDOWN             1
	#define TB_PAGEUP               2
	#define TB_PAGEDOWN             3
	#define TB_THUMBPOSITION        4
	#define TB_THUMBTRACK           5
	#define TB_TOP                  6
	#define TB_BOTTOM               7
	#define TB_ENDTRACK             8	
	#define TBSTYLE_BUTTON          0x0000  // obsolete; use BTNS_BUTTON instead
	#define TBSTYLE_SEP             0x0001  // obsolete; use BTNS_SEP instead
	#define TBSTYLE_CHECK           0x0002  // obsolete; use BTNS_CHECK instead
	#define TBSTYLE_GROUP           0x0004  // obsolete; use BTNS_GROUP instead
	#define TBSTYLE_CHECKGROUP      (TBSTYLE_GROUP | TBSTYLE_CHECK)     // obsolete; use BTNS_CHECKGROUP instead
	#define TBSTYLE_TOOLTIPS        0x0100
	#define TBSTYLE_WRAPABLE        0x0200
	#define TBSTYLE_ALTDRAG         0x0400
	#define TBSTYLE_FLAT            0x0800
	#define TBSTYLE_LIST            0x1000
	#define TBSTYLE_CUSTOMERASE     0x2000
	#define TBSTYLE_REGISTERDROP    0x4000
	#define TBSTYLE_TRANSPARENT     0x8000
	#define TBSTYLE_EX_DRAWDDARROWS 0x00000001
	
	#define TBSTATE_CHECKED         0x01
	#define TBSTATE_PRESSED         0x02
	#define TBSTATE_ENABLED         0x04
	#define TBSTATE_HIDDEN          0x08
	#define TBSTATE_INDETERMINATE   0x10
	#define TBSTATE_WRAP            0x20

	#define TBN_DROPDOWN            (TBN_FIRST - 10)
	#define TBN_GETINFOTIPA         (TBN_FIRST - 18)
	#define TBN_GETINFOTIP			TBN_GETINFOTIPA

	#define TCM_SETCURSEL           (TCM_FIRST + 12)
	#define TCM_DELETEITEM          (TCM_FIRST + 8)

	#define SBARS_SIZEGRIP			0x100

	#define SB_SETTEXT              (WM_USER+1)
	#define SB_SETPARTS             (WM_USER+4)

	typedef struct tagNMHDR{
    	HWND hwndFrom;
    	UINT idFrom;
    	UINT code;
	} NMHDR,*LPNMHDR;

	typedef UINT (CALLBACK *LPFNPSPCALLBACK)(HWND hwnd, UINT uMsg, struct _PROPSHEETPAGE *ppsp);

	typedef HWND HPROPSHEETPAGE;
	typedef int (CALLBACK *PFNPROPSHEETCALLBACK)(HWND, UINT, LPARAM);

	typedef struct _PROPSHEETPAGE {
        DWORD           dwSize;
        DWORD           dwFlags;
        HINSTANCE       hInstance;
        union {
            LPCSTR          pszTemplate;
            LPCDLGTEMPLATE  pResource;
        };
        union {
            HICON       hIcon;
            LPCSTR      pszIcon;
        } DUMMYUNIONNAME2;
        LPCSTR          pszTitle;
        DLGPROC         pfnDlgProc;
        LPARAM          lParam;
        LPFNPSPCALLBACK pfnCallback;
        UINT FAR * pcRefParent;
        LPCSTR pszHeaderTitle;    // this is displayed in the header
        LPCSTR pszHeaderSubTitle; //
	} PROPSHEETPAGE, *LPPROPSHEETPAGE;
	typedef const PROPSHEETPAGE *LPCPROPSHEETPAGE;

	typedef struct _PROPSHEETHEADERA {
        DWORD           dwSize;
        DWORD           dwFlags;
        HWND            hwndParent;
        HINSTANCE       hInstance;
        union {
            HICON       hIcon;
            LPCSTR      pszIcon;
        };
        LPCSTR          pszCaption;
        UINT            nPages;
        union {
            UINT        nStartPage;
            LPCSTR      pStartPage;
        }DUMMYUNIONNAME2;
        union {
            LPCPROPSHEETPAGE ppsp;
            HPROPSHEETPAGE FAR *phpage;
        };
        PFNPROPSHEETCALLBACK pfnCallback;
        union {
            HBITMAP hbmWatermark;
            LPCSTR pszbmWatermark;
        } DUMMYUNIONNAME4;
        HPALETTE hplWatermark;
        union {
            HBITMAP hbmHeader;     // Header  bitmap shares the palette with watermark
            LPCSTR pszbmHeader;
        } DUMMYUNIONNAME5;
	} PROPSHEETHEADER,*LPPROPSHEETHEADER;
	typedef const PROPSHEETHEADER *LPCPROPSHEETHEADER; 

	typedef struct _NM_UPDOWN
	{
    	NMHDR hdr;
    	int iPos;
    	int iDelta;
	} NMUPDOWN, FAR *LPNMUPDOWN;

	typedef struct _TBBUTTON {
    	int iBitmap;
    	int idCommand;
    	BYTE fsState;
    	BYTE fsStyle;
    	BYTE bReserved[2];
    	DWORD dwData;
    	INT iString;
	} TBBUTTON, *PTBBUTTON, *LPTBBUTTON;

	typedef struct tagNMTBGETINFOTIP {
    	NMHDR hdr;
    	LPSTR pszText;
    	int cchTextMax;
    	int iItem;
    	LPARAM lParam;
	} NMTBGETINFOTIP, *LPNMTBGETINFOTIP;

	#define TC_ITEM                 TCITEM

	typedef struct tagTCITEM
	{
    	UINT mask;
    	DWORD dwState;
    	DWORD dwStateMask;
    	LPSTR pszText;
    	int cchTextMax;
    	int iImage;
    	LPARAM lParam;
	} TCITEM, *LPTCITEM;

	#define TCM_GETITEMA            (TCM_FIRST + 5)
	#define TCM_GETITEMW            (TCM_FIRST + 60)
	#define TCM_GETITEM             TCM_GETITEMA

	#define TabCtrl_GetItem(hwnd, iItem, pitem) \
    	(BOOL)::SendMessage((hwnd), TCM_GETITEM, (WPARAM)(int)(iItem), (LPARAM)(TC_ITEM FAR*)(pitem))


	#define TCM_SETITEMA            (TCM_FIRST + 6)
	#define TCM_SETITEMW            (TCM_FIRST + 61)

	#define TCM_SETITEM             TCM_SETITEMA

	#define TabCtrl_SetItem(hwnd, iItem, pitem) \
    	(BOOL)::SendMessage((hwnd), TCM_SETITEM, (WPARAM)(int)(iItem), (LPARAM)(TC_ITEM FAR*)(pitem))

	#define TCM_INSERTITEMA         (TCM_FIRST + 7)
	#define TCM_INSERTITEMW         (TCM_FIRST + 62)
	#define TCM_INSERTITEM          TCM_INSERTITEMA

	#define TabCtrl_InsertItem(hwnd, iItem, pitem)   \
    	(int)::SendMessage((hwnd), TCM_INSERTITEM, (WPARAM)(int)(iItem), (LPARAM)(const TC_ITEM FAR*)(pitem))

	#define TCM_DELETEITEM          (TCM_FIRST + 8)
	#define TabCtrl_DeleteItem(hwnd, i) \
    	(BOOL)::SendMessage((hwnd), TCM_DELETEITEM, (WPARAM)(int)(i), 0L)

	#define TCM_DELETEALLITEMS      (TCM_FIRST + 9)
	#define TabCtrl_DeleteAllItems(hwnd) \
    	(BOOL)::SendMessage((hwnd), TCM_DELETEALLITEMS, 0, 0L)

	#define TCM_GETITEMRECT         (TCM_FIRST + 10)
	#define TabCtrl_GetItemRect(hwnd, i, prc) \
    	(BOOL)::SendMessage((hwnd), TCM_GETITEMRECT, (WPARAM)(int)(i), (LPARAM)(RECT FAR*)(prc))

	#define TCM_GETCURSEL           (TCM_FIRST + 11)
	#define TabCtrl_GetCurSel(hwnd) \
    	(int)::SendMessage((hwnd), TCM_GETCURSEL, 0, 0)

	#define TCM_SETCURSEL           (TCM_FIRST + 12)
	#define TabCtrl_SetCurSel(hwnd, i) \
    (int)::SendMessage((hwnd), TCM_SETCURSEL, (WPARAM)(i), 0)

	#define TCHT_NOWHERE            0x0001
	#define TCHT_ONITEMICON         0x0002
	#define TCHT_ONITEMLABEL        0x0004
	#define TCHT_ONITEM             (TCHT_ONITEMICON | TCHT_ONITEMLABEL)

	#define LPTC_HITTESTINFO        LPTCHITTESTINFO
	#define TC_HITTESTINFO          TCHITTESTINFO

	typedef struct tagTCHITTESTINFO
	{
    	POINT pt;
    	UINT flags;
	} TCHITTESTINFO,* LPTCHITTESTINFO;

	#define TCM_HITTEST             (TCM_FIRST + 13)
	#define TabCtrl_HitTest(hwndTC, pinfo) \
    	(int)::SendMessage((hwndTC), TCM_HITTEST, 0, (LPARAM)(TC_HITTESTINFO FAR*)(pinfo))

	#define TCM_SETITEMEXTRA        (TCM_FIRST + 14)
	#define TabCtrl_SetItemExtra(hwndTC, cb) \
    (BOOL)::SendMessage((hwndTC), TCM_SETITEMEXTRA, (WPARAM)(cb), 0L)

	#define TCM_ADJUSTRECT          (TCM_FIRST + 40)
	#define TabCtrl_AdjustRect(hwnd, bLarger, prc) \
    	(int)::SendMessage(hwnd, TCM_ADJUSTRECT, (WPARAM)(BOOL)(bLarger), (LPARAM)(RECT FAR *)prc)

	#define TCM_SETITEMSIZE         (TCM_FIRST + 41)
	#define TabCtrl_SetItemSize(hwnd, x, y) \
    	(DWORD)::SendMessage((hwnd), TCM_SETITEMSIZE, 0, MAKELPARAM(x,y))

	#define TCM_REMOVEIMAGE         (TCM_FIRST + 42)
	#define TabCtrl_RemoveImage(hwnd, i) \
        (void)::SendMessage((hwnd), TCM_REMOVEIMAGE, i, 0L)

	#define TCM_SETPADDING          (TCM_FIRST + 43)
	#define TabCtrl_SetPadding(hwnd,  cx, cy) \
        (void)::SendMessage((hwnd), TCM_SETPADDING, 0, MAKELPARAM(cx, cy))


	#define TCM_GETROWCOUNT         (TCM_FIRST + 44)
	#define TabCtrl_GetRowCount(hwnd) \
        (int)::SendMessage((hwnd), TCM_GETROWCOUNT, 0, 0L)


	#define TCM_GETTOOLTIPS         (TCM_FIRST + 45)
	#define TabCtrl_GetToolTips(hwnd) \
        (HWND)::SendMessage((hwnd), TCM_GETTOOLTIPS, 0, 0L)

	#define TCM_SETTOOLTIPS         (TCM_FIRST + 46)
	#define TabCtrl_SetToolTips(hwnd, hwndTT) \
        (void)::SendMessage((hwnd), TCM_SETTOOLTIPS, (WPARAM)(hwndTT), 0L)

	#define TCM_GETCURFOCUS         (TCM_FIRST + 47)
	#define TabCtrl_GetCurFocus(hwnd) \
    	(int)::SendMessage((hwnd), TCM_GETCURFOCUS, 0, 0)

	#define TCM_SETCURFOCUS         (TCM_FIRST + 48)
	#define TabCtrl_SetCurFocus(hwnd, i) \
    	::SendMessage((hwnd),TCM_SETCURFOCUS, i, 0)

	#define TCM_SETMINTABWIDTH      (TCM_FIRST + 49)
	#define TabCtrl_SetMinTabWidth(hwnd, x) \
        (int)::SendMessage((hwnd), TCM_SETMINTABWIDTH, 0, x)

	#define TCM_DESELECTALL         (TCM_FIRST + 50)
	#define TabCtrl_DeselectAll(hwnd, fExcludeFocus)\
        (void)::SendMessage((hwnd), TCM_DESELECTALL, fExcludeFocus, 0)

	#define TCM_HIGHLIGHTITEM       (TCM_FIRST + 51)
	#define TabCtrl_HighlightItem(hwnd, i, fHighlight) \
    	(BOOL)::SendMessage((hwnd), TCM_HIGHLIGHTITEM, (WPARAM)(i), (LPARAM)MAKELONG (fHighlight, 0))
	
	#define TCM_SETEXTENDEDSTYLE    (TCM_FIRST + 52)  // optional wParam == mask
	#define TabCtrl_SetExtendedStyle(hwnd, dw)\
        (DWORD)::SendMessage((hwnd), TCM_SETEXTENDEDSTYLE, 0, dw)

	#define TCM_GETEXTENDEDSTYLE    (TCM_FIRST + 53)
	#define TabCtrl_GetExtendedStyle(hwnd)\
        (DWORD)::SendMessage((hwnd), TCM_GETEXTENDEDSTYLE, 0, 0)

	#define TCM_SETUNICODEFORMAT     CCM_SETUNICODEFORMAT
	#define TabCtrl_SetUnicodeFormat(hwnd, fUnicode)  \
    	(BOOL)::SendMessage((hwnd), TCM_SETUNICODEFORMAT, (WPARAM)(fUnicode), 0)

	#define TCM_GETUNICODEFORMAT     CCM_GETUNICODEFORMAT
	#define TabCtrl_GetUnicodeFormat(hwnd)  \
    	(BOOL)::SendMessage((hwnd), TCM_GETUNICODEFORMAT, 0, 0)
	
	#define TCM_GETIMAGELIST        (TCM_FIRST + 2)
	#define TabCtrl_GetImageList(hwnd) \
    	(HIMAGELIST)::SendMessage((hwnd), TCM_GETIMAGELIST, 0, 0L)

	#define TCM_SETIMAGELIST        (TCM_FIRST + 3)
	#define TabCtrl_SetImageList(hwnd, himl) \
    	(HIMAGELIST)::SendMessage((hwnd), TCM_SETIMAGELIST, 0, (LPARAM)(HIMAGELIST)(himl))

	#define TCM_GETITEMCOUNT        (TCM_FIRST + 4)
	#define TabCtrl_GetItemCount(hwnd) \
    	(int)::SendMessage((hwnd), TCM_GETITEMCOUNT, 0, 0L)

	#define TCIF_TEXT               0x0001
	#define TCIF_IMAGE              0x0002
	#define TCIF_RTLREADING         0x0004
	#define TCIF_PARAM              0x0008
	#define TCIF_STATE              0x0010
	#define TCIS_BUTTONPRESSED      0x0001
	#define TCIS_HIGHLIGHTED        0x0002

	#define TCN_SELCHANGE           (TCN_FIRST - 1)
	#define TCN_SELCHANGING         (TCN_FIRST - 2)

	#define ILC_MASK                0x0001
	#define ILC_COLOR               0x0000
	#define ILC_COLORDDB            0x00FE
	#define ILC_COLOR4              0x0004
	#define ILC_COLOR8              0x0008
	#define ILC_COLOR16             0x0010
	#define ILC_COLOR24             0x0018
	#define ILC_COLOR32             0x0020
	#define ILC_PALETTE             0x0800      // (not implemented)

	#define ILD_NORMAL              0x0000
	#define ILD_TRANSPARENT         0x0001
	#define ILD_MASK                0x0010
	#define ILD_IMAGE               0x0020
	#define ILD_ROP                 0x0040
	#define ILD_BLEND25             0x0002
	#define ILD_BLEND50             0x0004
	#define ILD_OVERLAYMASK         0x0F00
	#define INDEXTOOVERLAYMASK(i)   ((i) << 8)
	#define ILD_SELECTED            ILD_BLEND50
	#define ILD_FOCUS               ILD_BLEND25
	#define ILD_BLEND               ILD_BLEND50
	#define CLR_HILIGHT             CLR_DEFAULT
	#define ILCF_MOVE   			(0x00000000)
	#define ILCF_SWAP   			(0x00000001)
	#define CLR_NONE                0xFFFFFFFFL
	#define CLR_DEFAULT             0xFF000000L

	class LImageList : public LList
	{
	public:
		LImageList();
		virtual ~LImageList();
	protected:	
		void DeleteElem(LPVOID ele);
	};

	typedef struct {
		class LImageList *set;
		int cx,cy;
	} IMAGELIST,FAR* HIMAGELIST;

	HIMAGELIST  ImageList_Create(int cx, int cy, UINT flags, int cInitial, int cGrow);
	BOOL ImageList_Destroy(HIMAGELIST himl);
	int ImageList_GetImageCount(HIMAGELIST himl);
	BOOL ImageList_SetImageCount(HIMAGELIST himl, UINT uNewCount);
	int ImageList_Add(HIMAGELIST himl, HBITMAP hbmImage, HBITMAP hbmMask);
	int ImageList_ReplaceIcon(HIMAGELIST himl, int i, HICON hicon);
	COLORREF ImageList_SetBkColor(HIMAGELIST himl, COLORREF clrBk);
	COLORREF ImageList_GetBkColor(HIMAGELIST himl);
	BOOL ImageList_SetOverlayImage(HIMAGELIST himl, int iImage, int iOverlay);
	BOOL ImageList_Draw(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, UINT fStyle);
	BOOL ImageList_Replace(HIMAGELIST himl, int i, HBITMAP hbmImage, HBITMAP hbmMask);
	int ImageList_AddMasked(HIMAGELIST himl, HBITMAP hbmImage, COLORREF crMask);
	BOOL ImageList_DrawEx(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, int dx, int dy, COLORREF rgbBk, COLORREF rgbFg, UINT fStyle);
	BOOL ImageList_Remove(HIMAGELIST himl, int i);
	HICON ImageList_GetIcon(HIMAGELIST himl, int i, UINT flags);
	HIMAGELIST ImageList_LoadImage(HINSTANCE hi, LPCSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags);
	BOOL ImageList_Copy(HIMAGELIST himlDst, int iDst, HIMAGELIST himlSrc, int iSrc, UINT uFlags);
	BOOL ImageList_BeginDrag(HIMAGELIST himlTrack, int iTrack, int dxHotspot, int dyHotspot);
	void ImageList_EndDrag();
	BOOL ImageList_DragEnter(HWND hwndLock, int x, int y);
	BOOL ImageList_DragLeave(HWND hwndLock);
	BOOL ImageList_DragMove(int x, int y);
	BOOL ImageList_SetDragCursorImage(HIMAGELIST himlDrag, int iDrag, int dxHotspot, int dyHotspot);
	BOOL ImageList_DragShowNolock(BOOL fShow);
	HIMAGELIST ImageList_GetDragImage(POINT FAR* ppt,POINT FAR* pptHotspot);

	#define     ImageList_RemoveAll(himl) ImageList_Remove(himl, -1)
	#define     ImageList_ExtractIcon(hi, himl, i) ImageList_GetIcon(himl, i, 0)
	#define     ImageList_LoadBitmap(hi, lpbmp, cx, cGrow, crMask) ImageList_LoadImage(hi, lpbmp, cx, cGrow, crMask, IMAGE_BITMAP, 0)
	#define     ImageList_AddIcon(himl, hicon) ImageList_ReplaceIcon(himl, -1, hicon)

	HPROPSHEETPAGE CreatePropertySheetPage(LPCPROPSHEETPAGE);
	BOOL DestroyPropertySheetPage(HPROPSHEETPAGE);
	int PropertySheet(LPCPROPSHEETHEADER);

	HWND CreateToolBar(DWORD dwStyle);
	HWND CreateStatusBar(DWORD dwStyle);
#endif

#endif

