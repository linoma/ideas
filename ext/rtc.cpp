#include "rtc.h"
#include "dsmem.h"
#include "util.h"

static RTC rtc;
static u16 control;
//---------------------------------------------------------------------------
void RTC_reset()
{
   ZeroMemory(&rtc,sizeof(RTC));
   rtc.reg[0] = 2;
   control = 0;
}
//---------------------------------------------------------------------------
static u8 toBCD(int value)
{
  	value = value % 100;
  	int l = value % 10;
  	int h = value / 10;                                    
  	return (u8)((h << 4) + l);
}
//---------------------------------------------------------------------------
static void ioDATAI(u32 address,u32 data,u8)
{
   SYSTEMTIME st;

   if(!(data & 0x10)){
       switch(rtc.state){
           default:
           	io_mem7[0x138] = rtc.dataOut;
           	if((data & 4) == 0 || rtc.state != 2){
              		if((data & 0x10) == 0){
                      	rtc.bits = 0;
                  		rtc.state = 0;
                  		rtc.command = 0;
              		}
              		return;
           	}
               else if(!(data & 2)){
                   rtc.dataOut = (u8)(0x66 | (u8)((rtc.data[rtc.bits >> 3] >> (rtc.bits & 7)) & 1));
                   io_mem7[0x138] = rtc.dataOut;
           		rtc.bits++;
               }
           	if(rtc.bits != 8*rtc.dataLen)
              		return;
           	rtc.bits = 0;
           	rtc.state = 0;
           	rtc.command = 0;
				return;
       	case 3:
           	rtc.bits++;
           	if(rtc.bits == rtc.dataLen) {
              		rtc.bits = 0;
              		rtc.state = 0;
           	}
				return;
       }
   }
  	switch(rtc.state){
   	case 0:
       	if((data & 0x76) == 0x76){
				rtc.command = 0;
               rtc.state = 1;
               rtc.bits = 0;
           }
       break;
     	case 2:
          	if((data & 0x76) != 0x76)
              	return;
           rtc.data[rtc.bits >> 3] |= (u8)((data & 1) << (7 - (rtc.bits & 7)));
           rtc.bits++;
          	if(rtc.bits != 8*rtc.dataLen)
          		return;
           switch(((rtc.command >> 1) & 7)){
               case 0:
                   rtc.reg[0] = rtc.data[0];
               break;
               case 1:
                   rtc.reg[1] = rtc.data[0];
               break;
               case 6:
                   rtc.reg[2] = rtc.data[0];
               break;
               case 7:
                   rtc.reg[3] = rtc.data[0];
               break;
           }
          	rtc.bits = 0;
          	rtc.state = 0;
          	rtc.command = 0;
       break;
     	case 1:
          	if((data & 0x76) != 0x76)
              	return;
          	rtc.command |= (u8)((data & 1) << (7 - rtc.bits));
          	rtc.bits++;
          	if(rtc.bits >= 8) {
              	rtc.bits = 0;
              	switch(((rtc.command >> 1) & 7)){
                  	case 0x0:
                      	rtc.dataLen = 1;
                      	rtc.data[0] = rtc.reg[0];
                      	rtc.state = 2;//Data ready
                  	break;
                   case 0x1:
                      	rtc.dataLen = 1;
                      	rtc.data[0] = rtc.reg[1];
                      	rtc.state = 2;//Data ready
                   break;
                   case 0x2:
                      	GetLocalTime(&st);
                      	rtc.data[0] = toBCD(st.wYear);
                      	rtc.data[1] = toBCD(st.wMonth);
                      	rtc.data[2] = toBCD(st.wDay);
                  	 	rtc.data[3] = toBCD((st.wDayOfWeek == 0 ? 6 : st.wDayOfWeek - 1));
                       if(!(rtc.reg[0] & 2) && st.wHour > 11)
                           st.wHour -= (WORD)12;
                     	rtc.data[4] = toBCD(st.wHour);
                  	 	rtc.data[5] = toBCD(st.wMinute);
                      	rtc.data[6] = toBCD(st.wSecond);
                       if(st.wHour > 11)
                           rtc.data[4] |= 0x40;
                      	rtc.dataLen = 7;
                      	rtc.state = 2;//Data ready
                  	break;
                  	case 0x3:
                      	GetLocalTime(&st);
                       if(!(rtc.reg[0] & 2) && st.wHour > 11)
                           st.wHour -= (WORD)12;
                      	rtc.data[0] = toBCD(st.wHour);
                      	rtc.data[1] = toBCD(st.wMinute);
                      	rtc.data[2] = toBCD(st.wSecond);
                       if(st.wHour > 11)
                           rtc.data[0] |= 0x40;
                      	rtc.dataLen = 3;
                      	rtc.state = 2;
                  	break;
                   case 4:
                      	rtc.dataLen = 3;
                      	rtc.state = 2;
                      	rtc.data[0] = 0;
                      	rtc.data[1] = 0;
                      	rtc.data[2] = 0;
                   break;
                   case 5:
                      	rtc.dataLen = 3;
                      	rtc.state = 2;
                      	rtc.data[0] = 0;
                      	rtc.data[1] = 0;
                      	rtc.data[2] = 0;
                   break;
                   case 6:
                      	rtc.dataLen = 1;
                      	rtc.state = 2;
                      	rtc.data[0] = rtc.reg[2];
                   break;
                   case 7:
                      	rtc.dataLen = 1;
                      	rtc.state = 2;
                      	rtc.data[0] = rtc.reg[3];
                   break;
              	}
          	}
      	break;
   }
}
//---------------------------------------------------------------------------
static void ioRCNTI(u32 address,u32 data,u8)
{
   control = (u16)data;
   if(data != 0x8100 && data != 0x8000){
       ds.set_IRQ(0x80,FALSE,2);
   }
}
//---------------------------------------------------------------------------
void InitRtcTable(LPIFUNC *p,LPIFUNC *p1,LPOFUNC *p2,LPOFUNC *p3)
{
   p1[0x138] = ioDATAI;
   p1[0x134] = ioRCNTI;
}

