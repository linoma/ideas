#include "lwindow.h"

#ifndef __WIN32__

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/soundcard.h>
#include <poll.h>

#ifdef AUD_ALSA
#include <alsa/asoundlib.h>
#endif


#ifndef __DSOUNDH__
#define __DSOUNDH__


typedef struct _DSCBUFFERDESC
{
    DWORD           dwSize;
    DWORD           dwFlags;
    DWORD           dwBufferBytes;
    DWORD           dwReserved;
    LPWAVEFORMATEX  lpwfxFormat;
} DSBUFFERDESC,DSCBUFFERDESC, *LPDSBUFFERDESC;

class LDirectSoundCaptureBuffer
{
public:
	LDirectSoundCaptureBuffer(LPDSBUFFERDESC pdsbd);
	virtual ~LDirectSoundCaptureBuffer();
	HRESULT Release();
	HRESULT Lock(DWORD dwWrite,DWORD dwLen,LPVOID *mem1,LPDWORD pdwByte1,LPVOID *mem2,LPDWORD pdwByte2,DWORD dwLock);
	HRESULT Unlock(LPVOID mem1,DWORD dwByte1,LPVOID mem2,DWORD dwByte2);
	HRESULT GetCurrentPosition(LPDWORD pdwPlay,LPDWORD pdwWrite);	
	HRESULT Stop();
	HRESULT Start(DWORD dwFlags);
	void OnLoop();
protected:
	static void *oss_loop(void *arg);
	pthread_t buffer_thread;
	DSBUFFERDESC desc;
	WAVEFORMATEX wf;
	BYTE *buffer,*map;
	int going,blockSize,volume,rd_index,wr_index,map_len,wr_ptr,fd;
#ifdef AUD_ALSA
	snd_pcm_t *pcm_handle;	
#endif	
};

class LDirectSoundBuffer
{
public:
	LDirectSoundBuffer(LPDSBUFFERDESC pdsbd);
	virtual ~LDirectSoundBuffer();
	HRESULT Release();
	HRESULT Stop();
	HRESULT Play(DWORD,DWORD,DWORD dwFlags);
	HRESULT GetCurrentPosition(LPDWORD pdwPlay,LPDWORD pdwWrite);
	HRESULT SetCurrentPosition(DWORD dwPlay);
	HRESULT Lock(DWORD dwWrite,DWORD dwLen,LPVOID *mem1,LPDWORD pdwByte1,LPVOID *mem2,LPDWORD pdwByte2,DWORD dwLock);
	HRESULT Unlock(LPVOID mem1,DWORD dwByte1,LPVOID mem2,DWORD dwByte2);
	HRESULT SetVolume(long lVolume);
	void OnLoop();
protected:
#ifdef AUD_ALSA
	HRESULT CreateMMAP(snd_pcm_hw_params_t *hw_params,snd_pcm_sw_params_t *sw_params);	
#endif
	static void *oss_loop(void *arg);
	void get_current_cursor(unsigned long *pplay,unsigned long *pwrite=NULL);
	pthread_t buffer_thread;

#ifdef USE_MUTEX
	pthread_mutex_t buffer_mutex;
#endif
	
	DSBUFFERDESC desc;
	WAVEFORMATEX wf;
	BYTE *buffer;
	int going,blockSize,volume,rd_index,wr_index,fd;
	DWORD dwPlayFlags;
#ifdef AUD_ALSA
	snd_pcm_t *pcm_handle;	
	snd_htimestamp_t start_timestamp;
	snd_pcm_uframes_t mmap_buflen_frames,mmap_pos;
	DWORD mmap_buflen_bytes;
	LPBYTE mmap_buffer;
#endif
};

typedef LDirectSoundBuffer *LPDIRECTSOUNDBUFFER;
typedef void *LPDIRECTSOUNDCAPTURE;
typedef LDirectSoundCaptureBuffer *LPDIRECTSOUNDCAPTUREBUFFER;
typedef void *LPDIRECTSOUND;

#define DSBLOCK_FROMWRITECURSOR     	0x00000001
#define DSBLOCK_ENTIREBUFFER        	0x00000002

#define DSBPLAY_LOOPING             	0x00000001
#define DSBPLAY_LOCHARDWARE         	0x00000002
#define DSBPLAY_LOCSOFTWARE         	0x00000004
#define DSBPLAY_TERMINATEBY_TIME    	0x00000008
#define DSBPLAY_TERMINATEBY_DISTANCE    0x000000010
#define DSBPLAY_TERMINATEBY_PRIORITY    0x000000020

#define DSSCL_NORMAL                	0x00000001
#define DSSCL_PRIORITY              	0x00000002
#define DSSCL_EXCLUSIVE             	0x00000003
#define DSSCL_WRITEPRIMARY          	0x00000004

#define DSBCAPS_PRIMARYBUFFER       	0x00000001
#define DSBCAPS_STATIC              	0x00000002
#define DSBCAPS_LOCHARDWARE         	0x00000004
#define DSBCAPS_LOCSOFTWARE         	0x00000008
#define DSBCAPS_CTRL3D              	0x00000010
#define DSBCAPS_CTRLFREQUENCY       	0x00000020
#define DSBCAPS_CTRLPAN             	0x00000040
#define DSBCAPS_CTRLVOLUME          	0x00000080
#define DSBCAPS_CTRLDEFAULT         	0x000000E0  // Pan + volume + frequency.
#define DSBCAPS_CTRLALL             	0x000000F0  // All control capabilities
#define DSBCAPS_STICKYFOCUS         	0x00004000
#define DSBCAPS_GLOBALFOCUS         	0x00008000
#define DSBCAPS_GETCURRENTPOSITION2 	0x00010000  // More accurate play cursor under emulation

#define DSCBCAPS_WAVEMAPPED         	0x80000000
#define DSCBLOCK_ENTIREBUFFER       	0x00000001
#define DSCBSTATUS_CAPTURING        	0x00000001
#define DSCBSTATUS_LOOPING          	0x00000002
#define DSCBSTART_LOOPING           	0x00000001
#define DSBPN_OFFSETSTOP            	0xFFFFFFFF

#define DS_OK                           S_OK

#define FAILED(a) a != DS_OK

HRESULT CreateSoundBuffer(LPDSBUFFERDESC,LPDIRECTSOUNDBUFFER *,void *);
HRESULT CreateSoundBufferCapture(LPDSBUFFERDESC pdsbd,LPDIRECTSOUNDCAPTUREBUFFER *lpBuffer,void *pUnk);

#define IDirectSoundBuffer_Stop(lpBuffer) ((LPDIRECTSOUNDBUFFER)lpBuffer)->Stop()
#define IDirectSoundBuffer_Play(lpBuffer,dw0,dw1,dwFlags) ((LPDIRECTSOUNDBUFFER)lpBuffer)->Play(dw0,dw1,dwFlags)
#define IDirectSoundBuffer_GetCurrentPosition(lpBuffer,pdwPlay,pdwWrite) ((LPDIRECTSOUNDBUFFER)lpBuffer)->GetCurrentPosition(pdwPlay,pdwWrite)
#define IDirectSoundBuffer_Lock(lpBuffer,dwWrite,dwLen,mem1,pdwByte1,mem2,pdwByte2,dwLock) ((LPDIRECTSOUNDBUFFER)lpBuffer)->Lock(dwWrite,dwLen,mem1,pdwByte1,mem2,pdwByte2,dwLock)
#define IDirectSoundBuffer_Unlock(lpBuffer,mem1,dwByte1,mem2,dwByte2) ((LPDIRECTSOUNDBUFFER)lpBuffer)->Unlock(mem1,dwByte1,mem2,dwByte2)
#define IDirectSoundBuffer_Release(lpBuffer) ((LPDIRECTSOUNDBUFFER)lpBuffer)->Release()
#define IDirectSoundBuffer_SetVolume(lpBuffer,lVolume) ((LPDIRECTSOUNDBUFFER)lpBuffer)->SetVolume(lVolume)
#define IDirectSoundBuffer_SetCurrentPosition(lpBuffer,dwPos) ((LPDIRECTSOUNDBUFFER)lpBuffer)->SetCurrentPosition(dwPos)

#endif

#endif // __WIN32__

