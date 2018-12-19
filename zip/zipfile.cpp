#include "lds.h"

#ifdef __BORLANDC__
#pragma link "zlib.lib"
#endif

static char m_gszSignature[] = {0x50, 0x4b, 0x01, 0x02};
static char m_gszLocalSignature[] = {0x50, 0x4b, 0x03, 0x04};
static char m_gszCentralDirSignature[] = {0x50, 0x4b, 0x05, 0x06};
//---------------------------------------------------------------------------
LZipFile::LZipFile() : LList()
{
   pFile = NULL;
   bRewriteLocal = bInternal = FALSE;
   m_iBufferSize = 32768;
   ZeroMemory(&m_internalinfo,sizeof(ZIPINTERNALINFO));
   ZeroMemory(&m_info,sizeof(ZIPCENTRALDIR));
   m_curZipFileHeader = NULL;
   m_pBuffer = NULL;
   m_iFileOpened = nothing;
   bModified = FALSE;
   ZeroMemory(tempFileName,sizeof(tempFileName));
}
//---------------------------------------------------------------------------
LZipFile::LZipFile(const char *name) : LList()
{
   pFile = NULL;
   bRewriteLocal = bInternal = FALSE;
   m_iBufferSize = 32768;
   ZeroMemory(&m_internalinfo,sizeof(ZIPINTERNALINFO));
   ZeroMemory(&m_info,sizeof(ZIPCENTRALDIR));
   m_curZipFileHeader = NULL;
   m_pBuffer = NULL;
   m_iFileOpened = nothing;
   bModified = FALSE;
   if(name != NULL)
       lstrcpy(fileName,name);
   ZeroMemory(tempFileName,sizeof(tempFileName));
}
//---------------------------------------------------------------------------
LZipFile::~LZipFile()
{
   Close();
   Clear();
}
//---------------------------------------------------------------------------
LPZIPFILEHEADER LZipFile::newHeader(const char* lpFilename,BOOL bLocal)
{
   LPZIPFILEHEADER p;
   int i;

   p = new ZIPFILEHEADER[1];
   if(p == NULL)
       return NULL;
   ZeroMemory(p,sizeof(ZIPFILEHEADER));
   if(bLocal)
       *((LPDWORD)p->m_szSignature) = *((LPDWORD)m_gszLocalSignature);
   else
       *((LPDWORD)p->m_szSignature) = *((LPDWORD)m_gszSignature);
   if(lpFilename != NULL){
       i = lstrlen(lpFilename) + 1;
       if((p->nameFile = new char[i]) == NULL){
           delete p;
           return NULL;
       }
       lstrcpy(p->nameFile,lpFilename);
       p->m_uFileNameSize = (WORD)(i - 1);
   }
   return p;
}
//---------------------------------------------------------------------------
void LZipFile::Rebuild()
{
   DWORD nFile,dwPos;
   ZIPFILEHEADER p,*p1,*p2;
   LZipFile *pList;
   elem_list *el;

   if(nCount < 1 || m_curZipFileHeader != NULL)
       return;
   pFile->SeekToBegin();
   dwPos = 0;
   el = First;
   for(nFile=1;nFile <= nCount && el != NULL;nFile++){
       if(!ReadZipFileHeader(&p,TRUE))
           break;
       p1 = (LPZIPFILEHEADER)el->Ele;
       if(dwPos != p1->m_uOffset)
           break;
       dwPos += p.m_uExtraFieldSize + p.m_uCommentSize + p.m_uComprSize +
           LOCALFILEHEADERSIZE + p.m_uFileNameSize;
       pFile->Seek(dwPos,FILE_BEGIN);
       el = el->Next;
   }
   if(nFile > nCount)
       return;
   pList = new LZipFile();
   if(pList == NULL)
       return;
   pFile->SeekToBegin();
   dwPos = 0;
   el = First;
   for(nFile=1;nFile <= nCount && el != NULL;nFile++){
       if(!ReadZipFileHeader(&p,TRUE))
           break;
       p1 = (LPZIPFILEHEADER)el->Ele;
       if((p2 = newHeader(p1->nameFile,TRUE)) == NULL)
           break;
       CopyMemory(&p2->m_uVersionMadeBy,&p.m_uVersionMadeBy,LOCALFILEHEADERSIZE);
       p2->m_uVersionNeeded = 0x14;
       p2->m_uOffset = dwPos;
       pList->Add((LPVOID)p2);
       dwPos += p2->m_uExtraFieldSize + p2->m_uCommentSize + p2->m_uComprSize + LOCALFILEHEADERSIZE + p2->m_uFileNameSize;
       pFile->Seek(dwPos,FILE_BEGIN);
       el = el->Next;
   }
   pFile->SetEndOfFile(dwPos);
   Clear();
   p1 = (LPZIPFILEHEADER)pList->GetFirstItem(&nFile);
   while(p1 != NULL){
       if((p2 = newHeader(p1->nameFile,FALSE)) == NULL)
           break;
       CopyMemory(p2,p1,((LPBYTE)&p1->nameFile - (LPBYTE)p1));
       Add((LPVOID)p2);
       p1 = (LPZIPFILEHEADER)pList->GetNextItem(&nFile);
   }
   delete pList;
   bModified = TRUE;
   Close();
}
//---------------------------------------------------------------------------
BOOL LZipFile::Open(const char *lpFileName,BOOL bOpenAlways)
{
   long uMaxRecordSize,uPosInFile;
	DWORD uFileSize;
	int uRead,iToRead,iActuallyRead,i;
   BOOL res;
   LPZIPFILEHEADER p;

   if(lpFileName == NULL && pFile == NULL)
       return FALSE;
   if(pFile != NULL && pFile->IsOpen() && m_curZipFileHeader != NULL)
   	return TRUE;
   if(m_curZipFileHeader != NULL)
       CloseZipFile();
   m_curZipFileHeader = NULL;
   if(m_info.m_pszComment != NULL)
       delete m_info.m_pszComment;
   ZeroMemory(&m_info,sizeof(ZIPCENTRALDIR));
   if(pFile == NULL){
       if((pFile = new LFile((char *)lpFileName)) == NULL)
           return FALSE;
       bInternal = TRUE;
   }
   if(!pFile->Open(GENERIC_READ|GENERIC_WRITE,(bOpenAlways ? OPEN_ALWAYS : OPEN_EXISTING)))
       return FALSE;
   Clear();
   uMaxRecordSize = 0xffff + CENTRALDIRSIZE;
	uFileSize = pFile->Size();
	if((DWORD)uMaxRecordSize > uFileSize)
		uMaxRecordSize = uFileSize;
   m_pBuffer = (LPBYTE)GlobalAlloc(GPTR,m_iBufferSize);
   if(m_pBuffer == NULL)
       return FALSE;
   res = FALSE;
	uPosInFile = 0;
	uRead = 0;
   if(uMaxRecordSize == 0) res = TRUE;
	while(uPosInFile < uMaxRecordSize){
       uPosInFile = uRead + m_iBufferSize;
		if(uPosInFile > uMaxRecordSize)
			uPosInFile = uMaxRecordSize;
		iToRead = uPosInFile - uRead;
       pFile->Seek(-uPosInFile,FILE_END);
       iActuallyRead = pFile->Read(m_pBuffer,iToRead);
		if(iActuallyRead != iToRead)
			goto ex_Open;
		for(i = iToRead - 4;i >=0 ;i--){
           if(*((LPDWORD)((char*)m_pBuffer + i)) == *((LPDWORD)m_gszCentralDirSignature)){
               m_info.m_uCentrDirPos = uFileSize - (uPosInFile - i);
               goto ex_Open;
           }
       }
		uRead += iToRead - 3;
	}
   goto ex_Open_1;
ex_Open:
   pFile->Seek(m_info.m_uCentrDirPos,FILE_BEGIN);
   uRead = pFile->Read(m_pBuffer,CENTRALDIRSIZE);
   if(uRead != CENTRALDIRSIZE)
       goto ex_Open_1;
   *((LPDWORD)m_info.m_szSignature) = *((LPDWORD)m_pBuffer);
   m_info.m_uThisDisk = *((LPWORD)(m_pBuffer + 4));
   m_info.m_uDiskWithCD = *((LPWORD)(m_pBuffer + 6));
   m_info.m_uDiskEntriesNo = *((LPWORD)(m_pBuffer + 8));
   m_info.m_uEntriesNumber = *((LPWORD)(m_pBuffer + 10));
   m_info.m_uSize = *((LPDWORD)(m_pBuffer + 12));
   m_info.m_uOffset = *((LPDWORD)(m_pBuffer + 16));
   m_info.uCommentSize = *((LPWORD)(m_pBuffer + 20));
   if(m_info.uCommentSize != 0){
       m_info.m_pszComment = new char[m_info.uCommentSize + 1];
       ZeroMemory(m_info.m_pszComment,m_info.uCommentSize + 1);
       pFile->Read(m_info.m_pszComment,m_info.uCommentSize);
   }
   pFile->Seek(m_info.m_uOffset + m_info.m_uBytesBeforeZip,FILE_BEGIN);
   for(i = 0;i < m_info.m_uEntriesNumber;i++){
		if((p = new ZIPFILEHEADER[1]) == NULL)
           break;
       if(!ReadZipFileHeader(p,FALSE))
           break;
       if(!Add((LPVOID)p))
           break;
	}
   if(i == m_info.m_uEntriesNumber)
       res = TRUE;
ex_Open_1:
   return res;
}
//---------------------------------------------------------------------------
void LZipFile::DeleteElem(LPVOID ele)
{
   LPZIPFILEHEADER p;

   p = (LPZIPFILEHEADER)ele;
   if(p->nameFile != NULL)
       delete []p->nameFile;
   delete (char *)ele;
}
//---------------------------------------------------------------------------
BOOL LZipFile::RewriteLocalHeader()
{
   elem_list *p;
   LPZIPFILEHEADER p1;

   if(pFile == NULL)
       return FALSE;
   p = First;
   while(p != NULL){
       p1 = ((LPZIPFILEHEADER)p->Ele);
       pFile->Seek(p1->m_uOffset,FILE_BEGIN);
       WriteZipFileHeader(p1);
       p = p->Next;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
void LZipFile::Close()
{
   CloseZipFile();
   if(bRewriteLocal)
       RewriteLocalHeader();
   bRewriteLocal = FALSE;
   if(bModified)
       WriteCentralDir();
   bModified = FALSE;
   if(m_pBuffer != NULL)
       GlobalFree((HGLOBAL)m_pBuffer);
   m_pBuffer = NULL;
   if(m_info.m_pszComment != NULL)
       delete []m_info.m_pszComment;
   if(pFile != NULL && bInternal){
       pFile->Close();
       delete pFile;
       pFile = NULL;
       bInternal = FALSE;
   }
   if(lstrlen(tempFileName) > 0){
       if(pFile != NULL){
           pFile->Close();
           delete pFile;
           pFile = NULL;
       }
       ::DeleteFile(tempFileName);
       ZeroMemory(tempFileName,sizeof(tempFileName));
   }
   ZeroMemory(&m_internalinfo,sizeof(ZIPINTERNALINFO));
   ZeroMemory(&m_info,sizeof(ZIPCENTRALDIR));
}
//---------------------------------------------------------------------------
BOOL LZipFile::WriteCentralDir()
{
   elem_list *p;
   char m_gszSignature[] = {0x50, 0x4b, 0x05, 0x06};

   if(pFile == NULL || m_curZipFileHeader != NULL)
       return FALSE;
   pFile->SeekToEnd();
   m_info.m_uOffset = pFile->GetCurrentPosition() - m_info.m_uBytesBeforeZip;
   p = First;
   m_info.m_uSize = 0;
   while(p != NULL){
       m_info.m_uSize += WriteZipFileHeader((LPZIPFILEHEADER)p->Ele,FALSE);
       p = p->Next;
   }
   m_info.m_uDiskEntriesNo = m_info.m_uEntriesNumber = (WORD)nCount;
   *((LPDWORD)m_pBuffer) = *((LPDWORD)m_gszSignature);
   *((LPWORD)(m_pBuffer + 4)) = m_info.m_uThisDisk;
   *((LPWORD)(m_pBuffer + 6)) = m_info.m_uDiskWithCD;
   *((LPWORD)(m_pBuffer + 8)) = m_info.m_uDiskEntriesNo;
   *((LPWORD)(m_pBuffer + 10)) = m_info.m_uEntriesNumber;
   *((LPDWORD)(m_pBuffer + 12)) = m_info.m_uSize;
   *((LPDWORD)(m_pBuffer + 16)) = m_info.m_uOffset;
   *((LPWORD)(m_pBuffer + 20)) = m_info.uCommentSize;
   if(m_info.m_pszComment != NULL)
	    CopyMemory(m_pBuffer + 22, m_info.m_pszComment,m_info.uCommentSize);
   pFile->Write(m_pBuffer,CENTRALDIRSIZE + m_info.uCommentSize);
   return TRUE;
}
//---------------------------------------------------------------------------
void LZipFile::SlashBackslashChg(LPBYTE buffer, BOOL bReplaceSlash)
{
	char t1 = '\\',t2 = '/', c1, c2;
	if (bReplaceSlash){
		c1 = t1;
		c2 = t2;
	}
	else{
		c1 = t2;
		c2 = t1;
	}
   while(*buffer != 0){
       if(*buffer == c2)
			*buffer = c1;
       buffer++;
	}
}
//---------------------------------------------------------------------------
BOOL LZipFile::DeleteAllFiles()
{
   if(pFile == NULL || !pFile->SetEndOfFile(m_info.m_uBytesBeforeZip))
       return FALSE;
   Clear();
   return (BOOL)(bModified = TRUE);
}
//---------------------------------------------------------------------------
BOOL LZipFile::DeleteZipFile(DWORD index)
{
   if(!DeleteFile(index))
       return FALSE;
   if(nCount)
       pFile->SetEndOfFile(m_info.m_uBytesBeforeZip + m_info.m_uOffset);
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LZipFile::DeleteFile(DWORD index)
{
   LPZIPFILEHEADER p;
   DWORD uTemp,uMoveBy,uOffsetStart,i;
   elem_list *tmp;

   if(pFile == NULL || !pFile->IsOpen())
       return FALSE;
   p = (LPZIPFILEHEADER)GetItem(index);
   if(p == NULL)
       return FALSE;
   uMoveBy = 0;
   if(index != nCount){
       uOffsetStart = 0;
       for(i=1,tmp = First;tmp != NULL;tmp = tmp->Next,i++){
           p = (LPZIPFILEHEADER)(tmp->Ele);
           if(i == index){
               uTemp = p->m_uOffset;
               if(uOffsetStart){
				    MovePackedFiles(uOffsetStart,uTemp,uMoveBy);
				    uOffsetStart = 0;
			    }
               if(i == nCount)
                   uTemp = pFile->Size() - m_info.m_uBytesBeforeZip - uTemp;
               else
                   uTemp = ((LPZIPFILEHEADER)GetItem(index+1))->m_uOffset - uTemp;
               uMoveBy += uTemp;
           }
           else{
               if(uOffsetStart == 0)
				    uOffsetStart = p->m_uOffset;
			    p->m_uOffset -= uMoveBy;
               bRewriteLocal = TRUE;
           }
       }
       if(uOffsetStart)
		    MovePackedFiles(uOffsetStart,pFile->Size() - m_info.m_uBytesBeforeZip,uMoveBy);
   }
   else
       pFile->SetEndOfFile(p->m_uOffset);
   if(uMoveBy != 0){
       pFile->SetEndOfFile(pFile->Size() - uMoveBy);
       m_info.m_uOffset -= uMoveBy;
   }
   Delete(index);
   m_info.m_uEntriesNumber = (WORD)nCount;
   return (BOOL)(bModified = TRUE);
}
//---------------------------------------------------------------------------
void LZipFile::MovePackedFiles(DWORD uStartOffset, DWORD uEndOffset, DWORD uMoveBy,BOOL bForward)
{
   DWORD uTotalToMove,uPack,size_read,uPosition;
	BOOL bBreak;

	uStartOffset += m_info.m_uBytesBeforeZip;
	uEndOffset += m_info.m_uBytesBeforeZip;
	uTotalToMove = uEndOffset - uStartOffset;
	uPack = uTotalToMove > (DWORD)m_iBufferSize ? m_iBufferSize : uTotalToMove;
   bBreak = FALSE;
	do{
		if(uEndOffset - uStartOffset < uPack){
			uPack = uEndOffset - uStartOffset;
			if(!uPack)
				break;
			bBreak = TRUE;
		}
		uPosition = bForward ? uEndOffset - uPack : uStartOffset;
       pFile->Seek(uPosition,FILE_BEGIN);
		size_read = pFile->Read(m_pBuffer,uPack);
		if(!size_read)
			break;
		if(bForward)
           uPosition += uMoveBy;
		else
			uPosition -= uMoveBy;
       pFile->Seek(uPosition,FILE_BEGIN);
       pFile->Write(m_pBuffer,size_read);
		if(bForward)
           uEndOffset -= size_read;
		else
           uStartOffset += size_read;
	}
	while(!bBreak);
}
//---------------------------------------------------------------------------
BOOL LZipFile::AddZipFile(const char *lpFileName,const int iLevel)
{
   WORD uIndex;
   DWORD dw,dwPos;
   LPZIPFILEHEADER p,p1;
   SYSTEMTIME sysTime;
   FILETIME fileTime;

   if(lpFileName == NULL || pFile == NULL || !pFile->IsOpen() || m_curZipFileHeader != NULL)
       return FALSE;
   p1 = newHeader(lpFileName,FALSE);
   if(p1 == NULL)
       return FALSE;
   SlashBackslashChg((LPBYTE)p1->nameFile,FALSE);
   GetLocalTime(&sysTime);
   SystemTimeToFileTime(&sysTime,&fileTime);
   FileTimeToDosDateTime(&fileTime,&p1->m_uModDate,&p1->m_uModTime);
   p1->m_uVersionNeeded = 0x14;
   p1->m_uMethod = Z_DEFLATED;
//   p1->m_uExternalAttr = FILE_ATTRIBUTE_ARCHIVE;
   switch (iLevel){
		case 1:
			p1->m_uFlag  |= 6;
       break;
		case 2:
			p1->m_uFlag  |= 4;
       break;
		case 8:
		case 9:
			p1->m_uFlag  |= 2;
       break;
   }
   p = (LPZIPFILEHEADER)GetFirstItem(&dwPos);
   dw = 1;
   uIndex = -1;
   while(p != NULL){
       if(lstrcmpi(lpFileName,p->nameFile) == 0){
           uIndex = (WORD)dw;
           break;
       }
       dw++;
       p = (LPZIPFILEHEADER)GetNextItem(&dwPos);
   }
   if(uIndex != WORD(-1))
       DeleteFile(uIndex);
   if(nCount)
       pFile->SetEndOfFile(m_info.m_uBytesBeforeZip + m_info.m_uOffset);
   Add((LPVOID)p1);
   pFile->SeekToEnd();
   WriteZipFileHeader(p1);
   ZeroMemory(&m_internalinfo,sizeof(ZIPINTERNALINFO));
   m_internalinfo.m_stream.avail_out = (uInt)m_iBufferSize;
   m_internalinfo.m_stream.next_out = (Bytef*)m_pBuffer;
   deflateInit2(&m_internalinfo.m_stream,iLevel,Z_DEFLATED,-MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
   m_curZipFileHeader = p1;
   m_iFileOpened = compress;
   extractMode = NORMAL;
   return (BOOL)(bModified = TRUE);
}
//---------------------------------------------------------------------------
BOOL LZipFile::CloseZipFile()
{
   int err;
   DWORD uTotal;

   if(m_curZipFileHeader == NULL)
       return FALSE;
   if(m_iFileOpened == extract){
       if(m_curZipFileHeader->m_uMethod == Z_DEFLATED)
           inflateEnd(&m_internalinfo.m_stream);
   }
   else if(m_iFileOpened == compress){
       m_internalinfo.m_stream.avail_in = 0;
       err = Z_OK;
		if(m_curZipFileHeader->m_uMethod == Z_DEFLATED){
           while(err == Z_OK){
				if(m_internalinfo.m_stream.avail_out == 0){
                   if((m_curZipFileHeader->m_uFlag & 1) != 0){
                   }
                   pFile->Write(m_pBuffer,m_internalinfo.m_uComprLeft);
					m_internalinfo.m_uComprLeft = 0;
					m_internalinfo.m_stream.avail_out = m_iBufferSize;
					m_internalinfo.m_stream.next_out = (Bytef*)m_pBuffer;
				}
				uTotal = m_internalinfo.m_stream.total_out;
				err = deflate(&m_internalinfo.m_stream, Z_FINISH);
				m_internalinfo.m_uComprLeft += m_internalinfo.m_stream.total_out - uTotal;
			}
       }
       if(err == Z_STREAM_END)
           err = Z_OK;
       if((err != Z_OK)&& (err != Z_NEED_DICT))
           return FALSE;
		if(m_internalinfo.m_uComprLeft > 0){
           if((m_curZipFileHeader->m_uFlag & 1) != 0){
           }
           pFile->Write(m_pBuffer,m_internalinfo.m_uComprLeft);
		}
		if(m_curZipFileHeader->m_uMethod == Z_DEFLATED){
			err = deflateEnd(&m_internalinfo.m_stream);
           if((err != Z_OK)&& (err != Z_NEED_DICT))
               return FALSE;
		}
		m_curZipFileHeader->m_uComprSize += m_internalinfo.m_stream.total_out;
		m_curZipFileHeader->m_uUncomprSize = m_internalinfo.m_stream.total_in;
       pFile->Seek(m_curZipFileHeader->m_uOffset+14,FILE_BEGIN);
       *((LPDWORD)m_pBuffer) = m_curZipFileHeader->m_uCrc32;
       *((LPDWORD)(m_pBuffer + 4)) = m_curZipFileHeader->m_uComprSize;
       *((LPDWORD)(m_pBuffer + 8)) = m_curZipFileHeader->m_uUncomprSize;
       m_curZipFileHeader->m_uOffset -= m_info.m_uBytesBeforeZip;
       pFile->Write(m_pBuffer,ZIPARCHIVE_DATADESCRIPTOR_LEN);
       pFile->Seek(m_curZipFileHeader->m_uOffset,FILE_BEGIN);
   }
   m_curZipFileHeader = NULL;
   m_iFileOpened = nothing;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LZipFile::get_FileCompressedInfo(DWORD index,LPCOMPRESSEDFILEINFO p)
{
   LPZIPFILEHEADER p1;

   if(index == 0 || index > (WORD)nCount || pFile == NULL || !pFile->IsOpen())
       return FALSE;
   p1 = (LPZIPFILEHEADER)GetItem(index);
   if(p1 == NULL)
       return FALSE;
   lstrcpy(p->fileName,p1->nameFile);
   p->dwSize = p1->m_uUncomprSize;
   p->dwSizeCompressed = p1->m_uComprSize;
   return TRUE;
}
//---------------------------------------------------------------------------
LPVOID LZipFile::OpenZipFile(WORD uIndex,COMPRESSEDMODE mode)
{
   LPZIPFILEHEADER p;
   int err;
   ZIPFILEHEADER zfh;
   LFile *pFileTmp;
   char *buf;
   DWORD ci,si;

   if(uIndex == 0 || uIndex > (WORD)nCount || pFile == NULL || !pFile->IsOpen())
       return NULL;
   if(m_curZipFileHeader != NULL)
       CloseZipFile();
   p = (LPZIPFILEHEADER)GetItem(uIndex);
   if(p == NULL)
       return NULL;
   if(p->m_uMethod != 0 && (p->m_uMethod != Z_DEFLATED) || (p->m_uFlag & 1) != 0)
       return NULL;
   pFile->Seek(p->m_uOffset + m_info.m_uBytesBeforeZip,FILE_BEGIN);
   ReadZipFileHeader(&zfh);
   ZeroMemory(&m_internalinfo,sizeof(ZIPINTERNALINFO));
   if(p->m_uMethod == Z_DEFLATED){
		m_internalinfo.m_stream.opaque =  0;
       err = inflateInit2(&m_internalinfo.m_stream,-MAX_WBITS);
       if((err != Z_OK)&& (err != Z_NEED_DICT))
           return NULL;
	}
	m_internalinfo.m_uComprLeft = p->m_uComprSize;
	m_internalinfo.m_uUncomprLeft = p->m_uUncomprSize;
	m_internalinfo.m_uCrc32 = 0;
	m_internalinfo.m_stream.total_out = 0;
	m_internalinfo.m_stream.avail_in = 0;
   m_curZipFileHeader = p;
   m_iFileOpened = extract;
   extractMode = NORMAL;
   if(mode == NORMAL)
       return p;
   GetTempPath(MAX_PATH,tempFileName);
   GetTempFileName(tempFileName,"iDeaS",0,tempFileName);
   if((pFileTmp = new LFile(tempFileName)) == NULL)
       goto ex_OpenZipFile;
   if((buf = (char *)LocalAlloc(LPTR,m_iBufferSize)) == NULL){
       delete pFileTmp;
       goto ex_OpenZipFile;
   }
   if(!pFileTmp->Open(GENERIC_WRITE,CREATE_ALWAYS)){
       LocalFree(buf);
       delete pFileTmp;
       goto ex_OpenZipFile;
   }
   ci = p->m_uUncomprSize;
   while(ci > 0){
       si = (ci > m_iBufferSize) ? m_iBufferSize : ci;
       if(ReadCompressedFile(buf,si) != si)
           break;
       pFileTmp->Write(buf,si);
       ci -=si;
       ds.ProcessMessages();
   }
   LocalFree(buf);
   delete pFileTmp;
   if(ci)
       mode = NORMAL;
   else{
       pFileTmp = new LFile(tempFileName);
       if(!pFileTmp->Open()){
           delete pFileTmp;
           goto ex_OpenZipFile;
       }
       pFile->Close();
       delete pFile;
       pFile = pFileTmp;
   }
   extractMode = mode;
ex_OpenZipFile:
   return p;
}
//---------------------------------------------------------------------------
DWORD LZipFile::WriteZipFile(LPVOID buf,DWORD dwByte)
{
   DWORD uTotal,uToCopy;
   int err;

   if(m_curZipFileHeader == NULL || pFile == NULL || !pFile->IsOpen() || m_iFileOpened != compress || extractMode != NORMAL)
       return 0;
   m_internalinfo.m_stream.next_in = (Bytef*)buf;
   m_internalinfo.m_stream.avail_in = dwByte;
   m_curZipFileHeader->m_uCrc32 = crc32(m_curZipFileHeader->m_uCrc32, (Bytef*)buf,dwByte);
   while(m_internalinfo.m_stream.avail_in > 0){
       if(m_internalinfo.m_stream.avail_out == 0){
           if((m_curZipFileHeader->m_uFlag & 1) != 0){
           }
           pFile->Write(m_pBuffer, m_internalinfo.m_uComprLeft);
			m_internalinfo.m_uComprLeft = 0;
           m_internalinfo.m_stream.avail_out = m_iBufferSize;
           m_internalinfo.m_stream.next_out = (Bytef*)m_pBuffer;
       }
       if(m_curZipFileHeader->m_uMethod == Z_DEFLATED){
           uTotal = m_internalinfo.m_stream.total_out;
           err = deflate(&m_internalinfo.m_stream, Z_NO_FLUSH);
           if((err != Z_OK) && (err != Z_NEED_DICT))
              break;
           m_internalinfo.m_uComprLeft += m_internalinfo.m_stream.total_out - uTotal;
       }
       else{
           uToCopy = (m_internalinfo.m_stream.avail_in < m_internalinfo.m_stream.avail_out) ? m_internalinfo.m_stream.avail_in : m_internalinfo.m_stream.avail_out;
			CopyMemory(m_internalinfo.m_stream.next_out, m_internalinfo.m_stream.next_in, uToCopy);
           m_internalinfo.m_stream.avail_in    -= uToCopy;
           m_internalinfo.m_stream.avail_out   -= uToCopy;
           m_internalinfo.m_stream.next_in     += uToCopy;
           m_internalinfo.m_stream.next_out    += uToCopy;
           m_internalinfo.m_stream.total_in    += uToCopy;
           m_internalinfo.m_stream.total_out   += uToCopy;
           m_internalinfo.m_uComprLeft         += uToCopy;
       }
   }
   return (DWORD)(dwByte - m_internalinfo.m_stream.avail_in);
}
//---------------------------------------------------------------------------
DWORD LZipFile::Seek(LONG dwDistanceToMove,DWORD dwMoveMethod)
{
   s64 rel_ofs,cur_pos,read_size;
   DWORD dw;
   char *buf;

   if(m_curZipFileHeader == NULL || pFile == NULL || !pFile->IsOpen() || m_iFileOpened != extract)
       return 0xFFFFFFFF;
   if(extractMode != NORMAL)
       return pFile->Seek(dwDistanceToMove,dwMoveMethod);
   cur_pos = m_curZipFileHeader->m_uUncomprSize - m_internalinfo.m_uUncomprLeft;
   switch(dwMoveMethod){
       case FILE_BEGIN:
           rel_ofs = dwDistanceToMove - cur_pos;
       break;
       case FILE_CURRENT:
           rel_ofs = dwDistanceToMove;
       break;
       case FILE_END:
           rel_ofs = m_curZipFileHeader->m_uUncomprSize + dwDistanceToMove - cur_pos;
       break;
       default:
           return (DWORD)-1;
   }
   if(rel_ofs == 0)
       return (DWORD)cur_pos;
   if(rel_ofs < 0){
       dw = IndexFromEle((LPVOID)m_curZipFileHeader);
       if(dw == (DWORD)-1)
           return (DWORD)-1;
       CloseZipFile();
       if(!OpenZipFile((WORD)dw))
          return (DWORD)-1;
       read_size = cur_pos + rel_ofs;
       cur_pos = 0;
   }
   else
       read_size = rel_ofs;
   if(read_size < 0)
       return (DWORD)-1;
   if(read_size + cur_pos > m_curZipFileHeader->m_uUncomprSize)
       return (DWORD)-1;
   if(read_size == 0)
       return (DWORD)cur_pos;
   if((buf = new char[m_iBufferSize]) == NULL)
       return (DWORD)-1;
   while(read_size > 0){
       dw = (DWORD)(read_size < m_iBufferSize ? read_size : m_iBufferSize);
       if(ReadZipFile(buf,dw) != dw)
           return (DWORD)-1;
       read_size -= dw;
   }
   delete []buf;
   return m_curZipFileHeader->m_uUncomprSize - m_internalinfo.m_uUncomprLeft;
}
//---------------------------------------------------------------------------
DWORD LZipFile::ReadZipFile(LPVOID buf,DWORD dwByte)
{
   DWORD iRead,uToRead,uToCopy,uTotal;
   BOOL bForce;
   int err;
   Bytef* pOldBuf;

   if(m_curZipFileHeader == NULL || pFile == NULL || !pFile->IsOpen() || m_iFileOpened != extract)
       return 0;
   if(extractMode != NORMAL)
       return pFile->Read(buf,dwByte);
   m_internalinfo.m_stream.next_out = (Bytef*)buf;
	m_internalinfo.m_stream.avail_out = dwByte > m_internalinfo.m_uUncomprLeft ? m_internalinfo.m_uUncomprLeft : dwByte;
   iRead = 0;
	bForce = m_internalinfo.m_stream.avail_out == 0 && m_internalinfo.m_uComprLeft > 0;
	while(m_internalinfo.m_stream.avail_out > 0 || (bForce && m_internalinfo.m_uComprLeft > 0)){
		if((m_internalinfo.m_stream.avail_in == 0) && m_internalinfo.m_uComprLeft > 0){
			uToRead = m_iBufferSize;
			if(m_internalinfo.m_uComprLeft < (DWORD)m_iBufferSize)
				uToRead = m_internalinfo.m_uComprLeft;
			if(uToRead == 0)
               goto ex_ReadZipFile;
           pFile->Read(m_pBuffer,uToRead);
           if((m_curZipFileHeader->m_uFlag & 1) != 0){
           }
			m_internalinfo.m_uComprLeft -= uToRead;
			m_internalinfo.m_stream.next_in = (Bytef*)m_pBuffer;
			m_internalinfo.m_stream.avail_in = uToRead;
		}
		if(m_curZipFileHeader->m_uMethod == 0){
			uToCopy = m_internalinfo.m_stream.avail_out < m_internalinfo.m_stream.avail_in ? m_internalinfo.m_stream.avail_out : m_internalinfo.m_stream.avail_in;
			CopyMemory(m_internalinfo.m_stream.next_out,m_internalinfo.m_stream.next_in,uToCopy);
			m_internalinfo.m_uCrc32 = crc32(m_internalinfo.m_uCrc32,m_internalinfo.m_stream.next_out,uToCopy);
			m_internalinfo.m_uUncomprLeft       -= uToCopy;
			m_internalinfo.m_stream.avail_in    -= uToCopy;
			m_internalinfo.m_stream.avail_out   -= uToCopy;
			m_internalinfo.m_stream.next_out    += uToCopy;
			m_internalinfo.m_stream.next_in     += uToCopy;
           m_internalinfo.m_stream.total_out   += uToCopy;
			iRead += uToCopy;
		}
		else{
			uTotal = m_internalinfo.m_stream.total_out;
			pOldBuf =  m_internalinfo.m_stream.next_out;
			err = inflate(&m_internalinfo.m_stream,Z_SYNC_FLUSH);
			uToCopy = m_internalinfo.m_stream.total_out - uTotal;
			m_internalinfo.m_uCrc32 = crc32(m_internalinfo.m_uCrc32, pOldBuf, uToCopy);
			m_internalinfo.m_uUncomprLeft -= uToCopy;
			iRead += uToCopy;
			if(err == Z_STREAM_END)
               goto ex_ReadZipFile;
           if((err != Z_OK) && (err != Z_NEED_DICT))
               break;
		}
	}
ex_ReadZipFile:                
	return iRead;
}
//---------------------------------------------------------------------------
DWORD LZipFile::WriteZipFileHeader(LPZIPFILEHEADER p,BOOL bLocal)
{
   DWORD iSize;

   if(bLocal)
       iSize = LOCALFILEHEADERSIZE;
   else
       iSize = FILEHEADERSIZE + p->m_uCommentSize;
   iSize += p->m_uFileNameSize + p->m_uExtraFieldSize;
   if(!bLocal){
       *((LPDWORD)p->m_szSignature) = *((LPDWORD)m_gszSignature);
       *((LPDWORD)m_pBuffer) = *((LPDWORD)m_gszSignature);
       *((LPWORD)(m_pBuffer + 4)) = p->m_uVersionMadeBy;
       *((LPWORD)(m_pBuffer + 6)) = p->m_uVersionNeeded;
       *((LPWORD)(m_pBuffer + 8)) = p->m_uFlag;
       *((LPWORD)(m_pBuffer + 10)) = p->m_uMethod;
       *((LPWORD)(m_pBuffer + 12)) = p->m_uModTime;
       *((LPWORD)(m_pBuffer + 14)) = p->m_uModDate;
       *((LPDWORD)(m_pBuffer + 16)) = p->m_uCrc32;
       *((LPDWORD)(m_pBuffer + 20)) = p->m_uComprSize;
       *((LPDWORD)(m_pBuffer + 24)) = p->m_uUncomprSize;
       *((LPWORD)(m_pBuffer + 28)) = p->m_uFileNameSize;
       *((LPWORD)(m_pBuffer + 30)) = p->m_uExtraFieldSize;
       *((LPWORD)(m_pBuffer + 32)) = p->m_uCommentSize;
       *((LPWORD)(m_pBuffer + 34)) = p->m_uDiskStart;
       *((LPWORD)(m_pBuffer + 36)) = p->m_uInternalAttr;
       *((LPDWORD)(m_pBuffer + 38)) = p->m_uExternalAttr;
       *((LPDWORD)(m_pBuffer + 42)) = p->m_uOffset;
       CopyMemory(m_pBuffer + 46,p->nameFile,p->m_uFileNameSize);
   }
   else{
       *((LPDWORD)p->m_szSignature) = *((LPDWORD)m_gszLocalSignature);
       *((LPDWORD)m_pBuffer) = *((LPDWORD)m_gszLocalSignature);
       *((LPWORD)(m_pBuffer + 4)) = p->m_uVersionNeeded;
       *((LPWORD)(m_pBuffer + 6)) = p->m_uFlag;
       *((LPWORD)(m_pBuffer + 8)) = p->m_uMethod;
       *((LPWORD)(m_pBuffer + 10)) = p->m_uModTime;
       *((LPWORD)(m_pBuffer + 12)) = p->m_uModDate;
       *((LPDWORD)(m_pBuffer + 14)) = p->m_uCrc32;
       *((LPDWORD)(m_pBuffer + 18)) = p->m_uComprSize;
       *((LPDWORD)(m_pBuffer + 22)) = p->m_uUncomprSize;
       *((LPWORD)(m_pBuffer + 26)) = p->m_uFileNameSize;
       *((LPWORD)(m_pBuffer + 28)) = p->m_uExtraFieldSize;
       *((LPWORD)(m_pBuffer + 30)) = p->m_uCommentSize;
       p->m_uOffset = pFile->GetCurrentPosition();
   }
   return pFile->Write(m_pBuffer,iSize);
}
//---------------------------------------------------------------------------
void LZipFile::SetFileStream(LStream *p)
{
   if(bInternal && pFile != NULL){
       pFile->Close();
       delete pFile;
       pFile = NULL;
   }
   if(p == NULL){
   	pFile = NULL;
       bInternal = TRUE;
       return;
   }
   bInternal = FALSE;
   pFile = p;
}
//---------------------------------------------------------------------------
BOOL LZipFile::ReadZipFileHeader(LPZIPFILEHEADER p,BOOL bLocal)
{
   int iSize;

   if(!bLocal)
       iSize = FILEHEADERSIZE;
   else
       iSize = LOCALFILEHEADERSIZE;
   pFile->Read(m_pBuffer,iSize);
   ZeroMemory(p,sizeof(ZIPFILEHEADER));
   if(!bLocal){
       *((LPDWORD)p->m_szSignature) = *((LPDWORD)m_pBuffer);
       p->m_uVersionMadeBy = *((LPWORD)(m_pBuffer + 4));
       p->m_uVersionNeeded = *((LPWORD)(m_pBuffer + 6));
	    p->m_uFlag = *((LPWORD)(m_pBuffer + 8));
	    p->m_uMethod = *((LPWORD)(m_pBuffer + 10));
	    p->m_uModTime = *((LPWORD)(m_pBuffer + 12));
	    p->m_uModDate = *((LPWORD)(m_pBuffer + 14));
	    p->m_uCrc32 = *((LPDWORD)(m_pBuffer + 16));
	    p->m_uComprSize = *((LPDWORD)(m_pBuffer + 20));
	    p->m_uUncomprSize = *((LPDWORD)(m_pBuffer + 24));
	    p->m_uFileNameSize = *((LPWORD)(m_pBuffer + 28));
	    p->m_uExtraFieldSize = *((LPWORD)(m_pBuffer + 30));
	    p->m_uCommentSize = *((LPWORD)(m_pBuffer + 32));
	    p->m_uDiskStart = *((LPWORD)(m_pBuffer + 34));
	    p->m_uInternalAttr = *((LPWORD)(m_pBuffer + 36));
	    p->m_uExternalAttr = *((LPDWORD)(m_pBuffer + 38));
	    p->m_uOffset = *((LPDWORD)(m_pBuffer + 42));
       if(*((LPDWORD)p->m_szSignature) != *((LPDWORD)m_gszSignature))
		    return FALSE;
   }
   else{
       *((LPDWORD)p->m_szSignature) = *((LPDWORD)m_pBuffer);
       p->m_uVersionNeeded = *((LPWORD)(m_pBuffer + 4));
	    p->m_uFlag = *((LPWORD)(m_pBuffer + 6));
	    p->m_uMethod = *((LPWORD)(m_pBuffer + 8));
	    p->m_uModTime = *((LPWORD)(m_pBuffer + 10));
	    p->m_uModDate = *((LPWORD)(m_pBuffer + 12));
	    p->m_uCrc32 = *((LPDWORD)(m_pBuffer + 14));
	    p->m_uComprSize = *((LPDWORD)(m_pBuffer + 18));
	    p->m_uUncomprSize = *((LPDWORD)(m_pBuffer + 22));
	    p->m_uFileNameSize = *((LPWORD)(m_pBuffer + 26));
	    p->m_uExtraFieldSize = *((LPWORD)(m_pBuffer + 28));
       if(*((LPDWORD)p->m_szSignature) != *((LPDWORD)m_gszLocalSignature))
		    return FALSE;
   }
   if(p->m_uFileNameSize != 0){
       if(!bLocal){
           if((p->nameFile = new char[p->m_uFileNameSize+1]) == NULL)
               return FALSE;
           ZeroMemory(p->nameFile,p->m_uFileNameSize+1);
           pFile->Read(p->nameFile,p->m_uFileNameSize);
           SlashBackslashChg((LPBYTE)p->nameFile,TRUE);
       }
       else
           pFile->Seek(p->m_uFileNameSize,FILE_CURRENT);
   }
   pFile->Seek(p->m_uExtraFieldSize + p->m_uCommentSize,FILE_CURRENT);
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LZipFile::Open(DWORD dwStyle,DWORD dwCreation,DWORD dwFlags)
{	
   return Open(fileName,(dwCreation == OPEN_ALWAYS ? TRUE : FALSE));
}
//---------------------------------------------------------------------------
DWORD LZipFile::Size(LPDWORD lpHigh)
{
   if(m_curZipFileHeader == NULL || pFile == NULL || !pFile->IsOpen() || m_iFileOpened != extract)
       return 0;
   return m_curZipFileHeader->m_uUncomprSize;
}
//---------------------------------------------------------------------------
BOOL LZipFile::SetEndOfFile(DWORD dw)
{
   return FALSE;
}
//---------------------------------------------------------------------------
DWORD LZipFile::GetCurrentPosition()
{
   if(m_curZipFileHeader == NULL || pFile == NULL || !pFile->IsOpen() || m_iFileOpened != extract)
       return 0xFFFFFFFF;
   return m_curZipFileHeader->m_uUncomprSize - m_internalinfo.m_uUncomprLeft;
}
//---------------------------------------------------------------------------
BOOL LZipFile::IsOpen()
{
   if(pFile == NULL || !pFile->IsOpen())
       return FALSE;
   return TRUE;
}
//---------------------------------------------------------------------------

