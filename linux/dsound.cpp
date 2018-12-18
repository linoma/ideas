#include "ideastypes.h"
#include "dsound.h"
#include <math.h>

#ifndef __WIN32__

#define min(a,b) (a < b ? a : b)

#ifdef _DEBUG

#pragma option push -a
#pragma option -a1
typedef struct
{
  BYTE  riff[4];
  DWORD len;
  BYTE  cWavFmt[8];
  DWORD dwHdrLen;
  WORD  wFormat;
  WORD  wNumChannels;
  DWORD dwSampleRate;
  DWORD dwBytesPerSec;
  WORD  wBlockAlign;
  WORD  wBitsPerSample;
  BYTE  cData[4];
  DWORD dwDataLen;
} WAVHDR, *PWAVHDR, *LPWAVHDR;
#pragma option pop

static void writeWavHeader(FILE *fp,DWORD len)
{
   WAVHDR wav;

   if(fp == NULL)
       return;
   CopyMemory(wav.riff,"RIFF",4);
   wav.len = len + 44 - 8;
   CopyMemory(wav.cWavFmt,"WAVEfmt ",8);
   wav.dwHdrLen        = 16;
   wav.wFormat         = WAVE_FORMAT_PCM;
   wav.wNumChannels    = 2;
   wav.dwSampleRate    = 22050;
   wav.dwBytesPerSec   = 44100;
   wav.wBlockAlign     = 2;
   wav.wBitsPerSample  = 8;
   CopyMemory(wav.cData,"data", 4);
   wav.dwDataLen = len;
   fseek(fp,0,SEEK_SET);
   fwrite(&wav,sizeof(wav),1,fp);
}
#endif


#ifdef AUD_ALSA
static int get_trigger_tstamp(snd_pcm_t *pcm_handle,snd_htimestamp_t *ts)
{
	int err;
	snd_pcm_status_t *status;	
	snd_pcm_uframes_t avail;

	if(pcm_handle == NULL || ts == NULL)
		return -1;
	snd_pcm_avail_update(pcm_handle);
	err = snd_pcm_htimestamp(pcm_handle,&avail,ts);
	return err;
}
//---------------------------------------------------------------------------
static long long timediff(snd_htimestamp_t t1,snd_htimestamp_t t2)
{
	signed long long l;
 
	t1.tv_sec -= t2.tv_sec;
	l = (signed long long)t1.tv_nsec - (signed long long)t2.tv_nsec;
	if (l < 0) {
		t1.tv_sec--;
		l = -l;
		l %= 1000000000;
	}
	return (t1.tv_sec * 1000000000) + l;
}
#endif
//---------------------------------------------------------------------------
HRESULT CreateSoundBufferCapture(LPDSBUFFERDESC pdsbd,LPDIRECTSOUNDCAPTUREBUFFER *lpBuffer,void *pUnk)
{
	LDirectSoundCaptureBuffer *pBuffer;
	
	if(pdsbd == NULL || lpBuffer == NULL || pdsbd->lpwfxFormat == NULL || pdsbd->dwSize != sizeof(DSBUFFERDESC))
		return (HRESULT)-1;
	if(pdsbd->lpwfxFormat->wFormatTag != WAVE_FORMAT_PCM)
		return (HRESULT)-1;
	pBuffer = new LDirectSoundCaptureBuffer(pdsbd);
	if(pBuffer == NULL)
		return E_FAIL;	
	*lpBuffer = pBuffer;
	return DS_OK;
}
//---------------------------------------------------------------------------
HRESULT CreateSoundBuffer(LPDSBUFFERDESC pdsbd,LPDIRECTSOUNDBUFFER *lpBuffer,void *pUnk)
{
	LDirectSoundBuffer *pBuffer;

	if(pdsbd == NULL || lpBuffer == NULL || pdsbd->lpwfxFormat == NULL || pdsbd->dwSize != sizeof(DSBUFFERDESC))
		return (HRESULT)-1;
	if(pdsbd->lpwfxFormat->wFormatTag != WAVE_FORMAT_PCM)
		return (HRESULT)-1;
	if(pdsbd->lpwfxFormat->nChannels < 1 || pdsbd->lpwfxFormat->nChannels > 2 || (pdsbd->lpwfxFormat->wBitsPerSample != 8 && pdsbd->lpwfxFormat->wBitsPerSample != 16))
		return (HRESULT)-1;
	pBuffer = new LDirectSoundBuffer(pdsbd);
	if(pBuffer == NULL)
		return E_FAIL;
	*lpBuffer = pBuffer;
	return DS_OK;
}
//---------------------------------------------------------------------------
LDirectSoundCaptureBuffer::LDirectSoundCaptureBuffer(LPDSBUFFERDESC pdsbd)
{
	memcpy(&desc,pdsbd,pdsbd->dwSize);
	memcpy(&wf,pdsbd->lpwfxFormat,sizeof(wf));
	buffer = NULL;
	buffer_thread = 0;
#ifdef AUD_ALSA
	pcm_handle = NULL;
#else
	fd = -1;
	map = (BYTE *)-1;	
#endif		
}
//---------------------------------------------------------------------------
LDirectSoundCaptureBuffer::~LDirectSoundCaptureBuffer()
{
	Release();
}
//---------------------------------------------------------------------------
HRESULT LDirectSoundCaptureBuffer::Release()
{
	void *res;

	going = 0;
	if(buffer_thread != 0)
		pthread_join(buffer_thread,&res);
	buffer_thread = 0;
#ifdef AUD_ALSA	
	if(pcm_handle != NULL){
		snd_pcm_close(pcm_handle);
		pcm_handle = NULL;
	}
#else
	if(fd != -1)
		close(fd);
	fd = -1;
	if(map != (BYTE *)-1)
		munmap(map,map_len);
	map = (BYTE *)-1;
#endif	
	if(buffer != NULL)
		LocalFree(buffer);
	buffer = NULL;
	return DS_OK;
}
//---------------------------------------------------------------------------
HRESULT LDirectSoundCaptureBuffer::Lock(DWORD dwWrite,DWORD dwLen,LPVOID *mem1,LPDWORD pdwByte1,LPVOID *mem2,LPDWORD pdwByte2,DWORD dwLock)
{
	DWORD dw;

	if(buffer == NULL){
		if(mem1 != NULL)
			*mem1 = NULL;
		if(pdwByte1 != NULL)
			*pdwByte1 = 0;
		if(mem2 != NULL)
			*mem2 = NULL;
		if(pdwByte2 != NULL)
			*pdwByte2 = 0;		
		return E_FAIL;
	}
	if(dwWrite > desc.dwBufferBytes)
		return E_FAIL;
	if(dwLock == DSBLOCK_ENTIREBUFFER)
		dwLen = desc.dwBufferBytes;
	*mem1 = &buffer[dwWrite];
	dw = desc.dwBufferBytes - dwWrite;
	if(dw > dwLen)
		dw = dwLen;
	*pdwByte1 = dw;
	dwLen -= dw;
	if(dwLen != 0){
		dwLen = desc.dwBufferBytes - dw;
		*mem2 = buffer;
		*pdwByte2 = dwLen;
	}
	else{
		*mem2 = NULL;
		*pdwByte2 = 0;
	}		
	return DS_OK;
}
//---------------------------------------------------------------------------
HRESULT LDirectSoundCaptureBuffer::Unlock(LPVOID mem1,DWORD dwByte1,LPVOID mem2,DWORD dwByte2)
{
	return DS_OK;
}
//---------------------------------------------------------------------------
HRESULT LDirectSoundCaptureBuffer::GetCurrentPosition(LPDWORD pdwPlay,LPDWORD pdwWrite)
{
}
//---------------------------------------------------------------------------
HRESULT LDirectSoundCaptureBuffer::Stop()
{
	going = 0;
	if(buffer_thread != 0)
		pthread_join(buffer_thread,0);
	buffer_thread = 0;
	return DS_OK;
}
//---------------------------------------------------------------------------
HRESULT LDirectSoundCaptureBuffer::Start(DWORD dwFlags)
{
	HRESULT res;
#ifdef AUD_ALSA	
	int rtn,dir;
	unsigned int val;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_uframes_t frames;
	struct pollfd pfd;
	
	if(pcm_handle != NULL)
		goto ex_Start;	
	rtn = snd_pcm_open(&pcm_handle,"default",SND_PCM_STREAM_CAPTURE,SND_PCM_NONBLOCK);
	if(rtn < 0)
		return E_FAIL;
	hw_params = NULL;
	res = E_FAIL;
	rtn = snd_pcm_hw_params_malloc(&hw_params);
	if(rtn < 0)
		goto ex_Start_Error;
	rtn = snd_pcm_hw_params_any(pcm_handle,hw_params);
	if(rtn < 0)
		goto ex_Start_Error;
	rtn = snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if(rtn < 0)
		goto ex_Start_Error;
	rtn = snd_pcm_hw_params_set_format(pcm_handle, hw_params,SND_PCM_FORMAT_U8);	
	if(rtn < 0)
		goto ex_Start_Error;
	val = 22050;
	dir = 0;
	rtn = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &val,&dir);	
	if(rtn < 0)
		goto ex_Start_Error;
	rtn = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, 1);
	if(rtn < 0)
		goto ex_Start_Error;
	rtn = snd_pcm_hw_params(pcm_handle, hw_params);
	if(rtn < 0)
		goto ex_Start_Error;
	rtn = snd_pcm_hw_params_get_period_size(hw_params,&frames,0);
	if(rtn < 0)
		goto ex_Start_Error;	
	blockSize = frames;	
	rtn = snd_pcm_prepare(pcm_handle);
	if(rtn < 0)
		goto ex_Start_Error;	
	rtn = snd_pcm_poll_descriptors_count(pcm_handle);
	if(rtn < 1)
		goto ex_Start_Error;	
	rtn = snd_pcm_poll_descriptors(pcm_handle,&pfd,1);
	if(rtn < 0)
		goto ex_Start_Error;	
	fd = pfd.fd;	
	res = DS_OK;	
#else
	DWORD dw,mask;	
	int shift,fsize,new_shift,min_shift,min_fsize,audio_fragment;
	BOOL found_one;
	audio_buf_info info;

	if(fd > 0)
		goto ex_Start;
	if((fd = open("/dev/dsp", O_RDONLY|O_NONBLOCK)) < 0)
		return E_FAIL;
	dw = desc.lpwfxFormat->nChannels == 1 ? 0 : 1;
	if(ioctl(fd,SNDCTL_DSP_CHANNELS, &dw) < 0){
		res = E_FAIL;
		goto ex_Start_Error;
	}
	dw = desc.lpwfxFormat->nSamplesPerSec;
	if(ioctl(fd, SNDCTL_DSP_SPEED,&dw) < 0 || dw != desc.lpwfxFormat->nSamplesPerSec){
		res = E_FAIL;
		goto ex_Start_Error;
	}
	mask = dw = desc.lpwfxFormat->wBitsPerSample == 8 ? AFMT_U8 : AFMT_S16_LE;
	if(ioctl(fd,SNDCTL_DSP_SETFMT,&dw) < 0 || mask != dw) {
		res = E_FAIL;
		goto ex_Start_Error;
	}
	mask = 0xffffffff;
    fsize = desc.lpwfxFormat->nAvgBytesPerSec / 100;        /* 10 ms chunk */
    shift = 0;
    while ((1 << shift) <= fsize)
    	shift++;
   	shift--;
    fsize = 1 << shift;
    mask = (mask >> (32 - shift));
    if(desc.dwBufferBytes & mask) {
    	new_shift = shift - 1;
        min_shift = 0;
        min_fsize = desc.lpwfxFormat->nAvgBytesPerSec / 1000;
        found_one = FALSE;
        while((1 << min_shift) <= min_fsize)
        	min_shift++;
        min_shift--;
        while(new_shift > min_shift){
        	if (desc.dwBufferBytes & (-1 >> (32 - new_shift))){
            	new_shift--;
                continue;
            } 
			else {
               	found_one = TRUE;
                break;
            }
    	}
        if (found_one)
        	audio_fragment = ((desc.dwBufferBytes >> new_shift) << 16) | new_shift;               
        else
           	audio_fragment = 0x00100000 + shift;
	}
  	else
    	audio_fragment = ((desc.dwBufferBytes >> shift) << 16) | shift;
	if(ioctl(fd, SNDCTL_DSP_SETFRAGMENT,&audio_fragment) != 0){
		res = E_FAIL;
		goto ex_Start_Error;
	}
	dw = desc.lpwfxFormat->wBitsPerSample;
	if(ioctl(fd, SNDCTL_DSP_SAMPLESIZE, &dw) < 0 || dw != desc.lpwfxFormat->wBitsPerSample){
		res = E_FAIL;
		goto ex_Start_Error;
	}
	if (ioctl(fd, SNDCTL_DSP_GETISPACE, &info) < 0) {
		res = E_FAIL;
		goto ex_Start_Error;
	}
	blockSize = info.fragsize;
	map_len = info.fragsize*info.fragstotal;
	map = (BYTE *)mmap(NULL,map_len,PROT_READ,MAP_SHARED,fd,0);
	if(map == (BYTE *)-1){
		res = E_FAIL;
		goto ex_Start_Error;
	}
#endif	
	if(buffer == NULL){
		buffer = (BYTE *)LocalAlloc(LPTR,desc.dwBufferBytes);
		if(buffer == NULL){
			res = E_FAIL;
			goto ex_Start_Error;
		}
	}
ex_Start:
	going = true;
	rd_index = wr_index = wr_ptr = 0;
	if(buffer_thread == 0){
		pthread_create(&buffer_thread,NULL,oss_loop,(void *)this);
		if(buffer_thread == 0){
			res = E_FAIL;
			goto ex_Start_Error;
		}		
	}
	res = DS_OK;
ex_Start_Error:
#ifdef AUD_ALSA
	if(hw_params != NULL)
		snd_pcm_hw_params_free(hw_params);
#endif
	return res;
}
//---------------------------------------------------------------------------
void *LDirectSoundCaptureBuffer::oss_loop(void *arg)
{
	if(arg != NULL)
		((LDirectSoundCaptureBuffer *)arg)->OnLoop();
	pthread_exit(NULL);		
}
//---------------------------------------------------------------------------
void LDirectSoundCaptureBuffer::OnLoop()
{
#ifdef AUD_ALSA
	int cnt,len;
	
	while(going){
		len = blockSize;
		if(wr_ptr+len >= desc.dwBufferBytes);
			len = desc.dwBufferBytes - wr_ptr;
		cnt = snd_pcm_readi(pcm_handle,buffer+wr_ptr,len);
		if(cnt == -EPIPE)
			snd_pcm_prepare(pcm_handle);
		else if(cnt < 0)
			continue;			
//            	wr_index = info.ptr;
          	wr_ptr = (wr_ptr + cnt) % desc.dwBufferBytes;
	}
#else
	struct pollfd poll_list[2]={0};
	int cnt,map_offset,offset,fragsize,first,second;
	count_info info;

	poll_list[0].fd = fd;
	poll_list[0].events = POLLIN;
	while(going){
		cnt = poll(poll_list,(unsigned long)1,10);
		if(cnt < 0)
            continue;
		if((poll_list[0].revents & POLLIN) == POLLIN) {
            if(ioctl(fd,SNDCTL_DSP_GETIPTR,&info) < 0)
                continue;
          	map_offset = wr_index;
            offset = wr_ptr;
            if(info.ptr < map_offset) {
            	fragsize = info.ptr + map_len - map_offset;
            	if((offset + fragsize) >desc.dwBufferBytes) {
                	if((map_len - map_offset) > (desc.dwBufferBytes - offset)){
                    	first = desc.dwBufferBytes - offset;
                        second = (map_len - map_offset) - first;
                        memcpy(buffer + offset, map + map_offset, first);
                        memcpy(buffer, map + map_offset + first, second);
                        memcpy(buffer + second, map, fragsize - (first + second));
                   	} 
					else {
                        first = map_len - map_offset;
                        second = (desc.dwBufferBytes - offset) - first;
                        memcpy(buffer + offset, map + map_offset, first);
                        memcpy(buffer + offset + first, map, second);
                        memcpy(buffer, map + second, fragsize - (first + second));
                    }
	            } 
				else{
                	first = map_len - map_offset;
                    memcpy(buffer + offset, map + map_offset, first);
                    memcpy(buffer + offset + first, map, fragsize - first);
                }
     		} 
			else{
            	fragsize = info.ptr - map_offset;
                if((offset + fragsize) > desc.dwBufferBytes){
                	first = desc.dwBufferBytes - offset;
                    memcpy(buffer + offset, map + map_offset, first);
                    memcpy(buffer, map + map_offset + first, fragsize - first);
                } 
				else
                	memcpy(buffer + offset, map + map_offset, fragsize);
         	}
            wr_index = info.ptr;
           	wr_ptr = (wr_ptr + fragsize) % desc.dwBufferBytes;
		}
	}
#endif	
}
//---------------------------------------------------------------------------
LDirectSoundBuffer::LDirectSoundBuffer(LPDSBUFFERDESC pdsbd)
{
	memcpy(&desc,pdsbd,pdsbd->dwSize);
	memcpy(&wf,pdsbd->lpwfxFormat,sizeof(wf));
	buffer = NULL;	
	buffer_thread = 0;
#ifdef USE_MUTEX
	buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
#ifdef AUD_ALSA
	pcm_handle = NULL;
#endif
	fd = -1;	
	volume = 0;
}
//---------------------------------------------------------------------------
LDirectSoundBuffer::~LDirectSoundBuffer()
{
	Release();
}
//---------------------------------------------------------------------------
HRESULT LDirectSoundBuffer::Release()
{	
	void *res;

#ifdef USE_MUTEX
	if(buffer_mutex != PTHREAD_MUTEX_INITIALIZER)
		pthread_mutex_lock(&buffer_mutex);
#endif
		
	going = 0;
	if(buffer_thread != 0)
		pthread_join(buffer_thread,&res);
	buffer_thread = 0;

#ifdef USE_MUTEX
	if(buffer_mutex != PTHREAD_MUTEX_INITIALIZER){
		pthread_mutex_destroy(&buffer_mutex);
		buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
	}
#endif

#ifdef AUD_ALSA
 	if(pcm_handle != NULL){
 		snd_pcm_drain(pcm_handle);
    	snd_pcm_close(pcm_handle);
   		pcm_handle = NULL;
   	}
#else
	if(fd != -1)
		close(fd);
	fd = -1;
#endif
	if(buffer != NULL)
		LocalFree(buffer);
	buffer = NULL;
	return DS_OK;
}
//---------------------------------------------------------------------------
HRESULT LDirectSoundBuffer::Stop()
{
	going = 0;
	if(buffer_thread != 0)
		pthread_join(buffer_thread,0);
	buffer_thread = 0;
	return DS_OK;
}
//---------------------------------------------------------------------------
HRESULT LDirectSoundBuffer::Play(DWORD dwReserved1,DWORD dwPriority,DWORD dwFlags)
{
	HRESULT res;
#ifndef AUD_ALSA	
	DWORD dw,frag_spec;
		
	if(fd > 0)
		goto ex_Load;
	if((fd = open("/dev/dsp", O_WRONLY|O_NONBLOCK)) < 0)
		return E_FAIL;
	/*if(ioctl(fd, SNDCTL_DSP_SETDUPLEX, 0)){
		res = E_FAIL;
		goto ex_Load_Error;
	}*/
	dw = desc.lpwfxFormat->nChannels == 1 ? 0 : 1;
	if(ioctl(fd,SNDCTL_DSP_STEREO, &dw) < 0){
		res = E_FAIL;
		goto ex_Load_Error;
	}
	dw = desc.lpwfxFormat->nSamplesPerSec;
	if(ioctl(fd, SNDCTL_DSP_SPEED,&dw) < 0 || dw != desc.lpwfxFormat->nSamplesPerSec){
		res = E_FAIL;
		goto ex_Load_Error;
	}
	frag_spec = dw = desc.lpwfxFormat->wBitsPerSample == 8 ? AFMT_U8 : AFMT_S16_LE;
	if(ioctl(fd,SNDCTL_DSP_SETFMT,&dw) < 0 || frag_spec != dw) {
		res = E_FAIL;
		goto ex_Load_Error;
	}
	dw = ((dw&0xFF)/8) * desc.lpwfxFormat->nChannels * 256;
	for(frag_spec = 0;(1<<frag_spec) < dw;++frag_spec);
	if((1<<frag_spec) != dw) {
		res = E_FAIL;
		goto ex_Load_Error;
	}
	frag_spec |= 0x00020000;
	//dw = 0x0020000B;
	if(ioctl(fd, SNDCTL_DSP_SETFRAGMENT,&frag_spec) != 0){
		res = E_FAIL;
		goto ex_Load_Error;
	}

	printf("frag_spec %d\r\n",frag_spec);

	dw = desc.lpwfxFormat->wBitsPerSample;
	if(ioctl(fd, SNDCTL_DSP_SAMPLESIZE, &dw) < 0 || dw != desc.lpwfxFormat->wBitsPerSample){
		res = E_FAIL;
		goto ex_Load_Error;
	}
	if(ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &blockSize) < 0){
		res = E_FAIL;
		goto ex_Load_Error;
	}

	printf("block_size %d\r\n",blockSize);
#else
	int rtn,dir;
	unsigned int val;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;
	snd_pcm_uframes_t frames;

	hw_params = NULL;
	sw_params = NULL;
	if(pcm_handle != NULL)
		goto ex_Load;	
	rtn = snd_pcm_open(&pcm_handle,"default",SND_PCM_STREAM_PLAYBACK,SND_PCM_NONBLOCK);
	if(rtn < 0)
		return E_FAIL;
	res = E_FAIL;	
 	rtn = snd_pcm_hw_params_malloc(&hw_params);
	if(rtn < 0)
		goto ex_Load_Error;  	
	rtn = snd_pcm_hw_params_any(pcm_handle,hw_params);
	if(rtn < 0)
		goto ex_Load_Error;	
  	rtn = snd_pcm_hw_params_set_access(pcm_handle, hw_params,SND_PCM_ACCESS_RW_INTERLEAVED);
	if(rtn < 0)
		goto ex_Load_Error;
	rtn = snd_pcm_hw_params_set_format(pcm_handle, hw_params,desc.lpwfxFormat->wBitsPerSample == 8 ? SND_PCM_FORMAT_U8 : SND_PCM_FORMAT_S16_LE);
	if(rtn < 0)
		goto ex_Load_Error;
	rtn = snd_pcm_hw_params_set_channels(pcm_handle, hw_params,desc.lpwfxFormat->nChannels);
	if(rtn < 0)
		goto ex_Load_Error;
  	val = desc.lpwfxFormat->nSamplesPerSec;
  	rtn = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params,&val, &dir);
	if(rtn < 0)
		goto ex_Load_Error;
	frames = 512;
	rtn = snd_pcm_hw_params_set_period_size_near(pcm_handle,hw_params,&frames,NULL);
	if(rtn < 0)
		goto ex_Load_Error;
	val = 2;
	rtn = snd_pcm_hw_params_set_periods_near(pcm_handle,hw_params,&val,NULL);
	if(rtn < 0)
		goto ex_Load_Error;
  	rtn = snd_pcm_hw_params(pcm_handle, hw_params);
	if(rtn < 0)
		goto ex_Load_Error;

	snd_pcm_sw_params_alloca(&sw_params);
	rtn = snd_pcm_sw_params_current(pcm_handle, sw_params);
	if(rtn < 0)
		goto ex_Load_Error;	
	rtn = snd_pcm_sw_params_set_start_threshold(pcm_handle, sw_params, 1024);
	if(rtn < 0)
		goto ex_Load_Error;	
	rtn = snd_pcm_sw_params_set_avail_min(pcm_handle, sw_params, frames);
	if(rtn < 0)
		goto ex_Load_Error;	
	rtn = snd_pcm_sw_params(pcm_handle, sw_params);
	if(rtn < 0)
		goto ex_Load_Error;	
	rtn = snd_pcm_hw_params_get_period_size(hw_params,&frames,0);
	if(rtn < 0)
		goto ex_Load_Error;	
	blockSize = frames;
	mmap_buflen_bytes = blockSize;
	rtn =  snd_pcm_prepare(pcm_handle);
	if(rtn < 0)
		goto ex_Load_Error;
	snd_pcm_hw_params_free(hw_params);
   	res = DS_OK;    
#endif
	
	if(buffer == NULL){
		buffer = (BYTE *)LocalAlloc(LPTR,desc.dwBufferBytes);
		if(buffer == NULL){
			res = E_FAIL;
			goto ex_Load_Error;
		}
	}
ex_Load:
	going = true;
	dwPlayFlags = dwFlags;
	rd_index = wr_index = 0;
#ifdef USE_MUTEX
	if(pthread_mutex_init(&buffer_mutex,NULL) != 0){
		res = E_FAIL;
		goto ex_Load_Error;
	}	
#endif	

#ifdef AUD_ALSA
	get_trigger_tstamp(pcm_handle,&start_timestamp);
#endif
	if(buffer_thread == 0){
		pthread_create(&buffer_thread,NULL,oss_loop,(void *)this);
		if(buffer_thread == 0){
			res = E_FAIL;
			goto ex_Load_Error;
		}
	}
	return DS_OK;
ex_Load_Error:
#ifdef AUD_ALSA
	if(hw_params != NULL)
		snd_pcm_hw_params_free(hw_params);
#endif		
	Release();
	return res;
}
//---------------------------------------------------------------------------
HRESULT LDirectSoundBuffer::Lock(DWORD dwWrite,DWORD dwLen,LPVOID *mem1,LPDWORD pdwByte1,LPVOID *mem2,LPDWORD pdwByte2,DWORD dwLock)
{
	DWORD dw;

	if(buffer == NULL){
		buffer = (BYTE *)LocalAlloc(LPTR,desc.dwBufferBytes);
		if(buffer == NULL)
			return E_FAIL;
	}	
	if(dwLock & DSBLOCK_ENTIREBUFFER)
		dwLen = desc.dwBufferBytes;
#ifdef USE_MUTEX
	pthread_mutex_lock(&buffer_mutex);
#endif	
	if(dwLock & DSBLOCK_FROMWRITECURSOR){
		dwWrite = wr_index;
#ifdef _DEBUG
		printf("DSBLOCK_FROMWRITECURSOR\n");
#endif
	}
	dwWrite %= desc.dwBufferBytes;
	*mem1 = &buffer[dwWrite];
	dw = desc.dwBufferBytes - dwWrite;
	if(dw > dwLen)
		dw = dwLen;
	*pdwByte1 = dw;
	dwLen -= dw;
	if(dwLen != 0){
		*mem2 = buffer;
		*pdwByte2 = dwLen;
	}
	else{
		*mem2 = NULL;
		*pdwByte2 = 0;
	}	
	return DS_OK;
}
//---------------------------------------------------------------------------
HRESULT LDirectSoundBuffer::Unlock(LPVOID mem1,DWORD dwByte1,LPVOID mem2,DWORD dwByte2)
{
	HRESULT res;
#ifdef EXPERIMENT
	DWORD dw;
	int sample;
	
	res = E_FAIL;
	if(mem1 == NULL && mem2 == NULL)
		goto UnLock_ex;
	res = DS_OK;
	if(volume == 0)		
		goto UnLock_ex;
	switch(desc.lpwfxFormat->wBitsPerSample){
		case 8:
			for(dw = 0;dw<dwByte1;dw++){
				sample = ((signed char *)mem1)[dw];
				sample -= sample * volume >> 7;
				((signed char *)mem1)[dw] = (signed char)sample;
			}
			for(dw = 0;dw<dwByte2;dw++){
				sample = ((signed char *)mem2)[dw];
				sample -= sample * volume >> 7;
				((signed char *)mem2)[dw] = (signed char)sample;
			}
		break;
		case 16:
			for(dw = 0;dw<dwByte1;dw+=2){
				sample = ((signed short *)mem1)[dw];
				sample -= sample * volume >> 7;
				((signed short *)mem1)[dw] = (signed short)sample;
			}
			for(dw = 0;dw<dwByte2;dw+=2){
				sample = ((signed short *)mem2)[dw];
				sample -= sample * volume >> 7;
				((signed short *)mem2)[dw] = (signed short)sample;
			}			
		break;
		default:
			res = E_FAIL;
		break;
	}
#else
	res = DS_OK;
#endif
UnLock_ex:	
#ifdef USE_MUTEX
	pthread_mutex_unlock(&buffer_mutex);
#endif	
	return res;
}
//---------------------------------------------------------------------------
HRESULT LDirectSoundBuffer::SetVolume(long lVolume)
{
	float vol;
	
	if(lVolume > 0)
		return E_FAIL;
	lVolume = abs(lVolume);
	if(lVolume < 1000)
		volume = 0;
	else
		volume = 128.0f * (log10(lVolume / 1000.0f));
	return DS_OK;
}
//---------------------------------------------------------------------------
void LDirectSoundBuffer::get_current_cursor(unsigned long *pplay,unsigned long *pwrite)
{
#ifdef AUD_ALSA
	unsigned long ptr;
	
	ptr = 0;	
	ptr = (rd_index + ptr) % desc.dwBufferBytes;	
	ptr &= ~3;
	if(pplay != NULL)
		*pplay = ptr;
	if(pwrite != NULL)
		*pwrite = (ptr + (wf.nAvgBytesPerSec * 24 / 1000)) % desc.dwBufferBytes;
#endif
}
//---------------------------------------------------------------------------
HRESULT LDirectSoundBuffer::GetCurrentPosition(LPDWORD pdwPlay,LPDWORD pdwWrite)
{
	unsigned long read,write;

#ifdef AUD_ALSA
	if(pcm_handle == NULL)
		return E_FAIL;
	get_current_cursor(&read,&write);
    if(pdwPlay)
    	*pdwPlay = read;	
    if(pdwWrite)
	    *pdwWrite = write;
#else
    count_info info;

    if(fd < 0 || ioctl(fd,SNDCTL_DSP_GETOPTR,&info) < 0)
		return E_FAIL;
    ptr = info.ptr & ~3;
	ptr = (wr_index + ptr) % desc.dwBufferBytes;	
    if(pdwPlay)
    	*pdwPlay = ptr;	
    if(pdwWrite)
	    *pdwWrite = (ptr + (wf.nAvgBytesPerSec * 32 / 1000)) % desc.dwBufferBytes;
#endif	

#ifdef _DEBUG
//	printf("pos : %d %d %d %d\n",ptr,(rd_index + (wf.nAvgBytesPerSec * 16 / 1000)) % desc.dwBufferBytes,rd_index,wr_index);
#endif
	return DS_OK;
}
//---------------------------------------------------------------------------
HRESULT LDirectSoundBuffer::SetCurrentPosition(DWORD dwPlay)
{
	if(dwPlay > desc.dwBufferBytes)
		dwPlay %= desc.dwBufferBytes;
	rd_index = dwPlay;
#ifdef _DEBUG
	printf("SetCurrentPosition %d\n",rd_index);
#endif
	return DS_OK;
}
//---------------------------------------------------------------------------
void LDirectSoundBuffer::OnLoop()
{
	int length, cnt,wr,sleepTime,first;
	
	length = min(blockSize,desc.dwBufferBytes);
	sleepTime = length * 970000 / wf.nAvgBytesPerSec;
#ifdef _DEBUG
	printf("start : %d %d\n",length,sleepTime);	
#endif
	first = 0;
	while(going){
		cnt = length;
		wr_index = rd_index;
#ifdef AUD_ALSA				
		wr = snd_pcm_avail_update(pcm_handle);
		if(wr < 0){
			snd_pcm_recover(pcm_handle, wr, 1);		
			wr = snd_pcm_avail_update(pcm_handle);
		}
#endif

#ifdef USE_MUTEX
		pthread_mutex_lock(&buffer_mutex);
#endif	
		while(cnt > 0){
			wr = min(cnt,desc.dwBufferBytes - rd_index);
#ifdef AUD_ALSA
			wr = snd_pcm_writei(pcm_handle,buffer + rd_index,snd_pcm_bytes_to_frames(pcm_handle,wr));
			if(wr > 0){
				wr = snd_pcm_frames_to_bytes(pcm_handle, wr);
				rd_index = (rd_index + wr) % desc.dwBufferBytes;
				cnt -= wr;								
			}
			else if(wr == -EAGAIN){
#ifdef _DEBUG
				printf("-EAGAIN\n");
#endif
				usleep(sleepTime/5);
				continue;
			}
			else if(wr == -EPIPE){
				snd_pcm_prepare(pcm_handle);
				continue;
			}
			else if(wr == -ESTRPIPE){
				while(going && snd_pcm_resume(pcm_handle) == -EAGAIN){
					usleep(400); 
				}
				continue;
			}
#else
			wr = write(fd,buffer + rd_index, wr);
			if(wr > 0){
				rd_index = (rd_index + wr) % desc.dwBufferBytes;
				cnt -= wr;				
			}
#endif
		}
#ifdef USE_MUTEX
		pthread_mutex_unlock(&buffer_mutex);
#endif			
#ifdef AUD_ALSA
		if(!first){
			snd_pcm_start(pcm_handle);		
			first = 1;
		}
#endif
		usleep(sleepTime);				
	}
}
//---------------------------------------------------------------------------
void *LDirectSoundBuffer::oss_loop(void *arg)
{
	if(arg != NULL)
		((LDirectSoundBuffer *)arg)->OnLoop();
	pthread_exit(NULL);		
}
#endif
//---------------------------------------------------------------------------
#ifdef AUD_ALSA
HRESULT LDirectSoundBuffer::CreateMMAP(snd_pcm_hw_params_t *hw_params,snd_pcm_sw_params_t *sw_params)
{
    snd_pcm_format_t format;
    snd_pcm_uframes_t frames, ofs, avail, psize, boundary;
    unsigned int channels, bits_per_sample, bits_per_frame;
    int err, mmap_mode;
    const snd_pcm_channel_area_t *areas;    
    

    mmap_mode = snd_pcm_type(pcm_handle);
    err = snd_pcm_hw_params_get_period_size(hw_params, &psize, NULL);
    err = snd_pcm_hw_params_get_format(hw_params, &format);
    err = snd_pcm_hw_params_get_buffer_size(hw_params, &frames);
    err = snd_pcm_hw_params_get_channels(hw_params, &channels);
    bits_per_sample = snd_pcm_format_physical_width(format);
    bits_per_frame = bits_per_sample * channels;
    
    mmap_buflen_frames = frames;
    mmap_buflen_bytes = snd_pcm_frames_to_bytes( pcm_handle, frames );

    snd_pcm_sw_params_current(pcm_handle, sw_params);
    snd_pcm_sw_params_set_start_threshold(pcm_handle, sw_params, 0);
    snd_pcm_sw_params_get_boundary(sw_params, &boundary);
    snd_pcm_sw_params_set_stop_threshold(pcm_handle, sw_params, boundary);
    snd_pcm_sw_params_set_silence_threshold(pcm_handle, sw_params, boundary);
    snd_pcm_sw_params_set_silence_size(pcm_handle, sw_params, 0);
    snd_pcm_sw_params_set_avail_min(pcm_handle, sw_params, 0);
    err = snd_pcm_sw_params(pcm_handle, sw_params);

    avail = snd_pcm_avail_update(pcm_handle);
    if ((snd_pcm_sframes_t)avail < 0)
    {
        return E_FAIL;
    }
    err = snd_pcm_mmap_begin(pcm_handle, &areas, &ofs, &avail);
    if ( err < 0 )
    {
        return E_FAIL;
    }
    snd_pcm_format_set_silence(format, areas->addr, mmap_buflen_frames);
    mmap_pos = ofs + snd_pcm_mmap_commit(pcm_handle, ofs, 0);
    mmap_buffer = (LPBYTE)areas->addr;
    return DS_OK;
}
#endif

