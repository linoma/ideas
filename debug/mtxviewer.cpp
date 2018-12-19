#include "mtxviewer.h"
#include "resource.h"
#include "lds.h"
#include "3d.h"
#include "videoplug.h"

#ifdef _DEBUG
//---------------------------------------------------------------------------
LMatrixViewer::LMatrixViewer() : LDlg()
{
}
//---------------------------------------------------------------------------
LMatrixViewer::~LMatrixViewer()
{
   Destroy();
}
//---------------------------------------------------------------------------
BOOL LMatrixViewer::Show(HWND parent)
{
   if(m_hWnd == NULL){
       if(!LDlg::Create(hInst,MAKEINTRESOURCE(IDD_DIALOG20),parent))
           return FALSE;
   }
   else{
       ::BringWindowToTop(m_hWnd);
       ::InvalidateRect(m_hWnd,NULL,TRUE);
       ::UpdateWindow(m_hWnd);
   }
   return TRUE;
}
//---------------------------------------------------------------------------
void LMatrixViewer::Update()
{
   int index,i;
   char s[50];
   float *f;
   PlugIn *p;
   void *p1[3];

   index = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
   i = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_GETCURSEL,0,0);
   p1[0] = NULL;
   if((p = pPlugInContainer->get_PlugInList(PIL_VIDEO)->get_ActivePlugIn(PIT_IS3D)) != NULL){
       p1[1] = (void *)i;
       p1[2] = (void *)index;
       p->NotifyState(PNMV_GETSTACK,PIS_NOTIFYMASK,(LPARAM)p1);
   }
   if(p1[0] != NULL){
       if(index == 0)
           f = (float *)p1[0];
       else
           f = (float *)(((stack_MTX *)p1[0])->get_matrix()) + (index - 1) * 16;
   }
   else
       f = NULL;
   index = SendDlgItemMessage(m_hWnd,IDC_CHECK1,BM_GETCHECK,0,0) == BST_CHECKED ? 1 : 0;
   for(i=0;i<16;i++){
       if(f != NULL){
           if(!index)
               sprintf(s,"%f",f[i]);
           else
               sprintf(s,"0x%08X",(int)(f[i] * 4096.0f));
       }
       else
           s[0] = 0;
       SetDlgItemText(m_hWnd,1000+i,s);
   }
}
//---------------------------------------------------------------------------
void LMatrixViewer::OnCommand(WORD wID,WORD wNotifyCode,HWND hwnd)
{
   int index,i;
   char s[50];
   PlugIn *p;
   void *p1[3];

   switch(wID){
       case IDC_COMBOBOX1:
           switch(wNotifyCode){
               case CBN_SELENDOK:
                   index = SendDlgItemMessage(m_hWnd,wID,CB_GETCURSEL,0,0);
                   SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_RESETCONTENT,0,0);
                   SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"Current");
                   switch(index){
                       case 0:
                       case 1:
                       case 2:
                           p1[0] = NULL;
                           if((p = pPlugInContainer->get_PlugInList(PIL_VIDEO)->get_ActivePlugIn(PIT_IS3D)) != NULL){
                               p1[1] = (void *)index;
                               p1[2] = (void *)1;
                               p->NotifyState(PNMV_GETSTACK,PIS_NOTIFYMASK,(LPARAM)p1);
                           }
                           if(p1[0] != NULL)
                               index = ((stack_MTX *)p1[0])->get_size();
                           else
                               index = -1;
                           for(i=0;i<=index;i++){
                               wsprintf(s,"Index %d",i+1);
                               SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)s);
                           }
                       break;
                   }
                   SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_SETCURSEL,-1,0);
                   for(i=0;i<16;i++)
                       SetDlgItemText(m_hWnd,1000+i,"");
               break;
           }
       break;
       case IDC_COMBOBOX2:
           switch(wNotifyCode){
               case CBN_SELENDOK:
					Update();
               break;
           }
       break;
       case IDC_CHECK1:
           if(wNotifyCode == BN_CLICKED)
               ::SendMessage(m_hWnd,WM_COMMAND,MAKEWPARAM(IDC_COMBOBOX2,CBN_SELENDOK),(LPARAM)GetDlgItem(m_hWnd,IDC_COMBOBOX2));
       break;
       case IDOK:
           if(wNotifyCode == BN_CLICKED)
               Destroy();
       break;
   }
}
//---------------------------------------------------------------------------
LRESULT LMatrixViewer::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   BOOL res;

   res = FALSE;
   switch(uMsg){
       case WM_INITDIALOG:
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"Projection");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"ModelView");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"Position");
           SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"Texture");
       break;
       case WM_COMMAND:
           OnCommand(LOWORD(wParam),HIWORD(wParam),(HWND)lParam);
       break;
       case WM_CLOSE:
           Destroy();
       break;
   }
   return res;
}
#endif
