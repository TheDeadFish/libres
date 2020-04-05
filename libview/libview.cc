#include <stdshit.h>
#include <win32hlp.h>
#include "resource.h"
#include "arFile.h"
#include "stuff.h"

const char progName[] = "libView";

cch* const symLst[] = { "Name", "File", 0};
cch* const fileLst[] = { "Name", "Size", 0};

static ArFile s_arFile;
static HWND s_hFileView;
static HWND s_hSymbView;
static int s_viewSel;

void reset_dlg(HWND hwnd)
{
	ListView_DeleteAllItems(s_hFileView);
	ListView_DeleteAllItems(s_hSymbView);	
	dlgCombo_reset(hwnd, IDC_FILESEL);
	EnableDlgItem(hwnd, IDC_SAVE, 0);
}


void symbView_init(HWND hwnd)
{
	ListView_DeleteAllItems(s_hSymbView);
	
	int sel = IsDlgButtonChecked(hwnd, IDC_FILESYMB)
		? dlgCombo_getSel(hwnd, IDC_FILESEL) : -1;
		
	xstr find = getDlgItemText(hwnd, IDC_EDIT1);

	FOR_FI(s_arFile.files, file, i, 
		if((sel < 0)||(sel == i)) {
			for(auto& symb : file.symb) {
				if(find && !stristr(symb, find)) continue;
				int i = lstView_iosText(s_hSymbView, -1, symb);
				lstView_iosText(s_hSymbView, i, 1, file.name);
			}
		}
	);

	lstView_autoSize(s_hSymbView, 0);
}

void file_select(HWND hwnd)
{
	symbView_init(hwnd);
}

void fileView_init(HWND hwnd)
{
	reset_dlg(hwnd);

	for(auto& file : s_arFile.files)
	{
		dlgCombo_addStr(hwnd, IDC_FILESEL, file.name);
		int i = lstView_iosText(s_hFileView, -1, file.name);
		lstView_iosInt(s_hFileView, i, 1, file.data.len);
	}
	
	lstView_autoSize(s_hFileView, 0);
	dlgCombo_setSel(hwnd, IDC_FILESEL, 0);
	file_select(hwnd);
}


void load_file(HWND hwnd, cch* file)
{
	if(file == NULL) return;

	reset_dlg(hwnd);
	if(int ec = s_arFile.load(file)) {
		contError(hwnd, "failed to load library: %s\n", 
			s_arFile.errStr(ec)); return; 
	}
	
	setDlgItemText(hwnd, IDC_FNAME, file);
	fileView_init(hwnd);
}

void save_file(HWND hwnd)
{
	xstr name = getDlgItemText(hwnd, IDC_FNAME);
	if(int ec = s_arFile.save(name)) {
		contError(hwnd, "failed to save library: %s\n", 
			s_arFile.errStr(ec)); return; 
	}	
}

void selectTab(HWND hwnd)
{
	// select the window to show
	s_viewSel = getDlgTabPage(hwnd, IDC_TAB1);
	showWindow(s_hFileView, s_viewSel == 0);
	showWindow(s_hSymbView, s_viewSel == 1);
	//section_select(hwnd);
	//symbol_select(hwnd);
}

void mainDlgInit(HWND hwnd, cch* file)
{
	s_hFileView = GetDlgItem(hwnd, IDC_FILEVIEW);
	s_hSymbView = GetDlgItem(hwnd, IDC_SYMBVIEW);
	
	// initialize tab control
	addDlgTabPage(hwnd, IDC_TAB1, 0, "Files");
	addDlgTabPage(hwnd, IDC_TAB1, 1, "Symbols");
	selectTab(hwnd);
	
	
	// initialize listView columns
	lstView_initCol(s_hFileView, fileLst);
	lstView_initCol(s_hSymbView, symLst);	
	
	
	load_file(hwnd, file);
}

void load_file(HWND hwnd)
{
	OpenFileName ofn;
	if(!ofn.doModal(hwnd)) return;
	load_file(hwnd, ofn.lpstrFile);
}

void dropFiles(HWND hwnd, LPARAM lParam)
{
	xArray<xstr> files = hDropGet((HANDLE)lParam);
	load_file(hwnd, files[0]);
}

void file_keyDown(HWND hwnd, NMLVKEYDOWN& nvm)
{
	if(nvm.wVKey == VK_DELETE) {
		int sel = dlgCombo_getSel(hwnd, IDC_FILESEL);
		if(sel >= 0) {
			s_arFile.remove(sel);
			fileView_init(hwnd);
			EnableDlgItem(hwnd, IDC_SAVE, 1);
		}
	}
}

BOOL CALLBACK mainDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DLGMSG_SWITCH(
		ON_MESSAGE(WM_DROPFILES, dropFiles(hwnd, wParam))
	  CASE_COMMAND(
	    ON_COMMAND(IDCANCEL, EndDialog(hwnd, 0))
			ON_COMMAND(IDC_LOAD, load_file(hwnd))
			ON_COMMAND(IDC_SAVE, save_file(hwnd))
			ON_COMMAND(IDC_FILESYMB, symbView_init(hwnd))
			ON_COMMAND(IDC_FINDKILL, setDlgItemText(hwnd, IDC_EDIT1, ""))
			ON_CONTROL(EN_CHANGE, IDC_EDIT1, symbView_init(hwnd))
			ON_CONTROL(CBN_SELCHANGE, IDC_FILESEL, file_select(hwnd))
	  ,) 

		ON_MESSAGE(WM_INITDIALOG, mainDlgInit(hwnd, (cch*)lParam))
		
		CASE_NOTIFY(
			ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, selectTab(hwnd))
			ON_LVN_NOTIFY(LVN_ITEMCHANGED, IDC_FILEVIEW,
				combo_userSel(hwnd, IDC_FILESEL, nmv.iItem))
			ON_LVN_KEYDOWN(IDC_FILEVIEW, file_keyDown(hwnd, nmv))
	  ,)
	,)
}

int main(int argc, char** argv)
{
	DialogBoxParamW(NULL, MAKEINTRESOURCEW(IDD_DIALOG1), 
		NULL, mainDlgProc, (LPARAM)argv[1]);
}
