#include "ideastypes.h"
#include "util.h"
#include "lstring.h"
#include "fstream.h"
#include "lopensavedlg.h"


static u16 crc16tab[] = {
	0x0000,0xCC01,0xD801,0x1400,0xF001,0x3C00,0x2800,0xE401,0xA001,0x6C00,
   0x7800,0x0B401,0x5000,0x9C01,0x8801,0x4400
};
//---------------------------------------------------------------------------
BOOL ControlMemoryBP(unsigned long address,unsigned long access,unsigned long value)
{
#ifdef _DEBUG
	return debugDlg.ControlMemoryBP(address,access,value);
#else
	return FALSE;
#endif
}
//---------------------------------------------------------------------------
DWORD StrToHex(char *string)
{
   DWORD iValue,iValue2;
   int i,i1;
   char *p,car,*p1;

   strlwr(string);
   while(*string == 32)
   	string++;
   if((string[0] == '0' || strpbrk(string,"abcdefx") !=  NULL)){
       if((p = strchr(string,'x')) != NULL){
			if((p1 = strpbrk(string,"+-*/")) != NULL){
           	if(p1 < p)
               	p = string;
               else
               	p++;
           }
           else
           	p++;
       }
       else
           p = string;
       i = lstrlen(p);
       if(i > 8){
       	i = 8;
       	if((p1 = strpbrk(string,"+-*/")) != NULL)
           	i = (int)((DWORD)p1 - (DWORD)p);
           while(p[i-1] == 32)	i--;
       }
       for(iValue = 0;i>0;i--){
           car = *p++;
           if((car >= 'a' && car < 'g'))
               i1 = (car - 87);
           else if(car > 47 && car < 58)
               i1 = car - 48;
           else
               i1 = 0;
           iValue = (iValue << 4) + i1;
       }
       if((p = strpbrk(string,"+-*/")) != NULL){
			iValue2 = StrToHex(p+1);
           switch(p[0]){
           	case '+':
               	iValue += iValue2;
               break;
               case '-':
               	iValue -= iValue2;
               break;
               case '*':
               	iValue *= iValue2;
               break;
               case '/':
               	iValue /= iValue2;
               break;
           }
       }
   }
   else
       iValue = atoi(string);
   return iValue;
}
//---------------------------------------------------------------------------
BOOL ShowOpenDialog(char *lpstrTitle,char *lpstrFilter,HWND hwnd,char *lpstrFileName,int nMaxFile,DWORD dwFlags)
{
	LOpenSaveDlg dlg;

	return dlg.ShowOpen(lpstrTitle,lpstrFilter,hwnd,lpstrFileName,nMaxFile,dwFlags);
}
//---------------------------------------------------------------------------
BOOL ShowSaveDialog(char *lpstrTitle,char *lpstrFileName,char *lpstrFilter,HWND hwnd,LPFNCSAVEDLG pFnc)
{
	LOpenSaveDlg dlg;

	return dlg.ShowSave(lpstrTitle,lpstrFilter,hwnd,lpstrFileName,pFnc);
}
//---------------------------------------------------------------------------
BOOL SaveBitmap(HDC hdc,HBITMAP bit,char *fileName)
{
	LString nameFile;
	LFile *fp;
   BITMAP bm;
   PBITMAPINFO pbmi;
	BITMAPFILEHEADER hdr;
	LPBYTE lpBits;
	PBITMAPINFOHEADER pbih;
	WORD cClrBits;
	int i;
   BOOL res;
#ifndef __WIN32__
	int x,y;
	LPBYTE p2,p,p1,p3;
#endif

   res = FALSE;
	fp = NULL;
   lpBits = NULL;
	pbmi = NULL;
#ifdef __WIN32__
   if(hdc == NULL || bit == NULL)
       goto Ex_SaveBitmap;
#else
   if(bit == NULL)
       goto Ex_SaveBitmap;
#endif
   nameFile = fileName;
	nameFile.LowerCase();
   if((i = nameFile.Pos(".")) < 1)
		nameFile += ".bmp";
	else if(nameFile.Pos(".bmp") < 1 && i > 0)
   	nameFile = nameFile.SubString(1,i-1) + ".bmp";
   GetObject(bit,sizeof(BITMAP),&bm);
   cClrBits = (WORD)(bm.bmPlanes * bm.bmBitsPixel);
   if (cClrBits == 1)
   	cClrBits = 1;
   else if (cClrBits <= 4)
   	cClrBits = 4;
   else if (cClrBits <= 8)
   	cClrBits = 8;
   else if (cClrBits <= 16)
   	cClrBits = 16;
   else if (cClrBits <= 24)
   	cClrBits = 24;
   else
   	cClrBits = 32;
   pbmi = (PBITMAPINFO)GlobalAlloc(GPTR,sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (2^cClrBits));
   if(pbmi == NULL)
   	goto Ex_SaveBitmap;
   pbih = &pbmi->bmiHeader;
	pbih->biSize = sizeof(BITMAPINFOHEADER);
   pbih->biWidth = bm.bmWidth;
   pbih->biHeight = bm.bmHeight;
   pbih->biPlanes = bm.bmPlanes;
   pbih->biBitCount = bm.bmBitsPixel;
   pbih->biClrUsed = (cClrBits < 9 ? cClrBits : 0);
   pbih->biCompression = BI_RGB;
   pbih->biSizeImage = (pbih->biWidth + 7) / 8 * pbih->biHeight * cClrBits;
   hdr.bfType = 0x4d42;
   hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof(RGBQUAD) + pbih->biSizeImage);
   hdr.bfReserved1 = 0;
   hdr.bfReserved2 = 0;
   hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof (RGBQUAD);
   lpBits = (LPBYTE)GlobalAlloc(GMEM_FIXED,pbih->biSizeImage);
	if(lpBits == NULL)
   	goto Ex_SaveBitmap;
#ifdef __WIN32__
   GetDIBits(hdc,bit,0,(WORD)pbih->biHeight,lpBits,pbmi, DIB_RGB_COLORS);
#else
	switch(bm.bmBitsPixel){
		case 24:
			p = lpBits;
			p1 = (LPBYTE)bm.bmBits;
			for(y=0;y<bm.bmHeight;y++){
				p2 = p;
				p3 = p1;
				for(x=0;x<bm.bmWidth;x++){
					*p2++ = p3[2];
					*p2++ = p3[1];
					*p2++ = p3[0];
					p3 += 3;
				}
				p += bm.bmWidthBytes;
				p1 += bm.bmWidthBytes;
  			}

		break;
	}
	//memcpy(lpBits,bm.bmBits,pbih->biSizeImage);
#endif
   fp = new LFile(nameFile.c_str());
   if(fp == NULL || !fp->Open(GENERIC_READ|GENERIC_WRITE,CREATE_ALWAYS))
   	goto Ex_SaveBitmap;
	fp->Write(&hdr,sizeof(BITMAPFILEHEADER));
   fp->Write(pbih,sizeof(BITMAPINFOHEADER) + pbih->biClrUsed * sizeof (RGBQUAD));
   fp->Write(lpBits,pbih->biSizeImage);
   res = TRUE;
Ex_SaveBitmap:
   if(fp != NULL)
       delete fp;
   if(lpBits != NULL)
   	GlobalFree((HGLOBAL)lpBits);
   if(pbmi != NULL)
   	GlobalFree((HGLOBAL)pbmi);
   return res;
}
//---------------------------------------------------------------------------
BOOL SaveTextureAsTGA(u8 *image,u16 width,u16 height,u32 data)
{
   u8 header[6],TGAheader[12]={0,0,2,0,0,0,0,0,0,0,0,0};
   LFile *pFile;
   u32 i;
   u8 temp,*buffer;
   LString fileName;
   char s[20];

   buffer = (u8 *)LocalAlloc(LPTR,width*height*sizeof(u32));
   if(buffer == NULL)
       return FALSE;
   fileName.Capacity(MAX_PATH);
   GetTempPath(MAX_PATH,fileName.c_str());
   fileName += "texture";
   wsprintf(s,"_0x%08X_",data);
   fileName += s;
   fileName.BuildFileName(fileName.c_str(),"tga",GetTickCount());
   if((pFile = new LFile(fileName.c_str())) == NULL || !pFile->Open(GENERIC_WRITE,CREATE_ALWAYS)){
       if(pFile != NULL)
           delete pFile;
       LocalFree(buffer);
       return FALSE;
   }
   pFile->Write(TGAheader,sizeof(TGAheader));
   header[4] = (u8)32;
   header[0] = (u8)(width % 256);
   header[1] = (u8)(width / 256);
   header[2] = (u8)(height % 256);
   header[3] = (u8)(height / 256);
   pFile->Write(header,sizeof(header));
   for(i=0;i<width*height*4;i+=4){
       temp = image[i];
       buffer[i] = image[i+2];
       buffer[i+1] = image[i+1];
       buffer[i+2] = image[i];
       buffer[i+3] = image[3];
   }
   pFile->Write(buffer,width*height*sizeof(u32));
   delete pFile;
   LocalFree(buffer);
   return TRUE;
}
//---------------------------------------------------------------------------
u16 CalcCrc16(u16 *data,int length,u16 startValue)
{
	u16 i,i1;
	u32 crc,value;

   crc = startValue;
   for(i=0;i < length/2;i++){
		for(i1=0;i1<0x10;i1+=(u16)4){
       	if(i1 == 0)
           	value = data[i];
           crc = (u16)(crc16tab[crc & 0xF] ^ ((crc << 12) >> 16) ^ crc16tab[((int)value >> i1) & 0xF]);
       }
   }
   return (u16)crc;
}
//---------------------------------------------------------------------------
void EnterDebugMode(BOOL bArm7)
{
#ifdef _DEBUG
	debugDlg.EnterDebugMode(bArm7);
#endif
}
//---------------------------------------------------------------------------
BOOL CheckMMX()
{
	return FALSE;
	DWORD dwFeature,dwStandard,ebx;

   dwStandard = dwFeature = 0;
#if defined(__BORLANDC__)
   ebx = _EBX;
   __asm{
		mov eax, 1
		cpuid
		mov dwStandard, eax
		mov dwFeature, edx
   }
   _EBX = ebx;
#elif defined(__GNUC__)
#endif
   return (BOOL)((dwFeature & MMX_SUPPORTED) ? TRUE : FALSE);
}
//---------------------------------------------------------------------------
BOOL CheckSSE()
{
	DWORD dwFeature,dwStandard,ebx;

   dwStandard = dwFeature = 0;
#ifdef __BORLANDC__
   ebx = _EBX;
   __asm{
		mov eax, 1
		cpuid
		mov dwStandard, eax
		mov dwFeature, edx
   }
   _EBX = ebx;
#elif defined(__GNUC__)
	__asm__ __volatile__(
		"movl %%ebx,%2\n"
		"movl $1,%%eax\n"
		"cpuid\n"
		"movl %%eax,%0\n"
		"movl %%edx,%1\n"
		"movl %2,%%ebx\n"
		: "=m" (dwStandard),"=m" (dwFeature) : "m" (ebx)
    );
#endif
   return (BOOL)((dwFeature & SSE_SUPPORTED) ? TRUE : FALSE);
}
//---------------------------------------------------------------------------
u8 I_FASTCALL log2(u32 value)
{
/*   signed char i;
   u32 pow;

   if(!value)
       return 0;
   pow = 1 << 31;
   for(i=31;i>=0;i--,pow >>= 1){
       if(value < pow)
           continue;
       return (u8)i;
   }
   return 0;*/
#ifdef __BORLANDC__
	__asm {
		db 0xf
       db 0xbd
       db 0xc0
   }
   return _AL;
#elif defined(__WATCOMC__)
   __asm {
       bsr eax,eax
       ret
   }
   return 0;
#else
	__asm__ __volatile__(
		"bsr %eax,%eax\n"
	);
#endif
}
