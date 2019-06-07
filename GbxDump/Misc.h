////////////////////////////////////////////////////////////////////////////////////////////////
// Misc.h - Copyright (c) 2010-2019 by Electron.
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

__inline DWORD GetFilePointer(HANDLE hFile)
{ return SetFilePointer(hFile, 0, NULL, FILE_CURRENT); }
__inline BOOL FileSeekBegin(HANDLE hFile, LONG lDistanceToMove)
{ return (SetFilePointer(hFile, lDistanceToMove, NULL, FILE_BEGIN) != 0xFFFFFFFF); }
__inline BOOL FileSeekCurrent(HANDLE hFile, LONG lDistanceToMove)
{ return (SetFilePointer(hFile, lDistanceToMove, NULL, FILE_CURRENT) != 0xFFFFFFFF); }

void OutputText(HWND hwndCtl, LPCTSTR lpszOutput);
void OutputTextFmt(HWND hwndCtl, LPTSTR lpszOutput, LPCTSTR lpszFormat, ...);
BOOL OutputTextErr(HWND hwndCtl, UINT uID);
BOOL OutputErrorMessage(HWND hwndCtl, DWORD dwError);

PTCHAR AppendFlagName(LPTSTR lpszOutput, SIZE_T cchLenOutput, LPCTSTR lpszFlagName);

BOOL FormatByteSize(DWORD dwSize, LPTSTR lpszString, SIZE_T cchStringLen);
BOOL FormatTime(DWORD dwTime, LPTSTR lpszTime, SIZE_T cchStringLen, BOOL bFormat = TRUE);
BOOL ConvertGbxString(LPVOID lpData, SIZE_T cbLenData, LPTSTR lpszOutput, SIZE_T cchLenOutput, BOOL bCleanup = FALSE);
BOOL CleanupString(LPCTSTR lpszInput, LPTSTR lpszOutput, SIZE_T cchLenOutput);

LPTSTR AllocReplaceString(LPCTSTR lpszOriginal, LPCTSTR lpszPattern, LPCTSTR lpszReplacement);
LPTSTR AllocCleanupString(LPCTSTR lpszOriginal);

////////////////////////////////////////////////////////////////////////////////////////////////
