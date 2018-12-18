#include "ideastypes.h"
#include "ltreeview.h"

#ifndef __WIN32__
//--------------------------------------------------------------------------------
typedef bool (*LPENUMITEMSFUNC)(GtkTreeModel *,GtkTreePath *,GtkTreeIter *,gpointer);
//--------------------------------------------------------------------------------
static gboolean OnEnumItemsFunc(GtkTreeModel *model,GtkTreePath *path,GtkTreeIter *iter,gpointer data)
{
	LPENUMITEMSFUNC p;

	if(data == NULL)
		return TRUE;
	p = (LPENUMITEMSFUNC)((LPDWORD)data)[0];
	if(p == NULL)
		return TRUE;
	p(model,path,iter,(gpointer)((LPDWORD)data)[1]);
	return FALSE;
}
//--------------------------------------------------------------------------------
static void OnDrawImageList(GtkTreeViewColumn *column,GtkCellRenderer *cell,GtkTreeModel *model,GtkTreeIter *iter,HWND hwnd)
{
	int index;
	HIMAGELIST himl;
	HBITMAP bit;

	if((himl = (HIMAGELIST)GetWindowLong(hwnd,GWL_IMAGELIST)) == NULL)
		return;
	gtk_tree_model_get (model, iter, 2, &index, -1);
	if((bit = (HBITMAP)himl->set->GetItem(index+1)) == NULL)
		return;
	g_object_set (cell, "pixbuf",bit, NULL);
}
//--------------------------------------------------------------------------------
static bool OnEnumItemsForCount(GtkTreeModel *model,GtkTreePath *path,GtkTreeIter *iter,gpointer data)
{
	((LPDWORD)data)[0]++;
	return true;
}
//--------------------------------------------------------------------------------
static void OnDeleteRow(GtkTreeModel *model,GtkTreePath *path,GtkTreeIter *iter,HWND hwnd)
{
	NMTREEVIEW hdr = {0};

	hdr.hdr.hwndFrom = hwnd;
	hdr.hdr.idFrom = GetWindowLong(hdr.hdr.hwndFrom,GWL_ID);
	hdr.hdr.code = TVN_DELETEITEM;
	hdr.itemOld.hItem = iter;
	//hdr.itemOld.mask =
	//gtk_tree_model_get(treemodel,&iter, 3, &hdr.itemOld.lParam, -1);
	SendMessage((HWND)GetWindowLong(hdr.hdr.hwndFrom,GWL_NOTIFYPARENT),WM_NOTIFY,hdr.hdr.idFrom,(LPARAM)&hdr);
}
//--------------------------------------------------------------------------------
static LRESULT WndProcTreeView(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	GtkTreeStore *model;
	GtkTreeIter par,root,*iter,*iter2;
	gint i,i1,i2;
	HWND w;
	GtkTreeViewColumn *column;
	GList *pList,*pList2;
	GtkCellRenderer *renderer;
	HIMAGELIST himl;
	TV_ITEM *p1;
	GtkTreeSelection *selection;
	DWORD dw[3];
	HWND hwndTreeView;
	LPTV_HITTESTINFO p2;
	GtkTreePath *path;
	gint x,y,width,height;
	gboolean active;
	GdkRectangle rect;

	hwndTreeView = gtk_bin_get_child((GtkBin *)hwnd);
	switch(uMsg){
		case TVM_DELETEITEM:
			if(lParam == 0)
				return 0;
			model = (GtkTreeStore *)gtk_tree_view_get_model((GtkTreeView *)hwndTreeView);
			if(lParam == (LPARAM)TVI_ROOT){
				dw[0] = (DWORD)OnDeleteRow;
				dw[1] = (DWORD)hwnd;
				dw[2] = 0;
				gtk_tree_model_foreach((GtkTreeModel *)model,OnEnumItemsFunc,(gpointer)dw);
				gtk_tree_store_clear(model);
			}
			else{
				OnDeleteRow((GtkTreeModel *)model,NULL,(GtkTreeIter *)lParam,hwnd);
			 	gtk_tree_store_remove(model,(GtkTreeIter *)lParam);
			}
		break;
		case TVM_GETCOUNT:
			model = (GtkTreeStore *)gtk_tree_view_get_model((GtkTreeView *)hwndTreeView);
			if(model == NULL)
				return 0;
			dw[0] = (DWORD)OnEnumItemsForCount;
			dw[1] = (DWORD)&dw[2];
			dw[2] = 0;
			gtk_tree_model_foreach((GtkTreeModel *)model,OnEnumItemsFunc,(gpointer)dw);
			return dw[2];
		case TVM_GETNEXTITEM:
			switch(wParam){
				case TVGN_CARET:
					if((selection = gtk_tree_view_get_selection((GtkTreeView *)hwndTreeView)) != NULL){
						if(gtk_tree_selection_get_selected(selection,(GtkTreeModel **)&model,&par)){
							gtk_tree_model_get ((GtkTreeModel *)model, &par, 4, &iter, -1);
							return (LRESULT)iter;
						}
					}
				break;
				case TVGN_ROOT:
					model = (GtkTreeStore *)gtk_tree_view_get_model((GtkTreeView *)hwndTreeView);
					if(!gtk_tree_model_get_iter_first((GtkTreeModel *)model,&root))
						return 0;
					gtk_tree_model_get((GtkTreeModel *)model, &root, 4, &iter, -1);
					return (LRESULT)iter;
				break;
				case TVGN_NEXT:
					if((iter = (GtkTreeIter *)lParam) == NULL)
						return 0;
					model = (GtkTreeStore *)gtk_tree_view_get_model((GtkTreeView *)hwndTreeView);
					memcpy(&par,iter,sizeof(GtkTreeIter));
					if(gtk_tree_model_iter_next((GtkTreeModel *)model,&par)){
						gtk_tree_model_get((GtkTreeModel *)model, &par, 4, &iter, -1);
						return (LRESULT)iter;
					}
					return 0;
				case TVGN_PARENT:
					if((iter = (GtkTreeIter *)lParam) == NULL)
						return 0;
					model = (GtkTreeStore *)gtk_tree_view_get_model((GtkTreeView *)hwndTreeView);
					if(gtk_tree_model_iter_parent((GtkTreeModel *)model,&par,iter)) {
						gtk_tree_model_get((GtkTreeModel *)model, &par, 4, &iter, -1);
						return (LRESULT)iter;
					}
					return 0;
				case TVGN_CHILD:
					if((iter = (GtkTreeIter *)lParam) == NULL)
						return 0;
					model = (GtkTreeStore *)gtk_tree_view_get_model((GtkTreeView *)hwndTreeView);
					if(gtk_tree_model_iter_children((GtkTreeModel *)model,&par,iter)){
						gtk_tree_model_get((GtkTreeModel *)model, &par, 4, &iter, -1);
						return (LRESULT)iter;
					}
					return 0;
				break;
			}
		break;
		case TVM_SETIMAGELIST:
			SetWindowLong(hwndTreeView,GWL_IMAGELIST,(LONG)lParam);
			if(lParam == 0)
				return 0;
			pList = gtk_tree_view_get_columns((GtkTreeView *)hwndTreeView);
			w = NULL;
			if(pList != NULL){
				for(i=0;i<g_list_length(pList);i++){
					column = (GtkTreeViewColumn *)g_list_nth_data(pList,i);
					if(column == NULL)
						continue;
					pList2 = gtk_tree_view_column_get_cell_renderers(column);
					if(pList2 == NULL)
						continue;
					for(i1=0;i1<g_list_length(pList2);i1++){
						renderer = (GtkCellRenderer *)g_list_nth_data(pList2,i1);
						if(GTK_IS_CELL_RENDERER_PIXBUF(renderer)){
							w = (HWND)renderer;
							break;
						}
					}
					g_list_free(pList2);
				}
				g_list_free(pList);
				if(w == NULL){
					renderer = gtk_cell_renderer_pixbuf_new();
					column = gtk_tree_view_get_column((GtkTreeView *)hwndTreeView,0);
					//gtk_tree_view_column_set_attributes(column,renderer,"active",0,NULL);
					gtk_tree_view_column_pack_start(column,renderer,FALSE);
					pList2 = gtk_tree_view_column_get_cell_renderers(column);
					i = 0;
					if(pList2 != NULL){
						if(g_list_length(pList2) > 1)
							i = 1;
						g_list_free(pList2);
					}
					gtk_cell_layout_reorder((GtkCellLayout *)column,renderer,i);
					gtk_tree_view_column_set_cell_data_func(column,renderer,(GtkTreeCellDataFunc)OnDrawImageList,hwndTreeView,NULL);
				}
			}
		break;
		case TVM_GETITEM:
			if((p1 = (TV_ITEM *)lParam) == NULL || p1->hItem == NULL)
				return 0;
			model = (GtkTreeStore *)gtk_tree_view_get_model((GtkTreeView *)hwndTreeView);
			if(p1->mask & TVIF_PARAM)
				gtk_tree_model_get((GtkTreeModel *)model, p1->hItem, 3, &p1->lParam, -1);
			if(p1->mask & TVIF_STATE){
				gtk_tree_model_get((GtkTreeModel *)model, p1->hItem, 0, &p1->state, -1);
				if(p1->state)
					p1->state = 0x2000;
				else
					p1->state = 0x1000;
			}
		break;
		case TVM_SETITEM:
			if((p1 = (TV_ITEM *)lParam) == NULL || p1->hItem == NULL)
				return 0;
			model = (GtkTreeStore *)gtk_tree_view_get_model((GtkTreeView *)hwndTreeView);

			if(p1->mask & TVIF_IMAGE)
				gtk_tree_store_set(model,p1->hItem,2,p1->iImage, -1);
			if(p1->mask & TVIF_PARAM)
				gtk_tree_store_set(model,p1->hItem,3,p1->lParam, -1);
			if(p1->mask & TVIF_STATE){
				if(p1->stateMask & TVIS_STATEIMAGEMASK)
					gtk_tree_store_set(model,p1->hItem,0,p1->state & 0x2000 ? true : false, -1);
			}
		break;
		case TVM_INSERTITEM:
			LPTVINSERTSTRUCT p;

			if((p = (LPTVINSERTSTRUCT)lParam) == NULL)
				return 0;
			model = (GtkTreeStore *)gtk_tree_view_get_model((GtkTreeView *)hwndTreeView);
			if(p->hParent != NULL){
				iter = p->hParent;
				if(p->hParent == TVI_ROOT){
					if(!gtk_tree_model_get_iter_first((GtkTreeModel *)model,&root))
						iter = NULL;
					else
						iter = &root;
				}
			}
			else{
				if(!gtk_tree_model_get_iter_first((GtkTreeModel *)model,&root))
					iter = NULL;
				else
					iter = &root;
			}
			gtk_tree_store_append(model,&par,iter);
			i = 0;
			gtk_tree_store_set(model,&par,0,false,1,p->item.pszText, -1);
/*			column = gtk_tree_view_get_column((GtkTreeView *)hwnd,0);
			pList = gtk_tree_view_column_get_cell_renderers(column);
			renderer = (GtkCellRenderer *)g_list_nth_data(pList,1);
			g_object_set(renderer,"weight",800,NULL);
			g_list_free(pList);*/
			if((p->item.mask & TVIF_IMAGE))
				gtk_tree_store_set(model,&par,2,p->item.iImage, -1);
			if(p->item.mask & TVIF_PARAM)
				gtk_tree_store_set(model,&par,3,p->item.lParam, -1);
			iter = gtk_tree_iter_copy(&par);
			gtk_tree_store_set(model,&par,4,iter, -1);

			return (LRESULT)iter;
		case TVM_HITTEST:
			if((p2 = (LPTV_HITTESTINFO)lParam) == NULL)
				return 0;
			p2->hItem = NULL;
			p2->flags |= TVHT_NOWHERE;
			if(gtk_tree_view_get_path_at_pos((GtkTreeView *)hwndTreeView,p2->pt.x,p2->pt.y,&path,&column,NULL,NULL) && path != NULL){
				gtk_tree_view_get_cell_area((GtkTreeView *)hwndTreeView,path,column,&rect);
				p2->flags |= TVHT_ONITEM;
				model = (GtkTreeStore *)gtk_tree_view_get_model((GtkTreeView *)hwndTreeView);
  				gtk_tree_model_get_iter ((GtkTreeModel *)model, &par, path);
				gtk_tree_model_get((GtkTreeModel *)model, &par, 4, &iter, -1);
				p2->hItem = iter;
				if(p2->pt.x > rect.x){
					i = rect.x;
					pList2 = gtk_tree_view_column_get_cell_renderers(column);
					if(pList2 != NULL){
						for(i2 = -1,i1=0;i1<g_list_length(pList2);i1++){
							renderer = (GtkCellRenderer *)g_list_nth_data(pList2,i1);
							ZeroMemory(&rect,sizeof(rect));
							gtk_cell_renderer_get_size(renderer,hwndTreeView,NULL,&x,&y,&width,&height);
							i += width;
							if(p2->pt.x < i){
								if(GTK_IS_CELL_RENDERER_TEXT(renderer))
                                	p2->flags |= TVHT_ONITEMLABEL;
                            	else if(GTK_IS_CELL_RENDERER_PIXBUF(renderer))
                                	p2->flags |= TVHT_ONITEMICON;
                            	else if(GTK_IS_CELL_RENDERER_TOGGLE(renderer))
                                	p2->flags |= TVHT_ONITEMSTATEICON;
								break;
							}
						}
						g_list_free(pList2);
					}
				}
				gtk_tree_path_free(path);
			}
			return (LRESULT)p2->hItem;
		break;
		case WM_DESTROY:
			model = (GtkTreeStore *)gtk_tree_view_get_model((GtkTreeView *)hwndTreeView);
			dw[0] = (DWORD)OnDeleteRow;
			dw[1] = (DWORD)hwnd;
			dw[2] = 0;
			gtk_tree_model_foreach((GtkTreeModel *)model,OnEnumItemsFunc,(gpointer)dw);
		break;
	}
	return 0;
}
//--------------------------------------------------------------------------------
static void OnClickCheckTreeView(GtkCellRendererToggle *cell_renderer,gchar *path_str,HWND hwnd)
{
  	GtkTreePath *path;
  	GtkTreeIter iter;
	GtkTreeModel *model;
	int value;

	model = gtk_tree_view_get_model((GtkTreeView *)hwnd);
  	path = gtk_tree_path_new_from_string (path_str);
  	gtk_tree_model_get_iter (model, &iter, path);
	value = !gtk_cell_renderer_toggle_get_active((GtkCellRendererToggle*)cell_renderer);
  	gtk_tree_store_set((GtkTreeStore *)model,&iter,0,value,-1);
  	gtk_tree_model_row_changed(model, path, &iter);
  	gtk_tree_path_free(path);
}
//--------------------------------------------------------------------------------
static gboolean OnQueryToolTipTreeView(GtkWidget *widget,gint x,gint y,gboolean keyboard_mode,GtkTooltip *tooltip,gpointer user_data)
{
	NMTVGETINFOTIP hdr={0};
	GtkTreePath *path;
	GtkTreeStore *model;
	GtkTreeIter iter;

	if(!gtk_tree_view_get_dest_row_at_pos((GtkTreeView *)widget,x,y,&path,NULL) || path == NULL)
		return TRUE;
	model = (GtkTreeStore *)gtk_tree_view_get_model((GtkTreeView *)widget);
	gtk_tree_model_get_iter((GtkTreeModel *)model,&iter,path);
	hdr.hdr.hwndFrom = widget;
	hdr.hdr.idFrom = GetWindowLong(widget,GWL_ID);
	hdr.hdr.code = TVN_GETINFOTIP;
	hdr.pszText = (LPSTR)LocalAlloc(LPTR,5000);
	hdr.cchTextMax = 5000;
	hdr.hItem = &iter;
	gtk_tree_model_get((GtkTreeModel *)model,&iter, 3, &hdr.lParam, -1);
	SendMessage((HWND)GetWindowLong(widget,GWL_NOTIFYPARENT),WM_NOTIFY,hdr.hdr.idFrom,(LPARAM)&hdr);
	gtk_tooltip_set_text(tooltip,hdr.pszText);
	LocalFree(hdr.pszText);
	return TRUE;
}
//--------------------------------------------------------------------------------
static void OnEditStart(GtkCellRenderer *renderer,GtkCellEditable *editable,gchar *path,gpointer user_data)
{
	NMTVDISPINFO hdr={0};

	hdr.hdr.hwndFrom = (HWND)user_data;
	hdr.hdr.idFrom = GetWindowLong(hdr.hdr.hwndFrom,GWL_ID);
	hdr.hdr.code = TVN_BEGINLABELEDIT;
	SendMessage((HWND)GetWindowLong(hdr.hdr.hwndFrom,GWL_NOTIFYPARENT),WM_NOTIFY,hdr.hdr.idFrom,(LPARAM)&hdr);
	gtk_cell_editable_editing_done(editable);
	gtk_cell_renderer_stop_editing(renderer,false);
}
//--------------------------------------------------------------------------------
static gboolean OnButtonDown(GtkWidget *widget,GdkEventButton *event,HWND hwnd)
{
	NMHDR hdr;

	if(event->button == 1){
		hdr.code = NM_CLICK;
	}
	else if(event->button == 2){
		return FALSE;
	}
	else if(event->button == 3){
		hdr.code = NM_RCLICK;
	}
	hdr.hwndFrom = (HWND)hwnd;
	hdr.idFrom = GetWindowLong(hwnd,GWL_ID);
	SendMessage((HWND)GetWindowLong(hwnd,GWL_NOTIFYPARENT),WM_NOTIFY,hdr.idFrom,(LPARAM)&hdr);
	return FALSE;
}
//--------------------------------------------------------------------------------
static gboolean OnKeyDown(GtkWidget *widget,GdkEventKey *event,HWND hwnd)
{
	GtkTreePath *path;
	GtkTreeStore *model;
	GtkTreeIter iter;
	gdouble xd,yd;
	gint x,y;

	switch(event->keyval){
		case 0xFFFF:
			gdk_event_get_coords((GdkEvent *)event,&xd,&yd);
			x = xd;
			y = yd;
			if(gtk_tree_view_get_dest_row_at_pos((GtkTreeView *)widget,x,y,&path,NULL) && path != NULL){
				model = (GtkTreeStore *)gtk_tree_view_get_model((GtkTreeView *)widget);
				gtk_tree_model_get_iter((GtkTreeModel *)model,&iter,path);
				OnDeleteRow((GtkTreeModel *)model,NULL,(GtkTreeIter *)&iter,widget);
				gtk_tree_store_remove(model,&iter);
			}

		break;
	}

	return FALSE;
}
//--------------------------------------------------------------------------------
HWND CreateTreeView(DWORD dwStyle)
{
	HWND hwnd,hwnd1;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeStore *model;
	gint w,h;

	hwnd1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (hwnd1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (hwnd1), GTK_SHADOW_IN);
	hwnd = gtk_tree_view_new();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_append_column((GtkTreeView *)hwnd, column);
	if(dwStyle & TVS_CHECKBOXES){
		renderer = gtk_cell_renderer_toggle_new();
		gtk_tree_view_column_pack_start(column,renderer,FALSE);
		gtk_tree_view_column_set_attributes(column,renderer,"active",0,NULL);
		g_signal_connect((gpointer)renderer,"toggled",G_CALLBACK(OnClickCheckTreeView),(gpointer)hwnd);
	}
	//column = gtk_tree_view_column_new ();
	//gtk_tree_view_append_column ((GtkTreeView *)hwnd, column);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column,renderer,FALSE);
	gtk_tree_view_column_set_attributes(column,renderer,"text",1,NULL);
	if(dwStyle & TVS_EDITLABELS){
		g_object_set(renderer,"editable",true,NULL);
		g_signal_connect((gpointer)renderer,"editing-started",G_CALLBACK(OnEditStart),(gpointer)hwnd);
	}
	model = gtk_tree_store_new(5,G_TYPE_BOOLEAN,G_TYPE_STRING,G_TYPE_INT,G_TYPE_ULONG,G_TYPE_ULONG);
	gtk_tree_view_set_model(GTK_TREE_VIEW (hwnd),GTK_TREE_MODEL(model));
	g_object_unref(G_OBJECT(model));
	w = gtk_tree_model_get_flags(GTK_TREE_MODEL(model));
	//g_signal_connect((gpointer)model,"row-deleted",G_CALLBACK(OnDeleteRow),(gpointer)hwnd1);
	g_signal_connect((gpointer)hwnd,"key-press-event",G_CALLBACK(OnKeyDown),(gpointer)hwnd1);
	g_signal_connect((gpointer)hwnd,"button-press-event",G_CALLBACK(OnButtonDown),(gpointer)hwnd1);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(hwnd),false);
	if(!(dwStyle & TVS_NOTOOLTIPS)){
		g_signal_connect((gpointer)hwnd,"query-tooltip",G_CALLBACK(OnQueryToolTipTreeView),(gpointer)0);
		gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(hwnd),0);
	}
	SetWindowLong(hwnd1,GWL_WNDPROC,(LONG)WndProcTreeView);
	gtk_tree_view_set_enable_search((GtkTreeView *)hwnd,false);
	gtk_widget_show (hwnd);
  	gtk_container_add(GTK_CONTAINER(hwnd1),hwnd);

	return hwnd1;
}
#endif

