#include <stdshit.h>
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

bool CoffObjLd::load(byte* data, size_t size)
{
	fileData = {data, size};
	FileRead rd = {{data, size}};
		
	// check header & section table
	if(rd.check(0, sizeof(IMAGE_FILE_HEADER))) 
		return false;
	IMAGE_FILE_HEADER* objHeadr = rd.get(0);
	DWORD nSects = objHeadr->NumberOfSections;
	if(rd.check(sizeof(IMAGE_FILE_HEADER), nSects, 
		sizeof(IMAGE_SECTION_HEADER))) return false;
		
	// check symbol table & string table
	DWORD nSymbols = objHeadr->NumberOfSymbols;
	DWORD iSymbols = objHeadr->PointerToSymbolTable;
	symbols.init(rd.get(iSymbols), nSymbols);
	DWORD iStrTab; if(rd.offset(iStrTab, iSymbols, 
		nSymbols, sizeof(ObjSymbol))) return false;
	strTab = rd.get(iStrTab);
	
	// load sections
	sections.init(rd.get(sizeof(IMAGE_FILE_HEADER)), nSects);
	for(auto& sect : sections) {
	
		// section name
		//char* Name = (char*)sect.Name;
		//if(Name[0] == '/') { u32 i = atoi(Name+1);
		//	RI(Name) = 0; RI(Name,4) = i; }
	}
	
	return true;
}	

int CoffObjLd::findSect(cch* find)
{
	for(int i = 0; i < sections.len; i++) {
		cstr name = sections[i].name(*this);
		if(!name.cmp(find)) return i; }
	return -1;
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
	if(strTab.dataSize == 0) {
		strTab.write32(4); }
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