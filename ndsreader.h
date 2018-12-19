#include "ideastypes.h"
#include "elf.h"

//---------------------------------------------------------------------------
#ifndef __ndsreaderH__
#define __ndsreaderH__

#pragma pack(1)

struct Header
{
	char title[0xC];
	char gamecode[0x4];
	unsigned char makercode[2];
	unsigned char unitcode;
	unsigned char devicetype;		// type of device in the game card
	unsigned char devicecap;		// capacity
	unsigned char reserved1[0x9];
	unsigned char romversion;
	unsigned char reserved2;
	unsigned int arm9_rom_offset;
	unsigned int arm9_entry_address;
	unsigned int arm9_ram_address;
	unsigned int arm9_size;
	unsigned int arm7_rom_offset;
	unsigned int arm7_entry_address;
	unsigned int arm7_ram_address;
	unsigned int arm7_size;
	unsigned int fnt_offset;
	unsigned int fnt_size;
	unsigned int fat_offset;
	unsigned int fat_size;
	unsigned int arm9_overlay_offset;
	unsigned int arm9_overlay_size;
	unsigned int arm7_overlay_offset;
	unsigned int arm7_overlay_size;
  	unsigned long cardControl13;  // used in modes 1 and 3
  	unsigned long cardControlBF;  // used in mode 2
  	unsigned long bannerOffset;
  	unsigned short secureCRC16;
  	unsigned short readTimeout;
	unsigned int icon_title_offset;
	unsigned short secure_area_crc;
	unsigned short rom_control_info2;
	unsigned int offset_0x70;
	unsigned int offset_0x74;
	unsigned int offset_0x78;
	unsigned int offset_0x7C;
	unsigned int application_end_offset;			// rom size
	unsigned int rom_header_size;
	unsigned int offset_0x88;
	unsigned int offset_0x8C;
	unsigned int offset_0x90;
	unsigned int offset_0x94;
	unsigned int offset_0x98;
	unsigned int offset_0x9C;
	unsigned int offset_0xA0;
	unsigned int offset_0xA4;
	unsigned int offset_0xA8;
	unsigned int offset_0xAC;
	unsigned int offset_0xB0;
	unsigned int offset_0xB4;
	unsigned int offset_0xB8;
	unsigned int offset_0xBC;
	unsigned char logo[156];
	unsigned short logo_crc;
	unsigned short header_crc;
	unsigned char zero[92];
   unsigned int arm9ex_entry_address;
	unsigned int arm9ex_ram_address;
	unsigned int arm9ex_size;
   unsigned int arm9ex_rom_offset;
   unsigned int arm7ex_entry_address;
	unsigned int arm7ex_ram_address;
	unsigned int arm7ex_size;
   unsigned int areaex_size;
   unsigned int areaex_rom_offset;
   unsigned int arm7ex_rom_offset;
	unsigned char zero_1[28];
};
//---------------------------------------------------------------------------
struct FNTDir
{
   unsigned int entry_offset;
   unsigned short entry_file_id;
   unsigned short parent_id;
};
//---------------------------------------------------------------------------
struct FNTStrFile
{
   unsigned char entry;
};
//---------------------------------------------------------------------------
struct ROM_OVT
{
	unsigned int id;
	unsigned int ram_address;
   unsigned int ram_size;
   unsigned int bss_size;
   unsigned int sinit_init;
   unsigned int sinit_init_end;
   unsigned int file_id;
   unsigned int reserved;
};
#pragma pack(4)


#define EMUM_NDS			0
#define EMUM_GBA			1
#define EMUM_DSI			2

//---------------------------------------------------------------------------
struct IRomReader
{
   virtual int GetCurrentFunction(DWORD addr,void **f,void **u,void **l) PURE;
   virtual int GetFunctionFromName(const char *name,void **f ,void **u,int index) PURE;
   virtual int GetVariableFromName(const char *name,DWORD adr,void **obj,void **sym,void **u,int index) PURE;
   virtual DWORD Load(const char *name) PURE;
   virtual int Preload() PURE;
   virtual int get_Type() PURE;
   virtual int get_Type(const char *name) PURE;
   virtual DWORD get_Info(char *buf,DWORD dwLen) PURE;
   virtual BOOL DrawIcon(HWND hwnd) PURE;
   virtual int Decrypt() PURE;
};
//---------------------------------------------------------------------------
class LRomReader : public IRomReader,public LStream
{
public:
	LRomReader();
   virtual ~LRomReader();
   virtual int GetCurrentFunction(DWORD addr,void **f,void **u,void **l){return -1;};
   virtual int GetFunctionFromName(const char *name,void **f ,void **u,int index){return -1;};
   virtual int GetVariableFromName(const char *name,DWORD adr,void **obj,void **sym,void **u,int index){return -1;};
   virtual BOOL Open(DWORD dwStyle=GENERIC_READ,DWORD dwCreation=OPEN_EXISTING,DWORD dwFlags = 0);
   virtual void Close();
   virtual DWORD Read(LPVOID lpBuffer,DWORD dwBytes);
   virtual DWORD Write(LPVOID lpBuffer,DWORD dwBytes);
   virtual DWORD Seek(LONG dwDistanceToMove = 0,DWORD dwMoveMethod = FILE_BEGIN);
   virtual BOOL SeekToBegin();
   virtual BOOL SeekToEnd();
   virtual DWORD Size(LPDWORD lpHigh = NULL);
   virtual BOOL SetEndOfFile(DWORD dw);
   virtual DWORD GetCurrentPosition();
	virtual BOOL IsOpen();
   virtual void Release();
   virtual DWORD Load(const char *name){return FALSE;};
   virtual int Preload(){return EMUM_NDS;};
   virtual DWORD get_Info(char *buf,DWORD dwLen){return 0;};
   virtual BOOL DrawIcon(HWND hwnd){return FALSE;};
   virtual int Decrypt(){return 0;};
   void set_Stream(LStream *p){pFile = p;};
protected:
	LStream *pFile;
};
//---------------------------------------------------------------------------
class LGBAReader : public LRomReader
{
public:
	LGBAReader();
   ~LGBAReader();
   DWORD Load(const char *name);
   int get_Type(){return nType;};
   int get_Type(const char *name);
protected:
	int nType;
};
//---------------------------------------------------------------------------
class LBINReader : public LRomReader
{
public:
	LBINReader();
   ~LBINReader();
   DWORD Load(const char *name);
   int get_Type(){return 2;};
   int get_Type(const char *name){return get_Type();};
};
//---------------------------------------------------------------------------
class LNDSReader : public LRomReader
{
public:
	LNDSReader();
   ~LNDSReader();
	DWORD Load(const char *name);
   int get_Type();
   int get_Type(const char *name){return get_Type();};
   DWORD get_Info(char *buf,DWORD dwLen);
   BOOL DrawIcon(HWND hwnd);
   int Preload();
   int Decrypt();
protected:
	int arm9_offset;
   Header h;
   HBITMAP hBitmap;
};
//---------------------------------------------------------------------------
class LElfReader : public LRomReader
{
public:
   LElfReader();
   ~LElfReader();
   int GetCurrentFunction(DWORD addr,void **f,void **u,void **l);
   int GetFunctionFromName(const char *name,void **f ,void **u,int index);
   int GetVariableFromName(const char *name,DWORD adr,void **obj,void **sym,void **u,int index);
   DWORD Load(const char *name);
   void Close();
   int get_Type(){return 3;};
   int get_Type(const char *name){return get_Type();};
protected:
   LElfFile *pFile1;
};

#endif
