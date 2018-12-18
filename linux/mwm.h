#ifndef __WIN32__

#ifndef __mwmh__
#define __mwmh__

	
typedef struct {
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long input_mode;
	unsigned long status;
}MotifWmHints, MwmHints;

typedef struct {
	long flags;
	Window wm_window;
} MotifWmInfo, MwmInfo;

#define MWM_HINTS_FUNCTIONS   1
#define MWM_HINTS_DECORATIONS 2
#define MWM_HINTS_INPUT_MODE  4
#define MWM_HINTS_STATUS      8
	
#define MWM_FUNC_ALL          0x01
#define MWM_FUNC_RESIZE       0x02
#define MWM_FUNC_MOVE         0x04
#define MWM_FUNC_MINIMIZE     0x08
#define MWM_FUNC_MAXIMIZE     0x10
#define MWM_FUNC_CLOSE        0x20

#define MWM_DECOR_ALL         0x01
#define MWM_DECOR_BORDER      0x02
#define MWM_DECOR_RESIZEH     0x04
#define MWM_DECOR_TITLE       0x08
#define MWM_DECOR_MENU        0x10
#define MWM_DECOR_MINIMIZE    0x20
#define MWM_DECOR_MAXIMIZE    0x40
	
#define MWM_INPUT_MODELESS                  0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL              2
#define MWM_INPUT_FULL_APPLICATION_MODAL    3
#define MWM_INPUT_APPLICATION_MODAL         1

#define MWM_TEAROFF_WINDOW 1
	
#define MWM_INFO_STARTUP_STANDARD 1
#define MWM_INFO_STARTUP_CUSTOM   2

#define _XA_MOTIF_WM_HINTS    "_MOTIF_WM_HINTS"
#define _XA_MOTIF_WM_MESSAGES "_MOTIF_WM_MESSAGES"
#define _XA_MOTIF_WM_OFFSET   "_MOTIF_WM_OFFSET"
#define _XA_MOTIF_WM_MENU     "_MOTIF_WM_MENU"
#define _XA_MOTIF_WM_INFO     "_MOTIF_WM_INFO"

#define _XA_MWM_HINTS    _XA_MOTIF_WM_HINTS
#define _XA_MWM_MESSAGES _XA_MOTIF_WM_MESSAGES
#define _XA_MWM_MENU     _XA_MOTIF_WM_MENU
#define _XA_MWM_INFO     _XA_MOTIF_WM_INFO

#endif

#endif

