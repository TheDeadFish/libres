#pragma once
struct CoffObjWr;

struct CoffObjLd
{
	bool load(byte* data, size_t size);
	
	struct ObjRelocs
	{
		DWORD offset;
		DWORD symbol;
		WORD type;
	} __attribute__((packed));
	
	
	struct ObjSymbol
	{
		//union { char* name; struct { 
		//	DWORD Name1, Name2; }; };
		
		DWORD Name1, Name2;
		
		
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
	
	
	
	xarray<byte> fileData;
	
	IMAGE_FILE_HEADER& ifh() { return 
		*(IMAGE_FILE_HEADER*)fileData.data; }
};

struct CoffObj
{
	// object info
	WORD Machine;
	WORD Characteristics;
	DWORD TimeDateStamp;
	
	struct Section {
		DWORD Name1, Name2;
		DWORD Characteristics;
		

		xarray<byte> data;
		xarray<CoffObjLd::ObjRelocs> relocs;
	};
	
	// load/save api
	void load(CoffObjLd& obj);
	int save(cch* file);
	xarray<byte> save();
	
	// section api
	Section& sect(int i) { return sections[i-1]; }
	cch* sect_name(int i);
	int sect_create(cch* name);
	
	// symbol api
	cch* symbol_name(int i);
	int symbol_create(cch* name, int extra);
	auto& symbol(int i) { return symbols[i-1]; }
	
	bool x64() { return Machine == 0x8664; }
	
//private:
	xArray<Section> sections;
	xVector<CoffObjLd::ObjSymbol> symbols;
	xVector<char> strTab;	
	int add_string(cch* name);
	void add_string(DWORD*, cch* name);
	void save(CoffObjWr& wr);
};
