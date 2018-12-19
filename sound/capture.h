#include "ideastypes.h"
#include "dstype.h"

#ifdef __WIN32__
#include <\borland\include\dsound.h>
#else
#include "dsound.h"
#endif
//---------------------------------------------------------------------------
#ifndef captureH
#define captureH
//---------------------------------------------------------------------------
class LSoundCapture
{
public:
   LSoundCapture();
   ~LSoundCapture();
   BOOL Init();
   void Destroy();
   BOOL Start();
   BOOL Stop();
   u16 get_Sample();
protected:
#ifdef __WIN32__
   LPDIRECTSOUNDCAPTURE m_dsCap;
   DSCBCAPS m_dsCapBufCaps;
	DSCCAPS m_dsCapCaps;
#endif
   DSBUFFERDESC m_dsCapBufDesc;
   LPDIRECTSOUNDCAPTUREBUFFER m_dsCapBuf;  

   WAVEFORMATEX m_dsWaveFormat;
   DWORD dwCaptureOffset;
   BOOL bStart;
};
#endif








