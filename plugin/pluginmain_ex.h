#ifdef _MSC_VER
	typedef bool BOOL;
	typedef unsigned long DWORD;
	typedef void * LPVOID;
	typedef signed long LONG;
	typedef unsigned long * LPDWORD;
	typedef void * HDC;
	typedef void * HWND;
	typedef unsigned char BYTE;
	typedef char *	LPSTR;
	typedef struct
	{
		unsigned long a;
		unsigned short b,c;
		unsigned char d[8];
	} GUID;

	#define NULL 0
	#define GENERIC_READ	0
	#define FILE_BEGIN		1
	#define OPEN_EXISTING	2

	#define TRUE	1
	#define S_OK	0
	#define FALSE	0
#endif
