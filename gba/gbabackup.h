#include "gbatypes.h"

#ifndef __GBABACKUPH__
#define __GBABACKUPH__

u32 WriteRomPack(int nPack,u32 adress,u32 data,u8 mode);
u32 ReadRomPack(int nPack,u32 adress,u8 mode);
int FileFlashROM(BOOL bRead);
BOOL FileSRAM(BOOL bRead);
u8 ReadSRAM(u32 adress);
void WriteSRAM(u32 adress,u8 data);
void ResetGBABackup();
void FreeRomPack(u8 pack);
void SetGamePackID(WORD wID);
WORD GetGamePackID();

#endif
