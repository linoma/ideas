#include "dstype.h"
#include "lvec.hpp"

#ifndef __CPU_0H__
#define __CPU_0H__

#define LINE_CYCLES            1992
#define LINE_CYCLES_VISIBLE    1435

#define USER_MODE		    	0x10
#define FIQ_MODE		    	0x11
#define IRQ_MODE		    	0x12
#define SUPERVISOR_MODE		0x13
#define ABORT_MODE		    	0x17
#define UNDEFINED_MODE	    	0x1B
#define SYSTEM_MODE			0x1F
#define FIQ_BIT				0x00000040
#define IRQ_BIT				0x00000080
#define V_BIT		        	0x10000000
#define C_BIT		        	0x20000000
#define Z_BIT		        	0x40000000
#define N_BIT		        	0x80000000
#define V_SHIFT            	28
#define C_SHIFT            	29
#define Z_SHIFT            	30
#define N_SHIFT            	31
#define Q_SHIFT            	27
#define Q_BIT					0x08000000
#define T_BIT		        	0x00000020
#define T_SHIFT            	5

#define cycI				    1
#define cycP				    cyclesP
#define cycS				    (u8)wait_state.dummy.cyclesS
#define cycN				    (u8)wait_state.dummy.cyclesN
#define cycPN				    (u8)wait_state.dummy.cyclesN

#define LDR_RET                (u8)(cycS + cycPN)
#define LDRPC_RET              LDR_RET
#define LDRBASE_RET            LDR_RET
#define LDRBASEPC_RET          LDR_RET
#define STR_RET                (u8)(cycN + cycPN)
#define STRBASE_RET            (u8)(cycN + cycPN)

#define DEST_REG_INDEX         ((u16)opcode >> 12)
#define BASE_REG_INDEX         ((opcode >> 16) & 0xF)
#define DEST_REG		        gp_reg[DEST_REG_INDEX]
#define BASE_REG		        gp_reg[BASE_REG_INDEX]
#define LO_REG                 DEST_REG
#define HI_REG                 BASE_REG
#define OP_REG_INDEX			(opcode & 0xF)
#define OP_REG			        gp_reg[OP_REG_INDEX]
#define SHFT_AMO_REG_INDEX 	((opcode >> 8) & 0xF)
#define SHFT_AMO_REG	        gp_reg[SHFT_AMO_REG_INDEX]
#define IMM_SHIFT	            ((opcode >> 7) & 0x1F)

struct IARM7 : public LBase
{
   virtual u8 r_stop() = 0;
   virtual void w_stop(u8 value) = 0;
   virtual u8 r_cf() = 0;
   virtual void w_cf(u8 value) = 0;
   virtual u8 r_qf() = 0;
   virtual void w_qf(u8 value) = 0;
   virtual u8 r_vf() = 0;
   virtual void w_vf(u8 value) = 0;
   virtual u8 r_nf() = 0;
   virtual void w_nf(u8 value) = 0;
   virtual u8 r_zf() = 0;
   virtual void w_zf(u8 value) = 0;
   virtual u32 r_opcode() = 0;
	virtual u32 r_gpreg(int index) = 0;
   virtual u32 r_modereg(int mode,int index) = 0;
   virtual void w_gpreg(int index,u32 value) = 0;
	virtual void decode_ins(u8 mode,u32 adress, char *dest) = 0;
   virtual BOOL Init() = 0;
   virtual void Reset() = 0;
   virtual void InitPipeLine(u32 offset) = 0;
	virtual u32 r_cpsr() = 0;
   virtual void w_cpsr(u32 value) = 0;
	virtual int r_index() = 0;
   virtual LVector<unsigned long> *r_callstack() = 0;
   virtual LPMEMORYINFO r_memmap() = 0;
   virtual int r_memmapsize() = 0;
   virtual u32 remap_address(u32 address) = 0;
   virtual u32 read_mem(u32 address,u8 mode) = 0;
   virtual void write_mem(u32 address,u32 data,u8 mode) = 0;
   virtual BOOL EnableCheats(void **func) = 0;
   virtual void InitTable(LPIFUNC *p,LPIFUNC *p1) = 0;
   virtual u32 get_CurrentPC() = 0;
   virtual u32 get_Stopped() = 0;
   virtual void set_Stopped() = 0;
   virtual int enter_dmaloop() = 0;
#ifdef __BORLANDC__
   __property u8 Stop = {read = r_stop,write=w_stop};
   __property u8 c_f = {read = r_cf,write=w_cf};
   __property u8 n_f = {read = r_nf,write=w_nf};
   __property u8 v_f = {read = r_vf,write=w_vf};
   __property u8 z_f = {read = r_zf,write=w_zf};
   __property u8 q_f = {read = r_qf,write=w_qf};
   __property u32 op = {read = r_opcode};
   __property u32 gp_regs[int index] = {read = r_gpreg,write = w_gpreg};
   __property u32 mode_regs[int mode][int index] = {read = r_modereg};
   __property u32 status = {read = r_cpsr,write = w_cpsr};
   __property int Index = {read = r_index};
   __property LVector<unsigned long>*CallStack = {read=r_callstack};
   __property LPMEMORYINFO MemoryMap={read=r_memmap};
   __property int MemoryMapSize={read=r_memmapsize};
#endif
};

typedef u8 (*ARM_INS)(void);
typedef void (*DECODEARM)(char **,u32,u32,char *,IARM7 *);

#ifndef I_FASTCALL
   #ifdef __BORLANDC__
       #define I_FASTCALL __fastcall
   #else
       #define I_FASTCALL __attribute__ ((regparm(2)))
   #endif
#endif
//---------------------------------------------------------------------------
#define SCNT_DATA  		1
#define SCNT_INS		2
#define SCNT_WRITE     	0x40
#define SCNT_READ		0x10
//---------------------------------------------------------------------------
void setup_hwdt_handles2(ARM_INS *arm_ins,u32 base,ARM_INS handle,ARM_INS handle2,ARM_INS handle3,
	ARM_INS handle4,ARM_INS handle2wb,ARM_INS handle4wb);
void setup_sdt_handles2(ARM_INS *arm_ins,u32 base,ARM_INS h,ARM_INS ht,ARM_INS h2,ARM_INS h2t,ARM_INS h3,
	ARM_INS h4,ARM_INS h3wb,ARM_INS h4wb);
void setup_dp_handle(ARM_INS *arm_ins,u32 base,ARM_INS ins,ARM_INS ins_lsl,ARM_INS ins_reg,ARM_INS ins_imm);
void setup_dp_strings(char **arm_opcode_strings,u32 base, char *string);
void setup_sdt_strings(char **arm_opcode_strings,u32 base, char *string);
void setup_dp_debug_handle(DECODEARM *dbgarm_ins,u32 base, DECODEARM handle);
void setup_sdt_debug_handles(DECODEARM *dbgarm_ins,u32 base, DECODEARM handle);
void setup_hwdt_debug_handles(DECODEARM *dbgarm_ins,u32 base, DECODEARM handle);
void standard_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void dpsingle_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char * dest,IARM7 *cpu);
void swi_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char * dest,IARM7 *cpu);
void dpnw_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char * dest,IARM7 *cpu);
void dp_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char * dest,IARM7 *cpu);
void mul_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void mull_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void sdt_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void hwdt_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void mdt_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void msr_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void mrs_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void swp_debug_handle(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void standard_debug_handle_thumb(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void standard_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void stack_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void mdt_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void bx_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void hireg_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void bcond_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void immshort_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void immlong_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void reg_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void immediate_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void b_debug_handle_t(char **thumb_opcode_strings,u32 op,u32 adress,char *dest,IARM7 *cpu);
void rs_debug_handle_t(char **thumb_opcode_strings,u32 op,u32 adress,char *dest,IARM7 *cpu);
void swi_debug_handle_t(char **thumb_opcode_strings,u32 op,u32 adress,char *dest,IARM7 *cpu);
void bl_debug_handle_t(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void bx_debug_handle_arm(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void clz_debug_handle_arm(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void b_debug_handle_arm(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void sc_debug_handle_arm(char **arm_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void ldrd_debug_handle_arm(char **arm_opcode_strings,u32 op,u32 adress,char *dest,IARM7 *cpu);
void qadd_debug_handle_arm(char **arm_opcode_strings,u32 op,u32 adress,char *dest,IARM7 *cpu);
void smlaxy_debug_handle_arm(char **arm_opcode_strings,u32 op,u32 adress,char *dest,IARM7 *cpu);
void smulxy_debug_handle_arm(char **arm_opcode_strings,u32 op,u32 adress,char *dest,IARM7 *cpu);
void smlawy_debug_handle_arm(char **arm_opcode_strings,u32 op,u32 adress,char *dest,IARM7 *cpu);
void setup_sdt_strings_00(char **arm_opcode_strings,u32 base, char *string);
void immediate_debug_handle_t2(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
void immediate_debug_handle_t4(char **thumb_opcode_strings,u32 op, u32 adress, char *dest,IARM7 *cpu);
#endif
