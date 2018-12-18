#include "ideastypes.h"
#include "ltimer.h"

static GList *timers;
//---------------------------------------------------------------------------
static gboolean timeout_callback(gpointer data)
{
	if(data != NULL)
		return ((LTimer *)data)->OnTimerProc();
   	return FALSE;
}
//---------------------------------------------------------------------------
LTimer *FindTimer(HWND hWnd,UINT nIDEvent)
{
	guint i,length;
	LTimer *timer;

	if(timers == NULL)
		return NULL;
	length = g_list_length(timers);
	for(i=0;i<length;i++){
		timer = (LTimer *)g_list_nth_data(timers,i);
		if(timer == NULL)
			continue;
		if(hWnd != NULL){
			if(hWnd == timer->Window() && nIDEvent == timer->ID())
				return timer;
		}
		else if(nIDEvent == timer->Event())
			return timer;
	}
	return NULL;	
}
//---------------------------------------------------------------------------
LTimer::LTimer()
{
	m_hWnd = NULL;
	proc = NULL;	
	uTimer = 0;
	timers = g_list_append(timers,(gpointer)this);		
}
//---------------------------------------------------------------------------
LTimer::~LTimer()
{
	Stop();
}
//---------------------------------------------------------------------------
UINT LTimer::Start(HWND hWnd,UINT nIDEvent,UINT uElapse,TIMERPROC lpTimerFunc)
{
	if(hWnd == NULL && lpTimerFunc == NULL)
		return 0;
	proc = lpTimerFunc;
	m_hWnd = hWnd;
	nID = nIDEvent;
	uTimer = gdk_threads_add_timeout(uElapse,timeout_callback,(gpointer)this);
	return uTimer;
}
//---------------------------------------------------------------------------
BOOL LTimer::OnTimerProc()
{
	if(m_hWnd != NULL){
		SendMessage(m_hWnd,WM_TIMER,nID,0);
		return TRUE;
	}
	else if(proc != NULL){
		proc(NULL,0,0,0);
		return TRUE;
	}
	return FALSE;
}
//---------------------------------------------------------------------------
BOOL LTimer::Stop()
{
	if(uTimer == 0)
		return FALSE;
	g_source_remove(uTimer);
	uTimer = 0;
	if(timers != NULL){
		if(g_list_index(timers,(gconstpointer)this) != -1){
			timers = g_list_remove(timers,(gconstpointer)this);
			if(timers != NULL && g_list_length(timers) < 1){
				g_list_free(timers);
				timers = NULL;
			}
		}
	}
	return TRUE;
}
