#include "ideastypes.h"
//---------------------------------------------------------------------------
#ifndef lwndH
#define lwndH

enum LAlign { alNone, alTop, alBottom, alLeft, alRight, alClient };
//---------------------------------------------------------------------------
class LWndBase
{
public:
	LWndBase();
  	virtual ~LWndBase(){};
  	inline HWND Handle(){return m_hWnd;};
  	BOOL MoveTo(int x,int y,HWND hwndAfter);
  	BOOL Assign(HWND hwnd);
  	BOOL set_Caption(char *lpText);
  	void set_Align(LAlign a){align = a;};
  	void Resize(int x,int y,int w,int h);
  	BOOL GetWindowRect(LPRECT pr);
  	BOOL GetClientRect(LPRECT pr);
  	virtual LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
  	int SendMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);
	BOOL PostMessage(UINT Msg,WPARAM wParam,LPARAM lParam);
   long SetWindowStyle(long style);
	void ClientToScreen(LPPOINT p);
	void ScreenToClient(LPPOINT p);
   BOOL Show(int nCmdShow);
	BOOL SetActive();
	HDC BeginPaint(LPPAINTSTRUCT ps);
	BOOL EndPaint(LPPAINTSTRUCT ps);
	virtual BOOL SetWindowPos(HWND hWndInsertAfter,int X,int Y,int cx,int cy,UINT uFlags);
	BOOL IsWindow();
protected:
	HWND m_hWnd;
   LAlign align;
	static LRESULT CALLBACK WindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	WNDPROC oldWndProc;
};
//---------------------------------------------------------------------------
class LWnd : public LWndBase
{
public:
	LWnd();
   virtual ~LWnd();
   virtual BOOL Destroy();
   virtual BOOL Create(LPCTSTR lpClassName,LPCTSTR lpWindowName,DWORD dwStyle,int x,int y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam);
   BOOL CreateEx(DWORD dwExStyle,LPCTSTR lpClassName,LPCTSTR lpWindowName,DWORD dwStyle,int x,int y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HANDLE hInstance,LPVOID lpParam);
   virtual BOOL Attack(HWND hwnd,BOOL bSubClass = TRUE);
   BOOL SetMenu(HMENU p);
   HMENU GetMenu();
   virtual BOOL AdjustWindowRect(LPRECT lpRect);
#ifndef __WIN32__
	virtual LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
protected:
	HWND vbox;
#endif
};
//---------------------------------------------------------------------------
class LCBView : public LWnd
{
public:
	LCBView();
  	virtual ~LCBView();
  	void Init(LPVOID p,WORD w,LPVOID p1=NULL);
	virtual LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
	virtual LRESULT OnWindowProcLB(UINT uMsg,WPARAM wParam,LPARAM lParam);
	virtual LRESULT OnWindowProcEdit(UINT uMsg,WPARAM wParam,LPARAM lParam);
protected:
 	HWND m_hWndEdit,m_hWndLB;
	static LRESULT CALLBACK WindowProcLB(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static LRESULT CALLBACK WindowProcEdit(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
  	LPVOID parent,parent1;
  	WORD wID;
  	HMENU hMenu;
  	WNDPROC oldWndProcLB,oldWndProcEdit;
};

#endif


