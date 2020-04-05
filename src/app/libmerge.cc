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

int main(int argc, char** argv)
{
	// display ussage
	if(argc < 3) {
		fatal_error("resmerge: <output library> [-p] [input lib/obj]\n"
			"  option -p: specify object prefix\n"
			"  deadfish shitware 2020\n");
	}

	// load the library
	cch* outFile = argv[1];
	if(arOut.load(outFile)) {
		fatal_error("failed to open output file: %s\n", outFile); }

	for(int i = 2; i < argc; i++) {
		if(strScmp(argv[i], "-p")) { prefix = argv[i]; continue; }
		auto file = loadFile(argv[i]);
		if(!file) fatal_error("failed to open input file: %s\n", argv[i]);
		
		if(ArFile::chk(file.data, file.len))
		{
			cch* err = arMerge_library(arOut, file, prefix);
			if(err) { if(IS_PTR(err)) { fatal_error(
				"bad object file: %s:%s", argv[i], err); }
				fatal_error("bad library file: %s", argv[i]); 
			}

			file.free();			
				
		} else {
			cch* err = arMerge_object(arOut, file, argv[i], prefix);
			if(err) fatal_error("bad object file: %s", err);
		}
	}
	
	// save the library file
	if(arOut.save(outFile))
		fatal_error("failed to write library: %s\n", outFile);
	return 0;
}
