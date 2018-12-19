#include "bp.h"
#include "inputtext.h"
#include "resource.h"
#include "util.h"
#include "lds.h"
//---------------------------------------------------------------------------
#if defined(_DEBPRO2)
//---------------------------------------------------------------------------
LBreakPoint::LBreakPoint()
{
	Enable = FALSE;
	Type = BT_PROGRAM;
   Address = 0;
   ZeroMemory(Condition,sizeof(Condition));
   ZeroMemory(Description,sizeof(Description));
   PassCount = 0;
	int_PassCount = int_PassCount2 = 0;
}
//---------------------------------------------------------------------------
LBreakPoint::~LBreakPoint()
{
}
//---------------------------------------------------------------------------
BOOL LBreakPoint::Read(LFile *pFile,int ver)
{
	if(pFile == NULL)
   	return FALSE;
	if(pFile->Read(&Address,sizeof(Address)) != sizeof(Address))
   	return FALSE;
	if(pFile->Read(&Enable,sizeof(Enable)) != sizeof(Enable))
   	return FALSE;
   if(pFile->Read(&PassCount,sizeof(PassCount)) != sizeof(PassCount))
   	return FALSE;
   if(pFile->Read(&int_PassCount,sizeof(int_PassCount)) != sizeof(int_PassCount))
   	return FALSE;
	if(pFile->Read(Condition,30) != 30)
   	return FALSE;
	if(pFile->Read(&Type,sizeof(Type)) != sizeof(Type))
   	return FALSE;
   if(ver != 0){
       if(pFile->Read(Description,100) != 100)
   	    return FALSE;
   }
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LBreakPoint::Write(LFile *pFile)
{
	if(pFile == NULL)
   	return FALSE;
	pFile->Write(&Address,sizeof(Address));
	pFile->Write(&Enable,sizeof(Enable));
   pFile->Write(&PassCount,sizeof(PassCount));
   pFile->Write(&int_PassCount,sizeof(int_PassCount));
	pFile->Write(Condition,30);
	pFile->Write(&Type,sizeof(Type));
	pFile->Write(Description,100);
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LBreakPoint::Check(unsigned long a0,int accessMode,IARM7 *cpu,unsigned long data)
{
   unsigned long a1;
   u32 value,value1;
	BOOL res;

	switch(Type){
   	case BT_PROGRAM:
       	if(Address != a0)
           	return FALSE;
           int_PassCount2++;
           if(!Enable)
           	return FALSE;
           if(PassCount == 0 && Condition[0] == 0)
				return TRUE;
           if(Condition[0] == 0){
                if(PassCount && ++int_PassCount >= PassCount)
                    return TRUE;
                return FALSE;
			}
           switch(Condition[0]){
               case 'h':
                   value1 = cpu->read_mem(value1,AMM_HWORD);
               break;
               case 'b':
                   value1 = cpu->read_mem(value1,AMM_BYTE);
               break;
               case 'w':
                   value1 = cpu->read_mem(value1,AMM_WORD);
               break;
               default:
           		value1 = cpu->gp_regs[Condition[1]];
               break;
               case 'c':
               	value1 = cpu->r_cf();
               break;
               case 'n':
               	value1 = cpu->r_nf();
               break;
               case 'v':
               	value1 = cpu->r_vf();
               break;
               case 'z':
               	value1 = cpu->r_zf();
               break;
           }
       	if(Condition[3] == 'r')
           	value = cpu->gp_regs[Condition[4]];
       	else if(Condition[3] == 'v')
           	value = *((u32 *)&Condition[4]);
   		res = FALSE;
       	switch(Condition[2]){
           	case CC_EQ:
              		if(value1 == value)
                  		res = TRUE;
				break;
              	case CC_NE:
              		if(value1 != value)
                   	res = TRUE;
               break;
               case CC_GT:
               	if(value1 > value)
                  		res = TRUE;
               break;
               case CC_GE:
               	if(value1 >= value)
                  		res = TRUE;
               break;
               case CC_LT:
               	if(value1 < value)
                  		res = TRUE;
               break;
               case CC_LE:
               	if(value1 <= value)
                  		res = TRUE;
               break;
           }
           if(res && PassCount && ++int_PassCount < PassCount)
           	res = FALSE;
			return res;
       case BT_MEMORY:
       	if(!Enable)
           	return FALSE;
   		a1 = a0;
   		switch(accessMode & 0xF){
       		case AMM_HWORD:
					a1 += 1;
       		break;
       		case AMM_WORD:
       			a1 += 3;
       		break;
   		}
           if(has_Range()){
           	if(!((a0 >= Address && a0 <= get_Address2()) ||
                  (a1 >= Address && a1 <= get_Address2())))
                  		return FALSE;
           }
           else{
				if(Address < a0 || Address > a1)
           		return FALSE;
           }
      		if(((accessMode & AMM_WRITE) && is_Write()))
          		goto Check_1;
          	if(((accessMode & AMM_READ) && is_Read()))
          		goto Check_1;
           return FALSE;
Check_1:
			if((value1 = get_Processor()) != 0){
				if(value1 == 1 && cpu->r_index() != 9)
               	return FALSE;
               if(value1 == 2 && cpu->r_index() != 7)
               	return FALSE;
           }
           if(Condition[10] == 0)
               return TRUE;
			if(accessMode & AMM_WRITE)
           	value1 = data;
           else{
               switch(accessMode & 0xF){
                   case AMM_HWORD:
                       value1 = cpu->read_mem(a0,AMM_HWORD);
                   break;
                   case AMM_WORD:
                       value1 = cpu->read_mem(a0,AMM_WORD);
                   break;
                   case AMM_BYTE:
                       value1 = cpu->read_mem(a0,AMM_BYTE);
                   break;
               }
           }
           value = *((u32 *)&Condition[12]);
       	switch(Condition[10]){
               case CC_EQ:
                   if(value1 == value)
                   	return TRUE;
               break;
              	case CC_NE:
                   if(value1 != value)
                       return TRUE;
               break;
               case CC_GT:
                   if(value1 > value)
                  	    return TRUE;
               break;
               case CC_GE:
                   if(value1 >= value)
                  	    return TRUE;
               break;
               case CC_LT:
                   if(value1 < value)
                  	    return TRUE;
               break;
               case CC_LE:
                   if(value1 <= value)
                  	    return TRUE;
               break;
           }
			return FALSE;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
u8 LBreakPoint::ConditionToValue(int *rd,u8 *cond,u32 *value,unsigned char type)
{
   if(type == BT_PROGRAM){
       if(Condition[0] == 0)
           return 0;
       *rd = Condition[1];
       if(Condition[0] != 'r'){
       	switch(Condition[0]){
				case 'b':
               	*rd |= 0x20;
               break;
				case 'h':
               	*rd |= 0x40;
               break;
				case 'w':
               	*rd |= 0x10;
               break;
               default:
               	*rd |= 0x80;
               break;
           }
       }
       *cond = Condition[2];
       if(Condition[3] == 'r')
           *value = Condition[4];
       else if(Condition[3] == 'v')
           *value = *((u32 *)&Condition[4]);
       if(Condition[3] == 'r')
           return 2;
       return 1;
   }
   if(Condition[10] == 0)
       return 0;
   *cond = Condition[10];
   *value = *((u32 *)&Condition[12]);
   return 1;
}
//---------------------------------------------------------------------------
void LBreakPoint::StringToCondition(char *s,unsigned char type)
{
	char *p,c[50],*p1,*p2,c1[10];
	int i,i1;

	if(s == NULL || s[0] == 0){
       if(type == BT_PROGRAM)
   	    ZeroMemory(Condition,sizeof(Condition));
       else
           ZeroMemory(&Condition[10],sizeof(Condition) - 10);
		return;
   }
   ((long *)c)[0] = 0;
   i1 = 0;
   if(type == BT_PROGRAM){
       p = strpbrk(s,"rcvnz");
       if(p == NULL)
   	    return;
       c[i1] = *p;
       if(p != s){
           i = (int)((u32)p - (u32)s);
           if(*(p-i) == '*'){
               c[i1] = 'w';
               if(i > 1){
                   if(*(p-i+1) == 'b' || *(p-i+1) =='B')
                       c[i1] = 'b';
                   else if(*(p-i+1) == 'h' || *(p-i+1) == 'H')
                       c[i1] = 'h';
               }
           }
       }
       i1++;
       c[i1++] = (char)atoi(p+1);
   }
   p = strpbrk(s,"!=<>");
   if(p == 0 || (strpbrk(s,"rcvnz") == NULL && type == BT_PROGRAM))
   	return;
   p2 = p + 1;
   do{
   	p1 = strpbrk(p2,"!=<>");
		if(p1 == NULL)
       	break;
		p2 = p1 + 1;
   }while(1);
	i = (int)p - (int)s;
   while(*(p-1) == 32){
   	i--;
       p--;
   }
	while(s[i] == 32)
   	i++;
   p1 = &s[i];
   i = (int)p2 - (int)p;
   lstrcpyn(c1,p1,i+1);
   i = 1;
   if(c1[0] == '!')
   	i = CC_NE;
	else if(c1[0] == '>'){
       i = c1[1] == '=' ? CC_GE : CC_GT;
   }
	else if(c1[0] == '<'){
   	i = c1[1] == '=' ? CC_LE : CC_LT;
   }
	c[i1++] = (char)i;
   while(*p2 == 32)
   	p2++;
   if(*p2 == 'r'){
   	c[i1++] = *p2;
       c[i1++] = (char)atoi(p2+1);
   }
   else{
   	c[i1++] = 'v';
		*((int *)&c[i1]) = StrToHex(p2);
       i1 += 4;
   }
   c[i1++] = 0;
   i = type == BT_PROGRAM ? 0 : 10;
   CopyMemory(&Condition[i],c,i1);
}
//---------------------------------------------------------------------------
LString LBreakPoint::ConditionToString(unsigned char type)
{
   LString condition;
   u8 cond,res;
   char s[50];
   int rd;
   u32 value;

   condition = "";
   if((res = ConditionToValue(&rd,&cond,&value,type)) != 0){
       if(type == BT_PROGRAM){
           if(rd & 0x70){
               condition = "*";
               if(rd & 0x20)
                   condition += "b";
               else if(rd & 0x40)
                   condition += "h";
           }
           condition += Condition[0];
           if(!(rd & 0x80))
           	condition += (int)(rd & 0xF);
       }
       switch(cond){
           case CC_EQ:
               condition += " == ";
           break;
           case CC_NE:
               condition += " != ";
           break;
           case CC_GT:
               condition += " >  ";
           break;
           case CC_GE:
               condition += " >= ";
           break;
           case CC_LT:
               condition += " <  ";
           break;
           case CC_LE:
               condition += " <= ";
           break;
       }
       if(res == 1)
       	wsprintf(s,"0x%08X",value);
       else
			wsprintf(s,"r%d",value);
       condition += s;
   }
   return condition;
}
//---------------------------------------------------------------------------
LListBreakPoint::LListBreakPoint() : LList(),LWnd()
{
	bModified = FALSE;
   currentType = -1;
   hMenu = NULL;
   fileName = "";
}
//---------------------------------------------------------------------------
LListBreakPoint::~LListBreakPoint()
{
	if(hMenu != NULL)
   	DestroyMenu(hMenu);
}
//---------------------------------------------------------------------------
BOOL LListBreakPoint::Delete(DWORD item,BOOL bFlag)
{
	if(!LList::Delete(item,bFlag))
   	return FALSE;
   bModified = TRUE;
//   currentType = -1;   
   return TRUE;
}
//---------------------------------------------------------------------------
LBreakPoint *LListBreakPoint::Add(unsigned long address,int type)
{
	LBreakPoint *p;

	if(Find(address,type) != NULL)
   	return NULL;
   if((p = new LBreakPoint()) == NULL)
   	return NULL;
	if(!Add(p)){
   	delete p;
       return NULL;
   }
   p->set_Enable();
   p->set_Type(type);
   p->set_Address(address);
	return p;
}
//---------------------------------------------------------------------------
BOOL LListBreakPoint::Add(LBreakPoint *p)
{
	if(!LList::Add((LPVOID)p))
   	return FALSE;
   bModified = TRUE;
//   currentType = -1;
   return TRUE;
}
//---------------------------------------------------------------------------
LBreakPoint *LListBreakPoint::Check(unsigned long address,int accessMode,IARM7 *cpu,unsigned long data)
{
	elem_list *tmp;
	LBreakPoint *p;
	int type;

   type = accessMode == 0 ? BT_PROGRAM : BT_MEMORY;
	tmp = First;
  	while(tmp != NULL){
   	p = (LBreakPoint *)tmp->Ele;
       if(p->get_Type() == type && p->Check(address,accessMode,cpu,data))
       	return p;
   	tmp = tmp->Next;
   }
   return NULL;
}
//----------------------------------------------------------------------------
LBreakPoint *LListBreakPoint::Find(unsigned long address,int type)
{
	elem_list *tmp;
	LBreakPoint *p;

	tmp = First;
   while(tmp != NULL){
   	p = (LBreakPoint *)tmp->Ele;
       if(p->get_Type() == type && p->get_Address() == address)
       	return p;
   	tmp = tmp->Next;
   }
   return NULL;
}
//----------------------------------------------------------------------------
void LListBreakPoint::Reset()
{
	elem_list *tmp;

	tmp = First;
   while(tmp != NULL){
   	((LBreakPoint *)tmp->Ele)->Reset();
   	tmp = tmp->Next;
   }
}
//---------------------------------------------------------------------------
BOOL LListBreakPoint::Save(const char *lpFileName,BOOL bForce)
{
	LFile *pFile;
	elem_list *tmp;
	LBreakPoint *p;
	LString s;

	if(nCount == 0)
   	return TRUE;
   if(!bForce && !bModified)
   	return TRUE;
   if(lpFileName == NULL || lpFileName[0] == 0)
   	s = fileName;
   else
   	s = (char *)lpFileName;
   if(s.IsEmpty()){
       s.Capacity(500);
   	if(!ShowSaveDialog(NULL,s.c_str(),"BreakPoint Files (*.lst)\0*.lst\0All files (*.*)\0*.*\0\0\0\0\0",NULL)){
           DWORD dw = CommDlgExtendedError();
       	return FALSE;
       }
   }
   if(s.IsEmpty() || (pFile = new LFile(s.c_str())) == NULL)
   	return FALSE;
   if(!pFile->Open(GENERIC_WRITE,CREATE_ALWAYS)){
   	delete pFile;
       return FALSE;
   }
   s = "VER\1";
   pFile->Write(s.c_str(),4);

	tmp = First;
   while(tmp != NULL){
   	p = (LBreakPoint *)tmp->Ele;
       p->Write(pFile);
   	tmp = tmp->Next;
   }
   delete pFile;
   bModified = FALSE;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LListBreakPoint::Load(const char *lpFileName)
{
	LFile *pFile;
	LBreakPoint *p;
	BOOL res;
   char c[MAX_PATH+100],ver;

   if(lpFileName == NULL || (pFile = new LFile(lpFileName)) == NULL)
   	return FALSE;
   if(!pFile->Open()){
   	delete pFile;
   	wsprintf(c,"Unable to read %s",lpFileName);
		MessageBox(debugDlg.Handle(),c,"iDeaS Emulator",MB_ICONERROR|MB_OK);
       return FALSE;
   }
   pFile->Read(c,4);
   ver = c[3];
   c[3] = 0;
   if(lstrcmpi(c,"VER")){
       ver = 0;
       pFile->SeekToBegin();
   }
	Clear();
   res = TRUE;
   while(1){
		if((p = new LBreakPoint()) != NULL){
   		if(!p->Read(pFile,ver)){
				delete p;
				break;
           }
      		if(!Add(p))
           	delete p;
       }
   }
   delete pFile;
   if(res){
   	fileName = (char *)lpFileName;
       bModified = FALSE;
   }
   return res;
}
//---------------------------------------------------------------------------
LONG LListBreakPoint::OnNotify(NM_LISTVIEW *p)
{
	LBreakPoint *pb;
   LV_ITEM lvi={0};
	int item,i,i1;
	RECT rc,rcClient;
   LVHITTESTINFO lvhi= {0};
  	char s[100],s1[200],*p1;
   SCROLLINFO si={0};
	HWND hwndLV;
	const char s2[][11]={{"Read"},{"Write"},{"Modify"},{"Read/Write"}};
   const char s3[][16]={{"Byte"},{"HWord"},{"Word"},{"Byte/HWord"},{"Byte/Word"},
   					 {"HWord/Word"},{"Byte/HWord/Word"}};
	const char s4[][4]={{"Yes"},{"No"}};
	const char s5[][5]={{"Both"},{"Arm9"},{"Arm7"}};

   hwndLV = p->hdr.hwndFrom;
	switch(p->hdr.code){
   	case NM_RCLICK:
			if(ListView_GetItemCount(hwndLV) == 0)
				return 0;
           if(DestroyMenu(hMenu))
           	hMenu = NULL;
       	hMenu = LoadMenu(hInst,MAKEINTRESOURCE(IDR_BP_MENU));
           GetCursorPos(&lvhi.pt);
           if(ListView_GetNextItem(hwndLV,-1,LVNI_ALL|LVNI_SELECTED) == -1){
               EnableMenuItem(GetSubMenu(hMenu,0),ID_BP_GO,MF_BYCOMMAND|MF_GRAYED);
#ifdef _DEBPRO
               EnableMenuItem(GetSubMenu(hMenu,0),ID_BP_RESET_INTERNAL_COUNT,MF_BYCOMMAND|MF_GRAYED);
#endif
           }
           TrackPopupMenu(GetSubMenu(hMenu,0),TPM_LEFTALIGN,lvhi.pt.x,lvhi.pt.y,0,m_hWnd,NULL);
           if(DestroyMenu(hMenu))
           	hMenu = NULL;
       break;
		case LVN_KEYDOWN:
           switch(((LV_KEYDOWN *)p)->wVKey){
           	case VK_DELETE:
              		if((item = ListView_GetNextItem(hwndLV,-1,LVNI_ALL|LVNI_SELECTED)) != -1){
						lvi.mask = LVIF_PARAM;
                       lvi.iItem = item;
                       ListView_GetItem(hwndLV,&lvi);
		                if((pb = (LBreakPoint *)lvi.lParam) != NULL){
                       	LList::Delete(pb);
                       	ListView_DeleteItem(hwndLV,lvi.iItem);
                           hwndLV = GetDlgItem(debugDlg.Handle(),IDC_DEBUG_DIS);
                           InvalidateRect(hwndLV,NULL,TRUE);
                           UpdateWindow(hwndLV);
                           bModified = TRUE;
                           debugDlg.UpdateToolBar();
                       }
                   }
               break;
           }
       break;
       case NM_DBLCLK:
       	GetCursorPos(&lvhi.pt);
           MapWindowPoints(NULL,hwndLV,&lvhi.pt,1);
           ListView_SubItemHitTest(hwndLV,&lvhi);
           if((item = lvhi.iItem) == -1){
               if((pb = new LBreakPoint()) == NULL)
           		return 0;
				pb->set_Type(currentType);
               if(!Add(pb))
               	return 0;
               lvi.mask = LVIF_TEXT;
               lvi.iItem = ListView_GetItemCount(hwndLV);
           	lvi.iSubItem = 0;
				ListView_InsertItem(hwndLV,&lvi);
               lvhi.iItem = lvi.iItem;
               lvhi.iSubItem = 0;
               lvi.lParam = (LPARAM)pb;
           	lvi.mask = LVIF_PARAM;
				ListView_SetItem(hwndLV,&lvi);
           }
           else{
           	lvi.mask = LVIF_PARAM;
           	lvi.iItem = lvhi.iItem;
           	ListView_GetItem(hwndLV,&lvi);
				pb = (LBreakPoint *)lvi.lParam;
           }
			ListView_EnsureVisible(hwndLV,lvhi.iItem,TRUE);
           ListView_GetSubItemRect(hwndLV,lvhi.iItem,lvhi.iSubItem,LVIR_LABEL,&rc);
           si.cbSize = sizeof(SCROLLINFO);
           si.fMask = SIF_ALL;
           GetScrollInfo(hwndLV,SB_HORZ,&si);
           ::GetClientRect(hwndLV,&rcClient);
           if(rc.left < 0 || si.nPos + rc.right > rcClient.right){
          		ListView_Scroll(hwndLV,rc.left - si.nPos,0);
               ListView_GetSubItemRect(hwndLV,lvhi.iItem,lvhi.iSubItem,LVIR_LABEL,&rc);
           }
           lvi.mask = LVIF_TEXT;
           lvi.iItem = lvhi.iItem;
           lvi.iSubItem = lvhi.iSubItem;
           lvi.pszText = s;
           lvi.cchTextMax = 50;
           ListView_GetItem(hwndLV,&lvi);
           if(currentType == BT_PROGRAM){
           	if(!InputText(hwndLV,&rc,0,s,100)){
               	if(item == -1 && lvhi.iSubItem == 0){
                   	LList::Delete(pb);
                       ListView_DeleteItem(hwndLV,lvhi.iItem);
                   }
               	return 0;
               }
               switch(lvhi.iSubItem){
               	case 0:
                   	i = StrToHex(s);
                   	if(lstrlen(s) == 0 || Find((unsigned long)i,BT_PROGRAM)){
                           if(item == -1){
                               LList::Delete(pb);
                               ListView_DeleteItem(hwndLV,lvhi.iItem);
                           }
                       	return 0;
                       }
                       wsprintf(s,"0x%08X",i);
                       pb->set_Address((unsigned long)i);
                       if(item == -1)
							UpdateList(hwndLV,BT_PROGRAM);
                   break;
                   case 1:
                   	pb->set_PassCount(StrToHex(s));
                   break;
                   case 2:
                   	pb->StringToCondition(s);
                       lstrcpy(s,pb->ConditionToString(BT_PROGRAM).c_str());
                   break;
                   case 3:
                       pb->set_Description(s);
                   break;
               }
               ListView_SetItem(hwndLV,&lvi);
           }
           else{
           	switch(lvhi.iSubItem){
                   case 6:
              			if(!InputText(hwndLV,&rc,0,s,50))
                       	return 0;
                       ListView_SetItem(hwndLV,&lvi);
                       pb->set_Description(s);
                   break;
           		default:
              			if(!InputText(hwndLV,&rc,0,s,30)){
               			if(item == -1 && lvhi.iSubItem == 0){
                   			LList::Delete(pb);
                       		ListView_DeleteItem(hwndLV,lvhi.iItem);
                   		}
                       	return 0;
                       }
						i1 = 0;
                       if((p1 = strchr(s,'-')) != NULL){
                           *p1++ = 0;
                           i1 = StrToHex(p1);
                       }
                   	i = StrToHex(s);
                   	if(lstrlen(s) == 0 || Find((unsigned long)i,BT_MEMORY)){
                           if(item == -1){
                               LList::Delete(pb);
                               ListView_DeleteItem(hwndLV,lvhi.iItem);
                           }
                       	return 0;
                       }
                       wsprintf(s,"0x%08X",i);
                       pb->set_Address((unsigned long)i);
                       ListView_SetItem(hwndLV,&lvi);
                       if(item == -1){
                       	pb->set_Enable();
                           pb->set_Flags(0xB3);
                           if(i1)
                           	pb->set_Address2((unsigned long)i1);
                           UpdateList(hwndLV,BT_MEMORY);
                       }
               	break;
                   case 1:
                   	strcpy(s1,s3[0]);
                       p1 = s1 + strlen(s1);
                       for(i=1;i<sizeof(s3)/sizeof(s3[0]);i++){
                       	strcpy(p1+1,s3[i]);
                           p1 += strlen(p1+1) + 1;
                       }
                       *(p1 + 1) = 0;
                       lstrcpy(p1+2,s);
						if(InputCombo(hwndLV,&rc,0,s1,30)){
                           if((item = *((int *)s1)) != CB_ERR){
                       		lstrcpy(s,&s1[4]);
                           	ListView_SetItem(hwndLV,&lvi);
                               i = item == 0 || item == 3 || item == 4 || item == 6 ? AMM_BYTE : 0;
                               i |= item == 1 || item == 3 || item == 5 || item == 6 ? AMM_HWORD : 0;
                               i |= item == 2 || item == 4 || item == 6 || item == 5 ? AMM_WORD : 0;
                               pb->set_Access((char)i);
                           }
                       }
                   break;
                   case 2:
                   	strcpy(s1,s5[0]);
                       p1 = s1 + strlen(s1);
                       for(i=1;i<sizeof(s5)/sizeof(s5[0]);i++){
                       	strcpy(p1+1,s5[i]);
                           p1 += strlen(p1+1) + 1;
                       }
                       *(p1 + 1) = 0;
                       lstrcpy(p1+2,s);
						if(InputCombo(hwndLV,&rc,0,s1,30)){
                           if((item = *((int *)s1)) != CB_ERR){
                           	pb->set_Processor(item);
                       		lstrcpy(s,&s1[4]);
                           	ListView_SetItem(hwndLV,&lvi);
                           }
                       }
                   break;
                   case 3:
                   	strcpy(s1,s2[0]);
                       p1 = s1 + strlen(s1);
                       for(i=1;i<sizeof(s2)/sizeof(s2[0]);i++){
                       	strcpy(p1+1,s2[i]);
                           p1 += strlen(p1+1) + 1;
                       }
                       *(p1 + 1) = 0;
                       lstrcpy(p1+2,s);
						if(InputCombo(hwndLV,&rc,0,s1,30)){
                           if((item = *((int *)s1)) != CB_ERR){
                       		lstrcpy(s,&s1[4]);
                           	ListView_SetItem(hwndLV,&lvi);
                               pb->set_Read(item == 0 || item == 3 ? TRUE : FALSE);
                               pb->set_Write(item == 1 || item == 3 ? TRUE : FALSE);
                               pb->set_Modify(item == 2 ? TRUE : FALSE);
                           }
                       }
                   break;
                   case 4:
                   	strcpy(s1,s4[0]);
                       p1 = s1 + strlen(s1);
                       for(i=1;i<sizeof(s4)/sizeof(s4[0]);i++){
                       	strcpy(p1+1,s4[i]);
                           p1 += strlen(p1+1) + 1;
                       }
                       *(p1 + 1) = 0;
                       lstrcpy(p1+2,s);
						if(InputCombo(hwndLV,&rc,0,s1,30)){
                           if((item = *((int *)s1)) != CB_ERR){
                               pb->set_Break(item == 0 ? TRUE : FALSE);
                       		lstrcpy(s,&s1[4]);
                           	ListView_SetItem(hwndLV,&lvi);
                           }
                       }
                   break;
                   case 5:
                       if(InputText(hwndLV,&rc,0,s,30)){
                   	    pb->StringToCondition(s,BT_MEMORY);
                           lstrcpy(s,pb->ConditionToString(BT_MEMORY).c_str());
                           ListView_SetItem(hwndLV,&lvi);
                       }
                   break;
           	}
           }
           bModified = TRUE;
           debugDlg.UpdateToolBar();
       break;
   	case LVN_ITEMCHANGED:
       	if((p->uChanged & LVIF_STATE)){
           	if((p->uNewState & 0x3000)){
                   if((pb = (LBreakPoint *)p->lParam) != NULL){
                   	pb->set_Enable(p->uNewState & 0x2000 ? TRUE : FALSE);
                       bModified = TRUE;
                       hwndLV = GetDlgItem(debugDlg.Handle(),IDC_DEBUG_DIS);
                       InvalidateRect(hwndLV,NULL,TRUE);
                       UpdateWindow(hwndLV);
                       debugDlg.UpdateToolBar();
                   }
               }
           }
       break;
   }
   return 0;
}
//---------------------------------------------------------------------------
BOOL LListBreakPoint::UpdateList(HWND hwndLV,int type)
{
   LBreakPoint *p;
	LV_ITEM lvi={0};
   DWORD dwPos;
	char s[100];
	int i,i1;

	ListView_DeleteAllItems(hwndLV);
	p = (LBreakPoint *)GetFirstItem(&dwPos);
   i = 0;
   while(p != NULL){
   	if(p->get_Type() == type){
       	lvi.mask = LVIF_TEXT;
           lvi.iItem = i;
       	if(type == BT_MEMORY && p->has_Range())
           	wsprintf(s,"0x%08X - 0x%08X",p->get_Address(),p->get_Address2());
           else
           	wsprintf(s,"0x%08X",p->get_Address());
           lvi.pszText = s;
           lvi.iSubItem = 0;
			ListView_InsertItem(hwndLV,&lvi);
			ListView_SetItemState(hwndLV,lvi.iItem,INDEXTOSTATEIMAGEMASK(p->is_Enable()?2:1),
				LVIS_STATEIMAGEMASK);
           lvi.mask = LVIF_PARAM;
           lvi.lParam = (LPARAM)p;
			ListView_SetItem(hwndLV,&lvi);
			lvi.mask = LVIF_TEXT;
           lvi.pszText = s;
           switch(type){
           	case BT_PROGRAM:
               	wsprintf(s,"%u",p->get_PassCount());
                   lvi.iSubItem = 1;
					ListView_SetItem(hwndLV,&lvi);
                   lstrcpy(s,p->ConditionToString((unsigned char)type).c_str());
                   lvi.iSubItem = 2;
					ListView_SetItem(hwndLV,&lvi);
                   p->get_Description(s);
                   lvi.iSubItem = 3;
					ListView_SetItem(hwndLV,&lvi);
#ifdef _DEBPRO
               	wsprintf(s,"%u",p->get_InternalPassCount());
                   lvi.iSubItem = 4;
					ListView_SetItem(hwndLV,&lvi);
#endif
               break;
               case BT_MEMORY:
                   *((long *)s) = 0;
                   i1 = p->get_Access();
					if((i1 & AMM_BYTE))
                   	lstrcpy(s,"Byte");
                   if((i1 & AMM_HWORD)){
                   	if(*s != 0)
                       	lstrcat(s,"/");
                       lstrcat(s,"HWord");
					}
                   if((i1 & AMM_WORD)){
                   	if(*s != 0)
                       	lstrcat(s,"/");
                       lstrcat(s,"Word");
					}
					lvi.iSubItem = 1;
					ListView_SetItem(hwndLV,&lvi);
					*((long *)s) = 0;
					switch(p->get_Processor()){
                   	case 0:
                       	strcpy(s,"Both");
                       break;
                       case 1:
                       	strcpy(s,"Arm9");
                       break;
                       case 2:
                       	strcpy(s,"Arm7");
                       break;
                   }
					lvi.iSubItem = 2;
					ListView_SetItem(hwndLV,&lvi);
					*((long *)s) = 0;
                   if(p->is_Read())
                   	lstrcpy(s,"Read");
                   if(p->is_Write()){
                   	if(*s != 0)
                       	lstrcat(s,"/");
                       lstrcat(s,"Write");
                   }
                   if(p->is_Modify()){
                   	if(*s != 0)
                       	lstrcat(s,"/");
                       lstrcat(s,"Modify");
                   }
					lvi.iSubItem = 3;
					ListView_SetItem(hwndLV,&lvi);

					wsprintf(s,"%s",p->is_Break() ? "Yes" : "No");
					lvi.iSubItem = 4;
					ListView_SetItem(hwndLV,&lvi);
                   lstrcpy(s,p->ConditionToString((unsigned char)type).c_str());
                   lvi.iSubItem = 5;
					ListView_SetItem(hwndLV,&lvi);
                   p->get_Description(s);
                   lvi.iSubItem = 6;
					ListView_SetItem(hwndLV,&lvi);
               break;
           }
			i++;
       }
       p = (LBreakPoint *)GetNextItem(&dwPos);
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LListBreakPoint::OnShowList(HWND hwndLV,int type)
{
   int i;
   LV_COLUMN lvc={0};
   
   currentType = type;
   ListView_DeleteAllItems(hwndLV);
   while(ListView_DeleteColumn(hwndLV,0));
   lvc.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM|LVCF_FMT;
   lvc.fmt = LVCFMT_LEFT;
   lvc.cx = 100;
   lvc.pszText = "Address";
   ListView_InsertColumn(hwndLV,0,&lvc);
   i = 1;
   if(type == BT_PROGRAM){
       lvc.cx = 80;
       lvc.fmt = LVCFMT_RIGHT;
       lvc.pszText = "Pass";
       lvc.iSubItem = i;
       ListView_InsertColumn(hwndLV,i++,&lvc);
   }
   else{
       lvc.pszText = "Access";
       lvc.iSubItem = i;
       ListView_InsertColumn(hwndLV,i++,&lvc);
       lvc.cx = 80;                                                    
       lvc.pszText = "CPU";
       lvc.iSubItem = i;
       ListView_InsertColumn(hwndLV,i++,&lvc);
       lvc.cx = 100;
       lvc.pszText = "Type";
       lvc.iSubItem = i;
       ListView_InsertColumn(hwndLV,i++,&lvc);
       lvc.cx = 80;
       lvc.pszText = "Break";
       lvc.iSubItem = i;
       ListView_InsertColumn(hwndLV,i++,&lvc);
   }
   lvc.fmt = LVCFMT_LEFT;
   lvc.cx = 130;
   lvc.pszText = "Condition";
   ListView_InsertColumn(hwndLV,i++,&lvc);
   lvc.pszText = "Description";
   lvc.cx = 200;
   ListView_InsertColumn(hwndLV,i++,&lvc);
#ifdef _DEBPRO
   lvc.cx = 80;
   lvc.fmt = LVCFMT_RIGHT;
   lvc.pszText = "Pass Count Internal";
   lvc.iSubItem = i;
   ListView_InsertColumn(hwndLV,i++,&lvc);
#endif
   return UpdateList(hwndLV,type);
}
//---------------------------------------------------------------------------
void LListBreakPoint::Select(HWND hwndLV,LBreakPoint *p,BOOL bShow)
{
#ifdef _DEBPRO
   char s[100];
#endif
   LV_FINDINFO lfi={0};
   SCROLLINFO si = {0};
   int index;
   RECT rc,rcClient;
   LV_ITEM lvi={0};
   
   if(hwndLV == NULL || p == NULL || currentType != p->get_Type())
       return;
   lfi.flags = LVFI_PARAM;
   lfi.lParam = (LPARAM)p;
   index = ListView_FindItem(hwndLV,-1,&lfi);
   if(index == -1)
       return;
   if(bShow){
       ListView_GetSubItemRect(hwndLV,index,p->get_Type() == BT_PROGRAM ? 3 : 4,LVIR_LABEL,&rc);
       si.cbSize = sizeof(SCROLLINFO);
       si.fMask = SIF_ALL;
       GetScrollInfo(hwndLV,SB_HORZ,&si);
       ::GetClientRect(hwndLV,&rcClient);
       if(rc.left < 0 || si.nPos + rc.right > rcClient.right)
           ListView_Scroll(hwndLV,rc.left - si.nPos,0);
   }
   ListView_SetItemState(hwndLV,index,LVIS_SELECTED,0xF);
#ifdef _DEBPRO

   lvi.pszText = s;
	for(index = 0;index < ListView_GetItemCount(hwndLV);index++){
       lvi.mask = LVIF_PARAM;
       lvi.iItem = index;
		ListView_GetItem(hwndLV,&lvi);
       lvi.iSubItem = currentType == BT_PROGRAM ? 4 : 8;
       lvi.mask = LVIF_TEXT;
       p = (LBreakPoint *)lvi.lParam;
       wsprintf(s,"%u",p->get_InternalPassCount());
       ListView_SetItem(hwndLV,&lvi);
   }
#endif
   ListView_EnsureVisible(hwndLV,index,FALSE);
}
//---------------------------------------------------------------------------
void LListBreakPoint::DeleteElem(LPVOID p)
{
	delete (LBreakPoint *)p;
}
//---------------------------------------------------------------------------
LRESULT LListBreakPoint::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	LV_ITEM lvi={0};
	int i;
   HWND hwnd;

   switch(uMsg){
   	case WM_COMMAND:
			if(HIWORD(wParam) < 2){
           	switch(LOWORD(wParam)){
#ifdef _DEBPRO
					case ID_BP_RESET_INTERNAL_COUNT:
                   	i = ListView_GetNextItem(m_hWnd,-1,LVNI_ALL|LVNI_SELECTED);
                       if(i != -1){
                            lvi.iItem = i;
                            lvi.mask = LVIF_PARAM;
                            ListView_GetItem(m_hWnd,&lvi);
                            if(lvi.lParam != 0)
                            	((LBreakPoint *)lvi.lParam)->set_InternalPassCount();
						}
                   break;
#endif
               	case ID_BP_GO:
                   	i = ListView_GetNextItem(m_hWnd,-1,LVNI_ALL|LVNI_SELECTED);
                       if(i != -1){
                      		lvi.iItem = i;
                           lvi.mask = LVIF_PARAM;
                           ListView_GetItem(m_hWnd,&lvi);
							debugDlg.OnGoBP((LBreakPoint *)lvi.lParam);
                       }
                   break;
					case ID_BP_DELALL:
                       for(i=0;i<ListView_GetItemCount(m_hWnd);i++){
                       	lvi.iItem = i;
                           lvi.mask = LVIF_PARAM;
                           ListView_GetItem(m_hWnd,&lvi);
                           LList::Delete((LBreakPoint *)lvi.lParam);
                       }
                       ListView_DeleteAllItems(m_hWnd);
                       hwnd = GetDlgItem(debugDlg.Handle(),IDC_DEBUG_DIS);
                       InvalidateRect(hwnd,NULL,TRUE);
                       UpdateWindow(hwnd);
                   break;
                   case ID_BP_DISABLEALL:
                   case ID_BP_ENABLEALL:
                       for(i=0;i<ListView_GetItemCount(m_hWnd);i++){
                       	lvi.iItem = i;
                           lvi.mask = LVIF_PARAM;
                           ListView_GetItem(m_hWnd,&lvi);
                           if(lvi.lParam){
                           	((LBreakPoint *)lvi.lParam)->set_Enable(LOWORD(wParam) == ID_BP_ENABLEALL ? TRUE : FALSE);
								ListView_SetItemState(m_hWnd,i,
                               	INDEXTOSTATEIMAGEMASK(LOWORD(wParam) == ID_BP_ENABLEALL ? 2 : 1),
                                   LVIS_STATEIMAGEMASK);
							}
                       }
                   break;
               }                                           
           }
       break;
   }
	return LWnd::OnWindowProc(uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
void LListBreakPoint::Clear()
{
   LList::Clear();
   fileName = "";
}
#endif
