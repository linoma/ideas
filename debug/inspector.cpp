#include "inspector.h"
#include "resource.h"
#include "lds.h"
#include "util.h"
#include "inputtext.h"

#ifdef _DEBPRO
//---------------------------------------------------------------------------
LInspectorDlg::LInspectorDlg(const char *n,Type *t,u32 adr,Function *f,IARM7 *p,LInspectorList *p1) : LDlg()
{
   name = n;
   type = t;
   obj = NULL;
   cpu = p;
   func = f;
   parent = p1;
   baseAdr = adr;
   sym = NULL;
   minRange = 0;
   maxRange = 50;
   bExternal = func != NULL ? FALSE : TRUE;
   bUseReg = FALSE;
}
//---------------------------------------------------------------------------
LInspectorDlg::LInspectorDlg(const char *n,elf_Object *t,Symbol *s,Function *f,IARM7 *p,LInspectorList *p1) : LDlg()
{
   name = n;
   type = t->type;
   obj = t;
   cpu = p;
   func = f;
   parent = p1;
   baseAdr = 0;
   bUseReg = FALSE;
   bExternal = obj->external;
   if((sym = s) != NULL){
       if(bExternal)
           baseAdr = sym->value;
       else
           baseAdr = get_Location((ELFBlock *)sym);
   }
   minRange = 0;
   maxRange = 50;
}
//---------------------------------------------------------------------------
LInspectorDlg::~LInspectorDlg()
{
}
//---------------------------------------------------------------------------
BOOL LInspectorDlg::Show(HWND parent)
{
//   parent = NULL;
   if(m_hWnd == NULL){
       if(!LDlg::Create(hInst,MAKEINTRESOURCE(IDD_DIALOG12),parent))
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
BOOL LInspectorDlg::OnInitDialog()
{
   LV_COLUMN lvc={0};
   LV_ITEM lvi = {0};
   HWND hwnd;
   int i;
   LString s,s2;
   char s1[10];
   HDC hdc;
   ITEM *p,*p1;

   GetClientRect(&rcClient);
   GetWindowRect(&rcWin);
   szFont.cx = szFont.cy = 0;
   if((hdc = GetDC(m_hWnd)) != NULL){
       GetTextExtentPoint32(hdc,"W",1,&szFont);
       ::ReleaseDC(m_hWnd,hdc);
   }
   hwnd = GetDlgItem(m_hWnd,IDC_LIST1);
   ListView_SetExtendedListViewStyle(hwnd,LVS_EX_FULLROWSELECT);
   ListView_SetBkColor(hwnd,GetSysColor(COLOR_BTNFACE));
   lvc.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM|LVCF_FMT;
   lvc.fmt = LVCFMT_LEFT;
   lvc.cx = 100;
	lvc.pszText = "Name";
   ListView_InsertColumn(hwnd,0,&lvc);
	lvc.pszText = "Value";
   lvc.iSubItem = 1;
   ListView_InsertColumn(hwnd,1,&lvc);
   if(type == NULL)
       return FALSE;
   s = name;
   s +=": ";
   p1 = new ITEM[1];
   p1->type = type;
   p1->location = 0;
   s2 = get_Type(p1);
   s += s2;
   if(!bUseReg){
       wsprintf(s1,"%08X",baseAdr);
       s += ": ";
       s += s1;
   }
   switch(type->type){
       case TYPE_base:
           lvi.mask = LVIF_TEXT|LVIF_PARAM|LVIF_STATE;
           lvi.state = LVIS_SELECTED;
           lvi.iItem = 0;
           lvi.pszText = name.c_str();
           lvi.iSubItem = 0;
           p = new ITEM[1];
           p->type = type;
           p->location = 0;
           lvi.lParam = (LPARAM)p;
			ListView_InsertItem(hwnd,&lvi);
           lvi.mask = LVIF_TEXT;
           lvi.iSubItem = 1;
           s2 = get_Value(p);
           lvi.pszText = s2.c_str();
           ListView_SetItem(hwnd,&lvi);
       break;
       case TYPE_struct:
           if(type->structure != NULL){
               for(i=0;i<type->structure->memberCount;i++){
                   lvi.mask = LVIF_TEXT|LVIF_PARAM;
                   if(i==0){
                       lvi.mask |= LVIF_STATE;
                       lvi.state = LVIS_SELECTED;
                   }
                   lvi.iItem = i;
                   lvi.pszText = type->structure->members[i].name;
                   lvi.iSubItem = 0;
                   p = new ITEM[1];
                   p->type = type->structure->members[i].type;
                   p->location = get_Location(type->structure->members[i].location);
                   lvi.lParam = (LPARAM)p;
			        ListView_InsertItem(hwnd,&lvi);
                   lvi.mask = LVIF_TEXT;
                   lvi.iSubItem = 1;
                   s2 = get_Value(p);
                   lvi.pszText = s2.c_str();
                   ListView_SetItem(hwnd,&lvi);
               }
           }
       break;
       case TYPE_array:
           for(i=0;i<type->array->bounds[0];i++){
               lvi.mask = LVIF_TEXT|LVIF_PARAM;
               if(i==0){
                   lvi.mask |= LVIF_STATE;
                   lvi.state = LVIS_SELECTED;
               }
               lvi.iItem = i;
               s2 = "[";
               s2 += i;
               s2 += "]";
               lvi.pszText = s2.c_str();
               lvi.iSubItem = 0;
               p = new ITEM[1];
               p->type = type->array->type;
               p->location = i*type->array->type->size;
               lvi.lParam = (LPARAM)p;
			    ListView_InsertItem(hwnd,&lvi);
               lvi.mask = LVIF_TEXT;
               lvi.iSubItem = 1;
               s2 = get_Value(p);
               lvi.pszText = s2.c_str();
               ListView_SetItem(hwnd,&lvi);
           }
       break;
       case TYPE_pointer:
           if(type->pointer != NULL){
               if(!bUseReg)
                   baseAdr = cpu->read_mem(baseAdr,AMM_WORD);
               s += ": ";
               s2 = get_Value(p1);
               s += s2;
               switch(type->pointer->type){
                   case TYPE_struct:
                       if(type->pointer->structure != NULL){
                           for(i=0;i<type->pointer->structure->memberCount;i++){
                               lvi.mask = LVIF_TEXT|LVIF_PARAM;
                               if(i==0){
                                   lvi.mask |= LVIF_STATE;
                                   lvi.state = LVIS_SELECTED;
                               }
                               lvi.iItem = i;
                               lvi.pszText = type->pointer->structure->members[i].name;
                               lvi.iSubItem = 0;
                               p = new ITEM[1];
                               p->type = type->pointer->structure->members[i].type;
                               p->location = get_Location(type->pointer->structure->members[i].location);
                               lvi.lParam = (LPARAM)p;
			                    ListView_InsertItem(hwnd,&lvi);
                               lvi.mask = LVIF_TEXT;
                               lvi.iSubItem = 1;
                               s2 = get_Value(p);
                               lvi.pszText = s2.c_str();
                               ListView_SetItem(hwnd,&lvi);
                           }
                       }
                   break;
               }
           }
       break;
   }
   delete p1;
   SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)s.c_str());
   SendDlgItemMessage(m_hWnd,IDC_COMBOBOX1,CB_SETCURSEL,0,0);
   SetWindowLong(hwnd,GWL_USERDATA,(LONG)this);
   oldListView1WndProc = (WNDPROC)SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WindowProcListView1);
   RecalWindowSize();
   return FALSE;
}
//---------------------------------------------------------------------------
void LInspectorDlg::RecalWindowSize()
{
   RECT rc,rc1,rc2;
   int count;

   count = ListView_GetItemCount(GetDlgItem(m_hWnd,IDC_LIST1));
   GetWindowRect(&rc);
   rc.bottom -= rc.top;
   ::GetWindowRect(GetDlgItem(m_hWnd,IDC_INSPECTOR_SB),&rc1);
   rc.bottom -= (rc1.bottom - rc1.top);
   ::GetWindowRect(GetDlgItem(m_hWnd,IDC_LIST1),&rc1);
   ::GetClientRect(GetDlgItem(m_hWnd,IDC_LIST1),&rc2);
   rc.bottom -= (rc1.bottom - rc1.top) + GetSystemMetrics(SM_CYCAPTION);
   rc.bottom += count * (szFont.cy + 2) + ((rc1.bottom - rc1.top) - (rc2.bottom - rc2.top));
   if(rc.bottom > (rcWin.bottom - rcWin.top))
       return;
   rcWin.bottom = rcWin.top + rc.bottom;
   ::SetWindowPos(m_hWnd,0,0,0,rc.right-rc.left,rc.bottom,SWP_NOMOVE|SWP_NOZORDER|SWP_FRAMECHANGED);
}
//---------------------------------------------------------------------------
void LInspectorDlg::OnListView1Notify(NM_LISTVIEW *p)
{
   LString s;
   LV_ITEM lvi;
   int i;
   LPITEM item;

   switch(p->hdr.code){
       case NM_DBLCLK:
           i = ListView_GetNextItem(p->hdr.hwndFrom,-1,LVNI_SELECTED|LVNI_ALL);
           if(i == -1)
               return;
           ZeroMemory(&lvi,sizeof(LV_ITEM));
           lvi.mask = LVIF_PARAM|LVIF_TEXT;
           lvi.iItem = i;
           s.Length(251);
           lvi.pszText = s.c_str();
           lvi.cchTextMax = 250;
           ListView_GetItem(p->hdr.hwndFrom,&lvi);
           item = (LPITEM)lvi.lParam;
           parent->Add(s.c_str(),item->type,baseAdr+item->location,func,cpu);
       break;
       case LVN_DELETEITEM:
           ZeroMemory(&lvi,sizeof(LV_ITEM));
           lvi.mask = LVIF_PARAM;
           lvi.iItem = p->iItem;
           ListView_GetItem(p->hdr.hwndFrom,&lvi);
           if(lvi.lParam != NULL)                               
               delete (LPITEM)lvi.lParam;
           lvi.lParam = 0;
           ListView_SetItem(p->hdr.hwndFrom,&lvi);
       break;
       case LVN_DELETEALLITEMS:
           ZeroMemory(&lvi,sizeof(LV_ITEM));
           for(i=ListView_GetItemCount(p->hdr.hwndFrom) - 1;i >=0;i--){
               lvi.mask = LVIF_PARAM;
               lvi.iItem = i;
               ListView_GetItem(p->hdr.hwndFrom,&lvi);
               if(lvi.lParam != NULL)
                   delete (LPITEM)lvi.lParam;
               lvi.lParam = 0;
               ListView_SetItem(p->hdr.hwndFrom,&lvi);
           }
       break;
       case LVN_ITEMCHANGED:
           if(p->uChanged & LVIF_STATE){
               if(p->uNewState & LVIS_SELECTED){
                   ZeroMemory(&lvi,sizeof(LV_ITEM));
                   lvi.mask = LVIF_PARAM;
                   lvi.iItem = p->iItem;
                   ListView_GetItem(p->hdr.hwndFrom,&lvi);
                   ShowEditButton();
                   s = get_Type((LPITEM)lvi.lParam);
                   ::SendDlgItemMessage(m_hWnd,IDC_INSPECTOR_SB,WM_SETTEXT,0,(LPARAM)s.c_str());
               }
           }
       break;
   }
}
//---------------------------------------------------------------------------
LRESULT CALLBACK LInspectorDlg::WindowProcListView1(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LInspectorDlg *p;

   p = (LInspectorDlg *)GetWindowLong(hwnd,GWL_USERDATA);
   return p->OnWindowProcListView1(uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
void LInspectorDlg::ShowEditButton()
{
   RECT rc;
   HWND hwnd;
   int i;
   LV_ITEM lvi;
   LPITEM item;

   ShowWindow(GetDlgItem(m_hWnd,IDC_BUTTON2),FALSE);
   hwnd = GetDlgItem(m_hWnd,IDC_LIST1);
   i = ListView_GetNextItem(hwnd,-1,LVNI_SELECTED|LVNI_ALL);
   if(i == -1)
       return;
   ZeroMemory(&lvi,sizeof(lvi));
   lvi.mask = LVIF_PARAM;
   lvi.iItem = i;
   if(!ListView_GetItem(hwnd,&lvi))
       return;
   if((item = (LPITEM)lvi.lParam) == NULL)
       return;
   if(item->type->type != TYPE_pointer && item->type->type != TYPE_base)
       return;
   ListView_EnsureVisible(hwnd,i,FALSE);
   if(!ListView_GetItemRect(hwnd,i,&rc,LVIR_BOUNDS))
       return;
   MapWindowPoints(hwnd,m_hWnd,(LPPOINT)&rc,2);
   rc.left = rc.right - szFont.cy - 1;
   rc.top++;
   rc.bottom = szFont.cy-1;
   ::SetWindowPos(GetDlgItem(m_hWnd,IDC_BUTTON2),HWND_TOP,rc.left,rc.top,rc.bottom,rc.bottom,SWP_SHOWWINDOW);
}
//---------------------------------------------------------------------------
void LInspectorDlg::OnSize(WPARAM wParam,LPARAM lParam)
{
   RECT rc,rc1;
   int cx,cy;
   HDWP hdwp;
   HWND hwnd;
   POINT pt;

   ::SendDlgItemMessage(m_hWnd,IDC_INSPECTOR_SB,WM_SIZE,wParam,lParam);
   cx = LOWORD(lParam);
   cy = HIWORD(lParam);
   CopyRect(&rc,&rcClient);
   SetRect(&rcClient,0,0,cx,cy);
	if((hdwp = BeginDeferWindowPos(2)) == NULL)
       return;
   hwnd = GetDlgItem(m_hWnd,IDC_COMBOBOX1);
   ::GetWindowRect(hwnd,&rc1);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc1,sizeof(RECT)/sizeof(POINT));
   pt.x = cx - (rc1.left << 1);
   hdwp = DeferWindowPos(hdwp,hwnd,0,0,0,pt.x,rc1.bottom - rc1.top,SWP_NOMOVE|SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_NOSENDCHANGING);
   if(hdwp == NULL) return;
   hwnd = GetDlgItem(m_hWnd,IDC_INSPECTOR_SB);
   ::GetWindowRect(hwnd,&rc1);
   pt.y = (rc1.bottom - rc1.top) + 6;
   hwnd = GetDlgItem(m_hWnd,IDC_LIST1);
   ::GetWindowRect(hwnd,&rc1);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc1,sizeof(RECT)/sizeof(POINT));
   pt.x = cx;
   pt.y = cy - pt.y;
   ::GetWindowRect(GetDlgItem(m_hWnd,IDC_COMBOBOX1),&rc1);
   pt.y -= (rc1.bottom - rc1.top) + 8;
   hdwp = DeferWindowPos(hdwp,hwnd,HWND_BOTTOM,0,0,pt.x,pt.y,SWP_NOMOVE|SWP_FRAMECHANGED);
   if(hdwp == NULL) return;
   EndDeferWindowPos(hdwp);
   hwnd = GetDlgItem(m_hWnd,IDC_LIST1);
   ::GetClientRect(hwnd,&rc);
   ListView_SetColumnWidth(hwnd,0,(rc.right - rc.left) >> 1);
   ListView_SetColumnWidth(hwnd,1,(rc.right - rc.left) >> 1);
   ShowEditButton();
}
//---------------------------------------------------------------------------
void LInspectorDlg::Update()
{
   int i;
   LPITEM item;
   LV_ITEM lvi;
   HWND hwnd;
   LString s;

   if(!bExternal){
       if(bUseReg && sym != NULL)
           baseAdr = get_Location((ELFBlock *)sym);
   }
   hwnd = GetDlgItem(m_hWnd,IDC_LIST1);
   i = ListView_GetItemCount(hwnd) - 1;
   for(;i >= 0;i--){
       ZeroMemory(&lvi,sizeof(LV_ITEM));
       lvi.mask = LVIF_PARAM;
       lvi.iItem = i;
       ListView_GetItem(hwnd,&lvi);
       if((item = (LPITEM)lvi.lParam) == NULL)
           continue;
       s = get_Value(item);
       ListView_SetItemText(hwnd,i,1,s.c_str());
   }
}
//---------------------------------------------------------------------------
void LInspectorDlg::OnButton2Click()
{
   int i;
   RECT rc;
   char s[30];
   LV_ITEM lvi;
   HWND hwnd;
   LPITEM item;

   hwnd = GetDlgItem(m_hWnd,IDC_LIST1);
   i = ListView_GetNextItem(hwnd,-1,LVNI_SELECTED|LVNI_ALL);
   if(i == -1)
       return;
   ZeroMemory(&lvi,sizeof(lvi));
   lvi.mask = LVIF_PARAM;
   lvi.iItem = i;
   ListView_GetItem(hwnd,&lvi);
   item = (LPITEM)lvi.lParam;
   lvi.iSubItem = 1;
   lvi.pszText = s;
   lvi.cchTextMax = 30;
   lvi.mask = LVIF_TEXT;
   ListView_GetItem(hwnd,&lvi);
   ListView_GetSubItemRect(GetDlgItem(m_hWnd,IDC_LIST1),i,1,LVIR_LABEL,&rc);
   MapWindowPoints(GetDlgItem(m_hWnd,IDC_LIST1),m_hWnd,(LPPOINT)&rc,2);
   ShowWindow(GetDlgItem(m_hWnd,IDC_BUTTON2),FALSE);
   rc.left++;
   rc.right--;
   if(InputText(m_hWnd,&rc,0,s,29)){
       lvi.iSubItem = 1;
       lvi.pszText = s;
       lvi.cchTextMax = 30;
       lvi.mask = LVIF_TEXT;
       ListView_SetItem(hwnd,&lvi);
   }
   ShowWindow(GetDlgItem(m_hWnd,IDC_BUTTON2),TRUE);
}
//---------------------------------------------------------------------------
void LInspectorDlg::OnListView1DrawItem(LPDRAWITEMSTRUCT p)
{
   HDC hdc;
   RECT rc;
   HWND hwnd;
   char s[300];

   hdc = p->hDC;
   SetBkMode(hdc,TRANSPARENT);
   hwnd = p->hwndItem;
   SetBkColor(hdc,GetSysColor(COLOR_BTNSHADOW));
   SetTextColor(hdc,GetSysColor(COLOR_WINDOWTEXT));
   ExtTextOut(hdc,p->rcItem.left,p->rcItem.top,ETO_OPAQUE,&p->rcItem,"",0,NULL);
   if(p->itemState & (ODS_SELECTED)){
       CopyRect(&rc,&p->rcItem);
       DrawEdge(hdc,&rc,EDGE_SUNKEN,BF_RECT);
   }
   ListView_GetSubItemRect(hwnd,p->itemID,0,LVIR_LABEL,&rc);
   rc.left = 0;
   ListView_GetItemText(hwnd,p->itemID,0,s,299);
   SetBkColor(hdc,GetSysColor(COLOR_BTNFACE));
   rc.bottom--;
   if(p->itemState & (ODS_SELECTED)){
       rc.bottom--;
       rc.top++;
   }
   ExtTextOut(hdc,rc.left,rc.top,ETO_OPAQUE,&rc,"",0,NULL);
   rc.left+=2;
   DrawText(hdc,s,-1,&rc,DT_SINGLELINE|DT_VCENTER|DT_LEFT);
   ListView_GetSubItemRect(hwnd,p->itemID,1,LVIR_LABEL,&rc);
   if(p->itemState & (ODS_SELECTED))
       SetBkColor(hdc,GetSysColor(COLOR_WINDOW));
   rc.bottom--;
   if(p->itemState & (ODS_SELECTED)){
       rc.bottom--;
       rc.top++;
   }
   rc.left++;
   ExtTextOut(hdc,rc.left,rc.top,ETO_OPAQUE,&rc,"",0,NULL);
   rc.left++;
   rc.right-= 2;
   ListView_GetItemText(hwnd,p->itemID,1,s,299);
   DrawText(hdc,s,-1,&rc,DT_SINGLELINE|DT_VCENTER|DT_LEFT);
}
//---------------------------------------------------------------------------
LRESULT LInspectorDlg::OnWindowProcListView1(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LRESULT res;
   int i;
   HWND hwnd;
   POINT pt;
   RECT rc;

   res = CallWindowProc(oldListView1WndProc,(hwnd = GetDlgItem(m_hWnd,IDC_LIST1)),uMsg,wParam,lParam);
   switch(uMsg){
       case WM_VSCROLL:
           i = ListView_GetNextItem(hwnd,-1,LVNI_SELECTED|LVNI_ALL);
           if(i != -1 && IsWindowVisible(GetDlgItem(m_hWnd,IDC_BUTTON2))){
               ListView_GetItemPosition(hwnd,i,&pt);
               ::GetClientRect(hwnd,&rc);
               if(pt.y < 0 || (pt.y + szFont.cy) > rc.bottom)
                   ShowWindow(GetDlgItem(m_hWnd,IDC_BUTTON2),FALSE);
               else{
                   MapWindowPoints(hwnd,m_hWnd,(LPPOINT)&pt,1);
                   hwnd = GetDlgItem(m_hWnd,IDC_BUTTON2);
                   ::GetWindowRect(hwnd,&rc);
                   MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,2);
                   ::SetWindowPos(hwnd,HWND_TOP,rc.left,pt.y+1,0,0,SWP_NOSIZE|SWP_SHOWWINDOW|SWP_DRAWFRAME);
               }
           }
       break;
   }
   return res;
}
//---------------------------------------------------------------------------
LRESULT LInspectorDlg::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LRESULT res;

   res = FALSE;
   switch(uMsg){
       case WM_GETMINMAXINFO:
           ((LPMINMAXINFO)lParam)->ptMinTrackSize.x = rcWin.right - rcWin.left;
           ((LPMINMAXINFO)lParam)->ptMinTrackSize.y = (rcWin.bottom - rcWin.top);
       break;
       case WM_NOTIFY:
           if(wParam == IDC_LIST1)
               OnListView1Notify((NM_LISTVIEW *)lParam);
       break;
       case WM_COMMAND:
           switch(LOWORD(wParam)){
               case IDC_BUTTON2:
                   switch(HIWORD(wParam)){
                       case BN_CLICKED:
                           OnButton2Click();
                       break;
                   }
               break;
           }
       break;
       case WM_SIZE:
           OnSize(wParam,lParam);
       break;
       case WM_INITDIALOG:
           return OnInitDialog();
       case WM_DRAWITEM:
           if(wParam == IDC_LIST1)
               OnListView1DrawItem((LPDRAWITEMSTRUCT)lParam);
       break;
       case WM_MEASUREITEM:
           ((MEASUREITEMSTRUCT *)lParam)->itemHeight = szFont.cy + 2;
           return TRUE;
       case WM_CLOSE:
           Destroy();
       break;
   }
   return res;
}
//---------------------------------------------------------------------------
LString LInspectorDlg::get_Type(ITEM *item)
{
   LString s;
   Type *p;

   p = item->type;
   s = "";
   if(p != NULL){
       switch(p->type){
           case TYPE_array:
               switch(p->array->type->type){
                   case TYPE_base:
                       s = p->array->type->name;
                       if(p->array->maxBounds){
                           s += "[";
                           s += *p->array->bounds;
                           s += "]";
                       }
                   break;
                   case TYPE_struct:
                       s = p->array->type->name;
                       s += " *";
                   break;
               }
           break;
           case TYPE_base:
               s = p->name;
           break;
           case TYPE_pointer:
               if(p->pointer != NULL)
                   s = p->pointer->name;
               else
                   s = "void";
               s += " *";
           break;
           case TYPE_struct:
               s = p->name;
           break;
       }
   }
   return s;
}
//---------------------------------------------------------------------------
LString LInspectorDlg::get_Value(ITEM *item)
{
   LString s,s2;
   u32 value,ba;
   int i;
   char s1[30];
   Type *p;
   LPITEM p1;
   BOOL b;

   if((p = item->type) == NULL)
       return "";
   b = p == type && bUseReg;
   s = "";
   ba = baseAdr + item->location;
   switch(p->type){
       case TYPE_struct:
           s = "{";
           if(ba){
               p1 = new ITEM[1];
               for(i=0;i<p->structure->memberCount;i++){
                   p1->type = p->structure->members[i].type;
                   p1->location = item->location + get_Location(p->structure->members[i].location);
                   s2 = get_Value(p1);
                   if(s.Length() > 1)
                       s += ",";
                   s += s2;
               }
               delete p1;
           }
           s += "}";
       break;
       case TYPE_array:
           s = "{";
           if(ba){
               p1 = new ITEM[1];
               p1->type = p->array->type;
               for(i=0;i<p->array->bounds[0] && i < maxRange;i++){
                   p1->location = item->location + i * p->array->type->size;
                   s2 = get_Value(p1);
                   if(s.Length() > 1)
                       s += ",";
                   s += s2;
               }
               delete p1;
           }
           s += "}";
       break;
       case TYPE_base:
           value = 0;
           switch(p->size){
               case 4:
                   if(!b){
                       if(ba)
                           value = cpu->read_mem(ba,(u8)AMM_WORD);
                   }
                   else
                       value = baseAdr;
                   wsprintf(s1,"0x%08X",value);
               break;
               case 2:
                   if(!b){
                       if(ba)
                           value = cpu->read_mem(ba,(u8)AMM_HWORD);
                   }
                   else
                       value = baseAdr;
                   wsprintf(s1,"0x%04X",value);
               break;
               default:
                   if(!b){
                       if(ba)
                           value = cpu->read_mem(ba,(u8)AMM_BYTE);
                   }
                   else
                       value = baseAdr;
                   wsprintf(s1,"0x%02X",value);
               break;
           }
           s = s1;
       break;
       case TYPE_pointer:
           if(ba){
               if(p != type)
                   value = cpu->read_mem(ba,AMM_WORD);
               else
                   value = ba;
           }
           else
               value = 0;
           if(value != 0){
               wsprintf(s1,":%08X",value);
               s = s1;
           }
           else
               s = "NULL";
       break;
   }
   return s;
}
//---------------------------------------------------------------------------
u32 LInspectorDlg::get_Location(ELFBlock *location)
{
   int i1,shift,i;
   u32 adr;
   u8 byte;

   if(location == NULL)
       return 0;
   adr = 0;
   if(location == (ELFBlock *)sym)
       bUseReg = FALSE;
   switch(location->data[0]){
       case DW_OP_plus_uconst:
           for(i1=location->length-1;i1>0;i1--){
               adr <<= 8;
               adr |= location->data[i1];
           }
       break;
       case DW_OP_fbreg:
           adr = cpu->r_gpreg(13);
           i = shift = 0;
           for(i1=1;i1 < location->length;i1++){
               byte = location->data[i1];
               i |= (byte & 0x7F) << shift;
               shift += 7;
               if(byte & 0x80)
                   break;
           }
           if(shift < 32 && (byte & 0x40))
               i |= -(1 << shift);
           adr = (int)adr + (i * -1);
       break;
       case DW_OP_reg0:
       case DW_OP_reg1:
       case DW_OP_reg2:
       case DW_OP_reg3:
       case DW_OP_reg4:
       case DW_OP_reg5:
       case DW_OP_reg6:
       case DW_OP_reg7:
       case DW_OP_reg8:
       case DW_OP_reg9:
       case DW_OP_reg10:
       case DW_OP_reg11:
       case DW_OP_reg12:
       case DW_OP_reg13:
       case DW_OP_reg14:
       case DW_OP_reg15:
       case DW_OP_reg16:
           i = location->data[0] - DW_OP_reg0;
           adr = cpu->r_gpreg(i);
           if(location == (ELFBlock *)sym)
               bUseReg = TRUE;
       break;
       case DW_OP_breg0:
       case DW_OP_breg1:
       case DW_OP_breg2:
       case DW_OP_breg3:
       case DW_OP_breg4:
       case DW_OP_breg5:
       case DW_OP_breg6:
       case DW_OP_breg7:
       case DW_OP_breg8:
       case DW_OP_breg9:
       case DW_OP_breg10:
       case DW_OP_breg11:
       case DW_OP_breg12:
       case DW_OP_breg13:
       case DW_OP_breg14:
       case DW_OP_breg15:
       case DW_OP_breg16:
           i = location->data[0] - DW_OP_breg0;
           adr = cpu->r_gpreg(i);
           i = shift = 0;
           for(i1=1;i1 < location->length;i1++){
               byte = location->data[i1];
               i |= (byte & 0x7F) << shift;
               shift += 7;
               if(byte & 0x80)
                   break;
           }
           if(shift < 32 && (byte & 0x40))
               i |= -(1 << shift);
           adr = (int)adr + i;
       break;
       default:
           adr = 0;
       break;
   }
   return adr;
}
//---------------------------------------------------------------------------
LInspectorList::LInspectorList() : LList()
{
}
//---------------------------------------------------------------------------
LInspectorList::~LInspectorList()
{
   Clear();
}
//---------------------------------------------------------------------------
BOOL LInspectorList::Add(const char *name,u32 adr,IARM7 *cpu)
{
   LInspectorDlg *p;
   elf_Object *obj;
   Symbol *sym;
   Function *func;

   if(name == NULL || lstrlen(name) == 0 || ds.get_RomReader() == NULL){
       ::MessageBox(NULL,"No Symbol Found.","iDeaS Emualtor",MB_OK|MB_ICONERROR);
       return FALSE;
   }
   if(ds.get_RomReader()->GetVariableFromName(name,adr,(void **)&obj,(void **)&sym,(void **)&func,cpu->r_index()) < 0 || obj == NULL){
       ::MessageBox(NULL,"No Symbol Found.","iDeaS Emualtor",MB_OK|MB_ICONERROR);
       return FALSE;
   }
   if((p = new LInspectorDlg(name,obj,sym,func,cpu,this)) == NULL)
       return FALSE;
   if(!LList::Add((LPVOID)p) || !p->Show(debugDlg.Handle())){
       delete p;
       return FALSE;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LInspectorList::Add(const char *name,Type *t,u32 adr,Function *f,IARM7 *cpu)
{
   LInspectorDlg *p;

   if(name == NULL || lstrlen(name) == 0 || t == NULL || cpu == NULL)
       return FALSE;
   if((p = new LInspectorDlg(name,t,adr,f,cpu,this)) == NULL)
       return FALSE;
   if(!LList::Add((LPVOID)p) || !p->Show(debugDlg.Handle())){
       delete p;
       return FALSE;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LInspectorList::IsDialogMessage(LPMSG p)
{
   elem_list *p1;

   p1 = First;
   while(p1 != NULL){
       if(::IsDialogMessage(((LInspectorDlg *)p1->Ele)->Handle(),p))
           return TRUE;
       p1 = p1->Next;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
void LInspectorList::Update()
{
   elem_list *p1,*p;

   p1 = First;
   while(p1 != NULL){
       p = p1->Next;
       if(((LInspectorDlg *)p1->Ele)->Handle() == NULL || !IsWindowVisible(((LInspectorDlg *)p1->Ele)->Handle()))
           RemoveInternal(p1,TRUE);
       else
           ((LInspectorDlg *)p1->Ele)->Update();
       p1 = p;
   }
}
//---------------------------------------------------------------------------
void LInspectorList::Reset()
{
   Clear();
}
//---------------------------------------------------------------------------
void LInspectorList::DeleteElem(LPVOID ele)
{
   if(ele != NULL)
       delete (LInspectorDlg *)ele;
}

#endif





