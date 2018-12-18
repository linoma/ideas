#include "ideastypes.h"
#include "lwindow_base.h"
#include "lstring.h"
#include "syncobject.h"
#include <math.h>

extern int GetResourceMemory(char **buf,unsigned long *size);
static cairo_user_data_key_t cr_data;
static char lpModuleFileName[MAX_PATH+1];
//---------------------------------------------------------------------------
static void OnEnumChild(GtkWidget *widget,gpointer data)
{
   if(((LPDWORD)data)[1] != 0)
       return;
	if(GTK_IS_CONTAINER(widget))
		gtk_container_foreach((GtkContainer *)widget,OnEnumChild,data);
	if(GetWindowLong(widget,GWL_ID) == ((LPDWORD)data)[0])
		((LPDWORD)data)[1] = (DWORD)widget;
	else if(GetWindowLong(widget,GWL_ID2) == ((LPDWORD)data)[0])
		((LPDWORD)data)[1] = (DWORD)widget;
}
//---------------------------------------------------------------------------
LONG GetWindowLong(HWND hWnd,int index)
{
	LPDWORD p;

	if(hWnd == NULL || index > 99)
		return 0;
	p = (LPDWORD)g_object_get_data((GObject *)hWnd,"UserData");
	if(p == NULL)
		return 0;
	return (LONG)p[index];
}
//---------------------------------------------------------------------------
static void OnDeleteEvent(GtkObject *object,gpointer user_data)
{
	LPDWORD p,p1;
	LCaret *caret;
	GList *list;
	guint i,length;

	if(object == NULL)
		return;
	p = (LPDWORD)g_object_get_data((GObject *)object,"UserData");
	if(p == NULL)
		return;
	SendMessage((HWND)object,WM_DESTROY,0,0);
	if((caret = (LCaret *)GetWindowLong((HWND)object,GWL_CARET)) != NULL)
		delete caret;
	list = (GList *)p[GWL_MESSAGES];
	if(list != NULL){
		length = g_list_length(list);
		for(i=0;i<length;i++){
			p1 = (LPDWORD)g_list_nth_data(list,i);
			delete p1;
		}
		g_list_free(list);
	}
	if(p[GWL_DATA_INT0] != NULL)
		LocalFree((LPVOID)p[GWL_DATA_INT0]);
	g_object_set_data((GObject *)object,"UserData",NULL);
	LocalFree(p);
}
//---------------------------------------------------------------------------
gpointer SetUserData(HWND w)
{
	LPDWORD p;

	if(w == NULL)
		return NULL;
	p = (LPDWORD)g_object_get_data((GObject *)w,"UserData");
	if(p != NULL)
		return p;
	p = (LPDWORD)LocalAlloc(LPTR,100*sizeof(DWORD));
	if(p == NULL)
		return NULL;
	g_object_set_data((GObject *)w,"UserData",(gpointer)p);
	g_signal_connect((gpointer)w,"destroy",G_CALLBACK(OnDeleteEvent),NULL);
	return p;
}
//---------------------------------------------------------------------------
int GetDlgItemText(HWND hwnd,WORD id,char *text,int len)
{
	return SendDlgItemMessage(hwnd,id,WM_GETTEXT,len,(LPARAM)text);
}
//---------------------------------------------------------------------------
int SetDlgItemText(HWND hwnd,WORD id,char *text)
{
	return SendDlgItemMessage(hwnd,id,WM_SETTEXT,0,(LPARAM)text);
}
//---------------------------------------------------------------------------
int SetWindowText(HWND hwnd,char *text)
{
	return SendMessage(hwnd,WM_SETTEXT,0,(LPARAM)text);
}
//---------------------------------------------------------------------------
int GetWindowText(HWND hWnd,LPSTR lpString,int nMaxCount)
{
	if(hWnd != NULL && GTK_IS_COMBO_BOX_ENTRY(hWnd))
		hWnd = gtk_bin_get_child((GtkBin *)hWnd);
	return SendMessage(hWnd,WM_GETTEXT,nMaxCount,(LPARAM)lpString);
}
//---------------------------------------------------------------------------
int SendDlgItemMessage(HWND hwnd,WORD id,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	DWORD dw[2];

	if(hwnd == NULL || !GTK_IS_CONTAINER(hwnd))
		return 0;
	dw[0] = id;
	dw[1] = 0;

	gtk_container_foreach((GtkContainer *)hwnd,OnEnumChild,(gpointer)dw);
	if(dw[1] == 0)
		return 0;
	return SendMessage((HWND)dw[1],uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
BOOL GetMessage(LPMSG lpMsg,HWND hWnd,UINT wMsgFilterMin,UINT wMsgFilterMax)
{
	return gtk_main_iteration();
}
//---------------------------------------------------------------------------
BOOL PostMessage(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	GList *list;
	LPDWORD msg;

	list = (GList *)GetWindowLong(hWnd,GWL_MESSAGES);
	msg = new DWORD[3];
	if(msg == NULL)
		return FALSE;
	msg[0] = Msg;
	msg[1] = wParam;
	msg[2] = lParam;
	list = g_list_append(list,(gpointer)msg);
	SetWindowLong(hWnd,GWL_MESSAGES,(LONG)list);
	return TRUE;
}
//---------------------------------------------------------------------------
int SendMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	WNDPROC p;

	if(hwnd == NULL)
		return 0;
	p = (WNDPROC)GetWindowLong(hwnd,GWL_WNDPROC);
	if(p == NULL)
		return 0;
	return p(hwnd,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
BOOL SetWindowStyle(HWND hWnd,LONG style)
{
  	MwmHints mwm_hints={0};
   	Atom mwm_wm_hints={0};
	Display *display;
	LPDWORD p;

	if(hWnd == NULL || hWnd->window == 0)
		return FALSE;
	if(/*(style & WS_DLGFRAME) || */(style & WS_SYSMENU))
		style |= WS_CAPTION;
   	mwm_hints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
   	if((style & WS_CAPTION) == WS_CAPTION)
   		mwm_hints.functions |= MWM_FUNC_MOVE;
   	if(style & WS_THICKFRAME)
   		mwm_hints.functions |= MWM_FUNC_MOVE | MWM_FUNC_RESIZE;
   	if(style & WS_MINIMIZEBOX)
   		mwm_hints.functions |= MWM_FUNC_MINIMIZE;
   	if(style & WS_MAXIMIZEBOX)
   		mwm_hints.functions |= MWM_FUNC_MAXIMIZE;
    if(style & WS_SYSMENU)
    	mwm_hints.functions |= MWM_FUNC_CLOSE;
    if((style & WS_CAPTION) == WS_CAPTION) {
		mwm_hints.decorations |= MWM_DECOR_TITLE;
		if(style & WS_SYSMENU)
			mwm_hints.decorations |= MWM_DECOR_MENU;
		if(style & WS_MINIMIZEBOX)
			mwm_hints.decorations |= MWM_DECOR_MINIMIZE;
		if(style & WS_MAXIMIZEBOX)
			mwm_hints.decorations |= MWM_DECOR_MAXIMIZE;
    }
    if(style & WS_THICKFRAME)
    	mwm_hints.decorations |= MWM_DECOR_BORDER | MWM_DECOR_RESIZEH;
    else if((style & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME)
    	mwm_hints.decorations |= MWM_DECOR_BORDER;
    else if(style & WS_BORDER)
    	mwm_hints.decorations |= MWM_DECOR_BORDER;
    else if(!(style & (WS_CHILD|WS_POPUP)))
    	mwm_hints.decorations |= MWM_DECOR_BORDER;
	display = gdk_x11_drawable_get_xdisplay(hWnd->window);
	mwm_wm_hints = XInternAtom(display,"_MOTIF_WM_HINTS", False);
    if(mwm_wm_hints == 0)
    	return FALSE;
    XChangeProperty(display,GDK_DRAWABLE_XID(hWnd->window),mwm_wm_hints,
    	mwm_wm_hints,32,PropModeReplace,(unsigned char*)&mwm_hints,sizeof(mwm_hints)/sizeof(long));
	p = (LPDWORD)SetUserData(hWnd);
	if(p != NULL)
		p[GWL_STYLE] = style;
	gtk_widget_show_now(hWnd);
	return TRUE;
}
//---------------------------------------------------------------------------
LONG SetWindowLong(HWND hWnd,int index,LONG dwNewLong)
{
   LPDWORD p;
	LONG l;

	if(hWnd == NULL || index > 99)
		return 0;
	p = (LPDWORD)g_object_get_data((GObject *)hWnd,"UserData");
	if(p == NULL){
		p = (LPDWORD)SetUserData(hWnd);
		if(p == NULL)
			return 0;
	}
	l = p[index];
	p[index] = (DWORD)dwNewLong;
	if(index == GWL_STYLE)
		SetWindowStyle(hWnd,dwNewLong);
	/*else if(index == GWL_WNDPROC)
		g_signal_connect((gpointer)hWnd,"event",G_CALLBACK(on_window_event),NULL);	*/
	return l;
}

//---------------------------------------------------------------------------
HWND GetDlgItem(HWND hwnd,WORD id)
{
	DWORD dw[2];

	if(hwnd == NULL || !GTK_IS_CONTAINER(hwnd))
		return 0;
	dw[0] = id;
	dw[1] = 0;
	gtk_container_foreach((GtkContainer *)hwnd,OnEnumChild,(gpointer)dw);
	return (HWND)dw[1];
}
//---------------------------------------------------------------------------
int GetDlgCtrlID(HWND hwnd)
{
   return (int)GetWindowLong(hwnd,GWL_ID);
}
//---------------------------------------------------------------------------
DWORD GetTickCount()
{
   struct timeval t;

   gettimeofday( &t, NULL );
   return (t.tv_sec * 1000) + (t.tv_usec / 1000);
}
//---------------------------------------------------------------------------
void Sleep(DWORD dwMilliseconds)
{
	usleep(dwMilliseconds*1000);
}
//---------------------------------------------------------------------------
DWORD SleepEx(DWORD dwMilliseconds,BOOL bAlertable)
{
	usleep(dwMilliseconds*1000);
	return 1;
}
//---------------------------------------------------------------------------
HGLOBAL GlobalAlloc(UINT uFlags,DWORD dwBytes)
{
	HGLOBAL hMem;

   	hMem = malloc(dwBytes + sizeof(DWORD));
   	if(hMem == NULL)
   		return NULL;
   	if(uFlags & GMEM_ZEROINIT)
   		memset(hMem,0,dwBytes + sizeof(DWORD));
   	*((LPDWORD)hMem) = dwBytes;
	hMem = (HGLOBAL)(((char *)hMem) + sizeof(DWORD));
   	return hMem;
}
//---------------------------------------------------------------------------
HGLOBAL GlobalFree(HGLOBAL hMem)
{
	if(hMem == NULL)
   		return NULL;
	hMem = (HGLOBAL)(((char *)hMem) - sizeof(DWORD));
   	free(hMem);
   	return NULL;
}
//---------------------------------------------------------------------------
LPVOID GlobalLock(HGLOBAL hMem)
{
	return hMem;
}
//---------------------------------------------------------------------------
BOOL GlobalUnlock(HGLOBAL hMem)
{
	return TRUE;
}
//---------------------------------------------------------------------------
HLOCAL LocalAlloc(UINT uFlags,DWORD dwBytes)
{
	return GlobalAlloc(uFlags,dwBytes);
}
//---------------------------------------------------------------------------
HLOCAL LocalFree(HGLOBAL hMem)
{
   return GlobalFree(hMem);
}
//---------------------------------------------------------------------------
DWORD LocalSize(HLOCAL hMem)
{
   return GlobalSize(hMem);
}
//---------------------------------------------------------------------------
DWORD GlobalSize(HGLOBAL hMem)
{
	if(hMem == NULL)
   		return 0;
	hMem = (HGLOBAL)(((char *)hMem) - sizeof(DWORD));
   return *((LPDWORD)hMem);
}
//---------------------------------------------------------------------------
DWORD GetModuleFileName(HMODULE hModule,LPTSTR lpFilename,DWORD nSize)
{
	struct {
		void *l_addr;
		char *l_name;
		void *l_ld;
		void *l_next,*l_prev;
	} *test;

	if(lpFilename == NULL)
   		return 0;
   	*lpFilename = 0;
	if(hModule == NULL)
   		strncpy(lpFilename,lpModuleFileName,nSize);
	else{
		dlinfo(hModule,2,&test);
		if(test == NULL)
			return 0;
		strncpy(lpFilename,test->l_name,nSize);
	}
   	return (DWORD)strlen(lpFilename);
}
//---------------------------------------------------------------------------
int MessageBox(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
   	MSGBOXPARAMS p={0};

   	p.dwStyle = uType;
   	p.lpszText = lpText;
   	p.hwndOwner = hWnd;
   	p.lpszCaption = lpCaption;
	return MessageBoxIndirect(&p);
}
//---------------------------------------------------------------------------
int MessageBoxIndirect(LPMSGBOXPARAMS p)
{
	GtkWidget *w;
	gint res;
	GtkMessageType type;
	GtkButtonsType btnType;
	GtkImage *img;

	if(p->dwStyle & MB_ICONERROR)
		type = GTK_MESSAGE_ERROR;
	else if(p->dwStyle & MB_ICONQUESTION)
		type = GTK_MESSAGE_QUESTION;
	else if(p->dwStyle & MB_ICONWARNING)
		type = GTK_MESSAGE_WARNING;
	else
		type = GTK_MESSAGE_INFO;
	if(p->dwStyle & MB_YESNO)
		btnType = GTK_BUTTONS_YES_NO;
	else
		btnType = GTK_BUTTONS_OK;
	w = gtk_message_dialog_new((GtkWindow *)p->hwndOwner,GTK_DIALOG_MODAL,type,btnType,p->lpszText);
	gtk_window_set_title((GtkWindow *)w,p->lpszCaption);
	if(p->lpszIcon != NULL){
		img = (GtkImage *)gtk_image_new_from_pixbuf(LoadIcon(p->hInstance,p->lpszIcon,32,32,0));
		gtk_widget_show((HWND)img);
		gtk_message_dialog_set_image((GtkMessageDialog *)w,(GtkWidget *)img);
	}
	//gtk_message_dialog_format_secondary_text((GtkMessageDialog *)w,lpText);
	gtk_widget_show (w);
	res = gtk_dialog_run((GtkDialog *)w);
	gtk_widget_destroy(w);
	switch(res){
		case GTK_RESPONSE_OK:
			return IDOK;
		case GTK_RESPONSE_NO:
			return IDNO;
		case GTK_RESPONSE_YES:
			return IDYES;
	}
	return 0;
}
//---------------------------------------------------------------------------
BOOL MessageBeep(UINT uType)
{
/*	int fd, time, freq, arg;

   	fd = open("/dev/tty0", O_RDONLY);
    freq = 5000
    time = 50;
   	arg = (time<<16)+(1193180/freq);
   	ioctl(fd,KDMKTONE,arg);
   	usleep(time * 1000);
   	close(fd);*/
	return TRUE;
}
//---------------------------------------------------------------------------
LONG InterlockedExchange(LPLONG Target,LONG Value)
{
    __asm(
        "mov %0,%%eax\n"
        "lock xchg %%eax,%1\n"
        "mov %%eax,%0\n"
        : "=m" (Value) : "m" (Target)
    );
    return Value;
}
//---------------------------------------------------------------------------
HANDLE CreateThread(LPVOID lpThreadAttributes,DWORD dwStackSize,LPTHREAD_START_ROUTINE lpStartAddress,LPVOID lpParameter,DWORD dwCreationFlags,LPDWORD lpThreadId)
{
	LThread *p;

	p = new LThread();
	if(p != NULL){
		if(!p->Create(lpStartAddress,lpParameter,dwCreationFlags)){
			delete p;
			return NULL;
		}
	}
	return p;
}
//---------------------------------------------------------------------------
DWORD ResumeThread(HANDLE hThread)
{
    return 0;
}
//---------------------------------------------------------------------------
BOOL TerminateThread(HANDLE hThread,DWORD dwExitCode)
{
    if(hThread == NULL)
        return FALSE;
    ((LThread *)hThread)->Terminate(dwExitCode);
	delete ((LThread *)hThread);
    return TRUE;
}
//---------------------------------------------------------------------------
HANDLE GetCurrentThread()
{
    return NULL;
}
//------------------------------------------------------------------------------
DWORD SetThreadAffinityMask(HANDLE hThread,DWORD dwThreadAffinityMask)
{
	cpu_set_t cpu_set;
	size_t i;
    pthread_t thread;

    if(hThread == NULL)
        thread = pthread_self();
	else
        thread = ((LThread *)hThread)->Handle();
    if(thread == NULL)
        return 0;
    CPU_ZERO(&cpu_set);
    for (i = 0; i < sizeof(dwThreadAffinityMask) * 8; i++){
        if((dwThreadAffinityMask >> i) & 1)
            CPU_SET(i, &cpu_set);
	}
    pthread_setaffinity_np(thread, sizeof(cpu_set), &cpu_set);
    return 1;
}
//------------------------------------------------------------------------------
DWORD SetThreadIdealProcessor(HANDLE hThread,DWORD dwThreadAffinityMask)
{
    return SetThreadAffinityMask(hThread,dwThreadAffinityMask);
}
//---------------------------------------------------------------------------
HANDLE CreateEvent(LPVOID lpEventAttributes,BOOL bManualReset,BOOL bInitialState,LPCTSTR lpName)
{
	LEvent *p;

	p = new LEvent();
	if(p!= NULL){
		if(!p->Create(bManualReset,bInitialState,lpName)){
			delete p;
			return NULL;
		}
	}
	return p;
}
//---------------------------------------------------------------------------
BOOL SetEvent(HANDLE hEvent)
{
	if(hEvent == NULL)
		return FALSE;
	return ((LEvent *)hEvent)->Set();
}
//---------------------------------------------------------------------------
HANDLE CreateMutex(LPVOID lpMutexAttributes,bool bInitialOwner,LPCTSTR lpName)
{
	LMutex *p;

	p = new LMutex();
	if(p != NULL){
		if(!p->Create(bInitialOwner,lpName)){
			delete p;
			return NULL;
		}
	}
	return p;
}
//---------------------------------------------------------------------------
BOOL ReleaseMutex(HANDLE hMutex)
{
	if(hMutex == NULL)
		return false;
	return ((LMutex *)hMutex)->Unlock();
}
//---------------------------------------------------------------------------
BOOL CloseHandle(HANDLE hObject)
{
	if(hObject == NULL)
		return false;
	((IWaitableObject *)hObject)->Release();
	return true;
}
//---------------------------------------------------------------------------
DWORD WaitForSingleObject(HANDLE hHandle,DWORD dwMilliseconds)
{
    if(hHandle == NULL)
		return WAIT_FAILED;
	return ((IWaitableObject *)hHandle)->Wait(dwMilliseconds);
}
//---------------------------------------------------------------------------
DWORD WaitForMultipleObjects(DWORD cEvents,HANDLE *lphEvents,BOOL bWaitAll,DWORD dwTimeout)
{
	DWORD dw,dw1,dw2,res;

	if(lphEvents == NULL || cEvents == 0)
		return WAIT_FAILED;
	//WSA_WAIT_EVENT_0
	//WSA_WAIT_TIMEOUT
	if(dwTimeout == INFINITE){
		while(1){
			for(dw2=0;dw2<cEvents;dw2++){
				res =  ((IWaitableObject *)lphEvents[dw2])->Wait(10);
				if(res == WAIT_OBJECT_0)
					return WAIT_OBJECT_0 + dw2;
			}
		}
	}
	else{
		if(dwTimeout > 10)
			dw = 10;
		else
			dw = dwTimeout;
		for(dw1=0;dw1<dwTimeout;dw1+=dw){
			for(dw2=0;dw2<cEvents;dw2++){
				res =  ((IWaitableObject *)lphEvents[dw2])->Wait(dw);
				if(res == WAIT_OBJECT_0)
					return WAIT_OBJECT_0 + dw2;
			}
		}
	}
	return WAIT_TIMEOUT;
}

//---------------------------------------------------------------------------
void *FindResource(HINSTANCE instance,LPCTSTR name,LPCTSTR type)
{
	LPRES_HEADER dir;
	DWORD dw,ofs,step;
	int (*pfn)(char **,unsigned long*);

	if(instance != NULL){
		pfn = (int (*)(char **,unsigned long*))GetProcAddress(instance,"_Z17GetResourceMemoryPPcPm");
		if(pfn == NULL)
			return NULL;
		if(!pfn((char **)&dir,&dw))
			return NULL;
	}
	else{
		if(!GetResourceMemory((char **)&dir,&dw))
			return NULL;
	}
	for(ofs=0;ofs < dw;){
 		step = dir->HeaderSize;
 		if(dir->ResType != 0xFFFF){
			if(HIWORD(dir->ResType) == (WORD)(DWORD)type){
				if(HIWORD(dir->ResName) == (WORD)(DWORD)name)
					return (void *)dir;
			}
		}
       	step += dir->DataSize;
       	if(step % 4)
       		step = ((step >> 2) + 1) << 2;
       	ofs += step;
       	dir = (LPRES_HEADER)((char *)dir + step);
   	}
   	return NULL;
}
//---------------------------------------------------------------------------
void *LockResource(void *res)
{
	if(res == NULL)
		return NULL;
	return (void *)(((char *)res) + ((LPRES_HEADER)res)->HeaderSize);
}
//---------------------------------------------------------------------------
unsigned long SizeofResource(void *p)
{
	return ((LPRES_HEADER)p)->DataSize;
}
//---------------------------------------------------------------------------
static bool get_WindowOrigin(HWND hWnd,POINT &org)
{
	gint x1,y1;
	HWND hwnd,parent;

	if(hWnd == NULL || hWnd->window == 0)
		return false;
    org.x = org.y = 0;
    hwnd = hWnd;
	if(!GTK_IS_WINDOW(hwnd)){
		while((parent = gtk_widget_get_parent(hwnd)) != NULL){
			if(GTK_IS_WINDOW(parent))
            	break;
			gtk_widget_translate_coordinates(hwnd,parent,0,0,&x1,&y1);
			org.x += abs(x1);
			org.y += abs(y1);
			hwnd = parent;
		}
		if(parent != NULL)
			gdk_window_get_position(parent->window,&x1,&y1);
	}
	else{
		gdk_window_get_root_origin(hwnd->window,(gint *)&x1,(gint *)&y1);
	}
	org.x += abs(x1);
	org.y += abs(y1);
	return true;
}
//---------------------------------------------------------------------------
int MapWindowPoints(HWND hWndFrom,HWND hWndTo,LPPOINT lpPoints,UINT cPoints)
{
	UINT i;
	GdkWindow *hwnd,*hwnd1;
	POINT orgFrom,orgTo;
	gint x;

	if(lpPoints == NULL || hWndFrom == hWndTo)
		return 0;
	orgFrom.x = orgFrom.y = 0;
	if(hWndFrom != NULL){
		get_WindowOrigin(hWndFrom,orgFrom);
	}
	orgTo.x = orgTo.y = 0;
	if(hWndTo != NULL){
		get_WindowOrigin(hWndTo,orgTo);

	}
	for(i=0;i<cPoints;i++){
		lpPoints[i].x += orgFrom.x - orgTo.x;
		lpPoints[i].y += orgFrom.y - orgTo.y;
	}
	return cPoints;
}
//---------------------------------------------------------------------------
BOOL ScreenToClient(HWND hWnd,LPPOINT lpPoint)
{
	return MapWindowPoints(NULL,hWnd,lpPoint,1);
}
//---------------------------------------------------------------------------
BOOL ClientToScreen(HWND hWnd,LPPOINT lpPoint)
{
	return MapWindowPoints(hWnd,NULL,lpPoint,1);
}
//---------------------------------------------------------------------------
BOOL GetClientRect(HWND hwnd,LPRECT pr)
{
	GtkRequisition sz;

	if(hwnd == NULL || hwnd->window == NULL || pr == NULL)
		return FALSE;
	gtk_widget_size_request(hwnd,&sz);
	pr->left = 0;
	pr->top = 0;
	pr->right = sz.width;
	pr->bottom = sz.height;
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL GetWindowRect(HWND hWnd,LPRECT pr)
{
	GtkRequisition sz;
	gint w,h,x,y,x1,y1;
	HWND hwnd;

	if(hWnd == NULL || pr == NULL)
		return FALSE;
	gtk_widget_size_request(hWnd,&sz);
	if(hWnd->window){
		if(!GTK_WIDGET_TOPLEVEL(hWnd)){
			if((hwnd = gtk_widget_get_ancestor(hWnd,GTK_TYPE_FIXED)) != NULL){
				gtk_widget_translate_coordinates(hWnd,hwnd,0,0,&x,&y);
				gdk_window_get_root_origin(hwnd->window,&x1,&y1);
				x += x1;
				y += y1;
			}
			else{
				hwnd = gtk_widget_get_toplevel(hWnd);
				if(hwnd != NULL){
					gtk_widget_translate_coordinates(hWnd,hwnd,0,0,&x,&y);
					gdk_window_get_root_origin(hwnd->window,&x1,&y1);
					x += x1;
					y += y1;
				}
			}
		}
		else{
			gdk_window_get_root_origin(hWnd->window,&x1,&y1);
//            while(gtk_events_pending())
//                gtk_main_iteration_do(false);
            gdk_window_get_position(hWnd->window,&x,&y);
            gdk_drawable_get_size(hWnd->window,&w,&h);
			x1 = abs(x1-x) / 2;
			//y1 = abs(y1-y);
			y1 = x1;
            sz.width = w+x1;
            sz.height = h+y1;
		}
	}
	else
		x = y = 0;
	pr->left = x;
	pr->top = y;
	pr->right = sz.width+x;
	pr->bottom = sz.height+y;
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL MoveWindow(HWND hWnd,int X,int Y,int nWidth,int nHeight,BOOL bRepaint)
{
	return SetWindowPos(hWnd,NULL,X,Y,nWidth,nHeight,SWP_NOZORDER);
}
//---------------------------------------------------------------------------
BOOL SetWindowPos(HWND hWnd,HWND hWndInsertAfter,int X,int Y,int cx,int cy,UINT uFlags)
{
	gint x1,y1,w1,h1,d1,i;
	HWND hwnd,hwnd1,vbox;
	//POINT pt;
	//RECT rc;
	WNDPROC pProc;

	if(hWnd == NULL)
		return FALSE;
	x1 = y1 = 0;
	if((uFlags & SWP_NOMOVE) == 0){
		if(GTK_IS_FIXED(hWnd)){
			hwnd = gtk_widget_get_parent(hWnd);
			if(hwnd != NULL && GTK_IS_FIXED(hwnd))
				gtk_fixed_move((GtkFixed *)hwnd,hWnd,X,Y);
		}
		else if((hwnd = gtk_widget_get_ancestor(hWnd,GTK_TYPE_FIXED)) != NULL){
            //if(!GTK_IS_FIXED(hwnd))
                gtk_fixed_move((GtkFixed *)hwnd,hWnd,X,Y);
		}
		else{
			if(GTK_WIDGET_TOPLEVEL(hWnd))
				gtk_window_move(GTK_WINDOW(hWnd),X,Y);
			else if(hWnd->window != 0)
				gdk_window_move(hWnd->window,X,Y);
		}
	}
	if((uFlags & SWP_NOSIZE) == 0){
		/*if(hWnd->window && GTK_WIDGET_TOPLEVEL(hWnd)){
			gdk_window_get_geometry(hWnd->window,&x1,&y1,&w1,&h1,&d1);
			x1 = 0;
			gtk_window_set_resizable(GTK_WINDOW(hWnd),FALSE);
		}
		else*/
			x1 = y1 = 0;
		hwnd = gtk_widget_get_parent(hWnd);
		if(hwnd != NULL && GTK_IS_SCROLLED_WINDOW(hwnd))
			gtk_widget_set_size_request(hwnd,cx-x1*2,cy-y1-x1);
		else
			gtk_widget_set_size_request(hWnd,cx-x1*2,cy-y1-x1);
		if(hWnd->window){
			if(GTK_WIDGET_TOPLEVEL(hWnd)){
				gtk_container_check_resize((GtkContainer *)hWnd);
				gtk_window_set_resizable(GTK_WINDOW(hWnd),(GetWindowLong(hWnd,GWL_STYLE) & WS_THICKFRAME) ? TRUE : FALSE);
			}
			else{
				pProc = (WNDPROC)GetWindowLong(hWnd,GWL_WNDPROC);
				if(pProc != NULL)
					pProc(hWnd,WM_SIZE,0,MAKELPARAM(cx-x1*2,cy-y1-x1));
			}
		}
	}
	if(!(uFlags & (SWP_NOZORDER|SWP_NOREPOSITION))){
		if(hWndInsertAfter == HWND_BOTTOM){
			if(GTK_WIDGET_TOPLEVEL(hWnd))
				gtk_window_set_keep_below(GTK_WINDOW(hWnd),true);
			else
				gdk_window_lower(hWnd->window);
		}
		else if(hWndInsertAfter == HWND_TOP){
		}
	}
	if(uFlags & SWP_SHOWWINDOW)
		gtk_widget_show(hWnd);
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL IsWindowEnabled(HWND hwnd)
{
    return gtk_widget_is_sensitive(hwnd);
}
//---------------------------------------------------------------------------
void EnableWindow(HWND hwnd,BOOL enable)
{
	gtk_widget_set_sensitive(hwnd,enable);
}
//--------------------------------------------------------------------------------
BOOL ShowWindow(HWND hWnd,int nCmdShow)
{
	if(hWnd == NULL)
		return FALSE;
	switch(nCmdShow){
 		case SW_SHOW:
		case SW_SHOWNORMAL:
			gtk_widget_show_all(hWnd);
		break;
		case SW_HIDE:
			gtk_widget_hide_all(hWnd);
			gtk_widget_hide(hWnd);
		break;
	}
	return TRUE;
}
//---------------------------------------------------------------------------
HWND SetFocus(HWND hWnd)
{
	HWND hwnd;

	hwnd = gtk_grab_get_current();
	gtk_widget_grab_focus(hWnd);
	return hwnd;
}
//--------------------------------------------------------------------------------
void DeleteObject(void *obj)
{
	g_object_unref(obj);
}
//------------------------------------------------------------------------------
int window_init(int argc,char **argv)
{
	LString s;

	s = LString(argv[0]).Path();
	if(s.IsEmpty() || s[1] == '.'){
		s.Length(MAX_PATH+1);
		GetCurrentDirectory(MAX_PATH,s.c_str());
	}
	if(s[s.Length()] != '/')
		s += "/";
	s += LString(argv[0]).FileName();
	lstrcpy(lpModuleFileName,s.c_str());
	return 1;
}

