////////////////////////////////////////////////////////////////////////////////////////////////
// ImgFmt.h - Copyright (c) 2010-2023 by Electron.
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

#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)

#define IS_OS2PM_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPCOREHEADER))
#define IS_WIN30_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPINFOHEADER))
#define IS_WIN40_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPV4HEADER))
#define IS_WIN50_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPV5HEADER))

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL GetFileName(HWND hDlg, LPTSTR lpszFileName, SIZE_T cchStringLen, LPDWORD lpdwFilterIndex, BOOL bSave = FALSE);

BOOL SaveBmpFile(LPCTSTR lpszFileName, HANDLE hDib);
BOOL SavePngFile(LPCTSTR lpszFileName, HANDLE hDib);

HANDLE JpegToDib(LPVOID lpJpegData, DWORD dwLenData, BOOL bFlipImage = FALSE, INT nTraceLevel = 0);
HANDLE WebpToDib(LPVOID lpWebpData, DWORD dwLenData, BOOL bFlipImage = FALSE, BOOL bShowFeatures = FALSE);
HANDLE DdsToDib(LPVOID lpDdsData, DWORD dwLenData, BOOL bFlipImage = FALSE, BOOL bShowTextureDesc = FALSE);
BOOL FreeDib(HANDLE hDib);

HBITMAP CreatePremultipliedBitmap(HANDLE hDib);
BOOL FreeBitmap(HBITMAP hbmpDib);

HPALETTE CreateDibPalette(HANDLE hDib);
UINT DibNumColors(LPCSTR lpbi);
UINT ColorMasksSize(LPCSTR lpbi);
UINT PaletteSize(LPCSTR lpbi);
UINT DibImageSize(LPCSTR lpbi);
UINT DibBitsOffset(LPCSTR lpbi);
LPRGBQUAD FindDibPalette(LPCSTR lpbi);
LPBYTE FindDibBits(LPCSTR lpbi);
BOOL DibHasColorProfile(LPCSTR lpbi);
BOOL IsDibVideoCompressed(LPCSTR lpbi);

////////////////////////////////////////////////////////////////////////////////////////////////
