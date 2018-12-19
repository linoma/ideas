#include "ideastypes.h"
#include "ldlg.h"

//---------------------------------------------------------------------------
#ifndef __rominfoH__
#define __rominfoH__
//---------------------------------------------------------------------------

class LRomInfo : public LDlg
{
public:
   LRomInfo();
   ~LRomInfo();
   BOOL Show(HWND parent = NULL);
protected:
	LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   BOOL OnInitDialog();
};
#endif
 