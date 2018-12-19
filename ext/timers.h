#include "ideastypes.h"
#include "dsmem.h"

#ifndef __TIMERSH__

void ResetTimer();
void RenderTimer(u16 cyc);
void InitTimersTable(LPIFUNC *p,LPIFUNC *p1,LPOFUNC *p2,LPOFUNC *p3);
BOOL TIMER_Save(LStream *pFile);
BOOL TIMER_Load(LStream *pFile,int ver);
#endif
