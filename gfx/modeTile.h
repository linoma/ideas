#include "ideastypes.h"
#include "gpu.h"

//---------------------------------------------------------------------------
#ifndef __modeTileH__
#define __modeTileH__
//---------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

void I_FASTCALL PostDrawMode1(LPLAYER layer);
void I_FASTCALL InitLayerMode1(LPLAYER layer);
u32 I_FASTCALL drawPixelMode1(LPLAYER layer);
void I_FASTCALL InitLayerMode0(LPLAYER layer);
u32 I_FASTCALL drawPixelMode0(LPLAYER layer);
void drawLineModeTile(void);
void drawLineModeTileWindow(void);
void I_FASTCALL InitLayerMode3D(LPLAYER layer);
u32 I_FASTCALL drawPixelMode3D(LPLAYER layer);
void I_FASTCALL PostDrawLineMode1(LPLAYER layer);
void I_FASTCALL InitLayerMode4(LPLAYER layer);
u32 I_FASTCALL drawPixelMode4(LPLAYER layer);

#ifdef __cplusplus
}
#endif

#endif

