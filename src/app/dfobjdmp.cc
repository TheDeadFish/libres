#include <stdshit.h>
#include "arFile.h"
#include "object.h"

const char progName[] = "objdump";

void fatal_error(cch* fmt, ...)
{
	va_list args;
  va_start (args, fmt);
  vprintf (fmt, args);
	exit(1);
}

int main(int argc, char** argv)
{
	// display ussage
	if(argc < 2) {
		fatal_error("dfobjdmp: <input object>\n");
	}
		
	// load the object
		
	CoffObjLd obj;
	obj.load(loadFile(argv[1]));
	
	// print header
	printf("[IMAGE_FILE_HEADER]\n");
	#define x(nm) printf("\t%-22s %08X\n", #nm, obj.ifh().nm);
	x(Machine) x(NumberOfSections) x(TimeDateStamp)
  x(PointerToSymbolTable)x(NumberOfSymbols)
	x(SizeOfOptionalHeader) x(Characteristics)
	#undef x
	
	// print sections
	int iSect = 1;
	for(auto& sect : obj.sections) {
		printf("\n\n[SECTION:%d]\n", iSect++);
		printf("\t%-22s %.*s\n", "Name", sect.name(obj).prn());
		#define x(nm) printf("\t%-22s %08X\n", #nm, sect.nm);
		x(Misc.VirtualSize) x(VirtualAddress) x(SizeOfRawData)
		x(PointerToRawData)x(PointerToRelocations)
		x(PointerToLinenumbers) x(NumberOfRelocations)		
		x(NumberOfLinenumbers) x(Characteristics)		
		#undef x
	}

	iSect = 1;
	for(auto& sect : obj.sections) {
		printf("\n\n[RELOCS:%d]\n", iSect++);
		for(auto& reloc : sect.relocs(obj)) {
			printf("\t%08X, %08X, %04X\n", reloc.offset,
				reloc.symbol, reloc.type);
		}
	}
		
	// enumerate symbols
	printf("\n[SYMBOLS]\n");
	FOR_FI(obj.symbols, symb, i,
		printf("%\t%08X, %4d, %02X, %02X, %02X, %.*s\n", 
			symb.Value, (short)symb.Section, symb.Type, symb.StorageClass, 
			symb.NumberOfAuxSymbols, symb.name(obj.strTab).prn());
		i += symb.NumberOfAuxSymbols;
	);
	
	return 0;
}