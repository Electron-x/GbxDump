////////////////////////////////////////////////////////////////////////////////////////////////
// Archive.cpp - Copyright (c) 2010-2023 by Electron.
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
//
SIZE_T GetCollectionString(DWORD dwId, LPSTR lpszCollection, SIZE_T cchStringLen);

////////////////////////////////////////////////////////////////////////////////////////////////
// Reads data from a file and checks whether the desired number of bytes has been read

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
// Reads a line of text from a file and returns it without the terminating newline characters.
// Note: A input line ends with a CR LF combination. A single LF is part of a string.

BOOL ReadLine(HANDLE hFile, LPSTR lpszString, SIZE_T cchStringLen)
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

			if (lpszString != NULL && uPos < cchStringLen)
				lpszString[uPos] = '\0';

			return TRUE;
		}

		if (lpszString != NULL && uPos < cchStringLen)
			lpszString[uPos++] = ch;
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Reads a boolean 32-bit number

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
// Reads a 32 bit hexadecimal number

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
// Reads a natural 8 bit number

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
// Reads a natural 16 bit number

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
// Reads a natural 32 bit number

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
// Reads a natural 64 bit number

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
// Reads a 128 bit data structure

BOOL ReadNat128(HANDLE hFile, LPVOID lpdwNat128, BOOL bIsText)
{
	UNREFERENCED_PARAMETER(bIsText);

	return ReadData(hFile, lpdwNat128, 16);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Reads a 256 bit data structure

BOOL ReadNat256(HANDLE hFile, LPVOID lpdwNat256, BOOL bIsText)
{
	UNREFERENCED_PARAMETER(bIsText);

	return ReadData(hFile, lpdwNat256, 32);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Reads a 32 bit integer

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
// Reads a 32 bit real number

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
// Reads a Nadeo string from a file and copies it to the passed variable.
// Returns the number of characters read or -1 in case of a read error.

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
		lstrcpynA(pszString, (LPSTR)lpData, (int)cchStringLen);

		MyGlobalFreePtr(lpData);
	}

	return strlen(pszString);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Reads an identifier from a file and adds the corresponding string to the given list.
// Returns the number of characters read or -1 in case of a read error.
// Supports version 2 and 3 identifiers.

SSIZE_T ReadIdentifier(HANDLE hFile, PIDENTIFIER pIdList, PSTR pszString, SIZE_T cchStringLen, PDWORD pdwId)
{
	if (hFile == NULL || pIdList == NULL || pszString == NULL || cchStringLen == 0)
		return -1;

	if (pIdList->dwVersion < 3)
	{
		// Identifier version
		if (!ReadNat32(hFile, &pIdList->dwVersion))
			return -1;

		// Check identifier version
		if (pIdList->dwVersion < 2)
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
	if (pIdList->dwVersion == 2)
		return ReadString(hFile, pszString, cchStringLen);

	// Is the identifier available as a string?
	if (IS_STRING(dwId) && GET_INDEX(dwId) == 0)
	{
		// Read the string
		SSIZE_T CONST cchLen = ReadString(hFile, pszString, cchStringLen);

		// Copy the string to the ID list and increment the index
		if (cchLen > 0 && pIdList->dwIndex < ID_DIM)
		{
			lstrcpynA(pIdList->aszList[pIdList->dwIndex], pszString, _countof(pIdList->aszList[pIdList->dwIndex]));
			pIdList->dwIndex++;
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

	// Get the string from the ID list
	lstrcpynA(pszString, pIdList->aszList[dwIndex-1], (int)cchStringLen);

	return strlen(pszString);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Determines the corresponding name for a collection ID

SIZE_T GetCollectionString(DWORD dwId, LPSTR lpszCollection, SIZE_T cchStringLen)
{
	if (lpszCollection == NULL || cchStringLen == 0)
		return 0;

	switch (dwId)
	{
		case 0: // Speed
			lstrcpynA(lpszCollection, "Desert", (int)cchStringLen);
			break;
		case 1: // Alpine
			lstrcpynA(lpszCollection, "Snow", (int)cchStringLen);
			break;
		case 2: // Rally
			lstrcpynA(lpszCollection, "Rally", (int)cchStringLen);
			break;
		case 3: // Island
			lstrcpynA(lpszCollection, "Island", (int)cchStringLen);
			break;
		case 4: // Bay
			lstrcpynA(lpszCollection, "Bay", (int)cchStringLen);
			break;
		case 5: // Coast
			lstrcpynA(lpszCollection, "Coast", (int)cchStringLen);
			break;
		case 6: // StadiumMP4
			lstrcpynA(lpszCollection, "Stadium", (int)cchStringLen);
			break;
		case 7: // Basic
			lstrcpynA(lpszCollection, "Basic", (int)cchStringLen);
			break;
		case 8: // Plain
			lstrcpynA(lpszCollection, "Plain", (int)cchStringLen);
			break;
		case 9: // Moon
			lstrcpynA(lpszCollection, "Moon", (int)cchStringLen);
			break;
		case 10: // Toy
			lstrcpynA(lpszCollection, "Toy", (int)cchStringLen);
			break;
		case 11: // Valley
			lstrcpynA(lpszCollection, "Valley", (int)cchStringLen);
			break;
		case 12: // Canyon
			lstrcpynA(lpszCollection, "Canyon", (int)cchStringLen);
			break;
		case 13: // Lagoon
			lstrcpynA(lpszCollection, "Lagoon", (int)cchStringLen);
			break;
		case 14: // Deprecated_Arena
			lstrcpynA(lpszCollection, "Arena", (int)cchStringLen);
			break;
		case 15: // TMTest8
			lstrcpynA(lpszCollection, "TMTest8", (int)cchStringLen);
			break;
		case 16: // TMTest9
			lstrcpynA(lpszCollection, "TMTest9", (int)cchStringLen);
			break;
		case 17: // TMCommon
			lstrcpynA(lpszCollection, "TMCommon", (int)cchStringLen);
			break;
		case 18: // Canyon4
			lstrcpynA(lpszCollection, "Canyon4", (int)cchStringLen);
			break;
		case 19: // Canyon256
			lstrcpynA(lpszCollection, "Canyon256", (int)cchStringLen);
			break;
		case 20: // Valley4
			lstrcpynA(lpszCollection, "Valley4", (int)cchStringLen);
			break;
		case 21: // Valley256
			lstrcpynA(lpszCollection, "Valley256", (int)cchStringLen);
			break;
		case 22: // Lagoon4
			lstrcpynA(lpszCollection, "Lagoon4", (int)cchStringLen);
			break;
		case 23: // Lagoon256
			lstrcpynA(lpszCollection, "Lagoon256", (int)cchStringLen);
			break;
		case 24: // Stadium4
			lstrcpynA(lpszCollection, "Stadium4", (int)cchStringLen);
			break;
		case 25: // Stadium256
			lstrcpynA(lpszCollection, "Stadium256", (int)cchStringLen);
			break;
		case 26: // Stadium
			lstrcpynA(lpszCollection, "Stadium", (int)cchStringLen);
			break;
		case 27: // Voxel
			lstrcpynA(lpszCollection, "Voxel", (int)cchStringLen);
			break;
		case 100: // History
			lstrcpynA(lpszCollection, "History", (int)cchStringLen);
			break;
		case 101: // Society
			lstrcpynA(lpszCollection, "Society", (int)cchStringLen);
			break;
		case 102: // Galaxy
			lstrcpynA(lpszCollection, "Galaxy", (int)cchStringLen);
			break;
		case 103: // QMTest1
			lstrcpynA(lpszCollection, "QMTest1", (int)cchStringLen);
			break;
		case 104: // QMTest2
			lstrcpynA(lpszCollection, "QMTest2", (int)cchStringLen);
			break;
		case 105: // QMTest3
			lstrcpynA(lpszCollection, "QMTest3", (int)cchStringLen);
			break;
		case 200: // Gothic
			lstrcpynA(lpszCollection, "Gothic", (int)cchStringLen);
			break;
		case 201: // Paris
			lstrcpynA(lpszCollection, "Paris", (int)cchStringLen);
			break;
		case 202: // Storm
			lstrcpynA(lpszCollection, "Storm", (int)cchStringLen);
			break;
		case 203: // Cryo
			lstrcpynA(lpszCollection, "Cryo", (int)cchStringLen);
			break;
		case 204: // Meteor
			lstrcpynA(lpszCollection, "Meteor", (int)cchStringLen);
			break;
		case 205: // Meteor4
			lstrcpynA(lpszCollection, "Meteor4", (int)cchStringLen);
			break;
		case 206: // Meteor256
			lstrcpynA(lpszCollection, "Meteor256", (int)cchStringLen);
			break;
		case 207: // SMTest3
			lstrcpynA(lpszCollection, "SMTest3", (int)cchStringLen);
			break;
		case 299: // SMCommon
			lstrcpynA(lpszCollection, "SMCommon", (int)cchStringLen);
			break;
		case 10000: // Vehicles
			lstrcpynA(lpszCollection, "Vehicles", (int)cchStringLen);
			break;
		case 10001: // Orbital
			lstrcpynA(lpszCollection, "Orbital", (int)cchStringLen);
			break;
		case 10002: // Actors
			lstrcpynA(lpszCollection, "Actors", (int)cchStringLen);
			break;
		case 10003: // Common
			lstrcpynA(lpszCollection, "Common", (int)cchStringLen);
			break;
		case UNASSIGNED:
			lstrcpynA(lpszCollection, "_Unassigned", (int)cchStringLen);
			break;
		default:
			{
				char szId[32];
				_snprintf(szId, _countof(szId), "%u", dwId);
				szId[31] = '\0';
				if (cchStringLen > strlen(szId))
					lstrcpynA(lpszCollection, szId, (int)cchStringLen);
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
