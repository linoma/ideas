#include "dsmem.h"
#include "fstream.h"
#include "io.h"
#include "lds.h"
#include "bios7.h"
#include "bios7_gba.h"
#include "bios9.h"
#include "card.h"
#include "zipfile.h"
#include "7zfile.h"
#include "elf.h"
#include "util.h"
#include "gbabackup.h"

//---------------------------------------------------------------------------
u8 *io_mem;
u8 *int_mem2;
u8 *int_mem3;
u8 *ext_mem;
u8 *ext_mem2;
u8 *io_mem;
u8 *io_mem2;
u8 *io_mem7;
u8 *rom_pack[0x200];
u8 *pal_mem;
u8 *obj_mem;
u8 *video_mem;
u8 *bios7_mem;
u8 *bios9_mem;
u8 *crIO_mem;
u8 *fw_mem;
u8 *sram_mem;
u32 *video_cache;
//---------------------------------------------------------------------------
u8 VRAMmap[1024]={0};
u8 mapVRAM[41]={0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,5,6,8,8,9};
u32 VRAMBankSize[10]={0x20000,0x20000,0x20000,0x20000,0x10000,0x4000,0x4000,0,0x8000,0x4000};
u32 waitState9[257];
u32 waitState7[257];
u32 ulExRamPack,ulMMMask;
#ifdef _DEBPRO
FILE *fpLog;
u32 lastWrite = 0;
#endif
//---------------------------------------------------------------------------
LMMU::LMMU()
{
	int_mem = NULL;
   int_mem3 = NULL;
   ext_mem = NULL;
   dwMMSize = 0;
	pReader[0] = pReader[1] = NULL;
	ZeroMemory(rom_pack,sizeof(rom_pack));
	nIncreaseSpeed = 0;
   ulExRamPack = 0;
   dwAllocated = 0x1000 + 0x18000 + 0x800 + 0x4000*4 +
       0xB000*2 + 0x40000 + 656*1024 + 0x24000 + 0x4000 + 82*1024;
   dwUsed = dwAllocated - (80 + 82) * 1024;
	savegamePath = "";
   savestatePath = "";
   screenshotPath = "";
   cheatPath = "";
   nEmuMode = EMUM_NDS;
#ifdef _DEBPRO
   fpLog = NULL;
#endif
}
//---------------------------------------------------------------------------
LMMU::~LMMU()
{
   int i;

#ifdef _DEBPRO
	if(fpLog != NULL)
   	fclose(fpLog);
   fpLog = NULL;
#endif
   for(i = 0;i < 0x200;i++){
   	if(rom_pack[i] != NULL)
       	LocalFree(rom_pack[i]);
       rom_pack[i] =  NULL;
   }
	CloseRom();
	if(int_mem != NULL)
   	LocalFree(int_mem);
   if(ext_mem != NULL)
       LocalFree(ext_mem);
}
//---------------------------------------------------------------------------
BOOL LMMU::BuildFileName(int type,char *buffer,int maxLength,const char *fileExt,int ID)
{
   LString ext,dir,s;

	if(buffer == NULL)                     
   	return FALSE;
   switch(type){
		case PT_SAVEGAME:
       	dir = savegamePath;
           ext = ".sav";
       break;
		case PT_SAVESTATE:
       	dir = savestatePath;
           ext = ".sgs";
       break;
		case PT_SCREENSHOT:
       	dir = screenshotPath;
           ext = ".bmp";
       break;
       case PT_CHEAT:
       	dir = cheatPath;
           ext = ".cht";
       break;
       case PT_GBA_SG_FLASH:
       	dir = savegamePath;
           ext = ".srm";
       break;
       case PT_GBA_SG_EEPROM:
       	dir = savegamePath;
           ext = ".erm";
       break;
   	default:
       	return FALSE;
   }
   if(GetFileAttributes(dir.c_str()) == 0xFFFFFFFF)
   	dir = "";
//   if(dir.IsEmpty())
//   	dir = LString(buffer).Path();
	if(dir.IsEmpty())
   	dir = lastFileName.Path();
	if(dir.IsEmpty())
   	return FALSE;
   if(dir[dir.Length()] != DPC_PATH)
   	dir += DPS_PATH;
/*   s = LString(buffer).FileName();
	if(!s.IsEmpty())
   	dir += s;
	else */if(!lastFileName.IsEmpty())
   	dir += lastFileName.FileName();
   else
   	return FALSE;
   if(ID != -1)
   	dir += ID;
   if(fileExt != NULL)
   	ext = fileExt;
   if(ext[1] != '.')
   	ext = LString(".") + ext;
   dir.AddEXT(ext.c_str());
   if(maxLength == -1)
   	maxLength = dir.Length() + 1;
   strncpy(buffer,dir.c_str(),maxLength);
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LMMU::Save(LStream *pFile)
{
	pFile->Write(int_mem,dwUsed);
   pFile->Write(VRAMmap,640);
   pFile->Write(ext_mem,dwMMSize);
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LMMU::Load(LStream *pFile,int ver)
{
	int i;

   i = dwUsed+dwMMSize;
   if(ver < 36)
       i -= 0x4000;
	pFile->Read(int_mem,0x18000);
   i -= 0x18000;
   if(ver < 46){//2072498
       pFile->Read(ext_mem,4*1024*1024);
   }
   i -= 4 *1024*1024;
   pFile->Read(ext_mem2,i);
   for(i = 0x240;i < 0x24a;i++)
       MMUMainremapVRAM((u8)(i - 0x240),(u8)io_mem[i]);
   pFile->Read(VRAMmap,640);
   if(ver > 45)
       pFile->Read(ext_mem,dwMMSize);
   return TRUE;
}
//---------------------------------------------------------------------------
void LMMU::ResetCpuSpeed()
{
	int i;
                                                   
   for(i=0;i<256;i++){
       waitState9[i] = 0x30002;
       waitState7[i] = 0x20001;
   }
   waitState9[0] = 0x10001;
   waitState9[1] = 0x10001;
   waitState9[6] = 0x20001;
   waitState9[0xFF] = 0x20001;
   waitState9[8] = waitState9[9] = 0x80005;
   waitState9[0xA] = 0x50005;
   waitState9[256] = 0x80008;

   waitState7[0] = 0x10001;
   waitState7[1] = 0x10001;
   waitState7[8] = waitState7[9] = 0x80005;
   waitState7[0xA] = 0x50005;
   waitState7[256] = 0x80008;
}
//---------------------------------------------------------------------------
void LMMU::IncreaseCpuSpeed(int i)
{
	int i1,i2;

   ResetCpuSpeed();
   if(i > 1){
       for(i1=0;i1<257;i1++){
           i2 = (u16)waitState9[i1] * i / 100;
           if(i2 == 0) i2 = 1;
           waitState9[i1] = (waitState9[i1] & 0xFFFF0000) | ((waitState9[i1] & 0xFFFF) + i2);
           i2 = (waitState9[i1] >> 16) * i / 100;
           if(i2 == 0) i2 = 1;
           waitState9[i1] = (waitState9[i1] & 0xFFFF) | (((waitState9[i1] >> 16) + i2) << 16);
           i2 = (u16)waitState7[i1] * i / 100;
           if(i2 == 0) i2 = 1;
           waitState7[i1] = (waitState7[i1] & 0xFFFF0000) | ((waitState7[i1] & 0xFFFF) + i2);
           i2 = (waitState7[i1] >> 16) * i / 100;
           if(i2 == 0) i2 = 1;
           waitState7[i1] = (waitState7[i1] & 0xFFFF) | (((waitState7[i1] >> 16) + i2) << 16);
       }
   }
   nIncreaseSpeed = i;
   if(pPlugInContainer != NULL)
       pPlugInContainer->NotifyState(-1,PNM_INCREASECPUSPEED,PIS_NOTIFYMASK,i);
}
//---------------------------------------------------------------------------
void LMMU::CloseRom(int slot)
{
	if(slot == 0 || slot == -1){
       if(pReader[0] != NULL)
           pReader[0]->Release();
       pReader[0] = NULL;
   }
   if(slot == 1 || slot == -1){
       if(pReader[1] != NULL)
           pReader[1]->Release();
       pReader[1] = NULL;
   }
}
//---------------------------------------------------------------------------
void LMMU::Reset()
{
	int i;

	CloseRom();
   for(i=0;i<0x200;i++){
   	if(rom_pack[i] != NULL)
       	LocalFree(rom_pack[i]);
       rom_pack[i] =  NULL;
   }
	crI = i_func;
   crIO_mem = io_mem;
   crO = o_func;

   ulExRamPack = (u32)(u8)ulExRamPack;
   dwIndexCache = 0;
	for(i=512;i<553;i++)
		VRAMmap[i] = (u8)(i - 512);

	for(i=0;i<45;i++)
		VRAMmap[i] = (u8)41;

	for(i=0;i<45;i++)
		VRAMmap[128+i] = (u8)41;

	for(i=0;i<45;i++)
		VRAMmap[256+i] = (u8)41;

	for(i=0;i<45;i++)
		VRAMmap[384+i] = (u8)41;

   MMUMainremapVRAM(0,0x81);
   MMUMainremapVRAM(2,0x84);
	if(int_mem != NULL)
		ZeroMemory(int_mem,dwUsed - 0x10000);
   if(ext_mem != NULL)
       ZeroMemory(ext_mem,dwMMSize);
#ifdef _DEBPRO
   if(fpLog != NULL)
   	fclose(fpLog);
//   fpLog = fopen("f:\\ideaslog.txt","wb+");
#endif
   IncreaseCpuSpeed(nIncreaseSpeed);

	ResetGBABackup();
}
//---------------------------------------------------------------------------
BOOL LMMU::Init()
{
   if((int_mem = (u8 *)LocalAlloc(LPTR,dwAllocated)) == NULL)
   	return FALSE;
   int_mem2 = int_mem + 0x8000;
// 	ext_mem = int_mem + 0x18000;
   ext_mem2 = int_mem + 0x18000;//(4*1048576);
   pal_mem = ext_mem2 + 0x1000;
	obj_mem = pal_mem + 0x800;
   io_mem = obj_mem + 0x4000;
   io_mem7 = io_mem + 0xB000;
	bios9_mem = io_mem7 + 0xB000;
   bios7_mem = bios9_mem + 0x8000;
	fw_mem = bios7_mem + 0x4000;
	video_mem = fw_mem + 0x40000;
   sram_mem = video_mem + 656*1024 + 16384;
   io_mem2 = io_mem + 0x3000;
   video_cache = (u32 *)(sram_mem + 0x10000);
   dwMMSize = 4 * 1024 * 1024;
   ulMMMask = dwMMSize - 1;
   ext_mem = (u8 *)LocalAlloc(LPTR,dwMMSize);
   if(ext_mem == NULL)
       return FALSE;
	Reset();
	return TRUE;
}
//---------------------------------------------------------------------------
void LMMU::set_ExpansionRam(u8 i)
{
   ulExRamPack = (u32)i;
}
//---------------------------------------------------------------------------
int LMMU::get_ExpansionRam()
{
   return (int)(u8)ulExRamPack;
}
//---------------------------------------------------------------------------
BOOL LMMU::LoadBios(const char *lpFileName,u8 type)
{
   LFile *pFile;
   u8 *p,c;
   FILE *fp;
   int i,i1;
   u32 size;
	BOOL res;

   if(type == 7){
   	res = FALSE;
   	if(lpFileName == NULL || lstrlen(lpFileName) == 0){
           CopyMemory(bios7_mem,my_bios7,sizeof(my_bios7));
           res = TRUE;
       }
       else{
           if((pFile = new LFile(lpFileName)) == NULL)
           	return FALSE;
           res = FALSE;
           if(pFile->Open()){
               if(pFile->Read(bios7_mem,16384))
                   res = TRUE;
           }
           delete pFile;
       }
       return res;
       type = 10;
   }
   if(type == 9){
		res = FALSE;
   	if(lpFileName == NULL || lstrlen(lpFileName) == 0){
           CopyMemory(bios9_mem,my_bios9,sizeof(my_bios9));
           res = TRUE;
       }
       else{
            if((pFile = new LFile(lpFileName)) == NULL)
                return FALSE;
            res = FALSE;
            if(pFile->Open()){
                if(pFile->Read(bios9_mem,32768))
                    res = TRUE;
            }
            delete pFile;
		}
       return res;
       type = 11;
   }
   if(type == 8){
		res = FALSE;
   	if(lpFileName == NULL || lstrlen(lpFileName) == 0){
           CopyMemory(bios7_mem,my_bios7_gba,sizeof(my_bios7_gba));
           res = TRUE;
       }
       else{
            if((pFile = new LFile(lpFileName)) == NULL)
                return FALSE;
            res = FALSE;
            if(pFile->Open()){
                if(pFile->Read(bios7_mem,0x4000))
                    res = TRUE;
            }
            delete pFile;
		}
       return res;
       type = 14;
   }
   if(type == 12 || type == 13){
       //       return TRUE;
   }
   if(lpFileName == NULL || (pFile = new LFile(lpFileName)) == NULL)
       return FALSE;
   if(pFile->Open()){
       switch(type){
           case 9:
           case 11:
               p = bios9_mem;
               size = 0x8000;
           break;
           case 7:
           case 10:
               p = bios7_mem;
               size = 0x4000;
           break;
           case 8:
           case 14:
           	p = bios7_mem;
               size = 0x4000;
           break;
           case 72:
               p = ext_mem + 0x7BFD80;
               size = 256*1024;
           break;
           case 13:
               p = NULL;
               size = pFile->Size();
           break;
           default:
               size = pFile->Size();
               p = int_mem;
           break;
       }
       if(p != NULL)
           pFile->Read(p,size);
#ifdef _DEBUG
       if(type == 12){
           fp = fopen("h:\\ideas\\arm7bin.h","wb");
           fprintf(fp,"#ifndef __ARM7BINH__\n");
           fprintf(fp,"#define __ARM7BINH__\n\n");
           fprintf(fp,"const unsigned char arm7bin[] = {\n");
               for(i=0;i<pFile->Size();){
                   for(i1=0;i1<16 && i < pFile->Size();i1++,i++)
                       fprintf(fp,"0x%02X,",(int)p[i]);
                   fprintf(fp,"\n");
               }
           fprintf(fp,"};\n#endif\n");
           fclose(fp);
       }
       if(type == 10){
           fp = fopen("h:\\ideas\\bios7.h","wb");
           fprintf(fp,"#ifndef __BIOS7H__\n");
           fprintf(fp,"#define __BIOS7H__\n\n");

           fprintf(fp,"const unsigned char my_bios7[] = {\n");
               for(i=0;i<pFile->Size();){
                   for(i1=0;i1<16 && i < pFile->Size();i1++,i++)
                       fprintf(fp,"0x%02X,",(int)p[i]);
                   fprintf(fp,"\n");
               }
           fprintf(fp,"};\n#endif\n");
           fclose(fp);
       }
       if(type == 14){
           fp = fopen("h:\\ideas\\bios7_gba.h","wb");
           fprintf(fp,"#ifndef __BIOS7GBAH__\n");
           fprintf(fp,"#define __BIOS7GBAH__\n\n");

           fprintf(fp,"const unsigned char my_bios7_gba[] = {\n");
               for(i=0;i<pFile->Size();){
                   for(i1=0;i1<16 && i < pFile->Size();i1++,i++)
                       fprintf(fp,"0x%02X,",(int)p[i]);
                   fprintf(fp,"\n");
               }
           fprintf(fp,"};\n#endif\n");
           fclose(fp);
       }
       if(type == 11){
           fp = fopen("h:\\ideas\\bios9.h","wb");
           fprintf(fp,"#ifndef __BIOS9H__\n");
           fprintf(fp,"#define __BIOS9H__\n\n");
           fprintf(fp,"const unsigned char my_bios9[] = {\n");
               for(i=0;i<pFile->Size();){
                   for(i1=0;i1<16 && i < pFile->Size();i1++,i++)
                       fprintf(fp,"0x%02X,",(int)p[i]);
                   fprintf(fp,"\n");
               }
           fprintf(fp,"};\n#endif\n");
           fclose(fp);
       }
       if(type == 13){
           fp = fopen("/mnt/win_e/ideas/shaders.h","wb");
           fprintf(fp,"#ifndef __SHADERSH__\n");
           fprintf(fp,"#define __SHADERSH__\n\n");
           fprintf(fp,"const unsigned char vertex_shaders[] = {\n");
               for(i=0;i<pFile->Size();){
                   for(i1=0;i1<16 && i < pFile->Size();i1++,i++){
                       pFile->Read(&c,1);
                       fprintf(fp,"0x%02X,",(int)c^25);
                   }
                   fprintf(fp,"\n");
               }
           fprintf(fp,"};\n\n");
           delete pFile;
           pFile = new LFile("/mnt/win_e/ideas/fsh.c");
           pFile->Open();
           fprintf(fp,"const unsigned char fragment_shaders[] = {\n");
               for(i=0;i<pFile->Size();){
                   for(i1=0;i1<16 && i < pFile->Size();i1++,i++){
                       pFile->Read(&c,1);
                       fprintf(fp,"0x%02X,",(int)c^25);
                   }
                   fprintf(fp,"\n");
               }
               
           fprintf(fp,"};\n\n#endif\n");
           fclose(fp);
       }
#endif
       delete pFile;
       return TRUE;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL LMMU::Open(const char *name,int slot)
{
	LStream *pf;
	int i,res,n;
	LString s;
   COMPRESSEDFILEINFO p;
	char *p1;
#ifdef _DEBPRO
	const char ext[] ={".pme\0.ds.gba\0.sc.nds\0.nds\0.srl\0.bin\0.elf\0.gba\0\0"};
#else
	const char ext[] ={".pme\0.ds.gba\0.sc.nds\0.nds\0.bin\0.elf\0.gba\0\0"};
#endif
	DWORD dw;

	pf = NULL;
   res = -1;
   s = LString(name).LowerCase();
   if(s.Pos(".zip") > 0 || s.Pos(".7z") > 0){
       if(s.Pos(".zip") > 0)
   	    pf = new LZipFile(name);
       else if(s.Pos(".7z") > 0)
           pf = new L7ZFile(name);
       if(pf == NULL || !pf->Open(GENERIC_READ,OPEN_EXISTING))
      	    goto ex_LRomReader_Open;
   	for(i=0;i < ((LCompressedFile *)pf)->Count();i++){
       	((LCompressedFile *)pf)->get_FileCompressedInfo(i+1,&p);
       	s = LString(p.fileName).LowerCase();
       	p1 = (char *)ext;
       	while(*p1 != 0){
           	if((n = s.Pos(p1)) > 0)
               	break;
           	p1 += lstrlen(p1) + 1;
       	}
       	if(n > 0){
           	if(s.Length() == n + lstrlen(p1) - 1){
                   upLcd.set_StatusLed(3);
                   if(nUseTempFile == AUTO){
                   	if(p.dwSize >= 4*1024*1024)
                       	dw = TEMP;
                       else
                       	dw = NORMAL;
                   }
                   else
                   	dw = nUseTempFile;
               	if(((LCompressedFile *)pf)->OpenCompressedFile((WORD)(i+1),(COMPRESSEDMODE)dw) == 0)
                   	goto ex_LRomReader_Open;
               	else if(s.Pos(".pme") > 0 || s.Pos(".ds.gba") > 0 || s.Pos(".sc.nds") > 0 || s.Pos(".gba") > 0)
                   	res = 2;
               	else if(s.Pos(".nds") > 0)
                   	res = 1;
               	else if(s.Pos(".bin") > 0)
                   	res = 3;
					else if(s.Pos(".elf") > 0)
       				res = 4;
               	break;
           	}
       	}
   	}
   }
   else{
   	if(s.Pos(".pme") > 0 || s.Pos(".ds.gba") > 0 || s.Pos(".sc.nds") > 0 || s.Pos(".gba") > 0)
       	res = 2;
   	else if(s.Pos(".bin") > 0)
       	res = 3;
   	else if(s.Pos(".nds") > 0)
       	res = 1;
		else if(s.Pos(".elf") > 0)
       	res = 4;
#ifdef _DEBPRO
		else if(s.Pos(".srl") > 0)
       	res = 1;
#endif
   }
ex_LRomReader_Open:
   upLcd.set_StatusLed(0);
   if(res == -1){
   	if(pf != NULL)
       	pf->Release();
       pf = NULL;
       return FALSE;
	}
	CloseRom(slot);
	switch(res){
   	case 1:
       	pReader[slot] = new LNDSReader();
           if(pf == NULL)
   	    	pf = new LFile(name);
       break;
		case 2:
       	pReader[slot] = new LGBAReader();
           if(pf == NULL)
   	    	pf = new LFile(name);
       break;
       case 3:
       	pReader[slot] = new LBINReader();
           if(pf == NULL)
   	    	pf = new LFile(name);
       break;
       case 4:
       	pReader[slot] = new LElfReader();
           if(pf == NULL)
   	    	pf = new LElfFile(name);
       break;
       default:
       	pReader[slot] = NULL;
       break;
   }
   if(pReader[slot] == NULL){
   	if(pf != NULL)
       	pf->Release();
       pf = NULL;
   	return FALSE;
   }
  	if(pf != NULL){
   	if(pf->Open(GENERIC_READ,OPEN_EXISTING)){
           pReader[slot]->set_Stream(pf);
           pReader[slot]->Preload();
           pReader[slot]->get_Type(s.c_str());
           return TRUE;
       }
       pf->Release();
   }
	return FALSE;
}
//---------------------------------------------------------------------------
DWORD LMMU::Load(const char *name,int slot)
{
	DWORD res,dw,dw1;

   if(pReader[slot] == NULL)
       return 0;
   res = 0;
   if(slot == 0){
       switch(pReader[slot]->get_Type()){
           default:
               dw = 4 * 1024 * 1024;
               dw1 = 0;
           break;
           case 4://DSI
               dw = 16 * 1024 * 1024;
               dw1 = 756*1024;
           break;
       }
       if(dw != dwMMSize){
           ulMMMask = dw - 1;
           if(ext_mem != NULL)
               LocalFree(ext_mem);
           dwMMSize = dw + dw1;
           ext_mem = (u8 *)LocalAlloc(LMEM_FIXED,dwMMSize);
           if(ext_mem == NULL)
               return res;
           if(dw1)
               int_mem3 = ext_mem + dw;
           else
               int_mem3 = NULL;
       }
       ZeroMemory(ext_mem,dwMMSize-0x37F);
   }
   res = pReader[slot]->Load(name);
   return res;
}
//---------------------------------------------------------------------------
u32 LMMU::SeekCard(u32 ofs)
{
   dwIndexCache = 0;
   if(get_EmuMode() != EMUM_GBA && ofs < 0x8000){
#ifdef _DEBPRO
       WriteMsgConsolle(NULL,"%cCard Memory Seek Error 0x%08X",MSGT_CARD,ofs);
#endif
		ofs = 0x8000 + (ofs & 0x1FF);
   }
   if(pReader[0]->Seek(ofs) == (DWORD)-1)
   	return (dwDataReady = 0);
   return (dwDataReady = 0x80);
}
//---------------------------------------------------------------------------
u32 LMMU::ReadCard()
{
   if(dwIndexCache == 0){
       DWORD dw;
//       if(dwDataReady != 0x81)
//           FillMemory(diskCache,0x200,0xFF);
       dw = pReader[0]->Read(diskCache,dwDataReady<<2);
       dwIndexCache = dw >> 2;
       if(dwIndexCache == 0)
           return 0;
//       if(dw != dwDataReady << 2)
//           ZeroMemory(&diskCache[dw],(dwDataReady<<2) - dw);
   }

   return (u32)diskCache[dwDataReady - dwIndexCache--];
}
//---------------------------------------------------------------------------
void write_ram_block_word(u32 address,u32 data)
{
   if(ulExRamPack & 0x80000000){
       if(rom_pack[(address>>16)&0x1FF] == NULL)
           rom_pack[(address>>16)&0x1FF] = (u8 *)LocalAlloc(LPTR,0x10000);
       *((u32 *)(rom_pack[(address>>16)&0x1FF] + (address & 0xFFFC))) = data;
   }
}
//---------------------------------------------------------------------------
void write_ram_block_hword(u32 address,u32 data)
{
   switch((u8)ulExRamPack){
       case 1:
           if(address == 0x09FFFFFE){
               if(data == 5)
                   ulExRamPack |= 0x80000000;
               else if(data == 3)
                   ulExRamPack &= ~0x80000000;
           }
       break;
       default:
           return;
   }
   if(ulExRamPack & 0x80000000){
       if(rom_pack[(address>>16)&0x1FF] == NULL)
           rom_pack[(address>>16)&0x1FF] = (u8 *)LocalAlloc(LPTR,0x10000);
       *((u16 *)(rom_pack[(address>>16)&0x1FF] + (address & 0xFFFE))) = (u16)data;
   }
}
//---------------------------------------------------------------------------
void write_ram_block_byte(u32 address,u32 data)
{
   if(ulExRamPack & 0x80000000){
       if(rom_pack[(address>>16)&0x1FF] == NULL)
           rom_pack[(address>>16)&0x1FF] = (u8 *)LocalAlloc(LPTR,0x10000);
       rom_pack[(address>>16)&0x1FF][(u16)address] = (u8)data;
   }
}
//---------------------------------------------------------------------------
u32 I_FASTCALL read_byte(u32 adress)
{
	switch((adress >> 24)){
   	case 0:
           return bios7_mem[adress & 0x3fff];
       case 2:
       	if(ds.get_EmuMode() == 0)
               return (u32)ext_mem[adress & ulMMMask];
           return (u32)ext_mem[adress & 0x3FFFF];
       case 3:
       	if((adress & 0x800000) && !ds.get_EmuMode())
           	return (u32)int_mem2[(u16)adress];
       	return (u32)int_mem[adress & 0x7FFF];
       case 4:
           if((adress & 0x00100000))
           	adress = 0x2000 + (u8)adress;
           else if((adress & 0x00004000))
           	adress = 0x2800 + (adress & 0x3FF);
           else
           	adress &= 0x1FFF;
           if(crO[adress] != NULL)
               return crO[adress](adress,AMM_BYTE);
           return (u32)crIO_mem[adress];
       case 5:
			return (u32)pal_mem[adress & 0x7FF];
		case 6:
           return (u32)video_mem[((VRAMmap[(adress>>14)&0x3FF]<<14)+(adress&0x3FFF))];
       case 7:
       	return (u32)obj_mem[adress & 0x7FF];
		case 8:
       case 9:
           if(rom_pack[(adress>>16)&0x1FF] == NULL)
               return (u32)0;
       	return (u32)(rom_pack[(adress>>16)&0x1FF][(u16)adress]);
       case 0xA:
           switch((u8)ulExRamPack){
               case 2:
                   if(adress == 0x0A000000)
                       return (u8)ds.get_GripKeys();
               break;
           }
           return sram_mem[(u16)adress];
       case 0xE:
       	if(ds.get_EmuMode() == EMUM_GBA)
       		return ReadSRAM(adress);
           return 0;
       default:
			return (u32)*((u8 *)(bios9_mem + (adress & 0x7FFF)));
   }
}
//---------------------------------------------------------------------------
u32 I_FASTCALL read_hword(u32 adress)
{
	switch(adress >> 24){
   	case 0:
			return (u16)*((u16 *)(bios7_mem + (adress & 0x3FFE)));
       case 2:
       	if(ds.get_EmuMode() == 0)
               return (u32)*((u16 *)(ext_mem + (adress & (ulMMMask & ~1))));
          	return (u32)*((u16 *)(ext_mem + (adress & 0x3FFFE)));
       case 3:
       	if((adress & 0x800000) && !ds.get_EmuMode())
			    return (u32)*((u16 *)(int_mem2 + (adress & 0xFFFE)));
			return (u32)*((u16 *)(int_mem + (adress & 0x7FFE)));
       case 4:
           if((adress & 0x00100000))
           	adress = 0x2000 + (adress & 0xFE);
           else if((adress & 0x00004000))
           	adress = 0x2800 + (adress & 0x3FE);
           else if((adress & 0x00800000))
           	adress = 0x3000 + (adress & 0x7FFE);
           else
           	adress &= 0x1FFE;
           if(crO[adress] != NULL)
               return (u32)(u16)crO[adress](adress,AMM_HWORD);
           return (u32)*(u16 *)(crIO_mem + adress);
       case 5:
			return (u32)*((u16 *)(pal_mem + (adress & 0x7FE)));
		case 6:
           return (u32)*((u16 *)(video_mem + ((VRAMmap[(adress>>14)&0x3FF]<<14)+(adress&0x3FFE))));
       case 7:
			return (u32)*((u16 *)(obj_mem + (adress & 0x7FE)));
		case 8:
       case 9:
           switch((u8)ulExRamPack){
               case 2:
                   if(adress < 0x080000C2 || adress == 0x0801FFFE)
                       return 0xF9FF;
               break;
           }
           if(rom_pack[(adress>>16)&0x1FF] == NULL)
               return (u32)0;
       	return (u32)*((u16 *)(rom_pack[(adress>>16)&0x1FF] + (adress & 0xFFFE)));
		case 0xA:
       case 0xB:
       	if(ds.get_RomType() == 10)
           	return ReadRomPack(0,adress,AMM_HWORD);
       	return 0;
       case 0xC:
       case 0xD:
       	if(ds.get_RomType() == 10)
           	return ReadRomPack(1,adress,AMM_HWORD);
           return 0;
       default:
			return (u32)*((u16 *)(bios9_mem + (adress & 0x7FFE)));
   }
}
//---------------------------------------------------------------------------
u32 I_FASTCALL read_word(u32 adress)
{
	switch((adress >> 24)){
   	case 0:
       	return (u32)*((u32 *)(bios7_mem + (adress & 0x3FFC)));
		case 2:
       	if(ds.get_EmuMode() == 0)
               return (u32)*((u32 *)(ext_mem + (adress & (ulMMMask & ~3))));
           return (u32)*((u32 *)(ext_mem + (adress & 0x3FFFC)));
       case 3:
       	if((adress & 0x800000) && !ds.get_EmuMode())
			    return (u32)*((u32 *)(int_mem2 + (adress & 0xFFFC)));
			return (u32)*((u32 *)(int_mem + (adress & 0x7FFC)));
		case 4:
           if((adress & 0x00100000))
           	adress = 0x2000 + (adress & 0xFC);
           else if((adress & 0x00004000))
           	adress = 0x2800 + (adress & 0x3FC);
           else
				adress &= 0x1FFC;
           if(crO[adress] != NULL)
               return crO[adress](adress,AMM_WORD);
           return *(u32 *)(crIO_mem + adress);
       case 5:
			return (u32)*((u32 *)(pal_mem + (adress & 0x7FC)));
		case 6:
           return (u32)*((u32 *)(video_mem + ((VRAMmap[(adress>>14)&0x3FF]<<14)+(adress&0x3FFC))));
       case 7:
			return (u32)*((u32 *)(obj_mem + (adress & 0x7FC)));
		case 8:
       case 9:
           if(rom_pack[(adress>>16)&0x1FF] == NULL)
               return (u32)0x0;
       	return (*((u32 *)(rom_pack[(adress>>16)&0x1FF] + (adress & 0xFFFC))));
       default:
			return (u32)*((u32 *)(bios9_mem + (adress & 0x7FFC)));
   }
}
//---------------------------------------------------------------------------
void I_FASTCALL write_byte(u32 adress,u32 data)
{
	switch((adress >> 24)){
   	case 0:
           data = data;
       break;
       case 2:
           if(ds.get_EmuMode() == 0)
               ext_mem[adress & ulMMMask] = (u8)data;
           else
           	ext_mem[adress & 0x3FFFF] = (u8)data;
       break;
       case 3:
       	if((adress & 0x800000) && !ds.get_EmuMode()){
           	int_mem2[(u16)adress] = (u8)data;
               return;
           }
       	int_mem[adress&0x7FFF] = (u8)data;
       break;
       case 4:
           if((adress & 0x00100000))
           	adress = 0x2000 + (u8)adress;
           else if((adress & 0x00004000))
           	adress = 0x2800 + (adress & 0x3FF);
           else if((adress & 0x00800000))
           	adress = 0x3000 + (adress & 0x7FFF);
           else
				adress &= 0x1FFF;
           crIO_mem[adress] = (u8)data;
           crI[adress](adress,data,AMM_BYTE);
       break;
       case 5:
       	pal_mem[adress & 0x7FF] = (u8)data;
			pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_PALETTECHANGED,PIS_NOTIFYMASK,(LPARAM)(adress<<16)|AMM_BYTE);
       break;
       case 6:
           *(video_mem + ((VRAMmap[(adress>>14)&0x3FF]<<14)+(adress&0x3FFF))) = (u8)data;
       break;
       case 7:
       	obj_mem[adress & 0x7FF] = (u8)data;
			pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_OAMCHANGED,PIS_NOTIFYMASK,(LPARAM)(adress<<16)|AMM_BYTE);
       break;
       case 8:
       case 9:
           write_ram_block_byte(adress,data);
       break;
       case 0xA:
           *(sram_mem + (u16)adress) = (u8)data;
       break;
       case 0xE:
       	if(ds.get_EmuMode() == EMUM_GBA)
       		WriteSRAM(adress,(u8)data);
		break;
       default:
       	data = data;
		break;
   }
}
//---------------------------------------------------------------------------
void I_FASTCALL write_hword(u32 adress,u32 data)
{
#if defined(_DEBPRO2)
   if(debugDlg.is_Active())
	    ControlMemoryBP(adress,MAKELONG(AMM_WORD|AMM_WRITE,9),data);
#endif
	switch((adress >> 24)){
   	case 0:
           data = data;
       break;
		case 2:
       	if(ds.get_EmuMode() == 0)
               *((u16 *)(ext_mem + (adress & (ulMMMask & ~1)))) = (u16)data;
           else
           	*((u16 *)(ext_mem + (adress & 0x3FFFE))) = (u16)data;
       break;
		case 3:
       	if((adress & 0x800000) && !ds.get_EmuMode()){
               *((u16 *)(int_mem2 + (adress & 0xFFFE))) = (u16)data;
               return;
           }
       	*((u16 *)(int_mem + (adress & 0x7FFE))) = (u16)data;
       break;
       case 4:
           if((adress & 0x00100000))
           	adress = 0x2000 + (adress & 0xFE);
           else if((adress & 0x00004000))
           	adress = 0x2800 + (adress & 0x3FE);
           else if((adress & 0x00800000))
           	adress = 0x3000 + (adress & 0x7FFE);
           else
				adress &= 0x1FFE;
  		    *((u16 *)(crIO_mem + adress)) = (u16)data;
           crI[adress](adress,data,AMM_HWORD);
       break;
       case 5:
			*((u16 *)(pal_mem + (adress &= 0x7FE))) = (u16)data;
			pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_PALETTECHANGED,PIS_NOTIFYMASK,(LPARAM)(adress<<16)|AMM_HWORD);
       break;
       case 6:
#ifdef _DEBPRO
//           if((adress & 0x0FF00000) == 0x06800000)
//               fprintf(fpLog,"%08X\n",adress);
#endif
           if(adress & 0x800000){
               u16 *p;

               p = ((u16 *)(video_mem + ((VRAMmap[(adress>>14) & 0x3FF]<<14) + (adress & 0x3FFE))));
               if(*p != (u16)data){
                   *p = (u16)data;
                   adress &= 0xFFFFE;
                   video_cache[adress >> 5] |= (0x3 << (adress & 0x1F));
               }
           }
           else
               *((u16 *)(video_mem + ((VRAMmap[(adress>>14)&0x3FF]<<14)+(adress&0x3FFE)))) = (u16)data;
       break;
       case 7:
			*((u16 *)(obj_mem + (adress & 0x7FE))) = (u16)data;
			pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_OAMCHANGED,PIS_NOTIFYMASK,(LPARAM)(adress<<16)|AMM_HWORD);
//           upLcd.WriteSprite((u16)adress,AMM_HWORD);
//           downLcd.WriteSprite((u16)adress,AMM_HWORD);
       break;
       case 8:
       case 9:
           write_ram_block_hword(adress,data);
       break;
       case 0xA:
       case 0xB:
       	if(ds.get_EmuMode() == EMUM_GBA)
				WriteRomPack(0,adress,data,AMM_HWORD);
       break;
       case 0xC:
		case 0xD:
       	if(ds.get_EmuMode() == EMUM_GBA)
       		WriteRomPack(1,adress,data,AMM_HWORD);
       break;
       default:
       	data = data;
		break;
   }
}
//---------------------------------------------------------------------------
void I_FASTCALL write_word(u32 adress,u32 data)
{
#if defined(_DEBPRO2)
   if(debugDlg.is_Active())
	    ControlMemoryBP(adress,MAKELONG(AMM_WORD|AMM_WRITE,9),data);
#endif
	switch((adress >> 24)){
   	case 0:
           data = data;
       break;
		case 2:
       	if(ds.get_EmuMode() == 0){
               if(adress == 0x027FFE24)
                   ds.StartArm9(data);
               *((u32 *)(ext_mem + (adress & (ulMMMask & ~3)))) = data;
           }
           else
				*((u32 *)(ext_mem + (adress & 0x3FFFC))) = data;
       break;
		case 3:
       	if((adress & 0x800000) && !ds.get_EmuMode()){
           	*((u32 *)(int_mem2 + (adress & 0xFFFC))) = data;
               return;
           }
       	*((u32 *)(int_mem + (adress & 0x7FFC))) = data;
       break;
       case 4:
           if((adress & 0x00100000))
           	adress = 0x2000 + (adress & 0xFC);
           else if(adress & 0x00004000)
           	adress = 0x2800 + (adress & 0x3FC);
           else if((adress & 0x00800000))
           	adress = 0x3000 + (adress & 0x7FFC);
           else
				adress &= 0x1FFC;
           *((u32 *)(crIO_mem + adress)) = data;
            crI[adress](adress,data,AMM_WORD);
       break;
       case 5:
			*((u32 *)(pal_mem + (adress & 0x7FC))) = data;
			pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_PALETTECHANGED,PIS_NOTIFYMASK,(LPARAM)(adress<<16)|AMM_WORD);
       break;
       case 6:
//           if((adress & 0x0FF00000) == 0x06200000 && data)
//               fprintf(fpLog,"%08X\n",adress);
           if((adress & 0xF00000) == 0x800000){
               u32 *p;
#ifdef _DEBPRO
/*				if(abs(lastWrite - adress) > 4){
               	fprintf(fpLog,"%08X\r\n",adress);
                   fflush(fpLog);
               }
               lastWrite = adress;*/
#endif
               p = ((u32 *)(video_mem + ((VRAMmap[(adress>>14) & 0x3FF] << 14) + (adress & 0x3FFC))));
               if(*p != data){
                   *p = data;
                   adress &= 0xFFFFF;
                   video_cache[adress >> 5] |= (0xF << (adress & 0x1C));
               }
           }
           else
               *((u32 *)(video_mem + ((VRAMmap[(adress>>14) & 0x3FF] << 14) + (adress & 0x3FFC)))) = data;
       break;
       case 7:
			*((u32 *)(obj_mem + (adress & 0x7FC))) = data;
			pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_OAMCHANGED,PIS_NOTIFYMASK,(LPARAM)(adress << 16)|AMM_WORD);
//           upLcd.WriteSprite((u16)adress,AMM_WORD);
//           downLcd.WriteSprite((u16)adress,AMM_WORD);
       break;
       case 8:
       case 9:
           write_ram_block_word(adress,data);
       break;
       default:
       	data = data;
		break;
   }
}
//---------------------------------------------------------------------------
void MMUMainremapVRAM(u8 cntindex,u8 vcnt)
{
	u8 master,offset,a;

	master = (u8)(vcnt&7);
   offset = (u8)((vcnt>>3) & 3);
/*   if(!(vcnt & 0x80)){
       switch(cntindex){
           case 0:
               for(a=0;a<8;a++){
                   VRAMmap[a+512] = 41;
                   VRAMmap[a] = 41;
                   VRAMmap[a+256] = 41;
               }
           break;
           case 1:
               for(a=0;a<8;a++){
                   VRAMmap[a+520]= 41;
                   VRAMmap[a] = 41;
                   VRAMmap[a+256] = 41;
               }
           break;
           case 2:
               for(a=0;a<8;a++){
                   VRAMmap[a+528]= 41;
                   VRAMmap[a] = 41;
                   VRAMmap[a+128] = 41;
               }
           break;
           case 3:
               for(a=0;a<8;a++){
                   VRAMmap[a+536]= 41;
                   VRAMmap[a] = 41;
                   VRAMmap[a+384] = 41;
               }
           break;
           case 4:
              	for(a=0;a<4;a++){
                   VRAMmap[a+544]= 41;
                 	VRAMmap[a] = 41;
                  	VRAMmap[256+a] = 41;
               }
           break;
           case 5:
               VRAMmap[548]=41;
				VRAMmap[0]=41;
				VRAMmap[256]=41;
           break;
           case 6://G
               VRAMmap[549]=41;
				VRAMmap[0]=41;
				VRAMmap[256]=41;
           break;
           case 8:
               for(a=0;a<2;a++){
                   VRAMmap[a+550]= 41;
                   VRAMmap[a] = 41;
                   VRAMmap[a+128] = 41;
               }
           break;
           case 9:
             	VRAMmap[552]= 41;
               VRAMmap[130] = 41;
               VRAMmap[384] = 41;
               VRAMmap[40] = 41;
           break;
       }
   }*/
   switch(cntindex){
   	case 0://A
       	switch(master){
            	case 0:
                	if(!offset) {
                    	for(a=0;a<8;a++)
                           VRAMmap[a+512]=a;
                	}
           	break;  //800000
            	case 1:
                  	for(a=0;a<8;a++)
                      	VRAMmap[a + (offset << 3)] = a;
               break;
            	case 2:
                  	for(a=0;a<8;a++)
                      	VRAMmap[a+256+(offset << 3)] = a;
               break;
               case 3:
               	for(a=0;a<8;a++){
                   	if(VRAMmap[a+512] == a)
                   		VRAMmap[a+512] = (u8)41;
/*                       if(VRAMmap[a] == a)
                       	VRAMmap[a] = 41;
                       if(VRAMmap[a+256] == a)
                       	VRAMmap[a+256] = 41;*/
                   }
               break;
           }
       break;
       case 1://B
        	switch(master){
            	case 0:
                	if(!offset) {
                        for(a=0;a<8;a++)
                            VRAMmap[a+520]=(u8)(a+8);
                	}
               break;
            	case 1:
                  	for(a=0;a<8;a++)
                      	VRAMmap[a+(offset << 3)] = (u8)(a+8);
               break;
            	case 2:
                  	for(a=0;a<8;a++)
                      	VRAMmap[a+256+(offset << 3)] = (u8)(a+8);
               break;
               case 3:
               	for(a=0;a<8;a++){
                   	if(VRAMmap[a+520] == (a+8))
                   		VRAMmap[a+520]=(u8)41;
/*                       if(VRAMmap[a] == (a+8))
                       	VRAMmap[a] = 41;
                       if(VRAMmap[a+256] == (a+8))
                       	VRAMmap[a+256] = 41;*/
                   }
               break;

        	}
       break;
       case 2://C
          	io_mem7[0x240] &= ~1;
       	switch(master){
          		case 0:
              		if(!offset) {
                  		for(a=0;a<8;a++){
                           VRAMmap[a+528]=(u8)(a+16);
                       }
              		}
               break;
         		case 1:
                  	for(a=0;a<8;a++)
                      	VRAMmap[a+(offset << 3)] = (u8)(a+16);
               break;
               case 2:
               	io_mem7[0x240] |= 1;
                   offset = (u8)((offset & 1) << 3);
                  	for(a=0;a<8;a++)
                      	VRAMmap[560+a+offset] = (u8)(a+16);
               break;
            	case 4://sub title
                  	for(a=0;a<8;a++){
                       VRAMmap[a+128] = (u8)(a+16);
                       //1036 beta fire emblem
                       VRAMmap[a+136] = (u8)(a+16);
//                       VRAMmap[a+136] = (u8)(a+16);
						if(VRAMmap[a+528] == a+16)
                   		VRAMmap[a+528]=(u8)41;
                   }
               break;
      		}
       break;
       case 3://D
           io_mem7[0x240] &=~2;
        	switch(master){
          		case 0:
					if(!offset){
                       for(a=0;a<8;a++)
                           VRAMmap[a+536]=(u8)(a+24);
              		}
               break;
          		case 1:
                  	for(a=0;a<8;a++)
                      	VRAMmap[a+(offset << 3)] = (u8)(a+24);
           	break;
               case 2:
               	io_mem7[0x240] |= 2;
                   offset = (u8)((offset & 1) << 3);
                  	for(a=0;a<8;a++)
                      	VRAMmap[560+a+offset] = (u8)(a+24);
               break;
               case 3:
               	for(a=0;a<8;a++){
                   	if(VRAMmap[a+536] == a+24)
                   		VRAMmap[a+536]=(u8)41;
                       if(VRAMmap[a] == a+24)
                       	VRAMmap[a] = 41;
                       if(VRAMmap[a+384] == a+24)
                       	VRAMmap[a+384] = 41;
                       if(VRAMmap[a+560] == a+24)
                       	VRAMmap[a+560] = 41;
                   }
               break;
            	case 4:  //sub sprite
               	for(a=0;a<8;a++){
                       VRAMmap[a+384]=(u8)(a+24);
                       if(VRAMmap[a+536] == a+24)
                           VRAMmap[a+536]=(u8)41;
                       if(VRAMmap[a] == a+24)
                           VRAMmap[a] = 41;
                       if(VRAMmap[a+560] == a+24)
                           VRAMmap[a+560] = 41;
                   }
               break;
           }
       break;
		case 4://E
       	switch(master){
           	case 0:
                  	for(a=0;a<4;a++){
                       VRAMmap[a+544]=(u8)(a+32);
                   }
               break;
               case 1:
                   for(a=0;a<4;a++){
                   	VRAMmap[a] = (u8)(a+32);
                       if(VRAMmap[a+4] < 32)
                       	VRAMmap[a+4] = (u8)41;
                   }
               break;
               case 2: //main sprite
               	for(a=0;a<4;a++)
                   	VRAMmap[256+a]=(u8)(a+32);
               break;
               case 3:
               	for(a=0;a<4;a++){
                   	if(VRAMmap[a+544] == a+32)
                   		VRAMmap[a+544]=(u8)41;
                       if(VRAMmap[a] == a+32)
                       	VRAMmap[a] = 41;
                       if(VRAMmap[a+256] == a+32)
                       	VRAMmap[a+256] = 41;
                   }
               break;
           }
       break;
       case 5://F
       	switch(master){
               case 0:
                   VRAMmap[548]=36;
               break;
               case 1:
                   VRAMmap[((offset & 2) << 1) + (offset & 1)] = 36;
               break;
               case 2:
                   VRAMmap[256] = 36;
               break;
               case 3:
                   if(VRAMmap[548] == 36)
                       VRAMmap[548]=(u8)41;
                   if(VRAMmap[256] == 36)
                       VRAMmap[256] = 41;
               break;
           }
       break;
       case 6://G
       	switch(master){
           	case 0:
                  	VRAMmap[549]=37;
               break;
               case 1:
                   for(a=0;a<8;a++)
                       VRAMmap[a] = 41;
					VRAMmap[((offset & 2) << 1) + (offset & 1)] = 37;
               break;
               case 2:
					VRAMmap[256+(offset & 1)]=37;
               break;
               case 3:
                   if(VRAMmap[549] == 37)
                       VRAMmap[549]=(u8)41;
                   if(VRAMmap[256] == 37)
                       VRAMmap[256] = 41;
               break;
           }
       break;
       case 8://H
       	switch(master){
           	case 0:
                  	for(a=0;a<2;a++)
                      	VRAMmap[a+550]=(u8)(a+38);
               break;
               case 1:
					for(a=0;a<2;a++){
                       VRAMmap[128+a]=(u8)(a+38);
//                   	if(VRAMmap[a+550] == a+38)
//                   		VRAMmap[a+550] = (u8)41;
                   }
               break;
           }
       break;
       case 9://I
			switch(master){
           	case 0:
                  	VRAMmap[552]=40;
               break;
               case 1:
               	VRAMmap[130] = 40;
               break;
               case 2:
               	VRAMmap[384] = 40;
               break;
               case 3:
                   VRAMmap[40] = 40;
               break;
           }
       break;
   }
}               









