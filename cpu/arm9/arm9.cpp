#include "arm9.h"
#include "dsmem.h"
#include "util.h"
#include "syscnt.h"
#include "bios.h"
#include "io.h"
//---------------------------------------------------------------------------

#ifdef READWORD
#undef READWORD
#undef READHWORD
#undef READBYTE
#endif

#define READWORD(a) read_word_data(a)
#define READHWORD(a) read_hword_data(a)
#define READBYTE(a) read_byte_data(a)


#ifdef WRITEWORD
#undef WRITEWORD
#undef WRITEHWORD
#undef WRITEBYTE
#endif
#define WRITEWORD(a,b) pfn_write_word(a,b)
#define WRITEHWORD(a,b) pfn_write_hword(a,b)
#define WRITEBYTE(a,b) pfn_write_byte(a,b)


#define O_DTCM(a,b,c)													\
   if(a >= sysCnt.dtcm_address && a < sysCnt.dtcm_end){				\
	    c;															    \
       wait_state.cycles = waitState9[0];                              \
       return;															\
   }

#define I_DTCM(a,b,c)											   	    \
   if(a >= sysCnt.dtcm_address && a < sysCnt.dtcm_end){				\
   	if(!(sysCnt.reg1 & 0x20000)){                                   \
           wait_state.cycles = waitState9[0];                          \
			return c;													\
       }                                                               \
   }

#define O_ITCM(a,b,c)													\
  	if(a < sysCnt.itcm_end){             							    \
       if(a < sysCnt.itcm_pu_rgn_start || a > sysCnt.itcm_pu_rgn_end){ \
           abort_nds(1);                                               \
           return;                                                     \
       }                                                               \
       b;															    \
       wait_state.cycles = waitState9[0];                              \
       return;															\
   }

#define I_ITCM(a,b,c)												    \
	if(a < sysCnt.itcm_end){                                  			\
      	if(!(sysCnt.reg1 & 0x80000)){                                   \
           wait_state.cycles = waitState9[0];                          \
			return b;													\
       }                                                               \
   }

#define O_TCM(a,b,c)													   	\
   O_ITCM(a,b,c)\
   O_DTCM(a,b,c)

#define I_TCM(a,b,c)												   	    \
   I_ITCM(a,b,c)\
   I_DTCM(a,b,c)

#define CalcAddc(a,b) a + b + c_flag
#define CalcSubc(a,b) a - b - (c_flag ^ 1)


#ifdef _DEBUG

#define CHECK_BRANCH(a)    \
       if(sysCnt.lastAddressCount > 2 && (ds.get_Optimize() & 2) && !debugDlg.is_Active()){ \
           sysCnt.loop = 1;\
           if(a < 0 && a > -5){\
           	if(arm7.r_stop())\
             		sysCnt.loop = -1;\
               else\
                   sysCnt.loop = 2;\
           }\
       }

#else

#define CHECK_BRANCH(a)    \
       if(sysCnt.lastAddressCount > 2 && (ds.get_Optimize() & 2)){ \
           sysCnt.loop = 1;\
           if(a < 0 && a > -5){\
               if(arm7.r_stop())\
                   sysCnt.loop = -1;\
               else\
                   sysCnt.loop = 2;\
           }\
       }
#endif

#define RUN_CHECK_BRANCH(a)    \
	if(a == sysCnt.lastAddress)\
  		sysCnt.lastAddressCount++;\
  	else{\
      	sysCnt.lastAddress = a;\
  		sysCnt.lastAddressCount = 0;\
       sysCnt.loop = 0;\
   }

#define SYSCNT_CAN_OPERATE_WRITE(a,b)\
   if(0) return;
#define SYSCNT_CAN_OPERATE_READ(a,b)\
   if(0) return 0;

extern void ExecDMA(u8 i);
extern DMA dmas[8];

#ifdef _DEBPRO
extern FILE *fpLog;
extern u32 lastWrite;

#endif

//---------------------------------------------------------------------------
u16 (*exec9)(void);
void (*pfn_arm9irq)(void);
CPU_ARM9 arm9;
u32 nIRQ9;
//---------------------------------------------------------------------------
static u16 (*exec9_i)(void);
static void (*fetch_func)(void);
static u32 I_FASTCALL (*pfn_read_word)(u32);
static u32 I_FASTCALL (*pfn_read_hword)(u32);
static u32 I_FASTCALL (*pfn_read_byte)(u32);
static void I_FASTCALL (*pfn_write_word)(u32,u32);
static void I_FASTCALL (*pfn_write_hword)(u32,u32);
static void I_FASTCALL (*pfn_write_byte)(u32,u32);
static u32 opcode,gp_reg[17],reg[6][17],cpsr,old_pc,stopped,fromStopped;
static ARM_INS arm_ins[0x1000];
static ARM_INS thumb_ins[0x400];
static u8 itcm_mem[32768],dtcm_mem[16384],n_flag,z_flag,c_flag,v_flag,q_flag,stop,cyclesP;
static union{
   u32 cycles;
   struct{
       u16 cyclesS;
       u16 cyclesN;
   } dummy;
} wait_state;

#ifdef _DEBPRO
static LVector<unsigned long> callStack;
#endif

static DECODEARM *dbgarm_ins;
static DECODEARM *dbgthumb_ins;
static char **arm_opcode_strings;
static char **thumb_opcode_strings;
static MEMORYINFO MemoryAddress[] = {
   {"EXTERN WRAM",0x02000000,0x400000,AMM_ALL,0,AMM_ALL,(DWORD)-1,NULL,NULL},
   {"INTERN WRAM",0x03000000,0x1000000,AMM_ALL,0,AMM_ALL,(DWORD)-1,NULL,NULL},
   {"I/O",0x04000000,0x2000,AMM_ALL,0,0,(DWORD)-1,NULL,NULL},
   {"I/O DSI",0x04004000,0x400,AMM_ALL,0,0,(DWORD)-1,NULL,NULL},
   {"PALETTE",0x05000000,0x0800,AMM_ALL,0,0,(DWORD)-1,NULL,NULL},
   {"MAIN - BG",0x06000000,0x80000,AMM_ALL,0,0,(DWORD)-1,NULL,NULL},
   {"SUB - BG",0x06200000,0x20000,AMM_ALL,0,0,(DWORD)-1,NULL,NULL},
   {"MAIN - OBJ",0x06400000,0x20000,AMM_ALL,0,0,(DWORD)-1,NULL,NULL},
   {"SUB - OBJ",0x06600000,0x20000,AMM_ALL,0,0,(DWORD)-1,NULL,NULL},
   {"LCD",0x06800000,0xA4000,AMM_ALL,0,AMM_ALL,(DWORD)-1,NULL,NULL},
   {"OAM",0x07000000,0x0800,AMM_ALL,0,0,(DWORD)-1,NULL,NULL},
   {"GBA ROM",0x08000000,0x01000000,AMM_ALL,0,0,(DWORD)-1,NULL,NULL},
   {"SRAM",0x0A000000,0x10000,AMM_BYTE,0,0,(DWORD)-1,NULL,NULL},
   {"ITCM",0,0x8000,AMM_ALL,0,AMM_ALL,(DWORD)-1,NULL,NULL},
   {"DTCM",0xFD000000,0x4000,AMM_ALL,0,AMM_ALL,(DWORD)-1,NULL,NULL},
   {"BIOS ARM9 ROM",0xFFFF0000,0x10000,AMM_ALL,0,AMM_ALL,(DWORD)-1,NULL,NULL}
};
//---------------------------------------------------------------
NO_IN_LINE static u32 I_FASTCALL CalcAddcFlags(u32 a,u32 b)
{
#ifdef __BORLANDC__
   __asm{
       bt  word ptr[c_flag],0
       adc  eax,edx
       sets byte ptr[n_flag]
       setz byte ptr[z_flag]
       setc byte ptr[c_flag]
       seto byte ptr[v_flag]
   }
   return _EAX;
#elif defined(__WATCOMC__)
   __asm{
       bt  word ptr[c_flag],0
       adc  eax,edx
       sets byte ptr[n_flag]
       setz byte ptr[z_flag]
       setc byte ptr[c_flag]
       seto byte ptr[v_flag]
       ret
   }
   return 0;
#elif defined(__GNUC__)
	u32 ret;

	__asm__ __volatile__(
 		"btw $0,%1\n"
		"adcl %%edx,%%eax\n"
		"setsb %2\n"
		"setzb %3\n"
		"setcb %1\n"
		"setob %4\n"
       "movl %%eax,%0"
		: "=m" (ret) : "m"(c_flag),"m"(n_flag),"m"(z_flag),"m"(v_flag)
   );
	return ret;
#endif
}

//---------------------------------------------------------------
NO_IN_LINE static u32 I_FASTCALL CalcAddFlags(u32 a,u32 b)
{
#ifdef __BORLANDC__
   __asm{
       add  eax,edx
       sets byte ptr[n_flag]
       setz byte ptr[z_flag]
       setc byte ptr[c_flag]
       seto byte ptr[v_flag]
   }
   return _EAX;
#elif defined(__WATCOMC__)
   __asm{
       add  eax,edx
       sets byte ptr[n_flag]
       setz byte ptr[z_flag]
       setc byte ptr[c_flag]
       seto byte ptr[v_flag]
       ret
   }
   return 0;
#elif defined(__GNUC__)
	u32 ret;

	__asm__ __volatile__(
		"addl %%edx,%%eax\n"
		"setsb %2\n"
		"setzb %3\n"
		"setcb %1\n"
		"setob %4\n"
        "movl %%eax,%0"
       : "=m" (ret) : "m"(c_flag),"m"(n_flag),"m"(z_flag),"m"(v_flag)
	);
	return ret;
#endif
}
//---------------------------------------------------------------
NO_IN_LINE static u32 I_FASTCALL CalcSubFlags(u32 a,u32 b)
{
#ifdef __BORLANDC__
   __asm{
       sub  eax,edx
       sets byte ptr[n_flag]
       setz byte ptr[z_flag]
       setnc byte ptr[c_flag]
       seto byte ptr[v_flag]
   }
   return _EAX;
#elif defined(__WATCOMC__)
   __asm{
       sub  eax,edx
       sets byte ptr[n_flag]
       setz byte ptr[z_flag]
       setnc byte ptr[c_flag]
       seto byte ptr[v_flag]
       ret
   }
   return 0;
#elif defined(__GNUC__)
	u32 ret;

	__asm__ __volatile__ (
		"subl %%edx,%%eax\n"
		"setsb %2\n"
		"setzb %3\n"
		"setncb %1\n"
		"setob %4\n"
       "movl %%eax,%0"
       : "=m" (ret) : "m"(c_flag),"m"(n_flag),"m"(z_flag),"m"(v_flag)
	);
	return ret;
#endif
}
//---------------------------------------------------------------
NO_IN_LINE static u32 I_FASTCALL CalcSubcFlags(u32 a,u32 b)
{
#ifdef __BORLANDC__
   __asm{                                                   
		movzx ecx,byte ptr[c_flag]
       xor ecx,1
       bt  cx,0                            
       sbb  eax,edx
       sets byte ptr[n_flag]
       setz byte ptr[z_flag]
       setnc byte ptr[c_flag]
       seto byte ptr[v_flag]
   }
   return _EAX;
#elif defined(__WATCOMC__)
   __asm{
		movzx ecx,byte ptr[c_flag]
       xor ecx,1
       bt  cx,0
       sbb  eax,edx
       sets byte ptr[n_flag]
       setz byte ptr[z_flag]
       setnc byte ptr[c_flag]
       seto byte ptr[v_flag]
       ret
   }
   return 0;
#elif defined(__GNUC__)
	u32 ret;

	__asm__ __volatile__(
       "movzbl %1,%%ecx\n"
       "xorl $1,%%ecx\n"
 		"btw $0,%%cx\n"
		"sbbl %%edx,%%eax\n"
		"setsb %2\n"
		"setzb %3\n"
		"setncb %1\n"
		"setob %4\n"
		"movl %%eax,%0\n"
		: "=m" (ret) : "m"(c_flag),"m"(n_flag),"m"(z_flag),"m"(v_flag)
	);
	return ret;
#endif
}
//---------------------------------------------------------------
#ifndef __BORLANDC__
NO_IN_LINE static void I_FASTCALL SET_DP_LOG_FLAGS(u32 a)
{
#if defined(__BORLANDC__) || defined(__WATCOMC__)
   __asm{
       test eax,eax
       sets byte ptr[n_flag]
       setz byte ptr[z_flag]
   }
#elif defined(__GNUC__)
	__asm__ __volatile__ (
		"test %%eax,%%eax\n"
		"setsb %0\n"
		"setzb %1\n"
       : : "m" (n_flag), "m" (z_flag)
	);
#endif
}
#else
   #define SET_DP_LOG_FLAGS(a)\
       if(a);\
           asm sets byte ptr[n_flag];\
           asm setz byte ptr[z_flag];
#endif                                                                            
//---------------------------------------------------------------
NO_IN_LINE static u32 DP_IMM_OPERAND(void)
{
#ifdef __BORLANDC__
   __asm{
       mov  ecx,[opcode]
       movzx eax,cl
       and  ecx,0F00h
       shr  ecx,7
       ror  eax,cl
   }
   return _EAX;
#elif defined(__WATCOMC__)
   __asm{
       mov  ecx,[opcode]
       movzx eax,cl
       and  ecx,0F00h
       shr  ecx,7
       ror  eax,cl
       ret
   }
   return 0;
#elif defined(__GNUC__)
	u32 ret;

	__asm__ __volatile__ (
		"movl %1,%%ecx\n"
		"movzbl %%cl,%%eax\n"
		"andl $3840,%%ecx\n"
		"shrl $7,%%ecx\n"
		"rorl %%cl,%%eax\n"
       "movl %%eax,%0"
		: "=m" (ret) : "m" (opcode)
	);
	return ret;
#endif
}
//---------------------------------------------------------------
NO_IN_LINE static u32 DP_IMM_OPERAND_UPD(void)
{
#ifdef __BORLANDC__
   __asm{
      mov  ecx,[opcode]
      movzx eax,cl
      and  ecx,0F00h
      shr  ecx,7
	   jz short @noupt
	   ror  eax,cl
	   setc byte ptr[c_flag]
	   @noupt:
   }
   return _EAX;                                    
#elif defined(__WATCOMC__)
   __asm{
      mov  ecx,[opcode]
      movzx eax,cl
      and  ecx,0F00h
      shr  ecx,7
      jz .noupt
      ror  eax,cl
      setc byte ptr[c_flag]
      .noupt:
      ret
   }
   return 0;
#elif defined(__GNUC__)
	u32 ret;

	__asm__ (
	   "movl %1,%%ecx\n"
	   "movzbl %%cl,%%edx\n"
	   "andl $3840,%%ecx\n"
	   "shrl $7,%%ecx\n"
	   "jz noupt\n"
	   "rorl %%cl,%%edx\n"
	   "setcb %2\n"
	   "noupt:\n"
      "movl %%edx,%0"
	   : "=m" (ret) : "m"(opcode),"m"(c_flag)
	);
	return ret;
#endif
}
//---------------------------------------------------------------------------
NO_IN_LINE static void fetch_arm()
{
   old_pc = gp_reg[15] - 4;
/*	if(!syscnt_can_operate(old_pc,SCNT_INS|SCNT_READ)){
		//supervisor
       old_pc = old_pc;
	}*/
   gp_reg[15] += 4;
   if(old_pc < sysCnt.itcm_end){
       if(!(sysCnt.reg1 & 0x80000)){
		    opcode = *((u32 *)(itcm_mem + (old_pc & 0x7FFC)));
           cyclesP = (u8)(waitState9[0]>>16);
           return;
       }
   }
   if(old_pc >= sysCnt.dtcm_address && old_pc < sysCnt.dtcm_end){
   	if(!(sysCnt.reg1 & 0x20000)){
			opcode = *((u32 *)(dtcm_mem + (old_pc & 0x3FFC)));
           cyclesP = (u8)(waitState9[1]>>16);
           return;
       }
   }
	switch(old_pc >> 24){
       case 2:
      		opcode = (u32)*((u32 *)(ext_mem + (old_pc & (ulMMMask & ~3))));
		break;
       case 3:
       	if((old_pc & 0x800000))
			    opcode = (u32)*((u32 *)(int_mem2 + (old_pc & 0xFFFC)));
           else
				opcode = (u32)*((u32 *)(int_mem + (old_pc & 0x7FFC)));
       break;
		case 4:
		break;
       case 5:
       break;
		case 6:
			opcode = (u32)*((u32 *)(video_mem + ((VRAMmap[(old_pc>>14)&0x3FF]<<14)+(old_pc&0x3FFC))));
       break;
       case 7:
       break;
		case 8:
       case 9:
           if(rom_pack[(old_pc>>16)&0x1FF] == NULL)
               opcode = (u32)0x0;
           else
       		opcode = (*((u32 *)(rom_pack[(old_pc>>16)&0x1FF] + (old_pc & 0xFFFC))));
       break;
		default:
			opcode = (u32)*((u32 *)(bios9_mem + (old_pc & 0x7FFC)));
       break;
   }
   cyclesP = (u8)(waitState9[old_pc>>24] >> 17);
}
//---------------------------------------------------------------------------
NO_IN_LINE static void fetch_thumb()
{
   old_pc = gp_reg[15] - 2;
/*	if(!syscnt_can_operate(old_pc,SCNT_INS|SCNT_READ)){
		//supervisor
		old_pc = old_pc;
	}*/
   gp_reg[15] += 2;
   if(old_pc < sysCnt.itcm_end){
       if(!(sysCnt.reg1 & 0x80000)){
		    opcode = (u32)*((u16 *)(itcm_mem + (old_pc & 0x7FFE)));
           cyclesP = (u8)(waitState9[0]>>16);
           return;
       }
   }
  	if(old_pc >= sysCnt.dtcm_address && old_pc < sysCnt.dtcm_end){
  		if(!(sysCnt.reg1 & 0x20000)){
           cyclesP = (u8)(waitState9[1]>>16);
			opcode = (u32)*((u16 *)(dtcm_mem + (old_pc & 0x3FFE)));
           return;
       }
   }
	switch(old_pc >> 24){
       case 2:
      		opcode = (u32)*((u16 *)(ext_mem + (old_pc & (ulMMMask & ~1))));
       break;
       case 3:
       	if((old_pc & 0x800000))
			    opcode = (u32)*((u16 *)(int_mem2 + (old_pc & 0xFFFE)));
           else
				opcode = (u32)*((u16 *)(int_mem + (old_pc & 0x7FFE)));
       break;
       case 4:
       break;
       case 5:
       break;
		case 6:
           opcode = (u32)*((u16 *)(video_mem + ((VRAMmap[(old_pc >> 14) & 0x3FF] << 14) + (old_pc & 0x3FFE))));
       break;
       case 7:
       break;
		case 8:
       case 9:
           if(rom_pack[(old_pc >> 16) & 0x1FF] == NULL)
               opcode = (u32)0;
           else
       		opcode = (u32)*((u16 *)(rom_pack[(old_pc >> 16) & 0x1FF] + (old_pc & 0xFFFE)));
       break;
       default:
			opcode = (u32)*((u16 *)(bios9_mem + (old_pc & 0x7FFE)));
       break;
   }
   cyclesP = (u8)(waitState9[old_pc >> 24] >> 17);
}
//---------------------------------------------------------------------------
NO_IN_LINE static u16 exec_dma_loop(void)
{
   int i,i1,res;

   res = 15;
   for(i1=i=0;i<4;i++){
       if(dmas[i+4].latency == 0)
           continue;
       i1++;
       if(dmas[i+4].latency > res)
           dmas[i+4].latency -= res;
       else{
           dmas[i+4].latency = 0;
           ExecDMA((u8)((i + 4)|0xF0));
       }
   }
   if(!i1)
       exec9 = exec9_i;
   return (u16)res;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u16 exec_thumb(void)
{
	u16 res;

	if(stop){
#ifdef _DEBUG
       if(!debugDlg.is_Active())
#endif
      	    ds.bDouble = 0;
       if(arm7.r_stop()){
		 	if(ds.get_Optimize() & 1){
       		res = (u16)ds.get_CurrentLineCycles();
               ds.set_StopCycles(1);
           	if(res < LINE_CYCLES_VISIBLE * 2)
           		return (u16)(LINE_CYCLES_VISIBLE * 2 - res);
           	return (u16)(LINE_CYCLES * 2 - res);
           }
     		return (u16)((waitState9[256] >> 16) * 16);
       }
       return (u16)(waitState9[256] >> 16);
   }
   res = (u16)thumb_ins[opcode >> 6]();
   fetch_func();
   if(sysCnt.loop){
      	ds.bDouble = 0;
       if(sysCnt.loop == -1){
           res = (u16)ds.get_CurrentLineCycles();
           ds.set_StopCycles(1);
           if(res < LINE_CYCLES_VISIBLE * 2)
           	res = (u16)(LINE_CYCLES_VISIBLE*2 - res);
           else
           	res = (u16)(LINE_CYCLES*2 - res);
       }
       else
   	    res <<= sysCnt.loop;
   }
   return (u16)res;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u16 exec_arm(void)
{                               //2115e84
	u16 run;

	if(stop){
#ifdef _DEBUG
       if(!debugDlg.is_Active())
#endif
      	    ds.bDouble = 0;
       if(arm7.r_stop()){
		 	if(ds.get_Optimize() & 1){
       		run = (u16)ds.get_CurrentLineCycles();
               ds.set_StopCycles(1);
           	if(run < LINE_CYCLES_VISIBLE * 2)
           		return (u16)(LINE_CYCLES_VISIBLE*2 - run);
           	return (u16)(LINE_CYCLES*2 - run);
           }
      		return (u16)((waitState9[256] >> 16) * 16);
		}
       return (u16)(waitState9[256] >> 16);
	}
#ifdef _LINO
   if((opcode & 0xFFF000F0) == 0xE1200070)
       opcode = opcode;
   if((opcode & 0xF550F000) == 0xF550F000)
       opcode = opcode;
   if((opcode >> 28) == 0xF && (opcode & 0x0FF00000) != 0x0AF00000 && (opcode & 0x0FF00000) != 0x0A000000 && (opcode & 0x0FF00000) != 0x0BF00000 && (opcode & 0x0FF00000) != 0x0b000000)
       opcode = opcode;
   if((opcode & 0xF550F000) == 0xF550F000)
       opcode = opcode;
#endif
   run = cycP;
	switch(opcode >> 28){
		case 0:
           if(z_flag)
               run = arm_ins[((opcode & 0x0FF00000)>>16)|(((u8)opcode)>>4)]();
		break;
		case 1:
			if(!z_flag)
				run = arm_ins[((opcode & 0x0FF00000)>>16)|(((u8)opcode)>>4)]();
		break;
		case 2:
			if(c_flag) //CS
				run = arm_ins[((opcode & 0x0FF00000)>>16)|(((u8)opcode)>>4)]();
		break;
		case 3:
	  		if(!c_flag) //CC
	  			run = arm_ins[((opcode & 0x0FF00000)>>16)|(((u8)opcode)>>4)]();
	  	break;
		case 4:
	  		if(n_flag)
	  			run = arm_ins[((opcode & 0x0FF00000)>>16)|(((u8)opcode)>>4)]();
	  	break;
		case 5:
	  		if(!n_flag)
	  			run = arm_ins[((opcode & 0x0FF00000)>>16)|(((u8)opcode)>>4)]();
	  	break;
		case 6:
	  		if(v_flag)
	  			run = arm_ins[((opcode & 0x0FF00000)>>16)|(((u8)opcode)>>4)]();
	  	break;
		case 7:
	    	if(!v_flag)
	  	    	run = arm_ins[((opcode & 0x0FF00000)>>16)|(((u8)opcode)>>4)]();
		break;
		case 8:
			if(c_flag && !z_flag) // HI
				run = arm_ins[((opcode & 0x0FF00000)>>16)|(((u8)opcode)>>4)]();
		break;
		case 9:
			if(!c_flag || z_flag) // LS
	    		run = arm_ins[((opcode & 0x0FF00000)>>16)|(((u8)opcode)>>4)]();
		break;
		case 10:
			if(n_flag == v_flag) //GE
				run = arm_ins[((opcode & 0x0FF00000)>>16)|(((u8)opcode)>>4)]();
		break;
		case 11:
			if(n_flag != v_flag)
				run = arm_ins[((opcode & 0x0FF00000)>>16)|(((u8)opcode)>>4)]();
		break;
		case 12:
			if(!z_flag && n_flag == v_flag)
				run = arm_ins[((opcode & 0x0FF00000)>>16)|(((u8)opcode)>>4)]();
		break;
   	case 13:
			if(z_flag || n_flag != v_flag) //LE
				run = arm_ins[((opcode & 0x0FF00000)>>16)|(((u8)opcode)>>4)]();
		break;
		default:
       	run = arm_ins[((opcode & 0x0FF00000)>>16)|(((u8)opcode)>>4)]();
       break;
   }
   fetch_func();
   if(sysCnt.loop){
      	ds.bDouble = 0;
       if(sysCnt.loop == -1){
           run = (u16)ds.get_CurrentLineCycles();
           ds.set_StopCycles(1);
           if(run < LINE_CYCLES_VISIBLE * 2)
           	run = (u16)(LINE_CYCLES_VISIBLE * 2 - run);
           else
           	run = (u16)(LINE_CYCLES * 2 - run);
       }
       else
   	    run <<= sysCnt.loop;
   }
	return (u16)run;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u16 exec_arm7(void)
{
   u16 res;

   res = exec7();
   ds.bDouble = 1;
   return res;
}
#if defined(_DEBPRO)
//---------------------------------------------------------------------------
NO_IN_LINE static void del_CallStack(u32 address)
{
	unsigned long i,i1;

   address &= ~1;
   if((i = callStack.count()) != 0){
       for(;i > 0;i--){
           i1 = callStack[i];
           if(i1 == address)
               break;
       }
       if(i1 == address){
           for(i = callStack.count();i > 0;i--){
               i1 = callStack[i];
               callStack.pop();
               if(i1 == address)
                   break;
           }
           return;
       }
   }
   callStack.push(address);
}
#endif
//---------------------------------------------------------------------------
NO_IN_LINE static u32 get_cpsr()
{
	u32 value;

   value = (cpsr & 0x1F) | (cpsr & T_BIT) | (cpsr & IRQ_BIT) | (cpsr & FIQ_BIT);
  	if(c_flag)
   	value |= C_BIT;
   if(n_flag)
   	value |= N_BIT;
   if(z_flag)
   	value |= Z_BIT;
   if(v_flag)
   	value |= V_BIT;
   if(q_flag)
   	value |= Q_BIT;
	return value;
}
//---------------------------------------------------------------------------
NO_IN_LINE static void set_cpsr(u32 value)
{
	cpsr = (value & 0x1F) | (value & T_BIT) | (value & IRQ_BIT) | (value & FIQ_BIT);
 	c_flag = (u8)((value >> C_SHIFT) & 1);
 	z_flag = (u8)((value >> Z_SHIFT) & 1);
 	v_flag = (u8)((value >> V_SHIFT) & 1);
 	n_flag = (u8)((value >> N_SHIFT) & 1);
   q_flag = (u8)((value >> Q_SHIFT) & 1);
}
//---------------------------------------------------------------------------
NO_IN_LINE static int regcpy(u8 to)
{
   int i;

   i = 13;
   if(to){
       switch(cpsr & 0x1f){
           case USER_MODE:
           case SYSTEM_MODE:
               memcpy(reg[0],gp_reg,sizeof(gp_reg));
           break;
           case UNDEFINED_MODE:
               memcpy(reg[5],gp_reg,sizeof(gp_reg));
           break;
           case ABORT_MODE:
               memcpy(reg[4],gp_reg,sizeof(gp_reg));
           break;
           case IRQ_MODE:
               memcpy(reg[1],gp_reg,sizeof(gp_reg));
           break;
           case SUPERVISOR_MODE:
               memcpy(reg[2],gp_reg,sizeof(gp_reg));
           break;
           case FIQ_MODE:
               memcpy(reg[3],gp_reg,sizeof(gp_reg));
               i = 8;
           break;
       }
   }
   else{
       switch(cpsr & 0x1f){
           case USER_MODE:
           case SYSTEM_MODE:
               memcpy(gp_reg,reg[0],sizeof(gp_reg));
           break;
           case UNDEFINED_MODE:
               memcpy(gp_reg,reg[5],sizeof(gp_reg));
           break;
           case ABORT_MODE:
               memcpy(gp_reg,reg[4],sizeof(gp_reg));
           break;
           case IRQ_MODE:
               memcpy(gp_reg,reg[1],sizeof(gp_reg));
           break;
           case SUPERVISOR_MODE:
               memcpy(gp_reg,reg[2],sizeof(gp_reg));
           break;
           case FIQ_MODE:
               memcpy(gp_reg,reg[3],sizeof(gp_reg));
           break;
       }
   }
   return i;
}
//---------------------------------------------------------------------------
NO_IN_LINE static void switchmode(u8 mode,u8 to)
{
	u32 *p,ss;
   int i;

   p = gp_reg;
   if(!to)
   	mode = (u8)((ss = gp_reg[16]) & 0x1F);
   else
		ss = 0;
   if((u8)(cpsr & 0x1F) == (mode & 0x1F))
   	goto switchmode_1;
   i = regcpy(1);
	switch(mode){
   	case USER_MODE:
       case SYSTEM_MODE:
			p = reg[0];
       break;
       case IRQ_MODE:
       	p = reg[1];
       break;
       case SUPERVISOR_MODE:
       	p = reg[2];
       break;
       case FIQ_MODE:
       	p = reg[3];
           i = 8;
       break;
       case ABORT_MODE:
       	p = reg[4];
       break;
       case UNDEFINED_MODE:
       	p = reg[5];
       break;
   }
   memcpy(p,gp_reg,sizeof(u32)*i);
	p[15] = gp_reg[15];
switchmode_1:
   if(to){
	    p[16] = get_cpsr();
       cpsr = (cpsr & ~0x1F) | (mode & 0x1F);
   }
   else
       set_cpsr(ss);
   memcpy(gp_reg,p,sizeof(gp_reg));
}
//---------------------------------------------------------------------------
NO_IN_LINE static int reload_base(BOOL bSwitch)
{
	u8 i;

   if(bSwitch && (i = (u8)(cpsr & 0x1F)) != SYSTEM_MODE && i != USER_MODE)
   	switchmode(0,0);
   if((cpsr & T_BIT)){
   	if(exec9 != exec_thumb){
           gp_reg[15] &= ~1;
           gp_reg[15] += 2;
       	exec9 = exec_thumb;
           fetch_func = fetch_thumb;
           return TRUE;
       }
   }
   else{
   	if(exec9 != exec_arm){
       	gp_reg[15] += 4;
       	exec9 = exec_arm;
           fetch_func = fetch_arm;
           return TRUE;
       }
   }
	return FALSE;
}
//---------------------------------------------------------------------------
void arm9irqvblank(void)
{
   *((u32 *)(dtcm_mem+0x3FF8)) |= 1;
}
//---------------------------------------------------------------------------
NO_IN_LINE void arm9irq(void)
{
	u8 tb;

   if(exec9 == exec_dma_loop)
       return;
   if(stop){
       stopped += ds.get_CurrentCycles() - fromStopped;
       stop = 0;
   }
//   if(*((u32 *)(dtcm_mem+0x3FFC)) == 0 && (sysCnt.reg1 & 0x2000) != 0)
//   	return;
   if((cpsr & IRQ_BIT) != 0)
   	return;
#ifdef _DEBPRO
	debugDlg.OnEnterIRQ(*((u32 *)&io_mem[0x210]) & *((u32 *)&io_mem[0x214]),9);
#endif
   tb = (u8)((cpsr & T_BIT) >> T_SHIFT);
   switchmode(IRQ_MODE,1);
   cpsr &= ~T_BIT;
   cpsr |= IRQ_BIT;
   exec9 = exec_arm;
	gp_reg[14] = gp_reg[15];
   if(!tb)
   	gp_reg[14] -= 4;
	gp_reg[15] = 0x1C;
	if((sysCnt.reg1 & 0x2000))
   	gp_reg[15] |= 0xFFFF0000;
	fetch_func = fetch_arm;
   fetch_func();
}
//---------------------------------------------------------------------------
NO_IN_LINE static void abort_nds(int type)
{
   u8 tb;

   tb = (u8)((cpsr & T_BIT)>>T_SHIFT);
   switchmode(ABORT_MODE,1);
   cpsr &= ~T_BIT;
   cpsr |= IRQ_BIT;
   exec9 = exec_arm;
   fetch_func = fetch_arm;
   gp_reg[14] = gp_reg[15];
   if(!tb)
   	gp_reg[14] -= 4;
   WriteMsgConsolle(&arm9,"%cARM9 : %s",MSGT_BIOS,(type == 0 ? "bkpt" : "data abort"));
   gp_reg[15] = 0x10;
   if((sysCnt.reg1 & 0x2000))
       gp_reg[15] |= 0xFFFF0000;
}
//---------------------------------------------------------------------------
NO_IN_LINE static void bios()
{
	u8 tb;

   tb = (u8)((cpsr & T_BIT)>>T_SHIFT);
	switchmode(SUPERVISOR_MODE,1);
   cpsr &= ~T_BIT;
   cpsr |= IRQ_BIT;
   exec9 = exec_arm;
   fetch_func = fetch_arm;
	gp_reg[14] = gp_reg[15] - 2;
   if(!tb)
   	gp_reg[14] -= 2;
	WriteMsgConsolle(&arm9,"%cARM9 : swi 0x%02X",MSGT_BIOS,(int)read_byte(gp_reg[14] - 2));
	gp_reg[15] = 0xC;
	if((sysCnt.reg1 & 0x2000))
   	gp_reg[15] |= 0xFFFF0000;
}
//---------------------------------------------------------------------------
NO_IN_LINE static void I_FASTCALL write_hword_data(u32 address,u32 data)
{
   RUN_CHECK_BRANCH(address)
   SYSCNT_CAN_OPERATE_WRITE(address,SCNT_DATA|SCNT_WRITE)
#if defined(_DEBPRO2)
   if(debugDlg.is_Active())
        ControlMemoryBP(address,MAKELONG(AMM_HWORD|AMM_WRITE,9),data);
#endif
    O_TCM(address,*((u16 *)(itcm_mem + (address & 0x7FFE))) = (u16)data,*((u16 *)(dtcm_mem + (address & 0x3FFE))) = (u16)data)
    switch(address >> 24){
       case 2:
           *((u16 *)(ext_mem + (address & (ulMMMask & ~1)))) = (u16)data;
           wait_state.cycles = waitState9[2] << 1;
       break;
       case 3:
           if((address & 0x800000))
               *((u16 *)(int_mem2 + (address & 0xFFFE))) = (u16)data;
           else
               *((u16 *)(int_mem + (address & 0x7FFE))) = (u16)data;
           wait_state.cycles = waitState9[3];
       break;
       case 4:
           if(address & 0x100000)
               address = 0x2000 + (address & 0xFE);
           else if(address & 0x4000)
               address = 0x2800 + (address & 0x3FE);
           else
               address &= 0x1FFE;
           *((u16 *)(io_mem + address)) = (u16)data;
           i_func[address](address,data,AMM_HWORD);
           wait_state.cycles = waitState9[4];
       break;
       case 5:
           *((u16 *)(pal_mem + (address &= 0x7FE))) = (u16)data;
			pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_PALETTECHANGED,PIS_NOTIFYMASK,(LPARAM)(address << 16)|AMM_HWORD);
           wait_state.cycles = waitState9[5];
       break;
       case 6:
           wait_state.cycles = waitState9[6];
           if(address & 0x800000){
               u16 *p;

               p = ((u16 *)(video_mem + ((VRAMmap[(address>>14)&0x3FF]<<14)+(address & 0x3FFE))));
               if(*p != data){
                   *p = data;
                   video_cache[(address & 0xFFFFF) >> 5] |= (0x3 << (address & 0x1E));
               }
           }
           else
               *((u16 *)(video_mem + ((VRAMmap[(address>>14)&0x3FF]<<14)+(address & 0x3FFE)))) = (u16)data;
       break;
       case 7:
           *((u16 *)(obj_mem + (address & 0x7FE))) = (u16)data;
			pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_OAMCHANGED,PIS_NOTIFYMASK,(LPARAM)(address<<16)|AMM_HWORD);
           wait_state.cycles = waitState9[7];
       break;
       case 8:
       case 9:
           wait_state.cycles = waitState9[8];
           write_ram_block_hword(address,data);
       break;
       default:
           wait_state.cycles = waitState9[address>>24];
       break;
   }
}
//---------------------------------------------------------------------------
NO_IN_LINE static void I_FASTCALL write_byte_data(u32 address,u32 data)
{
   RUN_CHECK_BRANCH(address)
   SYSCNT_CAN_OPERATE_WRITE(address,SCNT_DATA|SCNT_WRITE)
#if defined(_DEBPRO2)
   if(debugDlg.is_Active())
	    ControlMemoryBP(address,MAKELONG(AMM_BYTE|AMM_WRITE,9),data);
#endif
	O_TCM(address,itcm_mem[address & 0x7FFF] = (u8)data,
		dtcm_mem[address & 0x3FFF] = (u8)data)
	switch((address >> 24)){
       case 2:
           wait_state.cycles = waitState9[2]<<2;
           ext_mem[address & ulMMMask] = (u8)data;
       break;
       case 3:
           wait_state.cycles = waitState9[3];
       	if((address & 0x800000)){
           	int_mem2[(u16)address] = (u8)data;
               return;
           }
       	int_mem[address&0x7FFF] = (u8)data;
       break;
       case 4:
           wait_state.cycles = waitState9[4];
           if((address & 0x00100000))
           	address = 0x2000 + (u8)address;
           else if(address & 0x4000)
				address = 0x2800 + (address & 0x3FF);
           else
				address &= 0x1FFF;
           io_mem[address] = (u8)data;
//           if(i_func[address] != NULL)
           i_func[address](address,data,AMM_BYTE);
       break;
       case 5:
           wait_state.cycles = waitState9[5];
       	pal_mem[address &= 0x7FF] = (u8)data;
			pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_PALETTECHANGED,PIS_NOTIFYMASK,(LPARAM)(address<<16)|AMM_BYTE);
       break;
       case 6:
           wait_state.cycles = waitState9[6];
           *(video_mem + ((VRAMmap[(address>>14)&0x3FF]<<14)+(address&0x3FFF))) = (u8)data;
       break;
       case 7:
           wait_state.cycles = waitState9[7];
       	obj_mem[address & 0x7FF] = (u8)data;
			pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_OAMCHANGED,PIS_NOTIFYMASK,(LPARAM)(address<<16)|AMM_BYTE);
       break;
       case 8:
       case 9:
           wait_state.cycles = waitState9[8];
           write_ram_block_byte(address,data);
       break;
       case 0xA:
           wait_state.cycles = waitState9[0xa];
           *(sram_mem + (u16)address) = (u8)data;
       break;
       default:
           wait_state.cycles = waitState9[address>>24];
       	data = data;
		break;
   }
}
//---------------------------------------------------------------------------
NO_IN_LINE static void I_FASTCALL write_word_data(u32 address,u32 data)
{
   RUN_CHECK_BRANCH(address)
   SYSCNT_CAN_OPERATE_WRITE(address,SCNT_DATA|SCNT_WRITE)
#if defined(_DEBPRO2)
   if(debugDlg.is_Active())
	    ControlMemoryBP(address,MAKELONG(AMM_WORD|AMM_WRITE,9),data);
#endif
	O_TCM(address,*((u32 *)(itcm_mem + (address & 0x7FFC))) = (u32)data,
		*((u32 *)(dtcm_mem + (address & 0x3FFC))) = (u32)data)
	switch((address >> 24)){
		case 2:
           wait_state.cycles = waitState9[2];
           *((u32 *)(ext_mem + (address & (ulMMMask & ~3)))) = data;
       break;
		case 3:
           wait_state.cycles = waitState9[3];
       	if((address & 0x800000)){
           	*((u32 *)(int_mem2 + (address & 0xFFFC))) = data;
               return;
           }
       	*((u32 *)(int_mem + (address & 0x7FFC))) = data;
       break;
       case 4:
           wait_state.cycles = waitState9[4];
           if((address & 0x00100000))
               address = 0x2000 + (address & 0xFC);
           else if(address & 0x4000)
               address = 0x2800 + (address & 0x3FC);
           else
               address &= 0x1FFC;
           *((u32 *)(io_mem + address)) = data;
               //           if(i_func[address] != NULL)
           i_func[address](address,data,AMM_WORD);
       break;
       case 5:
           wait_state.cycles = waitState9[5];
			*((u32 *)(pal_mem + (address &= 0x7FC))) = data;
			pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_PALETTECHANGED,PIS_NOTIFYMASK,(LPARAM)(address<<16)|AMM_WORD);
       break;
       case 6:
           wait_state.cycles = waitState9[6];
           if((address & 0xF00000) == 0x800000){
               u32 *p;

               p = ((u32 *)(video_mem + ((VRAMmap[(address>>14) & 0x3FF] << 14) + (address & 0x3FFC))));
               if(*p != data){
                   *p = data;
                   video_cache[(address & 0xFFFFF) >> 5] |= (0xF << (address & 0x1C));
               }
           }
           else
               *((u32 *)(video_mem + ((VRAMmap[(address>>14) & 0x3FF] << 14) + (address & 0x3FFC)))) = data;
       break;
       case 7:
           wait_state.cycles = waitState9[7];
			*((u32 *)(obj_mem + (address & 0x7FC))) = data;
			pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_OAMCHANGED,PIS_NOTIFYMASK,(LPARAM)(address << 16)|AMM_WORD);
       break;
       case 8:
       case 9:
           wait_state.cycles = waitState9[8];
           write_ram_block_word(address,data);
       break;
       default:
           wait_state.cycles = waitState9[address>>24];
       	data = data;
		break;
   }
}
//---------------------------------------------------------------------------
NO_IN_LINE static u32 I_FASTCALL read_word_data(u32 address)
{
   RUN_CHECK_BRANCH(address)
   SYSCNT_CAN_OPERATE_READ(address,SCNT_DATA|SCNT_READ)
#if defined(_DEBPRO2)
   if(debugDlg.is_Active())
	    ControlMemoryBP(address,MAKELONG(AMM_WORD|AMM_READ,9));
#endif
#ifdef _LINO
	if((address & 3) != 0 && (opcode & 0x0FF00000) != 0x05900000/* && (opcode & 0x0FF00000) != 0x04900000 && (opcode & 0x0FF00000) != 0x05b00000 && (opcode & 0x0FF00000) != 0x05300000*/)
   	address = address;
#endif
	I_TCM(address,*((u32 *)(itcm_mem + (address & 0x7FFC))),*((u32 *)(dtcm_mem + (address & 0x3FFC))))
	switch(address >> 24){
		case 2:
           wait_state.cycles = waitState9[2];
     	    return (u32)*((u32 *)(ext_mem + (address & (ulMMMask & ~3))));
       case 3:
           wait_state.cycles = waitState9[3];
       	if((address & 0x800000))
			    return (u32)*((u32 *)(int_mem2 + (address & 0xFFFC)));
           return (u32)*((u32 *)(int_mem + (address & 0x7FFC)));
		case 4:
           wait_state.cycles = waitState9[4];
           if((address & 0x00100000))
           	address = 0x2000 + (address & 0xFC);
           else if((address & 0x00004000))
           	address = 0x2800 + (address & 0x3FC);
           else
				address &= 0x1FFC;
           if(o_func[address] != NULL)
               return o_func[address](address,AMM_WORD);
           return *(u32 *)(io_mem + address);
       case 5:
           wait_state.cycles = waitState9[5];
			return (u32)*((u32 *)(pal_mem + (address & 0x7FC)));
		case 6:
           wait_state.cycles = waitState9[6];
           return (u32)*((u32 *)(video_mem + ((VRAMmap[(address>>14)&0x3FF]<<14)+(address&0x3FFC))));
       case 7:
           wait_state.cycles = waitState9[7];
			return (u32)*((u32 *)(obj_mem + (address & 0x7FC)));
		case 8:
       case 9:
           wait_state.cycles = waitState9[8];
           if(rom_pack[(address>>16)&0x1FF] == NULL)
               return (u32)0x0;
       	return (*((u32 *)(rom_pack[(address>>16)&0x1FF] + (address & 0xFFFC))));
       default:
           wait_state.cycles = waitState9[address >> 24];
			return (u32)*((u32 *)(bios9_mem + (address & 0x7FFC)));
   }
}
//---------------------------------------------------------------------------
NO_IN_LINE static u32 I_FASTCALL read_hword_data(u32 address)
{
   RUN_CHECK_BRANCH(address)
   SYSCNT_CAN_OPERATE_READ(address,SCNT_DATA|SCNT_READ)
#if defined(_DEBPRO2)
   if(debugDlg.is_Active())
	    ControlMemoryBP(address,MAKELONG(AMM_HWORD|AMM_READ,9));
#endif
#ifdef _DEBPRO
	if((address & 1) != 0/* && (opcode & 0x0FF00000) != 0x01900000 && (opcode & 0x0FF00000) != 0x01D00000*/)
   	address = address;
#endif
	I_TCM(address,*((u16 *)(itcm_mem + (address & 0x7FFE))),*((u16 *)(dtcm_mem + (address & 0x3FFE))))
	switch(address >> 24){
       case 2:
           wait_state.cycles = waitState9[2];
      	    return (u32)*((u16 *)(ext_mem + ((address & ~1) & ulMMMask)));
       case 3:
           wait_state.cycles = waitState9[3];
       	if((address & 0x800000))
			    return (u32)*((u16 *)(int_mem2 + (address & 0xFFFE)));
			return (u32)*((u16 *)(int_mem + (address & 0x7FFE)));
       case 4:
           wait_state.cycles = waitState9[4];
           if((address & 0x00100000))
           	address = 0x2000 + (address & 0xFE);
           else if((address & 0x00004000))
           	address = 0x2800 + (address & 0x3FE);
           else
           	address &= 0x1FFE;
           if(o_func[address] != NULL)
               return (u32)(u16)o_func[address](address,AMM_HWORD);
           return (u32)*(u16 *)(io_mem + address);
       case 5:
           wait_state.cycles = waitState9[5];
			return (u32)*((u16 *)(pal_mem + (address & 0x7FE)));
		case 6:
           wait_state.cycles = waitState9[6];
           return (u32)*((u16 *)(video_mem + ((VRAMmap[(address>>14) & 0x3FF]<<14)+(address & 0x3FFE))));
       case 7:
           wait_state.cycles = waitState9[7];
			return (u32)*((u16 *)(obj_mem + (address & 0x7FE)));
		case 8:
       case 9:
           wait_state.cycles = waitState9[8];
           switch((u8)ulExRamPack){
               case 2:
                   if(address < 0x08000008 || address == 0x0801FFFE)
                       return 0xF9FF;
               break;
           }
           if(rom_pack[(address>>16)&0x1FF] == NULL)
               return (u32)0;
       	return (u32)*((u16 *)(rom_pack[(address>>16)&0x1FF] + (address & 0xFFFE)));
       break;
       default:
           wait_state.cycles = waitState9[address>>24];
			return (u32)*((u16 *)(bios9_mem + (address & 0x7FFE)));
  }
}
//---------------------------------------------------------------------------
NO_IN_LINE static u32 I_FASTCALL read_byte_data(u32 address)
{
   RUN_CHECK_BRANCH(address)
   SYSCNT_CAN_OPERATE_READ(address,SCNT_DATA|SCNT_READ)
#if defined(_DEBPRO2)
   if(debugDlg.is_Active())
	    ControlMemoryBP(address,MAKELONG(AMM_BYTE|AMM_READ,9));
#endif
	I_TCM(address,itcm_mem[address & 0x7FFF],dtcm_mem[address & 0x3FFF])
	switch(address >> 24){
       case 2:
           wait_state.cycles = waitState9[2]<<2;
           return (u32)ext_mem[address & ulMMMask];
       case 3:
           wait_state.cycles = waitState9[3];
       	if((address & 0x800000))
           	return (u32)int_mem2[(u16)address];
       	return (u32)int_mem[address & 0x7FFF];
       case 4:
           wait_state.cycles = waitState9[4];
           if((address & 0x00100000))
           	address = 0x2000 + (u8)address;
           else if((address & 0x00004000))
           	address = 0x2800 + (address & 0x3FF);
           else
           	address &= 0x1FFF;
           if(o_func[address] != NULL)
               return o_func[address](address,AMM_BYTE);
           return (u32)io_mem[address];
       case 5:
           wait_state.cycles = waitState9[5];
			return (u32)pal_mem[address & 0x7FF];
		case 6:
           wait_state.cycles = waitState9[6];
           return (u32)video_mem[((VRAMmap[(address>>14)&0x3FF]<<14)+(address&0x3FFF))];
       case 7:
           wait_state.cycles = waitState9[7];
       	return (u32)obj_mem[address & 0x7FF];
		case 8:
       case 9:
           wait_state.cycles = waitState9[8];
           if(rom_pack[(address>>16)&0x1FF] == NULL)
               return (u32)0;
       	return (u32)(rom_pack[(address>>16)&0x1FF][(u16)address]);
       case 0xA:
           wait_state.cycles = waitState9[0xA];
           switch((u8)ulExRamPack){
               case 2:
                   if(address == 0x0A000000)
                       return (u8)ds.get_GripKeys();
               break;
           }
           return sram_mem[(u16)address];
       default:
           wait_state.cycles = waitState9[address>>24];
			return (u32)*((u8 *)(bios9_mem + (address & 0x7FFF)));
   }
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 unknown_ins(void)
{
#ifdef _DEBPRO
	EnterDebugMode(FALSE);
#endif
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 ldc(void)
{
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 stc(void)
{
	u8 cp;

   cp = (u8)((opcode >> 8) & 0xF);
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 mcr(void)
{
   syscnt_write(opcode);
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 mrc(void)
{
	DEST_REG = syscnt_read(opcode);
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 cdp(void)
{
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 clz(void)
{
#if defined(__BORLANDC__) || defined(__WATCOMC__)
   __asm{
		mov edx,dword ptr [opcode]
		mov eax,32
		and edx,15
	    mov edx, dword ptr[gp_reg+4*edx]
		test edx,edx
		jz short @nobsr
		db 0xf
		db 0xbd
		db 0xc2
		mov edx,31
		sub edx,eax
		mov eax,edx
		@nobsr:
		movzx ecx,word ptr [opcode]
		shr ecx,12
	    mov dword ptr[gp_reg+4*ecx],eax
	}
#elif defined(__GNUC__)
	__asm__ __volatile__(
 		"movl %0,%%edx\n"
		"andl $15,%%edx\n"
 		"movl _ZL6gp_reg(,%%edx,4),%%edx\n"
        "movl $32,%%eax\n"
 		"testl %%edx,%%edx\n"
 		"jz nobsf\n"
 		"bsr %%edx,%%eax\n"
 		"movl $31,%%edx\n"
 		"subl %%eax,%%edx\n"
 		"movl %%edx,%%eax\n"
 		"nobsf:\n"
 		"movzwl %0,%%ecx\n"
 		"shrl $12,%%ecx\n"
 		"movl %%eax,_ZL6gp_reg(,%%ecx,4)\n"
       : : "m" (opcode)
 	);
#endif
	return cycP;
}
//---------------------------------------------------------------
NO_IN_LINE static u32 DP_REG_OPERAND_IMM(void)
{
   u8 shift;

   switch((opcode & 0x60) >> 5){
       case 0: //LSL
           return OP_REG << IMM_SHIFT;
       case 1: // LSR
           if((shift = (u8)IMM_SHIFT) == 0)
               return 0;
           return OP_REG >> shift;
       case 2://asr
           if((shift = (u8)IMM_SHIFT) == 0){
               if((OP_REG & 0x80000000))
                   return 0xFFFFFFFF;
               return 0;
           }
           return (signed long)OP_REG >> shift;
       case 3://ROR
           if((shift = (u8)IMM_SHIFT) == 0)
               return ((OP_REG >> 1)|(c_flag << 31));
#if defined(__BORLANDC__)
           _EAX = OP_REG;
           _CL = shift;
           __asm ror eax,cl
           return _EAX;
#elif defined(__GNUC__) || defined(__WATCOMC__)
           return ((OP_REG << (32-shift))|(OP_REG>>shift));
#endif
   }
   return 0;
}
//---------------------------------------------------------------
NO_IN_LINE static u32 DP_REG_OPERAND(void)
{
   u8 shift;
   u32 res;

   res = OP_REG_INDEX;
   res = (res == 15) ? gp_reg[15] + 4 : gp_reg[res];
   switch((opcode & 0x60) >> 5){
       case 0: //LSL
           if((shift = (u8)SHFT_AMO_REG) == 0)
               return res;
           if(shift < 32)
               return res << shift;
           return 0;
       case 1: // LSR
           if((shift = (u8)SHFT_AMO_REG) == 0)
               return res;
           if(shift < 32)
               return res >> shift;
           return 0;
       case 2://asr
           if((shift = (u8)SHFT_AMO_REG) == 0)
               return res;
           if(shift < 32)
               return (signed long)res >> shift;
           if((res & 0x80000000))
               return 0xFFFFFFFF;
           return 0;
       case 3://ROR
#if defined(__BORLANDC__)
           if((shift = (u8)SHFT_AMO_REG) == 0)
               return res;
           if(shift < 32){
               _EAX = res;
               _CL = shift;
               __asm ror eax,cl
               return _EAX;
           }
           if(shift == 32)
               return res;
           shift &= 0x1F;
           _EAX = res;
           _CL = shift;
           __asm ror eax,cl
           return _EAX;
#elif defined(__GNUC__) || defined(__WATCOMC__)
			if((shift = (u8)SHFT_AMO_REG) == 0)
           	return res;
			if(shift < 32)
               return ((res << (32-shift))|(res>>shift));
           if(shift == 32)
               return res;
           shift &= 0x1F;
           return ((res << (32-shift))|(res>>shift));
#endif
   }
   return 0;
}
//---------------------------------------------------------------
NO_IN_LINE static u32 I_FASTCALL DP_REG_OPERAND_UPD(u8 shift)
{
   u32 temp_reg;

   temp_reg = OP_REG_INDEX;
   temp_reg = (temp_reg == 15) ? gp_reg[15] + 4 : gp_reg[temp_reg];
   switch((opcode & 0x60) >> 5){
       case 0: //LSL
           if(!shift)
               return temp_reg;
           else if(shift < 32){
               temp_reg <<= shift;
#if defined(__GNUC__)
				__asm__ __volatile__("setcb %0\n" : : "m" (c_flag));
#else
               __asm setc byte ptr[c_flag]
#endif
           }
           else if(shift == 32){
               c_flag = (u8)(temp_reg & 1);
               temp_reg = 0;
           }
           else{
               temp_reg = 0;
               c_flag = (u8)0;
           }
       break;
       case 1: // LSR
           if(!shift){
               if(!(opcode & 0x10)){
                   c_flag = (u8)(temp_reg >> 31);
                   temp_reg = 0;
               }
           }
           else if(shift < 32){
               temp_reg >>= shift;
#ifdef __BORLANDC__
               __asm setc byte ptr[c_flag]
#else
				__asm__ __volatile__("setcb %0\n" : : "m" (c_flag));
#endif
           }
           else if(shift == 32){
               c_flag = (u8)(temp_reg >> 31);
               temp_reg = 0;
           }
           else{
               temp_reg = 0;
               c_flag = 0;
           }
           return temp_reg;
       case 2://asr
           if(!shift){
               if(!(opcode & 0x10)){
                   if((temp_reg & 0x80000000)){
                       temp_reg = 0xFFFFFFFF;
                       c_flag = (u8)1;
                   }
                   else{
                       temp_reg = 0;
                       c_flag = (u8)0;
                   }
               }
           }
           else if(shift < 32){
               temp_reg = (signed long)temp_reg >> shift;
#ifdef __BORLANDC__
               __asm setc byte ptr[c_flag]
#else
				__asm__ __volatile__("setcb %0\n" : : "m" (c_flag));
#endif
           }
           else{
               if((temp_reg & 0x80000000)){
                   temp_reg = 0xFFFFFFFF;
                   c_flag = (u8)1;
               }
               else{
                   temp_reg = 0;
                   c_flag = (u8)0;
               }
           }
       break;
       case 3://ROR
           if(!shift){
               if(!(opcode & 0x10)){ //RRX
                   shift = (u8)(temp_reg & 1);
                   temp_reg = ((temp_reg>>1)|(c_flag<<31));
                   c_flag = shift;
               }
           }
           else if(shift < 32){

#ifdef __BORLANDC__
               _CL = _AL;
               _EAX = temp_reg;
               __asm{
                   ror eax,cl
                	setc byte ptr[c_flag]
               }
               temp_reg = _EAX;
#else
               __asm__ __volatile__ (
               	"movl %1,%%eax\n"
               	"movb %2,%%cl\n"
               	"rorl %%cl,%%eax\n"
               	"setcb %3\n"
               	"movl %%eax,%0"
               	: "=m" (temp_reg) : "m" (temp_reg), "m" (shift), "m"(c_flag)
               );
#endif
           }
           else if(shift == 32)
               c_flag = (u8)(temp_reg >> 31);
           else{
#ifdef __BORLANDC__
               _CL = (u8)(_AL & 31);
               _EAX = temp_reg;
               __asm{
                   ror eax,cl
                	setc byte ptr[c_flag]
               }
               temp_reg = _EAX;
#else
               __asm__ __volatile__ (
               	"movl %1,%%eax\n"
               	"movb %2,%%cl\n"
               	"rorl %%cl,%%eax\n"
               	"setcb %3\n"
               	"movl %%eax,%0"
               	: "=m" (temp_reg) : "m" (temp_reg), "m" (shift), "m" (c_flag)
               );
#endif
           }
       break;
  	}
   return temp_reg;
}
//--------------------------------------------------------------------------------------
NO_IN_LINE static u8 ins_bmi(void)
{
   int adr;

   if(((adr = (opcode & 0xFFFFFF)) & 0x800000))
       adr = 0 - (0x1000000 - adr);
   {
       u32 jump_adr;

       jump_adr = gp_reg[15] + (adr << 2);
       if(jump_adr > 0x02FFFFFF && jump_adr < 0x08000000)
           return (u8)(cycS + cycS + cycP);
   }
	if((opcode >> 28) == 0xF){
       gp_reg[14] = gp_reg[15] - 4;
#if defined(_DEBPRO)
 		del_CallStack(gp_reg[14]);
#endif
   	gp_reg[15] += (adr << 2) + ((opcode & 0x1000000) >> 23) + 2;
       cpsr |= T_BIT;
       exec9 = exec_thumb;
       fetch_func = fetch_thumb;
   }
   else{
		gp_reg[15] += (adr << 2) + 4;
       CHECK_BRANCH(adr)
   }
	return (u8)(cycS + cycS + cycP);
}
//--------------------------------------------------------------------------------------
NO_IN_LINE static u8 ins_blmi(void)
{
   int adr;

	gp_reg[14] = gp_reg[15] - 4;
#if defined(_DEBPRO)
   del_CallStack(gp_reg[14]);
#endif
   if(((adr = (opcode&0xFFFFFF)) & 0x800000))
       adr = 0 - (0x1000000 - adr);
   if((opcode >> 28) == 0xF){
		gp_reg[15] += (adr << 2) + (opcode & 0x1000000 ? 4 : 2);
       cpsr |= T_BIT;
       exec9 = exec_thumb;
		fetch_func = fetch_thumb;
   }
   else
		gp_reg[15] += (adr << 2) + 4;
	return (u8)(cycS + cycS + cycP);
}
//--------------------------------------------------------------------------------------
NO_IN_LINE static u8 ins_blx(void)
{
   u32 adr;

   adr = gp_reg[15] - 4;
#if defined(_DEBPRO)
	del_CallStack(adr);
#endif
	gp_reg[15] = OP_REG;
   gp_reg[14] = adr;
	if(gp_reg[15] > 0x04000000 && gp_reg[15] < 0xF0000000){
   	gp_reg[15] = gp_reg[14];
   }
	if((gp_reg[15] & 1)) {
		cpsr |= T_BIT;
       exec9 = exec_thumb;
       gp_reg[15] &= ~1;
       gp_reg[15] += 2;
		fetch_func = fetch_thumb;
	}
   else
   	gp_reg[15] += 4;
	return (u8)(cycS + cycS + cycP);
}
//--------------------------------------------------------------------------------------
NO_IN_LINE static u8 ins_bx(void)
{
	gp_reg[15] = OP_REG;
#if defined(_DEBPRO)
	del_CallStack(gp_reg[15]);
#endif
	if((gp_reg[15] & 1)) {
		cpsr |= T_BIT;
       exec9 = exec_thumb;
       gp_reg[15] &= ~1;
       gp_reg[15] += 2;
		fetch_func = fetch_thumb;
	}
   else
   	gp_reg[15] += 4;
	return (u8)(cycS + cycS + cycP);
}
//--------------------------------------------------------------------------------------
NO_IN_LINE static u8 ins_mul(void)
{
   u8 m;
   u32 value;

	BASE_REG = OP_REG * (value = SHFT_AMO_REG);
   value >>= 8;
   if(value == 0 || value == 0xFFFFFF)
       m = 1;
   else{
       value >>= 8;
       if(value == 0 || value == 0xFFFF)
           m = 2;
       else{
           value >>= 8;
           m = (u8)((value == 0 || value == 0xFF) ? 3 : 4);
       }
   }
   return (u8)(cycP + m);
}
//--------------------------------------------------------------------------------------
NO_IN_LINE static u8 ins_muls(void)
{
   u8 m;
   u32 value;

	SET_DP_LOG_FLAGS((BASE_REG = OP_REG * (value = SHFT_AMO_REG)));
   value >>= 8;
   if(value == 0 || value == 0xFFFFFF)
       m = 1;
   else{
       value >>= 8;
       if(value == 0 || value == 0xFFFF)
           m = 2;
       else{
           value >>= 8;
           m = (u8)((value == 0 || value == 0xFF) ? 3 : 4);
       }
   }
   return (u8)(cycP + m);
}
//--------------------------------------------------------------------------------------
NO_IN_LINE static u8 ins_mla(void)
{
   u32 value;
   u8 m;

	BASE_REG = (u32)(OP_REG * (value = SHFT_AMO_REG) + DEST_REG);
   value >>= 8;
   if(value == 0 || value == 0xFFFFFF)
       m = 1;
   else{
       value >>= 8;
       if(value == 0 || value == 0xFFFF)
           m = 2;
       else{
           value >>= 8;
           m = (u8)((value == 0 || value == 0xFF) ? 3 : 4);
       }
   }
   return (u8)(cycP + m + cycI);
}
//--------------------------------------------------------------------------------------
NO_IN_LINE static u8 ins_mlas(void)
{
   u32 value;
   u8 m;

	SET_DP_LOG_FLAGS(BASE_REG = OP_REG * (value = SHFT_AMO_REG) + DEST_REG);
   value >>= 8;
   if(value == 0 || value == 0xFFFFFF)
       m = 1;
   else{
       value >>= 8;
       if(value == 0 || value == 0xFFFF)
           m = 2;
       else{
           value >>= 8;
           m = (u8)((value == 0 || value == 0xFF) ? 3 : 4);
       }
   }
   return (u8)(cycP + m);
}
//---------------------------------------------------------------
NO_IN_LINE static u8 ins_mull(void)
{
   s32 op1,op2;
   u32 value;
   u8 m;

   op1 = OP_REG;
   value = op2 = SHFT_AMO_REG;
#if defined(__BORLANDC__)
   __asm{
       mov eax,op1
       imul dword ptr[op2]
       mov dword ptr[op1],eax
       mov dword ptr[op2],edx
   }
#elif defined(__GNUC__)
	__asm__ __volatile__ (
		"movl %2,%%eax\n"
		"imull %3\n"
		"movl %%eax,%0\n"
		"movl %%edx,%1\n"
		: "=m" (op1),"=m" (op2) : "m" (op1), "m" (op2) : "eax","edx","memory"
	);
#endif
	LO_REG = (u32)op1;
	HI_REG = (u32)op2;
	value >>= 8;
   if(value == 0 || value == 0xFFFFFF)
       m = 1;
   else{
       value >>= 8;
       if(value == 0 || value == 0xFFFF)
           m = 2;
       else{
           value >>= 8;
           m = (u8)((value == 0 || value == 0xFF) ? 3 : 4);
       }
   }
   return (u8)(cycP + m);
}
//---------------------------------------------------------------
NO_IN_LINE static u8 ins_mulls(void)
{
   s32 op1,op2;
   u32 value;
   u8 m;

   op1 = OP_REG;
   value = op2 = SHFT_AMO_REG;
#if defined(__BORLANDC__)
   __asm{
       mov eax,op1
       imul dword ptr[op2]
       mov dword ptr[op1],eax
       mov dword ptr[op2],edx
   }
#elif defined(__GNUC__)
	__asm__ __volatile__ (
		"movl %2,%%eax\n"
		"imull %3\n"
		"movl %%eax,%0\n"
		"movl %%edx,%1\n"
		: "=m" (op1),"=m" (op2) : "m" (op1), "m" (op2) : "eax","edx","memory"
	);
#endif
	LO_REG = (u32)op1;
	HI_REG = (u32)op2;
	z_flag = (u8)(!op1 && !op2 ? 1 : 0);
   n_flag = (u8)(op2 >> 31);
   value >>= 8;
   if(value == 0 || value == 0xFFFFFF)
       m = 1;
   else{
       value >>= 8;
       if(value == 0 || value == 0xFFFF)
           m = 2;
       else{
           value >>= 8;
           m = (u8)((value == 0 || value == 0xFF) ? 3 : 4);
       }
   }
   return (u8)(cycP + m);
}
//---------------------------------------------------------------
NO_IN_LINE static u8 ins_mull_unsigned(void)
{
   u32 op1,op2,value;
   u8 m;

   op1 = OP_REG;
   value = SHFT_AMO_REG;
#if defined(__BORLANDC__)
   __asm{
       mov eax,dword ptr[op1]
       mul dword ptr[value]
       mov dword ptr[op1],eax
       mov dword ptr[op2],edx
   }
#elif defined(__GNUC__)
	__asm__ __volatile__ (
		"movl %2,%%eax\n"
		"mull %3\n"
		"movl %%eax,%0\n"
		"movl %%edx,%1\n"
		: "=m" (op1),"=m" (op2) : "m" (op1), "m" (value) : "eax","edx","memory"
	);
#endif
	LO_REG = op1;
	HI_REG = op2;
	value >>= 8;
   if(value == 0)
       m = 1;
   else{
       value >>= 8;
       if(value == 0)
           m = 2;
       else{
           value >>= 8;
           m = (u8)(value == 0 ? 3 : 4);
       }
   }
   return (u8)(cycP + m + cycI);
}
//---------------------------------------------------------------
NO_IN_LINE static u8 ins_mulls_unsigned(void)
{
   u32 op1,op2,value;
   u8 m;

   op1 = OP_REG;
   value = SHFT_AMO_REG;
#if defined(__BORLANDC__)
   __asm{
       mov eax,op1
       mul dword ptr[value]
       mov dword ptr[op1],eax
       mov dword ptr[op2],edx
   }
#elif defined(__GNUC__)
	__asm__ __volatile__ (
		"movl %2,%%eax\n"
		"mull %3\n"
		"movl %%eax,%0\n"
		"movl %%edx,%1\n"
		: "=m" (op1),"=m" (op2) : "m" (op1), "m" (op2) : "eax","edx","memory"
	);
#endif
	LO_REG = (u32)op1;
	HI_REG = (u32)op2;
	z_flag = (u8)(!op1 && !op2 ? 1 : 0);
   n_flag = (u8)(op2 >> 31);
   value >>= 8;
   if(value == 0)
       m = 1;
   else{
       value >>= 8;
       if(value == 0)
           m = 2;
       else{
           value >>= 8;
           m = 3;
           if(value != 0)
               m++;
       }
   }
   return (u8)(cycP + m);
}
//---------------------------------------------------------------
NO_IN_LINE static u8 ins_mlal(void)
{
   s32 op1;
   u32 d_reg,b_reg,value;
   u8 m;

   op1 = OP_REG;
   value = (u32)SHFT_AMO_REG;
   d_reg = DEST_REG;
   b_reg = BASE_REG;
#if defined(__BORLANDC__)
   __asm{
       mov eax,op1
       imul dword ptr[value]
       add d_reg,eax
       adc b_reg,edx
   }
#elif defined(__GNUC__)
	__asm__ __volatile__ (
		"movl %2,%%eax\n"
		"imull %3\n"
		"addl %%eax,%0\n"
		"adcl %%edx,%1\n"
		: "=m" (d_reg),"=m" (b_reg) : "m" (op1), "m" (value) : "eax","edx","memory"
	);
#endif
	LO_REG = (u32)d_reg;
	HI_REG = (u32)b_reg;
   value >>= 8;
   if(value == 0 || value == 0xFFFFFF)
       m = 1;
   else{
       value >>= 8;
       if(value == 0 || value == 0xFFFF)
           m = 2;
       else{
           value >>= 8;
           m = (u8)((value == 0 || value == 0xFF) ? 3 : 4);
       }
   }
   return (u8)(cycP + m + cycI);
}
//---------------------------------------------------------------
NO_IN_LINE static u8 ins_mlals(void)
{
   s32 op1,op2;
   u32 d_reg,b_reg,value;
   u8 m;

   op1 = OP_REG;
   value = op2 = SHFT_AMO_REG;
   d_reg = DEST_REG;
   b_reg = BASE_REG;
#if defined(__BORLANDC__)
   __asm{
       mov eax,op1
       imul dword ptr[op2]
       add d_reg,eax
       adc b_reg,edx
   }
#elif defined(__GNUC__)
	__asm__ __volatile__ (
		"movl %2,%%eax\n"
		"imull %3\n"
		"addl %%eax,%0\n"
		"adcl %%edx,%1\n"
		: "=m" (d_reg),"=m" (b_reg) : "m" (op1), "m" (op2) : "eax","edx","memory"
	);
#endif
	LO_REG = (u32)d_reg;
	HI_REG = (u32)b_reg;
   z_flag = (u8)(!d_reg && !b_reg ? 1 : 0);
   n_flag = (u8)(b_reg >> 31);
   value >>= 8;
   if(value == 0 || value == 0xFFFFFF)
       m = 1;
   else{
       value >>= 8;
       if(value == 0 || value == 0xFFFF)
           m = 2;
       else{
           value >>= 8;
           m = (u8)((value == 0 || value == 0xFF) ? 3 : 4);
       }
   }
   return (u8)(cycP + cycI + m);
}
//---------------------------------------------------------------
NO_IN_LINE static u8 ins_mlal_unsigned(void)
{
   u32 op1,op2,d_reg,b_reg,value;
   u8 m;

   op1 = OP_REG;
   value = op2 = SHFT_AMO_REG;
   d_reg = DEST_REG;
   b_reg = BASE_REG;
#if defined(__BORLANDC__)
   __asm{
       mov eax,op1
       mul dword ptr[op2]
       add d_reg,eax
       adc b_reg,edx
   }
#elif defined(__GNUC__)
	__asm__ __volatile__ (
		"movl %2,%%eax\n"
		"mull %3\n"
		"addl %%eax,%0\n"
		"adcl %%edx,%1\n"
		: "=m" (d_reg),"=m" (b_reg) : "m" (op1), "m" (op2) : "eax","edx","memory"
	);
#endif
	LO_REG = (u32)d_reg;
	HI_REG = (u32)b_reg;
   value >>= 8;
   if(value == 0)
       m = 1;
   else{
       value >>= 8;
       if(value == 0)
           m = 2;
       else{
           value >>= 8;
           m = (u8)(value == 0 ? 3 : 4);
       }
   }
   return (u8)(cycP + cycI + m);
}
//---------------------------------------------------------------
NO_IN_LINE static u8 ins_mlals_unsigned(void)
{
   u32 op1,op2,d_reg,b_reg,value;
   u8 m;

   op1 = OP_REG;
   value = op2 = SHFT_AMO_REG;
   d_reg = DEST_REG;
   b_reg = BASE_REG;
#if defined(__BORLANDC__)
   __asm{
       mov eax,op1
       mul dword ptr[op2]
       add d_reg,eax
       adc b_reg,edx
   }
#elif defined(__GNUC__)
	__asm__ __volatile__ (
		"movl %2,%%eax\n"
		"mull %3\n"
		"addl %%eax,%0\n"
		"adcl %%edx,%1\n"
		: "=m" (d_reg),"=m" (b_reg) : "m" (op1), "m" (op2) : "eax","edx","memory"
	);
#endif
	LO_REG = (u32)d_reg;
   HI_REG = (u32)b_reg;
   z_flag = (u8)(!d_reg && !b_reg ? 1 : 0);
   n_flag = (u8)(b_reg >> 31);
   value >>= 8;
   if(value == 0)
       m = 1;
   else{
       value >>= 8;
       if(value == 0)
           m = 2;
       else{
           value >>= 8;
           m = 3;
           if(value != 0)
               m++;
       }
   }
   return (u8)(cycP + cycI + m);
}
//---------------------------------------------------------------------
NO_IN_LINE static u8 ins_mrs_cpsr(void)
{
	DEST_REG = get_cpsr();
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ins_msr_cpsr(void)
{
   u32 value;
   u8 field;

   //   EnterCriticalSection(&crSection);
	value = opcode & 0x2000000 ? DP_IMM_OPERAND() : OP_REG;
   field = (u8)((opcode >> 16) & 0xF);
   if((cpsr & 0x1F) != USER_MODE){
       if((field & 1)){
           if((cpsr & 0x1F) != (value & 0x1F))
			    switchmode((u8)(value & 0x1F),1);
           cpsr = (cpsr & 0xFFFFFF00) | (value & 0xDF);
       }
       if((field & 2))
           cpsr = (cpsr & 0xFFFF00FF) | (value & 0x0000FF00);
       if((field & 4))
           cpsr = (cpsr & 0xFF00FFFF) | (value & 0x00FF0000);
   }
   if((field & 8)){
 	    c_flag = (u8)((value >> C_SHIFT) & 1);
 	    z_flag = (u8)((value >> Z_SHIFT) & 1);
 	    v_flag = (u8)((value >> V_SHIFT) & 1);
 	    n_flag = (u8)((value >> N_SHIFT) & 1);
       q_flag = (u8)((value >> Q_SHIFT) & 1);
   }
//   LeaveCriticalSection(&crSection);
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ins_mrs_spsr(void)
{
	DEST_REG = gp_reg[16];
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ins_msr_spsr(void)
{
   u32 value;
   u8 field;

   value = opcode & 0x2000000 ? DP_IMM_OPERAND() : OP_REG;
   field = (u8)((opcode >> 16) & 0xF);
   if(field & 8)
       gp_reg[16] = (gp_reg[16] & ~0xFF000000) | (0xFF000000 & value);
   if(field & 4)
       gp_reg[16] = (gp_reg[16] & ~0x00FF0000) | (0x00FF0000 & value);
   if(field & 2)
       gp_reg[16] = (gp_reg[16] & ~0x0000FF00) | (0x0000FF00 & value);
   if(field & 1)
       gp_reg[16] = (gp_reg[16] & ~0x000000FF) | ((u8)value);
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ins_bkpt(void)
{
	abort_nds(0);
   return (u8)(cycS + cycS + cycN);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ins_swi(void)
{
	int i;
   char *p;

   switch(opcode & 0x000FFFFF){
   	case 0xFD000:
			BiosBreakPoint(FALSE);
       break;
       case 0xFC000:
			WriteConsoleMessage(gp_reg[0],&arm9);
		break;
       case 0xEFEFE:
           switch((u8)gp_reg[12]){
               case 9:
                   if(reg[0][1] != 0){
                       i = (int)reg[0][0] / (int)reg[0][1];
                       reg[0][1] = (int)reg[0][0] % (int)reg[0][1];
                       reg[0][0] = i;
                       reg[0][3] = abs(i);
                   }
                   else{
                       reg[0][1] = 0;
                       reg[0][0] = 1;
                       reg[0][3] = 1;
                   }
               break;
               case 11:
                   CpuSet(reg[0][1],reg[0][0],reg[0][2],&arm9);
               break;
               case 12:
                   CpuFastSet(reg[0][1],reg[0][0],reg[0][2],&arm9);
               break;
               case 13:
                   reg[0][0] = SquareRoot(reg[0][0]);
               break;
               case 19:
                   HuffUnComp(reg[0][1],reg[0][0],&arm9);
               break;
               case 14:
                   if(reg[0][1] & 1)
                       reg[0][1] += 1;
                   reg[0][2] &= ~1;
                   p = (char *)LocalAlloc(LPTR,reg[0][2]);
                   for(i=0;i<(int)reg[0][2];i++)
                       p[i] = (char)read_byte_data(reg[0][1]+i);
                   reg[0][0] = CalcCrc16((u16 *)p,reg[0][2],(u16)reg[0][0]);
                   LocalFree(p);
               break;
               case 18:
               case 17:
                   LZ77UnComp(reg[0][0],reg[0][1],&arm9);
               break;
               case 20:
                   RLUnComp(reg[0][1],reg[0][0],&arm9);
               break;
               case 15:
                   i = 0;
               break;
               default:
					EnterDebugMode(FALSE);
               break;
           }
           if((i = (cpsr & 0x1F)) == SYSTEM_MODE || i == USER_MODE){
               gp_reg[0] = reg[0][0];
               gp_reg[1] = reg[0][1];
               gp_reg[3] = reg[0][3];
           }
       break;
       default:
           bios();
       break;
   }
	return (u8)(cycP+cycS+cycS);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ins_swp(void)
{
	u32 temp,adr;

   adr = BASE_REG;
   temp = READWORD(adr);
   WRITEWORD(adr, OP_REG);
   DEST_REG = temp;
   return (u8)(cycI + cycS + cycN + cycN);
}                                                        
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ins_swpb(void)
{
   u32 adr;
	u8 temp;

   adr = BASE_REG;
   temp = (u8)READBYTE(adr);
   WRITEBYTE(adr,(u8)OP_REG);
   DEST_REG = temp;
   return (u8)(cycI + cycS + cycN + cycN);
}
//-----------------------------------------------------------------------
//LDRSB
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsb_preup(void)
{
   DEST_REG = (u32)((s8)READBYTE(BASE_REG + OP_REG));
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsb_predown(void)
{
   DEST_REG = (u32)((s8)READBYTE(BASE_REG - OP_REG));
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsb_preupwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + OP_REG;
   DEST_REG = (u32)((s8)READBYTE(base));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsb_predownwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - OP_REG;
   DEST_REG = (u32)((s8)READBYTE(base));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsb_postup(void)
{
   u32 *regs;

   DEST_REG = (u32)((s8)READBYTE(*(regs = &BASE_REG)));
   *regs += OP_REG;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsb_postdown(void)
{
   u32 *regs;

   DEST_REG = (u32)((s8)READBYTE(*(regs = &BASE_REG)));
   *regs -= OP_REG;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsb_preupimm(void)
{
   DEST_REG = (u32)((s8)READBYTE(BASE_REG + (((opcode&0xF00)>>4)|(opcode&0xF))));
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsb_predownimm(void)
{
   DEST_REG = (u32)((s8)READBYTE(BASE_REG - (((opcode&0xF00)>>4)|(opcode&0xF))));
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsb_preupimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + (((opcode&0xF00)>>4)|(opcode&0xF));
   DEST_REG = (u32)((s8)READBYTE(base));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsb_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (((opcode&0xF00)>>4)|(opcode&0xF));
   DEST_REG = (u32)((s8)READBYTE(base));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsb_postupimm(void)
{
   u32 *regs;

   DEST_REG = (u32)((s8)READBYTE(*(regs = &BASE_REG)));
   *regs += ((opcode & 0xF00) >> 4)|(opcode & 0xF);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsb_postdownimm(void)
{
   u32 *regs;

   DEST_REG = (u32)((s8)READBYTE(*(regs = &BASE_REG)));
   *regs -= ((opcode & 0xF00) >> 4)|(opcode & 0xF);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
//LDRSH
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsh_preup(void)
{
   DEST_REG = (u32)((s16)READHWORD(BASE_REG + OP_REG));
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsh_predown(void)
{
   DEST_REG = (u32)((s16)READHWORD(BASE_REG - OP_REG));
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsh_preupwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + OP_REG;
   DEST_REG = (u32)((s16)READHWORD(base));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsh_predownwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - OP_REG;
   DEST_REG = (u32)((s16)READHWORD(base));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsh_postup(void)
{
   u32 *regs;

   DEST_REG = (u32)((s16)READHWORD(*(regs = &BASE_REG)));
   *regs += OP_REG;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsh_postdown(void)
{
   u32 *regs;

   DEST_REG = (u32)((s16)READHWORD(*(regs = &BASE_REG)));
   *regs -= OP_REG;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsh_preupimm(void)
{
   DEST_REG = (u32)((s16)READHWORD(BASE_REG + (((opcode&0xF00)>>4)|(opcode&0xF))));
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsh_predownimm(void)
{
   DEST_REG = (u32)((s16)READHWORD(BASE_REG - (((opcode&0xF00)>>4)|(opcode&0xF))));
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsh_preupimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + (((opcode&0xF00)>>4)|(opcode&0xF));
   DEST_REG = (u32)((s16)READHWORD(base));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsh_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (((opcode&0xF00)>>4)|(opcode&0xF));
   DEST_REG = (u32)((s16)READHWORD(base));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsh_postupimm(void)
{
   u32 *regs;

   DEST_REG = (u32)((s16)READHWORD(*(regs = &BASE_REG)));
   *regs += ((opcode&0xF00)>>4)|(opcode&0xF);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsh_postdownimm(void)
{
   u32 *regs;

   DEST_REG = (u32)((s16)READHWORD(*(regs = &BASE_REG)));
   *regs -= ((opcode&0xF00)>>4)|(opcode&0xF);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
//STM
//-----------------------------------------------------------------------
NO_IN_LINE static u8 stm(void)
{
   u32 *regs,base,stack,reg1[16],*p2,*p3;
   u8 r,i;
   u16 p1;

   stack = *(regs = &(p3 = gp_reg)[(opcode >> 16) & 0xF]);
   p2 = reg1;
   if((opcode & 0x400000) && (cpsr & 0x1F) != USER_MODE){
       for(p1 = 1;p1 > 0;p1 <<= 1,p3++){
           if((opcode & p1)){
               if(p1 == 0x2000){
                   *p2++ = reg[0][13];
                   continue;
               }
               else if(p1 == 0x4000){
                   *p2++ = reg[0][14];
                   continue;
               }
               *p2++ = *p3;
           }
       }
   }
   else{
#ifdef __BORLANDC__
       __asm{
           movzx eax,word ptr [opcode]
           db 0xf
           db 0xbc
           db 0xc8
           shr eax,cl
           mov word ptr[p1],ax
           shl ecx,2
           add dword ptr[p3],ecx
       }
#else
       p1 = (u16)opcode;
#endif
       for(;p1 != 0;p1 >>= 1,p3++){
           if(p1 & 1)
               *p2++ = *p3;
       }
   }
   r = (u8)(p2 - reg1);
   if((opcode & 0x800000)){
       base = stack + (r << 2);
       if((opcode & 0x1000000))
           stack += 4;
   }
   else{
       stack -= (r << 2);
       base = stack;
       if(!(opcode & 0x1000000))
           stack += 4;
   }
   if((opcode & 0x200000))
       *regs = base;
   stack &= ~3;
   p2 = reg1;
   for(i = 0;r > 0;r--){
       WRITEWORD(stack,*p2++);
       stack += 4;
       i += (u8)cycS;
   }
   return (u8)((i - cycS) + cycN + cycPN);
}
//-----------------------------------------------------------------------
//LDM
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldm(void) //ldm-stm
{
   u32 *regs,base,stack,reg1[16],*p2,*p3;
   u8 r,i;
   u16 p1;

   stack = *(regs = &(p3 = gp_reg)[(opcode >> 16) & 0xF]) & ~3;
   p2 = reg1;
   if((opcode & 0x408000) == 0x400000 && (cpsr & 0x1F) != USER_MODE){
       for(p1 = 1;p1 != 0;p1 <<= 1,p3++){
           if(opcode & p1){
               if(p1 == 0x2000){
                   *p2++ = (u32)&reg[0][13];
                   continue;                               
               }
               else if(p1 == 0x4000){
                   *p2++ = (u32)&reg[0][14];
                   continue;
               }
               *p2++ = (u32)p3;
           }
       }
   }
   else{
#ifdef __BORLANDC__
       __asm{
           movzx eax,word ptr [opcode]
           db 0xf
           db 0xbc
           db 0xc8
           shr eax,cl
           mov word ptr[p1],ax
           shl ecx,2
           add dword ptr[p3],ecx
       }
#else
       p1 = (u16)opcode;
#endif
       for(;p1 != 0;p1 >>= 1,p3++){
           if(p1 & 1)
               *p2++ = (u32)p3;
       }
   }
   r = (u8)(p2 - reg1);
   if((opcode & 0x800000)){
       base = stack + (r << 2);
       if(opcode & 0x1000000)
           stack += 4;
   }
   else{
       stack = (base = stack - (r << 2));
       if(!(opcode & 0x1000000))
           stack += 4;
   }
   p2 = reg1;
   *((u32 *)(*p2++)) = READWORD(stack);
   if(opcode & 0x200000)
       *regs = base;
   for(i = 0;r > 1;r--,p2++){
       stack += 4;
       *((u32 *)*p2) = READWORD(stack);
       i += (u8)cycS;
   }
#if defined(_DEBPRO)
   if((opcode & 0x8000))
	    del_CallStack(gp_reg[15]);
#endif
    if(opcode & 0x8000){
       if(opcode & 0x400000){
           if(!reload_base(TRUE))
       	    gp_reg[15] += 4;
       }
       else{
           if(gp_reg[15] & 1){
               gp_reg[15] &= ~1;
		        cpsr |= T_BIT;
               exec9 = exec_thumb;
               gp_reg[15] += 2;
		        fetch_func = fetch_thumb;
           }
           else
               gp_reg[15] += 4;
       }
   }
   return (u8)(cycI + i + cycN);
}
//-----------------------------------------------------------------------
//LDRH
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrh_preup(void)
{
   DEST_REG = READHWORD(BASE_REG + OP_REG);
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrh_predown(void)
{
   DEST_REG = READHWORD(BASE_REG - OP_REG);
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrh_preupwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + OP_REG;
   *regs = base;
   DEST_REG = (u32)READHWORD(base);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrh_predownwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - OP_REG;
   DEST_REG = READHWORD(base);
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrh_postup(void)
{
   u32 *regs;

   DEST_REG = READHWORD(*(regs = &BASE_REG));
   *regs += OP_REG;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrh_postdown(void)
{
   u32 *regs;

   DEST_REG = READHWORD(*(regs = &BASE_REG));
   *regs -= OP_REG;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrh_preupimm(void)
{
   DEST_REG = READHWORD(BASE_REG + (((opcode&0xF00)>>4)|(opcode&0xF)));
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrh_predownimm(void)
{
   DEST_REG = READHWORD(BASE_REG - (((opcode&0xF00)>>4)|(opcode&0xF)));
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrh_preupimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + (((opcode&0xF00)>>4)|(opcode&0xF));
   *regs = base;
   DEST_REG = READHWORD(base);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrh_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (((opcode&0xF00)>>4)|(opcode&0xF));
   *regs = base;
   DEST_REG = READHWORD(base);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrh_postupimm(void)
{
   u32 *regs,value;

   regs = &BASE_REG;
	value = READHWORD(*regs);
   *regs += ((opcode & 0xF00)>>4)|(opcode & 0xF);
	DEST_REG = value;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrh_postdownimm(void)
{
   u32 *regs;

   DEST_REG = READHWORD(*(regs = &BASE_REG));
   *regs -= ((opcode&0xF00)>>4)|(opcode&0xF);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
//STRH
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strh_preup(void)
{
   WRITEHWORD(BASE_REG + OP_REG,(u16)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strh_predown(void)
{
   WRITEHWORD(BASE_REG - OP_REG,(u16)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strh_preupwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + OP_REG;
   WRITEHWORD(base,(u16)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strh_predownwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - OP_REG;
   WRITEHWORD(base,(u16)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strh_postup(void)
{
   u32 *regs;

   WRITEHWORD(*(regs = &BASE_REG),(u16)DEST_REG);
   *regs += OP_REG;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strh_postdown(void)
{
   u32 *regs;

   WRITEHWORD(*(regs = &BASE_REG),(u16)DEST_REG);
   *regs -= OP_REG;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strh_preupimm(void)
{
   WRITEHWORD(BASE_REG + (((opcode & 0xF00)>>4)|(opcode & 0xF)),(u16)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strh_predownimm(void)
{
   WRITEHWORD(BASE_REG - (((opcode & 0xF00)>>4)|(opcode & 0xF)),(u16)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strh_preupimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + (((opcode&0xF00)>>4)|(opcode&0xF));
   WRITEHWORD(base,(u16)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strh_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (((opcode&0xF00)>>4)|(opcode&0xF));
   WRITEHWORD(base,(u16)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strh_postupimm(void)
{
   u32 *regs;

   WRITEHWORD(*(regs = &BASE_REG),(u16)DEST_REG);
   *regs += ((opcode&0xF00)>>4)|(opcode&0xF);
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strh_postdownimm(void)
{
   u32 *regs;

   WRITEHWORD(*(regs = &BASE_REG),(u16)DEST_REG);
   *regs -= ((opcode&0xF00)>>4)|(opcode&0xF);
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
//STRB
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strb_postup(void)
{
   u32 *regs;

   regs = &BASE_REG;
   WRITEBYTE(*regs,(u8)DEST_REG);
   *regs += DP_REG_OPERAND_IMM();
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strb_postdown(void)
{
   u32 *regs;

   regs = &BASE_REG;
   WRITEBYTE(*regs,(u8)DEST_REG);
   *regs -= DP_REG_OPERAND_IMM();
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strb_preup(void)
{
   WRITEBYTE(BASE_REG + DP_REG_OPERAND_IMM(),(u8)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strb_predown(void)
{
   WRITEBYTE(BASE_REG - DP_REG_OPERAND_IMM(),(u8)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strb_preupwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + DP_REG_OPERAND_IMM();
   WRITEBYTE(base,(u8)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strb_predownwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - DP_REG_OPERAND_IMM();
   WRITEBYTE(base,(u8)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strb_postupimm(void)
{
   u32 *regs;

   WRITEBYTE(*(regs = &BASE_REG),(u8)DEST_REG);
   *regs += opcode & 0xFFF;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strb_postdownimm(void)
{
   u32 *regs;

   regs = &BASE_REG;
   WRITEBYTE(*regs,(u8)DEST_REG);
   *regs -= opcode & 0xFFF;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strb_preupimm(void)
{
   WRITEBYTE(BASE_REG + (opcode & 0xFFF),(u8)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strb_predownimm(void)
{
   WRITEBYTE(BASE_REG - (opcode & 0xFFF),(u8)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strb_preupimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + (opcode & 0xFFF);
   WRITEBYTE(base,(u8)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strb_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (opcode & 0xFFF);
   WRITEBYTE(base,(u8)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
//LDRB
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrb_postup(void)
{
   u32 *regs;

   DEST_REG = READBYTE(*(regs = &BASE_REG));
   *regs += DP_REG_OPERAND_IMM();
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrb_postdown(void)
{
   u32 *regs;

   DEST_REG = READBYTE(*(regs = &BASE_REG));
   *regs -= DP_REG_OPERAND_IMM();
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrb_preup(void)
{
   DEST_REG = READBYTE(BASE_REG + DP_REG_OPERAND_IMM());
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrb_predown(void)
{
   DEST_REG = READBYTE(BASE_REG - DP_REG_OPERAND_IMM());
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrb_preupwb(void)
{
   u32 base,*regs;

   DEST_REG = READBYTE(base = *(regs = &BASE_REG) + DP_REG_OPERAND_IMM());
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrb_predownwb(void)
{
   u32 base,*regs;

   DEST_REG = READBYTE(base = *(regs = &BASE_REG) - DP_REG_OPERAND_IMM());
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrb_postupimm(void)
{
   u32 *regs;

   DEST_REG = READBYTE(*(regs = &BASE_REG));
   *regs += opcode & 0xFFF;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrb_postdownimm(void)
{
   u32 *regs;

   DEST_REG = READBYTE(*(regs = &BASE_REG));
   *regs -= opcode & 0xFFF;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrb_preupimm(void)
{
#ifdef _DEBPR0
   if((opcode & 0xF0000000) == 0xF0000000){
       return cycP;
   }
#endif
   DEST_REG = READBYTE(BASE_REG + (opcode & 0xFFF));
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrb_predownimm(void)
{
#ifdef _DEBPR0
   if((opcode & 0xF0000000) == 0xF0000000){
       return cycP;
   }
#endif
   DEST_REG = READBYTE(BASE_REG - (opcode & 0xFFF));
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrb_preupimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + (opcode & 0xFFF);
   *regs = base;
   DEST_REG = READBYTE(base);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrb_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (opcode & 0xFFF);
   *regs = base;
   DEST_REG = READBYTE(base);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
//STR
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_postup(void)
{
   u32 *regs;

   regs = &BASE_REG;
   WRITEWORD(*regs,gp_reg[DEST_REG_INDEX]);
   *regs += DP_REG_OPERAND_IMM();
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_postdown(void)
{
   u32 *regs;

   regs = &BASE_REG;
   WRITEWORD(*regs,gp_reg[DEST_REG_INDEX]);
   *regs -= DP_REG_OPERAND_IMM();
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_preup(void)
{
   WRITEWORD(BASE_REG + DP_REG_OPERAND_IMM(),gp_reg[DEST_REG_INDEX]);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_predown(void)
{
   WRITEWORD(BASE_REG - DP_REG_OPERAND_IMM(),gp_reg[DEST_REG_INDEX]);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_preupwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + DP_REG_OPERAND_IMM();
   WRITEWORD(base,gp_reg[DEST_REG_INDEX]);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_predownwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - DP_REG_OPERAND_IMM();
   WRITEWORD(base,gp_reg[DEST_REG_INDEX]);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_postupimm(void)
{
   u32 *regs;

   regs = &BASE_REG;
   WRITEWORD(*regs,gp_reg[DEST_REG_INDEX]);
   *regs += opcode & 0xFFF;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_postdownimm(void)
{
   u32 *regs;

   regs = &BASE_REG;
   WRITEWORD(*regs,gp_reg[DEST_REG_INDEX]);
   *regs -= opcode & 0xFFF;
   return STRBASE_RET;
}                                   //1ff99c4
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_preupimm(void) //20aaf18
{
   WRITEWORD(BASE_REG + (opcode & 0xFFF),gp_reg[DEST_REG_INDEX]);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_predownimm(void)
{
   WRITEWORD(BASE_REG - (opcode & 0xFFF),gp_reg[DEST_REG_INDEX]);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_preupimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + (opcode & 0xFFF);
   WRITEWORD(base,gp_reg[DEST_REG_INDEX]);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (opcode & 0xFFF);
   WRITEWORD(base,gp_reg[DEST_REG_INDEX]);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
//LDR
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrt_postup(void)
{
   u32 *regs;
   u8 rd;

   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(*(regs = &BASE_REG));
   if(*regs & 3)
       gp_reg[rd] = (gp_reg[rd] << (32-(8*(*regs&3)))) | (gp_reg[rd] >> (8*(*regs&3)));
   *regs += DP_REG_OPERAND_IMM();
   if(rd != 15)
   	return LDRBASE_RET;
   gp_reg[15] += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_postup(void)
{
   u32 *regs,base;
   u8 rd;

   regs = &BASE_REG;
   base = *regs;
   *regs += DP_REG_OPERAND_IMM();
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(base);
   if(base & 3)
       gp_reg[rd] = (gp_reg[rd] << (32-(8 * (base & 3)))) | (gp_reg[rd] >> (8 * (base & 3)));
   if(rd != 15)
       return LDRBASE_RET;
   gp_reg[15] += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_postdown(void)
{
   u32 *regs,base;
   u8 rd;

   regs = &BASE_REG;
   base = *regs;
 	*regs -= DP_REG_OPERAND_IMM();
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(base);
   if(base & 3)
       gp_reg[rd] = (gp_reg[rd] << (32-(8*(base&3)))) | (gp_reg[rd] >> (8*(base&3)));
   if(rd != 15)
       return LDRBASE_RET;
   gp_reg[15] += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_preupwb(void)
{
   u32 base,*regs;
   u8 rd;

   base = *(regs = &BASE_REG) + DP_REG_OPERAND_IMM();
   *regs = base;
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(base);
   if(base & 3)
       gp_reg[rd] = (gp_reg[rd] << (32-(8*(base&3)))) | (gp_reg[rd] >> (8*(base&3)));
   if(rd != 15)
       return LDRBASE_RET;
   gp_reg[15] += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_predown(void)
{
   u8 rd;
   u32 base;

   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(base = BASE_REG - DP_REG_OPERAND_IMM());
   if(base & 3)
       gp_reg[rd] = (gp_reg[rd] << (32-(8*(base&3)))) | (gp_reg[rd] >> (8*(base&3)));
   if(rd != 15)
       return LDR_RET;
   gp_reg[15] += 4;
   return LDRPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_preup(void)
{
   u8 rd;
   u32 adr;

   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD((adr = BASE_REG + DP_REG_OPERAND_IMM()));
   if(adr & 3)
       gp_reg[rd] = (gp_reg[rd] << (32-(8*(adr&3)))) | (gp_reg[rd] >> (8*(adr&3)));
   if(rd != 15)
       return LDR_RET;
   gp_reg[15] += 4;
   return LDRPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_predownwb(void)
{
   u32 base,*regs;
   u8 rd;

   base = *(regs = &BASE_REG) - DP_REG_OPERAND_IMM();
   *regs = base;
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(base);
   if(base & 3)
       gp_reg[rd] = (gp_reg[rd] << (32-(8*(base&3)))) | (gp_reg[rd] >> (8*(base&3)));
   if(rd != 15)
       return LDRBASE_RET;
   gp_reg[15] += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrt_postupimm(void)
{
   u32 *regs;
   u8 rd;

   regs = &BASE_REG;
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(*regs);
   if(*regs & 3)
       gp_reg[rd] = (gp_reg[rd] << (32 - (8*(*regs&3)))) | (gp_reg[rd] >> (8*(*regs&3)));
   *regs += opcode & 0xFFF;
   if(rd != 15)
       return LDRBASE_RET;
   gp_reg[15] += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_postupimm(void)
{
   u32 *regs,base;
   u8 rd;

   regs = &BASE_REG;
   base = *regs;
   *regs += opcode & 0xFFF;
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(base);
   if(base & 3)
       gp_reg[rd] = (gp_reg[rd] << (32-(8*(base&3)))) | (gp_reg[rd] >> (8*(base&3)));
   if(rd != 15)
   	return LDRBASE_RET;
	if(gp_reg[15] & 1){
       gp_reg[15] &= ~1;
       cpsr |= T_BIT;
       exec9 = exec_thumb;
       gp_reg[15] += 2;
       fetch_func = fetch_thumb;
   }
   else
   	gp_reg[15] += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_postdownimm(void)
{
   u32 *regs,base;
   u8 rd;

   regs = &BASE_REG;
   base = *regs;
   *regs -= opcode & 0xFFF;
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(base);
   if(base & 3)
       gp_reg[rd] = (gp_reg[rd] << (32-(8*(base&3)))) | (gp_reg[rd] >> (8*(base&3)));
   if(rd != 15)
       return LDRBASE_RET;
   gp_reg[15] += 4;
   return LDRBASEPC_RET;                                   
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_preupimm(void)
{
   u8 rd;
   u32 adr;

   adr = BASE_REG + (opcode & 0xFFF);
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(adr);
   if(adr & 3)
       gp_reg[rd] = (gp_reg[rd] << (32-(8*(adr&3)))) | (gp_reg[rd] >> (8*(adr&3)));
   if(rd != 15)
       return LDR_RET;
   if((gp_reg[15] & 1)){
       gp_reg[15] &= ~1;
		cpsr |= T_BIT;
       exec9 = exec_thumb;
       gp_reg[15] += 2;
		fetch_func = fetch_thumb;
   }
   else
       gp_reg[15] += 4;
   return LDRPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_predownimm(void)
{
   u8 rd;
   u32 base;

   base = BASE_REG - (opcode & 0xFFF);
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(base);
   if(base & 3)
       gp_reg[rd] = (gp_reg[rd] << (32-(8*(base&3)))) | (gp_reg[rd] >> (8*(base&3)));
   if(rd != 15)
       return LDR_RET;
	if(gp_reg[15] & 1){
		cpsr |= T_BIT;
       exec9 = exec_thumb;
       gp_reg[15] = (gp_reg[15] & ~1) + 2;
		fetch_func = fetch_thumb;
   }
   else
   	gp_reg[15] += 4;
   return LDRPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_preupimmwb(void)
{
   u32 *regs,base;
   u8 rd;

   base = *(regs = &BASE_REG) + (opcode & 0xFFF);
   *regs = base;
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(base);
   if(base & 3)
       gp_reg[rd] = (gp_reg[rd] << (32-(8*(base&3)))) | (gp_reg[rd] >> (8*(base&3)));
   if(rd != 15)
       return LDRBASE_RET;
   gp_reg[15] += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_predownimmwb(void)
{
   u32 base,*regs;
   u8 rd;

   base = *(regs = &BASE_REG) - (opcode & 0xFFF);
   *regs = base;
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(base);
   if(base & 3)
       gp_reg[rd] = (gp_reg[rd] << (32-(8*(base&3)))) | (gp_reg[rd] >> (8*(base&3)));
   if(rd != 15)
       return LDRBASE_RET;
   gp_reg[15] += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
// TEQ
//-----------------------------------------------------------------------
NO_IN_LINE static u8 teq_imm(void)
{
   SET_DP_LOG_FLAGS(BASE_REG ^ DP_IMM_OPERAND_UPD());
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 teq(void)
{
   SET_DP_LOG_FLAGS(BASE_REG ^ DP_REG_OPERAND_UPD((u8)IMM_SHIFT));
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 teq_reg(void)
{
   SET_DP_LOG_FLAGS(BASE_REG ^ DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG));
   return (u8)(cycI + cycP);
}
//-----------------------------------------------------------------------
//TST
//-----------------------------------------------------------------------
NO_IN_LINE static u8 tst_imm(void)
{
   SET_DP_LOG_FLAGS(BASE_REG & DP_IMM_OPERAND_UPD());
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 tst(void)
{
   SET_DP_LOG_FLAGS(BASE_REG & DP_REG_OPERAND_UPD((u8)IMM_SHIFT));
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 tst_reg(void)
{
   SET_DP_LOG_FLAGS(BASE_REG & DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG));
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
//CMN
//-----------------------------------------------------------------------
NO_IN_LINE static u8 cmn_imm(void)
{
   CalcAddFlags(BASE_REG,DP_IMM_OPERAND());
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 cmn(void)
{
   CalcAddFlags(BASE_REG,DP_REG_OPERAND_IMM());
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 cmn_reg(void)
{
   CalcAddFlags(BASE_REG,DP_REG_OPERAND());
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
//CMP
//-----------------------------------------------------------------------
NO_IN_LINE static u8 cmp_imm(void)
{
   CalcSubFlags(BASE_REG,DP_IMM_OPERAND());
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 cmp_reg(void)
{
   CalcSubFlags(BASE_REG,DP_REG_OPERAND());
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 cmp_lsl(void)
{
	CalcSubFlags(BASE_REG,OP_REG << IMM_SHIFT);
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 cmp(void)
{
   CalcSubFlags(BASE_REG,DP_REG_OPERAND_IMM());
   return cycP;
}
//-----------------------------------------------------------------------
//ADC
//-----------------------------------------------------------------------
NO_IN_LINE static u8 adc(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcAddc(BASE_REG,DP_REG_OPERAND_IMM());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 adc_reg(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcAddc(BASE_REG,DP_REG_OPERAND());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 adc_imm(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcAddc(BASE_REG,DP_IMM_OPERAND());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 adcs(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcAddcFlags(BASE_REG,DP_REG_OPERAND_IMM());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE))
			gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 adcs_reg(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcAddcFlags(BASE_REG,DP_REG_OPERAND());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 adcs_imm(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcAddcFlags(BASE_REG,DP_IMM_OPERAND());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE))
			gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
//RS
//-----------------------------------------------------------------------
NO_IN_LINE static u8 rss(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcSubFlags(DP_REG_OPERAND_IMM(),BASE_REG);
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 rss_reg(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcSubFlags(DP_REG_OPERAND(),BASE_REG);
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 rss_imm(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcSubFlags(DP_IMM_OPERAND(),BASE_REG);
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 rs(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = DP_REG_OPERAND_IMM() - BASE_REG;
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 rs_reg(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = DP_REG_OPERAND() - BASE_REG;
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 rs_imm(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = DP_IMM_OPERAND() - BASE_REG;
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 rsbc(void)
{
   DEST_REG = CalcSubc(DP_REG_OPERAND_IMM(),BASE_REG);
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 rsbc_reg(void)
{
   DEST_REG = CalcSubc(DP_REG_OPERAND(),BASE_REG);
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 rsbc_imm(void)
{
   DEST_REG = CalcSubc(DP_IMM_OPERAND(),BASE_REG);
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 rsbcs(void)
{
   u8 pipe;

   if((pipe = (u8)DEST_REG_INDEX) == 15){
       gp_reg[15] = CalcSubc(DP_REG_OPERAND_IMM(),BASE_REG);
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       if(!reload_base(TRUE))
       	gp_reg[15] += 4;
   }
   else
       gp_reg[pipe] = CalcSubcFlags(DP_REG_OPERAND_IMM(),BASE_REG);
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 rsbcs_reg(void)
{
   u8 pipe;

   if((pipe = (u8)DEST_REG_INDEX) == 15){
       gp_reg[15] = CalcSubc(DP_REG_OPERAND(),BASE_REG);
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       if(!reload_base(TRUE))
       	gp_reg[15] += 4;
   }
   else
       gp_reg[pipe] = CalcSubcFlags(DP_REG_OPERAND(),BASE_REG);
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 rsbcs_imm(void)
{
   u8 pipe;

   if((pipe = (u8)DEST_REG_INDEX) == 15){
       gp_reg[15] = CalcSubc(DP_IMM_OPERAND(),BASE_REG);
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       if(!reload_base(TRUE))
       	gp_reg[15] += 4;
   }
   else
       gp_reg[pipe] = CalcSubcFlags(DP_IMM_OPERAND(),BASE_REG);
   return cycP;
}
//-----------------------------------------------------------------------
//SUB
//-----------------------------------------------------------------------
NO_IN_LINE static u8 subs(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcSubFlags(BASE_REG,DP_REG_OPERAND_IMM());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE))
			gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 subs_reg(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcSubFlags(BASE_REG,DP_REG_OPERAND());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 subs_imm(void)
{
   u8 pipe;

   if((pipe = (u8)DEST_REG_INDEX) == 15){
       gp_reg[15] = BASE_REG - DP_IMM_OPERAND();
#if defined(_DEBPRO)
		del_CallStack(gp_reg[15]);
#endif
       if(!reload_base(TRUE))
           gp_reg[15] += 4;
   }
   else
       gp_reg[pipe] = CalcSubFlags(BASE_REG,DP_IMM_OPERAND());
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 sub_lsl(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG - (OP_REG << IMM_SHIFT);
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 sub(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG - DP_REG_OPERAND_IMM();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 sub_reg(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG - DP_REG_OPERAND();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 sub_imm(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG - DP_IMM_OPERAND();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 subcs(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcSubcFlags(BASE_REG,DP_REG_OPERAND_IMM());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 subcs_reg(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcSubcFlags(BASE_REG,DP_REG_OPERAND());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 subcs_imm(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcSubcFlags(BASE_REG,DP_IMM_OPERAND());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 subc(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcSubc(BASE_REG,DP_REG_OPERAND_IMM());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 subc_reg(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcSubc(BASE_REG,DP_REG_OPERAND());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 subc_imm(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcSubc(BASE_REG,DP_IMM_OPERAND());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
//ADD
//-----------------------------------------------------------------------
NO_IN_LINE static u8 adds(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcAddFlags(BASE_REG,DP_REG_OPERAND_IMM());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       if(!reload_base(TRUE)){
       	gp_reg[15] += 4;
       }
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 adds_reg(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcAddFlags(BASE_REG,DP_REG_OPERAND());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 adds_imm(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcAddFlags(BASE_REG,DP_IMM_OPERAND());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 add_lsl(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG + (OP_REG << IMM_SHIFT);
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 add(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG + DP_REG_OPERAND_IMM();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 add_reg(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG + DP_REG_OPERAND();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 add_imm(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG + DP_IMM_OPERAND();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
//MOV
//-----------------------------------------------------------------------
NO_IN_LINE static u8 mov_lsl(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = (OP_REG << IMM_SHIFT);
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 mov(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = DP_REG_OPERAND_IMM();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 mov_reg(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = DP_REG_OPERAND();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 mov_imm(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = DP_IMM_OPERAND();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 movs(void)
{
   u8 pipe;

   if((pipe = (u8)DEST_REG_INDEX) == 15){
       gp_reg[15] = DP_REG_OPERAND_IMM();
#if defined(_DEBPRO)
	    del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE))
			gp_reg[15] += 4;
   }
   else
       SET_DP_LOG_FLAGS((gp_reg[pipe] = DP_REG_OPERAND_UPD((u8)IMM_SHIFT)));
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 movs_reg(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG)));
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 movs_imm(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = DP_IMM_OPERAND_UPD()));
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return cycP;
}
//-----------------------------------------------------------------------
//MVN
//-----------------------------------------------------------------------
NO_IN_LINE static u8 mvn_lsl(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = ~(OP_REG << IMM_SHIFT);
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 mvn(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = ~DP_REG_OPERAND_IMM();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 mvn_reg(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = ~DP_REG_OPERAND();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 mvn_imm(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = ~DP_IMM_OPERAND();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 mvns(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = ~DP_REG_OPERAND_UPD((u8)IMM_SHIFT)));
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 mvns_reg(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = ~DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG)));
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 mvns_imm(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = ~DP_IMM_OPERAND_UPD()));
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return cycP;
}
//-----------------------------------------------------------------------
//BIC
//-----------------------------------------------------------------------
NO_IN_LINE static u8 bic(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)(((u16)opcode>> 12)))] = BASE_REG & ~DP_REG_OPERAND_IMM();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 bic_reg(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & ~DP_REG_OPERAND();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 bic_imm(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & ~DP_IMM_OPERAND();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 bics(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & ~DP_REG_OPERAND_UPD((u8)IMM_SHIFT)));
   if(pipe == 15){
#if defined(_DEBPRO)
    	del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 bics_reg(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & ~DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG)));
   if(pipe == 15){
#if defined(_DEBPRO)
	    del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 bics_imm(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & ~DP_IMM_OPERAND_UPD()));
   if(pipe == 15){
#if defined(_DEBPRO)
    	del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return cycP;
}
//-----------------------------------------------------------------------
// AND
//-----------------------------------------------------------------------
NO_IN_LINE static u8 and_lsl(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & (OP_REG << IMM_SHIFT);
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 and_(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & DP_REG_OPERAND_IMM();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 and_reg(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & DP_REG_OPERAND();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 and_imm(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & DP_IMM_OPERAND();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ands(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & DP_REG_OPERAND_UPD((u8)IMM_SHIFT)));
   if(pipe == 15){
#if defined(_DEBPRO)
		del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ands_reg(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG)));
   if(pipe == 15){
#if defined _DEBPRO
       del_CallStack(gp_reg[15]);
#endif
       if(!reload_base(TRUE)){
           gp_reg[15] += 4;
       }
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ands_imm(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & DP_IMM_OPERAND_UPD()));
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       if(!reload_base(TRUE)){
           gp_reg[15] += 4;
       }
   }
   return cycP;
}
//-----------------------------------------------------------------------
// EOR
//-----------------------------------------------------------------------
NO_IN_LINE static u8 eor_lsl(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG ^ (OP_REG << IMM_SHIFT);
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 eor(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG ^ DP_REG_OPERAND_IMM();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 eor_reg(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG ^ DP_REG_OPERAND();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 eor_imm(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG ^ DP_IMM_OPERAND();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 eors(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG ^ DP_REG_OPERAND_UPD((u8)IMM_SHIFT)));
   if(pipe == 15){
#if defined(_DEBPRO)
	    del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 eors_reg(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG ^ DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG)));
   if(pipe == 15){
#if defined(_DEBPRO)
    	del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 eors_imm(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG ^ DP_IMM_OPERAND_UPD()));
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return cycP;
}
//-----------------------------------------------------------------------
// ORR
//-----------------------------------------------------------------------
NO_IN_LINE static u8 orr_lsl(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG | (OP_REG << IMM_SHIFT);
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 orr(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG | DP_REG_OPERAND_IMM();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 orr_reg(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG | DP_REG_OPERAND();
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return (u8)(cycP + cycI);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 orr_imm(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = (BASE_REG | DP_IMM_OPERAND());
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 orrs(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG | DP_REG_OPERAND_UPD((u8)IMM_SHIFT)));
   if(pipe == 15){
#if defined(_DEBPRO)
	    del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 orrs_reg(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG | DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG)));
   if(pipe == 15){
#if defined(_DEBPRO)
    	del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return (u8)(cycP + cycI);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 orrs_imm(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG | DP_IMM_OPERAND_UPD()));
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
		if(!reload_base(TRUE)){
			gp_reg[15] += 4;
       }
   }
   return cycP;
}
/*---------------------------------------------------------------------------
dsp instruction
---------------------------------------------------------------------------*/
NO_IN_LINE static u8 ldrd(void)
{
   u32 ofs,rn;
	u8 rd;

	rn = BASE_REG;
   if((opcode & 0x400000))
		ofs = ((opcode & 0xF) | ((opcode & 0xF00) >> 4));
   else
		ofs = OP_REG;
   if((opcode & 0x1000000)){
   	if((opcode & 0x800000))
			rn += ofs;
       else
       	rn -= ofs;
   }
   rd = (u8)DEST_REG_INDEX;
   if(!(rd & 1) && rd < 14){
       gp_reg[rd] = READWORD(rn);
       gp_reg[rd+1] = READWORD(rn + 4);
   }
   if(!(opcode & 0x1000000)){
   	if((opcode & 0x800000))
			rn += ofs;
       else
       	rn -= ofs;
		BASE_REG = rn ;
   }
   else if((opcode & 0x200000))
   	BASE_REG = rn;
   return (u8)(cycI + cycP + cycPN + cycS);
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 strd(void)
{
   u32 ofs,rn;
	u8 rd;

	rn = BASE_REG;
   if((opcode & 0x400000))
		ofs = ((opcode & 0xF) | ((opcode & 0xF00) >> 4));
   else
		ofs = OP_REG;
   if((opcode & 0x1000000)){
   	if((opcode & 0x800000))
			rn += ofs;
       else
       	rn -= ofs;
   }
   rd = (u8)DEST_REG_INDEX;
   if((rn & 7) == 0 && !(rd & 1) && rd < 14){
       WRITEWORD(rn,gp_reg[rd]);
       WRITEWORD(rn+4,gp_reg[rd+1]);
   }
   if(!(opcode & 0x1000000)){
   	if((opcode & 0x800000))
			rn += ofs;
       else
       	rn -= ofs;
		BASE_REG = rn;
   }
   else if((opcode & 0x200000))
   	BASE_REG = rn;
   return (u8)(cycI + cycP + cycPN + cycS);
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 smulxy(void)
{
	s32 op1;

   if(!(opcode & 0x20))
   	op1 = (s32)(s16)OP_REG;
   else
   	op1 = (s32)(s16)(OP_REG >> 16);
   if(!(opcode & 0x40))
   	op1 *= (s32)(s16)SHFT_AMO_REG;
   else
   	op1 *= (s32)(s16)(SHFT_AMO_REG >> 16);
   BASE_REG = (u32)op1;
   return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 smlaxy(void)
{
	s32 op1;

   if(!(opcode & 0x20))
   	op1 = (s32)(s16)OP_REG;
   else
   	op1 = (s32)(s16)(OP_REG >> 16);
   if(!(opcode & 0x40))
   	op1 *= (s32)(s16)SHFT_AMO_REG;
   else
   	op1 *= (s32)(s16)(SHFT_AMO_REG >> 16);
#if defined(__BORLANDC__) || defined(__WATCOMC__)
	__asm{
       movzx eax,word ptr[opcode]
       mov edx,dword ptr[op1]
       mov ecx,dword ptr[opcode]
       shr eax,12
       shr ecx,16
       add edx, dword ptr[gp_reg+4*eax]
       jno short smlaxy1:           
       mov byte ptr[q_flag],1
smlaxy1:
       and ecx,0xF
       mov dword ptr[gp_reg+ecx*4],edx
   }
#elif defined(__GNUC__)
	__asm__ __volatile__ (
		"movzwl %1,%%eax\n"
		"movl %0,%%edx\n"
       "movl %1,%%ecx\n"
		"shrl $12,%%eax\n"
		"shrl $16,%%ecx\n"
		"addl _ZL6gp_reg(,%%eax,4),%%edx\n"
		"jno smlaxy1\n"
       "movb $1,%2\n"
       "smlaxy1:\n"
		"andl $15,%%ecx\n"
		"movl %%edx,_ZL6gp_reg(,%%ecx,4)\n"
		: : "m" (op1), "m" (opcode), "m" (q_flag)
	);
#endif
   return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 smlalxy(void)
{
   s32 op1,op2,d_reg,b_reg;

   if(!(opcode & 0x20))
   	op1 = (s32)(s16)OP_REG;
   else
   	op1 = (s32)(s16)(OP_REG >> 16);
   if(!(opcode & 0x40))
   	op2 = (s32)(s16)SHFT_AMO_REG;
   else
   	op2 = (s32)(s16)(SHFT_AMO_REG >> 16);
   d_reg = DEST_REG;
   b_reg = BASE_REG;
#if defined(__BORLANDC__) || defined(__WATCOMC__)
   __asm{
       mov eax,op1
       imul dword ptr[op2]
       add d_reg,eax
       adc b_reg,edx
   }
#elif defined(__GNUC__)
	__asm__ __volatile__ (
		"movl %2,%%eax\n"
		"imull %3\n"
		"addl %%eax,%0\n"
		"adcl %%edx,%1\n"
		: "=m" (d_reg),"=m" (b_reg) : "m" (op1), "m" (op2) : "eax","edx","memory"
	);
#endif
	LO_REG = (u32)d_reg;
	HI_REG = (u32)b_reg;
   return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 smlawy(void)
{
	int op2;

   if(!(opcode & 0x40))
   	op2 = (int)(s16)SHFT_AMO_REG;
   else
   	op2 = (int)(s16)(SHFT_AMO_REG >> 16);
#if defined(__BORLANDC__) || defined(__WATCOMC__)
   __asm{
   	mov eax,opcode
       and eax,0xF
       mov eax,dword ptr[gp_reg+eax*4]
       imul dword ptr[op2]
       shrd eax,edx,16
       mov edx,opcode
       shr edx,16
       and edx,0xF
	    add dword ptr[gp_reg+edx*4],eax
       seto byte ptr[q_flag]
   }
#elif defined(__GNUC__)
   __asm__ __volatile__ (
        "movl %1,%%eax\n"
		"andl $15,%%eax\n"
		"movl _ZL6gp_reg(,%%eax,4),%%eax\n"
		"imull %0\n"
		"shrd $0x10,%%edx,%%eax\n"
		"movl %1,%%edx\n"
		"shrl $16,%%edx\n"
		"andl $15,%%edx\n"
		"addl %%eax,_ZL6gp_reg(,%%edx,4)\n"
		"setob %2\n"
		: : "m" (op2), "m" (opcode), "m" (q_flag)
	);
#endif
   return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 smulwy(void)
{
	int op2;

   if(!(opcode & 0x40))
   	op2 = (int)(s16)SHFT_AMO_REG;
   else
   	op2 = (int)(s16)(SHFT_AMO_REG >> 16);
#if defined(__BORLANDC__) || defined(__WATCOMC__)
   __asm{
       mov eax,opcode
       and eax,0xF
       mov eax,dword ptr[gp_reg+eax*4]
       imul dword ptr[op2]
       shrd eax,edx,16
       mov edx,opcode
       shr edx,16
       and edx,0xF
       mov dword ptr[gp_reg+edx*4],eax
   }
#elif defined(__GNUC__)
	__asm__ __volatile__ (
		"movl %1,%%eax\n"
		"andl $15,%%eax\n"
		"movl _ZL6gp_reg(,%%eax,4),%%eax\n"
		"imull %0\n"
       "shrd $16,%%edx,%%eax\n"
		"movl %1,%%edx\n"
		"shrl $16,%%edx\n"
		"andl $15,%%edx\n"
		"movl %%eax,_ZL6gp_reg(,%%edx,4)\n"
		: : "m" (op2), "m" (opcode)
 	);
#endif
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 qadd(void)
{
#if defined(__BORLANDC__) || defined(__WATCOMC__)
	__asm{
		mov edx,dword ptr [opcode]
       mov	ecx,edx
		and edx,15
		shr ecx,16
	    mov edx, dword ptr[gp_reg+4*edx]
       and ecx,15
		add edx, dword ptr[gp_reg+4*ecx]
		seto byte ptr[q_flag]
       jno short @nosat
       jns short @nosat1
       mov edx,0x7fffffff
       jmp short @nosat
       @nosat1:
       mov edx,0xffffffff
       @nosat:
       movzx eax,word ptr[opcode]
		shr eax,12
		mov dword ptr[gp_reg+eax*4],edx
   }
#elif defined(__GNUC__)
	__asm__ __volatile__ (
		"movl %0,%%edx\n"
		"movl %%edx,%%ecx\n"
		"andl $15,%%edx\n"
		"shrl $16,%%ecx\n"
		"movl _ZL6gp_reg(,%%edx,4),%%edx\n"
		"andl $15,%%ecx\n"
		"addl _ZL6gp_reg(,%%ecx,4),%%edx\n"
		"setob %1\n"
		"jno nosat\n"
		"jns nosat1\n"
		"movl $0x7fffffff,%%edx\n"
		"jmp nosat\n"
		"nosat1:\n"
		"movl $0xffffffff,%%edx\n"
		"nosat:\n"
		"movzwl %0,%%eax\n"
		"shrl $12,%%eax\n"
		"movl %%edx,_ZL6gp_reg(,%%eax,4)\n"
       : : "m" (opcode), "m" (q_flag)
 	);
#endif
   return cycP;
}
/*---------------------------------------------------------------------------
thumb instruction
---------------------------------------------------------------------------*/
NO_IN_LINE static u8 tins_swi(void)
{
	switch((u8)opcode){
   	case 0xFD:
			BiosBreakPoint(FALSE);
       break;
       case 0xFC:
			WriteConsoleMessage(gp_reg[0],&arm9);
		break;
       default:
       	bios();
       break;
   }
   return (u8)(cycS + cycS + cycN);
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_bkpt(void)
{
	abort_nds(0);
	return (u8)(cycS + cycS + cycN);
}
//---------------------------------------------------------------------------
// SBC
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_sbc(void)
{
   u32 *regd;

   *regd = CalcSubcFlags(*(regd = &gp_reg[opcode & 7]),gp_reg[(opcode >> 3) & 7]);
   return cycP;
}
//---------------------------------------------------------------------------
// TST
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_tst(void)
{
 	SET_DP_LOG_FLAGS(gp_reg[opcode & 7] & gp_reg[(opcode >> 3) & 7]);
	return cycP;
}
//---------------------------------------------------------------------------
// ROR
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_ror_reg(void)
{
   u8 s;
   u32 value,*regd;

   value = *(regd = &gp_reg[opcode & 7]);
   s = (u8)gp_reg[(opcode >> 3) & 7];
   if(s){
       s &= 0x1F;
       if(s == 0)
           c_flag = (u8)(value >> 31);
       else{
#if defined(__BORLANDC__)
       _EAX = value;
       _CL = s;
       __asm{
           ror eax,cl
           setc byte ptr[c_flag]
       }
       value = _EAX;
#elif defined(__GNUC__)
	__asm__ __volatile__ (
		"movb %2,%%cl\n"
		"movl %1,%%eax\n"
		"rorl %%cl,%%eax\n"
		"setcb %3\n"
		"mov %%eax,%0\n"
		: "=m" (value) : "m" (value), "m" (s), "m" (c_flag)
	);
#endif
       }
   }
   SET_DP_LOG_FLAGS(*regd = value);
	return (u8)(cycI + cycP);
}
//---------------------------------------------------------------------------
// MUL
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_mul(void)
{
   u8 m;
   u32 value;

   SET_DP_LOG_FLAGS((gp_reg[opcode & 7] *= (value = gp_reg[(opcode >> 3) & 7])));
   value >>= 8;
   if(value == 0 || value == 0xFFFFFF)
       m = 1;
   else{
       value >>= 8;
       if(value == 0 || value == 0xFFFF)
           m = 2;
       else{
           value >>= 8;
           m = (u8)((value == 0 || value == 0xFF) ? 3 : 4);
       }
   }
   return (u8)(cycP + m);
}
//---------------------------------------------------------------------------
// ADD
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_adc_reg(void)
{
   u32 *regd;

	*regd = CalcAddcFlags(gp_reg[(opcode >> 3) & 7],*(regd = &gp_reg[opcode & 7]));
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_add_pc(void)
{
   gp_reg[(opcode >> 8) & 7] = (gp_reg[15] & ~3) + (((u8)opcode) << 2);
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_add_lo_hi(void)
{
   u8 rd;

   if((rd = (u8)(((opcode & 0x80) >> 4) | (opcode & 7))) == 15)
       gp_reg[15] += gp_reg[(opcode >> 3) & 0xF] + 2;
   else
       gp_reg[rd] += gp_reg[(opcode >> 3) & 0xF];
	return (u8)(cycS + cycS + cycP);
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_add_rel_sp (void)
{
   gp_reg[13] += ((opcode&0x7F)<<2);
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_add_sp (void)
{
   gp_reg[(opcode >> 8) & 7] = gp_reg[13] + (((u8)opcode)<<2);
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_add_short_imm (void)
{
	gp_reg[opcode & 7] = CalcAddFlags(gp_reg[(opcode >> 3) & 7],(u32)((u8)((opcode >> 6) & 7)));
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_add(void)
{
	gp_reg[opcode & 7] = CalcAddFlags(gp_reg[(opcode >> 3) & 7],gp_reg[(opcode >> 6) & 7]);
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_add_imm (void)
{
	u32 *regd;

	*regd = CalcAddFlags(*(regd = &gp_reg[(opcode >> 8) & 7]),(u32)((u8)opcode));
	return cycP;
}
//---------------------------------------------------------------------------
// MOV
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_mov_hi_lo (void)
{
   u8 r;

   r = (u8)(((opcode & 0x80) >> 4) | (opcode & 7));
   gp_reg[r] = gp_reg[((opcode >> 3) & 0xF)];
   if(r == 15)
       gp_reg[15] = (gp_reg[15] & ~1) + 2;
	return (u8)(cycS + cycS + cycP);
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_mov_imm (void)
{
	SET_DP_LOG_FLAGS((gp_reg[(opcode >> 8) & 7] = (u8)opcode));
	return cycP;
}
//---------------------------------------------------------------------------
// LDR
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_ldmia(void)
{
   u8 n,p;
   u32 address,*regd,*reg;

   address = *(regd = &(reg = gp_reg)[(opcode >> 8) & 7]) & ~3;
   for(p=(u8)opcode,n=0;p != 0;reg++,p >>= 1){
       if(!(p & 1))
           continue;
       *reg = READWORD(address);
       if(reg == regd)
           regd = NULL;
       address += 4;
       n += (u8)cycS;
   }
   if(regd != NULL)
       *regd = address;
   return (u8)(cycI + n + cycN);
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_sp_rel_ldr(void)
{
	gp_reg[(opcode >> 8) & 7] = READWORD(gp_reg[13] + (((u8)opcode)<<2));
	return LDR_RET;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_pc_rel_ldr(void)
{
	gp_reg[(opcode >> 8) & 7] = READWORD((gp_reg[15] & ~3) + (((u8)opcode)<<2));
	return LDR_RET;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_ldr_reg(void)
{
   u8 rd;
   u32 adr;

	gp_reg[rd = (u8)(opcode & 7)] = READWORD(adr = gp_reg[(opcode >> 3) & 7] + gp_reg[(opcode >> 6) & 7]);
   if(adr & 3)
       gp_reg[rd] = (gp_reg[rd] << (32-(8*(adr&3)))) | (gp_reg[rd] >> (8*(adr&3)));
	return LDR_RET;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_ldr_imm(void)
{
   u32 adr;
   u8 rd;

	gp_reg[rd = (u8)(opcode & 7)] = READWORD((adr = gp_reg[(opcode >> 3) & 7] + (((opcode>>6)&0x1F)<<2)));
   if(adr & 3)
       gp_reg[rd] = (gp_reg[rd] << (32-(8*(adr&3)))) | (gp_reg[rd] >> (8*(adr&3)));
	return LDR_RET;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_ldrb_imm(void)
{                                                                         //805640a
	gp_reg[opcode & 7] = READBYTE(gp_reg[(opcode >> 3) & 7] + (((opcode>>6)&0x1F)));
	return LDR_RET;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_ldrb_reg(void)
{
	gp_reg[opcode & 7] = READBYTE(gp_reg[(opcode >> 3) & 7] + gp_reg[(opcode >> 6) & 7]);
	return LDR_RET;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_ldrh_reg(void)
{
	gp_reg[opcode & 7] = READHWORD(gp_reg[(opcode >> 3) & 7] + gp_reg[(opcode >> 6) & 7]);
	return LDR_RET;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_ldrh_imm(void)
{
	gp_reg[opcode & 7] = READHWORD(gp_reg[(opcode >> 3) & 7] + (((opcode>>6)&0x1F)<<1));
	return LDR_RET;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_ldrsh(void)
{
	gp_reg[opcode & 7] = (u32)((s16)READHWORD(gp_reg[(opcode >> 3) & 7] + gp_reg[(opcode >> 6) & 7]));
	return LDR_RET;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_ldrsb_reg(void)
{
	gp_reg[opcode & 7] = (u32)((s8)READBYTE(gp_reg[(opcode >> 3) & 7] + gp_reg[(opcode >> 6) & 7]));
	return LDR_RET;
}
//---------------------------------------------------------------------------
// STR
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_stmia(void)
{
   u8 n;
   u32 address,*regd,*reg;
   u16 p;

   address = *(regd = &(reg = gp_reg)[(opcode >> 8) & 7]) & ~3;
   n = 0;
   for(p = (u8)opcode;p != 0;p >>= 1,reg++){
       if(!(p & 1))
           continue;
       WRITEWORD(address,*reg);
       address += 4;
       n += (u8)cycS;
   }
   *regd = address;
   return (u8)(n + cycN + cycPN);
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_str_reg(void)
{
	WRITEWORD(gp_reg[(opcode >> 3) & 7] + gp_reg[(opcode >> 6) & 7], gp_reg[opcode & 7]);
	return STR_RET;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_str_imm(void)
{
   WRITEWORD(gp_reg[(opcode >> 3) & 7] + (((opcode>>6)&0x1F)<<2), gp_reg[opcode & 7]);
	return STR_RET;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_sp_rel_str(void)
{
	WRITEWORD(gp_reg[13] + (((u8)opcode)<<2), gp_reg[(opcode >> 8) & 7]);
	return STR_RET;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_strh_reg(void)
{
   WRITEHWORD(gp_reg[(opcode >> 3) & 7] + gp_reg[(opcode >> 6) & 7],(u16)gp_reg[opcode & 7]);
	return STR_RET;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_strb_reg(void)
{
   WRITEBYTE(gp_reg[(opcode >> 3) & 7] + gp_reg[(opcode >> 6) & 7],gp_reg[opcode & 7]);
	return STR_RET;
}                                                                           //801f6f2
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_strb_imm(void)
{
	WRITEBYTE(gp_reg[(opcode >> 3) & 7] + (((opcode>>6)&0x1F)),gp_reg[opcode & 7]);
	return STR_RET;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_strh_imm(void)
{
	WRITEHWORD(gp_reg[(opcode >> 3) & 7] + (((opcode>>6)&0x1F)<<1),(u16)gp_reg[opcode & 7]);
	return STR_RET;
}
//---------------------------------------------------------------------------
// lsl
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_lsl_reg(void)
{
   u8 temp;
   register u32 temp1,*regd;

   temp1 = *(regd = &gp_reg[opcode & 7]);
   temp = (u8)gp_reg[(opcode >> 3) & 7];
   if(temp && temp < 32){
       *regd = temp1 << temp;
#if defined(__BORLANDC__)
		__asm {
           sets byte ptr[n_flag]
           setz byte ptr[z_flag]
           setc byte ptr[c_flag]
       }
#elif defined(__GNUC__)
       __asm__ __volatile__ (
       	"setcb %0\r\n"
       	"setzb %1\r\n"
       	"setsb %2\r\n"
       	: : "m" (c_flag), "m" (z_flag), "m" (n_flag)
       );
#endif
       return (u8)(cycI + cycP);
   }
   else if(temp == 32){
       c_flag = (u8)(temp1 & 1);
       temp1 = 0;
   }
   else if(temp > 0){
       temp1 = 0;
       c_flag = 0;
   }
	SET_DP_LOG_FLAGS(*regd = temp1);
	return (u8)(cycI + cycP);
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_lsl_imm(void)
{
   u32 volatile temp1;

   temp1 = gp_reg[(opcode >> 3) & 7] << ((opcode>>6)&0x1F);
#if defined(__BORLANDC__)
   __asm {
      	sets byte ptr[n_flag]
      	setz byte ptr[z_flag]
      	setc byte ptr[c_flag]
   }
#elif defined(__GNUC__)
   __asm__ __volatile__ (
       "setcb %0\r\n"
       "setzb %1\r\n"
       "setsb %2\r\n"
       : : "m" (c_flag), "m" (z_flag), "m" (n_flag)
   );
#endif
   gp_reg[opcode & 7] = temp1;
	return cycP;
}
//---------------------------------------------------------------------------
// lsr
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_lsr_reg(void)
{
   u8 temp;
   u32 volatile temp1,*regd;

   temp = (u8)(gp_reg[(opcode >> 3) & 7]);
   temp1 = *(regd = &gp_reg[opcode & 7]);
   if(temp && temp < 32){
       temp1 >>= temp;
#if defined(__BORLANDC__)
       __asm {
       	sets byte ptr[n_flag]
       	setz byte ptr[z_flag]
       	setc byte ptr[c_flag]
       }
#elif defined(__GNUC__)
   __asm__ __volatile__ (
       "setcb %0\r\n"
       "setzb %1\r\n"
       "setsb %2\r\n"
       : : "m" (c_flag), "m" (z_flag), "m" (n_flag)
   );
#endif
       gp_reg[opcode & 7] = temp1;
       return (u8)(cycP+cycI);
   }
   else if(temp == 32){
       c_flag = (u8)(temp1 >> 31);
       temp1 = 0;
   }
   else if(temp > 32){
       c_flag = 0;
       temp1 = 0;
   }
 	SET_DP_LOG_FLAGS(*regd = temp1);
	return (u8)(cycI + cycP);
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_lsr_imm(void)
{
   u32 volatile temp1;
   u8 temp;

   temp1 = gp_reg[(opcode >> 3) & 7];
   if((temp = (u8)((opcode>>6)&0x1F)) == 0){
   	c_flag = (u8)(temp1 >> 31);
       temp1 = 0;
   }
   else{
   	temp1 >>= temp;
#if defined(__BORLANDC__)
       __asm {
       	sets byte ptr[n_flag]
       	setz byte ptr[z_flag]
       	setc byte ptr[c_flag]
       }
#elif defined(__GNUC__)
       __asm__ __volatile__ (
           "setcb %0\r\n"
           "setzb %1\r\n"
           "setsb %2\r\n"
           : : "m" (c_flag), "m" (z_flag), "m" (n_flag)
       );
#endif
	}
   gp_reg[opcode & 7] = temp1;
	return cycP;
}
//---------------------------------------------------------------------------
//ASR
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_asr_reg(void)
{
   u8 shift;
   u32 *regd;
   s32 volatile temp1;

   temp1 = *(regd = &gp_reg[opcode & 7]);
   shift = (u8)gp_reg[(opcode >> 3) & 7];
   if(shift && shift < 32){
       temp1 >>= shift;
#if defined(__BORLANDC__)
       __asm {
       	sets byte ptr[n_flag]
       	setz byte ptr[z_flag]
       	setc byte ptr[c_flag]
       }
#elif defined(__GNUC__)
       __asm__ __volatile__ (
           "setcb %0\r\n"
           "setzb %1\r\n"
           "setsb %2\r\n"
           : : "m" (c_flag), "m" (z_flag), "m" (n_flag)
       );
#endif
       *regd = temp1;
   }
   else if(shift > 31){
       if((c_flag = (u8)(temp1 >> 31)) != 0)
           temp1 = 0xFFFFFFFF;
       else
           temp1 = 0;
	    SET_DP_LOG_FLAGS(*regd = temp1);
   }
   return (u8)(cycI + cycP);
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_asr_imm(void)
{
   u8 shift;
   s32 volatile temp1;

   temp1 = gp_reg[(opcode >> 3) & 7];
   if((shift = (u8)((opcode>>6) & 0x1F)) == 0){
       if((c_flag = (u8)(temp1 >> 31)) != 0)
           temp1 = 0xFFFFFFFF;
       else
           temp1 = 0;
	    SET_DP_LOG_FLAGS(gp_reg[opcode & 7] = temp1);
   }
   else{
       temp1 >>= shift;
#if defined(__BORLANDC__)
       __asm {
       	sets byte ptr[n_flag]
       	setz byte ptr[z_flag]
       	setc byte ptr[c_flag]
       }
#elif defined(__GNUC__)
       __asm__ __volatile__ (
           "setcb %0\r\n"
           "setzb %1\r\n"
           "setsb %2\r\n"
           : : "m" (c_flag), "m" (z_flag), "m" (n_flag)
       );
#endif
       gp_reg[opcode & 7] = temp1;
   }
	return cycP;
}
//---------------------------------------------------------------------------
// CMN
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_cmn_reg(void)
{
   CalcAddFlags(gp_reg[opcode & 7],gp_reg[(opcode >> 3) & 7]);
	return cycP;
}
//---------------------------------------------------------------------------
// CMP
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_cmp_reg(void)
{
	CalcSubFlags(gp_reg[((opcode & 0x80) >> 4)|(opcode & 0x7)],gp_reg[(opcode>>3)&0xf]);
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_cmp(void)
{
	CalcSubFlags(gp_reg[opcode & 7],gp_reg[(opcode >> 3) & 7]);
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_cmp_imm(void)
{
	CalcSubFlags(gp_reg[(opcode >> 8) & 7],(u8)opcode);
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_eor(void)
{
	SET_DP_LOG_FLAGS((gp_reg[opcode & 7] ^= gp_reg[(opcode >> 3) & 7]));
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_and(void)
{
	SET_DP_LOG_FLAGS((gp_reg[opcode & 7] &= gp_reg[(opcode >> 3) & 7]));
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_orr(void)
{
	SET_DP_LOG_FLAGS((gp_reg[opcode & 7] |= gp_reg[(opcode >> 3) & 7]));
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_bic(void)
{
	SET_DP_LOG_FLAGS((gp_reg[opcode & 7] &= ~gp_reg[(opcode >> 3) & 7]));
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_mvn(void)
{
	SET_DP_LOG_FLAGS ((gp_reg[opcode & 7] = ~gp_reg[(opcode >> 3) & 7]));
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_neg(void)
{
   gp_reg[opcode & 7] = CalcSubFlags(0,gp_reg[(opcode >> 3) & 7]);
	return cycP;
}
//---------------------------------------------------------------------------
// SUB
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_sub_reg_imm(void)
{
   gp_reg[opcode & 7] = CalcSubFlags(gp_reg[(opcode >> 3) & 7],((opcode >> 6) & 7));
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_sub_sp(void)
{
	gp_reg[13] -= (opcode & 0x7F) << 2;
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_sub(void)
{
	gp_reg[opcode & 7] = CalcSubFlags(gp_reg[(opcode >> 3) & 7], gp_reg[(opcode >> 6) & 7]);
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_sub_imm(void)
{
	u32 *regd;

	*regd = CalcSubFlags(*(regd = &gp_reg[(opcode >> 8) & 7]), (u32)((u8)opcode));
	return cycP;
}
//---------------------------------------------------------------------------
// stack
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_push(void)
{
   u8 p,n;
   u32 *reg,adress;

   adress = (reg = &gp_reg[7])[6];
   n=0;
   if((opcode & 0x100)){
       adress -= 4;
       WRITEWORD(adress,gp_reg[14]);
       n += (u8)cycS;
   }
   for(p = (u8)opcode;p != 0;p <<= 1,reg--){
       if(!(p & 0x80))
           continue;
       adress -= 4;
       WRITEWORD(adress,*reg);
       n += (u8)cycS;
   }
   gp_reg[13] = adress;
	return (u8)(n + cycN + cycPN);
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_pop(void)
{
   u8 n,p;
   u32 *reg,adress;

   n = 0;
   adress = (reg = gp_reg)[13];
   for(p = (u8)opcode;p != 0;p >>= 1,reg++) {
	    if(!(p & 1))
           continue;
       *reg = READWORD(adress);
       adress += 4;
       n += (u8)cycS;
   }
   if(opcode & 0x100){
       gp_reg[15] = READWORD(adress);
#if defined(_DEBPRO)
		del_CallStack(gp_reg[15]);
#endif
       adress += 4;
       if(!(gp_reg[15] & 1)){
       	gp_reg[15] = (gp_reg[15] & ~3) + 4;
       	cpsr &= ~T_BIT;
       	exec9 = exec_arm;
       	fetch_func = fetch_arm;
       }
       else
       	gp_reg[15] = (gp_reg[15] & ~1)+ 2;
   }
   gp_reg[13] = adress;
	return (u8)(cycI + n + cycN + cycPN);
}
//---------------------------------------------------------------------------
// branch
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_bx(void)
{
   if(opcode & 0x80){
   	gp_reg[14] = (gp_reg[15] - 2) | 1;
#if defined(_DEBPRO)
		del_CallStack(gp_reg[14]);
#endif
	}
	gp_reg[15] = gp_reg[(opcode>> 3) & 0xf];
#ifdef _DEBPRO
	if(!(opcode & 0x80))
   	del_CallStack(gp_reg[15]);
#endif
   if(!(gp_reg[15] & 1)){
       gp_reg[15] = (gp_reg[15] & ~3) + 4;
       cpsr &= ~T_BIT;
       exec9 = exec_arm;
       fetch_func = fetch_arm;
   }
   else
       gp_reg[15] = (gp_reg[15] & ~1) + 2;
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_bl(void)
{
	int temp;
   u8 h,res;

   res = (u8)(cycP + cycS);
   h = (u8)((opcode >> 11) & 0x3);
   if(h == 3){
       temp = gp_reg[14];
       gp_reg[14] = (gp_reg[15] - 2) | 1;
#if defined(_DEBPRO)
       del_CallStack(gp_reg[14]);
#endif
       gp_reg[15] = (u32)temp + ((opcode & 0x7FF) << 1);
       if(!(gp_reg[15] & 1)){
           gp_reg[15] = (gp_reg[15] & ~3) + 4;
       	cpsr &= ~T_BIT;
       	exec9 = exec_arm;
			fetch_func = fetch_arm;
       }
       else
           gp_reg[15] = (gp_reg[15] & ~1) + 2;
       return res;
   }
   gp_reg[14] = gp_reg[15] | 1;
   temp = ((opcode & 0x7ff) << 12);
   opcode = READHWORD(gp_reg[15]-2);
  	h = (u8)((opcode >> 11) & 0x3);
  	if(((temp |= ((opcode & 0x7ff) << 1)) & 0x400000))
      	temp = -(0x800000 - temp);
#if defined(_DEBPRO)
   del_CallStack(gp_reg[14]);
#endif
  	gp_reg[15] += temp;
  	if(h == 1){
      	gp_reg[15] = (gp_reg[15] & ~3) + 4;
      	cpsr &= ~T_BIT;
      	exec9 = exec_arm;
		fetch_func = fetch_arm;
  	}
  	else if(h == 3)
   	gp_reg[15] += 2;
   else
       res = res;
   return res;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_bu(void) // opcode 0x380
{
   int offset;

	if((offset = (opcode & 0x7ff)) & 0x400)
   	offset = 0 - (2048 - offset);
   gp_reg[15] += (offset << 1) + 2;
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_b(void) // opcode 0x340
{
   int offset;

   switch((opcode>>8) & 0xF){
       case 0: // beq
           if(!z_flag)
               return cycP;
       break;
       case 1: //BNE
           if(z_flag)
               return cycP;
       break;
       case 2: //BCS
           if(!c_flag)
               return cycP;
       break;
       case 3: //BCC
           if(c_flag)
               return cycP;
       break;
       case 4: //BMI
           if(!n_flag)
               return cycP;
       break;
       case 5://BPL
           if(n_flag)
               return cycP;
       break;
       case 6: //BVS
           if(!v_flag)
               return cycP;
       break;
       case 7: //BVC
           if(v_flag)
               return cycP;
       break;
       case 8: // BHI
           if(!(c_flag && !z_flag))
               return cycP;
       break;
       case 9: //BLS
       	if(!(!c_flag || z_flag))
               return cycP;
       break;
       case 10: //BGE
           if(v_flag != n_flag)
               return cycP;
       break;
       case 11: // BLT
           if(v_flag == n_flag)
               return cycP;
       break;
       case 12: //BGT
           if(!(!z_flag && v_flag == n_flag))
               return cycP;
       break;
       case 13: // BLE
           if(!(z_flag || n_flag != v_flag))
               return cycP;
       break;
   }
   if(((offset = ((u8)opcode) << 1) & 0x100))
       offset = -(0x200 - offset);
   gp_reg[15] += offset + 2;
   CHECK_BRANCH(offset)
	return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u32 read_PC(int *state)
{
	u32 d;

	if(state != NULL)
   	*state = ((cpsr & T_BIT) >> T_SHIFT);
   d = old_pc;
   if((cpsr & T_BIT) != 0){
       if((gp_reg[15] - old_pc) > 4)
           d = gp_reg[15] - 2;
       d &= ~1;
   }
   else{
       if((gp_reg[15] - old_pc) > 8)
           d = gp_reg[15] - 4;
   }
   return d;
}
//---------------------------------------------------------------------------
static void write_PC(u32 value)
{
}
//---------------------------------------------------------------------------
NO_IN_LINE void arm9irq_empty(void)
{
   if((((u32 *)(io_mem + 0x208))[0] & 1)){
       if(((u32 *)(io_mem + 0x210))[0] & nIRQ9)
           arm9irq();
   }
}
//---------------------------------------------------------------------------
BOOL CPU_ARM9::Init()
{
   int i, n;

   pfn_read_word = read_word_data;
   pfn_read_hword = read_hword_data;
   pfn_read_byte = read_byte_data;
   pfn_write_word = write_word_data;
   pfn_write_hword = write_hword_data;
   pfn_write_byte = write_byte_data;
   dbgarm_ins = (DECODEARM *)LocalAlloc(LPTR,0x1400 * sizeof(DECODEARM) + 0x1400 * sizeof(char *));
   if(dbgarm_ins == NULL)
       return FALSE;
   dbgthumb_ins = (DECODEARM *)(dbgarm_ins + 0x1000);
   arm_opcode_strings = (char **)(dbgthumb_ins + 0x400);
   thumb_opcode_strings = (char **)(arm_opcode_strings + 0x1000);
   for(i=0;i<0x1000;i++){
       arm_ins[i] = unknown_ins;
#ifdef _DEBUG
       dbgarm_ins[i] = standard_debug_handle;
       arm_opcode_strings[i] = "Unknown";
#endif
   }
   for(i=0;i<0x400;i++){
       thumb_ins[i] = unknown_ins;
#ifdef _DEBUG
       dbgthumb_ins[i] = standard_debug_handle_thumb;
       thumb_opcode_strings[i] = "Unknown";
#endif
   }
   //arm
   setup_dp_handle(arm_ins,0x00,and_,and_lsl,and_reg,and_imm);//ok
   setup_dp_handle(arm_ins,0x10,ands,ands,ands_reg,ands_imm);//ok
   setup_dp_handle(arm_ins,0x20,eor,eor_lsl,eor_reg,eor_imm);//ok
   setup_dp_handle(arm_ins,0x30,eors,eors,eors_reg,eors_imm);//ok
   setup_dp_handle(arm_ins,0x40,sub,sub_lsl,sub_reg,sub_imm);//ok
   setup_dp_handle(arm_ins,0x50,subs,subs,subs_reg,subs_imm);//ok
   setup_dp_handle(arm_ins,0x60,rs,rs,rs_reg,rs_imm);//ok
   setup_dp_handle(arm_ins,0x70,rss,rss,rss_reg,rss_imm);//ok
   setup_dp_handle(arm_ins,0x80,add,add_lsl,add_reg,add_imm);//ok
   setup_dp_handle(arm_ins,0x90,adds,adds,adds_reg,adds_imm);//ok
   setup_dp_handle(arm_ins,0xA0,adc,adc,adc_reg,adc_imm);//ok
   setup_dp_handle(arm_ins,0xB0,adcs,adcs,adcs_reg,adcs_imm);//ok
   setup_dp_handle(arm_ins,0xC0,subc,subc,subc_reg,subc_imm);//ok
   setup_dp_handle(arm_ins,0xD0,subcs,subcs,subcs_reg,subcs_imm);//ok
   setup_dp_handle(arm_ins,0xE0,rsbc,rsbc,rsbc_reg,rsbc_imm);//ok
   setup_dp_handle(arm_ins,0xF0,rsbcs,rsbcs,rsbcs_reg,rsbcs_imm);//ok
   setup_dp_handle(arm_ins,0x110,tst,tst,tst_reg,tst_imm);//ok
   setup_dp_handle(arm_ins,0x130,teq,teq,teq_reg,teq_imm);//ok
   setup_dp_handle(arm_ins,0x150,cmp,cmp_lsl,cmp_reg,cmp_imm);//ok
   setup_dp_handle(arm_ins,0x170,cmn,cmn,cmn_reg,cmn_imm);//ok
   setup_dp_handle(arm_ins,0x180,orr,orr_lsl,orr_reg,orr_imm);//ok
   setup_dp_handle(arm_ins,0x190,orrs,orrs,orrs_reg,orrs_imm);//ok
   setup_dp_handle(arm_ins,0x1A0,mov,mov_lsl,mov_reg,mov_imm);//ok
   setup_dp_handle(arm_ins,0x1B0,movs,movs,movs_reg,movs_imm);//ok
   setup_dp_handle(arm_ins,0x1C0,bic,bic,bic_reg,bic_imm);//ok
   setup_dp_handle(arm_ins,0x1D0,bics,bics,bics_reg,bics_imm);//ok
   setup_dp_handle(arm_ins,0x1E0,mvn,mvn_lsl,mvn_reg,mvn_imm);//ok
   setup_dp_handle(arm_ins,0x1F0,mvns,mvns,mvns_reg,mvns_imm);//ok

   arm_ins[0x100] = ins_mrs_cpsr;//ok
   arm_ins[0x140] = ins_mrs_spsr;//ok

   arm_ins[0x120] = ins_msr_cpsr;//ok
   arm_ins[0x160] = ins_msr_spsr;//ok

   for (i=0; i<0x10; i++) {
       if(!(i & 1)){
       }
       arm_ins[0x320|i] = ins_msr_cpsr;//ok
       arm_ins[0x360|i] = ins_msr_spsr;
       for(n=0;n<=0xF;n++) {
           arm_ins[0x800|(n<<5)|i] = stm;//ok
           arm_ins[0x810|(n<<5)|i] = ldm;//ok
       }
   }
   setup_sdt_handles2(arm_ins,0x400,str_postdownimm,str_postdownimm,str_postupimm,str_postupimm,str_predownimm,str_preupimm,str_predownimmwb,str_preupimmwb);
   setup_sdt_handles2(arm_ins,0x440,strb_postdownimm,strb_postdownimm,strb_postupimm,strb_postupimm,strb_predownimm,strb_preupimm,strb_predownimmwb,strb_preupimmwb);
   setup_sdt_handles2(arm_ins,0x600,str_postdown,str_postdown,str_postup,str_postup,str_predown,str_preup,str_predownwb,str_preupwb);
   setup_sdt_handles2(arm_ins,0x640,strb_postdown,strb_postdown,strb_postup,strb_postup,strb_predown,strb_preup,strb_predownwb,strb_preupwb);
   setup_sdt_handles2(arm_ins,0x410,ldr_postdownimm,ldr_postdownimm,ldr_postupimm,ldrt_postupimm,ldr_predownimm,ldr_preupimm,ldr_predownimmwb,ldr_preupimmwb);
   setup_sdt_handles2(arm_ins,0x450,ldrb_postdownimm,ldrb_postdownimm,ldrb_postupimm,ldrb_postupimm,ldrb_predownimm,ldrb_preupimm,ldrb_predownimmwb,ldrb_preupimmwb);
   setup_sdt_handles2(arm_ins,0x610,ldr_postdown,ldr_postdown,ldr_postup,ldrt_postup,ldr_predown,ldr_preup,ldr_predownwb,ldr_preupwb);
   setup_sdt_handles2(arm_ins,0x650,ldrb_postdown,ldrb_postdown,ldrb_postup,ldrb_postup,ldrb_predown,ldrb_preup,ldrb_predownwb,ldrb_preupwb);

   setup_hwdt_handles2(arm_ins,0xB,strh_postdown,strh_predown,strh_postup,strh_preup,strh_predownwb,strh_preupwb);
   setup_hwdt_handles2(arm_ins,0x1B,ldrh_postdown,ldrh_predown,ldrh_postup,ldrh_preup,ldrh_predownwb,ldrh_preupwb);
   setup_hwdt_handles2(arm_ins,0x4B,strh_postdownimm,strh_predownimm,strh_postupimm,strh_preupimm,strh_predownimmwb,strh_preupimmwb);
   setup_hwdt_handles2(arm_ins,0x5B,ldrh_postdownimm,ldrh_predownimm,ldrh_postupimm,ldrh_preupimm,ldrh_predownimmwb,ldrh_preupimmwb);

   setup_hwdt_handles2(arm_ins,0x1D,ldrsb_postdown,ldrsb_predown,ldrsb_postup,ldrsb_preup,ldrsb_predownwb,ldrsb_preupwb);
   setup_hwdt_handles2(arm_ins,0x5D,ldrsb_postdownimm,ldrsb_predownimm,ldrsb_postupimm,ldrsb_preupimm,ldrsb_predownimmwb,ldrsb_preupimmwb);
   setup_hwdt_handles2(arm_ins,0x1F,ldrsh_postdown,ldrsh_predown,ldrsh_postup,ldrsh_preup,ldrsh_predownwb,ldrsh_preupwb);
   setup_hwdt_handles2(arm_ins,0x5F,ldrsh_postdownimm,ldrsh_predownimm,ldrsh_postupimm,ldrsh_preupimm,ldrsh_predownimmwb,ldrsh_preupimmwb);

   arm_ins[0x9]  = ins_mul;//ok
   arm_ins[0x19] = ins_muls;//ok
   arm_ins[0x29] = ins_mla;//ok
   arm_ins[0x39] = ins_mlas;//ok
   arm_ins[0x89] = ins_mull_unsigned;
   arm_ins[0x99] = ins_mulls_unsigned;
   arm_ins[0xA9] = ins_mlal_unsigned;
   arm_ins[0xB9] = ins_mlals_unsigned;
   arm_ins[0xC9] = ins_mull;
   arm_ins[0xD9] = ins_mulls;
   arm_ins[0xE9] = ins_mlal;//long mla signed
   arm_ins[0xF9] = ins_mlals;//long mlas signed
   arm_ins[0x109] = ins_swp;//ok
   arm_ins[0x121] = ins_bx;
   arm_ins[0x149] = ins_swpb;//ok
   arm_ins[0x161] = clz;
   for(i=0;i<0x100;i++)
       arm_ins[0xF00|i] = ins_swi;
	arm_ins[0x127] = ins_bkpt;

   for(i=0;i<0x80;i++){
       arm_ins[0xA00|i] = ins_bmi;
       arm_ins[0xA80|i] = ins_bmi;
       arm_ins[0xB00|i] = ins_blmi;
       arm_ins[0xB80|i] = ins_blmi;
   }
   for(i=0;i<0x100;i += 0x10){
       if(i & 0x10){
           for(n=0;n<0x10;n++){
               arm_ins[0xC00+i+n] = ldc;
               arm_ins[0xD00+i+n] = ldc;
           }
       }
       else{
           for(n=0;n<0x10;n++){
               arm_ins[0xC00+i+n] = stc;
               arm_ins[0xD00+i+n] = stc;
           }
       }
   }

   for(i=0;i<0x100;i++){
       if((i & 1)){
           if((i >> 4) & 1)
               arm_ins[0xE00+i] = mrc;
           else
               arm_ins[0xE00+i] = mcr;
       }
       else
           arm_ins[0xE00 + i] = cdp;
   }
   arm_ins[0x123] = ins_blx;
   arm_ins[0xD] = ldrd;
   arm_ins[0x2D] = ldrd;
   arm_ins[0x4D] = ldrd;
   arm_ins[0x6D] = ldrd;
   arm_ins[0x8D] = ldrd;
   arm_ins[0xAD] = ldrd;
   arm_ins[0xCD] = ldrd;
   arm_ins[0xED] = ldrd;
   arm_ins[0x10D] = ldrd;
   arm_ins[0x12D] = ldrd;
   arm_ins[0x14D] = ldrd;
   arm_ins[0x16D] = ldrd;
   arm_ins[0x18D] = ldrd;
   arm_ins[0x1AD] = ldrd;
   arm_ins[0x1CD] = ldrd;
   arm_ins[0x1ED] = ldrd;

   arm_ins[0xF] = strd;
   arm_ins[0x2F] = strd;
   arm_ins[0x4F] = strd;
   arm_ins[0x6F] = strd;
   arm_ins[0x8F] = strd;
   arm_ins[0xAF] = strd;
   arm_ins[0xCF] = strd;
   arm_ins[0xEF] = strd;
   arm_ins[0x10F] = strd;
   arm_ins[0x12F] = strd;
   arm_ins[0x14F] = strd;
   arm_ins[0x16F] = strd;
   arm_ins[0x18F] = strd;
   arm_ins[0x1AF] = strd;
   arm_ins[0x1CF] = strd;
   arm_ins[0x1EF] = strd;

   arm_ins[0x105]  = qadd;
   arm_ins[0x12A]  = smulwy;
   arm_ins[0x12E]  = smulwy;

   arm_ins[0x128]  = smlawy;
   arm_ins[0x12C]  = smlawy;

   arm_ins[0x148] = smlalxy;
   arm_ins[0x14C] = smlalxy;
   arm_ins[0x14A] = smlalxy;
   arm_ins[0x14E] = smlalxy;

   arm_ins[0x108] = smlaxy;
   arm_ins[0x10C] = smlaxy;
   arm_ins[0x10A] = smlaxy;
   arm_ins[0x10E] = smlaxy;

   arm_ins[0x125] = unknown_ins;
   arm_ins[0x165] = unknown_ins;
   arm_ins[0x145] = unknown_ins;

   arm_ins[0x168] = smulxy;
   arm_ins[0x16A] = smulxy;
   arm_ins[0x16C] = smulxy;
   arm_ins[0x16E] = smulxy;

   for(i=0;i<16;i++)
       arm_ins[0xc40+i] = arm_ins[0xc50+i] = unknown_ins;
#ifdef _DEBUG
   setup_dp_strings(arm_opcode_strings,0x00,"and");//ok
   setup_dp_strings(arm_opcode_strings,0x10,"ands");//ok
   setup_dp_strings(arm_opcode_strings,0x20,"eor");//ok
   setup_dp_strings(arm_opcode_strings,0x30,"eors");//ok
   setup_dp_strings(arm_opcode_strings,0x40,"sub");//ok
   setup_dp_strings(arm_opcode_strings,0x50,"subs");//ok
   setup_dp_strings(arm_opcode_strings,0x60,"rsb");//ok
   setup_dp_strings(arm_opcode_strings,0x70,"rsbs");//ok
   setup_dp_strings(arm_opcode_strings,0x80,"add");//ok
   setup_dp_strings(arm_opcode_strings,0x90,"adds");//ok
   setup_dp_strings(arm_opcode_strings,0xA0,"adc");//ok
   setup_dp_strings(arm_opcode_strings,0xB0,"adcs");//ok
   setup_dp_strings(arm_opcode_strings,0xC0,"subc");//ok
   setup_dp_strings(arm_opcode_strings,0xD0,"subcs");//ok
   setup_dp_strings(arm_opcode_strings,0xE0,"rsbc");//ok
   setup_dp_strings(arm_opcode_strings,0xF0,"rsbcs");//ok
   setup_dp_strings(arm_opcode_strings,0x110,"tst");//ok
   setup_dp_strings(arm_opcode_strings,0x130,"teq");//ok
   setup_dp_strings(arm_opcode_strings,0x150,"cmp");//ok
   setup_dp_strings(arm_opcode_strings,0x170,"cmn");//ok
   setup_dp_strings(arm_opcode_strings,0x180,"orr");//ok
   setup_dp_strings(arm_opcode_strings,0x190,"orrs");//ok
   setup_dp_strings(arm_opcode_strings,0x1A0,"mov");//ok
   setup_dp_strings(arm_opcode_strings,0x1B0,"movs");//ok
   setup_dp_strings(arm_opcode_strings,0x1C0,"bic");//ok
   setup_dp_strings(arm_opcode_strings,0x1D0,"bics");//ok
   setup_dp_strings(arm_opcode_strings,0x1E0,"mvn");//ok
   setup_dp_strings(arm_opcode_strings,0x1F0,"mvns");//ok
   for (i=0; i<0x10; i++) {
       if(!(i & 1)){
           arm_opcode_strings[0x100|i] = "mrs";//ok
           arm_opcode_strings[0x120|i] = "msr";
           arm_opcode_strings[0x140|i] = "mrs";
           arm_opcode_strings[0x160|i] = "msr";
       }
       arm_opcode_strings[0x320|i] = "msr";
       arm_opcode_strings[0x360|i] = "msr";
       for (n=0; n<=0xF; n++) {
           arm_opcode_strings[0x800|(n<<5)|i] = "stm";//ok
           arm_opcode_strings[0x810|(n<<5)|i] = "ldm";//ok
       }
   }
   for(i=0;i<0x100;i++){
       if((i >> 4) & 1){
           arm_opcode_strings[0xC00+i] = "ldc";
           arm_opcode_strings[0xD00+i] = "ldc";
       }
       else{
           arm_opcode_strings[0xC00+i] = "stc";
           arm_opcode_strings[0xD00+i] = "stc";
       }
   }
   setup_sdt_strings(arm_opcode_strings,0x600,"str");
   setup_sdt_strings(arm_opcode_strings,0x640,"strb");
   setup_sdt_strings(arm_opcode_strings,0x400,"str");
   setup_sdt_strings(arm_opcode_strings,0x440,"strb");
   setup_sdt_strings(arm_opcode_strings,0x610,"ldr");
   setup_sdt_strings(arm_opcode_strings,0x650,"ldrb");
   setup_sdt_strings(arm_opcode_strings,0x410,"ldr");
   setup_sdt_strings(arm_opcode_strings,0x450,"ldrb");
   setup_sdt_strings(arm_opcode_strings,0x5B,"ldrh");
   setup_sdt_strings(arm_opcode_strings,0x1B,"ldrh");
   setup_sdt_strings_00(arm_opcode_strings,0x5F,"ldrsh");
   setup_sdt_strings_00(arm_opcode_strings,0x1F,"ldrsh");
   setup_sdt_strings_00(arm_opcode_strings,0x5D,"ldrsb");
   setup_sdt_strings_00(arm_opcode_strings,0x1D,"ldrsb");
   setup_sdt_strings(arm_opcode_strings,0x4B,"strh");
   setup_sdt_strings(arm_opcode_strings,0xB, "strh");

   arm_opcode_strings [0x9] = "mul";
   arm_opcode_strings[0x19] = "muls";
   arm_opcode_strings[0x29] = "mla";
   arm_opcode_strings[0x39] = "mlas";
   arm_opcode_strings[0x89] = "umull";
   arm_opcode_strings[0x99] = "umulls";
   arm_opcode_strings[0xC9] = "smull";
   arm_opcode_strings[0xE9] = "smulls";
   arm_opcode_strings[0xA9] = "umlal";
   arm_opcode_strings[0xB9] = "umlals";
   arm_opcode_strings[0xE9] = "smlal";
   arm_opcode_strings[0xF9] = "smlals";
   arm_opcode_strings[0x121] = "bx";
   arm_opcode_strings[0x109] = "swp";
   arm_opcode_strings[0x149] = "swpb";
   arm_opcode_strings[0x161] = "clz";
   for(i=0;i<0xFF;i++){
       if((i & 1)){
           if((i >> 4) & 1)
               arm_opcode_strings[0xE00+i] = "mrc";
           else
               arm_opcode_strings[0xE00+i] = "mcr";
       }
       else
           arm_opcode_strings[0xE00 + i] = "cdp";
   }
   for(i=0;i<0x100;i++)
       arm_opcode_strings[0xF00|i] = "swi";
	arm_opcode_strings[0x127] = "bkpt";

   for(i=0;i<0x100;i++){
       arm_opcode_strings[0xA00|i] = "b";
       arm_opcode_strings[0xB00|i] = "bl";
   }
   for(i=0;i<0x1000;i++)
       dbgarm_ins[i] = standard_debug_handle;
   setup_dp_debug_handle(dbgarm_ins,0x00, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins, 0x10, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins, 0x20, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins, 0x30, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins, 0x40, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins, 0x50, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins, 0x60, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins, 0x70, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins, 0x80, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins, 0x90, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins, 0xA0, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins, 0xB0, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins, 0xC0, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins, 0xD0, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins, 0xE0, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins, 0xF0, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins,0x110, dpnw_debug_handle);
   setup_dp_debug_handle(dbgarm_ins,0x130, dpnw_debug_handle);
   setup_dp_debug_handle(dbgarm_ins,0x150, dpnw_debug_handle);
   setup_dp_debug_handle(dbgarm_ins,0x170, dpnw_debug_handle);
   setup_dp_debug_handle(dbgarm_ins,0x180, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins,0x190, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins,0x1A0, dpsingle_debug_handle);
   setup_dp_debug_handle(dbgarm_ins,0x1B0, dpsingle_debug_handle);
   setup_dp_debug_handle(dbgarm_ins,0x1C0, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins,0x1D0, dp_debug_handle);
   setup_dp_debug_handle(dbgarm_ins,0x1E0, dpsingle_debug_handle);
   setup_dp_debug_handle(dbgarm_ins,0x1F0, dpsingle_debug_handle);

   for(i=0;i<0x10;i++){
       if(!(i & 1)){
           dbgarm_ins[0x100|i] = msr_debug_handle;//ok
           dbgarm_ins[0x120|i] = msr_debug_handle;//ok
           dbgarm_ins[0x140|i] = msr_debug_handle;//ok
           dbgarm_ins[0x160|i] = msr_debug_handle;//ok
       }
       dbgarm_ins[0x320|i] = msr_debug_handle;//ok
       dbgarm_ins[0x360|i] = msr_debug_handle;
       for(n=0;n<=0xF;n++){
           dbgarm_ins[0x800|(n<<5)|i] = mdt_debug_handle;
           dbgarm_ins[0x810|(n<<5)|i] = mdt_debug_handle;
       }
   }

   setup_sdt_debug_handles(dbgarm_ins,0x600, sdt_debug_handle);
   setup_sdt_debug_handles(dbgarm_ins,0x640, sdt_debug_handle);
   setup_sdt_debug_handles(dbgarm_ins,0x400, sdt_debug_handle);
   setup_sdt_debug_handles(dbgarm_ins,0x440, sdt_debug_handle);
   setup_sdt_debug_handles(dbgarm_ins,0x610, sdt_debug_handle);
   setup_sdt_debug_handles(dbgarm_ins,0x650, sdt_debug_handle);
   setup_sdt_debug_handles(dbgarm_ins,0x410, sdt_debug_handle);
   setup_sdt_debug_handles(dbgarm_ins,0x450, sdt_debug_handle);

   setup_hwdt_debug_handles(dbgarm_ins,0x5B|0x4,hwdt_debug_handle);
   setup_hwdt_debug_handles(dbgarm_ins,0x5D,hwdt_debug_handle);
   setup_hwdt_debug_handles(dbgarm_ins,0x1B,hwdt_debug_handle);
   setup_hwdt_debug_handles(dbgarm_ins,0x1B|0x4,hwdt_debug_handle);
   setup_hwdt_debug_handles(dbgarm_ins,0x1D,hwdt_debug_handle);
   setup_hwdt_debug_handles(dbgarm_ins,0x4B,hwdt_debug_handle);
   setup_hwdt_debug_handles(dbgarm_ins,0xB,hwdt_debug_handle);
   setup_hwdt_debug_handles(dbgarm_ins,0x5B,hwdt_debug_handle);

   dbgarm_ins [0x9] = mul_debug_handle;
   dbgarm_ins[0x19] = mul_debug_handle;
   dbgarm_ins[0x29] = mul_debug_handle;
   dbgarm_ins[0x39] = mul_debug_handle;
   dbgarm_ins[0x89] = mull_debug_handle;
   dbgarm_ins[0x99] = mull_debug_handle;
   dbgarm_ins[0xC9] = mull_debug_handle;
   dbgarm_ins[0xE9] = mull_debug_handle;
   dbgarm_ins[0xA9] = mull_debug_handle;
   dbgarm_ins[0xB9] = mull_debug_handle;
   dbgarm_ins[0xE9] = mull_debug_handle;
   dbgarm_ins[0xF9] = mull_debug_handle;
   dbgarm_ins[0x109] = swp_debug_handle;
   dbgarm_ins[0x149] =	swp_debug_handle;

   dbgarm_ins[0x121] = bx_debug_handle_arm;
   dbgarm_ins[0x161] =	clz_debug_handle_arm;

   for (i=0; i<0x100; i++) {
       dbgarm_ins[0xF00|i] = swi_debug_handle;
       dbgarm_ins[0xA00|i] = b_debug_handle_arm;
       dbgarm_ins[0xB00|i] = b_debug_handle_arm;
       dbgarm_ins[0xE00|i] = sc_debug_handle_arm;
   }
   arm_opcode_strings[0x123] = "blx";
   dbgarm_ins[0x123] = bx_debug_handle_arm;
#endif
   //thumb
   for(i=0;i<0x400;i++)
       thumb_ins[i] = unknown_ins;
   for (i=0; i<0x8; i++) {
       thumb_ins [0x60|i] = tins_add;//ok
       thumb_ins [0x68|i] = tins_sub;//ok
       thumb_ins [0x70|i] = tins_add_short_imm;//ok
       thumb_ins [0x78|i] = tins_sub_reg_imm;//ok
       thumb_ins [0x140|i] = tins_str_reg;//ok
       thumb_ins [0x150|i] = tins_strb_reg;//ok
       thumb_ins [0x160|i] = tins_ldr_reg;//ok
       thumb_ins [0x170|i] = tins_ldrb_reg;//ok
       thumb_ins [0x148|i] = tins_strh_reg;//ok
       thumb_ins [0x158|i] = tins_ldrsb_reg;//ok
       thumb_ins [0x168|i] = tins_ldrh_reg;//ok
       thumb_ins [0x178|i] = tins_ldrsh;//ok
       thumb_ins [0x2D0|i] = tins_push;
       thumb_ins [0x2F0|i] = tins_pop;
   }
   for(i=0;i<0x20; i++) {
       thumb_ins[i] =         tins_lsl_imm;//ok
       thumb_ins[0x20|i] =    tins_lsr_imm;//ok
       thumb_ins[0x40|i] =    tins_asr_imm;//ok
       thumb_ins[0x80|i] =    tins_mov_imm;//ok
       thumb_ins[0xA0|i] =    tins_cmp_imm;//ok
       thumb_ins[0xC0|i] =    tins_add_imm;//ok
       thumb_ins[0xE0|i] =    tins_sub_imm;//ok
       thumb_ins[0x120|i] =   tins_pc_rel_ldr;//ok//OK
       thumb_ins[0x180|i] =   tins_str_imm;//ok32
       thumb_ins[0x1A0|i] =   tins_ldr_imm;//ok32
       thumb_ins[0x1C0|i] =   tins_strb_imm;//ok
       thumb_ins[0x1E0|i] =   tins_ldrb_imm;//OK
       thumb_ins[0x200|i] =   tins_strh_imm;//OK
       thumb_ins[0x220|i] =   tins_ldrh_imm;//OK
       thumb_ins[0x240|i] =   tins_sp_rel_str;//ok
       thumb_ins[0x260|i] =   tins_sp_rel_ldr;//ok
       thumb_ins[0x280|i] =   tins_add_pc;//ok
       thumb_ins[0x2A0|i] =   tins_add_sp;//ok
       thumb_ins[0x300|i] =   tins_stmia;//ok
       thumb_ins[0x320|i] =   tins_ldmia;//ok
   }
   for (i=0;i<0x20;i++){
       thumb_ins[0x380|i] =   tins_bu;
       thumb_ins[0x3A0|i] =   tins_bl;
       thumb_ins[0x3C0|i] =   tins_bl;
       thumb_ins[0x3E0|i] =   tins_bl;
   }
   for(i=0;i<0x3c;i++)
       thumb_ins[0x340|i] =   tins_b;
   for(i=0;i<4;i++)
       thumb_ins[0x37C|i] =   tins_swi;
   for(i=0;i<4;i++)
       thumb_ins[0x2F8|i] =   tins_bkpt;
   thumb_ins[0x105] =         tins_adc_reg;
   thumb_ins[0x106] =         tins_sbc;
   thumb_ins[0x108] =         tins_tst;
   thumb_ins[0x107] =         tins_ror_reg;
   thumb_ins[0x109] =         tins_neg;
   thumb_ins[0x100] =         tins_and;
   thumb_ins[0x101] =         tins_eor;
   thumb_ins[0x102] =         tins_lsl_reg;
   thumb_ins[0x10d] =         tins_mul;
   thumb_ins[0x10c] =         tins_orr;
   thumb_ins[0x10f] =         tins_mvn;
   thumb_ins[0x10e] =         tins_bic;
   thumb_ins[0x103] =         tins_lsr_reg;
   thumb_ins[0x104] =         tins_asr_reg;
   thumb_ins[0x10A] =         tins_cmp;//ok
   thumb_ins[0x10b] =         tins_cmn_reg;
   thumb_ins[0x110] =         tins_add_lo_hi;//ok
   thumb_ins[0x111] =         tins_add_lo_hi;//ok
   thumb_ins[0x112] =         tins_add_lo_hi;//ok
   thumb_ins[0x113] =         tins_add_lo_hi;//ok
   thumb_ins[0x114] =         tins_cmp_reg;//ok
   thumb_ins[0x115] =         tins_cmp_reg;//ok
   thumb_ins[0x116] =         tins_cmp_reg;//ok
   thumb_ins[0x117] =         tins_cmp_reg;//ok
   thumb_ins[0x118] =         tins_mov_hi_lo;//ok
   thumb_ins[0x119] =         tins_mov_hi_lo;//ok
   thumb_ins[0x11A] =         tins_mov_hi_lo;//ok
   thumb_ins[0x11B] =         tins_mov_hi_lo;//ok
   thumb_ins[0x11c] =         tins_bx;//ok
   thumb_ins[0x11D] =         tins_bx;//ok
   thumb_ins[0x11e] =         tins_bx;//ok
   thumb_ins[0x11f] =         tins_bx;//ok
   thumb_ins[0x2C0] =         tins_add_rel_sp;//ok
   thumb_ins[0x2C1] =         tins_add_rel_sp;//ok
   thumb_ins[0x2c2] =         tins_sub_sp;//ok
   thumb_ins[0x2c3] =         tins_sub_sp;//ok
#ifdef _DEBUG
   for (i=0; i<0x8; i++) {
       dbgthumb_ins[0x60|i] = reg_debug_handle_t;
       dbgthumb_ins[0x68|i] = reg_debug_handle_t;
       dbgthumb_ins[0x70|i]  = immshort_debug_handle_t;//ok
       dbgthumb_ins[0x78|i]  = immshort_debug_handle_t;//ok
       dbgthumb_ins[0x140|i] = reg_debug_handle_t;//ok
       dbgthumb_ins[0x150|i] = reg_debug_handle_t;//ok
       dbgthumb_ins[0x160|i] = reg_debug_handle_t;//ok
       dbgthumb_ins[0x170|i] = reg_debug_handle_t;//ok
       dbgthumb_ins[0x148|i] = reg_debug_handle_t;//ok
       dbgthumb_ins[0x158|i] = reg_debug_handle_t;//ok
       dbgthumb_ins[0x168|i] = reg_debug_handle_t;//ok
       dbgthumb_ins[0x178|i] = reg_debug_handle_t;//ok
       dbgthumb_ins[0x2D0|i] = stack_debug_handle_t;
       dbgthumb_ins[0x2F0|i] = stack_debug_handle_t;
   }
   for(i=0;i<0x20; i++) {
       dbgthumb_ins[i]       = immediate_debug_handle_t;//ok
       dbgthumb_ins[0x20|i]  = immediate_debug_handle_t;//ok
       dbgthumb_ins[0x40|i]  = immediate_debug_handle_t;//ok
       dbgthumb_ins[0x80|i]  = immlong_debug_handle_t;//ok
       dbgthumb_ins[0xA0|i]  = immlong_debug_handle_t;//ok
       dbgthumb_ins[0xC0|i]  = immlong_debug_handle_t;//ok
       dbgthumb_ins[0xE0|i]  = immlong_debug_handle_t;//ok
       dbgthumb_ins[0x120|i] = rs_debug_handle_t;
       dbgthumb_ins[0x180|i] = immediate_debug_handle_t4;//ok32
       dbgthumb_ins[0x1A0|i] = immediate_debug_handle_t4;//ok32
       dbgthumb_ins[0x1C0|i] = immediate_debug_handle_t;//ok
       dbgthumb_ins[0x1E0|i] = immediate_debug_handle_t;//OK
       dbgthumb_ins[0x200|i] = immediate_debug_handle_t2;//OK
       dbgthumb_ins[0x220|i] = immediate_debug_handle_t2;//OK
       dbgthumb_ins[0x240|i] = rs_debug_handle_t;//ok
       dbgthumb_ins[0x260|i] = rs_debug_handle_t;//ok
       dbgthumb_ins[0x280|i] = rs_debug_handle_t;//ok
       dbgthumb_ins[0x2A0|i] = rs_debug_handle_t;//ok
       dbgthumb_ins[0x300|i] = mdt_debug_handle_t;//ok
       dbgthumb_ins[0x320|i] = mdt_debug_handle_t;//ok
   }
   for (i=0; i<0x20; i++){
       dbgthumb_ins[0x380|i] = b_debug_handle_t;
       dbgthumb_ins[0x3A0|i] = bl_debug_handle_t;
       dbgthumb_ins[0x3C0|i] = bl_debug_handle_t;
       dbgthumb_ins[0x3E0|i] = bl_debug_handle_t;
   }
   for(i=0;i<0x3c;i++)
       dbgthumb_ins[0x340|i] = bcond_debug_handle_t;
   for(i=0;i<4;i++)
       dbgthumb_ins[0x37c|i] = swi_debug_handle_t;
   dbgthumb_ins[0x105] = standard_debug_handle_t;
   dbgthumb_ins[0x106] = standard_debug_handle_t;
   dbgthumb_ins[0x108] = standard_debug_handle_t;
   dbgthumb_ins[0x107] = standard_debug_handle_t;
   dbgthumb_ins[0x109] = standard_debug_handle_t;
   dbgthumb_ins[0x100] = standard_debug_handle_t;
   dbgthumb_ins[0x101] = standard_debug_handle_t;
   dbgthumb_ins[0x102] = standard_debug_handle_t;
   dbgthumb_ins[0x10d] = standard_debug_handle_t;
   dbgthumb_ins[0x10c] = standard_debug_handle_t;
   dbgthumb_ins[0x10f] = standard_debug_handle_t;
   dbgthumb_ins[0x10e] = standard_debug_handle_t;
   dbgthumb_ins[0x103] = standard_debug_handle_t;
   dbgthumb_ins[0x104] = standard_debug_handle_t;
   dbgthumb_ins[0x10A] = standard_debug_handle_t;//ok
   dbgthumb_ins[0x10b] = standard_debug_handle_t;
   dbgthumb_ins[0x110] = hireg_debug_handle_t;//ok
   dbgthumb_ins[0x111] = hireg_debug_handle_t;//ok
   dbgthumb_ins[0x112] = hireg_debug_handle_t;//ok
   dbgthumb_ins[0x113] = hireg_debug_handle_t;//ok
   dbgthumb_ins[0x114] = hireg_debug_handle_t;//ok
   dbgthumb_ins[0x115] = hireg_debug_handle_t;//ok
   dbgthumb_ins[0x116] = hireg_debug_handle_t;//ok
   dbgthumb_ins[0x117] = hireg_debug_handle_t;//ok
   dbgthumb_ins[0x118] = hireg_debug_handle_t;//ok
   dbgthumb_ins[0x119] = hireg_debug_handle_t;//ok
   dbgthumb_ins[0x11A] = hireg_debug_handle_t;//ok
   dbgthumb_ins[0x11B] = hireg_debug_handle_t;//ok
   dbgthumb_ins[0x11c] = bx_debug_handle_t;//ok
   dbgthumb_ins[0x11D] = bx_debug_handle_t;//ok
   dbgthumb_ins[0x11e] = bx_debug_handle_t;//ok
   dbgthumb_ins[0x11f] = bx_debug_handle_t;//ok
   dbgthumb_ins[0x2C0] = rs_debug_handle_t;//ok
   dbgthumb_ins[0x2C1] = rs_debug_handle_t;//ok
   dbgthumb_ins[0x2c2] = rs_debug_handle_t;//ok
   dbgthumb_ins[0x2c3] = rs_debug_handle_t;//ok
   for (i=0; i<0x8; i++) {
       thumb_opcode_strings [0x60|i]  = "add";
       thumb_opcode_strings [0x68|i]  = "sub";
       thumb_opcode_strings [0x70|i]  = "add";
       thumb_opcode_strings [0x78|i]  = "sub";
       thumb_opcode_strings [0x140|i] = "str";
       thumb_opcode_strings [0x150|i] = "strb";
       thumb_opcode_strings [0x160|i] = "ldr";
       thumb_opcode_strings [0x170|i] = "ldrb";
       thumb_opcode_strings [0x148|i] = "strh";
       thumb_opcode_strings [0x158|i] = "ldrsb";
       thumb_opcode_strings [0x168|i] = "ldrh";
       thumb_opcode_strings [0x178|i] = "ldrsh";
       thumb_opcode_strings [0x2D0|i] = "push";
       thumb_opcode_strings [0x2F0|i] = "pop";
   }
   for(i=0;i<0x20; i++) {
       thumb_opcode_strings[i] =        "lsl";
       thumb_opcode_strings[0x20|i]  =  "lsr";
       thumb_opcode_strings[0x40|i]  =  "asr";
       thumb_opcode_strings[0x80|i]  =  "mov";
       thumb_opcode_strings[0xA0|i]  =  "cmp";
       thumb_opcode_strings[0xC0|i]  =  "add";
       thumb_opcode_strings[0xE0|i]  =  "sub";
       thumb_opcode_strings[0x120|i] =  "ldr";
       thumb_opcode_strings[0x180|i] =  "str";
       thumb_opcode_strings[0x1A0|i] =  "ldr";
       thumb_opcode_strings[0x1C0|i] =  "strb";
       thumb_opcode_strings[0x1E0|i] =  "ldrb";
       thumb_opcode_strings[0x200|i] =  "strh";
       thumb_opcode_strings[0x220|i] =  "ldrh";
       thumb_opcode_strings[0x240|i] =  "str";
       thumb_opcode_strings[0x260|i] =  "ldr";
       thumb_opcode_strings[0x280|i] =  "add";
       thumb_opcode_strings[0x2A0|i] =  "add";
       thumb_opcode_strings[0x300|i] =  "stmia";
       thumb_opcode_strings[0x320|i] =  "ldmia";
   }
   for (i=0; i<0x20; i++){
       thumb_opcode_strings[0x380|i] =  "b";
       thumb_opcode_strings[0x3A0|i] =  "bl";
       thumb_opcode_strings[0x3C0|i] =  "bl";
       thumb_opcode_strings[0x3E0|i] =  "bl";
   }
   for(i=0;i<0x3c;i++)
       thumb_opcode_strings[0x340|i] =  "b";
   for(i=0;i<4;i++){
       thumb_opcode_strings[0x37c|i] =  "swi";
       thumb_opcode_strings[0x2F8|i] =  "bkpt";
   }

   thumb_opcode_strings[0x105] =        "adc";
   thumb_opcode_strings[0x106] =        "sbc";
   thumb_opcode_strings[0x108] =        "tst";
   thumb_opcode_strings[0x107] =        "ror";
   thumb_opcode_strings[0x109] =        "neg";
   thumb_opcode_strings[0x100] =        "and";
   thumb_opcode_strings[0x101] =        "eor";
   thumb_opcode_strings[0x102] =        "lsl";
   thumb_opcode_strings[0x10d] =        "mul";
   thumb_opcode_strings[0x10c] =        "orr";
   thumb_opcode_strings[0x10f] =        "mvn";
   thumb_opcode_strings[0x10e] =        "bic";
   thumb_opcode_strings[0x103] =        "lsr";
   thumb_opcode_strings[0x104] =        "asr";
   thumb_opcode_strings[0x10A] =        "cmp";
   thumb_opcode_strings[0x10b] =        "cmn";
   thumb_opcode_strings[0x110] =        "add";
   thumb_opcode_strings[0x111] =        "add";
   thumb_opcode_strings[0x112] =        "add";
   thumb_opcode_strings[0x113] =        "add";
   thumb_opcode_strings[0x114] =        "cmp";
   thumb_opcode_strings[0x115] =        "cmp";
   thumb_opcode_strings[0x116] =        "cmp";
   thumb_opcode_strings[0x117] =        "cmp";
   thumb_opcode_strings[0x118] =        "mov";
   thumb_opcode_strings[0x119] =        "mov";
   thumb_opcode_strings[0x11A] =        "mov";
   thumb_opcode_strings[0x11B] =        "mov";
   thumb_opcode_strings[0x11c] =        "bx";
   thumb_opcode_strings[0x11D] =        "bx";
   thumb_opcode_strings[0x11e] =        "blx";
   thumb_opcode_strings[0x11f] =        "bx";
   thumb_opcode_strings[0x2C0] =        "add";
   thumb_opcode_strings[0x2C1] =        "add";
   thumb_opcode_strings[0x2c2] =        "sub";
   thumb_opcode_strings[0x2c3] =        "sub";
   //dsp inst
   arm_opcode_strings[0x105] = "qadd";
   arm_opcode_strings[0x128] = "smlaw";
   arm_opcode_strings[0x12C] = "smlaw";
   dbgarm_ins[0x128] = smlawy_debug_handle_arm;
   dbgarm_ins[0x12C] = smlawy_debug_handle_arm;
   dbgarm_ins[0x105] = qadd_debug_handle_arm;

   setup_sdt_strings_00(arm_opcode_strings,0xD,"ldrd");
   setup_sdt_strings_00(arm_opcode_strings,0x4D,"ldrd");
   setup_sdt_strings_00(arm_opcode_strings,0xF,"strd");
   setup_sdt_strings_00(arm_opcode_strings,0x4F,"strd");

   setup_hwdt_debug_handles(dbgarm_ins,0xD,ldrd_debug_handle_arm);
   setup_hwdt_debug_handles(dbgarm_ins,0x2D,ldrd_debug_handle_arm);
   setup_hwdt_debug_handles(dbgarm_ins,0x4D,ldrd_debug_handle_arm);

   setup_hwdt_debug_handles(dbgarm_ins,0xF,ldrd_debug_handle_arm);
   setup_hwdt_debug_handles(dbgarm_ins,0x2F,ldrd_debug_handle_arm);
   setup_hwdt_debug_handles(dbgarm_ins,0x4F,ldrd_debug_handle_arm);

   for(i=0;i<7;i+=2){
       dbgarm_ins[i+0x148] = dbgarm_ins[i+0x108] = smlaxy_debug_handle_arm;
       dbgarm_ins[i+0x168] = smulxy_debug_handle_arm;
       arm_opcode_strings[i+0x108] = "smla";
       arm_opcode_strings[i+0x168] = "smul";
       arm_opcode_strings[i+0x148] = "smlal";
   }
   dbgarm_ins[0x12A] = dbgarm_ins[0x12E] = smlawy_debug_handle_arm;
   arm_opcode_strings[0x12A]= arm_opcode_strings[0x12E] = "smulw";
#endif
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL CPU_ARM9::EnableCheats(void **func)
{
   if(func[0] != NULL)
		pfn_read_word = (u32 I_FASTCALL (*)(u32))func[0];
   else
		pfn_read_word = ::read_word_data;
   func[0] = (void *)::read_word_data;
	if(func[1] != NULL)
		pfn_read_hword = (u32 I_FASTCALL (*)(u32))func[1];
   else
   	pfn_read_hword = read_hword_data;
   func[1] = (void *)read_hword_data;
   if(func[2] != NULL)
  		pfn_read_byte = (u32 I_FASTCALL (*)(u32))func[2];
   else
		pfn_read_byte = read_byte_data;
   func[2] = (void *)read_byte_data;
   if(func[3] != NULL)
		pfn_write_word = (void I_FASTCALL (*)(u32,u32))func[3];
   else
		pfn_write_word = write_word_data;
   func[3] = (void *)write_word_data;
   if(func[4] != NULL)
		pfn_write_hword = (void I_FASTCALL (*)(u32,u32))func[4];
   else
		pfn_write_hword = write_hword_data;
   func[4] = (void *)write_hword_data;
   if(func[5] != NULL)
		pfn_write_byte = (void I_FASTCALL (*)(u32,u32))func[5];
   else
		pfn_write_byte = write_byte_data;
   func[5] = (void *)write_byte_data;
   func[6] = (void *)read_PC;
   func[7] = (void *)write_PC;
   return TRUE;
}
//---------------------------------------------------------------------------
void CPU_ARM9::Reset()
{
#ifdef _DEBPRO
	callStack.clear();
#endif
	ZeroMemory(reg,sizeof(reg));
   q_flag = c_flag = z_flag = v_flag = n_flag = 0;
   opcode = 0;
   stop = 0;
   cpsr = SYSTEM_MODE;
   exec9 = exec_arm7;
   fetch_func = fetch_arm;
   nIRQ9 = 0;
   reg[4][13] = 0x027FFD90;
   reg[2][13] = 0x00804000 - 0x100;
   reg[1][13] = reg[2][13] - 0x100;
   reg[0][13] = reg[1][13] - 0x100;
   memcpy(gp_reg,reg[0],sizeof(gp_reg));
   ZeroMemory(itcm_mem,sizeof(itcm_mem));
   ZeroMemory(dtcm_mem,sizeof(dtcm_mem));
   pfn_arm9irq = arm9irq_empty;
}
//---------------------------------------------------------------------------
u8 CPU_ARM9::r_stop()
{
	return stop;
}
//---------------------------------------------------------------------------
void CPU_ARM9::w_stop(u8 value)
{
	stop = (u8)(value & 1);
   fromStopped = ds.get_CurrentCycles();
}
//---------------------------------------------------------------------------
u8 CPU_ARM9::r_cf()
{
	return c_flag;
}
//---------------------------------------------------------------------------
void CPU_ARM9::w_cf(u8 value)
{
	c_flag = (u8)(value & 1);
}
//---------------------------------------------------------------------------
u8 CPU_ARM9::r_nf()
{
	return n_flag;
}
//---------------------------------------------------------------------------
void CPU_ARM9::w_nf(u8 value)
{
	n_flag = (u8)(value & 1);
}
//---------------------------------------------------------------------------
u8 CPU_ARM9::r_zf()
{
	return z_flag;
}
//---------------------------------------------------------------------------
void CPU_ARM9::w_zf(u8 value)
{
	z_flag = (u8)(value & 1);
}
//---------------------------------------------------------------------------
u8 CPU_ARM9::r_vf()
{
	return v_flag;
}
//---------------------------------------------------------------------------
void CPU_ARM9::w_vf(u8 value)
{
	v_flag = (u8)(value & 1);
}
//---------------------------------------------------------------------------
u8 CPU_ARM9::r_qf()
{
	return q_flag;
}
//---------------------------------------------------------------------------
void CPU_ARM9::w_qf(u8 value)
{
	q_flag = (u8)(value & 1);
}
//---------------------------------------------------------------------------
u32 CPU_ARM9::r_opcode()
{
	return opcode;
}
//---------------------------------------------------------------------------
u32 CPU_ARM9::r_modereg(int mode,int index)
{
	switch(mode){
   	case USER_MODE:
       case SYSTEM_MODE:
			return reg[0][index % 17];
       case IRQ_MODE:
       	return reg[1][index % 17];
       case SUPERVISOR_MODE:
       	return reg[2][index % 17];
       case FIQ_MODE:
       	return reg[3][index % 17];
       case ABORT_MODE:
       	return reg[4][index % 17];
       case UNDEFINED_MODE:
			return reg[5][index % 17];
   }
   return 0;
}
//---------------------------------------------------------------------------
u32 CPU_ARM9::r_gpreg(int index)
{
	if(index == -1)
   	return old_pc;
	return gp_reg[index % 17];
}
//---------------------------------------------------------------------------
void CPU_ARM9::w_gpreg(int index,u32 value)
{
	gp_reg[index % 17] = value;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u32 int_remap_address(u32 address)
{
/*   if(address >= sysCnt.itcm_address && address < sysCnt.itcm_end){
       if(!(sysCnt.reg1 & 0x80000))
			address = 0xFE000000 | (address & 0x7FFF);
   }
	else if(sysCnt.dtcm_enable){
   	if(address >= sysCnt.dtcm_address && address < sysCnt.dtcm_end){
   		if(!(sysCnt.reg1 & 0x20000))
   			address = 0xFD000000 | (address & 0x3FFF);
       }
   }*/
   return address;
}
//---------------------------------------------------------------------------
u32 CPU_ARM9::remap_address(u32 address)
{
   return int_remap_address(address);
}
//---------------------------------------------------------------------------
void CPU_ARM9::decode_ins(u8 mode, u32 adress, char *dest)
{
   u32 op,adr;

   adr = int_remap_address(adress);
	if(mode){
       op = read_mem(adr,AMM_HWORD);
  		wsprintf(dest,"%08X %08X ",adress,op);
       dbgthumb_ins[op >> 6](thumb_opcode_strings,op,adress,dest+18,this);
   }
   else{
       op = read_mem(adr,AMM_WORD);
  	    wsprintf(dest,"%08X %08X ",adress,op);
  		dbgarm_ins[((op & 0x0FF00000)>>16)|(((u8)op)>>4)](arm_opcode_strings,op,adress,dest+18,this);
   }
}
//---------------------------------------------------------------------------
void CPU_ARM9::InitPipeLine(u32 offset)
{
   opcode = read_word(offset);
   old_pc = offset;
   gp_reg[15] = offset + 8;
   exec9 = exec_arm;
}
//---------------------------------------------------------------------------
inline u32 CPU_ARM9::r_cpsr()
{
	return cpsr;
}
//---------------------------------------------------------------------------
void CPU_ARM9::w_cpsr(u32 value)
{
	cpsr = value;
}
//---------------------------------------------------------------------------
LVector<unsigned long> *CPU_ARM9::r_callstack()
{
#ifdef _DEBPRO
	return &callStack;
#else
	return NULL;
#endif
}
//---------------------------------------------------------------------------
LPMEMORYINFO CPU_ARM9::r_memmap()
{
	return MemoryAddress;
}
//---------------------------------------------------------------------------
inline int CPU_ARM9::r_memmapsize()
{
	return (int)(sizeof(MemoryAddress) / sizeof(MEMORYINFO));
}
//---------------------------------------------------------------------------
u32 CPU_ARM9::read_mem(u32 address,u8 mode)
{
	switch(mode){
   	case AMM_WORD:
			I_TCM(address,*((u32 *)(itcm_mem + (address & 0x7FFC))),
				*((u32 *)(dtcm_mem + (address & 0x3FFC))))
			switch((address >> 24)){
				case 4:
           		if((address & 0x00100000))
           			address = 0x2000 + (address & 0xFC);
                   else if(address & 0x4000)
				        address = 0x2800 + (address & 0x3FC);
           		else
						address &= 0x1FFC;
           		return *(u32 *)(io_mem + address);
       		default:
					return read_word(address);
   		}
       case AMM_HWORD:
			I_TCM(address,*((u16 *)(itcm_mem + (address & 0x7FFE))),
				*((u16 *)(dtcm_mem + (address & 0x3FFE))))
			switch((address >> 24)){
				case 4:
           		if((address & 0x00100000))
           			address = 0x2000 + (address & 0xFE);
           		else if((address & 0x4000))
           			address = 0x2800 + (address & 0x3FE);
           		else
						address &= 0x1FFE;
           		return *(u16 *)(io_mem + address);
       		default:
					return read_hword(address);
   		}
       case AMM_BYTE:
			I_TCM(address,itcm_mem[address & 0x7FFF],dtcm_mem[address & 0x3FFF])
			switch((address >> 24)){
				case 4:
           		if((address & 0x00100000))
           			address = 0x2000 + (address & 0xFF);
           		else if(address & 0x00004000)
           			address = 0x2800 + (address & 0x3FF);
           		else
						address &= 0x1FFF;
           		return *(u8 *)(io_mem + address);
       		default:
					return read_byte(address);
   		}
   }
   return 0;
}
//---------------------------------------------------------------------------
void CPU_ARM9::write_mem(u32 address,u32 data,u8 mode)
{
	switch(mode){
   	case AMM_WORD:
	        O_TCM(address,*((u32 *)(itcm_mem + (address & 0x7FFC))) = (u32)data,
		        *((u32 *)(dtcm_mem + (address & 0x3FFC))) = (u32)data)
  	        write_word(address,data);
       break;
       case AMM_HWORD:
	        O_TCM(address,*((u16 *)(itcm_mem + (address & 0x7FFE))) = (u16)data,
		        *((u16 *)(dtcm_mem + (address & 0x3FFE))) = (u16)data)
           write_hword(address,data);
       break;
       case AMM_BYTE:
	        O_TCM(address,itcm_mem[address & 0x7FFF] = (u8)data,
		        dtcm_mem[address & 0x3FFF] = (u8)data)
  	        write_byte(address,data);
       break;
   }
}
//---------------------------------------------------------------------------
BOOL CPU_ARM9::Load(LStream *pFile,int ver)
{
	if(pFile == NULL || !pFile->IsOpen())
   	return FALSE;
   pFile->Read(&opcode,sizeof(u32));
   pFile->Read(&z_flag,sizeof(u8));
   pFile->Read(&n_flag,sizeof(u8));
   pFile->Read(&c_flag,sizeof(u8));
   pFile->Read(&v_flag,sizeof(u8));
   pFile->Read(&q_flag,sizeof(u8));
   if(ver < 42)
   	pFile->Read(reg,272);
   else
  		pFile->Read(reg,sizeof(reg));
  	pFile->Read(gp_reg,sizeof(gp_reg));
   pFile->Read(&cpsr,sizeof(u32));
   pFile->Read(&old_pc,sizeof(u32));
   pFile->Read(&stop,sizeof(u8));
   pFile->Read(itcm_mem,32768);
   pFile->Read(dtcm_mem,16384);
   pFile->Read(&nIRQ9,sizeof(nIRQ9));
   if(cpsr & T_BIT){
       exec9 = exec_thumb;
		fetch_func = fetch_thumb;
   }
   else{
       exec9 = exec_arm;
       fetch_func = fetch_arm;
   }
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL CPU_ARM9::Save(LStream *pFile)
{
	if(pFile == NULL || !pFile->IsOpen())
   	return FALSE;
   pFile->Write(&opcode,sizeof(u32));
   pFile->Write(&z_flag,sizeof(u8));
   pFile->Write(&n_flag,sizeof(u8));
   pFile->Write(&c_flag,sizeof(u8));
   pFile->Write(&v_flag,sizeof(u8));
   pFile->Write(&q_flag,sizeof(u8));
  	pFile->Write(reg,sizeof(reg));
   pFile->Write(gp_reg,sizeof(gp_reg));
   pFile->Write(&cpsr,sizeof(u32));
   pFile->Write(&old_pc,sizeof(u32));
   pFile->Write(&stop,sizeof(u8));
   pFile->Write(itcm_mem,32768);
   pFile->Write(dtcm_mem,16384);
   pFile->Write(&nIRQ9,sizeof(nIRQ9));
	return TRUE;
}
//---------------------------------------------------------------------------
static void ioIF(u32 adr,u32 data,u8)
{
   nIRQ9 &= ~data;
   *((u32 *)(io_mem + 0x214)) = nIRQ9;
}
//---------------------------------------------------------------------------
void CPU_ARM9::InitTable(LPIFUNC *p,LPIFUNC *p1)
{
   int i;

   for(i=0x214;i<0x218;i++)
       p[i] = ioIF;
}
//---------------------------------------------------------------------------
u32 CPU_ARM9::get_CurrentPC()
{
   u32 d;

   d = old_pc;
   if((cpsr & T_BIT) != 0){
       if((gp_reg[15] - old_pc) > 4)
           d = gp_reg[15] - 2;
       d &= ~1;
   }
   else{
       if((gp_reg[15] - old_pc) > 8)
           d = gp_reg[15] - 4;
   }
   return d;
}
//---------------------------------------------------------------------------
u32 CPU_ARM9::get_Stopped()
{
   if(stop)
       stopped += ds.get_CurrentCycles() - fromStopped;
   return stopped;
}
//---------------------------------------------------------------------------
void CPU_ARM9::set_Stopped()
{
   fromStopped = stopped = 0;
}
//---------------------------------------------------------------------------
int CPU_ARM9::enter_dmaloop()
{
   if(exec9 == exec_dma_loop)
       return 1;
   exec9_i = exec9;
   exec9 = exec_dma_loop;
   return 0;
}














