#include "ideastypes.h"
#include "arm7.h"
#include "syscnt.h"
#include "util.h"

SYSCNT sysCnt;

#define RECALC_ITCM_SIZE   \
   if(sysCnt.itcm_enable)\
       sysCnt.itcm_end = sysCnt.itcm_address + sysCnt.itcm_size;\
   else\
       sysCnt.itcm_end = 0;

#define RECALC_DTCM_SIZE   \
   if(sysCnt.dtcm_enable)\
       sysCnt.dtcm_end = sysCnt.dtcm_address + sysCnt.dtcm_size;\
   else\
       sysCnt.dtcm_end = 0;

//---------------------------------------------------------------------------
static void syscnt_rebuild_tcm_region(int flags)
{
	LPSYSRGN p;
   u32 i;

   if(flags & 1){
       sysCnt.itcm_pu_rgn_start = 0;
       sysCnt.itcm_pu_rgn_end = sysCnt.itcm_end;
       //Check PU Enabled
       if(sysCnt.reg1 & 1){
           u32 start,end;
           
           //try to find the region
           start = 0xFFFFFFFF;
           end = 0;
           for(p = &sysCnt.rgn[7],i=0;i<8;i++,p--){
               if(p->address_start > 0 && p->address_end < sysCnt.itcm_end){
                   if(p->address_start < start)
                       start = p->address_start;
                   if(p->address_end > end)
                       end = p->address_end;
               }
           }
           if(end != 0){
               sysCnt.itcm_pu_rgn_start = start;
               sysCnt.itcm_pu_rgn_end = end;
           }
       }
   }
}
//---------------------------------------------------------------------------
void syscnt_write(u32 opcode)
{
   u32 i,rd;
	LPSYSRGN p;
	u8 r2,op2;

    r2 = (u8)(opcode & 0xF);
    op2 = (u8)((opcode >> 5) & 7);    //0F00217A
    rd = arm9.r_gpreg((opcode >> 12) & 0xF);
    switch((opcode >> 16) & 0xF){
       case 1:
           if(!op2 && !r2){
       	    sysCnt.reg1 = rd;
				sysCnt.dtcm_enable = (u8)((rd >> 16) & 1);
           	sysCnt.itcm_enable = (u8)((rd >> 18) & 1);
               RECALC_ITCM_SIZE
               RECALC_DTCM_SIZE
               syscnt_rebuild_tcm_region(1);
           }
       break;
       case 0:
           rd = rd;
       break;
       case 5:
       	sysCnt.reg5[op2 &= 0x3] = rd;
          	if(op2 < 2){
          		for(i=0;i<8;i++){
          			sysCnt.rgn[i].ap[op2] = (u8)(rd & 0x3);
              		rd >>= 2;
          		}
          	}
          	else{
      		    for(i=0;i<8;i++){
          			sysCnt.rgn[i].ap[op2] = (u8)(rd & 0xF);
               	rd >>= 4;
           	}
           }
       break;
   	case 6:
           p = (LPSYSRGN)&sysCnt.rgn[r2 & 0x7];
           p->cnt = 0;
           if((i = (rd & 0x3E) >> 1) > 10 && i < 28){
           	p->size = (1 << (i + 1));
               p->address_start = rd & 0xFFFFF000;
				i = (1 << i);
               i |= ((i - 1) & 0xFFFFF000);
               if(rd & i)
                  	return;
               if(p->address_start == (rd & 0xFFFFF000)){
          			p->address_end = p->address_start + (p->size - 1);
          			p->cnt = (u8)(rd & 1);
               }
           }
       break;
		case 7:
       	switch(r2){
				case 0:
               	if(op2 == 4)
                   	arm9.w_stop(1);
               break;
               case 14:                              //2052158
               	if(op2 == 1){
                   }
/*	                if(0 == sysCnt.lastAddress)
  		                sysCnt.lastAddressCount++;
                   else{
                       sysCnt.lastAddress = 0;
                       sysCnt.lastAddressCount = 0;
                       sysCnt.loop = 0;
                   }*/
               break;
               case 6:
               	if(op2 == 1){
                   }
               break;
               case 8:
                   if(op2 == 2)
                       arm9.w_stop(1);
               break;
           }
       break;
       case 9:
       	switch(r2){
           	case 1:
               	switch(op2){
                   	case 0:
                           //sysCnt.dtcm_end
                   		sysCnt.dtcm_address = rd & 0xFFFFF000;
                       	sysCnt.dtcm_size = (1 << (((rd & 0x3E) >> 1) - 1)) << 10;
                           RECALC_DTCM_SIZE
                           syscnt_rebuild_tcm_region(0);
#if defined(_DEBUG)
                           arm9.r_memmap()[14].Address = sysCnt.dtcm_address;
                           arm9.r_memmap()[14].vSize = sysCnt.dtcm_size;
#endif
                       break;
                       case 1:
                   		sysCnt.itcm_address = rd & 0xFFFFF000;
                       	sysCnt.itcm_size = (1 << (((rd & 0x3E) >> 1) - 1)) << 10;
                           RECALC_ITCM_SIZE
                           syscnt_rebuild_tcm_region(1);                           
#if defined(_DEBUG)
                           arm9.r_memmap()[13].Address = sysCnt.itcm_address;
                           arm9.r_memmap()[13].vSize = sysCnt.itcm_size;
#endif
						break;
                   }
               break;
           }
       break;
       case 15:
           switch(r2){
               case 8:
                   if(op2 == 2)
                       arm9.w_stop(1);
               break;
           }
       break;
       default:
       	rd = rd;
       break;
   }
	UpdateControlProcessor((u8)((arm9.r_opcode() >> 16) & 0xF),r2,op2);
}
//---------------------------------------------------------------------------
u32 syscnt_read(u32 opcode)
{
	u8 r2,op2;

   r2 = (u8)(opcode & 0xF);
   op2 = (u8)((opcode >> 5) & 7);
	WriteMsgConsolle(&arm9,"%cRead System Control %d,%d,%d",MSGT_CP,(int)((opcode >> 16) & 0xF),r2,op2);
	switch((opcode >> 16) & 0xF){
       case 2:
           r2 = r2;
       break;
   	case 0:
           if(op2 == 1)
               return sysCnt.reg1;
       	return sysCnt.reg0;
   	case 1:
       	if(!op2 && !r2)
       		return sysCnt.reg1;
		case 5:
       	return sysCnt.reg5[op2 & 0x3];
       case 9:
       	switch(r2){
           	case 1:
               	if(op2 == 0)
                   	return (sysCnt.dtcm_address | ((log2(sysCnt.dtcm_size >> 10) + 1) << 1));
                   if(op2 == 1)
                   	return (sysCnt.itcm_address | ((log2(sysCnt.itcm_size >> 10) + 1) << 1));
               break;
           }
       break;
       default:
           opcode = opcode;
       break;
   }
	return 0;
}
//---------------------------------------------------------------------------
void syscnt_reset()
{
	int i;
   LPSYSRGN p;

	sysCnt.itcm_address = 0;
   sysCnt.dtcm_address = 0x800000;
   sysCnt.itcm_end = sysCnt.itcm_size = 32768;
   sysCnt.dtcm_size = 16384;
   sysCnt.dtcm_end = sysCnt.dtcm_address + sysCnt.dtcm_size;
#if defined(_DEBUG)
   arm9.r_memmap()[14].Address = sysCnt.dtcm_address;
   arm9.r_memmap()[13].Address = sysCnt.itcm_address;
#endif
   sysCnt.dtcm_enable = 1;
   sysCnt.itcm_enable = 1;
   RECALC_ITCM_SIZE
   RECALC_DTCM_SIZE
   
/*   sysCnt.reg0 = 0x0F0D2112;//0x41059461;
   sysCnt.reg1 = 0x41059461;*/
   sysCnt.reg0 = 0x41059461;
   sysCnt.reg1 = 0x0F0D2112;

   for(p = &sysCnt.rgn[7],i=0;i<8;i++,p--){
   	*((u32 *)p->ap) = (u32)-1;
       p->cnt = 0;
       p->address_start = p->address_end = 0;
       p->size = 0;
   }
}
//---------------------------------------------------------------------------
BOOL syscnt_Save(LStream *pFile)
{
   pFile->Write(&sysCnt.itcm_address,sizeof(sysCnt.itcm_address));
   pFile->Write(&sysCnt.dtcm_address,sizeof(sysCnt.dtcm_address));
   pFile->Write(&sysCnt.itcm_end,sizeof(sysCnt.itcm_end));
   pFile->Write(&sysCnt.itcm_size,sizeof(sysCnt.itcm_size));
   pFile->Write(&sysCnt.dtcm_size,sizeof(sysCnt.dtcm_size));
   pFile->Write(&sysCnt.dtcm_end,sizeof(sysCnt.dtcm_end));
   pFile->Write(&sysCnt.dtcm_enable,sizeof(sysCnt.dtcm_enable));
   pFile->Write(&sysCnt.reg0,sizeof(sysCnt.reg0));
   pFile->Write(&sysCnt.reg1,sizeof(sysCnt.reg1));
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL syscnt_Load(LStream *pFile,int ver)
{
	pFile->Read(&sysCnt.itcm_address,sizeof(sysCnt.itcm_address));
   pFile->Read(&sysCnt.dtcm_address,sizeof(sysCnt.dtcm_address));
   pFile->Read(&sysCnt.itcm_end,sizeof(sysCnt.itcm_end));
   pFile->Read(&sysCnt.itcm_size,sizeof(sysCnt.itcm_size));
   pFile->Read(&sysCnt.dtcm_size,sizeof(sysCnt.dtcm_size));
   pFile->Read(&sysCnt.dtcm_end,sizeof(sysCnt.dtcm_end));
   pFile->Read(&sysCnt.dtcm_enable,sizeof(sysCnt.dtcm_enable));
   pFile->Read(&sysCnt.reg0,sizeof(sysCnt.reg0));
   pFile->Read(&sysCnt.reg1,sizeof(sysCnt.reg1));
   RECALC_ITCM_SIZE
   RECALC_DTCM_SIZE
#if defined(_DEBUG)
   arm9.r_memmap()[14].Address = sysCnt.dtcm_address;
   arm9.r_memmap()[14].vSize = sysCnt.dtcm_size;
   arm9.r_memmap()[13].Address = sysCnt.itcm_address;
   arm9.r_memmap()[13].vSize = sysCnt.itcm_size;
#endif
	return TRUE;
}
//---------------------------------------------------------------------------
u8 I_FASTCALL syscnt_can_operate(u32 address,u8 oper)
{
	int ap;
   LPSYSRGN p;

	if((sysCnt.reg1 & 1) == 0)
   	return 1;
   return 1;

   ap = 1 + (oper & (SCNT_DATA|SCNT_INS));
   for(p = &sysCnt.rgn[7];p >= sysCnt.rgn;p--){
   	if(!p->cnt || address < p->address_start || address > p->address_end)
       	continue;
       if(!p->ap[ap])
           return 0;
       if((oper & AMM_READ))
          	break;
		if(p->ap[ap] & 0xC)
          	return 0;
       break;
   }
	return 1;
}
#if defined(_DEBPRO)
//---------------------------------------------------------------------------
void syscnt_get_reg1(TV_ITEM *p,HWND hwndTV)
{
	HTREEITEM h;
   int i;
	char s[100];

   h = TreeView_GetParent(hwndTV,p->hItem);
   if(h == NULL)
   	return;
   h = TreeView_GetChild(hwndTV,h);
   for(i=0;h != NULL && h != p->hItem;i++)
   	h = TreeView_GetNextSibling(hwndTV,h);
	if(h == NULL)
   	return;
	switch(i){
   	case 0:
			wsprintf(s,"Protection Unit : %d",(int)(sysCnt.reg1 & 1));
       break;
   	case 1:
			wsprintf(s,"Alignment fault : %d",(int)((sysCnt.reg1 >> 1) & 1));
       break;
       case 2:
       	wsprintf(s,"Data Cache Enable : %d",(int)((sysCnt.reg1 >> 2) & 1));
       break;
       case 3:
       	wsprintf(s,"Write Buffer Enable : %d",(int)((sysCnt.reg1 >> 3) & 1));
       break;
       case 4:
       	wsprintf(s,"Exception Handlers : %d",(int)((sysCnt.reg1 >> 4) & 1));
       break;
       case 5:
       	wsprintf(s,"26-bits Address Exception Checking Enable : %d",(int)((sysCnt.reg1 >> 5) & 1));
       break;
       case 6:
       	wsprintf(s,"Abort Model Selected : %d",(int)((sysCnt.reg1 >> 6) & 1));
       break;
       case 7:
       	wsprintf(s,"Endianness Memory System : %d",(int)((sysCnt.reg1 >> 7) & 1));
       break;
       case 8:
       	wsprintf(s,"System Protection Unit : %d",(int)((sysCnt.reg1 >> 8) & 1));
       break;
       case 9:
       	wsprintf(s,"ROM Protection Unit : %d",(int)((sysCnt.reg1 >> 9) & 1));
       break;
       case 10:
       	wsprintf(s,"Branch Prediction Mode : %d",(int)((sysCnt.reg1 >> 10) & 1));
       break;
       case 12:
       	wsprintf(s,"Instruction Cache Enable : %d",(int)((sysCnt.reg1 >> 12) & 1));
       break;
       case 13:
       	wsprintf(s,"High Exception Vector : %d",(int)((sysCnt.reg1 >> 13) & 1));
       break;
       case 16:
			wsprintf(s,"DTCM Enable : %d",(int)((sysCnt.reg1 >> 16) & 1));
       break;
       case 17:
       	wsprintf(s,"DTCM Load Enable : %d",(int)((sysCnt.reg1 >> 17) & 1));
       break;
       case 18:
			wsprintf(s,"ITCM Enable : %d",(int)((sysCnt.reg1 >> 18) & 1));
       break;
       case 19:
       	wsprintf(s,"ITCM Load Enable : %d",(int)((sysCnt.reg1 >> 19) & 1));
       break;
       default:
       	lstrcpy(s," ");
       break;
   }
   p->pszText = s;
}
//---------------------------------------------------------------------------
void syscnt_show_reg1(TV_ITEM *p,HWND hwndTV)
{
   TV_INSERTSTRUCT tvi={NULL,TVI_LAST,{TVIF_TEXT|TVIF_PARAM,NULL,0,0,NULL,0,0,0,0}};
   int i;

	if((p->mask & TVIF_CHILDREN)){
		if((p->mask & TVIF_HANDLE)){
       	if(TreeView_GetChild(hwndTV,p->hItem) == NULL){
              	tvi.item.lParam = (LPARAM)syscnt_get_reg1;
				tvi.hParent = p->hItem;
              	tvi.item.pszText = LPSTR_TEXTCALLBACK;
               for(i=0;i<32;i++)
					TreeView_InsertItem(hwndTV,&tvi);
           }
       }
       else
   		p->cChildren = 1;
   }
}
//---------------------------------------------------------------------------
void syscnt_get_reg9(TV_ITEM *p,HWND hwndTV)
{
	HTREEITEM h;
   int i;
	char s[100];
   TV_ITEM tvi={0};

   h = TreeView_GetParent(hwndTV,p->hItem);
   if(h == NULL)
   	return;
   tvi.hItem = h;
   tvi.mask = TVIF_TEXT;
   tvi.pszText = s;
   tvi.cchTextMax = 100;
   TreeView_GetItem(hwndTV,&tvi);
   h = TreeView_GetChild(hwndTV,h);
   for(i=0;h != NULL && h != p->hItem;i++)
   	h = TreeView_GetNextSibling(hwndTV,h);
	if(h == NULL)
   	return;
   if(!lstrcmpi(s,"itcm")){
		if(i == 0)
       	wsprintf(s,"Address : 0x%08X",sysCnt.itcm_address);
       else
			wsprintf(s,"Size : 0x%08X",sysCnt.itcm_size);
   }
   else{
		if(i == 0)
       	wsprintf(s,"Address : 0x%08X",sysCnt.dtcm_address);
       else
			wsprintf(s,"Size : 0x%08X",sysCnt.dtcm_size);
   }
   p->pszText = s;
}
//---------------------------------------------------------------------------
void syscnt_show_reg9(TV_ITEM *p,HWND hwndTV)
{
   TV_INSERTSTRUCT tvi={NULL,TVI_LAST,{TVIF_TEXT,NULL,0,0,NULL,0,0,0,0}};
   int i;
	HTREEITEM h;

	if((p->mask & TVIF_CHILDREN)){
		if((p->mask & TVIF_HANDLE) && TreeView_GetChild(hwndTV,p->hItem) == NULL){
			tvi.hParent = p->hItem;
           tvi.item.pszText = "ITCM";
           h = TreeView_InsertItem(hwndTV,&tvi);
           tvi.item.mask |= TVIF_PARAM;
           tvi.item.lParam = (LPARAM)syscnt_get_reg9;
           tvi.hParent = h;
           tvi.item.pszText = LPSTR_TEXTCALLBACK;
           for(i=0;i<2;i++)
				TreeView_InsertItem(hwndTV,&tvi);
			tvi.hParent = p->hItem;
           tvi.item.pszText = "DTCM";
           h = TreeView_InsertItem(hwndTV,&tvi);
           tvi.item.mask |= TVIF_PARAM;
           tvi.item.lParam = (LPARAM)syscnt_get_reg9;
           tvi.hParent = h;
           tvi.item.pszText = LPSTR_TEXTCALLBACK;
           for(i=0;i<2;i++)
				TreeView_InsertItem(hwndTV,&tvi);
   	}
       else
   		p->cChildren = 1;
   }
}
//---------------------------------------------------------------------------
void syscnt_get_reg6(TV_ITEM *p,HWND hwndTV)
{
	HTREEITEM h;
   int i;
	char s[100];
   TV_ITEM tvi={0};
   LPSYSRGN p2;

   h = TreeView_GetParent(hwndTV,p->hItem);
   if(h == NULL)
   	return;
   tvi.hItem = h;
   tvi.mask = TVIF_TEXT;
   tvi.pszText = s;
   tvi.cchTextMax = 100;
   TreeView_GetItem(hwndTV,&tvi);
   h = TreeView_GetChild(hwndTV,h);
   for(i=0;h != NULL && h != p->hItem;i++)
   	h = TreeView_GetNextSibling(hwndTV,h);
	if(h == NULL)
   	return;
   p2 = &sysCnt.rgn[atoi(&s[7]) & 7];
   if(i == 0)
      	wsprintf(s,"Address : 0x%08X",p2->address_start);
   else
		wsprintf(s,"End : 0x%08X",p2->address_end);
   p->pszText = s;
}
//---------------------------------------------------------------------------
void syscnt_show_reg6(TV_ITEM *p,HWND hwndTV)
{
   TV_INSERTSTRUCT tvi={NULL,TVI_LAST,{TVIF_TEXT,NULL,0,0,NULL,0,0,0,0}};
   int i,i1;
	HTREEITEM h;
	char s[30];

	if((p->mask & TVIF_CHILDREN)){
		if((p->mask & TVIF_HANDLE) && TreeView_GetChild(hwndTV,p->hItem) == NULL){
           for(i=0;i<8;i++){
           	tvi.item.mask &= ~TVIF_PARAM;
				tvi.hParent = p->hItem;
               wsprintf(s,"Region %d",i);
           	tvi.item.pszText = s;
           	h = TreeView_InsertItem(hwndTV,&tvi);
           	tvi.item.mask |= TVIF_PARAM;
           	tvi.item.lParam = (LPARAM)syscnt_get_reg6;
           	tvi.hParent = h;
           	tvi.item.pszText = LPSTR_TEXTCALLBACK;
           	for(i1=0;i1<2;i1++)
					TreeView_InsertItem(hwndTV,&tvi);
           }
   	}
       else
   		p->cChildren = 1;
   }
}
//---------------------------------------------------------------------------
void syscnt_get_reg5(TV_ITEM *p,HWND hwndTV)
{
	HTREEITEM h;
   int i,i2;
	char s[100];
   TV_ITEM tvi={0};
   LPSYSRGN p2;

   h = p->hItem;
   i2 = 0;
   do{
   	h = TreeView_GetParent(hwndTV,h);
   	if(h == NULL)
   		return;
   	tvi.hItem = h;
   	tvi.mask = TVIF_TEXT;
   	tvi.pszText = s;
   	tvi.cchTextMax = 100;
   	TreeView_GetItem(hwndTV,&tvi);
       i2++;
   }while(strstr(s,"Region") == NULL);
	h = TreeView_GetParent(hwndTV,p->hItem);

   h = TreeView_GetChild(hwndTV,h);
   for(i=0;h != NULL && h != p->hItem;i++)
   	h = TreeView_GetNextSibling(hwndTV,h);
	if(h == NULL)
   	return;
   p2 = &sysCnt.rgn[atoi(&s[7]) & 7];
   if(i2 == 1)
		wsprintf(s,"Enable : %d",p2->cnt);
   else{
   	switch(i){
           case 0:
           case 1:
           case 2:
           case 3:
           case 5:
           case 6:
				lstrcpy(s," ");
           break;
       	default:
       		lstrcpy(s,"UNP");
       	break;
   	}
   }
   p->pszText = s;
}
//---------------------------------------------------------------------------
void syscnt_show_reg5(TV_ITEM *p,HWND hwndTV)
{
   TV_INSERTSTRUCT tvi={NULL,TVI_LAST,{TVIF_TEXT,NULL,0,0,NULL,0,0,0,0}};
   int i,i1;
	HTREEITEM h[3];
	char s[30];

	if((p->mask & TVIF_CHILDREN)){
		if((p->mask & TVIF_HANDLE) && TreeView_GetChild(hwndTV,p->hItem) == NULL){
           for(i=0;i<8;i++){
           	tvi.item.mask &= ~TVIF_PARAM;
				tvi.hParent = p->hItem;
               wsprintf(s,"Region %d",i);
           	tvi.item.pszText = s;
           	h[0] = TreeView_InsertItem(hwndTV,&tvi);
           	tvi.item.mask |= TVIF_PARAM;
           	tvi.item.lParam = (LPARAM)syscnt_get_reg5;
           	tvi.hParent = h[0];
           	tvi.item.pszText = LPSTR_TEXTCALLBACK;
               TreeView_InsertItem(hwndTV,&tvi);
				tvi.item.mask &= ~TVIF_PARAM;
				tvi.hParent = h[0];
           	tvi.item.pszText = "Data";
           	h[1] = TreeView_InsertItem(hwndTV,&tvi);
				tvi.item.mask |= TVIF_PARAM;
           	tvi.item.lParam = (LPARAM)syscnt_get_reg5;
           	tvi.hParent = h[1];
           	tvi.item.pszText = LPSTR_TEXTCALLBACK;
           	for(i1=0;i1<16;i1++)
					TreeView_InsertItem(hwndTV,&tvi);
				tvi.item.mask &= ~TVIF_PARAM;
				tvi.hParent = h[0];
           	tvi.item.pszText = "Instruction";
           	h[1] = TreeView_InsertItem(hwndTV,&tvi);
				tvi.item.mask |= TVIF_PARAM;
           	tvi.item.lParam = (LPARAM)syscnt_get_reg5;
           	tvi.hParent = h[1];
           	tvi.item.pszText = LPSTR_TEXTCALLBACK;
           	for(i1=0;i1<16;i1++)
					TreeView_InsertItem(hwndTV,&tvi);
           }
   	}
       else
   		p->cChildren = 1;
   }
}
#endif

