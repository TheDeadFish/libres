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
			for(WCHAR* s : nameLst) { if(str.cmp(s)) goto FOUND; }
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
	
	// print main body
	fprintf(fp, ".globl %s; .section .rdata$%s,\"r\";"
		".align 2; %s: .word ", symName, symName, symName);

	// output strings
	do { fprintf(fp, "%d%c", *str, 
		*str ? ',' : '\n'); }while(*str++);
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
		
	// get temp filename
	char tmpObj[MAX_PATH];
	GetTempFileNameA(".", "lro", 0, tmpObj);
	printf("tmp file: %s\n", tmpObj);
	
	// generate output
	FILE* fp = _wpopen(widen(
		Xstrfmt("as -o %z", tmpObj)), L"w");
	if(!fp) fatal_error("as failed");
	for(WCHAR* str : strLst) asm_getStr(fp, str);
	if(pclose(fp)) fatal_error("-- as failed --");

	// generate final output
	int x = system(Xstrfmt("ld %z %z -r -o %z",
		tmpObj, resObj, outObj));
	if(x) fatal_error("-- ld failed --");
	remove(tmpObj); return 0;
}
