#include "ioviewer.h"
#include "lds.h"
#include "util.h"
#include "io.h"

#ifdef _DEBPRO
//---------------------------------------------------------------------------
LIOViewer::LIOViewer() : LDlg()
{
   ZeroMemory(hWnd,sizeof(hWnd));
   pInfoMemList = NULL;
   currentInfoMem = NULL;
}
//---------------------------------------------------------------------------
LIOViewer::~LIOViewer()
{
   Destroy();
}
//---------------------------------------------------------------------------
BOOL LIOViewer::Show(HWND parent)
{
   if(m_hWnd == NULL){
       if(!LDlg::Create(hInst,MAKEINTRESOURCE(IDD_DIALOG5),parent))
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
BOOL LIOViewer::Destroy()
{
   DestroyControl();
   if(pInfoMemList != NULL)
       delete pInfoMemList;
   pInfoMemList = NULL;
   currentInfoMem = NULL;
   LDlg::Destroy();
}
//---------------------------------------------------------------------------
BOOL LIOViewer::OnInitDialog()
{
   int i;

   SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"Arm9");
   SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"Arm7");
   i = debugDlg.get_CurrentCPU()->r_index();
   SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_SETCURSEL,i == 9 ? 0 : 1,0);
   FillComboBox2();
   return FALSE;
}

//---------------------------------------------------------------------------
void LIOViewer::DestroyControl()
{
   int i;

   for(i=0;i<sizeof(hWnd)/sizeof(HWND);i++){
       if(hWnd[i] == NULL)
           continue;
       ::DestroyWindow(hWnd[i]);
       hWnd[i] = NULL;
   }
}
//---------------------------------------------------------------------------
void LIOViewer::OnSelChangeComboBox2(BOOL bUpdate)
{
   DWORD dwPos,dw,dwAddress;
   LPINFOMEM im;
   char s[100],s1[200];
   char *p,*p1;
   int i,len,i1,i2,bLoad,bInsert,i4,i5;
   u32 i3,bit;
   RECT rc;

   SendDlgItemMessage(m_hWnd,IDC_LABEL_REG_DESCR,WM_SETTEXT,0,(LPARAM)"");
   if(!bUpdate)
   	DestroyControl();
   else{

   }
   if(pInfoMemList == NULL || (i = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_GETCURSEL,0,0)) == CB_ERR)
       return;
   SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_GETLBTEXT,i,(LPARAM)s);
   s[10] = 0;
   dwAddress = StrToHex(s);
   im = (LPINFOMEM)pInfoMemList->GetFirstItem(&dwPos);
   while(im != NULL){
       if(dwAddress == im->adr)
           break;
       im = (LPINFOMEM)pInfoMemList->GetNextItem(&dwPos);
   }
   if(im == NULL || im->descrbit[0] == 0)
       return;
   currentInfoMem = im;
   dw = access_mem(dwAddress,0,currentInfoMem->flags,SCNT_READ);
   wsprintf(s,"0x%08X",dw);
   SendDlgItemMessage(m_hWnd,IDC_EDIT1,WM_SETTEXT,0,(LPARAM)s);
   p = im->descrbit;
   len = lstrlen(im->descrbit);
   SendDlgItemMessage(m_hWnd,IDC_LABEL_REG_DESCR,WM_SETTEXT,0,(LPARAM)im->descrex);
   ::GetWindowRect(GetDlgItem(m_hWnd,IDC_COMBOBOX1),&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,2);
   i = rc.bottom + 3;
   GetClientRect(&rc);
   rc.top = i;
   rc.right = (rc.right - rc.left - 10 - 210) >> 1;
   rc.left = 5;
   for(i5=0,i4=32,i=0,bLoad = 1,i2=0,i3=1;i2<32;i2++,i3<<=1){
       rc.left = i2 > 15 ? rc.right + 112 : 5;
       if(bLoad){
           if(i >= len)
               break;
           for(;i<len && p[i] == 9;i++);
           p1 = s;
           *p1++ = '0';
           *p1++ = 'x';
           if((im->flags & 3) == 0)
               i1 = 2;
           else if((im->flags & 3) == 1)
               i1 = 4;
           else
               i1 = 8;
           for(;i1 > 0 && i <len;i1--,i++)
               *p1++ = p[i];
           *p1 = 0;
           bit = (int)StrToHex(s);
           p1 = s1;
           for(;i<len && p[i] != 9;i++)
               *p1++ = p[i];
           *p1 = 0;
           bInsert = 1;
           bLoad = 0;
       }
       if((i3 & bit) != 0){
       	if(!bUpdate){
           	hWnd[i2] = ::CreateWindow("Button","",BS_FLAT|WS_CHILD|BS_AUTOCHECKBOX|WS_VISIBLE,rc.left,rc.top + (i2&15)*20,
               	rc.right,20,m_hWnd,(HMENU)(i2+100),hInst,NULL);
           	::SendMessage(hWnd[i2],WM_SETFONT,SendMessage(WM_GETFONT,0,0),MAKELPARAM(TRUE,0));
           }
           if(!bInsert)
           	continue;
           bInsert = 0;
           ::SendMessage(hWnd[i2],WM_SETTEXT,0,(LPARAM)s1);
       }
       else if(i3 > bit){
           bLoad = 1;
           i2--;
           i3 >>= 1;
           rc.left = i2 > 15 ? rc.right + 112 : 5;
           if((1 << log2(bit)) != bit && bit){
               for(i5=0;i5<32;i5++){
                   if(bit & (1 << i5))
                       break;
               }
               wsprintf(s1,"0x%X",(dw & bit) >> i5);
               if(!bUpdate){
               	hWnd[i4] = ::CreateWindowEx(WS_EX_CLIENTEDGE,"Static",s1,WS_CHILD|WS_VISIBLE|SS_RIGHT,rc.right+rc.left+1,rc.top + (i2&15)*20,
                   	100,19,m_hWnd,(HMENU)(i4+100),hInst,NULL);
					::SendMessage(hWnd[i4],WM_SETFONT,SendMessage(WM_GETFONT,0,0),MAKELPARAM(TRUE,0));
               }
               else{
               	if(hWnd[i4] != NULL)
               		::SetWindowText(hWnd[i4],s1);
               }
               i4++;
           }
       }
       else{
       	if(!bUpdate){
           	hWnd[i2] = ::CreateWindow("Button","",WS_DISABLED|BS_FLAT|WS_CHILD|BS_AUTOCHECKBOX|WS_VISIBLE,rc.left,rc.top + (i2&15)*20,
               	rc.right,19,m_hWnd,(HMENU)(i2+100),hInst,NULL);
           }
       }
   }
   if(!i3 && (1 << log2(bit)) != bit && bit){
       wsprintf(s1,"0x%X",(dw & bit) >> i5);
       if(!bUpdate){
       	hWnd[i4] = ::CreateWindowEx(WS_EX_CLIENTEDGE,"Static",s1,WS_CHILD|WS_VISIBLE|SS_RIGHT,rc.right+rc.left+1,rc.top + 15*20,
           	100,19,m_hWnd,(HMENU)(i4+100),hInst,NULL);
       	::SendMessage(hWnd[i4],WM_SETFONT,SendMessage(WM_GETFONT,0,0),MAKELPARAM(TRUE,0));
       }
       else{
       	if(hWnd[i4] != NULL)
           	::SetWindowText(hWnd[i4],s1);
       }
       i4++;
   }
   for(i=0;hWnd[i] != NULL && i < i2;i++)
       ::SendMessage(hWnd[i],BM_SETCHECK,((dw & (1<<i)) != 0 ? BST_CHECKED : BST_UNCHECKED),0);
/*   for(;i2<32;i2++){
       rc.left = i2 > 15 ? rc.right + 112 : 5;
       hWnd[i2] = ::CreateWindow("Button","",WS_DISABLED|BS_FLAT|WS_CHILD|BS_AUTOCHECKBOX|WS_VISIBLE,rc.left,rc.top + (i2&15)*20,
               rc.right,19,m_hWnd,(HMENU)(i2+100),hInst,NULL);
   }*/
}
//---------------------------------------------------------------------------
u32 LIOViewer::access_mem(u32 adr,u32 data,u8 flags,u8 oper)
{
   IARM7 *cpu;
   LPIFUNC *cI;
   LPOFUNC *cO;
   u8 *cIO_mem,am;
   u32 value;

   cpu = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_GETCURSEL,0,0) == 0 ? (IARM7 *)&arm9 : (IARM7 *)&arm7;
   cI = crI;
   cIO_mem = crIO_mem;
   cO = crO;
   if(cpu->r_index() == 9){
       crI = i_func;
       crIO_mem = io_mem;
       crO = o_func;
   }
   else{
       crI = i_func7;
       crO = o_func7;
       crIO_mem = io_mem7;
   }
   switch(flags & 3){
       case 0:
           am = AMM_BYTE;
       break;
       case 1:
           am = AMM_HWORD;
       break;
       default:
           am = AMM_WORD;
       break;
   }
   if(oper == SCNT_READ)
       value = cpu->read_mem(adr,am);
   else
       cpu->write_mem(adr,data,am);
   crI = cI;
   crIO_mem = cIO_mem;
   crO = cO;
   return value;
}
//---------------------------------------------------------------------------
void LIOViewer::OnChangeEditValue()
{
   char s[50];
   u32 value;

   if(currentInfoMem == NULL)
       return;
   SendDlgItemMessage(m_hWnd,IDC_EDIT1,WM_GETTEXT,50,(LPARAM)s);
   value = StrToHex(s);
   access_mem(currentInfoMem->adr,value,currentInfoMem->flags,SCNT_WRITE);
   OnSelChangeComboBox2();
}
//---------------------------------------------------------------------------
void LIOViewer::Update()
{
   if(currentInfoMem == NULL || m_hWnd == NULL || !::IsWindow(m_hWnd))
       return;
   OnSelChangeComboBox2(TRUE);
}
//---------------------------------------------------------------------------
void LIOViewer::OnClickCkeckBits(int id)
{
   u32 value,chkValue;

   if(currentInfoMem == NULL)
       return;
   value = access_mem(currentInfoMem->adr,0,currentInfoMem->flags,SCNT_READ);
   chkValue = (1<<(id-100));
   if(SendDlgItemMessage(m_hWnd,id,BM_GETCHECK,0,0) == BST_CHECKED)
       value |= chkValue;
   else
       value &= ~chkValue;
   access_mem(currentInfoMem->adr,value,currentInfoMem->flags,SCNT_WRITE);
   OnSelChangeComboBox2();
}
//---------------------------------------------------------------------------
void LIOViewer::FillComboBox2()
{
   LString s;
   LMemoryFile File;
   LZipFile zipFile;
   HRSRC hRes;
   HGLOBAL hgRes;
   LPBYTE pRes;
   DWORD dwWrite;
   BYTE c;
   int i,len;
   LPINFOMEM im;
   char *p,adr[11];

   DestroyControl();
   if(pInfoMemList != NULL)
       delete pInfoMemList;
   pInfoMemList = NULL;
   SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_RESETCONTENT,0,0);
   SendDlgItemMessage(m_hWnd,IDC_LABEL_REG_DESCR,WM_SETTEXT,0,(LPARAM)"");
   s = "";
   SendDlgItemMessage(m_hWnd,IDC_EDIT1,WM_SETTEXT,0,(LPARAM)s.c_str());
   i = SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_GETCURSEL,0,0);
   hRes = FindResource(hInst,MAKEINTRESOURCE(i == 0 ? IDI_ZIP_REG9 : IDI_ZIP_REG7),RT_RCDATA);
   if(hRes == NULL)
       return;
   if((dwWrite = SizeofResource(hInst,hRes)) == 0)
       return;
   hgRes = LoadResource(hInst,hRes);
   if(hgRes == NULL)
       return;
   pRes = (LPBYTE)LockResource(hgRes);
   if(pRes == NULL)
       goto ex_FillComboBox2;
   if(!File.Open())
       goto ex_FillComboBox2;
   File.Write(pRes,dwWrite);
   zipFile.SetFileStream(&File);
   if(!zipFile.Open())
       goto ex_FillComboBox2;
   if(!zipFile.OpenZipFile(1))
       goto ex_FillComboBox2;
   if((pInfoMemList = new LList()) == NULL)
       goto ex_FillComboBox2;
   while(zipFile.ReadZipFile(&c,1)){
       if(c == 10){
           len = s.Length();
           if(len == 0)
               continue;
           if((im = new INFOMEM) != NULL){
               lstrcpy(adr,"0x");
               lstrcpyn(&adr[2],s.c_str(),2);
               im->flags = (u8)StrToHex(adr);
               if(!(im->flags & 4)){
                   lstrcpy(adr,"0x0400");
                   lstrcpyn(adr+6,s.c_str()+1,5);
                   i = 6;
               }
               else{
                   lstrcpy(adr,"0x");
                   lstrcpyn(adr+2,s.c_str()+1,9);
                   i = 10;
               }
               im->adr = StrToHex(adr);
               p = im->descr;
               for(;i<=len && s[i] != 9;i++)
                   *p++ = s[i];
               *p = 0;
               for(;i <= len && s[i] == 9;i++);
               p = im->descrex;
               for(;i<=len && s[i] != 9;i++)
                   *p++ = s[i];
               *p = 0;
               for(;i <= len && s[i] == 9;i++);
               p = im->descrbit;
               for(;i<=len;i++)
                   *p++ = s[i];
               *p = 0;
               wsprintf(adr,"0x%08X",im->adr);
               s = adr;
               s += " ";
               s += im->descr;
               pInfoMemList->Add((LPVOID)im);
               SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)s.c_str());
           }
           s = "";
           continue;
       }
       s += (char)c;
   }
   SendDlgItemMessage(m_hWnd,IDC_COMBOBOX2,CB_SETCURSEL,-1,0);
ex_FillComboBox2:
   FreeResource(hgRes);
}
//---------------------------------------------------------------------------
LRESULT LIOViewer::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LRESULT res;
   int id;

   res = FALSE;
   switch(uMsg){
       case WM_CTLCOLORSTATIC:
           id = GetDlgCtrlID((HWND)lParam);
           if(id > 131 && id < 163 || id == IDC_LABEL_REG_DESCR){
               SetBkColor((HDC)wParam,GetSysColor(COLOR_WINDOW));
               SetTextColor((HDC)wParam,GetSysColor(COLOR_WINDOWTEXT));
               return (BOOL)GetSysColorBrush(COLOR_WINDOW);
           }
       break;
       case WM_INITDIALOG:
           return OnInitDialog();
       case WM_COMMAND:
           switch((id = LOWORD(wParam))){
               case IDOK:
                   Destroy();
               break;
               case IDC_EDIT1:
                   switch(HIWORD(wParam)){
                       case EN_KILLFOCUS:
                           if(SendDlgItemMessage(m_hWnd,IDC_EDIT1,EM_GETMODIFY,0,0))
                               OnChangeEditValue();
                       break;
                   }
               break;
               case IDC_COMBOBOX1:
                   if(HIWORD(wParam) == CBN_SELENDOK)
                       FillComboBox2();
               break;
               case IDC_COMBOBOX2:
                   if(HIWORD(wParam) == CBN_SELENDOK)
                       OnSelChangeComboBox2();
               break;
               default:
                   if(id > 99 && id < 132 && HIWORD(wParam) == BN_CLICKED){
                       if(currentInfoMem != NULL)
                           OnClickCkeckBits(id);
                   }
               break;
           }
       break;
       case WM_CLOSE:
           Destroy();
       break;
   }
   return res;
}
#endif
