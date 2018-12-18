#include "ldlg.h"
#include "lapp.h"

//---------------------------------------------------------------------------
LDlg::LDlg() : LWnd()
{
	bDoModal = FALSE;
}
//---------------------------------------------------------------------------
LDlg::~LDlg()
{
	Destroy();
}
//---------------------------------------------------------------------------
BOOL LDlg::Destroy()
{
	BOOL res;

	if(m_hWnd == NULL)
   	return TRUE;
   if(bDoModal)
   	res = EndDialog(m_hWnd,0);
   else
   	res = ::DestroyWindow(m_hWnd);
   if(res)
   	m_hWnd = NULL;
   return res;
}
//---------------------------------------------------------------------------
LRESULT LDlg::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	return FALSE;
}
//---------------------------------------------------------------------------
BOOL LDlg::CreateModal(HINSTANCE hInstance,LPCTSTR lpTemplate,HWND hParent)
{
#ifdef __WIN32__
	if(bDoModal && m_hWnd != NULL && ::IsWindow(m_hWnd))
   		return TRUE;
	bDoModal = TRUE;
	return ::DialogBoxParam(hInstance,lpTemplate,hParent,(DLGPROC)WindowProc,(LPARAM)this);
#else
	if(bDoModal && m_hWnd != NULL)
   		return TRUE;
	bDoModal = TRUE;
	return DialogBoxParam(hInstance,lpTemplate,hParent,(DLGPROC)WindowProc,(LPARAM)this);
#endif
}
//---------------------------------------------------------------------------
BOOL LDlg::Create(HINSTANCE hInstance,LPCTSTR lpTemplate,HWND hParent)
{
	if(!bDoModal && m_hWnd != NULL && ::IsWindow(m_hWnd))
   		return TRUE;
	bDoModal = FALSE;
	m_hWnd = CreateDialogParam(hInstance,lpTemplate,hParent,(DLGPROC)WindowProc,(LPARAM)this);
   if(m_hWnd == NULL)
       return FALSE;
   SetWindowLong(m_hWnd,GWL_USERDATA,(LONG)this);
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LDlg::CreateEmpty(HINSTANCE hInstance,LPCTSTR lpWindowName,DWORD dwExStyle,DWORD dwStyle,int x,int y,int cx,int cy,HWND hParent)
{
   LPDLGTEMPLATE lpdt;
   BOOL res;
   LPWORD lpw;

#ifdef __WIN32__	
   	HGLOBAL hgbl;
   	LONG l;
   	LPWSTR lpwsz;
   	
   	if(!bDoModal && m_hWnd != NULL && ::IsWindow(m_hWnd))
   		return TRUE;
	    hgbl = GlobalAlloc(GMEM_ZEROINIT, 1024);
       if(!hgbl)
		    return FALSE;
   	lpdt = (LPDLGTEMPLATE)GlobalLock(hgbl);
   	lpdt->style = dwStyle;
   	lpdt->x  = (short)x;
   	lpdt->y  = (short)y;
   	lpdt->cx = (short)cx;
   	lpdt->cy = (short)cy;
   	lpw = (LPWORD) (lpdt + 1);
   	*lpw++ = 0;
   	*lpw++ = 0;
   	lpwsz = (LPWSTR) lpw;
   	MultiByteToWideChar(CP_ACP,0,lpWindowName,-1,lpwsz, 50);
   	res = CreateIndirect(hInstance,lpdt,hParent);
   	GlobalUnlock(lpdt);
   	GlobalFree(hgbl);
   	if(res && dwExStyle){
   		l = GetWindowLong(m_hWnd,GWL_EXSTYLE);
       	l |= dwExStyle;
       	SetWindowLong(m_hWnd,GWL_EXSTYLE,l);
       	::SetWindowPos(m_hWnd,0,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSENDCHANGING|SWP_FRAMECHANGED|SWP_NOACTIVATE);
   	}
#else
   	int i;

   	if(!bDoModal && m_hWnd != NULL)
   		return TRUE;
   	if((lpdt = (LPDLGTEMPLATE)malloc(1024)) == NULL)
   		return FALSE;
   	lpdt->style = dwStyle;
   	lpdt->x  = x;
   	lpdt->y  = y;
   	lpdt->cx = cx;
   	lpdt->cy = cy;
   	lpw = (LPWORD) (lpdt + 1);
   	*lpw++ = 0;
   	*lpw++ = 0;
   	for(i=0;*lpWindowName != 0;i++)
		    *lpw++ = *lpWindowName++;
  	    res = CreateIndirect(hInstance,lpdt,hParent);
       free(lpdt);
#endif
	return res;
}
//---------------------------------------------------------------------------
BOOL LDlg::CreateIndirect(HINSTANCE hInstance,LPCDLGTEMPLATE lpTemplate,HWND hParent)
{
#ifdef __WIN32__
	if(!bDoModal && m_hWnd != NULL && ::IsWindow(m_hWnd))
   		return TRUE;
	bDoModal = FALSE;
	m_hWnd = CreateDialogIndirectParam(hInstance,lpTemplate,hParent,(DLGPROC)WindowProc,(LPARAM)this);
   if(m_hWnd == NULL)
       return FALSE;
   SetWindowLong(m_hWnd,GWL_USERDATA,(LONG)this);
#else
/*	LPWORD lpw;
	LPCTSTR lpWindowName;
	int i;
	char c[250];
			
	if(!bDoModal && m_hWnd != NULL && IsWindow(m_hWnd))
   	return TRUE;
	bDoModal = FALSE;
	if(hParent == NULL)
		hParent = DefaultRootWindow((Display *)hInstance);		
	lpw = (LPWORD)(lpTemplate + 1);
	for(i=0;i<250;i++){
		if(lpw[i+1] == 0)
			break;
		c[i] = lpw[i+1];
	}
	c[i] = 0;		
	if(!LWnd::Create("LDlg",c,lpTemplate->style,lpTemplate->x,lpTemplate->y,
		lpTemplate->cx,lpTemplate->cy,hParent,NULL,hInstance,NULL))
		return FALSE;
	XSelectInput(pApp->GetDesktopWindow(),m_hWnd,StructureNotifyMask);
	XSetWindowBackground(pApp->GetDesktopWindow(),m_hWnd,pApp->GetSysColor(COLOR_BTNFACE));	
	XMapSubwindows(pApp->GetDesktopWindow(),m_hWnd);	*/
#endif
   return TRUE;
}
