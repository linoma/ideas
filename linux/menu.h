#include "lwindow.h"

#ifndef __MENUH__

#define __MENUH__

HMENU LoadMenu(HINSTANCE instance,LPCTSTR name);
HMENU LoadMenuBar(HINSTANCE instance,LPCTSTR name);
BOOL IsCheckedMenuItem(HMENU menu,UINT uIDCheckItem);
DWORD CheckMenuItem(HMENU menu,UINT uIDCheckItem,UINT uCheck);
BOOL DeleteMenu(HMENU menu,UINT uPosition,UINT uFlags);
BOOL AppendMenu(HMENU menu,UINT uFlags,UINT uIDNewItem,LPCTSTR lpNewItem);
BOOL EnableMenuItem(HMENU menu,UINT uIDEnableItem,UINT uEnable);
HMENU GetSubMenu(HMENU menu,int nPos);
BOOL InsertMenuItem(HMENU menu,UINT uItem,BOOL fByPosition,LPMENUITEMINFO lpmii);
BOOL GetMenuItemInfo(HMENU menu,UINT uItem,BOOL fByPosition,LPMENUITEMINFO lpmii);
int GetMenuItemCount(HMENU menu);
BOOL DestroyMenu(HMENU menu);
BOOL SetMenuItemInfo(HMENU menu,UINT uItem,BOOL fByPosition,LPMENUITEMINFO lpmii);
void set_ParentInMenuItem(GtkContainer *container,HWND parent);
BOOL SetMenu(HWND hWnd,HMENU hMenu);
HMENU CreatePopupMenu();
HMENU GetMenu(HWND hWnd);
BOOL DrawMenuBar(HWND hWnd);
BOOL CheckMenuRadioItem(HMENU hMenu,UINT,UINT,UINT,UINT);
BOOL TrackPopupMenu(HMENU hMenu,UINT uFlags,int x,int y,int nReserved,HWND hWnd,CONST RECT *prcRect);


#endif


