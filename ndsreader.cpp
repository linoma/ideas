#include "ndsreader.h"
#include "lds.h"
#include "card.h"
#include "cheats.h"
#include "cbds.h"

//---------------------------------------------------------------------------
LRomReader::LRomReader()
{
   pFile = NULL;
}
//---------------------------------------------------------------------------
LRomReader::~LRomReader()
{
	LString nameFile;

	if(pFile != NULL)
   	pFile->Release();
   pFile = NULL;
   nameFile.Length(MAX_PATH+1);
   if(ds.BuildFileName(PT_SAVEGAME,nameFile.c_str(),MAX_PATH))
		EEPROM_Save(nameFile.c_str());
}
//---------------------------------------------------------------------------
BOOL LRomReader::Open(DWORD dwStyle,DWORD dwCreation,DWORD dwFlags)
{
	if(pFile == NULL)       
   	return FALSE;
   return pFile->Open(GENERIC_READ,OPEN_EXISTING,dwFlags);
}
//---------------------------------------------------------------------------
void LRomReader::Close()
{
	if(pFile != NULL)
   	pFile->Close();
}
//---------------------------------------------------------------------------
DWORD LRomReader::Read(LPVOID lpBuffer,DWORD dwBytes)
{
	if(pFile == NULL)
   	return 0;
   return pFile->Read(lpBuffer,dwBytes);
}
//---------------------------------------------------------------------------
DWORD LRomReader::Write(LPVOID lpBuffer,DWORD dwBytes)
{
	if(pFile == NULL)
   	return 0;
   return pFile->Write(lpBuffer,dwBytes);
}
//---------------------------------------------------------------------------
DWORD LRomReader::Seek(LONG dwDistanceToMove,DWORD dwMoveMethod)
{
	if(pFile == NULL)
   	return 0xFFFFFFFF;
   return pFile->Seek(dwDistanceToMove,dwMoveMethod);
}
//---------------------------------------------------------------------------
BOOL LRomReader::SeekToBegin()
{
	if(pFile == NULL)
   	return 0xFFFFFFFF;
	return pFile->SeekToBegin();
}
//---------------------------------------------------------------------------
BOOL LRomReader::SeekToEnd()
{
	if(pFile == NULL)
   	return 0xFFFFFFFF;
	return pFile->SeekToEnd();
}
//---------------------------------------------------------------------------
DWORD LRomReader::Size(LPDWORD lpHigh)
{
	if(pFile == NULL)
   	return 0xFFFFFFFF;
   return pFile->Size(lpHigh);
}
//---------------------------------------------------------------------------
BOOL LRomReader::SetEndOfFile(DWORD dw)
{
	if(pFile == NULL)
   	return FALSE;
   return pFile->SetEndOfFile(dw);
}
//---------------------------------------------------------------------------
DWORD LRomReader::GetCurrentPosition()
{
	if(pFile == NULL)
   	return 0xFFFFFFFF;
	return pFile->GetCurrentPosition();
}
//---------------------------------------------------------------------------
BOOL LRomReader::IsOpen()
{
	if(pFile == NULL)
   	return FALSE;
   return pFile->IsOpen();
}
//---------------------------------------------------------------------------
void LRomReader::Release()
{
	delete this;
}
//---------------------------------------------------------------------------
LGBAReader::LGBAReader() : LRomReader()
{
	nType = 1;
}
//---------------------------------------------------------------------------
LGBAReader::~LGBAReader()
{
   int i;

   for(i=0;i<0x200;i++){
   	if(rom_pack[i] != NULL)
       	LocalFree(rom_pack[i]);
       rom_pack[i] =  NULL;
   }
}
//---------------------------------------------------------------------------
int LGBAReader::get_Type(const char *name)
{
   LString s;
   
   s = LString(name).LowerCase();
   if(s.Pos("ds.gba") > 0)
       nType = 1;
   else if(s.Pos(".gba") > 0)
       nType = 10;
   return nType;
}
//---------------------------------------------------------------------------
DWORD LGBAReader::Load(const char *name)
{
	int i,n;

	if(!SeekToBegin())
   	return 0;
   n = Size() >> 16;
   for(i=0;i<=n;i++){
   	if((rom_pack[i] = (u8 *)LocalAlloc(LPTR,0x10000)) == NULL)
       	return FALSE;
       Read(rom_pack[i],0x10000);
   }
   for(;i<0x200;i++)
		rom_pack[i] = NULL;
	arm7.InitPipeLine(0x8000000);
   get_Type(name);
	return 1;
}
//---------------------------------------------------------------------------
LBINReader::LBINReader() : LRomReader()
{
}
//---------------------------------------------------------------------------
LBINReader::~LBINReader()
{
}
//---------------------------------------------------------------------------
DWORD LBINReader::Load(const char *name)
{
	DWORD dw;
	LFile *p;

	if(!SeekToBegin())
   	return 0;
   Read(ext_mem,Size());
  	arm9.InitPipeLine(0x02000000);
   dw = 0x00010000;
	if(name[lstrlen(name) + 1]){
   	if((p = new LFile(&name[lstrlen(name)+1])) != NULL){
       	p->Read(&int_mem2[(u16)0x3800000],p->Size());
       	delete p;
       	arm7.InitPipeLine(0x3800000);
           dw |= 0x0001;
       }
   }
	return dw;
}
//---------------------------------------------------------------------------
LNDSReader::LNDSReader() : LRomReader()
{
	arm9_offset = 0;
   hBitmap = NULL;
}
//---------------------------------------------------------------------------
LNDSReader::~LNDSReader()
{
   if(hBitmap != NULL)
   	DeleteObject(hBitmap);
}
//---------------------------------------------------------------------------
DWORD LNDSReader::get_Info(char *buf,DWORD dwLen)
{
   char *p,c[40],cr[]={13,10};
   DWORD dw,dw1,dw2;
   int i;

	if(buf == NULL || dwLen == 0 || pFile == NULL)
   	return 0;
   if(h.bannerOffset < 0x8000)
   	return 0;
   lstrcpy(buf,((LFile *)pFile)->FileName());
   dw = lstrlen(buf);
   p = buf + lstrlen(buf);
   *p++ = 0;
   dw++;

   dw2 = pFile->GetCurrentPosition();
   pFile->Seek(h.bannerOffset+0x240);
   for(dw1=0,i=0;i<6;){
		while(pFile->Read(&c,2) == 2){
       	if(*((short *)&c) == 0)
           	break;
           *p++ = c[0];
           dw1++;
       }
       *p++ = 0;
       dw1++;
       pFile->Seek(h.bannerOffset+0x240+ ++i * 256);
   }
	dw += dw1;
   pFile->Seek(dw2);
   memset(c,0,40);
	lstrcpyn(c,h.title,13);
   c[12] = 0;
   wsprintf(p,"Title : %s\r\n",c);
   p += (dw1 = lstrlen(p));
   dw += dw1;
	memset(c,0,40);
	lstrcpyn(c,h.gamecode,5);
   c[4] = 0;
   wsprintf(p,"Game Code : %s\r\n",c);
   p += (dw1 = lstrlen(p));
   dw+=dw1;

   wsprintf(p,"Maker Code : %s\r\n",h.makercode);
   p += (dw1 = lstrlen(p));
   dw+=dw1;

	wsprintf(p,"Chip Size : %dMB(%dMBit)\r\n",(0x80 << h.devicecap) >> 10,(0x80 << h.devicecap)>>7);
   p += (dw1 = lstrlen(p));
   dw+=dw1;

	wsprintf(p,"ARM9 binary ROM offset : 0x%08X\r\n",h.arm9_rom_offset);
   p += (dw1 = lstrlen(p));
   dw+=dw1;

	wsprintf(p,"ARM9 binary entry address : 0x%08X\r\n",h.arm9_entry_address);
   p += (dw1 = lstrlen(p));
   dw+=dw1;

	wsprintf(p,"ARM9 binary start address : 0x%08X\r\n",h.arm9_ram_address);
   p += (dw1 = lstrlen(p));
   dw+=dw1;

	wsprintf(p,"ARM9 binary size : %d bytes\r\n",h.arm9_size);
   p += (dw1 = lstrlen(p));
   dw+=dw1;

	wsprintf(p,"ARM7 binary ROM offset : 0x%08X\r\n",h.arm7_rom_offset);
   p += (dw1 = lstrlen(p));
   dw+=dw1;

	wsprintf(p,"ARM7 binary entry address : 0x%08X\r\n",h.arm7_entry_address);
   p += (dw1 = lstrlen(p));
   dw+=dw1;

	wsprintf(p,"ARM7 binary start address : 0x%08X\r\n",h.arm7_ram_address);
   p += (dw1 = lstrlen(p));
   dw+=dw1;

	wsprintf(p,"ARM7 binary size : %d bytes\r\n",h.arm7_size);
   p += (dw1 = lstrlen(p));
   dw+=dw1;

	wsprintf(p,"Game ID : 0x%08X\r\n",cheatsManager.get_GameID());
   p += (dw1 = lstrlen(p));
   dw+=dw1;

	return dw;
}
//---------------------------------------------------------------------------
BOOL LNDSReader::DrawIcon(HWND hwnd)
{
   BITMAPINFO bminfo;
	DWORD dw;
   HDC hdc;
   LPBYTE buf;
   USHORT *screen,pal[16];
   int x,y;

   if(hBitmap != NULL){
       DeleteObject(hBitmap);
       hBitmap = NULL;
   }
   if(h.bannerOffset < 0x8000)
   	return FALSE;
   hdc = CreateCompatibleDC(NULL);
   if(hdc == NULL)
   	return FALSE;
   dw = pFile->GetCurrentPosition();
	buf = NULL;
	ZeroMemory(&bminfo,sizeof(BITMAPINFO));
   bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bminfo.bmiHeader.biWidth = 32;
   bminfo.bmiHeader.biHeight = -32;
   bminfo.bmiHeader.biPlanes = 1;
   bminfo.bmiHeader.biBitCount = 16;
   bminfo.bmiHeader.biCompression = BI_RGB;
   hBitmap = CreateDIBSection(hdc,&bminfo,DIB_RGB_COLORS,(LPVOID *)&screen,NULL,0);
	if(hBitmap == NULL)
   	goto DrawIcon_ex;
   pFile->Seek(h.bannerOffset+0x20+0x200);
	pFile->Read(pal,sizeof(pal));
	for(x=0;x<16;x++){
       y = pal[x];
		pal[x] = (USHORT)(((y & 0x1F) << 10)|(y & 0x3E0)|((y & 0x7C00) >> 10));
   }
	pFile->Seek(h.bannerOffset+0x20);
	buf = (LPBYTE)LocalAlloc(LPTR,512);
	if(buf == NULL)
		goto DrawIcon_ex;
	pFile->Read(buf,512);
	for(y = 0; y < 32; y++){
       for(x = 0; x < 32; x++){
           int tilenum = (((y / 8) * 4) + (x / 8));
			int tilex = (x % 8);
			int tiley = (y % 8);
			int mapoffset = ((tilenum * 64) + (tiley * 8) + tilex);

			BYTE val = buf[mapoffset>>1];

			if(mapoffset & 1)
               val >>= 4;
			screen[(y * 32) + x] = pal[val & 0xF];
       }
   }
   StretchDIBits(hdc,0,0,32,32,0,0,32,32,screen,&bminfo,DIB_RGB_COLORS,SRCCOPY);
   ::SendMessage(hwnd,STM_SETIMAGE,(WPARAM)IMAGE_BITMAP,(LPARAM)hBitmap);
DrawIcon_ex:
	pFile->Seek(dw);
   if(buf != NULL)
   	LocalFree(buf);
	if(hdc != NULL)
   	DeleteDC(hdc);
	return TRUE;
}
//---------------------------------------------------------------------------
int LNDSReader::Preload()
{
   if(!SeekToBegin() || Read(&h,sizeof(Header)) != sizeof(Header))
   	return (DWORD)-1;
   arm9_offset = h.arm9_rom_offset;
   return h.unitcode == 2 ? EMUM_DSI : EMUM_NDS;
}
//---------------------------------------------------------------------------
int LNDSReader::Decrypt()
{
   u32 *ram;
   unsigned char *data;

   ram = (u32 *)&ext_mem[h.arm9_ram_address & 0x3FFFFF];
   //decrypt area
   if(h.unitcode < 0 || h.unitcode > 3)
       return -1;
   if(h.arm9_rom_offset < 0x4000)
       return -2;
   if(!ram[0x1000] && !ram[0x1001])
       return -3;
   if(ram[0x1000] == 0xE7FFDEFF && ram[0x1001] == 0xE7FFDEFF)
       return -4;
   data = (unsigned char *)LocalAlloc(LPTR,0x5000);
   if(data == NULL)
       return -5;
/*   SeekToBegin();
   Read(data,16384);
	for(int i=0x200;i<0x4000;i++){
       if(data[i]){
           LocalFree(data);
           return -6;
       }
   }*/
   memcpy(data,ram,0x4000);
	if(decrypt_arm9(*(u32 *)h.gamecode,data)){
       memcpy(ram,data,0x800);
       LocalFree(data);
   }
   return 0;
}
//---------------------------------------------------------------------------
DWORD LNDSReader::Load(const char *name)
{
   int n,i,start;

   if(Preload() < 0)
       return 0;
//   CopyMemory(&ext_mem2[0xE00],&h,0x160);
   CopyMemory(&ext_mem[ulMMMask - 0x1FF],&h,0x160);
   ext_mem[ulMMMask - 0x3BF] = 1;
   LRomReader::Seek(h.arm9_rom_offset);
   Read(&ext_mem[h.arm9_ram_address & 0x3FFFFF],h.arm9_size);
 	arm9.InitPipeLine(h.arm9_entry_address);
   Decrypt();
   if(ds.get_UseFATxEFS() && h.arm9_rom_offset < 0x4000){
       if((rom_pack[0] = (u8 *)LocalAlloc(LPTR,0x10000)) == NULL)
           return FALSE;
       memcpy(rom_pack[0],&h,sizeof(h));
       LRomReader::Seek(h.fnt_offset);
       i = h.fnt_offset >> 16;
       n = h.offset_0x78 >> 16;
       start = h.fnt_offset - (i << 16);
       for(;i<=n;i++){
           if((rom_pack[i] = (u8 *)LocalAlloc(LPTR,0x10000)) == NULL)
               return FALSE;
           Read(&rom_pack[i][start],0x10000 - start);
           start = 0;
       }
       for(;i<0x200;i++)
           rom_pack[i] = NULL;
   }
   LRomReader::Seek(h.arm7_rom_offset);
	if((h.arm7_ram_address & 0x0f000000) == 0x03000000){
       if((h.arm7_ram_address & 0x00F00000) == 0x00700000)
           Read(&int_mem[(h.arm7_ram_address & 0x7FFF)],h.arm7_size);
       else
   	    Read(&int_mem2[(u16)h.arm7_ram_address],h.arm7_size);
   }
   else
   	Read(&ext_mem[(h.arm7_ram_address & 0x3FFFFF)],h.arm7_size);
 	arm7.InitPipeLine(h.arm7_entry_address);
   return 0x00010001;
}
//---------------------------------------------------------------------------
int LNDSReader::get_Type()
{
	if(arm9_offset < 0x4000)
   	return 5;
   return 3;//h.unitcode == 0 ? 3 : 4;
}
//---------------------------------------------------------------------------
LElfReader::LElfReader() : LRomReader()
{
   pFile1 = NULL;
}
//---------------------------------------------------------------------------
LElfReader::~LElfReader()
{
   if(pFile1 != NULL)
       delete pFile1;
}
//---------------------------------------------------------------------------
void LElfReader::Close()
{
   LRomReader::Close();
   if(pFile1 != NULL)
       delete pFile1;
   pFile1 = NULL;
}
//---------------------------------------------------------------------------
int LElfReader::GetFunctionFromName(const char *name,void **f ,void **u,int index)
{
   int res;

   res = -1;
   if(pFile != NULL && (index == 9 || index == -1))
       res = ((LElfFile *)pFile)->GetFunctionFromName(name,f,u);
   if(res == -1 && pFile1 != NULL && (index == 7 || index == -1))
       res = pFile1->GetFunctionFromName(name,f,u);
  	return res;
}
//---------------------------------------------------------------------------
int LElfReader::GetVariableFromName(const char *name,DWORD adr,void **obj,void **sym,void **u,int index)
{
   int res;

   res = -1;
   if(pFile != NULL && (index == 9 || index == -1))
       res = ((LElfFile *)pFile)->GetVariableFromName(name,adr,obj,sym,u);
   if(res == -1 && pFile1 != NULL && (index == 7 || index == -1))
       res = pFile1->GetVariableFromName(name,adr,obj,sym,u);
  	return res;
}
//---------------------------------------------------------------------------
int LElfReader::GetCurrentFunction(DWORD addr,void **f,void **u,void **l)
{
   int res;

   res = -1;
   if(pFile != NULL)
       res = ((LElfFile *)pFile)->GetCurrentFunction(addr,f,u,l);
   if(res == -1 && pFile1 != NULL)
       res = pFile1->GetCurrentFunction(addr,f,u,l);
  	return res;
}
//---------------------------------------------------------------------------
DWORD LElfReader::Load(const char *name)
{
	DWORD dwRes,dw;
   int i;

   dwRes = 0;
   if(!Open())
   	return 0;
   dwRes = ((LElfFile *)pFile)->Load();
   if(!dwRes)
       return 0;
   arm9.InitPipeLine(dwRes);
   dwRes = 0x10000;
#ifdef _DEBPRO
   if(name[i = lstrlen(name) + 1]){
       pFile1 = new LElfFile(&name[i]);
       if(pFile1 != NULL && pFile1->Open()){
           dw = pFile1->Load();
           if(dw){
               arm7.InitPipeLine(dw);
               dwRes |= 1;
           }
       }
   }
#endif
	return dwRes;
}


