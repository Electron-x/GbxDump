////////////////////////////////////////////////////////////////////////////////////////////////
// Tmx.cpp - Copyright (c) 2010-2023 by Electron.
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
//
#include "stdafx.h"
#include "internet.h"
#include "tmx.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
//
#define GAME_TMNF   1
#define GAME_TMU    2
#define GAME_TMN    3
#define GAME_TMS    4
#define GAME_TMO    5
#define GAME_TM2    6
#define GAME_SM     7
#define GAME_QM     8
#define GAME_TM2020 9

#define TMX_MAX_DATASIZE 32768
#define MX_MAX_DATASIZE 524288

////////////////////////////////////////////////////////////////////////////////////////////////
// Data Types
//
typedef enum XmlArrayType
{
	XmlArrayType_None = 0,
	XmlArrayType_TrackInfo = 1,
	XmlArrayType_TrackObject = 2,
	XmlArrayType_Replay = 3,
	XmlArrayType_Item = 4,
	_XmlArrayType_Last = 4
} 	XmlArrayType;

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module
//
BOOL ProcessTmx(HWND hwndCtl, LPCSTR lpszUid, int nGame, PBOOL pbTrackFound);
BOOL ProcessMx(HWND hwndCtl, LPCSTR lpszUid, int nGame, PBOOL pbTrackFound);
BOOL RequestMxData(HWND hwndCtl, LPCTSTR lpszMxUrl, PINT pnTrackId = NULL);
HRESULT ParseAndPrintXml(HWND hwndCtl, HGLOBAL hXml, PINT pnTrackId = NULL);
BOOL FormatTimeT(int nTime, TCHAR* pszTime, SIZE_T cchStringLen);
BOOL FormatTimeW(int nTime, WCHAR* pwszTime, SIZE_T cchStringLen);

////////////////////////////////////////////////////////////////////////////////////////////////
// String Constants
//
const TCHAR g_szTmx[]         = TEXT("Track-/Mania Exchange:\r\n");
const TCHAR g_szHttp[]        = TEXT("http");
const TCHAR g_szHttps[]       = TEXT("https");
const TCHAR g_szTM[]          = TEXT("tm");
const TCHAR g_szSM[]          = TEXT("sm");
const TCHAR g_szQM[]          = TEXT("qm");
const TCHAR g_szTrackMania[]  = TEXT("trackmania");
const TCHAR g_szForever[]     = TEXT("tmnforever");
const TCHAR g_szUnited[]      = TEXT("united");
const TCHAR g_szNations[]     = TEXT("nations");
const TCHAR g_szSunrise[]     = TEXT("sunrise");
const TCHAR g_szOriginal[]    = TEXT("original");
const TCHAR g_szParamInfo[]   = TEXT("apitrackinfo&uid");
const TCHAR g_szParamSearch[] = TEXT("apisearch&trackid");
const TCHAR g_szParamRecord[] = TEXT("apitrackrecords&id");
const TCHAR g_szUrlTmx[]      = TEXT("%s://%s.tm-exchange.com/apiget.aspx?action=%s=%hs");
const TCHAR g_szUrlMpMaps[]   = TEXT("%s://%s.mania.exchange/api/maps/get_map_info/uid/%hs?format=xml");
const TCHAR g_szUrlMpItems[]  = TEXT("%s://%s.mania.exchange/api/maps/embeddedobjects/%d?format=xml");
const TCHAR g_szUrlMpRepl[]   = TEXT("%s://%s.mania.exchange/api/replays/get_replays/%d?format=xml");
const TCHAR g_szUrlMxMaps[]   = TEXT("%s://%s.exchange/api/maps/get_map_info/uid/%hs?format=xml");
const TCHAR g_szUrlMxItems[]  = TEXT("%s://%s.exchange/api/maps/embeddedobjects/%d?format=xml");
const TCHAR g_szUrlMxRepl[]   = TEXT("%s://%s.exchange/api/replays/get_replays/%d?format=xml");
const TCHAR g_szErrOom[]      = TEXT("Out of memory.\r\n");

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL GetTmxData(HWND hwndCtl, LPCSTR lpszUid, LPCSTR lpszEnvi)
{
	if (hwndCtl == NULL || lpszUid == NULL || lpszEnvi == NULL)
		return FALSE;

	HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	OutputText(hwndCtl, g_szTmx);

	BOOL bSuccess = TRUE;
	BOOL bTrackFound = FALSE;

	// Specify the T/MX subdomain according to the environment
	if ((strcmp(lpszEnvi, "Canyon") == 0) || (strcmp(lpszEnvi, "Valley") == 0) ||
		(strcmp(lpszEnvi, "Lagoon") == 0) || (strcmp(lpszEnvi, "Arena") == 0))
		bSuccess = ProcessMx(hwndCtl, lpszUid, GAME_TM2, &bTrackFound);
	else if ((strcmp(lpszEnvi, "Storm") == 0) || (strcmp(lpszEnvi, "Cryo") == 0) || (strcmp(lpszEnvi, "Meteor") == 0) ||
		(strcmp(lpszEnvi, "Paris") == 0) || (strcmp(lpszEnvi, "Gothic") == 0))
		bSuccess = ProcessMx(hwndCtl, lpszUid, GAME_SM, &bTrackFound);
	else if ((strcmp(lpszEnvi, "Galaxy") == 0) || (strcmp(lpszEnvi, "History") == 0) ||
		(strcmp(lpszEnvi, "Society") == 0) || (strcmp(lpszEnvi, "Future") == 0))
		bSuccess = ProcessMx(hwndCtl, lpszUid, GAME_QM, &bTrackFound);
	else if (strcmp(lpszEnvi, "Stadium") == 0)
	{
		bSuccess = ProcessMx(hwndCtl, lpszUid, GAME_TM2020, &bTrackFound);
		if (bSuccess && bTrackFound == FALSE)
		{
			bSuccess = ProcessMx(hwndCtl, lpszUid, GAME_TM2, &bTrackFound);
			if (bSuccess && bTrackFound == FALSE)
			{
				bSuccess = ProcessTmx(hwndCtl, lpszUid, GAME_TMNF, &bTrackFound);
				if (bSuccess && bTrackFound == FALSE)
				{
					bSuccess = ProcessTmx(hwndCtl, lpszUid, GAME_TMU, &bTrackFound);
					if (bSuccess && bTrackFound == FALSE)
						bSuccess = ProcessTmx(hwndCtl, lpszUid, GAME_TMN, &bTrackFound);
				}
			}
		}
	}
	else if ((strcmp(lpszEnvi, "Coast") == 0) || (strcmp(lpszEnvi, "Bay") == 0) || (strcmp(lpszEnvi, "Island") == 0))
	{
		bSuccess = ProcessTmx(hwndCtl, lpszUid, GAME_TMU, &bTrackFound);
		if (bSuccess && bTrackFound == FALSE)
			bSuccess = ProcessTmx(hwndCtl, lpszUid, GAME_TMS, &bTrackFound);
	}
	else if ((strcmp(lpszEnvi, "Desert") == 0) || (strcmp(lpszEnvi, "Snow") == 0) || (strcmp(lpszEnvi, "Rally") == 0) ||
		(strcmp(lpszEnvi, "Alpine") == 0) || (strcmp(lpszEnvi, "Speed") == 0))
	{
		bSuccess = ProcessTmx(hwndCtl, lpszUid, GAME_TMU, &bTrackFound);
		if (bSuccess && bTrackFound == FALSE)
			ProcessTmx(hwndCtl, lpszUid, GAME_TMO, &bTrackFound);
	}

	if (bSuccess && !bTrackFound)
	{ // Map not found
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_TRACK : IDP_ENG_ERR_TRACK);
	}

	OutputText(hwndCtl, g_szSep2);
	SetCursor(hOldCursor);

	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ProcessTmx(HWND hwndCtl, LPCSTR lpszUid, int nGame, PBOOL pbTrackFound)
{
	if (hwndCtl == NULL || lpszUid == NULL || pbTrackFound == NULL)
		return FALSE;

	TCHAR szOutput[OUTPUT_LEN];
	UINT uCodePage = CP_ACP;
	if (IsValidCodePage(CP_UTF8))
		uCodePage = CP_UTF8;

	*pbTrackFound = FALSE;

	// Subdomain
	TCHAR szSubDomain[16];
	switch (nGame)
	{
		case GAME_TMNF:
			MyStrNCpy(szSubDomain, g_szForever, _countof(szSubDomain));
			break;
		case GAME_TMU:
			MyStrNCpy(szSubDomain, g_szUnited, _countof(szSubDomain));
			break;
		case GAME_TMN:
			MyStrNCpy(szSubDomain, g_szNations, _countof(szSubDomain));
			break;
		case GAME_TMS:
			MyStrNCpy(szSubDomain, g_szSunrise, _countof(szSubDomain));
			break;
		case GAME_TMO:
			MyStrNCpy(szSubDomain, g_szOriginal, _countof(szSubDomain));
			break;
		default:
			return TRUE;
	}

	// Allocate memory for the TMX data
	DWORD dwSize = TMX_MAX_DATASIZE;
	LPSTR lpszData = (LPSTR)MyGlobalAllocPtr(GHND, dwSize);
	if (lpszData == NULL)
	{
		OutputText(hwndCtl, g_szSep1);
		OutputText(hwndCtl, g_szErrOom);
		return FALSE;
	}

	// Create query URL and retrieve data via track UID
	TCHAR szTmxUrl[512];
	_sntprintf(szTmxUrl, _countof(szTmxUrl), g_szUrlTmx, g_szHttp, szSubDomain, g_szParamInfo, lpszUid);

	if (!ReadInternetFile(hwndCtl, szTmxUrl, lpszData, dwSize))
	{
		MyGlobalFreePtr((LPVOID)lpszData);
		return FALSE;
	}

	if (lpszData[0] == '\0' || strchr(lpszData, '\t') == NULL)
	{ // No valid track data available
		MyGlobalFreePtr((LPVOID)lpszData);
		return TRUE;
	}

	*pbTrackFound = TRUE;

	BOOL bIsStunts = FALSE;

	// Initialize TMX ID and track version for string comparison
	char szVersion[32];
	char szTrackId[32];
	szVersion[0] = '\0';
	strcpy(szTrackId, "0");

	// Place the cursor at the end of the edit control
	int nLen = Edit_GetTextLength(hwndCtl);
	Edit_SetSel(hwndCtl, (WPARAM)nLen, (LPARAM)nLen);
	OutputText(hwndCtl, g_szSep1);

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
				MyStrNCpyA(szTrackId, token, _countof(szTrackId));
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Track ID:\t%hs\r\n"), token);
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
				MyStrNCpyA(szVersion, token, _countof(szVersion));
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Uploaded:\t%hs\r\n"), token);
				break;
			case 6:
				MyStrNCpyA(szVersion, token, _countof(szVersion));
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Updated:\t%hs\r\n"), token);
				break;
			case 7:
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Visible:\t%hs\r\n"), token);
				break;
			case 8:
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Type:\t\t%hs\r\n"), token);
				if (strcmp(token, "Stunts") == 0)
					bIsStunts = TRUE;
				break;
			case 9:
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Environment:\t%hs\r\n"), token);
				break;
			case 10:
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Mood:\t\t%hs\r\n"), token);
				break;
			case 11:
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Style:\t\t%hs\r\n"), token);
				break;
			case 12:
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Routes:\t\t%hs\r\n"), token);
				break;
			case 13:
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Length:\t\t%hs\r\n"), token);
				break;
			case 14:
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Difficulty:\t%hs\r\n"), token);
				break;
			case 15:
				OutputText(hwndCtl, TEXT("LB Rating:\t"));
				if (strcmp(token, "0") == 0)
					OutputText(hwndCtl, TEXT("Classic!\r\n"));
				else
					OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("%hs\r\n"), token);
				break;
			case 16:
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Game:\t\t%hs\r\n"), token);
				break;
		}
		token = strtok(NULL, "\t");
	}

	// Do we have a valid TMX track ID?
	if (strcmp(szTrackId, "0") == 0)
	{
		MyGlobalFreePtr((LPVOID)lpszData);
		return TRUE;
	}

	// Request additional TMX data by TMX track ID
	_sntprintf(szTmxUrl, _countof(szTmxUrl), g_szUrlTmx, g_szHttp, szSubDomain, g_szParamSearch, szTrackId);

	if (!ReadInternetFile(hwndCtl, szTmxUrl, lpszData, dwSize))
	{
		MyGlobalFreePtr((LPVOID)lpszData);
		return FALSE;
	}

	if (lpszData[0] == '\0' || strchr(lpszData, '\t') == NULL)
	{ // No valid track data available
		MyGlobalFreePtr((LPVOID)lpszData);
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
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Awards:\t\t%hs\r\n"), token);
				break;
			case 14:
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Comments:\t%hs\r\n"), token);
				break;
		}
		token = strtok(NULL, "\t");
	}

	// Request additional TMX data (Replays) via TMX track ID
	_sntprintf(szTmxUrl, _countof(szTmxUrl), g_szUrlTmx, g_szHttp, szSubDomain, g_szParamRecord, szTrackId);

	if (!ReadInternetFile(hwndCtl, szTmxUrl, lpszData, dwSize))
	{
		MyGlobalFreePtr((LPVOID)lpszData);
		return FALSE;
	}

	if (lpszData[0] == '\0' || strchr(lpszData, '\t') == NULL)
	{ // No valid track data available
		MyGlobalFreePtr((LPVOID)lpszData);
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

		LPSTR lpszNewLine = strstr(lpsz, "\r\n");
		if (lpszNewLine != NULL)
		{
			*lpszNewLine = '\0';
			lpszNewLine += 2;
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
					OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("%02u. "), uRecords);
					break;
				case 3:
					MultiByteToWideChar(uCodePage, 0, token, -1, szOutput, _countof(szOutput)-1);
					OutputText(hwndCtl, szOutput);
					OutputText(hwndCtl, TEXT(" "));
					break;
				case 4:
					{
						int nTime = atoi(token);
						if (bIsStunts)
							OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("(%d)"), nTime);
						else
						{
							FormatTimeT(nTime, szOutput, _countof(szOutput));
							OutputText(hwndCtl, szOutput);
						}
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

	MyGlobalFreePtr((LPVOID)lpszData);
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ProcessMx(HWND hwndCtl, LPCSTR lpszUid, int nGame, PBOOL pbTrackFound)
{
	if (hwndCtl == NULL || lpszUid == NULL || pbTrackFound == NULL)
		return FALSE;

	*pbTrackFound = FALSE;

	// Subdomain
	TCHAR szSubDomain[16];
	switch (nGame)
	{
		case GAME_TM2020:
			MyStrNCpy(szSubDomain, g_szTrackMania, _countof(szSubDomain));
			break;
		case GAME_TM2:
			MyStrNCpy(szSubDomain, g_szTM, _countof(szSubDomain));
			break;
		case GAME_SM:
			MyStrNCpy(szSubDomain, g_szSM, _countof(szSubDomain));
			break;
		case GAME_QM:
			MyStrNCpy(szSubDomain, g_szQM, _countof(szSubDomain));
			break;
		default:
			return TRUE;
	}

	int nTrackId = 0;

	// Request, parse and output MX data (TrackInfo) via map UID
	TCHAR szMxUrl[512];
	_sntprintf(szMxUrl, _countof(szMxUrl),
		nGame == GAME_TM2020 ? g_szUrlMxMaps : g_szUrlMpMaps, g_szHttps, szSubDomain, lpszUid);
	
	if (!RequestMxData(hwndCtl, szMxUrl, &nTrackId))
		return FALSE;

	// Do we have a valid MX track ID?
	if (nTrackId == 0)
		return TRUE;

	*pbTrackFound = TRUE;

	// Request, parse and output additional MX data (TrackObject) via MX track ID
	_sntprintf(szMxUrl, _countof(szMxUrl),
		nGame == GAME_TM2020 ? g_szUrlMxItems : g_szUrlMpItems, g_szHttps, szSubDomain, nTrackId);
	
	if (!RequestMxData(hwndCtl, szMxUrl))
		return FALSE;

	// Replay data is only available for TrackMania² and Trackmania 2020
	if (nGame != GAME_TM2020 && nGame != GAME_TM2)
		return TRUE;

	// Request, parse and output additional MX data (Replay) via MX track ID
	_sntprintf(szMxUrl, _countof(szMxUrl),
		nGame == GAME_TM2020 ? g_szUrlMxRepl : g_szUrlMpRepl, g_szHttps, szSubDomain, nTrackId);
	
	if (!RequestMxData(hwndCtl, szMxUrl, &nTrackId))
		return FALSE;

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Request, parse and output data from Mania Exchange

BOOL RequestMxData(HWND hwndCtl, LPCTSTR lpszMxUrl, PINT pnTrackId)
{
	if (hwndCtl == NULL || lpszMxUrl == NULL)
		return FALSE;

	DWORD dwSize = MX_MAX_DATASIZE;
	HGLOBAL hXml = GlobalAlloc(GHND, dwSize);
	if (hXml == NULL)
	{
		OutputText(hwndCtl, g_szSep1);
		OutputText(hwndCtl, g_szErrOom);
		return FALSE;
	}

	LPSTR lpszData = (LPSTR)GlobalLock(hXml);
	if (lpszData == NULL)
	{
		GlobalFree(hXml);
		return FALSE;
	}

	if (!ReadInternetFile(hwndCtl, lpszMxUrl, lpszData, dwSize))
	{
		GlobalUnlock(hXml);
		GlobalFree(hXml);
		return FALSE;
	}

	GlobalUnlock(hXml);

	// Perform a simple test for valid data before initializing XmlLite
	if (lpszData[0] == '\0' ||
		(strstr(lpszData, "<TrackInfo") == NULL && strstr(lpszData, "<SmMapInfo") == NULL &&
		strstr(lpszData, "<QmMapInfo") == NULL && strstr(lpszData, "<TrackObject") == NULL &&
		strstr(lpszData, "<Replay") == NULL && strstr(lpszData, "<Item") == NULL))
	{
		GlobalFree(hXml);
		return TRUE;
	}

	// Place the cursor at the end of the edit control
	int nLen = Edit_GetTextLength(hwndCtl);
	Edit_SetSel(hwndCtl, (WPARAM)nLen, (LPARAM)nLen);
	OutputText(hwndCtl, g_szSep1);

	HRESULT hr = S_FALSE;
	if (FAILED(hr = ParseAndPrintXml(hwndCtl, hXml, pnTrackId)))
	{
		GlobalFree(hXml);
		return FALSE;
	}

	GlobalFree(hXml);
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Parse XML using Microsoft XmlLite.
// Supports the array types TrackInfo, TrackObject, Replay and Item.

HRESULT ParseAndPrintXml(HWND hwndCtl, HGLOBAL hXml, PINT pnTrackId)
{
	if (hwndCtl == NULL || hXml == NULL)
		return E_INVALIDARG;

	HRESULT hr = S_FALSE;
	IStream* pStream = NULL;
	IXmlReader* pReader = NULL;

	XmlNodeType nodeType = XmlNodeType_None;
	XmlArrayType arrayType = XmlArrayType_None;

	const WCHAR* pwszElement = NULL;
	const WCHAR* pwszValue = NULL;

	int nReplayPos = 0;
	int nReplayTime = 0;
	int nObjectNumber = 0;

	WCHAR wszOutput[OUTPUT_LEN];
	WCHAR wszElement[256];
	WCHAR wszUsername[256];
	WCHAR wszReplayTime[32];
	WCHAR wszStuntScore[32];
	WCHAR wszObjectPath[260];
	WCHAR wszObjectAuthor[256];

	wszOutput[0] = L'\0';
	wszElement[0] = L'\0';
	wszUsername[0] = L'\0';
	wszReplayTime[0] = L'\0';
	wszStuntScore[0] = L'\0';
	wszObjectPath[0] = L'\0';
	wszObjectAuthor[0] = L'\0';

	if (FAILED(hr = CreateStreamOnHGlobal(hXml, FALSE, &pStream)))
		goto CleanUp;
	if (FAILED(hr = CreateXmlReader(IID_IXmlReader, (void**)&pReader, NULL)))
		goto CleanUp;
	if (FAILED(hr = pReader->SetProperty(XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit)))
		goto CleanUp;
	if (FAILED(hr = pReader->SetInput(pStream)))
		goto CleanUp;

	// Read until there are no more nodes
	while ((hr = pReader->Read(&nodeType)) == S_OK)
	{
		switch (nodeType)
		{
		case XmlNodeType_Element:
			if (FAILED(hr = pReader->GetLocalName(&pwszElement, NULL)))
				goto CleanUp;
			if (pwszElement == NULL)
				break;

			if (_wcsicmp(pwszElement, L"TrackInfo") == 0)
				arrayType = XmlArrayType_TrackInfo;
			else if (_wcsicmp(pwszElement, L"TrackObject") == 0)
				arrayType = XmlArrayType_TrackObject;
			else if (_wcsicmp(pwszElement, L"Replay") == 0)
				arrayType = XmlArrayType_Replay;
			else if (_wcsicmp(pwszElement, L"Item") == 0)
				arrayType = XmlArrayType_Item;

			if (arrayType != XmlArrayType_None)
				MyStrNCpyW(wszElement, pwszElement, _countof(wszElement));

			break;

		case XmlNodeType_Text:
			if (FAILED(hr = pReader->GetValue(&pwszValue, NULL)))
				goto CleanUp;
			if (pwszValue == NULL)
				break;

			if (arrayType == XmlArrayType_TrackInfo)
			{
				if (_wcsicmp(wszElement, L"Comments") == 0)
					break;

				if (pnTrackId != NULL &&
					(_wcsicmp(wszElement, L"TrackID") == 0 || _wcsicmp(wszElement, L"MapID") == 0))
					*pnTrackId = _wtoi(pwszValue);

				wcsncat(wszElement, L":", _countof(wszElement) - wcslen(wszElement) - 1);
				OutputTextFmt(hwndCtl, wszOutput, _countof(wszOutput), L"%-23s %s\r\n", wszElement, pwszValue);
			}
			else if (arrayType == XmlArrayType_TrackObject)
			{
				if (_wcsicmp(wszElement, L"ObjectPath") == 0)
					MyStrNCpyW(wszObjectPath, pwszValue, _countof(wszObjectPath));
				if (_wcsicmp(wszElement, L"ObjectAuthor") == 0)
				{
					wszUsername[0] = L'\0';	// Username element can be empty or missing
					MyStrNCpyW(wszObjectAuthor, pwszValue, _countof(wszObjectAuthor));
				}
				if (_wcsicmp(wszElement, L"Username") == 0)
					MyStrNCpyW(wszUsername, pwszValue, _countof(wszUsername));
			}
			else if (arrayType == XmlArrayType_Replay)
			{
				if (_wcsicmp(wszElement, L"Username") == 0)
					MyStrNCpyW(wszUsername, pwszValue, _countof(wszUsername));
				if (_wcsicmp(wszElement, L"ReplayTime") == 0)
					nReplayTime = _wtoi(pwszValue);
				if (_wcsicmp(wszElement, L"StuntScore") == 0)
					MyStrNCpyW(wszStuntScore, pwszValue, _countof(wszStuntScore));
				if (_wcsicmp(wszElement, L"Position") == 0)
					nReplayPos = _wtoi(pwszValue);
			}
			else if (arrayType == XmlArrayType_Item)
			{
				wcsncat(wszElement, L":", _countof(wszElement) - wcslen(wszElement) - 1);
				OutputTextFmt(hwndCtl, wszOutput, _countof(wszOutput), L"%-15s %s\r\n", wszElement, pwszValue);
			}

			break;

		case XmlNodeType_EndElement:
			if (FAILED(hr = pReader->GetLocalName(&pwszElement, NULL)))
				goto CleanUp;
			if (pwszElement == NULL)
				break;

			if (_wcsicmp(pwszElement, L"TrackInfo") == 0)
				arrayType = XmlArrayType_None;
			else if (_wcsicmp(pwszElement, L"TrackObject") == 0)
			{
				OutputTextFmt(hwndCtl, wszOutput, _countof(wszOutput), L"%02d. %s (%s",
					++nObjectNumber, wszObjectPath, wszObjectAuthor);
				if (wcsncmp(wszUsername, wszObjectAuthor, wcslen(wszUsername)) != 0)
					OutputTextFmt(hwndCtl, wszOutput, _countof(wszOutput), L"/%s", wszUsername);
				OutputText(hwndCtl, L")\r\n");
				arrayType = XmlArrayType_None;
			}
			else if (_wcsicmp(pwszElement, L"Replay") == 0)
			{
				FormatTimeW(nReplayTime, wszReplayTime, _countof(wszReplayTime));
				OutputTextFmt(hwndCtl, wszOutput, _countof(wszOutput), L"%02d. %s (%s)",
					nReplayPos, wszUsername, wszReplayTime);
				if (wcsncmp(wszStuntScore, L"0", wcslen(wszStuntScore)) != 0)
					OutputTextFmt(hwndCtl, wszOutput, _countof(wszOutput), L" %s pt.", wszStuntScore);
				OutputText(hwndCtl, L"\r\n");
				arrayType = XmlArrayType_None;
			}
			else if (_wcsicmp(pwszElement, L"Item") == 0)
				arrayType = XmlArrayType_None;

			break;
		}
	}

	hr = S_OK;

CleanUp:
	if (pReader != NULL)
	{
		pReader->SetInput(NULL);
		pReader->Release();
	}

	if (pStream != NULL)
		pStream->Release();

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Converts a time value into a formatted string (tchar.h version)

static BOOL FormatTimeT(int nTime, TCHAR* pszTime, SIZE_T cchStringLen)
{
	if (pszTime == NULL)
		return FALSE;

	if (nTime < 0)
		_sntprintf(pszTime, cchStringLen, TEXT("%d"), nTime);
	else
	{
		if (nTime < 3600000)
		{
			DWORD dwMinute = (nTime / 60000);
			DWORD dwSecond = (nTime % 60000 / 1000);
			DWORD dwMilliSec = (nTime % 60000 % 1000);

			_sntprintf(pszTime, cchStringLen, TEXT("%u:%02u.%003u"),
				dwMinute, dwSecond, dwMilliSec);
		}
		else
		{
			DWORD dwHour = (nTime / 3600000);
			DWORD dwMinute = (nTime % 3600000 / 60000);
			DWORD dwSecond = (nTime % 3600000 % 60000 / 1000);
			DWORD dwMilliSec = (nTime % 3600000 % 60000 % 1000);

			_sntprintf(pszTime, cchStringLen, TEXT("%u:%02u:%02u.%003u"),
				dwHour, dwMinute, dwSecond, dwMilliSec);
		}
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Converts a time value into a formatted string (wide-character version for XmlLite)

static BOOL FormatTimeW(int nTime, WCHAR* pwszTime, SIZE_T cchStringLen)
{
	if (pwszTime == NULL)
		return FALSE;

	if (nTime < 0)
		_snwprintf(pwszTime, cchStringLen, L"%d", nTime);
	else
	{
		if (nTime < 3600000)
		{
			DWORD dwMinute = (nTime / 60000);
			DWORD dwSecond = (nTime % 60000 / 1000);
			DWORD dwMilliSec = (nTime % 60000 % 1000);

			_sntprintf(pwszTime, cchStringLen, L"%u:%02u.%003u",
				dwMinute, dwSecond, dwMilliSec);
		}
		else
		{
			DWORD dwHour = (nTime / 3600000);
			DWORD dwMinute = (nTime % 3600000 / 60000);
			DWORD dwSecond = (nTime % 3600000 % 60000 / 1000);
			DWORD dwMilliSec = (nTime % 3600000 % 60000 % 1000);

			_snwprintf(pwszTime, cchStringLen, L"%u:%02u:%02u.%003u",
				dwHour, dwMinute, dwSecond, dwMilliSec);
		}
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
