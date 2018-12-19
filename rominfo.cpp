#include "rominfo.h"
#include "resource.h"
#include "language.h"
#include "lds.h"

//---------------------------------------------------------------------------
LRomInfo::LRomInfo() : LDlg()
{
}
//---------------------------------------------------------------------------
LRomInfo::~LRomInfo()
{
}
//---------------------------------------------------------------------------
BOOL LRomInfo::Show(HWND parent)
{
	if(!LDlg::CreateModal(langManager.get_CurrentLib(),MAKEINTRESOURCE(IDD_DIALOG24),parent))
   	return FALSE;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LRomInfo::OnInitDialog()
{
   TC_ITEM tci={TCIF_TEXT | TCIF_PARAM|TCIF_IMAGE,0,0,NULL,0,0,NULL};
	LRomReader *p;
	char *buf,*c;
   DWORD dw;
   int i;

	p = ds.get_RomReader();
   if(p == NULL)
   	return FALSE;
   buf = (char *)LocalAlloc(LPTR,5000);
   if(buf == NULL)
   	return FALSE;
   dw = p->get_Info(buf,4999);
   if(dw == 0)
   	goto OnInitDialog_0;
   SetDlgItemText(m_hWnd,IDC_EDIT2,buf);
	c = buf + lstrlen(buf) + 1;
 	tci.pszText = "JPN";
	SendDlgItemMessage(m_hWnd,IDC_TAB1,TCM_INSERTITEM,(i=0),(LPARAM)&tci);
 	tci.pszText = "USA";
	SendDlgItemMessage(m_hWnd,IDC_TAB1,TCM_INSERTITEM,++i,(LPARAM)&tci);
 	tci.pszText = "FRA";
	SendDlgItemMessage(m_hWnd,IDC_TAB1,TCM_INSERTITEM,++i,(LPARAM)&tci);
 	tci.pszText = "GER";
	SendDlgItemMessage(m_hWnd,IDC_TAB1,TCM_INSERTITEM,++i,(LPARAM)&tci);
 	tci.pszText = "ITA";
	SendDlgItemMessage(m_hWnd,IDC_TAB1,TCM_INSERTITEM,++i,(LPARAM)&tci);
 	tci.pszText = "SPA";
	SendDlgItemMessage(m_hWnd,IDC_TAB1,TCM_INSERTITEM,++i,(LPARAM)&tci);

   for(i=0;i<6;i++){
   	SetDlgItemText(m_hWnd,6022+i,c);
		c += lstrlen(c)+1;
   }
	SetDlgItemText(m_hWnd,IDC_EDIT1,c);
   p->DrawIcon(GetDlgItem(m_hWnd,IDC_IMAGE1));
OnInitDialog_0:
   LocalFree(buf);
   return TRUE;
}
//---------------------------------------------------------------------------
LRESULT LRomInfo::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int i;

	switch(uMsg){
   	case WM_INITDIALOG:
       	OnInitDialog();
       break;
       case WM_NOTIFY:
       	if(wParam == IDC_TAB1 && ((LPNMHDR)lParam)->code == TCN_SELCHANGE){
   			for(i=0;i<6;i++)
					ShowWindow(GetDlgItem(m_hWnd,IDC_CHECK2+i),SW_HIDE);
               ShowWindow(GetDlgItem(m_hWnd,IDC_CHECK2+TabCtrl_GetCurSel(((LPNMHDR)lParam)->hwndFrom)),SW_SHOW);
           }
       break;
   	case WM_COMMAND:
       	switch(LOWORD(wParam)){
           	case IDOK:
               	::EndDialog(m_hWnd,1);
               break;
           }
       break;
       case WM_CLOSE:
           ::EndDialog(m_hWnd,0);
       break;
   }
   return FALSE;
}


