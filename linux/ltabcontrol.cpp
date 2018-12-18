#include "ideastypes.h"
#include "ltabcontrol.h"

#ifndef __WIN32__

//---------------------------------------------------------------------------
static void OnChangePage(GtkNotebook *notebook,gboolean arg1,gpointer user_data)
{
	NMHDR hdr={0};
	WNDPROC pProc;
	HWND hwnd;

	hwnd = (HWND)GetWindowLong((HWND)notebook,GWL_NOTIFYPARENT);
	if(hwnd == NULL)
		return;
   hdr.code = TCN_SELCHANGE;
   hdr.hwndFrom = (HWND)notebook;
   hdr.idFrom = GetWindowLong((HWND)notebook,GWL_ID);
  	SendMessage(hwnd,WM_NOTIFY,(WPARAM)hdr.idFrom,(LPARAM)&hdr);	
}
//---------------------------------------------------------------------------
static LRESULT WindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	HWND page,label;
	LPTCITEM item;
 	LPRECT lprc;
	GtkRequisition sz;
	GValue o;

	switch(uMsg){
		case TCM_SETCURSEL:
			g_signal_handler_block((gpointer)hWnd,GetWindowLong(hWnd,GWL_SIGNAL+0));
			gtk_notebook_set_current_page(GTK_NOTEBOOK(hWnd),wParam);
			g_signal_handler_unblock((gpointer)hWnd,GetWindowLong(hWnd,GWL_SIGNAL+0));
		break;
		case TCM_GETITEMCOUNT:
			return gtk_notebook_get_n_pages(GTK_NOTEBOOK(hWnd));
		case TCM_GETCURSEL:
			return gtk_notebook_get_current_page(GTK_NOTEBOOK(hWnd));
		case TCM_GETITEM:
			if((item = (LPTCITEM)lParam) != NULL){
				page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(hWnd),wParam);
				if(page != NULL){
					if(item->mask & TCIF_PARAM)
						item->lParam = (LPARAM)GetWindowLong(page,GWL_LPARAM);
					return TRUE;
				}
			}
			return 0;
		case TCM_GETITEMRECT:
			if((lprc = (LPRECT)lParam) != NULL){
				page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(hWnd),wParam);
				if(page != NULL){
					label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(hWnd),page);
					if(label != NULL){
						lprc->left = lprc->top = 0;
						gtk_widget_size_request(label,&sz);
						lprc->right = sz.width;
						lprc->bottom = sz.height;
						o.g_type = G_TYPE_UINT;
						g_object_get_property((GObject *)hWnd,"tab-vborder",&o);
						lprc->bottom += (o.data[0].v_uint + 3) * 2;
					}
				}
			}
		break;
		case TCM_DELETEITEM:
			gtk_notebook_remove_page(GTK_NOTEBOOK(hWnd),wParam);
		break;
		case TCM_INSERTITEM:
  			if((item = (LPTCITEM)lParam) != NULL){
				g_signal_handler_block((gpointer)hWnd,GetWindowLong(hWnd,GWL_SIGNAL+0));
				page = gtk_fixed_new();
				//gtk_container_add(GTK_CONTAINER(hWnd),page);
				label = gtk_label_new(item->pszText);
  				gtk_widget_show(label);
				gtk_widget_show(page);
				gtk_notebook_insert_page(GTK_NOTEBOOK(hWnd),page,label,wParam);
//				gtk_notebook_set_tab_label(GTK_NOTEBOOK(hWnd),page, label);
				
				if(item->mask & TCIF_PARAM)
					SetWindowLong((HWND)page,GWL_LPARAM,(LONG)item->lParam);
				g_signal_handler_unblock((gpointer)hWnd,GetWindowLong(hWnd,GWL_SIGNAL+0));
			}
		break;
	}
	return 0;
}
//---------------------------------------------------------------------------
HWND CreateTabControl(DWORD dwStyle)
{
	HWND hwnd;

	hwnd = gtk_notebook_new();
	if(hwnd == NULL)
		return NULL;
	SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WindowProc);
	change_signal(hwnd,0,"switch-page",G_CALLBACK (OnChangePage),(gpointer)NULL,TRUE);
	return hwnd;
}

#endif

