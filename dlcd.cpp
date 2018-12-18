#include "lds.h"
#include "resource.h"
#include "dsmem.h"
#include "touch.h"

//---------------------------------------------------------------------------
LDownLcd::LDownLcd() : LGPU()
{
   type = LCD_MAIN;
}
//---------------------------------------------------------------------------
LDownLcd::~LDownLcd()
{
}
//---------------------------------------------------------------------------
BOOL LDownLcd::OnMenuSelect(WORD wID)
{
	LONG l,lParam;
  	IMenu *menu;

	switch(wID){
    	case ID_LAYER_DOWN_0:
      	case ID_LAYER_DOWN_1:
      	case ID_LAYER_DOWN_2:
      	case ID_LAYER_DOWN_3:
      	case ID_LAYER_DOWN_OAM:
//      		layers[wID-ID_LAYER_DOWN_0].Visible ^= 1;
          	menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
           l = !menu->IsCheckedMenuItem(wID);
           menu->CheckMenuItem(wID,MF_BYCOMMAND|(l ? MF_CHECKED : MF_UNCHECKED));
			for(lParam = l = 0;l<7;l++){
               if(menu->IsCheckedMenuItem(l+ID_LAYER_DOWN_0))
                   lParam |= 1 << l;
           }
          	menu->Release();
           lParam |= IID_DOWNLCD << 16;
           pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_SHOWLAYERS,PIS_NOTIFYMASK,(LPARAM)&lParam);
			return TRUE;
       case ID_LAYER_DOWN_WINDOW0:
       case ID_LAYER_DOWN_WINDOW1:
//      		win[wID-ID_LAYER_DOWN_WINDOW0].Visible ^= 1;
          	menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
           l = !menu->IsCheckedMenuItem(wID);
           menu->CheckMenuItem(wID,MF_BYCOMMAND|(l ? MF_CHECKED : MF_UNCHECKED));
			for(lParam = l = 0;l<7;l++){
               if(menu->IsCheckedMenuItem(l+ID_LAYER_DOWN_0))
                   lParam |= 1 << l;
           }
           lParam |= IID_DOWNLCD << 16;
           pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_SHOWLAYERS,PIS_NOTIFYMASK,(LPARAM)&lParam);
       	return TRUE;
       case ID_LAYER_DOWN_ALL:
       	menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
			for(lParam = l = 0;l<7;l++){
               if(menu->IsCheckedMenuItem(l+ID_LAYER_DOWN_0))
                   lParam |= 1 << l;
           }
			lParam = lParam == 0 ? MF_CHECKED : MF_UNCHECKED;
          	for(l=0;l<5;l++)
              	menu->CheckMenuItem(l+ID_LAYER_DOWN_0,MF_BYCOMMAND|lParam);
           for(l=0;l<2;l++)
				menu->CheckMenuItem(l+ID_LAYER_DOWN_WINDOW0,MF_BYCOMMAND|lParam);
			for(lParam = l = 0;l<7;l++){
               if(menu->IsCheckedMenuItem(l+ID_LAYER_DOWN_0))
                   lParam |= 1 << l;
           }
           menu->Release();
           lParam |= IID_DOWNLCD << 16;
           pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_SHOWLAYERS,PIS_NOTIFYMASK,(LPARAM)&lParam);
       	return TRUE;
   	default:
       	return FALSE;
   }
}
//---------------------------------------------------------------------------
void LDownLcd::Reset()
{
	IMenu *menu;

   set_PowerMode();
	LGPU::Reset();
  	Touch_reset();
  	menu = ds.CoCreateInstance_Menu(upLcd.GetMenu());
	if(menu != NULL){
       for(int i = 0;i<5;i++)
       	menu->CheckMenuItem(i+ID_LAYER_DOWN_0,MF_BYCOMMAND|(layers[i].Visible ? MF_CHECKED : MF_UNCHECKED));
   	menu->Release();
	}
}
//---------------------------------------------------------------------------
void LDownLcd::set_PowerMode()
{
   if(ds.get_EmuMode() == 0){
       reg = io_mem;
       vram_obj = video_mem + 0x60000;
   }
   else{
       reg = io_mem7;
       vram_obj = video_mem + 0x10000;
   }
	pal_bg = pal_mem;
  	pal_obj = pal_bg + 0x200;
  	obj = obj_mem;
}
//---------------------------------------------------------------------------
void LDownLcd::Release()
{
   LCanvas::ReleaseDC();
   LWnd::Destroy();
}
//---------------------------------------------------------------------------
BOOL LDownLcd::Create()
{
#ifdef __WIN32__
	if(!CreateWnd())
  		return FALSE;
   if(!LCanvas::GetDC(m_hWnd))
       return FALSE;
	return Show(SW_SHOW);
#else
	m_hWnd = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	if(m_hWnd == NULL)
		return FALSE;
	gtk_widget_set_events(m_hWnd,(GDK_ALL_EVENTS_MASK & ~(GDK_SCROLL_MASK|GDK_SUBSTRUCTURE_MASK|GDK_PROXIMITY_OUT_MASK|GDK_PROXIMITY_IN_MASK)));
	g_signal_connect((gpointer)m_hWnd,"event",G_CALLBACK(on_window_event),NULL);
  	gtk_window_set_title(GTK_WINDOW(m_hWnd),"iDeaS Emulator Bottom");
  	gtk_window_set_position(GTK_WINDOW(m_hWnd), GTK_WIN_POS_CENTER);
  	gtk_window_set_resizable(GTK_WINDOW(m_hWnd), FALSE);
  	gtk_window_set_decorated(GTK_WINDOW(m_hWnd), TRUE);

	gtk_widget_set_size_request(m_hWnd,256*fScale,192*fScale);
	SetWindowLong(m_hWnd,GWL_WNDPROC,(LONG)WindowProc);
  	SetWindowLong(m_hWnd,GWL_USERDATA,(LONG)(LWnd *)this);
  	oldWndProc = (WNDPROC)SetWindowLong(m_hWnd,GWL_WNDPROC,(LONG)WindowProc);
	if(!LCanvas::GetDC(m_hWnd))
		return FALSE;
	return TRUE;
#endif
}
//---------------------------------------------------------------------------
BOOL LDownLcd::Map()
{
#ifndef __WIN32__
	GdkColor color={0,0,0,255};

	gtk_widget_realize(m_hWnd);
	gdk_window_set_background(m_hWnd->window,&color);
	SetWindowLong(m_hWnd,GWL_STYLE,WS_POPUP|WS_DLGFRAME);
	gtk_window_set_transient_for(GTK_WINDOW(m_hWnd),GTK_WINDOW(upLcd.Handle()));
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW (m_hWnd),TRUE);
	gtk_window_set_icon((GtkWindow *)m_hWnd,LoadIcon(hInst,MAKEINTRESOURCE(2)));
	Reposition();
//	XSelectInput(gdk_x11_drawable_get_xdisplay(m_hWnd->window),gdk_x11_drawable_get_xid(m_hWnd->window),0x13FFFF);
#endif
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LDownLcd::CreateWnd()
{
#ifdef __WIN32__
   if(!CreateEx(WS_EX_ACCEPTFILES,"IDEAS","iDeaS Emulator Bottom",WS_POPUP|WS_DLGFRAME,
       0,0,256,192,ds.Handle(),NULL,hInst,NULL))
           return FALSE;
#else
  	GdkColor color={0,0,0,255};

	if(!LWnd::CreateEx(WS_EX_ACCEPTFILES,"IDEAS","iDeaS Emulator Bottom",WS_POPUP|WS_DLGFRAME,0,0,256,192,
	    NULL,NULL,NULL,NULL))
        return FALSE;
	gdk_window_set_background(m_hWnd->window,&color);
	gtk_window_set_transient_for(GTK_WINDOW(m_hWnd),GTK_WINDOW(upLcd.Handle()));
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (m_hWnd),TRUE);
	gtk_window_set_icon((GtkWindow *)m_hWnd,LoadIcon(hInst,MAKEINTRESOURCE(2)));
#endif
	Reposition();
  	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LDownLcd::Reposition()
{
	RECT rc,rc1,rc2;

	GetWindowRect(&rc);
  	GetClientRect(&rc1);
	rc1.right = ((rc.right - rc.left) - rc1.right) + 256 * fScale;
  	rc1.bottom = ((rc.bottom - rc.top) - rc1.bottom) + 192 * fScale;
	upLcd.GetWindowRect(&rc2);
  	rc.left = rc2.left + ((rc1.right - (rc2.right - rc2.left)) / 2);
  	rc.top = rc2.bottom;
  	Resize(rc.left,rc.top,rc1.right,rc1.bottom);
}
//---------------------------------------------------------------------------
BOOL LDownLcd::Load(LStream *pFile,int ver)
{
   VideoPlugList *list;
   PlugIn *plugin;
   HMENU hMenu;
   LPARAM lParam;
	int l;

   if(!LGPU::Load(pFile,ver))
       return FALSE;
   hMenu = upLcd.GetMenu();
	if(hMenu == NULL)
   	return FALSE;
   list = (VideoPlugList *)pPlugInContainer->get_PlugInList(PIL_VIDEO);
   if(list == NULL)
   	return FALSE;
   plugin = list->get_Active2DPlugin();
   if(plugin == NULL)
   	return FALSE;
   lParam = (IID_DOWNLCD << 16) | 0xFFFF;
   plugin->NotifyState(PNMV_SHOWLAYERS,PIS_NOTIFYMASK,(LPARAM)&lParam);
   for(l=0;l<5;l++)
      	CheckMenuItem(hMenu,l+ID_LAYER_DOWN_0,MF_BYCOMMAND|(lParam & (1 << l)) ? MF_CHECKED : MF_UNCHECKED);
   for(l=0;l<2;l++)
      	CheckMenuItem(hMenu,l+ID_LAYER_DOWN_WINDOW0,MF_BYCOMMAND|(lParam & (1 << (5 + l))) ? MF_CHECKED : MF_UNCHECKED);
   return TRUE;
}
