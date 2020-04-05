#include "arFile.h"
#include "object.h"

cch* arMerge_object(ArFile& arOut,
	xarray<byte> file, cch* name, cch* prefix);
cch* arMerge_library(ArFile& arOut,
	xarray<byte> file, cch* prefix);
