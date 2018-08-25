////////////////////////////////////////////////////////////////////////////////////////////////
// Archive.cpp - Copyright (c) 2010-2018 by Electron.
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
// TODO: Replace static string list with a dynamic list.
//
////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "archive.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module
//
BOOL ReadLine(HANDLE hFile, LPSTR lpszString, SIZE_T cchStringLen);
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
		CHAR achBuffer[12] = {0};
		if (!ReadLine(hFile, achBuffer, _countof(achBuffer)))
			return FALSE;

		if (lpdwMask != NULL)
			sscanf(achBuffer, "%x", lpdwMask);

		return TRUE;
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
		CHAR achBuffer[12] = {0};
		if (!ReadLine(hFile, achBuffer, _countof(achBuffer)))
			return FALSE;

		if (lpcNat8 != NULL)
			sscanf(achBuffer, "%hu", lpcNat8);

		return TRUE;
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
		CHAR achBuffer[12] = {0};
		if (!ReadLine(hFile, achBuffer, _countof(achBuffer)))
			return FALSE;

		if (lpwNat16 != NULL)
			sscanf(achBuffer, "%hu", lpwNat16);

		return TRUE;
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
		CHAR achBuffer[12] = {0};
		if (!ReadLine(hFile, achBuffer, _countof(achBuffer)))
			return FALSE;

		if (lpdwNat32 != NULL)
			sscanf(achBuffer, "%u", lpdwNat32);

		return TRUE;
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
		CHAR achBuffer[12] = {0};
		if (!ReadLine(hFile, achBuffer, _countof(achBuffer)))
			return FALSE;

		if (pullNat64 != NULL)
			sscanf(achBuffer, "%I64u", pullNat64);

		return TRUE;
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
		CHAR achBuffer[12] = {0};
		if (!ReadLine(hFile, achBuffer, _countof(achBuffer)))
			return FALSE;

		if (lpnInteger != NULL)
			sscanf(achBuffer, "%d", lpnInteger);

		return TRUE;
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
		_atoflt(&fltval, achBuffer);

		if (pfReal != NULL)
			*pfReal = fltval.f;

		return TRUE;
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
		LPVOID lpData = GlobalAllocPtr(GHND, dwLen + 1);
		if (lpData == NULL)
		{
			pszString[0] = '\0';
			return -1;
		}

		// Read the string
		if (!ReadData(hFile, lpData, dwLen))
		{
			GlobalFreePtr(lpData);
			return -1;
		}

		// Copy the read string into the return buffer
		strncpy(pszString, (LPSTR)lpData, cchStringLen);

		GlobalFreePtr(lpData);
	}

	return strlen(pszString);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Reads an identifier from a file and adds the corresponding string to the given list.
// Returns the number of characters read or -1 in case of a read error.
// Supports version 2 and 3 identifiers.

SSIZE_T ReadIdentifier(HANDLE hFile, PIDENTIFIER pId, PSTR pszString, SIZE_T cchStringLen)
{
	if (hFile == NULL || pId == NULL || pszString == NULL || cchStringLen == 0)
		return -1;

	if (pId->dwVersion < 3)
	{
		// Identifier version
		if (!ReadNat32(hFile, &pId->dwVersion))
			return -1;

		// Check identifier version
		if (pId->dwVersion < 2)
			return -1;
	}

	// Read index
	DWORD dwId = 0;
	if (!ReadData(hFile, (LPVOID)&dwId, 4))
		return -1;

	if (IS_UNASSIGNED(dwId))
	{ // Unassigned
		pszString[0] = '\0';
		return 0;
	}

	// Is the identifier a collection ID?
	if (!IS_STRING(dwId) && IS_NUMBER(dwId))
		return GetCollectionString(dwId, pszString, cchStringLen);

	// In version 2, the identifier is always available as a string
	if (pId->dwVersion == 2)
		return ReadString(hFile, pszString, cchStringLen);

	// Is the identifier available as a string?
	if (IS_STRING(dwId) && GET_INDEX(dwId) == 0)
	{
		// Read the string
		SSIZE_T cchLen = ReadString(hFile, pszString, cchStringLen);

		// Copy the string to the ID list and increment the index
		if (cchLen > 0 && pId->dwIndex < ID_DIM)
		{
			strncpy(pId->aszList[pId->dwIndex], pszString, _countof(pId->aszList[pId->dwIndex]));
			pId->dwIndex++;
		}

		return cchLen;
	}
	
	// Determine identifier index (delete topmost two MSBs)
	DWORD dwIndex = GET_INDEX(dwId);
	if (dwIndex == 0 || dwIndex > ID_DIM)
	{
		pszString[0] = '\0';
		return 0;
	}

	// Get the string from the ID list
	strncpy(pszString, pId->aszList[dwIndex-1], cchStringLen);

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
		case 11: // Valley
			strncpy(lpszCollection, "Valley", cchStringLen);
			break;
		case 12: // Canyon
			strncpy(lpszCollection, "Canyon", cchStringLen);
			break;
		case 13: // Lagoon
			strncpy(lpszCollection, "Lagoon", cchStringLen);
			break;
		case 14: // Arena
			strncpy(lpszCollection, "Arena", cchStringLen);
			break;
		case 15: // Test8
			strncpy(lpszCollection, "Test8", cchStringLen);
			break;
		case 16: // Test9
			strncpy(lpszCollection, "Test9", cchStringLen);
			break;
		case 17: // TMCommon
			strncpy(lpszCollection, "TMCommon", cchStringLen);
			break;
		case 100: // History
			strncpy(lpszCollection, "History", cchStringLen);
			break;
		case 101: // Society
			strncpy(lpszCollection, "Society", cchStringLen);
			break;
		case 102: // Galaxy
			strncpy(lpszCollection, "Galaxy", cchStringLen);
			break;
		case 103: // Test1
			strncpy(lpszCollection, "Test1", cchStringLen);
			break;
		case 104: // Test2
			strncpy(lpszCollection, "Test2", cchStringLen);
			break;
		case 105: // Test3
			strncpy(lpszCollection, "Test3", cchStringLen);
			break;
		case 200: // Gothic
			strncpy(lpszCollection, "Gothic", cchStringLen);
			break;
		case 201: // Paris
			strncpy(lpszCollection, "Paris", cchStringLen);
			break;
		case 202: // Storm
			strncpy(lpszCollection, "Storm", cchStringLen);
			break;
		case 203: // Cryo
			strncpy(lpszCollection, "Cryo", cchStringLen);
			break;
		case 204: // Meteor
			strncpy(lpszCollection, "Meteor", cchStringLen);
			break;
		case 205: // Test1
			strncpy(lpszCollection, "Test1", cchStringLen);
			break;
		case 206: // Test2
			strncpy(lpszCollection, "Test2", cchStringLen);
			break;
		case 207: // Test3
			strncpy(lpszCollection, "Test3", cchStringLen);
			break;
		case 299: // SMCommon
			strncpy(lpszCollection, "SMCommon", cchStringLen);
			break;
		case 10001: // Orbital
			strncpy(lpszCollection, "Orbital", cchStringLen);
			break;
		case 10002: // Actors
			strncpy(lpszCollection, "Actors", cchStringLen);
			break;
		case 10003: // Common
			strncpy(lpszCollection, "Common", cchStringLen);
			break;
		default:
			{
				char szId[32];
				_snprintf(szId, _countof(szId), "%d", dwId);
				if (cchStringLen > strlen(szId))
					strncpy(lpszCollection, szId, cchStringLen);
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
