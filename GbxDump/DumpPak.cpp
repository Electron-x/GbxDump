////////////////////////////////////////////////////////////////////////////////////////////////
// DumpPak.cpp - Copyright (c) 2010-2024 by Electron.
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
// Based on information from https://wiki.xaseco.org/wiki/PAK
//
////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "classid.h"
#include "archive.h"
#include "dumppak.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Data Types

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

BOOL DumpChecksum(HWND hwndEdit, HANDLE hFile, SIZE_T cbLen);
BOOL DumpAuthorInfo(HWND hwndEdit, HANDLE hFile);
BOOL DumpIncludedPacksHeaders(HWND hwndEdit, HANDLE hFile, DWORD dwVersion);
BOOL DumpPackHeader(HWND hwndEdit, HANDLE hFile, DWORD dwVersion, DWORD dwHeaderMaxSize);

////////////////////////////////////////////////////////////////////////////////////////////////
// String Constants

const TCHAR g_chNil        = TEXT('\0');
const TCHAR g_szAsterisk[] = TEXT("*");
const TCHAR g_szCRLF[]     = TEXT("\r\n");

////////////////////////////////////////////////////////////////////////////////////////////////
// DumpPack is called by DumpFile from GbxDump.cpp

BOOL DumpPack(HWND hwndEdit, HANDLE hFile)
{
	SSIZE_T nRet = 0;
	DWORD   dwTxtSize = 0;
	DWORD   dwXmlSize = 0;
	LPVOID  lpDataTxt = NULL;
	LPVOID  lpDataXml = NULL;
	CHAR    szRead[ID_LEN];
	TCHAR   szOutput[OUTPUT_LEN];

	if (hwndEdit == NULL || hFile == NULL)
		return FALSE;

	// Skip the file signature (already checked in DumpFile())
	if (!FileSeekBegin(hFile, 8))
		return FALSE;

	// Version
	DWORD dwVersion = 0;
	if (!ReadNat32(hFile, &dwVersion))
		return FALSE;

	OutputText(hwndEdit, g_szSep1);
	OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Pack Version:\t%d"), dwVersion);
	if (dwVersion > 18) OutputText(hwndEdit, g_szAsterisk);
	OutputText(hwndEdit, g_szCRLF);

	if (dwVersion < 6)
	{ // Packs with version < 6 don't have a Pack header
		OutputTextErr(hwndEdit, g_bGerUI ? IDP_GER_ERR_UNAVAIL : IDP_ENG_ERR_UNAVAIL);
		return TRUE;
	}

	// ContentsChecksum
	if (!DumpChecksum(hwndEdit, hFile, 32))
		return FALSE;

	// SHeaderFlagsUncrypt
	DWORD dwCryptFlags = 0;
	if (!ReadNat32(hFile, &dwCryptFlags))
		return FALSE;

	OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Decrypt Flags:\t%08X"), dwCryptFlags);

	PHEADERFLAGSUNCRYPT pCryptFlags = (PHEADERFLAGSUNCRYPT)&dwCryptFlags;

	szOutput[0] = g_chNil;

	if (pCryptFlags->IsHeaderPrivate)
		AppendFlagName(szOutput, _countof(szOutput), TEXT("IsHeaderPrivate"));

	if (pCryptFlags->UseDefaultHeaderKey)
		AppendFlagName(szOutput, _countof(szOutput), TEXT("UseDefaultHeaderKey"));

	if (pCryptFlags->IsDataPrivate)
		AppendFlagName(szOutput, _countof(szOutput), TEXT("IsDataPrivate"));

	if (pCryptFlags->IsImpostor)
		AppendFlagName(szOutput, _countof(szOutput), TEXT("IsImpostor"));

	if (pCryptFlags->__Unused__)
		AppendFlagName(szOutput, _countof(szOutput), g_szAsterisk);

	if (szOutput[0] != g_chNil)
	{
		OutputText(hwndEdit, TEXT(" ("));
		OutputText(hwndEdit, szOutput);
		OutputText(hwndEdit, TEXT(")"));
	}

	OutputText(hwndEdit, g_szCRLF);

	if (dwVersion < 7)
		return TRUE;

	// Header Max Size
	DWORD dwHeaderMaxSize = 0;
	if (dwVersion >= 15)
	{
		if (!ReadNat32(hFile, &dwHeaderMaxSize))
			return FALSE;

		OutputText(hwndEdit, TEXT("Header Size:\t"));
		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("%08X"), dwHeaderMaxSize);

		if (FormatByteSize(dwHeaderMaxSize, szOutput, _countof(szOutput)))
		{
			OutputText(hwndEdit, TEXT(" ("));
			OutputText(hwndEdit, szOutput);
			OutputText(hwndEdit, TEXT(")"));
		}

		OutputText(hwndEdit, g_szCRLF);
	}

	// SAuthorInfo
	if (!DumpAuthorInfo(hwndEdit, hFile))
		return FALSE;

	if (dwVersion < 9)
	{
		// Comment
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndEdit, TEXT("Comment:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
			OutputText(hwndEdit, szOutput);
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
			OutputText(hwndEdit, TEXT("Build Info:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndEdit, szOutput);
		}

		// URL
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndEdit, TEXT("URL:\t\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndEdit, szOutput);
		}

		return TRUE;
	}

	// Manialink
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndEdit, TEXT("Manialink:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
		OutputText(hwndEdit, szOutput);
	}

	if (dwVersion >= 13)
	{
		// Download URL
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndEdit, TEXT("Download URL:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndEdit, szOutput);
		}
	}

	// Creation time
	FILETIME ftDate = {0};
	SYSTEMTIME stDate = {0};
	if (!ReadData(hFile, (LPVOID)&ftDate, 8))
		return FALSE;

	if ((ftDate.dwLowDateTime != 0 || ftDate.dwHighDateTime != 0) && FileTimeToSystemTime(&ftDate, &stDate))
		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Creation Date:\t%02u-%02u-%02u %02u:%02u:%02u\r\n"),
			stDate.wYear, stDate.wMonth, stDate.wDay, stDate.wHour, stDate.wMinute, stDate.wSecond);

	// Comment
	if (!ReadNat32(hFile, &dwTxtSize))
		return FALSE;
	if (dwTxtSize > 0xFFFF) // sanity check
		return FALSE;

	if (dwTxtSize > 0)
	{
		lpDataTxt = MyGlobalAllocPtr(GHND, (SIZE_T)dwTxtSize + 1); // 1 more character for terminating zero
		if (lpDataTxt == NULL)
			return FALSE;

		if (!ReadData(hFile, lpDataTxt, dwTxtSize))
		{
			MyGlobalFreePtr(lpDataTxt);
			return FALSE;
		}
	}

	if (dwVersion >= 12)
	{
		// XML
		if (!ReadNat32(hFile, &dwXmlSize))
		{
			if (lpDataTxt != NULL)
				MyGlobalFreePtr(lpDataTxt);
			return FALSE;
		}
		if (dwXmlSize > 0xFFFF) // sanity check
		{
			if (lpDataTxt != NULL)
				MyGlobalFreePtr(lpDataTxt);
			return FALSE;
		}

		if (dwXmlSize > 0)
		{
			lpDataXml = MyGlobalAllocPtr(GHND, (SIZE_T)dwXmlSize + 1); // 1 more character for terminating zero
			if (lpDataXml == NULL)
			{
				if (lpDataTxt != NULL)
					MyGlobalFreePtr(lpDataTxt);
				return FALSE;
			}

			if (!ReadData(hFile, lpDataXml, dwXmlSize))
			{
				MyGlobalFreePtr(lpDataXml);
				if (lpDataTxt != NULL)
					MyGlobalFreePtr(lpDataTxt);
				return FALSE;
			}
		}

		// Title ID
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		{
			if (lpDataXml != NULL)
				MyGlobalFreePtr(lpDataXml);
			if (lpDataTxt != NULL)
				MyGlobalFreePtr(lpDataTxt);
			return FALSE;
		}

		if (nRet > 0)
		{
			OutputText(hwndEdit, TEXT("Title ID:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndEdit, szOutput);
		}
	}

	// Usage/SubDir
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
	{
		if (lpDataXml != NULL)
			MyGlobalFreePtr(lpDataXml);
		if (lpDataTxt != NULL)
			MyGlobalFreePtr(lpDataTxt);
		return FALSE;
	}

	if (nRet > 0)
	{
		OutputText(hwndEdit, TEXT("Pack Type:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndEdit, szOutput);
	}

	// CreationBuildInfo
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
	{
		if (lpDataXml != NULL)
			MyGlobalFreePtr(lpDataXml);
		if (lpDataTxt != NULL)
			MyGlobalFreePtr(lpDataTxt);
		return FALSE;
	}

	if (nRet > 0)
	{
		OutputText(hwndEdit, TEXT("Build Info:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndEdit, szOutput);
	}

	// Jump to the included packages (skip unused 16 bytes)
	if (!FileSeekCurrent(hFile, 16))
	{
		if (lpDataXml != NULL)
			MyGlobalFreePtr(lpDataXml);
		if (lpDataTxt != NULL)
			MyGlobalFreePtr(lpDataTxt);
		return FALSE;
	}

	if (dwVersion >= 10)
	{
		// Number of included packages
		DWORD dwNumIncludedPacks = 0;
		if (!ReadNat32(hFile, &dwNumIncludedPacks) || dwNumIncludedPacks > 0x10000000)
		{
			if (lpDataXml != NULL)
				MyGlobalFreePtr(lpDataXml);
			if (lpDataTxt != NULL)
				MyGlobalFreePtr(lpDataTxt);
			return FALSE;
		}

		OutputText(hwndEdit, g_szSep1);
		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Included Packs:\t%d\r\n"), dwNumIncludedPacks);

		while (dwNumIncludedPacks--)
		{
			// Included Packs Headers
			if (!DumpIncludedPacksHeaders(hwndEdit, hFile, dwVersion))
			{
				if (lpDataXml != NULL)
					MyGlobalFreePtr(lpDataXml);
				if (lpDataTxt != NULL)
					MyGlobalFreePtr(lpDataTxt);
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

		OutputText(hwndEdit, g_szSep1);
		ConvertGbxString(lpDataTxt, dwTxtSize, szOutput, _countof(szOutput), TRUE);
		OutputText(hwndEdit, szOutput);

		MyGlobalFreePtr(lpDataTxt);
	}

	if (lpDataXml != NULL)
	{ // Output XML
		// Allocate memory for Unicode
		LPVOID pXmlString = MyGlobalAllocPtr(GHND, 2 * ((SIZE_T)dwXmlSize + 3));
		if (pXmlString != NULL)
		{
			__try
			{
				// Convert to Unicode
				ConvertGbxString(lpDataXml, dwXmlSize, (LPTSTR)pXmlString, (SIZE_T)dwXmlSize + 3);

				// Insert line breaks
				LPCTSTR lpszTemp = AllocReplaceString((LPCTSTR)pXmlString, TEXT("><"), TEXT(">\r\n<"));

				OutputText(hwndEdit, g_szSep1);

				if (lpszTemp == NULL)
					OutputText(hwndEdit, (LPCTSTR)pXmlString);
				else
				{
					OutputText(hwndEdit, lpszTemp);
					MyGlobalFreePtr((LPVOID)lpszTemp);
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER) { ; }

			MyGlobalFreePtr(pXmlString);
		}

		MyGlobalFreePtr(lpDataXml);
	}

	if (pCryptFlags->IsHeaderPrivate || pCryptFlags->UseDefaultHeaderKey)
		return TRUE;

	return DumpPackHeader(hwndEdit, hFile, dwVersion, dwHeaderMaxSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpChecksum(HWND hwndEdit, HANDLE hFile, SIZE_T cbLen)
{
	LPVOID lpData = MyGlobalAllocPtr(GHND, cbLen);
	if (lpData == NULL)
		return FALSE;

	// Contents Checksum
	if (!ReadData(hFile, lpData, cbLen))
	{
		MyGlobalFreePtr(lpData);
		return FALSE;
	}

	TCHAR szOutput[OUTPUT_LEN];
	szOutput[0] = g_chNil;

	LPBYTE lpByte = (LPBYTE)lpData;
	cbLen = min(cbLen, _countof(szOutput)/2 - 1);
	while (cbLen--)
		_sntprintf(szOutput, _countof(szOutput), TEXT("%s%02X"), (LPTSTR)szOutput, (WORD)*lpByte++);

	OutputText(hwndEdit, TEXT("Checksum:\t"));
	OutputText(hwndEdit, szOutput);
	OutputText(hwndEdit, g_szCRLF);

	MyGlobalFreePtr(lpData);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpAuthorInfo(HWND hwndEdit, HANDLE hFile)
{
	SSIZE_T nRet = 0;
	CHAR    szRead[ID_LEN];
	TCHAR   szOutput[OUTPUT_LEN];

	// AuthorInfo version
	DWORD dwAuthorVer = 0;
	if (!ReadNat32(hFile, &dwAuthorVer))
		return FALSE;

	OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Author Version:\t%d"), dwAuthorVer);
	if (dwAuthorVer > 0) OutputText(hwndEdit, g_szAsterisk);
	OutputText(hwndEdit, g_szCRLF);

	// Login
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndEdit, TEXT("Author Login:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndEdit, szOutput);
	}

	// Nick Name
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndEdit, TEXT("Author Nick:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
		OutputText(hwndEdit, szOutput);
	}

	// Zone
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndEdit, TEXT("Author Zone:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndEdit, szOutput);
	}

	// Extra Info
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndEdit, TEXT("Extra Info:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndEdit, szOutput);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpIncludedPacksHeaders(HWND hwndEdit, HANDLE hFile, DWORD dwVersion)
{
	SSIZE_T nRet = 0;
	CHAR    szRead[ID_LEN];
	TCHAR   szOutput[OUTPUT_LEN];

	OutputText(hwndEdit, g_szSep0);

	// ContentsChecksum
	if (!DumpChecksum(hwndEdit, hFile, 32))
		return FALSE;

	// Package Name
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndEdit, TEXT("Pack Name:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
		OutputText(hwndEdit, szOutput);
	}

	// SAuthorInfo
	if (!DumpAuthorInfo(hwndEdit, hFile))
		return FALSE;

	// Manialink
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndEdit, TEXT("Manialink:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
		OutputText(hwndEdit, szOutput);
	}

	// Creation Date
	FILETIME ftDate = {0};
	SYSTEMTIME stDate = {0};
	if (!ReadData(hFile, (LPVOID)&ftDate, 8))
		return FALSE;

	if ((ftDate.dwLowDateTime != 0 || ftDate.dwHighDateTime != 0) && FileTimeToSystemTime(&ftDate, &stDate))
		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Creation Date:\t%02u-%02u-%02u %02u:%02u:%02u\r\n"),
			stDate.wYear, stDate.wMonth, stDate.wDay, stDate.wHour, stDate.wMinute, stDate.wSecond);

	// Package Name
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndEdit, TEXT("Pack Name:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
		OutputText(hwndEdit, szOutput);
	}

	if (dwVersion >= 11)
	{
		// Include Depth
		DWORD dwIncludeDepth = 0;
		if (!ReadNat32(hFile, &dwIncludeDepth))
			return FALSE;

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Include Depth:\t%d\r\n"), dwIncludeDepth);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpPackHeader(HWND hwndEdit, HANDLE hFile, DWORD dwVersion, DWORD dwHeaderMaxSize)
{
	SSIZE_T nRet = 0;
	CHAR    szRead[ID_LEN];
	TCHAR   szOutput[OUTPUT_LEN];

	OutputText(hwndEdit, g_szSep1);

	// Checksum
	if (!DumpChecksum(hwndEdit, hFile, 16))
		return FALSE;

	// Gbx Headers Start
	DWORD dwGbxHeadersStart = 0;
	if (!ReadNat32(hFile, &dwGbxHeadersStart))
		return FALSE;

	OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Headers Start:\t%d\r\n"), dwGbxHeadersStart);

	// Data Start
	DWORD dwDataStart = dwHeaderMaxSize;

	if (dwVersion < 15)
	{
		if (!ReadNat32(hFile, &dwDataStart))
			return FALSE;
	}

	OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Data Start:\t%d\r\n"), dwDataStart);

	if (dwVersion >= 2)
	{
		// Gbx Headers Size
		DWORD dwGbxHeadersSize = 0;
		if (!ReadNat32(hFile, &dwGbxHeadersSize))
			return FALSE;

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Headers Size:\t%d\r\n"), dwGbxHeadersSize);

		// Gbx Headers Compressed Size
		DWORD dwGbxHeadersComprSize = 0;
		if (!ReadNat32(hFile, &dwGbxHeadersComprSize))
			return FALSE;

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Hdr Compr Size:\t%d\r\n"), dwGbxHeadersComprSize);
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

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("File Size:\t%d\r\n"), dwFileSize);
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
		if (!DumpAuthorInfo(hwndEdit, hFile))
			return FALSE;
	}

	// Flags
	DWORD dwFlags = 0;
	if (!ReadNat32(hFile, &dwFlags))
		return FALSE;

	OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Flags:\t\t%08X\r\n"), dwFlags);

	// Num Folders
	DWORD dwNumFolders = 0;
	if (!ReadNat32(hFile, &dwNumFolders) || dwNumFolders > 0x10000000)
		return FALSE;

	OutputText(hwndEdit, g_szSep0);
	OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Num Folders:\t%d\r\n"), dwNumFolders);
	OutputText(hwndEdit, g_szSep0);

	while (dwNumFolders--)
	{
		// Folder Index Parent
		DWORD dwFolderIndex = 0;
		if (!ReadNat32(hFile, &dwFolderIndex))
			return FALSE;

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Folder Index:\t%d\r\n"), dwFolderIndex);

		// Folder Name
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndEdit, TEXT("Folder Name:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndEdit, szOutput);
		}
	}

	// Num Files
	DWORD dwNumFiles = 0;
	if (!ReadNat32(hFile, &dwNumFiles) || dwNumFiles > 0x10000000)
		return FALSE;

	OutputText(hwndEdit, g_szSep0);
	OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Num Files:\t%d\r\n"), dwNumFiles);

	while (dwNumFiles--)
	{
		OutputText(hwndEdit, g_szSep0);

		// Folder index
		DWORD dwFolderIndex = 0;
		if (!ReadNat32(hFile, &dwFolderIndex))
			return FALSE;

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Folder Index:\t%d\r\n"), dwFolderIndex);

		// File name
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndEdit, TEXT("File Name:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndEdit, szOutput);
		}

		// Unknown
		DWORD dwUnknown = 0;
		if (!ReadNat32(hFile, &dwUnknown))
			return FALSE;

		if (dwUnknown != UNASSIGNED) // Show only if set
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Unknown:\t%d\r\n"), dwUnknown);

		// Uncompressed Size
		DWORD dwUncompressedSize = 0;
		if (!ReadNat32(hFile, &dwUncompressedSize))
			return FALSE;

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Uncompr Size:\t%d\r\n"), dwUncompressedSize);

		// Compressed Size
		DWORD dwCompressedSize = 0;
		if (!ReadNat32(hFile, &dwCompressedSize))
			return FALSE;

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Compr Size:\t%d\r\n"), dwCompressedSize);

		// Offset
		DWORD dwOffset = 0;
		if (!ReadNat32(hFile, &dwOffset))
			return FALSE;

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Offset:\t\t%d\r\n"), dwOffset);

		// Class ID
		DWORD dwClassId = 0;
		if (!ReadMask(hFile, &dwClassId))
			return FALSE;

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Class ID:\t%08X"), dwClassId);
		switch (dwClassId)
		{
			case CLSID_FILETEXT_TM:
				OutputText(hwndEdit, TEXT(" (FileText)"));
				break;
			case CLSID_FILEDDS_TM:
				OutputText(hwndEdit, TEXT(" (FileDds)"));
				break;
			case CLSID_FILETGA_TM:
				OutputText(hwndEdit, TEXT(" (FileTga)"));
				break;
			case CLSID_FILEPNG_TM:
				OutputText(hwndEdit, TEXT(" (FilePng)"));
				break;
			case CLSID_FILEJPG_TM:
				OutputText(hwndEdit, TEXT(" (FileJpg)"));
				break;
			case CLSID_FILEWAV_TM:
				OutputText(hwndEdit, TEXT(" (FileWav)"));
				break;
			case CLSID_FILEOGGVORBIS_TMF:
				OutputText(hwndEdit, TEXT(" (FileOggVorbis)"));
				break;
			case CLSID_FILEWEBM_MP:
				OutputText(hwndEdit, TEXT(" (FileWebM)"));
				break;
			case CLSID_FILEPACK_TM:
				OutputText(hwndEdit, TEXT(" (FilePack)"));
				break;
			case CLSID_FILEZIP_TMF:
				OutputText(hwndEdit, TEXT(" (FileZip)"));
				break;
		}
		OutputText(hwndEdit, g_szCRLF);

		if (dwVersion >= 17)
		{
			// Size
			DWORD dwSize = 0;
			if (!ReadNat32(hFile, &dwSize))
				return FALSE;

			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Size:\t\t%d\r\n"), dwSize);
		}

		if (dwVersion >= 14)
		{
			// Checksum
			if (!DumpChecksum(hwndEdit, hFile, 16))
				return FALSE;
		}

		// SFileDescFlags
		ULARGE_INTEGER ullFlags = {0};
		if (!ReadNat64(hFile, &ullFlags))
			return FALSE;

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Flags:\t\t%08X%08X"), ullFlags.HighPart, ullFlags.LowPart);

		PFILEDESCFLAGS pFileFlags = (PFILEDESCFLAGS)&ullFlags;

		szOutput[0] = g_chNil;

		if (pFileFlags->IsHashed)
			AppendFlagName(szOutput, _countof(szOutput), TEXT("IsHashed"));

		if (pFileFlags->PublishFid)
			AppendFlagName(szOutput, _countof(szOutput), TEXT("PublishFid"));

		if (pFileFlags->Compression)
			AppendFlagName(szOutput, _countof(szOutput), TEXT("Compressed"));

		if (pFileFlags->IsSeekable)
			AppendFlagName(szOutput, _countof(szOutput), TEXT("IsSeekable"));

		if (pFileFlags->_Unknown_)
			AppendFlagName(szOutput, _countof(szOutput), TEXT("_Unknown_"));

		if (pFileFlags->DontUseDummyWrite)
			AppendFlagName(szOutput, _countof(szOutput), TEXT("DontUseDummyWrite"));

		if (pFileFlags->OpaqueUserData)
			AppendFlagName(szOutput, _countof(szOutput), TEXT("OpaqueUserData"));

		if (pFileFlags->PublicFile)
			AppendFlagName(szOutput, _countof(szOutput), TEXT("PublicFile"));

		if (pFileFlags->ForceNoCrypt)
			AppendFlagName(szOutput, _countof(szOutput), TEXT("ForceNoCrypt"));

		if (pFileFlags->__Unused1__ || pFileFlags->__Unused2__)
			AppendFlagName(szOutput, _countof(szOutput), g_szAsterisk);

		if (szOutput[0] != g_chNil)
		{
			OutputText(hwndEdit, TEXT(" ("));
			OutputText(hwndEdit, szOutput);
			OutputText(hwndEdit, TEXT(")"));
		}

		OutputText(hwndEdit, g_szCRLF);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
