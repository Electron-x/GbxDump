////////////////////////////////////////////////////////////////////////////////////////////////
// Misc.cpp - Copyright (c) 2010-2019 by Electron.
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

#include "stdafx.h"
#include "archive.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Inserts the passed text at the current cursor position of an edit control.
// Reduces many relocations to SendMessage.
void OutputText(HWND hwndCtl, LPCTSTR lpszOutput)
{
	if (hwndCtl != NULL && lpszOutput != NULL)
		Edit_ReplaceSel(hwndCtl, lpszOutput);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Formats text with wvsprintf and inserts it at the current cursor position of an edit control.

void OutputTextFmt(HWND hwndCtl, LPTSTR lpszOutput, LPCTSTR lpszFormat, ...)
{
	if (hwndCtl != NULL && lpszOutput != NULL && lpszFormat != NULL)
	{
		va_list arglist;
		va_start(arglist, lpszFormat);
		wvsprintf(lpszOutput, lpszFormat, arglist);
		va_end(arglist);

		Edit_ReplaceSel(hwndCtl, lpszOutput);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Loads an error message from the resources and inserts it with a separator
// line at the current cursor position of an edit control.

BOOL OutputTextErr(HWND hwndCtl, UINT uID)
{
	if (hwndCtl == NULL)
		return FALSE;

	TCHAR szOutput[OUTPUT_LEN];
	if (LoadString(g_hInstance, uID, szOutput, _countof(szOutput)) == 0)
		return FALSE;

	Edit_ReplaceSel(hwndCtl, g_szSep1);
	Edit_ReplaceSel(hwndCtl, szOutput);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Retrieves the message text for a system-defined error and inserts it
// with separator lines at the current cursor position of an edit control.

BOOL OutputErrorMessage(HWND hwndCtl, DWORD dwError)
{
	if (hwndCtl == NULL)
		return FALSE;

	LPVOID lpMsgBuf = NULL;
	TCHAR szOutput[OUTPUT_LEN];

	DWORD dwLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);

	if (dwLen > 0 && lpMsgBuf != NULL)
	{
		lstrcpyn(szOutput, (LPTSTR)lpMsgBuf, _countof(szOutput));
		OutputText(hwndCtl, g_szSep1);
		OutputText(hwndCtl, szOutput);
		LocalFree(lpMsgBuf);
	}

	OutputText(hwndCtl, g_szSep2);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Appends a string to another string with an OR separator. Is used to list flag names.

PTCHAR AppendFlagName(LPTSTR lpszOutput, SIZE_T cchLenOutput, LPCTSTR lpszFlagName)
{
	if (lpszOutput[0] != TEXT('\0'))
		_tcsncat(lpszOutput, TEXT("|"), cchLenOutput - _tcslen(lpszOutput) - 1);
	return _tcsncat(lpszOutput, lpszFlagName, cchLenOutput - _tcslen(lpszOutput) - 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Converts any byte value into a string of three digits.

BOOL FormatByteSize(DWORD dwSize, LPTSTR lpszString, SIZE_T cchStringLen)
{
	if (lpszString == NULL)
		return FALSE;

	if (dwSize < 1024)
		_sntprintf(lpszString, cchStringLen, TEXT("%u bytes"), dwSize);
	else if (dwSize < 10240)
		_sntprintf(lpszString, cchStringLen, TEXT("%.2f KB"), (double)(dwSize / 1024.0));
	else if (dwSize < 102400)
		_sntprintf(lpszString, cchStringLen, TEXT("%.1f KB"), (double)(dwSize / 1024.0));
	else if (dwSize < 1048576)
		_sntprintf(lpszString, cchStringLen, TEXT("%.0f KB"), (double)(dwSize / 1024.0));
	else if (dwSize < 10485760)
		_sntprintf(lpszString, cchStringLen, TEXT("%.2f MB"), (double)(dwSize / 1048576.0));
	else if (dwSize < 104857600)
		_sntprintf(lpszString, cchStringLen, TEXT("%.1f MB"), (double)(dwSize / 1048576.0));
	else if (dwSize < 1073741824)
		_sntprintf(lpszString, cchStringLen, TEXT("%.0f MB"), (double)(dwSize / 1048576.0));
	else
		_sntprintf(lpszString, cchStringLen, TEXT("%.2f GB"), (double)(dwSize / 1073741824.0));

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Converts a time value into a formatted string.

BOOL FormatTime(DWORD dwTime, LPTSTR lpszTime, SIZE_T cchStringLen, BOOL bFormat)
{
	if (lpszTime == NULL)
		return FALSE;

	if (bFormat && (dwTime != UNASSIGNED))
	{
		if (dwTime < 3600000)
		{
			DWORD dwMinute   = (dwTime / 60000);
			DWORD dwSecond   = (dwTime % 60000 / 1000);
			DWORD dwMilliSec = (dwTime % 60000 % 1000);

			_sntprintf(lpszTime, cchStringLen, TEXT("%u (%u:%02u.%003u)\r\n"),
				dwTime, dwMinute, dwSecond, dwMilliSec);
		}
		else
		{
			DWORD dwHour     = (dwTime / 3600000);
			DWORD dwMinute   = (dwTime % 3600000 / 60000);
			DWORD dwSecond   = (dwTime % 3600000 % 60000 / 1000);
			DWORD dwMilliSec = (dwTime % 3600000 % 60000 % 1000);

			_sntprintf(lpszTime, cchStringLen, TEXT("%u (%u:%02u:%02u.%003u)\r\n"),
				dwTime, dwHour, dwMinute, dwSecond, dwMilliSec);
		}
	}
	else
		_sntprintf(lpszTime, cchStringLen, TEXT("%d\r\n"), (int)dwTime);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Converts a Gbx string to Unicode and removes formatting characters.

BOOL ConvertGbxString(LPVOID lpData, SIZE_T cbLenData, LPTSTR lpszOutput, SIZE_T cchLenOutput, BOOL bCleanup)
{
	if (lpData == NULL || lpszOutput == NULL || cchLenOutput < cbLenData)
		return FALSE;

	UINT uCodePage = CP_ACP;
	if (IsValidCodePage(CP_UTF8))
		uCodePage = CP_UTF8;

	LPBYTE lpByte = (LPBYTE)lpData;
	if (cbLenData >= 3 && lpByte[0] == 0xEF && lpByte[1] == 0xBB && lpByte[2] == 0xBF)
		lpByte += 3; // Remove UTF8 Byte Order Mark

	MultiByteToWideChar(uCodePage, 0, (LPCSTR)lpByte, -1, lpszOutput, (int)(cchLenOutput-3));

	if (bCleanup)
	{
		SIZE_T cchLen = _tcslen(lpszOutput);
		LPTSTR lpszTemp = (LPTSTR)GlobalAllocPtr(GHND, cchLen * sizeof(TCHAR));
		if (lpszTemp != NULL)
		{
			if (CleanupString(lpszOutput, lpszTemp, cchLen))
			{
				_tcsncat(lpszOutput, TEXT(" ("), cchLenOutput-_tcslen(lpszOutput)-3);
				_tcsncat(lpszOutput, lpszTemp, cchLenOutput-_tcslen(lpszOutput)-3);
				_tcsncat(lpszOutput, TEXT(")"), cchLenOutput-_tcslen(lpszOutput)-3);
			}
			GlobalFreePtr((LPVOID)lpszTemp);
		}
	}

	_tcsncat(lpszOutput, TEXT("\r\n"), cchLenOutput-_tcslen(lpszOutput)-1);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Removes the Nadeo formatting characters from a string.

BOOL CleanupString(LPCTSTR lpszInput, LPTSTR lpszOutput, SIZE_T cchLenOutput)
{
	if (lpszInput == NULL || lpszOutput == NULL || cchLenOutput == 0)
		return FALSE;

	if (_tcschr(lpszInput, TEXT('$')) == NULL)
		return FALSE;

	__try
	{
		// Replace dollar signs with alert escape characters
		LPCTSTR lpszTemp1 = AllocReplaceString(lpszInput, TEXT("$$"), TEXT("\a"));
		if (lpszTemp1 == NULL)
			return FALSE;

		// Remove formatting characters
		LPCTSTR lpszTemp2 = AllocCleanupString(lpszTemp1);
		if (lpszTemp2 == NULL)
		{
			GlobalFreePtr((LPVOID)lpszTemp1);
			return FALSE;
		}

		// Replace alert escape characters back to dollar signs
		LPCTSTR lpszTemp3 = AllocReplaceString(lpszTemp2, TEXT("\a"), TEXT("$"));
		if (lpszTemp3 == NULL)
		{
			GlobalFreePtr((LPVOID)lpszTemp2);
			GlobalFreePtr((LPVOID)lpszTemp1);
			return FALSE;
		}

		lstrcpyn(lpszOutput, lpszTemp3, (int)cchLenOutput);

		GlobalFreePtr((LPVOID)lpszTemp3);
		GlobalFreePtr((LPVOID)lpszTemp2);
		GlobalFreePtr((LPVOID)lpszTemp1);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return FALSE;
	}

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Replaces each occurrence of a search pattern in a string with a different string.
// After a successful function call, the memory of the returned string must be released
// using GlobalFreePtr.

LPTSTR AllocReplaceString(LPCTSTR lpszOriginal, LPCTSTR lpszPattern, LPCTSTR lpszReplacement)
{
	if (lpszOriginal == NULL || lpszPattern == NULL || lpszReplacement == NULL)
		return NULL;

	SIZE_T CONST cchRepLen = _tcslen(lpszReplacement);
	SIZE_T CONST cchPatLen = _tcslen(lpszPattern);
	SIZE_T CONST cchOriLen = _tcslen(lpszOriginal);

	UINT uPatCount = 0;
	LPCTSTR lpszOrigPtr;
	LPCTSTR lpszPatLoc;

	// Determine how often the pattern occurs in the original string
	for (lpszOrigPtr = lpszOriginal; lpszPatLoc = _tcsstr(lpszOrigPtr, lpszPattern);
		lpszOrigPtr = lpszPatLoc + cchPatLen)
		uPatCount++;

	// Allocate memory for the new string
	SIZE_T CONST cchRetLen = cchOriLen + uPatCount * (cchRepLen - cchPatLen);
	LPTSTR lpszReturn = (LPTSTR)GlobalAllocPtr(GHND, (cchRetLen + 1) * sizeof(TCHAR));
	if (lpszReturn != NULL)
	{
		// Copy the original string and replace each occurrence of the pattern
		LPTSTR lpszRetPtr = lpszReturn;
		for (lpszOrigPtr = lpszOriginal; lpszPatLoc = _tcsstr(lpszOrigPtr, lpszPattern);
			lpszOrigPtr = lpszPatLoc + cchPatLen)
		{
			SIZE_T CONST cchSkpLen = lpszPatLoc - lpszOrigPtr;
			// Copy the substring up to the beginning of the pattern
			_tcsncpy(lpszRetPtr, lpszOrigPtr, cchSkpLen);
			lpszRetPtr += cchSkpLen;
			// Copy the replacement string
			_tcsncpy(lpszRetPtr, lpszReplacement, cchRepLen);
			lpszRetPtr += cchRepLen;
		}
		// Copy the rest of the string
		_tcscpy(lpszRetPtr, lpszOrigPtr);
	}

	return lpszReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Removes from a string all two- and four-digit Nadeo formatting characters that begin with a $.

LPTSTR AllocCleanupString(LPCTSTR lpszOriginal)
{
	if (lpszOriginal == NULL)
		return NULL;

	LPTSTR lpszReturn = (LPTSTR)GlobalAllocPtr(GHND, (_tcslen(lpszOriginal) + 1) * sizeof(TCHAR));
	if (lpszReturn != NULL)
	{
		LPCTSTR lpszPatLoc;
		LPCTSTR lpszOriPtr = lpszOriginal;
		LPTSTR  lpszRetPtr = lpszReturn;

		// Search every occurrence of the $ character
		while (lpszPatLoc = _tcsstr(lpszOriPtr, TEXT("$")))
		{
			// Determining the length of the formatting pattern
			// TODO: Add handling of incorrectly formatted strings
			UINT uIncrement = 4;
			TCHAR ch = lpszPatLoc[1];
			if ((ch < TEXT('0') || ch > TEXT('9')) &&
				(ch < TEXT('A') || ch > TEXT('F')) &&
				(ch < TEXT('a') || ch > TEXT('f')))
				uIncrement = 2;

			if (_tcslen(lpszPatLoc) < uIncrement)
				break;

			// Determining the distance to the pattern
			SIZE_T CONST cchSkpLen = lpszPatLoc - lpszOriPtr;
			// Copy the substring up to the beginning of the pattern
			_tcsncpy(lpszRetPtr, lpszOriPtr, cchSkpLen);
			lpszRetPtr += cchSkpLen;

			// Skip the pattern in the source string
			lpszOriPtr = lpszPatLoc + uIncrement;
		}

		// Attach any remaining text to the target string
		_tcscpy(lpszRetPtr, lpszOriPtr);
	}

	return lpszReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
