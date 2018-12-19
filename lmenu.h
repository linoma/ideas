#include "ideastypes.h"

#ifndef __lmenuh__
#define __lmenuh__
//--------------------------------------------------------------------------------
class LPopupMenu : public IMenu
{
public:
	LPopupMenu();
   virtual ~LPopupMenu();
	BOOL DeleteMenu(UINT uPosition,UINT uFlags);
	BOOL AppendMenu(UINT uFlags,UINT uIDNewItem,LPCTSTR lpNewItem);
   BOOL EnableMenuItem(UINT uIDEnableItem,UINT uEnable);
   DWORD CheckMenuItem(UINT uIDCheckItem,UINT uCheck);
   BOOL IsCheckedMenuItem(UINT uIDCheckItem);
   BOOL Attack(HMENU h);
   BOOL Destroy();
   int GetMenuItemCount(){return ::GetMenuItemCount(menu);};
   IMenu *GetSubMenu(int nPos);
   BOOL InsertMenuItem(UINT uItem,BOOL fByPosition,LPMENUITEMINFO lpmii){return ::InsertMenuItem(menu,uItem,fByPosition,lpmii);};
	void Release(){delete this;};
   inline HMENU Handle(){return menu;};
   BOOL GetMenuItemInfo(UINT uItem,BOOL fByPosition,LPMENUITEMINFO lpmii){return ::GetMenuItemInfo(menu,uItem,fByPosition,lpmii);};
protected:
   HMENU menu;
   BOOL bAttack;
};

#endif

