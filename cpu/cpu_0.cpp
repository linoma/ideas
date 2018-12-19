#include "ideastypes.h"
#include "dsmem.h"
#include "cpu_0.h"

#ifdef _DEBUG
//-----------------------------------------------------------------------
static char *condition_strings[] = {"eq","ne","cs","cc","mi","pl","vs","vc","hi","ls","ge","lt","gt","le","",""};
static char *register_strings[]  = {"r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12","sp","lr","pc"};
static char *shift_strings[]     = {"lsl","lsr","asr","ror"};
static char *msr_fields[] 		  = {"","c","x","xc","s","sc","sx","sxc","f","fc","fx","fxc","fs","fsc","fsx","fsxc"};
#endif
//-----------------------------------------------------------------------
void setup_hwdt_handles2(ARM_INS *arm_ins,u32 base,ARM_INS handle,ARM_INS handle2,ARM_INS handle3,
	ARM_INS handle4,ARM_INS handle2wb,ARM_INS handle4wb)
{
	arm_ins[base]        = handle;
	arm_ins[base|0x20]   = handle;
	arm_ins[base|0x80]   = handle3;//up
	arm_ins[base|0xA0]   = handle3;//up
	arm_ins[base|0x100]  = handle2;
	arm_ins[base|0x120]  = handle2wb;          //writeback
	arm_ins[base|0x180]  = handle4;//up
	arm_ins[base|0x1A0]  = handle4wb;//up      //writeback
}
//-----------------------------------------------------------------------
void setup_sdt_handles2(ARM_INS *arm_ins,u32 base,ARM_INS h,ARM_INS ht,ARM_INS h2,ARM_INS h2t,ARM_INS h3,
	ARM_INS h4,ARM_INS h3wb,ARM_INS h4wb)
{
	int i;

	for(i=0;i<0x10;i++){
		arm_ins[base|i]		  = h;//offset_12
		arm_ins[base|0x20|i]  = ht;//transtale
		arm_ins[base|0x80|i]  = h2;
		arm_ins[base|0xA0|i]  = h2t;//translate
		arm_ins[base|0x100|i] = h3;
		arm_ins[base|0x120|i] = h3wb;//writeback preindex
		arm_ins[base|0x180|i] = h4;//preindex add
		arm_ins[base|0x1A0|i] = h4wb;//writeback preindex
	}
}
//-----------------------------------------------------------------------
void setup_dp_handle(ARM_INS *arm_ins,u32 base,ARM_INS ins,ARM_INS ins_lsl,ARM_INS ins_reg,ARM_INS ins_imm)
{
	int i;

	for(i=0;i<0x10;i += 2){
		arm_ins[base|i]        = ins;//ok immediate shifts
       arm_ins[base|0x200|i]  = ins_imm;//ok 32-bit immediate
       arm_ins[base|0x201|i]  = ins_imm;//ok 32-bit immediate
	}
   arm_ins[base] = ins_lsl;
   arm_ins[base|8] = ins_lsl;
   for(i=1;i<8;i += 2)
   	arm_ins[base|i] = ins_reg;//ok register shifts
//   arm_ins[base|1] = ins_reg_lsl;
}
#ifdef _DEBUG
//-----------------------------------------------------------------------
void setup_dp_strings(char **arm_opcode_strings,u32 base, char *string)
{
	int i;

	for (i=0; i<0x10;i += 2){
		arm_opcode_strings[base|i]        = string;//ok
       arm_opcode_strings[base|0x200|i]  = string;//ok
       arm_opcode_strings[base|0x201|i]  = string;//ok
	}
   for(i=1;i<8;i+=2)
   	arm_opcode_strings[base|i]    = string;
}
//-----------------------------------------------------------------------
void setup_sdt_strings(char **arm_opcode_strings,u32 base, char *string)
{
	int i;

	for (i=0; i<0x10; i++) {
		arm_opcode_strings[base|i] = string;
		arm_opcode_strings[base|0x20|i] = string;
		arm_opcode_strings[base|0x80|i] = string;
		arm_opcode_strings[base|0xA0|i] = string;
		arm_opcode_strings[base|0x100|i] = string;
		arm_opcode_strings[base|0x120|i] = string;
		arm_opcode_strings[base|0x180|i] = string;
		arm_opcode_strings[base|0x1A0|i] = string;
	}
}
//-----------------------------------------------------------------------
void setup_dp_debug_handle(DECODEARM *dbgarm_ins,u32 base, DECODEARM handle)
{
	int i;

	for(i=0;i<0x10;i += 2){
		dbgarm_ins[base|i] = handle;//ok
       dbgarm_ins[base|0x200|i] = handle;//ok
       dbgarm_ins[base|0x201|i] = handle;//ok
	}
   for(i=1;i<8;i+= 2)
   	dbgarm_ins[base|i] = handle;
}
//-----------------------------------------------------------------------
void setup_sdt_debug_handles(DECODEARM *dbgarm_ins,u32 base, DECODEARM handle)
{
	int i;

	for(i=0; i<0x10; i++) {
		dbgarm_ins[base|i] = handle;
		dbgarm_ins[base|0x20|i]	= handle;
		dbgarm_ins[base|0x80|i]	= handle;
		dbgarm_ins[base|0x80|0x20|i] = handle;
		dbgarm_ins[base|0x100|i] = handle;
		dbgarm_ins[base|0x100|0x80|i] = handle;
		dbgarm_ins[base|0x100|0x20|i] = handle;
		dbgarm_ins[base|0x100|0x80|0x20|i] = handle;
	}
}
//-----------------------------------------------------------------------
void setup_hwdt_debug_handles(DECODEARM *dbgarm_ins,u32 base, DECODEARM handle)
{
	dbgarm_ins[base] = handle;
	dbgarm_ins[base|0x20] = handle;
	dbgarm_ins[base|0x80] = handle;
	dbgarm_ins[base|0x80|0x20] = handle;
	dbgarm_ins[base|0x100] = handle;
	dbgarm_ins[base|0x100|0x20] = handle;
	dbgarm_ins[base|0x100|0x80] = handle;
	dbgarm_ins[base|0x100|0x80|0x20] = handle;
}
//---------------------------------------------------------------------------
static void FillMultipleRegisterString(BYTE *s1,char *dest)
{
   BYTE *p,enterLoop,*p1;

   enterLoop = 0;
   lstrcat(dest,"{");
   p = s1;
   while(*p != 0xFF){
       if(!enterLoop){
           if(((int)p - (int)s1) > 0)
               lstrcat(dest,",");
           lstrcat(dest,register_strings[*p]);
           p1 = p;
       }
       if(abs(*(p+1) - *p) == 1)
           enterLoop = 1;
       else{
           if(enterLoop){
				if(abs(*p1 - *p) > 1)
               	lstrcat(dest,"-");
               else
               	lstrcat(dest,",");
               lstrcat(dest,register_strings[*p]);
               enterLoop=0;
           }
       }
       p++;
   }
   lstrcat(dest,"}");
}
//---------------------------------------------------------------------------
void standard_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
	lstrcpy(dest,arm_opcode_strings[((op&0xFF00000)>>16)|((op&0xF0)>>4)]);
   lstrcat(dest,condition_strings[op>>28]);
   lstrcat(dest," ");
}
//---------------------------------------------------------------------------
void clz_debug_handle_arm(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
	standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
   lstrcat(dest,register_strings[(op >> 12) & 0xF]);
   lstrcat(dest,",");
   lstrcat(dest,register_strings[op&0xF]);
}
//---------------------------------------------------------------------------
void sc_debug_handle_arm(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   char s[50];

	standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
   wsprintf(s,"p%d,%d,%s,c%d,c%d,%d",(int)((op >>8) & 0xF),(int)((op >> 20) & 0xf),
   	register_strings[(op >> 12)&0xF],(int)((op >> 16) & 0xF),(int)(op & 0xF),(int)((op >> 5) & 7));
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void b_debug_handle_arm(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
	char string[33];

	if((op >> 28) != 0xF)
		standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
   else
   	lstrcpy(dest,"blx ");
   if(op&0x800000)
       adress += (((op&0xFFFFFF)<<2)-0x4000000) + 8;
	else
       adress += ((op&0x7FFFFF)<<2) + 8;
   wsprintf(string,"0x%08X",adress);
	lstrcat(dest, string);
}
//---------------------------------------------------------------------------
void bx_debug_handle_arm(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
	standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
   lstrcat(dest,register_strings[(op&0xF)]);
}
//---------------------------------------------------------------------------
void second_operand(u32 op, u32 adress, char *dest)
{
   u32 temp;
	u32 shift_amount;
	char string [33], string2 [33];

	if((op&0x2000000)){
		shift_amount = ((op>>8)&0xF)<<1;
		temp = ((op&0xFF)<<(32-shift_amount))|((op&0xFF)>>shift_amount);
       wsprintf(string,"0x%X",temp);
		lstrcat (dest, string);
	}
	else{
       lstrcat(dest,register_strings[op&0xF]);
		lstrcpy(string,", ");
		lstrcat(string,shift_strings[(op>>5)&0x3]);
		if((op&0x10)) {
			lstrcat(string," ");
			lstrcat(string,register_strings[(op>>8)&0xF]);
			lstrcat(dest, string);
		}
		else {
			temp = (op>>7)&0x1F;
			if(!temp){
				switch((op>>5)&0x3) {
					case 0:
                   break;
					case 1:
                       lstrcat(dest,", lsr 0x20");
                   break;
					case 2:
						lstrcat(dest,", asr 0x20");
                   break;
					case 3:
						lstrcat(dest,", rrx");
                   break;
				}
			}
           else {
				lstrcat(string, " ");
               wsprintf(string2,"0x%X",temp);
				lstrcat(string, string2);
				lstrcat(dest, string);
			}
		}
	}
}
//---------------------------------------------------------------------------
void dpsingle_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char * dest,IARM7 *cpu)
{
	standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
	lstrcat(dest,register_strings[(op>>12)&0xF]);
	lstrcat(dest,", ");
	second_operand(op, adress, dest);
}
//---------------------------------------------------------------------------
void swi_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char * dest,IARM7 *cpu)
{
   char s[30];

	standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
   wsprintf(s,"0x%02X",(op & 0x000FFFFF));
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void dpnw_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char * dest,IARM7 *cpu)
{
	standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
	lstrcat(dest,register_strings[(op>>16)&0xF]);
	lstrcat(dest,", ");
	second_operand(op, adress, dest);
}
//---------------------------------------------------------------------------
void dp_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char * dest,IARM7 *cpu)
{
	standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
	lstrcat(dest,register_strings[(op>>12)&0xF]);
	lstrcat(dest,", ");
	lstrcat(dest,register_strings[(op>>16)&0xF]);
	lstrcat(dest,", ");
	second_operand(op, adress, dest);
}
//---------------------------------------------------------------------------
void mul_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
	standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
	lstrcat(dest,register_strings[(op>>16)&0xF]);
	lstrcat(dest,", ");
	lstrcat(dest,register_strings[op&0xF]);
	lstrcat(dest,", ");
	lstrcat(dest,register_strings[(op>>8)&0xF]);
   if(op & 0x00200000){
	    lstrcat(dest,", ");
	    lstrcat(dest,register_strings[(op>>12)&0xF]);
   }
}
//---------------------------------------------------------------------------
void mull_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
	standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
	lstrcat(dest,register_strings[(op>>12)&0xF]);
	lstrcat(dest,", ");
   lstrcat(dest,register_strings[(op>>16)&0xF]);
	lstrcat(dest,", ");
   lstrcat(dest,register_strings[op&0xF]);
   lstrcat(dest,",");
   lstrcat(dest,register_strings[(op>>8)&0xF]);
}
//---------------------------------------------------------------------------
void sdt_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
	u32 temp;
	char string [33], string2 [33];

	standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
	lstrcat(dest,register_strings[(op>>12)&0xF]);
	if((op&0x2000000)) {
		lstrcat(dest, ", [");
		lstrcat(dest, register_strings[(op>>16)&0xF]);
		if(!(op&0x1000000))
           lstrcat(dest, "]");
		lstrcat(dest, ", ");
		lstrcat(dest, register_strings[op&0xF]);
		lstrcpy(string, ", ");
		lstrcat(string, shift_strings[(op>>5)&0x3]);
		temp = (op>>7)&0x1F;
 		if(!temp){
			switch((op>>5)&0x3) {
				case 0:
               break;
				case 1:
                   lstrcpy(string2, ", ");
                   lstrcat(string2, "lsr 0x20");
                   lstrcat(dest, string2);
               break;
				case 2:
                   lstrcpy(string2, ", ");
                   lstrcat(string2, "asr 0x20");
                   lstrcat(dest, string2);
               break;
				case 3:
                   lstrcpy(string2, ", ");
                   lstrcat(string2, "rrx");
                   lstrcat(dest, string2);
               break;
			}
		}
       else{
			lstrcat(string," ");
           wsprintf(string2,"0x%08X",temp);
			lstrcat(string,string2);
			lstrcat(dest,string);
		}
		if((op&0x1000000)){
			lstrcat(dest, "]");
			if((op&0x200000))
               lstrcat(dest, "!");
		}
	}
	else{
		temp = (op&0xFFF);
		if(temp){
			if((((op>>16)&0xF)==15)&&(op&0x100000)) {
				if(op&0x800000)
                   temp = adress+8+temp;
               else
                   temp = adress+8-temp;
				if(op&0x400000)
                   temp = (u32)cpu->read_mem(temp,AMM_BYTE);
               else
                   temp = cpu->read_mem(temp,AMM_WORD);
               wsprintf(string,", 0x%X",temp);
				lstrcat(dest, string);
			}
           else {
				lstrcat(dest, ", [");
				lstrcat(dest, register_strings[(op>>16)&0xF]);
				if(!(op&0x1000000))
                   lstrcat (dest, "]");
				if((op&0x800000))
                   lstrcat(dest, ", ");
               else
                   lstrcat(dest, ", -");
               wsprintf(string,"0x%08X",temp);
				lstrcat(dest, string);
				if((op&0x1000000)) {
					lstrcat(dest,"]");
					if((op&0x200000))
                       lstrcat(dest,"!");
				}
			}
		}
		else {
			lstrcat(dest,", [");
			lstrcat(dest,register_strings[(op>>16)&0xF]);
			lstrcat(dest,"]");
		}
	}
}
//---------------------------------------------------------------------------
void hwdt_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   char string[33];
   int i;

	standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
	lstrcat(dest,register_strings[(op>>12)&0xF]);
   lstrcat(dest, ", [");
	lstrcat(dest, register_strings[(op>>16)&0xF]);
   if((op & 0x400000)){
       if((i = (((op&0xF00)>>4)|(op&0xF))) != 0){
           wsprintf(string,"0x%X",i);
           if((op & 0x800000) != 0)
               lstrcat(dest,"+");
           else
               lstrcat(dest,"-");
           lstrcat(dest,string);
       }
   }
   else{
       if((op & 0x800000) != 0)
           lstrcat(dest,"+");
       else
           lstrcat(dest,"-");
       lstrcat(dest,register_strings[op&0xF]);
   }
   lstrcat(dest,"]");
}
//---------------------------------------------------------------------------
void mdt_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   int i;
   BYTE s[20],*p;

	lstrcpy(dest,arm_opcode_strings[((op&0xFF00000)>>16)|((op&0xF0)>>4)]);
   if((op & 0x800000)){//up
       if((op & 0x1000000))
           lstrcat(dest,"ib");
       else
           lstrcat(dest,"ia");
   }
   else{
       if((op & 0x1000000))
           lstrcat(dest,"db");
       else
           lstrcat(dest,"da");
   }
	lstrcat(dest,condition_strings[(op>>28)]);
   lstrcat(dest," ");
   lstrcat(dest,register_strings[(op>>16)&0xF]);
   p = s;
   for(i=0;i<16;i++){
       if((op & (1 << i)))
           *p++ = (BYTE)i;
   }
   *p = 0xFF;
   if((op & 0x200000))
       lstrcat(dest,"!");
   lstrcat(dest,",");
   FillMultipleRegisterString(s,dest);
   if((op & 0x400000))
       lstrcat(dest,"^");
}
//---------------------------------------------------------------------------
void msr_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
	char s[15];

	standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
   if(!(op & (1 << 25)) && !(op & (1 << 21))){
       lstrcat(dest,register_strings[(op >> 12) & 0xF]);
       if((op & (1 << 22)))
           lstrcat(dest,",spsr");
       else
           lstrcat(dest,",cpsr");
   }
   else{
       if((op & (1 << 22)))
           lstrcat(dest,"spsr");
       else
           lstrcat(dest,"cpsr");
       wsprintf(s,"_%s,%s",msr_fields[(op >> 16) & 0xF],register_strings[op & 0xF]);
     	lstrcat(dest,s);
   }
}
//---------------------------------------------------------------------------
void mrs_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
	standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
}
//---------------------------------------------------------------------------
void swp_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   char s[30];

	standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
   wsprintf(s,"%s, %s, [%s]",register_strings[(op >> 12) & 0xF],
   	register_strings[op & 0xF],register_strings[(op >> 16) & 0xF]);
	lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void ldrd_debug_handle_arm(char **arm_opcode_strings,u32 op,u32 adress,char *dest,IARM7 *cpu)
{
	char s[30];

	standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
   lstrcat(dest,condition_strings[(op>>28)]);
	lstrcat(dest," ");
   wsprintf(s,"%s,[%s",register_strings[(op>>12)&0xF],register_strings[(op >> 16) & 0xF]);
	lstrcat(dest,s);
  	lstrcat(dest,(op & 0x800000) ? "+" : "-");
	if(op & 0x400000)
   	wsprintf(s,"0x%X]",((op & 0xF) | ((op & 0xF00) >> 4)));
   else
		wsprintf(s,"%s]",register_strings[op&0xF]);
   lstrcat(dest,s);
   if((op & 0x200000))
       lstrcat(dest,"!");
}
//---------------------------------------------------------------------------
void qadd_debug_handle_arm(char **arm_opcode_strings,u32 op,u32 adress,char *dest,IARM7 *cpu)
{
	char s[30];

	standard_debug_handle(arm_opcode_strings,op,adress,dest,cpu);
   lstrcat(dest,condition_strings[(op>>28)]);
   wsprintf(s,"%s,%s,%s",register_strings[(op >> 12) & 15],register_strings[op & 15],
       register_strings[(op >> 16) & 15]);
	lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void smulxy_debug_handle_arm(char **arm_opcode_strings,u32 op,u32 adress,char *dest,IARM7 *cpu)
{
	char s[30];

	lstrcpy(dest,arm_opcode_strings[((op&0xFF00000)>>16)|((op&0xF0)>>4)]);
   lstrcat(dest,condition_strings[(op>>28)]);
   wsprintf(s,"%c%c %s,%s,%s",(op & 0x20) ? 't' : 'b',(op & 0x40) ? 't' : 'b',
   	register_strings[(op >> 16) & 15],register_strings[op & 15],register_strings[(op >> 8) & 15]);
	lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void smlaxy_debug_handle_arm(char **arm_opcode_strings,u32 op,u32 adress,char *dest,IARM7 *cpu)
{
	char s[30];

	lstrcpy(dest,arm_opcode_strings[((op&0xFF00000)>>16)|((op&0xF0)>>4)]);
   lstrcat(dest,condition_strings[(op>>28)]);
   wsprintf(s,"%c%c %s,%s,%s,%s",(op & 0x20) ? 't' : 'b',(op & 0x40) ? 't' : 'b',
   	register_strings[(op >> 16) & 15],register_strings[op & 15],register_strings[(op >> 8) & 15],
           register_strings[(op >> 12) & 15]);
	lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void smlawy_debug_handle_arm(char **arm_opcode_strings,u32 op,u32 adress,char *dest,IARM7 *cpu)
{
	char s[30];

	lstrcpy(dest,arm_opcode_strings[((op&0xFF00000)>>16)|((op&0xF0)>>4)]);
   lstrcat(dest,condition_strings[(op>>28)]);
   if(!(op & 0x20)){
   	wsprintf(s,"%c %s,%s,%s,%s",(op & 0x40) ? 't' : 'b',
   		register_strings[(op >> 16) & 15],register_strings[op & 15],register_strings[(op >> 8) & 15]
           	,register_strings[(op >> 12) & 15]);
   }
   else{
   	wsprintf(s,"%c %s,%s,%s",(op & 0x40) ? 't' : 'b',register_strings[(op >> 16) & 15],
       	register_strings[op & 15],register_strings[(op >> 8) & 15]);
   }
	lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void standard_debug_handle_thumb(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
	lstrcpy(dest,thumb_opcode_strings[op>>6]);
	lstrcat(dest," ");
}
//---------------------------------------------------------------------------
void standard_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   char s[30];

   standard_debug_handle_thumb(thumb_opcode_strings,op,adress,dest,cpu);
   wsprintf(s,"%s,%s",register_strings[(op&0x7)],register_strings[(op >> 3) & 0x7]);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void stack_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   int i;
   BYTE *p,s1[20];

   standard_debug_handle_thumb(thumb_opcode_strings,op,adress,dest,cpu);
   p = s1;
   if((op & 0x800)){
       for(i=0;i<8;i++){
           if((op & (1 << i)))
               *p++ = (BYTE)i;
       }
       if((op & 0x100))
           *p++ = 15;
   }
   else{
       if((op & 0x100))
           *p++ = 14;
       for(i=0;i<8;i++){
           if((op & (1 << i)))
               *p++ = (BYTE)i;
       }
   }
   *p = 0xFF;
   FillMultipleRegisterString(s1,dest);
}
//---------------------------------------------------------------------------
void mdt_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   int i;
   BYTE *p,s1[20];

   standard_debug_handle_thumb(thumb_opcode_strings,op,adress,dest,cpu);
   lstrcat(dest,register_strings[(op>>8)&0x7]);
   lstrcat(dest,"!,");
   p = s1;
   for(i=0;i<8;i++){
       if((op & (1 << i)))
           *p++ = (BYTE)i;
   }
   *p = 0xFF;
   FillMultipleRegisterString(s1,dest);
}
//---------------------------------------------------------------------------
void bx_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   standard_debug_handle_thumb(thumb_opcode_strings,op,adress,dest,cpu);
   lstrcat(dest,register_strings[(op >> 3) & 0xF]);
}
//---------------------------------------------------------------------------
void hireg_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   char s[30];

   standard_debug_handle_thumb(thumb_opcode_strings,op,adress,dest,cpu);
   wsprintf(s,"%s,%s",register_strings[((op & 0x80) >> 4)|(op&0x7)],register_strings[(op >> 3) & 0xF]);
   lstrcat(dest,s);
}                                          
//---------------------------------------------------------------------------
void bcond_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   int offset;
   char s[30];

   lstrcpy(dest,thumb_opcode_strings[op>>6]);
   lstrcat(dest,condition_strings[(op>>8)& 0xF]);
   if(((offset = ((u8)op) << 1) & 0x100))
       offset = -(512 - offset);
   adress += offset + 4;
   wsprintf(s," 0x%08X",adress);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void immshort_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   char s[30];

   standard_debug_handle_thumb(thumb_opcode_strings,op,adress,dest,cpu);
   wsprintf(s,"%s, [%s, 0x%1X]",register_strings[op & 0x7],register_strings[(op >> 3) & 0x7],(op >> 6) & 0x7);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void immlong_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   char s[30];

   standard_debug_handle_thumb(thumb_opcode_strings,op,adress,dest,cpu);
   wsprintf(s,"%s,0x%02X",register_strings[(op>>8)&0x7],(u8)op);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void reg_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   char s[30];

   standard_debug_handle_thumb(thumb_opcode_strings,op,adress,dest,cpu);
   wsprintf(s,"%s, [%s, %s]",register_strings[(op&0x7)],register_strings[(op >> 3) & 0x7],register_strings[(op >> 6) & 0x7]);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void immediate_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   char s[30];
   int i;

   standard_debug_handle_thumb(thumb_opcode_strings,op,adress,dest,cpu);
   wsprintf(s,"%s, [%s",register_strings[(op&0x7)],register_strings[(op >> 3) & 0x7]);
   lstrcat(dest,s);
   i = (op >> 6) & 0x1F;
   wsprintf(s,", 0x%02X]",i);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void immediate_debug_handle_t4(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   char s[30];                  
   int i;

   standard_debug_handle_thumb(thumb_opcode_strings,op,adress,dest,cpu);
   wsprintf(s,"%s, [%s",register_strings[(op&0x7)],register_strings[(op >> 3) & 0x7]);
   lstrcat(dest,s);
   i = ((op >> 6) & 0x1F) * 4;
   wsprintf(s,", 0x%02X]",i);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void immediate_debug_handle_t2(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   char s[30];
   int i;

   standard_debug_handle_thumb(thumb_opcode_strings,op,adress,dest,cpu);
   wsprintf(s,"%s, %s",register_strings[(op&0x7)],register_strings[(op >> 3) & 0x7]);
   lstrcat(dest,s);
   i = ((op >> 6) & 0x1F) * 2;
   wsprintf(s,", 0x%02X",i);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void b_debug_handle_t(char **thumb_opcode_strings,u32 op,u32 adress,char *dest,IARM7 *cpu)
{
   char s[30];
   int offset;

   standard_debug_handle_thumb(thumb_opcode_strings,op,adress,dest,cpu);
   if(((offset = ((op&0x7ff) << 1)) & 0x800))
       offset = -(4096 - offset);
   adress = (int)adress + offset + 4;
   wsprintf(s,"0x%08X",adress);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void rs_debug_handle_t(char **thumb_opcode_strings,u32 op,u32 adress,char *dest,IARM7 *cpu)
{
   char s[30];

   standard_debug_handle_thumb(thumb_opcode_strings,op,adress,dest,cpu);
   if((op >> 11) == 0x9){
       wsprintf(s,"%s,0x%08X",register_strings[(op >> 8)&0x7],
       	cpu->read_mem((adress & ~3) + 4 + (((u8)op) << 2),AMM_WORD));
   }
   else if(!(op & 0x2000)) //ldr,str PC
       wsprintf(s,"%s,sp,0x%03X",register_strings[(op >> 8)&0x7],((u8)op) << 2);
   else{
       if((op & 0x1000))
           wsprintf(s,"sp,sp,0x%03X",(op & 0x7F)<<2);
       else{
           if((op & 0x800))
               wsprintf(s,"%s,sp,0x%03X",register_strings[(op >> 8)&0x7],((u8)op) << 2);
           else
               wsprintf(s,"%s,pc,0x%03X",register_strings[(op >> 8)&0x7],((u8)op) << 2);
       }
   }
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void swi_debug_handle_t(char **thumb_opcode_strings,u32 op,u32 adress,char *dest,IARM7 *cpu)
{
   char s[30];

   standard_debug_handle_thumb(thumb_opcode_strings,op,adress,dest,cpu);
   wsprintf(s,"0x%02X",(u8)op);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void bl_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu)
{
   int temp;
   char s[30],h;

   standard_debug_handle_thumb(thumb_opcode_strings,op,adress,dest,cpu);
   h = (char)((op >> 11) & 3);
   if(h == 2){
       temp = ((op & 0x7ff) << 12);
       op = (u16)(cpu->read_mem(adress+2,AMM_HWORD));
       if(((temp |= ((op & 0x7ff)<<1)) & 0x400000))
           temp = -(0x800000-temp);
       adress = (int)adress + temp + 4;
       wsprintf(s,"0x%08X",adress);
       lstrcat(dest,s);
   }
   else{
       lstrcpy(dest,"**blh 0x");
       wsprintf(s,"%08X**",(op & 0x7FF));
       lstrcat(dest,s);
   }
}
//---------------------------------------------------------------------------
void setup_sdt_strings_00(char **arm_opcode_strings,u32 base, char *string)
{
   arm_opcode_strings[base]	   = string;
	arm_opcode_strings[base|0x20]  = string;
	arm_opcode_strings[base|0x80]  = string;
	arm_opcode_strings[base|0xA0]  = string;
	arm_opcode_strings[base|0x100] = string;
	arm_opcode_strings[base|0x120] = string;
	arm_opcode_strings[base|0x180] = string;
	arm_opcode_strings[base|0x1A0] = string;
}

#endif


