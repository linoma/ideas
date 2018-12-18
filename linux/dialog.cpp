#include "ideastypes.h"

#ifndef __WIN32__
//--------------------------------------------------------------------------------
static const WCHAR class_names[6][10] ={
   { 'B','u','t','t','o','n', },             /* 0x80 */
   { 'E','d','i','t', },                     /* 0x81 */
   { 'S','t','a','t','i','c', },             /* 0x82 */
   { 'L','i','s','t','B','o','x', },         /* 0x83 */
   { 'S','c','r','o','l','l','B','a','r', }, /* 0x84 */
   { 'C','o','m','b','o','B','o','x', }      /* 0x85 */
};
static GSList *radiobutton1_group = NULL;
//--------------------------------------------------------------------------------
static gboolean OnScrollChange(GtkRange *range,GtkScrollType scroll,gdouble value,gpointer user_data)
{
	UINT uMsg;
	HWND hwnd;
	WORD wScrollMode;

	if((hwnd = (HWND)GetWindowLong((HWND)range,GWL_NOTIFYPARENT)) != NULL){
		if(GTK_IS_HSCROLLBAR(range))
			uMsg = WM_HSCROLL;
		else
			uMsg = WM_VSCROLL;
		switch(scroll){
			case GTK_SCROLL_JUMP:
				wScrollMode = SB_THUMBPOSITION;
			break;
			case GTK_SCROLL_END:
				wScrollMode  = SB_ENDSCROLL;
			break;
			case GTK_SCROLL_STEP_BACKWARD:
				wScrollMode = SB_LINEUP;
			break;
			case GTK_SCROLL_PAGE_FORWARD:
				wScrollMode = SB_PAGEDOWN;
			break;
			case GTK_SCROLL_PAGE_BACKWARD:
				wScrollMode = SB_PAGEUP;
			break;
			case GTK_SCROLL_STEP_FORWARD:
			default:
				wScrollMode = SB_LINEDOWN;
			break;
		}
		SendMessage(hwnd,uMsg,MAKEWPARAM(wScrollMode,0),(LPARAM)range);
	}
	return FALSE;
}
//--------------------------------------------------------------------------------
static gboolean OnKeyDown(GtkWidget *widget,GdkEventKey *event,gpointer user_data)
{
	if(!SendMessage(widget,WM_KEYDOWN,KeyCodeToVK(event),(LPARAM)(KeyCodeToVK(event) << 16)))
		return TRUE;
	return FALSE;
}
//--------------------------------------------------------------------------------
static void OnChangeTextMultiLineEdit(GtkTextBuffer *textbuffer,gpointer user_data)
{
	HWND hwnd;

	hwnd = (HWND)user_data;
	SendMessage((HWND)GetWindowLong(hwnd,GWL_NOTIFYPARENT),WM_COMMAND,(WPARAM)MAKEWPARAM(GetWindowLong(hwnd,GWL_ID),EN_CHANGE),(LPARAM)hwnd);
}
//--------------------------------------------------------------------------------
static void OnButtonClick(HWND hwnd,gpointer data)
{
	SendMessage((HWND)GetWindowLong(hwnd,GWL_NOTIFYPARENT),WM_COMMAND,(WPARAM)MAKEWPARAM(GetWindowLong(hwnd,GWL_ID),BN_CLICKED),(LPARAM)hwnd);
}
//--------------------------------------------------------------------------------
static void OnComboBoxSelectChanged(HWND hwnd,gpointer user_data)
{
	SendMessage((HWND)GetWindowLong(hwnd,GWL_NOTIFYPARENT),WM_COMMAND,(WPARAM)MAKEWPARAM(GetWindowLong(hwnd,GWL_ID),CBN_SELENDOK),(LPARAM)hwnd);
}
//--------------------------------------------------------------------------------
static void OnTrackBarChangeValue(HWND hwnd,gpointer data)
{
	UINT command;

	command = GTK_IS_VSCALE(hwnd) ? WM_VSCROLL : WM_HSCROLL;
	SendMessage((HWND)GetWindowLong(hwnd,GWL_NOTIFYPARENT),command,(WPARAM)MAKEWPARAM(TB_THUMBTRACK,0),(LPARAM)hwnd);
}
//--------------------------------------------------------------------------------
static LRESULT WndProcButton(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	gchar *p;
	GtkImage *image;
	GdkPixmap *map;
	HBITMAP bit,bitMask;
	gint width,height;

	switch(uMsg){
		case WM_SETTEXT:
			gtk_button_set_label((GtkButton*)hwnd,(gchar *)lParam);
		break;
		case WM_GETTEXT:
			p = (gchar *)gtk_button_get_label((GtkButton *)hwnd);
			if(p != NULL && lParam != NULL)
				lstrcpyn((char *)lParam,p,wParam);
			return wParam;
		case BM_SETIMAGE:
			if((bit = (HBITMAP)lParam) != NULL){
				width = gdk_pixbuf_get_width (bit);
				height = gdk_pixbuf_get_height(bit);
				p = (gchar *)gdk_pixbuf_get_pixels(bit);
				bitMask = gdk_pixbuf_add_alpha(bit,true,p[0],p[1],p[2]);
				map = gdk_pixmap_new(NULL,width,height,24);
				gdk_draw_pixbuf(map,NULL,(HBITMAP)bitMask,0,0,0,0,width,height,GDK_RGB_DITHER_NONE,0,0);
				image = (GtkImage *)gtk_image_new_from_pixmap(map,NULL);
				gtk_button_set_image((GtkButton *)hwnd,(HWND)image);
				if(bit != bitMask)
					DeleteObject(bitMask);
				return 1;
			}
			return 0;
		default:
			return 0;
	}
}
//--------------------------------------------------------------------------------
static LRESULT WndProcCheckBox(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	bool value;
	GtkImage *image;
	GdkPixmap *map;
	int width,height;
	HBITMAP bit,bitMask;
	GSList *list;
	guint length,i;
	HWND hwnd1;

	switch(uMsg){
		case BM_SETCHECK:
			value = (wParam & BST_CHECKED) ? true : false;
			if(value != gtk_toggle_button_get_active((GtkToggleButton *)hwnd)){
				g_signal_handler_block(hwnd,GetWindowLong(hwnd,GWL_SIGNAL));
				if(GTK_IS_RADIO_BUTTON(hwnd)){
					list = gtk_radio_button_get_group((GtkRadioButton *)hwnd);
					length = g_slist_length(list);
					for(i=0;i<length;i++){
						hwnd1 = (HWND)g_slist_nth_data(list,i);
						g_signal_handler_block(hwnd1,GetWindowLong(hwnd1,GWL_SIGNAL));
					}
				}
				else
					list = NULL;
				gtk_toggle_button_set_active((GtkToggleButton *)hwnd,value);
				g_signal_handler_unblock(hwnd,GetWindowLong(hwnd,GWL_SIGNAL));
				if(list != NULL){
					for(i=0;i<length;i++){
						hwnd1 = (HWND)g_slist_nth_data(list,i);
						g_signal_handler_unblock(hwnd1,GetWindowLong(hwnd1,GWL_SIGNAL));
					}
				}
			}
		break;
		case BM_GETCHECK:
			return gtk_toggle_button_get_active((GtkToggleButton *)hwnd) ? BST_CHECKED : BST_UNCHECKED;
		case BM_SETIMAGE:
			if((bit = (HBITMAP)lParam) != NULL){
				width = gdk_pixbuf_get_width (bit);
				height = gdk_pixbuf_get_height(bit);
//				bitMask = gdk_pixbuf_add_alpha(bit,true,0xFF,255,255);
				map = gdk_pixmap_new(NULL,width,height,24);
				gdk_draw_pixbuf(map,NULL,(HBITMAP)bit,0,0,0,0,width,height,GDK_RGB_DITHER_NONE,0,0);
				image = (GtkImage *)gtk_image_new_from_pixmap(map,NULL);
				gtk_button_set_image((GtkButton *)hwnd,(HWND)image);
//				if(bit != bitMask)
//					DeleteObject(bitMask);
				return 1;
			}
			return 0;
	}
	return 0;
}
//--------------------------------------------------------------------------------
static LRESULT WndProcComboBox(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gint i;

	switch(uMsg){
		case CB_ADDSTRING:
//			gtk_combo_box_append_text((GtkComboBox *)hwnd,(gchar *)lParam);
			model = (GtkTreeModel *)gtk_combo_box_get_model((GtkComboBox *)hwnd);
			if(model != NULL){
				gtk_list_store_append((GtkListStore *)model,&iter);
				gtk_list_store_set((GtkListStore *)model,&iter,0,lParam,-1);
				i = 0;
				gtk_tree_model_get_iter_first((GtkTreeModel *)model,&iter);
				do{
					i++;
					if(!gtk_tree_model_iter_next((GtkTreeModel *)model,&iter))
						break;
				}while(1);
			}
			return i-1;
		break;
		case CB_RESETCONTENT:
			model = (GtkTreeModel *)gtk_combo_box_get_model((GtkComboBox *)hwnd);
			if(model != NULL){
				gtk_list_store_clear((GtkListStore *)model);
			}
		break;
		case CB_SETITEMDATA:
			model = (GtkTreeModel *)gtk_combo_box_get_model((GtkComboBox *)hwnd);
			if(model != NULL){
				gtk_tree_model_get_iter_first((GtkTreeModel *)model,&iter);
				for(i=1;i<=wParam;i++){
					if(!gtk_tree_model_iter_next((GtkTreeModel *)model,&iter))
						return CB_ERR;
				}
				gtk_list_store_set((GtkListStore *)model,&iter,1,lParam,-1);
			}
		break;
		case CB_GETITEMDATA:
			model = (GtkTreeModel *)gtk_combo_box_get_model((GtkComboBox *)hwnd);
			if(model != NULL){
				if(!gtk_tree_model_get_iter_first((GtkTreeModel *)model,&iter))
					return CB_ERR;
				for(i=1;i<=wParam;i++){
					if(!gtk_tree_model_iter_next((GtkTreeModel *)model,&iter))
						return CB_ERR;
				}
				gtk_tree_model_get((GtkTreeModel *)model, &iter, 1, &i, -1);
				return i;
			}
			return CB_ERR;
		case CB_SETCURSEL:
			g_signal_handler_block((gpointer)hwnd,GetWindowLong(hwnd,GWL_SIGNAL+0));
			gtk_combo_box_set_active((GtkComboBox *)hwnd,(gint)wParam);
			g_signal_handler_unblock((gpointer)hwnd,GetWindowLong(hwnd,GWL_SIGNAL+0));
		break;
		case CB_GETCURSEL:
			return gtk_combo_box_get_active((GtkComboBox *)hwnd);
		case CB_GETCOUNT:
			i = 0;
			model = (GtkTreeModel *)gtk_combo_box_get_model((GtkComboBox *)hwnd);
			if(model != NULL){
				gtk_tree_model_get_iter_first((GtkTreeModel *)model,&iter);
				do{
					i++;
					if(!gtk_tree_model_iter_next((GtkTreeModel *)model,&iter))
						break;
				}while(1);
			}
			return i;
	}
	return 0;
}
//--------------------------------------------------------------------------------
LRESULT WndProcEdit(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	gchar *p;

	switch(uMsg){
		case WM_SETTEXT:
			gtk_entry_set_text((GtkEntry *)hwnd,(gchar *)lParam);
		break;
		case WM_GETTEXT:
			p = (gchar *)gtk_entry_get_text((GtkEntry *)hwnd);
			if(p != NULL && lParam != NULL)
				lstrcpyn((char *)lParam,p,wParam);
		break;
		case WM_GETTEXTLENGTH:
			p = (gchar *)gtk_entry_get_text((GtkEntry *)hwnd);
			if(p != NULL)
				return lstrlen(p);
			return 0;
		case WM_KEYDOWN:
			return TRUE;
	}
	return 0;
}
//---------------------------------------------------------------------------
static gboolean OnPaintStaticControl(GtkWidget *widget,GdkEventExpose *event,gpointer user_data)
{
	WNDPROC pWndProc;
	HWND hParent;
	HDC hdc;
	gboolean res;

	res = FALSE;
	hParent = (HWND)GetWindowLong(widget,GWL_NOTIFYPARENT);
	if(hParent == NULL)
		goto ex_OnPaintStaticControl;
	pWndProc = (WNDPROC)GetWindowLong(hParent,GWL_WNDPROC);
	if(pWndProc == NULL)
		goto ex_OnPaintStaticControl;
	hdc = GetDC(widget);
	pWndProc(hParent,WM_CTLCOLORSTATIC,(WPARAM)hdc,(LPARAM)widget);
	ReleaseDC(widget,hdc);
	res = FALSE;
	pWndProc = (WNDPROC)GetWindowLong(widget,GWL_WNDPROC);
	if(pWndProc != NULL){
		if(!pWndProc(widget,WM_PAINT,0,0))
			res = TRUE;
	}
ex_OnPaintStaticControl:
    if(!res){
        switch(((unsigned long)user_data) & SS_TYPEMASK){
            case SS_BLACKRECT:
                gdk_draw_rectangle(widget->window,widget->style->fg_gc[GTK_WIDGET_STATE (widget)],TRUE,
                    0, 0, widget->allocation.width, widget->allocation.height);
                res = TRUE;
            break;
        }
    }
	return res;
}
//--------------------------------------------------------------------------------
static LRESULT WndProcStatic(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg){
		case WM_SETTEXT:
			gtk_label_set_text((GtkLabel *)hwnd,(gchar *)lParam);
		break;
		case STM_SETIMAGE:
			gtk_image_set_from_pixbuf((GtkImage *)hwnd,(HBITMAP)lParam);
		break;
		case WM_PAINT:
			return 1;
	}
	return 0;
}
//--------------------------------------------------------------------------------
static void UpdateListBoxID(GtkTreeView *hwnd)
{
	GtkListStore *model;
	GtkTreeIter iter;
	int id;

	model = (GtkListStore *)gtk_tree_view_get_model((GtkTreeView *)hwnd);
	if(model == NULL)
		return;
	id = 0;
	gtk_tree_model_get_iter_first((GtkTreeModel *)model,&iter);
	do{
		gtk_list_store_set(model,&iter,1,id++, -1);
		if(!gtk_tree_model_iter_next((GtkTreeModel *)model,&iter))
			break;
	}while(1);
}
//--------------------------------------------------------------------------------
static LRESULT WndProcListBox(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	GtkTreeViewColumn *column;
	gint x,y,width,height,i;
	GdkRectangle rc;
	GtkListStore *model;
	GtkTreeIter iter;
	char *string;
	GtkBorder border;
	GtkTreeSelection *select;
	GtkTreePath *path,*path1;

	switch(uMsg){
		case LB_GETCOUNT:
			i = 0;
			model = (GtkListStore *)gtk_tree_view_get_model((GtkTreeView *)hwnd);
			if(model != NULL){
				gtk_tree_model_get_iter_first((GtkTreeModel *)model,&iter);
				do{
					i++;
					if(!gtk_tree_model_iter_next((GtkTreeModel *)model,&iter))
						break;
				}while(1);
			}
			return i;
		case LB_GETITEMHEIGHT:
			column = gtk_tree_view_get_column(GTK_TREE_VIEW(hwnd),0);
			gtk_tree_view_column_cell_get_size(column,NULL,&x,&y,&width,&height);
			gtk_widget_style_get(GTK_WIDGET(hwnd),"expander-size", &i,NULL);
			x = 0;
			if(i >= height){
				height = i;
				x = 1;
			}
			i = 0;
			gtk_widget_style_get(GTK_WIDGET(hwnd),"vertical-separator", &i,NULL);
			if(x != 0)
				i *= 2;
			return height+i;
		case LB_INSERTSTRING:
			model = (GtkListStore *)gtk_tree_view_get_model((GtkTreeView *)hwnd);
			if(model != NULL){
				if(wParam == -1)
					gtk_list_store_append(model,&iter);
				else
					gtk_list_store_insert(model,&iter,wParam);
				gtk_list_store_set(model,&iter,0,lParam, -1);
				UpdateListBoxID((GtkTreeView *)hwnd);
				gtk_widget_queue_resize_no_redraw(hwnd);
			}
		break;
		case LB_ADDSTRING:
			g_object_freeze_notify(G_OBJECT(hwnd));
			model = (GtkListStore *)gtk_tree_view_get_model((GtkTreeView *)hwnd);
			if(model != NULL){
				gtk_list_store_append(model,&iter);
				gtk_list_store_set(model,&iter,0,lParam, -1);
				if(GetWindowLong(hwnd,GWL_STYLE) & LBS_OWNERDRAWFIXED)
					UpdateListBoxID((GtkTreeView *)hwnd);
				gtk_widget_queue_resize_no_redraw(hwnd);
			}
			g_object_thaw_notify(G_OBJECT(hwnd));
		break;
		case LB_GETTEXT:
			model = (GtkListStore *)gtk_tree_view_get_model((GtkTreeView *)hwnd);
			if(model == NULL)
				return LB_ERR;
			if(!gtk_tree_model_get_iter_first((GtkTreeModel *)model,&iter))
				return LB_ERR;
			for(i=1;i<=wParam;i++){
				if(!gtk_tree_model_iter_next((GtkTreeModel *)model,&iter))
					return LB_ERR;
			}
			gtk_tree_model_get((GtkTreeModel *)model, &iter, 0, &string, -1);
			if(string != NULL){
				lstrcpy((char*)lParam,string);
				free(string);
			}
		break;
		case LB_DELETESTRING:
			model = (GtkListStore *)gtk_tree_view_get_model((GtkTreeView *)hwnd);
			if(model != NULL){
				gtk_tree_model_get_iter_first((GtkTreeModel *)model,&iter);
				for(i=1;i<=wParam;i++){
					if(!gtk_tree_model_iter_next((GtkTreeModel *)model,&iter))
						return LB_ERR;
				}
				gtk_list_store_remove((GtkListStore *)model,&iter);
				UpdateListBoxID((GtkTreeView *)hwnd);
			}
		break;
		case LB_RESETCONTENT:
			g_object_freeze_notify(G_OBJECT(hwnd));
			model = (GtkListStore *)gtk_tree_view_get_model((GtkTreeView *)hwnd);
			if(model != NULL)
				gtk_list_store_clear(model);
			g_object_thaw_notify(G_OBJECT(hwnd));
		break;
		case LB_GETCURSEL:
			select = gtk_tree_view_get_selection((GtkTreeView *)hwnd);
			if(select == NULL)
				return LB_ERR;
			if(!gtk_tree_selection_get_selected(select,(GtkTreeModel **)&model,&iter))
				return LB_ERR;
			path = gtk_tree_model_get_path((GtkTreeModel *)model,&iter);
			if(path == NULL)
				return LB_ERR;
			i = LB_ERR;
			x = -1;
			if(gtk_tree_model_get_iter_first((GtkTreeModel *)model,&iter)){
				i = 0;
				do{
					path1 = gtk_tree_model_get_path((GtkTreeModel*)model,&iter);
					if(path1 != NULL){
						x = gtk_tree_path_compare(path,path1);
						gtk_tree_path_free(path1);
						if(x == 0)
							break;
					}
					if(!gtk_tree_model_iter_next((GtkTreeModel *)model,&iter))
						break;
					i++;
				}while(1);
				if(x != 0)
					i = LB_ERR;
			}
			gtk_tree_path_free(path);
			return i;
		case LB_GETITEMRECT:
			column = gtk_tree_view_get_column(GTK_TREE_VIEW(hwnd),0);
			gtk_tree_view_column_cell_get_size(column,NULL,&x,&y,&width,&height);
			gtk_widget_style_get(GTK_WIDGET(hwnd),"expander-size", &i,NULL);
			x = 0;
			if(i >= height){
				height = i;
				x = 1;
			}
			i = 0;
			gtk_widget_style_get(GTK_WIDGET(hwnd),"vertical-separator", &i,NULL);
			if(x != 0)
				i *= 2;
			((LPRECT)lParam)->left = 0;
			((LPRECT)lParam)->top = wParam * (height+i);
			((LPRECT)lParam)->right = ((LPRECT)lParam)->left+width;
			((LPRECT)lParam)->bottom = ((LPRECT)lParam)->top + (height + i);
		break;
		case LB_SETCURSEL:
			model = (GtkListStore *)gtk_tree_view_get_model((GtkTreeView *)hwnd);
			if(model != NULL){
				gtk_tree_model_get_iter_first((GtkTreeModel *)model,&iter);
				for(i=1;i<=wParam;i++){
					if(!gtk_tree_model_iter_next((GtkTreeModel *)model,&iter))
						return LB_ERR;
				}
				select = gtk_tree_view_get_selection((GtkTreeView *)hwnd);
				if(select == NULL)
					return LB_ERR;
				gtk_tree_selection_select_iter(select,&iter);
			}
		break;
		case WM_LBUTTONDBLCLK:
			SendMessage((HWND)GetWindowLong(hwnd,GWL_NOTIFYPARENT),WM_COMMAND,MAKEWPARAM(GetWindowLong(hwnd,GWL_ID),LBN_DBLCLK)
				,(LPARAM)hwnd);
		break;
	}
	return 0;
}
//--------------------------------------------------------------------------------
static LRESULT WndProcMultiLineEdit(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	gchar *p;

	switch(uMsg){
		case WM_SETTEXT:
			buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(hwnd));
			gtk_text_buffer_set_text(buffer,(gchar *)lParam, -1);
		break;
		case WM_GETTEXT:
			buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (hwnd));
			gtk_text_buffer_get_start_iter(buffer, &start);
			gtk_text_buffer_get_end_iter(buffer, &end);
			p = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
			if(p != NULL && lParam != NULL)
				lstrcpyn((char *)lParam,p,wParam);
		break;
		case WM_GETTEXTLENGTH:
			buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (hwnd));
			gtk_text_buffer_get_start_iter (buffer, &start);
			gtk_text_buffer_get_end_iter(buffer, &end);
			p = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
			if(p != NULL)
				return lstrlen(p);
			return 0;
		break;
	}
	return 0;
}
//--------------------------------------------------------------------------------
static LRESULT WndProcTrackBar(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg){
		case TBM_SETPOS:
			gtk_range_set_value((GtkRange *)hwnd,(gdouble)lParam);
		break;
		case TBM_GETPOS:
			return (LRESULT)gtk_range_get_value((GtkRange *)hwnd);
		case TBM_SETRANGE:
			gtk_range_set_range((GtkRange *)hwnd,LOWORD(lParam),HIWORD(lParam));
		break;
	}
	return 0;
}
//--------------------------------------------------------------------------------
static LRESULT WndProcGroupBox(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg){
		case WM_SETTEXT:
			gtk_frame_set_label((GtkFrame *)hwnd,(gchar *)lParam);
		break;
	}
	return 0;
}
//--------------------------------------------------------------------------------
const WORD *DIALOG_GetControl32(const WORD *p, DLG_CONTROL_INFO *info,BOOL dialogEx)
{
	WORD id;
	int i,i1;
	WCHAR *p1;

    if(dialogEx){
        info->helpId  = *((LPDWORD)p);
		p += 2;
        info->exStyle = *((LPDWORD)p);
		p += 2;
        info->style   = *((LPDWORD)p);
		p += 2;
    }
    else{
        info->helpId  = 0;
        info->style   = *((LPDWORD)p);
		p += 2;
        info->exStyle = *((LPDWORD)p);
		p += 2;
    }
    info->x = *p++;
    info->y = *p++;
    info->cx = *p++;
    info->cy = *p++;
    if(dialogEx){
        info->id = *((LPDWORD)p);
        p += 2;
    }
    else
        info->id = *p++;
    if(*p == 0xffff){
        id = *(p+1);
        if((id >= 0x80) && (id <= 0x85))
			id -= 0x80;
        if(id <= 5){
            info->className = (WCHAR *)class_names[id];
			info->classId = id;
		}
        else{
            info->className = NULL;
			info->classId = 0;
		}
        p += 2;
    }
    else{
        info->className = (LPCWSTR)p;
		info->classId = -1;
		for(i=0;i<sizeof(class_names) / sizeof(class_names[0]);i++){
			for(p1 = (WCHAR *)p,i1=0;*p1 != 0 && class_names[i][i1] != 0;i1++,p1++){
				if(tolower(*p1) != tolower(class_names[i][i1]))
					break;
			}
			if(*p1 == 0 && class_names[i][i1] == 0){
				info->classId = i;
				break;
			}
		}
        p += strlenW(info->className) + 1;
    }
    if(*p == 0xffff){
        info->windowName = MAKEINTRESOURCEW(*(p+1));
        p += 2;
    }
    else{
        info->windowName = (LPCWSTR)p;
        p += strlenW( info->windowName ) + 1;
    }
    if(*p){
        info->data = (LPVOID)(p + 1);
        p += *p / sizeof(WORD);
    }
    else
		info->data = NULL;
    p++;
    return (const WORD *)(((int)p + 3) & ~3);
}
//--------------------------------------------------------------------------------
const WORD *DIALOG_Get32(const WORD *p,LPDLG_INFO info)
{
   WORD signature,dlgver;

   	signature = *p++;
   	dlgver = *p++;
	if(signature == 1 && dlgver == 0xffff){
	info->dialogEx = TRUE;
    	p += 2;
       info->exstyle = *((LPDWORD)p);
		p += 2;
       info->style = *((LPDWORD)p);
		p += 2;
   	}
   	else{
       info->style = *((LPDWORD)(p-2));
       info->dialogEx = FALSE;
       info->exstyle  = *((LPDWORD)p);
	    p += 2;
   	}
   	info->nItems = *p++;
   	info->x       = *p++;
   	info->y       = *p++;
   	info->cx      = *p++;
   	info->cy      = *p++;
   	switch(*p){
       case 0x0000:
        	info->menuName = NULL;
        	p++;
		break;
    	case 0xffff:
        	info->menuName = MAKEINTRESOURCEW(*(p + 1));
        	p += 2;
       break;
    	default:
        	info->menuName = (LPCWSTR)p;
        	p += strlenW(info->menuName ) + 1;
       break;
   }
   switch(*p){
    	case 0x0000:
           info->className = (LPCWSTR)NULL;
        	p++;
       break;
    	case 0xffff:
        	info->className = MAKEINTRESOURCEW(*(p + 1));
        	p += 2;
       break;
    	default:
        	info->className = (LPCWSTR)p;
        	p += strlenW(info->className ) + 1;
       break;
   }
   info->caption = (LPCWSTR)p;
   p += strlenW(info->caption) + 1;
   if (info->style & DS_SETFONT){
       info->pointSize = *p++;
       if (info->dialogEx){
           info->weight = *p++;
           info->italic = LOBYTE(*p++);
       }
       info->faceName = (LPCWSTR)p;
       p += strlenW(info->faceName) + 1;
   }
   return (LPWORD)(((UINT)p + 3) & ~3);
}
//--------------------------------------------------------------------------------
static void OnUpDownChangeValue(GtkSpinButton *spinbutton,gpointer user_data)
{
	SendMessage((HWND)GetWindowLong((HWND)spinbutton,GWL_NOTIFYPARENT),WM_VSCROLL,0,(LPARAM)spinbutton);
}
//--------------------------------------------------------------------------------
HWND DIALOG_CreateControl(DLG_CONTROL_INFO *info,gint xBaseUnit,gint yBaseUnit,HWND hDlg)
{
	char temp[1024];
	HWND hwnd,hwnd1;
	gint cy,cx,i;
	GtkTextBuffer *buffer;
	bool bResize;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkListStore *list_store;

	temp[0] = 0;
	WideCharToMultiByte(0,0,info->windowName,-1,temp,300,NULL,FALSE);
	cy = info->cy;
	cx = info->cx;
	hwnd1 = hwnd = NULL;
	bResize = true;
	switch(info->classId){
		case -1:
			WideCharToMultiByte(0,0,info->className,-1,&temp[300],300,NULL,FALSE);
			if(strcasecmp(&temp[300],"msctls_trackbar32") == 0){
				if(info->style & TBS_VERT)
					hwnd = gtk_vscale_new_with_range(0,100,1);
				else
 					hwnd = gtk_hscale_new_with_range (0,100,1);
				gtk_scale_set_draw_value(GTK_SCALE (hwnd), FALSE);
				SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WndProcTrackBar);
				SetWindowLong(hwnd,GWL_SIGNAL,(LONG)g_signal_connect((gpointer)hwnd,"value-changed",G_CALLBACK(OnTrackBarChangeValue),(gpointer)0));
			}
			else if(strcasecmp(&temp[300],"SysTreeView32") == 0){
				hwnd = CreateTreeView(info->style);
				hwnd1 = gtk_bin_get_child((GtkBin *)hwnd);
				SetWindowLong(hwnd,GWL_ID,(LONG)info->id);
				SetWindowLong(hwnd1,GWL_ID,(LONG)info->id);
				SetWindowLong(hwnd,GWL_NOTIFYPARENT,(LONG)hDlg);
				SetWindowLong(hwnd1,GWL_NOTIFYPARENT,(LONG)hDlg);
			}
			else if(strcasecmp(&temp[300],"ToolbarWindow32") == 0){
				hwnd = CreateToolBar(info->style);
				SetWindowLong(hwnd,GWL_ID,(LONG)info->id);
				SetWindowLong(hwnd,GWL_NOTIFYPARENT,(LONG)hDlg);
				bResize = false;
			}
			else if(strcasecmp(&temp[300],"msctls_statusbar32") == 0){
				hwnd = CreateStatusBar(info->style);
				SetWindowLong(hwnd,GWL_ID,(LONG)info->id);
				SetWindowLong(hwnd,GWL_NOTIFYPARENT,(LONG)hDlg);
				bResize = false;
			}
			else if(strcasecmp(&temp[300],"SysTabControl32") == 0){
				hwnd = CreateTabControl(info->style);
				SetWindowLong(hwnd,GWL_ID,(LONG)info->id);
				SetWindowLong(hwnd,GWL_NOTIFYPARENT,(LONG)hDlg);
				bResize = false;
			}
			else if(strcasecmp(&temp[300],"msctls_updown32") == 0){
				hwnd = CreateUpDownControl(info->style);
				SetWindowLong(hwnd,GWL_SIGNAL,(LONG)g_signal_connect((gpointer)hwnd,"value-changed",G_CALLBACK(OnUpDownChangeValue),(gpointer)0));
				SetWindowLong(hwnd,GWL_ID,(LONG)info->id);
				SetWindowLong(hwnd,GWL_NOTIFYPARENT,(LONG)hDlg);
				bResize = (info->style & UDS_AUTOBUDDY) ? false : true;
			}
		break;
		case 0: // button
			switch((info->style & 0xF)){
				case BS_GROUPBOX:
					hwnd = gtk_frame_new(temp);
					radiobutton1_group = NULL;
					if(info->id != -1)
					{
						SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WndProcGroupBox);
					}
				break;
				case BS_AUTOCHECKBOX:
					if(info->style & BS_PUSHLIKE){
						if(temp[0] == 0)
							hwnd = gtk_toggle_button_new();
						else
							hwnd = gtk_toggle_button_new_with_label(temp);
					}
					else{
						if(temp[0] == 0)
							hwnd = gtk_check_button_new();
						else{
							hwnd = gtk_check_button_new_with_label(temp);
							cx += 5;
						}
					}
					SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WndProcCheckBox);
					change_signal(hwnd,0,"clicked",G_CALLBACK(OnButtonClick),(gpointer)0,FALSE);
				break;
				case BS_RADIOBUTTON:
				case BS_AUTORADIOBUTTON:
					if(temp[0] == 0)
						hwnd = gtk_radio_button_new(radiobutton1_group);
					else
						hwnd = gtk_radio_button_new_with_label(radiobutton1_group,temp);
					radiobutton1_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(hwnd));
					SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WndProcCheckBox);
					change_signal(hwnd,0,"clicked",G_CALLBACK(OnButtonClick),(gpointer)0,FALSE);
				break;
				default:
					if(temp[0] != 0){
						hwnd = gtk_button_new_with_label(temp);
					}
					else
						hwnd = gtk_button_new();
					SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WndProcButton);
					g_signal_connect((gpointer)hwnd,"clicked",G_CALLBACK(OnButtonClick),(gpointer)0);
				break;
			}
		break;
		case 1: //Edit;
			if(info->style & ES_MULTILINE){
				hwnd = gtk_scrolled_window_new (NULL, NULL);
  				gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (hwnd), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  				gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (hwnd), GTK_SHADOW_IN);
  				hwnd1 = gtk_text_view_new ();
				gtk_widget_set_size_request(hwnd1,info->cx*xBaseUnit/4,info->cy*yBaseUnit/8);
  				gtk_widget_show (hwnd1);
  				gtk_container_add (GTK_CONTAINER (hwnd), hwnd1);
				SetWindowLong(hwnd1,GWL_WNDPROC,(LONG)WndProcMultiLineEdit);
				SetWindowLong(hwnd1,GWL_ID,(LONG)info->id);
				SetWindowLong(hwnd1,GWL_NOTIFYPARENT,(LONG)hDlg);
				buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (hwnd1));
				if(buffer != NULL)
					g_signal_connect((gpointer)buffer,"changed",G_CALLBACK(OnChangeTextMultiLineEdit),(gpointer)hwnd1);
				if(info->style & ES_READONLY)
					g_object_set (hwnd1, "editable",false, NULL);
					//gtk_widget_set_sensitive(hwnd1,false);
				if(info->style & ES_CENTER)
					gtk_text_view_set_justification(GTK_TEXT_VIEW (hwnd1),GTK_JUSTIFY_CENTER);
				else if(info->style & ES_RIGHT)
					gtk_text_view_set_justification(GTK_TEXT_VIEW (hwnd1),GTK_JUSTIFY_RIGHT);
			}
			else{
				hwnd = gtk_entry_new();
				if(info->style & ES_READONLY)
					g_object_set (hwnd, "editable",false, NULL);
					//gtk_widget_set_sensitive(hwnd,false);
				if(info->style & ES_CENTER)
					gtk_entry_set_alignment((GtkEntry *)hwnd,0.5);
				else if(info->style & ES_RIGHT)
					gtk_entry_set_alignment((GtkEntry *)hwnd,1);
				SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WndProcEdit);
				g_signal_connect((gpointer)hwnd,"key-press-event",G_CALLBACK(OnKeyDown),(gpointer)NULL);
			}
		break;
		case 2: // Static
			if(info->style & SS_NOTIFY){
				hwnd1 = gtk_event_box_new();
  				gtk_widget_show(hwnd1);
			}
			switch(info->style & SS_TYPEMASK){
                case SS_BITMAP:
                    hwnd = gtk_image_new();
                break;
                case SS_BLACKRECT:
                    hwnd = gtk_drawing_area_new();
                break;
                default:
                    hwnd = gtk_label_new(temp);
                    if(info->style & SS_CENTER){
                    }
                    else if(info->style & SS_RIGHT)
                        gtk_misc_set_alignment((GtkMisc *)hwnd,1,0);
                    else
                        gtk_misc_set_alignment((GtkMisc *)hwnd,0,0);
                break;
			}
			if(hwnd1 != NULL){
				gtk_container_add(GTK_CONTAINER(hwnd1),hwnd);
				SetWindowLong(hwnd1,GWL_WNDPROC,(LONG)WndProcStatic);
				change_signal(hwnd,0,"expose-event",G_CALLBACK(OnPaintStaticControl),(gpointer)NULL,FALSE);
				//g_object_set(hwnd,"can-focus",true,NULL);
				g_object_set(hwnd1,"can-focus",true,NULL);
				hwnd = hwnd1;
				hwnd1 = NULL;
			}
			else{
				SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WndProcStatic);
				change_signal(hwnd,0,"expose-event",G_CALLBACK (OnPaintStaticControl),(gpointer)info->style,FALSE);
			}
		break;
		case 5: //ComboBox
            list_store = gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_INT);
			if((info->style & CBS_DROPDOWNLIST) == CBS_DROPDOWN){
				hwnd = gtk_combo_box_entry_new_with_model((GtkTreeModel *)list_store,0);
				//gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(hwnd),renderer,true);
			}
			else{
            	hwnd = gtk_combo_box_new_with_model((GtkTreeModel *)list_store);
				renderer = gtk_cell_renderer_text_new();
				gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(hwnd),renderer,true);
				gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(hwnd),renderer,"text",0,NULL);
			}
			gtk_container_set_border_width((GtkContainer *)hwnd,0);
			SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WndProcComboBox);
			change_signal(hwnd,0,"changed",G_CALLBACK(OnComboBoxSelectChanged),(gpointer)NULL,FALSE);
			cy = (31*8) / yBaseUnit;
			g_object_unref(list_store);
		break;
		case 3: //ListBox
			hwnd = NULL;
			if(info->style & (WS_VSCROLL|WS_HSCROLL)){
				hwnd = gtk_scrolled_window_new(NULL,NULL);
  				gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(hwnd),GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  				gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(hwnd),GTK_SHADOW_ETCHED_IN);
			}
			list_store = gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_INT);
			hwnd1 = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
			if(hwnd1 != NULL){
  				gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(hwnd1), FALSE);
  				gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(hwnd1),TRUE);
				column = gtk_tree_view_column_new();
				g_object_set(column,"sizing",GTK_TREE_VIEW_COLUMN_FIXED,NULL);
				gtk_tree_view_column_set_title(column,"Text");
				if(info->style & LBS_OWNERDRAWFIXED)
					renderer = cell_renderer_ownerdraw_new();
				else
					renderer = gtk_cell_renderer_text_new();
				g_object_set(renderer,"ypad",0,NULL);
				g_object_set(renderer,"yalign",0.5,NULL);
				gtk_cell_renderer_set_fixed_size(renderer,-1,12);
				gtk_tree_view_column_pack_start(column,renderer,TRUE);
				if(info->style & LBS_OWNERDRAWFIXED)
					gtk_tree_view_column_set_attributes(column,renderer,"text",0,"id",1,NULL);
				else
					gtk_tree_view_column_set_attributes(column,renderer,"text",0,NULL);
				gtk_tree_view_append_column((GtkTreeView *)hwnd1, column);
				SetWindowLong(hwnd1,GWL_WNDPROC,(LONG)WndProcListBox);
			}
			if(hwnd != NULL){
				gtk_widget_set_size_request(hwnd1,info->cx*xBaseUnit/4,info->cy*yBaseUnit/8);
  				gtk_widget_show(hwnd1);
  				gtk_container_add(GTK_CONTAINER (hwnd), hwnd1);
				SetWindowLong(hwnd1,GWL_ID,(LONG)info->id);
				SetWindowLong(hwnd1,GWL_NOTIFYPARENT,(LONG)hDlg);
			}
			else{
				hwnd = hwnd1;
				hwnd1 = NULL;
			}
			g_object_unref(list_store);
		break;
		case 4: //Scrollbar
  			hwnd = gtk_event_box_new();
  			//gtk_widget_set_name (eventbox1, "eventbox1");
  			gtk_widget_show(hwnd);
			if(info->style & SBS_VERT)
				hwnd1 = gtk_vscrollbar_new(GTK_ADJUSTMENT(gtk_adjustment_new(0,0,0,0,0,0)));
			else
				hwnd1 = gtk_hscrollbar_new(GTK_ADJUSTMENT(gtk_adjustment_new(0,0,0,0,0,0)));
			gtk_container_add (GTK_CONTAINER (hwnd), hwnd1);
			change_signal(hwnd1,0,"change-value",G_CALLBACK(OnScrollChange),NULL,FALSE);
			//SetWindowLong(hwnd1,GWL_WNDPROC,(LONG)WndProcMultiLineEdit);
			SetWindowLong(hwnd1,GWL_NOTIFYPARENT,(LONG)hDlg);
			hwnd1 = NULL;
		break;
		default:
			hwnd = NULL;
		break;
	}
	if(hwnd != NULL){
		if(bResize){
			cy = cy*yBaseUnit/8;
			gtk_widget_set_size_request(hwnd,cx*xBaseUnit/4,cy);
		}
		if(info->style & WS_VISIBLE)
			gtk_widget_show(hwnd);
		if(info->style & WS_DISABLED)
			 gtk_widget_set_sensitive(hwnd,false);
		if(hwnd1 == NULL){
			SetWindowLong(hwnd,GWL_ID,(LONG)info->id);
			SetWindowLong(hwnd,GWL_STYLE,info->style);
			SetWindowLong(hwnd,GWL_NOTIFYPARENT,(LONG)hDlg);
		}
	}
	return hwnd;
}
//---------------------------------------------------------------------------
static void OnEnumChild(GtkWidget *widget,gpointer data)
{
	if(GTK_IS_CONTAINER(widget))
		gtk_container_foreach((GtkContainer *)widget,OnEnumChild,data);
	if(GetWindowLong(widget,GWL_ID) != 0)
		SetWindowLong(widget,GWL_NOTIFYPARENT,((LPDWORD)data)[0]);
}
//---------------------------------------------------------------------------
void set_DialogNotifyParent(HWND hDlg)
{
	DWORD dw[2];

	dw[0] = (DWORD)hDlg;
	gtk_container_foreach((GtkContainer *)hDlg,OnEnumChild,(gpointer)dw);
}

#endif


