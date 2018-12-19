#include "lwnd.h"
#include "lapp.h"
#include "lmenu.h"
#include "lopensavedlg.h"

extern LRESULT WndProcEdit(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

//---------------------------------------------------------------------------
LWndBase::LWndBase()
{
	m_hWnd = NULL;
	align = alNone;
}
//---------------------------------------------------------------------------
BOOL LWndBase::Assign(HWND hwnd)
{
	if(hwnd == NULL)
  		return FALSE;
 	if(!::IsWindow(hwnd))
 		return FALSE;
  	m_hWnd = hwnd;
	return TRUE;
}
//---------------------------------------------------------------------------
void LWndBase::ClientToScreen(LPPOINT p)
{
#ifndef __WIN32__
	gint xPos,yPos;

	gdk_window_get_root_origin(m_hWnd->window,&xPos,&yPos);	
	p->x += xPos;
	p->y += yPos;
#else
	::ClientToScreen(m_hWnd,p);
#endif
}
//---------------------------------------------------------------------------
void LWndBase::ScreenToClient(LPPOINT p)
{
#ifndef __WIN32__
/*	HWND w;
	
	XTranslateCoordinates(pApp->GetDesktopWindow(),
		DefaultRootWindow(pApp->GetDesktopWindow()),
		m_hWnd,p->x,p->y,(int *)&p->x,(int *)&p->y,&w);*/
	gint xPos,yPos;

	gdk_window_get_root_origin(m_hWnd->window,&xPos,&yPos);	
	p->x -= xPos;
	p->y -= yPos;
#else
   if(m_hWnd != NULL)
       ::ScreenToClient(m_hWnd,p);
#endif
}
//---------------------------------------------------------------------------
BOOL LWndBase::GetClientRect(LPRECT pr)
{
	if(m_hWnd == NULL || pr == NULL)
		return FALSE;
	return ::GetClientRect(m_hWnd,pr);
}
//---------------------------------------------------------------------------
BOOL LWndBase::GetWindowRect(LPRECT pr)
{
	if(m_hWnd == NULL || pr == NULL)
		return FALSE;
	return ::GetWindowRect(m_hWnd,pr);
}
//---------------------------------------------------------------------------
long LWndBase::SetWindowStyle(long style)
{
	return ::SetWindowLong(m_hWnd,GWL_STYLE,style);
}
//---------------------------------------------------------------------------
BOOL LWndBase::Show(int nCmdShow)
{
	return ::ShowWindow(m_hWnd,nCmdShow);
}
//---------------------------------------------------------------------------
BOOL LWndBase::set_Caption(char *lpText)
{
	if(m_hWnd == NULL || lpText == NULL)
		return FALSE;
#ifndef __WIN32__
	//XStoreName(pApp->GetDesktopWindow(),m_hWnd,lpText);
	 gdk_window_set_title(m_hWnd->window,lpText);
#else
	::SetWindowText(m_hWnd,lpText);
#endif		
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LWndBase::SetActive()
{
	if(m_hWnd == NULL)
		return FALSE;
#ifdef __WIN32__
	SetActiveWindow(m_hWnd);
#else
	//gtk_widget_grab_focus(m_hWnd);		
	gtk_window_present((GtkWindow *)m_hWnd);
#endif
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LWndBase::MoveTo(int x,int y,HWND hwndAfter)
{
	if(m_hWnd == NULL)
		return FALSE;
#ifdef __WIN32__
	return ::SetWindowPos(m_hWnd,hwndAfter,x,y,0,0,SWP_DRAWFRAME|SWP_FRAMECHANGED|SWP_NOSENDCHANGING|SWP_NOSIZE);
#else
	gtk_widget_set_uposition(m_hWnd,x,y);
	return TRUE;
#endif
}
//---------------------------------------------------------------------------
int LWndBase::SendMessage(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
#ifdef __WIN32__
	return ::SendMessage(m_hWnd,uMsg,wParam,lParam);
#else
	return OnWindowProc(uMsg,wParam,lParam);	
#endif
}
//---------------------------------------------------------------------------
BOOL LWndBase::PostMessage(UINT Msg,WPARAM wParam,LPARAM lParam)
{
	return ::PostMessage(m_hWnd,Msg,wParam,lParam);
}
//---------------------------------------------------------------------------
void LWndBase::Resize(int x,int y,int w,int h)
{
	if(m_hWnd == NULL)
		return;
	::SetWindowPos(m_hWnd,0,x,y,w,h,SWP_FRAMECHANGED|SWP_NOREPOSITION|SWP_DRAWFRAME);
}
#ifndef __WIN32__
//---------------------------------------------------------------------------
static void OnEnumChilds(GtkWidget *w,gpointer data)
{
	GtkRequisition sz,*p;	
	
	if(GTK_IS_MENU_BAR(w)){
		gtk_widget_size_request(w,&sz);		
		p = (GtkRequisition *)data;
		p->width += sz.width;
		p->height += sz.height;			
	}
	else if(GTK_IS_TOOLBAR(w)){
		gtk_widget_size_request(w,&sz);		
		p = (GtkRequisition *)data;
		p->width += sz.width;
		p->height += sz.height;
	}
	else if(GTK_IS_CONTAINER(w))
		gtk_container_foreach((GtkContainer*)w,OnEnumChilds,(gpointer)data);
}
#endif
//---------------------------------------------------------------------------
HDC LWndBase::BeginPaint(LPPAINTSTRUCT ps)
{
	return ::BeginPaint(m_hWnd,ps);
}
//---------------------------------------------------------------------------
BOOL LWndBase::EndPaint(LPPAINTSTRUCT ps)
{
	return ::EndPaint(m_hWnd,ps);
}
//---------------------------------------------------------------------------
BOOL LWndBase::SetWindowPos(HWND hWndInsertAfter,int X,int Y,int cx,int cy,UINT uFlags)
{
	return ::SetWindowPos(m_hWnd,hWndInsertAfter,X,Y,cx,cy,uFlags);
}
//---------------------------------------------------------------------------
BOOL LWndBase::IsWindow()
{
	return ::IsWindow(m_hWnd);
}
//---------------------------------------------------------------------------
LRESULT LWndBase::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   if(oldWndProc != NULL)
	    return ::CallWindowProc((WNDPROC)oldWndProc,m_hWnd,uMsg,wParam,lParam);
   return ::DefWindowProc(m_hWnd,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LRESULT CALLBACK LWndBase::WindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LWnd *p;
   
   switch(uMsg){
       case WM_INITDIALOG:                    
           p = (LWnd *)lParam;
           p->Assign(hwnd);
           SetWindowLong(hwnd,GWL_USERDATA,lParam);
       break;
       default:
           p = (LWnd *)GetWindowLong(hwnd,GWL_USERDATA);
       break;
   }
   if(p != NULL)
       return p->OnWindowProc(uMsg,wParam,lParam);
   return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LWnd::LWnd() : LWndBase()
{
}
//---------------------------------------------------------------------------
LWnd::~LWnd()
{
}
//---------------------------------------------------------------------------
BOOL LWnd::Create(LPCTSTR lpClassName,LPCTSTR lpWindowName,DWORD dwStyle,int x,int y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam)
{
#ifdef __WIN32__
   m_hWnd = CreateWindow(lpClassName,lpWindowName,dwStyle,x,y,nWidth,nHeight,
		hWndParent,hMenu,hInstance,lpParam);
  	if(m_hWnd == NULL)
  		return FALSE;
#else
	BOOL res;
	gint style;

	res = false;
	m_hWnd = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	if(m_hWnd == NULL)
		return FALSE;
   if(x == CW_USEDEFAULT)
		x = 100;
	if(y == CW_USEDEFAULT)
		y = 100;
	if(nWidth == CW_USEDEFAULT)
		nWidth = 0;
	if(nHeight == CW_USEDEFAULT)
		nHeight = 0;
	gtk_widget_set_events(m_hWnd,(GDK_ALL_EVENTS_MASK & ~(GDK_SCROLL_MASK|GDK_SUBSTRUCTURE_MASK|GDK_PROXIMITY_OUT_MASK|GDK_PROXIMITY_IN_MASK)));
	g_signal_connect((gpointer)m_hWnd,"event",G_CALLBACK(on_window_event),NULL);	
  	gtk_window_set_title(GTK_WINDOW(m_hWnd),lpWindowName);
  	gtk_window_set_position(GTK_WINDOW(m_hWnd),GTK_WIN_POS_CENTER);
  	gtk_window_set_resizable(GTK_WINDOW(m_hWnd),FALSE);
  	gtk_window_set_decorated(GTK_WINDOW(m_hWnd),TRUE);	
	gtk_widget_set_size_request(m_hWnd,nWidth,nHeight);	
	SetWindowLong(m_hWnd,GWL_WNDPROC,(LONG)WindowProc);
	gtk_widget_realize(m_hWnd);
	SetWindowStyle(dwStyle);
	if((dwStyle & WS_CHILD) == 0 && hMenu != NULL){
  		vbox = gtk_vbox_new(FALSE,0);
		if(vbox == NULL){
			gtk_widget_destroy(m_hWnd);
			m_hWnd = NULL;
			return FALSE;
		}
  		gtk_container_add(GTK_CONTAINER(m_hWnd),vbox);	
  		gtk_widget_show(vbox);
		::SetMenu(m_hWnd,hMenu);
  		gtk_box_pack_start(GTK_BOX(vbox),(HWND)hMenu,FALSE,FALSE,0);		
	}		
//	XSelectInput(gdk_x11_drawable_get_xdisplay(m_hWnd->window),gdk_x11_drawable_get_xid(m_hWnd->window),PointerMotionMask|ExposureMask |  ButtonPressMask);
	SetWindowLong(m_hWnd,GWL_STYLE,dwStyle);
#endif
  	SetWindowLong(m_hWnd,GWL_USERDATA,(LONG)this);
  	oldWndProc = (WNDPROC)SetWindowLong(m_hWnd,GWL_WNDPROC,(LONG)WindowProc);
 	return TRUE;
}
//---------------------------------------------------------------------------
HMENU LWnd::GetMenu()
{
   return ::GetMenu(m_hWnd);
}
//---------------------------------------------------------------------------
BOOL LWnd::SetMenu(HMENU p)
{
#ifndef __WIN32__
	gtk_widget_show((HWND)p);
	gtk_box_pack_start(GTK_BOX(vbox),(HWND)p,FALSE,FALSE,0);
#endif
	return ::SetMenu(m_hWnd,(HMENU)p);
}
//---------------------------------------------------------------------------
#ifndef __WIN32__
static void OnDragDrop(GtkWidget *widget,GdkDragContext *drag_context,gint x,gint y,GtkSelectionData *data,guint info,guint time,gpointer user_data)
{
	WNDPROC proc;

	proc = (WNDPROC)GetWindowLong(widget,GWL_WNDPROC);
	if(proc != NULL){
		proc(widget,WM_DROPFILES,(WPARAM)data,NULL);
	}
	gtk_drag_finish(drag_context,true,false,time);
}
#endif
//---------------------------------------------------------------------------
BOOL LWnd::CreateEx(DWORD dwExStyle,LPCTSTR lpClassName,LPCTSTR lpWindowName,DWORD dwStyle,int x,int y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HANDLE hInstance,LPVOID lpParam)
{
#ifdef __WIN32__
	m_hWnd = CreateWindowEx(dwExStyle,lpClassName,lpWindowName,dwStyle,x,y,nWidth,nHeight,
		hWndParent,hMenu,(HINSTANCE)hInstance,lpParam);
  	if(m_hWnd == NULL)
  		return FALSE;
  	SetWindowLong(m_hWnd,GWL_USERDATA,(LONG)this);
  	oldWndProc = (WNDPROC)SetWindowLong(m_hWnd,GWL_WNDPROC,(LONG)WindowProc);
  	return TRUE;
#else
	GtkTargetEntry target_entry[3] = {{"text/plain",0,0}};

	Create(lpClassName,lpWindowName,dwStyle,x,y,nWidth,nHeight,hWndParent,hMenu,(HINSTANCE)hInstance,lpParam);
	if(m_hWnd == NULL)
		return FALSE;
	if(dwExStyle & WS_EX_ACCEPTFILES){
		gtk_drag_dest_set(m_hWnd,(GtkDestDefaults)
			(GTK_DEST_DEFAULT_MOTION|GTK_DEST_DEFAULT_HIGHLIGHT|GTK_DEST_DEFAULT_DROP),
			target_entry,1,(GdkDragAction)(GDK_ACTION_MOVE));
		gtk_signal_connect(GTK_OBJECT(m_hWnd), "drag-data-received",GTK_SIGNAL_FUNC(OnDragDrop),m_hWnd);
 	}
	return TRUE;
#endif
}
//---------------------------------------------------------------------------
BOOL LWnd::Attack(HWND hwnd,BOOL bSubClass)
{
	if(!Assign(hwnd))
       return FALSE;
  	if(!bSubClass)
  		return TRUE;
  	SetWindowLong(hwnd,GWL_USERDATA,(LONG)this);
  	oldWndProc = (WNDPROC)SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WindowProc);
#ifndef __WIN32__
	gtk_widget_subclass(hwnd);
#endif
  	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LWnd::Destroy()
{
	if(m_hWnd == NULL)
  		return TRUE;
  	if(!::DestroyWindow(m_hWnd))
  		return FALSE;
  	m_hWnd = NULL;
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LWnd::AdjustWindowRect(LPRECT lpRect)
{
   return ::AdjustWindowRect(lpRect,GetWindowLong(m_hWnd,GWL_STYLE),GetMenu() != NULL);
}
//---------------------------------------------------------------------------
#ifndef __WIN32__
LRESULT LWnd::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg){
		case WM_CLOSE:
			pApp->PostQuitMessage();
		break;
		default:
			return LWndBase::OnWindowProc(uMsg,wParam,lParam);
	}
}
#endif
//---------------------------------------------------------------------------
LCBView::LCBView() : LWnd()
{
	parent = NULL;
   parent1 = NULL;
   m_hWndEdit = m_hWndLB = NULL;
	hMenu = NULL;
}
//---------------------------------------------------------------------------
LCBView::~LCBView()
{
#ifdef __WIN32__
	if(hMenu != NULL)
   	DestroyMenu(hMenu);
#endif
}
//---------------------------------------------------------------------------
void LCBView::Init(LPVOID p,WORD w,LPVOID p1)
{
	parent = p;
	parent1 = p1;
   wID = w;
   Attack(GetDlgItem(((LWnd *)p)->Handle(),wID));

#ifdef __WIN32__
	struct
	{
    	DWORD cbSize;
    	RECT  rcItem;
    	RECT  rcButton;
    	DWORD stateButton;
    	HWND  hwndCombo;
    	HWND  hwndItem;
    	HWND  hwndList;
	} cbInfo;
   BOOL WINAPI (*fn)(HWND,LPVOID);
	HINSTANCE hdll;

   if((hdll = LoadLibrary("user32.dll")) != NULL){
   	fn = (BOOL WINAPI (*)(HWND,LPVOID))GetProcAddress(hdll,"GetComboBoxInfo");
      	if(fn != NULL){
      		cbInfo.cbSize = sizeof(cbInfo);
      		if(fn(m_hWnd,&cbInfo)){
          		m_hWndEdit = cbInfo.hwndItem;
              	m_hWndLB = cbInfo.hwndList;
          	}
      	}
   	FreeLibrary(hdll);
   }
#else
	m_hWndEdit = gtk_bin_get_child((GtkBin *)m_hWnd);
#endif
   if(m_hWndEdit != NULL){
   		SetWindowLong(m_hWndEdit,GWL_USERDATA,(LONG)this);
#ifndef __WIN32__
		SetWindowLong(m_hWndEdit,GWL_WNDPROC,(LONG)WndProcEdit);
		g_signal_connect((gpointer)m_hWndEdit,"event",G_CALLBACK(on_window_event),NULL);	
#endif
   		oldWndProcEdit = (WNDPROC)SetWindowLong(m_hWndEdit,GWL_WNDPROC,(LONG)WindowProcEdit);
   }
   if(m_hWndLB != NULL){
   		SetWindowLong(m_hWndLB,GWL_USERDATA,(LONG)this);
   		oldWndProcLB = (WNDPROC)SetWindowLong(m_hWndLB,GWL_WNDPROC,(LONG)WindowProcLB);
   }
}
//---------------------------------------------------------------------------
LRESULT LCBView::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	return ::CallWindowProc((WNDPROC)oldWndProc,m_hWnd,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LRESULT LCBView::OnWindowProcLB(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	return ::CallWindowProc((WNDPROC)oldWndProcLB,m_hWndLB,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LRESULT CALLBACK LCBView::WindowProcLB(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LCBView *p;
   
   p = (LCBView *)GetWindowLong(hwnd,GWL_USERDATA);
   if(p != NULL)
       return p->OnWindowProcLB(uMsg,wParam,lParam);
   return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LRESULT LCBView::OnWindowProcEdit(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	return ::CallWindowProc((WNDPROC)oldWndProcEdit,m_hWndEdit,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
LRESULT CALLBACK LCBView::WindowProcEdit(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	LCBView *p;

   p = (LCBView *)GetWindowLong(hwnd,GWL_USERDATA);
   if(p != NULL)
   	return p->OnWindowProcEdit(uMsg,wParam,lParam);
   return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}

