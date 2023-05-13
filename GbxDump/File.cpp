////////////////////////////////////////////////////////////////////////////////////////////////
// File.cpp - Copyright (c) 2010-2023 by Electron.
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
#include "file.h"
#include "dumpbmp.h"
#include "..\libjpeg\jpeglib.h"
#include "..\libwebp\src\webp\decode.h"
#include "..\crunch\inc\crnlib.h"
#include "..\crunch\crnlib\crn_miniz.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module

BYTE GetColorValue(DWORD dwPixel, DWORD dwMask);

////////////////////////////////////////////////////////////////////////////////////////////////
// Displays the File Open or Save As dialog box

BOOL GetFileName(HWND hDlg, LPTSTR lpszFileName, SIZE_T cchStringLen, LPDWORD lpdwFilterIndex, BOOL bSave)
{
	// Filter String
	TCHAR szFilter[512];
	szFilter[0] = TEXT('\0');
	if (bSave)
		LoadString(g_hInstance, g_bGerUI ? IDS_GER_FILTER_PNG : IDS_ENG_FILTER_PNG, szFilter, _countof(szFilter));
	else
		LoadString(g_hInstance, g_bGerUI ? IDS_GER_FILTER_GBX : IDS_ENG_FILTER_GBX, szFilter, _countof(szFilter));
	TCHAR* psz = szFilter;
	while (psz = _tcschr(psz, TEXT('|')))
		*psz++ = TEXT('\0');

	// Initial Directory
	TCHAR szInitialDir[MAX_PATH];
	TCHAR* pszInitialDir = szInitialDir;
	if (lpszFileName != NULL && lpszFileName[0] != TEXT('\0'))
	{
		lstrcpyn(pszInitialDir, lpszFileName, _countof(szInitialDir));
		TCHAR* token = _tcsrchr(pszInitialDir, TEXT('\\'));
		if (token != NULL)
			pszInitialDir[token - szInitialDir] = TEXT('\0');
		else
			pszInitialDir = NULL;
	}
	else
		pszInitialDir = NULL;

	// File Name
	TCHAR szFile[MAX_PATH];
	if (bSave)
	{
		if (lpszFileName != NULL && lpszFileName[0] != TEXT('\0'))
			lstrcpyn(szFile, lpszFileName, _countof(szFile));
		else
			_tcscpy(szFile, TEXT("*"));
		TCHAR* token = _tcsrchr(szFile, TEXT('.'));
		if (token != NULL)
			szFile[token - szFile] = TEXT('\0');
		_tcsncat(szFile, TEXT(".png"), _countof(szFile)-_tcslen(szFile)-1);
	}
	else
		szFile[0] = TEXT('\0');

	OPENFILENAME of = { 0 };
	of.lStructSize     = sizeof(OPENFILENAME);
	of.hwndOwner       = hDlg;
	of.Flags           = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	of.lpstrFile       = szFile;
	of.nMaxFile        = _countof(szFile);
	of.lpstrFilter     = szFilter;
	of.nFilterIndex    = (lpdwFilterIndex != NULL) ? *lpdwFilterIndex : 1;
	of.lpstrDefExt     = bSave ? TEXT("png") : TEXT("gbx");
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
		if (lpszFileName != NULL)
			lstrcpyn(lpszFileName, szFile, (int)cchStringLen);
		if (lpdwFilterIndex != NULL)
			*lpdwFilterIndex = of.nFilterIndex;
	}

	return bRet;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Saves a DIB as a Windows Bitmap file

BOOL SaveBmpFile(LPCTSTR lpszFileName, HANDLE hDIB)
{
	if (hDIB == NULL || lpszFileName == NULL)
		return FALSE;

	LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	if (lpbi == NULL)
		return FALSE;

	DWORD dwPaletteSize = 0;
	DWORD dwDIBSize = 0;
	DWORD dwOffBits = 0;
	DWORD dwBmBitsSize = 0;
	DWORD dwFileSize = 0;

	if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
	{
		LPBITMAPCOREHEADER lpbc = (LPBITMAPCOREHEADER)lpbi;
		if (lpbc->bcBitCount < 16)
			dwPaletteSize = (1 << lpbc->bcBitCount) * (DWORD)sizeof(RGBTRIPLE);
		dwDIBSize = lpbc->bcSize + dwPaletteSize;
		dwBmBitsSize = (((lpbc->bcWidth * lpbc->bcPlanes *
			lpbc->bcBitCount) + 31) / 32 * 4) * lpbc->bcHeight;
	}
	else
	{
		dwPaletteSize = lpbi->biClrUsed * sizeof(RGBQUAD);
		if (dwPaletteSize == 0 && lpbi->biBitCount < 16)
			dwPaletteSize = (1 << lpbi->biBitCount) * (DWORD)sizeof(RGBQUAD);
		dwDIBSize = lpbi->biSize + dwPaletteSize;
		if (lpbi->biSize == sizeof(BITMAPINFOHEADER) && lpbi->biCompression == BI_BITFIELDS)
			dwDIBSize += 3 * sizeof(DWORD);
		else if (lpbi->biSize == sizeof(BITMAPINFOHEADER) && lpbi->biCompression == BI_ALPHABITFIELDS)
			dwDIBSize += 4 * sizeof(DWORD);
		if (lpbi->biCompression == BI_RGB || lpbi->biCompression == BI_BITFIELDS ||
			lpbi->biCompression == BI_ALPHABITFIELDS || lpbi->biCompression == BI_CMYK)
			dwBmBitsSize = (((lpbi->biWidth * lpbi->biPlanes *
				lpbi->biBitCount) + 31) / 32 * 4) * abs(lpbi->biHeight);
		else
			dwBmBitsSize = lpbi->biSizeImage;
	}

	dwOffBits = sizeof(BITMAPFILEHEADER) + dwDIBSize;
	dwDIBSize += dwBmBitsSize;
	dwFileSize = dwOffBits + dwBmBitsSize;

	BITMAPFILEHEADER bmfHdr;
	bmfHdr.bfType = 0x4d42;
	bmfHdr.bfSize = dwFileSize;
	bmfHdr.bfReserved1 = 0;
	bmfHdr.bfReserved2 = 0;
	bmfHdr.bfOffBits = dwOffBits;

	HANDLE hFile = CreateFile(lpszFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		GlobalUnlock(hDIB);
		return FALSE;
	}

	DWORD dwWrite;
	if (!WriteFile(hFile, &bmfHdr, sizeof(BITMAPFILEHEADER), &dwWrite, NULL) ||
		dwWrite != sizeof(BITMAPFILEHEADER))
	{
		GlobalUnlock(hDIB);
		CloseHandle(hFile);
		DeleteFile(lpszFileName);
		return FALSE;
	}

	if (!WriteFile(hFile, (LPVOID)lpbi, dwDIBSize, &dwWrite, NULL) || dwWrite != dwDIBSize)
	{
		GlobalUnlock(hDIB);
		CloseHandle(hFile);
		DeleteFile(lpszFileName);
		return FALSE;
	}

	GlobalUnlock(hDIB);
	CloseHandle(hFile);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Saves a DIB as a PNG file using miniz. Supports only
// 8-bit grayscale, 16-bit, 24-bit and 32-bit color images.

BOOL SavePngFile(LPCTSTR lpszFileName, HANDLE hDIB)
{
	if (hDIB == NULL || lpszFileName == NULL)
		return FALSE;

	LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	if (lpbi == NULL || lpbi->biSize < sizeof(BITMAPINFOHEADER) || lpbi->biBitCount < 8 ||
		(lpbi->biCompression != BI_RGB && lpbi->biCompression != BI_BITFIELDS))
		return FALSE;

	DWORD dwPaletteSize = lpbi->biClrUsed * (DWORD)sizeof(RGBQUAD);
	if (dwPaletteSize == 0 && lpbi->biBitCount <= 8)
		dwPaletteSize = (1 << lpbi->biBitCount) * (DWORD)sizeof(RGBQUAD);

	LPBYTE lpDIB = (LPBYTE)lpbi + lpbi->biSize + dwPaletteSize;
	if (lpbi->biSize == sizeof(BITMAPINFOHEADER) && lpbi->biCompression == BI_BITFIELDS)
		lpDIB += 3 * sizeof(DWORD);

	BOOL bFlipImage = FALSE;
	LONG lWidth = lpbi->biWidth;
	LONG lHeight = lpbi->biHeight;
	if (lHeight < 0)
	{
		lHeight = -lHeight;
		bFlipImage = TRUE;
	}

	BOOL bIsCMYK = lpbi->biSize >= sizeof(BITMAPV4HEADER) &&
		((LPBITMAPV4HEADER)lpbi)->bV4CSType == LCS_DEVICE_CMYK;

	// CMYK DIBs are converted to RGB
	INT nNumChannels = bIsCMYK ? 3 : lpbi->biBitCount >> 3;

	// 16-bit bitmaps are saved with 3 channels (or 4 incl. alpha channel)
	LONG lSizeImage = lHeight * lWidth * (lpbi->biBitCount == 16 ? 4 : nNumChannels);
	if (lSizeImage == 0)
	{
		GlobalUnlock(hDIB);
		return FALSE;
	}

	LPBYTE lpRGBA = (LPBYTE)GlobalAllocPtr(GHND, lSizeImage);
	if (lpRGBA == NULL)
	{
		GlobalUnlock(hDIB);
		return FALSE;
	}

	LONG h, w;
	LPBYTE lpSrc, lpDest;
	LPDWORD lpdwColorMasks;
	DWORD dwColor, dwRedMask, dwGreenMask, dwBlueMask, dwAlphaMask;
	DWORD dwIncrement = ((lWidth * lpbi->biBitCount) + 31) / 32 * 4;

	__try
	{
		switch (nNumChannels)
		{
			case 1:
				for (h = 0; h < lHeight; h++)
				{
					lpSrc = lpDIB + (bFlipImage ? h : lHeight-1 - h) * dwIncrement;
					lpDest = lpRGBA + h * lWidth * nNumChannels;
					for (w = 0; w < lWidth; w++)
						*lpDest++ = *lpSrc++;
				}
				break;

			case 2:
				if (lpbi->biCompression == BI_BITFIELDS)
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
					dwAlphaMask = 0x00000000;
				}

				nNumChannels = dwAlphaMask ? 4 : 3;

				for (h = 0; h < lHeight; h++)
				{
					lpSrc = lpDIB + (bFlipImage ? h : lHeight-1 - h) * dwIncrement;
					lpDest = lpRGBA + h * lWidth * nNumChannels;
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

			case 3:
				if (bIsCMYK)
				{
					BYTE cInvKey;
					for (h = 0; h < lHeight; h++)
					{
						lpSrc = lpDIB + (bFlipImage ? h : lHeight-1 - h) * dwIncrement;
						lpDest = lpRGBA + h * lWidth * nNumChannels;
						for (w = 0; w < lWidth; w++)
						{
							cInvKey = 0xFF - lpSrc[3];
							*lpDest++ = (0xFF - lpSrc[2]) * cInvKey / 0xFF;
							*lpDest++ = (0xFF - lpSrc[1]) * cInvKey / 0xFF;
							*lpDest++ = (0xFF - lpSrc[0]) * cInvKey / 0xFF;
							lpSrc += 4;
						}
					}
				}
				else
				{
					for (h = 0; h < lHeight; h++)
					{
						lpSrc = lpDIB + (bFlipImage ? h : lHeight-1 - h) * dwIncrement;
						lpDest = lpRGBA + h * lWidth * nNumChannels;
						for (w = 0; w < lWidth; w++)
						{
							lpDest[2] = *lpSrc++;
							lpDest[1] = *lpSrc++;
							lpDest[0] = *lpSrc++;
							lpDest += 3;
						}
					}
				}
				break;

			case 4:
				if (lpbi->biCompression == BI_BITFIELDS)
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

				for (h = 0; h < lHeight; h++)
				{
					lpSrc = lpDIB + (bFlipImage ? h : lHeight-1 - h) * dwIncrement;
					lpDest = lpRGBA + h * lWidth * nNumChannels;
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

				break;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) { ; }

	size_t cbSize = 0;
	LPVOID lpPNG = tdefl_write_image_to_png_file_in_memory(lpRGBA, lWidth, lHeight, nNumChannels, &cbSize);
	if (lpPNG == NULL)
	{
		GlobalFreePtr((LPVOID)lpRGBA);
		GlobalUnlock(hDIB);
		return FALSE;
	}

	HANDLE hFile = CreateFile(lpszFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		mz_free(lpPNG);
		GlobalFreePtr((LPVOID)lpRGBA);
		GlobalUnlock(hDIB);
		return FALSE;
	}

	DWORD dwWrite;
	if (!WriteFile(hFile, lpPNG, (DWORD)cbSize, &dwWrite, NULL) || dwWrite != cbSize)
	{
		mz_free(lpPNG);
		GlobalFreePtr((LPVOID)lpRGBA);
		GlobalUnlock(hDIB);
		CloseHandle(hFile);
		DeleteFile(lpszFileName);
		return FALSE;
	}

	mz_free(lpPNG);
	GlobalFreePtr((LPVOID)lpRGBA);
	GlobalUnlock(hDIB);
	CloseHandle(hFile);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// JPEG decoder using the Independent JPEG Group's JPEG software
// Based on code by Thomas W. Lipp from german Microsoft Systems Journal Nov./Dec. 1993
// Note: The arrangement of the color components must be changed in jmorecfg.h from RGB to BGR

// Data Types
typedef struct jpeg_decompress_struct j_decompress;
typedef struct jpeg_error_mgr         j_error_mgr;

// JPEG decompression structure (overloads jpeg_decompress_struct)
typedef struct _JPEG_DECOMPRESS
{
	j_decompress    jInfo;               // Decompression structure
	j_error_mgr     jError;              // Error manager
	BOOL            bNeedDestroy;        // jInfo must be destroyed
	UINT            uWidth;              // Width
	UINT            uHeight;             // Height
	UINT            uBPP;                // Bit per pixel
	UINT            uColors;             // Number of colours in the palette
	UINT            uWinColors;          // Number of colors after optimization
	DWORD           dwXPelsPerMeter;     // Horizontal resolution
	DWORD           dwYPelsPerMeter;     // Vertical Resolution
	DWORD           dwIncrement;         // Size of a picture row
	DWORD           dwSize;              // Size of the image
	INT             nTraceLevel;         // Max. message level that will be displayed
} JPEG_DECOMPRESS, *LPJPEG_DECOMPRESS;

static jmp_buf JmpBuffer;  // Buffer for processor status

// Helper functions
void cleanup_jpeg_to_dib(LPJPEG_DECOMPRESS lpJpegDecompress, HANDLE hDIB);
void set_error_manager(j_common_ptr pjInfo, j_error_mgr *pjError);

// Callback functions
METHODDEF(void my_error_exit(j_common_ptr pjInfo));
METHODDEF(void my_output_message(j_common_ptr pjInfo));
METHODDEF(void my_emit_message(j_common_ptr pjInfo, int nMessageLevel));

////////////////////////////////////////////////////////////////////////////////////////////////
// JpegToDib: Converts a JPEG image to a DIB

HANDLE JpegToDib(LPVOID lpJpegData, DWORD dwLenData, BOOL bFlipImage, INT nTraceLevel)
{
	HANDLE hDIB = NULL;  // Handle of the DIB

	// Initialize the JPEG decompression object
	JPEG_DECOMPRESS JpegDecompress; // JPEG decompression structure
	j_decompress_ptr pjInfo = &JpegDecompress.jInfo;
	JpegDecompress.nTraceLevel = nTraceLevel;
	set_error_manager((j_common_ptr)pjInfo, &JpegDecompress.jError);

	// Save processor status for error handling
	if (setjmp(JmpBuffer))
	{
		cleanup_jpeg_to_dib(&JpegDecompress, hDIB);
		return NULL;
	}

	jpeg_create_decompress(pjInfo);
	JpegDecompress.bNeedDestroy = TRUE;

	// Determine data source
	jpeg_mem_src(pjInfo, (LPBYTE)lpJpegData, dwLenData);

	// Determine image information
	jpeg_read_header(pjInfo, TRUE);

	// Image resolution
	JpegDecompress.dwXPelsPerMeter = 0;
	JpegDecompress.dwYPelsPerMeter = 0;
	if (pjInfo->saw_JFIF_marker)
		switch (pjInfo->density_unit)
		{
			case 1:  // Pixel per inch
				JpegDecompress.dwXPelsPerMeter = ((DWORD)pjInfo->X_density * 5000ul) / 127ul;
				if (JpegDecompress.dwXPelsPerMeter == (DWORD)-1)
					JpegDecompress.dwXPelsPerMeter = 0;
				JpegDecompress.dwYPelsPerMeter = ((DWORD)pjInfo->Y_density * 5000ul) / 127ul;
				if (JpegDecompress.dwYPelsPerMeter == (DWORD)-1)
					JpegDecompress.dwYPelsPerMeter = 0;
				break;
			case 2:  // Pixel per cm
				JpegDecompress.dwXPelsPerMeter = (DWORD)pjInfo->X_density * 100ul;
				JpegDecompress.dwYPelsPerMeter = (DWORD)pjInfo->Y_density * 100ul;
				break;
			default: // No unit of measurement (image ratio only) or undefined
				break;
		}

	// Decompression settings
	if (JpegDecompress.jInfo.jpeg_color_space == JCS_GRAYSCALE)
	{
		JpegDecompress.jInfo.out_color_space   = JCS_GRAYSCALE;
		JpegDecompress.jInfo.quantize_colors   = (boolean)FALSE;
		JpegDecompress.jInfo.two_pass_quantize = (boolean)FALSE;
		JpegDecompress.jInfo.dither_mode       = JDITHER_NONE;
	}
	else
	{
		JpegDecompress.jInfo.out_color_space   = JCS_RGB;
		JpegDecompress.jInfo.quantize_colors   = (boolean)FALSE;
		JpegDecompress.jInfo.two_pass_quantize = (boolean)FALSE;
		JpegDecompress.jInfo.dither_mode       = JDITHER_NONE;
		JpegDecompress.jInfo.desired_number_of_colors = 236;
	}

	// Set parameters for decompression
	pjInfo->dct_method = JDCT_FLOAT;  // Quality

	// Start decompression in the JPEG library
	jpeg_start_decompress(pjInfo);

	// Determine output image format
	JpegDecompress.uWidth  = pjInfo->output_width;
	JpegDecompress.uHeight = pjInfo->output_height;
	if (pjInfo->output_components == 3)
	{  // RGB image
		JpegDecompress.uBPP       = 24;
		JpegDecompress.uColors    = 0;
		JpegDecompress.uWinColors = 0;
	}
	else
	{  // Grayscale or palette image
		JpegDecompress.uBPP = 8;
		if (pjInfo->out_color_space == JCS_GRAYSCALE)
		{
			JpegDecompress.uColors    = 256;
			JpegDecompress.uWinColors = 256;
		}
		else
		{
			JpegDecompress.uColors = pjInfo->actual_number_of_colors;
			JpegDecompress.uWinColors = min(JpegDecompress.uColors + 20, 256);
		}
	}

	// Size of one DIB row
	JpegDecompress.dwIncrement = (((DWORD)JpegDecompress.uWidth * JpegDecompress.uBPP + 31) / 32) * 4;
	// Size of the DIB data
	JpegDecompress.dwSize = JpegDecompress.dwIncrement * JpegDecompress.uHeight;

	// Create a Windows Bitmap
	// Allocate memory for the DIB
	hDIB = GlobalAlloc(GHND, sizeof(BITMAPINFOHEADER) +
		JpegDecompress.uWinColors * sizeof(RGBQUAD) + JpegDecompress.dwSize);
	if (hDIB == NULL)
	{
		cleanup_jpeg_to_dib(&JpegDecompress, hDIB);
		return NULL;
	}

	// Fill bitmap information block
	LPBITMAPINFO lpBMI = (LPBITMAPINFO)GlobalLock(hDIB);
	LPBITMAPINFOHEADER lpBI = (LPBITMAPINFOHEADER)lpBMI;
	if (lpBI == NULL)
	{
		cleanup_jpeg_to_dib(&JpegDecompress, hDIB);
		return NULL;
	}

	lpBI->biSize          = sizeof(BITMAPINFOHEADER);
	lpBI->biWidth         = JpegDecompress.uWidth;
	lpBI->biHeight        = JpegDecompress.uHeight;
	lpBI->biPlanes        = 1;
	lpBI->biBitCount      = (WORD)JpegDecompress.uBPP;
	lpBI->biCompression   = BI_RGB;
	lpBI->biSizeImage     = 0;
	lpBI->biXPelsPerMeter = JpegDecompress.dwXPelsPerMeter;
	lpBI->biYPelsPerMeter = JpegDecompress.dwYPelsPerMeter;
	lpBI->biClrUsed       = JpegDecompress.uWinColors;
	lpBI->biClrImportant  = 0;

	// Create palette
	if (JpegDecompress.uBPP == 8)
	{
		if (pjInfo->out_color_space == JCS_GRAYSCALE)
		{
			// Grayscale image
			for (UINT u = 0; u < JpegDecompress.uColors; u++)
			{  // Create grayscale
				lpBMI->bmiColors[u].rgbRed      = (BYTE)u;
				lpBMI->bmiColors[u].rgbGreen    = (BYTE)u;
				lpBMI->bmiColors[u].rgbBlue     = (BYTE)u;
				lpBMI->bmiColors[u].rgbReserved = (BYTE)0;
			}
		}
		else
		{
			// Palette image
			for (UINT u = 0; u < JpegDecompress.uColors; u++)
			{  // Copy palette
				lpBMI->bmiColors[u].rgbRed      = pjInfo->colormap[0][u];
				lpBMI->bmiColors[u].rgbGreen    = pjInfo->colormap[1][u];
				lpBMI->bmiColors[u].rgbBlue     = pjInfo->colormap[2][u];
				lpBMI->bmiColors[u].rgbReserved = 0;
			}

			// Fill up palette with system colors
			if (JpegDecompress.uWinColors > JpegDecompress.uColors)
			{
				RGBQUAD SystemPalette[] =
				{
					{ 0x00, 0x00, 0x00, 0x00 },
					{ 0xFF, 0xFF, 0xFF, 0x00 },

					{ 0x00, 0x00, 0xFF, 0x00 },
					{ 0x00, 0xFF, 0x00, 0x00 },
					{ 0x00, 0xFF, 0xFF, 0x00 },
					{ 0xFF, 0x00, 0x00, 0x00 },
					{ 0xFF, 0x00, 0xFF, 0x00 },
					{ 0xFF, 0xFF, 0x00, 0x00 },

					{ 0x00, 0x00, 0x80, 0x00 },
					{ 0x00, 0x80, 0x00, 0x00 },
					{ 0x00, 0x80, 0x80, 0x00 },
					{ 0x80, 0x00, 0x00, 0x00 },
					{ 0x80, 0x00, 0x80, 0x00 },
					{ 0x80, 0x80, 0x00, 0x00 },
					{ 0x80, 0x80, 0x80, 0x00 },
					{ 0xC0, 0xC0, 0xC0, 0x00 },

					{ 0xA4, 0xA0, 0xA0, 0x00 },
					{ 0xC0, 0xDC, 0xC0, 0x00 },
					{ 0xF0, 0xCA, 0xA6, 0x00 },
					{ 0xF0, 0xFB, 0xFF, 0x00 }
				};

				CopyMemory(&(lpBMI->bmiColors[JpegDecompress.uColors]), SystemPalette, min(sizeof(SystemPalette),
					(JpegDecompress.uWinColors - JpegDecompress.uColors) * sizeof(RGBQUAD)));
			}
		}
	}

	// Determine pointer to start of image data
	LPBYTE lpDIB = (LPBYTE)&(lpBMI->bmiColors[lpBI->biClrUsed]);
	LPBYTE lpBits = NULL;     // Pointer to a DIB image row
	JSAMPROW lpScanlines[1];  // Pointer to a scanline
	JDIMENSION uScanline;     // Row index

	// Copy image rows (scanlines)
	while (pjInfo->output_scanline < pjInfo->output_height)
	{
		uScanline = bFlipImage ? pjInfo->output_scanline : JpegDecompress.uHeight-1 - pjInfo->output_scanline;
		lpBits = lpDIB + uScanline * JpegDecompress.dwIncrement;
		lpScanlines[0] = lpBits;
		jpeg_read_scanlines(pjInfo, lpScanlines, 1);  // Decompress one line
	}

	// Finish decompression
	GlobalUnlock(hDIB);
	jpeg_finish_decompress(pjInfo);

	// Free the JPEG decompression object
	jpeg_destroy_decompress(pjInfo);
	JpegDecompress.bNeedDestroy = FALSE;

	// Return the DIB handle
	return hDIB;
}

////////////////////////////////////////////////////////////////////////////////////////////////

// cleanup_jpeg_to_dib: Performs housekeeping
void cleanup_jpeg_to_dib(LPJPEG_DECOMPRESS lpJpegDecompress, HANDLE hDIB)
{
	// Destroy the JPEG decompress object
	if (lpJpegDecompress)
		if (lpJpegDecompress->bNeedDestroy)
			jpeg_destroy_decompress(&lpJpegDecompress->jInfo);

	// Release the DIB
	if (hDIB != NULL)
	{
		GlobalUnlock(hDIB);
		GlobalFree(hDIB);
		hDIB = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

// set_error_manager: Registers the callback functions for the error manager
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

// my_error_exit: Error handling
// If an error occurs in LIBJPEG, my_error_exit is called. In this case, a
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

// my_output_message: Text output
// This callback function formats a message and displays it on the screen.
void my_output_message(j_common_ptr pjInfo)
{
	char szBuffer[JMSG_LENGTH_MAX];    // Buffer for message text
	TCHAR szMessage[JMSG_LENGTH_MAX];  // Buffer for message text

	// Format text
	(*pjInfo->err->format_message)(pjInfo, szBuffer);

	// Output text
	mbstowcs(szMessage, szBuffer, JMSG_LENGTH_MAX - 1);

	HWND hwndCtl = NULL;
	HWND hwndDlg = GetActiveWindow();
	if (hwndDlg != NULL)
		hwndCtl = GetDlgItem(hwndDlg, IDC_OUTPUT);
	if (hwndCtl == NULL)
		MessageBox(hwndDlg, szMessage, g_szTitle, MB_OK | MB_ICONEXCLAMATION);
	else
	{
		OutputText(hwndCtl, szMessage);
		OutputText(hwndCtl, TEXT("\r\n"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

// my_emit_message: Message handling
// This function handles all messages (trace, debug or error printouts) generated by LIBJPEG.
// my_emit_message determines whether the message is suppressed or displayed on the screen:
//  -1: Warning
//   0: Important message (e.g. error)
// 1-3: Trace information
void my_emit_message(j_common_ptr pjInfo, int nMessageLevel)
{
	char szBuffer[JMSG_LENGTH_MAX];  // Buffer for message text

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
// WebpToDib: Decodes a WebP image into a DIB using libwebp

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
		HWND hwndCtl = NULL;
		HWND hwndDlg = GetActiveWindow();
		if (hwndDlg != NULL)
			hwndCtl = GetDlgItem(hwndDlg, IDC_OUTPUT);
		if (hwndCtl != NULL)
		{
			TCHAR szOutput[OUTPUT_LEN];
			OutputTextFmt(hwndCtl, szOutput, TEXT("Width:\t\t%d pixels\r\n"), nWidth);
			OutputTextFmt(hwndCtl, szOutput, TEXT("Height:\t\t%d pixels\r\n"), nHeight);
			OutputTextFmt(hwndCtl, szOutput, TEXT("Has Alpha:\t%s\r\n"),
				features.has_alpha ? TEXT("True") : TEXT("False"));
			OutputTextFmt(hwndCtl, szOutput, TEXT("Has Animation:\t%s\r\n"),
				features.has_animation ? TEXT("True") : TEXT("False"));
			OutputText(hwndCtl, TEXT("Format:\t\t"));
			switch (features.format)
			{
				case 0:
					OutputText(hwndCtl, TEXT("Undefined/Mixed"));
					break;
				case 1:
					OutputText(hwndCtl, TEXT("Lossy"));
					break;
				case 2:
					OutputText(hwndCtl, TEXT("Lossless"));
					break;
				default:
					OutputTextFmt(hwndCtl, szOutput, TEXT("%d"), features.format);
			}
			OutputText(hwndCtl, TEXT("\r\n"));
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
	int nIncrement = ((nWidth * nNumChannels * 8) + 31) / 32 * 4;
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
	lpbi->biSizeImage = 0;
	lpbi->biXPelsPerMeter = 0;
	lpbi->biYPelsPerMeter = 0;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;

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
// DdsToDib: Decodes a DDS image into a DIB using crunch/crnlib

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
		HWND hwndCtl = NULL;
		HWND hwndDlg = GetActiveWindow();
		if (hwndDlg != NULL)
			hwndCtl = GetDlgItem(hwndDlg, IDC_OUTPUT);
		if (hwndCtl != NULL)
		{
			BYTE achFourCC[4];
			TCHAR szOutput[OUTPUT_LEN];
			memcpy(achFourCC, &tex_desc.m_fmt_fourcc, 4);
			OutputTextFmt(hwndCtl, szOutput, TEXT("Faces:\t\t%u\r\n"), tex_desc.m_faces);
			OutputTextFmt(hwndCtl, szOutput, TEXT("Width:\t\t%u pixels\r\n"), tex_desc.m_width);
			OutputTextFmt(hwndCtl, szOutput, TEXT("Height:\t\t%u pixels\r\n"), tex_desc.m_height);
			OutputTextFmt(hwndCtl, szOutput, TEXT("Levels:\t\t%u\r\n"), tex_desc.m_levels);
			OutputTextFmt(hwndCtl, szOutput, TEXT("Format:\t\t%hc%hc%hc%hc\r\n"),
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
	lpbi->biSizeImage = 0;
	lpbi->biXPelsPerMeter = 0;
	lpbi->biYPelsPerMeter = 0;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;

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
				lpDest[uDestPtr + 2] = lpSrc[uSrcPtr++];	// B <-- R
				lpDest[uDestPtr + 1] = lpSrc[uSrcPtr++];	// G <-- G
				lpDest[uDestPtr + 0] = lpSrc[uSrcPtr++];	// R <-- B
				lpDest[uDestPtr + 3] = lpSrc[uSrcPtr++];	// A <-- A
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
// FreeDib: Frees the memory allocated for the DIB

BOOL FreeDib(HANDLE hDib)
{
	if (hDib == NULL)
		return FALSE;

	if (GlobalFree(hDib) != NULL)
		return FALSE;

	hDib = NULL;

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// CreatePremultipliedBitmap: Creates a 32-bit DIB of a 16-/32-bit DIB with premultiplied alpha

HBITMAP CreatePremultipliedBitmap(HANDLE hDib)
{
	if (hDib == NULL)
		return NULL;

	LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
	if (lpbi == NULL || lpbi->biSize < sizeof(BITMAPINFOHEADER) ||
		(lpbi->biBitCount != 16 && lpbi->biBitCount != 32) ||
		(lpbi->biCompression != BI_RGB && lpbi->biCompression != BI_BITFIELDS) ||
		(lpbi->biSize >= sizeof(BITMAPV4HEADER) &&
			((LPBITMAPV4HEADER)lpbi)->bV4CSType == LCS_DEVICE_CMYK))
		return NULL;

	LPBYTE lpDIB = (LPBYTE)lpbi + lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);
	if (lpbi->biSize == sizeof(BITMAPINFOHEADER) && lpbi->biCompression == BI_BITFIELDS)
		lpDIB += 3 * sizeof(DWORD);

	LONG lWidth = lpbi->biWidth;
	LONG lHeight = lpbi->biHeight;
	if (lHeight < 0)
		lHeight = -lHeight;

	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = lpbi->biWidth;
	bmi.bmiHeader.biHeight = lpbi->biHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0;

	LPBYTE lpBGRA = NULL;
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
	DWORD dwIncrement = ((lWidth * lpbi->biBitCount) + 31) / 32 * 4;
	BOOL bHasVisiblePixels = FALSE;

	__try
	{
		if (lpbi->biBitCount == 16)
		{
			if (lpbi->biCompression == BI_BITFIELDS)
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
				dwAlphaMask = 0x00000000;
			}

			for (h = 0; h < lHeight; h++)
			{
				lpSrc = lpDIB + h * dwIncrement;
				lpDest = lpBGRA + h * lWidth * 4;
				for (w = 0; w < lWidth; w++)
				{
					dwColor = MAKELONG(MAKEWORD(lpSrc[0], lpSrc[1]), 0);
					cAlpha = dwAlphaMask ? GetColorValue(dwColor, dwAlphaMask) : 0xFF;
					if (cAlpha != 0) bHasVisiblePixels = TRUE;
					*lpDest++ = GetColorValue(dwColor, dwBlueMask)  * cAlpha / 0xFF;
					*lpDest++ = GetColorValue(dwColor, dwGreenMask) * cAlpha / 0xFF;
					*lpDest++ = GetColorValue(dwColor, dwRedMask)   * cAlpha / 0xFF;
					*lpDest++ = cAlpha;
					lpSrc += 2;
				}
			}
		}
		else if(lpbi->biBitCount == 32)
		{
			if (lpbi->biCompression == BI_BITFIELDS)
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

			for (h = 0; h < lHeight; h++)
			{
				lpSrc = lpDIB + h * dwIncrement;
				lpDest = lpBGRA + h * lWidth * 4;
				for (w = 0; w < lWidth; w++)
				{
					dwColor = MAKELONG(MAKEWORD(lpSrc[0], lpSrc[1]), MAKEWORD(lpSrc[2], lpSrc[3]));
					cAlpha = dwAlphaMask ? GetColorValue(dwColor, dwAlphaMask) : 0xFF;
					if (cAlpha != 0) bHasVisiblePixels = TRUE;
					*lpDest++ = GetColorValue(dwColor, dwBlueMask)  * cAlpha / 0xFF;
					*lpDest++ = GetColorValue(dwColor, dwGreenMask) * cAlpha / 0xFF;
					*lpDest++ = GetColorValue(dwColor, dwRedMask)   * cAlpha / 0xFF;
					*lpDest++ = cAlpha;
					lpSrc += 4;
				}
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) { ; }

	GlobalUnlock(hDib);

	if (!bHasVisiblePixels)
	{
		FreeBitmap(hbmpDib);
		return FALSE;
	}

	return hbmpDib;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// FreeBitmap: Frees the memory allocated for the DIB section

BOOL FreeBitmap(HBITMAP hbmpDib)
{
	if (hbmpDib == NULL)
		return FALSE;

	return DeleteObject(hbmpDib);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// GetColorValue: Calculates the value of a color component

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
