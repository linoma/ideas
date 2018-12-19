#include "cpm.h"
#include "dsmem.h"
#include <math.h>
#ifdef _DEBPRO
	#include "util.h"
#endif
//---------------------------------------------------------------------------
static void DoDiv()
{
	s64 n,d,r,m;
   BOOL div0;

	div0 = FALSE;
   switch(*((u16 *)&io_mem[0x280]) & 3){
       case 0:
       	n = (s64)*((s32 *)(io_mem + 0x290));
			d = (s64)*((s32 *)(io_mem + 0x298));
           div0 = FALSE;
       break;
       case 1:
       	n = *((s64 *)(io_mem + 0x290));
			d = (s64)*((s32 *)(io_mem + 0x298));
           div0 = TRUE;
       break;
       case 2:
   		n = *((s64 *)(io_mem + 0x290));
   		d = *((s64 *)(io_mem + 0x298));
           div0 = TRUE;
   	break;
       case 3:
           n = d = 0;
       break;
   }
#ifdef _DEBPRO
   if(d == -1 || n == -1 || d == 0x8000000000000000 || n == 0x8000000000000000)
   	d = d;
#endif
   if(d != 0){
		r = n / d;
       m = n % d;
		*((u16 *)(io_mem + 0x280)) &= ~0x4000;
   }
   else{
   	r = n >= 0 ? 1 : -1;
       m = n;
       if(div0)
			*((u16 *)(io_mem + 0x280)) |= 0x4000;
   }
	*((u16 *)(io_mem + 0x280)) &= (u16)~0x8000;
   *((u64 *)(io_mem + 0x2A0)) = r;
   *((u64 *)(io_mem + 0x2A8)) = m;
}
//---------------------------------------------------------------------------
static void DoSqr()
{
	long double ld;

	if(*((u16 *)&io_mem[0x2b0]) & 1)
       ld = (long double)*((u64 *)(io_mem + 0x2B8));
   else
       ld = (long double)*((u32 *)(io_mem + 0x2B8));
  	*((u32 *)(io_mem + 0x2B4)) = (u32)sqrtl(ld);
   *((u16 *)(io_mem + 0x2B0)) &= (u16)~0x8000;
}
//---------------------------------------------------------------------------
static void ioDIVCNT(u32 adr,u32,u8)
{
   *((u16 *)(io_mem + 0x280)) &= (u16)~0x8000;
}
//---------------------------------------------------------------------------
static void ioDIV(u32 adr,u32,u8)
{
   if(adr == 0x298 || adr == 0x29C)
       DoDiv();
}
//---------------------------------------------------------------------------
static void ioSQRT(u32 adr,u32,u8)
{
   if(adr == 0x2B8 || adr == 0x2BC)
  		DoSqr();
}
//---------------------------------------------------------------------------
static void ioSQRTCNT(u32 adr,u32,u8)
{
   *((u16 *)(io_mem + 0x2B0)) &= (u16)~0x8000;
}
//---------------------------------------------------------------------------
void InitCPMTable(LPIFUNC *p,LPIFUNC *p1,LPOFUNC *p2,LPOFUNC *p3)
{
   int i;

   for(i = 0x280;i<0x282;i++)
       p[i] = p1[i] = ioDIVCNT;
   for(i = 0x298;i<0x2A0;i++)
       p[i] = p1[i] = ioDIV;
   for(i=0x2B0;i<0x2B2;i++)
       p[i] = p1[i] = ioSQRTCNT;
   for(i=0x2B8;i<0x2C0;i++)
       p[i] = p1[i] = ioSQRT;
}
