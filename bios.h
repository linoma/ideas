#include "ideastypes.h"

#ifndef BIOS_H
#define BIOS_H
void LZ77UnComp(u32 src,u32 dst,IARM7 *cpu);
void CpuSet(u32 dst,u32 src,u32 flags,IARM7 *cpu);
void RLUnComp(u32 dst,u32 src,IARM7 *cpu);
void HuffUnComp(u32 dst,u32 src,IARM7 *cpu);
u32 SquareRoot(u32 value);
u16 GetPitchTable(u32 value);
u16 GetSineTable(u32 value);
u8 GetVolumeTable(u32 value);
void CpuFastSet(u32 dst,u32 src,u32 flags,IARM7 *cpu);
void BiosBreakPoint(BOOL bArm7);
BOOL WriteConsoleMessage(u32 adr_mes,IARM7 *cpu);
void ObjAffineSet(IARM7 *cpu);
void BgAffineSet(IARM7 *cpu);
void RegisterRamReset(IARM7 *cpu);

#endif
