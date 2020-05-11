#include <stdshit.h>
#include "object.h"

xarray<byte> thunkobj_iat(
	WORD Machine, cch* symName, cch* target)
{
	// create object & section
	CoffObj obj; obj.init(Machine);
	int iSect = obj.sect_create(".text$iat");
	auto& sect = obj.sect(iSect);
	sect.init(obj.ptrSz(), sect.TYPE_RDATA);
	sect.xalloc(obj.ptrSz());
	
	// create target symbol
	int iSymb = obj.symbol_create(target, 0);
	obj.symbol(iSymb).init_extData();
	
	// create relocation
	obj.sect(iSect).reloc_create(0, iSymb, 
		peCoff_relocType_ptr(Machine));
		
	// create pointer symbol
	iSymb = obj.symbol_create(symName, 0);
	obj.symbol(iSymb).init_extData(iSect);
	
	return obj.save();
}
