#include "ideastypes.h"
#include "lds.h"
#include "dma.h"
#include "util.h"

CARD card;
static EEPROMCARD eeprom;
static u8 bModified;
//---------------------------------------------------------------------------
void EEPROM_reset()
{
   eeprom.mode = 0;
   eeprom.adr = 0;
   eeprom.status = 0;
   eeprom.adrMax = 0;
   eeprom.index = 0;
   memset(eeprom.buffer,0,sizeof(eeprom.buffer));
   bModified = 0;
   if(eeprom.id == ID_EEPROM_AUTO)
       eeprom.adrSize = (u8)-1;
}
//---------------------------------------------------------------------------
void CARD_reset()
{
   ZeroMemory(&card,sizeof(CARD));
}
//---------------------------------------------------------------------------
u8 CARD_is_enable()
{
   return (u8)(card.control >> 31);
}
//---------------------------------------------------------------------------
static u32 ioCONTROLO1(u32 adr,u8)
{
   if(--card.ofs < 1){
       card.control &= ~0x80000000;
       card.control |= 0x800000;
   }
   return card.control;
}
//---------------------------------------------------------------------------
static void ioDATAI(u32 adr,u32 data,u8 am)
{
	u32 res;
   DLDIPlugIn *p;

   if(ds.is_FatEnable(1) && (p = (DLDIPlugIn *)pPlugInContainer->get_PlugInList(PIL_DLDI)->get_ActivePlugIn()) != NULL){
       res = (u32)*((u32 *)card.command);
   	res = p->Run(res,&data,AMM_WRITE);
       switch(res){
			default:
               ds.FatWrite(adr,data,AMM_WORD);
               if(--card.dataRead == 0){
                   card.control &= ~0x80800000;
                   if(card.bIrq)
                       ds.set_IRQ(0x80000,FALSE,(u8)(crIO_mem == io_mem7 ? 2 : 1));
               }
           break;
       }
   }
}
//---------------------------------------------------------------------------
static u32 ioDATAO(u32 adr,u8)
{
	u32 res,data;
	DLDIPlugIn *p;

  	if(ds.is_FatEnable(1) && (p = (DLDIPlugIn *)pPlugInContainer->get_PlugInList(PIL_DLDI)->get_ActivePlugIn()) != NULL){
       res = (u32)*((u32 *)card.command);
       data = card.dataRead;
   	res = p->Run(res,&data,AMM_READ);
       switch((u16)res){
       	case PIR_DLDI_NULL:
          		if((res & PIR_DLDI_TRIGGERIRQ) && card.bIrq)
              		ds.set_IRQ(0x80000,FALSE,(u8)(crIO_mem == io_mem7 ? 2 : 1));
               if(!(res & PIR_DLDI_DATAISVALID))
                   data = 0;
               if(res & PIR_DLDI_ADJUSTIOCONTROL){
                   card.control &= ~0x80800000;
                   *((u32 *)&crIO_mem[0x1A4]) = card.control;
               }
               return data;
           case PIR_DLDI_UNSUPPORTED:
           break;
           case PIR_DLDI_READ:
           	if((res & (PIR_DLDI_ONLYDECODE|PIR_DLDI_FAT)) == (PIR_DLDI_ONLYDECODE|PIR_DLDI_FAT))
                   data = ds.FatRead(0,AMM_WORD);
               card.dataOut = data;
               if(--card.dataRead == 0){
                   if(res & PIR_DLDI_ADJUSTIOCONTROL){
                       card.control &= ~0x80800000;
                       *((u32 *)&crIO_mem[0x1A4]) = card.control;
                   }
                   if((res & PIR_DLDI_TRIGGERIRQ) && card.bIrq)
                       ds.set_IRQ(0x80000,FALSE,(u8)(crIO_mem == io_mem7 ? 2 : 1));
                   if(res & PIR_DLDI_TRIGGERIRQ_MC)
                       ds.set_IRQ(0x100000,FALSE,(u8)(crIO_mem == io_mem7 ? 2 : 1));
               }
               return card.dataOut;
           case PIR_DLDI_CONTROL:
               if(res & PIR_DLDI_ADJUSTIOCONTROL){
                   card.control &= ~0x80800000;
                   *((u32 *)&crIO_mem[0x1A4]) = card.control;
               }
               if((res & PIR_DLDI_TRIGGERIRQ) && card.bIrq)
               	ds.set_IRQ(0x80000,FALSE,(u8)(crIO_mem == io_mem7 ? 2 : 1));
               if(res & PIR_DLDI_TRIGGERIRQ_MC)
					ds.set_IRQ(0x100000,FALSE,(u8)(crIO_mem == io_mem7 ? 2 : 1));
           return data;
       }
   }
   switch(card.command[0]){
		case 0x0:
           res = 0;
  	    case 0xB7:
      	    card.dataOut = ds.ReadCard();
           card.ofs--;
           if(--card.dataRead == 0){
               card.control &= ~0x80800000;
               *((u32 *)&crIO_mem[0x1A4]) = card.control;
//               *((u16 *)&crIO_mem[0x1A0]) &= ~0x80;
               if(card.bIrq)
             	    ds.set_IRQ(0x80000,FALSE,(u8)(crIO_mem == io_mem7 ? 2 : 1));
           }
           if((card.mode && ((data + 0x200) < card.command_count ||
                   data > (card.command_count + 0x200) )))
               card.dataOut = 0x0;
           return card.dataOut;
       case 0x90:
           if(--card.dataRead == 0){
               card.control &= ~0x80800000;
               if(card.bIrq)
             	    ds.set_IRQ(0x80000,FALSE,(u8)(crIO_mem == io_mem7 ? 2 : 1));
           }
           return card.dataOut;
       case 0xB8:
           card.control &= ~0x80800000;
           *((u32 *)&crIO_mem[0x1A4]) = card.control;
//           *((u16 *)&crIO_mem[0x1A0]) &= ~0x80;
           if(card.bIrq)
               ds.set_IRQ(0x80000,FALSE,(u8)(crIO_mem == io_mem7 ? 2 : 1));
           return 0;
       case 0xD6:
           card.control &= ~0x80800000;
           *((u32 *)&crIO_mem[0x1A4]) = card.control;
//           *((u16 *)&crIO_mem[0x1A0]) &= ~0x8000;
           if(card.bIrq)
               ds.set_IRQ(0x80000,FALSE,(u8)(crIO_mem == io_mem7 ? 2 : 1));
           return 0x20|(card.mode << 2);
       default:
       	card.control &= ~0x80800000;
           if(card.bIrq)
           	ds.set_IRQ(0x80000,FALSE,(u8)(crIO_mem == io_mem7 ? 2 : 1));
           return 0;
   }
   return 0;
}
//---------------------------------------------------------------------------
static void ioCONTROLI1(u32 adr,u32 data,u8 am)
{
	u32 res;
   DLDIPlugIn *p;

   if(!(data & 0x80000000)){
       return;
   }
   card.control = data & ~0x800000;
   data = MAKELONG(MAKEWORD(card.command[4],card.command[3]),MAKEWORD(card.command[2],card.command[1]));
   WriteMsgConsolle(NULL,"%cCard Memory 0x%08X 0x%08X %02X",MSGT_CARD,card.control,data,card.command[0]);
   card.control |= 0x800000;
   if(ds.is_FatEnable(1) && (p = (DLDIPlugIn *)pPlugInContainer->get_PlugInList(PIL_DLDI)->get_ActivePlugIn()) != NULL){
       res = (u32)*((u32 *)card.command);
//		data = MAKELONG(MAKEWORD(card.command[4],card.command[3]),MAKEWORD(card.command[2],card.command[1]));
		data = (u32)*((u32 *)&card.command[4]);
   	res = p->Run(res,&data,AMM_CNT);
       switch((u16)res){
       	case PIR_DLDI_NULL:
               if(res & PIR_DLDI_ADJUSTIOCONTROL)
           	    card.control &= ~0x80800000;
           break;
           case PIR_DLDI_CONTROL:
               if((res & (PIR_DLDI_ONLYDECODE|PIR_DLDI_SEEK|PIR_DLDI_FAT)) == (PIR_DLDI_ONLYDECODE|PIR_DLDI_SEEK|PIR_DLDI_FAT)){
           		if(ds.FatSeek(data,0))
                       data = 0x80 << 2;
                   else
                       data = 0;
               }
               if(res & PIR_DLDI_SETLENGTH){
                   if(res & PIR_DLDI_DATAISVALID)
                       card.dataRead = data;
                   else{
                       switch((card.control >> 24) & 7){
                           case 7:
                               card.dataRead = 4;
                           break;
                           case 0:
                               card.dataRead = 0;
                           break;
                           default:
                               card.dataRead = 256 << ((card.control >> 24) & 7);
                           break;
                       }
                   }
                   card.ofs = (s16)card.dataRead;
                   card.dataRead >>= 2;
               }
               if(res & PIR_DLDI_ADJUSTIOCONTROL){
                   if(!(res & PIR_DLDI_ADJUSTIOCONTROL_CHECK))
                       card.control &= ~0x80000000;
                   else{
                       if(!data)
						    card.control &= ~0x80800000;
                       else
           			    card.control |= 0x80800000;
                   }
               }
               if((res & PIR_DLDI_TRIGGERIRQ) && card.bIrq)
               	ds.set_IRQ(0x80000,FALSE,(u8)(crIO_mem == io_mem7 ? 2 : 1));
               if(res & PIR_DLDI_TRIGGERIRQ_MC)
					ds.set_IRQ(0x100000,FALSE,(u8)(crIO_mem == io_mem7 ? 2 : 1));
           break;
           default:
   			data = MAKELONG(MAKEWORD(card.command[4],card.command[3]),MAKEWORD(card.command[2],card.command[1]));
               goto ioCONTROLI1_1;
       }
       goto ioCONTROLI1_2;
   }
ioCONTROLI1_1:
   card.ofs = 512;
   switch(card.command[0]){
		case 0x0:
           if(!(card.dataRead = ds.SeekCard(0))){
               card.control &= ~0x80800000;
               card.ofs = 32;
           }
       break;
		case 0x3C:
       	card.mode = 1;
           card.command_count = 0;
       break;
       case 0x94://read raw mode
       case 0xB7:
           if(ds.SeekCard(data)){
//               if(data == 0xcd5fc00)
//                   EnterDebugMode(FALSE);
               res = (card.control >> 24) & 7;
               if(res == 7)
                   res = 4;
               else
                   res = 0x100 << res;
               res >>= 2;
               card.dataRead = res;
               card.control |= 0x800000;
//               card.cycles = 36;
               StartDMA(5);
           }
           else
               card.control &= ~0x80800000;
       break;
       case 0x90:
       	card.dataRead = 1;
           ds.SeekCard(0x14);
           card.dataOut = ds.ReadCard();
			card.dataOut = (((0x80 << card.dataOut) >> 10) << 16) | 0xC2000000;
       break;
       case 0xB8:
           card.control |= 0x800000;
       break;
       case 0xD6:
           card.control |= 0x800000;
       break;
       case 0xB5:
           card.mode = 1;
           card.control |= 0x800000;
//           EnterDebugMode(FALSE);
       break;
       case 0xB2://lock
           card.mode = 2;
           card.control |= 0x800000;
           card.command_count = data;
//           EnterDebugMode(FALSE);
       break;
       case 0x8B://unlock
           card.mode = 0;
           card.control |= 0x800000;
//           EnterDebugMode(FALSE);
       break;
#ifdef _DEBPRO
       default://94 0x80 06-0A
//           EnterDebugMode(FALSE);
//           res = 0;
       break;
#endif
   }
ioCONTROLI1_2:
   card.ofs *= (s16)16;
}
//---------------------------------------------------------------------------
static void ioCONTROLI2(u32 adr,u32 data,u8 am)
{
   adr -= 0x1A8;
   do{
       card.command[adr] = (u8)data;
       adr++;
       data >>= 8;
       am >>= 1;
   }while(am);
}
//---------------------------------------------------------------------------
static void ioCONTROLI0(u32 adr,u32 data,u8 am) //37
{
   data = *((u16 *)&io_mem[0x1A0]);
	if(!(data & 0x2000)){                                       //a1d86000  a1c16017
  		card.bEnable = (u8)(data >> 15);                         //a7c16000
  		card.bIrq	 = (u8)((data >> 14) & 1);
  		card.control = 0;
   }
   *((u16 *)&io_mem[0x1A0]) &= ~0x80;
}
//---------------------------------------------------------------------------
static void ioCONTROLI07(u32 adr,u32 data,u8 am) //37
{
	if(*((u16 *)&io_mem7[0x204]) & 0x800)
   	ioCONTROLI0(adr,*((u16 *)&io_mem7[0x1A0]),AMM_HWORD);
   if((data & 0xA000) == 0){
       if(eeprom.id == ID_EEPROM_AUTO && (eeprom.control == 3 || eeprom.control == 0xB) && eeprom.adrSize == (u8)-1)
           eeprom.adrSize = (u8)(eeprom.mode-1);
       eeprom.mode = 0;
       eeprom.status &= ~1;
   }
}
//---------------------------------------------------------------------------
static void ioEEPROMCONTROLI(u32 adr,u32 data,u8 am)
{
	if(eeprom.mode == 0){
       if(!data)
           return;
#ifdef _DEBPRO
       if(data != 1 && data != 3 && data != 10 && data != 11 && data != 5 && data != 2 && data != 6)
           EnterDebugMode(FALSE);
#endif
		eeprom.control = data;
       eeprom.mode = 1;
       eeprom.adr = 0;
       WriteMsgConsolle(NULL,"%cFlash Memory 0x%04X 0%08X %08X",MSGT_FLASH,eeprom.control,data,eeprom.adr);
       return;
   }
   switch(eeprom.control){
       case 1:
           if(++eeprom.mode == 2){
               eeprom.status = (u8)(data & ~1);
               eeprom.mode = 0;
           }
       break;
       case 0xB:
           if(eeprom.mode == 1 && eeprom.adrSize == 2)
               eeprom.adr = 1;
       case 3:
           if(eeprom.mode >= eeprom.adrSize){
               if(eeprom.adrSize != (u8)-1)
                   eeprom.mode = 0;
               return;
           }
		    eeprom.adr <<= 8;
		    eeprom.adr |= data;
           eeprom.mode++;
       break;
       case 0xA:
           if(eeprom.mode == 1 && eeprom.adrSize == 2)
               eeprom.adr = 1;
       case 2:
           if(eeprom.mode >= eeprom.adrSize){
				if(eeprom.status & 2){
                   if(eeprom.adr > 1048575)
                   	eeprom.adr = 1048575;
               	eeprom.buffer[eeprom.adr++] = (u8)data;
               	if(eeprom.adr > eeprom.adrMax)
                   	eeprom.adrMax = eeprom.adr;
                   bModified = 1;
               }
               else
                   adr = 0;
             	eeprom.mode++;
           }
           else{
               eeprom.index = 0;
           	eeprom.adr <<= 8;
//               data = (data >> 4) << 4;
		    	eeprom.adr |= data;
           	eeprom.mode++;
           }
       break;
       default:
           eeprom.mode = 0;
       break;
   }
}
//---------------------------------------------------------------------------
static u32 ioEEPROMCONTROLO(u32 adr,u8 am)
{
   switch(eeprom.control){
       case 1:
           return 0;
       case 3:
       case 0xB:
          	if(!eeprom.mode){
           	if(eeprom.adr > 1048576)
               	eeprom.adr &= 1048575;
              	return eeprom.buffer[eeprom.adr++];
          	}
           return 0;
       break;
       case 5:
           eeprom.mode = 0;
           if(eeprom.id == ID_EEPROM_NONE)
           	return 0xFF;
           return (eeprom.status & ~3);//0xFF - None
       case 2:
       case 0xA:
           return 0;
       case 6:
           eeprom.adr = 0;
           eeprom.status |= 2;
       default:
           eeprom.mode = 0;
           return 0;
   }          //83DE600a
}
//---------------------------------------------------------------------------
void OnSelectEEPROM(WORD wID)
{
   switch(wID){
       case ID_FLASH_2MBIT:
       case ID_FLASH_4MBIT:
       case ID_FLASH_8MBIT:
           eeprom.adrSize = 4;
       break;
       case ID_EEPROM_4KBIT:
           eeprom.adrSize = 2;
       break;
       case ID_EEPROM_512KBIT:
       case ID_EEPROM_64KBIT:
           eeprom.adrSize = 3;
       break;
       case ID_EEPROM_AUTO:
           eeprom.adrSize = (u8)-1;
       break;
		default:
       	eeprom.adrSize = 0;
       break;
   }
   eeprom.id = wID;
}
//---------------------------------------------------------------------------
BOOL EEPROM_Import(const char *fileName,WORD wID)
{
   LFile *pFile;
   u32 fileSize;
   BOOL res;
   u8 buf[10],*p;

   fileSize = 0;
   if((pFile = new LFile(fileName)) == NULL)
       return FALSE;
   res = FALSE;
   if(!pFile->Open())
       goto ex_EEPROM_Import;
   if((fileSize = pFile->Size()) == 0)
       goto ex_EEPROM_Import;
   switch(wID){
       case ID_EEPROM_SCFSAVE:
       case ID_EEPROM_SCONESAVE:
           if((p = (u8 *)LocalAlloc(LPTR,fileSize+10)) != NULL){
           	if(pFile->Read(p,fileSize) == fileSize){
               	for(;fileSize > 0;fileSize--){
                   	if(p[fileSize-1] != 0xFF)
                       	break;
                   }
                   fileSize = (1 << (log2(fileSize) + 1));
                   if(fileSize)
               		res = TRUE;

           	}
               LocalFree(p);
           }
       break;
       case ID_EEPROM_M3SAVE:
           fileSize -= 1024;
           pFile->Seek(fileSize);
           pFile->Read(buf,5);
           if(lstrcmpi((char *)buf,"BOOT") == 0){
               pFile->SeekToBegin();
               res = TRUE;
           }
       break;
   }
   if(!res)
       goto ex_EEPROM_Import;
   if(fileSize < 513){
       fileSize = 512;
       wID = ID_EEPROM_4KBIT;
   }
   else if(fileSize < 8193){
       fileSize = 8192;
       wID = ID_EEPROM_64KBIT;
   }
   else if(fileSize < 65537){
       fileSize = 65536;
       wID = ID_EEPROM_512KBIT;
   }
   else if(fileSize < 262145){
       wID = ID_FLASH_2MBIT;
       fileSize = 262144;
   }
   else if(fileSize < 524289){
       wID = ID_FLASH_4MBIT;
       fileSize = 524288;
   }
   else{
       wID = ID_FLASH_8MBIT;
       fileSize = 1048576;
   }
   ds.OnMenuSelect(wID);
//   pFile->Read(eeprom.buffer,fileSize);
   res = TRUE;
ex_EEPROM_Import:
   delete pFile;
   return res;
}
//---------------------------------------------------------------------------
BOOL EEPROM_Save(LStream *pFile)
{
   pFile->Write(&eeprom,sizeof(eeprom));
   pFile->Write(&card,sizeof(card));
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL EEPROM_Load(LStream *pFile,int ver)
{
	u8 adrSize;
   u32 size;

	if(ver < 34)
       size = 0x80014;
   else if(ver < 38)
       size = 0x8001C;
   else
       size = sizeof(eeprom);
   pFile->Read(&eeprom,size);
	if(ver > 38)
   	size = sizeof(CARD);
   else
		size = 32;
 	pFile->Read(&card,size); //1776
   adrSize = eeprom.adrSize;
   ds.OnMenuSelect(eeprom.id);
   eeprom.adrSize = adrSize;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL EEPROM_Save(char *fileName)
{
   u32 fileSize;
   LFile *pFile;
   LString s;

#ifdef _LINO
   return TRUE;
#endif
   if(eeprom.adrMax == 0)
   	return TRUE;
   s = fileName;
   if(s.IsEmpty())
       return FALSE;
   s.AddEXT(".sav");
   fileSize = 0;
   switch(eeprom.id){
   	default:
       	fileSize = 0;
       break;
       case ID_FLASH_8MBIT:
           fileSize = 1048576;
       break;
       case ID_FLASH_4MBIT:
           fileSize = 524288;
       break;
       case ID_FLASH_2MBIT:
           fileSize = 262144;
       break;
       case ID_EEPROM_4KBIT:
           fileSize = 512;
       break;
       case ID_EEPROM_512KBIT:
           fileSize = 65536;
       break;
       case ID_EEPROM_64KBIT:
           fileSize = 8192;
       break;
       case ID_EEPROM_AUTO:
           if(eeprom.adrSize == 2)
           	fileSize = 512;
           else if(eeprom.adrSize == 3){
           	if(eeprom.adrMax < 8193)
               	fileSize = 8192;
           	else
               	fileSize = 65536;
           }
           else if(eeprom.adrSize == 4){
           	if(eeprom.adrMax < 262145)
               	fileSize = 262144;
				else
               	fileSize = 524288;
           }
           else
           	return TRUE;
       break;
   }
   if(fileSize < 1)
       return FALSE;
   if((pFile = new LFile(s.c_str())) == NULL)
       return FALSE;
   if(!pFile->Open(GENERIC_WRITE,CREATE_ALWAYS)){
       delete pFile;
       return FALSE;
   }
   pFile->Write(eeprom.buffer,fileSize);
   delete pFile;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL EEPROM_Load()
{
   LFile *pFile;
   u32 fileSize,i;
   LString s;

   s.Length(MAX_PATH+1);
  	if(!ds.BuildFileName(PT_SAVEGAME,s.c_str(),MAX_PATH))
   	return FALSE;
   if((pFile = new LFile(s.c_str())) == NULL)
       return FALSE;
   if(!pFile->Open()){
       delete pFile;
       return FALSE;
   }
   fileSize = pFile->Size();
	i = pFile->Read(eeprom.buffer,fileSize);
   if(i != fileSize){
		delete pFile;
		return FALSE;
	}
   delete pFile;
   if(eeprom.id != ID_EEPROM_AUTO){
       switch(fileSize){
           case 1048576:
               eeprom.id = ID_FLASH_8MBIT;
           break;
           case 524288:
               eeprom.id = ID_FLASH_4MBIT;
           break;
           case 262144:
               eeprom.id = ID_FLASH_2MBIT;
           break;
           case 512:
               eeprom.id = ID_EEPROM_4KBIT;
           break;
           case 8192:
               eeprom.id = ID_EEPROM_64KBIT;
           break;
           case 65536:
               eeprom.id = ID_EEPROM_512KBIT;
           break;
       }
#ifdef __WIN32__
       SendMessage(ds.Handle(),WM_COMMAND,MAKEWPARAM(eeprom.id,0),0);
#endif
   }
   else if(eeprom.id != ID_EEPROM_NONE){
       if(fileSize < 513){
           eeprom.adrSize = 2;
           eeprom.adrMax = 512;
       }
       else if(fileSize < 8193){
           eeprom.adrSize = 3;
           eeprom.adrMax = 8192;
       }
       else if(fileSize < 65537){
           eeprom.adrSize = 3;
           eeprom.adrMax = 65536;
       }
       else if(fileSize < 262145){
           eeprom.adrSize = 4;
           eeprom.adrMax = 262144;
       }
       else if(fileSize < 524289){
           eeprom.adrSize = 4;
           eeprom.adrMax = 524288;
       }
       else{
           eeprom.adrMax = 1048576;
           eeprom.adrSize = 4;
       }
   }
   else{
   	eeprom.adrMax = 0;
       eeprom.adrSize = 0;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
WORD get_EEPROMID()
{
   return eeprom.id;
}
//---------------------------------------------------------------------------
BOOL EEPROM_IsModified()
{
   return bModified;
}
//---------------------------------------------------------------------------
void EEPROM_AutoSave(BOOL bForce)
{
	LString nameFile;

   if(bForce)
       bModified = 1;
   if(!bModified)
       return;
   nameFile.Length(MAX_PATH+1);
   if(ds.BuildFileName(PT_SAVEGAME,nameFile.c_str(),MAX_PATH))
		EEPROM_Save(nameFile.c_str());
   bModified = 0;
}
//---------------------------------------------------------------------------
void InitCARDTable(LPIFUNC *p,LPIFUNC *p1,LPOFUNC *p2,LPOFUNC *p3)
{
   int i;

   p[0x1A0] = ioCONTROLI0;
   p[0x1A1] = ioCONTROLI0;
   p1[0x1A0] = ioCONTROLI07;
   p1[0x1A1] = ioCONTROLI07;
   p1[0x1A2] = ioEEPROMCONTROLI;
   p3[0x1A2] = ioEEPROMCONTROLO;
   for(i=0x1A4;i<0x1A8;i++){
       p[i] = p1[i] = ioCONTROLI1;
       p2[i] = p3[i] = ioCONTROLO1;
   }
   for(;i<0x1B0;i++)
       p[i] = p1[i] = ioCONTROLI2;
   p2[0x2010] = p3[0x2010] = ioDATAO;
   p[0x2010] = p1[0x2010] = ioDATAI;
}









