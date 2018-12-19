#include "gbabackup.h"
#include "lds.h"
#include "util.h"

//---------------------------------------------------------------------------
enum eeCommand{eecNull,eecWrite,eecErase,eecSeek,eecNull2,eecUnlock,eecNull4,eecLock};
//---------------------------------------------------------------------------
static SRAMID sramID[]= {
   {0xd4bf,0,0},
   {0x1cc2,0,0},
   {0x1b32,0,0},
   {0x3d1f,0,0},
   {0x1362,0,1},
   {0x09c2,0,1},
   {0xd5bf,0,1},
   {0,0}
};
static u8 indexSRAM=(u8)-1;
CNTPACKROM eeprom_pack[2];
//---------------------------------------------------------------------------
BOOL IsSRAM128K()
{
   if(indexSRAM > ID_SRAM_END - ID_SRAM_START)
       return FALSE;
   return (BOOL)(sramID[indexSRAM].mode == 1 ? TRUE : FALSE);
}
//-----------------------------------------------------------------------
BOOL AllocSRAM(BOOL bAlloc)
{
   int size;
   LPSRAM sram;

   if(!bAlloc)
       return TRUE;
   if(IsSRAM128K())
       size = 128 * 1024;
   else
       size = 64*1024;
   sram = (LPSRAM)sram_mem;
   sram->buffer = sram_mem + sizeof(SRAM);
   sram->mask = size - 1;
   return TRUE;
}
//---------------------------------------------------------------------------
void SetGamePackID(WORD wID)
{
   int i;
   u8 sizeCommand,bitCommand;

   switch(wID){
       case ID_SRAM_SST64K:
           indexSRAM = 0;
       break;
       case ID_SRAM_SST128K:
           indexSRAM = 6;
       break;
       case ID_SRAM_ATMEL64K:
           indexSRAM = 3;
       break;
       case ID_SDRAM_SANYO128K:
           indexSRAM = 4;
       break;
       case ID_SRAM_MACRO64K:
           indexSRAM = 1;
       break;
       case ID_SRAM_MACRO128K:
           indexSRAM = 5;
       break;
       case ID_SRAM_PANASONIC64K:
           indexSRAM = 2;
       break;
       case ID_EEPROM_32K:
           indexSRAM = 0x80;
           sizeCommand = 16;
           bitCommand = 3;
       break;
       case ID_EEPROM_128K:
           indexSRAM = 0x81;
           sizeCommand = 32;
           bitCommand = 3;
       break;
   }
   if(indexSRAM >= 0x80){
       eeprom_pack[0].sizeCommand = sizeCommand;
       eeprom_pack[1].sizeCommand = sizeCommand;
       eeprom_pack[0].bitCommand = bitCommand;
       eeprom_pack[1].bitCommand = bitCommand;
   }
   for(i=ID_SRAM_START;i<=ID_EEPROM_END;i++)
       CheckMenuItem(GetMenu(ds.Handle()),i,MF_BYCOMMAND|(wID == i ? MF_CHECKED : MF_UNCHECKED));
   AllocSRAM((BOOL)(indexSRAM < 0x80 ? TRUE : FALSE));
}
//---------------------------------------------------------------------------
WORD GetGamePackID()
{
   switch(indexSRAM){
       case 0:
           return ID_SRAM_SST64K;
       case 1:
           return ID_SRAM_MACRO64K;
       case 2:
           return ID_SRAM_PANASONIC64K;
       case 3:
           return ID_SRAM_ATMEL64K;
       case 4:
           return ID_SDRAM_SANYO128K;
       case 5:
           return ID_SRAM_MACRO128K;
       case 6:
           return ID_SRAM_SST128K;
       case 0x80:
           return ID_EEPROM_32K;
       case 0x81:
           return ID_EEPROM_128K;
   }
   return 0;
}
//---------------------------------------------------------------------------
BOOL UseSRAM()
{
   return (BOOL)(indexSRAM <= ID_SRAM_END - ID_SRAM_START ? TRUE : FALSE);
}
//---------------------------------------------------------------------------
BOOL WriteFileEEPROM(LStream *pFile)
{
   LPPACKROM p1;
   LPCNTPACKROM p;
   int i1,i,rom_size;
   u8 ver;

   if(pFile == NULL || !pFile->IsOpen())
       return FALSE;
   for(i1=0;i1<2;i1++){
       p = &eeprom_pack[i1];
       for(i=0;i<0x200;i++){
           if(p->rom_pack[i].buffer != NULL){
               if(!pFile->GetCurrentPosition()){
                   ver = 0xFF;
                   pFile->Write(&ver,1);
               }
               pFile->Write(&i1,sizeof(u8));
               pFile->Write(&i,sizeof(u16));
               p1 = ((LPPACKROM)&p->rom_pack[i]);
               rom_size = p1->size + 1;
               pFile->Write(&rom_size,sizeof(u32));
               pFile->Write(p1->buffer,rom_size);
           }
       }
   }
   return TRUE;
}
//---------------------------------------------------------------------------
DWORD ReadFileEEPROM(LStream *pFile)
{
   DWORD res;
   u8 ver,*pBuffer,value8;
   int i,i1,rom_size,i3,i4,i5,i2;
   u16 *pBuffer2,value;

   if(UseSRAM() || pFile == NULL || !pFile->IsOpen())
       return 0;
  	pFile->SeekToBegin();
   if(pFile->Read(&ver,1) != 1)
       return 0;
   if(ver != 0xFF)
       pFile->SeekToBegin();
   res = 0;
   do{
       i = rom_size = 0;
       i1 = 0;
       if(pFile->Read(&i1,sizeof(u8)) != 1)
           goto ex_ReadFileEEPROM;
       if(pFile->Read(&i,sizeof(u16)) != 2)
           goto ex_ReadFileEEPROM;
       if(ver != 0xFF){
           if(pFile->Read(&rom_size,sizeof(u16)) != 2)
               goto ex_ReadFileEEPROM;
           pBuffer = (u8 *)GlobalAlloc(GPTR,0x20000);
           if(pBuffer == NULL)
               goto ex_ReadFileEEPROM;
           pBuffer2 = (u16 *)GlobalAlloc(GPTR,0x20000);
           if(pBuffer2 == NULL){
               GlobalFree((HGLOBAL)pBuffer);
               goto ex_ReadFileEEPROM;
           }
           if(!pFile->Read((char *)pBuffer2,rom_size*sizeof(u16))){
               GlobalFree((HGLOBAL)pBuffer2);
               GlobalFree((HGLOBAL)pBuffer);
               goto ex_ReadFileEEPROM;
           }
           //Converte da old a new
           for(i4 = i3 = 0;i3< rom_size;i3+=16,i4+=8){
               for(i5 = 0;i5 < 9;i5++){
                   value = pBuffer2[i3+i5];
                   value8 = 0;
                   for(i2=0;i2<16;i2 += 2){
                       if((value & (1 << i2)) != 0)
                           value8 |= (u8)(1 << (i2 >> 1));
                   }
                   if(i3 != 0 && i5 == 0 && pBuffer[i4] != 0)
                       continue;
                   pBuffer[i4+i5] = value8;
               }
           }
           rom_size = i4;
           GlobalFree((HGLOBAL)pBuffer2);
           i1 = 1;
           i = 0;
/*           HANDLE fp1;
           fp1 = OpenStream("c:\\windows\\temp\\lino",GENERIC_WRITE,CREATE_ALWAYS);
           ver = 0xFF;
           WriteStream(fp1,&ver,1);
           WriteStream(fp1,&i1,sizeof(u8));
           WriteStream(fp1,&i,sizeof(u16));
           WriteStream(fp1,&rom_size,sizeof(u16));
           WriteStream(fp1,pBuffer,rom_size);
           CloseStream(fp1);*/
       }
       else{
           if(pFile->Read(&rom_size,sizeof(u32)) != 4)
               goto ex_ReadFileEEPROM;
           pBuffer = (u8 *)GlobalAlloc(GPTR,0x4000);
           if(pBuffer == NULL)
               goto ex_ReadFileEEPROM;
           if(!pFile->Read(pBuffer,rom_size)){
               GlobalFree((HGLOBAL)pBuffer);
               break;
           }
       }
       eeprom_pack[i1].rom_pack[i].buffer = pBuffer;
       eeprom_pack[i1].rom_pack[i].size = rom_size - 1;
       eeprom_pack[i1].isUsed = 1;
       res |= i1 == 0 ? 2 : 4;
   }while(1);
ex_ReadFileEEPROM:
   return res;
}
//---------------------------------------------------------------------------
int FileFlashROM(BOOL bRead)
{
   LFile *fp;
   LString nameFile;
   BOOL res;

   if(UseSRAM())
       return FALSE;
   nameFile.Capacity(MAX_PATH+2);
   if(!ds.BuildFileName(PT_GBA_SG_EEPROM,nameFile.c_str(),MAX_PATH))
       return FALSE;
   if((fp = new LFile(nameFile.c_str())) == NULL)
       return FALSE;
   res = FALSE;
   if(!bRead){
       if(!eeprom_pack[0].isUsed && !eeprom_pack[1].isUsed){
           res = TRUE;
           goto EX_FileFlashROM;
       }
       if(!fp->Open(GENERIC_WRITE,CREATE_ALWAYS))
           goto EX_FileFlashROM;
       res = WriteFileEEPROM(fp);
       eeprom_pack[0].isUsed = eeprom_pack[1].isUsed = 0;
   }
   else{
       if(!fp->Open(GENERIC_READ,OPEN_EXISTING))
           goto EX_FileFlashROM;
       res = ReadFileEEPROM(fp);
   }
EX_FileFlashROM:
   if(fp != NULL)
       delete fp;
   return res;
}
//---------------------------------------------------------------------------
BOOL WriteFileSRAM(LStream *pFile)
{
   DWORD dwSize;

   if(pFile == NULL || !pFile->IsOpen())
       return FALSE;
   if((dwSize = (DWORD)(((LPSRAM)sram_mem)->size + 1)) == 1)
       return TRUE;
   if(pFile->Write(((LPSRAM)sram_mem)->buffer,dwSize) != dwSize)
       return FALSE;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL ReadFileSRAM(LStream *pFile)
{
   DWORD file_size;

   if(pFile == NULL || !pFile->IsOpen())
       return FALSE;
   if(!UseSRAM())
       return FALSE;
   if((file_size = pFile->Size() - 1) < 1)
       return FALSE;
   pFile->SeekToBegin();
   ((LPSRAM)sram_mem)->size = 0;
   if(file_size <= 0x20000){
       if(!IsSRAM128K() && file_size > 0xFFFF)
           SetGamePackID(ID_SRAM_MACRO128K);
       if(IsSRAM128K())
           ZeroMemory(((LPSRAM)sram_mem)->buffer,0x20000);
       else
           ZeroMemory(((LPSRAM)sram_mem)->buffer,0x10000);
       pFile->Read(((LPSRAM)sram_mem)->buffer,file_size+1);
       ((LPSRAM)sram_mem)->size = file_size;
       return TRUE;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL FileSRAM(BOOL bRead)
{
   LFile *fp;
   LString nameFile;
   BOOL res,bLoad;

	nameFile.Capacity(MAX_PATH+10);
   if(!UseSRAM() || !ds.BuildFileName(PT_GBA_SG_FLASH,nameFile.c_str(),MAX_PATH))
       return FALSE;
   nameFile.LowerCase();
   if((fp = new LFile(nameFile.c_str())) == NULL)
       return FALSE;
   ds.get_RomStatus(&bLoad,NULL);
   res = FALSE;
   if(bRead){
       if(fp->Open())
           res = ReadFileSRAM(fp);
   }
   else if(bLoad && sram_mem != NULL && ((LPSRAM)sram_mem)->size > 0){
       if(fp->Open(GENERIC_WRITE,CREATE_ALWAYS))
           res = WriteFileSRAM(fp);
   }
   if(fp != NULL)
       delete fp;
   return res;
}
//---------------------------------------------------------------------------
u32 ReadRomPack(int nPack,u32 adress,u8 mode)
{
	LPCNTPACKROM pPack;
	u8 *p;
   u32 value;

   if(UseSRAM())
       return 0;
   if(nPack > 1 || nPack < 0)
   	return 0;
	pPack = &eeprom_pack[nPack];
	switch(mode){
		case AMM_BYTE:
       	return 0;
       case AMM_HWORD:
           switch(pPack->mode){
               case 1:
                   return 1;
               case 0:
                   if((p = pPack->rom_pack[pPack->blocco].buffer) == NULL)
                       return 0;
                   if((p[pPack->byteIndex] & (1 << pPack->bitIndex)) != 0)
                       value = 1;
                   else
                       value = 0;
                   if((pPack->bitIndex = (u8)((pPack->bitIndex + 1) & 0x7)) == 0)
                       pPack->byteIndex++;
                   return value;
           }
       case AMM_WORD:
			return 0;
   }
	return 0;
}
//---------------------------------------------------------------------------
u32 WriteRomPack(int nPack,u32 adress,u32 data,u8 mode)
{
   LPCNTPACKROM pPack;
   u8 *p;

   if(UseSRAM())
       return 0;
   if(nPack > 1 || nPack < 0)
   	return 0;
	pPack = &eeprom_pack[nPack];
   switch(mode){
       case AMM_HWORD:
           if((adress = (u32)(u16)adress) == 0){
               pPack->com = 0x0;
               pPack->mode = 0;
               pPack->blocco = 0;
               pPack->byteIndex = 0;
               pPack->bitIndex = 0;
               pPack->bytesWrite = 0;
           }
           if(adress < pPack->sizeCommand){
               if((data & 1) != 0)
                   pPack->com |= (u32)(1 << (adress >> 1));
               if(adress == (pPack->sizeCommand - 2)){
                   pPack->blocco = (u16)(((pPack->com & 0x01FE000) >> 13));
                   if(pPack->rom_pack[pPack->blocco].buffer == NULL){
                       pPack->rom_pack[pPack->blocco].buffer = (u8 *)GlobalAlloc(GPTR,0x4000);
                       pPack->rom_pack[pPack->blocco].size = 0;
                   }
                   pPack->byteIndex = (u32)((pPack->com & (0x1FFF & ~pPack->bitCommand)) << 1);
                   pPack->bitIndex = (u8)((pPack->com & pPack->bitCommand) == eecWrite ? 4 : 0);
                   pPack->bytesWrite = 0;
               }
           }
           else{
               switch((pPack->com & pPack->bitCommand)){
                   case eecSeek:
                   break;
                   case eecWrite:
                       if(pPack->bytesWrite > 63){
                           pPack->isUsed = 1;
                           pPack->mode = 1;
                       	break;
                   	}
                   	p = pPack->rom_pack[pPack->blocco].buffer;
                   	if((data & 1))
                       	p[pPack->byteIndex] |= (u8)(1 << pPack->bitIndex);
                   	else
                       	p[pPack->byteIndex] &= (u8)~(1 << pPack->bitIndex);
                   	if((pPack->bitIndex = (u8)((pPack->bitIndex + 1) & 0x7)) == 0)
                       	pPack->byteIndex++;
                   	if(pPack->byteIndex > pPack->rom_pack[pPack->blocco].size)
                       	pPack->rom_pack[pPack->blocco].size = pPack->byteIndex;
                   	pPack->bytesWrite++;
               	break;
           	}
       	}
   	break;
	}
	return TRUE;
}
//---------------------------------------------------------------------------
u8 ReadSRAM(u32 adress)
{
   LPSRAM p;
   LPSRAMID p1;

   if(!UseSRAM())
       return 0;
   p = (LPSRAM)sram_mem;
   adress = (u32)(u16)adress;
   switch(p->com){
       case 0x80:
           p->com = 0;
           p->mode = 0;
           return 0xFF;
       case 0x90:
           if(adress < 2){
               p1 = &sramID[indexSRAM];
               if((adress & 1))
                   return *((u8 *)(&p1->ID) + 1);
               else
                   return *((u8 *)(&p1->ID));
           }
       break;
   }
   return p->buffer[(p->blocco << 16) | adress];
}
//---------------------------------------------------------------------------
void WriteSRAM(u32 adress,u8 data)
{
	LPSRAM p;
   
   if(!UseSRAM())
       return;
   p = (LPSRAM)sram_mem;
   adress = (u16)adress;
   if(adress == 0x5555 && p->com != 0xA0){
       if(data == 0xF0 || data == 0xAA){
           if(p->mode != 2 || (p->com != 0x80 && p->com != 0xA0)){
               p->com = 0;
               p->mode = 0;
           }
           return;
       }
       if(p->mode == 1){
           p->com = (u16)data;
           p->mode = 2;
           if(data == 0xF0){
               p->com = 0;
               p->mode = 0;
           }
           return;
       }
   }
   else if(adress == 0x2AAA && p->com != 0xA0){
       if(p->mode == 0){
           if(data == 0x55){
               p->mode = 1;
               return;
           }
       }
       else if(p->mode == 2){
           if(data == 0x55 && p->com == 0x80)
               p->mode = 3;
           return;
       }
   }
   else if(p->mode == 3 && p->com != 0xA0){
       if(data == 0x30){
           //erase sector
           adress = (p->blocco << 16) | (adress & 0xF000);
           ZeroMemory(&p->buffer[adress],0x1000);
       }
       else
           ZeroMemory(p->buffer,IsSRAM128K() ? 0x20000 : 0x10000);
       p->mode = 0;
       return;
   }
   else if(p->mode == 2 && p->com == 0xB0){
       p->blocco = (u16)(u8)data;
       p->mode = 0;
       return;
   }
   adress |= (p->blocco << 16);
   if((s32)adress > p->size)
       p->size = adress;
   p->buffer[adress] = (u8)data;
   if(p->mode == 2 && p->com == 0xA0){
       p->mode = 0;
       p->com = 0;
   }
}
//---------------------------------------------------------------------------
void ResetGBABackup()
{
	ZeroMemory(&eeprom_pack[0],sizeof(eeprom_pack[0]));
	ZeroMemory(&eeprom_pack[1],sizeof(eeprom_pack[1]));
 	if(indexSRAM == (u8)-1)
		SetGamePackID(ID_EEPROM_32K);
   else
       SetGamePackID(GetGamePackID());
}
//---------------------------------------------------------------------------
void FreeRomPack(u8 pack)
{
   int i;
   LPPACKROM p1;

   pack = (u8)log2(pack);
   p1 = eeprom_pack[pack].rom_pack;
   for(i=0;i<0x200;i++){
       if(p1[i].buffer != NULL)
           GlobalFree((HGLOBAL)p1[i].buffer);
       p1[i].buffer = NULL;
   }
   eeprom_pack[pack].com = 0x0;
   eeprom_pack[pack].mode = 0;
   eeprom_pack[pack].blocco = 0;
   eeprom_pack[pack].byteIndex = 0;
   eeprom_pack[pack].bitIndex = 0;
   eeprom_pack[pack].bytesWrite = 0;
}
