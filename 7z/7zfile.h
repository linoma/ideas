#include "ideastypes.h"
#include "fstream.h"
#include <7zCrc.h>
#include <7zIn.h>
#include <7zExtract.h>
#include <LzmaDecode.h>
#include <BranchX86.h>
#include <BranchX86_2.h>
#include <BranchARM.h>
#include <BranchARMThumb.h>

//---------------------------------------------------------------------------
#ifndef __7zfileH__
#define __7zfileH__

#define k_Copy     0
#define k_LZMA     0x30101
#define k_BCJ      0x03030103
#define k_BCJ2     0x0303011B
#define k_ARM      0x03030501
#define k_ARMThumb 0x03030701

#define IS_UNSUPPORTED_METHOD(m) ((m) != k_Copy && (m) != k_LZMA)
#define IS_UNSUPPORTED_CODER(c) (IS_UNSUPPORTED_METHOD(c.MethodID) || c.NumInStreams != 1 || c.NumOutStreams != 1)
#define IS_NO_BCJ(c)   (c.MethodID != k_BCJ || c.NumInStreams != 1 || c.NumOutStreams != 1)
#define IS_NO_BCJ2(c)  (c.MethodID != k_BCJ2 || c.NumInStreams != 4 || c.NumOutStreams != 1)
#define IS_NO_ARM(c)   (c.MethodID != k_ARM || c.NumInStreams != 1 || c.NumOutStreams != 1)
#define IS_NO_THUMB(c) (c.MethodID != k_ARMThumb || c.NumInStreams != 1 || c.NumOutStreams != 1)
//---------------------------------------------------------------------------
typedef struct _CFileInStream
{
  ISzInStream InStream;
  LStream *File;
} CFileInStream;
//---------------------------------------------------------------------------
class L7ZFile : public LCompressedFile
{
public:
   L7ZFile();
   L7ZFile(const char *name);
   virtual ~L7ZFile();
	BOOL Open(DWORD dwStyle = GENERIC_READ,DWORD dwCreation = OPEN_EXISTING,DWORD dwFlags = 0);
   void Close();
   DWORD Read(LPVOID lpBuffer,DWORD dwBytes){return ReadCompressedFile(lpBuffer,dwBytes);};
   DWORD Write(LPVOID lpBuffer,DWORD dwBytes){return WriteCompressedFile(lpBuffer,dwBytes);};
   DWORD Seek(LONG dwDistanceToMove = 0,DWORD dwMoveMethod = FILE_BEGIN){return SeekCompressedFile(dwDistanceToMove,dwMoveMethod);};
   inline BOOL SeekToBegin(){if(Seek() != 0xFFFFFFFF) return TRUE; return FALSE;};
   inline BOOL SeekToEnd(){if(Seek(0,FILE_END) != 0xFFFFFFFF) return TRUE; return FALSE;};
   DWORD Size(LPDWORD lpHigh = NULL);
   BOOL SetEndOfFile(DWORD dw);
   DWORD GetCurrentPosition();
	BOOL IsOpen();
   void Release(){delete this;};
	void SetFileStream(LStream *pStream){};
   BOOL AddCompressedFile(const char *lpFileName,const int iLevel = 9){return FALSE;};
   BOOL DeleteCompressedFile(DWORD index){return FALSE;};
   DWORD ReadCompressedFile(LPVOID buf,DWORD dwByte);
   BOOL OpenCompressedFile(WORD index,COMPRESSEDMODE mode = NORMAL);
   DWORD WriteCompressedFile(LPVOID buf,DWORD dwByte){return 0;};
   void Rebuild(){};
   BOOL get_FileCompressedInfo(DWORD index,LPCOMPRESSEDFILEINFO p);
   DWORD Count(){return db.Database.NumFiles;};
   BOOL Open(const char *lpFileName,BOOL bOpenAlways = FALSE);
   void CloseCompressedFile();
   DWORD SeekCompressedFile(LONG dwDistanceToMove,DWORD dwMoveMethod);
protected:
   SZ_RESULT CheckSupportedFolder(const CFolder *f);
   CFileSize GetSum(const CFileSize *values, DWORD index);
   /*
       Variables
   */
   LStream *pFile;
   char fileName[MAX_PATH],tempFileName[MAX_PATH];
   CArchiveDatabaseEx db;
   CFileInStream archiveStream;
   CFolder *folder;
   ISzAlloc allocImp;
   ISzAlloc allocTempImp;
   CFileSize inSize,inExtract,startOffset,offset;
   CLzmaDecoderState state;
   LPBYTE m_pBuffer,m_pBuffer1;
   int m_iBufferSize,dwPosx86[2];
   DWORD unPackSize,dwNowPos,dwAvail,dwPos,dwPos1;
   UInt32 x86state;
   COMPRESSEDMODE extractMode;
};
#endif
