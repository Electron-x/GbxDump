////////////////////////////////////////////////////////////////////////////////////////////////
// Dedimania.cpp - Copyright (c) 2010-2024 by Electron.
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
#include "Internet.h"
#include "Dedimania.h"

#define DEDI_MAX_DATASIZE 8192

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module

// Request, parse and output data from Dedimania
BOOL RequestAndOutputDediData(HWND hwndEdit, LPCSTR lpszUid, BOOL bIsManiaPlanet, LPBOOL lpbTrackFound);
// Converts a UTF-8 string to Unicode and removes the Nadeo formatting characters
BOOL ConvertDediString(LPVOID lpData, SIZE_T cbLenData, LPTSTR lpszOutput, SIZE_T cchLenOutput);

////////////////////////////////////////////////////////////////////////////////////////////////
// String Constants

const char g_szTm1Envis[] = "Stadium,Coast,Bay,Island,Desert,Rally,Snow,Alpine,Speed";

const TCHAR g_szDedimania[] = TEXT("Dedimania Records System:\r\n");
const TCHAR g_szUrlDedi[]   = TEXT("%s://dedimania.net:%u/%s?uid=%hs");
const TCHAR g_szErrOom[]    = TEXT("Out of memory.\r\n");

////////////////////////////////////////////////////////////////////////////////////////////////

// strtok replacement that also supports empty tokens
static char *estrtok(char *str, const char *delim)
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

BOOL DumpDedimania(HWND hwndEdit, LPCSTR lpszUid, LPCSTR lpszEnvi)
{
	if (hwndEdit == NULL || lpszUid == NULL || lpszEnvi == NULL)
		return FALSE;

	HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	OutputText(hwndEdit, g_szDedimania);

	BOOL bTrackFound = FALSE;

	// Does the environment belong to TrackMania 1 or ManiaPlanet?
	BOOL bIsTwice = FALSE;
	BOOL bIsManiaPlanet = TRUE;
	if (strcmp("Stadium", lpszEnvi) == 0)
		bIsTwice = TRUE;
	else if (strstr(g_szTm1Envis, lpszEnvi) != NULL)
		bIsManiaPlanet = FALSE;

	// Retrieve and display the data
	BOOL bSuccess = RequestAndOutputDediData(hwndEdit, lpszUid, bIsManiaPlanet, &bTrackFound);
	if (bSuccess && !bTrackFound && bIsTwice)
		bSuccess = RequestAndOutputDediData(hwndEdit, lpszUid, !bIsManiaPlanet, &bTrackFound);

	if (bSuccess && !bTrackFound)
	{ // Track not found
		OutputTextErr(hwndEdit, g_bGerUI ? IDP_GER_ERR_TRACK : IDP_ENG_ERR_TRACK);
	}

	OutputText(hwndEdit, g_szSep2);
	SetCursor(hOldCursor);

	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL RequestAndOutputDediData(HWND hwndEdit, LPCSTR lpszUid, BOOL bIsManiaPlanet, LPBOOL lpbTrackFound)
{
	if (hwndEdit == NULL || lpszUid == NULL || lpbTrackFound == NULL)
		return FALSE;

	TCHAR szOutput[OUTPUT_LEN];
	*lpbTrackFound = FALSE;

	// Allocate memory for the Dedimania data
	LPSTR lpszData = (LPSTR)MyGlobalAllocPtr(GHND, DEDI_MAX_DATASIZE);
	if (lpszData == NULL)
	{
		OutputText(hwndEdit, g_szSep1);
		OutputText(hwndEdit, g_szErrOom);
		return FALSE;
	}

	// Build the URL for querying the data
	TCHAR szDediUrl[512];
	_sntprintf(szDediUrl, _countof(szDediUrl), g_szUrlDedi, TEXT("http"),
		bIsManiaPlanet ? 8080 : 8000, bIsManiaPlanet ? TEXT("MX") : TEXT("TMX"), lpszUid);
	if (!ReadInternetFile(hwndEdit, szDediUrl, lpszData, DEDI_MAX_DATASIZE))
	{
		MyGlobalFreePtr((LPVOID)lpszData);
		return FALSE;
	}

	// Have user data been found?
	if (lpszData[0] == '\0' || strchr(lpszData, ',') == NULL)
	{
		MyGlobalFreePtr((LPVOID)lpszData);
		return TRUE;
	}

	*lpbTrackFound = TRUE;

	// Place the cursor at the end of the line
	int nLen = Edit_GetTextLength(hwndEdit);
	Edit_SetSel(hwndEdit, (WPARAM)nLen, (LPARAM)nLen);
	OutputText(hwndEdit, g_szSep1);

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
		MyGlobalFreePtr((LPVOID)lpszData);
		return TRUE;
	}

	// Check for existing records
	if (*lpsz == '\0')
	{
		TCHAR szText[OUTPUT_LEN];
		if (LoadString(g_hInstance, g_bGerUI ? IDP_GER_ERR_RECORDS : IDP_ENG_ERR_RECORDS, szText, _countof(szText)) > 0)
			OutputText(hwndEdit, szText);
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
						OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("%02u. "), uRecords);
						int nTime = atoi(token);
						if (nTime < 3600000)
						{
							int nMinute   = (nTime / 60000);
							int nSecond   = (nTime % 60000 / 1000);
							int nMilliSec = (nTime % 60000 % 1000);

							OutputTextFmt(hwndEdit, szOutput, _countof(szOutput),
								TEXT("%d:%02d.%003d, "), nMinute, nSecond, nMilliSec);
						}
						else
						{
							int nHour     = (nTime / 3600000);
							int nMinute   = (nTime % 3600000 / 60000);
							int nSecond   = (nTime % 3600000 % 60000 / 1000);
							int nMilliSec = (nTime % 3600000 % 60000 % 1000);

							OutputTextFmt(hwndEdit, szOutput, _countof(szOutput),
								TEXT("%d:%02d:%02d.%003d, "), nHour, nMinute, nSecond, nMilliSec);
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
								case 2: OutputText(hwndEdit, TEXT("TA,  ")); break;
								case 1: OutputText(hwndEdit, TEXT("Rnd, ")); break;
								case 4: OutputText(hwndEdit, TEXT("Laps, ")); break;
								case 3: OutputText(hwndEdit, TEXT("Team, ")); break;
								case 5: OutputText(hwndEdit, TEXT("Cup, ")); break;
								case 0: OutputText(hwndEdit, TEXT("Script, ")); break;
								case 6: OutputText(hwndEdit, TEXT("Stunts, ")); break;
								default: OutputText(hwndEdit, TEXT("?, "));
							}
						}
						else
						{
							if (nGamemode == 1)
								OutputText(hwndEdit, TEXT("TA,  "));
							else if (nGamemode == 0)
								OutputText(hwndEdit, TEXT("Rnd, "));
							else
								OutputText(hwndEdit, TEXT("?,   "));
						}
					}
					break;
				/*
				case 4: // Login
					ConvertDediString(token, strlen(token), szOutput, _countof(szOutput));
					OutputText(hwndEdit, szOutput);
					OutputText(hwndEdit, TEXT(", "));
					break;
				*/
				case 6: // Nick name
					ConvertDediString(token, strlen(token), szOutput, _countof(szOutput));
					OutputText(hwndEdit, szOutput);
					OutputText(hwndEdit, TEXT(", "));
					break;
				case 7: // Server name
					ConvertDediString(token, strlen(token), szOutput, _countof(szOutput));
					OutputText(hwndEdit, szOutput);
					OutputText(hwndEdit, TEXT("\r\n"));
					break;
			}
			token = estrtok(NULL, ",");
		}
		lpsz = lpszNewLine;
	}

	// Release memory
	MyGlobalFreePtr((LPVOID)lpszData);
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
	LPTSTR lpszTemp = (LPTSTR)MyGlobalAllocPtr(GHND, cchLen * sizeof(TCHAR));
	if (lpszTemp != NULL)
	{
		if (CleanupString(lpszOutput, lpszTemp, cchLen))
			MyStrNCpy(lpszOutput, lpszTemp, (int)cchLenOutput);
		MyGlobalFreePtr((LPVOID)lpszTemp);
	}

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
