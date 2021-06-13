#include <stdshit.h>
#include "object.h"

const char progName[] = "objglobl";

void fatal_error(cch* fmt, ...)
{
	va_list args;
  va_start (args, fmt);
  vprintf (fmt, args);
	exit(1);
}

struct SymbolInfo {
	cch* name;
	bool write;
};

xarray<SymbolInfo> symbols;

void symbol_add(cstr name, bool write)
{
	// find existing symbol
	for(auto& symb : symbols) {
		if(!name.cmp(symb.name)) {
			symb.write |= write;
			return;
		}
	}

	// create new symbol
	symbols.push_back(name.xdup(), write);
}

bool symbol_isRead(cch* name)
{
	// find existing symbol
	for(auto& symb : symbols) {
		if(!strcmp(name,symb.name))
			return !symb.write;
	}

	return false;
}

void symbol_init(CoffObjLd& obj)
{
	FOR_FI(obj.symbols, symb, iSymb,
		if(symb.isExtern()) { symbol_add(symb.name(obj),
			obj.sect(symb.iSect()).isWr()); }
		iSymb += symb.NumberOfAuxSymbols;
	);
}

cch* g_sectName;
void print_sect_name()
{
	if(!g_sectName) return;
	printf("[SECTIONS: %s]\n", g_sectName);
	g_sectName = NULL;
}

void print_sect_symb(CoffObjLd& obj, cch* sectName, int& size)
{
	FOR_FI(obj.sections, sect, iSect,

		// validate the name
		cch* name = sect.name(obj);
		if((!strScmp(name, sectName))
		||(sect.SizeOfRawData == 0))
			continue;

		print_sect_name();
		printf("%s : %X\n", name, sect.SizeOfRawData);
		size += sect.SizeOfRawData;

		// loop over symbols
		FOR_FI(obj.symbols, symb, iSymb,
			if(symb.iSect() == (iSect+1)) {
				cstr symbName = symb.name(obj);
				if(symbName.cmp(name)) {
					printf("  %.*s\n", symbName.prn());
				}
			}

			iSymb += symb.NumberOfAuxSymbols;
		);
	);
}

struct RelocInfo
{
	cch* name; int type;
	cch* objName;
	cch* sectName;
	bool isExtern;

	RelocInfo* runLen(RelocInfo* end);
	RelocInfo* runLen2(RelocInfo* end);
};

int reloc_cmp(const RelocInfo& a, const RelocInfo& b) {
	IFRET(a.type - b.type);
	return strcmp(a.name, b.name);
};

int reloc_cmp2(const RelocInfo& a, const RelocInfo& b) {
	IFRET(reloc_cmp(a, b));
	IFRET(strcmp(a.objName, b.objName));
	IFRET(strcmp(a.sectName, b.sectName));
	return a.isExtern - b.isExtern;
}

RelocInfo* RelocInfo::runLen(RelocInfo* end)
{
	RelocInfo* pos = this+1;
	for(; pos < end; pos++) {
		if(reloc_cmp(*this, *pos))
			break;
	}
	return pos;
}

RelocInfo* RelocInfo::runLen2(RelocInfo* end)
{
	RelocInfo* pos = this+1;
	for(; pos < end; pos++) {
		if(reloc_cmp2(*this, *pos))
			break;
	}
	return pos;
}

xarray<RelocInfo> relocs;


void relocs_add(cch* name, int type, cch* objName, cch* sectName, bool isExtern)
{
	// lookup exsting item
	RelocInfo tmp = {name, type, objName, sectName, isExtern};
	for(auto& reloc : relocs) {
		if(!reloc_cmp2(reloc, tmp)) return;
	}

	// add unique item
	relocs.push_back(tmp);
}

void print_sect_relocs(CoffObjLd& obj, int type, cch* objName)
{
	// print relocations
	for(auto& sect : obj.sections) {
		cch* sectName = sect.name(obj);
		if(strScmp(sectName, ".debug"))
			continue;

		for(auto& reloc : sect.relocs(obj)) {
			if(reloc.type == type) {
				auto& symb = obj.symb(reloc.symbol);

				cstr symbName = symb.name(obj);
				if((symbName.scmp("__imp"))
				||(symbName.scmp("__tls"))
				||(symbName.scmp("??_"))
				||(symbName.scmp("___security_cookie")))
					continue;

				// check for data section
				bool isExtern = symb.isExtern();

				if(isExtern == false) {
					if((symb.hasSect())
					&&(!obj.sect(symb.iSect()).isWr()))
						continue;
				}

				relocs_add(symbName.xdup(), reloc.type, objName, sectName, isExtern);
			}
		}
	}
}


int g_dataSize;
int g_bssSize;
int g_tlsSize;

void dump_obj(cch* name)
{
	// load the object
	CoffObjLd obj;
	int err = obj.load(loadFile(name));
	if(err) { printf("load error: %d\n", err);
		exit(1); }

	g_sectName = name;
	symbol_init(obj);
	print_sect_symb(obj, ".data", g_dataSize);
	print_sect_symb(obj, ".bss", g_bssSize);
	print_sect_symb(obj, ".tls", g_tlsSize);
	print_sect_relocs(obj, 6, name);
	print_sect_relocs(obj, 11, name);
	if(!g_sectName) printf("\n");
}

void print_relocs()
{
	printf("[RELOCS]\n");

	qsort(relocs, reloc_cmp2);
	RelocInfo* relocPos = relocs.begin();
	RelocInfo* relocEnd = relocs.end();
	while(relocPos < relocEnd) {
		RelocInfo* next = relocPos->runLen(relocEnd);

		if((!relocPos->isExtern)
		||(!symbol_isRead(relocPos->name))) {
			printf("%d : %s - [", relocPos->type, relocPos->name);
			bool first = true;
			for(;relocPos < next; relocPos++) {
				if(!first) printf(", "); first = false;
				printf("%s:%s", relocPos->objName, relocPos->sectName);
			}
			printf("]\n");
		} else {
			relocPos = next;
		}
	}
}

void print_sizes()
{
	printf("\n");
	printf(".bss size: %d\n", g_bssSize);
	printf(".data size: %d\n", g_dataSize);
	printf(".tls size: %d\n", g_tlsSize);
}


int main(int argc, char** argv)
{
	// display ussage
	if(argc < 2) {
		fatal_error("objglobl: <input object>\n");
	}

	for(int i = 1; i < argc; i++) {
		dump_obj(argv[i]);
	}

	print_relocs();
	print_sizes();

	return 0;
}
