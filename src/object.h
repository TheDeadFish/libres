#pragma once
struct CoffObjWr;

constexpr bool peCoff64(WORD Machine) { return Machine == 0x8664; }
constexpr WORD peCoff_relocType_ptr(WORD Machine) {
	if(Machine == 0x8664) return IMAGE_REL_AMD64_ADDR64;
	else return IMAGE_REL_I386_DIR32; }

struct CoffObjLd
{
	int load(cch* file);
	int load(byte* data, size_t size);
	int load(xarray<byte> xa) {
		return load(xa.data, xa.len); }
	void free();
	
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
		
		cstr name(char* strTab);
		cstr name(CoffObjLd& obj) { return name(obj.strTab); }
		
		// 
		void init_extData() { StorageClass=2; }
		void init_extFunc() { init_extData(); Type = 0x20; }
		void init_extData(DWORD sect) { Section = sect; init_extData(); }
		void init_extFunc(DWORD sect) { Section = sect; init_extFunc(); }
		void init_extData(DWORD sect, DWORD val) { Value = val; init_extData(sect); }
		void init_extFunc(DWORD sect, DWORD val) { Value = val; init_extFunc(sect); }		
		
		bool isFunc() { return Type == 0x20; }
		bool hasSect() { return iSect() < 0xFFFD; }
		u32 iSect() { return (Section-1); }
		
		DWORD Value;
		WORD Section;
		WORD Type;
		BYTE StorageClass;
		BYTE NumberOfAuxSymbols;
	} __attribute__((packed));
	
	struct Section : IMAGE_SECTION_HEADER
	{
		cstr name(CoffObjLd& This) { 
			return sect_getName(this, This.strTab); }
		xarray<byte> data(CoffObjLd& This) { 
			return {This.get(PointerToRawData), SizeOfRawData}; }
		xarray<ObjRelocs> relocs(CoffObjLd& This) {
			return {This.get(PointerToRelocations), NumberOfRelocations}; }
	};
	
	// section helper functions
	int findSect(cch* name);
	xarray<byte> sectData(int iSect) {
		return sections[iSect].data(*this); }

	Section& sect(int i) { return sections[i-1]; }
	cch* sect_name(int i) { return sect(i).name(*this); }
	u32 sect_size(int i) { return sect(i).SizeOfRawData; }
	xarray<byte> sect_data(int i) { return sect(i).data(*this); }
	xarray<ObjRelocs> sect_relocs(int i) { return sect(i).relocs(*this); }
	
	// symbol helper functions
	cstr symbName(int iSymb) { return symbols[iSymb].name(*this); }
	u32 symbValue(int iSymb) { return symbols[iSymb].Value; }
	u32 symbSect(int iSymb) { return symbols[iSymb].iSect(); }

	xarray<ObjSymbol> symbols;
	xarray<Section> sections;
	
	// string access
	Void get(u32 offset) { return 
		offset ? fileData+offset : 0; }
	
	char* strTab;
	
	
	
	xarray<byte> fileData;
	
	IMAGE_FILE_HEADER& ifh() { return 
		*(IMAGE_FILE_HEADER*)fileData.data; }
		
		
	static __thiscall cstr sect_getName(
		IMAGE_SECTION_HEADER* ish, char* strTab);
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
		bool mustFree;
		
		
		byte* xalloc(u32 size);
		

		xarray<byte> data;
		xarray<CoffObjLd::ObjRelocs> relocs;
		
		
		
		
		enum { 
			TYPE_CODE = IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_CNT_CODE,
			TYPE_RDATA = IMAGE_SCN_MEM_READ|IMAGE_SCN_CNT_INITIALIZED_DATA
		};
		
		
		constexpr int sectAlignMask(int x) {
			return ((__builtin_clz(x)^31)+1)<<20; }
			
		void init(int align, DWORD flags) {
			Characteristics = sectAlignMask(align)|flags; }
			
		void reloc_create(DWORD offset, DWORD symbol, WORD type) { 
			relocs.push_back(offset, symbol, type); }
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
	auto& symbol(int i) { return symbols[i]; }
	int symbol_find(cch* name);
	
	
	void init(WORD Machine);
	
	
	
	bool x64() { return Machine == 0x8664; }
	int ptrSz() { return x64() ? 8 : 4; }
	
	CoffObj();
	~CoffObj();
	void free();

	
//private:
	xArray<Section> sections;
	xVector<CoffObjLd::ObjSymbol> symbols;
	xVector<char> strTab;	
	int add_string(cch* name);
	void add_string(DWORD*, cch* name);
	void save(CoffObjWr& wr);
	cstr strGet(void* str);
};
