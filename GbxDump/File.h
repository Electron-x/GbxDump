////////////////////////////////////////////////////////////////////////////////////////////////
// File.h - Copyright (c) 2010-2022 by Electron.
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

BOOL GetFileName(HWND hDlg, LPTSTR lpszFileName, SIZE_T cchStringLen, LPDWORD lpdwFilterIndex, BOOL bSave = FALSE);

BOOL SaveBmpFile(LPCTSTR lpszFileName, HANDLE hDIB);

HANDLE JpegToDib(LPVOID lpJpegData, DWORD dwLenData);
HANDLE DdsToDib(LPVOID lpDdsData, DWORD dwLenData);
HANDLE WebpToDib(LPVOID lpWebpData, DWORD dwLenData);

BOOL FreeDib(HANDLE hDib);

////////////////////////////////////////////////////////////////////////////////////////////////
