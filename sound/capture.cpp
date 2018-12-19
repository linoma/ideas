#include "capture.h"

#ifdef __BORLANDC__
#pragma link "\\borland\\lib\\psdk\\dsound.lib"
#endif


//---------------------------------------------------------------------------
LSoundCapture::LSoundCapture()
{
#ifdef __WIN32__
   m_dsCap = NULL;
#endif
   m_dsCapBuf = NULL;
   m_dsWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
   m_dsWaveFormat.nChannels = 1;
   m_dsWaveFormat.nSamplesPerSec = 22050;
   m_dsWaveFormat.nBlockAlign = 1;
   m_dsWaveFormat.wBitsPerSample = 8;
   m_dsWaveFormat.nAvgBytesPerSec = m_dsWaveFormat.nSamplesPerSec * m_dsWaveFormat.nBlockAlign;
   m_dsWaveFormat.cbSize = 0;
   bStart = FALSE;
}
//---------------------------------------------------------------------------
LSoundCapture::~LSoundCapture()
{
   Destroy();
}
//---------------------------------------------------------------------------
BOOL LSoundCapture::Init()
{
#ifdef __WIN32__
  	if(::CoCreateInstance(CLSID_DirectSoundCapture,NULL,CLSCTX_INPROC_SERVER,IID_IDirectSoundCapture,(LPVOID *)&m_dsCap) != S_OK)
  		return FALSE;
  	if(m_dsCap->Initialize(NULL) != DS_OK)
  		return FALSE;
	m_dsCapCaps.dwSize = sizeof(m_dsCapCaps);
	if(FAILED(m_dsCap->GetCaps(&m_dsCapCaps)))
		return FALSE;
#endif
	m_dsCapBufDesc.dwSize = sizeof(DSCBUFFERDESC);
	m_dsCapBufDesc.dwFlags = 0;
	m_dsCapBufDesc.dwBufferBytes = m_dsWaveFormat.nAvgBytesPerSec * 4;
	m_dsCapBufDesc.dwReserved = 0;
	m_dsCapBufDesc.lpwfxFormat = &m_dsWaveFormat;
#ifdef __WIN32__
 	if(FAILED(m_dsCap->CreateCaptureBuffer((LPDSCBUFFERDESC)&m_dsCapBufDesc,&m_dsCapBuf,NULL)))
		return FALSE;
#else
	if(CreateSoundBufferCapture(&m_dsCapBufDesc,&m_dsCapBuf,NULL) != DS_OK)
		return FALSE;
#endif  
	return TRUE;
}
//---------------------------------------------------------------------------
void LSoundCapture::Destroy()
{
   Stop();
   if(m_dsCapBuf != NULL)
	    m_dsCapBuf->Release();
   m_dsCapBuf = NULL;
#ifdef __WIN32__
   if(m_dsCap != NULL)
	    m_dsCap->Release();
   m_dsCap = NULL;
#endif
}
//---------------------------------------------------------------------------
BOOL LSoundCapture::Start()
{
   if(m_dsCapBuf == NULL)
       return FALSE;
   if(FAILED(m_dsCapBuf->Start(DSCBSTART_LOOPING)))
       return FALSE;
   dwCaptureOffset = 0;
   return (bStart = TRUE);
}
//---------------------------------------------------------------------------
BOOL LSoundCapture::Stop()
{
   if(m_dsCapBuf == NULL)
       return FALSE;
   if(FAILED(m_dsCapBuf->Stop()))
       return FALSE;
   bStart = FALSE;
   return TRUE;
}
//---------------------------------------------------------------------------
u16 LSoundCapture::get_Sample()
{
   u8 *pbCaptureData,*pbCaptureData2;
   DWORD dwCaptureLength2,dwCaptureLength;
   u16 sample;

   if(m_dsCapBuf == NULL || !bStart)
       return 0;
   if(FAILED(m_dsCapBuf->Lock(dwCaptureOffset,1,(LPVOID *)&pbCaptureData,&dwCaptureLength,(LPVOID *)&pbCaptureData2,&dwCaptureLength2,0L)))
       return 0;
   sample = (u16)pbCaptureData[0];
   m_dsCapBuf->Unlock(pbCaptureData,dwCaptureLength,pbCaptureData2,dwCaptureLength2);  
	dwCaptureOffset = (dwCaptureOffset + 1) % m_dsCapBufDesc.dwBufferBytes;
   return sample;
}
