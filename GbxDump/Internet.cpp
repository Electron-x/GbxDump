////////////////////////////////////////////////////////////////////////////////////////////////
// Internet.cpp - Copyright (c) 2010-2024 by Electron.
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
#include "internet.h"

#define TIMEOUT 10000 // 10 seconds

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module

// Determines whether the system is globally offline
BOOL IsGlobalOffline(HINTERNET hInternet = NULL);
// Retrieves error messages from the WinINet module
void LastInternetError(HWND hwndEdit, DWORD dwError);

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ReadInternetFile(HWND hwndEdit, LPCTSTR lpszUrl, LPSTR lpszData, DWORD dwSize)
{
	if (hwndEdit == NULL || lpszUrl == NULL || lpszData == NULL || dwSize == 0)
		return FALSE;

	HINTERNET hInternet = InternetOpen(g_szUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hInternet == NULL)
	{
		LastInternetError(hwndEdit, GetLastError());
		return FALSE;
	}

	DWORD dwTimeout = TIMEOUT;
	InternetSetOption(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &dwTimeout, sizeof(dwTimeout));
	InternetSetOption(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &dwTimeout, sizeof(dwTimeout));

	HINTERNET hInternetFile = InternetOpenUrl(hInternet, lpszUrl, NULL, 0, 0, 0);
	if (hInternetFile == NULL)
	{
		DWORD dwError = GetLastError();

		if (!IsGlobalOffline(hInternet))
		{
			LastInternetError(hwndEdit, dwError);
			InternetCloseHandle(hInternet);
			return FALSE;
		}

		if (!InternetGoOnline((LPTSTR)lpszUrl, GetParent(hwndEdit), INTERENT_GOONLINE_REFRESH))
		{
			InternetCloseHandle(hInternet);
			return FALSE;
		}

		hInternetFile = InternetOpenUrl(hInternet, lpszUrl, NULL, 0, 0, INTERNET_NO_CALLBACK);
		if (hInternetFile == NULL)
		{
			LastInternetError(hwndEdit, GetLastError());
			InternetCloseHandle(hInternet);
			return FALSE;
		}
	}

	DWORD dwRead = 0;
	DWORD dwReadTotal = 0;
	LPSTR lpsz = lpszData;

	do
	{
		if (!InternetReadFile(hInternetFile, lpsz, dwSize-dwReadTotal-1, &dwRead))
		{
			LastInternetError(hwndEdit, GetLastError());
			InternetCloseHandle(hInternetFile);
			InternetCloseHandle(hInternet);
			return FALSE;
		}

		lpsz += dwRead;
		dwReadTotal += dwRead;

	} while (dwRead != 0);

	lpszData[dwReadTotal] = '\0';

	InternetCloseHandle(hInternetFile);
	InternetCloseHandle(hInternet);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL IsGlobalOffline(HINTERNET hInternet)
{ // NULL is a valid value for HINTERNET
	BOOL bRet = FALSE;
	INTERNET_CONNECTED_INFO ici = {0};
	DWORD dwBufferLength = sizeof(INTERNET_CONNECTED_INFO);

	if (InternetQueryOption(hInternet, INTERNET_OPTION_CONNECTED_STATE, &ici, &dwBufferLength) &&
		(ici.dwConnectedState & INTERNET_STATE_DISCONNECTED_BY_USER))
		bRet = TRUE;

	return bRet;
}

////////////////////////////////////////////////////////////////////////////////////////////////

void LastInternetError(HWND hwndEdit, DWORD dwError)
{
	if (hwndEdit == NULL)
		return;

	LPVOID lpMsgBuf = NULL;

	DWORD dwLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
		FORMAT_MESSAGE_IGNORE_INSERTS, GetModuleHandle(TEXT("wininet.dll")), dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

	if (lpMsgBuf == NULL)
		return;

	if (dwLen == 0)
	{
		LocalFree(lpMsgBuf);
		return;
	}

	int nLen = Edit_GetTextLength(hwndEdit);
	Edit_SetSel(hwndEdit, (WPARAM)nLen, (LPARAM)nLen);
	OutputText(hwndEdit, g_szSep1);
	Edit_ReplaceSel(hwndEdit, lpMsgBuf);

	LocalFree(lpMsgBuf);
}

////////////////////////////////////////////////////////////////////////////////////////////////
