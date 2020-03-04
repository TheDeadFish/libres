#include "stdshit.h"
#include "arFile.h"

// stdshit candidates
#define RngRBL Range
TMPL2(T,U) T pstAdd(T& dst, U dx) {
	T tmp = dst; dst += dx; return tmp; }	
REGCALL(2)
void fast_move(void* dst, void* src, size_t size) {
	memcpy(dst, src, size); memset(src, 0, size); }
TMPL(T) void fast_move(T& x, T& y) { 
	fast_move(&x, &y, sizeof(x)); }

static REGCALL(1) 
void itoax(char* pos, u32 val) 
{
	// convert to string
	char* base = pos;
	do { u32 rem; asm("xor %1,%1; div %2": "+a"(val),
		"=d"(rem) : "rm"(10)); *pos = '0'+rem;
		pos++; } while(val);
		
	// reverse the string
	for(;--pos > base; base++)	
		std::swap(*base, *pos);
}


	struct ArHeader {
		char fileName[16];
		char timeStamp[12];
		char ownerID[6];
		char groupID[6];
		char fileMode[8];
		char fileSize[10];
		char fileMagic[2]; 
	};
	
static const u64 arMagic = 0x0A3E686372613C21LL;
	
ArFile::~ArFile() { }
void ArFile::free() { pDel(this); ZINIT; }

struct EndPtr { 
	byte* end;
	
	EndPtr(void* x) : end((byte*)x) {}
	
	TMPL2(T=void, U) auto chk(U* ptr, size_t size) { TMPL_ALT(X,T,U); 
		return ovfAddChk2(end, ptr, ptr, size, sizeof(X)) ? 0 : notNull((X*)ptr); }
		
	TMPL3(T=void, U=void, V) auto get(V* p, size_t ofs=1, size_t size=0) {
		TMPL_ALT(X,U,V); TMPL_ALT(Y,T,V); return ovfAddChk(p, p, ofs, sizeof(X))
			? (Y*)0 : chk((Y*)p, size); }

	TMPL2(T=void, U) auto geti(U*& ptr, size_t size=1) { 
		TMPL_ALT(X,T,U); U* tmp;
		if(ovfAddChk2(end, tmp, ptr, size, sizeof(X))) return (X*)0;
		return notNull((X*)release(ptr, tmp)); }
	
	TMPL(T) u64 read64(T*& x) {
		if(!chk((byte*)x, 8)) return 0;
		return RDI(PQ(x)); 
	}
	
	TMPL(T) char* strchk(T*& pos) { return ovfStrChk(pos, end); }
	
	bool eof(void* x) { return (byte*)x == end; }
	bool chk(void* x) { return (byte*)x >= end; }
	size_t remain(void* x) { return PTRDIFF(end,x); }
};


static
void parse_stat(ArFile::Stat* stat, ArHeader* head)
{
	stat->date = atoi(head->timeStamp);
	stat->owner = atoi(head->ownerID);
	stat->group	= atoi(head->groupID);
	stat->mode = atoi(head->fileMode);


}

bool ArFile::load(byte* data, u32 size)
{
	byte* extNames = 0;
	u32* symTable = 0;
	byte* curPos = data;
	EndPtr end = data+size;	
		
	// read header
	if(end.read64(curPos) != arMagic)
		return false;
		
		
	// loop over files
	while(1) {
		// get file header
		curPos += (curPos-data)&1;
		if(end.eof(curPos)) break;
		ArHeader* head = end.geti<ArHeader>(curPos);
		if(!head) return false;
		
		// get file data
		u32 size = atoi(head->fileSize);
		byte* dataPos = end.geti(curPos, size);
		if(!dataPos) return false;
		
		// check the name
		char* name = head->fileName;
		if(head->fileName[0] == '/') {
		
			// special files
			if(head->fileName[1] == ' ') { 
				symTable = ((u32*)dataPos)+1;
				parse_stat(&stat, head); continue; }
			if(head->fileName[1] == '/') { 
				extNames = dataPos; continue; }
				
				
			// extended name
			u32 offset = atoi(head->fileName+1);
			if(!extNames || ovf_add(name, 
				extNames, offset)) return false;
		}
		
		// 
		auto& fInfo = files.xnxcalloc();
		fInfo.data.init(xmemdup8(dataPos, size), size);
		fInfo.stat.date = PTRDIFF(head, data);
		for(u32 len = 0;;len++) {
			if(end.chk(name+len)) return false;
			if(name[len] == '/') { fInfo.name 
				= xstrdup(name, len);	break; }
		}
	}

	// process the symbols
	if(symTable) {

		// check offset array
		if(end.chk(symTable)) return false;
		u32 count = bswap32(symTable[-1]);
		char* namePos = end.get<char>(symTable, count);
		if(!namePos) return false;
		
		// loop over symbols
		for(u32 offset : RngRBL(symTable, count)) {
			char* name = end.strchk(namePos);
			if(!name) return false;
			for(auto& fInfo : files) {
				if(fInfo.stat.date == bswap32(offset)) {
					fInfo.symb.push_back(xstrdup(name));
						goto FOUND_OFFSET; } }
			return false;
			FOUND_OFFSET:;
			
		}
	}

	// get the dates
	for(auto& file : files) {
		ArHeader* head = Void(data, file.stat.date);
		parse_stat(&file.stat, head);
	}

	return true;
}

int ArFile::load(cch* name)
{
	auto file = loadFile(name);
	if(!file) return ERR_OPEN;
	SCOPE_EXIT(::free(file));
	if(!load(file.data, file.len))
		return ERR_BAD; return 0;
}



struct ArFileWrite 
{
	u32 nameSize = 0;
	FILE* fp;
	
	void write(const void* data, int size) {
		xfwrite((byte*)data, size, fp); }
		
	NEVER_INLINE
	void put(int data, int size=4) {
		write(&data, size); } 
		
	void write_pad(int data) {
		if(ftell(fp)&1) put(data,1); }
		
	void write_head(ArFile::Stat* stat, u32 size, cch* name)
	{
		// initialize header
		ArHeader head;
		memset(&head, ' ', sizeof(head));
		head.fileName[0] = '/';
		
		// initialize name
		if(!name) { if(!stat) head.fileName[1] = '/'; 
		} else { int len = strlen(name); if(len < 16) {
				memcpyX(head.fileName, name, len)[0] = '/'; 
			} else { u32 index = pstAdd(nameSize, len+2);
				itoax(head.fileName+1, index); }
		}
		
		
		// initialize stat
		if(stat) {
			itoax(head.timeStamp, stat->date);
			itoax(head.ownerID, stat->owner);
			itoax(head.groupID, stat->group);
			itoax(head.fileMode, stat->mode);
		}		
		
		// write the header
		itoax(head.fileSize, size);
		RW(head.fileMagic) = 0x0A60;		
		write(&head, sizeof(head));
	}

		
ALWAYS_INLINE
int save(ArFile& ar, cch* name) 
{
	fp = xfopen(name, "wb");
	if(!fp) return ar.ERR_OPEN;
	write(&arMagic, 8);

	// calculate sizes
	u32 symSize = 0;
	u32 symCount = 0;
	for(auto& file: ar.files) {
		int len = strlen(file.name);
		if(len >= 16) nameSize += len+2;
		for(cch* name : file.symb) {
			symCount += 1;
			symSize += strlen(name)+5;
		}
	}

	// align sizes and get dataPos
	symSize = (symSize+5)&~1;
	u32 dataPos = symSize+sizeof(ArHeader)+8;
	nameSize = (nameSize+1)&~1; 
	dataPos += nameSize+sizeof(ArHeader);

	// write symbol table
	write_head(&ar.stat, symSize, 0);
	put(bswap32(symCount));
	for(auto& file : ar.files) {
		for(auto& sym : file.symb) {
			put(bswap32(dataPos)); }
		dataPos += sizeof(ArHeader);
		dataPos += ALIGN(file.data.len,1);
	}
	
	// write symbol names
	for(auto& file : ar.files)
	for(auto& sym : file.symb) {
		int len = strlen(sym);
		write(sym, len+1); }
	write_pad(0);
	
	// write extended names
	write_head(0, nameSize, 0);
	for(auto& file : ar.files) {
		int len = strlen(file.name);
		if(len < 16) continue;
		write(file.name, len);
		put('\n/', 2);
	}
	write_pad('\n');
	
	// write file data
	nameSize = 0;
	for(auto& file : ar.files) {
		write_head(&file.stat, file.data.len, file.name);
		write(file.data, file.data.len);
		write_pad('\n');
	}
	
	return 0;
}	



};

int ArFile::save(cch* file)
{
	ArFileWrite wr;
	return wr.save(*this, file);
}
	
cch* ArFile::errStr(int ec)
{
	switch(ec) {
	case ERR_OPEN: return "ERR_OPEN";
	case ERR_READ: return "ERR_READ";
	case ERR_BAD: return "ERR_BAD";
	default: return NULL; };
}

ArFile::FileInfo* ArFile::find(cch* symb)
{
	for(auto& file : files)
	for(char* fSymb : file.symb) {
		if(!strcmp(fSymb, symb))
			return &file; }
	return NULL;
}

void ArFile::addMove(FileInfo& that)
{
	auto& file = files.xnxalloc();
	fast_move(file, that);
}

ArFile::FileInfo& ArFile::addNew(void)
{
	auto& file = files.xnxcalloc();
	file.stat.mode = 100666;
	return file;
}

bool ArFile::chk(byte* data, u32 size)
{
	if(size < 8) return false;
	return RQ(data) == arMagic;
}
