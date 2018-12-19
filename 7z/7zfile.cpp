#include "7zfile.h"
#include "lds.h"

#ifdef __WIN32__
#pragma link "lzma.lib"

#else
#define S_OK 0

#endif

//---------------------------------------------------------------------------
static void *Alloc(size_t size)
{
   if (size == 0)
       return NULL;
   return LocalAlloc(LPTR,size);
}
//---------------------------------------------------------------------------
static void Free(void *address)
{
   LocalFree(address);
}
//---------------------------------------------------------------------------
static void *AllocTemp(size_t size)
{
   if (size == 0)
       return NULL;
   return LocalAlloc(LPTR,size);
}
//---------------------------------------------------------------------------
static void FreeTemp(void *address)
{
   LocalFree(address);
}
//---------------------------------------------------------------------------
static SZ_RESULT FileReadImp(void *object, void *buffer, size_t size, size_t *processedSize)
{
   CFileInStream *s;
   size_t processedSizeLoc;

   s = (CFileInStream *)object;
   if(s == NULL || s->File == NULL)
       return SZE_FAIL;
   processedSizeLoc = s->File->Read(buffer,size);
   if (processedSize != 0)
       *processedSize = processedSizeLoc;
   return SZ_OK;
}
//---------------------------------------------------------------------------
static SZ_RESULT FileSeekImp(void *object, CFileSize pos)
{
   CFileInStream *s;

   s = (CFileInStream *)object;
   if(s == NULL || s->File == NULL)
       return SZE_FAIL;
   if(s->File->Seek((DWORD)pos,FILE_BEGIN) == 0xFFFFFFFF)
       return SZE_FAIL;
   return SZ_OK;
}
//---------------------------------------------------------------------------
L7ZFile::L7ZFile()
{
   ZeroMemory(&db,sizeof(CArchiveDatabaseEx));
   ZeroMemory(&archiveStream,sizeof(archiveStream));
   ZeroMemory(&allocImp,sizeof(allocImp));
   ZeroMemory(&allocTempImp,sizeof(allocTempImp));
   ZeroMemory(tempFileName,sizeof(tempFileName));
   pFile = NULL;
   folder = NULL;
   m_pBuffer = NULL;
   allocImp.Alloc = Alloc;
   allocImp.Free = Free;
   allocTempImp.Alloc = AllocTemp;
   allocTempImp.Free = FreeTemp;
   archiveStream.InStream.Read = FileReadImp;
   archiveStream.InStream.Seek = FileSeekImp;
   dwPosx86[0] = dwPosx86[1] = 0;
   dwNowPos = 0;
}
//---------------------------------------------------------------------------
L7ZFile::L7ZFile(const char *name)
{
   pFile = NULL;
   folder = NULL;
   m_pBuffer = NULL;
   ZeroMemory(&db,sizeof(CArchiveDatabaseEx));
   ZeroMemory(&archiveStream,sizeof(archiveStream));
   ZeroMemory(&allocImp,sizeof(allocImp));
   ZeroMemory(&allocTempImp,sizeof(allocTempImp));
   ZeroMemory(tempFileName,sizeof(tempFileName));
   allocImp.Alloc = Alloc;
   allocImp.Free = Free;
   allocTempImp.Alloc = AllocTemp;
   allocTempImp.Free = FreeTemp;
   archiveStream.InStream.Read = FileReadImp;
   archiveStream.InStream.Seek = FileSeekImp;
   dwPosx86[0] = dwPosx86[1] = 0;
   dwNowPos = 0;

   if(name != NULL)
       lstrcpy(fileName,name);
}
//---------------------------------------------------------------------------
L7ZFile::~L7ZFile()
{
   Close();             
   if(pFile != NULL)
       pFile->Release();
}
//---------------------------------------------------------------------------
BOOL L7ZFile::Open(DWORD dwStyle,DWORD dwCreation,DWORD dwFlags)
{
   return Open(fileName,(dwCreation == OPEN_ALWAYS ? TRUE : FALSE));
}
//---------------------------------------------------------------------------
BOOL L7ZFile::Open(const char *lpFileName,BOOL bOpenAlways)
{
   BOOL res;

   if(lpFileName == NULL && pFile == NULL)
       return FALSE;
   if(pFile != NULL && pFile->IsOpen())
   	return TRUE;
   if(pFile == NULL){
       if((pFile = new LFile((char *)lpFileName)) == NULL)
           return FALSE;
   }
   if(!pFile->Open(GENERIC_READ|GENERIC_WRITE,(bOpenAlways ? OPEN_ALWAYS : OPEN_EXISTING)))
       return FALSE;
   res = FALSE;
   SzArDbExInit(&db);
   archiveStream.File = pFile;
   if(SzArchiveOpen(&archiveStream.InStream, &db, &allocImp, &allocTempImp) == SZ_OK)
       res = TRUE;
   return res;
}
//---------------------------------------------------------------------------
BOOL L7ZFile::get_FileCompressedInfo(DWORD index,LPCOMPRESSEDFILEINFO p)
{
   DWORD folderIndex;
   CFileSize *packSizes;
   CFolder *f;

   if(p == NULL || index > db.Database.NumFiles || db.Database.Files == NULL || index < 1)
       return FALSE;
   lstrcpy(p->fileName,db.Database.Files[--index].Name);
   folderIndex = db.FileIndexToFolderIndexMap[index];
   f = db.Database.Folders + folderIndex;
   p->dwSize = (DWORD)SzFolderGetUnPackSize(f);
   packSizes = db.Database.PackSizes + db.FolderStartPackStreamIndex[folderIndex];
   p->dwSizeCompressed = (DWORD)packSizes[0];
   p->dwFlags = 0;
   if(db.Database.Files[index].IsDirectory)
       p->dwFlags |= FILE_ATTRIBUTE_DIRECTORY;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL L7ZFile::OpenCompressedFile(WORD index,COMPRESSEDMODE mode)
{
   DWORD folderIndex,si,ci,outSizeCur,indices[] = {3, 2, 0};
   CFileSize *packSizes,unPackSizeSpec,packSizeSpec,unpackSize;
   BOOL bIsOk;
   CCoderInfo *coder;
   LFile *pFileTmp;
   char *buf;

   buf = NULL;
   pFileTmp = NULL;
   if(index > Count())
       goto ex_OpenFileCompressed_Error;
   folderIndex = db.FileIndexToFolderIndexMap[index-1];
   if (folderIndex == (DWORD)-1)
       goto ex_OpenFileCompressed_Error;
   folder = db.Database.Folders + folderIndex;
   unPackSizeSpec = SzFolderGetUnPackSize(folder);
   unPackSize = (DWORD)unPackSizeSpec;
   startOffset = SzArDbGetFolderStreamPos(&db,folderIndex,0);
   SzArDbGetFolderFullPackSize(&db,folderIndex,&packSizeSpec);
   if(unPackSize != unPackSizeSpec)
       goto ex_OpenFileCompressed_Error;
   if(archiveStream.InStream.Seek(&archiveStream.InStream,startOffset) != S_OK)
       goto ex_OpenFileCompressed_Error;
   if(CheckSupportedFolder(folder) != S_OK)
       goto ex_OpenFileCompressed_Error;
   bIsOk = TRUE;
   for(ci = 0; ci < folder->NumCoders; ci++){
       coder = &folder->Coders[ci];
       if (coder->MethodID == k_LZMA){
           si = 0;
           if (folder->NumCoders == 4){
               unpackSize = folder->UnPackSizes[ci];
               si = indices[ci];
               if(ci < 2){
                   outSizeCur = (size_t)unpackSize;
                   if (outSizeCur != unpackSize)
                       goto ex_OpenFileCompressed_Error;
               }
               else if (ci == 2){
                   if (unpackSize > unPackSize)
                       goto ex_OpenFileCompressed_Error;
               }
               else
                   goto ex_OpenFileCompressed_Error;
           }
           packSizes = db.Database.PackSizes + db.FolderStartPackStreamIndex[folderIndex];
           offset = GetSum(packSizes,si);
           inSize = packSizes[si];
           inExtract = 0;
           if(LzmaDecodeProperties(&state.Properties, coder->Properties.Items,(unsigned)coder->Properties.Capacity) != LZMA_RESULT_OK)
               goto ex_OpenFileCompressed_Error;
           state.Probs = (CProb *)allocImp.Alloc(LzmaGetNumProbs(&state.Properties) * sizeof(CProb));
           if (state.Probs == 0)
               goto ex_OpenFileCompressed_Error;
           if (state.Properties.DictionarySize == 0)
               state.Dictionary = 0;
           else{
               state.Dictionary = (unsigned char *)allocImp.Alloc(state.Properties.DictionarySize);
               if (state.Dictionary == 0){
                   allocImp.Free(state.Probs);
                   goto ex_OpenFileCompressed_Error;
               }
           }
           LzmaDecoderInit(&state);
       }
       else if (coder->MethodID == k_BCJ){
           if(ci != 1)
               goto ex_OpenFileCompressed_Error;
           x86_Convert_Init(x86state);
           dwPosx86[0] = dwPosx86[1] = 0;
           dwNowPos = 0;
       }
       else if(coder->MethodID == k_ARM || coder->MethodID == k_ARMThumb){
           if(ci != 1)
               goto ex_OpenFileCompressed_Error;
           dwPosx86[0] = dwPosx86[1] = 0;
           dwNowPos = 0;
       }
       else
           bIsOk = FALSE;
   }
   if(!bIsOk)
       goto ex_OpenFileCompressed_Error;
   dwAvail = dwPos = 0;
   offset = 0;
   extractMode = NORMAL;
   if(m_pBuffer == NULL){
       m_iBufferSize = 32768;
       m_pBuffer = (BYTE *)LocalAlloc(LPTR,m_iBufferSize*2+50);
       if(m_pBuffer == NULL)
           goto ex_OpenFileCompressed_Error;
       m_pBuffer1 = m_pBuffer + m_iBufferSize;
   }
   if(mode != NORMAL){
       GetTempPath(MAX_PATH,tempFileName);
       GetTempFileName(tempFileName,"iDeaS",0,tempFileName);
       if((pFileTmp = new LFile(tempFileName)) == NULL)
           goto ex_OpenFileCompressed_Error;
       if((buf = (char *)LocalAlloc(LPTR,m_iBufferSize)) == NULL){
           delete pFileTmp;
           goto ex_OpenFileCompressed_Error;
       }
       if(!pFileTmp->Open(GENERIC_WRITE,CREATE_ALWAYS)){
           LocalFree(buf);
           delete pFileTmp;
           goto ex_OpenFileCompressed_Error;
       }
       ci = unPackSize;
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
       if(ci){
           mode = NORMAL;
       }
       else{
           pFileTmp = new LFile(tempFileName);
           if(!pFileTmp->Open()){
               delete pFileTmp;
               goto ex_OpenFileCompressed_Error;
           }
           else{
               pFile->Close();
               delete pFile;
               pFile = pFileTmp;
           }
       }
   }
   extractMode = mode;
   if(mode != NORMAL){
       if(state.Probs != NULL)
           allocImp.Free(state.Probs);
       if(state.Dictionary != NULL)
           allocImp.Free(state.Dictionary);
       if(m_pBuffer != NULL)
           LocalFree(m_pBuffer);
       m_pBuffer = NULL;
       ZeroMemory(&state,sizeof(CLzmaDecoderState));
   }
   return TRUE;
ex_OpenFileCompressed_Error:
   folder = NULL;
   return FALSE;
}
//---------------------------------------------------------------------------
DWORD L7ZFile::ReadCompressedFile(LPVOID buf,DWORD dwByte)
{
   SizeT inProcessed,outSizeProcessedLoc;
   int result;
   DWORD dw,res,ci;
   CCoderInfo *coder;

   if(folder == NULL || pFile == NULL || !pFile->IsOpen())
       return 0;
   if(extractMode != NORMAL)
       return pFile->Read(buf,dwByte);
   res = dwByte;
   while(dwByte > 0){
       if(dwAvail){
           dw = (dwByte > dwAvail) ? dwAvail : dwByte;
           memcpy(buf,&m_pBuffer1[dwPos1],dw);
           dwAvail -= dw;
           buf = ((char *)buf) + dw;
           dwByte -= dw;
           dwPos1 += dw;
           dwPos += dw;
       }
       if(dwAvail == 0 && (inExtract || inSize)){
           if(inExtract < 20000 && inSize){
               if(inExtract){
                   memcpy(m_pBuffer,&m_pBuffer[(DWORD)offset],(DWORD)inExtract);
                   if(offset > inSize){
                       offset = m_iBufferSize - inExtract;
                       if(inSize < offset)
                           offset = inSize;
                   }
                   inExtract += pFile->Read(&m_pBuffer[(DWORD)inExtract],(DWORD)offset);
                   inSize -= offset;
               }
               else{
                   dw = (DWORD)(inSize > m_iBufferSize ? m_iBufferSize : inSize);
                   inExtract = pFile->Read(m_pBuffer,dw);
                   inSize -= dw;
               }
               offset = 0;
           }
           if(dwPosx86[0])
               memcpy(m_pBuffer1,&m_pBuffer1[dwPosx86[1]],dwPosx86[0]);
           for(ci = 0;ci < folder->NumCoders;ci++){
               coder = &folder->Coders[ci];
               if (coder->MethodID == k_LZMA){
                   dw = unPackSize - dwPos - dwPosx86[0];
                   if(dw > 16384)
                       dw = 16384;
                   result = LzmaDecode(&state,&m_pBuffer[(DWORD)offset],(SizeT)inExtract,&inProcessed,&m_pBuffer1[dwPosx86[0]],(SizeT)dw,&outSizeProcessedLoc);
                   if(result == LZMA_RESULT_DATA_ERROR)
                       return 0;
                   if(result != LZMA_RESULT_OK)
                       return 0;
                   offset += inProcessed;
                   inExtract -= inProcessed;
                   dw = outSizeProcessedLoc;
               }
               else{
                   if(coder->MethodID == k_BCJ)
                       dw = ::x86_Convert(m_pBuffer1,outSizeProcessedLoc+dwPosx86[0],dwNowPos,&x86state,0);
                   else if(coder->MethodID == k_ARM)
                       dw = ARM_Convert(m_pBuffer1,outSizeProcessedLoc+dwPosx86[0],dwNowPos,0);
                   else if(coder->MethodID == k_ARMThumb)
                       dw = ARMThumb_Convert(m_pBuffer1,outSizeProcessedLoc+dwPosx86[0],dwNowPos,0);
                   dwNowPos += dw;
                   dwPosx86[0] = (outSizeProcessedLoc + dwPosx86[0]) - dw;
                   dwPosx86[1] = dw;
                   if(inExtract == 0)
                       dwAvail += dwPosx86[0];
               }
           }
           dwAvail += dw;
           dwPos1 = 0;
       }
       else
           break;
   }
   return (res-dwByte);
}
//---------------------------------------------------------------------------
DWORD L7ZFile::SeekCompressedFile(LONG dwDistanceToMove,DWORD dwMoveMethod)
{
   s64 rel_ofs,cur_pos,read_size;
   DWORD dw,dw1;
   char *buf;

   if(folder == NULL || pFile == NULL || !pFile->IsOpen())
       return 0xFFFFFFFF;
   if(extractMode != NORMAL)
       return pFile->Seek(dwDistanceToMove,dwMoveMethod);
   cur_pos = dwPos;
   switch(dwMoveMethod){
       case FILE_BEGIN:
           rel_ofs = dwDistanceToMove - cur_pos;
       break;
       case FILE_CURRENT:
           rel_ofs = dwDistanceToMove;
       break;
       case FILE_END:
           rel_ofs = unPackSize + dwDistanceToMove - cur_pos;
       break;
       default:
           return (DWORD)-1;
   }
   if(rel_ofs == 0)
       return (DWORD)cur_pos;
   if(rel_ofs < 0){
       dw = (DWORD)(folder - db.Database.Folders);
       for(dw1=0;dw1<db.Database.NumFolders;dw1++){
           if(db.FileIndexToFolderIndexMap[dw1] == dw)
               break;
       }
       if(dw1 == db.Database.NumFolders)
           return (DWORD)-1;
       CloseCompressedFile();
       if(!OpenCompressedFile((WORD)(dw1+1)))
          return (DWORD)-1;
       read_size = cur_pos + rel_ofs;
       cur_pos = 0;
   }
   else
       read_size = rel_ofs;
   if(read_size < 0)
       return (DWORD)-1;
   if(read_size + cur_pos > unPackSize)
       return (DWORD)-1;
   if(read_size == 0)
       return (DWORD)cur_pos;
   if((buf = (char *)new char[m_iBufferSize]) == NULL)
       return (DWORD)-1;
   while(read_size > 0){                       
       dw = (DWORD)(read_size < m_iBufferSize ? read_size : m_iBufferSize);
       if(ReadCompressedFile(buf,dw) != dw)
           return (DWORD)-1;
       read_size -= dw;
   }
   delete []buf;
   return dwPos;
}
//---------------------------------------------------------------------------
void L7ZFile::CloseCompressedFile()
{
   dwPosx86[0] = dwPosx86[1] = 0;
   if(state.Probs != NULL)
       allocImp.Free(state.Probs);
   if(state.Dictionary != NULL)
       allocImp.Free(state.Dictionary);
   if(m_pBuffer != NULL)
       LocalFree(m_pBuffer);
   m_pBuffer = NULL;
   ZeroMemory(&state,sizeof(CLzmaDecoderState));
   folder = NULL;
   if(lstrlen(tempFileName) > 0){
       if(pFile != NULL){
           pFile->Close();
           delete pFile;
           pFile = NULL;
       }
       DeleteFile(tempFileName);
       ZeroMemory(tempFileName,sizeof(tempFileName));
   }
}
//---------------------------------------------------------------------------
void L7ZFile::Close()
{
   if(pFile != NULL){
       pFile->Close();
       delete pFile;
       pFile = NULL;                       
   }
   CloseCompressedFile();
   SzArDbExFree(&db,allocImp.Free);
   ZeroMemory(&db,sizeof(CArchiveDatabaseEx));
   ZeroMemory(&archiveStream,sizeof(archiveStream));
   ZeroMemory(&allocImp,sizeof(allocImp));
   ZeroMemory(&allocTempImp,sizeof(allocTempImp));
   if(m_pBuffer != NULL){
       LocalFree(m_pBuffer);
       m_pBuffer = NULL;  
       m_iBufferSize = 0;
   }
}
//---------------------------------------------------------------------------
CFileSize L7ZFile::GetSum(const CFileSize *values, DWORD index)
{
   CFileSize sum;
   DWORD i;

   sum = 0;
   for(i = 0; i < index; i++)
       sum += values[i];
   return sum;
}
//---------------------------------------------------------------------------
SZ_RESULT L7ZFile::CheckSupportedFolder(const CFolder *f)
{
   if (f->NumCoders < 1 || f->NumCoders > 4)
       return SZE_NOTIMPL;
   if (IS_UNSUPPORTED_CODER(f->Coders[0]))
       return SZE_NOTIMPL;
   if (f->NumCoders == 1){
       if (f->NumPackStreams != 1 || f->PackStreams[0] != 0 || f->NumBindPairs != 0)
           return SZE_NOTIMPL;
       return SZ_OK;
   }
   if (f->NumCoders == 2){
       if ((IS_NO_BCJ(f->Coders[1]) && IS_NO_ARM(f->Coders[1]) && IS_NO_THUMB(f->Coders[1])) ||
           f->NumPackStreams != 1 || f->PackStreams[0] != 0 ||
           f->NumBindPairs != 1 ||
           f->BindPairs[0].InIndex != 1 || f->BindPairs[0].OutIndex != 0)
               return SZE_NOTIMPL;
       return SZ_OK;
   }
   if (f->NumCoders == 4){
       if (IS_UNSUPPORTED_CODER(f->Coders[1]) ||
           IS_UNSUPPORTED_CODER(f->Coders[2]) ||
           IS_NO_BCJ2(f->Coders[3]))
               return SZE_NOTIMPL;
       if (f->NumPackStreams != 4 ||
           f->PackStreams[0] != 2 ||
           f->PackStreams[1] != 6 ||
           f->PackStreams[2] != 1 ||
           f->PackStreams[3] != 0 ||
           f->NumBindPairs != 3 ||
           f->BindPairs[0].InIndex != 5 || f->BindPairs[0].OutIndex != 0 ||
           f->BindPairs[1].InIndex != 4 || f->BindPairs[1].OutIndex != 1 ||
           f->BindPairs[2].InIndex != 3 || f->BindPairs[2].OutIndex != 2)
               return SZE_NOTIMPL;
       return SZ_OK;
   }
   return SZE_NOTIMPL;
}
//---------------------------------------------------------------------------
DWORD L7ZFile::Size(LPDWORD lpHigh)
{
   if(folder == NULL || pFile == NULL || !pFile->IsOpen())
       return 0;
   return unPackSize;
}
//---------------------------------------------------------------------------
BOOL L7ZFile::SetEndOfFile(DWORD dw)
{
   return FALSE;
}
//---------------------------------------------------------------------------
DWORD L7ZFile::GetCurrentPosition()
{
   if(folder == NULL || pFile == NULL || !pFile->IsOpen())
       return 0xFFFFFFFF;
   return dwPos;
}
//---------------------------------------------------------------------------
BOOL L7ZFile::IsOpen()
{
   if(pFile == NULL || !pFile->IsOpen())
       return FALSE;
   return TRUE;
}


