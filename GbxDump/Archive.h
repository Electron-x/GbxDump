////////////////////////////////////////////////////////////////////////////////////////////////
// Archive.h - Copyright (c) 2010-2019 by Electron.
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

typedef struct _IDENTIFIER
{
	DWORD dwVersion;
	DWORD dwIndex;
	CHAR aszList[ID_DIM][ID_LEN];
} IDENTIFIER, *PIDENTIFIER, *LPIDENTIFIER;

////////////////////////////////////////////////////////////////////////////////////////////////

__inline void ResetIdentifier(PIDENTIFIER pId)
{
	pId->dwVersion = 0;
	pId->dwIndex = 0;
	for (int i = ID_DIM-1; i >= 0; i--)
		pId->aszList[i][0] = '\0';
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ReadData(HANDLE hFile, LPVOID lpBuffer, SIZE_T cbSize);
BOOL ReadBool(HANDLE hFile, LPBOOL lpbBool, BOOL bIsText = FALSE);
BOOL ReadMask(HANDLE hFile, LPDWORD lpdwMask, BOOL bIsText = FALSE);

BOOL ReadNat8(HANDLE hFile, LPBYTE lpcNat8, BOOL bIsText = FALSE);
BOOL ReadNat16(HANDLE hFile, LPWORD lpwNat16, BOOL bIsText = FALSE);
BOOL ReadNat32(HANDLE hFile, LPDWORD lpdwNat32, BOOL bIsText = FALSE);
BOOL ReadNat64(HANDLE hFile, PULARGE_INTEGER pullNat64, BOOL bIsText = FALSE);
BOOL ReadNat128(HANDLE hFile, LPVOID lpNat128, BOOL bIsText = FALSE);
BOOL ReadNat256(HANDLE hFile, LPVOID lpNat256, BOOL bIsText = FALSE);

BOOL ReadInteger(HANDLE hFile, LPINT lpnInteger, BOOL bIsText = FALSE);
BOOL ReadReal(HANDLE hFile, PFLOAT pfReal, BOOL bIsText = FALSE);

SSIZE_T ReadString(HANDLE hFile, PSTR pszString, SIZE_T cchStringLen, BOOL bIsText = FALSE);
SSIZE_T ReadIdentifier(HANDLE hFile, PIDENTIFIER pId, PSTR pszString, SIZE_T cchStringLen);

////////////////////////////////////////////////////////////////////////////////////////////////
