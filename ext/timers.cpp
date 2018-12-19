#include "ideastypes.h"
#include "lds.h"

TIMER timers[8];
static u16 value[2][4] = {{0,6,8,10},{1,64,256,1024}};
//-----------------------------------------------------------------------
BOOL TIMER_Save(LStream *pFile)
{
   pFile->Write(timers,sizeof(timers));
   return TRUE;
}
//-----------------------------------------------------------------------
BOOL TIMER_Load(LStream *pFile,int ver)
{
   pFile->Read(timers,sizeof(timers));
   return TRUE;
}
//-----------------------------------------------------------------------
void ResetTimer()
{
   u8 i;
   PTIMER p;

	p = timers;
   ZeroMemory(timers,sizeof(timers));
   for(i=0;i<8;i++,p++){
       p->Index = (u8)(i&3);
   	p->Freq = value[1][0];
   	p->logFreq = value[0][0];
   }
}
/*extern void ExecDMA(u8 i);
   for(i=0;i<4;i++){
       if(dma9_cycles_irq[i] == 0)
           continue;
       if(dma9_cycles_irq[i] > cyc)
           dma9_cycles_irq[i] -= cyc;
       else{
           dma9_cycles_irq[i] = 0;
           ExecDMA((i + 4)|0xF0);
       }
   }*/

//-----------------------------------------------------------------------
void RenderTimer(u16 cyc)
{
   u8 i,ovr,i1;
   u16 res,div;
   PTIMER p;

   p = timers;
   for(i1=0;i1<2;i1++){
       ovr = 0;
       for(i=0;i<4;i++,p++){
           if(!p->Enable)
               continue;
           if(p->Cascade){
               if(!ovr)
                   continue;
               res = p->Count;
               p->Count += ovr;
               ovr = 0;
               if(p->Count < res){
                   p->Count += p->ResetValue;
                   if(p->Irq)
                       ds.set_IRQ((u32)(1 << (p->Index + 3)),FALSE,(i1 ^ 1) +1);
                   ovr = 1;
               }
           }
           else{
               p->Value += cyc;
               ovr = 0;
               if(p->Value < p->Freq)
                   continue;
               res = p->Count;
               p->Count += (div = (u16)(p->Value >> p->logFreq));
               p->Value -= (div << p->logFreq);
               if(p->Count < res){
                   p->Remainder += p->Count;
                   p->Count += p->ResetValue;
                   ovr = 1;
                   if(p->Remainder >= p->Diff && p->Diff >= 2){
                       p->Remainder -= (u16)((div = (u16)(p->Remainder / p->Diff)) * p->Diff);
                       p->Inc += (u8)div;
                       ovr += (u8)div;
                   }
                   if(p->Irq)
                       ds.set_IRQ((u32)(1 << (p->Index + 3)),FALSE,(i1 ^ 1) +1);
               }
           }
       }
   }
}
//-----------------------------------------------------------------------
static u32 oTIMER_L7(u32 adr,u8 am)
{
   LPTIMER p;

	p = &timers[((adr - 0x100) >> 2)];
   if(am == AMM_HWORD)
       return (u32)p->Count;
   else if(am == AMM_WORD)
       return (u32)MAKELONG(p->Count,*((u16 *)(io_mem7 + adr + 2)));
   return 0;
}
//-----------------------------------------------------------------------
static void iTIMER_H7(u32 adr,u32 data,u8 am)
{
   u16 Control;
	LPTIMER p;
   u8 Enable;

	p = &timers[((adr - 0x100) >> 2)];
   p->Control = (u16)data;
   Enable = p->Enable;
   p->Enable = (u8)((Control = p->Control) >> 7);
   if(p->Enable != Enable)
       p->Count = p->ResetValue;
   p->Irq = (u8)((Control & 0x40) >> 6);
   p->Cascade = (u8)(p->Index == 0 ? 0 : ((Control & 0x4) >> 2));
   p->Freq = value[1][(Control & 0x3)];
   p->logFreq = value[0][(Control & 0x3)];
}
//-----------------------------------------------------------------------
static void iTIMER_L7(u32 adr,u32 data,u8 am)
{
	LPTIMER p;

	p = &timers[((adr - 0x100) >> 2)];
   p->ResetValue = (u16)data;
   p->Diff = (u16)(65536 - p->ResetValue);               //802fefA
   p->Inc = 0;
   p->Remainder = 0;
   p->Value = 0;
   if(am == AMM_WORD)
       iTIMER_H7(adr,(u32)(data >> 16),AMM_WORD);
}
//-----------------------------------------------------------------------
static u32 oTIMER_L9(u32 adr,u8 am)
{
   LPTIMER p;

	p = &timers[4+((adr - 0x100) >> 2)];
   if(am == AMM_HWORD)
       return (u32)p->Count;
   else if(am == AMM_WORD)
       return (u32)MAKELONG(p->Count,*((u16 *)(io_mem + adr + 2)));
   return 0;
}
//-----------------------------------------------------------------------
static void iTIMER_H9(u32 adr,u32 data,u8 am)
{
   u16 Control;
	LPTIMER p;
   u8 Enable;

	p = &timers[4+((adr - 0x100) >> 2)];
   p->Control = (u16)data;
   Enable = p->Enable;
   p->Enable = (u8)((Control = p->Control) >> 7);
//   if(p->Enable != Enable)
//       p->Count = p->ResetValue;
   p->Irq = (u8)((Control & 0x40) >> 6);
   p->Cascade = (u8)(p->Index == 0 ? 0 : ((Control & 0x4) >> 2));
   p->Freq = value[1][(Control & 0x3)];
   p->logFreq = value[0][(Control & 0x3)];
}
//-----------------------------------------------------------------------
static void iTIMER_L9(u32 adr,u32 data,u8 am)
{
	LPTIMER p;

	p = &timers[4+((adr - 0x100) >> 2)];
   p->ResetValue = (u16)data;
	p->Count = p->ResetValue;   
   p->Diff = (u16)(65536 - p->ResetValue);               //802fefA
   p->Inc = 0;
   p->Remainder = 0;
   p->Value = 0;
   if(am == AMM_WORD)
       iTIMER_H9(adr,(u32)(data >> 16),AMM_WORD);
}
//-----------------------------------------------------------------------
void InitTimersTable(LPIFUNC *p,LPIFUNC *p1,LPOFUNC *p2,LPOFUNC *p3)
{
	int i,i1;

   for(i=0x100;i<0x10F;){
       for(i1=0;i1<2;i1++,i++){
   	    p1[i] = iTIMER_L7;
           p[i] = iTIMER_L9;
           p2[i] = oTIMER_L9;
           p3[i] = oTIMER_L7;
       }
       for(i1=0;i1<2;i1++,i++){
           p1[i] = iTIMER_H7;
           p[i] = iTIMER_H9;
       }
   }
}

