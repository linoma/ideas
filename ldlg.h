#include "ideastypes.h"
#include "lwnd.h"
//---------------------------------------------------------------------------
#ifndef __ldlgH__
#define __ldlgH__

//---------------------------------------------------------------------------
class LDlg : public LWnd
{
public:
   LDlg();
   virtual ~LDlg();
  	BOOL Create(HINSTANCE hInstance,LPCTSTR lpTemplate,HWND hParent = NULL);
	BOOL CreateEmpty(HINSTANCE hInstance,LPCTSTR lpWindowName,DWORD dwExStyle,DWORD dwStyle,int x,int y,int cx,int cy,HWND hParent = NULL);
  	BOOL CreateIndirect(HINSTANCE hInstance,LPCDLGTEMPLATE lpTemplate,HWND hParent = NULL);
  	virtual BOOL Destroy();
  	virtual LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   BOOL CreateModal(HINSTANCE hInstance,LPCTSTR lpTemplate,HWND hParent = NULL);
protected:
  	BOOL bDoModal;
};
//---------------------------------------------------------------------------
#endif
