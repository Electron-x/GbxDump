////////////////////////////////////////////////////////////////////////////////////////////////
// DumpPak.cpp - Copyright (c) 2010-2018 by Electron.
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
// Based on information from https://wiki.xaseco.org/wiki/PAK
//
////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "classid.h"
#include "archive.h"
#include "dumppak.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Data Types
//
typedef struct SHeaderFlagsUncrypt
{
	UINT IsHeaderPrivate		: 1;
	UINT UseDefaultHeaderKey	: 1;
	UINT IsDataPrivate			: 1;
	UINT IsImpostor				: 1;
	UINT __Unused__				: 28;
} HEADERFLAGSUNCRYPT, *PHEADERFLAGSUNCRYPT;

typedef struct SFileDescFlags
{
	UINT IsHashed			: 1;
	UINT PublishFid			: 1;
	UINT Compression		: 4;
	UINT IsSeekable			: 1;
	UINT _Unknown_			: 1;
	UINT __Unused1__		: 24;
	UINT DontUseDummyWrite	: 1;
	UINT OpaqueUserData		: 16;
	UINT PublicFile			: 1;
	UINT ForceNoCrypt		: 1;
	UINT __Unused2__		: 13;
} FILEDESCFLAGS, *PFILEDESCFLAGS;

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module
//
BOOL DumpChecksum(HWND hwndCtl, HANDLE hFile, SIZE_T cbLen);
BOOL DumpAuthorInfo(HWND hwndCtl, HANDLE hFile);
BOOL DumpIncludedPacksHeaders(HWND hwndCtl, HANDLE hFile, DWORD dwVersion);
BOOL DumpPackHeader(HWND hwndCtl, HANDLE hFile, DWORD dwVersion, DWORD dwHeaderMaxSize);

////////////////////////////////////////////////////////////////////////////////////////////////
// String Constants
//
const TCHAR g_chNil        = TEXT('\0');
const TCHAR g_szOR[]       = TEXT("|");
const TCHAR g_szAsterisk[] = TEXT("*");
const TCHAR g_szCRLF[]     = TEXT("\r\n");

////////////////////////////////////////////////////////////////////////////////////////////////
// DumpPack is called by DumpFile from GbxDump.cpp

BOOL DumpPack(HWND hwndCtl, HANDLE hFile)
{
	SSIZE_T nRet = 0;
	DWORD   dwTxtSize = 0;
	DWORD   dwXmlSize = 0;
	LPVOID  lpDataTxt = NULL;
	LPVOID  lpDataXml = NULL;
	CHAR    szRead[ID_LEN];
	TCHAR   szOutput[OUTPUT_LEN];

	if (hwndCtl == NULL || hFile == NULL)
		return FALSE;

	// Jump to version (skip file signature)
	if (!FileSeekBegin(hFile, 8))
		return FALSE;

	// Version
	DWORD dwVersion = 0;
	if (!ReadNat32(hFile, &dwVersion))
		return FALSE;

	OutputText(hwndCtl, g_szSep1);
	OutputTextFmt(hwndCtl, szOutput, TEXT("Pack Version:\t%d"), dwVersion);
	if (dwVersion > 18) OutputText(hwndCtl, g_szAsterisk);
	OutputText(hwndCtl, g_szCRLF);

	if (dwVersion < 6)
	{ // Packs with version < 6 don't have a Pack header
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_UNAVAIL : IDP_ENG_ERR_UNAVAIL);
		return TRUE;
	}

	// ContentsChecksum
	if (!DumpChecksum(hwndCtl, hFile, 32))
		return FALSE;

	// SHeaderFlagsUncrypt
	DWORD dwCryptFlags = 0;
	if (!ReadNat32(hFile, &dwCryptFlags))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Decrypt Flags:\t%08X"), dwCryptFlags);

	PHEADERFLAGSUNCRYPT pCryptFlags = (PHEADERFLAGSUNCRYPT)&dwCryptFlags;

	szOutput[0] = g_chNil;

	if (pCryptFlags->IsHeaderPrivate)
	{
		if (szOutput[0] != g_chNil)
			_tcsncat(szOutput, g_szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
		_tcsncat(szOutput, TEXT("IsHeaderPrivate"), _countof(szOutput) - _tcslen(szOutput) - 1);
	}

	if (pCryptFlags->UseDefaultHeaderKey)
	{
		if (szOutput[0] != g_chNil)
			_tcsncat(szOutput, g_szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
		_tcsncat(szOutput, TEXT("UseDefaultHeaderKey"), _countof(szOutput) - _tcslen(szOutput) - 1);
	}

	if (pCryptFlags->IsDataPrivate)
	{
		if (szOutput[0] != g_chNil)
			_tcsncat(szOutput, g_szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
		_tcsncat(szOutput, TEXT("IsDataPrivate"), _countof(szOutput) - _tcslen(szOutput) - 1);
	}

	if (pCryptFlags->IsImpostor)
	{
		if (szOutput[0] != g_chNil)
			_tcsncat(szOutput, g_szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
		_tcsncat(szOutput, TEXT("IsImpostor"), _countof(szOutput) - _tcslen(szOutput) - 1);
	}

	if (pCryptFlags->__Unused__)
	{
		if (szOutput[0] != g_chNil)
			_tcsncat(szOutput, g_szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
		_tcsncat(szOutput, g_szAsterisk, _countof(szOutput) - _tcslen(szOutput) - 1);
	}

	if (szOutput[0] != g_chNil)
	{
		OutputText(hwndCtl, TEXT(" ("));
		OutputText(hwndCtl, szOutput);
		OutputText(hwndCtl, TEXT(")"));
	}

	OutputText(hwndCtl, g_szCRLF);

	if (dwVersion < 7)
		return TRUE;

	// Header Max Size
	DWORD dwHeaderMaxSize = 0;
	if (dwVersion >= 15)
	{
		if (!ReadNat32(hFile, &dwHeaderMaxSize))
			return FALSE;

		OutputText(hwndCtl, TEXT("Header Size:\t"));
		OutputTextFmt(hwndCtl, szOutput, TEXT("%08X"), dwHeaderMaxSize);

		switch (dwHeaderMaxSize)
		{
			case 0x4000:	// 16 KB
				OutputText(hwndCtl, TEXT(" (small)"));
				break;
			case 0x100000:	// 1 MB
				OutputText(hwndCtl, TEXT(" (big)"));
				break;
			case 0x1000000:	// 16 MB
				OutputText(hwndCtl, TEXT(" (huge)"));
				break;
		}
		OutputText(hwndCtl, g_szCRLF);
	}

	// SAuthorInfo
	if (!DumpAuthorInfo(hwndCtl, hFile))
		return FALSE;

	if (dwVersion < 9)
	{
		// Comment
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("Comment:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
			OutputText(hwndCtl, szOutput);
		}

		// Skip unused variable (16 bytes)
		if (!FileSeekCurrent(hFile, 16))
			return FALSE;
	}

	if (dwVersion < 8)
		return TRUE;

	if (dwVersion < 9)
	{
		// CreationBuildInfo
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("Build Info:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}

		// URL
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("URL:\t\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}

		return TRUE;
	}

	// Manialink
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Manialink:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
		OutputText(hwndCtl, szOutput);
	}

	if (dwVersion >= 13)
	{
		// Download URL
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("Download URL:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}
	}

	// Creation time
	FILETIME ftDate = {0};
	SYSTEMTIME stDate = {0};
	if (!ReadData(hFile, (LPVOID)&ftDate, 8))
		return FALSE;

	if ((ftDate.dwLowDateTime != 0 || ftDate.dwHighDateTime != 0) && FileTimeToSystemTime(&ftDate, &stDate))
		OutputTextFmt(hwndCtl, szOutput, TEXT("Creation Date:\t%02u-%02u-%02u %02u:%02u:%02u\r\n"),
			stDate.wYear, stDate.wMonth, stDate.wDay, stDate.wHour, stDate.wMinute, stDate.wSecond);

	// Comment
	if (!ReadNat32(hFile, &dwTxtSize))
		return FALSE;
	if (dwTxtSize > 0xFFFF) // sanity check
		return FALSE;

	if (dwTxtSize > 0)
	{
		lpDataTxt = GlobalAllocPtr(GHND, dwTxtSize + 1); // 1 more character for terminating zero
		if (lpDataTxt == NULL)
			return FALSE;

		if (!ReadData(hFile, lpDataTxt, dwTxtSize))
		{
			GlobalFreePtr(lpDataTxt);
			return FALSE;
		}
	}

	if (dwVersion >= 12)
	{
		// XML
		if (!ReadNat32(hFile, &dwXmlSize))
		{
			if (lpDataTxt != NULL)
				GlobalFreePtr(lpDataTxt);
			return FALSE;
		}
		if (dwXmlSize > 0xFFFF) // sanity check
		{
			if (lpDataTxt != NULL)
				GlobalFreePtr(lpDataTxt);
			return FALSE;
		}

		if (dwXmlSize > 0)
		{
			lpDataXml = GlobalAllocPtr(GHND, dwXmlSize + 1); // 1 more character for terminating zero
			if (lpDataXml == NULL)
			{
				if (lpDataTxt != NULL)
					GlobalFreePtr(lpDataTxt);
				return FALSE;
			}

			if (!ReadData(hFile, lpDataXml, dwXmlSize))
			{
				GlobalFreePtr(lpDataXml);
				if (lpDataTxt != NULL)
					GlobalFreePtr(lpDataTxt);
				return FALSE;
			}
		}

		// Title ID
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		{
			if (lpDataXml != NULL)
				GlobalFreePtr(lpDataXml);
			if (lpDataTxt != NULL)
				GlobalFreePtr(lpDataTxt);
			return FALSE;
		}

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("Title ID:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}
	}

	// Usage/SubDir
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
	{
		if (lpDataXml != NULL)
			GlobalFreePtr(lpDataXml);
		if (lpDataTxt != NULL)
			GlobalFreePtr(lpDataTxt);
		return FALSE;
	}

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Pack Type:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// CreationBuildInfo
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
	{
		if (lpDataXml != NULL)
			GlobalFreePtr(lpDataXml);
		if (lpDataTxt != NULL)
			GlobalFreePtr(lpDataTxt);
		return FALSE;
	}

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Build Info:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Jump to the included packages (skip unused 16 bytes)
	if (!FileSeekCurrent(hFile, 16))
	{
		if (lpDataXml != NULL)
			GlobalFreePtr(lpDataXml);
		if (lpDataTxt != NULL)
			GlobalFreePtr(lpDataTxt);
		return FALSE;
	}

	if (dwVersion >= 10)
	{
		// Number of included packages
		DWORD dwNumIncludedPacks = 0;
		if (!ReadNat32(hFile, &dwNumIncludedPacks) || dwNumIncludedPacks > 0x10000000)
		{
			if (lpDataXml != NULL)
				GlobalFreePtr(lpDataXml);
			if (lpDataTxt != NULL)
				GlobalFreePtr(lpDataTxt);
			return FALSE;
		}

		OutputText(hwndCtl, g_szSep1);
		OutputTextFmt(hwndCtl, szOutput, TEXT("Included Packs:\t%d\r\n"), dwNumIncludedPacks);

		while (dwNumIncludedPacks--)
		{
			// Included Packs Headers
			if (!DumpIncludedPacksHeaders(hwndCtl, hFile, dwVersion))
			{
				if (lpDataXml != NULL)
					GlobalFreePtr(lpDataXml);
				if (lpDataTxt != NULL)
					GlobalFreePtr(lpDataTxt);
				return FALSE;
			}
		}
	}

	if (lpDataTxt != NULL)
	{ // Output comment
		// Replace line breaks with separators
		LPSTR lpsz = (LPSTR)lpDataTxt;
		while (lpsz = strchr(lpsz, '\n'))
			*lpsz++ = '|';

		OutputText(hwndCtl, g_szSep1);
		ConvertGbxString(lpDataTxt, dwTxtSize, szOutput, _countof(szOutput), TRUE);
		OutputText(hwndCtl, szOutput);

		GlobalFreePtr(lpDataTxt);
	}

	if (lpDataXml != NULL)
	{ // Output XML
		// Allocate memory for Unicode
		LPVOID pXmlString = GlobalAllocPtr(GHND, 2 * (dwXmlSize + 3));
		if (pXmlString != NULL)
		{
			__try
			{
				// Convert to Unicode
				ConvertGbxString(lpDataXml, dwXmlSize, (LPTSTR)pXmlString, dwXmlSize + 3);

				// Insert line breaks
				LPTSTR lpszTemp = AllocReplaceString((LPCTSTR)pXmlString, TEXT("><"), TEXT(">\r\n<"));

				OutputText(hwndCtl, g_szSep1);

				if (lpszTemp == NULL)
					OutputText(hwndCtl, (LPCTSTR)pXmlString);
				else
				{
					OutputText(hwndCtl, lpszTemp);
					GlobalFreePtr((LPVOID)lpszTemp);
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER) { ; }

			GlobalFreePtr(pXmlString);
		}

		GlobalFreePtr(lpDataXml);
	}

	if (pCryptFlags->IsHeaderPrivate || pCryptFlags->UseDefaultHeaderKey)
		return TRUE;

	return DumpPackHeader(hwndCtl, hFile, dwVersion, dwHeaderMaxSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpChecksum(HWND hwndCtl, HANDLE hFile, SIZE_T cbLen)
{
	LPVOID lpData = GlobalAllocPtr(GHND, cbLen);
	if (lpData == NULL)
		return FALSE;

	// Contents Checksum
	if (!ReadData(hFile, lpData, cbLen))
	{
		GlobalFreePtr(lpData);
		return FALSE;
	}

	TCHAR szOutput[OUTPUT_LEN];
	szOutput[0] = g_chNil;

	LPBYTE lpByte = (LPBYTE)lpData;
	cbLen = min(cbLen, _countof(szOutput)/2 - 1);
	while (cbLen--)
		_sntprintf(szOutput, _countof(szOutput), TEXT("%s%02X"), (LPTSTR)szOutput, (WORD)*lpByte++);

	OutputText(hwndCtl, TEXT("Checksum:\t"));
	OutputText(hwndCtl, szOutput);
	OutputText(hwndCtl, g_szCRLF);

	GlobalFreePtr(lpData);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpAuthorInfo(HWND hwndCtl, HANDLE hFile)
{
	SSIZE_T nRet = 0;
	CHAR    szRead[ID_LEN];
	TCHAR   szOutput[OUTPUT_LEN];

	// AuthorInfo version
	DWORD dwAuthorVer = 0;
	if (!ReadNat32(hFile, &dwAuthorVer))
		return FALSE;

#ifndef _DEBUG
	if (dwAuthorVer > 0)
#endif
	{ // Display only if changed
		OutputTextFmt(hwndCtl, szOutput, TEXT("Author Version:\t%d"), dwAuthorVer);
		if (dwAuthorVer > 0) OutputText(hwndCtl, g_szAsterisk);
		OutputText(hwndCtl, g_szCRLF);
	}

	// Login
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Author Login:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Nick Name
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Author Nick:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
		OutputText(hwndCtl, szOutput);
	}

	// Zone
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Author Zone:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Extra Info
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Extra Info:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpIncludedPacksHeaders(HWND hwndCtl, HANDLE hFile, DWORD dwVersion)
{
	SSIZE_T nRet = 0;
	CHAR    szRead[ID_LEN];
	TCHAR   szOutput[OUTPUT_LEN];

	OutputText(hwndCtl, g_szSep0);

	// ContentsChecksum
	if (!DumpChecksum(hwndCtl, hFile, 32))
		return FALSE;

	// Package Name
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Pack Name:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
		OutputText(hwndCtl, szOutput);
	}

	// SAuthorInfo
	if (!DumpAuthorInfo(hwndCtl, hFile))
		return FALSE;

	// Manialink
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Manialink:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
		OutputText(hwndCtl, szOutput);
	}

	// Creation Date
	FILETIME ftDate = {0};
	SYSTEMTIME stDate = {0};
	if (!ReadData(hFile, (LPVOID)&ftDate, 8))
		return FALSE;

	if ((ftDate.dwLowDateTime != 0 || ftDate.dwHighDateTime != 0) && FileTimeToSystemTime(&ftDate, &stDate))
		OutputTextFmt(hwndCtl, szOutput, TEXT("Creation Date:\t%02u-%02u-%02u %02u:%02u:%02u\r\n"),
			stDate.wYear, stDate.wMonth, stDate.wDay, stDate.wHour, stDate.wMinute, stDate.wSecond);

	// Package Name
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Pack Name:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
		OutputText(hwndCtl, szOutput);
	}

	if (dwVersion >= 11)
	{
		// Include Depth
		DWORD dwIncludeDepth = 0;
		if (!ReadNat32(hFile, &dwIncludeDepth))
			return FALSE;

		OutputTextFmt(hwndCtl, szOutput, TEXT("Include Depth:\t%d\r\n"), dwIncludeDepth);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpPackHeader(HWND hwndCtl, HANDLE hFile, DWORD dwVersion, DWORD dwHeaderMaxSize)
{
	SSIZE_T nRet = 0;
	CHAR    szRead[ID_LEN];
	TCHAR   szOutput[OUTPUT_LEN];

	OutputText(hwndCtl, g_szSep1);

	// Checksum
	if (!DumpChecksum(hwndCtl, hFile, 16))
		return FALSE;

	// Gbx Headers Start
	DWORD dwGbxHeadersStart = 0;
	if (!ReadNat32(hFile, &dwGbxHeadersStart))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Headers Start:\t%d\r\n"), dwGbxHeadersStart);

	// Data Start
	DWORD dwDataStart = dwHeaderMaxSize;

	if (dwVersion < 15)
	{
		if (!ReadNat32(hFile, &dwDataStart))
			return FALSE;
	}

	OutputTextFmt(hwndCtl, szOutput, TEXT("Data Start:\t%d\r\n"), dwDataStart);

	if (dwVersion >= 2)
	{
		// Gbx Headers Size
		DWORD dwGbxHeadersSize = 0;
		if (!ReadNat32(hFile, &dwGbxHeadersSize))
			return FALSE;

		OutputTextFmt(hwndCtl, szOutput, TEXT("Headers Size:\t%d\r\n"), dwGbxHeadersSize);

		// Gbx Headers Compressed Size
		DWORD dwGbxHeadersComprSize = 0;
		if (!ReadNat32(hFile, &dwGbxHeadersComprSize))
			return FALSE;

		OutputTextFmt(hwndCtl, szOutput, TEXT("Hdr Compr Size:\t%d\r\n"), dwGbxHeadersComprSize);
	}

	if (dwVersion >= 14)
	{
		// Skip unknown Nat128
		if (!FileSeekCurrent(hFile, 16))
			return FALSE;
	}

	if (dwVersion >= 16)
	{
		// File Size
		DWORD dwFileSize = 0;
		if (!ReadNat32(hFile, &dwFileSize))
			return FALSE;

		OutputTextFmt(hwndCtl, szOutput, TEXT("File Size:\t%d\r\n"), dwFileSize);
	}

	if (dwVersion >= 3)
	{
		// Skip unknown Nat128
		if (!FileSeekCurrent(hFile, 16))
			return FALSE;
	}

	if (dwVersion == 6)
	{
		// SAuthorInfo
		if (!DumpAuthorInfo(hwndCtl, hFile))
			return FALSE;
	}

	// Flags
	DWORD dwFlags = 0;
	if (!ReadNat32(hFile, &dwFlags))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Flags:\t\t%08X\r\n"), dwFlags);

	// Num Folders
	DWORD dwNumFolders = 0;
	if (!ReadNat32(hFile, &dwNumFolders) || dwNumFolders > 0x10000000)
		return FALSE;

	OutputText(hwndCtl, g_szSep0);
	OutputTextFmt(hwndCtl, szOutput, TEXT("Num Folders:\t%d\r\n"), dwNumFolders);
	OutputText(hwndCtl, g_szSep0);

	while (dwNumFolders--)
	{
		// Folder Index Parent
		DWORD dwFolderIndex = 0;
		if (!ReadNat32(hFile, &dwFolderIndex))
			return FALSE;

		OutputTextFmt(hwndCtl, szOutput, TEXT("Folder Index:\t%d\r\n"), dwFolderIndex);

		// Folder Name
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("Folder Name:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}
	}

	// Num Files
	DWORD dwNumFiles = 0;
	if (!ReadNat32(hFile, &dwNumFiles) || dwNumFiles > 0x10000000)
		return FALSE;

	OutputText(hwndCtl, g_szSep0);
	OutputTextFmt(hwndCtl, szOutput, TEXT("Num Files:\t%d\r\n"), dwNumFiles);

	while (dwNumFiles--)
	{
		OutputText(hwndCtl, g_szSep0);

		// Folder index
		DWORD dwFolderIndex = 0;
		if (!ReadNat32(hFile, &dwFolderIndex))
			return FALSE;

		OutputTextFmt(hwndCtl, szOutput, TEXT("Folder Index:\t%d\r\n"), dwFolderIndex);

		// File name
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("File Name:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}

		// Unknown
		DWORD dwUnknown = 0;
		if (!ReadNat32(hFile, &dwUnknown))
			return FALSE;

		OutputTextFmt(hwndCtl, szOutput, TEXT("Unknown:\t%d\r\n"), dwUnknown);

		// Uncompressed Size
		DWORD dwUncompressedSize = 0;
		if (!ReadNat32(hFile, &dwUncompressedSize))
			return FALSE;

		OutputTextFmt(hwndCtl, szOutput, TEXT("Uncompr Size:\t%d\r\n"), dwUncompressedSize);

		// Compressed Size
		DWORD dwCompressedSize = 0;
		if (!ReadNat32(hFile, &dwCompressedSize))
			return FALSE;

		OutputTextFmt(hwndCtl, szOutput, TEXT("Compr Size:\t%d\r\n"), dwCompressedSize);

		// Offset
		DWORD dwOffset = 0;
		if (!ReadNat32(hFile, &dwOffset))
			return FALSE;

		OutputTextFmt(hwndCtl, szOutput, TEXT("Offset:\t\t%d\r\n"), dwOffset);

		// Class ID
		DWORD dwClassId = 0;
		if (!ReadMask(hFile, &dwClassId))
			return FALSE;

		OutputTextFmt(hwndCtl, szOutput, TEXT("Class ID:\t%08X"), dwClassId);
		switch (dwClassId)
		{
			case CLSID_FILETEXT_TM:
				OutputText(hwndCtl, TEXT(" (FileText)"));
				break;
			case CLSID_FILEDDS_TM:
				OutputText(hwndCtl, TEXT(" (FileDds)"));
				break;
			case CLSID_FILEPNG_TM:
				OutputText(hwndCtl, TEXT(" (FilePng)"));
				break;
			case CLSID_FILEJPG_TM:
				OutputText(hwndCtl, TEXT(" (FileJpg)"));
				break;
			case CLSID_FILEWAV_TM:
				OutputText(hwndCtl, TEXT(" (FileWav)"));
				break;
			case CLSID_FILEOGGVORBIS_TMF:
				OutputText(hwndCtl, TEXT(" (FileOggVorbis)"));
				break;
			case CLSID_FILEWEBM_MP:
				OutputText(hwndCtl, TEXT(" (FileWebM)"));
				break;
			case CLSID_FILEPACK_TM:
				OutputText(hwndCtl, TEXT(" (FilePack)"));
				break;
		}
		OutputText(hwndCtl, g_szCRLF);

		if (dwVersion >= 17)
		{
			// Size
			DWORD dwSize = 0;
			if (!ReadNat32(hFile, &dwSize))
				return FALSE;

			OutputTextFmt(hwndCtl, szOutput, TEXT("Size:\t\t%d\r\n"), dwSize);
		}

		if (dwVersion >= 14)
		{
			// Checksum
			if (!DumpChecksum(hwndCtl, hFile, 16))
				return FALSE;
		}

		// SFileDescFlags
		ULARGE_INTEGER ulFlags = {0};
		if (!ReadNat64(hFile, &ulFlags))
			return FALSE;

		OutputTextFmt(hwndCtl, szOutput, TEXT("Flags:\t\t%08X%08X"), ulFlags.HighPart, ulFlags.LowPart);

		PFILEDESCFLAGS pFileFlags = (PFILEDESCFLAGS)&ulFlags;

		szOutput[0] = g_chNil;

		if (pFileFlags->IsHashed)
		{
			if (szOutput[0] != g_chNil)
				_tcsncat(szOutput, g_szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("IsHashed"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}

		if (pFileFlags->PublishFid)
		{
			if (szOutput[0] != g_chNil)
				_tcsncat(szOutput, g_szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("PublishFid"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}

		if (pFileFlags->Compression)
		{
			if (szOutput[0] != g_chNil)
				_tcsncat(szOutput, g_szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("Compressed"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}

		if (pFileFlags->IsSeekable)
		{
			if (szOutput[0] != g_chNil)
				_tcsncat(szOutput, g_szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("IsSeekable"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}

		if (pFileFlags->_Unknown_)
		{
			if (szOutput[0] != g_chNil)
				_tcsncat(szOutput, g_szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("_Unknown_"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}

		if (pFileFlags->DontUseDummyWrite)
		{
			if (szOutput[0] != g_chNil)
				_tcsncat(szOutput, g_szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("DontUseDummyWrite"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}

		if (pFileFlags->OpaqueUserData)
		{
			if (szOutput[0] != g_chNil)
				_tcsncat(szOutput, g_szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("OpaqueUserData"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}

		if (pFileFlags->PublicFile)
		{
			if (szOutput[0] != g_chNil)
				_tcsncat(szOutput, g_szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("PublicFile"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}

		if (pFileFlags->ForceNoCrypt)
		{
			if (szOutput[0] != g_chNil)
				_tcsncat(szOutput, g_szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("ForceNoCrypt"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}

		if (pFileFlags->__Unused1__ || pFileFlags->__Unused2__)
		{
			if (szOutput[0] != g_chNil)
				_tcsncat(szOutput, g_szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, g_szAsterisk, _countof(szOutput) - _tcslen(szOutput) - 1);
		}

		if (szOutput[0] != g_chNil)
		{
			OutputText(hwndCtl, TEXT(" ("));
			OutputText(hwndCtl, szOutput);
			OutputText(hwndCtl, TEXT(")"));
		}

		OutputText(hwndCtl, g_szCRLF);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
