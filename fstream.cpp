#include "fstream.h"

//---------------------------------------------------------------------------
LFile::LFile(const char *name)
{                                      
	handle = NULL_STREAM;
   *fileName = 0;
   if(name != NULL)
		lstrcpy(fileName,name);
}
//---------------------------------------------------------------------------
LFile::~LFile()
{
	Close();
}
//---------------------------------------------------------------------------
BOOL LFile::Open(DWORD dwStyle,DWORD dwCreation,DWORD dwFlags)
{
	if(handle != NULL_STREAM)
       return TRUE;
#ifdef __WIN32__
	handle = ::CreateFile(fileName,dwStyle,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,dwCreation,FILE_ATTRIBUTE_NORMAL|dwFlags,NULL);
#else
   char s[4];

	*((DWORD *)s) = 0;
	switch(dwCreation){
		case OPEN_EXISTING:
           s[0] = 'r';
			s[1] = 'b';
           if(dwStyle & GENERIC_WRITE)
               s[2] = '+';
			handle = fopen(fileName,s);
		break;
		case CREATE_ALWAYS:
           s[0] = 'w';
			s[1] = 'b';

           if(dwStyle & GENERIC_READ)
               s[2] = '+';
			handle = fopen(fileName,s);
		break;
		case OPEN_ALWAYS:
           s[0] = 'r';
			s[1] = 'b';
           if(dwStyle & GENERIC_WRITE)
               s[2] = '+';
			if((handle = fopen(fileName,s)) == NULL){
            	s[0] = 'w';
				s[1] = 'b';
               if(dwStyle & GENERIC_READ)
                   s[2] = '+';
				handle = fopen(fileName,s);
           }
		break;
		case CREATE_NEW:
			if((handle = fopen(fileName,"rb")) != NULL){
				fclose(handle);
				handle = NULL_STREAM;
			}
			else{
            	s[0] = 'r';
				s[1] = 'b';
               if(dwStyle & GENERIC_WRITE)
                   s[2] = '+';
				handle = fopen(fileName,s);
           }
		break;
		default:
			handle = NULL_STREAM;
		break;
	}		
#endif
   return (BOOL)(handle != NULL_STREAM ? TRUE : FALSE);
}
//---------------------------------------------------------------------------
void LFile::Close()
{
   if(handle != NULL_STREAM)
#ifdef __WIN32__
		CloseHandle(handle);
#else
		fclose(handle);
#endif
   handle = NULL_STREAM;
}
//---------------------------------------------------------------------------
DWORD LFile::Seek(LONG dwDistanceToMove,DWORD dwMoveMethod)
{
	if(handle == NULL_STREAM)
 		return 0xFFFFFFFF;
#ifdef __WIN32__
   return ::SetFilePointer(handle,dwDistanceToMove,NULL,dwMoveMethod);
#else
	if(!fseek(handle,dwDistanceToMove,dwMoveMethod))
		return ftell(handle);
	return 0xFFFFFFFF;
#endif
}
//---------------------------------------------------------------------------
DWORD LFile::Size(LPDWORD lpHigh)
{
	if(handle == NULL_STREAM)
       return 0xFFFFFFFF;
#ifdef __WIN32__
   return ::GetFileSize(handle,lpHigh);
#else
	unsigned long ul,uSize;

	ul = ftell(handle);
	fseek(handle,0,SEEK_END);
	uSize = ftell(handle);
	fseek(handle,ul,SEEK_SET);
	return uSize;
#endif
}
//---------------------------------------------------------------------------
BOOL LFile::SetEndOfFile(DWORD dw)
{
#ifdef __WIN32__
	if(handle == NULL_STREAM || SetFilePointer(handle,dw,NULL,FILE_BEGIN) == 0xFFFFFFFF)
    	return FALSE;
   return ::SetEndOfFile(handle);
#else
	if(handle == NULL_STREAM || ftruncate(fileno(handle),dw) || fseek(handle,dw,SEEK_SET))
		return false;
	return true;
#endif
}
//---------------------------------------------------------------------------
DWORD LFile::Read(LPVOID lpBuffer,DWORD dwBytes)
{
   if(handle == NULL_STREAM)
       return 0;
#ifdef __WIN32__
   DWORD dwBytesRead;

   if(::ReadFile(handle,lpBuffer,dwBytes,&dwBytesRead,NULL))
       return dwBytesRead;
   return 0;
#else
	return fread(lpBuffer,1,dwBytes,handle);
#endif
}
//---------------------------------------------------------------------------
DWORD LFile::Write(LPVOID lpBuffer,DWORD dwBytes)
{
   if(handle == NULL_STREAM)
    	return 0;
#ifdef __WIN32__
	DWORD dwBytesWrite;

   if(::WriteFile(handle,lpBuffer,dwBytes,&dwBytesWrite,NULL))
      	return dwBytesWrite;
   else
      	return 0;
#else
	return fwrite(lpBuffer,1,dwBytes,handle);
#endif
}
//---------------------------------------------------------------------------
DWORD LFile::WriteF(char *mes,...)
{
	char s[2000];
	va_list ap;

   va_start(ap, mes);
	vsprintf(s,mes,ap);
   va_end(ap);
   return Write(s,lstrlen(s));
}
//---------------------------------------------------------------------------
DWORD LFile::ReadLine(DWORD dwLine,char *lpBuffer,DWORD dwBytes)
{
   char c[100];
   DWORD dw,dw1,dw2;

	if(lpBuffer == NULL || !SeekToBegin())
   	return 0;
   for(dw1=0,dw = 1;dw < dwLine;){
   	if(!dw1 && !(dw1 = Read(c,100)))
       	break;
       for(dw2=0;dw < dwLine && dw2 < dw1;dw2++){
       	if(c[dw2] == 0xA)
       		dw++;
       }
       if(dw2 != dw1)
       	Seek(dw2-dw1,FILE_CURRENT);
       dw1 = 0;
   }
   if(dw != dwLine)
   	return 0;
   for(dw1=dw=0;dw<dwBytes;dw++){
   	if(!dw1 && !(dw1 = Read(c,100)))
       	break;
       for(dw2=0;dw < dwBytes && dw2 < dw1;dw2++,dw++){
       	if(c[dw2] == 0xA)
       		break;
           else if(c[dw2] != 0xD)
				*lpBuffer++ = c[dw2];
       }
       if(dw2 != dw1){
       	Seek(dw2-dw1,FILE_CURRENT);
           break;
       }
		dw1 = 0;
   }
   if(dw < dwBytes)
   	*lpBuffer = 0;
   return dw;
}
//---------------------------------------------------------------------------
LMemoryFile::LMemoryFile(DWORD dw) : LList()
{
   dwBufferSize = dw;
}
//---------------------------------------------------------------------------
LMemoryFile::~LMemoryFile()
{
   Close();
}
//---------------------------------------------------------------------------
LMemoryFile::buffer::buffer(DWORD dw)
{
   buf = (LPBYTE)::GlobalAlloc(GPTR,dw);
   if(buf == NULL)
       return;
   dwSize = dw;
   dwPos = 1;
   dwBytesWrite = 0;
}
//---------------------------------------------------------------------------
void LMemoryFile::DeleteElem(LPVOID ele)
{
   buffer *tmp;

   if(ele == NULL)
       return;
   tmp = (buffer *)ele;
   if(tmp->buf != NULL)
       GlobalFree((HGLOBAL)tmp->buf);
   delete (buffer *)ele;
}
//---------------------------------------------------------------------------
LMemoryFile::buffer *LMemoryFile::AddBuffer()
{
   buffer *tmp;

   tmp = new buffer(dwBufferSize);
   if(tmp == NULL || tmp->buf == NULL)
       return NULL;
   if(!Add((LPVOID)tmp)){
       DeleteElem((LPVOID)tmp);
       return NULL;
   }
   dwSize += tmp->dwSize;
   return tmp;
}
//---------------------------------------------------------------------------
/*BOOL LMemoryFile::SaveToFile(const char *lpFileName)
{
   LFile *pFile;
   LList::elem_list *tmp;
   buffer *p;

   pFile = new LFile(lpFileName);
   if(pFile == NULL)
       return FALSE;
   if(!pFile->Open(GENERIC_WRITE,CREATE_ALWAYS)){
       delete pFile;
       return FALSE;
   }
   tmp = First;
   while(tmp != NULL){
       p = (buffer *)tmp->Ele;
       pFile->Write(p->buf,p->dwBytesWrite);
       tmp = tmp->Next;
   }
   delete pFile;
   return TRUE;
}*/
//---------------------------------------------------------------------------
BOOL LMemoryFile::Open(DWORD,DWORD,DWORD)
{
   if(nCount > 0)
       return TRUE;
   dwSize = 0;
   if((currentBuffer = AddBuffer()) == NULL)
       return FALSE;
   dwPos = 1;
   dwIndex = 1;
   return TRUE;
}
//---------------------------------------------------------------------------
void LMemoryFile::Close()
{
   Clear();
}
//---------------------------------------------------------------------------
DWORD LMemoryFile::Read(LPVOID lpBuffer,DWORD dwBytes)
{
   buffer *tmp;
   DWORD dwRead,dwpos,dw;

   if(nCount == 0 || lpBuffer == NULL || dwBytes < 1)
       return 0;
   if((dwpos = (DWORD)CercaItem(dwIndex)) == 0)
       return 0;
   if((tmp = (buffer *)(((LList::elem_list *)dwpos)->Ele)) == NULL)
       return 0;
   dwRead = 0;
   while(1){
       dw = tmp->dwBytesWrite - tmp->dwPos + 1;
       if(dwBytes < dw)
           dw = dwBytes;
       CopyMemory(lpBuffer,&tmp->buf[tmp->dwPos-1],dw);
       lpBuffer = ((LPBYTE)lpBuffer) + dw;
       dwBytes -= dw;
       dwRead += dw;
       dwPos += dw;
       tmp->dwPos += dw;
       if(dwBytes == 0 || (tmp = (buffer *)GetNextItem(&dwpos)) == NULL)
           break;
       tmp->dwPos = 1;
       dwIndex++;
       currentBuffer = tmp;
   }
   return dwRead;
}
//---------------------------------------------------------------------------
DWORD LMemoryFile::Write(LPVOID lpBuffer,DWORD dwBytes)
{
   buffer *tmp;
   DWORD dwWrite,dw;

   if(nCount == 0 || lpBuffer == NULL || dwBytes < 1)
       return 0;
   tmp = currentBuffer;
   dwWrite = 0;
   while(dwBytes != 0){
       if(tmp == NULL || tmp->dwSize == (tmp->dwPos - 1)){
           if(dwIndex == nCount){
               if((tmp = AddBuffer()) == NULL)
                   break;
               dwIndex = nCount;
           }
           else{
               tmp = (buffer *)GetItem(++dwIndex);
               tmp->dwPos = 1;
           }
           currentBuffer = tmp;
       }
       dw = tmp->dwSize - (tmp->dwPos - 1);
       if(dwBytes < dw)
           dw = dwBytes;
       CopyMemory(&tmp->buf[tmp->dwPos-1],lpBuffer,dw);
       lpBuffer = ((LPBYTE)lpBuffer) + dw;
       if(tmp->dwPos >= tmp->dwBytesWrite)
           tmp->dwBytesWrite += dw;
       tmp->dwPos += dw;
       dwBytes -= dw;
       dwWrite += dw;
       dwPos += dw;
   }
   return dwWrite;
}
//---------------------------------------------------------------------------
DWORD LMemoryFile::Seek(LONG dwDistanceToMove,DWORD dwMoveMethod)
{
   DWORD dw,dw1,dw2,dwRes;
   buffer *tmp;
   LONG lEnd;

   if(nCount == 0)
       return 0xFFFFFFFF;
   dwRes = 0xFFFFFFFF;
   switch(dwMoveMethod){
       case FILE_BEGIN:
           lEnd = dwDistanceToMove;
       break;
       case FILE_END:
           lEnd = (LONG)((LONG)Size() + dwDistanceToMove);
       break;
       case FILE_CURRENT:
           lEnd = ((LONG)dwPos + dwDistanceToMove - 1);
       break;
       default:
           return dwRes;
   }
   if(lEnd < 0)
       lEnd = 0;
   else if(lEnd > (LONG)Size())
       lEnd = (LONG)Size();
   if((tmp = (buffer *)GetFirstItem(&dw)) == NULL)
       return dwRes;
   dw1 = 0;
   dw2 = 1;
   while(tmp != NULL){
       if((LONG)(dw1 + tmp->dwBytesWrite) >= lEnd){
           dwRes = dwPos = (DWORD)lEnd;
           dwPos++;
           currentBuffer = tmp;
           tmp->dwPos = lEnd - dw1 + 1;
           dwIndex = dw2;
           break;
       }
       dw1 += tmp->dwBytesWrite;
       dw2++;
       tmp = (buffer *)GetNextItem(&dw);
   }
   return dwRes;
}
//---------------------------------------------------------------------------
DWORD LMemoryFile::Size(LPDWORD)
{
   buffer *tmp;
   DWORD dw;

   if(nCount == 0)
       return 0;
   if((tmp = (buffer *)GetLastItem(&dw)) == NULL)
       return 0;
   return (DWORD)(dwSize - (tmp->dwSize - tmp->dwBytesWrite));
}
//---------------------------------------------------------------------------
BOOL LMemoryFile::SetEndOfFile(DWORD dw)
{
   buffer *tmp;
   DWORD dw1,dw2,dw3;

   if(nCount == 0 || (tmp = (buffer *)GetFirstItem(&dw1)) == NULL)
       return FALSE;
   dw2 = 0;
   dw3 = 1;
   while(tmp != NULL){
       if(dw2 + tmp->dwBytesWrite > dw){
           nCount = dwIndex = dw3;
           if(dwPos > dw)
               dwPos = dw + 1;
           dwSize = dw2 + tmp->dwSize;
           tmp->dwBytesWrite = tmp->dwPos = dw - dw2;
           tmp->dwPos++;
           Last = (LList::elem_list *)dw1;
           currentBuffer = tmp;
           dw1 = (DWORD)(((LList::elem_list *)dw1)->Next);
           while(dw1 != NULL){
               dw2 = (DWORD)(((LList::elem_list *)dw1)->Next);
               DeleteElem((LPVOID)dw1);
               dw1 = dw2;
           }
           Last->Next = NULL;
           return TRUE;
       }
       dw3++;
       dw2 += tmp->dwBytesWrite;
       tmp = (buffer *)GetNextItem(&dw1);
   }
   return FALSE;
}

