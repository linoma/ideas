#include "elf.h"
#include "lds.h"

//---------------------------------------------------------------------------
LElfFile::LElfFile(const char *name) : LFile(name)
{
   ProgramHeader = NULL;
   SectionHeader = NULL;
   CompileUnits = NULL;
   dbgHeader = NULL;
	Cies = NULL;
	Fdes = NULL;
	FdeCount = 0;
	SymbolsCount = 0;
  	Symbols = NULL;
   SymbolsStrTab = NULL;
   Locations = NULL;
   LocationsCount = 0;
   cache = NULL;
   dwCachePos = dwCacheStart = dwCacheEnd = 0;
}
//---------------------------------------------------------------------------
LElfFile::~LElfFile()
{
   Close();
}
//---------------------------------------------------------------------------
void LElfFile::Close()
{
	CompileUnit *unit,*unit1;
	int i;
	Function *func,*func1;
   elf_Object *o,*o1;
	Type *t,*t1;
	ELFcie *cie,*cie1;

   if(ProgramHeader != NULL)
       delete[]ProgramHeader;
   ProgramHeader = NULL;
   if(SectionHeader != NULL)
       delete[]SectionHeader;
   SectionHeader = NULL;
   unit = CompileUnits;
   while(unit != NULL){
   	unit1 = unit->next;
       if(unit->name)
       	LocalFree(unit->name);
       if(unit->compdir)
       	LocalFree(unit->compdir);
       if(unit->abbrevs != NULL){
       	for(i=0;i<121;i++){
           	if(unit->abbrevs[i] != NULL && unit->abbrevs[i]->attrs != NULL){
               	if(unit->abbrevs[i]->attrs->string != NULL){
               		switch(unit->abbrevs[i]->attrs->form){
                   		case DW_FORM_string:
                       	case DW_FORM_strp:
             					LocalFree(unit->abbrevs[i]->attrs->string);
                       	break;
                       	case DW_FORM_block:
                       	case DW_FORM_block1:
								if(unit->abbrevs[i]->attrs->block->data)
                           		LocalFree(unit->abbrevs[i]->attrs->block->data);
                               LocalFree(unit->abbrevs[i]->attrs->block);
                       	break;
                   	}
  					}
               	LocalFree(unit->abbrevs[i]->attrs);
               }
           }
       }
       if(unit->lineInfoTable != NULL){
       	if(unit->lineInfoTable->files != NULL){
           	for(i=0;i<unit->lineInfoTable->fileCount;i++){
					if(unit->lineInfoTable->files[i].name != NULL)
                   	LocalFree(unit->lineInfoTable->files[i].name);
               }
               LocalFree(unit->lineInfoTable->files);
           }
           if(unit->lineInfoTable->lines != NULL)
              	LocalFree(unit->lineInfoTable->lines);
           if(unit->lineInfoTable->dirs != NULL){
           	for(i=0;i<unit->lineInfoTable->dirCount;i++){
					if(unit->lineInfoTable->dirs[i] != NULL)
                   	LocalFree(unit->lineInfoTable->dirs[i]);
               }
               LocalFree(unit->lineInfoTable->dirs);
           }
       	LocalFree(unit->lineInfoTable);
       }
       if(unit->functions != NULL){
       	func = unit->functions;
           while(func != NULL){
           	func1 = func->next;
       		LocalFree(func);
               func = func1;
           }
       }
       if(unit->variables != NULL){
           o = unit->variables;
           while(o != NULL){
           	o1 = o->next;
       		LocalFree(o);
               o= o1;
       	}
       }
       if(unit->types != NULL){
           t = unit->types;
           while(t != NULL){
           	t1 = t->next;
       		LocalFree(t);
               t = t1;
           }
       }
       LocalFree(unit);
       unit = unit1;
   }
   CompileUnits = NULL;
   if(dbgInfo.ranges != NULL)
   	LocalFree(dbgInfo.ranges);
   dbgInfo.ranges = NULL;
   cie = Cies;
   while(cie != NULL){
   	cie1 = cie->next;
       if(cie->data != NULL)
       	LocalFree(cie->data);
       LocalFree(cie);
		cie = cie1;
   }
   Cies = NULL;
	if(Fdes != NULL){
   	for(i=0;i<FdeCount;i++){
       	if(Fdes[i]->data != NULL)
           	LocalFree(Fdes[i]->data);
       	LocalFree(Fdes[i]);
       }
       LocalFree(Fdes);
   }
   Fdes = NULL;
   if(Symbols != NULL)
   	LocalFree(Symbols);
   Symbols = NULL;
   if(SymbolsStrTab != NULL)
   	LocalFree(SymbolsStrTab);
   SymbolsStrTab = NULL;
   if(Locations != NULL){
       for(i=0;i<LocationsCount;i++){
           if(Locations[i].block != NULL)
               LocalFree(Locations[i].block);
       }
       LocalFree(Locations);
   }
   Locations = NULL;
   LocationsCount = 0;
   LFile::Close();
   ZeroMemory(&header,sizeof(ELFHeader));
   if(cache != NULL)
       LocalFree(cache);
   cache = NULL;
   dwCachePos = dwCacheStart = dwCacheEnd = 0;
}
//---------------------------------------------------------------------------
DWORD LElfFile::Read(LPVOID lpBuffer,DWORD dwBytes)
{
   DWORD dwRead,dwLength;

   if(cache == NULL || dwCacheEnd == 0){
       if(cache == NULL)
           cache = (char *)LocalAlloc(LPTR,16384);
       if(cache != NULL){
           dwRead = LFile::Read(cache,16384);
           dwCacheEnd = dwCacheStart + dwRead;
           dwCachePos = dwCacheStart;
       }
   }
   if(cache != NULL){
       for(dwLength = dwBytes;dwLength > 0;){
           if((dwRead = dwCacheEnd - dwCachePos) == 0){
               dwRead = LFile::Read(cache,16384);
               dwCacheStart = dwCacheEnd;
               dwCacheEnd = dwCacheStart + dwRead;
               dwCachePos = dwCacheStart;
               if((dwRead = dwCacheEnd - dwCachePos) == 0)
                   return 0;
           }
           if(dwRead > dwLength)
               dwRead = dwLength;
           CopyMemory(lpBuffer,&cache[dwCachePos - dwCacheStart],dwRead);
           lpBuffer = ((char *)lpBuffer) + dwRead;
           dwLength -= dwRead;
           dwCachePos += dwRead;
       }
       return dwBytes - dwLength;
   }
   return LFile::Read(lpBuffer,dwBytes);
}
//---------------------------------------------------------------------------
DWORD LElfFile::Seek(LONG dwDistanceToMove,DWORD dwMoveMethod)
{
   s64 rel_ofs;

   switch(dwMoveMethod){
       case FILE_BEGIN:
           rel_ofs = dwDistanceToMove;
       break;
       case FILE_CURRENT:
           rel_ofs = dwCachePos + dwDistanceToMove;
       break;
       case FILE_END:
           rel_ofs = Size() + dwDistanceToMove;
       break;
       default:
           return (DWORD)-1;
   }
   if(rel_ofs == 0)
       return dwCachePos;
   if(rel_ofs < 0)
       rel_ofs = 0;
   if(rel_ofs < dwCacheStart || rel_ofs >= dwCacheEnd){
       dwCacheEnd = 0;
       dwCachePos = dwCacheStart = (DWORD)rel_ofs;
       return LFile::Seek(dwDistanceToMove,dwMoveMethod);
   }
//   return LFile::Seek(dwDistanceToMove,dwMoveMethod);
   return (DWORD)(dwCachePos = (DWORD)rel_ofs);
}
//---------------------------------------------------------------------------
BOOL LElfFile::Open(DWORD dwStyle,DWORD dwCreation,DWORD dwFlags)
{
   int i;
   ELFSectionHeader *h;
   DWORD abbrev,total,end,ddata;
   CompileUnit *last,*unit,*comp;
 	ARanges *r;

   if(!LFile::Open(GENERIC_READ,OPEN_EXISTING,dwFlags))
       return FALSE;
   if(header.magic == 0x464C457F && header.clazz == 1 && header.e_machine == 40)
       return TRUE;
   SeekToBegin();
   if(Read(&header,sizeof(ELFHeader)) != sizeof(ELFHeader))
       return FALSE;
   if(header.magic != 0x464C457F || header.clazz != 1 || header.e_machine != 40)
       return FALSE;
   if((ProgramHeader = new ELFProgramHeader[header.e_phnum]) == NULL)
       return FALSE;
   Seek(header.e_phoff,FILE_BEGIN);
   for(i=0;i<header.e_phnum;i++){
       Read(&ProgramHeader[i],sizeof(ELFProgramHeader));
       if(header.e_phentsize != sizeof(ELFProgramHeader))
           Seek(header.e_phentsize - sizeof(ELFProgramHeader),FILE_CURRENT);
   }
   Seek(header.e_shoff,FILE_BEGIN);
   if((SectionHeader = new ELFSectionHeader[header.e_shnum]) == NULL)
       return FALSE;
   for(i=0;i<header.e_shnum;i++){
       Read(&SectionHeader[i],sizeof(ELFSectionHeader));
       if(header.e_shentsize != sizeof(ELFSectionHeader))
           Seek(header.e_shentsize - sizeof(ELFSectionHeader),FILE_CURRENT);
   }
   if((dbgHeader = GetSectionByName(".debug_info")) == NULL)
		return TRUE;
   h = GetSectionByName(".debug_abbrev");
   abbrev = ReadSection(h);
   if((h = GetSectionByName(".debug_str")) != NULL)
       dbgStringsOfs = ReadSection(h);
   dbgInfo.debugdata = 0;
   dbgInfo.infodata = ReadSection(dbgHeader);
   total = dbgHeader->size;
   end = dbgInfo.infodata + total;
   ddata = dbgInfo.infodata;
   last = NULL;
   unit = NULL;
   while(ddata < end){
       unit = ParseCompUnit(ddata,abbrev);
       unit->offset = ddata;
       ParseLineInfo(unit,0);
       if(last == NULL)
           CompileUnits = unit;
       else
           last->next = unit;
       last = unit;
       ddata += 4 + unit->length;
   }
	ParseAranges(0);
   comp = CompileUnits;
   while(comp) {
   	r = dbgInfo.ranges;
      	for(i = 0; i < dbgInfo.numRanges; i++){
        	if(r[i].offset == comp->offset) {
          		comp->ranges = &r[i];
          		break;
        	}
       }
       comp = comp->next;
 	}
   ParseCFA(0);
   ReadSymtab(0);
   ParseLocations(0);
   return TRUE;
}//---------------------------------------------------------------------------
LONG LElfFile::ReadSignedLEB128(DWORD data,int *bytesRead)
{
   LONG result;
   int shift,count;
   BYTE byte;

   result = 0;
   shift = count = 0;
   Seek(data,FILE_BEGIN);
   do {
       Read(&byte,sizeof(BYTE));
       count++;
       result |= (byte & 0x7f) << shift;
       shift += 7;
   } while(byte & 0x80);
   if(shift < 32 && (byte & 0x40))
       result |= -(1 << shift);
   *bytesRead = count;
   return result;
}
//---------------------------------------------------------------------------
DWORD LElfFile::ReadLEB128(DWORD data,int *bytesRead)
{
   DWORD result;
   int shift,count;
   BYTE byte;

   result = 0;
   shift = count = 0;
   Seek(data,FILE_BEGIN);
   do{
       Read(&byte,sizeof(BYTE));
       count++;
       result |= (byte & 0x7f) << shift;
       shift += 7;
   }while(byte & 0x80);
   *bytesRead = count;
   return result;
}
//---------------------------------------------------------------------------
DWORD LElfFile::Read4Bytes(DWORD ofs)
{
   DWORD value;

	SeekRead(ofs,&value,sizeof(DWORD));
   return value;
}
//---------------------------------------------------------------------------
WORD LElfFile::Read2Bytes(DWORD ofs)
{
   WORD value;

   SeekRead(ofs,&value,sizeof(WORD));
   return value;
}
//---------------------------------------------------------------------------
CompileUnit *LElfFile::ParseCompUnit(DWORD data,DWORD abbrevData)
{
   int bytes,i;
   DWORD top,length,offset,abbrevNum;
   WORD version;
   BYTE addrSize;
   ELFAbbrev *abbrev,**abbrevs;
   CompileUnit *unit;
   ELFAttr *attr;

   top = data;
   length = Read4Bytes(data);
   data += 4;
   version = Read2Bytes(data);
   data += 2;
   offset = Read4Bytes(data);
   data += 4;
   SeekRead(data++,&addrSize,sizeof(BYTE));
   if(version != 2 || addrSize != 4)
       return NULL;
   abbrevs = ReadAbbrevs(abbrevData,offset);
   abbrevNum = ReadLEB128(data,&bytes);
   data += bytes;
   abbrev = GetAbbrev(abbrevs,abbrevNum);
   unit = (CompileUnit *)LocalAlloc(LPTR,sizeof(CompileUnit));
   unit->top = top;
   unit->length = length;
   unit->abbrevs = abbrevs;
   unit->next = NULL;
   CurrentUnit = unit;
   for(i = 0;i< abbrev->numAttrs;i++) {
       attr = &abbrev->attrs[i];
       data = ReadAttribute(data,attr);
       switch(attr->name) {
           case DW_AT_name:
               unit->name = attr->string;
           break;
           case DW_AT_stmt_list:
               unit->hasLineInfo = true;
               unit->lineInfo = attr->value;
           break;
           case DW_AT_low_pc:
               unit->lowPC = attr->value;
           break;
           case DW_AT_high_pc:
               unit->highPC = attr->value;
           break;
           case DW_AT_compdir:
               unit->compdir = attr->string;
           break;
           case DW_AT_language:
           case DW_AT_producer:
           case DW_AT_macro_info:
           case DW_AT_entry_pc:
           break;
           default:
           //    fprintf(stderr, "Unknown attribute %02x\n", attr->name);
               unit = unit;
           break;
       }
   }
   if(abbrev->hasChildren)
   	ParseCompileUnitChildren(data, unit);
   return unit;
}
//---------------------------------------------------------------------------
ELFAbbrev **LElfFile::ReadAbbrevs(DWORD data,DWORD offset)
{
   BYTE c;
   int bytes,name,form,hash;
   ELFAbbrev **abbrevs,*abbrev;
   DWORD number;

   data += offset;
   abbrevs = (ELFAbbrev **)LocalAlloc(LPTR,sizeof(ELFAbbrev *)*121);
   number = ReadLEB128(data,&bytes);
   data += bytes;
   while(number){
       abbrev = (ELFAbbrev *)LocalAlloc(LPTR,sizeof(ELFAbbrev));
       abbrev->number = number;
       abbrev->tag = ReadLEB128(data, &bytes);
       data += bytes;
       SeekRead(data++,&c,sizeof(BYTE));
       abbrev->hasChildren = c ? true: false;
       name = ReadLEB128(data,&bytes);
       data += bytes;
       form = ReadLEB128(data,&bytes);
       data += bytes;
       while(name){
           if((abbrev->numAttrs % 4) == 0)
              	abbrev->attrs = (ELFAttr *)LocalReAlloc(abbrev->attrs,(abbrev->numAttrs + 4) * sizeof(ELFAttr),LMEM_ZEROINIT);
           abbrev->attrs[abbrev->numAttrs].name = name;
           abbrev->attrs[abbrev->numAttrs++].form = form;
           name = ReadLEB128(data,&bytes);
           data += bytes;
           form = ReadLEB128(data,&bytes);
           data += bytes;
       }
       hash = number % 121;
       abbrev->next = abbrevs[hash];
       abbrevs[hash] = abbrev;
       number = ReadLEB128(data,&bytes);
       data += bytes;
       if(GetAbbrev(abbrevs,number) != NULL)
       	break;
   }
   return abbrevs;
}
//---------------------------------------------------------------------------
ELFAbbrev *LElfFile::GetAbbrev(ELFAbbrev **table,DWORD number)
{
   ELFAbbrev *abbrev;

   abbrev = table[number % 121];
   while(abbrev){
       if(abbrev->number == number)
           return abbrev;
       abbrev = abbrev->next;
   }
   return NULL;
}
//---------------------------------------------------------------------------
ELFSectionHeader *LElfFile::GetSectionByName(char *name)
{
   DWORD ofs;
   int i;
   char *p;

   if(header.e_shstrndx == 0)
       return NULL;
   ofs = SectionHeader[header.e_shstrndx].offset;
   for(i=0;i<header.e_shnum;i++){
       if((p = ReadString(ofs+SectionHeader[i].name)) == NULL)
           continue;
       if(lstrcmp(p,name) == 0){
           LocalFree(p);
           return &SectionHeader[i];
       }
       LocalFree(p);
       p = NULL;
   }
   return NULL;
}
//---------------------------------------------------------------------------
char *LElfFile::ReadString(DWORD ofs)
{
   char c,*p;
   LString s;

   Seek(ofs);
   s = "";
   do{
       if(Read(&c,1) != 1 || c == 0)
           break;
       s += c;
   }while(1);
   if(s.IsEmpty())
       return NULL;
   p = (char *)LocalAlloc(LPTR,s.Length()+2);
   CopyMemory(p,s.c_str(),s.Length());
   return p;
}
//---------------------------------------------------------------------------
DWORD LElfFile::ReadAttribute(DWORD data,ELFAttr *attr)
{
   int bytes;
   int form;

   form = attr->form;
start:
   switch(form) {
       case DW_FORM_addr:
           attr->value = Read4Bytes(data);
           data += 4;
       break;
       case DW_FORM_data2:
           attr->value = Read2Bytes(data);
           data += 2;
       break;
       case DW_FORM_data4:
           attr->value = Read4Bytes(data);
           data += 4;
       break;
       case DW_FORM_string:
           attr->string = ReadString(data);
           data += lstrlen(attr->string)+1;
       break;
       case DW_FORM_strp:
           attr->string = ReadString(dbgStringsOfs + Read4Bytes(data));
           data += 4;
       break;
       case DW_FORM_block:
           attr->block = (ELFBlock *)LocalAlloc(LPTR,sizeof(ELFBlock));
           attr->block->length = ReadLEB128(data,&bytes);
           data += bytes;
           attr->block->data = (BYTE *)LocalAlloc(LPTR,attr->block->length+1);
           SeekRead(data,attr->block->data,attr->block->length);
           data += attr->block->length;
       break;
       case DW_FORM_block1:
           attr->block = (ELFBlock *)LocalAlloc(LPTR,sizeof(ELFBlock));
           SeekRead(data++,&attr->block->length,sizeof(BYTE));
           attr->block->data = (BYTE *)LocalAlloc(LPTR,attr->block->length+1);
           SeekRead(data,attr->block->data,attr->block->length);
           data += attr->block->length;
       break;
       case DW_FORM_data1:
       	SeekRead(data++,&attr->value,sizeof(BYTE));
       break;
       case DW_FORM_flag:
           SeekRead(data++,&attr->flag,sizeof(BYTE));
           attr->flag = attr->flag ? true : false;
       break;
       case DW_FORM_sdata:
           attr->value = ReadSignedLEB128(data,&bytes);
           data += bytes;
       break;
       case DW_FORM_udata:
           attr->value = ReadLEB128(data,&bytes);
           data += bytes;
       break;
       case DW_FORM_ref_addr:
           attr->value = (dbgInfo.infodata + Read4Bytes(data)) - GetCompileUnitForData(data)->top;
           data += 4;
       break;
       case DW_FORM_ref4:
           attr->value = Read4Bytes(data);
           data += 4;
       break;
       case DW_FORM_ref_udata:
           attr->value = (dbgInfo.infodata + (GetCompileUnitForData(data)->top - dbgInfo.infodata) + ReadLEB128(data, &bytes)) - CurrentUnit->top;
           data += bytes;
       break;
       case DW_FORM_indirect:
           form = ReadLEB128(data, &bytes);
           data += bytes;
           goto start;
       default:
//           fprintf(stderr, "Unsupported FORM %02x\n", form);
//           exit(-1);
           data = data;
		break;
   }
   return data;
}
//---------------------------------------------------------------------------
void LElfFile::ParseLineInfo(CompileUnit *unit,DWORD top)
{
   ELFSectionHeader *h;
   LineInfo *l;
   int max,bytes,count,index;
   signed char minInstrSize,defaultIsStmt,lineBase,lineRange,opcodeBase;
   DWORD data,totalLen,end,address;
   char *s;
   int file,line,col,isStmt,basicBlock,endSeq,op;

   h = GetSectionByName(".debug_line");
   if(h == NULL)
       return;
   data = ReadSection(h,top);
   data += unit->lineInfo;
   totalLen = Read4Bytes(data);
   data += 4;
   end = data + totalLen;
  //  u16 version = elfRead2Bytes(data);
   data += 2;
  //  u32 offset = elfRead4Bytes(data);
   data += 4;
   SeekRead(data++,&minInstrSize,1);
   SeekRead(data++,&defaultIsStmt,1);
   SeekRead(data++,&lineBase,1);
   SeekRead(data++,&lineRange,1);
   SeekRead(data++,&opcodeBase,1);
/*   stdOpLen = (char *)LocalAlloc(LPTR,opcodeBase * sizeof(u8));
   stdOpLen[0] = 1;
   for(i = 1; i < opcodeBase; i++)
       SeekRead(data++,&stdOpLen[i],1);
   LocalFree(stdOpLen);*/
   data += opcodeBase - 1;
   l = unit->lineInfoTable = (LineInfo *)LocalAlloc(LPTR,sizeof(LineInfo));
   while((s = ReadString(data++)) != NULL) {
		if((l->dirCount % 4) == 0)
       	l->dirs = (char **)LocalReAlloc(l->dirs,sizeof(char *)*(l->dirCount + 4),LMEM_ZEROINIT);
       l->dirs[l->dirCount++] = s;
       data += lstrlen(s);
   }
   max = 1000;
   l->lines = (LineInfoItem *)LocalAlloc(LPTR,max*sizeof(LineInfoItem));
   count = 4;
   index = 0;
   l->files = (LineInfoFileItem *)LocalAlloc(LPTR,sizeof(LineInfoFileItem)*count);
   while((s = ReadString(data++)) != NULL) {
       l->files[index].name = s;
       data += lstrlen(s);
    // directory
       l->files[index].dir = ReadLEB128(data,&bytes);
       data += bytes;
    // time
       ReadLEB128(data,&bytes);
       data += bytes;
    // size
       ReadLEB128(data,&bytes);
       data += bytes;
    //    fprintf(stderr, "File is %s\n", s);
       if(++index == count){
           count += 4;
      		l->files = (LineInfoFileItem*)LocalReAlloc(l->files,sizeof(LineInfoFileItem)*count,LMEM_ZEROINIT);
       }
   }
   l->fileCount = index;
   while(data < end){
       address = 0;
       file = 1;
       line = 1;
       col = 0;
       isStmt = defaultIsStmt;
       basicBlock = 0;
       endSeq = 0;
       op = 0;
       while(!endSeq) {
           SeekRead(data++,&op,1);
           switch(op){
               case DW_LNS_extended_op:
                   data++;
                   SeekRead(data++,&op,1);
                   switch(op) {
                       case DW_LNE_end_sequence:
                           endSeq = 1;
                           line = 1;
                       break;
                       case DW_LNE_set_address:
                           address = Read4Bytes(data);
                           data += 4;
                       break;
                       case DW_LNE_define_file:
                       	op = op;
                       break;
                       default:
                           //fprintf(stderr, "Unknown extended LINE opcode %02x\n", op);
                           op = op;
                       break;
                   }
               break;
               case DW_LNS_copy:
                   AddLine(l, address, file, line, &max);
                   basicBlock = 0;
               break;
               case DW_LNS_advance_pc:
                   address += minInstrSize * ReadLEB128(data,&bytes);
                   data += bytes;
               break;
               case DW_LNS_advance_line:
                   line += ReadSignedLEB128(data,&bytes);
                   data += bytes;
               break;
               case DW_LNS_set_file:
                   file = ReadLEB128(data,&bytes);
                   data += bytes;
               break;
               case DW_LNS_set_column:
                   col = ReadLEB128(data,&bytes);
                   data += bytes;
               break;
               case DW_LNS_negate_stmt:
                   isStmt = !isStmt;
               break;
               case DW_LNS_set_basic_block:
                   basicBlock = 1;
               break;
               case DW_LNS_const_add_pc:
                   address += (minInstrSize *((255 - opcodeBase)/lineRange));
               break;
               case DW_LNS_fixed_advance_pc:
                   address += Read2Bytes(data);
                   data += 2;
               break;
				case DW_LNS_set_prologue_end:
				case DW_LNS_set_epilogue_begin:
               	data = data;
               break;
				case DW_LNS_set_isa:
					ReadLEB128(data,&bytes);
                   data += bytes;
				break;
               default:
                   op -= opcodeBase;
                   address += (op / lineRange) * minInstrSize;
                   line += lineBase + (op % lineRange);
                   AddLine(l,address,file,line,&max);
                   basicBlock = 1;
               break;
           }
       }
   }
	l->lines = (LineInfoItem *)LocalReAlloc(l->lines,l->number*sizeof(LineInfoItem),LMEM_ZEROINIT);
}
//---------------------------------------------------------------------------
DWORD LElfFile::ParseCompileUnitChildren(DWORD data, CompileUnit *unit)
{
  	int bytes;
  	DWORD abbrevNum;
  	elf_Object *lastObj,*var;
	ELFAbbrev *abbrev;
	Function *func;

  	abbrevNum = ReadLEB128(data,&bytes);
  	data += bytes;
  	lastObj = NULL;
  	while(abbrevNum) {
    	if((abbrev = GetAbbrev(unit->abbrevs,abbrevNum)) != NULL){
    		switch(abbrev->tag) {
    			case DW_TAG_subprogram:
        			func = NULL;
               	data = ParseFunction(data, abbrev, unit, &func);
        			if(func != NULL) {
          				if(unit->lastFunction)
            				unit->lastFunction->next = func;
          				else
            				unit->functions = func;
          				unit->lastFunction = func;
        			}
      			break;
    			CASE_TYPE_TAG:
      				data = SkipData(data, abbrev, unit->abbrevs);
      			break;
    			case DW_TAG_variable:
        			var = NULL;
        			data = ParseObject(data, abbrev, unit, &var);
        			if(lastObj)
          				lastObj->next = var;
               	else
          				unit->variables = var;
        			lastObj = var;
      			break;
    			default:
      				data = ParseUnknownData(data, abbrev, unit->abbrevs);
      			break;
    		}
       }
    	abbrevNum = ReadLEB128(data, &bytes);
    	data += bytes;
  	}
  	return data;
}
//---------------------------------------------------------------------------
DWORD LElfFile::ParseFunction(DWORD data, ELFAbbrev *abbrev, CompileUnit *unit,Function **f)
{
  	Function *func,*fnc;
  	int bytes,i,nesting;
  	bool mangled,declaration;
	ELFAttr *attr;
	elf_Object *lastParam,*lastVar,*o;
	DWORD abbrevNum;

   func = (Function *)LocalAlloc(LPTR,sizeof(Function));
  	*f = func;
 	mangled = false;
  	declaration = false;
  	for(i = 0; i < abbrev->numAttrs; i++) {
    	attr = &abbrev->attrs[i];
    	data = ReadAttribute(data,attr);
    	switch(attr->name) {
    		case DW_AT_sibling:
      		break;
    		case DW_AT_name:
      			if(func->name == NULL)
        			func->name = attr->string;
      		break;
    		case DW_AT_MIPS_linkage_name:
      			func->name = attr->string;
      			mangled = true;
      		break;
    		case DW_AT_low_pc:
      			func->lowPC = attr->value;
      		break;
    		case DW_AT_high_pc:
      			func->highPC = attr->value;
      		break;
    		case DW_AT_prototyped:
      		break;
    		case DW_AT_decl_file:
      			func->file = attr->value;
      		break;
    		case DW_AT_decl_line:
      			func->line = attr->value;
      		break;
    		case DW_AT_external:
      			func->external = attr->flag;
      		break;
    		case DW_AT_frame_base:
  			    func->frameBase = attr->block;
      		break;
    		case DW_AT_type:
      			func->returnType = ParseType(unit, attr->value);
           break;
    		case DW_AT_abstract_origin:
      			GetFunctionAttributes(unit, attr->value, func);
      		break;
    		case DW_AT_declaration:
      			declaration = attr->flag;
      		break;
    		case DW_AT_inline:
    		case DW_AT_specification:
    		case DW_AT_artificial:
    		case DW_AT_proc_body:
    		case DW_AT_save_offset:
    		case DW_AT_user_2002:
    		case DW_AT_virtuality:
    		case DW_AT_containing_type:
    		case DW_AT_accessibility:
      		break;
    		case DW_AT_vtable_elem_location:
      			LocalFree(attr->block);
               attr->block = NULL;
      		break;
    		default:
      			//fprintf(stderr, "Unknown function attribute %02x\n", attr->name);
      		break;
    	}
  	}
  	if(declaration){
    	CleanUp(func);
    	LocalFree(func);
    	*f = NULL;
    	while(1) {
      		abbrevNum = ReadLEB128(data, &bytes);
      		data += bytes;
      		if(!abbrevNum)
        		return data;
      		abbrev = GetAbbrev(unit->abbrevs, abbrevNum);
      		data = SkipData(data, abbrev, unit->abbrevs);
    	}
  	}
  	if(abbrev->hasChildren) {
    	nesting = 1;
    	lastParam = NULL;
    	lastVar = NULL;
    	while(nesting) {
      		abbrevNum = ReadLEB128(data, &bytes);
      		data += bytes;
      		if(!abbrevNum){
        		nesting--;
        		continue;
      		}
      		abbrev = GetAbbrev(unit->abbrevs, abbrevNum);
      		switch(abbrev->tag) {
      			CASE_TYPE_TAG:
      			case DW_TAG_label: // not needed
        			data = SkipData(data, abbrev, unit->abbrevs);
        		break;
      			case DW_TAG_subprogram:
          			fnc = NULL;
          			data = ParseFunction(data, abbrev, unit, &fnc);
          			if(fnc != NULL) {
            			if(unit->lastFunction == NULL)
              				unit->functions = fnc;
            			else
              				unit->lastFunction->next = fnc;
            			unit->lastFunction = fnc;
          			}
        		break;
      			case DW_TAG_lexical_block:
          			data = ParseBlock(data, abbrev, unit, func, &lastVar);
        		break;
      			case DW_TAG_formal_parameter:
          			data = ParseObject(data, abbrev, unit, &o);
          			if(func->parameters)
            			lastParam->next = o;
          			else
            			func->parameters = o;
          			lastParam = o;
        		break;
      			case DW_TAG_variable:
          			data = ParseObject(data,abbrev,unit,&o);
          			if(func->variables)
            			lastVar->next = o;
          			else
            			func->variables = o;
          			lastVar = o;
        		break;
      			case DW_TAG_unspecified_parameters:
      			case DW_TAG_inlined_subroutine:
          			for(i = 0; i < abbrev->numAttrs; i++) {
            			data = ReadAttribute(data,  &abbrev->attrs[i]);
            			if(abbrev->attrs[i].form == DW_FORM_block1){
              				LocalFree(abbrev->attrs[i].block);
                           abbrev->attrs[i].block = NULL;
                       }
          			}
          			if(abbrev->hasChildren)
            			nesting++;
        		break;
      			default:
          			//fprintf(stderr, "Unknown function TAG %02x\n", abbrev->tag);
          			data = SkipData(data, abbrev, unit->abbrevs);
        		break;
      		}
    	}
  	}
  	return data;
}
//---------------------------------------------------------------------------
DWORD LElfFile::SkipData(DWORD data, ELFAbbrev *abbrev, ELFAbbrev **abbrevs)
{
  	int i,bytes,nesting;
	DWORD abbrevNum;

   if(abbrev == NULL)
   	return data;
  	for(i = 0; i < abbrev->numAttrs; i++) {
    	data = ReadAttribute(data,&abbrev->attrs[i]);
    	if(abbrev->attrs[i].form == DW_FORM_block1){
      		LocalFree(abbrev->attrs[i].block);
           abbrev->attrs[i].block = NULL;
       }
  	}
  	if(!abbrev->hasChildren)
   	return data;
 	nesting = 1;
  	while(nesting) {
  		abbrevNum = ReadLEB128(data, &bytes);
       data += bytes;
       if(!abbrevNum){
       	nesting--;
        	continue;
       }
      	abbrev = GetAbbrev(abbrevs,abbrevNum);
       if(abbrev != NULL){
      		for(i = 0; i < abbrev->numAttrs; i++) {
       		data = ReadAttribute(data,&abbrev->attrs[i]);
       		if(abbrev->attrs[i].form == DW_FORM_block1){
       			LocalFree(abbrev->attrs[i].block);
               	abbrev->attrs[i].block = NULL;
           	}
      		}
       	if(abbrev->hasChildren)
       		nesting++;
       }
   }
  	return data;
}
//---------------------------------------------------------------------------
DWORD LElfFile::ParseObject(DWORD data, ELFAbbrev *abbrev, CompileUnit *unit,elf_Object **object)
{
  	elf_Object *o;
	int i;
	ELFAttr *attr;

   o = (elf_Object *)LocalAlloc(LPTR,sizeof(elf_Object));
  	o->next = NULL;
  	for(i = 0; i < abbrev->numAttrs; i++) {
    	attr = &abbrev->attrs[i];
    	data = ReadAttribute(data, attr);
       switch(attr->name){
    		case DW_AT_location:
      			o->location = attr->block;
      		break;
    		case DW_AT_name:
      			if(o->name == NULL)
        			o->name = attr->string;
      		break;
    		case DW_AT_MIPS_linkage_name:
      			o->name = attr->string;
      		break;
    		case DW_AT_decl_file:
      			o->file = attr->value;
      		break;
    		case DW_AT_decl_line:
      			o->line = attr->value;
      		break;
    		case DW_AT_type:
      			o->type = ParseType(unit,attr->value);
      		break;
    		case DW_AT_external:
      			o->external = attr->flag;
      		break;
    		case DW_AT_abstract_origin:
      			GetObjectAttributes(unit,attr->value,o);
      		break;
    		case DW_AT_const_value:
    		case DW_AT_declaration:
    		case DW_AT_artificial:
      		break;
    		case DW_AT_specification:
      		break;
    		default:
      			//fprintf(stderr, "Unknown object attribute %02x\n", attr->name);
               o = o;
      		break;
    	}
  	}
  	*object = o;
  	return data;
}
//---------------------------------------------------------------------------
DWORD LElfFile::ParseUnknownData(DWORD data, ELFAbbrev *abbrev, ELFAbbrev **abbrevs)
{
  	int i,bytes,nesting;
	DWORD abbrevNum;

	for(i = 0; i < abbrev->numAttrs; i++) {
   	data = ReadAttribute(data,  &abbrev->attrs[i]);
      	if(abbrev->attrs[i].form == DW_FORM_block1){
        	LocalFree(abbrev->attrs[i].block);
           abbrev->attrs[i].block = NULL;
       }
	}
   if(abbrev->hasChildren) {
   	nesting = 1;
      	while(nesting) {
       	abbrevNum = ReadLEB128(data, &bytes);
        	data += bytes;
        	if(!abbrevNum) {
          		nesting--;
          		continue;
        	}
        	abbrev = GetAbbrev(abbrevs, abbrevNum);
           if(abbrev != NULL){
        		for(i = 0; i < abbrev->numAttrs; i++) {
          			data = ReadAttribute(data,  &abbrev->attrs[i]);
          			if(abbrev->attrs[i].form == DW_FORM_block1){
            			LocalFree(abbrev->attrs[i].block);
                   	abbrev->attrs[i].block = NULL;
               	}
        		}
        		if(abbrev->hasChildren)
          			nesting++;
           }
      	}
	}
  	return data;
}
//---------------------------------------------------------------------------
void LElfFile::CleanUp(elf_Object *o)
{
	if(o->location)
   	LocalFree(o->location);
	o->location = NULL;
}
//---------------------------------------------------------------------------
void LElfFile::CleanUp(Function *func)
{
  	elf_Object *o,*next;

   o = func->parameters;
  	while(o) {
    	CleanUp(o);
    	next = o->next;
    	LocalFree(o);
    	o = next;
  	}
  	o = func->variables;
  	while(o) {
    	CleanUp(o);
    	next = o->next;
    	LocalFree(o);
    	o = next;
  	}
   LocalFree(func->frameBase);
   func->frameBase = NULL;
}
//---------------------------------------------------------------------------
DWORD LElfFile::ParseBlock(DWORD data,ELFAbbrev *abbrev,CompileUnit *unit,Function *func,elf_Object **lastVar)
{
  	int bytes,i,nesting;
   DWORD start,end,abbrevNum;
	ELFAttr *attr;
	Function *f;
	elf_Object *o;

  	start = func->lowPC;
  	end = func->highPC;
  	for(i = 0; i < abbrev->numAttrs; i++) {
    	attr = &abbrev->attrs[i];
    	data = ReadAttribute(data, attr);
       switch(attr->name) {
    		case DW_AT_sibling:
      		break;
    		case DW_AT_low_pc:
      			start = attr->value;
      		break;
    		case DW_AT_high_pc:
      			end = attr->value;
      		break;
    		case DW_AT_ranges: // ignore for now
      		break;
    		default:
      			//fprintf(stderr, "Unknown block attribute %02x\n", attr->name);
               attr = attr;
      		break;
    	}
  	}
  	if(abbrev->hasChildren) {
    	nesting = 1;
    	while(nesting){
      		abbrevNum = ReadLEB128(data, &bytes);
      		data += bytes;
      		if(!abbrevNum){
        		nesting--;
        		continue;
      		}
      		abbrev = GetAbbrev(unit->abbrevs, abbrevNum);
      		switch(abbrev->tag){
      			CASE_TYPE_TAG: // types only parsed when used
      			case DW_TAG_label: // not needed
        			data = SkipData(data, abbrev, unit->abbrevs);
        		break;
      			case DW_TAG_lexical_block:
        			data = ParseBlock(data, abbrev, unit, func, lastVar);
        		break;
      			case DW_TAG_subprogram:
               	f = NULL;
          			data = ParseFunction(data, abbrev, unit, &f);
          			if(f != NULL) {
            			if(unit->lastFunction)
              				unit->lastFunction->next = f;
            			else
              				unit->functions = f;
            			unit->lastFunction = f;
          			}
        		break;
      			case DW_TAG_variable:
          			data = ParseObject(data, abbrev, unit, &o);
          			if(o->startScope == 0)
            			o->startScope = start;
          			if(o->endScope == 0)
            			o->endScope = 0;
          			if(func->variables)
            			(*lastVar)->next = o;
          			else
            			func->variables = o;
          			*lastVar = o;
        		break;
      			case DW_TAG_inlined_subroutine:
             		data = SkipData(data, abbrev, unit->abbrevs);
        		break;
      			default:
          			//fprintf(stderr, "Unknown block TAG %02x\n", abbrev->tag);
          			data = SkipData(data, abbrev, unit->abbrevs);
        		break;
      		}
    	}
  	}
  	return data;
}
//---------------------------------------------------------------------------
Type* LElfFile::ParseType(CompileUnit *unit,DWORD offset)
{
	Type *t,*type;
   DWORD data;
	int bytes,abbrevNum;
	ELFAbbrev *abbrev;

  	t = unit->types;
  	while(t) {
    	if(t->offset == offset)
      		return t;
    	t = t->next;
  	}
  	if(offset == 0) {
    	t = (Type *)LocalAlloc(LPTR,sizeof(Type));
       t->type = TYPE_void;
    	t->offset = 0;
    	AddType(t,unit,0);
    	return t;
  	}
  	data = (DWORD)unit->top + offset;
  	abbrevNum = ReadLEB128(data, &bytes);
  	data += bytes;
  	type = NULL;
  	abbrev = GetAbbrev(unit->abbrevs, abbrevNum);
  	ParseType(data,offset,abbrev,unit,&type);
  	return type;
}
//---------------------------------------------------------------------------
void LElfFile::ParseType(DWORD data,DWORD offset,ELFAbbrev *abbrev,CompileUnit *unit,Type **type)
{
	DWORD typeref,num;
   char *name;
	int i,bytes,index,count,maxBounds;
	ELFAttr *attr;
	Type *t;
	Struct *s;
	ELFAbbrev *abbr;
	Function *fnc;
	Enum *e;
	Member *m;
	FunctionType *f;
	elf_Object *lastVar,*o;
	Array *array;
	EnumMember *m1;

	if(abbrev == NULL)
   	return;
  	switch(abbrev->tag) {
  		case DW_TAG_typedef:
      		typeref = 0;
      		name = NULL;
      		for(i = 0; i < abbrev->numAttrs; i++) {
        		attr = &abbrev->attrs[i];
        		data = ReadAttribute(data, attr);
        		switch(attr->name) {
        			case DW_AT_name:
          				name = attr->string;
          			break;
        			case DW_AT_type:
          				typeref = attr->value;
          			break;
        			case DW_AT_decl_file:
        			case DW_AT_decl_line:
          			break;
        			default:
          				//fprintf(stderr, "Unknown attribute for typedef %02x\n", attr->name);
                       attr = attr;
          			break;
        		}
      		}
      		if(abbrev->hasChildren){
        	//fprintf(stderr, "Unexpected children for typedef\n");
       	}
           *type = ParseType(unit, typeref);
      		if(name)
        		(*type)->name = name;
      		return;
  		case DW_TAG_union_type:
  		case DW_TAG_structure_type:
      		t = (Type *)LocalAlloc(LPTR,sizeof(Type));
      		if(abbrev->tag == DW_TAG_structure_type)
        		t->type = TYPE_struct;
      		else
        		t->type = TYPE_union;
      		s = (Struct *)LocalAlloc(LPTR,sizeof(Struct));
      		t->structure = s;
      		AddType(t, unit, offset);
      		for(i = 0; i < abbrev->numAttrs; i++) {
        		attr = &abbrev->attrs[i];
        		data = ReadAttribute(data, attr);
        		switch(attr->name) {
        			case DW_AT_name:
          				t->name = attr->string;
          			break;
        			case DW_AT_byte_size:
          				t->size = attr->value;
          			break;
        			case DW_AT_decl_file:
                       t->file = attr->value;
                   break;
        			case DW_AT_decl_line:
                       t->line = attr->value;
                   break;
        			case DW_AT_sibling:
        			case DW_AT_containing_type: // todo?
        			case DW_AT_declaration:
  					case DW_AT_specification: // TODO:
          			break;
        			default:
          				//fprintf(stderr, "Unknown attribute for struct %02x\n", attr->name);
                       t = t;
          			break;
        		}
      		}
      		if(abbrev->hasChildren) {
   			num = ReadLEB128(data, &bytes);      //196568
        		data += bytes;
        		index = 0;
        		while(num) {         //1917,196294,0,,NULL,sgIP_ARP.c,5034
          			abbr = GetAbbrev(unit->abbrevs, num);
          			switch(abbr->tag) {
          				case DW_TAG_member:
              				if((index % 4) == 0)
              					s->members = (Member *)LocalReAlloc(s->members,sizeof(Member)*(index+4),LMEM_ZEROINIT);
              				m = &s->members[index];
              				m->location = NULL;
              				m->bitOffset = 0;
              				m->bitSize = 0;
              				m->byteSize = 0;
              				for(i = 0; i < abbr->numAttrs; i++) {
                				attr = &abbr->attrs[i];
                				data = ReadAttribute(data, attr);
                				switch(attr->name) {
                					case DW_AT_name:
                  						m->name = attr->string;
                  					break;
                					case DW_AT_type:
                  						m->type = ParseType(unit, attr->value);
                  					break;
                					case DW_AT_data_member_location:
                  						m->location = attr->block;
                  					break;
                					case DW_AT_byte_size:
                  						m->byteSize = attr->value;
                  					break;
                                   case DW_AT_bit_offset:
                  						m->bitOffset = attr->value;
                  					break;
                					case DW_AT_bit_size:
                  						m->bitSize = attr->value;
                  					break;
                					case DW_AT_decl_file:
                					case DW_AT_decl_line:
                					case DW_AT_accessibility:
                					case DW_AT_artificial: // todo?
                  					break;
                					default:
                                       m = m;
                                   break;
                				}
              				}
              				index++;
            			break;
          				case DW_TAG_subprogram:
              				fnc = NULL;
              				data = ParseFunction(data, abbr, unit, &fnc);
              				if(fnc != NULL) {
                				if(unit->lastFunction)
                  					unit->lastFunction->next = fnc;
                				else
                  					unit->functions = fnc;
                				unit->lastFunction = fnc;
              				}
            			break;
          				case DW_TAG_inheritance:
            				data = SkipData(data, abbr, unit->abbrevs);
            			break;
          				CASE_TYPE_TAG:
            				data = SkipData(data, abbr, unit->abbrevs);
            			break;
          				case DW_TAG_variable:
            				data = SkipData(data, abbr, unit->abbrevs);
            			break;
          				default:
                        	data = SkipData(data, abbr, unit->abbrevs);
            			break;
          			}
          			num = ReadLEB128(data, &bytes);
          			data += bytes;
        		}
        		s->memberCount = index;
      		}
      		*type = t;
      		return;
    	break;
  		case DW_TAG_base_type:
      		t = (Type *)LocalAlloc(LPTR,sizeof(Type));
      		t->type = TYPE_base;
      		AddType(t, unit, offset);
      		for(i = 0; i < abbrev->numAttrs; i++) {
        		attr = &abbrev->attrs[i];
        		data = ReadAttribute(data, attr);
        		switch(attr->name) {
        			case DW_AT_name:
          				t->name = attr->string;
          			break;
        			case DW_AT_encoding:
          				t->encoding = attr->value;
          			break;
        			case DW_AT_byte_size:
          				t->size = attr->value;
          			break;
        			case DW_AT_bit_size:
          				t->bitSize = attr->value;
          			break;
        			default:
                       t = t;
          			break;
        		}
      		}
      		if(abbrev->hasChildren){
//        	fprintf(stderr, "Unexpected children for base type\n");
       	}
      		*type = t;
      		return;
		case DW_TAG_pointer_type:
      		t = (Type *)LocalAlloc(LPTR,sizeof(Type));
      		t->type = TYPE_pointer;
      		AddType(t, unit, offset);
      		for(i = 0; i < abbrev->numAttrs; i++) {
        		attr = &abbrev->attrs[i];
               data = ReadAttribute(data, attr);
        		switch(attr->name) {
        			case DW_AT_type:
          				t->pointer = ParseType(unit, attr->value);
          			break;
        			case DW_AT_byte_size:
          				t->size = attr->value;
          			break;
        			default:
          				//fprintf(stderr, "Unknown pointer type attribute %02x\n", attr->name);
          			break;
        		}
      		}
      		if(abbrev->hasChildren){
//        		fprintf(stderr, "Unexpected children for pointer type\n");
           }
      		*type = t;
      		return;
       case DW_TAG_reference_type:
      		t = (Type *)LocalAlloc(LPTR,sizeof(Type));
      		t->type = TYPE_reference;
      		AddType(t, unit, offset);
      		for(i = 0; i < abbrev->numAttrs; i++) {
        		attr = &abbrev->attrs[i];
        		data = ReadAttribute(data, attr);
        		switch(attr->name) {
        			case DW_AT_type:
          				t->pointer = ParseType(unit, attr->value);
          			break;
        			case DW_AT_byte_size:
          				t->size = attr->value;
          			break;
        			default:
          				//fprintf(stderr, "Unknown ref type attribute %02x\n", attr->name);

                       t = t;
          			break;
        		}
      		}
      		if(abbrev->hasChildren){
//        		fprintf(stderr, "Unexpected children for ref type\n");
           }
      		*type = t;
      		return;
  		case DW_TAG_volatile_type:
       	typeref = 0;
      		for(i = 0; i < abbrev->numAttrs; i++) {
        		attr = &abbrev->attrs[i];
        		data = ReadAttribute(data, attr);
               switch(attr->name) {
        			case DW_AT_type:
          				typeref = attr->value;
          			break;
        			default:
          			break;
        		}
      		}
      		if(abbrev->hasChildren){
			}
      		*type = ParseType(unit, typeref);
      		return;
  		case DW_TAG_const_type:
      		typeref = 0;
      		for(i = 0; i < abbrev->numAttrs; i++) {
        		attr = &abbrev->attrs[i];
               data = ReadAttribute(data, attr);
        		switch(attr->name) {
        			case DW_AT_type:
          				typeref = attr->value;
          			break;
        			default:
          			break;
        		}
      		}
      		if(abbrev->hasChildren){
       	}
      		*type = ParseType(unit, typeref);
      		return;
  		case DW_TAG_enumeration_type:
      		t = (Type *)LocalAlloc(LPTR,sizeof(Type));
      		t->type = TYPE_enum;
           e = (Enum *)LocalAlloc(LPTR,sizeof(Enum));
      		t->enumeration = e;
      		AddType(t,unit,offset);
      		count = 0;
      		for(i = 0; i < abbrev->numAttrs; i++) {
        		attr = &abbrev->attrs[i];
        		data = ReadAttribute(data, attr);
        		switch(attr->name) {
        			case DW_AT_name:
          				t->name = attr->string;
          			break;
        			case DW_AT_byte_size:
          				t->size = attr->value;
          			break;
        			case DW_AT_sibling:
        			case DW_AT_decl_file:
        			case DW_AT_decl_line:
          			break;
        			default:
          			break;
        		}
      		}
      		if(abbrev->hasChildren) {
        		num = ReadLEB128(data, &bytes);
        		data += bytes;
        		while(num) {
          			abbr = GetAbbrev(unit->abbrevs, num);
          			switch(abbr->tag) {
          				case DW_TAG_enumerator:
              				count++;
          					e->members = (EnumMember *)LocalReAlloc(e->members,count*sizeof(EnumMember),LMEM_ZEROINIT);
              				m1 = &e->members[count-1];
              				for(i = 0; i < abbr->numAttrs; i++) {
                				attr = &abbr->attrs[i];
                				data = ReadAttribute(data, attr);
                               switch(attr->name) {
                					case DW_AT_name:
                  						m1->name = attr->string;
                  					break;
                					case DW_AT_const_value:
                  						m1->value = attr->value;
                  					break;
                					default:
                                   break;
                				}
              				}
            			break;
          				default:
            				data = SkipData(data, abbr, unit->abbrevs);
            			break;
          			}
          			num = ReadLEB128(data, &bytes);
          			data += bytes;
        		}
      		}
      		e->count = count;
      		*type = t;
      		return;
    	break;
  		case DW_TAG_subroutine_type:
      		t = (Type *)LocalAlloc(LPTR,sizeof(Type));
      		t->type = TYPE_function;
      		f = (FunctionType *)LocalAlloc(LPTR,sizeof(FunctionType));
      		t->function = f;
      		AddType(t, unit, offset);
      		for(i = 0; i < abbrev->numAttrs; i++) {
        		attr = &abbrev->attrs[i];
        		data = ReadAttribute(data, attr);
               switch(attr->name) {
        			case DW_AT_prototyped:
        			case DW_AT_sibling:
          			break;
        			case DW_AT_type:
          				f->returnType = ParseType(unit, attr->value);
          			break;
        			default:
                   break;
        		}
      		}
      		if(abbrev->hasChildren) {
        		num = ReadLEB128(data, &bytes);
        		data += bytes;
        		lastVar = NULL;
        		while(num) {
          			abbr = GetAbbrev(unit->abbrevs, num);
          			switch(abbr->tag) {
          				case DW_TAG_formal_parameter:
              				data = ParseObject(data, abbr, unit, &o);
              				if(f->args)
                				lastVar->next = o;
              				else
                				f->args = o;
              				lastVar = o;
            			break;
          				case DW_TAG_unspecified_parameters:
            				data = SkipData(data, abbr, unit->abbrevs);
                       break;
          				CASE_TYPE_TAG:
            				data = SkipData(data, abbr, unit->abbrevs);
            			break;
          				default:
            				data = SkipData(data, abbr, unit->abbrevs);
            			break;
          			}
          			num = ReadLEB128(data, &bytes);
          			data += bytes;
        		}
      		}
      		*type = t;
      		return;
  		case DW_TAG_array_type:
      		typeref = 0;
      		array = (Array *)LocalAlloc(LPTR,sizeof(Array));
           t = (Type *)LocalAlloc(LPTR,sizeof(Type));
      		t->type = TYPE_array;
      		AddType(t, unit, offset);
      		for(i = 0; i < abbrev->numAttrs; i++) {
        		attr = &abbrev->attrs[i];
        		data = ReadAttribute(data, attr);
        		switch(attr->name) {
        			case DW_AT_sibling:
          			break;
        			case DW_AT_type:
          				typeref = attr->value;
          				array->type = ParseType(unit, typeref);
          			break;
        			default:
					break;
        		}
      		}
      		if(abbrev->hasChildren) {
        		num = ReadLEB128(data, &bytes);
        		data += bytes;
        		index = 0;
        		maxBounds = 0;
        		while(num) {
          			abbr = GetAbbrev(unit->abbrevs, num);
          			switch(abbr->tag) {
          				case DW_TAG_subrange_type:
              				if(maxBounds == index) {
                				maxBounds += 4;
             					array->bounds = (int *)LocalReAlloc(array->bounds,sizeof(int)*maxBounds,LMEM_ZEROINIT);
              				}
              				for(i = 0; i < abbr->numAttrs; i++) {
                				attr = &abbr->attrs[i];
                				data = ReadAttribute(data, attr);
                				switch(attr->name) {
                					case DW_AT_upper_bound:
                  						array->bounds[index] = attr->value+1;
                  					break;
                					case DW_AT_type: // ignore
                  					break;
                					default:
                               	break;
                				}
              				}
              				index++;
            			break;
          				default:
            				data = SkipData(data, abbr, unit->abbrevs);
            			break;
          			}
          			num = ReadLEB128(data, &bytes);
          			data += bytes;
        		}
        		array->maxBounds = index;
      		}
      		t->size = array->type->size;
     	 	for(i = 0; i < array->maxBounds; i++)
        		t->size *= array->bounds[i];
      		t->array = array;
      		*type = t;
      		return;
  		default:
//    		fprintf(stderr, "Unknown type TAG %02x\n", abbrev->tag);
//    		exit(-1);
           t = t;
       break;
  	}
}
//---------------------------------------------------------------------------
void LElfFile::GetObjectAttributes(CompileUnit *unit,DWORD offset, elf_Object *o)
{
  	DWORD data,abbrevNum;
  	int bytes,i;
	ELFAbbrev *abbrev;
   ELFAttr *attr;

  	data = (DWORD)unit->top + offset;
  	abbrevNum = ReadLEB128(data, &bytes);
  	data += bytes;
  	if(!abbrevNum)
    	return;
  	abbrev = GetAbbrev(unit->abbrevs, abbrevNum);
  	for(i = 0; i < abbrev->numAttrs; i++) {
   	attr = &abbrev->attrs[i];
    	data = ReadAttribute(data, attr);
       switch(attr->name) {
    		case DW_AT_location:
      			o->location = attr->block;
      		break;
    		case DW_AT_name:
      			if(o->name == NULL)
        			o->name = attr->string;
      		break;
    		case DW_AT_MIPS_linkage_name:
      			o->name = attr->string;
      		break;
    		case DW_AT_decl_file:
      			o->file = attr->value;
      		break;
    		case DW_AT_decl_line:
      			o->line = attr->value;
      		break;
    		case DW_AT_type:
      			o->type = ParseType(unit, attr->value);
      		break;
    		case DW_AT_external:
      			o->external = attr->flag;
      		break;
    		case DW_AT_const_value:
    		case DW_AT_abstract_origin:
    		case DW_AT_declaration:
    		case DW_AT_artificial:
      		break;
    		case DW_AT_specification:
      		break;
    		default:
      			//fprintf(stderr, "Unknown object attribute %02x\n", attr->name);
               o = o;
      		break;
    	}
  	}
}
//---------------------------------------------------------------------------
void LElfFile::AddType(Type *type, CompileUnit *unit, u32 offset)
{
  	if(type->next == NULL) {
    	if(unit->types != type && type->offset == 0) {
      		type->offset = offset;
      		type->next = unit->types;
      		unit->types = type;
    	}
  	}
}
//---------------------------------------------------------------------------
void LElfFile::AddLine(LineInfo *l,DWORD a, int file, int line, int *max)
{
   LineInfoItem *li;

  	if(l->number == *max) {
    	*max += 1000;
  		l->lines = (LineInfoItem *)LocalReAlloc(l->lines, *max*sizeof(LineInfoItem),LMEM_ZEROINIT);
  	}
  	li = &l->lines[l->number];
  	li->file = file;
  	li->address = a;
  	li->line = line;
  	l->number++;
}
//---------------------------------------------------------------------------
CompileUnit *LElfFile::GetCompileUnitForData(DWORD data)
{
  	DWORD end;
	CompileUnit *unit;

  	end = CurrentUnit->top + 4 + CurrentUnit->length;
  	if(data >= CurrentUnit->top && data < end)
   	return CurrentUnit;
  	unit = CompileUnits;
  	while(unit) {
    	end = unit->top + 4 + unit->length;
    	if(data >= unit->top && data < end)
      		return unit;
    	unit = unit->next;
  	}
   return NULL;
}
//---------------------------------------------------------------------------
void LElfFile::GetFunctionAttributes(CompileUnit *unit, u32 offset, Function *func)
{
  	DWORD data,abbrevNum;
  	int bytes,i;
   ELFAbbrev *abbrev;
	ELFAttr *attr;

  	data = (DWORD)unit->top + offset;
  	abbrevNum = ReadLEB128(data, &bytes);
  	data += bytes;
  	if(!abbrevNum)
    	return;
  	abbrev = GetAbbrev(unit->abbrevs, abbrevNum);
  	for(i = 0; i < abbrev->numAttrs; i++) {
   	attr = &abbrev->attrs[i];
    	data = ReadAttribute(data, attr);
    	switch(attr->name) {
    		case DW_AT_sibling:
      		break;
    		case DW_AT_name:
      			if(func->name == NULL)
        			func->name = attr->string;
      		break;
    		case DW_AT_MIPS_linkage_name:
      			func->name = attr->string;
      		break;
    		case DW_AT_low_pc:
      			func->lowPC = attr->value;
      		break;
    		case DW_AT_high_pc:
      			func->highPC = attr->value;
      		break;
    		case DW_AT_decl_file:
      			func->file = attr->value;
      		break;
           case DW_AT_decl_line:
      			func->line = attr->value;
      		break;
           case DW_AT_external:
      			func->external = attr->flag;
      		break;
    		case DW_AT_frame_base:
      			func->frameBase = attr->block;
      		break;
    		case DW_AT_type:
      			func->returnType = ParseType(unit, attr->value);
      		break;
    		case DW_AT_inline:
    		case DW_AT_specification:
    		case DW_AT_declaration:
    		case DW_AT_artificial:
    		case DW_AT_prototyped:
    		case DW_AT_proc_body:
    		case DW_AT_save_offset:
    		case DW_AT_user_2002:
    		case DW_AT_virtuality:
    		case DW_AT_containing_type:
    		case DW_AT_accessibility:
      			// todo;
      		break;
    		case DW_AT_vtable_elem_location:
      			LocalFree(attr->block);
      		break;
    		default:
      			//fprintf(stderr, "Unknown function attribute %02x\n", attr->name);
      		break;
    	}
  	}
}
//---------------------------------------------------------------------------
void LElfFile::ParseAranges(DWORD data)
{
  	ELFSectionHeader *sh;
	DWORD end,len,offset,addr;
	int max,index,i;
   ARanges *ranges;

  	sh = GetSectionByName(".debug_aranges");
  	if(sh == NULL)
    	return;
  	data = ReadSection(sh,data);
  	end = data + sh->size;
  	max = 4;
  	ranges = (ARanges *)LocalAlloc(LPTR,sizeof(ARanges)*max);
  	index = 0;
  	while(data < end) {
    	len = Read4Bytes(data);
    	data += 4;
    //    u16 version = elfRead2Bytes(data);
    	data += 2;
    	offset = Read4Bytes(data);
    	data += 4;
    //    u8 addrSize = *data++;
    //    u8 segSize = *data++;
    	data += 2; // remove if uncommenting above
    	data += 4;
    	ranges[index].count = (len-20)/8;
    	ranges[index].offset = offset;
    	ranges[index].ranges = (ARange *)LocalAlloc(LPTR,sizeof(ARange)*((len-20)/8));
    	i = 0;
    	while(true) {
      		addr = Read4Bytes(data);
      		data += 4;
      		len = Read4Bytes(data);
      		data += 4;
      		if(addr == 0 && len == 0)
        		break;
      		ranges[index].ranges[i].lowPC = addr;
      		ranges[index].ranges[i].highPC = addr+len;
      		i++;
    	}
    	index++;
    	if(index == max) {
      		max += 4;
      		ranges = (ARanges *)LocalReAlloc(ranges, max*sizeof(ARanges),LMEM_ZEROINIT);
    	}
  	}
  	dbgInfo.numRanges = index;
  	dbgInfo.ranges = ranges;
}
//---------------------------------------------------------------------------
void LElfFile::ParseLocations(DWORD top)
{
  	ELFSectionHeader *h;
   DWORD data,end,lowPC,highPC,offset,count;
   WORD len;
   Function *f;
   elf_Object *o;
   int i1,i;
   CompileUnit *comp;

   h = GetSectionByName(".debug_loc");
  	if(h == NULL)
    	return;
   top = data = ReadSection(h,top);
   end = data + h->size;
   offset = 0;
   count = 0;
   while(data < end){
       lowPC = Read4Bytes(data);
       data += 4;
       highPC = Read4Bytes(data);
       data += 4;
       if(lowPC == (DWORD)-1){
           data = data;
           continue;
       }
       if(!lowPC && !highPC){
           offset = data-top;
           continue;
       }
       if((count & 3) == 0)
           Locations = (ELFLocation *)LocalReAlloc(Locations,(count+4)*sizeof(ELFLocation),LPTR);
       Locations[count].offset = offset;
       Locations[count].lowPC = lowPC;
       Locations[count].highPC = highPC;
       len = Read2Bytes(data);
       data += 2;
       Locations[count].block = (ELFBlock *)LocalAlloc(LPTR,sizeof(ELFBlock) + len);
       Locations[count].block->data = (BYTE *)(Locations[count].block + 1);
       Locations[count].block->length = len;
       Read(Locations[count++].block->data,len);
       data +=len;
   }
   LocationsCount = count;
   for(i=0;i<LocationsCount;i++){
       comp = CompileUnits;
       while(comp != NULL){
           f = comp->functions;
           while(f != NULL){
               if(f->frameBase == (ELFBlock *)Locations[i].offset){
                   for(i1=i;i < LocationsCount && Locations[i].offset == Locations[i1].offset;i++){
                       Locations[i].lowPC += comp->lowPC;
                       Locations[i].highPC += comp->lowPC;
                   }
               }
               o = f->variables;
               while(o != NULL){
                   if(o->location == (ELFBlock *)Locations[i].offset){
                       for(i1=i;i < LocationsCount && Locations[i].offset == Locations[i1].offset;i++){
                           Locations[i].lowPC += comp->lowPC;
                           Locations[i].highPC += comp->lowPC;
                       }
                   }
                   o = o->next;
               }
               o = f->parameters;
               while(o != NULL){
                   if(o->location == (ELFBlock *)Locations[i].offset){
                       for(i1=i;i < LocationsCount && Locations[i].offset == Locations[i1].offset;i++){
                           Locations[i].lowPC += comp->lowPC;
                           Locations[i].highPC += comp->lowPC;
                       }
                   }
                   o = o->next;
               }
               f = f->next;
           }
           o = comp->variables;
           while(o != NULL){
               if(o->location == (ELFBlock *)Locations[i].offset){
                   for(i1=i;i < LocationsCount && Locations[i].offset == Locations[i1].offset;i++){
                       Locations[i].lowPC += comp->lowPC;
                       Locations[i].highPC += comp->lowPC;
                   }
               }
               o = o->next;
           }
           comp = comp->next;
       }
   }
}
//---------------------------------------------------------------------------
void LElfFile::ParseCFA(DWORD top)
{
  	ELFSectionHeader *h;
   DWORD data,topOffset,end,offset,len,dataEnd,id;
   ELFcie *cies,*cie;
	int bytes;
	ELFfde *fde;

   h = GetSectionByName(".debug_frame");
  	if(h == NULL)
    	return;
  	data = ReadSection(h,top);
  	topOffset = data;
 	end = data + h->size;
  	cies = NULL;
  	while(data < end) {
   	offset = data - topOffset;
    	len = Read4Bytes(data);
    	data += 4;
    	dataEnd = data + len;
    	id = Read4Bytes(data);
    	data += 4;
    	if(id == 0xffffffff){
      		data++;
      		cie = (ELFcie *)LocalAlloc(LPTR,sizeof(ELFcie));
      		cie->next = cies;
           cies = cie;
      		cie->offset = offset;
      		if((cie->augmentation = (BYTE *)ReadString(data++)) != NULL)
           	data += lstrlen((char *)cie->augmentation);
      		if(cie->augmentation != NULL && *cie->augmentation) {
        		//fprintf(stderr, "Error: augmentation not supported\n");
        		//exit(-1);
        		return;
      		}
      		cie->codeAlign = ReadLEB128(data,&bytes);
      		data += bytes;
      		cie->dataAlign = ReadSignedLEB128(data,&bytes);
      		data += bytes;
      		cie->returnAddress = 0;
           SeekRead(data++,&cie->returnAddress,1);
           cie->dataLen = dataEnd - data;
      		cie->data = (BYTE *)LocalAlloc(LPTR,cie->dataLen);
           SeekRead(data,cie->data,cie->dataLen);
           data += cie->dataLen;
    	}
       else {
      		fde = (ELFfde *)LocalAlloc(LPTR,sizeof(ELFfde));
      		cie = cies;
      		while(cie != NULL) {
        		if(cie->offset == id)
          			break;
        		cie = cie->next;
      		}
      		if(!cie) {
//        		fprintf(stderr, "Cannot find CIE %08x\n", id);
//        		exit(-1);
           	return;
      		}
      		fde->cie = cie;
      		fde->address = Read4Bytes(data);
      		data += 4;
      		fde->end = fde->address + Read4Bytes(data);
      		data += 4;
      		fde->dataLen = dataEnd - data;
           fde->data = (BYTE *)LocalAlloc(LPTR,fde->dataLen);
           SeekRead(data,fde->data,fde->dataLen);
           data += fde->dataLen;
      		if((FdeCount %10) == 0)
           	Fdes = (ELFfde **)LocalReAlloc(Fdes,(FdeCount+10) * sizeof(ELFfde *),LMEM_ZEROINIT);
      		Fdes[FdeCount++] = fde;
    	}
    	data = dataEnd;
  	}
  	Cies = cies;
}
//---------------------------------------------------------------------------
void LElfFile::ReadSymtab(DWORD data)
{
  	ELFSectionHeader *sh,*sh1;
	int table,count,i,type,binding,bind;
	ELFSymbol *s,*symtab;
	Symbol *sym;
	char *strtable;
	DWORD top;

  	sh = GetSectionByName(".symtab");
   if(sh == NULL)
   	return;
  	table = sh->link;
   top = ReadSection((sh1 = GetSectionByNumber(table)),data);
  	strtable = (char *)LocalAlloc(LPTR,sh1->size);
   SeekRead(top,strtable,sh1->size);
  	count = sh->size / sizeof(ELFSymbol);
   top = ReadSection(sh,data);
	symtab = (ELFSymbol *)LocalAlloc(LPTR,sh->size);
   SeekRead(top,symtab,sh->size);
  	SymbolsCount = 0;
  	Symbols = (Symbol *)LocalAlloc(LPTR,sizeof(Symbol)*count);
  	for(i = 0; i < count; i++) {
    	s = &symtab[i];
    	type = s->info & 15;
    	binding = s->info >> 4;
    	if(binding) {
      		sym = &Symbols[SymbolsCount];
      		sym->name = &strtable[s->name];
      		sym->binding = binding;
      		sym->type = type;
      		sym->value = s->value;
      		sym->size = s->size;
      		SymbolsCount++;
       }
  	}
  	for(i = 0; i < count; i++) {
    	s = &symtab[i];
    	bind = s->info>>4;
    	type = s->info & 15;
    	if(!bind) {
      		sym = &Symbols[SymbolsCount];
      		sym->name = &strtable[s->name];
      		sym->binding = (s->info >> 4);
      		sym->type = type;
      		sym->value = s->value;
      		sym->size = s->size;
      		SymbolsCount++;
       }
  	}
  	SymbolsStrTab = strtable;
}
//---------------------------------------------------------------------------
ELFSectionHeader *LElfFile::GetSectionByNumber(int number)
{
  	if(number < header.e_shnum)
   	return &SectionHeader[number];
  	return NULL;
}
//---------------------------------------------------------------------------
CompileUnit *LElfFile::GetCompileUnit(DWORD addr)
{
	CompileUnit *unit;
	ARanges *r;
	int count,j;

  	if(CompileUnits) {
    	unit = CompileUnits;
    	while(unit) {
      		if(unit->lowPC){
        		if(addr >= unit->lowPC && addr < unit->highPC)
          			return unit;
      		}
           else{
        		r = unit->ranges;
        		if(r){
          			count = r->count;
          			for(j = 0;j < count;j++) {
            			if(addr >= r->ranges[j].lowPC && addr < r->ranges[j].highPC)
              				return unit;
          			}
        		}
      		}
      		unit = unit->next;
    	}
  	}
  	return NULL;
}
//---------------------------------------------------------------------------
int LElfFile::GetVariableFromName(const char *name,DWORD adr,void **obj,void **sym,void **u)
{
   Symbol *s;
   elf_Object *v;
   int i,res;
  	CompileUnit *unit;
   Function *func;

   *obj = NULL;
   *sym = NULL;
   *u = NULL;
   res = -1;
   if(adr != (DWORD)-1){
  	    unit = GetCompileUnit(adr);
  	    if(unit != NULL){
  	        func = unit->functions;
  	        while(func){
  		        if(adr >= func->lowPC && adr < func->highPC){
                   v = func->variables;
                   while(v){
                       if(lstrcmpi(name,v->name) == 0){
                           *obj = v;
                           *u = func;
                           *sym = v->location;
                           for(i=0;i<LocationsCount;i++){
                               if(v->location == (ELFBlock *)Locations[i].offset){
                                   *sym = Locations[i].block;
                                   res++;
                                   break;
                               }
                           }
                           return ++res;
                       }
                       v = v->next;
                   }
                   v = func->parameters;
                   while(v != NULL){
                       if(lstrcmpi(name,v->name) == 0){
                           *obj = v;
                           *u = func;
                           *sym = v->location;
                           for(i=0;i<LocationsCount;i++){
                               if(v->location == (ELFBlock *)Locations[i].offset){
                                   *sym = Locations[i].block;
                                   res++;
                                   break;
                               }
                           }
                           return ++res;
                       }
                       v = v->next;
                   }
               }
  		        func = func->next;
  	        }
       }
   }
   for(i = 0;i < SymbolsCount;i++) {
       s = &Symbols[i];
       if(s->name != NULL && lstrcmpi(s->name,name) == 0){
           *sym = s;
           res++;
           break;
       }
   }
  	unit = CompileUnits;
   while(unit != NULL) {
       v = unit->variables;
       while(v){
           if(lstrcmpi(name, v->name) == 0) {
               *obj = v;
               return ++res;
           }
           v = v->next;
       }
       unit = unit->next;
   }
   return res;
}
//---------------------------------------------------------------------------
int LElfFile::GetFunctionFromName(const char *name,void **f,void **u)
{
  	CompileUnit *unit;
	Function *func;
   int res;

   res = -1;
  	unit = CompileUnits;
   while(unit != NULL){
  	    func = unit->functions;
  	    while(func){
  		    if(func->name != NULL && lstrcmpi(name,func->name) == 0){
               *f = func;
               *u = unit;
               return 1;
           }
  		    func = func->next;
  	    }
       unit = unit->next;
   }
   return res;
}
//---------------------------------------------------------------------------
int LElfFile::GetCurrentFunction(DWORD addr,void **f,void **u,void **l)
{
  	CompileUnit *unit;
	Function *func;
  	int count,i;
	LineInfoItem *table;

   if(f != NULL)
       *f = NULL;
   if(u != NULL)
       *u = NULL;
   if(l != NULL)
       *l = NULL;
  	unit = GetCompileUnit(addr);
  	if(unit == NULL || !unit->hasLineInfo)
   	return -1;
  	func = unit->functions;
  	while(func){
  		if(addr >= func->lowPC && addr < func->highPC)
          	break;
  		func = func->next;
  	}
 	count = unit->lineInfoTable->number;
  	table = unit->lineInfoTable->lines;
//   addr = unit->lowPC;
   for(i = 0;i < count;i++){
       if(addr > table[i].address)
           continue;
       if(f != NULL)
           *f = func;
       if(u != NULL)
           *u = unit;
       if(l != NULL)
           *l = &table[--i];
       return i;
   }
   return -1;
}
//---------------------------------------------------------------------------
DWORD LElfFile::Load()
{
	DWORD dwRes,adr;
	int i;
   u8 *mem;

   dwRes = 0;
   if(!Open())
   	return 0;
	if(ProgramHeader != NULL){
   	for(i=0;i<header.e_phnum;i++){
       	Seek(ProgramHeader[i].offset);
           if((ProgramHeader[i].paddr & 0x0F000000) == 0x02000000){
               mem = ext_mem;
               adr = ProgramHeader[i].paddr & 0x3FFFFF;
           }
           else if((ProgramHeader[i].paddr & 0x0F000000) == 0x03000000){
               if(ProgramHeader[i].paddr & 0x00800000){
                   adr = (DWORD)(WORD)ProgramHeader[i].paddr;
                   mem = int_mem2;
               }
               else{
                   adr = ProgramHeader[i].paddr & 0x7FFF;
                   mem = int_mem;
               }
           }
           else
               continue;
           Read(&mem[adr],ProgramHeader[i].filesz);
           dwRes = 0x10000;
       }
       if(dwRes)
           dwRes = header.e_entry;
   }
   if(SectionHeader != NULL){
      	for(i=0;i<header.e_shnum;i++){
       	if((SectionHeader[i].flags & 2) == 0)
           	continue;
//       	Seek(SectionHeader[i].offset);
//           Read(&ext_mem[SectionHeader[i].addr & 0x3FFFFF],SectionHeader[i].size);
       }
   }
	return dwRes;
}
//---------------------------------------------------------------------------
HLOCAL LElfFile::LocalReAlloc(HLOCAL hMem,UINT uBytes,UINT uFlags)
{
#ifdef __WIN32__
	HLOCAL mem;
   UINT uSize;

   mem = LocalAlloc(LPTR,uBytes);
   if(mem == NULL)
   	return NULL;
   if(hMem != NULL){
   	uSize = LocalSize(hMem);
       if(uSize > uBytes)
       	uSize = uBytes;
       CopyMemory(mem,hMem,uSize);
       LocalFree(hMem);
   }
   return mem;
#else
   return realloc(hMem,uBytes);
#endif
}










