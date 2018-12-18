#include "ideastypes.h"
#include "syncobject.h"

#ifndef __WIN32__

//--------------------------------------------------------------------------------
LThread::LThread()
{
	bCreate = FALSE;
	Id = 0;
}
//--------------------------------------------------------------------------------
LThread::~LThread()
{
}
//--------------------------------------------------------------------------------
void LThread::Release()
{
}
//--------------------------------------------------------------------------------
void *LThread::thread_proc(void *arg)
{
	pthread_exit((void *)((LThread *)arg)->OnThreadProc());
}
//--------------------------------------------------------------------------------
DWORD LThread::OnThreadProc()
{
	if(proc != NULL)
		return proc(param);
	return (DWORD)-1;
}
//--------------------------------------------------------------------------------
int LThread::Wait(unsigned long timeout)
{
	int val;
  	void *status;
	struct timespec ts;
  	struct timeval tv;

	if(timeout == INFINITE)
		pthread_join((pthread_t)thread,0);
	else{
		gettimeofday(&tv,NULL);
  		TIMEVAL_TO_TIMESPEC(&tv,&ts);
  		ts.tv_nsec += timeout * 1000000;
  		if (ts.tv_nsec >= 1000000000){
      		ts.tv_nsec -= 1000000000;
      		ts.tv_sec++;
    	}
  		val = pthread_timedjoin_np(thread, &status, &ts);
	}
}
//--------------------------------------------------------------------------------
DWORD LThread::Resume()
{
}
//--------------------------------------------------------------------------------
BOOL LThread::Create(LPTHREAD_START_ROUTINE lpStartAddress,LPVOID lpParameter,DWORD dwCreationFlags)
{
    if(lpStartAddress == NULL)
        return FALSE;
    if(pthread_create(&thread,NULL,(void * (*)(void *))lpStartAddress,lpParameter) != 0)
		return FALSE;
	bCreate = TRUE;
    return TRUE;
}
//--------------------------------------------------------------------------------
BOOL LThread::Terminate(DWORD dwExitCode)
{
	if(!bCreate)
		return FALSE;
	pthread_join((pthread_t)thread,0);
	return TRUE;
}
//--------------------------------------------------------------------------------
LEvent::LEvent()
{
//    event = PTHREAD_COND_INITIALIZER;
//    mutex = PTHREAD_MUTEX_INITIALIZER;
    bCreate = FALSE;
    bSignaled = FALSE;
}
//--------------------------------------------------------------------------------
LEvent::~LEvent()
{
    Destroy();
}
//--------------------------------------------------------------------------------
BOOL LEvent::Create(BOOL bManualReset,BOOL bInitialState,LPCSTR lpName)
{
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&event,NULL);
    bCreate = TRUE;
    if(bInitialState)
        Set();
    return TRUE;
}
//--------------------------------------------------------------------------------
void LEvent::Release()
{
    delete this;
}
//--------------------------------------------------------------------------------
int LEvent::Wait(DWORD timeout)
{
   struct timespec ts;
   int res;

   res = WAIT_FAILED;
   if(!bCreate)
        return res;
   pthread_mutex_lock(&mutex);
   if(!bSignaled){
       if(timeout != INFINITE){
           clock_gettime(CLOCK_REALTIME, &ts);
           ts.tv_sec += timeout;
           if((res = pthread_cond_timedwait(&event,&mutex,&ts)) == 0)
               res = WAIT_OBJECT_0;
           else{
                switch(res){
                   case ETIMEDOUT:
                       res = WAIT_TIMEOUT;
                   break;
                   default:
                       res = WAIT_ABANDONED;
                   break;
               }
           }
       }
       else{
           pthread_cond_wait(&event,&mutex);
           res = WAIT_OBJECT_0;
       }
   }
   else
       res = WAIT_OBJECT_0;
   pthread_mutex_unlock(&mutex);
   if(res == WAIT_OBJECT_0)
       bSignaled = FALSE;
   return res;
}
//--------------------------------------------------------------------------------
int LEvent::Set()
{
   if(!bCreate)
       return 0;
   pthread_mutex_lock(&mutex);
   bSignaled = TRUE;
   pthread_cond_broadcast(&event);
   pthread_mutex_unlock(&mutex);
   return 1;
}
//--------------------------------------------------------------------------------
void LEvent::Reset()
{
}
//--------------------------------------------------------------------------------
void LEvent::Destroy()
{
	Set();
	if(!bCreate)
		return;
	pthread_mutex_lock(&mutex);
	pthread_mutex_destroy(&mutex);
   pthread_cond_destroy(&event);
	bCreate = FALSE;
}
//--------------------------------------------------------------------------------
LMutex::LMutex()
{
	bCreate = false;
}
//--------------------------------------------------------------------------------
LMutex::~LMutex()
{
	Destroy();
}
//--------------------------------------------------------------------------------
void LMutex::Release()
{
	delete this;
}
//--------------------------------------------------------------------------------
int LMutex::Wait(unsigned long timeout)
{
	int res;
	struct timespec ts;

    res = WAIT_FAILED;
    if(!bCreate)
        return res;
    if(timeout == INFINITE){
    	 pthread_mutex_lock(&mutex);
    	 res = WAIT_OBJECT_0;
	}
	else{
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout;
		if(pthread_mutex_timedlock(&mutex,&ts) == 0)
			res = WAIT_OBJECT_0;
	}
   	return res;
}
//--------------------------------------------------------------------------------
int LMutex::Lock()
{
	if(!bCreate)
		return 0;
	pthread_mutex_lock(&mutex);
	return 1;
}
//--------------------------------------------------------------------------------
int LMutex::Unlock()
{
	if(!bCreate)
		return 0;
	pthread_mutex_unlock(&mutex);
	return 1;
}
//--------------------------------------------------------------------------------
BOOL LMutex::Create(BOOL bInitialState,LPCSTR lpName)
{
	if(bCreate)
		return true;
	pthread_mutex_init(&mutex,NULL);
	bCreate = true;
	return true;
}
//--------------------------------------------------------------------------------
void LMutex::Destroy()
{
	if(!bCreate)
		return;
	pthread_mutex_unlock(&mutex);
	pthread_mutex_destroy(&mutex);
	bCreate = false;
}
#endif


