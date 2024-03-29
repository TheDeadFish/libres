#include <stdshit.h>
#include <time.h>
#include "object.h"

cstr CoffObjLd::sect_getName(
	IMAGE_SECTION_HEADER* ish, char* strTab)
{
	char* name = (char*)ish->Name;
	if(*name != '/') return cstr_len(name,8);
	return strTab+atoi(name+1);
}

cstr CoffObjLd::ObjSymbol::name(char* strTab)
{
	char* str = (char*)&Name1;
	if(RI(str)) return cstr_len(str, 8);
	return cstr_len(strTab+RI(str,4));	
}

struct FileRead : xarray<byte>
{
	bool offset(DWORD& dst, DWORD ofs, DWORD len1, DWORD len2=1) {
		return  __builtin_mul_overflow(len1, len2, &len1) ||
		__builtin_add_overflow(len1, ofs, &dst) || dst >= len; }
	bool check(DWORD ofs, DWORD len1, DWORD len2=1) {
		return offset(ofs, ofs, len1, len2); }
	
	Void get(DWORD offset) { return Void(data, offset); }
};

int CoffObjLd::load(cch* name)
{
	fileName = name;
	auto data = loadFile(name);
	if(!data) return -1;
	int ret = load(data.data, data.len);
	if(ret) free(); return ret;
}

void CoffObjLd::free()
{
	::free(fileData); ZINIT;
}

int CoffObjLd::load(byte* data, size_t size)
{
	fileData = {data, size};
	FileRead rd = {{data, size}};
		
	// check header & section table
	if(rd.check(0, sizeof(IMAGE_FILE_HEADER))) 
		return 1;
	IMAGE_FILE_HEADER* objHeadr = rd.get(0);
	DWORD nSects = objHeadr->NumberOfSections;
	if(rd.check(sizeof(IMAGE_FILE_HEADER), nSects, 
		sizeof(IMAGE_SECTION_HEADER))) return 2;
		
	// check symbol table & string table
	DWORD nSymbols = objHeadr->NumberOfSymbols;
	DWORD iSymbols = objHeadr->PointerToSymbolTable;
	symbols.init(rd.get(iSymbols), nSymbols);
	DWORD iStrTab; if(rd.offset(iStrTab, iSymbols, 
		nSymbols, sizeof(ObjSymbol))) return 3;
	strTab = rd.get(iStrTab);
	
	// load sections
	sections.init(rd.get(sizeof(IMAGE_FILE_HEADER)), nSects);
	for(auto& sect : sections) {
	
		// section name
		//char* Name = (char*)sect.Name;
		//if(Name[0] == '/') { u32 i = atoi(Name+1);
		//	RI(Name) = 0; RI(Name,4) = i; }
	}
	
	return 0;
}	

int CoffObjLd::findSect(cch* find)
{
	for(int i = 0; i < sections.len; i++) {
		cstr name = sections[i].name(*this);
		if(!name.cmp(find)) return i+1; }
	return 0;
}


int CoffObj::sect_create(cch* name)
{
	auto& sect = sections.xnxcalloc();
	add_string(&sect.Name1, name);
	return sections.len;
}

int CoffObj::symbol_create(cch* name, int extra)
{
	// create 
	int index = symbols.getCount();
	auto* symb = symbols.xNalloc(extra+1);
	add_string(&symb->Name1, name);
	return index;
}

void CoffObj::add_string(DWORD* dst, cch* name)
{
	if(strlen(name) <= 8) {
		strncpy((char*)dst, name, 8);
	} else { dst[0] = 0;
		dst[1] = add_string(name); }
}

int CoffObj::add_string(cch* name)
{
	int index = strTab.getCount();
	strTab.strcat2(name);
	RI(strTab.dataPtr) = strTab.dataSize;
	return index;
}


void CoffObj::load(CoffObjLd& obj)
{
	Machine = obj.ifh().Machine;
	Characteristics = obj.ifh().Characteristics;
	TimeDateStamp = obj.ifh().TimeDateStamp;

	for(auto& ss : obj.sections) {
		auto& ds = sections.xnxalloc();
		RQ(&ds.Name1) = RQ(ss.Name);
		ds.data = ss.data(obj);
		ds.relocs = ss.relocs(obj);
		ds.Characteristics = ss.Characteristics;
	}

	symbols.xcopy(obj.symbols.data, obj.symbols.len);
	strTab.xcopy(obj.strTab, RI(obj.strTab));
}

struct CoffObjWr
{
	FILE* fp; byte *wrPos;
	DWORD dataPos, relocPos, symbPos;

	DWORD init(CoffObj& obj);
	void save(CoffObj& obj);
	void write(const void* data, int size);
};

DWORD CoffObjWr::init(CoffObj& obj)
{
	ZINIT;

	// calculate total size
	int size = sizeof(IMAGE_FILE_HEADER) +
		sizeof(IMAGE_SECTION_HEADER) * obj.sections.len;
	dataPos = size;
	for(auto& sect : obj.sections) size += sect.data.len;
	relocPos = size = ALIGN4(size);
	for(auto& sect : obj.sections) size += sect.relocs.dataSize();
	symbPos = size = ALIGN4(size);
	return size + obj.symbols.dataSize + obj.strTab.dataSize;
}

void CoffObjWr::write(const void* data, int size)
{
	if(fp) { if(!ferror(fp)) fwrite(data, size, 1, fp);
	} else { memcpy(wrPos, data, size); wrPos += size; }
}

void CoffObjWr::save(CoffObj& obj)
{
	// write header
	IMAGE_FILE_HEADER ifh = {obj.Machine, obj.sections.len, obj.TimeDateStamp,
		symbPos, obj.symbols.getCount(), 0, obj.Characteristics};
	write(&ifh, sizeof(ifh));
	
	// write sections
	for(auto& sect : obj.sections) {
		IMAGE_SECTION_HEADER ish = {};
		ish.Characteristics = sect.Characteristics;

		// write the name
		if(sect.Name1) RQ(ish.Name) = RQ(&sect.Name1);
		else { ish.Name[0] = '/';
			itoa(sect.Name2, (char*)ish.Name+1, 10); }
				
		// data pointer & size
		if(sect.data.len) {
			ish.SizeOfRawData = sect.data.len;
			ish.PointerToRawData = dataPos;
			dataPos += sect.data.len;
		}
		
		// relocs pointer & size
		if(sect.relocs.len) {
			ish.NumberOfRelocations = sect.relocs.len;
			ish.PointerToRelocations = relocPos;
			relocPos += sect.relocs.dataSize();
		}

		write(&ish, sizeof(ish)); 
	}
	
	// write section data
	for(auto& sect : obj.sections) {
		size_t size = sect.data.len;
		write(sect.data.data, size);
	} write("\0\0", -dataPos & 3);
	
	
	
	// write section relocs
	for(auto& sect : obj.sections) {
		size_t size = sect.relocs.dataSize();
		write(sect.relocs.data, size); 
	} write("\0\0", -relocPos & 3);
	
	// write symbols
	write(obj.symbols.data(), obj.symbols.dataSize); 
	write(obj.strTab.data(), obj.strTab.dataSize);
}

CoffObj::CoffObj() { strTab.write32(4); }
CoffObj::~CoffObj() {}
void CoffObj::free() { pRst(this); }

int CoffObj::save(cch* file)
{
	CoffObjWr wr; wr.init(*this);
	wr.fp = xfopen(file, "wb");
	if(!wr.fp) return errno;
	SCOPE_EXIT(fclose(wr.fp));
	wr.save(*this);
	return ferror(wr.fp);
}

xarray<byte> CoffObj::save()
{
	CoffObjWr wr;
	byte* data = malloc(wr.init(*this));
	if((wr.wrPos = data)) wr.save(*this);
	return {data, wr.wrPos};
}

int CoffObj::symbol_find(cch* name)
{
	FOR_FI(symbols, symb, i,
		cstr str = symb.name(strTab);
		if(!str.cmp(name)) return i+1;
		i += symb.NumberOfAuxSymbols;
	);

	return 0;
}

void CoffObj::init(WORD Machine)
{
	this->free(); this->Machine = Machine;
	Characteristics = peCoff64(Machine) ? 0x04 : 0x104;
	TimeDateStamp = time(0);
}

byte* CoffObj::Section::xalloc(u32 size)
{
	mustFree = true;
	return data.xcalloc(size);
}

int peCoff_mapRelocType(WORD Machine, WORD type)
{
	// x86 relocation types
	if(Machine == 0x14c) {
		switch(type) {
		case 0x06: return RelocType_DIR32;
		case 0x07: return RelocType_DIR32NB;
		case 0x14: return RelocType_REL32;
		default: break; }
	}

	// x64 relocation types
	ei(Machine == 0x8664) {
		switch(type) {
		case 0x01: return RelocType_DIR64;
		case 0x02: return RelocType_DIR32;
		case 0x03: return RelocType_DIR32NB;
		case 0x04: return RelocType_REL32;
		default: break; }
	}

	return -1;
}
