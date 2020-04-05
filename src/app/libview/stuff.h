#pragma once

static
void showWindow(HWND hwnd, bool show) {
	ShowWindow(hwnd, show ? SW_SHOW : SW_HIDE); }

static
void lstView_initCol(HWND hList, cch* const lst[]) {
	for(int i = 0; lst[i]; i++) {
		lstView_insColumn(hList, i, 78, lst[i]); } }

static
void lstView_autoSize(HWND hList, int iCol)
{
	ListView_SetColumnWidth(hList, iCol, LVSCW_AUTOSIZE_USEHEADER);
	int headSize = ListView_GetColumnWidth(hList, iCol);
	ListView_SetColumnWidth(hList, iCol, LVSCW_AUTOSIZE);
	int bodySize = ListView_GetColumnWidth(hList, iCol);
	if(bodySize < headSize)
		ListView_SetColumnWidth(hList, iCol, headSize);
}

static
void combo_userSel(HWND hwnd, int ctrlId, int sel)
{
	if(dlgCombo_getSel(hwnd, ctrlId) == sel) return;
	dlgCombo_setSel(hwnd, ctrlId, sel);
	HWND hcb = GetDlgItem(hwnd, ctrlId);
	sendMessage(GetParent(hcb), WM_COMMAND, 
		MAKEWPARAM(ctrlId, CBN_SELCHANGE), (LPARAM)hcb);
}

char* stristr( const char* str1, const char* str2 )
{
	const char* p1 = str1 ;
	const char* p2 = str2 ;
	const char* r = *p2 == 0 ? str1 : 0 ;

	while( *p1 != 0 && *p2 != 0 )
	{
		if( tolower( (unsigned char)*p1 ) == tolower( (unsigned char)*p2 ) )
		{
			if( r == 0 )
			{
				r = p1 ;
			}

			p2++ ;
		}
		else
		{
			p2 = str2 ;
			if( r != 0 )
			{
				p1 = r + 1 ;
			}

			if( tolower( (unsigned char)*p1 ) == tolower( (unsigned char)*p2 ) )
			{
				r = p1 ;
				p2++ ;
			}
			else
			{
				r = 0 ;
			}
		}

		p1++ ;
	}

	return *p2 == 0 ? (char*)r : 0 ;
}