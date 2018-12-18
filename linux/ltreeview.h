
#ifndef __WIN32__

#ifndef __LTREEVIEWH__
#define __LTREEVIEWH__

typedef GtkTreeIter *HTREEITEM;

#define TVN_FIRST               -400
#define TVN_LAST                -499
#define TV_FIRST                0x1100

#define TVS_HASBUTTONS          0x0001
#define TVS_HASLINES            0x0002
#define TVS_LINESATROOT         0x0004
#define TVS_EDITLABELS          0x0008
#define TVS_DISABLEDRAGDROP     0x0010
#define TVS_SHOWSELALWAYS       0x0020
#define TVS_RTLREADING          0x0040

#define TVS_NOTOOLTIPS          0x0080
#define TVS_CHECKBOXES          0x0100
#define TVS_TRACKSELECT         0x0200
#define TVS_SINGLEEXPAND        0x0400
#define TVS_INFOTIP             0x0800
#define TVS_FULLROWSELECT       0x1000
#define TVS_NOSCROLL            0x2000
#define TVS_NONEVENHEIGHT       0x4000
#define TVS_NOHSCROLL           0x8000  // TVS_NOSCROLL overrides this

#define TVIF_TEXT               0x0001
#define TVIF_IMAGE              0x0002
#define TVIF_PARAM              0x0004
#define TVIF_STATE              0x0008
#define TVIF_HANDLE             0x0010
#define TVIF_SELECTEDIMAGE      0x0020
#define TVIF_CHILDREN           0x0040
#define TVIF_INTEGRAL           0x0080
#define TVIS_SELECTED           0x0002
#define TVIS_CUT                0x0004
#define TVIS_DROPHILITED        0x0008
#define TVIS_BOLD               0x0010
#define TVIS_EXPANDED           0x0020
#define TVIS_EXPANDEDONCE       0x0040
#define TVIS_EXPANDPARTIAL      0x0080
#define TVIS_OVERLAYMASK        0x0F00
#define TVIS_STATEIMAGEMASK     0xF000
#define TVIS_USERMASK           0xF000

#define I_CHILDRENCALLBACK  (-1)

#define LPTV_ITEM               LPTVITEM
#define TV_ITEM                 TVITEM

typedef struct tagTVITEM {
    UINT      mask;
    HTREEITEM hItem;
    UINT      state;
    UINT      stateMask;
    LPSTR     pszText;
    int       cchTextMax;
    int       iImage;
    int       iSelectedImage;
    int       cChildren;
    LPARAM    lParam;
} TVITEM, FAR *LPTVITEM;


typedef struct tagTVITEMEX {
    UINT      mask;
    HTREEITEM hItem;
    UINT      state;
    UINT      stateMask;
    LPSTR     pszText;
    int       cchTextMax;
    int       iImage;
    int       iSelectedImage;
    int       cChildren;
    LPARAM    lParam;
    int       iIntegral;
} TVITEMEX, FAR *LPTVITEMEX;

#define TVI_ROOT                ((HTREEITEM)(ULONG_PTR)-0x10000)
#define TVI_FIRST               ((HTREEITEM)(ULONG_PTR)-0x0FFFF)
#define TVI_LAST                ((HTREEITEM)(ULONG_PTR)-0x0FFFE)
#define TVI_SORT                ((HTREEITEM)(ULONG_PTR)-0x0FFFD)

#define LPTV_INSERTSTRUCT      LPTVINSERTSTRUCT
#define TV_INSERTSTRUCT        TVINSERTSTRUCT
#define TV_INSERTSTRUCT         TVINSERTSTRUCT
#define LPTV_INSERTSTRUCT       LPTVINSERTSTRUCT


typedef struct tagTVINSERTSTRUCT {
    HTREEITEM hParent;
    HTREEITEM hInsertAfter;
    union
    {
        TVITEMEX itemex;
        TV_ITEM  item;
    };
} TVINSERTSTRUCT, FAR *LPTVINSERTSTRUCT;


#define TVM_INSERTITEM         (TV_FIRST + 0)

#define TreeView_InsertItem(hwnd, lpis) \
    (HTREEITEM)::SendMessage((hwnd), TVM_INSERTITEM, 0, (LPARAM)(LPTV_INSERTSTRUCT)(lpis))


#define TVM_DELETEITEM          (TV_FIRST + 1)

#define TreeView_DeleteItem(hwnd, hitem) \
    (BOOL)::SendMessage((hwnd), TVM_DELETEITEM, 0, (LPARAM)(HTREEITEM)(hitem))


#define TreeView_DeleteAllItems(hwnd) \
    (BOOL)::SendMessage((hwnd), TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT)


#define TVM_EXPAND              (TV_FIRST + 2)
#define TreeView_Expand(hwnd, hitem, code) \
    (BOOL)::SendMessage((hwnd), TVM_EXPAND, (WPARAM)(code), (LPARAM)(HTREEITEM)(hitem))


#define TVE_COLLAPSE            0x0001
#define TVE_EXPAND              0x0002
#define TVE_TOGGLE              0x0003
#define TVE_EXPANDPARTIAL       0x4000
#define TVE_COLLAPSERESET       0x8000

#define TVM_GETITEMRECT         (TV_FIRST + 4)
#define TreeView_GetItemRect(hwnd, hitem, prc, code) \
    (*(HTREEITEM FAR *)prc = (hitem), (BOOL)::SendMessage((hwnd), TVM_GETITEMRECT, (WPARAM)(code), (LPARAM)(RECT FAR*)(prc)))


#define TVM_GETCOUNT            (TV_FIRST + 5)
#define TreeView_GetCount(hwnd) \
    (UINT)::SendMessage((hwnd), TVM_GETCOUNT, 0, 0)


#define TVM_GETINDENT           (TV_FIRST + 6)
#define TreeView_GetIndent(hwnd) \
    (UINT)::SendMessage((hwnd), TVM_GETINDENT, 0, 0)


#define TVM_SETINDENT           (TV_FIRST + 7)
#define TreeView_SetIndent(hwnd, indent) \
    (BOOL)::SendMessage((hwnd), TVM_SETINDENT, (WPARAM)(indent), 0)


#define TVM_GETIMAGELIST        (TV_FIRST + 8)
#define TreeView_GetImageList(hwnd, iImage) \
    (HIMAGELIST)::SendMessage((hwnd), TVM_GETIMAGELIST, iImage, 0)


#define TVSIL_NORMAL            0
#define TVSIL_STATE             2

#define TVM_SETIMAGELIST        (TV_FIRST + 9)

#define TreeView_SetImageList(hwnd, himl, iImage) \
    (HIMAGELIST)::SendMessage((hwnd), TVM_SETIMAGELIST, iImage, (LPARAM)(HIMAGELIST)(himl))

#define TVM_GETNEXTITEM         (TV_FIRST + 10)
#define TreeView_GetNextItem(hwnd, hitem, code) \
    (HTREEITEM)::SendMessage((hwnd), TVM_GETNEXTITEM, (WPARAM)(code), (LPARAM)(HTREEITEM)(hitem))


#define TVGN_ROOT               0x0000
#define TVGN_NEXT               0x0001
#define TVGN_PREVIOUS           0x0002
#define TVGN_PARENT             0x0003
#define TVGN_CHILD              0x0004
#define TVGN_FIRSTVISIBLE       0x0005
#define TVGN_NEXTVISIBLE        0x0006
#define TVGN_PREVIOUSVISIBLE    0x0007
#define TVGN_DROPHILITE         0x0008
#define TVGN_CARET              0x0009
#define TVGN_LASTVISIBLE        0x000A

#define TreeView_GetChild(hwnd, hitem)          TreeView_GetNextItem(hwnd, hitem, TVGN_CHILD)
#define TreeView_GetNextSibling(hwnd, hitem)    TreeView_GetNextItem(hwnd, hitem, TVGN_NEXT)
#define TreeView_GetPrevSibling(hwnd, hitem)    TreeView_GetNextItem(hwnd, hitem, TVGN_PREVIOUS)
#define TreeView_GetParent(hwnd, hitem)         TreeView_GetNextItem(hwnd, hitem, TVGN_PARENT)
#define TreeView_GetFirstVisible(hwnd)          TreeView_GetNextItem(hwnd, NULL,  TVGN_FIRSTVISIBLE)
#define TreeView_GetNextVisible(hwnd, hitem)    TreeView_GetNextItem(hwnd, hitem, TVGN_NEXTVISIBLE)
#define TreeView_GetPrevVisible(hwnd, hitem)    TreeView_GetNextItem(hwnd, hitem, TVGN_PREVIOUSVISIBLE)
#define TreeView_GetSelection(hwnd)             TreeView_GetNextItem(hwnd, NULL,  TVGN_CARET)
#define TreeView_GetDropHilight(hwnd)           TreeView_GetNextItem(hwnd, NULL,  TVGN_DROPHILITE)
#define TreeView_GetRoot(hwnd)                  TreeView_GetNextItem(hwnd, NULL,  TVGN_ROOT)
#define TreeView_GetLastVisible(hwnd)           TreeView_GetNextItem(hwnd, NULL,  TVGN_LASTVISIBLE)


#define TVM_SELECTITEM          (TV_FIRST + 11)
#define TreeView_Select(hwnd, hitem, code) \
    (BOOL)::SendMessage((hwnd), TVM_SELECTITEM, (WPARAM)(code), (LPARAM)(HTREEITEM)(hitem))


#define TreeView_SelectItem(hwnd, hitem)            TreeView_Select(hwnd, hitem, TVGN_CARET)
#define TreeView_SelectDropTarget(hwnd, hitem)      TreeView_Select(hwnd, hitem, TVGN_DROPHILITE)
#define TreeView_SelectSetFirstVisible(hwnd, hitem) TreeView_Select(hwnd, hitem, TVGN_FIRSTVISIBLE)

#define TVM_GETITEM            (TV_FIRST + 12)

#define TreeView_GetItem(hwnd, pitem) \
    (BOOL)::SendMessage((hwnd), TVM_GETITEM, 0, (LPARAM)(TV_ITEM FAR*)(pitem))


#define TVM_SETITEM            (TV_FIRST + 13)
#define TreeView_SetItem(hwnd, pitem) \
    (BOOL)::SendMessage((hwnd), TVM_SETITEM, 0, (LPARAM)(const TV_ITEM FAR*)(pitem))


#define TVM_EDITLABELA          (TV_FIRST + 14)

#define TreeView_EditLabel(hwnd, hitem) \
    (HWND)::SendMessage((hwnd), TVM_EDITLABEL, 0, (LPARAM)(HTREEITEM)(hitem))


#define TVM_GETEDITCONTROL      (TV_FIRST + 15)
#define TreeView_GetEditControl(hwnd) \
    (HWND)::SendMessage((hwnd), TVM_GETEDITCONTROL, 0, 0)


#define TVM_GETVISIBLECOUNT     (TV_FIRST + 16)
#define TreeView_GetVisibleCount(hwnd) \
    (UINT)::SendMessage((hwnd), TVM_GETVISIBLECOUNT, 0, 0)


#define TVM_HITTEST             (TV_FIRST + 17)
#define TreeView_HitTest(hwnd, lpht) \
    (HTREEITEM)::SendMessage((hwnd), TVM_HITTEST, 0, (LPARAM)(LPTV_HITTESTINFO)(lpht))


#define LPTV_HITTESTINFO   LPTVHITTESTINFO
#define TV_HITTESTINFO     TVHITTESTINFO

typedef struct tagTVHITTESTINFO {
    POINT       pt;
    UINT        flags;
    HTREEITEM   hItem;
} TVHITTESTINFO, FAR *LPTVHITTESTINFO;

#define TVHT_NOWHERE            0x0001
#define TVHT_ONITEMICON         0x0002
#define TVHT_ONITEMLABEL        0x0004
#define TVHT_ONITEM             (TVHT_ONITEMICON | TVHT_ONITEMLABEL | TVHT_ONITEMSTATEICON)
#define TVHT_ONITEMINDENT       0x0008
#define TVHT_ONITEMBUTTON       0x0010
#define TVHT_ONITEMRIGHT        0x0020
#define TVHT_ONITEMSTATEICON    0x0040

#define TVHT_ABOVE              0x0100
#define TVHT_BELOW              0x0200
#define TVHT_TORIGHT            0x0400
#define TVHT_TOLEFT             0x0800


#define TVM_CREATEDRAGIMAGE     (TV_FIRST + 18)
#define TreeView_CreateDragImage(hwnd, hitem) \
    (HIMAGELIST)::SendMessage((hwnd), TVM_CREATEDRAGIMAGE, 0, (LPARAM)(HTREEITEM)(hitem))

#define TVM_SORTCHILDREN        (TV_FIRST + 19)
#define TreeView_SortChildren(hwnd, hitem, recurse) \
    (BOOL)::SendMessage((hwnd), TVM_SORTCHILDREN, (WPARAM)(recurse), (LPARAM)(HTREEITEM)(hitem))


#define TVM_ENSUREVISIBLE       (TV_FIRST + 20)
#define TreeView_EnsureVisible(hwnd, hitem) \
    (BOOL)::SendMessage((hwnd), TVM_ENSUREVISIBLE, 0, (LPARAM)(HTREEITEM)(hitem))


#define TVM_SORTCHILDRENCB      (TV_FIRST + 21)
#define TreeView_SortChildrenCB(hwnd, psort, recurse) \
    (BOOL)::SendMessage((hwnd), TVM_SORTCHILDRENCB, (WPARAM)(recurse), \
    (LPARAM)(LPTV_SORTCB)(psort))


#define TVM_ENDEDITLABELNOW     (TV_FIRST + 22)
#define TreeView_EndEditLabelNow(hwnd, fCancel) \
    (BOOL)::SendMessage((hwnd), TVM_ENDEDITLABELNOW, (WPARAM)(fCancel), 0)


#define TVM_GETISEARCHSTRINGA   (TV_FIRST + 23)

#define TVM_SETTOOLTIPS         (TV_FIRST + 24)
#define TreeView_SetToolTips(hwnd,  hwndTT) \
    (HWND)::SendMessage((hwnd), TVM_SETTOOLTIPS, (WPARAM)(hwndTT), 0)

#define TVM_GETTOOLTIPS         (TV_FIRST + 25)
#define TreeView_GetToolTips(hwnd) \
    (HWND)::SendMessage((hwnd), TVM_GETTOOLTIPS, 0, 0)

#define TreeView_GetISearchString(hwndTV, lpsz) \
        (BOOL)::SendMessage((hwndTV), TVM_GETISEARCHSTRING, 0, (LPARAM)(LPTSTR)(lpsz))

#define TVM_SETINSERTMARK       (TV_FIRST + 26)
#define TreeView_SetInsertMark(hwnd, hItem, fAfter) \
        (BOOL)::SendMessage((hwnd), TVM_SETINSERTMARK, (WPARAM) (fAfter), (LPARAM) (hItem))

#define TVM_SETUNICODEFORMAT     CCM_SETUNICODEFORMAT
#define TreeView_SetUnicodeFormat(hwnd, fUnicode)  \
    (BOOL)::SendMessage((hwnd), TVM_SETUNICODEFORMAT, (WPARAM)(fUnicode), 0)

#define TVM_GETUNICODEFORMAT     CCM_GETUNICODEFORMAT
#define TreeView_GetUnicodeFormat(hwnd)  \
    (BOOL)::SendMessage((hwnd), TVM_GETUNICODEFORMAT, 0, 0)


#define TVM_SETITEMHEIGHT         (TV_FIRST + 27)
#define TreeView_SetItemHeight(hwnd,  iHeight) \
    (int)::SendMessage((hwnd), TVM_SETITEMHEIGHT, (WPARAM)(iHeight), 0)

#define TVM_GETITEMHEIGHT         (TV_FIRST + 28)
#define TreeView_GetItemHeight(hwnd) \
    (int)::SendMessage((hwnd), TVM_GETITEMHEIGHT, 0, 0)

#define TVM_SETBKCOLOR              (TV_FIRST + 29)
#define TreeView_SetBkColor(hwnd, clr) \
    (COLORREF)::SendMessage((hwnd), TVM_SETBKCOLOR, 0, (LPARAM)(clr))

#define TVM_SETTEXTCOLOR              (TV_FIRST + 30)
#define TreeView_SetTextColor(hwnd, clr) \
    (COLORREF)::SendMessage((hwnd), TVM_SETTEXTCOLOR, 0, (LPARAM)(clr))

#define TVM_GETBKCOLOR              (TV_FIRST + 31)
#define TreeView_GetBkColor(hwnd) \
    (COLORREF)::SendMessage((hwnd), TVM_GETBKCOLOR, 0, 0)

#define TVM_GETTEXTCOLOR              (TV_FIRST + 32)
#define TreeView_GetTextColor(hwnd) \
    (COLORREF)::SendMessage((hwnd), TVM_GETTEXTCOLOR, 0, 0)

#define TVM_SETSCROLLTIME              (TV_FIRST + 33)
#define TreeView_SetScrollTime(hwnd, uTime) \
    (UINT)::SendMessage((hwnd), TVM_SETSCROLLTIME, uTime, 0)

#define TVM_GETSCROLLTIME              (TV_FIRST + 34)
#define TreeView_GetScrollTime(hwnd) \
    (UINT)::SendMessage((hwnd), TVM_GETSCROLLTIME, 0, 0)

#define TVM_SETINSERTMARKCOLOR              (TV_FIRST + 37)
#define TreeView_SetInsertMarkColor(hwnd, clr) \
    (COLORREF)SendMessage((hwnd), TVM_SETINSERTMARKCOLOR, 0, (LPARAM)(clr))

#define TVM_GETINSERTMARKCOLOR              (TV_FIRST + 38)
#define TreeView_GetInsertMarkColor(hwnd) \
    (COLORREF)SendMessage((hwnd), TVM_GETINSERTMARKCOLOR, 0, 0)


#define TreeView_SetItemState(hwndTV, hti, data, _mask) \
{ \
	TVITEM _ms_TVi;\
  	_ms_TVi.mask = TVIF_STATE; \
  	_ms_TVi.hItem = hti; \
  	_ms_TVi.stateMask = _mask;\
  	_ms_TVi.state = data;\
  	SendMessage((hwndTV), TVM_SETITEM, 0, (LPARAM)(TV_ITEM FAR *)&_ms_TVi);\
}

#define TreeView_SetCheckState(hwndTV, hti, fCheck) \
  TreeView_SetItemState(hwndTV, hti, INDEXTOSTATEIMAGEMASK((fCheck)?2:1), TVIS_STATEIMAGEMASK)

#define TVM_GETITEMSTATE        (TV_FIRST + 39)
#define TreeView_GetItemState(hwndTV, hti, mask) \
   (UINT)SendMessage((hwndTV), TVM_GETITEMSTATE, (WPARAM)(hti), (LPARAM)(mask))

#define TreeView_GetCheckState(hwndTV, hti) \
   ((((UINT)(SendMessage((hwndTV), TVM_GETITEMSTATE, (WPARAM)(hti), TVIS_STATEIMAGEMASK))) >> 12) -1)

#define TVM_SETLINECOLOR            (TV_FIRST + 40)
#define TreeView_SetLineColor(hwnd, clr) \
	(COLORREF)SendMessage((hwnd), TVM_SETLINECOLOR, 0, (LPARAM)(clr))

#define TVM_GETLINECOLOR            (TV_FIRST + 41)
#define TreeView_GetLineColor(hwnd) \
    (COLORREF)SendMessage((hwnd), TVM_GETLINECOLOR, 0, 0)

typedef int (CALLBACK *PFNTVCOMPARE)(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

typedef struct tagTVSORTCB
{
    HTREEITEM       hParent;
    PFNTVCOMPARE    lpfnCompare;
	LPARAM          lParam;
} TVSORTCB, FAR *LPTVSORTCB;


#define LPNM_TREEVIEW           LPNMTREEVIEW
#define NM_TREEVIEW             NMTREEVIEW

typedef struct tagNMTREEVIEW {
    NMHDR       hdr;
    UINT        action;
    TVITEM    itemOld;
    TVITEM    itemNew;
    POINT       ptDrag;
} NMTREEVIEW, FAR *LPNMTREEVIEW;

#define TVN_SELCHANGING        (TVN_FIRST-1)
#define TVN_SELCHANGED         (TVN_FIRST-2)

#define TVC_UNKNOWN             0x0000
#define TVC_BYMOUSE             0x0001
#define TVC_BYKEYBOARD          0x0002

#define TVN_GETDISPINFO        TVN_FIRST-3
#define TVN_SETDISPINFO        TVN_FIRST-4

#define TVIF_DI_SETITEM         0x1000
#define TV_DISPINFO             NMTVDISPINFO

typedef struct tagTVDISPINFOA {
    NMHDR hdr;
    TVITEM item;
} NMTVDISPINFO, FAR *LPNMTVDISPINFO;

#define TVN_ITEMEXPANDING      TVN_FIRST-5
#define TVN_ITEMEXPANDED       TVN_FIRST-6
#define TVN_BEGINDRAG          TVN_FIRST-7
#define TVN_BEGINRDRAG         TVN_FIRST-8
#define TVN_DELETEITEM         TVN_FIRST-9
#define TVN_BEGINLABELEDIT     TVN_FIRST-10
#define TVN_ENDLABELEDIT       TVN_FIRST-11
#define TVN_KEYDOWN            TVN_FIRST-12

#define TVN_GETINFOTIP         TVN_FIRST-13
#define TVN_SINGLEEXPAND       TVN_FIRST-15

#define TVNRET_DEFAULT          0
#define TVNRET_SKIPOLD          1
#define TVNRET_SKIPNEW          2

typedef struct tagTVKEYDOWN {
    NMHDR hdr;
    WORD wVKey;
    UINT flags;
} NMTVKEYDOWN, FAR *LPNMTVKEYDOWN;

typedef struct tagNMTVGETINFOTIP
{
    NMHDR hdr;
    LPSTR pszText;
    int cchTextMax;
    HTREEITEM hItem;
    LPARAM lParam;
} NMTVGETINFOTIP, *LPNMTVGETINFOTIP;

#define TVCDRF_NOIMAGES         0x00010000


#define TTDT_AUTOMATIC          0
#define TTDT_RESHOW             1
#define TTDT_AUTOPOP            2
#define TTDT_INITIAL            3

// Tool Tip Messages
#define TTM_ACTIVATE            (WM_USER + 1)
#define TTM_SETDELAYTIME        (WM_USER + 3)
#define TTM_ADDTOOL            (WM_USER + 4)
#define TTM_DELTOOL            (WM_USER + 5)
#define TTM_NEWTOOLRECT        (WM_USER + 6)
#define TTM_RELAYEVENT          (WM_USER + 7)
#define TTM_GETTOOLINFO        (WM_USER + 8)

#define TTM_SETTOOLINFO        (WM_USER + 9)

#define TTM_HITTESTA            (WM_USER +10)
#define TTM_GETTEXTA            (WM_USER +11)
#define TTM_UPDATETIPTEXT      (WM_USER +12)
#define TTM_GETTOOLCOUNT        (WM_USER +13)
#define TTM_ENUMTOOLS          (WM_USER +14)
#define TTM_GETCURRENTTOOL     (WM_USER + 15)
#define TTM_WINDOWFROMPOINT     (WM_USER + 16)
#define TTM_TRACKACTIVATE       (WM_USER + 17)  // wParam = TRUE/FALSE start end  lparam = LPTOOLINFO
#define TTM_TRACKPOSITION       (WM_USER + 18)  // lParam = dwPos
#define TTM_SETTIPBKCOLOR       (WM_USER + 19)
#define TTM_SETTIPTEXTCOLOR     (WM_USER + 20)
#define TTM_GETDELAYTIME        (WM_USER + 21)
#define TTM_GETTIPBKCOLOR       (WM_USER + 22)
#define TTM_GETTIPTEXTCOLOR     (WM_USER + 23)
#define TTM_SETMAXTIPWIDTH      (WM_USER + 24)
#define TTM_GETMAXTIPWIDTH      (WM_USER + 25)
#define TTM_SETMARGIN           (WM_USER + 26)  // lParam = lprc
#define TTM_GETMARGIN           (WM_USER + 27)  // lParam = lprc
#define TTM_POP                 (WM_USER + 28)
#define TTM_UPDATE              (WM_USER + 29)
#define TTM_GETBUBBLESIZE       (WM_USER + 30)
#define TTM_ADJUSTRECT          (WM_USER + 31)
#define TTM_SETTITLE           (WM_USER + 32)  // wParam = TTI_*, lParam = char* szTitle


HWND CreateTreeView(DWORD dwStyle);

#endif

#endif

