#pragma once

struct CoffObjLd
{
	int load(const char* name);
	enum { ERROR_NONE, ERROR_LOAD, ERROR_EOF };
	
	struct ObjRelocs
	{
		DWORD offset;
		DWORD symbol;
		WORD type;
	} __attribute__((packed));
	
	
	struct ObjSymbol
	{
		union { char* name; struct { 
			DWORD Name1, Name2; }; };
		
		DWORD Value;
		WORD Section;
		WORD Type;
		BYTE StorageClass;
		BYTE NumberOfAuxSymbols;
	} __attribute__((packed));
	
	struct Section : IMAGE_SECTION_HEADER
	{
		cstr name(CoffObjLd& This) { return This.strGet(Name); }
		xarray<byte> data(CoffObjLd& This) { 
			return {This.get(PointerToRawData), SizeOfRawData}; }
		xarray<ObjRelocs> relocs(CoffObjLd& This) {
			return {This.get(PointerToRelocations), NumberOfRelocations}; }
	};
	
	// section helper functions
	int findSect(cch* name);
	xarray<byte> sectData(int iSect) {
		return sections[iSect].data(*this); }
		
		
	
	
	
	

	xarray<ObjSymbol> symbols;
	xarray<Section> sections;
	
	// string access
	Void get(u32 offset) { return 
		offset ? fileData+offset : 0; }
	
	cstr strGet(void* str);
	
	
	char* strTab;
	
	
	
	xArray<byte> fileData;
};
