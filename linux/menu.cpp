#include "ideastypes.h"

//---------------------------------------------------------------------------
static gboolean OnMenuOwnerDraw(GtkWidget *widget,GdkEventExpose *event,gpointer user_data)
{
	DRAWITEMSTRUCT dis={0};
	GtkRequisition sz;
	gint x,y,w,h,z;
	HWND hwnd;

	dis.itemID = GetWindowLong(widget,GWL_ID);
	/*dis.rcItem.left = event->area.x;
	dis.rcItem.top = event->area.y;
	dis.rcItem.right = event->area.x+event->area.width;
	dis.rcItem.bottom = event->area.y+event->area.height;*/	
	hwnd = gtk_widget_get_parent(widget);
	gtk_widget_translate_coordinates(widget,hwnd,0,0,&x,&y);
	dis.rcItem.left = x;
	dis.rcItem.top = y;
	gtk_widget_size_request(widget,&sz);			
	dis.rcItem.right = x+sz.width;
	gtk_widget_size_request(hwnd,&sz);			
	dis.rcItem.bottom = y+sz.height;
	dis.hDC = GetDC(widget);
	SendMessage((HWND)GetWindowLong(widget,GWL_NOTIFYPARENT),WM_DRAWITEM,dis.itemID,(LPARAM)&dis);
	ReleaseDC(NULL,dis.hDC);
	return FALSE;
}
//---------------------------------------------------------------------------
static void MenuItemsEnum(GtkWidget *w,gpointer data)
{
	LPDWORD pd;
	GtkWidget *pSubMenu;

	pd = (LPDWORD)data;
	if(pd == NULL)
		return;
	SetWindowLong(w,GWL_NOTIFYPARENT,pd[0]);
	pSubMenu = gtk_menu_item_get_submenu((GtkMenuItem *)w);
	if(pSubMenu != NULL){
		SetWindowLong((HWND)pSubMenu,GWL_NOTIFYPARENT,pd[0]);
		gtk_container_forall((GtkContainer *)pSubMenu,MenuItemsEnum,data);	
	}
}
//--------------------------------------------------------------------------------
static void MenuSelectDone(GtkItem *item,gpointer data)
{
	HWND hwnd;
	WNDPROC pProc;

	if(item == NULL)
		return;	
	hwnd = (HWND)GetWindowLong((HWND)item,GWL_NOTIFYPARENT);
	if(hwnd == NULL)
		return;
	if(data != NULL){
		if(GTK_IS_MENU_BAR(gtk_widget_get_parent((HWND)item))){
			if((int)data == 1 && GetWindowLong((HWND)item,GWL_STATE) == 0){
				SendMessage(hwnd,WM_ENTERMENULOOP,(WPARAM)0,0);
				SetWindowLong((HWND)item,GWL_STATE,1);
			}
			else if((int)data == 2 && GetWindowLong((HWND)item,GWL_STATE) != 0){
				SendMessage(hwnd,WM_EXITMENULOOP,(WPARAM)0,0);
				SetWindowLong((HWND)item,GWL_STATE,0);
			}
			SendMessage(hwnd,WM_INITMENU,(WPARAM)gtk_menu_item_get_submenu((GtkMenuItem *)item),0);
		}
		else
			SendMessage(hwnd,WM_INITMENUPOPUP,(WPARAM)gtk_menu_item_get_submenu((GtkMenuItem *)item),0);
	}
	else{
		SendMessage(hwnd,WM_COMMAND,MAKEWPARAM(GetWindowLong((HWND)item,GWL_ID),0),0);
	}
}
//--------------------------------------------------------------------------------
static char *ParseMenu(char *res,HMENU pParent)
{
	WORD flags, id;
   	short *str;
	GtkWidget *pItem,*pSubMenu;
	char c[300];
	int len,i,pos,add;
	LPDWORD pdw;
	bool bAmp;

	do{
       pSubMenu = NULL;
		add = 1;
		bAmp = false;
		flags = *((WORD *)res);
       	res += sizeof(WORD);
		id = 0;
       	if(!(flags & MF_POPUP)){
        	id = *((WORD *)res);
            res += sizeof(WORD);
        }
        str = (short *)res;
		for(i=0;i<300;i++){
			if(str[i] == 0)
				break;
			if((char)str[i] == '&'){
				c[i] = '_';
				bAmp = true;
				continue;
			}
			c[i] = str[i];
		}
		c[i] = 0;
		res += (i+1) * sizeof(short);
        if(flags & MF_SEPARATOR)
			pItem = gtk_separator_menu_item_new();
		else if(flags & MF_BITMAP)
			pItem = gtk_image_menu_item_new();
		else if((flags & MF_POPUP)){			
			id = 1;
			pItem = gtk_menu_item_new_with_mnemonic(c);
			pSubMenu = gtk_menu_new();
			gtk_menu_item_set_submenu((GtkMenuItem *)pItem,pSubMenu);
            if(!(res = ParseMenu(res,(HMENU)pSubMenu)))
                return NULL;
		}
		else if(flags & MF_CHECKED){
			if(!bAmp)
				pItem = gtk_check_menu_item_new_with_label(c);
			else
				pItem = gtk_check_menu_item_new_with_mnemonic(c);
			gtk_check_menu_item_set_active((GtkCheckMenuItem *)pItem,true);
		}
		else if((flags & 0xF) < 4){
			if(id){
				if(bAmp)
					pItem = gtk_menu_item_new_with_mnemonic(c);
				else
					pItem = gtk_menu_item_new_with_label(c);
			}
			else{
				pItem = gtk_separator_menu_item_new();
				add = 0;
			}
		}
		if(pItem == NULL)
			continue;
		if(id){
			if(pSubMenu == NULL)
				SetWindowLong(pItem,GWL_SIGNAL,(LONG)g_signal_connect_after((gpointer)pItem, "activate",G_CALLBACK (MenuSelectDone),NULL));
			else{				
				g_signal_connect_after((gpointer)pItem,"select",G_CALLBACK (MenuSelectDone),(gpointer)1);
				if(GTK_IS_MENU_BAR(pParent))
					g_signal_connect_after((gpointer)pItem,"deselect",G_CALLBACK (MenuSelectDone),(gpointer)2);
			}
		}
		if(add){
			if(pSubMenu == NULL){
				SetWindowLong(pItem,GWL_ID,id);
			}
			else
				SetWindowLong(pSubMenu,GWL_ID,-1);
		}
		gtk_widget_show((HWND)pItem);
		gtk_container_add((GtkContainer *)pParent,pItem);
	}while(!(flags & 0x80));
	return res;
}
//---------------------------------------------------------------------------
HMENU LoadMenu(HINSTANCE instance,LPCTSTR name)
{
	char *p;
	GtkMenuBar *pMenu;

	p = (char *)FindResource(NULL,name,(char *)RT_MENU);
	p = (char *)LockResource(p);
	if(p == NULL)
		return NULL;
    if(*((WORD *)p) != 0)
    	return NULL;
    p += sizeof(WORD);
    p += sizeof(WORD) + *((WORD *)p);
	pMenu = (GtkMenuBar *)gtk_menu_new();
	gtk_widget_show ((HWND)pMenu);
	ParseMenu(p,(HMENU)pMenu);
	return (HMENU)pMenu;
}
//---------------------------------------------------------------------------
HMENU LoadMenuBar(HINSTANCE instance,LPCTSTR name)
{
	char *p;
	GtkMenuBar *pMenu;

	p = (char *)FindResource(NULL,name,(char *)RT_MENU);
	p = (char *)LockResource(p);
	if(p == NULL)
		return NULL;
    if(*((WORD *)p) != 0)
    	return NULL;
    p += sizeof(WORD);
    p += sizeof(WORD) + *((WORD *)p);
	pMenu = (GtkMenuBar *)gtk_menu_bar_new();
	gtk_widget_show ((HWND)pMenu);
	ParseMenu(p,(HMENU)pMenu);	
	return (HMENU)pMenu;
}
//--------------------------------------------------------------------------------
static void MenuItemSearchFromID(GtkWidget *w,gpointer data)
{
	LPDWORD pd,pUserData;
	GtkWidget *pSubMenu;
	DWORD dw;

	pd = (LPDWORD)data;
	if(pd == NULL || pd[1] != NULL)
		return;
	pd[3]++;
	pSubMenu = gtk_menu_item_get_submenu((GtkMenuItem *)w);
	if(pSubMenu != NULL){
		dw = pd[3];
		pd[3] = 0;
		gtk_container_forall((GtkContainer *)pSubMenu,MenuItemSearchFromID,(gpointer)data);
		pd[3] = dw;
		return;
	}	
	if(GetWindowLong(w,GWL_ID) != pd[0])
		return;
	pd[1] = (DWORD)w;
	pd[2] = pd[3];
}
//--------------------------------------------------------------------------------
BOOL IsCheckedMenuItem(HMENU menu,UINT uIDCheckItem)
{
	DWORD dw[4];
	
 	if(menu == NULL)
       return false;
	dw[1] = dw[2] = dw[3] = 0;
	dw[0] = uIDCheckItem;
	gtk_container_forall((GtkContainer *)menu,MenuItemSearchFromID,(gpointer)dw);
	if(dw[1] == 0 || !GTK_IS_CHECK_MENU_ITEM(dw[1]))
		return false;
	return gtk_check_menu_item_get_active((GtkCheckMenuItem *)dw[1]);
}
//--------------------------------------------------------------------------------
static DWORD checkMenuItem(LPDWORD dw,UINT uIDCheckItem,UINT uCheck)
{
	GtkContainer *pParent;
	HWND label,hwnd;
	DWORD dwOldState;
	bool bNew;

	if(!GTK_IS_CHECK_MENU_ITEM(dw[1])){
		pParent = (GtkContainer *)gtk_widget_get_parent((HWND)dw[1]);
		if(pParent == NULL)
			return 0xFFFFFFFF;
		label = gtk_bin_get_child((GtkBin *)dw[1]);
		if(label == NULL)
			return 0xFFFFFFFF;
		hwnd = gtk_check_menu_item_new_with_label(gtk_label_get_text((GtkLabel *)label));
		if(!GTK_WIDGET_IS_SENSITIVE((HWND)dw[1]))
			gtk_widget_set_sensitive(hwnd,false);
       	SetWindowLong(hwnd,GWL_ID,GetWindowLong((HWND)dw[1],GWL_ID));
       	SetWindowLong(hwnd,GWL_NOTIFYPARENT,GetWindowLong((HWND)dw[1],GWL_NOTIFYPARENT));
		gtk_widget_show((HWND)hwnd);
		gtk_menu_shell_insert((GtkMenuShell *)pParent,hwnd,dw[2]);
		gtk_container_remove(pParent,(HWND)dw[1]);		
		dw[1] = (DWORD)hwnd;
       	dwOldState = MF_UNCHECKED;
		SetWindowLong(hwnd,GWL_SIGNAL,(LONG)g_signal_connect_after((gpointer)hwnd, "activate",G_CALLBACK (MenuSelectDone),NULL));
	}
   	else
       dwOldState = gtk_check_menu_item_get_active((GtkCheckMenuItem *)dw[1]) ? MF_CHECKED : MF_UNCHECKED;
	g_signal_handler_block((gpointer)dw[1],GetWindowLong((HWND)dw[1],GWL_SIGNAL));
	gtk_check_menu_item_set_active((GtkCheckMenuItem *)dw[1],(uCheck & MF_CHECKED) ? true : false);
	g_signal_handler_unblock((gpointer)dw[1],GetWindowLong((HWND)dw[1],GWL_SIGNAL));
   	return dwOldState;
}
//--------------------------------------------------------------------------------
DWORD CheckMenuItem(HMENU menu,UINT uIDCheckItem,UINT uCheck)
{
	MENUITEMINFO mii;
  	UINT old;
	BOOL bPosition;
  	DWORD dw[4],dwOldState;
	GtkContainer *pParent;
	HWND hwnd,label;
	GList *p;

  	if(menu == NULL)
       return 0xFFFFFFFF;
	dw[1] = dw[2] = dw[3] = 0;
	if(uCheck & MF_BYPOSITION){
		p = gtk_container_get_children((GtkContainer *)menu);
		if(p == NULL)
			return 0xFFFFFFFF;
		dw[1] = (DWORD)g_list_nth_data(p,uIDCheckItem);
		dw[2] = uIDCheckItem;
	}
	else{
		dw[0] = uIDCheckItem;
		gtk_container_forall((GtkContainer *)menu,MenuItemSearchFromID,(gpointer)dw);
	}
	if(dw[1] == 0)
		return 0xFFFFFFFF;
	return checkMenuItem(dw,uIDCheckItem,uCheck);
}
//--------------------------------------------------------------------------------
BOOL DeleteMenu(HMENU menu,UINT uPosition,UINT uFlags)
{
	GList *p;
	DWORD dw;

	if(menu == NULL)
		return FALSE;
	if(uFlags & MF_BYPOSITION){
		p = gtk_container_get_children((GtkContainer *)menu);
		if(p == NULL)
			return FALSE;   
		dw = (DWORD)g_list_nth_data(p,uPosition);
	}
	else
		dw = 0;
	if(dw == 0)
		return FALSE;
	gtk_container_remove((GtkContainer *)menu,(HWND)dw);
	return TRUE;
}
//--------------------------------------------------------------------------------
BOOL AppendMenu(HMENU menu,UINT uFlags,UINT uIDNewItem,LPCTSTR lpNewItem)
{
	MENUITEMINFO mii={0};
	GList *pList;
	UINT uItem;

	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_TYPE|MIIM_STATE|MIIM_ID;
	mii.fType = (uFlags & (MF_BITMAP|MF_SEPARATOR|MF_OWNERDRAW|MFT_MENUBREAK|MFT_MENUBARBREAK));
	mii.dwTypeData = lpNewItem;
	mii.fState = (uFlags & (MF_DISABLED|MF_GRAYED));
	mii.wID = uIDNewItem;
	pList = gtk_container_get_children((GtkContainer *)menu);
	if(pList != NULL)			
		uItem = g_list_length(pList);
	else
		uItem = 0;
	return InsertMenuItem(menu,uItem,TRUE,&mii);
}
//--------------------------------------------------------------------------------
BOOL EnableMenuItem(HMENU menu,UINT uIDEnableItem,UINT uEnable)
{
	DWORD dw[4];
	bool value;
	GList *p;
	
	if(menu == NULL)
		return FALSE;
	dw[1] = dw[2] = dw[3] = 0;
	if(uEnable & MF_BYPOSITION){
		return FALSE;
		p = gtk_container_get_children((GtkContainer *)menu);
		if(p == NULL)
			return FALSE;
		dw[1] = (DWORD)g_list_nth_data(p,uIDEnableItem);
	}
	else{
		dw[0] = uIDEnableItem;
		dw[1] = 0;
		gtk_container_forall((GtkContainer *)menu,MenuItemSearchFromID,(gpointer)dw);
	}
	if(dw[1] == 0)
		return FALSE;
	value = (uEnable & (MF_GRAYED | MF_DISABLED)) ? false : true;
	gtk_widget_set_sensitive((GtkWidget *)dw[1],(gboolean)value);
	return TRUE;
}
//--------------------------------------------------------------------------------
HMENU GetSubMenu(HMENU menu,int nPos)
{
	GList *p;
	GtkMenuItem *pItem;
	HWND submenu;

	if(menu == NULL)
		return NULL;
	p = gtk_container_get_children((GtkContainer *)menu);
	if(p == NULL)
		return NULL;
	pItem = (GtkMenuItem *)g_list_nth_data(p,nPos);
	if(pItem == NULL || (submenu = gtk_menu_item_get_submenu(pItem)) == NULL)
		return NULL;
	return (HMENU)submenu;
}
//--------------------------------------------------------------------------------
BOOL InsertMenuItem(HMENU menu,UINT uItem,BOOL fByPosition,LPMENUITEMINFO lpmii)
{
	HWND pItem,hwnd;
	bool bSignal,bAmp;
   char *p;
   UINT i;
	DWORD dw[4];
	GList *pList;

	if(menu == NULL || lpmii == NULL)
		return FALSE;
	pItem = NULL;
	bSignal = false;
	if(lpmii->fMask & MIIM_TYPE){
		switch(lpmii->fType){
			case MFT_OWNERDRAW:
				pItem = gtk_image_menu_item_new();
				g_signal_connect((gpointer)pItem,"expose-event",G_CALLBACK (OnMenuOwnerDraw),(gpointer)NULL);
				bSignal = true;
			break;
			case MFT_SEPARATOR:
				pItem = gtk_separator_menu_item_new();
			break;
			case MFT_STRING:
				if(lpmii->dwTypeData != 0){
                   p = (char *)lpmii->dwTypeData;
                   bAmp = false;
                   for(i=0;i<lstrlen(p) && *p != 0;i++,p++){
                       if(*p == '&'){
                           bAmp = true;
                           *p = '_';
                       }
                   }
					if((lpmii->fMask & MIIM_STATE) && (lpmii->fState & MF_CHECKED)){
                       if(bAmp)
                           pItem = gtk_check_menu_item_new_with_mnemonic((char *)lpmii->dwTypeData);
                       else
						    pItem = gtk_check_menu_item_new_with_label((char *)lpmii->dwTypeData);
						gtk_check_menu_item_set_active((GtkCheckMenuItem *)pItem,true);
					}
					else{
                       if(!bAmp)
                           pItem = gtk_menu_item_new_with_label((char *)lpmii->dwTypeData);
                       else
						    pItem = gtk_menu_item_new_with_mnemonic((char *)lpmii->dwTypeData);
                   }
					bSignal = true;
				}
			break;
		}
	}
	if(pItem == NULL)
		return FALSE;
	if((lpmii->fMask & MIIM_STATE) && (lpmii->fState & (MF_DISABLED|MF_GRAYED)))
		gtk_widget_set_sensitive(pItem,false);
	if(lpmii->fMask & MIIM_DATA)
		SetWindowLong(pItem,GWL_USERDATA,(LONG)lpmii->dwItemData);
	gtk_widget_show((HWND)pItem);
	if(lpmii->fMask & MIIM_ID)
		SetWindowLong(pItem,GWL_ID,(LONG)lpmii->wID);
	pList = gtk_container_get_children((GtkContainer *)menu);
	if(!fByPosition){
		dw[0] = uItem;
		dw[1] = dw[2] = dw[3] = 0;
		gtk_container_forall((GtkContainer *)menu,MenuItemSearchFromID,(gpointer)dw);
		if(dw[1] != 0){
			uItem = dw[2]-1;
		}
		else{
			if(pList != NULL)			
				uItem = g_list_length(pList);
			else
				uItem = 0;
		}
	}
	gtk_menu_shell_insert((GtkMenuShell *)menu,pItem,uItem);
	if(bSignal){
		if(pList != NULL)
			SetWindowLong(pItem,GWL_NOTIFYPARENT,(LONG)GetWindowLong((HWND)g_list_nth_data(pList,0),GWL_NOTIFYPARENT));
		else{
			SetWindowLong(pItem,GWL_NOTIFYPARENT,(LONG)GetWindowLong((HWND)menu,GWL_NOTIFYPARENT));
		}
		SetWindowLong(pItem,GWL_SIGNAL,(LONG)g_signal_connect_after((gpointer)pItem,"activate",G_CALLBACK (MenuSelectDone),NULL));		
	}
	return TRUE;
}
//--------------------------------------------------------------------------------
BOOL GetMenuItemInfo(HMENU menu,UINT uItem,BOOL fByPosition,LPMENUITEMINFO lpmii)
{
	GList *p;
	DWORD dw[4];
	HWND hwnd;
	gchar *text;
	GList *pList;

	if(menu == NULL || lpmii == NULL)
		return FALSE;
	dw[1] = dw[2] = dw[3] = 0;
	if(fByPosition){
		p = gtk_container_get_children((GtkContainer *)menu);
		if(p == NULL)
			return FALSE;
		dw[1] = (DWORD)g_list_nth_data(p,uItem);
	}
	else{
		dw[0] = uItem;
		dw[1] = 0;
		gtk_container_forall((GtkContainer *)menu,MenuItemSearchFromID,(gpointer)dw);
	}
	if(dw[1] == 0)
		return FALSE;
	if(lpmii->fMask & MIIM_STATE){
		lpmii->fState = 0;
		if(!GTK_WIDGET_IS_SENSITIVE(dw[1]))
			lpmii->fState |= MF_GRAYED;
		if(GTK_IS_CHECK_MENU_ITEM(dw[1])){
			if(gtk_check_menu_item_get_active((GtkCheckMenuItem*)dw[1]))
				lpmii->fState |= MF_CHECKED;
		}
	}
   if(lpmii->fMask & MIIM_ID)
       lpmii->wID = GetWindowLong((HWND)dw[1],GWL_ID);
   if(lpmii->fMask & MIIM_DATA)
       lpmii->dwItemData = GetWindowLong((HWND)dw[1],GWL_USERDATA);
   if(lpmii->fMask & MIIM_TYPE){
		if(lpmii->fType == MFT_STRING && lpmii->dwTypeData != 0){
			hwnd = gtk_bin_get_child((GtkBin *)(GtkMenuItem *)dw[1]);
			if(hwnd != NULL){
				text = (gchar *)gtk_label_get_text((GtkLabel *)hwnd);
				if(text != NULL)
					lstrcpyn(lpmii->dwTypeData,text,lpmii->cch);			
			}		
		}
   }
	return TRUE;
}
//--------------------------------------------------------------------------------
int GetMenuItemCount(HMENU menu)
{
	GList *p;

	if(menu == NULL)
		return 0;
	p = gtk_container_get_children((GtkContainer *)menu);
	if(p == NULL)
		return 0;
	return (int)g_list_length(p);
}
//--------------------------------------------------------------------------------
BOOL DestroyMenu(HMENU menu)
{
	return FALSE;
}
//--------------------------------------------------------------------------------
BOOL SetMenuItemInfo(HMENU menu,UINT uItem,BOOL fByPosition,LPMENUITEMINFO lpmii)
{
	GList *p;
	DWORD dw[4];
	bool value;
	UINT check;

	if(menu == NULL || lpmii == NULL)
		return FALSE;
	dw[1] = dw[2] = dw[3] = 0;
	if(fByPosition){
		p = gtk_container_get_children((GtkContainer *)menu);
		if(p == NULL)
			return FALSE;
		dw[1] = (DWORD)g_list_nth_data(p,uItem);
		dw[2] = uItem;
	}
	else{
		dw[0] = uItem;
		dw[1] = 0;
		gtk_container_forall((GtkContainer *)menu,MenuItemSearchFromID,(gpointer)dw);
	}
	if(dw[1] == 0)
		return FALSE;
	if((lpmii->fMask & MIIM_STATE)){
		if((lpmii->fState & MF_CHECKED))
			checkMenuItem(dw,uItem,MF_CHECKED);		
		else{
			if(GTK_IS_CHECK_MENU_ITEM(dw[1]))
				checkMenuItem(dw,uItem,lpmii->fState & MF_CHECKED);
		}	
		value = (lpmii->fState & (MF_GRAYED | MF_DISABLED)) ? false : true;
		gtk_widget_set_sensitive((GtkWidget *)dw[1],(gboolean)value);
	}
	return TRUE;
}
//--------------------------------------------------------------------------------
BOOL SetMenu(HWND hWnd,HMENU hMenu)
{
	DWORD dw[2];

	dw[0] = (DWORD)hWnd;
	dw[1] = (DWORD)hWnd;
	gtk_container_forall((GtkContainer *)hMenu,MenuItemsEnum,(gpointer)dw);
	SetWindowLong(hWnd,GWL_MENU,(LONG)hMenu);
	return TRUE;
}
//--------------------------------------------------------------------------------
HMENU CreatePopupMenu()
{
	return NULL;
}
//--------------------------------------------------------------------------------
HMENU GetMenu(HWND hWnd)
{
	return (HMENU)GetWindowLong(hWnd,GWL_MENU);
}
//--------------------------------------------------------------------------------
BOOL DrawMenuBar(HWND hWnd)
{
	if(hWnd == NULL || (hWnd = (HWND)GetWindowLong(hWnd,GWL_MENU)) == NULL)
		return FALSE;
	if(!GTK_IS_MENU_BAR(hWnd))
		return FALSE;
	gtk_widget_queue_draw(hWnd);
	return TRUE;
}
//--------------------------------------------------------------------------------
BOOL TrackPopupMenu(HMENU hMenu,UINT uFlags,int x,int y,int nReserved,HWND hWnd,CONST RECT *prcRect)
{
	DWORD dw[2];

	dw[0] = (DWORD)hWnd;
	dw[1] = (DWORD)hWnd;
	gtk_container_forall((GtkContainer *)hMenu,MenuItemsEnum,(gpointer)dw);
	gtk_menu_popup((GtkMenu *)hMenu,NULL,NULL,NULL,NULL,0,0);
	return FALSE;
}
//--------------------------------------------------------------------------------
BOOL CheckMenuRadioItem(HMENU hMenu,UINT idFirst,UINT idLast,UINT idCheck,UINT uFlags)
{
	guint id;
	DWORD dw[4];

	for(;idFirst <=idLast;idFirst++){
		dw[0] = idFirst;
		dw[1] = 0;
		gtk_container_forall((GtkContainer *)hMenu,MenuItemSearchFromID,(gpointer)dw);
		if(dw[1] == 0)
			continue;
		id = GetWindowLong((HWND)dw[1],GWL_ID);
		CheckMenuItem(hMenu,id,id == idCheck ? MF_CHECKED : MF_UNCHECKED);
	}
	return TRUE;
}