#include "arm7.h"
#include "dsmem.h"
#include "util.h"
#include "cpu_0.h"
#include "syscnt.h"
#include "bios.h"
#include "io.h"
#include "gbabackup.h"
#include <math.h>

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
#define WRITEWORD(a,b) write_word_data(a,b)
#define WRITEHWORD(a,b) write_hword_data(a,b)
#define WRITEBYTE(a,b) write_byte_data(a,b)

#define CalcAddc(a,b) a + b + c_flag
#define CalcSubc(a,b) a - b - (c_flag ^ 1)

//---------------------------------------------------------------------------
u16 (*exec7)(void);
void (*pfn_arm7irq)(void);
u32 nIRQ7;
CPU_ARM7 arm7;
//---------------------------------------------------------------------------
static void (*fetch_func)(void);
static u32 gp_reg[17],opcode,cpsr,old_pc,reg[6][17],stopped,fromStopped;
static u8 n_flag,z_flag,c_flag,v_flag,stop,cyclesP;
static union{
   u32 cycles;
   struct{
       u16 cyclesS;
       u16 cyclesN;
   } dummy;
} wait_state;
static LVector<unsigned long>callStack;
static ARM_INS arm_ins[0x1000];
static ARM_INS thumb_ins[0x400];
static DECODEARM *dbgarm_ins;
static DECODEARM *dbgthumb_ins;
static char **arm_opcode_strings;
static char **thumb_opcode_strings;
static MEMORYINFO MemoryAddress[] = {
   {"BIOS ROM",0x00000000,0x4000,AMM_ALL,0,AMM_ALL,(DWORD)-1,NULL,NULL},
   {"EXTERN WRAM",0x02000000,0x400000,AMM_ALL,0,AMM_ALL,(DWORD)-1,NULL,NULL},
   {"INTERN WRAM",0x03000000,0x1000000,AMM_ALL,0,AMM_ALL,(DWORD)-1,NULL,NULL},
   {"I/O",0x04000000,0x2000,AMM_ALL,0,0,(DWORD)-1,NULL,NULL},
   {"I/O DSI",0x04004000,0x400,AMM_ALL,0,0,(DWORD)-1,NULL,NULL},
   {"I/O WIFI",0x04800000,0x8000,AMM_HWORD,0,0,(DWORD)-1,NULL,NULL},
   {"VIDEO",0x06000000,0x20000,AMM_ALL,0,AMM_ALL,(DWORD)-1,NULL,NULL},
   {"GBA ROM",0x08000000,0x01000000,AMM_ALL,0,AMM_ALL,(DWORD)-1,NULL,NULL},
   {"SRAM",0x0A000000,0x10000,AMM_BYTE,0,0,-1,NULL,NULL},
   {"EEPROM",0x0C000000,0x10000,AMM_HWORD,0,AMM_HWORD,-1,NULL,NULL},
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
		"setsb %1\n"
		"setzb %2\n"
		"setcb %3\n"
		"setob %4\n"
        "movl %%eax,%0"
		: "=m" (ret) : "m" (n_flag),"m" (z_flag),"m" (c_flag),"m" (v_flag)
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
		"setsb %1\n"
		"setzb %2\n"
		"setncb %3\n"
		"setob %4\n"
       "movl %%eax,%0"
		: "=m" (ret) : "m" (n_flag), "m" (z_flag), "m" (c_flag), "m" (v_flag)
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
		: "=m" (ret) : "m" (c_flag),"m" (n_flag), "m" (z_flag), "m" (v_flag)
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
       : : "m" (n_flag), "m"(z_flag)
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
		: "=m" (ret) : "m"(opcode),"m" (c_flag)
	);
	return ret;
#endif
}
//---------------------------------------------------------------------------
NO_IN_LINE static void fetch_arm()
{
fetch_arm_00:
	opcode = old_pc;
/*   if((gp_reg[15] >> 24) == 6){
   	EnterDebugMode(TRUE);
   }*/
   old_pc = gp_reg[15]-4;
   gp_reg[15] += 4;
   cyclesP = (u8)(waitState7[old_pc >> 24] >> 16);
	switch(old_pc >> 24){
		default:
       	opcode = (u32)*((u32 *)(bios7_mem + (old_pc & 0x3FFC)));
       break;
		case 2:
       	if(ds.get_EmuMode() == 0)
               opcode = (u32)*((u32 *)(ext_mem + (old_pc & (ulMMMask & ~3))));
           else
           	opcode = (u32)*((u32 *)(ext_mem + (old_pc & 0x3FFFC)));
       break;
       case 3:
       	if((old_pc & 0x800000) && ds.get_EmuMode() == 0)
			    opcode = (u32)*((u32 *)(int_mem2 + (old_pc & 0xFFFC)));
           else
				opcode = (u32)*((u32 *)(int_mem + (old_pc & 0x7FFC)));
       break;
		case 6:
       	if((io_mem[0x242] & 0x87) == 0x82 || (io_mem[0x243] & 0x87) == 0x82)
           	opcode = (u32)*((u32 *)(video_mem + ((VRAMmap[560+((old_pc>>14)&0x3FF)]<<14)+(old_pc&0x3FFC))));
       	else{
           	gp_reg[15] = opcode + 8;
               goto fetch_arm_00;
			}
       break;
		case 8:
       case 9:
           if(rom_pack[(old_pc>>16)&0x1FF] == NULL)
               opcode = (u32)0x0;
           else
       		opcode = (*((u32 *)(rom_pack[(old_pc>>16)&0x1FF] + (old_pc & 0xFFFC))));
       break;
   }
}
//---------------------------------------------------------------------------
NO_IN_LINE static void fetch_thumb()
{
fetch_thumb_00:
	opcode = old_pc;
   old_pc = gp_reg[15]-2;
   gp_reg[15] += 2;
	switch(old_pc >> 24){
   	default:
			opcode = (u32)*((u16 *)(bios7_mem + (old_pc & 0x3FFE)));
       break;
       case 2:
       	if(ds.get_EmuMode() == 0)
              opcode = (u32)*((u16 *)(ext_mem + (old_pc & (ulMMMask & ~1))));
           else
           	opcode = (u32)*((u16 *)(ext_mem + (old_pc & 0x3FFFE)));
           break;
       case 3:
       	if((old_pc & 0x800000) && ds.get_EmuMode() == 0)
			    opcode = (u32)*((u16 *)(int_mem2 + (old_pc & 0xFFFE)));
           else
				opcode = (u32)*((u16 *)(int_mem + (old_pc & 0x7FFE)));
       break;
		case 6:
       	if((io_mem[0x242] & 0x87) == 0x82 || (io_mem[0x243] & 0x87) == 0x82)
           	opcode = (u32)*((u16 *)(video_mem + ((VRAMmap[560+((old_pc>>14)&0x3FF)]<<14)+(old_pc&0x3FFE))));
       	else{
           	gp_reg[15] = opcode + 4;
              	goto fetch_thumb_00;
   		}
       break;
		case 8:
       case 9:
           if(rom_pack[(old_pc>>16)&0x1FF] == NULL)
               opcode = (u32)0;
           else
       		opcode = (u32)*((u16 *)(rom_pack[(old_pc>>16)&0x1FF] + (old_pc & 0xFFFE)));
       break;
   }
   cyclesP = (u8)(waitState7[old_pc >> 24]>>16);
}
//---------------------------------------------------------------------------
static u16 exec_thumb(void)
{
   u8 res;

	if(stop)
       return (u16)((waitState7[256] >> 16) * 4);
   res = thumb_ins[opcode >> 6]();
   fetch_func();
   return (u16)res;
}
//---------------------------------------------------------------------------
static u16 exec_arm(void)
{
   u8 run;               

   if(stop)
       return (u16)((waitState7[256] >> 16) * 4);
#ifdef _LINO
   if((opcode & 0xFFF000F0) == 0xE1200070)
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
   return (u16)run;
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
}
//---------------------------------------------------------------------------
NO_IN_LINE static void switchmode(u8 mode,u8 to)
{
	u32 *p,ss;
   int i;

   p = gp_reg;
   if(!to)
   	mode = (u8)((ss = gp_reg[16]) & 0x1F);
   if((cpsr & 0x1F) == (mode & 0x1F))
   	goto switchmode_1;
   i = 13;
   switch(cpsr & 0x1f){
   	case USER_MODE:
       case SYSTEM_MODE:
           memcpy(reg[0],gp_reg,sizeof(gp_reg));
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
static int reload_base(BOOL bSwitch)
{
   u8 i;

   if(bSwitch && (i = (u8)(cpsr & 0x1F)) != SYSTEM_MODE && i != USER_MODE)
        switchmode(0,0);
   if((cpsr & T_BIT)){
        if(exec7 != exec_thumb){
           gp_reg[15] &= ~1;
           gp_reg[15] += 2;
           exec7 = exec_thumb;
           fetch_func = fetch_thumb;
           return TRUE;
       }
   }
   else{
       if(exec7 != exec_arm){
           gp_reg[15] += 4;
           exec7 = exec_arm;
           fetch_func = fetch_arm;
           return TRUE;
       }
   }
   return FALSE;
}

//---------------------------------------------------------------------------
void arm7irq(void)
{
	u8 tb;

   if(stop)
       stopped += ds.get_CurrentCycles() - fromStopped;
	stop = 0;
   if((cpsr & IRQ_BIT))
   	return;
#ifdef _DEBPRO
	if(ds.get_EmuMode() == 0)
		debugDlg.OnEnterIRQ(*((u32 *)&io_mem7[0x210]) & *((u32 *)&io_mem7[0x214]),7);
   else
		debugDlg.OnEnterIRQ(*((u16 *)&io_mem7[0x200]) & *((u16 *)&io_mem7[0x202]),7);
#endif
   tb = (u8)((cpsr & T_BIT)>>T_SHIFT);
   switchmode(IRQ_MODE,1);
   cpsr &= ~T_BIT;
   cpsr |= IRQ_BIT;
   exec7 = exec_arm;
   if(!tb)
   	gp_reg[14] = gp_reg[15]-4;
   else
   	gp_reg[14] = gp_reg[15];
	gp_reg[15] = 0x1C;
	fetch_func = fetch_arm;
   fetch_func();
}
//---------------------------------------------------------------------------
NO_IN_LINE static void bios()
{
	u8 tb;

   tb = (u8)((cpsr & T_BIT)>>T_SHIFT);
	switchmode(SUPERVISOR_MODE,1);
   cpsr &= ~T_BIT;
   cpsr |= IRQ_BIT;
   exec7 = exec_arm;
   fetch_func = fetch_arm;
   if(!tb)
   	gp_reg[14] = gp_reg[15] - 4;
   else
   	gp_reg[14] = gp_reg[15] - 2;
   WriteMsgConsolle(&arm7,"%cARM7 : swi 0x%02X",MSGT_BIOS,(int)read_byte(gp_reg[14] - 2));
	gp_reg[15] = 0xC;
}
//---------------------------------------------------------------------------
NO_IN_LINE static void I_FASTCALL write_hword_data(u32 address,u32 data)
{
#if defined(_DEBPRO2)
   if(debugDlg.is_Active())
       ControlMemoryBP(address,MAKELONG(AMM_HWORD|AMM_WRITE,7),data);
#endif
   wait_state.cycles = waitState7[address>>24];
	switch((address >> 24)){
   	case 0:
           data = data;
       break;                                    
		case 2:
       	if(ds.get_EmuMode() == 0)
               *((u16 *)(ext_mem + (address & (ulMMMask & ~1)))) = (u16)data;
           else
				*((u16 *)(ext_mem + (address & 0x3FFFE))) = (u16)data;
       break;
		case 3:
       	if((address & 0x800000) && !ds.get_EmuMode()){
               *((u16 *)(int_mem2 + (address & 0xFFFE))) = (u16)data;
               return;
           }
       	*((u16 *)(int_mem + (address & 0x7FFE))) = (u16)data;
       break;
       case 4:
           if((address & 0x00100000))
           	address = 0x2000 + (address & 0xFE);
           else if((address & 0x00800000)){
  		    	*((u16 *)(io_mem7 + (0x3000 + (address & 0x7FFE)))) = (u16)data;
//               if(i_func7[0x3000 + (address & 0x7FFE)] != NULL)
           	    i_func7[0x3000 + (address & 0x7FFE)]((u32)(u16)address,data,AMM_HWORD);
               return;
           }
           else if((address & 0x00004000)){
#ifdef _LINO
               WriteMsgConsolle(&arm7,"%cARM7 write %08X",MSGT_OUTONCONSOLLE,address);
#endif
               address = 0x2800 + (address & 0x3FE);
           }
           else
               address &= 0x1FFE;
  		    *((u16 *)(io_mem7 + address)) = (u16)data;
//           if(i_func7[address] != NULL)
               i_func7[address](address,data,AMM_HWORD);
       break;
       case 5:
       	if(ds.get_EmuMode()){
               *((u16 *)(pal_mem + (address &= 0x7FE))) = (u16)data;
               pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_PALETTECHANGED,PIS_NOTIFYMASK,(LPARAM)(address << 16)|AMM_HWORD);
           }
       break;
       case 6:
       	if(ds.get_EmuMode() == 0){
               if((io_mem[0x242] & 0x87) == 0x82 || (io_mem[0x243] & 0x87) == 0x82)
                   *((u16 *)(video_mem + ((VRAMmap[560+((address>>14)&0x3FF)]<<14)+(address&0x3FFE)))) = (u16)data;
           }
           else{
               *((u16 *)(video_mem + ((VRAMmap[(address>>14)&0x3FF]<<14)+(address & 0x3FFE)))) = (u16)data;
           }
       break;
       case 7:
       	if(ds.get_EmuMode()){
                *((u16 *)(obj_mem + (address & 0x7FE))) = (u16)data;
                pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_OAMCHANGED,PIS_NOTIFYMASK,(LPARAM)(address<<16)|AMM_HWORD);
			}
       case 8:
       case 9:
           write_ram_block_hword(address,data);
       break;
       case 0xA:
       case 0xB:
       	if(ds.get_EmuMode() == EMUM_GBA)
				WriteRomPack(0,address,data,AMM_HWORD);
       break;
       case 0xC:
		case 0xD:
       	if(ds.get_EmuMode() == EMUM_GBA)
       		WriteRomPack(1,address,data,AMM_HWORD);
       break;
       default:
       	data = data;
		break;
   }
}
//---------------------------------------------------------------------------
NO_IN_LINE static void I_FASTCALL write_byte_data(u32 address,u32 data)
{
#if defined(_DEBPRO2)
   if(debugDlg.is_Active())
       ControlMemoryBP(address,MAKELONG(AMM_BYTE|AMM_WRITE,7),data);
#endif
   wait_state.cycles = waitState7[address>>24];
	switch((address >> 24)){
   	case 0:
           data = data;
       break;
       case 2:
			if(ds.get_EmuMode() == 0)
               ext_mem[address & ulMMMask] = (u8)data;
		    else
				ext_mem[address & 0x3FFFF] = (u8)data;
       break;
       case 3:
       	if((address & 0x800000) && !ds.get_EmuMode()){
           	int_mem2[(u16)address] = (u8)data;
               return;
           }
       	int_mem[address & 0x7FFF] = (u8)data;
       break;
       case 4:
           if((address & 0x00100000))
           	address = 0x2000 + (u8)address;
           else if((address & 0x00800000))
           	return;
           else if((address & 0x00004000)){
#ifdef _LINO
               WriteMsgConsolle(&arm7,"%cARM7 write %08X",MSGT_OUTONCONSOLLE,address);
#endif
               address = 0x2800 + (address & 0x3FF);
           }
           else
               address &= 0x1FFF;
           io_mem7[address] = (u8)data;
//           if(i_func7[address] != NULL)
               i_func7[address](address,data,AMM_BYTE);
       break;
		case 5:
       	if(ds.get_EmuMode()){
           	pal_mem[address &= 0x7FF] = (u8)data;
               pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_PALETTECHANGED,PIS_NOTIFYMASK,(LPARAM)(address<<16)|AMM_BYTE);
			}
       break;
       case 6:
       	if(ds.get_EmuMode() == 0){
               if((io_mem[0x242] & 0x87) == 0x82 || (io_mem[0x243] & 0x87) == 0x82)
                   *(video_mem + ((VRAMmap[560+((address>>14)&0x3FF)]<<14)+(address&0x3FFF))) = (u8)data;
           }
           else{
           	*(video_mem + ((VRAMmap[(address>>14)&0x3FF]<<14)+(address&0x3FFF))) = (u8)data;
           }
       break;
       case 7:
       	if(ds.get_EmuMode()){
               obj_mem[address & 0x7FF] = (u8)data;
               pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_OAMCHANGED,PIS_NOTIFYMASK,(LPARAM)(address<<16)|AMM_BYTE);
           }
       case 8:
       case 9:
           write_ram_block_byte(address,data);
       break;
       case 0xA:
       	if(ds.get_EmuMode() != EMUM_GBA)
           	*(sram_mem + (u16)address) = (u8)data;
       break;
       case 0xE:
       	if(ds.get_EmuMode() == EMUM_GBA)
           	WriteSRAM(address,(u8)data);
       break;
       default:
       	data = data;
		break;
   }
}
//---------------------------------------------------------------------------
NO_IN_LINE static void I_FASTCALL write_word_data(u32 address,u32 data)
{
#if defined(_DEBPRO2)
   if(debugDlg.is_Active())
       ControlMemoryBP(address,MAKELONG(AMM_WORD|AMM_WRITE,7),data);
#endif
   wait_state.cycles = waitState7[address>>24];
	switch((address >> 24)){
   	case 0:
           data = data;
       break;
		case 2:
       	if(ds.get_EmuMode() == 0){
               *((u32 *)(ext_mem + (address & (ulMMMask & ~3)))) = data;
               if(address == 0x027FFE24)
                   ds.StartArm9(data);
           }
           else
           	*((u32 *)(ext_mem + (address & 0x3FFFC))) = data;
       break;
		case 3:
       	if((address & 0x800000) && !ds.get_EmuMode()){
           	*((u32 *)(int_mem2 + (address & 0xFFFC))) = data;
               return;
           }
       	*((u32 *)(int_mem + (address & 0x7FFC))) = data;
       break;
       case 4:
           if((address & 0x00100000))
           	address = 0x2000 + (address & 0xFC);
           else if((address & 0x00800000)){
           	*((u32 *)(io_mem7 + (0x3000 + (address & 0x7FFC)))) = data;
//               if(i_func7[(0x3000 + (address & 0x7FFC))] != NULL)
           	    i_func7[(0x3000 + (address & 0x7FFC))]((u32)(u16)address,data,AMM_WORD);
               return;
           }
           else if((address & 0x00004000)){
#ifdef _LINO
               WriteMsgConsolle(&arm7,"%cARM7 write %08X",MSGT_OUTONCONSOLLE,address);
#endif
               address = 0x2800 + (address & 0x3FC);
           }
           else
               address &= 0x1FFC;
           *((u32 *)(io_mem7 + address)) = data;
//           if(i_func7[address] != NULL)
               i_func7[address](address,data,AMM_WORD);
       break;
       case 5:
       	if(ds.get_EmuMode()){
               *((u32 *)(pal_mem + (address &= 0x7FC))) = data;
               pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_PALETTECHANGED,PIS_NOTIFYMASK,(LPARAM)(address<<16)|AMM_WORD);
           }
		break;
       case 6:
       	if(ds.get_EmuMode() == 0){
               if((io_mem[0x242] & 0x87) == 0x82 || (io_mem[0x243] & 0x87) == 0x82)
                   *((u32 *)(video_mem + ((VRAMmap[560+((address>>14)&0x3FF)]<<14)+(address&0x3FFC)))) = data;
           }
           else{
           	*((u32 *)(video_mem + ((VRAMmap[(address>>14) & 0x3FF] << 14) + (address & 0x3FFC)))) = data;
           }
       break;
       case 7:
       	if(ds.get_EmuMode()){
               *((u32 *)(obj_mem + (address & 0x7FC))) = data;
               pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_OAMCHANGED,PIS_NOTIFYMASK,(LPARAM)(address<<16)|AMM_WORD);
		    }
       case 8:
       case 9:
           write_ram_block_word(address,data);
       break;
		case 0xA:
       case 0xB:
       	if(ds.get_EmuMode() == EMUM_GBA)
				WriteRomPack(0,address,data,AMM_WORD);
       break;
       case 0xC:
		case 0xD:
       	if(ds.get_EmuMode() == EMUM_GBA)
       		WriteRomPack(1,address,data,AMM_WORD);
       break;
       default:
       	data = data;
		break;
   }
}
//---------------------------------------------------------------------------
NO_IN_LINE static u32 I_FASTCALL read_word_data(u32 address)
{
#if defined(_DEBPRO2)
   if(debugDlg.is_Active())
       ControlMemoryBP(address,MAKELONG(AMM_WORD|AMM_READ,7));
#endif
#ifdef _LINO
   if((address & 3) != 0 && (opcode & 0x0FF00000) != 0x05900000)
       address = address;
#endif
   wait_state.cycles = waitState7[address>>24];
	switch((address >> 24)){
   	default:
           if(ds.get_EmuMode() == EMUM_GBA && (gp_reg[15] & 0x0F000000))
               return (u32)0xFFFFFFFF;
       	return (u32)*((u32 *)(bios7_mem + (address & 0x3FFC)));
		case 2:
			if(ds.get_EmuMode() != EMUM_GBA)
               return (u32)*((u32 *)(ext_mem + (address & (ulMMMask & ~3))));
          	return (u32)*((u32 *)(ext_mem + (address & 0x3FFFC)));
       case 3:
       	if((address & 0x800000) && !ds.get_EmuMode())
			    return (u32)*((u32 *)(int_mem2 + (address & 0xFFFC)));
			return (u32)*((u32 *)(int_mem + (address & 0x7FFC)));
		case 4:
           if((address & 0x00100000))
           	address = 0x2000 + (address & 0xFC);
           else if((address & 0x00800000)){
           	if(o_func7[0x3000 + (address & 0x7FFC)] != NULL)
               	return o_func7[0x3000 + (address & 0x7FFC)]((u32)(u16)address,AMM_WORD);
           	return *(u32 *)(io_mem7 + (0x3000 + (address & 0x7FFC)));
           }
           else if((address & 0x00004000)){
#ifdef _LINO
               WriteMsgConsolle(&arm7,"%cARM7 read %08X",MSGT_OUTONCONSOLLE,address);
#endif
               address = 0x2800 + (address & 0x3FC);
           }
           else
               address &= 0x1FFC;
           if(o_func7[address] != NULL)
               return o_func7[address](address,AMM_WORD);
           return *(u32 *)(io_mem7 + address);
       case 5:
           if(ds.get_EmuMode() == EMUM_GBA)
			    return (u32)*((u32 *)(pal_mem + (address&0x3FF)));
           return 0;
		case 6:
           if(ds.get_EmuMode() != EMUM_GBA){
               if((io_mem[0x242] & 0x87) == 0x82 || (io_mem[0x243] & 0x87) == 0x82)
                   return (u32)*((u32 *)(video_mem + ((VRAMmap[560+((address>>14)&0x3FF)]<<14)+(address&0x3FFC))));
           }
			else
           	return (u32)*((u32 *)(video_mem + ((VRAMmap[((address>>14)&0x3FF)]<<14)+(address&0x3FFC))));
		case 8:
       case 9:
       	if(ulExRamPack == 3){
               if(address >= 0x08000004 && address < 0x080000A0)
                   return *((u32 *)&bios9_mem[(address + 0x1C) & 0xFFFC]);
           }
           if(rom_pack[(address>>16)&0x1FF] == NULL)
               return (u32)0x0;
       	return (*((u32 *)(rom_pack[(address>>16)&0x1FF] + (address & 0xFFFC))));
		case 0xA:
       case 0xB:
       	if(ds.get_EmuMode() == EMUM_GBA)
           	return ReadRomPack(0,address,AMM_WORD);
           return 0;
       break;
       case 0xC:
       case 0xD:
       	if(ds.get_EmuMode() == EMUM_GBA)
           	return ReadRomPack(1,address,AMM_WORD);
           return 0;
       break;
   }
}
//---------------------------------------------------------------------------
NO_IN_LINE static u32 I_FASTCALL read_hword_data(u32 address)
{
#if defined(_DEBPRO)
   if(debugDlg.is_Active())
       ControlMemoryBP(address,MAKELONG(AMM_HWORD|AMM_READ,7));
#endif
   wait_state.cycles = waitState7[address>>24];
	switch((address >> 24)){
   	default:
           if(ds.get_EmuMode() == EMUM_GBA && (gp_reg[15] & 0x0F000000))
               return (u32)0xFFFF;
	        return (u16)*((u16 *)(bios7_mem + (address & 0x3FFE)));
       case 2:
       	if(ds.get_EmuMode() != EMUM_GBA)
               return (u32)*((u16 *)(ext_mem + (address & (ulMMMask & ~1))));
           return (u32)*((u16 *)(ext_mem + (address & 0x3FFFE)));
       case 3:
       	if((address & 0x800000) && !ds.get_EmuMode())
			    return (u32)*((u16 *)(int_mem2 + (address & 0xFFFE)));
			return (u32)*((u16 *)(int_mem + (address & 0x7FFE)));
       case 4:
           if((address & 0x00100000))
           	address = 0x2000 + (address & 0xFE);
           else if((address & 0x00800000)){
           	if(o_func7[0x3000 + (address & 0x7FFE)] != NULL)
               	return (u32)(u16)o_func7[0x3000 + (address & 0x7FFE)]((u32)(u16)address,AMM_HWORD);
           	return (u32)*(u16 *)(io_mem7 + (0x3000 + (address & 0x7FFE)));
           }
           else if((address & 0x00004000)){
#ifdef _LINO
               WriteMsgConsolle(&arm7,"%cARM7 read %08X",MSGT_OUTONCONSOLLE,address);
#endif
               address = 0x2800 + (address & 0x3FE);
           }
           else
               address &= 0x1FFE;
           if(o_func7[address] != NULL)
               return (u32)(u16)o_func7[address](address,AMM_HWORD);
           return (u32)*(u16 *)(io_mem7 + address);
       case 5:
           if(ds.get_EmuMode() == EMUM_GBA)
			    return (u32)*((u16 *)(pal_mem + (address&0x3FF)));
           return 0;
		case 6:
			if(ds.get_EmuMode() != EMUM_GBA){
               if((io_mem[0x242] & 0x87) == 0x82 || (io_mem[0x243] & 0x87) == 0x82)
                   return (u32)*((u16 *)(video_mem + ((VRAMmap[560+((address>>14)&0x3FF)]<<14)+(address&0x3FFE))));
           }
           else
				return (u32)*((u16 *)(video_mem + ((VRAMmap[(address>>14)&0x3FF]<<14)+(address&0x3FFF))));
		case 8:
       case 9:
       	if(ulExRamPack == 3){
               if(address >= 0x08000004 && address < 0x080000A0)
                   return *((u16 *)&bios9_mem[(address + 0x1C) & 0xFFFE]);
           }
           if(rom_pack[(address>>16)&0x1FF] == NULL)
               return (u32)0;
       	return (u32)*((u16 *)(rom_pack[(address>>16)&0x1FF] + (address & 0xFFFE)));
		case 0xA:
       case 0xB:
       	if(ds.get_EmuMode() == EMUM_GBA)
           	return ReadRomPack(0,address,AMM_HWORD);
           return 0;
       break;
       case 0xC:
       case 0xD:
       	if(ds.get_EmuMode() == EMUM_GBA)
           	return ReadRomPack(1,address,AMM_HWORD);
           return 0;
       break;
   }
}
//---------------------------------------------------------------------------
NO_IN_LINE static u32 I_FASTCALL read_byte_data(u32 address)
{
#if defined(_DEBPRO2)
   if(debugDlg.is_Active())
       ControlMemoryBP(address,MAKELONG(AMM_BYTE|AMM_READ,7));
#endif
   wait_state.cycles = waitState7[address>>24];
	switch(address >> 24){
   	default:
           if(ds.get_EmuMode() == EMUM_GBA && (gp_reg[15] & 0x0F000000))
               return 0xFF;
           return bios7_mem[address & 0x3fff];     //80474be
       case 2:
       	if(ds.get_EmuMode() == 0)
               return (u32)ext_mem[address & ulMMMask];
			return (u32)ext_mem[address & 0x3FFFF];
       case 3:
       	if((address & 0x800000) && !ds.get_EmuMode())
           	return (u32)int_mem2[(u16)address];
       	return (u32)int_mem[address & 0x7FFF];
       case 4:
           if((address & 0x00100000))
           	address = 0x2000 + (u8)address;
           else if((address & 0x00800000))
           	address = 0x3000 + (address & 0x7FFF);
           else if((address & 0x00004000)){
#ifdef _LINO
               WriteMsgConsolle(&arm7,"%cARM7 read %08X",MSGT_OUTONCONSOLLE,address);
#endif
               address = 0x2800 + (address & 0x3FF);
           }
           else
               address &= 0x1FFF;
           if(o_func7[address] != NULL)
               return o_func7[address](address,AMM_BYTE);
           return (u32)io_mem7[address];
		case 6:
           if(ds.get_EmuMode() != EMUM_GBA){
               if((io_mem[0x242] & 0x87) == 0x82 || (io_mem[0x243] & 0x87) == 0x82)
                   return (u32)*((u8 *)(video_mem + ((VRAMmap[560+((address>>14)&0x3FF)]<<14)+(address&0x3FFF))));
           }
           else
               return (u32)*((u16 *)(video_mem + ((VRAMmap[(address>>14)&0x3FF]<<14)+(address&0x3FFF))));
		case 8:
       case 9:
       	if(ulExRamPack == 3){
               if(address >= 0x08000004 && address < 0x080000A0)
                   return bios9_mem[(u16)(address + 0x1C)];
           }
           if(rom_pack[(address>>16)&0x1FF] == NULL)
               return (u32)0;
       	return (u32)(rom_pack[(address>>16)&0x1FF][(u16)address]);
       case 0xA:
       	if(ds.get_EmuMode() != EMUM_GBA)
           	return sram_mem[(u16)address];
           return ReadRomPack(0,address,AMM_BYTE);
       break;
       case 0xB:
       	if(ds.get_EmuMode() == EMUM_GBA)
           	return ReadRomPack(0,address,AMM_BYTE);
           return 0;
       break;
       case 0xC:
       case 0xD:
       	if(ds.get_EmuMode() == EMUM_GBA)
           	return ReadRomPack(1,address,AMM_BYTE);             
           return 0;
       break;
       case 0xE:
       	if(ds.get_EmuMode() == EMUM_GBA)
       		return ReadSRAM(address);
           return 0;
       break;
   }
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 unknown_ins(void)
{
#ifdef _DEBPRO                                    
	EnterDebugMode(TRUE);
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
   return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 mcr(void)
{
   return cycP;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 mrc(void)
{
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
               else
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
           return ((OP_REG << (32-IMM_SHIFT))|(OP_REG>>IMM_SHIFT));
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
           else if(shift < 32)
               return res << shift;
           return 0;
       case 1: // LSR
           if((shift = (u8)SHFT_AMO_REG) == 0)
               return res;
           else if(shift < 32)
               return res >> shift;
           return 0;
       case 2://asr
           if((shift = (u8)SHFT_AMO_REG) == 0)
               return res;
           else if(shift < 32)
               return (signed long)res >> shift;
           else{
               if((res & 0x80000000))
                   return 0xFFFFFFFF;
               else
                   return 0;
           }
       case 3://ROR
#if defined(__BORLANDC__)
           if((shift = (u8)SHFT_AMO_REG) == 0)
               return res;
           else if(shift < 32){
               __asm{
               	mov eax,opcode
                   mov cl,shift
                   and eax,15
               	mov eax,dword ptr[gp_reg+4*eax]
                   ror eax,cl
               }
               return _EAX;
           }
           else if(shift == 32)
               return res;
           else{
               shift &= 0x1F;
               _EAX = res;
               _CL = shift;
               __asm ror eax,cl
               return _EAX;
           }
#elif defined(__GNUC__) || defined(__WATCOMC__)
           if(!(shift = (u8)SHFT_AMO_REG))
               return res;
           else if(shift < 32)
               return ((res << (32-shift))|(res>>shift));
           else if(shift == 32)
               return res;
           else{
               shift &= 0x1F;
               return ((res << (32-shift))|(res>>shift));
           }
#endif
   }
   return 0;
}
//---------------------------------------------------------------
NO_IN_LINE static u32 I_FASTCALL DP_REG_OPERAND_UPD(u8 shift)
{
   register u32 temp_reg;

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
               _CL = _AL & 31;
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
   if((opcode >> 28) == 0xF){
       gp_reg[14] = gp_reg[15] - 4;
#if defined(_DEBPRO)
       del_CallStack(gp_reg[14]);
#endif
       gp_reg[15] += (adr << 2) + (opcode & 0x1000000 ? 4 : 2);
       cpsr |= T_BIT;
       exec7 = exec_thumb;
       fetch_func = fetch_thumb;
   }
   else
       gp_reg[15] += (adr << 2) + 4;
   return cycP;
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
/*   if((opcode >> 28) == 0xF){
       gp_reg[15] += (adr << 2) + (opcode & 0x1000000 ? 4 : 2);
       cpsr |= T_BIT;
       exec7 = exec_thumb;
       fetch_func = fetch_thumb;
   }
   else*/
       gp_reg[15] += (adr << 2) + 4;
   return cycP;
}
//--------------------------------------------------------------------------------------
NO_IN_LINE static u8 ins_blx(void)
{
   u32 value;

   gp_reg[14] = gp_reg[15] - 4;
#if defined(_DEBPRO)
   del_CallStack(gp_reg[14]);
#endif
   gp_reg[15] = (value = gp_reg[opcode & 0xF]) & ~0x1;
   if(value & 1) {
       cpsr |= T_BIT;
       exec7 = exec_thumb;
       gp_reg[15] += 2;
       fetch_func = fetch_thumb;
   }
   else
       gp_reg[15] += 4;
   return cycP;
}
//--------------------------------------------------------------------------------------
NO_IN_LINE static u8 ins_bx(void)
{
   u32 value;

   gp_reg[15] = (value = OP_REG) & ~0x1;
#if defined(_DEBPRO)
   del_CallStack(gp_reg[15]);
#endif
   if(value & 1){
       cpsr |= T_BIT;
       exec7 = exec_thumb;
       gp_reg[15] += 2;
       fetch_func = fetch_thumb;
   }
   else
        gp_reg[15] += 4;
   return cycP;
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

   BASE_REG = (u32)OP_REG * (value = (u32)SHFT_AMO_REG) + DEST_REG;
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
   return (u8)(cycP + m + 1);
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
   return (u8)(cycP + m + 1);
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
           if(value == 0)
               m = 3;
           else
               m = 4;
       }
   }
   return (u8)(cycP + m);
}
//---------------------------------------------------------------
NO_IN_LINE static u8 ins_mlal(void)
{
   s32 op1;
   u32 d_reg,b_reg;
   u32 value;
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
   return (u8)(cycP + m + 1);
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
   return (u8)(cycP + 1 + m);
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
   return (u8)(cycP + 1 + m);
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
           m = (u8)(value == 0 ? 3 : 4);
       }
   }
   return (u8)(cycP + 1 + m);
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
   if((opcode & 0x2000000))
       value = DP_IMM_OPERAND();
   else
       value = OP_REG;
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

   if((opcode & 0x2000000))
       value = DP_IMM_OPERAND();
   else
       value = OP_REG;
   field = (u8)((opcode >> 16) & 0xF);
   if((field & 8))
       gp_reg[16] = (gp_reg[16] & ~0xFF000000) | (0xFF000000 & value);
   if((field & 4))
       gp_reg[16] = (gp_reg[16] & ~0x00FF0000) | (0x00FF0000 & value);
   if((field & 2))
       gp_reg[16] = (gp_reg[16] & ~0x0000FF00) | (0x0000FF00 & value);
   if((field & 1))
       gp_reg[16] = (gp_reg[16] & ~0x000000FF) | ((u8)value);
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ins_swi (void)
{
   int i;
   char *p;

   switch(opcode & 0x000FFFFF){
   	case 0xFD000:
			BiosBreakPoint(TRUE);
       break;
       case 0xFC000:
			WriteConsoleMessage(gp_reg[0],&arm7);
		break;
       case 0xEFEFE:
           switch((u8)gp_reg[12]){
           	case 1:
					if(ds.get_EmuMode() == EMUM_GBA)
                   	RegisterRamReset(&arm7);
               break;
               case 14:
					if(ds.get_EmuMode() == EMUM_NDS){
                       reg[0][2] &= ~1;
                       reg[0][1] &= ~1;
                       
                       p = (char *)LocalAlloc(LPTR,reg[0][2]);
                       for(i=0;i<reg[0][2];i++)
                           p[i] = (char)read_byte(reg[0][1]+i);
                       reg[0][0] = CalcCrc16((u16 *)p,reg[0][2],(u16)reg[0][0]);
                       LocalFree(p);
                       //r0 crc
                       //r1 source
                       //r2 length
                       //r0 result
                   }
                   else
                   	BgAffineSet(&arm7);
               break;
               case 6:
               	if(ds.get_EmuMode() == EMUM_GBA){
                       if(reg[0][1] != 0){
                           i = (int)reg[0][0] / (int)reg[0][1];
                           reg[0][1] = (int)reg[0][0] % (int)reg[0][1];
                           reg[0][0] = i;
                           reg[0][3] = abs(i);
                       }
                   }
				break;
               case 7:
               	if(ds.get_EmuMode() == EMUM_GBA){
                       if(reg[0][0] != 0){
                           i = (int)reg[0][1] / (int)reg[0][0];
                           reg[0][1] = (int)reg[0][1] % (int)reg[0][0];
                           reg[0][0] = i;
                           reg[0][3] = abs(i);
                       }
                   }
               break;
               case 8:
               	if(ds.get_EmuMode() == EMUM_GBA)
                   	reg[0][0] = SquareRoot(reg[0][0]);
               break;
               case 9:
                   if(reg[0][1] != 0){
                       i = (int)reg[0][0] / (int)reg[0][1];
                       reg[0][1] = (int)reg[0][0] % (int)reg[0][1];
                       reg[0][0] = i;
                       reg[0][3] = abs(i);
                   }
               break;
               case 10:
                   if(ds.get_EmuMode() == EMUM_GBA){
                       float theta;
                       s16 x,y;
                       
                       y = (s16)reg[0][1];
                       x = (s16)reg[0][0];
                       if(x != 0)
                           theta = atan2(y,x);
                       else if(y != 0)
                           theta = atan(y);
                       else
                           theta = 0;
                       reg[0][0] = (u32)((s16)(theta * 65535.0 / 2.0 / M_PI));
                   }
               break;
               case 11:
                   CpuSet(reg[0][1],reg[0][0],reg[0][2],&arm7);
               break;
               case 12:
                   CpuFastSet(reg[0][1],reg[0][0],reg[0][2],&arm7);
               break;
               case 13:
               	if(ds.get_EmuMode() != EMUM_GBA)
               		reg[0][0] = SquareRoot(reg[0][0]);
               break;
               case 15:
                   ObjAffineSet(&arm7);
               break;
               case 17:
                   LZ77UnComp(reg[0][0],reg[0][1],&arm7);
               break;
               case 18:
               	if(ds.get_EmuMode() == EMUM_GBA)
						LZ77UnComp(reg[0][0],reg[0][1],&arm7);
               break;
               case 19:
                   HuffUnComp(reg[0][1],reg[0][0],&arm7);
               break;
               case 20:
               case 21:
                   RLUnComp(reg[0][1],reg[0][0],&arm7);
               break;
               case 0x1A:
                   reg[0][0] = GetSineTable(reg[0][0]);
               break;
               case 0x1B:
                   reg[0][0] = GetPitchTable(reg[0][0]);
               break;
               case 0x1C:
                   reg[0][0] = (u32)GetVolumeTable(reg[0][0]);
               break;
               case 0x1F:
               	if(ds.get_EmuMode() == EMUM_GBA)
                     RLUnComp(reg[0][1],reg[0][0],&arm7);
               break;
               default:
                   gp_reg[12] = gp_reg[12];
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
   return (u8)(cycS + cycS + cycN);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ins_swp (void)
{
   u32 temp;

   temp = READWORD(BASE_REG);
   WRITEWORD(BASE_REG, OP_REG);
   DEST_REG = temp;
   return (u8)(cycI + cycS + cycN + cycN);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ins_swpb(void)
{
   u8 temp;

	temp = (u8)READBYTE(BASE_REG);
   WRITEBYTE(BASE_REG,OP_REG);
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
   *regs += ((opcode&0xF00)>>4)|(opcode&0xF);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrsb_postdownimm(void)
{
   u32 *regs;

   DEST_REG = (u32)((s8)READBYTE(*(regs = &BASE_REG)));
   *regs -= ((opcode&0xF00)>>4)|(opcode&0xF);
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
       stack = (base = stack - (r << 2));
       if(!(opcode & 0x1000000))
           stack += 4;
   }
   if((opcode & 0x200000))
       *regs = base;
   stack &= ~3;
   p2 = reg1;
   for(i = r,r=0;i > 0;i--){
       WRITEWORD(stack,*p2++);
       stack += 4;
       r += (u8)cycS;
   }
   return (u8)((r - cycS) + cycN + cycPN);
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
       for(p1 = 1;p1 > 0;p1 <<= 1,p3++){
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
   i = (u8)(r-1);
   r = 0;
   *((u32 *)*p2) = READWORD(stack);
   stack += 4;
   if((opcode & 0x200000))
       *regs = base;
   for(p2++;i > 0;i--,p2++){
       *((u32 *)*p2) = READWORD(stack);
       stack += 4;
       r += (u8)cycS;
   }
#if defined(_DEBPRO)
   if((opcode & 0x8000))
	    del_CallStack(gp_reg[15]);
#endif
   if(opcode & 0x400000){
       if(opcode & 0x8000){
           if(!reload_base(TRUE))
       	    gp_reg[15] += 4;
       }
   }
   else{
       if(opcode & 0x8000){
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
   return (u8)(cycI + r + cycN);
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
   DEST_REG = (u32)READHWORD(base);
   *regs = base;
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
   DEST_REG = READHWORD(base);
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrh_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (((opcode&0xF00)>>4)|(opcode&0xF));
   DEST_REG = READHWORD(base);
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrh_postupimm(void)
{
   u32 *regs,value;

   regs = &BASE_REG;
	value = READHWORD(*regs);
   *regs += ((opcode&0xF00) >> 4)|(opcode & 0xF);
	DEST_REG = value;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrh_postdownimm(void)
{
   u32 *regs;

   DEST_REG = READHWORD(*(regs = &BASE_REG));
   *regs -= ((opcode&0xF00) >> 4)|(opcode & 0xF);
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
   WRITEHWORD(BASE_REG + (((opcode&0xF00)>>4)|(opcode&0xF)),(u16)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strh_predownimm(void)
{
   WRITEHWORD(BASE_REG - (((opcode&0xF00) >> 4)|(opcode& 0xF)),(u16)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strh_preupimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + (((opcode & 0xF00) >> 4)|(opcode & 0xF));
   WRITEHWORD(base,(u16)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 strh_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (((opcode & 0xF00)>>4)|(opcode & 0xF));
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
   return STRBASE_RET;//lino
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
   DEST_REG = READBYTE(BASE_REG + (opcode & 0xFFF));
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrb_predownimm(void)
{
   DEST_REG = READBYTE(BASE_REG - (opcode & 0xFFF));
   return LDR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrb_preupimmwb(void)
{
   u32 base,*regs;

   DEST_REG = READBYTE(base = *(regs = &BASE_REG) + (opcode & 0xFFF));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldrb_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (opcode & 0xFFF);
   DEST_REG = READBYTE(base);
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
//STR
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_postup(void)
{
   u32 *regs;
   u8 rd;

   regs = &BASE_REG;
   if((rd = (u8)DEST_REG_INDEX) != 15)
       WRITEWORD(*regs,gp_reg[rd]);
   else
       WRITEWORD(*regs,gp_reg[15] + 4);
   *regs += DP_REG_OPERAND_IMM();
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_postdown(void)
{
   u32 *regs;
   u8 rd;

   regs = &BASE_REG;
   if((rd = (u8)DEST_REG_INDEX) != 15)
       WRITEWORD(*regs,gp_reg[rd]);
   else
       WRITEWORD(*regs,gp_reg[15]+4);
   *regs -= DP_REG_OPERAND_IMM();
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_preup(void)
{
   u8 rd;

   if((rd = (u8)DEST_REG_INDEX) != 15)
       WRITEWORD(BASE_REG + DP_REG_OPERAND_IMM(),gp_reg[rd]);
   else
       WRITEWORD(BASE_REG + DP_REG_OPERAND_IMM(),gp_reg[15]+4);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_predown(void)
{
   u8 rd;

   if((rd = (u8)DEST_REG_INDEX) != 15)
       WRITEWORD(BASE_REG - DP_REG_OPERAND_IMM(),gp_reg[rd]);
   else
       WRITEWORD(BASE_REG - DP_REG_OPERAND_IMM(),gp_reg[15]+4);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_preupwb(void)
{
   u32 base,*regs;
   u8 rd;

   base = *(regs = &BASE_REG) + DP_REG_OPERAND_IMM();
   if((rd = (u8)DEST_REG_INDEX) != 15)
       WRITEWORD(base,gp_reg[rd]);
   else
       WRITEWORD(base,gp_reg[15]+4);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_predownwb(void)
{
   u32 base,*regs;
   u8 rd;

   base = *(regs = &BASE_REG) - DP_REG_OPERAND_IMM();
   if((rd = (u8)DEST_REG_INDEX) != 15)
       WRITEWORD(base,gp_reg[rd]);
   else
       WRITEWORD(base,gp_reg[15]+4);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_postupimm(void)
{
   u32 *regs;
   u8 rd;

   regs = &BASE_REG;
   if((rd = (u8)DEST_REG_INDEX) != 15)
       WRITEWORD(*regs,gp_reg[rd]);
   else
       WRITEWORD(*regs,gp_reg[15] + 4);
   *regs += opcode & 0xFFF;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_postdownimm(void)
{
   u32 *regs;
   u8 rd;

   regs = &BASE_REG;
   if((rd = (u8)DEST_REG_INDEX) == 15)
       WRITEWORD(*regs,gp_reg[15] + 4);
   else
       WRITEWORD(*regs,gp_reg[rd]);
   *regs -= opcode & 0xFFF;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_preupimm(void)
{
   u8 rd;

   if((rd = (u8)DEST_REG_INDEX) != 15)
       WRITEWORD(BASE_REG + (opcode & 0xFFF),gp_reg[rd]);
   else
       WRITEWORD(BASE_REG + (opcode & 0xFFF),gp_reg[15]+4);
   return STR_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 str_predownimm(void)
{
   u8 rd;

   if(((rd = (u8)DEST_REG_INDEX)) != 15)
       WRITEWORD(BASE_REG - (opcode & 0xFFF),gp_reg[rd]);
   else
       WRITEWORD(BASE_REG - (opcode & 0xFFF),gp_reg[15]+4);
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
NO_IN_LINE static u8 ldr_postup(void)
{
   u32 *regs;
   u8 rd;

   regs = &BASE_REG;
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(*regs);
   *regs += DP_REG_OPERAND_IMM();
   if(rd != 15)
       return LDRBASE_RET;
   gp_reg[15] += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_postdown(void)
{
   u32 *regs;
   u8 rd;

   regs = &BASE_REG;
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(*regs);
        *regs -= DP_REG_OPERAND_IMM();
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
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(base);
   *regs = base;
   if(rd != 15)
       return LDRBASE_RET;
   gp_reg[15] += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_predown(void)
{
   u8 rd;

   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(BASE_REG - DP_REG_OPERAND_IMM());
   if(rd != 15)
       return LDR_RET;
   gp_reg[15] += 4;
   return LDRPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_preup(void)
{
   u8 rd;

   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(BASE_REG + DP_REG_OPERAND_IMM());
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
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(base);
   *regs = base;
   if(rd != 15)
       return LDRBASE_RET;
   gp_reg[15] += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_postupimm(void)
{
   u32 *regs;
   u8 rd;

   regs = &BASE_REG;
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(*regs);
   *regs += opcode & 0xFFF;
   if(rd != 15)
       return LDRBASE_RET;
        if(gp_reg[15] & 1){
                cpsr |= T_BIT;
       exec7 = exec_thumb;
       gp_reg[15] = (gp_reg[15] & ~1) + 2;
                fetch_func = fetch_thumb;
   }
   else
        gp_reg[15] += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_postdownimm(void)
{
   u32 *regs;
   u8 rd;

   regs = &BASE_REG;
   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(*regs);
   *regs -= opcode & 0xFFF;
   if(rd != 15) return LDRBASE_RET;
   gp_reg[15] += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_preupimm(void)
{
   u8 rd;
   u32 base;

   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD((base = BASE_REG + (opcode & 0xFFF)));
   if(base & 3)
       gp_reg[rd] = (gp_reg[rd] << (32-(8*(base&3)))) | (gp_reg[rd] >> (8*(base&3)));
   if(rd != 15) return LDR_RET;
   gp_reg[15] += 4;
   return LDRPC_RET;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ldr_predownimm(void)
{
   u8 rd;

   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD(BASE_REG - (opcode & 0xFFF));
   if(rd != 15) return LDR_RET;
   if(gp_reg[15] & 1){
       cpsr |= T_BIT;
       exec7 = exec_thumb;
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

   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD((base = *(regs = &BASE_REG) + (opcode & 0xFFF)));
   *regs = base;
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

   gp_reg[(rd = (u8)DEST_REG_INDEX)] = READWORD((base = *(regs = &BASE_REG) - (opcode & 0xFFF)));
   *regs = base;
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 subs_imm(void)
{
   u8 pipe;

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = CalcSubFlags(BASE_REG,DP_IMM_OPERAND());
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
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
//MOV
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

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = DP_REG_OPERAND_UPD((u8)IMM_SHIFT)));
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
NO_IN_LINE static u8 movs_reg(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG)));
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
//MVN
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

   gp_reg[(pipe = (u8)(((u16)opcode>> 12)))] = ~DP_REG_OPERAND();
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
// AND
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

   gp_reg[(pipe = (u8)DEST_REG_INDEX)] = (BASE_REG & DP_IMM_OPERAND());
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
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
NO_IN_LINE static u8 ands_reg(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((gp_reg[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG)));
   if(pipe == 15){
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
// EOR
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
// ORR
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
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
       gp_reg[15] += 4;
   }
   return cycP;
}
/*---------------------------------------------------------------------------
thumb instruction
---------------------------------------------------------------------------*/
NO_IN_LINE static u8 tins_swi(void)
{
	switch((u8)opcode){
   	case 0xFD:
			BiosBreakPoint(TRUE);
       break;
       case 0xFC:
			WriteConsoleMessage(gp_reg[0],&arm7);
		break;
       default:
       	bios();
       break;
   }
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
       __asm{
           mov eax,value
           mov cl,s
           ror eax,cl
           setc byte ptr[c_flag]
           mov value,eax
       }
#elif defined(__GNUC__)
       __asm__ __volatile__ (
           "movl %0,%%eax\n"
           "movb %1,%%cl\n"
           "rorl %%cl,%%eax\n"
           "setcb %2\n"
           "mov %%eax,%0\n"
           : : "m" (value), "m" (s), "m" (c_flag)
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

   if((rd = (u8)(((opcode & 0x80) >> 4) | (opcode & 7))) == 15){
       gp_reg[15] += gp_reg[(opcode >> 3) & 0xF] + 2;
       return cycP;
   }
   gp_reg[rd] += gp_reg[(opcode >> 3) & 0xF];
   return cycP;
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
   return cycP;
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
   u8 n;
   u32 address,*regd,*reg;
   u16 p;

   address = *(regd = &(reg = gp_reg)[(opcode >> 8) & 7]) & ~3;
   for(p=1,n=0;p < 0x100;reg++,p <<= 1){
       if(!(opcode & p))
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
   gp_reg[opcode & 7] = READWORD(gp_reg[(opcode >> 3) & 7] + gp_reg[(opcode >> 6) & 7]);
   return LDR_RET;
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_ldr_imm(void)
{
   gp_reg[opcode & 7] = READWORD(gp_reg[(opcode >> 3) & 7] + (((opcode>>6)&0x1F)<<2));
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
   gp_reg[opcode & 7] = READHWORD(gp_reg[(opcode >> 3) & 7] + (((opcode>>6)&0x1F)<<1) & ~1);
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
   for(p=1;p < 0x100;p <<= 1,reg++){
       if(!(opcode & p))
           continue;
       WRITEWORD(address,*reg);
       address += 4;
       n += (u8)cycS;
   }
   *regd = address;
   return (u8)((n - cycS) + cycN + cycPN);
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
       __asm__ __volatile__(
           "setcb %0\n"
           "setzb %1\n"
           "setsb %2\n"
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
   __asm__ __volatile__(
       "setcb %0\n"
       "setzb %1\n"
       "setsb %2\n"
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
       __asm__ __volatile__(
           "setcb %0\n"
           "setzb %1\n"
           "setsb %2\n"
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
       __asm__ __volatile__(
           "setcb %0\n"
           "setzb %1\n"
           "setsb %2\n"
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
       __asm__ __volatile__(
           "setcb %0\n"
           "setzb %1\n"
           "setsb %2\n"
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
       __asm__ __volatile__(
           "setcb %0\n"
           "setzb %1\n"
           "setsb %2\n"
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
   for(p = 0x80;p > 0;p >>= 1,reg--){
       if(!(opcode & p))
           continue;
       adress -= 4;
       WRITEWORD(adress,*reg);
       n += (u8)cycS;
   }
   gp_reg[13] = adress;
   return (u8)((n - cycS) + cycN + cycPN);
}
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_pop(void)
{
   u8 n;
   u32 *reg,adress;
   u16 p;

   n = 0;
   adress = (reg = gp_reg)[13];
   for(p = 1;p < 0x100;p <<= 1,reg++) {
       if(!(opcode & p))
           continue;
       *reg = READWORD(adress);
       adress += 4;
       n += (u8)cycS;
   }
   if((opcode & 0x100)){
       gp_reg[15] = READWORD(adress);
#if defined(_DEBPRO)
       del_CallStack(gp_reg[15]);
#endif
       adress += 4;
       gp_reg[15] = (gp_reg[15] & ~1) + 2;
   }
   gp_reg[13] = adress;
   return (u8)(cycI + n + cycN + cycPN);
}
//---------------------------------------------------------------------------
// branch
//---------------------------------------------------------------------------
NO_IN_LINE static u8 tins_bx(void)
{
   if((opcode & 0x80))
       gp_reg[14] = (gp_reg[15] - 2) | 1;
   gp_reg[15] = gp_reg[(opcode>> 3) & 0xf];
#if defined(_DEBPRO)
   del_CallStack(gp_reg[15]);
#endif
   if(!(gp_reg[15] & 1)){
       gp_reg[15] = (gp_reg[15] & ~3) + 4;
       cpsr &= ~T_BIT;
       exec7 = exec_arm;
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
       del_CallStack(gp_reg[14] & ~1);
#endif
       gp_reg[15] = (u32)temp + ((opcode & 0x7FF) << 1);
       if(!(gp_reg[15] & 1)){
           gp_reg[15] = (gp_reg[15] & ~3) + 4;
           cpsr &= ~T_BIT;
           exec7 = exec_arm;
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
   gp_reg[15] += temp;
   if(h == 1){
       gp_reg[15] = (gp_reg[15] & ~3) + 4;
       cpsr &= ~T_BIT;
       exec7 = exec_arm;
       fetch_func = fetch_arm;
   }
   else
       gp_reg[15] += 2;
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

   switch(((opcode>>8) & 0xF)){
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
   return cycP;
}
//---------------------------------------------------------------------------
static void arm7irq_empty(void)
{
   if((((u32 *)(io_mem7 + 0x208))[0] & 1)){
       if(((u32 *)(io_mem7 + 0x210))[0] & nIRQ7)
           arm7irq();
   }
}
//---------------------------------------------------------------------------
BOOL CPU_ARM7::Init()
{
   int i, n;

   dbgarm_ins = (DECODEARM *)LocalAlloc(LPTR,0x1400 * sizeof(DECODEARM) +
        0x1400 * sizeof(char *));
   if(dbgarm_ins == NULL)
        return FALSE;
   dbgthumb_ins = (DECODEARM *)(dbgarm_ins + 0x1000);
        arm_opcode_strings = (char **)(dbgthumb_ins + 0x400);
   thumb_opcode_strings = (char **)(arm_opcode_strings + 0x1000);

   for(i = 0;i<0x1000;i++){
       arm_ins[i] = unknown_ins;
#ifdef _DEBUG
       dbgarm_ins[i] = standard_debug_handle;
       arm_opcode_strings[i] = "Unknown";
#endif
   }
   for(i = 0;i<0x400;i++){
       thumb_ins[i] = unknown_ins;
#ifdef _DEBUG
       dbgthumb_ins[i] = standard_debug_handle_thumb;
       thumb_opcode_strings[i] = "Unknown";
#endif
   }
//arm
   setup_dp_handle(arm_ins,0x00,and_,and_,and_reg,and_imm);//ok
   setup_dp_handle(arm_ins,0x10,ands,ands,ands_reg,ands_imm);//ok
   setup_dp_handle(arm_ins,0x20,eor,eor,eor_reg,eor_imm);//ok
   setup_dp_handle(arm_ins,0x30,eors,eors,eors_reg,eors_imm);//ok
   setup_dp_handle(arm_ins,0x40,sub,sub,sub_reg,sub_imm);//ok
   setup_dp_handle(arm_ins,0x50,subs,subs,subs_reg,subs_imm);//ok
   setup_dp_handle(arm_ins,0x60,rs,rs,rs_reg,rs_imm);//ok
   setup_dp_handle(arm_ins,0x70,rss,rss,rss_reg,rss_imm);//ok
   setup_dp_handle(arm_ins,0x80,add,add,add_reg,add_imm);//ok
   setup_dp_handle(arm_ins,0x90,adds,adds,adds_reg,adds_imm);//ok
   setup_dp_handle(arm_ins,0xA0,adc,adc,adc_reg,adc_imm);//ok
   setup_dp_handle(arm_ins,0xB0,adcs,adcs,adcs_reg,adcs_imm);//ok
   setup_dp_handle(arm_ins,0xC0,subc,subc,subc_reg,subc_imm);//ok
   setup_dp_handle(arm_ins,0xD0,subcs,subcs,subcs_reg,subcs_imm);//ok
   setup_dp_handle(arm_ins,0xE0,rsbc,rsbc,rsbc_reg,rsbc_imm);//ok
   setup_dp_handle(arm_ins,0xF0,rsbcs,rsbcs,rsbcs_reg,rsbcs_imm);//ok
   setup_dp_handle(arm_ins,0x110,tst,tst,tst_reg,tst_imm);//ok
   setup_dp_handle(arm_ins,0x130,teq,teq,teq_reg,teq_imm);//ok
   setup_dp_handle(arm_ins,0x150,cmp,cmp,cmp_reg,cmp_imm);//ok
   setup_dp_handle(arm_ins,0x170,cmn,cmn,cmn_reg,cmn_imm);//ok
   setup_dp_handle(arm_ins,0x180,orr,orr,orr_reg,orr_imm);//ok
   setup_dp_handle(arm_ins,0x190,orrs,orrs,orrs_reg,orrs_imm);//ok
   setup_dp_handle(arm_ins,0x1A0,mov,mov,mov_reg,mov_imm);//ok
   setup_dp_handle(arm_ins,0x1B0,movs,movs,movs_reg,movs_imm);//ok
   setup_dp_handle(arm_ins,0x1C0,bic,bic,bic_reg,bic_imm);//ok
   setup_dp_handle(arm_ins,0x1D0,bics,bics,bics_reg,bics_imm);//ok
   setup_dp_handle(arm_ins,0x1E0,mvn,mvn,mvn_reg,mvn_imm);//ok
   setup_dp_handle(arm_ins,0x1F0,mvns,mvns,mvns_reg,mvns_imm);//ok

   for (i=0; i<0x10; i++) {
       if(!(i & 1)){
           arm_ins[0x100|i] = ins_mrs_cpsr;//ok
           arm_ins[0x120|i] = ins_msr_cpsr;//ok
           arm_ins[0x140|i] = ins_mrs_spsr;//ok
           arm_ins[0x160|i] = ins_msr_spsr;//ok
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
   setup_sdt_handles2(arm_ins,0x410,ldr_postdownimm,ldr_postdownimm,ldr_postupimm,ldr_postupimm,ldr_predownimm,ldr_preupimm,ldr_predownimmwb,ldr_preupimmwb);
   setup_sdt_handles2(arm_ins,0x450,ldrb_postdownimm,ldrb_postdownimm,ldrb_postupimm,ldrb_postupimm,ldrb_predownimm,ldrb_preupimm,ldrb_predownimmwb,ldrb_preupimmwb);
   setup_sdt_handles2(arm_ins,0x610,ldr_postdown,ldr_postdown,ldr_postup,ldr_postup,ldr_predown,ldr_preup,ldr_predownwb,ldr_preupwb);
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
   for (i=0; i<0x100; i++)
       arm_ins[0xF00|i] = ins_swi;
   for (i=0; i<0x80; i++) {
       arm_ins[0xA00|i] = ins_bmi;
       arm_ins[0xA80|i] = ins_bmi;
       arm_ins[0xB00|i] = ins_blmi;
       arm_ins[0xB80|i] = ins_blmi;
   }
   for(i=0;i<0xFF;i++){
       if((i & 1)){
           if((i >> 4) & 1)
               arm_ins[0xE00+i] = mrc;
           else
               arm_ins[0xE00+i] = mcr;
       }
       else
           arm_ins[0xE00 + i] = cdp;
   }
//   arm_ins[0x123] = ins_blx;
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

#ifdef _DEBUG
   setup_dp_strings(arm_opcode_strings, 0x00,"and");//ok
   setup_dp_strings(arm_opcode_strings, 0x10,"ands");//ok
   setup_dp_strings(arm_opcode_strings, 0x20,"eor");//ok
   setup_dp_strings(arm_opcode_strings, 0x30,"eors");//ok
   setup_dp_strings(arm_opcode_strings, 0x40,"sub");//ok
   setup_dp_strings(arm_opcode_strings, 0x50,"subs");//ok
   setup_dp_strings(arm_opcode_strings, 0x60,"rsb");//ok
   setup_dp_strings(arm_opcode_strings, 0x70,"rsbs");//ok
   setup_dp_strings(arm_opcode_strings, 0x80,"add");//ok
   setup_dp_strings(arm_opcode_strings, 0x90,"adds");//ok
   setup_dp_strings(arm_opcode_strings, 0xA0,"adc");//ok
   setup_dp_strings(arm_opcode_strings, 0xB0,"adcs");//ok
   setup_dp_strings(arm_opcode_strings, 0xC0,"subc");//ok
   setup_dp_strings(arm_opcode_strings, 0xD0,"subcs");//ok
   setup_dp_strings(arm_opcode_strings, 0xE0,"rsbc");//ok
   setup_dp_strings(arm_opcode_strings, 0xF0,"rsbcs");//ok
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
       for(n=0;n<=0xF;n++) {
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
   for(i=0;i<0x100;i++) {
       arm_opcode_strings[0xA00|i] = "b";
       arm_opcode_strings[0xB00|i] = "bl";
   }
   for (i=0; i<0x1000; i++)
       dbgarm_ins[i] = standard_debug_handle;

   setup_dp_debug_handle (dbgarm_ins,0x00, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins, 0x10, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins, 0x20, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins, 0x30, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins, 0x40, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins, 0x50, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins, 0x60, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins, 0x70, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins, 0x80, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins, 0x90, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins, 0xA0, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins, 0xB0, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins, 0xC0, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins, 0xD0, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins, 0xE0, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins, 0xF0, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins,0x110, dpnw_debug_handle);
   setup_dp_debug_handle (dbgarm_ins,0x130, dpnw_debug_handle);
   setup_dp_debug_handle (dbgarm_ins,0x150, dpnw_debug_handle);
   setup_dp_debug_handle (dbgarm_ins,0x170, dpnw_debug_handle);
   setup_dp_debug_handle (dbgarm_ins,0x180, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins,0x190, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins,0x1A0, dpsingle_debug_handle);
   setup_dp_debug_handle (dbgarm_ins,0x1B0, dpsingle_debug_handle);
   setup_dp_debug_handle (dbgarm_ins,0x1C0, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins,0x1D0, dp_debug_handle);
   setup_dp_debug_handle (dbgarm_ins,0x1E0, dpsingle_debug_handle);
   setup_dp_debug_handle (dbgarm_ins,0x1F0, dpsingle_debug_handle);

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

   setup_sdt_debug_handles (dbgarm_ins,0x600, sdt_debug_handle);
   setup_sdt_debug_handles (dbgarm_ins,0x640, sdt_debug_handle);
   setup_sdt_debug_handles (dbgarm_ins,0x400, sdt_debug_handle);
   setup_sdt_debug_handles (dbgarm_ins,0x440, sdt_debug_handle);
   setup_sdt_debug_handles (dbgarm_ins,0x610, sdt_debug_handle);
   setup_sdt_debug_handles (dbgarm_ins,0x650, sdt_debug_handle);
   setup_sdt_debug_handles (dbgarm_ins,0x410, sdt_debug_handle);
   setup_sdt_debug_handles (dbgarm_ins,0x450, sdt_debug_handle);

   setup_hwdt_debug_handles (dbgarm_ins,0x5B|0x4,hwdt_debug_handle);
   setup_hwdt_debug_handles (dbgarm_ins,0x5D,hwdt_debug_handle);
   setup_hwdt_debug_handles (dbgarm_ins,0x1B,hwdt_debug_handle);
   setup_hwdt_debug_handles (dbgarm_ins,0x1B|0x4,hwdt_debug_handle);
   setup_hwdt_debug_handles (dbgarm_ins,0x1D,hwdt_debug_handle);
   setup_hwdt_debug_handles (dbgarm_ins,0x4B,hwdt_debug_handle);
   setup_hwdt_debug_handles (dbgarm_ins,0xB,hwdt_debug_handle);
   setup_hwdt_debug_handles (dbgarm_ins,0x5B,hwdt_debug_handle);

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
   dbgarm_ins[0x121] = bx_debug_handle_arm;
   dbgarm_ins[0x149] =     swp_debug_handle;
   dbgarm_ins[0x161] =     clz_debug_handle_arm;

   for(i=0;i<0x100;i++){
       dbgarm_ins[0xF00|i] = swi_debug_handle;
       dbgarm_ins[0xA00|i] = b_debug_handle_arm;
       dbgarm_ins[0xB00|i] = b_debug_handle_arm;
       dbgarm_ins[0xE00+i] = sc_debug_handle_arm;
   }
//   arm_opcode_strings[0x123] = "blx";
   dbgarm_ins[0x123] = bx_debug_handle_arm;
#endif
//thumb
   for(i=0;i<0x400;i++)
       thumb_ins[i] = unknown_ins;
   for (i=0; i<0x8; i++) {
       thumb_ins [0x60|i] =  tins_add;//ok
       thumb_ins [0x68|i] =   tins_sub;//ok
       thumb_ins [0x70|i] =   tins_add_short_imm;//ok
       thumb_ins [0x78|i] =   tins_sub_reg_imm;//ok
       thumb_ins [0x140|i] =  tins_str_reg;//ok
       thumb_ins [0x150|i] =  tins_strb_reg;//ok
       thumb_ins [0x160|i] =  tins_ldr_reg;//ok
       thumb_ins [0x170|i] =  tins_ldrb_reg;//ok
       thumb_ins [0x148|i] =  tins_strh_reg;//ok
       thumb_ins [0x158|i] =  tins_ldrsb_reg;//ok
       thumb_ins [0x168|i] =  tins_ldrh_reg;//ok
       thumb_ins [0x178|i] =  tins_ldrsh;//ok
       thumb_ins [0x2D0|i] =  tins_push;
       thumb_ins [0x2F0|i] =  tins_pop;
   }

   for(i=0;i<0x20;i++){
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

   for (i=0; i<0x20; i++){
       thumb_ins[0x380|i] =   tins_bu;
       thumb_ins[0x3A0|i] =   tins_bl;
       thumb_ins[0x3C0|i] =   tins_bl;
       thumb_ins[0x3E0|i] =   tins_bl;
   }
   for(i=0;i<0x3c;i++)
       thumb_ins[0x340|i] =   tins_b;
   for(i=0;i<4;i++)
       thumb_ins[0x37c|i] =   tins_swi;
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
   for (i=0; i<0x8; i++){
       dbgthumb_ins [0x60|i] = reg_debug_handle_t;
       dbgthumb_ins [0x68|i] = reg_debug_handle_t;
       dbgthumb_ins [0x70|i]  = immshort_debug_handle_t;//ok
       dbgthumb_ins [0x78|i]  = immshort_debug_handle_t;//ok
       dbgthumb_ins [0x140|i] = reg_debug_handle_t;//ok
       dbgthumb_ins [0x150|i] = reg_debug_handle_t;//ok
       dbgthumb_ins [0x160|i] = reg_debug_handle_t;//ok
       dbgthumb_ins [0x170|i] = reg_debug_handle_t;//ok
       dbgthumb_ins [0x148|i] = reg_debug_handle_t;//ok
       dbgthumb_ins [0x158|i] = reg_debug_handle_t;//ok
       dbgthumb_ins [0x168|i] = reg_debug_handle_t;//ok
       dbgthumb_ins [0x178|i] = reg_debug_handle_t;//ok
       dbgthumb_ins [0x2D0|i] = stack_debug_handle_t;
       dbgthumb_ins [0x2F0|i] = stack_debug_handle_t;
   }

   for(i=0;i<0x20;i++){
       dbgthumb_ins[i]       = immediate_debug_handle_t;//ok
       dbgthumb_ins[0x20|i]  = immediate_debug_handle_t;//ok
       dbgthumb_ins[0x40|i]  = immediate_debug_handle_t;//ok
       dbgthumb_ins[0x80|i]  = immlong_debug_handle_t;//ok
       dbgthumb_ins[0xA0|i]  = immlong_debug_handle_t;//ok
       dbgthumb_ins[0xC0|i]  = immlong_debug_handle_t;//ok
       dbgthumb_ins[0xE0|i]  = immlong_debug_handle_t;//ok
       dbgthumb_ins[0x120|i] = rs_debug_handle_t;
       dbgthumb_ins[0x180|i] = immediate_debug_handle_t;//ok32
       dbgthumb_ins[0x1A0|i] = immediate_debug_handle_t;//ok32
       dbgthumb_ins[0x1C0|i] = immediate_debug_handle_t;//ok
       dbgthumb_ins[0x1E0|i] = immediate_debug_handle_t;//OK
       dbgthumb_ins[0x200|i] = immediate_debug_handle_t;//OK
       dbgthumb_ins[0x220|i] = immediate_debug_handle_t;//OK
       dbgthumb_ins[0x240|i] = rs_debug_handle_t;//ok
       dbgthumb_ins[0x260|i] = rs_debug_handle_t;//ok
       dbgthumb_ins[0x280|i] = rs_debug_handle_t;//ok
       dbgthumb_ins[0x2A0|i] = rs_debug_handle_t;//ok
       dbgthumb_ins[0x300|i] = mdt_debug_handle_t;//ok
       dbgthumb_ins[0x320|i] = mdt_debug_handle_t;//ok
   }
   for(i=0;i<0x20;i++){
       dbgthumb_ins[0x380|i] = b_debug_handle_t;
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
       thumb_opcode_strings[0x3C0|i] =  "bl";
       thumb_opcode_strings[0x3E0|i] =  "bl";
   }
   for(i=0;i<0x3c;i++)
       thumb_opcode_strings[0x340|i] =  "b";
   for(i=0;i<4;i++)
       thumb_opcode_strings[0x37c|i] =  "swi";
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
   thumb_opcode_strings[0x11e] =        "bx";
   thumb_opcode_strings[0x11f] =        "bx";
   thumb_opcode_strings[0x2C0] =        "add";
   thumb_opcode_strings[0x2C1] =        "add";
   thumb_opcode_strings[0x2c2] =        "sub";
   thumb_opcode_strings[0x2c3] =        "sub";
#endif
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL CPU_ARM7::EnableCheats(void **func)
{
   return TRUE;
}
//---------------------------------------------------------------------------
void CPU_ARM7::Reset()
{
#ifdef _DEBPRO
   callStack.clear();
#endif
   ZeroMemory(reg,sizeof(reg));
   c_flag = z_flag = v_flag = n_flag = 0;
   opcode = 0;
   stop = 0;
   cpsr = SYSTEM_MODE;
   exec7 = exec_arm;
   fetch_func = fetch_arm;
   nIRQ7 = 0;
   reg[0][13] = 0x380FEC0;
   reg[1][13] = 0x380FFA0;
   reg[2][13] = 0x380FFC0;
   memcpy(gp_reg,reg[0],sizeof(gp_reg));
   pfn_arm7irq = arm7irq_empty;
}
//---------------------------------------------------------------------------
u8 CPU_ARM7::r_stop()
{
   return stop;
}
//---------------------------------------------------------------------------
void CPU_ARM7::w_stop(u8 value)
{
   stop = (u8)(value & 1);
   fromStopped = ds.get_CurrentCycles();   
}
//---------------------------------------------------------------------------
u8 CPU_ARM7::r_cf()
{
   return c_flag;
}
//---------------------------------------------------------------------------
void CPU_ARM7::w_cf(u8 value)
{
   c_flag = (u8)(value & 1);
}
//---------------------------------------------------------------------------
u8 CPU_ARM7::r_nf()
{
   return n_flag;
}
//---------------------------------------------------------------------------
void CPU_ARM7::w_nf(u8 value)
{
   n_flag = (u8)(value & 1);
}
//---------------------------------------------------------------------------
u8 CPU_ARM7::r_zf()
{
   return z_flag;
}
//---------------------------------------------------------------------------
void CPU_ARM7::w_zf(u8 value)
{
   z_flag = (u8)(value & 1);
}
//---------------------------------------------------------------------------
u8 CPU_ARM7::r_vf()
{
   return v_flag;
}
//---------------------------------------------------------------------------
void CPU_ARM7::w_vf(u8 value)
{
   v_flag = (u8)(value & 1);
}
//---------------------------------------------------------------------------
u8 CPU_ARM7::r_qf()
{
   return 0;
}
//---------------------------------------------------------------------------
void CPU_ARM7::w_qf(u8 value)
{
}
//---------------------------------------------------------------------------
u32 CPU_ARM7::r_opcode()
{
   return opcode;
}
//---------------------------------------------------------------------------
u32 CPU_ARM7::r_modereg(int mode,int index)
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
u32 CPU_ARM7::r_gpreg(int index)
{
   if(index == -1)
       return old_pc;
   return gp_reg[index % 17];
}
//---------------------------------------------------------------------------
void CPU_ARM7::w_gpreg(int index,u32 value)
{
   gp_reg[index % 17] = value;
}
//---------------------------------------------------------------------------
void CPU_ARM7::decode_ins(u8 mode,u32 adress, char *dest)
{
   u32 op;

   if(mode){
       op = read_mem(adress,AMM_HWORD);
       wsprintf(dest,"%08X %08X ",adress,op);
       dbgthumb_ins[op >> 6](thumb_opcode_strings,op,adress,dest+18,this);
   }
   else{
       op = read_mem(adress,AMM_WORD);
       wsprintf(dest,"%08X %08X ",adress,op);
       dbgarm_ins[((op & 0x0FF00000)>>16)|(((u8)op)>>4)](arm_opcode_strings,op,adress,dest+18,this);
   }
}
//---------------------------------------------------------------------------
void CPU_ARM7::InitPipeLine(u32 offset)
{
   opcode = read_word(offset);
   old_pc = offset;
   gp_reg[15] = offset + 8;
   exec7 = exec_arm;
}
//---------------------------------------------------------------------------
u32 CPU_ARM7::r_cpsr()
{
   return cpsr;
}
//---------------------------------------------------------------------------
void CPU_ARM7::w_cpsr(u32 value)
{
   cpsr = value;
}
//---------------------------------------------------------------------------
LVector<unsigned long> *CPU_ARM7::r_callstack()
{
   return &callStack;
}
//---------------------------------------------------------------------------
LPMEMORYINFO CPU_ARM7::r_memmap()
{
   return MemoryAddress;
}
//---------------------------------------------------------------------------
inline int CPU_ARM7::r_memmapsize()
{
   return (int)(sizeof(MemoryAddress) / sizeof(MEMORYINFO));
}
//---------------------------------------------------------------------------
u32 CPU_ARM7::remap_address(u32 address)                                 
{
   return address;
}
//---------------------------------------------------------------------------
void CPU_ARM7::write_mem(u32 address,u32 data,u8 mode)
{
   switch(mode){
       case AMM_WORD:
       	if(ds.get_EmuMode() == EMUM_NDS){
               if((address & 0x0F000000) == 0x06000000){
                   if((io_mem[0x242] & 0x87) == 0x82 || (io_mem[0x243] & 0x87) == 0x82)
                       *((u32 *)(video_mem + ((VRAMmap[560+((address>>14)&0x3FF)]<<14)+(address&0x3FFC)))) = data;
                   return;
               }
           }
           write_word(address,data);
       break;
       case AMM_HWORD:
       	if(ds.get_EmuMode() == EMUM_NDS){
               if((address & 0x0F000000) == 0x06000000){
                   if((io_mem[0x242] & 0x87) == 0x82 || (io_mem[0x243] & 0x87) == 0x82)
                       *((u16 *)(video_mem + ((VRAMmap[560+((address>>14)&0x3FF)]<<14)+(address&0x3FFE)))) = (u16)data;
                   return;
               }
           }
           write_hword(address,data);
       break;
       case AMM_BYTE:
       	if(ds.get_EmuMode() == EMUM_NDS){
               if((address & 0x0F000000) == 0x06000000){
                   if((io_mem[0x242] & 0x87) == 0x82 || (io_mem[0x243] & 0x87) == 0x82)
                       *((video_mem + ((VRAMmap[560+((address>>14)&0x3FF)]<<14)+(address&0x3FFF)))) = (u8)data;
                   return;
               }
           }
           write_byte(address,data);
       break;
   }
}
//---------------------------------------------------------------------------
u32 CPU_ARM7::read_mem(u32 address,u8 mode)
{
   switch(mode){
       case AMM_WORD:
			switch((address >> 24)){
				case 4:
           		if((address & 0x00100000))
           			address = 0x2000 + (address & 0xFC);
           		else if((address & 0x00800000)){
           			return *(u32 *)(io_mem7 + (0x3000 + (address & 0x7FFC)));
           		}
           		else
						address &= 0x1FFC;
           		return *(u32 *)(io_mem7 + address);
       		default:
					return read_word(address);
   		}
       break;
       case AMM_HWORD:
       	switch((address >> 24)){
               case 4:
                   if((address & 0x00100000))
           	        address = 0x2000 + (address & 0xFE);
                   else if((address & 0x00800000)){
//                      	if(o_func7[0x3000 + (address & 0x7FFE)] != NULL)
//               	        return (u32)(u16)o_func7[0x3000 + (address & 0x7FFE)]((u32)(u16)address,AMM_HWORD);
           	        return (u32)*(u16 *)(io_mem7 + (0x3000 + (address & 0x7FFE)));
                   }
                   else
           	        address &= 0x1FFE;
//                   if(o_func7[address] != NULL)
//                       return (u32)(u16)o_func7[address](address,AMM_HWORD);
                   return (u32)*(u16 *)(io_mem7 + address);
               default:
			        return read_hword(address);
           }
       break;
       case AMM_BYTE:
			switch(address >> 24){
       		case 4:
           		if((address & 0x00100000))
           			address = 0x2000 + (u8)address;
           		else if((address & 0x00800000))
           			address = 0x3000 + (address & 0x7FFF);
           		else
           			address &= 0x1FFF;
           		return (u32)io_mem7[address];
       		default:
					return read_byte(address);
   		}
       break;
   }
   return 0;
}
//---------------------------------------------------------------------------
BOOL CPU_ARM7::Load(LStream *pFile,int ver)
{
   if(pFile == NULL || !pFile->IsOpen())
        return FALSE;
   pFile->Read(&opcode,sizeof(u32));
   pFile->Read(&z_flag,sizeof(u8));
   pFile->Read(&n_flag,sizeof(u8));
   pFile->Read(&c_flag,sizeof(u8));
   pFile->Read(&v_flag,sizeof(u8));
	if(ver < 42)
   	pFile->Read(reg,272);
   else
   	pFile->Read(reg,sizeof(reg));
   pFile->Read(gp_reg,sizeof(gp_reg));
   pFile->Read(&cpsr,sizeof(u32));
   pFile->Read(&old_pc,sizeof(u32));
   pFile->Read(&stop,sizeof(u8));
   pFile->Read(&nIRQ7,sizeof(nIRQ7));
   if(cpsr & T_BIT){
       exec7 = exec_thumb;
       fetch_func = fetch_thumb;
   }
   else{
       exec7 = exec_arm;
       fetch_func = fetch_arm;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL CPU_ARM7::Save(LStream *pFile)
{
   if(pFile == NULL || !pFile->IsOpen())
       return FALSE;
   pFile->Write(&opcode,sizeof(u32));
   pFile->Write(&z_flag,sizeof(u8));
   pFile->Write(&n_flag,sizeof(u8));
   pFile->Write(&c_flag,sizeof(u8));
   pFile->Write(&v_flag,sizeof(u8));
   pFile->Write(reg,sizeof(reg));
   pFile->Write(gp_reg,sizeof(gp_reg));   
   pFile->Write(&cpsr,sizeof(u32));
   pFile->Write(&old_pc,sizeof(u32));
   pFile->Write(&stop,sizeof(u8));
   pFile->Write(&nIRQ7,sizeof(nIRQ7));
   return TRUE;
}
//---------------------------------------------------------------------------
NO_IN_LINE static void ioIF(u32 adr,u32 data,u8 am)
{
   if(ds.get_EmuMode() == EMUM_NDS){
       nIRQ7 &= ~data;
       *((u32 *)(io_mem7 + 0x214)) = nIRQ7;
   }
   else{
   	if(adr == 0x200 && am != AMM_WORD)
       	return;
       if(am == AMM_WORD)
           data = (u32)(u16)(data >> 16);
       nIRQ7 &= ~data;
       *((u16 *)(io_mem7 + 0x202)) = (u16)nIRQ7;
   }
}
//---------------------------------------------------------------------------
u32 CPU_ARM7::get_CurrentPC()
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
void CPU_ARM7::InitTable(LPIFUNC *p,LPIFUNC *p1)
{
   int i;

   if(ds.get_EmuMode() == EMUM_NDS){
       for(i=0x202;i<0x204;i++)
           p1[i] = nullIOFunc;
       for(i=0x214;i<0x218;i++)
           p1[i] = ioIF;
       reg[0][13] = 0x380FEC0;
       reg[1][13] = 0x380FFA0;
       reg[2][13] = 0x380FFC0;
   }
   else{
       for(i=0x200;i<0x204;i++)
           p1[i] = ioIF;
       for(i=0x214;i<0x218;i++)
           p1[i] = nullIOFunc;
       reg[0][13] = 0x03007F00;
       reg[1][13] = 0x03007FA0;
       reg[2][13] = 0x03007FE0;
   }
}
//---------------------------------------------------------------------------
u32 CPU_ARM7::get_Stopped()
{
   if(stop)
       stopped += ds.get_CurrentCycles() - fromStopped;
   return stopped;
}
//---------------------------------------------------------------------------
void CPU_ARM7::set_Stopped()
{
   fromStopped = stopped = 0;
}
//---------------------------------------------------------------------------
int CPU_ARM7::enter_dmaloop()
{
   return 0;
}



