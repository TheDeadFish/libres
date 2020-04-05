#include <stdshit.h>
#include "arMerge.h"

static
void arMerge_symbols(ArFile::FileInfo& fi, CoffObjLd& co)
{
	// enumerate symbols
	FOR_FI(co.symbols, symb, i,
		if(symb.StorageClass == 2) {
			cstr str = symb.name(co.strTab);
			fi.symb.push_back(str.xdup().data);
		}
		i += symb.NumberOfAuxSymbols;
	);
}

cch* arMerge_object(ArFile& arOut,
	xarray<byte> file, cch* name, cch* prefix)
{
	CoffObjLd co;
	if(!co.load(file)) return name;
	auto& fiOut = arOut.replNew(Xstrfmt(
		"%s%s", prefix, getName(name).data));
	fiOut.data.init(co.fileData);
	arMerge_symbols(fiOut, co);
	return NULL;
}

cch* arMerge_library(ArFile& arOut,
	xarray<byte> file, cch* prefix)
{
	ArFile arIn;
	if(!arIn.load(file.data, file.len))	
		return (cch*)1;
	
	CoffObjLd co;
	for(auto& fi : arIn) {
		if(!co.load(fi.data)) return fi.name;
		auto& fiOut = arOut.replNew(Xstrfmt(
			"%s%s", prefix, fi.name.data));
		memswap(&fiOut.stat, &fi.stat,	
			sizeof(fi)-sizeof(xstr)); 
	}
	
	return 0;
}