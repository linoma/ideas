#include "ideastypes.h"
#include "io.h"
#include "lds.h"
#include "util.h"
#include "card.h"

DMA dmas[8];
static char mode[][8][30] = {
       {"Immediately","V-Blank","DS Slot","GBA Slot/Wirelesse","","","",""},
   	{"Immediately","V-Blank","H-Blank","Synchronize with display","Main memory","DS Slot","GBA Slot","GC FIFO"}
   };

//---------------------------------------------------------------------------
BOOL DMA_Save(LStream *pFile)
{
   pFile->Write(dmas,sizeof(dmas));
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL DMA_Load(LStream *pFile,int ver)
{
	int i;

   if(ver < 48){
       if(ver < 43)
           ver = 40;
       else
           ver = 44;
       for(i=0;i<8;i++)
           pFile->Read(&dmas[i],ver); //1600
   }
   else
		pFile->Read(dmas,sizeof(dmas)); //1600
   return TRUE;
}
//---------------------------------------------------------------------------
void ResetDMA()
{
   u8 i;
   LPDMA p;

   p = dmas;
   ZeroMemory(p,sizeof(dmas));
   for(i=0;i<8;i++,p++){
       p->Index = (u8)(i & 3);
       p->MaxCount = 0x1FFFFF;
   }
}
//---------------------------------------------------------------------------
void ExecDMA(u8 i)
{
   LPDMA p;
   IARM7 *cpu;
   int i1;

   i1 = (int)i;
   p = &dmas[i &= 0xF];
   if(p->Enable == 0)
       return;
	if(i > 3){
       crI = i_func;                                                                                        
       crO = o_func;
       crIO_mem = io_mem;
       cpu = &arm9;
       if(p->Start == 5)
           p->InternalCount = card.dataRead;
       if((ds.get_Options() & NDSO_USE_DMA_LATENCY) && !(i1 & 0xF0) && p->InternalCount > 0x10){
           dmas[i].latency = /*((waitState9[p->Src >> 24] & 0xFF) + (waitState9[p->Dst >> 24] & 0xFF)) **/ p->InternalCount * 8;//5.3
           if(p->Start == 5)
               dmas[i].latency *= 48;
           cpu->enter_dmaloop();
           return;
       }
       i1 = MSGT_DMA9;
   }
   else{
   	crI = i_func7;
       crO = o_func7;
       crIO_mem = io_mem7;
       cpu = &arm7;
       i1 = MSGT_DMA7;
   }
   WriteMsgConsolle(cpu,"%c ARM%1d: Ch%1d %08X->%08X %08X %s %s IRQ %s %02X",
       i1,cpu->r_index(),p->Index,p->Src,p->Dst,p->InternalCount,
       p->Mode != 0 ? "W" : "HW",mode[cpu->r_index() / 9][p->Start],p->Irq ? "Yes" : "No",VCOUNT);
   if(i > 3 && p->Src < 0x2000000)
       goto ExecDMA_0;
	if(p->Mode != 0){
       p->Dst &= ~3;
       p->Src &= ~3;
	    for(i1 = p->InternalCount;i1 > 0;i1--){           //8003f0c
/*			u32 value;

			value = read_word(p->Src);
           if((p->Dst & 0x0FFF0000) == 0x06890000 && p->InternalCount > 0x1000){//0x6892320
           	FILE *fp;

               fp = fopen("c:\\palette_ideas.xt","ab+");
               fprintf(fp,"%08X\r\n",value);
               fclose(fp);
           }*/
		    write_word(p->Dst,read_word(p->Src));
//           cpu->write_mem(p->Dst,cpu->read_mem(p->Src,AMM_WORD),AMM_WORD);
           p->Dst += p->IncD;
           p->Src += p->IncS;
           p->Count++;
       }                                     //20fd9c0
   }
	else{
       p->Dst &= ~1;
       p->Src &= ~1;
	    for(i1 = p->InternalCount;i1 > 0;i1--){           //8003f0c
	    	write_hword(p->Dst,read_hword(p->Src));
//           cpu->write_mem(p->Dst,cpu->read_mem(p->Src,AMM_HWORD),AMM_HWORD);
           p->Dst += p->IncD;
           p->Src += p->IncS;
       }
   }
ExecDMA_0:
   if(!p->Repeat/* || p->Start == 5*/ || (p->Start == 4 && p->Count > 24575)){
       p->Enable = 0;
       p->InternalCount = (u32)(p->Control & p->MaxCount);
       p->Dst = p->Dest;
       p->Src = p->Source;
       *((u16 *)&crIO_mem[0xBA + ((i & 3) * 12)]) &= (u16)~0x8000;
       p->Control &= 0x7FFFFFFF;
   }
   if(p->Start == 5)
       p->InternalCount = (u32)(p->Control & p->MaxCount);
   if(p->Reload != 0){
       p->Dst = p->Dest;
//       p->Src = *p->Source;
       p->InternalCount = (u16)(p->Control & p->MaxCount);
       p->Enable = 1;
   }
   if(p->Irq != 0){
       ds.set_IRQ((u32)(0x100 << p->Index),FALSE,(u8)(i > 3 ? 1 : 2));
   }
}
//---------------------------------------------------------------------------
void StartDMA(u8 type)
{
   LPDMA p;
   u8 i;

   p = dmas;
   for(i=0;i<8;i++,p++){
       if(!p->Enable || p->Start != type)
           continue;
       ExecDMA(i);
   }
}
//---------------------------------------------------------------------------
static void iDMASRC7(u32 adr,u32 data,u8 am)
{
   LPDMA p;
   u8 i;

   p = &dmas[i = (u8)(((adr - 0xB0) / 12))];
   if(am != AMM_WORD)
   	data = *((u32 *)&io_mem7[(i*12)+0xB0]);
   p->Src = p->Source = data;
   if(p->Enable && p->Start == 0)
       ExecDMA(i);
}
//---------------------------------------------------------------------------
static void iDMADST7(u32 adr,u32 data,u8 am)
{
   LPDMA p;
   u8 i;

   p = &dmas[i = (u8)(((adr - 0xB0) / 12))];
   if(am != AMM_WORD)
   	data = *((u32 *)&io_mem7[(i*12)+0xB4]);
   p->Dst = p->Dest = data;
   if(p->Enable && p->Start == 0)
       ExecDMA(i);
}
//---------------------------------------------------------------------------
static void iDMACNT_H7(u32 adr,u32 data,u8 am)
{
   LPDMA p;                                   //2058d68 // 2053140
   u16 cnt;
   u8 bEnable,i;
//r2 == 0x83DE600A
   p = &dmas[(i = (u8)((adr - 0xB0) / 12))];
   p->Control = am == AMM_HWORD ? (data << 16)|((u16)p->Control) :
       (data & 0xFFFF0000)|((u16)p->Control);
   bEnable = p->Enable;
   p->Enable = (u8)((cnt = (u16)(p->Control >> 16)) >> 15);
   p->Irq = (u8)((cnt >> 14) & 1);
   p->Repeat = (u8)((cnt >> 9) & 1);
   if(!bEnable && p->Enable)
   	p->Count = 0;
   if(!p->Enable && bEnable && p->Repeat){
       p->Dst = p->Dest;
       p->Src = p->Source;
   }
   p->Reload = 0;
   switch((cnt & 0x60) >> 5){
       case 0:
           p->IncD = 4;
       break;
       case 1:
           p->IncD = -4;
       break;
		case 2:
           p->IncD = 0;
       break;
       case 3:
           p->Reload = 1;
           p->IncD = 4;
       break;
   }
   switch((cnt & 0x180) >> 7){
       case 0:
           p->IncS = 4;
       break;
       case 1:
           p->IncS = -4;
       break;
		default:
           p->IncS = 0;
       break;
   }
   p->Start = (u8)((cnt >> 12) & 3);
   if(!(p->Mode = (u8)((cnt >> 10) & 1))){
       p->IncS >>= 1;
       p->IncD >>= 1;
   }
   if(p->Enable && p->Start == 0)
       ExecDMA(i);
}
//---------------------------------------------------------------------------
static void iDMACNT_L7(u32 adr,u32 data,u8 am)
{
   LPDMA p;
   u8 i;

   p = &dmas[i = (u8)(((adr - 0xB0) / 12))];
   p->Control = (p->Control & 0xFFFF0000) | ((u16)data);
   if((p->InternalCount = (u32)(data & p->MaxCount)) == 0)
       p->InternalCount = p->MaxCount;
   if(am == AMM_WORD)
       iDMACNT_H7(adr,data,am);
   else{
       if(p->Enable && p->Start == 0)
           ExecDMA(i);
   }
}
//---------------------------------------------------------------------------
static u32 oDMACNT_L7(u32 adr,u8 am)
{
   LPDMA p;

   p = &dmas[((adr - 0xB0) / 12)];
   if(am == AMM_WORD)
       return p->Control;
   else if(am == AMM_HWORD)
       return (u32)(u16)p->Control;
   return (u32)(u8)(p->Control);
}
//---------------------------------------------------------------------------
static u32 oDMACNT_H7(u32 adr,u8 am)
{
   LPDMA p;

   p = &dmas[((adr - 0xB0) / 12)];
   if(am == AMM_HWORD)
       return (u32)(p->Control >> 16);
   return 0;
}
//---------------------------------------------------------------------------
static void iDMASRC9(u32 adr,u32 data,u8)
{
   LPDMA p;
   u8 i;

   p = &dmas[i = (u8)(4+((adr - 0xB0) / 12))];
   p->Src = p->Source = data;
   if(p->Enable && p->Start == 0)
       ExecDMA(i);
}
//---------------------------------------------------------------------------
static void iDMADST9(u32 adr,u32 data,u8)
{
   LPDMA p;
   u8 i;

   p = &dmas[i = (u8)(4+((adr - 0xB0) / 12))];
   p->Dst = p->Dest = data;
   if(p->Enable && p->Start == 0)
       ExecDMA(i);
}
//---------------------------------------------------------------------------
static void iDMACNT_H9(u32 adr,u32 data,u8 am)
{
   LPDMA p;                                   //2058d68 // 2053140
   u16 cnt;
   u8 bEnable,i;

   p = &dmas[(i = (u8)(4+((adr - 0xB0) / 12)))];
   if(am == AMM_HWORD)
   	p->Control = (data << 16)|((u16)p->Control);
   else
		p->Control = (data & 0xFFFF0000)|((u16)p->Control);
   bEnable = p->Enable;
   p->Enable = (u8)((cnt = (u16)(p->Control >> 16)) >> 15);
   p->Irq = (u8)((cnt >> 14) & 1);
   p->Repeat = (u8)((cnt >> 9) & 1);
   if(!bEnable && p->Enable)
   	p->Count = 0;
   if(!p->Enable && bEnable && p->Repeat){
       p->Dst = p->Dest;
       p->Src = p->Source;
   }
   p->Reload = 0;
   switch((cnt & 0x60) >> 5){
       case 0:
           p->IncD = 4;
       break;
       case 1:
           p->IncD = -4;
       break;
		case 2:
           p->IncD = 0;
       break;
       case 3:
           p->Reload = 1;
           p->IncD = 4;
       break;
   }
   switch((cnt & 0x180) >>7){
       case 0:
           p->IncS = 4;
       break;
       case 1:
           p->IncS = -4;
       break;
		default:
           p->IncS = 0;
       break;
   }
   //fix me card is not ready
   {
       u8 old_start;

       old_start = p->Start;
       p->Start = (u8)((cnt >> 11) & 7);
       if(old_start == 5 && old_start != p->Start){
           p->Dst = p->Dest;
           p->Control &= ~p->MaxCount;
           p->InternalCount = (u32)(p->Control & p->MaxCount);
       }
   }
   if(!(p->Mode = (u8)((cnt >> 10) & 1))){
       p->IncS >>= 1;
       p->IncD >>= 1;
   }
   if(p->Enable && (p->Start == 0 || (p->Start == 7 && (ds.get_CurrentStatus() & 1))))
       ExecDMA(i);
}
//---------------------------------------------------------------------------
static void iDMACNT_L9(u32 adr,u32 data,u8 am)
{
   LPDMA p;
   u8 i;

   p = &dmas[i = (u8)(4+((adr - 0xB0) / 12))];
   p->Control = (p->Control & 0xFFFF0000) | ((u16)data);
   if((p->InternalCount = (u32)(data & p->MaxCount)) == 0)
       p->InternalCount = p->MaxCount;
   if(am == AMM_WORD)
       iDMACNT_H9(adr,data,am);
   else{
       if(p->Enable && p->Start == 0)
           ExecDMA(i);
   }
}
//---------------------------------------------------------------------------
static u32 oDMACNT_L9(u32 adr,u8 am)
{
   LPDMA p;

   p = &dmas[4+((adr - 0xB0) / 12)];
   if(am == AMM_WORD)
       return p->Control;
   else if(am == AMM_HWORD)
       return (u32)(u16)p->Control;
   return (u32)(u8)(p->Control);
}
//---------------------------------------------------------------------------
static u32 oDMACNT_H9(u32 adr,u8 am)
{
   LPDMA p;

   p = &dmas[4+((adr - 0xB0) / 12)];
   if(am == AMM_HWORD)
       return (u32)(p->Control >> 16);
   return 0;
}
//---------------------------------------------------------------------------
void InitDmaTable(LPIFUNC *p,LPIFUNC *p1,LPOFUNC *p2,LPOFUNC *p3)
{
	int i,i1;

   for(i=0xB0;i<0xE0;){
       for(i1=0;i1 < 4;i1++,i++){
           p[i] = iDMASRC9;
           p1[i]= iDMASRC7;
       }
       for(i1=0;i1 < 4;i1++,i++){
           p[i] = iDMADST9;
           p1[i]= iDMADST7;
       }
       for(i1=0;i1 < 2;i1++,i++){
           p[i] = iDMACNT_L9;
           p1[i]= iDMACNT_L7;
           p2[i] = oDMACNT_L9;
           p3[i] = oDMACNT_L7;
       }
       for(i1=0;i1 < 2;i1++,i++){
           p[i] = iDMACNT_H9;
           p1[i]= iDMACNT_H7;
           p2[i] = oDMACNT_H9;
           p3[i] = oDMACNT_H7;
       }
   }
}
