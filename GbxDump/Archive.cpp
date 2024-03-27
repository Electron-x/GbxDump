////////////////////////////////////////////////////////////////////////////////////////////////
// Archive.cpp - Copyright (c) 2010-2024 by Electron.
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
// TODO: Replace static string list with a dynamic list.
//
////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "archive.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module

// Gets the name of a collection ID. Returns the length of this name.
SIZE_T GetCollectionString(DWORD dwId, LPSTR lpszCollection, SIZE_T cchStringLen);

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ReadData(HANDLE hFile, LPVOID lpBuffer, SIZE_T cbSize)
{
	if (hFile == NULL || lpBuffer == NULL)
		return FALSE;

	DWORD dwRead;
	if (!ReadFile(hFile, lpBuffer, (DWORD)cbSize, &dwRead, NULL) || dwRead != cbSize)
		return FALSE;

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ReadLine(HANDLE hFile, PSTR pszString, SIZE_T cchStringLen)
{
	if (hFile == NULL)
		return FALSE;

	int i = 0;
	CHAR ch = 0;
	SIZE_T uPos = 0;

	while (i++ <= 0xFFF)
	{
		if (!ReadData(hFile, &ch, 1))
			return FALSE;

		if (ch == 0xD)	// CR
		{
			if (!ReadData(hFile, &ch, 1))
				return FALSE;

			if (ch != 0xA)	// LF
				return FALSE;

			if (pszString != NULL && uPos < cchStringLen)
				pszString[uPos] = '\0';

			return TRUE;
		}

		if (pszString != NULL && uPos < cchStringLen)
			pszString[uPos++] = ch;
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ReadBool(HANDLE hFile, LPBOOL lpbBool, BOOL bIsText)
{
	if (!bIsText)
		return ReadData(hFile, lpbBool, 4);
	else
	{
		CHAR achBuffer[12] = {0};
		if (!ReadLine(hFile, achBuffer, _countof(achBuffer)))
			return FALSE;

		if (lpbBool != NULL)
			*lpbBool = (achBuffer[0] == 'T');	// "[T]rue"

		return TRUE;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ReadMask(HANDLE hFile, LPDWORD lpdwMask, BOOL bIsText)
{
	if (!bIsText)
		return ReadData(hFile, lpdwMask, 4);
	else
	{
		int nRet = 0;
		CHAR achBuffer[12] = {0};

		if (!ReadLine(hFile, achBuffer, _countof(achBuffer)))
			return FALSE;

		if (lpdwMask != NULL)
			nRet = sscanf(achBuffer, "%x", lpdwMask);

		return (nRet != 0 && nRet != EOF);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ReadNat8(HANDLE hFile, LPBYTE lpcNat8, BOOL bIsText)
{
	if (!bIsText)
		return ReadData(hFile, lpcNat8, 1);
	else
	{
		int nRet = 0;
		CHAR achBuffer[12] = {0};

		if (!ReadLine(hFile, achBuffer, _countof(achBuffer)))
			return FALSE;

		WORD wNat16 = 0;
		nRet = sscanf(achBuffer, "%hu", &wNat16);

		if (lpcNat8 != NULL)
			*lpcNat8 = LOBYTE(wNat16);

		return (nRet != 0 && nRet != EOF);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ReadNat16(HANDLE hFile, LPWORD lpwNat16, BOOL bIsText)
{
	if (!bIsText)
		return ReadData(hFile, lpwNat16, 2);
	else
	{
		int nRet = 0;
		CHAR achBuffer[12] = {0};

		if (!ReadLine(hFile, achBuffer, _countof(achBuffer)))
			return FALSE;

		if (lpwNat16 != NULL)
			nRet = sscanf(achBuffer, "%hu", lpwNat16);

		return (nRet != 0 && nRet != EOF);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ReadNat32(HANDLE hFile, LPDWORD lpdwNat32, BOOL bIsText)
{
	if (!bIsText)
		return ReadData(hFile, lpdwNat32, 4);
	else
	{
		int nRet = 0;
		CHAR achBuffer[12] = {0};

		if (!ReadLine(hFile, achBuffer, _countof(achBuffer)))
			return FALSE;

		if (lpdwNat32 != NULL)
			nRet = sscanf(achBuffer, "%u", lpdwNat32);

		return (nRet != 0 && nRet != EOF);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ReadNat64(HANDLE hFile, PULARGE_INTEGER pullNat64, BOOL bIsText)
{
	if (!bIsText)
		return ReadData(hFile, pullNat64, 8);
	else
	{
		int nRet = 0;
		CHAR achBuffer[12] = {0};

		if (!ReadLine(hFile, achBuffer, _countof(achBuffer)))
			return FALSE;

		if (pullNat64 != NULL)
			nRet = sscanf(achBuffer, "%I64u", &pullNat64->QuadPart);

		return (nRet != 0 && nRet != EOF);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ReadNat128(HANDLE hFile, LPVOID lpdwNat128, BOOL bIsText)
{
	UNREFERENCED_PARAMETER(bIsText);

	return ReadData(hFile, lpdwNat128, 16);
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ReadNat256(HANDLE hFile, LPVOID lpdwNat256, BOOL bIsText)
{
	UNREFERENCED_PARAMETER(bIsText);

	return ReadData(hFile, lpdwNat256, 32);
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ReadInteger(HANDLE hFile, LPINT lpnInteger, BOOL bIsText)
{
	if (!bIsText)
		return ReadData(hFile, lpnInteger, 4);
	else
	{
		int nRet = 0;
		CHAR achBuffer[12] = {0};

		if (!ReadLine(hFile, achBuffer, _countof(achBuffer)))
			return FALSE;

		if (lpnInteger != NULL)
			nRet = sscanf(achBuffer, "%d", lpnInteger);

		return (nRet != 0 && nRet != EOF);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ReadReal(HANDLE hFile, PFLOAT pfReal, BOOL bIsText)
{
	if (!bIsText)
		return ReadData(hFile, pfReal, 4);
	else
	{
		CHAR achBuffer[12] = {0};
		if (!ReadLine(hFile, achBuffer, _countof(achBuffer)))
			return FALSE;

		_CRT_FLOAT fltval;
		int nRet = _atoflt(&fltval, achBuffer);

		if (pfReal != NULL)
			*pfReal = fltval.f;

		return (nRet == 0);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

SSIZE_T ReadString(HANDLE hFile, PSTR pszString, SIZE_T cchStringLen, BOOL bIsText)
{
	if (hFile == NULL || pszString == NULL || cchStringLen == 0)
		return -1;

	if (bIsText)
	{
		if (!ReadLine(hFile, pszString, cchStringLen))
			return -1;
	}
	else
	{
		// Read the string length
		DWORD dwLen = 0;
		if (!ReadData(hFile, (LPVOID)&dwLen, 4) || dwLen >= 0xFFFF)
			return -1;

		if (dwLen == 0)
		{
			pszString[0] = '\0';
			return 0;
		}

		// Allocate memory for the string (1 additional character for the terminating zero)
		LPVOID lpData = MyGlobalAllocPtr(GHND, (SIZE_T)dwLen + 1);
		if (lpData == NULL)
		{
			pszString[0] = '\0';
			return -1;
		}

		// Read the string
		if (!ReadData(hFile, lpData, dwLen))
		{
			MyGlobalFreePtr(lpData);
			return -1;
		}

		// Copy the read string into the return buffer
		MyStrNCpyA(pszString, (LPSTR)lpData, (int)cchStringLen);

		MyGlobalFreePtr(lpData);
	}

	return strlen(pszString);
}

////////////////////////////////////////////////////////////////////////////////////////////////

SSIZE_T ReadIdentifier(HANDLE hFile, PIDENTIFIER pIdTable, PSTR pszString, SIZE_T cchStringLen, PDWORD pdwId)
{
	if (hFile == NULL || pIdTable == NULL || pszString == NULL || cchStringLen == 0)
		return -1;

	if (pIdTable->dwVersion < 3)
	{
		// Identifier version
		if (!ReadNat32(hFile, &pIdTable->dwVersion))
			return -1;

		// Check identifier version
		if (pIdTable->dwVersion < 2)
			return -1;
	}

	// Read identifier
	DWORD dwId = 0;
	if (!ReadData(hFile, (LPVOID)&dwId, 4))
		return -1;

	if (pdwId != NULL)
		*pdwId = dwId;

	if (IS_UNASSIGNED(dwId))
	{ // Unassigned
		pszString[0] = '\0';
		return 0;
	}

	// Is the identifier a collection ID?
	if (IS_NUMBER(dwId))
		return GetCollectionString(dwId, pszString, cchStringLen);

	// In version 2, the identifier is always available as a string
	if (pIdTable->dwVersion == 2)
		return ReadString(hFile, pszString, cchStringLen);

	// Is the identifier available as a string?
	if (IS_STRING(dwId) && GET_INDEX(dwId) == 0)
	{
		// Read the string
		SSIZE_T CONST cchLen = ReadString(hFile, pszString, cchStringLen);

		// Copy the string to the ID name table and increment the index
		if (cchLen > 0 && pIdTable->dwIndex < ID_DIM)
		{
			MyStrNCpyA(pIdTable->aszName[pIdTable->dwIndex], pszString, _countof(pIdTable->aszName[pIdTable->dwIndex]));
			pIdTable->dwIndex++;
		}

		return cchLen;
	}

	// Determine identifier index (delete topmost two MSBs)
	DWORD CONST dwIndex = GET_INDEX(dwId);
	if (dwIndex == 0 || dwIndex > ID_DIM)
	{
		pszString[0] = '\0';
		return 0;
	}

	// Get the string from the ID name table
	MyStrNCpyA(pszString, pIdTable->aszName[dwIndex-1], (int)cchStringLen);

	return strlen(pszString);
}

////////////////////////////////////////////////////////////////////////////////////////////////

SIZE_T GetCollectionString(DWORD dwId, LPSTR lpszCollection, SIZE_T cchStringLen)
{
	if (lpszCollection == NULL || cchStringLen == 0)
		return 0;

	switch (dwId)
	{
		case 0: // Speed
			MyStrNCpyA(lpszCollection, "Desert", (int)cchStringLen);
			break;
		case 1: // Alpine
			MyStrNCpyA(lpszCollection, "Snow", (int)cchStringLen);
			break;
		case 2: // Rally
			MyStrNCpyA(lpszCollection, "Rally", (int)cchStringLen);
			break;
		case 3: // Island
			MyStrNCpyA(lpszCollection, "Island", (int)cchStringLen);
			break;
		case 4: // Bay
			MyStrNCpyA(lpszCollection, "Bay", (int)cchStringLen);
			break;
		case 5: // Coast
			MyStrNCpyA(lpszCollection, "Coast", (int)cchStringLen);
			break;
		case 6: // StadiumMP4
			MyStrNCpyA(lpszCollection, "Stadium", (int)cchStringLen);
			break;
		case 7: // Basic
			MyStrNCpyA(lpszCollection, "Basic", (int)cchStringLen);
			break;
		case 8: // Plain
			MyStrNCpyA(lpszCollection, "Plain", (int)cchStringLen);
			break;
		case 9: // Moon
			MyStrNCpyA(lpszCollection, "Moon", (int)cchStringLen);
			break;
		case 10: // Toy
			MyStrNCpyA(lpszCollection, "Toy", (int)cchStringLen);
			break;
		case 11: // Valley
			MyStrNCpyA(lpszCollection, "Valley", (int)cchStringLen);
			break;
		case 12: // Canyon
			MyStrNCpyA(lpszCollection, "Canyon", (int)cchStringLen);
			break;
		case 13: // Lagoon
			MyStrNCpyA(lpszCollection, "Lagoon", (int)cchStringLen);
			break;
		case 14: // Deprecated_Arena
			MyStrNCpyA(lpszCollection, "Arena", (int)cchStringLen);
			break;
		case 15: // TMTest8
			MyStrNCpyA(lpszCollection, "TMTest8", (int)cchStringLen);
			break;
		case 16: // TMTest9
			MyStrNCpyA(lpszCollection, "TMTest9", (int)cchStringLen);
			break;
		case 17: // TMCommon
			MyStrNCpyA(lpszCollection, "TMCommon", (int)cchStringLen);
			break;
		case 18: // Canyon4
			MyStrNCpyA(lpszCollection, "Canyon4", (int)cchStringLen);
			break;
		case 19: // Canyon256
			MyStrNCpyA(lpszCollection, "Canyon256", (int)cchStringLen);
			break;
		case 20: // Valley4
			MyStrNCpyA(lpszCollection, "Valley4", (int)cchStringLen);
			break;
		case 21: // Valley256
			MyStrNCpyA(lpszCollection, "Valley256", (int)cchStringLen);
			break;
		case 22: // Lagoon4
			MyStrNCpyA(lpszCollection, "Lagoon4", (int)cchStringLen);
			break;
		case 23: // Lagoon256
			MyStrNCpyA(lpszCollection, "Lagoon256", (int)cchStringLen);
			break;
		case 24: // Stadium4
			MyStrNCpyA(lpszCollection, "Stadium4", (int)cchStringLen);
			break;
		case 25: // Stadium256
			MyStrNCpyA(lpszCollection, "Stadium256", (int)cchStringLen);
			break;
		case 26: // Stadium
			MyStrNCpyA(lpszCollection, "Stadium", (int)cchStringLen);
			break;
		case 27: // Voxel
			MyStrNCpyA(lpszCollection, "Voxel", (int)cchStringLen);
			break;
		case 100: // History
			MyStrNCpyA(lpszCollection, "History", (int)cchStringLen);
			break;
		case 101: // Society
			MyStrNCpyA(lpszCollection, "Society", (int)cchStringLen);
			break;
		case 102: // Galaxy
			MyStrNCpyA(lpszCollection, "Galaxy", (int)cchStringLen);
			break;
		case 103: // QMTest1
			MyStrNCpyA(lpszCollection, "QMTest1", (int)cchStringLen);
			break;
		case 104: // QMTest2
			MyStrNCpyA(lpszCollection, "QMTest2", (int)cchStringLen);
			break;
		case 105: // QMTest3
			MyStrNCpyA(lpszCollection, "QMTest3", (int)cchStringLen);
			break;
		case 200: // Gothic
			MyStrNCpyA(lpszCollection, "Gothic", (int)cchStringLen);
			break;
		case 201: // Paris
			MyStrNCpyA(lpszCollection, "Paris", (int)cchStringLen);
			break;
		case 202: // Storm
			MyStrNCpyA(lpszCollection, "Storm", (int)cchStringLen);
			break;
		case 203: // Cryo
			MyStrNCpyA(lpszCollection, "Cryo", (int)cchStringLen);
			break;
		case 204: // Meteor
			MyStrNCpyA(lpszCollection, "Meteor", (int)cchStringLen);
			break;
		case 205: // Meteor4
			MyStrNCpyA(lpszCollection, "Meteor4", (int)cchStringLen);
			break;
		case 206: // Meteor256
			MyStrNCpyA(lpszCollection, "Meteor256", (int)cchStringLen);
			break;
		case 207: // SMTest3
			MyStrNCpyA(lpszCollection, "SMTest3", (int)cchStringLen);
			break;
		case 299: // SMCommon
			MyStrNCpyA(lpszCollection, "SMCommon", (int)cchStringLen);
			break;
		case 10000: // Vehicles
			MyStrNCpyA(lpszCollection, "Vehicles", (int)cchStringLen);
			break;
		case 10001: // Orbital
			MyStrNCpyA(lpszCollection, "Orbital", (int)cchStringLen);
			break;
		case 10002: // Actors
			MyStrNCpyA(lpszCollection, "Actors", (int)cchStringLen);
			break;
		case 10003: // Common
			MyStrNCpyA(lpszCollection, "Common", (int)cchStringLen);
			break;
		case UNASSIGNED:
			MyStrNCpyA(lpszCollection, "_Unassigned", (int)cchStringLen);
			break;
		default:
			{
				char szId[32];
				_snprintf(szId, _countof(szId), "%u", dwId);
				szId[31] = '\0';
				if (cchStringLen > strlen(szId))
					MyStrNCpyA(lpszCollection, szId, (int)cchStringLen);
				else
				{
					lpszCollection[0] = '\0';
					return 0;
				}
			}
	}

	return strlen(lpszCollection);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
