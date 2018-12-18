
#ifndef __LTIMERH__
#define __LTIMERH__

class LTimer
{
public:
	LTimer();
	~LTimer();
	UINT Start(HWND hWnd,UINT nIDEvent,UINT uElapse,TIMERPROC lpTimerFunc);
	BOOL Stop();
	BOOL OnTimerProc();
	HWND Window(){return m_hWnd;};
	UINT ID(){return nID;};
	guint Event(){return uTimer;}
protected:
	HWND m_hWnd;
	UINT nID;
	TIMERPROC proc;
	guint uTimer;
};

extern LTimer *FindTimer(HWND hWnd,UINT nIDEvent);

#endif

