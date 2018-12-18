#include "ideastypes.h"
#include "lcommctrl.h"
#ifndef __WIN32__
#include <cairo/cairo-xlib.h>


//---------------------------------------------------------------------------
static void OnEnumChild(GtkWidget *widget,gpointer data)
{
   if(((LPDWORD)data)[1] != 0)
       return;
	if(GTK_IS_CONTAINER(widget))
		gtk_container_foreach((GtkContainer *)widget,OnEnumChild,data);
	if(GetWindowLong(widget,GWL_ID) == ((LPDWORD)data)[0])
		((LPDWORD)data)[1] = (DWORD)widget;
}
//--------------------------------------------------------------------------------
LImageList::LImageList() : LList()
{
}
//--------------------------------------------------------------------------------
LImageList::~LImageList()
{
	Clear();
}
//--------------------------------------------------------------------------------
void LImageList::DeleteElem(LPVOID ele)
{
	if(ele == NULL)
		return;
	DeleteObject((HBITMAP)ele);
}
//--------------------------------------------------------------------------------
HIMAGELIST ImageList_Create(int cx, int cy, UINT flags, int cInitial, int cGrow)
{
	HIMAGELIST img;
	gint x,y,i,i1;
	char s[300];

	img = new IMAGELIST[1];
	if(img == NULL)
		return NULL;
	img->set = new LImageList();
	if(img->set == NULL){
		delete img;
		return NULL;
	}
	img->cx = cx;
	img->cy = cy;
	return img;
}
//--------------------------------------------------------------------------------
BOOL ImageList_Destroy(HIMAGELIST himl)
{
	if(himl == NULL)
		return FALSE;
	if(himl->set)
		delete himl->set;
	delete himl;
	return TRUE;
}
//--------------------------------------------------------------------------------
int ImageList_GetImageCount(HIMAGELIST himl)
{
	return 0;
}
//--------------------------------------------------------------------------------
BOOL ImageList_SetImageCount(HIMAGELIST himl, UINT uNewCount)
{
	return FALSE;
}
//--------------------------------------------------------------------------------
int ImageList_Add(HIMAGELIST himl, HBITMAP hbmImage, HBITMAP hbmMask)
{
	return 0;
}
//--------------------------------------------------------------------------------
int ImageList_ReplaceIcon(HIMAGELIST himl, int i, HICON hicon)
{
	if(himl == NULL || hicon == NULL)
		return 0;
	if(himl->set == NULL){
		himl->set = new LImageList();
		if(himl->set == NULL)
			return 0;
	}
	himl->set->Add((LPVOID)hicon);
	return 1;
}
//--------------------------------------------------------------------------------
COLORREF ImageList_SetBkColor(HIMAGELIST himl, COLORREF clrBk)
{
	return 0;
}
//--------------------------------------------------------------------------------
COLORREF ImageList_GetBkColor(HIMAGELIST himl)
{
	return 0;
}
//--------------------------------------------------------------------------------
BOOL ImageList_SetOverlayImage(HIMAGELIST himl, int iImage, int iOverlay)
{
	return FALSE;
}
//--------------------------------------------------------------------------------
BOOL ImageList_DrawEx(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, int dx, int dy, COLORREF rgbBk, COLORREF rgbFg, UINT fStyle)
{
	HBITMAP bit,bitBlend;
	GdkDrawable *dest;
	int width,height,x1,y1,i1;
	guchar *p;
	cairo_surface_t *surface;
	Drawable xid;

	if(himl == NULL || himl->set == NULL || hdcDst == NULL)
		return FALSE;
	surface = cairo_get_group_target(hdcDst);
	if(surface == NULL)
		return FALSE;
	xid = cairo_xlib_surface_get_drawable(surface);
	dest = (GdkDrawable *)gdk_xid_table_lookup(xid);
	if(dest == NULL)
		return FALSE;
	bit = (HBITMAP)himl->set->GetItem(i+1);
	if(bit == NULL)
		return FALSE;
	if((fStyle & (ILD_BLEND25|ILD_BLEND50)) != 0){
		width = gdk_pixbuf_get_width(bit);
		height = gdk_pixbuf_get_height(bit);
		bitBlend = gdk_pixbuf_new(GDK_COLORSPACE_RGB,true,8,width,height);
		if(bitBlend == NULL)
			return FALSE;
		gdk_pixbuf_copy_area(bit,0,0,width,height,bitBlend,0,0);
		p = gdk_pixbuf_get_pixels(bitBlend);
		if(fStyle & ILD_BLEND25)
			i1 = 25;
		else
			i1 = 50;
		for(y1=0;y1<height;y1++){
			for(x1=0;x1<width;x1++,p+=4){
				if(p[3] == 0)
					continue;
				p[3] = 32 + ((p[3] * i1) / 100);
			}
		}
		gdk_draw_pixbuf(dest,NULL,bitBlend,0,0,x,y,dx,dy,GDK_RGB_DITHER_NONE,0,0);
		DeleteObject(bitBlend);
	}
	else
		gdk_draw_pixbuf(dest,NULL,bit,0,0,x,y,dx,dy,GDK_RGB_DITHER_NONE,0,0);
	return TRUE;
}
//--------------------------------------------------------------------------------
BOOL ImageList_Draw(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, UINT fStyle)
{
	return ImageList_DrawEx(himl,i,hdcDst,x,y,-1,-1,0,0,fStyle);
}
//--------------------------------------------------------------------------------
BOOL ImageList_Replace(HIMAGELIST himl, int i, HBITMAP hbmImage, HBITMAP hbmMask)
{
	return FALSE;
}
//--------------------------------------------------------------------------------
int ImageList_AddMasked(HIMAGELIST himl,HBITMAP hbmImage,COLORREF crMask)
{
	HBITMAP bit,bitMask;
	int width,height,cx,cy,i;

	if(himl == NULL || hbmImage == NULL)
		return 0;
	if(himl->set == NULL){
		himl->set = new LImageList();
		if(himl->set == NULL)
			return 0;
	}
	width = gdk_pixbuf_get_width (hbmImage);
	height = gdk_pixbuf_get_height(hbmImage);
	cx = i = 0;
	while(cx < width){
		bit = gdk_pixbuf_new_subpixbuf(hbmImage,cx,0,himl->cx,himl->cy);
		bitMask = gdk_pixbuf_add_alpha(bit,true,(crMask>>16)&0xFF,(crMask>>8) & 0xFF,(crMask & 0xFF));
		himl->set->Add((LPVOID)bitMask);
		if(bit != bitMask)
			DeleteObject(bit);
		cx += himl->cx;
		i++;
	}
	return i;
}
//--------------------------------------------------------------------------------
BOOL ImageList_Remove(HIMAGELIST himl, int i)
{
	return FALSE;
}
//--------------------------------------------------------------------------------
HICON ImageList_GetIcon(HIMAGELIST himl, int i, UINT flags)
{
	return NULL;
}
//--------------------------------------------------------------------------------
HIMAGELIST ImageList_LoadImage(HINSTANCE hi, LPCSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags)
{
	HBITMAP bit;
	HIMAGELIST himl;

	bit = LoadBitmap(hi,lpbmp);
	if(bit == NULL)
		return NULL;
	himl = ImageList_Create(cx,gdk_pixbuf_get_height((GdkPixbuf *)bit),uFlags,1,cGrow);
	if(himl != NULL){
		ImageList_AddMasked(himl,bit,crMask);
	}
	DeleteObject(bit);
	return himl;
}
//--------------------------------------------------------------------------------
BOOL ImageList_Copy(HIMAGELIST himlDst, int iDst, HIMAGELIST himlSrc, int iSrc, UINT uFlags)
{
	return FALSE;
}
//--------------------------------------------------------------------------------
BOOL ImageList_BeginDrag(HIMAGELIST himlTrack, int iTrack, int dxHotspot, int dyHotspot)
{
	return FALSE;
}
//--------------------------------------------------------------------------------
void ImageList_EndDrag()
{
}
//--------------------------------------------------------------------------------
BOOL ImageList_DragEnter(HWND hwndLock, int x, int y)
{
	return FALSE;
}
//--------------------------------------------------------------------------------
BOOL ImageList_DragLeave(HWND hwndLock)
{
	return FALSE;
}
//--------------------------------------------------------------------------------
BOOL ImageList_DragMove(int x, int y)
{
	return FALSE;
}
//--------------------------------------------------------------------------------
BOOL ImageList_SetDragCursorImage(HIMAGELIST himlDrag, int iDrag, int dxHotspot, int dyHotspot)
{
	return FALSE;
}
//--------------------------------------------------------------------------------
BOOL ImageList_DragShowNolock(BOOL fShow)
{
	return FALSE;
}
//--------------------------------------------------------------------------------
HIMAGELIST ImageList_GetDragImage(POINT FAR* ppt,POINT FAR* pptHotspot)
{
	return NULL;
}
//--------------------------------------------------------------------------------
static void on_toolbutton_clicked(GtkToolButton *toolbutton,gpointer user_data)
{
	NMHDR hdr;
	HWND hwnd;

	if(GTK_IS_MENU_TOOL_BUTTON(toolbutton)){
		hdr.code = TBN_DROPDOWN;
		hwnd = (HWND)GetWindowLong((HWND)toolbutton,GWL_NOTIFYPARENT);
		hdr.hwndFrom = hwnd;
		hdr.idFrom = GetWindowLong(hwnd,GWL_ID);
		SendMessage((HWND)GetWindowLong(hwnd,GWL_NOTIFYPARENT),WM_NOTIFY,(WPARAM)hdr.idFrom,(LPARAM)&hdr);
	}
	else
		SendMessage((HWND)GetWindowLong((HWND)GetWindowLong((HWND)toolbutton,GWL_NOTIFYPARENT),GWL_NOTIFYPARENT),WM_COMMAND,(WPARAM)GetWindowLong((HWND)toolbutton,GWL_ID),0);
}
//--------------------------------------------------------------------------------
static gboolean OnQueryToolTip(GtkWidget *widget,gint x,gint y,gboolean keyboard_mode,GtkTooltip *tooltip,gpointer user_data)
{
	NMTBGETINFOTIP hdr={0};
	int index,count,x1;
	HWND hwnd;
	GList *pList;
	GtkRequisition sz;
	GtkToolItem *item;

	count = gtk_toolbar_get_n_items(GTK_TOOLBAR(widget));
	hwnd = NULL;
	for(x1=index = 0;index<count;index++){
		item = gtk_toolbar_get_nth_item(GTK_TOOLBAR(widget),index);
		gtk_widget_size_request((HWND)item,&sz);
		x1 += sz.width;
		if(x1 > x){
			hwnd = (HWND)item;
			break;
		}
	}
	if(hwnd == NULL || GTK_IS_SEPARATOR_TOOL_ITEM(hwnd) || !GTK_WIDGET_VISIBLE(hwnd))
		return FALSE;
	hdr.hdr.hwndFrom = widget;
	hdr.hdr.idFrom = GetWindowLong(widget,GWL_ID);
	hdr.hdr.code = TBN_GETINFOTIPA;
	hdr.pszText = (LPSTR)LocalAlloc(LPTR,5000);
	hdr.cchTextMax = 5000;
	hdr.iItem = GetWindowLong(hwnd,GWL_ID);
	SendMessage((HWND)GetWindowLong(widget,GWL_NOTIFYPARENT),WM_NOTIFY,hdr.hdr.idFrom,(LPARAM)&hdr);
	gtk_tooltip_set_text(tooltip,hdr.pszText);
	LocalFree(hdr.pszText);
	return TRUE;
}
//--------------------------------------------------------------------------------
static LRESULT WndProcToolBar(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	DWORD dw[2];
	int i;
	LPTBBUTTON p;
	GtkToolItem *btn;
	HIMAGELIST himl;
	HBITMAP bit;
	GtkImage *icon;

	switch(uMsg){
		case TB_SETIMAGELIST:
			SetWindowLong(hwnd,GWL_IMAGELIST,(LONG)lParam);
		break;
		case TB_BUTTONSTRUCTSIZE:
		break;
		case TB_ADDBUTTONS:
			if((p = (LPTBBUTTON)lParam) == NULL)
				return 0;
			for(i=0;i<wParam;i++){
				if(p[i].fsStyle == TBSTYLE_BUTTON || p[i].fsStyle == TBSTYLE_CHECK){
					if(p[i].fsStyle == TBSTYLE_CHECK){
						btn = gtk_toggle_tool_button_new();
						if(p[i].fsState & TBSTATE_CHECKED)
							gtk_toggle_tool_button_set_active((GtkToggleToolButton*)btn,true);
					}
					else
						btn = gtk_tool_button_new(NULL,"");
					if(p[i].iBitmap != -1 && (himl = (HIMAGELIST)GetWindowLong(hwnd,GWL_IMAGELIST)) != 0){
						if(himl->set != NULL){
							if((bit = (HBITMAP)himl->set->GetItem(p[i].iBitmap+1)) != NULL){
								icon = (GtkImage *)gtk_image_new_from_pixbuf(bit);
								gtk_tool_button_set_icon_widget((GtkToolButton *)btn,(HWND)icon);
							}
						}
					}
					SetWindowLong((HWND)btn,GWL_ID,p[i].idCommand);
					SetWindowLong((HWND)btn,GWL_NOTIFYPARENT,(LONG)hwnd);
					SetWindowLong((HWND)btn,GWL_SIGNAL,(LONG)g_signal_connect((gpointer)btn, "clicked",G_CALLBACK (on_toolbutton_clicked),NULL));
				}
				else if(p[i].fsStyle == TBSTYLE_SEP){
					btn = gtk_separator_tool_item_new ();
				}
				else if(p[i].fsStyle == 0x80){
					btn = gtk_menu_tool_button_new(NULL,"");
					if(p[i].iBitmap != -1 && (himl = (HIMAGELIST)GetWindowLong(hwnd,GWL_IMAGELIST)) != 0){
						if(himl->set != NULL){
							if((bit = (HBITMAP)himl->set->GetItem(p[i].iBitmap+1)) != NULL){
								icon = (GtkImage *)gtk_image_new_from_pixbuf(bit);
								gtk_tool_button_set_icon_widget((GtkToolButton *)btn,(HWND)icon);
							}
						}
					}
					SetWindowLong((HWND)btn,GWL_ID,p[i].idCommand);
					SetWindowLong((HWND)btn,GWL_NOTIFYPARENT,(LONG)hwnd);
					SetWindowLong((HWND)btn,GWL_SIGNAL,(LONG)g_signal_connect((gpointer)btn, "clicked",G_CALLBACK(on_toolbutton_clicked),NULL));
				}
				else
					btn = NULL;
				if(btn != NULL){
					ShowWindow((HWND)btn,SW_SHOW);
					EnableWindow((HWND)btn,((p[i].fsState & TBSTATE_ENABLED) ? TRUE : FALSE));
					gtk_toolbar_insert(GTK_TOOLBAR (hwnd),btn,-1);
				}
			}
		break;
		case TB_SETDISABLEDIMAGELIST:
		break;
		case TB_ENABLEBUTTON:
			dw[0] = wParam;
			dw[1] = 0;
			gtk_container_foreach((GtkContainer *)hwnd,OnEnumChild,(gpointer)dw);
			if(dw[1] != 0)
				EnableWindow((HWND)dw[1],lParam != 0 ? true : false);
		break;
		case TB_CHECKBUTTON:
			dw[0] = wParam;
			dw[1] = 0;
			gtk_container_foreach((GtkContainer *)hwnd,OnEnumChild,(gpointer)dw);
			if(dw[1] != 0){
				g_signal_handler_block((gpointer)dw[1],GetWindowLong((HWND)dw[1],GWL_SIGNAL));
				gtk_toggle_tool_button_set_active((GtkToggleToolButton *)dw[1],lParam != 0 ? true : false);
				g_signal_handler_unblock((gpointer)dw[1],GetWindowLong((HWND)dw[1],GWL_SIGNAL));
			}
		break;
	}
	return 0;
}
//--------------------------------------------------------------------------------
HWND CreateToolBar(DWORD dwStyle)
{
	HWND hwnd;

	hwnd = gtk_toolbar_new();
	if(hwnd == NULL)
		return NULL;
	if(dwStyle & 0x80)
		gtk_toolbar_set_orientation(GTK_TOOLBAR(hwnd),GTK_ORIENTATION_VERTICAL);
	gtk_toolbar_set_style(GTK_TOOLBAR(hwnd),GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(hwnd), GTK_ICON_SIZE_SMALL_TOOLBAR);
	SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WndProcToolBar);
	SetWindowLong(hwnd,GWL_STYLE,dwStyle);
	if(dwStyle & TBSTYLE_TOOLTIPS){
		gtk_toolbar_set_tooltips(GTK_TOOLBAR(hwnd),false);
		gtk_widget_set_has_tooltip(hwnd,true);
		g_signal_connect((gpointer)hwnd,"query-tooltip",G_CALLBACK(OnQueryToolTip),(gpointer)0);
	}
	if(dwStyle & WS_VISIBLE)
		gtk_widget_show_all(hwnd);
	return hwnd;
}
//--------------------------------------------------------------------------------
static LRESULT WndProcStatusBar(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	char s[20];
	guint *parts;
	int i;

	switch(uMsg){
		case WM_SETTEXT:
		break;
		case SB_SETTEXT:
			parts = (guint *)GetWindowLong(hwnd,GWL_DATA_INT0);
			if(parts == NULL || lParam == 0)
				return 0;
			if(wParam >= parts[0])
				return 0;
			gtk_statusbar_pop((GtkStatusbar *)hwnd,parts[wParam]);
			gtk_statusbar_push((GtkStatusbar *)hwnd,parts[wParam],(gchar *)lParam);
			return 1;
		case SB_SETPARTS:
			if(wParam > 0 && lParam != 0){
				parts = (guint *)LocalAlloc(LPTR,(wParam + 1)*sizeof(guint));
				parts[0] = wParam;
				for(i=0;i<wParam;i++){
					sprintf(s,"%d %d",i,((int *)lParam)[i]);
					parts[i+1] = gtk_statusbar_get_context_id((GtkStatusbar *)hwnd,s);
				}
				parts = (guint *)SetWindowLong(hwnd,GWL_DATA_INT0,(LONG)parts);
				if(parts != NULL)
					LocalFree(parts);
			}
		break;
	}
	return 0;
}
//--------------------------------------------------------------------------------
HWND CreateStatusBar(DWORD dwStyle)
{
	HWND hwnd;

  	hwnd = gtk_statusbar_new();
  	if(hwnd == NULL)
		return NULL;
  	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(hwnd),((dwStyle & SBARS_SIZEGRIP) != 0 ? true : false));
	SetWindowLong(hwnd,GWL_STYLE,dwStyle);
	SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WndProcStatusBar);
	if(dwStyle & WS_VISIBLE)
		gtk_widget_show_all(hwnd);
	return hwnd;
}
//--------------------------------------------------------------------------------
static LRESULT WndProcUpDownControl(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg){
		case UDM_SETRANGE:
			gtk_spin_button_set_range((GtkSpinButton *)hwnd,HIWORD(lParam),LOWORD(lParam));
		break;
		case UDM_SETPOS:
			gtk_spin_button_set_value((GtkSpinButton *)hwnd,lParam);
		break;
		case UDM_GETPOS:
			return (LRESULT)gtk_spin_button_get_value_as_int((GtkSpinButton *)hwnd);
        case WM_SETTEXT:
            if(lParam != 0)
            {
                int value;

                value = atoi((char *)lParam);
                gtk_spin_button_set_value((GtkSpinButton *)hwnd,value);
            }
        break;
        case WM_GETTEXT:
            if(lParam){
                int value;

                value = gtk_spin_button_get_value_as_int((GtkSpinButton *)hwnd);
                sprintf((char *)lParam,"%d",value);
            }
        break;
	}
	return 0;
}
//--------------------------------------------------------------------------------
HWND CreateUpDownControl(DWORD dwStyle)
{
	HWND hwnd;

	hwnd = gtk_spin_button_new_with_range(0,100,1);
	if(dwStyle & UDS_ALIGNRIGHT)
		gtk_entry_set_alignment(&((GtkSpinButton *)hwnd)->entry,1);
	SetWindowLong(hwnd,GWL_STYLE,dwStyle);
	SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WndProcUpDownControl);
	if(dwStyle & WS_VISIBLE)
		gtk_widget_show_all(hwnd);
	return hwnd;
}

#endif

