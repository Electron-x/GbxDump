////////////////////////////////////////////////////////////////////////////////////////////////
// ImgFmt.cpp - Copyright (c) 2010-2024 by Electron.
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
// This software includes code from the Independent JPEG Group's JPEG software
// (libjpeg), the WebP encoding and decoding library (libwebp) and the Advanced
// DXTn texture compression library (crunch/crnlib). These parts of the code are
// subject to their own copyright and license terms, which can be found in the
// libjpeg, libwebp and crunch directories.
//
////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "imgfmt.h"
#include "dumpbmp.h"
#include "..\libjpeg\jpeglib.h"
#include "..\libjpeg\iccprofile.h"
#include "..\libwebp\src\webp\decode.h"
#include "..\crunch\inc\crnlib.h"
#include "..\crunch\crnlib\crn_miniz.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module

// Incorporating rounding, Mul8Bit is an approximation of a * b / 255 for values in the
// range [0...255]. For details, see the documentation of the DrvAlphaBlend function.
int Mul8Bit(int a, int b);
// Determines the value of a color component using a color mask
BYTE GetColorValue(DWORD dwPixel, DWORD dwMask);

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL GetFileName(HWND hDlg, LPTSTR lpszFileName, SIZE_T cchStringLen, LPDWORD lpdwFilterIndex, BOOL bSave)
{
	if (lpszFileName == NULL || lpdwFilterIndex == NULL)
		return FALSE;

	// Filter string
	TCHAR szFilter[1024] = {0};
	UINT uID = g_bGerUI ? IDS_GER_FILTER_GBX : IDS_ENG_FILTER_GBX;
	if (bSave)
		uID = g_bGerUI ? IDS_GER_FILTER_PNG : IDS_ENG_FILTER_PNG;
	LoadString(g_hInstance, uID, szFilter, _countof(szFilter));
	TCHAR* psz = szFilter;
	while (psz = _tcschr(psz, TEXT('|')))
		*psz++ = TEXT('\0');

	// Initial directory
	TCHAR* pszInitialDir = NULL;
	TCHAR szInitialDir[MY_OFN_MAX_PATH] = {0};
	if (lpszFileName[0] != TEXT('\0'))
	{
		MyStrNCpy(szInitialDir, lpszFileName, _countof(szInitialDir));
		if ((psz = _tcsrchr(szInitialDir, TEXT('\\'))) == NULL)
			psz = _tcsrchr(szInitialDir, TEXT('/'));
		if (psz != NULL)
		{
			pszInitialDir = szInitialDir;
			szInitialDir[psz - pszInitialDir] = TEXT('\0');
		}
	}

	// File name
	TCHAR szFile[MY_OFN_MAX_PATH] = {0};
	if (bSave)
	{
		if (lpszFileName[0] != TEXT('\0'))
		{
			if ((psz = _tcsrchr(lpszFileName, TEXT('\\'))) == NULL)
				psz = _tcsrchr(lpszFileName, TEXT('/'));
			psz = (psz != NULL ? (*(psz + 1) != TEXT('\0') ? psz + 1 : TEXT("*")) : lpszFileName);
			MyStrNCpy(szFile, psz, _countof(szFile));
		}
		else
			_tcscpy(szFile, TEXT("*"));
		if ((psz = _tcsrchr(szFile, TEXT('.'))) != NULL)
			szFile[psz - szFile] = TEXT('\0');
		_tcsncat(szFile, *lpdwFilterIndex == 1 ? TEXT(".png") : TEXT(".bmp"),
			_countof(szFile) - _tcslen(szFile) - 1);
	}

	OPENFILENAME of    = {0};
	of.lStructSize     = sizeof(OPENFILENAME);
	of.hwndOwner       = hDlg;
	of.Flags           = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	of.lpstrFile       = szFile;
	of.nMaxFile        = _countof(szFile);
	of.lpstrFilter     = szFilter;
	of.nFilterIndex    = *lpdwFilterIndex;
	of.lpstrDefExt     = bSave ? (*lpdwFilterIndex == 1 ? TEXT("png") : TEXT("bmp")) : TEXT("gbx");
	of.lpstrInitialDir = pszInitialDir;

	BOOL bRet = FALSE;

	__try
	{
		if (bSave)
		{
			of.Flags |= OFN_OVERWRITEPROMPT;
			bRet = GetSaveFileName(&of);
		}
		else
		{
			of.Flags |= OFN_FILEMUSTEXIST;
			bRet = GetOpenFileName(&of);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) { ; }

	if (bRet)
	{
		MyStrNCpy(lpszFileName, szFile, (int)cchStringLen);
		*lpdwFilterIndex = of.nFilterIndex;
	}
	else
	{
		DWORD dwErr = CommDlgExtendedError();
		if (dwErr == FNERR_INVALIDFILENAME || dwErr == FNERR_BUFFERTOOSMALL)
			MessageBeep(MB_ICONEXCLAMATION);
	}

	return bRet;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL SaveBmpFile(LPCTSTR lpszFileName, HANDLE hDib)
{
	if (hDib == NULL || lpszFileName == NULL)
		return FALSE;

	LPCSTR lpbi = (LPCSTR)GlobalLock(hDib);
	if (lpbi == NULL)
		return FALSE;

	DWORD dwOffBits = DibBitsOffset(lpbi);
	DWORD dwBitsSize = DibImageSize(lpbi);
	DWORD dwDibSize = dwOffBits + dwBitsSize;

	if (DibHasColorProfile(lpbi))
	{
		LPBITMAPV5HEADER lpbiv5 = (LPBITMAPV5HEADER)lpbi;
		if (lpbiv5->bV5ProfileData > dwDibSize)
			dwDibSize += lpbiv5->bV5ProfileData - dwDibSize;
		dwDibSize += lpbiv5->bV5ProfileSize;
	}

	BITMAPFILEHEADER bmfHdr = {0};
	bmfHdr.bfType = BFT_BMAP;
	bmfHdr.bfSize = dwDibSize + sizeof(BITMAPFILEHEADER);
	bmfHdr.bfOffBits = dwOffBits + sizeof(BITMAPFILEHEADER);

	HANDLE hFile = CreateFile(lpszFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		GlobalUnlock(hDib);
		return FALSE;
	}

	DWORD dwWrite = 0;
	if (!WriteFile(hFile, &bmfHdr, sizeof(BITMAPFILEHEADER), &dwWrite, NULL) ||
		dwWrite != sizeof(BITMAPFILEHEADER))
	{
		GlobalUnlock(hDib);
		CloseHandle(hFile);
		DeleteFile(lpszFileName);
		return FALSE;
	}

	if (!WriteFile(hFile, (LPVOID)lpbi, dwDibSize, &dwWrite, NULL) || dwWrite != dwDibSize)
	{
		GlobalUnlock(hDib);
		CloseHandle(hFile);
		DeleteFile(lpszFileName);
		return FALSE;
	}

	GlobalUnlock(hDib);
	CloseHandle(hFile);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL SavePngFile(LPCTSTR lpszFileName, HANDLE hDib)
{
	if (hDib == NULL || lpszFileName == NULL)
		return FALSE;

	LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
	if (lpbi == NULL)
		return FALSE;

	if (lpbi->biSize >= sizeof(BITMAPINFOHEADER) &&
		(lpbi->biCompression != BI_RGB && lpbi->biCompression != BI_RLE4 &&
		lpbi->biCompression != BI_RLE8 && lpbi->biCompression != BI_BITFIELDS))
	{
		GlobalUnlock(hDib);
		return FALSE;
	}

	LPBITMAPCOREHEADER lpbc = (LPBITMAPCOREHEADER)lpbi;
	BOOL bIsCore   = IS_OS2PM_DIB(lpbi);
	LONG lWidth    = bIsCore ? lpbc->bcWidth : lpbi->biWidth;
	LONG lHeight   = bIsCore ? lpbc->bcHeight : lpbi->biHeight;
	WORD wBitCount = bIsCore ? lpbc->bcBitCount : lpbi->biBitCount;
	BOOL bIsCMYK   = DibIsCMYK((LPCSTR)lpbi);

	if (wBitCount < 16)
	{
		GlobalUnlock(hDib);
		// Convert a 1-, 4-, or 8-bpp DIB to a 24-bpp DIB
		HANDLE hNewDib = ChangeDibBitDepth(hDib, 24);
		if (hNewDib == NULL)
			return FALSE;
		else
		{
			// Restart with the supported format
			BOOL bSuccess = SavePngFile(lpszFileName, hNewDib);
			FreeDib(hNewDib);
			return bSuccess;
		}
	}

	BOOL bFlipImage = FALSE;
	if (lWidth < 0)
		lWidth = -lWidth;
	if (lHeight < 0)
	{
		lHeight = -lHeight;
		bFlipImage = TRUE;
	}

	int nNumChannels = max(wBitCount >> 3, 1);
	// 16-bit bitmaps are saved with 3 channels (or 4, if there is an alpha channel)
	LONG lSizeImage = lHeight * lWidth * (wBitCount == 16 ? 4 : nNumChannels);
	if (lSizeImage == 0)
	{
		GlobalUnlock(hDib);
		return FALSE;
	}

	LPBYTE lpDIB = FindDibBits((LPCSTR)lpbi);
	LPBYTE lpRGBA = (LPBYTE)MyGlobalAllocPtr(GHND, lSizeImage);
	if (lpRGBA == NULL)
	{
		GlobalUnlock(hDib);
		return FALSE;
	}

	LONG h, w;
	LPBYTE lpSrc, lpDest;
	LPDWORD lpdwColorMasks;
	DWORD dwColor, dwRedMask, dwGreenMask, dwBlueMask, dwAlphaMask;
	DWORD dwIncrement = WIDTHBYTES(lWidth * wBitCount);

	__try
	{
		switch (nNumChannels)
		{
			case 1: // 1-, 4-, or 8-bpp DIB --> 8-bpp grayscale PNG; requires a sorted color table
				if (wBitCount == 8)
				{
					for (h = 0; h < lHeight; h++)
					{
						lpSrc = lpDIB + (ULONG_PTR)(bFlipImage ? h : lHeight-1 - h) * dwIncrement;
						lpDest = lpRGBA + (ULONG_PTR)h * lWidth * nNumChannels;
						for (w = 0; w < lWidth; w++)
							*lpDest++ = *lpSrc++;
					}
				}
				else if (wBitCount == 4)
				{
					for (h = 0; h < lHeight; h++)
					{
						lpSrc = lpDIB + (ULONG_PTR)(bFlipImage ? h : lHeight-1 - h) * dwIncrement;
						lpDest = lpRGBA + (ULONG_PTR)h * lWidth * nNumChannels;
						for (w = 0; w < lWidth; w += 2)
						{
							*lpDest++ = ((*lpSrc >> 4) & 0x0F) * 0x11;
							*lpDest++ = (*lpSrc++ & 0x0F) * 0x11;
						}
					}
				}
				else if (wBitCount == 1)
				{
					for (h = 0; h < lHeight; h++)
					{
						lpSrc = lpDIB + (ULONG_PTR)(bFlipImage ? h : lHeight-1 - h) * dwIncrement;
						lpDest = lpRGBA + (ULONG_PTR)h * lWidth * nNumChannels;
						for (w = 0; w < lWidth; w += 8)
						{
							for (int b = 0; b < 8; b++)
								*lpDest++ = ((*lpSrc >> (7 - b)) & 1) ? 0xFF : 0x00;
							lpSrc++;
						}
					}
				}

				break;

			case 2: // 16-bpp DIB --> 24- or 32-bpp PNG
				if (!bIsCore && lpbi->biCompression == BI_BITFIELDS)
				{
					lpdwColorMasks = (LPDWORD)&(((LPBITMAPINFO)lpbi)->bmiColors[0]);
					dwRedMask      = lpdwColorMasks[0];
					dwGreenMask    = lpdwColorMasks[1];
					dwBlueMask     = lpdwColorMasks[2];
					dwAlphaMask    = lpbi->biSize >= 56 ? lpdwColorMasks[3] : 0;
				}
				else
				{
					dwRedMask   = 0x00007C00;
					dwGreenMask = 0x000003E0;
					dwBlueMask  = 0x0000001F;
					dwAlphaMask = 0x00008000; // TODO: Add test for visible pixels
				}

				nNumChannels = dwAlphaMask ? 4 : 3;

				for (h = 0; h < lHeight; h++)
				{
					lpSrc = lpDIB + (ULONG_PTR)(bFlipImage ? h : lHeight-1 - h) * dwIncrement;
					lpDest = lpRGBA + (ULONG_PTR)h * lWidth * nNumChannels;
					for (w = 0; w < lWidth; w++)
					{
						dwColor = MAKELONG(MAKEWORD(lpSrc[0], lpSrc[1]), 0);
						*lpDest++ = GetColorValue(dwColor, dwRedMask);
						*lpDest++ = GetColorValue(dwColor, dwGreenMask);
						*lpDest++ = GetColorValue(dwColor, dwBlueMask);
						if (nNumChannels == 4)
							*lpDest++ = GetColorValue(dwColor, dwAlphaMask);
						lpSrc += 2;
					}
				}

				break;

			case 3: // 24-bpp RGB --> 24-bpp PNG
				for (h = 0; h < lHeight; h++)
				{
					lpSrc = lpDIB + (ULONG_PTR)(bFlipImage ? h : lHeight-1 - h) * dwIncrement;
					lpDest = lpRGBA + (ULONG_PTR)h * lWidth * nNumChannels;
					for (w = 0; w < lWidth; w++)
					{
						lpDest[2] = *lpSrc++;
						lpDest[1] = *lpSrc++;
						lpDest[0] = *lpSrc++;
						lpDest += 3;
					}
				}

				break;

			case 4:
				if (bIsCMYK)
				{ // 32-bpp CMYK DIB --> 24-bpp RGB PNG
					nNumChannels = 3;

					BYTE cInvKey;
					for (h = 0; h < lHeight; h++)
					{
						lpSrc = lpDIB + (ULONG_PTR)(bFlipImage ? h : lHeight-1 - h) * dwIncrement;
						lpDest = lpRGBA + (ULONG_PTR)h * lWidth * nNumChannels;
						for (w = 0; w < lWidth; w++)
						{
							cInvKey = 0xFF - lpSrc[3];
							*lpDest++ = Mul8Bit(0xFF - lpSrc[2], cInvKey);
							*lpDest++ = Mul8Bit(0xFF - lpSrc[1], cInvKey);
							*lpDest++ = Mul8Bit(0xFF - lpSrc[0], cInvKey);
							lpSrc += 4;
						}
					}
				}
				else
				{ // 32-bpp DIB --> 32-bpp PNG
					if (!bIsCore && lpbi->biCompression == BI_BITFIELDS)
					{
						lpdwColorMasks = (LPDWORD)&(((LPBITMAPINFO)lpbi)->bmiColors[0]);
						dwRedMask      = lpdwColorMasks[0];
						dwGreenMask    = lpdwColorMasks[1];
						dwBlueMask     = lpdwColorMasks[2];
						dwAlphaMask    = lpbi->biSize >= 56 ? lpdwColorMasks[3] : 0;
					}
					else
					{
						dwRedMask   = 0x00FF0000;
						dwGreenMask = 0x0000FF00;
						dwBlueMask  = 0x000000FF;
						dwAlphaMask = 0xFF000000; // TODO: Add test for visible pixels
					}

					for (h = 0; h < lHeight; h++)
					{
						lpSrc = lpDIB + (ULONG_PTR)(bFlipImage ? h : lHeight-1 - h) * dwIncrement;
						lpDest = lpRGBA + (ULONG_PTR)h * lWidth * nNumChannels;
						for (w = 0; w < lWidth; w++)
						{
							dwColor = MAKELONG(MAKEWORD(lpSrc[0], lpSrc[1]), MAKEWORD(lpSrc[2], lpSrc[3]));
							*lpDest++ = GetColorValue(dwColor, dwRedMask);
							*lpDest++ = GetColorValue(dwColor, dwGreenMask);
							*lpDest++ = GetColorValue(dwColor, dwBlueMask);
							*lpDest++ = dwAlphaMask ? GetColorValue(dwColor, dwAlphaMask) : 0xFF;
							lpSrc += 4;
						}
					}
				}

				break;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) { ; }

	size_t cbSize = 0;
	LPVOID lpPNG = tdefl_write_image_to_png_file_in_memory(lpRGBA, lWidth, lHeight, nNumChannels, &cbSize);
	if (lpPNG == NULL)
	{
		MyGlobalFreePtr((LPVOID)lpRGBA);
		GlobalUnlock(hDib);
		return FALSE;
	}

	HANDLE hFile = CreateFile(lpszFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		mz_free(lpPNG);
		MyGlobalFreePtr((LPVOID)lpRGBA);
		GlobalUnlock(hDib);
		return FALSE;
	}

	DWORD dwWrite;
	if (!WriteFile(hFile, lpPNG, (DWORD)cbSize, &dwWrite, NULL) || dwWrite != cbSize)
	{
		mz_free(lpPNG);
		MyGlobalFreePtr((LPVOID)lpRGBA);
		GlobalUnlock(hDib);
		CloseHandle(hFile);
		DeleteFile(lpszFileName);
		return FALSE;
	}

	mz_free(lpPNG);
	MyGlobalFreePtr((LPVOID)lpRGBA);
	GlobalUnlock(hDib);
	CloseHandle(hFile);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// JPEG decoder using the Independent JPEG Group's JPEG software

////////////////////////////////////////////////////////////////////////////////////////////////
// Data Types

typedef struct jpeg_decompress_struct j_decompress;
typedef struct jpeg_error_mgr         j_error_mgr;

// JPEG decompression structure (overloads jpeg_decompress_struct)
typedef struct _JPEG_DECOMPRESS
{
	j_decompress    jInfo;          // Decompression structure
	j_error_mgr     jError;         // Error manager
	INT             nTraceLevel;    // Max. message level that will be displayed
	BOOL            bNeedDestroy;   // jInfo must be destroyed
	LPBYTE          lpProfileData;  // Pointer to ICC profile data
} JPEG_DECOMPRESS, *LPJPEG_DECOMPRESS;

// Buffer for processor status
static jmp_buf JmpBuffer;

////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions

// Performs housekeeping
void cleanup_jpeg_to_dib(LPJPEG_DECOMPRESS lpJpegDecompress, HANDLE hDib);
// Registers the callback functions for the error manager
void set_error_manager(j_common_ptr pjInfo, j_error_mgr *pjError);

////////////////////////////////////////////////////////////////////////////////////////////////
// Callback functions

// Error handling
static void my_error_exit(j_common_ptr pjInfo);
// Text output
static void my_output_message(j_common_ptr pjInfo);
// Message handling
static void my_emit_message(j_common_ptr pjInfo, int nMessageLevel);

////////////////////////////////////////////////////////////////////////////////////////////////

HANDLE JpegToDib(LPVOID lpJpegData, DWORD dwLenData, BOOL bFlipImage, INT nTraceLevel)
{
	HANDLE hDib = NULL;

	// Initialize the JPEG decompression object
	JPEG_DECOMPRESS JpegDecompress = {0};
	j_decompress_ptr pjInfo = &JpegDecompress.jInfo;
	JpegDecompress.lpProfileData = NULL;
	JpegDecompress.nTraceLevel = nTraceLevel;
	set_error_manager((j_common_ptr)pjInfo, &JpegDecompress.jError);

	// Save processor status for error handling
	if (setjmp(JmpBuffer))
	{
		cleanup_jpeg_to_dib(&JpegDecompress, hDib);
		return NULL;
	}

	jpeg_create_decompress(pjInfo);
	JpegDecompress.bNeedDestroy = TRUE;

	// Prepare for reading an ICC profile
	setup_read_icc_profile(pjInfo);

	// Determine data source
	jpeg_mem_src(pjInfo, (LPBYTE)lpJpegData, dwLenData);

	// Determine image information
	jpeg_read_header(pjInfo, TRUE);

	// Read an existing ICC profile
	UINT uProfileLen = 0;
	BOOL bHasProfile = FALSE;
	if (pjInfo->out_color_space != JCS_CMYK)  // We convert CMYK rudimentary to RGB and discard an existing ICC profile
		bHasProfile = read_icc_profile(pjInfo, &JpegDecompress.lpProfileData, &uProfileLen);

	// Image resolution
	LONG lXPelsPerMeter = 0;
	LONG lYPelsPerMeter = 0;
	if (pjInfo->saw_JFIF_marker)
	{
		if (pjInfo->density_unit == 1)
		{  // Pixel per inch
			lXPelsPerMeter = pjInfo->X_density * 5000 / 127;
			lYPelsPerMeter = pjInfo->Y_density * 5000 / 127;
		}
		else if (pjInfo->density_unit == 2)
		{  // Pixel per cm
			lXPelsPerMeter = pjInfo->X_density * 100;
			lYPelsPerMeter = pjInfo->Y_density * 100;
		}
	}

	// Start decompression in the JPEG library
	jpeg_start_decompress(pjInfo);

	// Determine output image format
	WORD wBitDepth = pjInfo->output_components << 3;
	UINT uNumColors = wBitDepth == 8 ? (pjInfo->out_color_space == JCS_GRAYSCALE ? 256 : pjInfo->actual_number_of_colors) : 0;
	UINT uIncrement = WIDTHBYTES(pjInfo->output_width * wBitDepth);
	DWORD dwImageSize = uIncrement * pjInfo->output_height;
	DWORD dwHeaderSize = bHasProfile ? sizeof(BITMAPV5HEADER) : sizeof(BITMAPINFOHEADER);

	// Allocate memory for the DIB
	hDib = GlobalAlloc(GHND, dwHeaderSize + uNumColors * sizeof(RGBQUAD) + dwImageSize + uProfileLen);
	if (hDib == NULL)
	{
		cleanup_jpeg_to_dib(&JpegDecompress, hDib);
		return NULL;
	}

	// Fill bitmap information block
	LPBITMAPINFO lpBMI = (LPBITMAPINFO)GlobalLock(hDib);
	LPBITMAPV5HEADER lpBIV5 = (LPBITMAPV5HEADER)lpBMI;
	if (lpBIV5 == NULL)
	{
		cleanup_jpeg_to_dib(&JpegDecompress, hDib);
		return NULL;
	}

	lpBIV5->bV5Size = dwHeaderSize;
	lpBIV5->bV5Width = pjInfo->output_width;
	lpBIV5->bV5Height = pjInfo->output_height;
	lpBIV5->bV5Planes = 1;
	lpBIV5->bV5BitCount = wBitDepth;
	lpBIV5->bV5Compression = BI_RGB;
	lpBIV5->bV5XPelsPerMeter = lXPelsPerMeter;
	lpBIV5->bV5YPelsPerMeter = lYPelsPerMeter;
	lpBIV5->bV5ClrUsed = uNumColors;

	// Create palette
	if (wBitDepth == 8)
	{
		if (pjInfo->out_color_space == JCS_GRAYSCALE)
		{  // Grayscale image
			for (UINT u = 0; u < uNumColors; u++)
			{  // Create grayscale
				lpBMI->bmiColors[u].rgbRed      = (BYTE)u;
				lpBMI->bmiColors[u].rgbGreen    = (BYTE)u;
				lpBMI->bmiColors[u].rgbBlue     = (BYTE)u;
				lpBMI->bmiColors[u].rgbReserved = (BYTE)0;
			}
		}
		else
		{  // Palette image
			for (UINT u = 0; u < uNumColors; u++)
			{  // Copy palette
				lpBMI->bmiColors[u].rgbRed      = pjInfo->colormap[0][u];
				lpBMI->bmiColors[u].rgbGreen    = pjInfo->colormap[1][u];
				lpBMI->bmiColors[u].rgbBlue     = pjInfo->colormap[2][u];
				lpBMI->bmiColors[u].rgbReserved = 0;
			}
		}
	}

	// Determine pointer to start of image data
	LPBYTE lpDIB = FindDibBits((LPCSTR)lpBIV5);
	LPBYTE lpBits = NULL;           // Pointer to a DIB image row
	JSAMPROW lpScanlines[1] = {0};  // Pointer to a scanline
	JDIMENSION uScanline = 0;       // Row index
	BYTE cKey, cRed, cBlue;         // Color components

	// Copy image rows (scanlines). The arrangement of the color
	// components must be changed in jmorecfg.h from RGB to BGR.
	while (pjInfo->output_scanline < pjInfo->output_height)
	{
		uScanline = bFlipImage ? pjInfo->output_scanline : pjInfo->output_height-1 - pjInfo->output_scanline;
		lpBits = lpDIB + (UINT_PTR)uScanline * uIncrement;
		lpScanlines[0] = lpBits;
		jpeg_read_scanlines(pjInfo, lpScanlines, 1);  // Decompress one line

		if (pjInfo->out_color_space == JCS_CMYK)
		{  // Convert from inverted CMYK to RGB
			for (UINT u = 0; u < (pjInfo->output_width * 4); u += 4)
			{
				cKey  = lpBits[u+3];
				cRed  = Mul8Bit(lpBits[u+0], cKey);
				cBlue = Mul8Bit(lpBits[u+2], cKey);
				lpBits[u+1] = Mul8Bit(lpBits[u+1], cKey);
				lpBits[u+0] = cBlue;
				lpBits[u+2] = cRed;
				lpBits[u+3] = 0xFF;
			}
		}
	}

	// Embed an existing ICC profile into the DIB
	if (bHasProfile)
	{
		lpBIV5->bV5CSType = PROFILE_EMBEDDED;
		lpBIV5->bV5Intent = LCS_GM_IMAGES;
		lpBIV5->bV5ProfileSize = uProfileLen;
		lpBIV5->bV5ProfileData = (DWORD)(lpDIB - (LPBYTE)lpBIV5) + dwImageSize;

		CopyMemory(lpDIB + dwImageSize, JpegDecompress.lpProfileData, uProfileLen);
	}

	// Finish decompression
	GlobalUnlock(hDib);
	jpeg_finish_decompress(pjInfo);

	// Free all requested memory, but keep the DIB we just created
	cleanup_jpeg_to_dib(&JpegDecompress, NULL);
	JpegDecompress.bNeedDestroy = FALSE;

	return hDib;
}

////////////////////////////////////////////////////////////////////////////////////////////////

void cleanup_jpeg_to_dib(LPJPEG_DECOMPRESS lpJpegDecompress, HANDLE hDib)
{
	if (lpJpegDecompress != NULL)
	{
		// Release the ICC profile data
		if (lpJpegDecompress->lpProfileData != NULL)
		{
			free(lpJpegDecompress->lpProfileData);
			lpJpegDecompress->lpProfileData = NULL;
		}

		// Destroy the JPEG decompress object
		if (lpJpegDecompress->bNeedDestroy)
			jpeg_destroy_decompress(&lpJpegDecompress->jInfo);
	}

	// Release the DIB
	if (hDib != NULL)
	{
		GlobalUnlock(hDib);
		GlobalFree(hDib);
		hDib = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

void set_error_manager(j_common_ptr pjInfo, j_error_mgr *pjError)
{
	// Activate the default error manager
	jpeg_std_error(pjError);

	// Set callback functions
	pjError->error_exit     = my_error_exit;
	pjError->output_message = my_output_message;
	pjError->emit_message   = my_emit_message;

	// Set trace level
	pjError->trace_level = ((LPJPEG_DECOMPRESS)pjInfo)->nTraceLevel;

	// Assign the error manager structure to the JPEG object
	pjInfo->err = pjError;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// If an error occurs in libjpeg, my_error_exit is called. In this case, a
// precise error message should be displayed on the screen. Then my_error_exit
// deletes the JPEG object and returns to the JpegToDib function using throw.

void my_error_exit(j_common_ptr pjInfo)
{
	// Display error message on screen
	(*pjInfo->err->output_message)(pjInfo);

	// Delete JPEG object (e.g. delete temporary files, memory, etc.)
	jpeg_destroy(pjInfo);
	((LPJPEG_DECOMPRESS)pjInfo)->bNeedDestroy = FALSE;

	longjmp(JmpBuffer, 1);  // Return to setjmp in JpegToDib
}

////////////////////////////////////////////////////////////////////////////////////////////////
// This callback function formats a message and displays it on the screen

void my_output_message(j_common_ptr pjInfo)
{
	char szBuffer[JMSG_LENGTH_MAX];
	TCHAR szMessage[JMSG_LENGTH_MAX];

	// Format text
	(*pjInfo->err->format_message)(pjInfo, szBuffer);

	// Output text
	mbstowcs(szMessage, szBuffer, JMSG_LENGTH_MAX - 1);

	HWND hDlg = GetActiveWindow();
	HWND hwndEdit = GetOutputWindow(hDlg);
	if (hwndEdit == NULL)
		MessageBox(hDlg, szMessage, g_szTitle, MB_OK | MB_ICONEXCLAMATION);
	else
	{
		OutputText(hwndEdit, szMessage);
		OutputText(hwndEdit, TEXT("\r\n"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
// This function handles all messages (trace, debug or error printouts) generated by libjpeg.
// my_emit_message determines whether the message is suppressed or displayed on the screen:
//  -1: Warning
//   0: Important message (e.g. error)
// 1-3: Trace information

void my_emit_message(j_common_ptr pjInfo, int nMessageLevel)
{
	char szBuffer[JMSG_LENGTH_MAX];

	// Process message level
	if (nMessageLevel < 0)
	{  // Warning
		if ((pjInfo->err->num_warnings == 0 && pjInfo->err->trace_level >= 1) ||
			pjInfo->err->trace_level >= 3)
			(*pjInfo->err->output_message)(pjInfo);
		// Increase warning counter
		pjInfo->err->num_warnings++;
	}
	else
	{
		if (nMessageLevel == 0)
			// Important message -> display on screen
			(*pjInfo->err->output_message)(pjInfo);
		else
		{  // Trace information -> Display if trace_level >= msg_level
			if (pjInfo->err->trace_level >= nMessageLevel)
				(*pjInfo->err->output_message)(pjInfo);
			else
				(*pjInfo->err->format_message)(pjInfo, szBuffer);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

HANDLE WebpToDib(LPVOID lpWebpData, DWORD dwLenData, BOOL bFlipImage, BOOL bShowFeatures)
{
	WebPBitstreamFeatures features;

	if (lpWebpData == NULL || dwLenData == 0 ||
		WebPGetFeatures((LPBYTE)lpWebpData, dwLenData, &features) != VP8_STATUS_OK)
		return NULL;

	int nWidth = features.width;
	int nHeight = features.height;

	if (bShowFeatures)
	{
		HWND hwndEdit = GetOutputWindow();
		if (hwndEdit != NULL)
		{
			TCHAR szOutput[OUTPUT_LEN];
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Width:\t\t%d pixels\r\n"), nWidth);
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Height:\t\t%d pixels\r\n"), nHeight);
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Has Alpha:\t%s\r\n"),
				features.has_alpha ? TEXT("True") : TEXT("False"));
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Has Animation:\t%s\r\n"),
				features.has_animation ? TEXT("True") : TEXT("False"));
			OutputText(hwndEdit, TEXT("Format:\t\t"));
			switch (features.format)
			{
				case 0:
					OutputText(hwndEdit, TEXT("Undefined/Mixed"));
					break;
				case 1:
					OutputText(hwndEdit, TEXT("Lossy"));
					break;
				case 2:
					OutputText(hwndEdit, TEXT("Lossless"));
					break;
				default:
					OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("%d"), features.format);
			}
			OutputText(hwndEdit, TEXT("\r\n"));
		}
	}

	LPBYTE lpBits = NULL;
	if (features.has_alpha)
		lpBits = WebPDecodeBGRA((LPBYTE)lpWebpData, dwLenData, &nWidth, &nHeight);
	else
		lpBits = WebPDecodeBGR((LPBYTE)lpWebpData, dwLenData, &nWidth, &nHeight);
	if (lpBits == NULL)
		return NULL;
	
	int nNumChannels = features.has_alpha ? 4 : 3;
	int nIncrement = WIDTHBYTES(nWidth * nNumChannels * 8);
	int nSizeImage = nHeight * nIncrement;
	if (nSizeImage == 0)
	{
		WebPFree(lpBits);
		return NULL;
	}

	HANDLE hDib = GlobalAlloc(GHND, sizeof(BITMAPINFOHEADER) + nSizeImage);
	if (hDib == NULL)
	{
		WebPFree(lpBits);
		return NULL;
	}

	LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
	if (lpbi == NULL)
	{
		GlobalFree(hDib);
		WebPFree(lpBits);
		return NULL;
	}

	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biWidth = nWidth;
	lpbi->biHeight = nHeight;
	lpbi->biPlanes = 1;
	lpbi->biBitCount = nNumChannels * 8;
	lpbi->biCompression = BI_RGB;

	int nSrcPtr = 0;
	LPBYTE lpSrc = lpBits;
	LPBYTE lpDest = ((LPBYTE)lpbi) + sizeof(BITMAPINFOHEADER);

	__try
	{
		for (int h = 0; h < nHeight; h++)
		{
			nSrcPtr = (bFlipImage ? h : nHeight-1 - h) * nWidth * nNumChannels;
			for (int w = 0; w < nIncrement; w++)
				*lpDest++ = lpSrc[nSrcPtr++];
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) { ; }

	GlobalUnlock(hDib);
	WebPFree(lpBits);

	return hDib;
}

////////////////////////////////////////////////////////////////////////////////////////////////

HANDLE DdsToDib(LPVOID lpDdsData, DWORD dwLenData, BOOL bFlipImage, BOOL bShowTextureDesc)
{
	if (lpDdsData == NULL || dwLenData == 0)
		return NULL;

	crn_texture_desc tex_desc;
	crn_uint32 *pImages[cCRNMaxFaces * cCRNMaxLevels];
	if (!crn_decompress_dds_to_images(lpDdsData, dwLenData, pImages, tex_desc))
		return NULL;

	if (bShowTextureDesc)
	{
		HWND hwndEdit = GetOutputWindow();
		if (hwndEdit != NULL)
		{
			BYTE achFourCC[4];
			TCHAR szOutput[OUTPUT_LEN];
			memcpy(achFourCC, &tex_desc.m_fmt_fourcc, 4);
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Faces:\t\t%u\r\n"), tex_desc.m_faces);
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Width:\t\t%u pixels\r\n"), tex_desc.m_width);
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Height:\t\t%u pixels\r\n"), tex_desc.m_height);
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Levels:\t\t%u\r\n"), tex_desc.m_levels);
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Format:\t\t%hc%hc%hc%hc\r\n"),
				achFourCC[0], achFourCC[1], achFourCC[2], achFourCC[3]);
		}
	}

	crn_uint32 uWidth = tex_desc.m_width;
	crn_uint32 uHeight = tex_desc.m_height;
	crn_uint32 uSizeImage = uHeight * uWidth * 4;
	if (uSizeImage == 0)
	{
		crn_free_all_images(pImages, tex_desc);
		return NULL;
	}

	HANDLE hDib = GlobalAlloc(GHND, sizeof(BITMAPINFOHEADER) + uSizeImage);
	if (hDib == NULL)
	{
		crn_free_all_images(pImages, tex_desc);
		return NULL;
	}

	LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
	if (lpbi == NULL)
	{
		GlobalFree(hDib);
		crn_free_all_images(pImages, tex_desc);
		return NULL;
	}

	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biWidth = uWidth;
	lpbi->biHeight = uHeight;
	lpbi->biPlanes = 1;
	lpbi->biBitCount = 32;
	lpbi->biCompression = BI_RGB;

	crn_uint32 uSrcPtr = 0;
	crn_uint32 uDestPtr = 0;
	LPBYTE lpSrc = (LPBYTE)pImages[0];
	LPBYTE lpDest = ((LPBYTE)lpbi) + sizeof(BITMAPINFOHEADER);

	__try
	{
		for (crn_uint32 h = 0; h < uHeight; h++)
		{
			uSrcPtr = (bFlipImage ? h : uHeight-1 - h) * uWidth * 4;
			for (crn_uint32 w = 0; w < uWidth; w++)
			{
				lpDest[uDestPtr + 2] = lpSrc[uSrcPtr++]; // B <-- R
				lpDest[uDestPtr + 1] = lpSrc[uSrcPtr++]; // G <-- G
				lpDest[uDestPtr + 0] = lpSrc[uSrcPtr++]; // R <-- B
				lpDest[uDestPtr + 3] = lpSrc[uSrcPtr++]; // A <-- A
				uDestPtr += 4;
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) { ; }

	GlobalUnlock(hDib);
	crn_free_all_images(pImages, tex_desc);

	return hDib;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL FreeDib(HANDLE hDib)
{
	if (hDib == NULL || GlobalFree(hDib) != NULL)
		return FALSE;

	hDib = NULL;

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

HBITMAP CreatePremultipliedBitmap(HANDLE hDib)
{
	if (hDib == NULL)
		return NULL;

	LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
	if (lpbi == NULL)
		return NULL;

	LPBITMAPCOREHEADER lpbc = (LPBITMAPCOREHEADER)lpbi;
	BOOL bIsCore = IS_OS2PM_DIB(lpbi);
	WORD wBitCount = bIsCore ? lpbc->bcBitCount : lpbi->biBitCount;

	if ((wBitCount != 16 && wBitCount != 32) ||
		(lpbi->biSize >= sizeof(BITMAPINFOHEADER) &&
		(lpbi->biCompression != BI_RGB && lpbi->biCompression != BI_BITFIELDS)) ||
		DibIsCMYK((LPCSTR)lpbi))
	{
		GlobalUnlock(hDib);
		return NULL;
	}

	LONG lWidth  = bIsCore ? lpbc->bcWidth : lpbi->biWidth;
	LONG lHeight = bIsCore ? lpbc->bcHeight : lpbi->biHeight;

	BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = lWidth;
	bmi.bmiHeader.biHeight = lHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	lWidth = abs(lWidth);
	lHeight = abs(lHeight);

	LPBYTE lpBGRA = NULL;
	LPBYTE lpDIB = FindDibBits((LPCSTR)lpbi);

	HBITMAP hbmpDib = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (PVOID*)&lpBGRA, NULL, NULL);
	if (hbmpDib == NULL)
	{
		GlobalUnlock(hDib);
		return NULL;
	}

	LONG h, w;
	BYTE cAlpha;
	LPBYTE lpSrc, lpDest;
	LPDWORD lpdwColorMasks;
	DWORD dwColor, dwRedMask, dwGreenMask, dwBlueMask, dwAlphaMask;
	DWORD dwIncrement = WIDTHBYTES(lWidth * wBitCount);
	BOOL bHasVisiblePixels = FALSE;
	BOOL bHasTransparentPixels = FALSE;

	__try
	{
		if (wBitCount == 16)
		{
			if (!bIsCore && lpbi->biCompression == BI_BITFIELDS)
			{
				lpdwColorMasks = (LPDWORD)&(((LPBITMAPINFO)lpbi)->bmiColors[0]);
				dwRedMask      = lpdwColorMasks[0];
				dwGreenMask    = lpdwColorMasks[1];
				dwBlueMask     = lpdwColorMasks[2];
				dwAlphaMask    = lpbi->biSize >= 56 ? lpdwColorMasks[3] : 0;
			}
			else
			{
				dwRedMask   = 0x00007C00;
				dwGreenMask = 0x000003E0;
				dwBlueMask  = 0x0000001F;
				dwAlphaMask = 0x00008000;
			}

			if (dwAlphaMask)
			{
				for (h = 0; h < lHeight; h++)
				{
					lpSrc = lpDIB + (ULONG_PTR)h * dwIncrement;
					lpDest = lpBGRA + (ULONG_PTR)h * lWidth * 4;
					for (w = 0; w < lWidth; w++)
					{
						dwColor = MAKELONG(MAKEWORD(lpSrc[0], lpSrc[1]), 0);
						cAlpha = dwAlphaMask ? GetColorValue(dwColor, dwAlphaMask) : 0xFF;
						if (cAlpha != 0x00) bHasVisiblePixels = TRUE;
						if (cAlpha != 0xFF) bHasTransparentPixels = TRUE;
						*lpDest++ = Mul8Bit(GetColorValue(dwColor, dwBlueMask), cAlpha);
						*lpDest++ = Mul8Bit(GetColorValue(dwColor, dwGreenMask), cAlpha);
						*lpDest++ = Mul8Bit(GetColorValue(dwColor, dwRedMask), cAlpha);
						*lpDest++ = cAlpha;
						lpSrc += 2;
					}
				}
			}
		}
		else if (wBitCount == 32)
		{
			if (!bIsCore && lpbi->biCompression == BI_BITFIELDS)
			{
				lpdwColorMasks = (LPDWORD)&(((LPBITMAPINFO)lpbi)->bmiColors[0]);
				dwRedMask      = lpdwColorMasks[0];
				dwGreenMask    = lpdwColorMasks[1];
				dwBlueMask     = lpdwColorMasks[2];
				dwAlphaMask    = lpbi->biSize >= 56 ? lpdwColorMasks[3] : 0;
			}
			else
			{
				dwRedMask   = 0x00FF0000;
				dwGreenMask = 0x0000FF00;
				dwBlueMask  = 0x000000FF;
				dwAlphaMask = 0xFF000000;
			}

			if (dwAlphaMask)
			{
				for (h = 0; h < lHeight; h++)
				{
					lpSrc = lpDIB + (ULONG_PTR)h * dwIncrement;
					lpDest = lpBGRA + (ULONG_PTR)h * lWidth * 4;
					for (w = 0; w < lWidth; w++)
					{
						dwColor = MAKELONG(MAKEWORD(lpSrc[0], lpSrc[1]), MAKEWORD(lpSrc[2], lpSrc[3]));
						cAlpha = dwAlphaMask ? GetColorValue(dwColor, dwAlphaMask) : 0xFF;
						if (cAlpha != 0x00) bHasVisiblePixels = TRUE;
						if (cAlpha != 0xFF) bHasTransparentPixels = TRUE;
						*lpDest++ = Mul8Bit(GetColorValue(dwColor, dwBlueMask), cAlpha);
						*lpDest++ = Mul8Bit(GetColorValue(dwColor, dwGreenMask), cAlpha);
						*lpDest++ = Mul8Bit(GetColorValue(dwColor, dwRedMask), cAlpha);
						*lpDest++ = cAlpha;
						lpSrc += 4;
					}
				}
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) { ; }

	GlobalUnlock(hDib);

	if (!bHasVisiblePixels || !bHasTransparentPixels)
	{ // The image is completely transparent or completely opaque
		DeleteBitmap(hbmpDib);
		return NULL;
	}

	return hbmpDib;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL FreeBitmap(HBITMAP hbmpDib)
{
	if (hbmpDib == NULL || !DeleteBitmap(hbmpDib))
		return FALSE;

	hbmpDib = NULL;

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

HANDLE DecompressDib(HANDLE hDib)
{
	if (hDib == NULL)
		return NULL;

	LPBITMAPINFO lpbmi = (LPBITMAPINFO)GlobalLock(hDib);
	if (lpbmi == NULL)
		return NULL;

	HANDLE hDibNew = NULL;

	if (DibIsVideoCompressed((LPCSTR)lpbmi))
		hDibNew = ICImageDecompress(NULL, 0, lpbmi, FindDibBits((LPCSTR)lpbmi), NULL);

	GlobalUnlock(hDib);

	return hDibNew;
}

////////////////////////////////////////////////////////////////////////////////////////////////

HANDLE ChangeDibBitDepth(HANDLE hDib, WORD wBitCount)
{
	if (hDib == NULL)
		return NULL;

	HDC hdc = GetDC(NULL);

	if (wBitCount == 0)
		wBitCount = (WORD)GetDeviceCaps(hdc, BITSPIXEL);

	HPALETTE hpalOld = NULL;
	HPALETTE hpal = CreateDibPalette(hDib);
	if (hpal != NULL)
	{
		hpalOld = SelectPalette(hdc, hpal, FALSE);
		RealizePalette(hdc);
	}

	HBITMAP hBitmap = NULL;
	LPSTR lpbi = (LPSTR)GlobalLock(hDib);
	if (lpbi != NULL)
	{
		if (DibHasColorSpaceData(lpbi))
			SetICMMode(hdc, ICM_ON);

		// Convert the DIB to a compatible bitmap with the bit depth of the screen
		// (CBM_CREATEDIB and CreateDIBSection doesn't support RLE compressed DIBs)
		hBitmap = CreateDIBitmap(hdc, (LPBITMAPINFOHEADER)lpbi, CBM_INIT, FindDibBits(lpbi),
			(LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

		GlobalUnlock(hDib);
	}

	HANDLE hNewDib = NULL;
	if (hBitmap != NULL)
		hNewDib = ConvertBitmapToDib(hBitmap, hdc, wBitCount);

	if (hBitmap != NULL)
		DeleteBitmap(hBitmap);
	if (hpalOld != NULL)
		SelectPalette(hdc, hpalOld, FALSE);
	if (hpal != NULL)
		DeletePalette(hpal);

	ReleaseDC(NULL, hdc);

	return hNewDib;
}

////////////////////////////////////////////////////////////////////////////////////////////////

HANDLE ConvertBitmapToDib(HBITMAP hBitmap, HDC hdc, WORD wBitCount)
{
	if (hBitmap == NULL)
		return NULL;

	BITMAP bm = {0};
	if (!GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm))
		return NULL;

	if (wBitCount == 0)
		wBitCount = bm.bmPlanes * bm.bmBitsPixel;

	// Define the format of the destination DIB
	BITMAPINFOHEADER bi = {0};
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bm.bmWidth;
	bi.biHeight = bm.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = wBitCount;
	bi.biCompression = BI_RGB;

	// Request memory for the target DIB. The memory block must
	// be large enough to hold the data supplied by GetDIBits.
	HANDLE hDib = GlobalAlloc(GHND, (SIZE_T)DibBitsOffset((LPCSTR)&bi) + DibImageSize((LPCSTR)&bi));
	if (hDib == NULL)
		return NULL;

	LPBITMAPINFO lpbmi = (LPBITMAPINFO)GlobalLock(hDib);
	if (lpbmi == NULL)
	{
		GlobalFree(hDib);
		return NULL;
	}

	BOOL bReleaseDC = FALSE;
	if (hdc == NULL)
	{
		hdc = GetDC(NULL);
		bReleaseDC = TRUE;
	}

	// Set the information header
	lpbmi->bmiHeader = bi;
	// Retrieve the color table and the bitmap bits
	int nRet = GetDIBits(hdc, hBitmap, 0, (UINT)bi.biHeight, FindDibBits((LPCSTR)lpbmi), lpbmi, DIB_RGB_COLORS);

	if (bReleaseDC)
	{
		ReleaseDC(NULL, hdc);
		hdc = NULL;
	}

	GlobalUnlock(hDib);

	if (!nRet)
	{
		GlobalFree(hDib);
		hDib = NULL;
	}

	return hDib;
}

////////////////////////////////////////////////////////////////////////////////////////////////

HANDLE CreateClipboardDib(HANDLE hDib, UINT *puFormat)
{
	if (hDib == NULL)
		return NULL;

	SIZE_T cbLen = GlobalSize((HGLOBAL)hDib);
	if (cbLen == 0)
		return NULL;

	LPBYTE lpSrc = (LPBYTE)GlobalLock((HGLOBAL)hDib);
	if (lpSrc == NULL)
		return NULL;
	
	HGLOBAL hNewDib = NULL;
	DWORD dwSrcHeaderSize = *(LPDWORD)lpSrc;

	if (dwSrcHeaderSize == sizeof(BITMAPCOREHEADER))
	{
		// Create a new DIBv3 from a DIBv2
		LPBITMAPCOREINFO lpbmci = (LPBITMAPCOREINFO)lpSrc;
		UINT uColors = DibNumColors((LPCSTR)lpbmci);
		UINT uImageSize = DibImageSize((LPCSTR)lpbmci);

		hNewDib = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DDESHARE,
			sizeof(BITMAPINFOHEADER) + uColors * sizeof(RGBQUAD) + uImageSize);
		if (hNewDib == NULL)
		{
			GlobalUnlock((HGLOBAL)hDib);
			return NULL;
		}

		LPBYTE lpDest = (LPBYTE)GlobalLock(hNewDib);
		if (lpDest == NULL)
		{
			GlobalFree(hNewDib);
			GlobalUnlock((HGLOBAL)hDib);
			return NULL;
		}

		LPBITMAPINFO lpbmi = (LPBITMAPINFO)lpDest;
		lpbmi->bmiHeader.biSize      = sizeof(BITMAPINFOHEADER);
		lpbmi->bmiHeader.biWidth     = lpbmci->bmciHeader.bcWidth;
		lpbmi->bmiHeader.biHeight    = lpbmci->bmciHeader.bcHeight;
		lpbmi->bmiHeader.biPlanes    = lpbmci->bmciHeader.bcPlanes;
		lpbmi->bmiHeader.biBitCount  = lpbmci->bmciHeader.bcBitCount;
		lpbmi->bmiHeader.biSizeImage = uImageSize;
		lpbmi->bmiHeader.biClrUsed   = uColors;

		__try
		{
			if (uColors > 0)
			{
				RGBQUAD rgbQuad = {0};
				for (UINT u = 0; u < uColors; u++)
				{
					rgbQuad.rgbRed   = lpbmci->bmciColors[u].rgbtRed;
					rgbQuad.rgbGreen = lpbmci->bmciColors[u].rgbtGreen;
					rgbQuad.rgbBlue  = lpbmci->bmciColors[u].rgbtBlue;
					rgbQuad.rgbReserved = 0;
					lpbmi->bmiColors[u] = rgbQuad;
				}
			}

			CopyMemory(FindDibBits((LPCSTR)lpDest), FindDibBits((LPCSTR)lpSrc), uImageSize);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) { ; }

		GlobalUnlock(hNewDib);
	}
	else if (dwSrcHeaderSize == 52 || dwSrcHeaderSize == 56 || dwSrcHeaderSize == sizeof(BITMAPV4HEADER))
	{
		// Create a new DIBv5 from an extended DIBv3 or from a DIBv4
		hNewDib = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DDESHARE,
			cbLen + sizeof(BITMAPV5HEADER) - dwSrcHeaderSize);
		if (hNewDib == NULL)
		{
			GlobalUnlock((HGLOBAL)hDib);
			return NULL;
		}

		LPBYTE lpDest = (LPBYTE)GlobalLock(hNewDib);
		if (lpDest == NULL)
		{
			GlobalFree(hNewDib);
			GlobalUnlock((HGLOBAL)hDib);
			return NULL;
		}

		__try
		{
			CopyMemory(lpDest, lpSrc, dwSrcHeaderSize);
			CopyMemory(lpDest + sizeof(BITMAPV5HEADER), lpSrc + dwSrcHeaderSize, cbLen - dwSrcHeaderSize);

			// Update the v5 header with the new header size
			((LPBITMAPV5HEADER)lpDest)->bV5Size = sizeof(BITMAPV5HEADER);

			if (dwSrcHeaderSize == 52 || dwSrcHeaderSize == 56)
				((LPBITMAPV5HEADER)lpDest)->bV5CSType = LCS_sRGB;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) { ; }

		GlobalUnlock(hNewDib);
	}
	else if (DibHasColorProfile((LPCSTR)lpSrc))
	{
		// The ICC profile data of a DIBv5 in memory must follow the color table
		LPBITMAPV5HEADER lpbiv5 = (LPBITMAPV5HEADER)lpSrc;
		DWORD dwInfoSize = lpbiv5->bV5Size + PaletteSize((LPCSTR)lpSrc);
		DWORD dwProfileSize = lpbiv5->bV5ProfileSize;
		SIZE_T cbImageSize = DibImageSize((LPCSTR)lpSrc);
		SIZE_T cbDibSize = cbImageSize + dwInfoSize + dwProfileSize;

		if (cbDibSize > cbLen)
		{
			GlobalUnlock((HGLOBAL)hDib);
			return NULL;
		}

		hNewDib = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DDESHARE, cbDibSize);
		if (hNewDib == NULL)
		{
			GlobalUnlock((HGLOBAL)hDib);
			return NULL;
		}

		LPBYTE lpDest = (LPBYTE)GlobalLock(hNewDib);
		if (lpDest == NULL)
		{
			GlobalFree(hNewDib);
			GlobalUnlock((HGLOBAL)hDib);
			return NULL;
		}

		__try
		{
			CopyMemory(lpDest, lpSrc, dwInfoSize);
			CopyMemory(lpDest + dwInfoSize, lpSrc + lpbiv5->bV5ProfileData, dwProfileSize);
			CopyMemory(lpDest + dwInfoSize + dwProfileSize, FindDibBits((LPCSTR)lpSrc), cbImageSize);

			// Update the header with the new position of the profile data
			((LPBITMAPV5HEADER)lpDest)->bV5ProfileData = dwInfoSize;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) { ; }

		GlobalUnlock(hNewDib);
	}
	else
	{
		// Copy the packed DIB
		hNewDib = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cbLen);
		if (hNewDib == NULL)
		{
			GlobalUnlock((HGLOBAL)hDib);
			return NULL;
		}

		LPBYTE lpDest = (LPBYTE)GlobalLock(hNewDib);
		if (lpDest == NULL)
		{
			GlobalFree(hNewDib);
			GlobalUnlock((HGLOBAL)hDib);
			return NULL;
		}

		__try { CopyMemory(lpDest, lpSrc, cbLen); }
		__except (EXCEPTION_EXECUTE_HANDLER) { ; }

		GlobalUnlock(hNewDib);
	}

	GlobalUnlock((HGLOBAL)hDib);

	// Set the clipboard format according to the created DIB
	if (puFormat != NULL)
		*puFormat = dwSrcHeaderSize <= sizeof(BITMAPINFOHEADER) ? CF_DIB : CF_DIBV5;

	return (HANDLE)hNewDib;
}

////////////////////////////////////////////////////////////////////////////////////////////////

HPALETTE CreateDibPalette(HANDLE hDib)
{
	LPSTR lpbi = NULL;
	LPLOGPALETTE lppal = NULL;
	HPALETTE hpal = NULL;

	if (hDib == NULL)
		goto CreateHalftonePal;

	// Create a palette from the colors of the DIB
	lpbi = (LPSTR)GlobalLock((HGLOBAL)hDib);
	if (lpbi == NULL)
		goto CreateHalftonePal;

	UINT uNumColors = DibNumColors(lpbi);
	if (uNumColors == 0)
		goto CreateHalftonePal;

	lppal = (LPLOGPALETTE)MyGlobalAllocPtr(GHND, sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * uNumColors);
	if (lppal == NULL)
		goto CreateHalftonePal;

	lppal->palVersion = 0x300;
	lppal->palNumEntries = (WORD)uNumColors;

	if (IS_OS2PM_DIB(lpbi))
	{
		LPBITMAPCOREINFO lpbmc = (LPBITMAPCOREINFO)lpbi;
		for (UINT i = 0; i < uNumColors; i++)
		{
			lppal->palPalEntry[i].peRed   = lpbmc->bmciColors[i].rgbtRed;
			lppal->palPalEntry[i].peGreen = lpbmc->bmciColors[i].rgbtGreen;
			lppal->palPalEntry[i].peBlue  = lpbmc->bmciColors[i].rgbtBlue;
			lppal->palPalEntry[i].peFlags = 0;
		}
	}
	else
	{
		LPRGBQUAD lprgbqColors = (LPRGBQUAD)FindDibPalette(lpbi);
		for (UINT i = 0; i < uNumColors; i++)
		{
			lppal->palPalEntry[i].peRed   = lprgbqColors[i].rgbRed;
			lppal->palPalEntry[i].peGreen = lprgbqColors[i].rgbGreen;
			lppal->palPalEntry[i].peBlue  = lprgbqColors[i].rgbBlue;
			lppal->palPalEntry[i].peFlags = 0;
		}
	}

	hpal = CreatePalette(lppal);

CreateHalftonePal:

	if (lppal != NULL)
		MyGlobalFreePtr((LPVOID)lppal);
	if (lpbi != NULL)
		GlobalUnlock((HGLOBAL)hDib);

	if (hpal == NULL)
	{ // Create a halftone palette
		HDC hdc = GetDC(NULL);
		hpal = CreateHalftonePalette(hdc);
		ReleaseDC(NULL, hdc);
	}

	return hpal;
}

////////////////////////////////////////////////////////////////////////////////////////////////

UINT DibNumColors(LPCSTR lpbi)
{
	if (lpbi == NULL)
		return 0;

	WORD  wBitCount = 0;
	DWORD dwClrUsed = 0;

	if (IS_OS2PM_DIB(lpbi))
		wBitCount = ((LPBITMAPCOREHEADER)lpbi)->bcBitCount;
	else
	{
		wBitCount = ((LPBITMAPINFOHEADER)lpbi)->biBitCount;
		dwClrUsed = ((LPBITMAPINFOHEADER)lpbi)->biClrUsed;
		// Allow up to 4096 palette entries
		if (dwClrUsed > 0 && dwClrUsed <= (1U << 12U))
			return dwClrUsed;
	}

	if (wBitCount > 0 && wBitCount <= 12)
		return (1U << wBitCount);
	else
		return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////

UINT ColorMasksSize(LPCSTR lpbi)
{
	if (lpbi == NULL || !IS_WIN30_DIB(lpbi))
		return 0;

	if (((LPBITMAPINFOHEADER)lpbi)->biCompression == BI_BITFIELDS)
		return 3 * sizeof(DWORD);
	else if (((LPBITMAPINFOHEADER)lpbi)->biCompression == BI_ALPHABITFIELDS)
		return 4 * sizeof(DWORD);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////

UINT PaletteSize(LPCSTR lpbi)
{
	if (lpbi == NULL)
		return 0;

	return DibNumColors(lpbi) * (IS_OS2PM_DIB(lpbi) ? sizeof(RGBTRIPLE) : sizeof(RGBQUAD));
}

////////////////////////////////////////////////////////////////////////////////////////////////

UINT DibImageSize(LPCSTR lpbi)
{
	if (lpbi == NULL)
		return 0;

	if (IS_OS2PM_DIB(lpbi))
	{
		LPBITMAPCOREHEADER lpbc = (LPBITMAPCOREHEADER)lpbi;
		return WIDTHBYTES(lpbc->bcWidth * lpbc->bcPlanes * lpbc->bcBitCount) * lpbc->bcHeight;
	}

	LPBITMAPINFOHEADER lpbih = (LPBITMAPINFOHEADER)lpbi;

	if (lpbih->biSizeImage != 0)
		return lpbih->biSizeImage;

	return WIDTHBYTES(abs(lpbih->biWidth) * lpbih->biPlanes * lpbih->biBitCount) * abs(lpbih->biHeight);
}

////////////////////////////////////////////////////////////////////////////////////////////////

UINT DibBitsOffset(LPCSTR lpbi)
{
	if (lpbi == NULL)
		return 0;

	return *(LPDWORD)lpbi + ColorMasksSize(lpbi) + PaletteSize(lpbi);
}

////////////////////////////////////////////////////////////////////////////////////////////////

LPBYTE FindDibPalette(LPCSTR lpbi)
{
	if (lpbi == NULL)
		return NULL;

	return (LPBYTE)lpbi + *(LPDWORD)lpbi + ColorMasksSize(lpbi);
}

////////////////////////////////////////////////////////////////////////////////////////////////

LPBYTE FindDibBits(LPCSTR lpbi)
{
	if (lpbi == NULL)
		return NULL;

	return (LPBYTE)lpbi + *(LPDWORD)lpbi + ColorMasksSize(lpbi) + PaletteSize(lpbi);
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DibHasColorProfile(LPCSTR lpbi)
{
	if (lpbi == NULL || !IS_WIN50_DIB(lpbi))
		return FALSE;

	LPBITMAPV5HEADER lpbiv5 = (LPBITMAPV5HEADER)lpbi;
	return (lpbiv5->bV5ProfileData != 0 && lpbiv5->bV5ProfileSize != 0 &&
		(lpbiv5->bV5CSType == PROFILE_LINKED || lpbiv5->bV5CSType == PROFILE_EMBEDDED));
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DibHasColorSpaceData(LPCSTR lpbi)
{
	if (lpbi == NULL)
		return FALSE;

	LPBITMAPV5HEADER lpbiv5 = (LPBITMAPV5HEADER)lpbi;
	DWORD dwSize = lpbiv5->bV5Size;

	return ((dwSize >= sizeof(BITMAPV4HEADER) && lpbiv5->bV5CSType == LCS_CALIBRATED_RGB) ||
		(dwSize >= sizeof(BITMAPV5HEADER) && lpbiv5->bV5CSType == PROFILE_EMBEDDED));
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DibIsVideoCompressed(LPCSTR lpbi)
{
	if (lpbi == NULL || !IS_WIN30_DIB(lpbi))
		return FALSE;

	DWORD dwCompression = ((LPBITMAPINFOHEADER)lpbi)->biCompression;

	return (isprint(dwCompression & 0xff) &&
		isprint((dwCompression >> 8) & 0xff) &&
		isprint((dwCompression >> 16) & 0xff) &&
		isprint((dwCompression >> 24) & 0xff));
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DibIsCMYK(LPCSTR lpbi)
{
	if (lpbi == NULL || *(LPDWORD)lpbi < sizeof(BITMAPV4HEADER))
		return FALSE;

	return (((LPBITMAPV4HEADER)lpbi)->bV4CSType == LCS_DEVICE_CMYK);
}

////////////////////////////////////////////////////////////////////////////////////////////////

static __inline int Mul8Bit(int a, int b)
{
	int t = a * b + 128;
	return (t + (t >> 8)) >> 8;
}

////////////////////////////////////////////////////////////////////////////////////////////////

static BYTE GetColorValue(DWORD dwPixel, DWORD dwMask)
{
	if (dwMask == 0)
		return 0;

	DWORD dwColor = dwPixel & dwMask;

	while ((dwMask & 0x80000000) == 0)
	{
		dwColor = dwColor << 1;
		dwMask  = dwMask  << 1;
	}

	return HIBYTE(HIWORD(dwColor));
}

////////////////////////////////////////////////////////////////////////////////////////////////
