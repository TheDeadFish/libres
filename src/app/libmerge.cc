#include <stdshit.h>
#include "arMerge.h"

const char progName[] = "libmerge";

void fatal_error(cch* fmt, ...)
{
	va_list args;
  va_start (args, fmt);
  vprintf (fmt, args);
	exit(1);
}

static cch* prefix="";
static ArFile arOut;


xarray<byte> thunkobj_iat(
	WORD Machine, cch* symName, cch* target);


void gen_iat_ptr(int iatIndex)
{
	int len = arOut.files.len;
	for(int i = 0; i < len; i++)
	for(cch* symb : arOut[i].symb) {

		// reasons to return 
		if(strScmp(symb, "__imp_")) continue;
		xstr iatName = xstrfmt("__imp_%s", symb);
		if(arOut.find(iatName)) continue;
		
		// create the iat thunk
		WORD Machine = RW(arOut[i].data.data);
		auto data = thunkobj_iat(Machine, iatName, symb);
		xstr objName = xstrfmt("iat_%s.o", symb);
		arMerge_object(arOut, data, objName, "");
	}
}



int main(int argc, char** argv)
{	
	int iatIndex = -1;

	// display ussage
	if(argc < 3) {
		fatal_error("resmerge: <output library> [-p] [input lib/obj]\n"
			"  option -p: specify object prefix\n"
			"  option -i: generate iat pointers\n"
			"  deadfish shitware 2020\n");
	}

	// load the library
	cch* outFile = argv[1];
	if(arOut.load(outFile) >= ArFile::ERR_READ) {
		fatal_error("failed to open output file: %s\n", outFile); }

	for(int i = 2; i < argc; i++) {
		if(strScmp(argv[i], "-p")) { prefix = argv[i]+2; continue; }
		if(strScmp(argv[i], "-i")) { iatIndex = arOut.files.len; continue; }
		auto file = loadFile(argv[i]);
		if(!file) fatal_error("failed to open input file: %s\n", argv[i]);
		
		char* err = arMerge_file(arOut, file, argv[i], prefix);
		if(err) fatal_error(err);
	}
	
	if(iatIndex >= 0) 
		gen_iat_ptr(iatIndex);
	
	// save the library file
	if(arOut.save(outFile))
		fatal_error("failed to write library: %s\n", outFile);
	return 0;
}
