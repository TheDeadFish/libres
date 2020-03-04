#include <stdshit.h>
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

void asm_getStr(FILE* fp, WCHAR* str)
{
	char symName[64];
	sprintf(symName, "_resn_%S", str);
	printf("libres res: %s\n", symName);
	
	// print main body
	fprintf(fp, ".globl %s; .section .rdata$%s,\"r\";"
		".align 2; %s: .word ", symName, symName, symName);

	// output strings
	do { fprintf(fp, "%d%c", *str, 
		*str ? ',' : '\n'); }while(*str++);
}

void add_string(CoffObj& obj, cchw* str)
{
	// get section name
	xstr sn = Xstrfmt(".rdata$%sresn_%S", 
		obj.x64() ? "" : "_", str);

	// create section
	int iSect = obj.sect_create(sn);
	obj.sect(iSect).Characteristics = 0x40300040;
	obj.sect(iSect).data.init((byte*)str, wcslen(str)*2+2);
	
	// create symbol
	int iSymb = obj.symbol_create(sn+7, 0);
	obj.symbol(iSymb).Section = iSect;
	obj.symbol(iSymb).StorageClass = 2;
}

int main(int argc, char** argv)
{
	if(argc != 3) {
		printf("libres <dst object> <res object>\n");
		printf("Deadfish shitware 2019\n"); return 1; }
	cch* resObj = argv[2];
	cch* outObj = argv[1];

	// get list of resources
	CoffObjLd co;
	if(co.load(resObj)) {
		fatal_error("failed to load resource object"); }
		
	int iRsrc = co.findSect(".rsrc");
	if(iRsrc < 0) fatal_error("no resource section");
	auto strLst = rsrc_getNames(co.sectData(iRsrc));
	
	CoffObj obj; obj.load(co);
	for(WCHAR* str : strLst) 
		add_string(obj, str);
	if(obj.save(outObj))
		fatal_error("failed to save resource object"); 
	
	return 0;
}
