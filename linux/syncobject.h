#include "lwindow.h"
#include <errno.h>

#ifndef __SYNCOBJECTH__
#define __SYNCOBJECTH__

//---------------------------------------------------------------------------
class LEvent : public IWaitableObject
{
public:
	LEvent();
	virtual ~LEvent();
	virtual void Release();
	virtual int Wait(unsigned long timeout);
	int Set();
	void Reset();
   BOOL Create(BOOL bManualReset,BOOL bInitialState,LPCSTR lpName);
protected:
	virtual void Destroy();
   pthread_cond_t event;
   pthread_mutex_t mutex;
   BOOL bCreate,bSignaled;
};
//---------------------------------------------------------------------------
class LThread : public IWaitableObject
{
public:
	LThread();
	virtual ~LThread();
	virtual void Release();
	virtual int Wait(unsigned long timeout);
	DWORD Resume();
	BOOL Terminate(DWORD dwExitCode);
	BOOL Create(LPTHREAD_START_ROUTINE lpStartAddress,LPVOID lpParameter,DWORD dwCreationFlags);
	DWORD OnThreadProc();
    pthread_t Handle(){return thread;};
protected:
	static void *thread_proc(void *arg);
	BOOL bCreate,bQuit;
	pthread_t thread;
	DWORD Id;
	LPTHREAD_START_ROUTINE proc;
	LPVOID param;
};
//---------------------------------------------------------------------------
class LMutex : public IWaitableObject
{
public:
	LMutex();
	virtual ~LMutex();
	virtual void Release();
	virtual int Wait(unsigned long timeout);
	int Lock();
	int Unlock();
    BOOL Create(BOOL bInitialState,LPCSTR lpName);
protected:
	virtual void Destroy();
	pthread_mutex_t mutex;
	bool bCreate;
};

#endif

