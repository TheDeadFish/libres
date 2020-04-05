#include <stdshit.h>
#include "arFile.h"
#include "object.h"
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

void add_object(ArFile::FileInfo& fi, CoffObjLd& co)
{
	// enumerate symbols
	FOR_FI(co.symbols, symb, i,
		if(symb.StorageClass == 2) {
			cstr str = symb.name(co.strTab);
			fi.symb.push_back(str.xdup().data);
		}
		i += symb.NumberOfAuxSymbols;
	);
	
	fi.data.init(co.fileData);
}

void add_file(xarray<byte> file, 
	cch* path, ArFile::FileInfo* fi)
{
	// open object file
	CoffObjLd co;
	if(!co.load(file.data, file.len)) {
		fatal_error("failed to load object: %s%c%s",
			path, fi ? ':' : 0, fi ? fi->name.data : "" );
	}
	
	// get object name
	xstr name = Xstrfmt("%s%s", prefix, 
		fi ? fi->name.data : getName(path).data);
	
	// insert the object
	auto& fiOut = arOut.replNew(name);
	if(fi) { memswap(&fiOut.stat, &fi->stat,
			sizeof(*fi)-sizeof(xstr)); 
	} else add_object(fiOut, co);
}

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
			ArFile arIn;
			if(!arIn.load(file.data, file.len))
				fatal_error("bad file: %s", argv[i]);
				
			for(auto& fi : arIn) {
				add_file(fi.data, argv[i], &fi);
			}
			
			file.free();			
				
		} else {
			add_file(file, argv[i], NULL);
		}
	}
	
	// save the library file
	if(arOut.save(outFile))
		fatal_error("failed to write library: %s\n", outFile);
	return 0;
}
