#include "dstype.h"

#ifndef __DMAH__
#define __DMAH__

void ResetDMA();
void StartDMA(u8 type);
void InitDmaTable(LPIFUNC *p,LPIFUNC *p1,LPOFUNC *p2,LPOFUNC *p3);
BOOL DMA_Save(LStream *pFile);
BOOL DMA_Load(LStream *pFile,int ver);

#endif
