
#ifndef __LWINDOWBASEH__
#define __LWINDOWBASEH__

#define CREATE_SUSPENDED            0x00000004

#define STATUS_WAIT_0                    ((DWORD   )0x00000000L)
#define STATUS_ABANDONED_WAIT_0          ((DWORD   )0x00000080L)
#define STATUS_USER_APC                  ((DWORD   )0x000000C0L)
#define STATUS_TIMEOUT                   ((DWORD   )0x00000102L)
#define STATUS_PENDING                   ((DWORD   )0x00000103L)
#define DBG_CONTINUE                     ((DWORD   )0x00010002L)
#define STATUS_SEGMENT_NOTIFICATION      ((DWORD   )0x40000005L)
#define DBG_TERMINATE_THREAD             ((DWORD   )0x40010003L)
#define DBG_TERMINATE_PROCESS            ((DWORD   )0x40010004L)
#define DBG_CONTROL_C                    ((DWORD   )0x40010005L)
#define DBG_CONTROL_BREAK                ((DWORD   )0x40010008L)
#define STATUS_GUARD_PAGE_VIOLATION      ((DWORD   )0x80000001L)
#define STATUS_DATATYPE_MISALIGNMENT     ((DWORD   )0x80000002L)
#define STATUS_BREAKPOINT                ((DWORD   )0x80000003L)
#define STATUS_SINGLE_STEP               ((DWORD   )0x80000004L)
#define DBG_EXCEPTION_NOT_HANDLED        ((DWORD   )0x80010001L)
#define STATUS_ACCESS_VIOLATION          ((DWORD   )0xC0000005L)
#define STATUS_IN_PAGE_ERROR             ((DWORD   )0xC0000006L)
#define STATUS_INVALID_HANDLE            ((DWORD   )0xC0000008L)
#define STATUS_NO_MEMORY                 ((DWORD   )0xC0000017L)
#define STATUS_ILLEGAL_INSTRUCTION       ((DWORD   )0xC000001DL)
#define STATUS_NONCONTINUABLE_EXCEPTION  ((DWORD   )0xC0000025L)
#define STATUS_INVALID_DISPOSITION       ((DWORD   )0xC0000026L)
#define STATUS_ARRAY_BOUNDS_EXCEEDED     ((DWORD   )0xC000008CL)
#define STATUS_FLOAT_DENORMAL_OPERAND    ((DWORD   )0xC000008DL)
#define STATUS_FLOAT_DIVIDE_BY_ZERO      ((DWORD   )0xC000008EL)
#define STATUS_FLOAT_INEXACT_RESULT      ((DWORD   )0xC000008FL)
#define STATUS_FLOAT_INVALID_OPERATION   ((DWORD   )0xC0000090L)
#define STATUS_FLOAT_OVERFLOW            ((DWORD   )0xC0000091L)
#define STATUS_FLOAT_STACK_CHECK         ((DWORD   )0xC0000092L)
#define STATUS_FLOAT_UNDERFLOW           ((DWORD   )0xC0000093L)
#define STATUS_INTEGER_DIVIDE_BY_ZERO    ((DWORD   )0xC0000094L)
#define STATUS_INTEGER_OVERFLOW          ((DWORD   )0xC0000095L)
#define STATUS_PRIVILEGED_INSTRUCTION    ((DWORD   )0xC0000096L)
#define STATUS_STACK_OVERFLOW            ((DWORD   )0xC00000FDL)
#define STATUS_CONTROL_C_EXIT            ((DWORD   )0xC000013AL)
#define STATUS_FLOAT_MULTIPLE_FAULTS     ((DWORD   )0xC00002B4L)
#define STATUS_FLOAT_MULTIPLE_TRAPS      ((DWORD   )0xC00002B5L)
#define STATUS_ILLEGAL_VLM_REFERENCE     ((DWORD   )0xC00002C0L)
#define STATUS_REG_NAT_CONSUMPTION       ((DWORD   )0xC00002C9L)

#define MAXIMUM_WAIT_OBJECTS 64     // Maximum number of wait objects


#define INFINITE                0xFFFFFFFF  // Infinite timeout
#define WAIT_FAILED             ((DWORD)0xFFFFFFFF)
#define WAIT_OBJECT_0           ((STATUS_WAIT_0 ) + 0 )

#define WAIT_ABANDONED         ((STATUS_ABANDONED_WAIT_0 ) + 0 )
#define WAIT_ABANDONED_0       ((STATUS_ABANDONED_WAIT_0 ) + 0 )

#define WAIT_TIMEOUT                     258L

#define WAIT_IO_COMPLETION          STATUS_USER_APC
#define STILL_ACTIVE                STATUS_PENDING

typedef DWORD (*PTHREAD_START_ROUTINE)(LPVOID lpThreadParameter);

typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;


struct IWaitableObject
{
public:
	virtual void Release() = 0;
	virtual int Wait(unsigned long timeout) = 0;
};

void *FindResource(HINSTANCE instance,LPCTSTR name,LPCTSTR type);
void *LockResource(void *res);
unsigned long SizeofResource(void *p);

void DeleteObject(void *obj);

BOOL SetWindowPos(HWND hWnd,HWND hWndInsertAfter,int X,int Y,int cx,int cy,UINT uFlags);

int MapWindowPoints(HWND hWndFrom,HWND hWndTo,LPPOINT lpPoints,UINT cPoints);
BOOL ScreenToClient(HWND hWnd,LPPOINT lpPoint);
BOOL ClientToScreen(HWND hWnd,LPPOINT lpPoint);
BOOL GetWindowRect(HWND hwnd,LPRECT pr);
BOOL GetClientRect(HWND hwnd,LPRECT pr);

void EnableWindow(HWND hwnd,BOOL enable);
BOOL IsWindowEnabled(HWND hwnd);
BOOL ShowWindow(HWND hWnd,int nCmdShow);
HWND SetFocus(HWND hWnd);

BOOL MessageBeep(UINT uType);
int MessageBoxIndirect(LPMSGBOXPARAMS p);
int MessageBox(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
LONG GetWindowLong(HWND hWnd,int index);
LONG SetWindowLong(HWND hWnd,int index,LONG dwNewLong);
int GetDlgItemText(HWND hwnd,WORD id,char *text,int len);
int SetDlgItemText(HWND hwnd,WORD id,char *text);
int SetWindowText(HWND hwnd,char *text);
int GetWindowText(HWND hWnd,LPSTR lpString,int nMaxCount);

int SendDlgItemMessage(HWND hwnd,WORD id,UINT uMsg,WPARAM wParam,LPARAM lParam);
int SendMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL PostMessage(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
BOOL GetMessage(LPMSG lpMsg,HWND hWnd,UINT wMsgFilterMin,UINT wMsgFilterMax);

HWND GetDlgItem(HWND hwnd,WORD id);
int GetDlgCtrlID(HWND hwnd);
DWORD GetTickCount();
void Sleep(DWORD dwMilliseconds);
DWORD SleepEx(DWORD dwMilliseconds,BOOL bAlertable);
HGLOBAL GlobalAlloc(UINT uFlags,DWORD dwBytes);
HGLOBAL GlobalFree(HGLOBAL hMem);
LPVOID GlobalLock(HGLOBAL hMem);
BOOL GlobalUnlock(HGLOBAL hMem);
HLOCAL LocalAlloc(UINT uFlags,DWORD dwBytes);
HLOCAL LocalFree(HGLOBAL hMem);
DWORD LocalSize(HLOCAL hMem);
DWORD GlobalSize(HGLOBAL hMem);
DWORD GetModuleFileName(HMODULE hModule,LPTSTR lpFilename,DWORD nSize);

HANDLE CreateThread(LPVOID lpThreadAttributes,DWORD dwStackSize,LPTHREAD_START_ROUTINE lpStartAddress,LPVOID lpParameter,DWORD dwCreationFlags,LPDWORD lpThreadId);
DWORD ResumeThread(HANDLE hThread);
BOOL TerminateThread(HANDLE hThread,DWORD dwExitCode);
DWORD SetThreadAffinityMask(HANDLE hThread,DWORD dwThreadAffinityMask);
DWORD SetThreadIdealProcessor(HANDLE hThread,DWORD dwThreadAffinityMask);
HANDLE GetCurrentThread();

HANDLE CreateMutex(LPVOID lpMutexAttributes,bool bInitialOwner,LPCTSTR lpName);
BOOL ReleaseMutex(HANDLE hMutex);

HANDLE CreateEvent(LPVOID lpEventAttributes,BOOL bManualReset,BOOL bInitialState,LPCTSTR lpName);
BOOL SetEvent(HANDLE hEvent);

DWORD WaitForSingleObject(HANDLE hHandle,DWORD dwMilliseconds);
DWORD WaitForMultipleObjects(DWORD cEvents,HANDLE *lphEvents,BOOL bWaitAll,DWORD dwTimeout);

BOOL CloseHandle(HANDLE hObject);

LONG InterlockedExchange(LPLONG Target,LONG Value);

int window_init(int argc,char **argv);
gpointer SetUserData(HWND w);
BOOL SetWindowStyle(HWND hWnd,LONG style);

#endif



