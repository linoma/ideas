#include "ideastypes.h"
#include "dstype.h"
#include "fstream.h"
#include "lstring.h"
#include "fat.h"
#include "ndsreader.h"

//---------------------------------------------------------------------------
#ifndef __DSMEMORYH__
#define __DSMEMORYH__
//---------------------------------------------------------------------------

#ifndef PT_SAVEGAME

#define PT_SAVEGAME		1
#define PT_SAVESTATE		2
#define PT_SCREENSHOT		3
#define PT_CHEAT			4
#define PT_GBA_SG_EEPROM	5
#define PT_GBA_SG_FLASH	6

#endif

//---------------------------------------------------------------------------
extern u8 *int_mem;
extern u8 *int_mem2;
extern u8 *int_mem3;
extern u8 *ext_mem;
extern u8 *ext_mem2;                              
extern u8 *io_mem;
extern u8 *io_mem2;
extern u8 *io_mem7;
extern u8 *pal_mem;
extern u8 *obj_mem;
extern u8 *video_mem;
extern u8 *fw_mem;
extern u8 *crIO_mem;
extern u8 *bios7_mem;
extern u8 *bios9_mem;
extern u8 *sram_mem;
extern u32 *video_cache;
extern u8 VRAMmap[1024];
extern u8 mapVRAM[41];
extern u32 VRAMBankSize[10];
extern u32 waitState9[257];
extern u32 waitState7[257];
extern FILE *fpLog;
extern u8 *rom_pack[0x200];
extern u32 ulExRamPack,ulMMMask;
//---------------------------------------------------------------------------

void write_ram_block_word(u32 address,u32 data);
void write_ram_block_hword(u32 address,u32 data);
void write_ram_block_byte(u32 address,u32 data);

extern u32 I_FASTCALL read_byte(u32 adress);
extern u32 I_FASTCALL read_hword(u32 adress);
extern u32 I_FASTCALL read_word(u32 adress);

extern void I_FASTCALL write_byte(u32 adress,u32 data);
extern void I_FASTCALL write_hword(u32 adress,u32 data);
extern void I_FASTCALL write_word(u32 adress,u32 data);

void MMUMainremapVRAM(u8 cntindex, u8 vcnt);

typedef void (*LPIOWRITEFNC)(u32 address,u32 data,u8 accessMode);

class LMMU : public LBase
{
public:
	LMMU();
   virtual ~LMMU();
   void Reset();
   BOOL Init();
   virtual BOOL LoadBios(const char *lpFileName,u8 type);
   u32 ReadCard();
   u32 SeekCard(u32 ofs);
   virtual BOOL Save(LStream *pFile);
   virtual BOOL Load(LStream *pFile,int ver);
   void set_ExpansionRam(u8 i);
   int get_ExpansionRam();
   void ResetCpuSpeed();
   void IncreaseCpuSpeed(int i);

   inline BOOL FatSeek(u32 adr,u8 mode){return fat.Seek(adr,mode);};
   inline u32 FatRead(u32 adr,u8 am){return fat.Read(adr,am);};
   inline BOOL FatWrite(u32 adr,u32 data,u8 am){return fat.Write(adr,data,am);};
	inline IFat *get_FatInterface(){return ((IFat *)&fat);};
	inline LFat *get_Fat(){return &fat;};
   inline LRomReader *get_RomReader(int slot = 0){return pReader[slot];};
   inline const char* get_FileName(){return lastFileName.c_str();};
   BOOL Open(const char *name,int slot = 0);
   DWORD Load(const char *name,int slot = 0);
   void CloseRom(int slot = -1);
   inline const int get_RomType(){if(pReader[0] != NULL) return pReader[0]->get_Type();return 0;};
   BOOL BuildFileName(int type,char *buffer,int maxLength,const char *fileExt = NULL,int ID = -1);
   inline const char *get_SaveGamePath(){return savegamePath.c_str();};
   inline const char *get_SaveStatePath(){return savestatePath.c_str();};
   inline const char *get_SaveScreenshotPath(){return screenshotPath.c_str();};
   inline const char *get_CheatPath(){return cheatPath.c_str();};
   inline const int get_IncreaseSpeed(){return nIncreaseSpeed;};
   inline int get_EmuMode(){return nEmuMode;};
protected:
	DWORD dwAllocated,dwUsed,dwIndexCache,diskCache[0x80],dwDataReady,dwMMSize;
   LString lastFileName,savegamePath,savestatePath,screenshotPath,cheatPath;
   LRomReader *pReader[2];
   LFat fat;
   int nIncreaseSpeed,nUseTempFile,nEmuMode;
   BOOL bAutoIncreaseSpeed;
};


#endif
