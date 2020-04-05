#pragma once





struct ArFile
{
	struct Stat
	{
		u32 date;
		u32 owner;
		u32 group;
		u32 mode;
	};
	
	struct FileInfo {
		xstr name; Stat stat;
		xArray<byte> data;
		xArray<xstr> symb;
	};
	
	xArray<FileInfo> files;
	DEF_BEGINEND(FileInfo, 
		files, files.len);
	Stat stat;
	

	int load(cch* file);
	bool load(byte* data, u32 size);
	bool load(xarray<byte> xa) {
		return load(xa, xa.len); }
	
	int save(cch* file);
	~ArFile(); void free();
	
	static bool chk(byte* data, u32 size);
	
	
	
	
	enum {
		ERR_NONE, ERR_OPEN, 
		ERR_READ, ERR_BAD };
		
	static cch* errStr(int ec);
	
	
	// data access functions
	FileInfo* find(cch* symb);
	FileInfo* findFile(cch* name);

	// create/edit
	void addMove(FileInfo& that);
	FileInfo& addNew(void);
	FileInfo& replNew(cch* name);
	bool remove(int i);
	
private:
	//int parseSymbol(FileRead& fp);
	//int mapError(FileRead& fp);
	//int load(FileRead& fp);
	//void symbSort(void);
	
	
	
};
