////////////////////////////////////////////////////////////////////////////////////////////////
// Archive.h - Copyright (c) 2010-2024 by Electron.
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

#define ID_DIM        8
#define ID_LEN        512

#define UNASSIGNED    0xFFFFFFFF

#define GET_INDEX(id) ((id) & 0x3FFFFFFF)
#define IS_NUMBER(id) (((id) & 0xC0000000) == 0)
#define IS_STRING(id) (((id) & 0xC0000000) != 0)
#define IS_UNASSIGNED(id) ((id) == UNASSIGNED)

////////////////////////////////////////////////////////////////////////////////////////////////

// ID name table
typedef struct _IDENTIFIER
{
	DWORD dwVersion;
	DWORD dwIndex;
	CHAR aszName[ID_DIM][ID_LEN];
} IDENTIFIER, *PIDENTIFIER, *LPIDENTIFIER;

////////////////////////////////////////////////////////////////////////////////////////////////

// Initializes and clears an ID name table
__inline void ResetIdentifier(PIDENTIFIER pIdList)
{
	pIdList->dwVersion = 0;
	pIdList->dwIndex = 0;
	for (int i = ID_DIM-1; i >= 0; i--)
		pIdList->aszName[i][0] = '\0';
}

////////////////////////////////////////////////////////////////////////////////////////////////

// Reads data from the specified file and checks whether the desired number of bytes has been read
BOOL ReadData(HANDLE hFile, LPVOID lpBuffer, SIZE_T cbSize);

// Reads a line of text from a file and returns it without the terminating newline characters
BOOL ReadLine(HANDLE hFile, PSTR pszString, SIZE_T cchStringLen);

// Reads a boolean 32-bit number
BOOL ReadBool(HANDLE hFile, LPBOOL lpbBool, BOOL bIsText = FALSE);

// Reads a 32 bit hexadecimal number
BOOL ReadMask(HANDLE hFile, LPDWORD lpdwMask, BOOL bIsText = FALSE);

// Reads a natural 8 bit number
BOOL ReadNat8(HANDLE hFile, LPBYTE lpcNat8, BOOL bIsText = FALSE);

// Reads a natural 16 bit number
BOOL ReadNat16(HANDLE hFile, LPWORD lpwNat16, BOOL bIsText = FALSE);

// Reads a natural 32 bit number
BOOL ReadNat32(HANDLE hFile, LPDWORD lpdwNat32, BOOL bIsText = FALSE);

// Reads a natural 64 bit number
BOOL ReadNat64(HANDLE hFile, PULARGE_INTEGER pullNat64, BOOL bIsText = FALSE);

// Reads a 128 bit data structure
BOOL ReadNat128(HANDLE hFile, LPVOID lpNat128, BOOL bIsText = FALSE);

// Reads a 256 bit data structure
BOOL ReadNat256(HANDLE hFile, LPVOID lpNat256, BOOL bIsText = FALSE);

// Reads a 32 bit integer
BOOL ReadInteger(HANDLE hFile, LPINT lpnInteger, BOOL bIsText = FALSE);

// Reads a 32 bit real number
BOOL ReadReal(HANDLE hFile, PFLOAT pfReal, BOOL bIsText = FALSE);

// Reads a Nadeo string from a file and copies it to the passed variable.
// Returns the number of characters read or -1 in case of a read error.
SSIZE_T ReadString(HANDLE hFile, PSTR pszString, SIZE_T cchStringLen, BOOL bIsText = FALSE);

// Reads an identifier from a file and adds the corresponding string to the given ID name table.
// Returns the number of characters read or -1 in case of a read error.
// Supports version 2 and 3 identifiers.
SSIZE_T ReadIdentifier(HANDLE hFile, PIDENTIFIER pIdTable, PSTR pszString, SIZE_T cchStringLen, PDWORD pdwId = NULL);

////////////////////////////////////////////////////////////////////////////////////////////////
