////////////////////////////////////////////////////////////////////////////////////////////////
// ImgFmt.h - Copyright (c) 2010-2024 by Electron.
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

// WIDTHBYTES returns the number of DWORD-aligned bytes needed to
// hold the bit count of a DIB scanline (biWidth * biBitCount)
#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)

#define IS_OS2PM_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPCOREHEADER))
#define IS_WIN30_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPINFOHEADER))
#define IS_WIN40_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPV4HEADER))
#define IS_WIN50_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPV5HEADER))

////////////////////////////////////////////////////////////////////////////////////////////////
// Image Opening and Saving

// Displays the File Open or Save As dialog box
BOOL GetFileName(HWND hDlg, LPTSTR lpszFileName, SIZE_T cchStringLen, LPDWORD lpdwFilterIndex, BOOL bSave = FALSE);

// Saves a DIB as a Windows Bitmap file
BOOL SaveBmpFile(LPCTSTR lpszFileName, HANDLE hDib);

// Saves a DIB as a PNG file using miniz. Creates images with 8-bit grayscale, 24-bit color,
// and 32-bit color with alpha channel only. Color tables or color spaces are not supported.
BOOL SavePngFile(LPCTSTR lpszFileName, HANDLE hDib);

////////////////////////////////////////////////////////////////////////////////////////////////
// Decoding Images

// Converts a JPEG image into a DIB using libjpeg
HANDLE JpegToDib(LPVOID lpJpegData, DWORD dwLenData, BOOL bFlipImage = FALSE, INT nTraceLevel = 0);

// Decodes a WebP image into a DIB using libwebpdecoder
HANDLE WebpToDib(LPVOID lpWebpData, DWORD dwLenData, BOOL bFlipImage = FALSE, BOOL bShowFeatures = FALSE);

// Decodes a DDS image into a DIB using crunch/crnlib
HANDLE DdsToDib(LPVOID lpDdsData, DWORD dwLenData, BOOL bFlipImage = FALSE, BOOL bShowTextureDesc = FALSE);

// Frees the memory allocated for the DIB
BOOL FreeDib(HANDLE hDib);

////////////////////////////////////////////////////////////////////////////////////////////////
// Alpha Blending

// Creates a 32-bit DIB with pre-multiplied alpha from a translucent 16/32-bit DIB
HBITMAP CreatePremultipliedBitmap(HANDLE hDib);

// Frees the memory allocated for the DIB section
BOOL FreeBitmap(HBITMAP hbmpDib);

////////////////////////////////////////////////////////////////////////////////////////////////
// DIB API

// Decompresses a video compressed DIB using Video Compression Manager
HANDLE DecompressDib(HANDLE hDib);

// Converts any DIB to a compatible bitmap and then back to a DIB with the desired bit depth
HANDLE ChangeDibBitDepth(HANDLE hDib, WORD wBitCount = 0);

// Converts a compatible bitmap to a device-independent bitmap with the desired bit depth.
// This function takes a DC instead of a logical palette (as support for ChangeDibBitDepth).
HANDLE ConvertBitmapToDib(HBITMAP hBitmap, HDC hdc = NULL, WORD wBitCount = 0);

// Creates a memory object from a given DIB, which can be placed on the clipboard.
// Depending on the source DIB, a CF_DIB or CF_DIBV5 is created. The system can then
// create the other formats itself. puFormat returns the format for SetClipboardData.
HANDLE CreateClipboardDib(HANDLE hDib, UINT* puFormat = NULL);

// Creates a logical color palette from a given DIB.
// If an error occurs, a halftone palette is created.
HPALETTE CreateDibPalette(HANDLE hDib);

// Calculates the number of colors in the DIBs color table
UINT DibNumColors(LPCSTR lpbi);

// Gets the size of the optional color masks of a DIBv3
UINT ColorMasksSize(LPCSTR lpbi);

// Gets the size required to store the DIBs palette
UINT PaletteSize(LPCSTR lpbi);

// Gets the size of the DIBs bitmap bits
UINT DibImageSize(LPCSTR lpbi);

// Gets the offset from the beginning of the DIB to the bitmap bits
UINT DibBitsOffset(LPCSTR lpbi);

// Returns a pointer to the DIBs color table 
LPBYTE FindDibPalette(LPCSTR lpbi);

// Returns a pointer to the pixel bits of a packed DIB
LPBYTE FindDibBits(LPCSTR lpbi);

// Checks if the DIBv5 has a linked or embedded ICC profile
BOOL DibHasColorProfile(LPCSTR lpbi);

// Checks if the DIB contains color space data
BOOL DibHasColorSpaceData(LPCSTR lpbi);

// Checks if the biCompression member of a DIBv3 struct contains a FourCC code
BOOL DibIsVideoCompressed(LPCSTR lpbi);

// Checks whether the DIB uses the CMYK color model
BOOL DibIsCMYK(LPCSTR lpbi);

////////////////////////////////////////////////////////////////////////////////////////////////
