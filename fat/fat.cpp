#include "fat.h"
#include "pluginctn.h"

//---------------------------------------------------------------------------
LFat::LFat(BOOL flag)
{
   pFile = NULL;
   dirEntries = NULL;
   dirEntryPtr = NULL;
   dirEntryLink = NULL;
   files = NULL;
   fileLink = NULL;
   bInternal = flag;
   Destroy();
}
//---------------------------------------------------------------------------
LFat::~LFat()
{
   Destroy();
}
//---------------------------------------------------------------------------
void LFat::Destroy()
{
   if(dirEntries != NULL)
       LocalFree(dirEntries);
   dirEntries = NULL;
   dirEntryPtr = NULL;
   dirEntryLink = NULL;
   if(files != NULL)
       LocalFree(files);
   files = NULL;
   fileLink = NULL;
   dirEntriesParent = NULL;
   if(pFile != NULL)
       delete pFile;
   pFile = NULL;
   activeDirEnt = -1;
   fileEndLBA = fileStartLBA = 0;
	numFiles  = 0;
	maxLevel  = -1;
}
//---------------------------------------------------------------------------
void LFat::Release()
{
   if(!bInternal)
       delete this;
}
//---------------------------------------------------------------------------
IFat *LFat::Create(const char *path)
{
   LFat *fat;
   LString s;

   fat = new LFat(FALSE);
   if(fat == NULL)
       return NULL;
   if(path == NULL)
       s = dirPath;
   else
       s = path;
   if(!fat->Init(s.c_str())){
       delete fat;
       return NULL;
   }
   return (IFat *)fat;
}
//---------------------------------------------------------------------------
int LFat::get_Path(char *path,int length)
{
   if(path == NULL)
       return dirPath.Length();
   if(length > dirPath.Length())
       length = dirPath.Length();
   lstrcpyn(path,dirPath.c_str(),length);
   return length;
}
//---------------------------------------------------------------------------
BOOL LFat::is_Enable(int slot)
{
   PlugIn *p;

   if(!bInternal)
       return TRUE;
	if(!bEnable || pPlugInContainer == NULL ||
   	(p = pPlugInContainer->get_PlugInList(PIL_DLDI)->get_ActivePlugIn()) == NULL)
   	return FALSE;
	if(slot == -1)
   	return TRUE;
	if(slot == -1)
   	return TRUE;
   if(slot == 1)
   	return p->IsAttribute(PIT_SLOT2) != 0 ? FALSE : TRUE;
   if(slot == 2)
   	return p->IsAttribute(PIT_SLOT2) != 0 ? TRUE : FALSE;
	return FALSE;
}
//---------------------------------------------------------------------------
void LFat::Add_File(char *fname,WIN32_FIND_DATA *wfd,u32 dirent)
{
	s32 i,j,k,n;
	u8 chk;
	s8 *p;
   const int lfnPos[13] = {1,3,5,7,9,14,16,18,20,22,24,28,30};

   if(numFiles < MAXFILES-1){
		if(strncmp(fname,"..",2) != 0) {
           for(i=0;i < lstrlen(fname);i++){
				if(fname[i]=='.')
                   break;
           }
			if(i==0 && strncmp(fname,".",1) == 0)
               i = 1;
			for(j=0;j<i;j++)
				files[numFiles].name[j] = fname[j];
			for(;j<8;j++)
				files[numFiles].name[j] = 0x20;
			for(j=0;j<3;j++){
				if((j+i+1) >= lstrlen(fname))
                   break;
				files[numFiles].ext[j] = fname[j+i+1];
			}
			for(;j<3;j++)
				files[numFiles].ext[j] = 0x20;
			if(lstrlen(wfd->cAlternateFileName) > 0) {
               if(strchr(wfd->cAlternateFileName,'~')){
                   k = lstrlen(wfd->cFileName);
                   chk = 0;
                   p = (s8 *)files[numFiles].name;
                   for(i=0;i < 11;i++)
		                chk = (u8)((chk << 7) + (chk >> 1) + p[i]);
                   numFiles += (k / 13) + ((k % 13) != 0 ? 1 : 0);
				    n = 0;
				    j = 13;
				    for(i=0;i<k;i++){
                       if(j == 13){
					        n++;
						    p = (s8 *)&files[numFiles-n].name[0];
						    p[0] = (s8)n;
						    p[0xB] = ATTRIB_LFN;
						    p[0xD] = chk;
                           fileLink[numFiles-n].parent = dirent;
                           j = 0;
                       }
					    *(p + lfnPos[j]) = wfd->cFileName[i];
					    *(p + lfnPos[j]+1) = 0;
					    j++;
                   }
				    for(;j<13;j++){
				        *(p + lfnPos[j]) = wfd->cFileName[i];
					    *(p + lfnPos[j]+1) = 0;
                   }
				    p[0] |= 0x40;
               }
				for(i=0;i<lstrlen(fname);i++){
					if(fname[i]=='.')
                       break;
               }
				if(i==0 && strncmp(fname,".",1) == 0)
                   i = 1;
				for(j=0;j<i;j++)
					files[numFiles].name[j] = fname[j];
				for(;j<8;j++)
					files[numFiles].name[j] = 0x20;
				for(j=0;j<3;j++){
					if((j+i+1) >= lstrlen(fname))
                       break;
					files[numFiles].ext[j] = fname[j+i+1];
				}
				for(;j<3;j++)
					files[numFiles].ext[j] = 0x20;
           }
           files[numFiles].fileSize = wfd->nFileSizeLow;
           if(wfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				files[numFiles].attrib = ATTRIB_DIR;
           else
               files[numFiles].attrib = 0x20;
			fileLink[numFiles++].parent = dirent;
		}
       else if(dirent > 0){
			lstrcpy((char*)&files[numFiles].name[0],"..      ");
			lstrcpy((char*)&files[numFiles].ext[0],"   ");
			fileLink[numFiles].parent = dirent;
			files[numFiles++].attrib = ATTRIB_DIR;
		}
	}
}
//---------------------------------------------------------------------------
void LFat::EnumFiles(const char *path,u32 dirent)
{
	WIN32_FIND_DATA	wfd;
	HANDLE hFind;
	LString s;
	char *fname;

	maxLevel++;
   dirEntriesParent[maxLevel] = dirent;
   s = LString(path).Path() + DPS_PATH;
	s += "*";
	if((hFind = FindFirstFile(s.c_str(),&wfd)) == INVALID_HANDLE_VALUE)
       return;
	fname = lstrlen(wfd.cAlternateFileName) > 0 ? wfd.cAlternateFileName : wfd.cFileName;
   s = LString(fname).UpperCase();
	Add_File(s.c_str(),&wfd,dirent);
	while(FindNextFile(hFind,&wfd) != 0){
       fname = lstrlen(wfd.cAlternateFileName) > 0 ? wfd.cAlternateFileName : wfd.cFileName;
       s = LString(fname).UpperCase();
	    Add_File(s.c_str(),&wfd,dirent);
		if(numFiles == MAXFILES-1)
           break;
       if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fname[0] != '.') {
           s = LString(path).Path();
           s += DPS_PATH;
           s += fname;
           s += DPS_PATH;
           EnumFiles(s.c_str(),numFiles-1);
       }
   }
	FindClose(hFind);
}
//---------------------------------------------------------------------------
BOOL LFat::Init(const char *path)
{
	int i,j,l,clust,numClusters,rootCluster,clusterNum,k,update,j1;
	LString s;

   Destroy();
   s = LString(path).LowerCase();
   bEnable = s.Pos(".zip") >= 0 ? FALSE : TRUE;
   if(!is_Enable(-1))
       return TRUE;
	files = (DIR_ENT*)LocalAlloc(LPTR,MAXFILES*sizeof(DIR_ENT)+MAXFILES*sizeof(FILE_INFO)+MAXFILES*sizeof(s32));
   if(files == NULL)
       return FALSE;
	fileLink = (FILE_INFO*)(files + MAXFILES);
   dirEntriesParent = (s32 *)(fileLink + MAXFILES);
   dirPath = LString(path).Path();
	EnumFiles(path,0);
   dirEntries = (DIR_ENT *)LocalAlloc(LPTR,MAXFILES*sizeof(DIR_ENT) + MAXFILES*sizeof(FILE_INFO)+ NUMCLUSTERS*sizeof(int) + NUMCLUSTERS*sizeof(DIR_ENT*));
   if(dirEntries == NULL){
       LocalFree(files);
       files = NULL;
       return FALSE;
   }
	dirEntryLink        = (FILE_INFO*)(dirEntries + MAXFILES);
	dirEntriesInCluster = (s32 *)(dirEntryLink + MAXFILES);
	dirEntryPtr         = (DIR_ENT**)(dirEntriesInCluster + NUMCLUSTERS);
	MBR.bytesPerSector = 512;
	MBR.numFATs = 1;
	lstrcpy((char *)MBR.OEMName,"iDeaS");
	lstrcpy((char *)MBR.fat16.fileSysType,"FAT16  ");
	MBR.reservedSectors = SECRESV;
	MBR.numSectors = NUMSECTORS;
	MBR.numSectorsSmall = 0;
   MBR.mediaDesc = 0xFF;
	MBR.sectorsPerCluster = SECPERCLUS;
	MBR.sectorsPerFAT = SECPERFAT;
	MBR.rootEntries = 512;
	MBR.fat16.signature = 0xAA55;
	filesysFAT = MBR.reservedSectors;
	filesysRootDir = filesysFAT + (MBR.numFATs * MBR.sectorsPerFAT);
	filesysData = filesysRootDir + ((MBR.rootEntries * sizeof(DIR_ENT)) / 512);
   clusterNum = rootCluster = (SECRESV + SECPERFAT) / SECPERCLUS;
	numClusters = 0;
   clust = 2;
	for(i=0;i<=maxLevel;i++){
       k = numClusters * 512 * 16 / sizeof(DIR_ENT);
       update = 1;
		for(j=0;j<numFiles;j++){
			if(fileLink[j].parent != dirEntriesParent[i] || k > MAXFILES - 1)
               continue;
			dirEntryLink[k].parent = k;
           fileLink[j].level = k;
			dirEntries[k] = files[j];
			if(files[j].attrib & ATTRIB_DIR){
               if(strncmp((char *)files[j].name,"..      ",8) == 0)
					dirEntries[k].startCluster = (u16)dirEntryLink[k].parent;
           }
           if(dirEntryPtr[clusterNum] == NULL)
				dirEntryPtr[clusterNum] = &dirEntries[k];
           if(++dirEntriesInCluster[clusterNum] == 256){
               k = numClusters * 512 * 16 / sizeof(DIR_ENT);
               for(j1=0;j1<dirEntriesInCluster[clusterNum];j1++,k++)
                   dirEntryLink[k].parent = fileLink[dirEntriesParent[i]].level;
               if(i){
                   dirEntries[fileLink[dirEntriesParent[i]].level].startCluster = (u16)numClusters;
                   update = 0;
               }
               FAT16[numClusters] = (u16)(numClusters + 1);
               clusterNum++;
               numClusters++;
           }
           else
               k++;
		}
       if(i && update)
           dirEntries[fileLink[dirEntriesParent[i]].level].startCluster = (u16)numClusters;
       k = numClusters * 512 * 16 / sizeof(DIR_ENT);
       for(j=0;j<dirEntriesInCluster[clusterNum];j++,k++)
           dirEntryLink[k].parent = fileLink[dirEntriesParent[i]].level;
       FAT16[numClusters] = 0xFFFF;
		clusterNum++;
       numClusters++;
       if((clusterNum - rootCluster) < 2){
           dirEntryPtr[clusterNum] = &dirEntries[numClusters * 512 * 16 / sizeof(DIR_ENT)];
           FAT16[numClusters] = 0xFFFF;
           clusterNum++;
           numClusters++;

       }
	}
   LocalFree(files);
   files = NULL;
   fileLink = NULL;
	lastDirEntCluster = clusterNum-1;
   clust = clusterNum;
	for(i=0;i<MAXFILES;i++){
		if(*((u32 *)dirEntries[i].name)!= 0 && (dirEntries[i].attrib & ATTRIB_DIR) == 0 && dirEntries[i].attrib != ATTRIB_LFN){
           dirEntries[i].startCluster = (u16)clust;
	        clust += dirEntries[i].fileSize / (512*SECPERCLUS) + 1;
		}
	}
	lastFileDataCluster = clust-1;
	for(i=0;i<MAXFILES;i++){
       if(!dirEntries[i].startCluster)
           continue;
		l = dirEntries[i].fileSize - (512*SECPERCLUS);
		for(j=0;l > 0;l -= 512*16,j++){
			if(dirEntries[i].startCluster+j < MAXFILES)
				FAT16[dirEntries[i].startCluster+j] = (u16)(dirEntries[i].startCluster+j+1);
		}
		if((dirEntries[i].attrib & ATTRIB_DIR) == 0){
			if(dirEntries[i].startCluster+j < MAXFILES)
				FAT16[dirEntries[i].startCluster+j] = 0xFFFF;
		}
	}
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LFat::Write(u32 address,u32 data,u8 am)
{
	u8 *p;
	int i;
	u32 cluster,cluster2,fileLBA;

   return FALSE;
   if(!is_Enable(-1))
       return 0;
	cluster = (currLBA / (512 * SECPERCLUS));
	cluster2 = (((currLBA>>9) - filesysData) / SECPERCLUS) + 2;
	if(currLBA < 512){
       p = (u8 *)&MBR;
       *(u32 *)(p + currLBA) = data;
	}
   else if(currLBA >= (filesysFAT<<9) && currLBA < (filesysRootDir<<9)){
       p = (u8 *)&FAT16[0];
		*(u32 *)(p + (currLBA-(filesysFAT<<9))) = data;
	}
   else if(currLBA >= (filesysRootDir<<9) && cluster <= lastDirEntCluster){
       i = currLBA-(((cluster-(filesysRootDir/SECPERCLUS))*SECPERCLUS+filesysRootDir)*512);
       if(data && i >= dirEntriesInCluster[cluster]*32)
           dirEntriesInCluster[cluster]++;
       p = (u8 *)dirEntryPtr[cluster];
       *(u32 *)(p + i) = data;
   }
   else{
       fileLBA = currLBA - ((filesysData-32)*512);
       for(i=0;i<MAXFILES;i++){
			if((fileLBA>=(dirEntries[i].startCluster*512*SECPERCLUS)) && (fileLBA <(dirEntries[i].startCluster*512*SECPERCLUS)+dirEntries[i].fileSize)) {
				cluster = fileLBA / (512 * SECPERCLUS);
				WriteToFile(i,cluster-dirEntries[i].startCluster,fileLBA&((512*SECPERCLUS)-1),data);
				return TRUE;
			}
		}
   }
   currLBA += 4;
}
//---------------------------------------------------------------------------
u32 LFat::Read(u32 address,u8 am)
{
	u8 *p;
	int i;
	u32 cluster2,fileLBA,s,cluster;

   if(!is_Enable(-1))
       return 0;
   s = 0;
   cluster = currLBA / 512 / SECPERCLUS;
	cluster2 = (((currLBA / 512) - filesysData) / SECPERCLUS) + 2;
	if(currLBA < 512){
       p = (u8 *)&MBR;
       s = *(u32 *)(p + currLBA);
	}
   else if(currLBA >= (filesysFAT*512) && currLBA < (filesysRootDir*512)){
       p = (u8 *)&FAT16[0];
		s = *(u32 *)(p + (currLBA-(filesysFAT*512)));
	}
   else if(currLBA >= (filesysRootDir*512) && cluster <= lastDirEntCluster){
       i = currLBA - (((cluster-(filesysRootDir/SECPERCLUS))*SECPERCLUS+filesysRootDir)*512);
       if(i < dirEntriesInCluster[cluster]*32){
       	p = (u8 *)dirEntryPtr[cluster];
	    	s = *(u32 *)(p + i);
       }
	}
   else if(cluster2 > lastDirEntCluster && cluster2 <= lastFileDataCluster){
       s = (u32)-1;
		fileLBA = currLBA - ((filesysData-32)*512);
		if(fileLBA >= fileStartLBA && fileLBA < fileEndLBA){
			cluster = fileLBA / (512 * SECPERCLUS);
			s = ReadFromFile(activeDirEnt,cluster-dirEntries[activeDirEnt].startCluster,fileLBA&((512*SECPERCLUS)-1));
		}
       else{
			for(i=0;i<MAXFILES;i++){
				if(dirEntries[i].attrib != ATTRIB_LFN && (fileLBA>=(dirEntries[i].startCluster*512*SECPERCLUS)) && (fileLBA <(dirEntries[i].startCluster*512*SECPERCLUS)+dirEntries[i].fileSize)) {
					cluster = fileLBA / (512 * SECPERCLUS);
					s = ReadFromFile(i,cluster-dirEntries[i].startCluster,fileLBA&((512*SECPERCLUS)-1));
					break;
				}
			}
		}
	}
	currLBA += 4;
	return s;
}
//---------------------------------------------------------------------------
BOOL LFat::Seek(u32 adr,u8 mode)
{
   switch(mode){
       case 0:
           currLBA = adr;
       break;
       case 1:
           currLBA += adr;
       break;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
void LFat::WriteToFile(int dirent,UINT cluster,UINT offset,u32 data)
{
	char fname[32];
	int i,j;
   LString s,fileName;

	offset += cluster*512*SECPERCLUS;
	if(dirent == activeDirEnt){
       pFile->Seek(offset,FILE_BEGIN);
       pFile->Write(&data,4);
       return;
	}
	if(activeDirEnt != -1 && pFile){
		delete pFile;
       pFile = NULL;
   }
   fileName = dirPath;
   fileName += DPS_PATH;
   s.Capacity(MAX_PATH+1);
   j = dirent;
   while(dirEntryLink[j].parent > 0){
		for(i=0;i<j;i++){
			if(dirEntryLink[j].parent == dirEntryLink[i].level && (dirEntries[i].attrib & ATTRIB_DIR) != 0){
				fatstring_to_asciiz(i,s.c_str());
               fileName += s;
               fileName += DPS_PATH;
				j = i;
				break;
			}
		}
	}
	fatstring_to_asciiz(dirent,fname);
   fileName += fname;
   if((pFile = new LFile(fileName.c_str())) == NULL || !pFile->Open(GENERIC_WRITE,OPEN_ALWAYS))
       return;
   pFile->Write(&data,4);
	activeDirEnt = dirent;
}
//---------------------------------------------------------------------------
u32 LFat::ReadFromFile(int dirent,UINT cluster,UINT offset)
{
	char fname[32];
	int j;
   LString s,fileName;

	offset += cluster*512*SECPERCLUS;
	if(dirent == activeDirEnt){
		if(offset < bufferStart || (offset >= bufferStart + 512)){
           pFile->Seek(offset,FILE_BEGIN);
           pFile->Read(freadBuffer,512);
			bufferStart = offset;
		}
		return *((u32 *)&freadBuffer[offset-bufferStart]);
	}
	if(activeDirEnt != -1 && pFile){
		delete pFile;
       pFile = NULL;
   }
	bufferStart = 0;
	activeDirEnt = -1;
	fileStartLBA = 0;
	fileEndLBA = 0;
   s.Capacity(MAX_PATH+1);
   j = dirEntryLink[dirent].parent;
   while(j > 0){
		fatstring_to_asciiz(j,s.c_str());
       s += DPS_PATH;
       fileName = s + fileName;
		j = dirEntryLink[j].parent;
	}
	fatstring_to_asciiz(dirent,fname);
   s = dirPath;
   s += DPS_PATH;
   fileName = s + fileName;
   fileName += fname;
   if((pFile = new LFile(fileName.c_str())) == NULL || !pFile->Open())
       return 0;
   pFile->Seek(offset,FILE_BEGIN);
   pFile->Read(freadBuffer,512);
  	bufferStart = offset;
	activeDirEnt = dirent;
	fileStartLBA = (dirEntries[dirent].startCluster*512*SECPERCLUS);
	fileEndLBA = fileStartLBA + dirEntries[dirent].fileSize;
	return *((u32 *)&freadBuffer[offset-bufferStart]);
}
//---------------------------------------------------------------------------
void LFat::fatstring_to_asciiz(int dirent,char *out)
{
	int i,j;

	for(i=0;i<8;i++){
		if(dirEntries[dirent].name[i] == ' ')
           break;
		out[i] = dirEntries[dirent].name[i];
	}
	if((dirEntries[dirent].attrib & 0x10)==0){
		out[i++] = '.';
		for(j=0;j<3;j++){
			if(dirEntries[dirent].ext[j] == ' ')
               break;
			out[i++] = dirEntries[dirent].ext[j];
		}
	}
	out[i] = 0;
}

