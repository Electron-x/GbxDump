////////////////////////////////////////////////////////////////////////////////////////////////
// GbxDump.cpp - Copyright (c) 2010-2019 by Electron.
//
// Licensed under the EUPL, Version 1.2 or - as soon they will be approved by
// the European Commission - subsequent versions of the EUPL (the "Licence");
// You may not use this work except in compliance with the Licence.
// You may obtain a copy of the Licence at:
//
// https://joinup.ec.europa.eu/software/page/eupl
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Licence is distributed on an "AS IS" basis,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the Licence for the specific language governing permissions and
// limitations under the Licence.
//
////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "file.h"
#include "tmx.h"
#include "dedimania.h"
#include "archive.h"
#include "dumpdds.h"
#include "dumppak.h"
#include "dumpgbx.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module
//
INT_PTR CALLBACK GbxDumpDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK OutputWndProc(HWND, UINT, WPARAM, LPARAM);

BOOL DumpFile(HWND hwndCtl, LPCTSTR lpszFileName, LPSTR lpszUid, LPSTR lpszEnvi);
BOOL DumpHex(HWND hwndCtl, HANDLE hFile, SIZE_T cbLen);
BOOL DumpMux(HWND hwndCtl, HANDLE hFile);

int SelectText(HWND hwndCtl);
BOOL SetWordWrap(HWND hDlg, BOOL bWordWrap);
HFONT CreateScaledFont(HDC hDC, LPCRECT lpRect, LPCTSTR lpszFormat);

HPALETTE DIBCreatePalette(HANDLE hDIB);
UINT DIBPaletteSize(LPSTR lpbi);
UINT DIBNumColors(LPCSTR lpbi);

void StoreWindowRect(HWND hwnd, LPRECT lprc);
void RetrieveWindowRect(HWND hwnd, LPRECT lprc);
void DeleteWindowRect(HWND hwnd);

////////////////////////////////////////////////////////////////////////////////////////////////
// Global Variables
//
#if defined(_WIN64)
#define PLATFORM TEXT("64-bit")
#else
#define PLATFORM TEXT("32-bit")
#endif

const TCHAR g_szTitle[]   = TEXT("GbxDump");
const TCHAR g_szAbout[]   = TEXT("Gbx File Dumper 1.64 (") PLATFORM TEXT(")\r\n")
                            TEXT("Copyright � 2010-2020 by Electron\r\n");
const TCHAR g_szDlgCls[]  = TEXT("GbxDumpDlgClass");
const TCHAR g_szTop[]     = TEXT("GbxDumpWndTop");
const TCHAR g_szBottom[]  = TEXT("GbxDumpWndBottom");
const TCHAR g_szLeft[]    = TEXT("GbxDumpWndLeft");
const TCHAR g_szRight[]   = TEXT("GbxDumpWndRight");
const TCHAR g_szArial[]   = TEXT("Arial");
const TCHAR g_szCourier[] = TEXT("Courier New");
const TCHAR g_szScrlBar[] = TEXT("Scrollbar");
const TCHAR g_szEdit[]    = TEXT("Edit");

WNDPROC g_lpPrevOutputWndProc = NULL;

HINSTANCE g_hInstance = NULL;
BOOL g_bGerUI = FALSE;

HANDLE g_hDibDefault = NULL;
HANDLE g_hDibThumb = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////
// Entry-point function of the application
//
int APIENTRY _tWinMain(__in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in LPTSTR lpCmdLine, __in int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(nCmdShow);

	LPTSTR pszCommandLine = NULL;
	LPTSTR pszFilename = NULL;

	// Check command arguments
	if (lpCmdLine != NULL && lpCmdLine[0] != '\0')
	{
		SIZE_T cchCmdLineLen = _tcslen(lpCmdLine);
		pszCommandLine = (LPTSTR)GlobalAllocPtr(GHND, (cchCmdLineLen + 1) * sizeof(TCHAR));
		if (pszCommandLine != NULL)
		{
			lstrcpyn(pszCommandLine, lpCmdLine, (int)cchCmdLineLen);
			pszFilename = pszCommandLine;

			// Remove quotation marks
			TCHAR* pch = _tcschr(pszFilename, TEXT('\"'));
			if (pch != NULL)
			{
				pszFilename = pch + 1;
				pch = _tcschr(pszFilename, TEXT('\"'));
				if (pch != NULL)
					*pch = TEXT('\0');
			}
		}
	}

	g_hInstance = hInstance;
	// Set the language of the user interface and error messages
	g_bGerUI = (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_GERMAN);

	// Load version 6 common controls and register scroll bar size box
	INITCOMMONCONTROLSEX iccex = {0};
	iccex.dwSize = sizeof(iccex);
	iccex.dwICC = ICC_BAR_CLASSES;
	InitCommonControlsEx(&iccex);

	WNDCLASSEX wcex    = {0};
	wcex.cbSize        = sizeof(wcex);
	wcex.style         = CS_DBLCLKS;
	wcex.lpfnWndProc   = DefDlgProc;
	wcex.cbClsExtra    = 0;
	wcex.cbWndExtra    = DLGWINDOWEXTRA;
	wcex.hInstance     = g_hInstance;
	wcex.hIcon         = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_GBXDUMP));
	wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wcex.lpszMenuName  = NULL;
	wcex.lpszClassName = g_szDlgCls;
	wcex.hIconSm       = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_GBXDUMP), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

	if (!RegisterClassEx(&wcex))
	{
		if (pszCommandLine != NULL)
			GlobalFreePtr((LPVOID)pszCommandLine);
		return 0;
	}

	// Load the default thumbnail from the resources and convert it to a DIB
	HRSRC hRes = FindResource(g_hInstance, MAKEINTRESOURCE(IDR_THUMB), RT_RCDATA);
	if (hRes != NULL)
	{
		DWORD dwSize = SizeofResource(g_hInstance, hRes);
		if (dwSize > 0)
		{
			HGLOBAL hResData = LoadResource(g_hInstance, hRes);
			if (hResData != NULL)
			{
				LPVOID pResData = LockResource(hResData);
				if (pResData != NULL)
				{
					__try { g_hDibDefault = JpegToDib(pResData, dwSize); }
					__except (EXCEPTION_EXECUTE_HANDLER) { g_hDibDefault = NULL; }
				}
			}
		}
	}

	// Create and display the main window
	INT_PTR nResult = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(g_bGerUI ? IDD_GER_GBXDUMP : IDD_ENG_GBXDUMP),
		NULL, (DLGPROC)GbxDumpDlgProc, (LPARAM)pszFilename);

	JpegFreeDib(g_hDibThumb);
	JpegFreeDib(g_hDibDefault);

	if (pszCommandLine != NULL)
		GlobalFreePtr((LPVOID)pszCommandLine);

	return (int)nResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Window function of the application
//
INT_PTR CALLBACK GbxDumpDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static POINT s_ptMinTrackSize = {0};
	static BOOL  s_bAboutBox = FALSE;
	static BOOL  s_bWordWrap = FALSE;
	static HFONT s_hfontDlgOrig = NULL;
	static HFONT s_hfontDlgCurr = NULL;
	static HFONT s_hfontEditBox = NULL;
	static HWND  s_hwndSizeBox = NULL;
	static DWORD s_dwFilterIndex = 1;
	static char  s_szUid[UID_LENGTH];
	static char  s_szEnvi[ENVI_LENGTH];
	static TCHAR s_szFileName[MAX_PATH];

	switch (message)
	{
		case WM_DRAWITEM:
			{ // Draw the thumbnail
				LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
				if (lpdis == NULL || wParam != IDC_THUMB)
					return FALSE;

				HDC hdc = lpdis->hDC;
				RECT rc = lpdis->rcItem;
				LONG cx = rc.right - rc.left;
				LONG cy = rc.bottom - rc.top;

				BOOL bDrawText = TRUE;
				HANDLE hDIB = g_hDibDefault;
				if (g_hDibThumb != NULL)
				{
					bDrawText = FALSE;
					hDIB = g_hDibThumb;
				}

				// If necessary, create and select a color palette
				HPALETTE hOldPal = NULL;
				HPALETTE hPal = (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) ? DIBCreatePalette(hDIB) : NULL;
				if (hPal != NULL)
				{
					hOldPal = SelectPalette(hdc, hPal, FALSE);
					if (hOldPal)
						RealizePalette(hdc);
				}

				if (hDIB != NULL)
				{ // Output the DIB
					LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
					if (lpbi != NULL)
					{
						int nSrcX = 0;
						int nSrcY = 0;
						int nSrcWidth  = ((LPBITMAPINFOHEADER)lpbi)->biWidth;
						int nSrcHeight = ((LPBITMAPINFOHEADER)lpbi)->biHeight;
						if (nSrcHeight < 0) nSrcHeight = -nSrcHeight;
						if (lpdis->itemState & ODS_SELECTED)
						{
							nSrcX = nSrcWidth  / 20;
							nSrcY = nSrcHeight / 20;
							nSrcWidth  -= 2 * nSrcX;
							nSrcHeight -= 2 * nSrcY;
						}

						LPCSTR pBuf = (LPSTR)lpbi + *(LPDWORD)lpbi + DIBPaletteSize((LPSTR)lpbi);

						SetStretchBltMode(hdc, HALFTONE);
						StretchDIBits(hdc, rc.left, rc.top, cx, cy, nSrcX, nSrcY, nSrcWidth, nSrcHeight,
							pBuf, (LPBITMAPINFO)lpbi, DIB_RGB_COLORS, SRCCOPY);

						GlobalUnlock(hDIB);
					}
				}
				else
					FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE + 1));

				if (bDrawText)
				{ // Draw "NO THUMBNAIL" lettering over the default thumbnail image
					TCHAR szText[256];
					if (LoadString(g_hInstance, g_bGerUI ? IDS_GER_THUMBNAIL : IDS_ENG_THUMBNAIL, szText, _countof(szText)) > 0)
					{
						COLORREF rgbTextOld = SetTextColor(hdc, RGB(255, 255, 255));
						int nBkModeOld = SetBkMode(hdc, TRANSPARENT);

						HFONT hfontOld = NULL;
						HFONT hfont = CreateScaledFont(hdc, &rc, szText);
						if (hfont != NULL)
							hfontOld = SelectFont(hdc, hfont);

						DrawText(hdc, szText, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX);

						if (hfontOld != NULL)
							SelectFont(hdc, hfontOld);
						if (hfont != NULL)
							DeleteFont(hfont);

						if (nBkModeOld != 0)
							SetBkMode(hdc, nBkModeOld);
						if (rgbTextOld != CLR_INVALID)
							SetTextColor(hdc, rgbTextOld);
					}
				}

				if (hOldPal != NULL)
					SelectPalette(hdc, hOldPal, FALSE);
				if (hPal != NULL)
					DeletePalette(hPal);

				return TRUE;
			}

		case WM_CTLCOLORSTATIC:
			if (GetDlgItem(hDlg, IDC_OUTPUT) == (HWND)lParam)
			{
				SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
				SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
				SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)GetSysColorBrush(COLOR_WINDOW));
				return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
			}

		case WM_CTLCOLORBTN:
			if (GetDlgItem(hDlg, IDC_THUMB) == (HWND)lParam)
			{
				SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)GetStockBrush(NULL_BRUSH));
				return (INT_PTR)GetStockBrush(NULL_BRUSH);
			}

		case WM_QUERYNEWPALETTE:
			{
				UINT colChanged = 0;

				if (g_hDibDefault != NULL || g_hDibThumb != NULL)
				{
					HPALETTE hPal = DIBCreatePalette(g_hDibThumb != NULL ? g_hDibThumb : g_hDibDefault);
					if (hPal != NULL)
					{
						HDC hDC = GetDC(hDlg);
						HPALETTE hOldPal = SelectPalette (hDC, hPal, FALSE);
						colChanged = RealizePalette(hDC);
						if (hOldPal != NULL)
							SelectPalette (hDC, hOldPal, FALSE);
						ReleaseDC(hDlg, hDC);
						DeletePalette(hPal);
						if (colChanged != 0)
							InvalidateRect(hDlg, NULL, TRUE);
					}
				}
				return (colChanged);
			}

		case WM_PALETTECHANGED:
			{
				if (wParam != (WPARAM)hDlg && (g_hDibDefault != NULL || g_hDibThumb != NULL))
				{
					HPALETTE hPal = DIBCreatePalette(g_hDibThumb != NULL ? g_hDibThumb : g_hDibDefault);
					if (hPal != NULL)
					{
						HDC hDC = GetDC(hDlg);
						HPALETTE hOldPal = SelectPalette(hDC, hPal, TRUE);
						UINT colChanged = RealizePalette(hDC);
						if (colChanged != 0)
							UpdateColors(hDC);
						if (hOldPal != NULL)
							SelectPalette(hDC, hOldPal, TRUE);
						ReleaseDC(hDlg, hDC);
						DeletePalette(hPal);
					}
				}
				return FALSE;
			}

		case WM_DPICHANGED:
			{
				LOGFONT lf;
				ZeroMemory(&lf, sizeof(lf));
				lf.lfHeight = -MulDiv(10, HIWORD(wParam), 72);
				lf.lfWeight = FW_NORMAL;
				lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
				lstrcpyn(lf.lfFaceName, g_szCourier, _countof(lf.lfFaceName));

				HWND hwndCtl = GetDlgItem(hDlg, IDC_OUTPUT);
				if (hwndCtl != NULL)
				{
					if (s_hfontEditBox != NULL)
						DeleteFont(s_hfontEditBox);
					s_hfontEditBox = CreateFontIndirect(&lf);
					if (s_hfontEditBox != NULL)
						SetWindowFont(hwndCtl, s_hfontEditBox, TRUE);
				}

				return FALSE;
			}

		case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
				lpmmi->ptMinTrackSize = s_ptMinTrackSize;
				return FALSE;
			}

		case WM_SIZE:
			{ // Processing of the child windows of a resizable dialog box
				if (wParam == SIZE_MINIMIZED || wParam == SIZE_MAXHIDE || wParam == SIZE_MAXSHOW)
					return FALSE;

				// Determine the new dimensions
				RECT rc, rcInit;
				if (!GetClientRect(hDlg, &rc))
					return FALSE;
				RetrieveWindowRect(hDlg, &rcInit);

				float xMult = (float)(rc.right - rc.left) / (rcInit.right - rcInit.left);
				float yMult = (float)(rc.bottom - rc.top) / (rcInit.bottom - rcInit.top);

				// Fill the LOGFONT structure with the old font
				LOGFONT lf;
				ZeroMemory(&lf, sizeof(lf));
				if (s_hfontDlgOrig != NULL)
					GetObject(s_hfontDlgOrig, sizeof(LOGFONT), (LPVOID)&lf);

				// Scale the font
				lf.lfHeight = (int)((yMult * lf.lfHeight) + ((lf.lfHeight < 0) ? (-0.5) : (0.5)));
				if (lf.lfWidth != 0) lf.lfWidth = (int)((xMult * lf.lfWidth) + 0.5);

				// Delete the old font and create the new one
				if (s_hfontDlgCurr != NULL)
					DeleteFont(s_hfontDlgCurr);

				s_hfontDlgCurr = CreateFontIndirect(&lf);

				// Assign the new font to the dialog window
				if (s_hfontDlgCurr != NULL)
					SetWindowFont(hDlg, s_hfontDlgCurr, FALSE);

				LONG lThumbnailMax = 0;
				HWND hCtlEdit = GetDlgItem(hDlg, IDC_OUTPUT);
				HWND hCtlThumb = GetDlgItem(hDlg, IDC_THUMB);
				HWND hCtlDediBtn = GetDlgItem(hDlg, IDC_DEDIMANIA);

				// Count all child windows for DeferWindowPos
				int nNumWindows = 0;
				HWND hCtl = GetWindow(hDlg, GW_CHILD);
				while (hCtl)
				{
					nNumWindows++;
					hCtl = GetNextSibling(hCtl);
				}

				// Reposition all child windows
				HDWP hWinPosInfo = BeginDeferWindowPos(nNumWindows);
				if (hWinPosInfo != NULL)
				{
					// Start with the first child window and run through all controls
					hCtl = GetWindow(hDlg, GW_CHILD);
					while (hCtl)
					{
						// Assign the new font to the control.
						// The Edit window keeps the globally defined font size.
						if (s_hfontDlgCurr != NULL && hCtl != hCtlEdit)
							SetWindowFont(hCtl, s_hfontDlgCurr, FALSE);

						// Determine the initial size of the control
						RetrieveWindowRect(hCtl, &rc);
						// Calculate the new window size accordingly
						rc.top    = (int)((yMult * rc.top)    + 0.5);
						rc.bottom = (int)((yMult * rc.bottom) + 0.5);
						rc.left   = (int)((xMult * rc.left)   + 0.5);
						rc.right  = (int)((xMult * rc.right)  + 0.5);

						// Maximum height for the thumbnail
						if (hCtl == hCtlDediBtn)
							lThumbnailMax = rc.bottom;

						// Adjust the size of the child window. The thumbnail
						// window and the size box require special handling.
						if (hCtl == hCtlThumb)
							hWinPosInfo = DeferWindowPos(hWinPosInfo, hCtl, NULL, rc.left,
								max(lThumbnailMax, rc.bottom-(rc.right-rc.left)), rc.right-rc.left,
								min(rc.bottom-lThumbnailMax, rc.right-rc.left), SWP_NOZORDER | SWP_NOACTIVATE);
						else if (hCtl == s_hwndSizeBox)
						{
							int cx = GetSystemMetrics(SM_CXVSCROLL);
							int cy = GetSystemMetrics(SM_CYHSCROLL);
							if (wParam == SIZE_MAXIMIZED)
								ShowWindow(hCtl, SW_HIDE);
							else if (!IsWindowVisible(hCtl))
								ShowWindow(hCtl, SW_SHOW);
							hWinPosInfo = DeferWindowPos(hWinPosInfo, hCtl, NULL,
								rc.right-cx, rc.bottom-cy, cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);
						}
						else
							hWinPosInfo = DeferWindowPos(hWinPosInfo, hCtl, NULL, rc.left, rc.top,
								(rc.right-rc.left), (rc.bottom-rc.top), SWP_NOZORDER | SWP_NOACTIVATE);

						// Determine the next child window
						hCtl = GetNextSibling(hCtl);
					}

					if (hWinPosInfo != NULL)
						EndDeferWindowPos(hWinPosInfo);
				}

				return FALSE;
			}

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:
					{
						DeleteWindowRect(hDlg);
						HWND hCtl = GetWindow(hDlg, GW_CHILD);
						while (hCtl)
						{
							DeleteWindowRect(hCtl);
							hCtl = GetNextSibling(hCtl);
						}

						if (s_hwndSizeBox != NULL)
						{
							DestroyWindow(s_hwndSizeBox);
							s_hwndSizeBox = NULL;
						}

						hCtl = GetWindow(hDlg, IDC_OUTPUT);
						if (hCtl != NULL && g_lpPrevOutputWndProc != NULL)
							SubclassWindow(hCtl, g_lpPrevOutputWndProc);

						if (s_hfontEditBox != NULL)
						{
							DeleteFont(s_hfontEditBox);
							s_hfontEditBox = NULL;
						}

						if (s_hfontDlgCurr != NULL)
						{
							DeleteFont(s_hfontDlgCurr);
							s_hfontDlgCurr = NULL;
						}

						EndDialog(hDlg, LOWORD(wParam));
					}
					return FALSE;

				case IDC_OPEN:
					{
						if (GetFileName(hDlg, s_szFileName, _countof(s_szFileName), &s_dwFilterIndex))
						{
							HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
							HWND hwndCtl = GetDlgItem(hDlg, IDC_OUTPUT);

							// Clear the output window
							Edit_SetText(hwndCtl, TEXT(""));
							Edit_EmptyUndoBuffer(hwndCtl);
							Edit_SetModify(hwndCtl, FALSE);
							UpdateWindow(hwndCtl);

							// Dump the file
							if (DumpFile(hwndCtl, s_szFileName, s_szUid, s_szEnvi) && GetFocus() != hwndCtl)
							{
								SetFocus(hwndCtl);
								int nLen = Edit_GetTextLength(hwndCtl);
								Edit_SetSel(hwndCtl, nLen, nLen);
							}

							SetCursor(hOldCursor);
						}
					}
					return FALSE;

				case IDC_COPY:
					{
						HWND hwndCtl = GetDlgItem(hDlg, IDC_OUTPUT);
						DWORD dwSelection = Edit_GetSel(hwndCtl);
						if (HIWORD(dwSelection) != LOWORD(dwSelection))
							SendMessage(hwndCtl, WM_COPY, 0, 0);
						else
						{
							Edit_SetSel(hwndCtl, 0, -1);
							SendMessage(hwndCtl, WM_COPY, 0, 0);
						}
					}
					return FALSE;

				case IDC_WORDWRAP:
					{
						HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
						if (s_bWordWrap)
						{
							if (SetWordWrap(hDlg, FALSE))
								s_bWordWrap = FALSE;
						}
						else
						{
							if (SetWordWrap(hDlg, TRUE))
								s_bWordWrap = TRUE;
						}
						SetCursor(hOldCursor);
					}
					return FALSE;

				case IDC_TMX:
					{
						HWND hwndCtl = GetDlgItem(hDlg, IDC_OUTPUT);
						if (GetFocus() != hwndCtl)
							SetFocus(hwndCtl);

						// Place the cursor at the end of the text
						int nLen = Edit_GetTextLength(hwndCtl);
						Edit_SetSel(hwndCtl, nLen, nLen);

						Button_Enable(GetDlgItem(hDlg, IDC_TMX), FALSE);
						if (!GetTmxData(hwndCtl, s_szUid, s_szEnvi))
							Button_Enable(GetDlgItem(hDlg, IDC_TMX), TRUE);
					}
					return FALSE;

				case IDC_DEDIMANIA:
					{
						HWND hwndCtl = GetDlgItem(hDlg, IDC_OUTPUT);
						if (GetFocus() != hwndCtl)
							SetFocus(hwndCtl);

						// Place the cursor at the end of the text
						int nLen = Edit_GetTextLength(hwndCtl);
						Edit_SetSel(hwndCtl, nLen, nLen);

						Button_Enable(GetDlgItem(hDlg, IDC_DEDIMANIA), FALSE);
						if (!GetDedimaniaData(hwndCtl, s_szUid, s_szEnvi))
							Button_Enable(GetDlgItem(hDlg, IDC_DEDIMANIA), TRUE);
					}
					return FALSE;

				case IDC_THUMB_COPY:
					{ // Copy the current thumbnail DIB to the clipboard
						HANDLE hDIB = g_hDibThumb ? g_hDibThumb : g_hDibDefault;
						if (hDIB == NULL)
							return FALSE;

						SIZE_T cbLen = GlobalSize((HGLOBAL)hDIB);
						if (cbLen == 0)
							return FALSE;

						HGLOBAL hNewDIB = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cbLen);
						if (hNewDIB == NULL)
							return FALSE;

						register LPBYTE lpSrc  = (LPBYTE)GlobalLock((HGLOBAL)hDIB);
						register LPBYTE lpDest = (LPBYTE)GlobalLock((HGLOBAL)hNewDIB);

						__try
						{
							while (cbLen--)
								*lpDest++ = *lpSrc++;
						}
						__except (EXCEPTION_EXECUTE_HANDLER) { cbLen = 0; }

						GlobalUnlock((HGLOBAL)hDIB);
						GlobalUnlock((HGLOBAL)hNewDIB);

						if (!OpenClipboard(hDlg))
						{
							GlobalFree((HGLOBAL)hNewDIB);
							return FALSE;
						}

						HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
						if (!EmptyClipboard())
						{
							CloseClipboard();
							SetCursor(hOldCursor);
							GlobalFree((HGLOBAL)hNewDIB);
							return FALSE;
						}

						if (SetClipboardData(CF_DIB, (HANDLE)hNewDIB) == NULL)
							GlobalFree((HGLOBAL)hNewDIB);

						CloseClipboard();
						SetCursor(hOldCursor);
					}
					return FALSE;

				case IDC_THUMB_SAVE:
					{ // Save the current thumbnail as a Windows Bitmap file
						DWORD dwFilterIndex = 1;
						TCHAR szFileName[MAX_PATH];
						lstrcpyn(szFileName, s_szFileName, _countof(szFileName));

						if (GetFileName(hDlg, szFileName, _countof(szFileName), &dwFilterIndex, TRUE))
						{
							HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
							SaveBmpFile(szFileName, g_hDibThumb ? g_hDibThumb : g_hDibDefault);
							SetCursor(hOldCursor);
						}
					}
					return FALSE;
			}
			return FALSE;

		case WMU_FILEOPEN:
			{ // Open the GBX file passed by program argument
				HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
				HWND hwndCtl = GetDlgItem(hDlg, IDC_OUTPUT);

				// Dump the file
				if (DumpFile(hwndCtl, (LPCTSTR)lParam, s_szUid, s_szEnvi))
				{
					SetFocus(hwndCtl);
					int nLen = Edit_GetTextLength(hwndCtl);
					Edit_SetSel(hwndCtl, nLen, nLen);
				}

				if ((GetWindowStyle(hwndCtl) & ES_READONLY) == 0)
					Edit_SetReadOnly(hwndCtl, TRUE);

				SetCursor(hOldCursor);
			}
			return TRUE;

		case WM_DROPFILES:
			{ // Open all GBX files transferred via drag-and-drop
				HDROP hDrop = (HDROP)wParam;
				if (hDrop == NULL)
					return FALSE;

				if (IsMinimized(hDlg))
					ShowWindow(hDlg, SW_SHOWNORMAL);
				SetForegroundWindow(hDlg);

				UINT nFiles = DragQueryFile(hDrop, (UINT)-1, NULL, 0);
				if (nFiles == 0)
				{
					DragFinish(hDrop);
					return FALSE;
				}

				HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
				HWND hwndCtl = GetDlgItem(hDlg, IDC_OUTPUT);

				// Clear the output window
				Edit_SetText(hwndCtl, TEXT(""));
				Edit_EmptyUndoBuffer(hwndCtl);
				Edit_SetModify(hwndCtl, FALSE);
				UpdateWindow(hwndCtl);

				BOOL bSuccess = FALSE;
				for (UINT iFile = 0; iFile < nFiles; iFile++)
				{
					DragQueryFile(hDrop, iFile, s_szFileName, _countof(s_szFileName));

					if (iFile > 0)
						Edit_ReplaceSel(hwndCtl, TEXT("\r\n"));

					// Dump the file
					bSuccess = DumpFile(hwndCtl, s_szFileName, s_szUid, s_szEnvi) || bSuccess;
				}

				DragFinish(hDrop);

				if (bSuccess && GetFocus() != hwndCtl)
				{
					SetFocus(hwndCtl);
					int nLen = Edit_GetTextLength(hwndCtl);
					Edit_SetSel(hwndCtl, nLen, nLen);
				}

				SetCursor(hOldCursor);
			}
			return TRUE;

		case WM_CONTEXTMENU:
			{ // Displays the context menu for the thumbnail
				HWND hwndCtl = GetDlgItem(hDlg, IDC_THUMB);
				if ((HWND)wParam == hwndCtl)
				{
					int x = (int)(short)LOWORD(lParam);
					int y = (int)(short)HIWORD(lParam);

					if (x == -1 || y == -1)
					{
						RECT rc;
						GetWindowRect(hwndCtl, &rc);
						x = (rc.left + rc.right) / 2;
						y = (rc.top + rc.bottom) / 2;
					}

					HMENU hmenuPopup = LoadMenu(g_hInstance,
						MAKEINTRESOURCE(g_bGerUI ? IDR_GER_POPUP : IDR_ENG_POPUP));
					if (hmenuPopup == NULL)
						return FALSE;

					HMENU hmenuTrackPopup = GetSubMenu(hmenuPopup, 0);
					if (hmenuTrackPopup == NULL)
					{
						DestroyMenu(hmenuPopup);
						return FALSE;
					}

					if (g_hDibThumb == NULL)
					{
						EnableMenuItem(hmenuTrackPopup, IDC_THUMB_COPY, MF_BYCOMMAND | MF_GRAYED);
						EnableMenuItem(hmenuTrackPopup, IDC_THUMB_SAVE, MF_BYCOMMAND | MF_GRAYED);
					}

					BOOL bSuccess = TrackPopupMenu(hmenuTrackPopup,
						TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, x, y, 0, hDlg, NULL);

					DestroyMenu(hmenuPopup);

					return bSuccess;
				}
			}
			return FALSE;

		case WM_HELP:
			if (!s_bAboutBox)
			{
				MSGBOXPARAMS mbp = {0};
				mbp.cbSize = sizeof(MSGBOXPARAMS);
				mbp.hwndOwner = hDlg;
				mbp.hInstance = g_hInstance;
				mbp.lpszText = g_szAbout;
				mbp.lpszCaption = g_szTitle;
				mbp.dwStyle = MB_OK | MB_USERICON;
				mbp.lpszIcon = MAKEINTRESOURCE(IDI_GBXDUMP);
				mbp.dwContextHelpId = 0;
				mbp.lpfnMsgBoxCallback = NULL;
				mbp.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

				s_bAboutBox = TRUE;
				PlaySound((LPCTSTR)SND_ALIAS_SYSTEMASTERISK, NULL, SND_ALIAS_ID | SND_ASYNC);
				MessageBoxIndirect(&mbp);
				s_bAboutBox = FALSE;
			}
			return TRUE;

		case WM_INITDIALOG:
			{
				s_bWordWrap = FALSE;
				SetWindowText(hDlg, g_szTitle);

				// Determine dialog box font (see WM_SIZE)
				s_hfontDlgOrig = GetWindowFont(hDlg);
				if (s_hfontDlgOrig == NULL)
					s_hfontDlgOrig = GetStockFont(DEFAULT_GUI_FONT);

				RECT rc;
				// Position thumbnail window
				HWND hwndCtl = GetDlgItem(hDlg, IDC_THUMB);
				GetWindowRect(hwndCtl, &rc);
				ScreenToClient(hDlg, (LPPOINT)&rc.left);
				ScreenToClient(hDlg, (LPPOINT)&rc.right);
				MoveWindow(hwndCtl, rc.left, rc.bottom-(rc.right-rc.left),
					rc.right-rc.left, rc.right-rc.left, FALSE);

				// Set the minimum size of the dialog box
				GetWindowRect(hDlg, &rc);
				s_ptMinTrackSize.x = (rc.right - rc.left) * 3 / 4;
				s_ptMinTrackSize.y = (rc.bottom - rc.top) * 3 / 4;

				// Save size of main window as property
				GetClientRect(hDlg, &rc);
				StoreWindowRect(hDlg, &rc);

				// Create size box bottom right
				int cx = GetSystemMetrics(SM_CXVSCROLL);
				int cy = GetSystemMetrics(SM_CYHSCROLL);
				s_hwndSizeBox = CreateWindow(g_szScrlBar, NULL,
					WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_GROUP |
					SBS_SIZEBOX | SBS_SIZEBOXBOTTOMRIGHTALIGN | SBS_SIZEGRIP,
					rc.right-cx, rc.bottom-cy, cx, cy, hDlg, (HMENU)-1, g_hInstance, NULL);

				// Save size of all child windows as property
				hwndCtl = GetWindow(hDlg, GW_CHILD);
				while (hwndCtl)
				{
					GetWindowRect(hwndCtl, &rc);
					ScreenToClient(hDlg, (LPPOINT)&rc.left);
					ScreenToClient(hDlg, (LPPOINT)&rc.right);
					StoreWindowRect(hwndCtl, &rc);
					hwndCtl = GetNextSibling(hwndCtl);
				}

				// Subclass the text output window
				hwndCtl = GetDlgItem(hDlg, IDC_OUTPUT);
				g_lpPrevOutputWndProc = SubclassWindow(hwndCtl, OutputWndProc);

				// Set the memory limit of the text output window
				Edit_LimitText(hwndCtl, 0);

				// Determine logical DPI
				int nDpi = USER_DEFAULT_SCREEN_DPI;
				HDC hdc = GetDC(hDlg);
				if (hdc != NULL)
				{
					nDpi = GetDeviceCaps(hdc, LOGPIXELSY);
					ReleaseDC(hDlg, hdc);
				}

				// Set font of the text output window dpi-aware
				LOGFONT lf;
				ZeroMemory(&lf, sizeof(lf));
				lf.lfHeight = -MulDiv(10, nDpi, 72);
				lf.lfWeight = FW_NORMAL;
				lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
				lstrcpyn(lf.lfFaceName, g_szCourier, _countof(lf.lfFaceName));

				if (s_hfontEditBox != NULL)
					DeleteFont(s_hfontEditBox);
				s_hfontEditBox = CreateFontIndirect(&lf);
				if (s_hfontEditBox != NULL)
					SetWindowFont(hwndCtl, s_hfontEditBox, FALSE);

				s_szUid[0] = '\0'; s_szEnvi[0] = '\0';
				Button_Enable(GetDlgItem(hDlg, IDC_TMX), FALSE);
				Button_Enable(GetDlgItem(hDlg, IDC_DEDIMANIA), FALSE);

				if ((LPCTSTR)lParam != NULL && ((LPCTSTR)lParam)[0] != TEXT('\0'))
				{ // Open a GBX file passed by program argument
					lstrcpyn(s_szFileName, (LPCTSTR)lParam, _countof(s_szFileName));
					PostMessage(hDlg, WMU_FILEOPEN, 0, lParam);
				}
				else
				{ // Display default text
					s_szFileName[0] = TEXT('\0');
					Edit_ReplaceSel(hwndCtl, g_szAbout);
					if ((GetWindowStyle(hwndCtl) & ES_READONLY) == 0)
						Edit_SetReadOnly(hwndCtl, TRUE);
				}

				// Allow drag-and-drop
				DragAcceptFiles(hDlg, TRUE);
			}
			return TRUE;

		case WM_CLOSE:
			{
				DeleteWindowRect(hDlg);
				HWND hCtl = GetWindow(hDlg, GW_CHILD);
				while (hCtl)
				{
					DeleteWindowRect(hCtl);
					hCtl = GetNextSibling(hCtl);
				}

				if (s_hwndSizeBox != NULL)
				{
					DestroyWindow(s_hwndSizeBox);
					s_hwndSizeBox = NULL;
				}

				hCtl = GetWindow(hDlg, IDC_OUTPUT);
				if (hCtl != NULL && g_lpPrevOutputWndProc != NULL)
					SubclassWindow(hCtl, g_lpPrevOutputWndProc);

				if (s_hfontEditBox != NULL)
				{
					DeleteFont(s_hfontEditBox);
					s_hfontEditBox = NULL;
				}

				if (s_hfontDlgCurr != NULL)
				{
					DeleteFont(s_hfontDlgCurr);
					s_hfontDlgCurr = NULL;
				}

				EndDialog(hDlg, 0);
			}
			return FALSE;
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Subclass of the text output window

LRESULT CALLBACK OutputWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Simplified URL selection
	if (uMsg == WM_LBUTTONDBLCLK && SelectText(hwnd) != 0)
		return FALSE;

	// CTRL+A to select the entire text
	if (uMsg == WM_KEYDOWN && GetKeyState(VK_CONTROL) < 0 && wParam == 'A')
	{
		Edit_SetSel(hwnd, 0, -1);
		return FALSE;
	}

	return CallWindowProc(g_lpPrevOutputWndProc, hwnd, uMsg, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Open and examine the passed file

BOOL DumpFile(HWND hwndCtl, LPCTSTR lpszFileName, LPSTR lpszUid, LPSTR lpszEnvi)
{
	BOOL bRet = FALSE;
	TCHAR szOutput[OUTPUT_LEN];

	if (hwndCtl == NULL || lpszFileName == NULL || lpszFileName[0] == TEXT('\0') ||
		lpszUid == NULL || lpszEnvi == NULL)
		return FALSE;

	lpszUid[0] = '\0';
	lpszEnvi[0] = '\0';

	// Disable the TMX and Dedimania buttons
	HWND hwndButton = GetDlgItem(GetParent(hwndCtl), IDC_TMX);
	if (IsWindowEnabled(hwndButton))
		Button_Enable(hwndButton, FALSE);
	hwndButton = GetDlgItem(GetParent(hwndCtl), IDC_DEDIMANIA);
	if (IsWindowEnabled(hwndButton))
		Button_Enable(hwndButton, FALSE);

	// Release old thumbnail
	if (g_hDibThumb != NULL)
	{
		JpegFreeDib(g_hDibThumb);
		g_hDibThumb = NULL;

		HWND hwndThumb = GetDlgItem(GetParent(hwndCtl), IDC_THUMB);
		if (hwndThumb != NULL)
			InvalidateRect(hwndThumb, NULL, FALSE);
	}

	// Output file name
	LPCTSTR lpsz = _tcsrchr(lpszFileName, TEXT('\\'));
	OutputTextFmt(hwndCtl, szOutput, TEXT("File Name:\t%s\r\n"),
		lpsz != NULL && *(lpsz + 1) != TEXT('\0') ? lpsz + 1 : lpszFileName);

	// Obtain attribute information about the file
	WIN32_FILE_ATTRIBUTE_DATA wfad = {0};
	if (!GetFileAttributesEx(lpszFileName, GetFileExInfoStandard, &wfad))
	{
		OutputErrorMessage(hwndCtl, GetLastError());
		return FALSE;
	}

	// Output file size
	if ((wfad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		OutputText(hwndCtl, TEXT("File Size:\t"));
		if (wfad.nFileSizeHigh > 0)
			OutputText(hwndCtl, TEXT("> 4 GB"));
		else if (FormatByteSize(wfad.nFileSizeLow, szOutput, _countof(szOutput)))
			OutputText(hwndCtl, szOutput);
		OutputText(hwndCtl, TEXT("\r\n"));
	}

	// Open the file
	HANDLE hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		OutputErrorMessage(hwndCtl, GetLastError());
		return FALSE;
	}

	// Read file signature
	BYTE achMagic[9] = {0}; // Large enough for "NadeoFile"
	if (!ReadData(hFile, (LPVOID)&achMagic, 9))
	{
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_MAGIC : IDP_ENG_ERR_MAGIC);
		OutputText(hwndCtl, g_szSep2);
		CloseHandle(hFile);
		return FALSE;
	}

	if (memcmp(achMagic, "GBX", 3) == 0)
	{ // GameBox
		OutputText(hwndCtl, TEXT("File Type:\tGameBox\r\n"));

		if (!(bRet = DumpGbx(hwndCtl, hFile, lpszUid, lpszEnvi)))
			OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_READ : IDP_ENG_ERR_READ);
	}
	else if (memcmp(achMagic, "NadeoPak", 8) == 0) // *.Pack.Gbx- or *.pak files
	{ // NadeoPak
		OutputText(hwndCtl, TEXT("File Type:\tNadeoPak\r\n"));

		if (!(bRet = DumpPack(hwndCtl, hFile)))
			OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_READ : IDP_ENG_ERR_READ);
	}
	else if (memcmp(achMagic, "NadeoFile", 9) == 0) // *.mux file
	{ // NadeoFile
		OutputText(hwndCtl, TEXT("File Type:\tNadeoFile\r\n"));

		if (!(bRet = DumpMux(hwndCtl, hFile)))
			OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_READ : IDP_ENG_ERR_READ);
	}
	else if (memcmp(achMagic, "DDS ", 4) == 0) // The fourth character is a space
	{ // DDS
		OutputText(hwndCtl, TEXT("File Type:\tDirectDraw Surface\r\n"));

		if (!(bRet = DumpDDS(hwndCtl, hFile)))
			OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_READ : IDP_ENG_ERR_READ);
	}
	else
	{ // Unsupported file format
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_MAGIC : IDP_ENG_ERR_MAGIC);

		if (!(bRet = DumpHex(hwndCtl, hFile, wfad.nFileSizeLow)))
			OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_READ : IDP_ENG_ERR_READ);
	}

	OutputText(hwndCtl, g_szSep2);
	CloseHandle(hFile);

	return bRet;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Displays version and salt of a MUX file

BOOL DumpMux(HWND hwndCtl, HANDLE hFile)
{
	TCHAR szOutput[OUTPUT_LEN];

	if (hwndCtl == NULL || hFile == NULL)
		return FALSE;

	// Jump to file version (skip signature)
	if (!FileSeekBegin(hFile, 9))
		return FALSE;

	// Version
	BYTE cVersion = 0;
	if (!ReadNat8(hFile, &cVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Version:\t%d"), (char)cVersion);
	if (cVersion > 1) OutputText(hwndCtl, TEXT("*"));
	OutputText(hwndCtl, TEXT("\r\n"));

	// Salt
	DWORD dwSalt = 0;
	if (!ReadNat32(hFile, &dwSalt))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Salt:\t\t%08X\r\n"), dwSalt);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Displays a hex dump of the first 1024 bytes of a file

#define COLUMNS 16
#define FORMAT_LEN 256

BOOL DumpHex(HWND hwndCtl, HANDLE hFile, SIZE_T cbLen)
{
	TCHAR szFormat[FORMAT_LEN];
	TCHAR szOutput[OUTPUT_LEN];

	if (hwndCtl == NULL || hFile == NULL)
		return FALSE;

	if (cbLen > 1024)
		cbLen = 1024;

	PBYTE pData = (PBYTE)GlobalAllocPtr(GHND, cbLen);
	if (pData == NULL)
		return FALSE;

	if (!FileSeekBegin(hFile, 0) || !ReadData(hFile, pData, cbLen))
	{
		GlobalFreePtr((LPVOID)pData);
		return FALSE;
	}

	OutputText(hwndCtl, g_szSep1);
	if (LoadString(g_hInstance, g_bGerUI ? IDS_GER_HEXDUMP : IDS_ENG_HEXDUMP,
		szOutput, _countof(szOutput)) > 0)
	{ // Keep the printf-style format specifiers under internal control
		_sntprintf(szFormat, _countof(szFormat), TEXT("%Iu"), cbLen);
		szFormat[FORMAT_LEN - 1] = TEXT('\0');

		LPCTSTR lpszText = AllocReplaceString(szOutput, TEXT("{COUNT}"), szFormat);
		if (lpszText != NULL)
		{
			OutputText(hwndCtl, lpszText);
			OutputText(hwndCtl, g_szSep1);
			GlobalFreePtr((LPVOID)lpszText);
		}
	}

	PBYTE pByte;
	SIZE_T i, j, c;
	for (i = 0; i < cbLen; i += COLUMNS)
	{
		c = ((cbLen - i) > COLUMNS) ? COLUMNS : cbLen - i;

		// Address
		_sntprintf(szOutput, _countof(szOutput), TEXT("%04IX| "), i);

		// Hex dump
		for (j = c, pByte = pData + i; j--; pByte++)
		{
			_sntprintf(szFormat, _countof(szFormat), TEXT("%02X "), *pByte);
			szFormat[FORMAT_LEN - 1] = TEXT('\0');
			_tcsncat(szOutput, szFormat, _countof(szOutput) - _tcslen(szOutput) - 4);
		}

		for (j = COLUMNS - c; j--; )
			_tcsncat(szOutput, TEXT("   "), _countof(szOutput) - _tcslen(szOutput) - 4);

		_tcsncat(szOutput, TEXT("|"), _countof(szOutput) - _tcslen(szOutput) - 4);

		// ASCII dump
		for (j = c, pByte = pData + i; j--; pByte++)
		{
			_sntprintf(szFormat, _countof(szFormat), TEXT("%hc"), isprint(*pByte) ? *pByte : '.');
			szFormat[FORMAT_LEN - 1] = TEXT('\0');
			_tcsncat(szOutput, szFormat, _countof(szOutput) - _tcslen(szOutput) - 4);
		}

		for (j = COLUMNS - c; j--; )
			_tcsncat(szOutput, TEXT(" "), _countof(szOutput) - _tcslen(szOutput) - 4);

		_tcsncat(szOutput, TEXT("|\r\n"), _countof(szOutput) - _tcslen(szOutput) - 1);

		OutputText(hwndCtl, szOutput);
	}

	GlobalFreePtr((LPVOID)pData);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Selects the complete URL in which the cursor is located

int SelectText(HWND hwndCtl)
{
	if (hwndCtl == NULL || Edit_GetTextLength(hwndCtl) == 0)
		return 0;

	INT_PTR iStartChar, iEndChar;
	SendMessage(hwndCtl, EM_GETSEL, (WPARAM)&iStartChar, (LPARAM)&iEndChar);

	int nSelLen = (int)(iEndChar - iStartChar);

	if (nSelLen != 0)
		return nSelLen;

	int iLine = Edit_LineFromChar(hwndCtl, -1);
	int nLineLen = Edit_LineLength(hwndCtl, -1);

	if (nLineLen > 1024)
		nLineLen = 1024;

	TCHAR szLine[1024];
	nLineLen = Edit_GetLine(hwndCtl, iLine, szLine, nLineLen);
	szLine[nLineLen] = TEXT('\0');

	if (nLineLen == 0)
		return 0;

	int iChar = Edit_LineIndex(hwndCtl, iLine);
	int iCharInLine = (int)(iStartChar - iChar);

	// Spaces and parentheses are valid characters for URLs,
	// but are required for the selection of regular words
	const TCHAR* pszCutOffChars = TEXT(" \t\"<>()[]{}|#");

	TCHAR* pszLine = szLine;
	pszLine += iCharInLine;

	TCHAR* pszChar = _tcspbrk(pszLine, pszCutOffChars);
	if (pszChar == NULL)
		iEndChar = (INT_PTR)iChar + nLineLen;
	else
		iEndChar = (INT_PTR)iChar + iCharInLine + (pszChar - pszLine);

	szLine[iCharInLine] = TEXT('\0');
	pszLine = _tcsrev(szLine);

	pszChar = _tcspbrk(pszLine, pszCutOffChars);
	if (pszChar == NULL)
		iStartChar = iChar;
	else
		iStartChar = (INT_PTR)iChar + iCharInLine - (pszChar - pszLine);

	nSelLen = (int)(iEndChar - iStartChar);

	if (nSelLen != 0)
		Edit_SetSel(hwndCtl, iStartChar, iEndChar);

	return nSelLen;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Activate or deactivate line break of the edit control

BOOL SetWordWrap(HWND hDlg, BOOL bWordWrap)
{
	if (hDlg == NULL)
		return FALSE;

	HWND hwndEditOld = GetDlgItem(hDlg, IDC_OUTPUT);
	if (hwndEditOld == NULL)
		return FALSE;

	// Remove subclassing
	if (g_lpPrevOutputWndProc != NULL)
		SubclassWindow(hwndEditOld, g_lpPrevOutputWndProc);

	// Save text and status of the edit control
	RECT rcProp;
	RetrieveWindowRect(hwndEditOld, &rcProp);
	HFONT hFont = GetWindowFont(hwndEditOld);
	int nLen = Edit_GetTextLength(hwndEditOld);
	LPTSTR pszSaveText = (LPTSTR)GlobalAllocPtr(GHND, ((SIZE_T)nLen + 1) * sizeof(TCHAR));
	Edit_GetText(hwndEditOld, pszSaveText, nLen+1);
	DWORD dwSelStart = (DWORD)nLen;
	DWORD dwSelEnd = (DWORD)nLen;
	BOOL bHasFocus = (GetFocus() == hwndEditOld);
	SendMessage(hwndEditOld, EM_GETSEL, (WPARAM)&dwSelStart, (LPARAM)&dwSelEnd);

	// Create a new edit control in the desired style
	DWORD dwExStyle = GetWindowExStyle(hwndEditOld);
	DWORD dwStyle = GetWindowStyle(hwndEditOld);
	if (dwExStyle == 0)
		dwExStyle = WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE;
	if (dwStyle == 0)
		dwStyle = WS_CHILD | WS_TABSTOP | WS_GROUP | WS_VSCROLL |
		ES_MULTILINE | ES_NOHIDESEL | ES_READONLY;
	if (bWordWrap)
		dwStyle = dwStyle & ~WS_HSCROLL;
	else
		dwStyle |= WS_HSCROLL;

	RECT rcSize;
	GetWindowRect(hwndEditOld, &rcSize);
	ScreenToClient(hDlg, (LPPOINT)&rcSize);
	ScreenToClient(hDlg, ((LPPOINT)&rcSize)+1);
	UINT nID = GetWindowID(hwndEditOld);

	HWND hwndEditNew = CreateWindowEx(dwExStyle, g_szEdit, NULL, dwStyle,
		rcSize.left, rcSize.top, rcSize.right-rcSize.left, rcSize.bottom-rcSize.top,
		hDlg, (HMENU)(UINT_PTR)nID, g_hInstance, NULL);
	if (hwndEditNew == NULL)
	{
		GlobalFreePtr((LPVOID)pszSaveText);
		return FALSE;
	}

	// To be safe, clear the content of the old edit control
	Edit_SetText(hwndEditOld, NULL);

	StoreWindowRect(hwndEditNew, &rcProp);

	// Restore all visible styles
	Edit_SetText(hwndEditNew, pszSaveText);
	GlobalFreePtr((LPVOID)pszSaveText);

	if (hFont != NULL)
		SetWindowFont(hwndEditNew, hFont, FALSE);

	SetWindowLong(hwndEditOld, GWL_ID, nID+2000);
	SetWindowPos(hwndEditNew, NULL, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);
	SetWindowPos(hwndEditNew, NULL, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_DRAWFRAME);

	// Destroy the old edit control
	SetWindowPos(hwndEditOld, NULL, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW | SWP_HIDEWINDOW);
	DeleteWindowRect(hwndEditOld);
	DestroyWindow(hwndEditOld);

	UpdateWindow(hwndEditNew);

	// Restore the remaining states
	Edit_LimitText(hwndEditNew, 0);
	if (bHasFocus)
		SetFocus(hwndEditNew);
	Edit_SetSel(hwndEditNew, dwSelStart, dwSelEnd);

	// Restore subclassing
	g_lpPrevOutputWndProc = SubclassWindow(hwndEditNew, OutputWndProc);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Create a font that fits the size of the thumbnail window

HFONT CreateScaledFont(HDC hDC, LPCRECT lpRect, LPCTSTR lpszText)
{
	if (hDC == NULL || lpRect == NULL || lpszText == NULL)
		return NULL;

	SIZE_T cchTextLen = _tcslen(lpszText);
	if (cchTextLen == 0)
		return NULL;

	// Define the maximum size of the rectangle in the center
	// of the thumbnail in which the text must be positioned
	SIZE sizeTextMax;
	sizeTextMax.cx = ((lpRect->right - lpRect->left)) * 3 / 4;
	sizeTextMax.cy = ((lpRect->bottom - lpRect->top)) / 6;

	LOGFONT lf;
	ZeroMemory(&lf, sizeof(lf));
	lf.lfHeight = -sizeTextMax.cy; // Set the max practicable font size
	lf.lfWeight = FW_BOLD;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
	lstrcpyn(lf.lfFaceName, g_szArial, _countof(lf.lfFaceName));

	HFONT hFont = CreateFontIndirect(&lf);
	if (hFont == NULL)
		return NULL;

	HFONT hFontOld = SelectFont(hDC, hFont);

	SIZE sizeTextRatio;
	GetTextExtentPoint32(hDC, lpszText, (int)cchTextLen, &sizeTextRatio);

	if (hFontOld != NULL)
		SelectFont(hDC, hFontOld);

	__try
	{
		// If the text has become too wide due to the maximum
		// font height, reduce the font size until it fits in
		if (((sizeTextMax.cy * sizeTextRatio.cx) / sizeTextRatio.cy) > sizeTextMax.cx)
		{
			DeleteFont(hFont);
			hFont = NULL;

			lf.lfHeight = -((sizeTextMax.cx * sizeTextRatio.cy) / sizeTextRatio.cx);
			hFont = CreateFontIndirect(&lf);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		if (hFont != NULL)
		{
			DeleteFont(hFont);
			hFont = NULL;
		}
	}

	return hFont;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Creates a logical color palette from a given DIB

HPALETTE DIBCreatePalette(HANDLE hDIB)
{
	if (hDIB == NULL)
		return NULL;

	HPALETTE hPal = NULL;
	LPSTR lpbi = (LPSTR)GlobalLock((HGLOBAL)hDIB);
	LPBITMAPINFO lpbmi = (LPBITMAPINFO)lpbi;
	if (lpbmi == NULL)
		return NULL;

	UINT uNumColors = DIBNumColors(lpbi);
	if (uNumColors != 0)
	{ // Create a palette from the colors of the DIB
		LPLOGPALETTE lpPal = (LPLOGPALETTE)GlobalAllocPtr(GHND,
			sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * uNumColors);
		if (lpPal == NULL)
		{
			GlobalUnlock((HGLOBAL)hDIB);
			return NULL;
		}

		lpPal->palVersion    = 0x300;
		lpPal->palNumEntries = (WORD)uNumColors;

		for (register UINT i = 0; i < uNumColors; i++)
		{
			lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;
			lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
			lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
			lpPal->palPalEntry[i].peFlags = 0;
		}

		hPal = CreatePalette(lpPal);

		GlobalFreePtr((LPVOID)lpPal);
	}
	else
	{ // Create a halftone palette
		HDC hDC = GetDC(NULL);
		hPal = CreateHalftonePalette(hDC);
		ReleaseDC(NULL, hDC);
	}

	GlobalUnlock((HGLOBAL)hDIB);

	return hPal;
}

////////////////////////////////////////////////////////////////////////////////////////////////

__inline UINT DIBPaletteSize(LPSTR lpbi)
{
	if ((((LPBITMAPINFOHEADER)lpbi)->biSize == sizeof(BITMAPINFOHEADER)) &&
			((((LPBITMAPINFOHEADER)lpbi)->biBitCount == 16) ||
			(((LPBITMAPINFOHEADER)lpbi)->biBitCount == 32)) &&
			((LPBITMAPINFOHEADER)lpbi)->biCompression == BI_BITFIELDS)
		return (3 * sizeof(DWORD));
	else
		return (DIBNumColors(lpbi) * sizeof(RGBQUAD));
}

////////////////////////////////////////////////////////////////////////////////////////////////

__inline UINT DIBNumColors(LPCSTR lpbi)
{
	DWORD dwClrUsed = ((LPBITMAPINFOHEADER)lpbi)->biClrUsed;
	if (dwClrUsed != 0)
		return dwClrUsed;

	WORD wBPP = ((LPBITMAPINFOHEADER)lpbi)->biBitCount;

	if (wBPP <= 8)
		return (1 << wBPP);
	else
		return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Save the size of a window in the property list of the window

void StoreWindowRect(HWND hwnd, LPRECT lprc)
{
	if (hwnd != NULL && lprc != NULL)
	{
		SetProp(hwnd, g_szTop,    (HANDLE)(LONG_PTR)lprc->top);
		SetProp(hwnd, g_szBottom, (HANDLE)(LONG_PTR)lprc->bottom);
		SetProp(hwnd, g_szLeft,   (HANDLE)(LONG_PTR)lprc->left);
		SetProp(hwnd, g_szRight,  (HANDLE)(LONG_PTR)lprc->right);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Retrieve a saved window size from the property list of a window

void RetrieveWindowRect(HWND hwnd, LPRECT lprc)
{
	if (lprc != NULL)
	{
		if (hwnd == NULL)
			SetRectEmpty(lprc);
		else
		{
			lprc->top = (LONG)(LONG_PTR)GetProp(hwnd, g_szTop);
			lprc->bottom = (LONG)(LONG_PTR)GetProp(hwnd, g_szBottom);
			lprc->left = (LONG)(LONG_PTR)GetProp(hwnd, g_szLeft);
			lprc->right = (LONG)(LONG_PTR)GetProp(hwnd, g_szRight);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Remove previously created entries from the property list of a window

void DeleteWindowRect(HWND hwnd)
{
	if (hwnd != NULL)
	{
		RemoveProp(hwnd, g_szTop);
		RemoveProp(hwnd, g_szBottom);
		RemoveProp(hwnd, g_szLeft);
		RemoveProp(hwnd, g_szRight);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
