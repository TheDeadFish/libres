#include <stdshit.h>

#define FOR_REV(var, rng, ...) { auto && __range = rng; \
	auto __begin = __range.end(); auto __end = __range.begin(); \
	while(__begin != __end) { var = *--__begin; __VA_ARGS__; }}

extern "C" void WINAPI OpenAs_RunDLLW(HWND hwnd, 
	HINSTANCE hAppInstance, LPCWSTR lpwszCmdLine, int nCmdShow);
void execute_file(HWND hwnd, LPCWSTR str)
{
	// explore the folder
	if(GetKeyState(VK_SHIFT) < 0) {
		int ec = int(ShellExecuteW(hwnd, L"explore", 
			getPath0(str), NULL, NULL, SW_SHOW));
		if(ec > 32) return;
		contError(hwnd, "failed to open folder");
		return;
	}

	// perform shellexecute
	if(GetKeyState(VK_CONTROL) >= 0) { 
		int ec = int(ShellExecuteW(hwnd,
			L"edit", str, NULL, NULL, SW_SHOW));
		if(ec > 32) return;
		ec = int(ShellExecuteW(hwnd,
			NULL, str, NULL, NULL, SW_SHOW));
		if(ec > 32) return; 
	}
	
	// open-with fallback
	OpenAs_RunDLLW(hwnd, (HINSTANCE)0,
		str, SW_SHOWNORMAL);
}

void execute_file(HWND hwnd, LPCSTR str) {
	execute_file(hwnd, widen(str)); }

// temporary directory creation
DWORD tmpInt7z(void)
{
	return (GetTickCount() << 12) 
		^ (GetCurrentThreadId() << 14) 
		^ (GetCurrentProcessId() << 0);
}

char* tmpDir7z(const char* prefix)
{
	WCHAR buff[MAX_PATH+32];
	int len = GetTempPathW(MAX_PATH, buff);
	if(prefix) {
	do { buff[len++] = *prefix;
	} while(*prefix++); }
	
	for(int i = 0; i < 100; i++) {
		_itow(tmpInt7z(), buff+len, 16);
		if(CreateDirectoryW(buff, 0))
			return utf816_dup(buff);
	}
	
	assert(0);
}

// tempory file delete list
static
xarray<char*> deleteList;

void deleteList_add(char* name)
{
	deleteList.push_back(name);
}

__attribute__((destructor))
void deleteList_func()
{
	FOR_REV(char* name, deleteList,
		wxstr wname = widen(name);
		DeleteFileW(wname);
		RemoveDirectoryW(wname);
	)
}

char* write_temp(char** tempDir, 
	cch* name, xarray<byte> data)
{
	if(!*tempDir) { *tempDir = tmpDir7z(0);
		deleteList_add(*tempDir); }
	char* fullName = pathCat(*tempDir, name);
	
	if(saveFile(fullName, data.data, data.len)) {
		free(fullName); return NULL; }
	deleteList_add(fullName);
	return fullName;
}	
