#include "lmenu.h"
#include "lapp.h"

//--------------------------------------------------------------------------------
LPopupMenu::LPopupMenu()
{
   menu = NULL;
   bAttack = FALSE;
}
//--------------------------------------------------------------------------------
LPopupMenu::~LPopupMenu()
{
   Destroy();
}
//--------------------------------------------------------------------------------
BOOL LPopupMenu::IsCheckedMenuItem(UINT uIDCheckItem)
{
   MENUITEMINFO mii;

   if(menu == NULL)
       return FALSE;
   ZeroMemory(&mii,sizeof(MENUITEMINFO));
   mii.cbSize = sizeof(MENUITEMINFO);
   mii.fMask = MIIM_STATE;
   if(!::GetMenuItemInfo(menu,uIDCheckItem,FALSE,&mii))
       return FALSE;
   return (BOOL)((mii.fState & MFS_CHECKED) ? TRUE : FALSE);
}
//--------------------------------------------------------------------------------
DWORD LPopupMenu::CheckMenuItem(UINT uIDCheckItem,UINT uCheck)
{
   MENUITEMINFO mii;
   UINT old;
	BOOL bPosition;

   if(menu == NULL)
       return 0xFFFFFFFF;
   bPosition = (BOOL)((uCheck & MF_BYPOSITION) ? TRUE : FALSE);
   ZeroMemory(&mii,sizeof(MENUITEMINFO));
   mii.cbSize = sizeof(MENUITEMINFO);
   mii.fMask = MIIM_STATE;
   if(!::GetMenuItemInfo(menu,uIDCheckItem,bPosition,&mii))
       return 0xFFFFFFFF;
   old = mii.fState & (MFS_CHECKED|MFS_UNCHECKED|MFS_DISABLED|MFS_ENABLED|MFS_GRAYED|MFS_HILITE|MFS_UNHILITE);
   mii.fState &= ~(MFS_CHECKED|MFS_UNCHECKED);
   mii.fState |= uCheck;
   if(SetMenuItemInfo(menu,uIDCheckItem,bPosition,&mii))
       return old;
   return 0xFFFFFFFF;
}
//--------------------------------------------------------------------------------
BOOL LPopupMenu::DeleteMenu(UINT uPosition,UINT uFlags)
{
   return ::DeleteMenu(menu,uPosition,uFlags);
}
//--------------------------------------------------------------------------------
BOOL LPopupMenu::AppendMenu(UINT uFlags,UINT uIDNewItem,LPCTSTR lpNewItem)
{
   return ::AppendMenu(menu,uFlags,uIDNewItem,lpNewItem);
}
//--------------------------------------------------------------------------------
BOOL LPopupMenu::Attack(HMENU h)
{
   if(h == NULL)
       return FALSE;
   menu = h;
   return (bAttack = TRUE);
}
//--------------------------------------------------------------------------------
BOOL LPopupMenu::Destroy()
{
	if(bAttack)
       return TRUE;
   if(menu == NULL || !::DestroyMenu(menu))
       return FALSE;
   menu = NULL;
	return TRUE;
}
//--------------------------------------------------------------------------------
BOOL LPopupMenu::EnableMenuItem(UINT uIDEnableItem,UINT uEnable)
{
/*   MENUITEMINFO mii={0};
   UINT old;
   BOOL bPosition;

   if(menu == NULL)
       return 0xFFFFFFFF;
   bPosition = (BOOL)((uEnable & MF_BYPOSITION) ? TRUE : FALSE);
   mii.cbSize = sizeof(MENUITEMINFO);
   mii.fMask = MIIM_STATE;
   if(!::GetMenuItemInfo(menu,uIDEnableItem,bPosition,&mii))
       return 0xFFFFFFFF;
   old = mii.fState & (MFS_CHECKED|MFS_UNCHECKED|MFS_DISABLED|MFS_ENABLED|MFS_GRAYED|MFS_HILITE|MFS_UNHILITE);
	memset(&mii,0,sizeof(mii));
   mii.cbSize = sizeof(MENUITEMINFO);
   mii.fMask = MIIM_STATE;
   mii.fState &= ~(MFS_DISABLED|MFS_ENABLED|MFS_GRAYED);
   mii.fState |= uEnable;
   if(SetMenuItemInfo(menu,uIDEnableItem,bPosition,&mii))
       return old;
   return 0xFFFFFFFF;*/
   return ::EnableMenuItem(menu,uIDEnableItem,uEnable);
}
//--------------------------------------------------------------------------------
IMenu *LPopupMenu::GetSubMenu(int nPos)
{
 	LPopupMenu *pMenu;
 	HMENU m;

   m = ::GetSubMenu(menu,nPos);
   if(m == NULL)
       return NULL;
 	if((pMenu = new LPopupMenu()) == NULL)
		return NULL;
	pMenu->Attack(m);
	return pMenu;
}



