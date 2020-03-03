#include <stdshit.h>
#include "object.h"

struct FileRead : xArray<byte>
{
	bool load(cch* name) { xArray::init(loadFile(name)); return data; }
	
	bool offset(DWORD& dst, DWORD ofs, DWORD len1, DWORD len2=1) {
		return  __builtin_mul_overflow(len1, len2, &len1) ||
		__builtin_add_overflow(len1, ofs, &dst) || dst >= len; }
	bool check(DWORD ofs, DWORD len1, DWORD len2=1) {
		return offset(ofs, ofs, len1, len2); }
	
	Void get(DWORD offset) { return Void(data, offset); }
};

int CoffObjLd::load(const char* name)
{
	// load the section
	FileRead rd; 
	if(!rd.load(name)) return ERROR_LOAD;
		
	// check header & section table
	if(rd.check(0, sizeof(IMAGE_FILE_HEADER))) 
		return ERROR_EOF;
	IMAGE_FILE_HEADER* objHeadr = rd.get(0);
	DWORD nSects = objHeadr->NumberOfSections;
	if(rd.check(sizeof(IMAGE_FILE_HEADER), nSects, 
		sizeof(IMAGE_SECTION_HEADER))) return ERROR_EOF;
		
	// check symbol table & string table
	DWORD nSymbols = objHeadr->NumberOfSymbols;
	DWORD iSymbols = objHeadr->PointerToSymbolTable;
	symbols.init(rd.get(iSymbols), nSymbols);
	DWORD iStrTab; if(rd.offset(iStrTab, iSymbols, 
		nSymbols, sizeof(ObjSymbol))) return ERROR_EOF;
	strTab = rd.get(iStrTab);
	
	// load sections
	sections.init(rd.get(sizeof(IMAGE_FILE_HEADER)), nSects);
	for(auto& sect : sections) {
	
		// section name
		char* Name = (char*)sect.Name;
		if(Name[0] == '/') { u32 i = atoi(Name+1);
			RI(Name) = 0; RI(Name,4) = i; }
	}
	
	fileData.init(rd.release());
	return ERROR_NONE;
}	
	
cstr CoffObjLd::strGet(void* str_)
{
	char* str = (char*)str_;
	if(RI(str)) return {str, strnlen(str, 8)};
	return cstr_len(strTab+RI(str,4));
}

int CoffObjLd::findSect(cch* name)
{
	for(int i = 0; i < sections.len; i++) {
		cstr name = sections[i].name(*this);
		if(!name.cmp(name)) return i; }
	return -1;
}
