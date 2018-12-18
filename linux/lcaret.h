#ifndef __WIN32__

#ifndef __LCARETH__
#define __LCARETH__

class LCaret
{
public:
	LCaret();
	~LCaret();
	BOOL Show(HWND hWnd,BOOL bShow);
	BOOL Pos(gint x,gint y);
	BOOL OnDraw();
	BOOL Create(HWND hWnd,HBITMAP bitmap,int nWidth,int nHeight);
	BOOL Destroy();
	inline BOOL IsDraw(){return bDraw;};
	inline void set_Draw(BOOL status){bDraw = status;};
	inline HWND Window(){return m_hWnd;};
protected:
	HWND m_hWnd;
	guint uTimer;
	gint X,Y,Width,Height;
	BOOL bDraw;
	HBITMAP hBitmap;
};

extern LCaret *FindCaret(LCaret *caret);

#endif

#endif
