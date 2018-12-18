#include "ideastypes.h"

#ifndef __DSTYPEH__
#define __DSTYPEH__

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
#if defined(__BORLANDC__) || defined(__WATCOMC__)
typedef signed __int64 s64;
typedef unsigned __int64 u64;
#else
typedef signed long long int s64;
typedef unsigned long long int u64;
#endif
//---------------------------------------------------------------------------
#ifndef AMM_ALL
   #define AMM_BYTE            1
   #define AMM_HWORD           2
   #define AMM_WORD            8
   #define AMM_READ            0x10
   #define AMM_WRITE           0x20
   #define AMM_CNT				0x40
   #define AMM_ALL             255
   #define AMM_FIFO            (0xF0|AMM_WORD)
#endif
//---------------------------------------------------------------------------
#ifndef I_FASTCALL
   #ifdef __BORLANDC__
       #define I_FASTCALL __fastcall
   #elif defined(__WATCOMC__)
       #define I_FASTCALL __watcall
   #else
       #define I_FASTCALL __attribute__ ((regparm(2)))
   #endif
#endif

#ifndef I_STDCALL
   #if defined(__BORLANDC__) || defined(__WATCOMC__)
       #define I_STDCALL __stdcall
   #else
       #define I_STDCALL __attribute__ ((stdcall))
   #endif
#endif

#ifndef NO_IN_LINE
	#ifdef __GNUC__
		#define NO_IN_LINE __attribute__ ((noinline))
	#else
		#define NO_IN_LINE
	#endif
#endif
//---------------------------------------------------------------------------
typedef struct
{
   char Name[30];
   DWORD Address;
   DWORD Size;
   BYTE  AccessMode;
   WORD  idMemoryInfo;
   BYTE  bFlags;
   DWORD vSize;
   u32 (*pfn_read)(u32,u8);
   void (*pfn_write)(u32,u32,u8);
} MEMORYINFO, *LPMEMORYINFO;
//---------------------------------------------------------------------------
typedef struct
{
   u16 control;
   u16 data;
   u32 buffer[16];
   u8 index;
} PXI,*LPPXI;
//---------------------------------------------------------------------------
typedef struct
{
   u8 command[8];
   u32 control;
   s16 cycles,ofs;
   u8 bEnable,bIrq,mode;
   u32 dataOut;
   u32 dataRead;
   u8 keybuf[0x1048];
   u32 keycode[4];
   u32 command_count;
} CARD,*LPCARD;
//---------------------------------------------------------------------------
typedef struct
{
   u32 control,mode;
   u32 adr,adrMax;
   u8 status;
   u8 buffer[1024*1024];
   u8 adrSize;
   WORD id;
   u16 index;
   u32 sizeMask;
} EEPROMCARD,*LPEEPROMCARD;
//---------------------------------------------------------------------------
typedef struct
{
   u16 control;
   u8 reg;
   u16 dataOut;
   u8 regValue[8];
} POWERMNG,*LPPOWERMNG;
//---------------------------------------------------------------------------
typedef struct
{
   u16 command;
   u16 control;
   s16 dataIn;
   u32 dataOut;
   u32 dataIdx;
   u32 address;
   u16 status;
} FIRMWARE,*LPFIRMWARE;
//---------------------------------------------------------------------------
typedef struct{
   u8 reg[4];
   u8 command;
   int dataLen;
   int bits;
   int state;
   u8 data[12];
   u8 dataOut;
} RTC,*LPRTC;
//---------------------------------------------------------------------------
typedef struct {
   u8 Index;
   u8 Enable;
   u16 Count;
   s32 Value;
   u16 ResetValue;
   u16 Diff,Remainder;
   u16 Control;
   u16 Freq;
   u8 logFreq;
   u8 Irq;
   u8 Cascade;
   u8 Inc;
} TIMER,*PTIMER,*LPTIMER;
//---------------------------------------------------------------------------
typedef struct{
   u8 Index;
   u32 Source;
   u32 Dest;
   u32 Control;
   u32 Dst,Src;
   u32 MaxCount;
   u32 InternalCount;
   u8 Enable;
   u8 Start;
   u8 Repeat;
   u8 Reload;
   u8 Irq;
   s8 IncS,IncD;
   u8 Mode;
   u32 Count;
   u32 latency;
} DMA,*PDMA,*LPDMA;
//---------------------------------------------------------------------------
typedef struct _touch
{
   LONG x,y;
   DWORD dwTick;
   u16 control;
   u16 dataIn;
   u16 dataOut;
   u8 dataIdx,mode;
   u16 command;
   s32 xscale,yscale,xoffset,yoffset;
} TOUCH,*LPTOUCH;
//---------------------------------------------------------------------------
typedef BOOL WINAPI (*LPFNCSAVEDLG)(LPOPENFILENAME);
typedef void (*LPIFUNC)(u32,u32,u8);
typedef u32 (*LPOFUNC)(u32,u8);
//---------------------------------------------------------------------------
#endif



