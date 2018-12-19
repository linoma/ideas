#include "ideastypes.h"
#include "dstype.h"

#ifndef __GBATYPESH__
#define __GBATYPESH__

typedef struct{
   u16 ID;
   u16 addrTest;
   u8 mode;
} SRAMID,*LPSRAMID;
//---------------------------------------------------------------------------
typedef struct{
   u8 *buffer;
   u32 size;
   u32 com;
   u16 blocco;
   u8 mode;
   u32 mask;
} SRAM,*LPSRAM;
//---------------------------------------------------------------------------
typedef struct{
   u8 *buffer;
   u32 size;
} PACKROM,*LPPACKROM;
//---------------------------------------------------------------------------
typedef struct{
   u32 com;
   u16 blocco;
   u8 mode;
   u32 byteIndex;
   u8 bitIndex,bytesWrite,sizeCommand,isUsed,bitCommand;
   PACKROM rom_pack[0x200];
} CNTPACKROM,*LPCNTPACKROM;

#endif