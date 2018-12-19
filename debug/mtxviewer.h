#include "ideastypes.h"
#include "dstype.h"
#include "ldlg.h"

//---------------------------------------------------------------------------
#ifndef __mtxviewerH__
#define __mtxviewerH__
//---------------------------------------------------------------------------
#ifdef _DEBUG
class LMatrixViewer : public LDlg
{
public:
   LMatrixViewer();
   ~LMatrixViewer();
   BOOL Show(HWND parent = NULL);
   void Update();
protected:
   void OnCommand(WORD wID,WORD wNotifyCode,HWND hwnd);
   LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
};
#endif
#endif


 