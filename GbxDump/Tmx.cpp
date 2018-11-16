////////////////////////////////////////////////////////////////////////////////////////////////
// Tmx.cpp - Copyright (c) 2010-2018 by Electron.
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
//
#include "stdafx.h"
#include "internet.h"
#include "tmx.h"

#define GAME_TMNF 1
#define GAME_TMU  2
#define GAME_TMN  3
#define GAME_TMS  4
#define GAME_TMO  5
#define GAME_TM2  6
#define GAME_SM   7
#define GAME_QM   8

#define TMX_MAX_DATASIZE 16384

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module
//
BOOL PrintTmxData(HWND hwndCtl, LPCSTR lpszUid, int nGame, PBOOL pbTrackFound);
BOOL PrintMxData(HWND hwndCtl, LPCSTR lpszUid, int nGame, PBOOL pbTrackFound);
BOOL GetReplayData(UINT uCodePage, LPSTR* lpszData, LPCSTR lpszMarker,
	LPTSTR lpszOutput, SIZE_T cchLenOutput);
BOOL OutputXmlData(HWND hwndCtl, UINT uCodePage, LPCSTR lpszData, LPCSTR lpszMarker,
	LPCTSTR lpszText, LPTSTR lpszValue = NULL, SIZE_T cchValueLen = 0);

////////////////////////////////////////////////////////////////////////////////////////////////
// String Constants
//
const TCHAR g_szTmx[]         = TEXT("Track-/Mania Exchange:\r\n");
const TCHAR g_szTrackMania[]  = TEXT("tm");
const TCHAR g_szShootMania[]  = TEXT("sm");
const TCHAR g_szQuestMania[]  = TEXT("qm");
const TCHAR g_szTracks[]      = TEXT("tracks");
const TCHAR g_szMaps[]        = TEXT("maps");
const TCHAR g_szForever[]     = TEXT("tmnforever");
const TCHAR g_szUnited[]      = TEXT("united");
const TCHAR g_szNations[]     = TEXT("nations");
const TCHAR g_szSunrise[]     = TEXT("sunrise");
const TCHAR g_szOriginal[]    = TEXT("original");
const TCHAR g_szParamInfo[]   = TEXT("apitrackinfo&uid");
const TCHAR g_szParamSearch[] = TEXT("apisearch&trackid");
const TCHAR g_szParamRecord[] = TEXT("apitrackrecords&id");
const TCHAR g_szUrlTmx[]      = TEXT("http://%s.tm-exchange.com/apiget.aspx?action=%s=%hs");
const TCHAR g_szUrlMx1[]      = TEXT("http://api.mania-exchange.com/%s/%s/%hs?format=xml");
const TCHAR g_szUrlMx2[]      = TEXT("http://api.mania-exchange.com/%s/replays/%s/10?format=xml");
const TCHAR g_szErrOom[]      = TEXT("Out of memory.\r\n");

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL GetTmxData(HWND hwndCtl, LPCSTR lpszUid, LPCSTR lpszEnvi)
{
	if (hwndCtl == NULL || lpszUid == NULL || lpszEnvi == NULL)
		return FALSE;

	HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	OutputText(hwndCtl, g_szTmx);
	OutputText(hwndCtl, g_szSep1);

	BOOL bSuccess = TRUE;
	BOOL bTrackFound = FALSE;

	// Specify the T/MX subdomain according to the environment
	if ((strcmp(lpszEnvi, "Canyon") == 0) || (strcmp(lpszEnvi, "Valley") == 0) ||
		(strcmp(lpszEnvi, "Lagoon") == 0) || (strcmp(lpszEnvi, "Arena") == 0))
		bSuccess = PrintMxData(hwndCtl, lpszUid, GAME_TM2, &bTrackFound);
	else if ((strcmp(lpszEnvi, "Storm") == 0) || (strcmp(lpszEnvi, "Cryo") == 0) || (strcmp(lpszEnvi, "Meteor") == 0) ||
		(strcmp(lpszEnvi, "Paris") == 0) || (strcmp(lpszEnvi, "Gothic") == 0))
		bSuccess = PrintMxData(hwndCtl, lpszUid, GAME_SM, &bTrackFound);
	else if ((strcmp(lpszEnvi, "Galaxy") == 0) || (strcmp(lpszEnvi, "History") == 0) ||
		(strcmp(lpszEnvi, "Society") == 0) || (strcmp(lpszEnvi, "Future") == 0))
		bSuccess = PrintMxData(hwndCtl, lpszUid, GAME_QM, &bTrackFound);
	else if (strcmp(lpszEnvi, "Stadium") == 0)
	{
		bSuccess = PrintMxData(hwndCtl, lpszUid, GAME_TM2, &bTrackFound);
		if (bSuccess && bTrackFound == FALSE)
		{
			bSuccess = PrintTmxData(hwndCtl, lpszUid, GAME_TMNF, &bTrackFound);
			if (bSuccess && bTrackFound == FALSE)
			{
				bSuccess = PrintTmxData(hwndCtl, lpszUid, GAME_TMU, &bTrackFound);
				if (bSuccess && bTrackFound == FALSE)
					bSuccess = PrintTmxData(hwndCtl, lpszUid, GAME_TMN, &bTrackFound);
			}
		}
	}
	else if ((strcmp(lpszEnvi, "Coast") == 0) || (strcmp(lpszEnvi, "Bay") == 0) || (strcmp(lpszEnvi, "Island") == 0))
	{
		bSuccess = PrintTmxData(hwndCtl, lpszUid, GAME_TMU, &bTrackFound);
		if (bSuccess && bTrackFound == FALSE)
			bSuccess = PrintTmxData(hwndCtl, lpszUid, GAME_TMS, &bTrackFound);
	}
	else if ((strcmp(lpszEnvi, "Desert") == 0) || (strcmp(lpszEnvi, "Snow") == 0) || (strcmp(lpszEnvi, "Rally") == 0) ||
		(strcmp(lpszEnvi, "Alpine") == 0) || (strcmp(lpszEnvi, "Speed") == 0))
	{
		bSuccess = PrintTmxData(hwndCtl, lpszUid, GAME_TMU, &bTrackFound);
		if (bSuccess && bTrackFound == FALSE)
			PrintTmxData(hwndCtl, lpszUid, GAME_TMO, &bTrackFound);
	}

	if (bSuccess && !bTrackFound)
	{ // Map not found
		TCHAR szText[MAX_PATH];
		if (LoadString(g_hInstance, g_bGerUI ? IDP_GER_ERR_TRACK : IDP_ENG_ERR_TRACK, szText, _countof(szText)) > 0)
			OutputText(hwndCtl, szText);
	}

	OutputText(hwndCtl, g_szSep2);
	SetCursor(hOldCursor);

	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL PrintTmxData(HWND hwndCtl, LPCSTR lpszUid, int nGame, PBOOL pbTrackFound)
{
	if (hwndCtl == NULL || lpszUid == NULL || pbTrackFound == NULL)
		return FALSE;

	TCHAR szOutput[OUTPUT_LEN];
	*pbTrackFound = FALSE;

	UINT uCodePage = CP_ACP;
	if (IsValidCodePage(CP_UTF8))
		uCodePage = CP_UTF8;

	// Subdomain
	TCHAR szSubDomain[16];
	switch (nGame)
	{
		case GAME_TMNF:
			_tcsncpy(szSubDomain, g_szForever, _countof(szSubDomain));
			break;
		case GAME_TMU:
			_tcsncpy(szSubDomain, g_szUnited, _countof(szSubDomain));
			break;
		case GAME_TMN:
			_tcsncpy(szSubDomain, g_szNations, _countof(szSubDomain));
			break;
		case GAME_TMS:
			_tcsncpy(szSubDomain, g_szSunrise, _countof(szSubDomain));
			break;
		case GAME_TMO:
			_tcsncpy(szSubDomain, g_szOriginal, _countof(szSubDomain));
			break;
		default:
			return TRUE;
	}

	// Allocate memory for the TMX data
	DWORD dwSize = TMX_MAX_DATASIZE;
	LPSTR lpszData = (LPSTR)GlobalAllocPtr(GHND, dwSize);
	if (lpszData == NULL)
	{
		OutputText(hwndCtl, g_szErrOom);
		return FALSE;
	}

	// Create query URL and retrieve data via Track UID
	TCHAR szTmxUrl[512];
	_sntprintf(szTmxUrl, _countof(szTmxUrl), g_szUrlTmx, szSubDomain, g_szParamInfo, lpszUid);
	if (!ReadInternetFile(hwndCtl, szTmxUrl, lpszData, dwSize))
	{
		GlobalFreePtr((HGLOBAL)lpszData);
		return FALSE;
	}

	if (lpszData[0] == '\0' || strchr(lpszData, '\t') == NULL)
	{ // No valid track data available
		GlobalFreePtr((HGLOBAL)lpszData);
		return TRUE;
	}

	*pbTrackFound = TRUE;

	BOOL bFormatTime = TRUE;

	// Initialize TMX ID and track version for string comparison
	char szVersion[32];
	szVersion[0] = '\0';
	char szTrackId[32];
	strncpy(szTrackId, "0", _countof(szTrackId));

	// Place the cursor at the end of the edit control
	int nLen = Edit_GetTextLength(hwndCtl);
	Edit_SetSel(hwndCtl, (WPARAM)nLen, (LPARAM)nLen);

	// Parse and output the TMX data
	int i = 0;
	LPSTR lpsz = lpszData;
	LPSTR token = strtok(lpsz, "\t");
	while (token != NULL)
	{
		i++;
		switch (i)
		{
			case 1:
				strncpy(szTrackId, token, _countof(szTrackId));
				OutputTextFmt(hwndCtl, szOutput, TEXT("Track ID:\t%hs\r\n"), token);
				break;
			case 2:
				OutputText(hwndCtl, TEXT("Name:\t\t"));
				MultiByteToWideChar(uCodePage, 0, token, -1, szOutput, _countof(szOutput)-1);
				OutputText(hwndCtl, szOutput);
				OutputText(hwndCtl, TEXT("\r\n"));
				break;
			case 4:
				OutputText(hwndCtl, TEXT("Author:\t\t"));
				MultiByteToWideChar(uCodePage, 0, token, -1, szOutput, _countof(szOutput)-1);
				OutputText(hwndCtl, szOutput);
				OutputText(hwndCtl, TEXT("\r\n"));
				break;
			case 5:
				strncpy(szVersion, token, _countof(szVersion));
				OutputTextFmt(hwndCtl, szOutput, TEXT("Uploaded:\t%hs\r\n"), token);
				break;
			case 6:
				strncpy(szVersion, token, _countof(szVersion));
				OutputTextFmt(hwndCtl, szOutput, TEXT("Updated:\t%hs\r\n"), token);
				break;
			case 7:
				OutputTextFmt(hwndCtl, szOutput, TEXT("Visible:\t%hs\r\n"), token);
				break;
			case 8:
				OutputTextFmt(hwndCtl, szOutput, TEXT("Type:\t\t%hs\r\n"), token);
				if (strcmp(token, "Stunts") == 0)
					bFormatTime = FALSE;
				break;
			case 9:
				OutputTextFmt(hwndCtl, szOutput, TEXT("Environment:\t%hs\r\n"), token);
				break;
			case 10:
				OutputTextFmt(hwndCtl, szOutput, TEXT("Mood:\t\t%hs\r\n"), token);
				break;
			case 11:
				OutputTextFmt(hwndCtl, szOutput, TEXT("Style:\t\t%hs\r\n"), token);
				break;
			case 12:
				OutputTextFmt(hwndCtl, szOutput, TEXT("Routes:\t\t%hs\r\n"), token);
				break;
			case 13:
				OutputTextFmt(hwndCtl, szOutput, TEXT("Length:\t\t%hs\r\n"), token);
				break;
			case 14:
				OutputTextFmt(hwndCtl, szOutput, TEXT("Difficulty:\t%hs\r\n"), token);
				break;
			case 15:
				OutputText(hwndCtl, TEXT("LB Rating:\t"));
				if (strcmp(token, "0") == 0)
					OutputText(hwndCtl, TEXT("Classic!\r\n"));
				else
					OutputTextFmt(hwndCtl, szOutput, TEXT("%hs\r\n"), token);
				break;
			case 16:
				OutputTextFmt(hwndCtl, szOutput, TEXT("Game:\t\t%hs\r\n"), token);
				break;
		}
		token = strtok(NULL, "\t");
	}

	// Do we have a valid TMX Track ID?
	if (strcmp(szTrackId, "0") == 0)
	{
		GlobalFreePtr((HGLOBAL)lpszData);
		return TRUE;
	}

	// Request additional TMX data by TMX Track ID
	_sntprintf(szTmxUrl, _countof(szTmxUrl), g_szUrlTmx, szSubDomain, g_szParamSearch, szTrackId);
	if (!ReadInternetFile(hwndCtl, szTmxUrl, lpszData, dwSize))
	{
		GlobalFreePtr((HGLOBAL)lpszData);
		return FALSE;
	}

	if (lpszData[0] == '\0' || strchr(lpszData, '\t') == NULL)
	{ // No valid track data available
		GlobalFreePtr((HGLOBAL)lpszData);
		return TRUE;
	}

	// Place the cursor at the end of the edit control
	nLen = Edit_GetTextLength(hwndCtl);
	Edit_SetSel(hwndCtl, (WPARAM)nLen, (LPARAM)nLen);

	// Parse and output TMX data
	i = 0;
	lpsz = lpszData;
	token = strtok(lpsz, "\t");
	while (token != NULL)
	{
		i++;
		switch (i)
		{
			case 13:
				OutputTextFmt(hwndCtl, szOutput, TEXT("Awards:\t\t%hs\r\n"), token);
				break;
			case 14:
				OutputTextFmt(hwndCtl, szOutput, TEXT("Comments:\t%hs\r\n"), token);
				break;
		}
		token = strtok(NULL, "\t");
	}

	// Request additional TMX data (Replays) via TMX Track ID
	_sntprintf(szTmxUrl, _countof(szTmxUrl), g_szUrlTmx, szSubDomain, g_szParamRecord, szTrackId);
	if (!ReadInternetFile(hwndCtl, szTmxUrl, lpszData, dwSize))
	{
		GlobalFreePtr((HGLOBAL)lpszData);
		return FALSE;
	}

	if (lpszData[0] == '\0' || strchr(lpszData, '\t') == NULL)
	{ // No valid track data available
		GlobalFreePtr((HGLOBAL)lpszData);
		return TRUE;
	}

	// Place the cursor at the end of the edit control
	nLen = Edit_GetTextLength(hwndCtl);
	Edit_SetSel(hwndCtl, (WPARAM)nLen, (LPARAM)nLen);
	OutputText(hwndCtl, g_szSep1);

	// Parse and output the Replay data
	UINT uRecords = 0;

	lpsz = lpszData;
	while (lpsz != NULL)
	{
		uRecords++;

		LPSTR lpszNewLine = strstr(lpsz, "<BR>\r\n");
		if (lpszNewLine != NULL)
		{
			*lpszNewLine = '\0';
			lpszNewLine += 6;
		}
		else
			break;

		i = 0;
		token = strtok(lpsz, "\t");
		while (token != NULL)
		{
			i++;
			switch (i)
			{
				case 2:
					OutputTextFmt(hwndCtl, szOutput, TEXT("%02u. "), uRecords);
					break;
				case 3:
					MultiByteToWideChar(uCodePage, 0, token, -1, szOutput, _countof(szOutput)-1);
					OutputText(hwndCtl, szOutput);
					OutputText(hwndCtl, TEXT(" "));
					break;
				case 4:
					{
						int nTime = atoi(token);
						if (bFormatTime)
						{
							if (nTime < 3600000)
							{
								int nMinute   = (nTime / 60000);
								int nSecond   = (nTime % 60000 / 1000);
								int nMilliSec = (nTime % 60000 % 1000);

								OutputTextFmt(hwndCtl, szOutput, TEXT("(%d:%02d.%003d)"),
									nMinute, nSecond, nMilliSec);
							}
							else
							{
								int nHour     = (nTime / 3600000);
								int nMinute   = (nTime % 3600000 / 60000);
								int nSecond   = (nTime % 3600000 % 60000 / 1000);
								int nMilliSec = (nTime % 3600000 % 60000 % 1000);

								OutputTextFmt(hwndCtl, szOutput, TEXT("(%d:%02d:%02d.%003d)"),
									nHour, nMinute, nSecond, nMilliSec);
							}
						}
						else
							OutputTextFmt(hwndCtl, szOutput, TEXT("(%d)"), nTime);
					}
					break;
				case 6:
					if (strcmp(szVersion, token) != 0)
						OutputText(hwndCtl, TEXT("!"));
					OutputText(hwndCtl, TEXT("\r\n"));
					break;
			}
			token = strtok(NULL, "\t");
		}
		lpsz = lpszNewLine;
	}

	GlobalFreePtr((HGLOBAL)lpszData);
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL PrintMxData(HWND hwndCtl, LPCSTR lpszUid, int nGame, PBOOL pbTrackFound)
{
	if (hwndCtl == NULL || lpszUid == NULL || pbTrackFound == NULL)
		return FALSE;

	*pbTrackFound = FALSE;

	UINT uCodePage = CP_ACP;
	if (IsValidCodePage(CP_UTF8))
		uCodePage = CP_UTF8;

	// Subdomain
	TCHAR szSubDomain[16];
	switch (nGame)
	{
		case GAME_TM2:
			_tcsncpy(szSubDomain, g_szTrackMania, _countof(szSubDomain));
			break;
		case GAME_SM:
			_tcsncpy(szSubDomain, g_szShootMania, _countof(szSubDomain));
			break;
		case GAME_QM:
			_tcsncpy(szSubDomain, g_szQuestMania, _countof(szSubDomain));
			break;
		default:
			return TRUE;
	}

	// Allocate memory for the MX data
	DWORD dwSize = TMX_MAX_DATASIZE;
	LPSTR lpszData = (LPSTR)GlobalAllocPtr(GHND, dwSize);
	if (lpszData == NULL)
	{
		OutputText(hwndCtl, g_szErrOom);
		return FALSE;
	}

	// Create query URL and retrieve data via map UID
	TCHAR szMxUrl[512];
	_sntprintf(szMxUrl, _countof(szMxUrl), g_szUrlMx1, szSubDomain,
		nGame == GAME_TM2 ? g_szTracks : g_szMaps, lpszUid);
	if (!ReadInternetFile(hwndCtl, szMxUrl, lpszData, dwSize))
	{
		GlobalFreePtr((HGLOBAL)lpszData);
		return FALSE;
	}

	if (lpszData[0] == '\0' ||
		(strstr(lpszData, "<TrackInfo") == NULL &&
		strstr(lpszData, "<SmMapInfo") == NULL &&
		strstr(lpszData, "<QmMapInfo") == NULL))
	{ // No valid map data available
		GlobalFreePtr((HGLOBAL)lpszData);
		return TRUE;
	}

	*pbTrackFound = TRUE;

	// Initialize MX Track ID for string comparison
	TCHAR szTrackId[32];
	TCHAR szTrackType[64];
	_tcsncpy(szTrackId, TEXT("0"), _countof(szTrackId));
	szTrackType[0] = TEXT('\0');

	// Place the cursor at the end of the edit control
	int nLen = Edit_GetTextLength(hwndCtl);
	Edit_SetSel(hwndCtl, (WPARAM)nLen, (LPARAM)nLen);

	// Parse and output the MX data
	if (nGame == GAME_TM2)
	{
		OutputXmlData(hwndCtl, uCodePage, lpszData, "TrackID", TEXT("Track ID:\t"), szTrackId, _countof(szTrackId));
		OutputXmlData(hwndCtl, uCodePage, lpszData, "TrackUID", TEXT("Track UID:\t"));
		OutputXmlData(hwndCtl, uCodePage, lpszData, "Name", TEXT("Track Name:\t"));
	}
	else
	{
		OutputXmlData(hwndCtl, uCodePage, lpszData, "MapID", TEXT("Map ID:\t\t"), szTrackId, _countof(szTrackId));
		OutputXmlData(hwndCtl, uCodePage, lpszData, "MapUID", TEXT("Map UID:\t"));
		OutputXmlData(hwndCtl, uCodePage, lpszData, "Name", TEXT("Map Name:\t"));
	}
	OutputXmlData(hwndCtl, uCodePage, lpszData, "UserID", TEXT("User ID:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "Username", TEXT("User Name:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "UpdatedAt", TEXT("Version:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "UploadedAt", TEXT("Uploaded:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "Unreleased", TEXT("Unreleased:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "UnlimiterRequired", TEXT("Unlimiter:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "HasGhostBlocks", TEXT("Ghost Blocks:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "EmbeddedObjectsCount", TEXT("Embedded Items:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "HasScreenshot", TEXT("Has Screenshot:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "HasThumbnail", TEXT("Has Thumbnail:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "ExeVersion", TEXT("Exe Version:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "ExeBuild", TEXT("Exe Build:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "EnvironmentName", TEXT("Environment:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "VehicleName", TEXT("Vehicle Name:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "TitlePack", TEXT("Title Pack:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "ModName", TEXT("Mod Name:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "Mood", TEXT("Mood:\t\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "DisplayCost", TEXT("Display Cost:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "Lightmap", TEXT("Lightmap:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "TypeName", TEXT("Type Name:\t"), szTrackType, _countof(szTrackType));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "MapType", TEXT("Map Type:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "StyleName", TEXT("Style:\t\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "LengthName", TEXT("Length:\t\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "RouteName", TEXT("Routes:\t\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "DifficultyName", TEXT("Difficulty:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "Laps", TEXT("Laps:\t\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "TrackValue", TEXT("Track Value:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "Rating", TEXT("Rating:\t\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "RatingExact", TEXT("Rating Exact:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "RatingCount", TEXT("Rating Count:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "AwardCount", TEXT("Award Count:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "ReplayCount", TEXT("Replay Count:\t"));
	OutputXmlData(hwndCtl, uCodePage, lpszData, "CommentCount", TEXT("Comment Count:\t"));

	// Do we have a valid MX Track ID?
	if (_tcscmp(szTrackId, TEXT("0")) == 0)
	{
		GlobalFreePtr((HGLOBAL)lpszData);
		return TRUE;
	}

	// Replay data is only available for TrackMania²
	if (nGame != GAME_TM2)
	{
		GlobalFreePtr((HGLOBAL)lpszData);
		return TRUE;
	}

	// Request additional MX data (Replays) via MX Track ID
	_sntprintf(szMxUrl, _countof(szMxUrl), g_szUrlMx2, szSubDomain, szTrackId);
	if (!ReadInternetFile(hwndCtl, szMxUrl, lpszData, dwSize))
	{
		GlobalFreePtr((HGLOBAL)lpszData);
		return FALSE;
	}

	LPSTR lpsz = (LPSTR)lpszData;
	if (lpsz[0] == '\0' || strstr(lpsz, "<Replay>") == NULL)
	{ // No valid track data available
		GlobalFreePtr((HGLOBAL)lpszData);
		return TRUE;
	}

	// Place the cursor at the end of the edit control
	nLen = Edit_GetTextLength(hwndCtl);
	Edit_SetSel(hwndCtl, (WPARAM)nLen, (LPARAM)nLen);
	OutputText(hwndCtl, g_szSep1);

	// Parse and output the Replay data
	TCHAR szUsername[256];
	TCHAR szReplayTime[32];
	TCHAR szStuntScore[32];
	TCHAR szPosition[32];

	for (int i = 1; i <= 10; i++)
	{
		if (!GetReplayData(uCodePage, &lpsz, "Username", szUsername, _countof(szUsername)))
			break;
		if (!GetReplayData(uCodePage, &lpsz, "ReplayTime", szReplayTime, _countof(szReplayTime)))
			break;
		if (!GetReplayData(uCodePage, &lpsz, "StuntScore", szStuntScore, _countof(szStuntScore)))
			break;
		if (!GetReplayData(uCodePage, &lpsz, "Position", szPosition, _countof(szPosition)))
			break;

		int nPos = _ttoi(szPosition);
		OutputTextFmt(hwndCtl, szPosition, TEXT("%02u. "), nPos);

		OutputText(hwndCtl, szUsername);
		OutputText(hwndCtl, TEXT(" "));

		int nTime = _ttoi(szReplayTime);
		if (_tcscmp(szTrackType, TEXT("Stunts")) != 0)
		{
			if (nTime < 3600000)
			{
				int nMinute   = (nTime / 60000);
				int nSecond   = (nTime % 60000 / 1000);
				int nMilliSec = (nTime % 60000 % 1000);

				OutputTextFmt(hwndCtl, szReplayTime, TEXT("(%d:%02d.%003d)"), nMinute, nSecond, nMilliSec);
			}
			else
			{
				int nHour     = (nTime / 3600000);
				int nMinute   = (nTime % 3600000 / 60000);
				int nSecond   = (nTime % 3600000 % 60000 / 1000);
				int nMilliSec = (nTime % 3600000 % 60000 % 1000);

				OutputTextFmt(hwndCtl, szReplayTime, TEXT("(%d:%02d:%02d.%003d)"), nHour, nMinute, nSecond, nMilliSec);
			}
		}
		else
			OutputTextFmt(hwndCtl, szReplayTime, TEXT("(%s)"), szStuntScore);

		OutputText(hwndCtl, TEXT("\r\n"));
	}

	GlobalFreePtr((HGLOBAL)lpszData);
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// XML parser for Replay data

BOOL GetReplayData(UINT uCodePage, LPSTR* lpszData, LPCSTR lpszMarker, LPTSTR szOutput, SIZE_T cchLenOutput)
{
	char szElement[64];
	char szValue[896];

	strncpy(szElement, "<", _countof(szElement));
	strncat(szElement, lpszMarker, _countof(szElement) - strlen(szElement) - 1);
	strncat(szElement, ">", _countof(szElement) - strlen(szElement) - 1);

	LPSTR lpszBegin = strstr(*lpszData, szElement);
	if (lpszBegin == NULL)
		return FALSE;

	lpszBegin += strlen(szElement);
	strncpy(szElement, "</", _countof(szElement));
	strncat(szElement, lpszMarker, _countof(szElement) - strlen(szElement) - 1);
	strncat(szElement, ">", _countof(szElement) - strlen(szElement) - 1);

	LPSTR lpszEnd = strstr(lpszBegin, szElement);
	if (lpszEnd == NULL)
		return FALSE;

	SIZE_T cchLen = lpszEnd - lpszBegin;
	if (cchLen >= _countof(szValue))
		return FALSE;

	strncpy(szValue, lpszBegin, cchLen);
	szValue[cchLen] = '\0';

	*lpszData = lpszEnd + strlen(szElement);

	MultiByteToWideChar(uCodePage, 0, szValue, -1, szOutput, (int)(cchLenOutput-1));

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Parse XML data and output it instantly

BOOL OutputXmlData(HWND hwndCtl, UINT uCodePage, LPCSTR lpszData, LPCSTR lpszMarker, LPCTSTR lpszText,
	LPTSTR lpszValue, SIZE_T cchValueLen)
{
	char szElement[64];
	char szValue[896];
	TCHAR szOutput[OUTPUT_LEN];

	strncpy(szElement, "<", _countof(szElement));
	strncat(szElement, lpszMarker, _countof(szElement) - strlen(szElement) - 1);
	strncat(szElement, ">", _countof(szElement) - strlen(szElement) - 1);

	LPCSTR lpszBegin = strstr(lpszData, szElement);
	if (lpszBegin == NULL)
		return FALSE;

	lpszBegin += strlen(szElement);
	strncpy(szElement, "</", _countof(szElement));
	strncat(szElement, lpszMarker, _countof(szElement) - strlen(szElement) - 1);
	strncat(szElement, ">", _countof(szElement) - strlen(szElement) - 1);

	LPCSTR lpszEnd = strstr(lpszBegin, szElement);
	if (lpszEnd == NULL)
		return FALSE;

	SIZE_T cchLen = lpszEnd - lpszBegin;
	if (cchLen >= _countof(szValue))
		return FALSE;

	strncpy(szValue, lpszBegin, cchLen);
	szValue[cchLen] = '\0';

	MultiByteToWideChar(uCodePage, 0, szValue, -1, szOutput, _countof(szOutput)-1);
	OutputText(hwndCtl, lpszText);
	OutputText(hwndCtl, szOutput);
	OutputText(hwndCtl, TEXT("\r\n"));

	if (lpszValue != NULL)
		_tcsncpy(lpszValue, szOutput, cchValueLen);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
