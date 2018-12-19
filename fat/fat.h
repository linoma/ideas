#include "ideastypes.h"
#include "dstype.h"
#include "dldiplugin.h"
#include "fstream.h"
#include "lstring.h"
#include "resource.h"

//---------------------------------------------------------------------------
#ifndef __fatH__
#define __fatH__
//---------------------------------------------------------------------------
#define ATTRIB_DIR     0x10
#define ATTRIB_LFN     0x0F
#define SECPERFAT	    128
#define SECPERCLUS	    16
#define MAXFILES	    32768
#define SECRESV	    2
#define NUMSECTORS	    0x80000
#define NUMCLUSTERS	(NUMSECTORS/SECPERCLUS)

#pragma pack(1)

typedef struct
{
	u8	jmpBoot[3];
	u8	OEMName[8];
	u16	bytesPerSector;
	u8	sectorsPerCluster;
	u16	reservedSectors;
	u8	numFATs;
	u16	rootEntries;
	u16	numSectorsSmall;
	u8	mediaDesc;
	u16	sectorsPerFAT;
	u16	sectorsPerTrk;
	u16	numHeads;
	u32	numHiddenSectors;
   u32	numSectors;
	struct
	{
       u8	driveNumber;
		u8	reserved1;
		u8	extBootSig;
		u32	volumeID;
		u8	volumeLabel[11];
		u8	fileSysType[8];
		u8	bootCode[448];
		u16 signature;
	} fat16;
} BOOT_RECORD;

typedef struct
{
	u8	name[8];
	u8	ext[3];
	u8	attrib;
	u8	reserved;
	u8	cTime_ms;
	u16	cTime;
	u16	cDate;
	u16	aDate;
	u16	startClusterHigh;
	u16	mTime;
	u16	mDate;
	u16	startCluster;
	u32	fileSize;
}	DIR_ENT;
#pragma pack(4)

typedef struct {
	int level,parent;
} FILE_INFO;

class LFat : public IFat
{
public:
   LFat(BOOL flag = TRUE);
   virtual ~LFat();
   void Destroy();
   BOOL Write(u32 address,u32 data,u8 am);
   u32 Read(u32 address,u8 am);
   BOOL Init(const char *path);
   BOOL Seek(u32 adr,u8 mode);
   IFat *Create(const char *path);
   BOOL is_Enable(int slot);
   void Release();
   int get_Path(char *path,int length);
protected:
   void Add_File(char *fname,WIN32_FIND_DATA *wfd,u32 dirent);
   void EnumFiles(const char *path,u32 dirent);
   void fatstring_to_asciiz(int dirent,char *out);
   u32 ReadFromFile(int dirent,UINT cluster,UINT offset);
   void WriteToFile(int dirent,UINT cluster,UINT offset,u32 data);
   LFile *pFile;
   BOOT_RECORD	MBR;
   DIR_ENT	*dirEntries,**dirEntryPtr,*files;
   FILE_INFO *dirEntryLink,*fileLink;
   u16	FAT16[SECPERFAT*256];
   s32	numFiles,maxLevel;
   s32	*dirEntriesInCluster,lastDirEntCluster,lastFileDataCluster,*dirEntriesParent;
   u32 currLBA,filesysFAT,filesysRootDir,filesysData;
   s32	activeDirEnt;
   u32 bufferStart,fileStartLBA,fileEndLBA;
   u8 freadBuffer[512];
   LString dirPath;
   BOOL bEnable,bInternal;
};

#endif
