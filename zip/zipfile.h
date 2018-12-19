#include "ideastypes.h"
#include <zlib.h>
#include "llist.h"
#include "fstream.h"

//---------------------------------------------------------------------------
#ifndef zipfileH
#define zipfileH
//---------------------------------------------------------------------------
#define CENTRALDIRSIZE	                22
#define FILEHEADERSIZE	                46
#define LOCALFILEHEADERSIZE	        30
#define ZIPARCHIVE_ENCR_HEADER_LEN     12
#define DEF_MEM_LEVEL                  8
#define ZIPARCHIVE_DATADESCRIPTOR_LEN  12
//---------------------------------------------------------------------------
typedef struct {
   char  m_szSignature[4];
	WORD  m_uVersionMadeBy;
	WORD  m_uVersionNeeded;
	WORD  m_uFlag;
	WORD  m_uMethod;
	WORD  m_uModTime;
	WORD  m_uModDate;
	DWORD m_uCrc32;
	DWORD m_uComprSize;
	DWORD m_uUncomprSize;
 	WORD  m_uFileNameSize;
 	WORD  m_uExtraFieldSize;
 	WORD  m_uCommentSize;
	WORD  m_uDiskStart;
	WORD  m_uInternalAttr;
	DWORD m_uExternalAttr;
	DWORD m_uOffset;
   char *nameFile;
} ZIPFILEHEADER,*LPZIPFILEHEADER;
//---------------------------------------------------------------------------
typedef struct {
   char  m_szSignature[4];
   DWORD m_uCentrDirPos;
	DWORD m_uBytesBeforeZip;
	WORD  m_uThisDisk;
	WORD  m_uDiskWithCD;
	WORD  m_uDiskEntriesNo;
	WORD  m_uEntriesNumber;
	DWORD m_uSize;
	DWORD m_uOffset;
   WORD  uCommentSize;
   char *m_pszComment;
} ZIPCENTRALDIR, *LPZIPCENTRALDIR;
//---------------------------------------------------------------------------
typedef struct {
   z_stream    m_stream;
   DWORD       m_uUncomprLeft;
   DWORD       m_uComprLeft;
	DWORD       m_uCrc32;
   DWORD       m_iBufferSize;
} ZIPINTERNALINFO,*LPZIPINTERNALINFO;
//---------------------------------------------------------------------------
class LZipFile : public LCompressedFile,public LList
{
public:
   LZipFile();
   LZipFile(const char *name);
   virtual ~LZipFile();
   void Release(){delete this;};
   BOOL Open(const char *lpFileName,BOOL bOpenAlways = FALSE);
   void Close();
   LPVOID OpenZipFile(WORD uIndex,COMPRESSEDMODE mode = NORMAL);
   BOOL CloseZipFile();
   DWORD ReadZipFile(LPVOID buf,DWORD dwByte);
   LPZIPFILEHEADER GetCurrentZipFileHeader(){return m_curZipFileHeader;};
   LPZIPFILEHEADER GetZipFileHeader(DWORD i){return (LPZIPFILEHEADER)GetItem(i);};
   BOOL AddZipFile(const char *lpFileName,const int iLevel = 9);
   BOOL DeleteZipFile(DWORD index);
   BOOL DeleteAllFiles();
   DWORD WriteZipFile(LPVOID buf,DWORD dwByte);
	BOOL Open(DWORD dwStyle = GENERIC_READ,DWORD dwCreation = OPEN_EXISTING,DWORD dwFlags = 0);
   inline DWORD Read(LPVOID lpBuffer,DWORD dwBytes){return ReadZipFile(lpBuffer,dwBytes);};
   DWORD Write(LPVOID lpBuffer,DWORD dwBytes){return WriteZipFile(lpBuffer,dwBytes);};
   inline BOOL SeekToBegin(){if(Seek() != 0xFFFFFFFF) return TRUE; return FALSE;};
   inline BOOL SeekToEnd(){if(Seek(0,FILE_END) != 0xFFFFFFFF) return TRUE; return FALSE;};
   DWORD Size(LPDWORD lpHigh = NULL);
   BOOL SetEndOfFile(DWORD dw);
   DWORD GetCurrentPosition();
	BOOL IsOpen();
   BOOL get_FileCompressedInfo(DWORD index,LPCOMPRESSEDFILEINFO p);
   BOOL AddCompressedFile(const char *lpFileName,const int iLevel = 9){return AddZipFile(lpFileName,iLevel);};
   BOOL DeleteCompressedFile(DWORD index){return DeleteZipFile(index);};
   DWORD ReadCompressedFile(LPVOID buf,DWORD dwByte){return ReadZipFile(buf,dwByte);};
   BOOL OpenCompressedFile(WORD uIndex,COMPRESSEDMODE mode = NORMAL){return OpenZipFile(uIndex,mode) != NULL ? TRUE : FALSE;};
   DWORD WriteCompressedFile(LPVOID buf,DWORD dwByte){return WriteZipFile(buf,dwByte);};
   void SetFileStream(LStream *pStream);
   void Rebuild();
   DWORD Seek(LONG dwDistanceToMove = 0,DWORD dwMoveMethod = FILE_BEGIN);
   DWORD Count(){return LList::nCount;};
protected:
   LStream *pFile;
   char fileName[MAX_PATH+1],tempFileName[MAX_PATH+1];
   int m_iBufferSize;
   enum OpenFileType{
       extract = -1,
	    nothing,
	    compress
   };
   OpenFileType m_iFileOpened;
   LPBYTE m_pBuffer;
   ZIPCENTRALDIR m_info;
   ZIPINTERNALINFO m_internalinfo;
   LPZIPFILEHEADER m_curZipFileHeader;
   BOOL bModified,bInternal,bRewriteLocal;
   COMPRESSEDMODE extractMode;
//---------------------------------------------------------------------------
   BOOL DeleteFile(DWORD index);
   BOOL RewriteLocalHeader();
   void DeleteElem(LPVOID ele);
   BOOL ReadZipFileHeader(LPZIPFILEHEADER p,BOOL bLocal = TRUE);
   DWORD WriteZipFileHeader(LPZIPFILEHEADER p,BOOL bLocal = TRUE);
   void MovePackedFiles(DWORD uStartOffset, DWORD uEndOffset, DWORD uMoveBy,BOOL bForward=FALSE);
   BOOL WriteCentralDir();
   void SlashBackslashChg(LPBYTE buffer,BOOL bReplaceSlash);
   LPZIPFILEHEADER newHeader(const char *lpFilename = NULL,BOOL bLocal = TRUE);
};
#endif
