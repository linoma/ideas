#include "ideastypes.h"
#include "dsmem.h"
#include "cpu_0.h"
#include "util.h"
#include <math.h>

//---------------------------------------------------------------------------
void BiosBreakPoint(BOOL bArm7)
{
	EnterDebugMode(bArm7);
}
//---------------------------------------------------------------------------
BOOL WriteConsoleMessage(u32 adr_mes,IARM7 *cpu)
{
	int i;
   char *p;
   BOOL res;

   if(adr_mes == 0)
   	return FALSE;
   res = FALSE;
   for(i=0;;i++){
       if(cpu->read_mem(adr_mes+i,AMM_BYTE) == 0)
           break;
   }
   if(i != 0){
       p = (char *)LocalAlloc(LPTR,i+1);
       if(p != NULL){
           for(i=0;;i++){
               p[i] = (char)cpu->read_mem(adr_mes+i,AMM_BYTE);
               if(p[i] == 0)
                   break;
           }
           WriteMsgConsolle(cpu,"%c%s",MSGT_OUTONCONSOLLE,p);
           LocalFree(p);
           res = TRUE;
       }
       else
           MessageBox(ds.Handle(),"Un error during memory allocation.","iDeaS Emulator",MB_ICONERROR|MB_OK);
   }
	return FALSE;
}
//---------------------------------------------------------------------------
u8 GetVolumeTable(u32 value)
{
   if(value < 483)
       return (u8)(value >> 2);
   else if(value < 603)
       return (u8)(0x20 + ((value - 483) * .78f));
   else if(value < 663)
       return (u8)(0x40 + (value - 603));
   return (u8)(0x40 + (value - 663));
}
//---------------------------------------------------------------------------
u16 GetSineTable(u32 value)
{
   return (u16)(sin(value*0.0245219f) * 32768.0f);
}
//---------------------------------------------------------------------------
u16 GetPitchTable(u32 value)
{
   return (u16)((59 + 0.03427f*value) * value);
}
//---------------------------------------------------------------------------
u32 SquareRoot(u32 value)
{
   return (u32)sqrt(value);
}
//---------------------------------------------------------------------------
void HuffUnComp(u32 dst,u32 src,IARM7 *cpu)
{
   u32 r3,tableAddress,value32,adr;
   int count;
   u8 i,i1,prog,max,sl,sr,carry;

   adr = (src &= ~3) + 4;
   tableAddress = adr + 1;
   value32 = cpu->read_mem(src,AMM_WORD);
   count = (int)(value32 >> 8);

   sr = (u8)(value32 & 0xF);
   sl = (u8)(0x20 - sr);
   max = (u8)((sr & 7) + 4);

   src = ((cpu->read_mem(adr,AMM_BYTE) + 1) << 1) + adr;
   adr = tableAddress;
   for(r3 = prog = 0;count > 0;){
       value32 = cpu->read_mem(src,AMM_WORD);
       src += 4;
       for(i = 32;i > 0 && count > 0;i--,value32 <<= 1){
           carry = (u8)(value32 >> 31);
           i1 = (u8)cpu->read_mem(adr,AMM_BYTE);
           adr = (adr & ~1) + (((i1 & 0x3F) + 1) << 1) + carry;
           i1 <<= carry;
           if(!(i1 & 0x80))
               continue;
           r3 = (r3 >> sr) | (cpu->read_mem(adr,AMM_BYTE) << (u8)sl);
           adr = tableAddress;
           prog++;
           if(prog != max)
               continue;
           cpu->write_mem(dst,r3,AMM_WORD);
           dst += 4;
           count -= 4;
           prog = 0;
       }
   }
}
//---------------------------------------------------------------------------
void RLUnComp(u32 dst,u32 src,IARM7 *cpu)
{
   u32 DataHeader;
   int DataSize;
   u8 FlagData,DataLength,b;

   DataHeader = cpu->read_mem(src,AMM_WORD);
   src += 4;
   DataSize = (DataHeader >> 8);
   while(DataSize > 0){
       FlagData = (u8)cpu->read_mem(src++,AMM_BYTE);
       DataLength = (u8)(FlagData & 0x7F);
       if((FlagData & 0x80) != 0){
           DataLength += (u8)3;
           DataSize -= DataLength;
           b = (u8)cpu->read_mem(src++,AMM_BYTE);
           for(;DataLength > 0;DataLength--)
               cpu->write_mem(dst++,b,AMM_BYTE);
       }
       else{
           DataLength += (u8)1;
           DataSize -= DataLength;
           for(;DataLength > 0;DataLength--)
               cpu->write_mem(dst++,cpu->read_mem(src++,AMM_BYTE),AMM_BYTE);
       }
   }
}
//---------------------------------------------------------------------------
void CpuFastSet(u32 dst,u32 src,u32 flags,IARM7 *cpu)
{
   int count;
   u32 i;

   count = (int)((u16)flags);
   switch((flags >> 24) & 1){
       case 0:
           for(;count>0;count--){
               cpu->write_mem(dst,cpu->read_mem(src,AMM_WORD),AMM_WORD);
               dst += 4;
               src += 4;
           }
       break;
       case 1:
           i = cpu->read_mem(src,AMM_WORD);
           for(;count>0;count--){
               cpu->write_mem(dst,i,AMM_WORD);
               dst += 4;
           }
       break;
   }
}
//---------------------------------------------------------------------------
void CpuSet(u32 dst,u32 src,u32 flags,IARM7 *cpu)
{
   u8 DataSize;
   int count;
   u32 i;

   count = (int)((u16)flags);
   if((DataSize = (u8)((flags >> 26) & 1)) != 0){
       dst &= ~3;
       src &= ~3;
   }
   else{
       dst &= ~1;
       src &= ~1;
   }
   switch((flags >> 24) & 1){
       case 0:
           if(DataSize != 0){
               for(;count>0;count--){
                   cpu->write_mem(dst,cpu->read_mem(src,AMM_WORD),AMM_WORD);
                   dst += 4;
                   src += 4;
               }
           }
           else{
               for(;count>0;count--){
                   cpu->write_mem(dst,cpu->read_mem(src,AMM_HWORD),AMM_HWORD);
                   dst += 2;
                   src += 2;
               }
           }
       break;
       case 1:
           if(DataSize != 0){
               i = cpu->read_mem(src,AMM_WORD);
               for(;count>0;count--){
                   cpu->write_mem(dst,i,AMM_WORD);
                   dst += 4;
               }
           }
           else{
               i = (u32)cpu->read_mem(src,AMM_HWORD);
               for(;count>0;count--){
                   cpu->write_mem(dst,(u16)i,AMM_HWORD);
                   dst += 2;
               }
           }
       break;
   }
}
//---------------------------------------------------------------------------
void LZ77UnComp(u32 src,u32 dst,IARM7 *cpu)
{
   u32 DataHeader,i1,i3;
   int DataSize;
   u32 b;
   u8 i,i4;

   DataHeader = cpu->read_mem(src,AMM_WORD);
   src += 4;
   DataSize = DataHeader >> 8;
   while(DataSize > 0){
       b = (u32)cpu->read_mem(src++,AMM_BYTE);
       for(i=8;i > 0 && DataSize > 0;i--){
           if((b & 0x80) == 0){
               cpu->write_mem(dst++,cpu->read_mem(src++,AMM_BYTE),AMM_BYTE);
               DataSize--;
           }
           else{
               i4 = (u8)cpu->read_mem(src++,AMM_BYTE);
               i1 = 3 + (i4 >> 4);
               i3 = (((i4 & 0xF) << 8) | cpu->read_mem(src++,AMM_BYTE)) + 1;
               DataSize -= i1;
               for(;i1 > 0;i1--)
                   cpu->write_mem(dst++,cpu->read_mem(dst-i3,AMM_BYTE),AMM_BYTE);
           }
           b <<= 1;
       }
   }
}
//---------------------------------------------------------------------------
void BgAffineSet(IARM7 *cpu)
{
   int i,sn,cs,i1,i2,i3,i4,i5,i6,i9,i10,i12,i11;
   u16 w;
   u32 dst,src;
   
   dst = cpu->r_gpreg(1);
   src = cpu->r_gpreg(0);
   for(i=cpu->r_gpreg(2);i>0;i--){
       w = (u16)(cpu->read_mem(src+0x10,AMM_HWORD) >> 8);
       sn = (int)(sin(((u8)(w + 0x40)) * M_PI / 128.0) * 16384.0);
       cs = (int)(sin(w * M_PI / 128.0) * 16384.0);
       i1 = (int)((s16)cpu->read_mem(src+12,AMM_HWORD));
       i2 = (int)((s16)cpu->read_mem(src+14,AMM_HWORD));
       i3 = (sn * i1) >> 14;
       i4 = (cs * i1) >> 14;
       i5 = (cs * i2) >> 14;
       i6 = (sn * i2) >> 14;
       i9 = cpu->read_mem(src,AMM_WORD);
       i10 = cpu->read_mem(src + 4,AMM_WORD);
       i12 = cpu->read_mem(src + 8,AMM_WORD);
       i11 = (int)((u16)i12);
       i12 >>= 16;
       i9 += i3 * -i11;
       cpu->write_mem(dst + 8,i4 * i12 + i9,AMM_WORD);
       i10 += i5 * -i11;
       cpu->write_mem(dst+12,i6 * -i12 + i10,AMM_WORD);
       cpu->write_mem(dst,(u16)i3,AMM_HWORD);
       cpu->write_mem(dst+2,(u16)(0-i4),AMM_HWORD);
       cpu->write_mem(dst+4,(u16)i5,AMM_HWORD);
       cpu->write_mem(dst+6,(u16)i6,AMM_HWORD);
       src += 20;
       dst += 16;
   }
}
//---------------------------------------------------------------------------
void ObjAffineSet(IARM7 *cpu)
{
   int i,sn,cs,i1,i2,i3,offset;
   u16 w;
   u32 dst,src;

   dst = cpu->r_gpreg(1);
   src = cpu->r_gpreg(0);
   offset = cpu->r_gpreg(3);
   for(i = cpu->r_gpreg(2);i > 0;i--){
       w = (u16)(cpu->read_mem(src+4,AMM_HWORD) >> 8);
       sn = (int)(sin(((u8)(w + 0x40)) * M_PI / 128.0) * 16384.0);
       cs = (int)(sin(w * M_PI / 128.0) * 16384.0);
       i1 = (int)((s16)cpu->read_mem(src,AMM_HWORD));
       i2 = (int)((s16)cpu->read_mem(src+2,AMM_HWORD));
       i3 = (sn * i1) >> 14;
       cpu->write_mem(dst,(u16)i3,AMM_HWORD);
       dst += offset;
       i3 = 0 - ((cs * i1) >> 14);
       cpu->write_mem(dst,(u16)i3,AMM_HWORD);
       dst += offset;
       i3 = (cs * i2) >> 14;
       cpu->write_mem(dst,(u16)i3,AMM_HWORD);
       dst += offset;
       i3 = (sn * i2) >> 14;
       cpu->write_mem(dst,(u16)i3,AMM_HWORD);
       dst += offset;
       src += 8;
   }
}
//---------------------------------------------------------------------------
static void ResetMem(u32 dst,u32 count,IARM7 *cpu)
{
   for(;count > 0;count--){
       cpu->write_mem(dst,0,AMM_WORD);
       dst += 4;
   }
}
//---------------------------------------------------------------------------
void RegisterRamReset(IARM7 *cpu)
{
   u8 value;

   value = (u8)cpu->r_gpreg(0);
   cpu->write_mem(0x04000000,0x80,AMM_WORD);
   if((value & 0x80)){
       ResetMem(0x04000200,8,cpu);
       cpu->write_mem(0x04000202,0xFFFF,AMM_HWORD);
       ResetMem(0x04000004,8,cpu);
       ResetMem(0x04000020,16,cpu);
       ResetMem(0x040000B0,24,cpu);
       cpu->write_mem(0x04000130,0x0000FFFF,AMM_WORD);
       cpu->write_mem(0x04000020,0x0100,AMM_HWORD);
       cpu->write_mem(0x04000026,0x0100,AMM_HWORD);
       cpu->write_mem(0x04000030,0x0100,AMM_HWORD);
       cpu->write_mem(0x04000036,0x0100,AMM_HWORD);
   }
   if((value & 0x40)){
       cpu->write_mem(0x04000084,0x80,AMM_BYTE);
       cpu->write_mem(0x04000080,0,AMM_WORD);
       cpu->write_mem(0x04000070,0,AMM_BYTE);
       ResetMem(0x04000090,8,cpu);
   }
   if((value & 0x20)){
       ResetMem(0x04000110,8,cpu);
       cpu->write_mem(0x04000130,0xFFFF,AMM_HWORD);
       cpu->write_mem(0x04000140,0x7,AMM_BYTE);
       ResetMem(0x04000140,7,cpu);
   }
   if((value & 0x10))
       ResetMem(0x07000000,0x0100,cpu);
   if((value & 0x8))
       ResetMem(0x06000000,0x6000,cpu);
   if((value & 0x4))
       ResetMem(0x05000000,0x0100,cpu);
   if((value & 0x2))
       ResetMem(0x03000000,0x1F7F,cpu);
   if((value & 0x1))
       ResetMem(0x02000000,0x10000,cpu);
}
