////////////////////////////////////////////////////////////////////////////////////////////////
// Dedimania.cpp - Copyright (c) 2010-2023 by Electron.
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
#include "dedimania.h"

#define DEDI_MAX_DATASIZE 8192

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module
//
BOOL PrintDedimaniaData(HWND hwndCtl, LPCSTR lpszUid, BOOL bIsManiaPlanet, LPBOOL lpbTrackFound);
BOOL ConvertDediString(LPVOID lpData, SIZE_T cbLenData, LPTSTR lpszOutput, SIZE_T cchLenOutput);

////////////////////////////////////////////////////////////////////////////////////////////////
// String Constants
//
const char g_szTm1Envis[] = "Stadium,Coast,Bay,Island,Desert,Rally,Snow,Alpine,Speed";

const TCHAR g_szDedimania[] = TEXT("Dedimania Records System:\r\n");
const TCHAR g_szUrlDedi[]   = TEXT("http://dedimania.net:%u/%s?uid=%hs");
const TCHAR g_szErrOom[]    = TEXT("Out of memory.\r\n");

////////////////////////////////////////////////////////////////////////////////////////////////
// strtok replacement that also supports empty tokens
// Source: http://codepad.org/JO8LWGdW
//
char *estrtok(char *str, const char *delim)
{
	static char *last = NULL;
	char *ret;

	if (str)
		last = str;
	else
	{
		if (!last)
			return NULL;
		last++;
	}

	ret = last;

	for (;;)
	{
		if (*last == 0)
		{
			last = NULL;
			return ret;
		}

		if (strchr(delim, *last))
		{
			*last = 0;
			return ret;
		}
		last++;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL GetDedimaniaData(HWND hwndCtl, LPCSTR lpszUid, LPCSTR lpszEnvi)
{
	if (hwndCtl == NULL || lpszUid == NULL || lpszEnvi == NULL)
		return FALSE;

	HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	OutputText(hwndCtl, g_szDedimania);

	BOOL bTrackFound = FALSE;

	// Does the environment belong to TrackMania 1 or ManiaPlanet?
	BOOL bIsTwice = FALSE;
	BOOL bIsManiaPlanet = TRUE;
	if (strcmp("Stadium", lpszEnvi) == 0)
		bIsTwice = TRUE;
	else if (strstr(g_szTm1Envis, lpszEnvi) != NULL)
		bIsManiaPlanet = FALSE;

	// Retrieve and display the data
	BOOL bSuccess = PrintDedimaniaData(hwndCtl, lpszUid, bIsManiaPlanet, &bTrackFound);
	if (bSuccess && !bTrackFound && bIsTwice)
		bSuccess = PrintDedimaniaData(hwndCtl, lpszUid, !bIsManiaPlanet, &bTrackFound);

	if (bSuccess && !bTrackFound)
	{ // Track not found
		TCHAR szText[MAX_PATH];
		if (LoadString(g_hInstance, g_bGerUI ? IDP_GER_ERR_TRACK : IDP_ENG_ERR_TRACK, szText, _countof(szText)) > 0)
		{
			OutputText(hwndCtl, g_szSep1);
			OutputText(hwndCtl, szText);
		}
	}

	OutputText(hwndCtl, g_szSep2);
	SetCursor(hOldCursor);

	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL PrintDedimaniaData(HWND hwndCtl, LPCSTR lpszUid, BOOL bIsManiaPlanet, LPBOOL lpbTrackFound)
{
	if (hwndCtl == NULL || lpszUid == NULL || lpbTrackFound == NULL)
		return FALSE;

	TCHAR szOutput[OUTPUT_LEN];
	*lpbTrackFound = FALSE;

	// Allocate memory for the Dedimania data
	LPSTR lpszData = (LPSTR)GlobalAllocPtr(GHND, DEDI_MAX_DATASIZE);
	if (lpszData == NULL)
	{
		OutputText(hwndCtl, g_szSep1);
		OutputText(hwndCtl, g_szErrOom);
		return FALSE;
	}

	// Build the URL for querying the data
	TCHAR szDediUrl[512];
	_sntprintf(szDediUrl, _countof(szDediUrl), g_szUrlDedi, bIsManiaPlanet ? 8080 : 8000,
		bIsManiaPlanet ? TEXT("MX") : TEXT("TMX"), lpszUid);
	if (!ReadInternetFile(hwndCtl, szDediUrl, lpszData, DEDI_MAX_DATASIZE))
	{
		GlobalFreePtr((LPVOID)lpszData);
		return FALSE;
	}

	// Have user data been found?
	if (lpszData[0] == '\0' || strchr(lpszData, ',') == NULL)
	{
		GlobalFreePtr((LPVOID)lpszData);
		return TRUE;
	}

	*lpbTrackFound = TRUE;

	// Place the cursor at the end of the line
	int nLen = Edit_GetTextLength(hwndCtl);
	Edit_SetSel(hwndCtl, (WPARAM)nLen, (LPARAM)nLen);
	OutputText(hwndCtl, g_szSep1);

	// Skip first line (track info)
	LPSTR lpsz = lpszData;
	LPSTR lpszNewLine = strstr(lpsz, "\r\n");
	if (lpszNewLine != NULL)
	{
		*lpszNewLine = '\0';
		lpsz = lpszNewLine + 2;
	}
	else
	{
		GlobalFreePtr((LPVOID)lpszData);
		return TRUE;
	}

	// Check for existing records
	if (*lpsz == '\0')
	{
		TCHAR szText[MAX_PATH];
		if (LoadString(g_hInstance, g_bGerUI ? IDP_GER_ERR_RECORDS : IDP_ENG_ERR_RECORDS, szText, _countof(szText)) > 0)
			OutputText(hwndCtl, szText);
	}

	UINT uRecords = 0;

	// Parse and output list of records
	while (lpsz != NULL)
	{
		uRecords++;

		// Set pointer to next line. Ignore last (empty) row.
		lpszNewLine = strstr(lpsz, "\r\n");
		if (lpszNewLine != NULL)
		{
			*lpszNewLine = '\0';
			lpszNewLine += 2;
		}
		else
			break;

		int i = 0;
		LPSTR token = estrtok(lpsz, ",");
		while (token != NULL)
		{
			i++;
			switch (i)
			{
				case 1: // Time (ms)
					{
						OutputTextFmt(hwndCtl, szOutput, TEXT("%02u. "), uRecords);
						int nTime = atoi(token);
						if (nTime < 3600000)
						{
							int nMinute   = (nTime / 60000);
							int nSecond   = (nTime % 60000 / 1000);
							int nMilliSec = (nTime % 60000 % 1000);

							OutputTextFmt(hwndCtl, szOutput, TEXT("%d:%02d.%003d, "), nMinute, nSecond, nMilliSec);
						}
						else
						{
							int nHour     = (nTime / 3600000);
							int nMinute   = (nTime % 3600000 / 60000);
							int nSecond   = (nTime % 3600000 % 60000 / 1000);
							int nMilliSec = (nTime % 3600000 % 60000 % 1000);

							OutputTextFmt(hwndCtl, szOutput, TEXT("%d:%02d:%02d.%003d, "), nHour, nMinute, nSecond, nMilliSec);
						}
					}
					break;
				case 2:
					{
						int nGamemode = atoi(token);
						if (bIsManiaPlanet)
						{
							switch (nGamemode)
							{
								case 2: OutputText(hwndCtl, TEXT("TA,  ")); break;
								case 1: OutputText(hwndCtl, TEXT("Rnd, ")); break;
								case 4: OutputText(hwndCtl, TEXT("Laps, ")); break;
								case 3: OutputText(hwndCtl, TEXT("Team, ")); break;
								case 5: OutputText(hwndCtl, TEXT("Cup, ")); break;
								case 0: OutputText(hwndCtl, TEXT("Script, ")); break;
								case 6: OutputText(hwndCtl, TEXT("Stunts, ")); break;
								default: OutputText(hwndCtl, TEXT("?, "));
							}
						}
						else
						{
							if (nGamemode == 1)
								OutputText(hwndCtl, TEXT("TA,  "));
							else if (nGamemode == 0)
								OutputText(hwndCtl, TEXT("Rnd, "));
							else
								OutputText(hwndCtl, TEXT("?,   "));
						}
					}
					break;
				/*
				case 4: // Login
					ConvertDediString(token, strlen(token), szOutput, _countof(szOutput));
					OutputText(hwndCtl, szOutput);
					OutputText(hwndCtl, TEXT(", "));
					break;
				*/
				case 6: // Nick name
					ConvertDediString(token, strlen(token), szOutput, _countof(szOutput));
					OutputText(hwndCtl, szOutput);
					OutputText(hwndCtl, TEXT(", "));
					break;
				case 7: // Server name
					ConvertDediString(token, strlen(token), szOutput, _countof(szOutput));
					OutputText(hwndCtl, szOutput);
					OutputText(hwndCtl, TEXT("\r\n"));
					break;
			}
			token = estrtok(NULL, ",");
		}
		lpsz = lpszNewLine;
	}

	// Release memory
	GlobalFreePtr((LPVOID)lpszData);
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ConvertDediString(LPVOID lpData, SIZE_T cbLenData, LPTSTR lpszOutput, SIZE_T cchLenOutput)
{
	if (lpData == NULL || lpszOutput == NULL || cchLenOutput < cbLenData)
		return FALSE;

	UINT uCodePage = CP_ACP;
	if (IsValidCodePage(CP_UTF8))
		uCodePage = CP_UTF8;

	MultiByteToWideChar(uCodePage, 0, (LPCSTR)lpData, -1, lpszOutput, (int)cchLenOutput);

	SIZE_T cchLen = _tcslen(lpszOutput);
	LPTSTR lpszTemp = (LPTSTR)GlobalAllocPtr(GHND, cchLen * sizeof(TCHAR));
	if (lpszTemp != NULL)
	{
		if (CleanupString(lpszOutput, lpszTemp, cchLen))
			lstrcpyn(lpszOutput, lpszTemp, (int)cchLenOutput);
		GlobalFreePtr((LPVOID)lpszTemp);
	}

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
