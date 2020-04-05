#include <stdshit.h>
#include "arFile.h"
#include "object.h"

const char progName[] = "test";


void fatal_error(cch* fmt)
{
	printf(fmt);
	exit(1);

}

void rsrc_getNames_(xarray<WCHAR*>& nameLst, byte* data, u32 irdIdx)
{
	// peform bounds check
	IMAGE_RESOURCE_DIRECTORY* rsrcDir = Void(data,irdIdx);
	IMAGE_RESOURCE_DIRECTORY_ENTRY* entry = Void(rsrcDir+1);
	IMAGE_RESOURCE_DIRECTORY_ENTRY* lastEnt = entry + rsrcDir->
		NumberOfNamedEntries+rsrcDir->NumberOfIdEntries;
		
	// process directory entries	
	for(; entry < lastEnt; entry++) {
		if( entry->DataIsDirectory ) rsrc_getNames_(nameLst, data, entry->OffsetToDirectory);
		if( entry->NameIsString ) {
			IMAGE_RESOURCE_DIR_STRING_U* irds = Void(data,entry->NameOffset);
			
			// insert string into list
			cstrW str = {irds->NameString, irds->Length};
			for(WCHAR* s : nameLst) { if(!str.cmp(s)) goto FOUND; }
			nameLst.push_back(str.xdup()); FOUND:;
		}
	}
}

xarray<WCHAR*> rsrc_getNames(byte* data)
{
	xarray<WCHAR*> nameLst = {};
	rsrc_getNames_(nameLst, data, 0);
	return nameLst;
}

void add_string(CoffObj& obj, cchw* str, 
	ArFile::FileInfo* fi)
{
	// get section name
	xstr sn = Xstrfmt(".rdata$%sresn_%S", 
		obj.x64() ? "" : "_", str);
	printf("libres res: %s\n", sn+7);
	if(fi) fi->symb.push_back(xstrdup(sn+7));

	// create section
	int iSect = obj.sect_create(sn);
	obj.sect(iSect).Characteristics = 0x40300040;
	obj.sect(iSect).data.init((byte*)str, wcslen(str)*2+2);
	
	// check for existing symbol
	int prevSymb = obj.symbol_find(sn+7);
	if(prevSymb) fatal_error("duplicate resource symbol");
	
	// create symbol
	int iSymb = obj.symbol_create(sn+7, 0);
	obj.symbol(iSymb).Section = iSect;
	obj.symbol(iSymb).StorageClass = 2;
}





bool do_object(CoffObj& obj, xarray<byte> file,
	ArFile::FileInfo* fi)
{
	// open object file
	CoffObjLd co;
	if(!co.load(file.data, file.len))
		fatal_error("resource object bad");
		
	// get the list of names
	int iRsrc = co.findSect(".rsrc");
	if(iRsrc < 0) return false;
	auto strLst = rsrc_getNames(co.sectData(iRsrc));	
	
	// add the strings
	obj.load(co);
	for(WCHAR* str : strLst)
		add_string(obj, str, fi);
	return true;
}

int main(int argc, char** argv)
{
	if(argc != 3) {
		printf("libres <dst object> <res object>\n");
		printf("Deadfish shitware 2019\n"); return 1; }
	cch* resObj = argv[2];
	cch* outObj = argv[1];
	
	auto file = loadFile(resObj);
	if(!file) fatal_error("failed to load input file");
	if(ArFile::chk(file.data, file.len))
	{
		ArFile ar;
		if(!ar.load(file.data, file.len))
			fatal_error("library file bad");
		
		for(auto& fi : ar) {
			CoffObj obj;
			if(do_object(obj, fi.data, &fi)) {
				auto tmp = obj.save();
				fi.data.free(); fi.data.init(tmp);
			}
		}
		
		if(ar.save(outObj))
			fatal_error("failed to save library"); 

	} else {

		CoffObj obj;
		if(!do_object(obj, file, 0))
			fatal_error("no resource section");
		if(obj.save(outObj))
			fatal_error("failed to save resource object"); 			
	}
	
	return 0;
}
