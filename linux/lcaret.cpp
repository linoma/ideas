#include "ideastypes.h"
#include "lcaret.h"

#ifndef __WIN32__

static GList *carets;
//---------------------------------------------------------------------------
static gboolean timeout_callback(gpointer data)
{
	if(data != NULL)
		return ((LCaret *)data)->OnDraw();
   	return FALSE;
}
//---------------------------------------------------------------------------
LCaret::LCaret()
{
	uTimer = 0;
	X = Y = 0;
	m_hWnd = NULL;
	bDraw = FALSE;
	hBitmap = NULL;
	carets = g_list_append(carets,(gpointer)this);
}
//---------------------------------------------------------------------------
LCaret::~LCaret()
{
	Destroy();
}
//---------------------------------------------------------------------------
BOOL LCaret::Show(HWND hWnd,BOOL bShow)
{
	if(bShow){
		if(hWnd == NULL)
			return FALSE;
		if(uTimer == 0)
			uTimer = gdk_threads_add_timeout(350,timeout_callback,(gpointer)this);
		if(uTimer == 0)
			return FALSE;
		m_hWnd = hWnd;
	}
	else{
		if(uTimer == 0)
			return FALSE;
		g_source_remove(uTimer);
		uTimer = 0;
	}
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LCaret::OnDraw()
{
	GdkGC *gc;
	GdkColor color;

	if(m_hWnd == NULL)
		return FALSE;
	gc = gdk_gc_new(m_hWnd->window);
	gdk_gc_set_function(gc,GDK_INVERT);
	color.red = color.green = color.blue = 1;
	gdk_gc_set_rgb_fg_color(gc,&color);
	gdk_draw_rectangle(m_hWnd->window,gc,true,X+1,Y+3,Width,Height);
	gdk_gc_unref(gc);
	bDraw = !bDraw;
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LCaret::Pos(gint x,gint y)
{
	X = x;
	Y = y;
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LCaret::Create(HWND hWnd,HBITMAP bitmap,int nWidth,int nHeight)
{
	m_hWnd = hWnd;
	Width = nWidth;
	Height = nHeight;
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LCaret::Destroy()
{
	Show(NULL,FALSE);
	if(carets != NULL){
		if(g_list_index(carets,(gconstpointer)this) != -1){
			carets = g_list_remove(carets,(gconstpointer)this);
			if(carets != NULL && g_list_length(carets) < 1){
				g_list_free(carets);
				carets = NULL;
			}
		}
	}
	return TRUE;
}
//---------------------------------------------------------------------------
LCaret *FindCaret(LCaret *caret)
{
	gint i,length;
	LCaret *obj;

	if(carets == NULL)
		return NULL;
	length = g_list_length(carets);
	for(i=0;i<length;i++){
		obj = (LCaret *)g_list_nth_data(carets,i);
		if(obj == caret)
			return obj;
	}
	return NULL;
}

#endif
