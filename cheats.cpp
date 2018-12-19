#include "cheats.h"
#include "lds.h"
#include "inputtext.h"
#include "util.h"
#include "cbds.h"
#include "language.h"
#include "lvec.hpp"
#include <7zCrc.h>

#ifdef READ_WORD
 	#undef READ_WORD
   #undef READ_HWORD
   #undef READ_BYTE
#endif
#ifdef WRITE_WORD
   #undef WRITE_WORD
   #undef WRITE_HWORD
	#undef WRITE_BYTE
#endif

#define READ_WORD(a) ((u32 I_FASTCALL (*)(u32))pfn_MemoryAccess[0])(a)
#define READ_HWORD(a) ((u16 I_FASTCALL (*)(u32))pfn_MemoryAccess[1])(a)
#define READ_BYTE(a) ((u8 I_FASTCALL (*)(u32))pfn_MemoryAccess[2])(a)

#define WRITE_WORD(a,b)((void I_FASTCALL (*)(u32,u32))pfn_MemoryAccess[3])(a,b)
#define WRITE_HWORD(a,b)((void I_FASTCALL (*)(u32,u32))pfn_MemoryAccess[4])(a,(u16)b)
#define WRITE_BYTE(a,b)((void I_FASTCALL (*)(u32,u32))pfn_MemoryAccess[5])(a,(u8)b)

LCheatsManager cheatsManager;
//---------------------------------------------------------------------------
static void *pfn_MemoryAccess[8];
static u32 ofsAR,dataAR,loopAR;
extern HINSTANCE hInst;
//---------------------------------------------------------------------------
static void I_FASTCALL write_byte_cheats(u32 adr,u32 value)
{
	WRITE_BYTE(adr,value);
   cheatsManager.EvaluateCheat(adr,AMM_BYTE);
}
//---------------------------------------------------------------------------
static void I_FASTCALL write_hword_cheats(u32 adr,u32 value)
{
	WRITE_HWORD(adr,value);
   cheatsManager.EvaluateCheat(adr,AMM_HWORD);
}
//---------------------------------------------------------------------------
static void I_FASTCALL write_word_cheats(u32 adr,u32 value)
{
	WRITE_WORD(adr,value);
   cheatsManager.EvaluateCheat(adr,AMM_WORD);
}
//---------------------------------------------------------------------------
static LPCHEATHEADER EvaluateCodeBreaker(LPCHEATHEADER p,u32 address,u8 accessMode)
{
	int i;
	u32 value,value1;

   switch(p->code){
   	case 0:
			WRITE_BYTE(p->adr,p->value);
       break;
       case 1:
			WRITE_HWORD(p->adr,p->value);
       break;
       case 2:
			WRITE_WORD(p->adr,(u32)p->value);
       break;
       case 3:
       	if((p->value & 0xFFF00000) == 0){
           	if(p->value & 0x00010000){
                   value = READ_HWORD(p->adr);
                   value = (u32)((s32)value + (s16)p->value);
					WRITE_HWORD(p->adr,value);
               }
               else{
                   value = READ_BYTE(p->adr);
                   value = (u32)((s32)value + (s8)p->value);
					WRITE_BYTE(p->adr,value);
               }
           }
           else{
           	value = READ_WORD(p->adr);
               value = (u32)((s32)value + (s32)p->value);
           	WRITE_WORD(p->adr,value);
           }
       break;
       case 4:
       	if(p->pNext != NULL && p->pNext != (LPCHEATHEADER)-1){
           	value = p->pNext->adr;
               address = p->adr;
				i = (p->value & 0x0FFF0000) >> 16;
               switch((p->value & 0x30000000) >> 28){
               	case 0:
           			for(;i>0;i--){
							WRITE_WORD(address,value);
                           address += (u16)p->value << 2;
                           value += p->pNext->value;
                       }
                   break;
                   case 1:
                   	for(;i>0;i--){
							WRITE_HWORD(address,value);
                           address += (u16)p->value << 1;
                           value += p->pNext->value;
                       }
                   break;
                   case 2:
                   	for(;i>0;i--){
                       	WRITE_BYTE(address,value);
                           address += (u16)p->value;
							value += p->pNext->value;
                       }
                   break;
               }
           }
       break;
       case 5:
       	if(p->pNext != NULL && p->pNext != (LPCHEATHEADER)-1){
           	value1 = p->pNext->adr;
           	for(value=0;value < p->value;value++){
					WRITE_BYTE(p->adr+value,READ_BYTE(value1++));
               }
           }
       break;
       case 6:

       break;
       case 7:
       	switch((p->value & 0x10000) >> 16){
           	case 0:
					value = READ_BYTE(p->adr);
               	switch((p->value & 0x300000) >> 20){
                   	case 0:
							WRITE_BYTE(p->adr,value|(u8)p->value);
                       break;
                       case 1:
							WRITE_BYTE(p->adr,value&(u8)p->value);
                       break;
                       case 2:
							WRITE_BYTE(p->adr,value^(u8)p->value);
                       break;
                   }
               break;
               case 1:
                  	value = READ_HWORD(p->adr);
               	switch((p->value & 0x300000) >> 20){
                   	case 0:
							WRITE_HWORD(p->adr,value | (u16)p->value);
                       break;
                       case 1:
							WRITE_HWORD(p->adr,value & (u16)p->value);
                       break;
                       case 2:
							WRITE_HWORD(p->adr,value ^ (u16)p->value);
                       break;
                   }
               break;
           }
       break;
   }
   if(p != NULL)
       return p->pNext;
   return NULL;
}
//---------------------------------------------------------------------------
static LPCHEATHEADER EvaluateActionReplay(LPCHEATHEADER p,u32 address,u8 accessMode)
{
   u32 i;

   switch(p->code){
       case 2:
           WRITE_BYTE(p->adr+ofsAR,p->value);
       break;
       case 1:
           WRITE_HWORD(p->adr+ofsAR,p->value);
       break;
       case 0:
           WRITE_WORD(p->adr+ofsAR,p->value);
       break;
       case 3:
       	if(p->value > READ_WORD(p->adr)){
               p = p->pNext;
           	while(p != NULL)
               	p = EvaluateActionReplay(p,p->adr,accessMode);
           }
		break;
       case 4:
       	if(p->value < READ_WORD(p->adr)){
               p = p->pNext;
           	while(p != NULL)
               	p = EvaluateActionReplay(p,p->adr,accessMode);
           }
		break;
       case 5:
       	if(p->value == READ_WORD(p->adr)){
               p = p->pNext;
           	while(p != NULL)
               	p = EvaluateActionReplay(p,p->adr,accessMode);
           }
		break;
       case 6:
       	if(p->value != READ_WORD(p->adr)){
               p = p->pNext;
           	while(p != NULL)
               	p = EvaluateActionReplay(p,p->adr,accessMode);
           }
		break;
       case 7:
       	if((u16)p->value > (READ_HWORD(p->adr) & ~(p->value >> 16))){
               p = p->pNext;
           	while(p != NULL)
               	p = EvaluateActionReplay(p,p->adr,accessMode);
           }
       break;
       case 8:
       	if((u16)p->value < (READ_HWORD(p->adr) & ~(p->value >> 16))){
               p = p->pNext;
           	while(p != NULL)
               	p = EvaluateActionReplay(p,p->adr,accessMode);
           }
       break;
       case 9:
       	if((u16)p->value == (READ_HWORD(p->adr) & ~(p->value >> 16))){
               p = p->pNext;
           	while(p != NULL)
               	p = EvaluateActionReplay(p,p->adr,accessMode);
           }
       break;
       case 0xA:
       	if((u16)p->value != (READ_HWORD(p->adr) & ~(p->value >> 16))){
               p = p->pNext;
           	while(p != NULL)
               	p = EvaluateActionReplay(p,p->adr,accessMode);
           }
       break;
       case 0xB:
			ofsAR = READ_WORD(p->adr+ofsAR);
       break;
       case 0xC0:
           address = (u32)p->pNext;
           for(loopAR = p->value;loopAR > 0;loopAR--){
               p = (LPCHEATHEADER)address;
           	while(p != NULL)
               	p = EvaluateActionReplay(p,p->adr,accessMode);
           }
           return NULL;
       break;
       case 0xD0:
       case 0xD2:
           if(loopAR < 2){
               ofsAR = 0;
               dataAR = 0;
           }
       break;
       case 0xD1:
           ofsAR = 0;
       break;
       case 0xD3:
       	ofsAR = p->value;
       break;
       case 0xD4:
       	dataAR += p->value;
       break;
       case 0xD5:
       	dataAR = p->value;
       break;
       case 0xD6:
			WRITE_WORD(p->value+ofsAR,dataAR);
       	ofsAR += 4;
       break;
       case 0xD7:
			WRITE_HWORD(p->value+ofsAR,dataAR);
       	ofsAR += 2;
       break;
       case 0xD8:
			WRITE_BYTE(p->value+ofsAR,dataAR);
       	ofsAR++;
       break;
       case 0xD9:
       	dataAR = READ_WORD(p->value+ofsAR);
		break;
       case 0xDA:
       	dataAR = (u32)READ_HWORD(p->value+ofsAR);
       break;
       case 0xDB:
       	dataAR = (u32)READ_BYTE(p->value+ofsAR);
       break;
		case 0xDC:
       	ofsAR += p->value;
       break;
       case 0xE:
           u8 i1;
           u32 value;

           i = p->value;
           address = p->adr + ofsAR;
           for(;p->pNext != NULL && i > 0;){
               p = p->pNext;
               value = p->adr;
               for(i1=0;i1<4 && i > 0;i1++,i--){
                   WRITE_BYTE(address++,(u8)(value >> (8 * (3 - i1))));
               }
               value = p->value;
               for(i1=0;i1<4 && i > 0;i1++,i--){
                   WRITE_BYTE(address++,(u8)(value >> (8 * (3 - i1))));
               }
           }
       break;
       case 0xF:
           for(i=0;i<p->value;i++)
               WRITE_BYTE(p->adr+i,READ_BYTE(ofsAR+i));
       break;
   }
   if(p != NULL)
       return p->pNext;
   return NULL;
}
//---------------------------------------------------------------------------
LCheatList::LCheatList(BOOL clone) : LList()
{
   maxAdr = 0;
   minAdr = (u32)-1;
   bClone = clone;
}
//---------------------------------------------------------------------------
LCheatList::~LCheatList()
{
   Clear();
}
//---------------------------------------------------------------------------
void LCheatList::DeleteElem(LPVOID ele)
{
	if(bClone)
   	return;
   LList::DeleteElem(ele);
}
//---------------------------------------------------------------------------
void LCheatList::Clear()
{
   LList::Clear();
   maxAdr = 0;
   minAdr = (u32)-1;
}
//---------------------------------------------------------------------------
BOOL LCheatList::Load(LStream *pFile)
{
   DWORD dw;
   char buffer[100],type[4];
	LPCHEATHEADER o,n;

   if(pFile == NULL || !pFile->IsOpen())
       return FALSE;
   Clear();
   o = n = NULL;
	do{
		ZeroMemory(buffer,sizeof(buffer));
       *((long *)type) = 0;
       if(pFile->Read(type,3) != 3)
       	break;
       if(pFile->Read(buffer,MAX_CHEATNAME) != MAX_CHEATNAME)
       	break;
       if(pFile->Read(&buffer[MAX_CHEATNAME+1],20) != 20)
       	break;
       if(lstrcmpi(type,"arr") == 0){
       	dw = ActionReplay;
       	n = cheatsManager.AddActionReplay(&buffer[MAX_CHEATNAME+1],buffer);
       }
       else if(lstrcmpi(type,"cbr") == 0){
       	dw = CodeBreaker;
       	n = cheatsManager.AddCodeBreaker(&buffer[MAX_CHEATNAME+1],buffer,(o != NULL && o->pNext == (LPCHEATHEADER)-1) ? o : NULL);
       }
		else if(lstrcmpi(type,"cbe") == 0){
       	dw = CodeBreakerEncrypt;
       	n = cheatsManager.AddCodeBreaker(&buffer[MAX_CHEATNAME+1],buffer,(o != NULL && o->pNext == (LPCHEATHEADER)-1) ? o : NULL,CodeBreakerEncrypt);
       }
       else{
       	dw = 0;
       	n = NULL;
       }
       pFile->Read(type,2);
       if(!n){
           if(o != NULL)
       		o->pNext = NULL;
       	o = NULL;
           continue;
       }
       if(dw == ActionReplay){
       	if(n->pNext == (LPCHEATHEADER)-2){
				n->skip = 1;
           	n->pNext = NULL;
           	if(o != NULL)
           		o->pNext = NULL;
       	}
       	else if(o != NULL && o->pNext == (LPCHEATHEADER)-1){
       		o->pNext = n;
           	n->skip = 1;
           	n->pNext = (LPCHEATHEADER)-1;
       	}
       }
       else if(dw == CodeBreaker || dw == CodeBreakerEncrypt){
			if(o != NULL && o->pNext == (LPCHEATHEADER)-1){
       		o->pNext = n;
           	n->skip = 1;
           }
       }
   	if(!Add(n)){
           if(o != NULL && o->pNext == n)
           	o->pNext = NULL;
           delete n;
       	continue;
   	}
       o = n;
   }while(1);
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LCheatList::Save(LStream *pFile)
{
   elem_list *tmp;
   LPCHEATHEADER cheatHeader;

   if(pFile == NULL || !pFile->IsOpen())
       return FALSE;
   if(nCount < 1)
       return TRUE;
   tmp = First;
   while(tmp != NULL){
       cheatHeader = (LPCHEATHEADER)tmp->Ele;
       switch(cheatHeader->type){
       	case ActionReplay:
           	pFile->Write((void *)"ARR",3);
           break;
           case CodeBreaker:
  	           	pFile->Write((void *)"CBR",3);
           break;
           case CodeBreakerEncrypt:
  	           	pFile->Write((void *)"CBE",3);
           break;
       }
       pFile->Write(cheatHeader->descr,MAX_CHEATNAME);
       pFile->Write(cheatHeader->codeString,20);
       pFile->Write((void *)"\r\n",2);
       tmp = tmp->Next;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LCheatList::Add(LPCHEATHEADER p)
{
	u32 adr;

   if(!LList::Add((LPVOID)p))
       return FALSE;
   if(p->skip || bClone)
   	return TRUE;
   if(p->adrEnd == (u32)-1){
       adr = p->adr & ~3;
       if(adr > maxAdr)
           maxAdr = adr + 4;
       if(adr < minAdr)
           minAdr = adr;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
void LCheatList::EvaluateCheat(u32 address,u8 accessMode)
{
   elem_list *tmp;
   LPCHEATHEADER cheatHeader;

   if(address > maxAdr || address < minAdr)
       return;
   tmp = First;
   switch(accessMode){
   	case AMM_BYTE:
       	while(tmp != NULL){
           	cheatHeader = (LPCHEATHEADER)(tmp->Ele);
               if(cheatHeader->skip)
                   break;
           	if(cheatHeader->Enable){
               	if(address == cheatHeader->adr){
               		if(cheatHeader->pEvaluateFunc != NULL)
                   		cheatHeader->pEvaluateFunc(cheatHeader,address,accessMode);
           		}
               }
               tmp = tmp->Next;
           }
       break;
       case AMM_HWORD:
       	while(tmp != NULL){
           	cheatHeader = (LPCHEATHEADER)(tmp->Ele);
               if(cheatHeader->skip)
                   break;
           	if(cheatHeader->Enable){
               	if(address >= (cheatHeader->adr & ~1) && address <= (cheatHeader->adr + 2)){
               		if(cheatHeader->pEvaluateFunc != NULL)
                   		cheatHeader->pEvaluateFunc(cheatHeader,address,accessMode);
           		}
               }
               tmp = tmp->Next;
           }
       break;
       case AMM_WORD:
       	while(tmp != NULL){
           	cheatHeader = (LPCHEATHEADER)(tmp->Ele);
               if(cheatHeader->skip)
                   break;
           	if(cheatHeader->Enable){
               	if(address >= (cheatHeader->adr & ~3) && address <= (cheatHeader->adr + 4)){
               		if(cheatHeader->pEvaluateFunc != NULL)
                   		cheatHeader->pEvaluateFunc(cheatHeader,address,accessMode);
           		}
               }
               tmp = tmp->Next;
           }
       break;
   }
/*   while(tmp != NULL){
       cheatHeader = (LPCHEATHEADER)tmp->Ele;
       if(cheatHeader->Enable && cheatHeader->skip == 0){
//       	if(address < cheatHeader->adr && (cheatHeader->adrEnd == 0xFFFFFFFF || address < cheatHeader->adrEnd))
//           	return;
       	if(cheatHeader->adrEnd == 0xFFFFFFFF){
           	switch(accessMode){
               	case AMM_BYTE:
           			if(address == cheatHeader->adr){
               			if(cheatHeader->pEvaluateFunc != NULL)
                   			cheatHeader->pEvaluateFunc(cheatHeader,address,accessMode);
           			}
                   break;
                   case AMM_HWORD:
           			if(address >= (cheatHeader->adr & ~1)&& address <= (cheatHeader->adr + 2)){
               			if(cheatHeader->pEvaluateFunc != NULL)
                   			cheatHeader->pEvaluateFunc(cheatHeader,address,accessMode);
           			}
                   break;
                   case AMM_WORD:
           			if(address >= (cheatHeader->adr & ~3) && address <= (cheatHeader->adr + 4)){
               			if(cheatHeader->pEvaluateFunc != NULL)
                   			cheatHeader->pEvaluateFunc(cheatHeader,address,accessMode);
           			}
                   break;
               }
       	}
       	else if(address < cheatHeader->adrEnd)
           	return;
       }
       tmp = tmp->Next;
   }*/
}
//---------------------------------------------------------------------------
LCheatsManager::LCheatsManager() : LDlg(),LCheatList(FALSE)
{
	bDrag = bAutoLoad = bEnable = bModified = FALSE;
	hilState = hilDrag = NULL;
   CrcGenerateTable();   
}
//---------------------------------------------------------------------------
LCheatsManager::~LCheatsManager()
{
	if(hilDrag != NULL)
   		ImageList_Destroy(hilDrag);
	if(m_hTV != NULL)
   		::DestroyWindow(m_hTV);
	if(hilState != NULL)
   		ImageList_Destroy(hilState);
	Clear();
}
//---------------------------------------------------------------------------
BOOL LCheatsManager::CreateTreeView()
{
   HWND hwndTT;
   HBITMAP bit;

   if(m_hTV == NULL){
#ifdef __WIN32__
		m_hTV = ::CreateWindowEx(0x200,"SysTreeView32","",0x50010923|TVS_EDITLABELS,0,0,0,0,ds.Handle(),(HMENU)IDC_TREE1,hInst,NULL);
		if(m_hTV == NULL)
   			return FALSE;
   }
#else
	    DLG_CONTROL_INFO info = {0};
   		const WCHAR className[]={'S','y','s','T','r','e','e','V','i','e','w','3','2',0,0};

	    info.style = 0x50010923|TVS_EDITLABELS;
	    info.id = IDC_TREE1;
	    info.className = (WCHAR *)className;
		info.classId = -1;
		m_hTV = DIALOG_CreateControl(&info,13,9,ds.Handle());
		if(m_hTV == NULL)
       	return FALSE;
		g_object_ref(m_hTV);
	}
	gtk_widget_show_all(m_hTV);
	gtk_fixed_put((GtkFixed *)GetWindowLong(m_hWnd,GWL_FIXED),m_hTV,0,0);
#endif
   hwndTT = (HWND)::SendMessage(m_hTV,TVM_GETTOOLTIPS,0,0);
   if(hwndTT != NULL)
   	::SendMessage(hwndTT,TTM_SETDELAYTIME,TTDT_AUTOPOP,20000);
   if(hilState != NULL)
   	return TRUE;
   hilState = ImageList_Create(16,16,ILC_COLOR16|ILC_MASK,3,3);
   if(hilState != NULL){
   	bit = LoadBitmap(hInst,MAKEINTRESOURCE(IDB_BITMAP_CHEATS));
   	if(bit != NULL){
   		ImageList_AddMasked(hilState,bit,RGB(255,0,255));
			TreeView_SetImageList(m_hTV,hilState,TVSIL_NORMAL);
           DeleteObject(bit);
       }
   }
   return TRUE;
}
//---------------------------------------------------------------------------
LPCHEATHEADER LCheatsManager::AddCodeBreaker(const char *lpCode,const char *lpDescr,LPCHEATHEADER prev,cheatType typeDef)
{
	u32 adr,value;
	u8 code,skip,sort;
	LPCODEBREAK p,pNext;
	u8 bEnable;
	cheatType type;

   if(!CheckCheatCode(&adr,&value,lpCode,CodeBreaker))
       return NULL;
#ifdef __WIN32__
   if(typeDef == CodeBreakerEncrypt || SendDlgItemMessage(m_hWnd,IDC_CHECK1,BM_GETCHECK,0,0) != BST_CHECKED){
   	DecryptCodeBreaker(&adr,&value);
       type = CodeBreakerEncrypt;
   }
   else
   	type = CodeBreaker;
#endif
   code = (u8)(adr >> 28);
	skip = 0;
   sort = 1;
   bEnable = TRUE;
   switch(code){
       case 0:
       case 1:
       case 2:
       case 3:
       case 7:
			pNext = NULL;
			adr &= 0xFFFFFFF;
       break;
       case 4:
       case 5:
       case 6:
       	pNext = (LPACTIONREPLAY)-1;
			adr &= 0xFFFFFFF;
       break;
   	default:
       	if(prev == NULL){
				MessageBox(m_hWnd,"Unsupported code.","iDeaS Emulator",MB_OK|MB_ICONERROR);
				return NULL;
           }
           pNext = NULL;
           bEnable = FALSE;
           skip = 1;
       break;
   }
   p = new CODEBREAK[1];
   if(p == NULL)
       return NULL;
   ZeroMemory(p,sizeof(CODEBREAK));
   p->pEvaluateFunc = EvaluateCodeBreaker;
   lstrcpyn(p->codeString,lpCode,20);
   if(lpDescr != NULL)
       lstrcpyn(p->descr,lpDescr,MAX_CHEATNAME+1);
   p->type = type;
   p->Enable = bEnable;
   p->adrEnd = (u32)-1;
   p->value = value;
  	p->adr = adr;
	p->code = code;
	p->pNext = pNext;
   p->pList = this;
   p->skip = skip;
   p->sort = sort;
   return p;
}
//---------------------------------------------------------------------------
LPCHEATHEADER LCheatsManager::AddActionReplay(const char *lpCode,const char *lpDescr,BOOL bCheck)
{
   LPACTIONREPLAY p,pNext;
   u32 adr,value;
   u8 code,skip,sort;

   if(!CheckCheatCode(&adr,&value,lpCode,ActionReplay))
       return NULL;
   sort = 0;
   if(!bCheck){
       skip = 1;
       code = 0xE;
       pNext = (LPACTIONREPLAY)-1;
       goto AddActionReplay_01;
   }
   code = (u8)(adr >> 28);
   skip = 0;
   switch(code){
       case 0:
       case 1:
       case 2:
           sort = 1;
			pNext = NULL;
       break;
       case 0x3:
       case 0x4:
       case 0x5:
       case 0x6:
       case 0x7:
       case 0x8:
       case 0x9:
       case 0xA:
           sort = 1;
       	pNext = (LPACTIONREPLAY)-1;
       break;
       case 0xB:
       	pNext = NULL;
       break;
       case 0xC:
           code = (u8)(adr >> 24);
           switch(code){
               case 0xC0:
       	        pNext = (LPACTIONREPLAY)-1;
                   goto AddActionReplay_01;
               break;
           }
       case 0xD:
       	code = (u8)(adr >> 24);
           switch(code){
           	case 0xD0:
               case 0xD2:
               case 0xD1:
					pNext = (LPACTIONREPLAY)-2;
               	goto AddActionReplay_01;
               case 0xD3:
               case 0xDC:
               case 0xD4:
               case 0xD5:
               case 0xD6:
               case 0xD7:
               case 0xD8:
               case 0xD9:
               case 0xDB:
               case 0xDA:
           		pNext = NULL;
//                   skip = 1;
                   goto AddActionReplay_01;
               break;
           }
       case 0xE:
           pNext = (LPACTIONREPLAY)-3;
       break;
       case 0xF:
			pNext = NULL;
       break;
       default:
       	MessageBox(m_hWnd,"Unsupported code.","iDeaS Emulator",MB_OK|MB_ICONERROR);
           return NULL;
       break;
   }
AddActionReplay_01:
   p = new ACTIONREPLAY[1];
   if(p == NULL)
       return NULL;
   ZeroMemory(p,sizeof(ACTIONREPLAY));
   p->pEvaluateFunc = EvaluateActionReplay;
   lstrcpyn(p->codeString,lpCode,20);
   if(lpDescr != NULL)
       lstrcpyn(p->descr,lpDescr,MAX_CHEATNAME+1);
   p->type = ActionReplay;
   p->Enable = TRUE;
   p->adrEnd = (u32)-1;
   p->value = value;
   if(bCheck)
       adr &= 0x0FFFFFFF;
  	p->adr = adr;
	p->code = code;
	p->pNext = pNext;
   p->pList = this;
   p->skip = skip;
   p->sort = sort;
   return p;
}
//---------------------------------------------------------------------------
BOOL LCheatsManager::CheckCheatCode(u32 *x,u32 *y,const char *lpCode,cheatType type)
{
   LString s,s1;
   int i;

   if(x == NULL || y == NULL || lpCode == NULL)
       return FALSE;
   s ="";
	for(i=0;lpCode[i] != 0;i++){
   	if(lpCode[i] == 32)
       	continue;
       s += lpCode[i];
   }
   if(s.Length() != 16){
       MessageBox(m_hWnd,"Invalid cheat code.","iDeaS Emualtor",MB_OK|MB_ICONERROR);
       return FALSE;
   }
   *x = *y = 0;
   s1 = "0x";
   s1 += s.SubString(1,8);
   *x = StrToHex(s1.c_str());
   s1 = "0x";
   s1 += s.SubString(9,s.Length() - 8);
   *y = StrToHex(s1.c_str());
   return TRUE;
}
//---------------------------------------------------------------------------
void LCheatsManager::Apply()
{
	DWORD dw;
   LPCHEATHEADER p;

   if(bEnable){
   	p = (LPCHEATHEADER)GetFirstItem(&dw);
   	for(;p != NULL;){
			if(p->skip == 0 && p->pEvaluateFunc != NULL)
           	p->pEvaluateFunc(p,p->adr,AMM_WORD);
			p = (LPCHEATHEADER)GetNextItem(&dw);
   	}
   }
}
//---------------------------------------------------------------------------
void LCheatsManager::DeleteCheat()
{
   LPCHEATHEADER p1;
	TV_ITEM tvi = {0};
   HTREEITEM hItem;
	LVector<LPCHEATHEADER> *pParam;
   int i;

   hItem = TreeView_GetSelection(m_hTV);
   if(hItem == NULL)
   	return;
   tvi.hItem = hItem;
   tvi.mask = TVIF_PARAM|TVIF_CHILDREN;
   TreeView_GetItem(m_hTV,&tvi);
   if(tvi.cChildren){
   	MessageBox(m_hWnd,"This item has childs","iDeaS Emulator",MB_OK|MB_ICONERROR);
   	return;
   }
   if(tvi.lParam != NULL){
		pParam = (LVector<LPCHEATHEADER> *)tvi.lParam;
   	for(i=1;i<=pParam->count();i++){
   		p1 = pParam->items(i);
       	p1->pList->Delete(p1);
   	}
   }
   TreeView_DeleteItem(m_hTV,hItem);
   ResetCheatSystem();
   bModified = TRUE;
}
//---------------------------------------------------------------------------
void LCheatsManager::ResetCheatSystem()
{
	u32 count;

   ofsAR = dataAR = 0;
   ZeroMemory(pfn_MemoryAccess,sizeof(pfn_MemoryAccess));
   if(bEnable){
   	count = 0;
       Enum(EnumCheatsReset,(LPARAM)&count);
       if(count){
			pfn_MemoryAccess[5] = (void *)write_byte_cheats;
			pfn_MemoryAccess[4] = (void *)write_hword_cheats;
			pfn_MemoryAccess[3] = (void *)write_word_cheats;
       }
   }
   arm9.EnableCheats(pfn_MemoryAccess);
}
//---------------------------------------------------------------------------
void LCheatsManager::DeleteAllCheats()
{
   TreeView_DeleteAllItems(m_hTV);
   Clear();
   ResetCheatSystem();
   bModified = TRUE;
}
//---------------------------------------------------------------------------
BOOL LCheatsManager::Load(const char *fileName)
{
   LFile *pFile;
   DWORD dwPos,dwRead,dwAR,dwCode;
	BOOL res;
   LString s,s1,s2,s3,sDescr,sCode;
	char buf[100];
	cheatType type;
	HTREEITEM hItem;
   int cheatCount,cc,folderCount,i,dataLen,j;

   res = FALSE;
   pFile = NULL;
   if(fileName == NULL || fileName[0] == 0){
   	s.Length(MAX_PATH+1);
		if(!ShowOpenDialog(NULL,"Cheats files (*.cht)\0*.cht;\0\0\0\0",m_hWnd,s.c_str(),MAX_PATH))
       	goto ex_Load;
   }
   else
   	s = fileName;
   pFile = new LFile(s.c_str());
   if(pFile == NULL)
       goto ex_Load;
   if(pFile->Open()){
   	s2 = "";
       s3 = "";
       sDescr = "";
       sCode = "";
       dwRead = 0;
       Clear();
       CreateTreeView();
       TreeView_DeleteAllItems(m_hTV);
       hItem = NULL;
		do{
           for(dwRead = 0;pFile->Read(&buf[dwRead],1) == 1 && buf[dwRead] != '\n';dwRead++);
			if(dwRead == 0)
           	break;
           if(buf[0] =='['){
           	if(!sCode.IsEmpty()){
               	s3 = sDescr;
                   s3 += "\\";
                   s3 += s2;
					InsertNewCheat(sCode.c_str(),s3.c_str(),type);
               }
               s2 = "";
           	s = "";
               sDescr = "";
               sCode = "";
   			for(dwPos = 1;dwPos<dwRead && buf[dwPos] != '\r' && buf[dwPos] != ']';dwPos++)
					s += buf[dwPos];
				s.AllTrim();
               if(s[1] == '\\'){
               	sDescr = "";
               	s = s.SubString(2,s.Length() - 1);
               }
              	sDescr += s;
               s.LowerCase();
               if(s == "ideas emulator")
					sDescr = "";
           }
           else{
               s = "";
               for(dwPos = 0;dwPos<3 && dwPos < dwRead;dwPos++)
               	s += buf[dwPos];
               s.UpperCase();
       		if(s == "ARR")
       			type = ActionReplay;
       		else if(s == "CBR")
       			type = CodeBreaker;
				else if(s == "CBE")
       			type = CodeBreakerEncrypt;
       		else
       			type = (cheatType)null;
				if(type == null)
               	continue;
               s = "";
               for(;dwPos < MAX_CHEATNAME + 3 && dwPos < dwRead;dwPos++)
					s += buf[dwPos];
               s1 = "";
				for(;dwPos < (MAX_CHEATNAME + 23) && dwPos < dwRead;dwPos++)
					s1 += buf[dwPos];
               s1.AllTrim();
               s.AllTrim();
               if(s2.IsEmpty() || s2 == s.c_str()){
					s2 = s;
                   if(!sCode.IsEmpty())
                   	sCode += "\r\n";
                   sCode += s1;
               }
               else{
               	s3 = sDescr;
                   s3 += "\\";
                   s3 += s2;
               	InsertNewCheat(sCode.c_str(),s3.c_str(),type);
                   s2 = s;
                   sCode = s1;
				}
           }
       }while(1);
       if(!sCode.IsEmpty()){
          	s3 = sDescr;
           s3 += "\\";
           s3 += s2;
       	InsertNewCheat(sCode.c_str(),s3.c_str(),type,hItem);
       }
       Sort(1,Count(),sortCheatList);
       bModified = FALSE;
   	ResetCheatSystem();
       Apply();
   	res = TRUE;
   }
ex_Load:
   if(pFile != NULL)
       delete pFile;
   if(res)
       return TRUE;
   s.Length(MAX_PATH + 1);
   GetModuleFileName(NULL,s.c_str(),MAX_PATH);
   s = s.Path();
   if(s[s.Length()] != DPC_PATH)
       s+= DPS_PATH;
   s += "USRCHEAT.DAT";
   if((pFile = new LFile(s.c_str())) == NULL)
       return FALSE;
   if(!pFile->Open())
       goto ex_Load_1;
	if(MessageBox(m_hWnd,"Do You want load cheats from USRCHEAT.DAT?","iDeaS Emulator",MB_YESNO|MB_ICONQUESTION) == IDNO)
		goto ex_Load_1;
   pFile->Seek(0x100,FILE_BEGIN);
   do{
       if(pFile->Read(s.c_str(),16) != 16)
           break;
       dwPos = ((LPDWORD)s.c_str())[0];
       if(!dwPos)
           break;
       if(dwPos != game_code)
           continue;
       dwRead = ((LPDWORD)s.c_str())[1];
       if(dwRead != crc_value)
           continue;
       break;
   }while(1);
   if(dwPos != game_code)
       goto ex_Load_1;

   Clear();
   CreateTreeView();
   TreeView_DeleteAllItems(m_hTV);

   dwPos = ((LPDWORD)s.c_str())[2];
   pFile->Seek(dwPos,FILE_BEGIN);
	s = "";
	do{
       if(pFile->Read(buf,1) != 1)
           break;
       if(buf[0] == 0)
           break;
       s += buf[0];
   }while(1);

   dwRead = pFile->GetCurrentPosition();
  	if(dwRead & 3)
  		pFile->Seek(4 - (dwRead & 3),FILE_CURRENT);
	pFile->Read(&cheatCount,sizeof(cheatCount));
   cheatCount &= ~0xF0000000;
   pFile->Seek(32,FILE_CURRENT);
	for(cc=0;cc<cheatCount;){
   	folderCount = 1;
       pFile->Read(&dwPos,sizeof(DWORD));
       if((dwPos & 0xF0000000) == 0x10000000){
           folderCount = dwPos & 0xFFFFFF;
           s = "";
           do{
               if(pFile->Read(buf,1) != 1)
                   break;
               if(buf[0] == 0)
                   break;
               s += buf[0];
           }while(1);
           s = "";
           do{
               if(pFile->Read(buf,1) != 1)
                   break;
               if(buf[0] == 0)
                   break;
               s += buf[0];
           }while(1);
           dwRead = pFile->GetCurrentPosition();
           if(dwRead & 3)
				dwRead = 4 - (dwRead & 3);
           else
           	dwRead = 0;
           pFile->Seek(dwRead + 4,FILE_CURRENT);
           cc++;
       }
       for(i=0;i<folderCount;i++){
           if(i > 0)
				pFile->Seek(4,FILE_CURRENT);
           sDescr = "";
           do{
               if(pFile->Read(buf,1) != 1)
                   break;
               if(buf[0] == 0)
                   break;
               sDescr += buf[0];
           }while(1);
           s = "";
           do{
               if(pFile->Read(buf,1) != 1)
                   break;
               if(buf[0] == 0)
                   break;
               s += buf[0];
           }while(1);
           dwRead = pFile->GetCurrentPosition();
           if(dwRead & 3)
               pFile->Seek(4 - (dwRead & 3),FILE_CURRENT);
           pFile->Read(&dataLen,sizeof(dataLen));
           for(j=0;j<dataLen;j +=2){
               pFile->Read(&dwAR,sizeof(DWORD));
               pFile->Read(&dwCode,sizeof(DWORD));
				wsprintf(buf,"%08X %08X",dwAR,dwCode);
				InsertNewCheat(buf,sDescr.c_str(),ActionReplay);
           }
           cc++;
       }
   }
ex_Load_1:
   if(pFile != NULL)
       delete pFile;
   return res;
}
//---------------------------------------------------------------------------
void LCheatsManager::OnEnumItemSaveCheat(HWND hwnd,HTREEITEM hItem,LPARAM lParam)
{
	TV_ITEM tvi={0};
	LString s,s1,*p;
   LVector<LPCHEATHEADER> *lvec;
	int i;
   LPCHEATHEADER p1;
	LStream *pFile;

   pFile = (LStream *)((LPVOID *)lParam)[0];
   p = (LString *)((LPVOID *)lParam)[1];
	if(hItem == (HTREEITEM)-2){
		s = p->Path();
       lstrcpy(p->c_str(),s.c_str());
		((LPVOID *)lParam)[2] = (LPVOID)TRUE;
      	return;
   }
	if(hItem == (HTREEITEM)-1)
   	return;
   s.Length(MAX_PATH+1);
   tvi.mask = TVIF_TEXT|TVIF_PARAM;
   tvi.cchTextMax = MAX_PATH;
   tvi.pszText = s.c_str();
   tvi.hItem = hItem;
   TreeView_GetItem(hwnd,&tvi);
   if(tvi.lParam != NULL){
   	if(((LPVOID *)lParam)[2]){
           s1 = "[";
       	if(!p->IsEmpty())
       		s1 += p->c_str();
           else
           	s1 += "\\";
       	s1 += "]\r\n";
   		pFile->Write(s1.c_str(),s1.Length());
           ((LPVOID *)lParam)[2] = (LPVOID)FALSE;
   	}
   	lvec = (LVector<LPCHEATHEADER> *)tvi.lParam;
		for(i=1;i<=lvec->count();i++){
       	p1 = lvec->items(i);
			switch(p1->type){
       		case ActionReplay:
           		pFile->Write((void *)"ARR",3);
           	break;
           	case CodeBreaker:
  	           		pFile->Write((void *)"CBR",3);
           	break;
           	case CodeBreakerEncrypt:
  	           		pFile->Write((void *)"CBE",3);
           	break;
       	}
           s1 = p1->descr;
       	pFile->Write(s1.c_str(),MAX_CHEATNAME);
       	pFile->Write(p1->codeString,20);
       	pFile->Write((void *)"\r\n",2);
       }
   }
   else{
   	if(p->IsEmpty() || p->c_str()[p->Length() - 1] != '\\')
     		p->Add("\\");
		if(hItem != TreeView_GetRoot(hwnd))
       	p->Add(s.c_str());
       ((LPVOID *)lParam)[2] = (LPVOID)TRUE;
   }
}
//---------------------------------------------------------------------------
void LCheatsManager::OnSaveCheats()
{
   char szFile[MAX_PATH];
   LString s,nameFile;
   LFile *pFile;
	LPVOID value[3];

   if(nameFile.BuildFileName((char *)ds.get_FileName(),"cht"))
       lstrcpy(szFile,nameFile.c_str());
   else
       *((LPDWORD)szFile) = 0;
   if(!ShowSaveDialog(NULL,szFile,"CheatsList(*.cht)\0*.cht\0\0\0\0\0",m_hWnd,NULL))
       return;
   s = szFile;
   s.AddEXT(".cht");
   pFile = new LFile(s.c_str());
   if(pFile == NULL)
       return;
   if(!pFile->Open(GENERIC_WRITE,CREATE_ALWAYS))
       goto ex_SaveCheats;
   s = "";
   value[0] = (LPVOID)pFile;
   value[1] = (LPVOID)&s;
   value[2] = (LPVOID)FALSE;
	OnEnumItem(m_hTV,TreeView_GetRoot(m_hTV),OnEnumItemSaveCheat,(LPARAM)value);
ex_SaveCheats:
   delete pFile;
}
//---------------------------------------------------------------------------
void LCheatsManager::OnCommand(WORD wID,WORD wNotifyCode,HWND hwndFrom)
{
	char s[20];

	switch(wID){
   	case IDC_EDIT1:
   	case IDC_EDIT2:
       	switch(wNotifyCode){
           	case EN_CHANGE:
               	lstrcpy(s,"Cancel");
                   editMode = null;
               	if(::SendDlgItemMessage(m_hWnd,IDC_EDIT2,WM_GETTEXTLENGTH,0,0) > 15){
                   	if(SendDlgItemMessage(m_hWnd,IDC_EDIT1,WM_GETTEXTLENGTH,0,0) > 0){
                       	lstrcpy(s,"Insert");
                           editMode = Insert;
                       }
                   }
                  	SetDlgItemText(m_hWnd,IDC_BUTTON1,s);
               break;
           }
       break;
		case IDC_BUTTON1:
       	switch(wNotifyCode){
           	case BN_CLICKED:
               	OnClickButton1();
               break;
           }
       break;
   	case IDOK:
       	switch(wNotifyCode){
           	case BN_CLICKED:
               	Destroy();
               break;
           }
       break;
   }
}
//---------------------------------------------------------------------------
void LCheatsManager::OnMenuSelect(WORD wID)
{
	switch(wID){
       case ID_CHEAT_NEW_CODEBREAKER:
           OnNewCheats(CodeBreaker);
       break;
       case ID_CHEAT_NEW_ACTIONREPLAY:
       	OnNewCheats(ActionReplay);
       break;
       case ID_CHEAT_DELETE:
       	DeleteCheat();
       break;
       case ID_CHEAT_DELETE_ALL:
       	DeleteAllCheats();
       break;
       case ID_CHEAT_LOAD:
       	Load();
       break;
       case ID_CHEAT_SAVE:
       	OnSaveCheats();
       break;
   }
}
#ifndef __WIN32__
//---------------------------------------------------------------------------
void LCheatsManager::OnDestroyParent()
{
	if(m_hTV != NULL){
		g_object_unref(m_hTV);
		gtk_widget_destroy(m_hTV);
		m_hTV = NULL;
	}
}
#endif
//---------------------------------------------------------------------------
BOOL LCheatsManager::Destroy()
{
   if(m_hTV != NULL){
       ShowWindow(m_hTV,SW_HIDE);
       ::SetParent(m_hTV,ds.Handle());
#ifndef __WIN32__
       gtk_container_remove((GtkContainer *)GetWindowLong(m_hWnd,GWL_FIXED),m_hTV);
#endif
   }
   return LDlg::Destroy();
}
//---------------------------------------------------------------------------
BOOL LCheatsManager::OnInitDialog(LPARAM lParam)
{
   RECT rc;
   
   SendDlgItemMessage(m_hWnd,IDC_CHECK1,BM_SETCHECK,BST_CHECKED,0);	
   CreateTreeView();
   GetClientRect(&rcClient);
   GetWindowRect(&rcWin);
   ::SetParent(m_hTV,m_hWnd);
   ::GetWindowRect(GetDlgItem(m_hWnd,IDC_EDIT2),&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,2);
   rc.top = 5;
   rc.left = 5;
   ::SetWindowPos(m_hTV,HWND_BOTTOM,rc.left,rc.top,rcClient.right - 10,rc.bottom-5,SWP_SHOWWINDOW);
   ::SetWindowPos((HWND)::SendMessage(m_hTV,TVM_GETTOOLTIPS,0,0),HWND_TOPMOST,0,0,0,0,SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOSIZE|SWP_NOREPOSITION);
   ::SetFocus(m_hTV);
   bOpen = FALSE;
   editMode = null;
   newType = (cheatType)null;
   return FALSE;
}
//---------------------------------------------------------------------------
void LCheatsManager::OnEnumItem(HWND hwnd,HTREEITEM hStart,LPENUMITEMFUNC lpfnc,LPARAM lParam,BOOL bEnumChild)
{
	HTREEITEM hItem,h;

	if(lpfnc == NULL)
   	return;
   hItem = hStart;
   while(hItem != NULL){
		h = TreeView_GetChild(hwnd,hItem);
      	lpfnc(hwnd,hItem,lParam);
		if(h != NULL && bEnumChild){
      		lpfnc(hwnd,(HTREEITEM)-1,lParam);
       		OnEnumItem(hwnd,h,lpfnc,lParam);
           lpfnc(hwnd,(HTREEITEM)-2,lParam);
       }
		hItem =  TreeView_GetNextSibling(hwnd,hItem);
   }
}
//---------------------------------------------------------------------------
void LCheatsManager::OnEnumItemEnableChanged(HWND hwnd,HTREEITEM hItem,LPARAM lParam)
{
	TV_ITEM *p,item;
	u8 state;
   LVector<LPCHEATHEADER> *lvec;
   LPCHEATHEADER p1;
	int i;

   if(hItem == (HTREEITEM)-1 || hItem == (HTREEITEM)-2)
   	return;
   p = (TV_ITEM *)lParam;
	state = (u8)((p->state & 8192) ? 0 : 1);
   if(p->hItem != hItem){
		item.mask = TVIF_STATE|TVIF_HANDLE;
       item.hItem = hItem;
       item.stateMask = TVIS_STATEIMAGEMASK;
       item.state = state == 0 ? 4096 : 8192;
       TreeView_SetItem(hwnd,&item);
   }
   item.mask = TVIF_PARAM;
   item.hItem = hItem;
   TreeView_GetItem(hwnd,&item);
	if(item.lParam == NULL)
   	return;
   lvec = (LVector<LPCHEATHEADER> *)item.lParam;
   for(i=1;i<=lvec->count();i++){
      	p1 = lvec->items(i);
       p1->Enable = state;
   }
}
//---------------------------------------------------------------------------
void LCheatsManager::OnNotify(int id,LPNMHDR lParam)
{
   LVector<LPCHEATHEADER> *lvec;
   LPCHEATHEADER p1;
   u8 state;
   LPNMTVGETINFOTIP tvgit;
	LString s;
	TV_HITTESTINFO tvhtt;
   HTREEITEM hItem;
	int i;
   LPNM_TREEVIEW tv;
	TV_ITEM item;
	TV_DISPINFO *tvdi;
	LPVOID value[3];
   POINT pt;

	switch(id){
       case IDC_TREE1:
       	switch(lParam->code){
/*           	case NM_DBLCLK:
   				::GetCursorPos(&tvhtt.pt);
                   ::ScreenToClient(lParam->hwndFrom,&tvhtt.pt);
                   hItem = TreeView_HitTest(lParam->hwndFrom,&tvhtt);
                   if(tvhtt.hItem != NULL){
                       ZeroMemory(&item,sizeof(TV_ITEM));
                       item.hItem = tvhtt.hItem;
                       item.mask = TVIF_PARAM;
                       TreeView_GetItem(lParam->hwndFrom,&item);
                       if(item.lParam != NULL){
                   		Open(TRUE);

                       }
                   }
               break;*/
               case NM_CLICK:
   					::GetCursorPos(&tvhtt.pt);
                   ::ScreenToClient(lParam->hwndFrom,&tvhtt.pt);
                   hItem = TreeView_HitTest(lParam->hwndFrom,&tvhtt);
                   if(tvhtt.hItem != NULL && (tvhtt.flags & TVHT_ONITEMSTATEICON)){
						ZeroMemory(&item,sizeof(item));
                       item.mask = TVIF_PARAM|TVIF_STATE|TVIF_HANDLE;
                       item.hItem = tvhtt.hItem;
                       TreeView_GetItem(lParam->hwndFrom,&item);
                       state = (u8)((item.state & 8192) ? 0 : 1);
                       if(item.state & 0x3000){
                       	if(item.lParam != NULL){
                           	lvec = (LVector<LPCHEATHEADER> *)item.lParam;
                        		for(i=1;i<=lvec->count();i++){
                           		p1 = lvec->items(i);
                                   p1->Enable = state;
                               }
                           }
                           else
                               OnEnumItem(lParam->hwndFrom,hItem,OnEnumItemEnableChanged,(LPARAM)&item);
                       }
                   }
               break;
               case TVN_BEGINDRAG:
					tv = (LPNM_TREEVIEW)lParam;
                   bDrag = FALSE;
                   ZeroMemory(&item,sizeof(TV_ITEM));
                   item.mask = TVIF_PARAM;
                   item.hItem = tv->itemNew.hItem;
                   TreeView_GetItem(lParam->hwndFrom,&item);
                   if(item.lParam != NULL){
                   	hilDrag = TreeView_CreateDragImage(lParam->hwndFrom,tv->itemNew.hItem);
						if(hilDrag != NULL){
							ImageList_BeginDrag(hilDrag,0,0,0);
							pt = tv->ptDrag;
							::ClientToScreen(m_hTV,&pt);
							ImageList_DragEnter(NULL,pt.x,pt.y);
                           hItemDrop = NULL;
                           hItemDrag = tv->itemNew.hItem;
                           bDrag = TRUE;
#ifdef __WIN32__
							SetCapture(m_hWnd);
#endif
                       }
                   }
               break;
               case TVN_BEGINLABELEDIT:
               	tvdi = (TV_DISPINFO *)lParam;
#ifdef __WIN32__
                   if(tvdi->item.hItem == TreeView_GetRoot(lParam->hwndFrom))
                   	SetWindowLong(m_hWnd,DWL_MSGRESULT,(LPARAM)TRUE);
                   else
						SetWindowLong(m_hWnd,DWL_MSGRESULT,(LPARAM)FALSE);
#endif
               break;
               case TVN_ENDLABELEDIT:
               	tvdi = (TV_DISPINFO *)lParam;
                   if(tvdi->item.mask & TVIF_TEXT){
                   	value[0] = tvdi->item.pszText;
       				value[1] = &hItem;
       				value[2] = &i;
                       hItem = TreeView_GetParent(lParam->hwndFrom,tvdi->item.hItem);
                       hItem = TreeView_GetChild(lParam->hwndFrom,hItem);
                       i = 0;
                   	OnEnumItem(lParam->hwndFrom,hItem,OnEnumItemSearchCheat,(LPARAM)value,FALSE);
						if(!i){
                       	TreeView_SetItem(lParam->hwndFrom,&tvdi->item);
                           SortTreeView();
                           bModified = TRUE;
                       }
						else
                       	MessageBox(m_hWnd,"The description already exists","iDeaS Emulator",MB_OK|MB_ICONERROR);
                   }
               break;
               case TVN_DELETEITEM:
               	tv = (LPNM_TREEVIEW)lParam;
                   if(!(tv->itemOld.mask & TVIF_PARAM)){
                   	ZeroMemory(&item,sizeof(item));
                       item.mask = TVIF_PARAM;
                       item.hItem = tv->itemOld.hItem;
                       TreeView_GetItem(tv->hdr.hwndFrom,&item);
                       if(item.lParam != NULL)
							delete (LVector<LPCHEATHEADER> *)tv->itemOld.lParam;
					}
                   else{
                   	if(tv->itemOld.lParam != NULL)
                       	delete (LVector<LPCHEATHEADER> *)tv->itemOld.lParam;
                   }
               break;
           	case TVN_GETINFOTIP:
               	tvgit = (LPNMTVGETINFOTIP)lParam;
                   if(tvgit->lParam != NULL){
                   	lvec = (LVector<LPCHEATHEADER> *)tvgit->lParam;
                       s = "";
                       for(i=1;i<=lvec->count();i++){
                           p1 = lvec->items(i);
                       	s += p1->codeString;
                           if(i < lvec->count())
                           	s += "\r\n";
                       }
                   	lstrcpy(tvgit->pszText,s.c_str());
                   }
               break;
           }
       break;
   }
}
//---------------------------------------------------------------------------
BOOL LCheatsManager::IsDialogMessage(LPMSG p)
{
	if(m_hWnd == NULL || !::IsWindow(m_hWnd))
   	return FALSE;
#ifdef __WIN32__
   if(::IsDialogMessage(m_hWnd,p))
   	return TRUE;
#endif
	return FALSE;
}
//---------------------------------------------------------------------------
void LCheatsManager::OnEnumItemSortCheat(HWND hwnd,HTREEITEM hItem,LPARAM lParam)
{
	TV_ITEM item;

	if(hItem == (HTREEITEM)-1){
   	TreeView_SortChildren(hwnd,*((HTREEITEM *)lParam),0);
       ZeroMemory(&item,sizeof(TV_ITEM));
       item.mask = TVIF_STATE;
       item.stateMask = -1;
       item.hItem = *((HTREEITEM *)lParam);
       TreeView_GetItem(hwnd,&item);
       item.state |= TVIS_BOLD;
       TreeView_SetItem(hwnd,&item);
   }
   else if(hItem != (HTREEITEM)-2)
		*((HTREEITEM *)lParam) = hItem;
}
//---------------------------------------------------------------------------
void LCheatsManager::SortTreeView()
{
	HTREEITEM hItem;

	OnEnumItem(m_hTV,TreeView_GetRoot(m_hTV),OnEnumItemSortCheat,(LPARAM)&hItem);
}
//---------------------------------------------------------------------------
void LCheatsManager::OnMouseMove(WPARAM wParam,int xPos,int yPos)
{
	POINT pt;
   TV_HITTESTINFO tvhti;
   HTREEITEM hItem,hChild;
	TV_ITEM item;
	int i;
	char s[500],s1[500];

	if(!bDrag)
   	return;
	GetCursorPos(&pt);
   ImageList_DragMove(pt.x,pt.y);
   ImageList_DragShowNolock(FALSE);
   tvhti.pt.x = xPos;
	tvhti.pt.y = yPos;
	MapWindowPoints(m_hWnd,m_hTV,&tvhti.pt,1);
   if((hItem = TreeView_HitTest(m_hTV,&tvhti)) == NULL)
   	goto ex_OnMouseMove;
   ZeroMemory(&item,sizeof(TV_ITEM));
   item.mask = TVIF_PARAM;
   item.hItem = hItem;
   TreeView_GetItem(m_hTV,&item);
   if(item.lParam == NULL && TreeView_GetParent(m_hTV,hItemDrag) != hItem){
		TreeView_Expand(m_hTV,hItem,TVE_EXPAND);
   	ZeroMemory(&item,sizeof(TV_ITEM));
   	item.mask = TVIF_TEXT;
   	item.hItem = hItemDrag;
   	item.pszText = s;
   	item.cchTextMax = 500;
   	TreeView_GetItem(m_hTV,&item);
       i = 0;
       hChild = TreeView_GetChild(m_hTV,hItem);
       while(hChild != NULL){
   		ZeroMemory(&item,sizeof(TV_ITEM));
   		item.mask = TVIF_TEXT;
   		item.hItem = hChild;
   		item.pszText = s1;
   		item.cchTextMax = 500;
   		TreeView_GetItem(m_hTV,&item);
			if(lstrcmp(s,s1) == 0){
           	i = 1;
           	break;
           }
       	hChild = TreeView_GetNextItem(m_hTV,hChild,TVGN_NEXT);
       }
   }
   else
   	i = 1;
   if(!i){
		TreeView_SelectDropTarget(m_hTV,hItem);
       hItemDrop = hItem;
	}
   else{
   	TreeView_SelectDropTarget(m_hTV,NULL);
   	hItemDrop = NULL;
   }
ex_OnMouseMove:
   ImageList_DragShowNolock(TRUE);
}
//---------------------------------------------------------------------------
void LCheatsManager::OnLButtonUp(WPARAM wParam,int xPos,int yPos)
{
   TV_ITEM item;
   HTREEITEM hItem;
   TV_HITTESTINFO tvhti;
	TV_INSERTSTRUCT tvis;
	char s[500];

	if(!bDrag)
   	return;
   ImageList_DragLeave(m_hWnd);
   ImageList_EndDrag();
   if(hilDrag != NULL)
   	ImageList_Destroy(hilDrag);
   hilDrag = NULL;
   TreeView_SelectDropTarget(m_hTV,NULL);
#ifdef __WIN32__
   ReleaseCapture();
#endif
   if(hItemDrag == hItemDrop || hItemDrop == NULL)
   	return;
   tvhti.pt.x = xPos;
	tvhti.pt.y = yPos;
	MapWindowPoints(m_hWnd,m_hTV,&tvhti.pt,1);
   if((hItem = TreeView_HitTest(m_hTV,&tvhti)) == NULL)
   	return;
   ZeroMemory(&item,sizeof(TV_ITEM));
   item.mask = TVIF_PARAM;
   item.hItem = hItemDrop;
   TreeView_GetItem(m_hTV,&item);
   if(item.lParam)
   	return;
   ZeroMemory(&item,sizeof(TV_ITEM));
   item.mask = TVIF_PARAM|TVIF_TEXT|TVIF_STATE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
   item.stateMask = -1;
   item.hItem = hItemDrag;
   item.pszText = s;
   item.cchTextMax = 500;
   TreeView_GetItem(m_hTV,&item);

	ZeroMemory(&tvis,sizeof(TV_INSERTSTRUCT));
   tvis.hParent = hItemDrop;
   tvis.hInsertAfter = TVI_LAST;
   tvis.item.pszText = s;
	tvis.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_PARAM|TVIF_SELECTEDIMAGE;
   tvis.item.iImage = item.iImage;
   tvis.item.iSelectedImage = item.iSelectedImage;
   tvis.item.lParam = item.lParam;
	hItem = TreeView_InsertItem(m_hTV,&tvis);

	ZeroMemory(&tvis.item,sizeof(TV_ITEM));
   tvis.item.mask = TVIF_STATE;
   tvis.item.hItem = hItem;
	tvis.item.state = item.state;
	tvis.item.stateMask = TVIS_STATEIMAGEMASK;
   TreeView_SetItem(m_hTV,&tvis.item);

	item.mask = TVIF_PARAM;
   item.lParam = NULL;
   TreeView_SetItem(m_hTV,&item);
   TreeView_DeleteItem(m_hTV,hItemDrag);
	SortTreeView();
   bModified = TRUE;
}
//---------------------------------------------------------------------------
LRESULT LCheatsManager::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	UINT i;

	switch(uMsg){
#ifdef __WIN32__
   		case WM_WINDOWPOSCHANGED:
			if(!(((LPWINDOWPOS)lParam)->flags & SWP_NOZORDER)){
           	::SetWindowPos((HWND)::SendMessage(m_hTV,TVM_GETTOOLTIPS,0,0),HWND_TOPMOST,0,0,0,0,SWP_NOSENDCHANGING|SWP_NOMOVE|SWP_NOSIZE|SWP_NOREPOSITION);
           }
       break;
#endif
       case WM_MOUSEMOVE:
			OnMouseMove(wParam,(int)(signed short)LOWORD(lParam),(int)(signed short)HIWORD(lParam));
       break;
       case WM_LBUTTONUP:
			OnLButtonUp(wParam,(int)(signed short)LOWORD(lParam),(int)(signed short)HIWORD(lParam));
       break;
  		case WM_INITDIALOG:
    		return OnInitDialog(lParam);
  		case WM_SIZE:
      		return OnSize(wParam,(int)(signed short)LOWORD(lParam),(int)(signed short)HIWORD(lParam));
       case WM_INITMENU:
           if(TreeView_GetSelection(m_hTV) != TreeView_GetRoot(GetDlgItem(m_hWnd,IDC_TREE1)))
               i = MF_ENABLED;
           else
               i = MF_GRAYED;
           EnableMenuItem((HMENU)wParam,ID_CHEAT_DELETE,i|MF_BYCOMMAND);
           if(TreeView_GetCount(m_hTV) > 1)
               i = MF_ENABLED;
           else
               i = MF_GRAYED;
           EnableMenuItem((HMENU)wParam,ID_CHEAT_DELETE_ALL,i|MF_BYCOMMAND);
           i = (bModified != 0 && Count() ? MF_ENABLED : MF_GRAYED);
           EnableMenuItem((HMENU)wParam,ID_CHEAT_SAVE,i|MF_BYCOMMAND);
           i = newType != null ? MF_GRAYED : MF_ENABLED;
           EnableMenuItem((HMENU)wParam,ID_CHEAT_NEW_CODEBREAKER,i|MF_BYCOMMAND);
           EnableMenuItem((HMENU)wParam,ID_CHEAT_NEW_ACTIONREPLAY,i|MF_BYCOMMAND);
       break;
   	case WM_COMMAND:
       	if(HIWORD(wParam) < 2 && lParam == NULL)
           	OnMenuSelect(LOWORD(wParam));
           else
           	OnCommand(LOWORD(wParam),HIWORD(wParam),(HWND)lParam);
       break;
       case WM_NOTIFY:
       	OnNotify((int)wParam,(LPNMHDR)lParam);
		break;
       case WM_CLOSE:
       	Destroy();
       break;
   }
	return FALSE;
}
//---------------------------------------------------------------------------
LString LCheatsManager::SearchCheatCode(const char *lpCode,int *len)
{
   LString s,s1,res,s2;
   int i;

   *len = 0;
   res = "";
   s = lpCode;
   if(s.IsEmpty())
       goto ex_SearchCheatCode;
   s2 = "0123456789ABCDEF";
   do{
       s1 = s.NextToken(13);
       if(s1.IsEmpty()){
           s1 = s.NextToken(0);
           if(s1.IsEmpty())
               break;
       }
       else
           (*len)++;
       *len += s1.Length();
       for(i=1;i<=s1.Length();i++){
           if(s1[i] == 32 || s2.Pos(s1[i]) < 1)
               continue;
           res += s1[i];
       }
   }while(res.Length() < 9);
ex_SearchCheatCode:
   return res;
}
//---------------------------------------------------------------------------
void LCheatsManager::OnEnumItemSearchCheat(HWND hwnd,HTREEITEM hItem,LPARAM lParam)
{
	char *s,s1[300];
	HTREEITEM *p;
	TV_ITEM tvi;
	int *i;

   if(hItem == (HTREEITEM)-1 || hItem == (HTREEITEM)-2)
   	return;
   s = (char *)((LPVOID *)lParam)[0];
   p = (HTREEITEM *)((LPVOID *)lParam)[1];
   i = (int *)((LPVOID *)lParam)[2];
   ZeroMemory(&tvi,sizeof(TV_ITEM));
   tvi.mask = TVIF_TEXT;
   tvi.cchTextMax = 299;
	tvi.pszText = s1;
   tvi.hItem = hItem;
   TreeView_GetItem(hwnd,&tvi);
   if(lstrcmp(s,s1) == 0){
   	if(p != NULL)
   		*p = hItem;
       if(i != NULL)
       	(*i)++;
   }
}
//---------------------------------------------------------------------------
HTREEITEM LCheatsManager::InsertNewCheat(const char *lpCode,const char *lpDescr,cheatType type,HTREEITEM hItemNew)
{
   int i,pos,len,iCount;
   LString s,s1,s3;
   LPCHEATHEADER n,o;
	HTREEITEM hItem,hRoot;
   TV_INSERTSTRUCT tvis;
   TV_ITEM tvi;
	LVector<LPCHEATHEADER> *pParam;
   LPVOID value[3];
   DWORD dw;

	if(lpDescr == NULL || lstrlen(lpDescr) == 0)
      	return NULL;
   s = (char *)lpDescr;
   pos = 0;
	hRoot = TreeView_GetRoot(m_hTV);
   if(s[1] == '\\')
       pos = 1;
   else{
   	hItem = NULL;
   	if(hItemNew == NULL){
   		if((hItem = TreeView_GetSelection(m_hTV)) == NULL)
           	hItem = hRoot;
       }
       else
       	hItem = hItemNew;
       if(hItem != hRoot){
   		ZeroMemory(&tvi,sizeof(TV_ITEM));
   		tvi.mask = TVIF_PARAM;
   		tvi.hItem = hItem;
   		TreeView_GetItem(m_hTV,&tvi);
			if(tvi.lParam != NULL){
      			::MessageBox(m_hWnd,"This item have codes.","iDeaS Emulator",MB_OK|MB_ICONERROR);
       		return FALSE;
           }
       }
       hRoot = hItem;
   }
   hItemNew = hRoot;
   hItem = NULL;
   iCount = 0;
   s1 = s;
   dw = 0;
	pParam = NULL;
   while(pos < s.Length()){
   	s1 = s.SubString(pos+1,s.Length()-pos);
       s3 = s1.NextToken('\\');
       if(s3.IsEmpty())
       	s3 = s1.NextToken(0);
       if(s3.IsEmpty())
       	break;
		pos += s3.Length() + 1;
       if(TreeView_GetCount(m_hTV) < 1){
   		ZeroMemory(&tvis,sizeof(TV_INSERTSTRUCT));
   		tvis.hParent = TVI_ROOT;
   		tvis.hInsertAfter = TVI_LAST;
   		tvis.item.pszText = "iDeaS Emulator";
   		tvis.item.mask = TVIF_TEXT;
   		hItemNew = TreeView_InsertItem(m_hTV,&tvis);
   		ZeroMemory(&tvis.item,sizeof(TV_ITEM));
   		tvis.item.mask = TVIF_STATE;
   		tvis.item.hItem = hItemNew;
			tvis.item.state = 8192;
			tvis.item.stateMask = TVIS_STATEIMAGEMASK;
   		TreeView_SetItem(m_hTV,&tvis.item);
       }
      	hItem = TreeView_GetChild(m_hTV,hItemNew);
       value[0] = (void *)s3.c_str();
       value[1] = &hItemNew;
       value[2] = &i;
       i = 0;
       OnEnumItem(m_hTV,hItem,OnEnumItemSearchCheat,(LPARAM)value,FALSE);
       if(i == 0){
			ZeroMemory(&tvis,sizeof(TV_INSERTSTRUCT));
           tvis.hParent = hItemNew;
           tvis.hInsertAfter = TVI_LAST;
           tvis.item.pszText = s3.c_str();
           tvis.item.mask = TVIF_TEXT;
           hItemNew = TreeView_InsertItem(m_hTV,&tvis);
           tvis.item.mask = TVIF_STATE;
           tvis.item.hItem = hItemNew;
			tvis.item.state = 8192;
           tvis.item.stateMask = TVIS_STATEIMAGEMASK;
           TreeView_SetItem(m_hTV,&tvis.item);
           iCount++;
       }
   }
   if(!iCount){
       s = "The description \r\n\r\n(";
       s += s3;
       s += ")\r\n\r\nalready exists!!! Do You want add?";
		if(::MessageBox(m_hWnd,s.c_str(),
       	"iDeaS Emulator - Cheats System",MB_YESNO|MB_ICONQUESTION) == IDNO)
      		return NULL;
  		ZeroMemory(&tvis.item,sizeof(TV_ITEM));
  		tvis.item.mask = TVIF_PARAM;
  		tvis.item.hItem = hItemNew;
		TreeView_GetItem(m_hTV,&tvis.item);
       pParam = (LVector<LPCHEATHEADER> *)tvis.item.lParam;
	}
   s = s1.FileName();
   if(lpCode == NULL || lstrlen(lpCode) == 0)
   	return hItemNew;
	s1 = (char *)lpCode;
   pos = 0;
   n = o = NULL;
   iCount = 0;
   do{
       s3 = SearchCheatCode(s1.c_str() + pos,&len);
       if(s3.IsEmpty())
           break;
		switch(type){
       	case ActionReplay:
				n = AddActionReplay(s3.c_str(),s.c_str(),dw == 0 ? TRUE : FALSE);
               if(dw) dw--;
           break;
           case CodeBreaker:
           	n = AddCodeBreaker(s3.c_str(),s.c_str(),(o != NULL && o->pNext == (LPCHEATHEADER)-1) ? o : NULL);
           break;
           default:
       		n = 0;
           break;
       }
       pos += len;
       if(!n){
       	if(o != NULL)
       		o->pNext = NULL;
       	o = NULL;
           continue;
       }
       switch(n->type){
       	case ActionReplay:
               i = 1;
           break;
       	case CodeBreaker:
           	i = 2;
           break;
       	case CodeBreakerEncrypt:
           	i = 3;
           break;
       }
       if(type == ActionReplay){
           if(n->pNext == (LPCHEATHEADER)-3){
               n->skip = 0;
               n->pNext = (LPCHEATHEADER)-1;
               dw = (n->value >> 3);
               if((dw << 3) != n->value)
                   dw++;
           	if(o != NULL)
           		o->pNext = n;
           }
       	else if(n->pNext == (LPCHEATHEADER)-2){
				n->skip = 1;
           	n->pNext = NULL;
           	if(o != NULL)
           		o->pNext = n;
       	}
       	else if(o != NULL && o->pNext == (LPCHEATHEADER)-1){
       		o->pNext = n;
           	n->skip = 1;
           	n->pNext = (LPCHEATHEADER)-1;
       	}
       }
       else if(type == CodeBreaker){
			if(o != NULL && o->pNext == (LPCHEATHEADER)-1){
       		o->pNext = n;
           	n->skip = 1;
           }
       }
   	if(!Add(n)){
           if(o != NULL && o->pNext == n)
           	o->pNext = NULL;
           delete n;
       	continue;
   	}
       iCount++;
       if(tvis.item.lParam == NULL){
           pParam = new LVector<LPCHEATHEADER>;
       	tvis.item.lParam = (LPARAM)pParam;
           tvis.item.mask = TVIF_PARAM|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
           tvis.item.iImage = tvis.item.iSelectedImage = i;
           tvis.item.hItem = hItemNew;
           TreeView_SetItem(m_hTV,&tvis.item);
       }
		if(pParam != NULL)
      		pParam->push(n);
     	o = n;
   }while(pos < s1.Length());
   if(iCount){
      bModified = TRUE;
      SortTreeView();
   }
   return hItemNew;
}
//---------------------------------------------------------------------------
void LCheatsManager::OnClickButton1()
{
   LString s,s1;
	int len;

	if(editMode != Insert)
		goto Ex_OnClickButton1;
   len = SendDlgItemMessage(m_hWnd,IDC_EDIT1,WM_GETTEXTLENGTH,0,0);
   s.Capacity(++len + 10);
   SendDlgItemMessage(m_hWnd,IDC_EDIT1,WM_GETTEXT,len,(LPARAM)s.c_str());
   len = SendDlgItemMessage(m_hWnd,IDC_EDIT2,WM_GETTEXTLENGTH,0,0);
   s1.Capacity(++len + 10);
   SendDlgItemMessage(m_hWnd,IDC_EDIT2,WM_GETTEXT,len,(LPARAM)s1.c_str());
   if(!InsertNewCheat(s1.c_str(),s.c_str(),newType))
		goto Ex_OnClickButton1;
   Sort(1,Count(),sortCheatList);
   ResetCheatSystem();
   Apply();
Ex_OnClickButton1:
	EnableWindow(GetDlgItem(m_hWnd,IDC_CHECK1),TRUE);
   SendDlgItemMessage(m_hWnd,IDC_CHECK1,BM_SETCHECK,BST_CHECKED,0);
   SendDlgItemMessage(m_hWnd,IDC_EDIT1,WM_SETTEXT,0,(LPARAM)"");
   SendDlgItemMessage(m_hWnd,IDC_EDIT2,WM_SETTEXT,0,(LPARAM)"");
   editMode = null;
	newType = (cheatType)null;
   Open(FALSE);
}
//---------------------------------------------------------------------------
void LCheatsManager::OnNewCheats(cheatType type)
{
   if(type == ActionReplay){
		EnableWindow(GetDlgItem(m_hWnd,IDC_CHECK1),FALSE);
       SendDlgItemMessage(m_hWnd,IDC_CHECK1,BM_SETCHECK,BST_CHECKED,0);
   }
   else{
		EnableWindow(GetDlgItem(m_hWnd,IDC_CHECK1),TRUE);
       SendDlgItemMessage(m_hWnd,IDC_CHECK1,BM_SETCHECK,BST_UNCHECKED,0);
   }
	Open(TRUE);
   SetFocus(GetDlgItem(m_hWnd,IDC_EDIT1));
   newType = type;
}
//---------------------------------------------------------------------------
BOOL LCheatsManager::OnGetMinxMax(LPMINMAXINFO p)
{
	return FALSE;
}
//---------------------------------------------------------------------------
BOOL LCheatsManager::Open(BOOL bFlag)
{
	int nWidth;
	RECT rc;

	if(bFlag == bOpen)
   		return TRUE;
   	if(bFlag)
   		::GetWindowRect(GetDlgItem(m_hWnd,IDC_EDIT1),&rc);
   	else
   		::GetWindowRect(m_hTV,&rc);
   	MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,2);
	nWidth = rc.right + 10;
	GetWindowRect(&rc);
	nWidth += GetSystemMetrics(SM_CXEDGE) * 2;
   rc.bottom = rc.bottom - rc.top;
   ::SetWindowPos(m_hWnd,NULL,0,0,nWidth,rc.bottom,SWP_NOMOVE|SWP_NOREPOSITION);
   bOpen = bFlag;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LCheatsManager::OnSize(WPARAM wParam,int nWidth,int nHeight)
{
	RECT rc;
   HWND hwnd;
   HDWP h;

	::GetWindowRect(m_hTV,&rc);
	::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,2);

   SetRect(&rcClient,0,0,nWidth,nHeight);
	h = BeginDeferWindowPos(5);
   if(h == NULL)
   	return TRUE;
   hwnd = GetDlgItem(m_hWnd,IDOK);
   ::GetWindowRect(hwnd,&rc);
	MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,2);
   rc.right -= rc.left;
   rc.left = nWidth - 10 - rc.right + GetSystemMetrics(SM_CXEDGE);
   h = DeferWindowPos(h,hwnd,NULL,rc.left,rc.top,rc.right,rc.bottom - rc.top,SWP_NOREPOSITION);
	if(h == NULL)
   	return TRUE;   
   EndDeferWindowPos(h);
	return TRUE;
}
//---------------------------------------------------------------------------
void LCheatsManager::Show(HWND p)
{
	CreateModal(langManager.get_CurrentLib(),MAKEINTRESOURCE(IDD_DIALOG10),p);
}
//---------------------------------------------------------------------------
void LCheatsManager::write_hword(u32 address,u32 value)
{
	if(!bEnable)
   	return;
   write_hword_cheats(address,value);
}
//---------------------------------------------------------------------------
BOOL LCheatsManager::EnumCheatsReset(LPVOID ele,DWORD index,LPARAM lParam)
{
	if(ele != NULL && lParam != NULL){
   	if(((LPCHEATHEADER)ele)->Enable && ((LPCHEATHEADER)ele)->skip == 0)
   		*((u32 *)lParam) = *((u32 *)lParam) + 1;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
int LCheatsManager::sortCheatList(LPVOID ele,LPVOID ele1)
{
   u32 adr0,adr1;

   if(((LPCHEATHEADER)ele)->pNext == (LPCHEATHEADER)-1 || ((LPCHEATHEADER)ele)->pNext == (LPCHEATHEADER)-1)
       ((LPCHEATHEADER)ele)->pNext = NULL;
   if(((LPCHEATHEADER)ele1)->pNext == (LPCHEATHEADER)-1 || ((LPCHEATHEADER)ele1)->pNext == (LPCHEATHEADER)-1)
       ((LPCHEATHEADER)ele1)->pNext = NULL;
   adr0 = ((LPCHEATHEADER)ele)->skip;
   adr1 = ((LPCHEATHEADER)ele1)->skip;
   if(adr0 && !adr1)
       return 1;
   else if(!adr0 && adr1)
       return -1;
   adr0 = ((LPCHEATHEADER)ele)->sort;
   adr1 = ((LPCHEATHEADER)ele1)->sort;
   if(adr0 && !adr1)
       return -1;
   else if(!adr0 && adr1)
       return 1;
   else if(!adr0 && !adr1)
       return 0;
   adr0 = ((LPCHEATHEADER)ele)->adr;
   adr1 = ((LPCHEATHEADER)ele1)->adr;
   if(adr0 < adr1)
       return -1;
   else if(adr0 > adr1)
       return 1;
   return 0;
}
//---------------------------------------------------------------------------
BOOL LCheatsManager::DecryptCodeBreaker(u32 *adr,u32 *val)
{
	u32 address,value;
	int counter;
   s64 temp;

	if(adr == NULL || val == NULL)
   	return FALSE;
   address = *adr;
   value = *val;
   counter = 0x4f;
	for(counter=0x4f;counter != -1;counter--){
   	temp = (u32)((Key0 - value) ^ address);
    	value = (u32)(value - ror(temp,0x1b) - Key1);
    	address = ror((u32)(temp ^ (value + key(counter))),0x13);
    	if(counter > 0x13){
      		temp = (u32)(key(counter) + address);
      		if(counter > 0x27 && counter <= 0x3b)
        		value = (u32)(value ^ (u32)(temp + (((Key2 ^ Key1) & Key0) ^ (Key1 & Key2))));
      		else
        		value = (u32)(value ^ (u32)(temp + (Key2 ^ Key0 ^ Key1)));
      		temp = EncryptedArea[((counter + 0xd) & 0x3f) + 0x40] ^
              	   EncryptedArea[((counter + 0x8) & 0x3f) + 0x80] ^
              	   EncryptedArea[((counter + 0x2) & 0x3f) + 0xC0] ^
              	   EncryptedArea[(counter & 0x3f)];
           address = (u32)(address - ror(temp,0x19));
		}
	}
  	if(address == 0xBEEFC0DE){
  		Key0 = (u32)(ror(value,0x1d) + Key0);
  		Key1 = (u32)(Key1 - ror(value,5));
  		Key2 = (u32)(Key0 ^ Key3 ^ Key2);
  		Key3 = (u32)(Key3 ^ (Key2 - Key1));
  		scramble();
	}
   *adr = address;
   *val = value;
   return TRUE;
}
//---------------------------------------------------------------------------
s64 LCheatsManager::ror(s64 a,s64 b)
{
	return ((a >> (u8)b) | (a << (0x20 - b)));
//	return (Int64ShrlMod32(a,b)|Int64ShllMod32(a,32-b));
}
//---------------------------------------------------------------------------
s64 LCheatsManager::key(s64 a)
{
	if(a > 0x3b)
    	return 0xb1bf0855;
  	else if (a > 0x27)
   	return 0x54A7818;
  	else if (a > 0x13)
   	return 0x59e5dc8a;
  	return 0x77628ecf;
}
//---------------------------------------------------------------------------
BOOL LCheatsManager::scramble()
{
	int counter;

   for(counter=0;counter < 0x100;counter++){
    	EncryptedArea[counter] = (((EncryptedArea[counter] ^ EncryptedArea[CBSData[counter]])
       	+ EncryptedArea[CBSData[counter] + 0x40]));
    	EncryptedArea[counter+0x100] = (((EncryptedArea[counter+0x100] ^
                                     EncryptedArea[CBSData[counter]]) - EncryptedArea[CBSData[counter] + 0x80]));
	}
   for(counter = 0;counter <0x64;counter++){
    	Key0 = (((EncryptedArea[counter] + EncryptedArea[counter+0x64]) ^ Key0));
    	Key1 = (((EncryptedArea[counter] + EncryptedArea[counter+0xC8]) ^ Key1));
    	Key2 = (((EncryptedArea[counter] + EncryptedArea[counter+0x12C]) ^ Key2));
    	Key3 = (((EncryptedArea[counter] + EncryptedArea[counter+0x190]) ^ Key3));
	}
  	Key3 = EncryptedArea[0x1F4] ^ EncryptedArea[0x1FC] ^ Key3;
  	Key0 = Key0 - EncryptedArea[0x1F4];
  	Key1 = EncryptedArea[0x1F8] ^ Key1;
  	Key2 = EncryptedArea[0x1FC] + Key2;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LCheatsManager::OnLoadRom()
{
	LString nameFile;
   LRomReader *pReader;
	BOOL res;

   if(ds.get_EmuMode() != EMUM_GBA){
       pReader = ds.get_RomReader();
       if(pReader != NULL && pReader->IsOpen()){
           pReader->Seek(0,SEEK_SET);
           pReader->Read(EncryptedArea,0x200);
           crc_value = CrcUpdate(CRC_INIT_VAL, EncryptedArea, 512);
           pReader->Seek(0x4000,SEEK_SET);
           pReader->Read(EncryptedArea,sizeof(EncryptedArea));
           pReader->Seek(12,SEEK_SET);
           pReader->Read(&game_code,sizeof(u32));
           encrypt_arm9(game_code,(u8 *)EncryptedArea);
       }
       Key0 = 0x0C2EAB3E;
       Key1 = 0xE2AE295D;
       Key2 = 0xE1ACC3FF;
       Key3 = 0x70D3AF46;
       scramble();
   }
   res = FALSE;
	if(bAutoLoad){
  		nameFile.Length(MAX_PATH+1);
  		if(ds.BuildFileName(PT_CHEAT,nameFile.c_str(),MAX_PATH))
			res = Load(nameFile.c_str());
   }
   else{
   	res = TRUE;
       ResetCheatSystem();
       Apply();
   }
   return res;
}
