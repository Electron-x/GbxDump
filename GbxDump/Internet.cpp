////////////////////////////////////////////////////////////////////////////////////////////////
// Internet.cpp - Copyright (c) 2010-2018 by Electron.
//
// Licensed under the EUPL, Version 1.2 or – as soon they will be approved by
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
#include "internet.h"

#define TIMEOUT 10000 // 10 Seconds

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module
//
BOOL IsGlobalOffline(HINTERNET hInternet = NULL);
void LastInternetError(HWND hwndCtl, DWORD dwError);

////////////////////////////////////////////////////////////////////////////////////////////////
// String Constants
//
const TCHAR g_szUserAgent[]  = TEXT("GbxDump/1.60");
const TCHAR g_szWininetDll[] = TEXT("wininet.dll");
const TCHAR g_szConnecting[] = TEXT("%s - Connecting...");
const TCHAR g_szDownload[]   = TEXT("%s - Downloading...");

////////////////////////////////////////////////////////////////////////////////////////////////
// Retrieves data from an Internet address

BOOL ReadInternetFile(HWND hwndCtl, LPCTSTR lpszUrl, LPSTR lpszData, DWORD dwSize)
{
	if (hwndCtl == NULL || lpszUrl == NULL || lpszData == NULL || dwSize == 0)
		return FALSE;

	TCHAR szTitle[256];
	HWND hwndDlg = GetParent(hwndCtl);

	HINTERNET hInternet = InternetOpen(g_szUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hInternet == NULL)
	{
		LastInternetError(hwndCtl, GetLastError());
		return FALSE;
	}

	DWORD dwTimeout = TIMEOUT;
	InternetSetOption(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &dwTimeout, sizeof(dwTimeout));
	InternetSetOption(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &dwTimeout, sizeof(dwTimeout));

	_sntprintf(szTitle, _countof(szTitle), g_szConnecting, g_szTitle);
	SetWindowText(hwndDlg, szTitle);

	HINTERNET hInternetFile = InternetOpenUrl(hInternet, lpszUrl, NULL, 0, 0, 0);
	if (hInternetFile == NULL)
	{
		DWORD dwError = GetLastError();

		if (!IsGlobalOffline(hInternet))
		{
			LastInternetError(hwndCtl, dwError);
			SetWindowText(hwndDlg, g_szTitle);
			InternetCloseHandle(hInternet);
			return FALSE;
		}

		if (!InternetGoOnline((LPTSTR)lpszUrl, GetParent(hwndCtl), INTERENT_GOONLINE_REFRESH))
		{
			SetWindowText(hwndDlg, g_szTitle);
			InternetCloseHandle(hInternet);
			return FALSE;
		}

		hInternetFile = InternetOpenUrl(hInternet, lpszUrl, NULL, 0, 0, 0);
		if (hInternetFile == NULL)
		{
			LastInternetError(hwndCtl, GetLastError());
			SetWindowText(hwndDlg, g_szTitle);
			InternetCloseHandle(hInternet);
			return FALSE;
		}
	}

	_sntprintf(szTitle, _countof(szTitle), g_szDownload, g_szTitle);
	SetWindowText(hwndDlg, szTitle);
	
	DWORD dwRead = 0;
	DWORD dwReadTotal = 0;
	LPSTR lpsz = lpszData;

	do
	{
		if (!InternetReadFile(hInternetFile, lpsz, dwSize-dwReadTotal-1, &dwRead))
		{
			LastInternetError(hwndCtl, GetLastError());
			SetWindowText(hwndDlg, g_szTitle);
			InternetCloseHandle(hInternetFile);
			InternetCloseHandle(hInternet);
			return FALSE;
		}

		lpsz += dwRead;
		dwReadTotal += dwRead;

	} while (dwRead != 0);

	lpszData[dwReadTotal] = '\0';

	SetWindowText(hwndDlg, g_szTitle);
	InternetCloseHandle(hInternetFile);
	InternetCloseHandle(hInternet);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Determines whether the system is globally offline

BOOL IsGlobalOffline(HINTERNET hInternet)
{
	BOOL bRet = FALSE;
	INTERNET_CONNECTED_INFO ici = {0};
	DWORD dwBufferLength = sizeof(INTERNET_CONNECTED_INFO);

	if (InternetQueryOption(hInternet, INTERNET_OPTION_CONNECTED_STATE, &ici, &dwBufferLength) &&
		(ici.dwConnectedState & INTERNET_STATE_DISCONNECTED_BY_USER))
		bRet = TRUE;

	return bRet;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Retrieves error messages from the WinINet module

void LastInternetError(HWND hwndCtl, DWORD dwError)
{
	LPVOID lpMsgBuf = NULL;
	TCHAR szOutput[OUTPUT_LEN];

	DWORD dwLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
		FORMAT_MESSAGE_IGNORE_INSERTS, GetModuleHandle(g_szWininetDll), dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

	if (dwLen > 0 && lpMsgBuf != NULL)
	{
		_tcsncpy(szOutput, (LPTSTR)lpMsgBuf, _countof(szOutput));
		if (hwndCtl != NULL)
		{
			int nLen = Edit_GetTextLength(hwndCtl);
			Edit_SetSel(hwndCtl, (WPARAM)nLen, (LPARAM)nLen);
			Edit_ReplaceSel(hwndCtl, szOutput);
		}
		LocalFree(lpMsgBuf);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
