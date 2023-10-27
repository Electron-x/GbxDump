////////////////////////////////////////////////////////////////////////////////////////////////
// DumpBmp.cpp - Copyright (c) 2023 by Electron.
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
#include "imgfmt.h"
#include "archive.h"
#include "dumpbmp.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module

BOOL IsDibSupported(LPCSTR lpbi);
DWORD QueryDibSupport(LPCSTR lpbi);
BOOL GetColorName(COLORREF rgbColor, LPCTSTR* lpszName);
void PrintProfileSignature(HWND hwndCtl, LPCTSTR lpszName, DWORD dwSignature, BOOL bAddCrLf = TRUE);
float Half2Float(WORD h);

////////////////////////////////////////////////////////////////////////////////////////////////
// DumpBitmap is called by DumpFile from GbxDump.cpp

BOOL DumpBitmap(HWND hwndCtl, HANDLE hFile, DWORD dwFileSize)
{
	TCHAR szOutput[OUTPUT_LEN];
	BITMAPFILEHEADER bfh;
	DWORD dwFileHeaderSize = sizeof(bfh);

	if (hwndCtl == NULL || hFile == NULL || dwFileSize < dwFileHeaderSize)
		return FALSE;

	HWND hDlg = GetParent(hwndCtl);

	// Jump to the beginning of the file
	if (!FileSeekBegin(hFile, 0))
		return FALSE;

	// Read the file header
	ZeroMemory(&bfh, sizeof(bfh));
	if (!ReadData(hFile, (LPVOID)&bfh, sizeof(bfh)))
		return FALSE;

	if (bfh.bfType == BFT_BITMAPARRAY)
	{ // OS/2 Bit-map Array
		LPBITMAPARRAYFILEHEADER lpbafh = (LPBITMAPARRAYFILEHEADER)&bfh;

		OutputText(hwndCtl, g_szSep1);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Type:\t\t%hc%hc\r\n"),
			LOBYTE(lpbafh->usType), HIBYTE(lpbafh->usType));
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Size:\t\t%u bytes\r\n"), lpbafh->cbSize);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("OffNext:\t%u bytes\r\n"), lpbafh->offNext);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("CxDisplay:\t%u\r\n"), lpbafh->cxDisplay);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("CyDisplay:\t%u\r\n"), lpbafh->cyDisplay);

		// Proceed only if the array contains only one bitmap
		if (lpbafh->offNext != 0)
		{
			MarkAsUnsupported(hDlg);
			return TRUE;
		}

		dwFileHeaderSize += dwFileHeaderSize;
		if (dwFileSize < dwFileHeaderSize)
			return FALSE;

		// Read the file header of the first bitmap
		ZeroMemory(&bfh, sizeof(bfh));
		if (!ReadData(hFile, (LPVOID)&bfh, sizeof(bfh)))
			return FALSE;

		if (bfh.bfType != BFT_BMAP)
		{ // No support for icons and pointers
			MarkAsUnsupported(hDlg);
			return TRUE;
		}
	}

	// Output the file header
	OutputText(hwndCtl, g_szSep1);
	OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Type:\t\t%hc%hc\r\n"),
		LOBYTE(bfh.bfType), HIBYTE(bfh.bfType));
	OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Size:\t\t%u bytes\r\n"), bfh.bfSize);
	OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Reserved1:\t%u\r\n"), bfh.bfReserved1);
	OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Reserved2:\t%u\r\n"), bfh.bfReserved2);
	OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("OffBits:\t%u bytes\r\n"), bfh.bfOffBits);

	DWORD dwDibSize = dwFileSize - dwFileHeaderSize;
	if (dwDibSize < sizeof(BITMAPCOREHEADER))
		return FALSE;

	HANDLE hDib = GlobalAlloc(GHND, dwDibSize);
	if (hDib == NULL)
		return FALSE;

	LPSTR lpbi = (LPSTR)GlobalLock(hDib);
	if (lpbi == NULL)
	{
		GlobalFree(hDib);
		return FALSE;
	}

	// Read the entire DIB. This is somewhat easier than allocating new memory for each component
	// of the DIB. Instead, before reading a new section, we check to see if there is enough data
	// available for it. We could predict this at this point, but with a truncated file, we still
	// want as much data as possible displayed. We get a packed DIB by modifying the color table.
	if (!ReadData(hFile, lpbi, dwDibSize))
	{
		GlobalUnlock(hDib);
		GlobalFree(hDib);
		return FALSE;
	}

	DWORD dwDibHeaderSize = *(LPDWORD)lpbi;
	LPBITMAPV5HEADER lpbih = (LPBITMAPV5HEADER)lpbi;

	OutputText(hwndCtl, g_szSep1);
	OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Size:\t\t%u bytes\r\n"), dwDibHeaderSize);

	if (dwDibHeaderSize > dwDibSize)
	{
		GlobalUnlock(hDib);
		GlobalFree(hDib);
		return FALSE;
	}

	if (dwDibHeaderSize != sizeof(BITMAPCOREHEADER) &&
		dwDibHeaderSize != sizeof(BITMAPINFOHEADER) &&
		dwDibHeaderSize != sizeof(BITMAPV2INFOHEADER) &&
		dwDibHeaderSize != sizeof(BITMAPV3INFOHEADER) &&
		dwDibHeaderSize != sizeof(BITMAPINFOHEADER2) &&
		dwDibHeaderSize != sizeof(BITMAPV4HEADER) &&
		dwDibHeaderSize != sizeof(BITMAPV5HEADER))
	{
		// Can be a EXBMINFOHEADER DIB with biSize > 40
		// or a BITMAPINFOHEADER2 DIB with cbFix < 64
		GlobalUnlock(hDib);
		GlobalFree(hDib);
		MarkAsUnsupported(hDlg);
		return TRUE;
	}

	// Since we don't perform format conversions here, all formats
	// that the GDI cannot directly display are marked as unsupported
	BOOL bIsDibSupported = TRUE;

	if (dwDibHeaderSize == sizeof(BITMAPCOREHEADER))
	{ // OS/2 Version 1.1 Bit-map (DIBv2)
		LPBITMAPCOREHEADER lpbch = (LPBITMAPCOREHEADER)lpbi;

		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Width:\t\t%u pixels\r\n"), lpbch->bcWidth);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Height:\t\t%u pixels\r\n"), lpbch->bcHeight);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Planes:\t\t%u\r\n"), lpbch->bcPlanes);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("BitCount:\t%u bpp\r\n"), lpbch->bcBitCount);
		
		// Perform some sanity checks
		UINT64 ullBitsSize = WIDTHBYTES((UINT64)lpbch->bcWidth * lpbch->bcPlanes * lpbch->bcBitCount) * lpbch->bcHeight;
		if (ullBitsSize == 0 || ullBitsSize > 0x80000000 || lpbch->bcBitCount > 32)
			bIsDibSupported = FALSE;
	}

	if (dwDibHeaderSize >= sizeof(BITMAPINFOHEADER))
	{ // Windows Version 3.0 Bitmap (DIBv3)
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Width:\t\t%d pixels\r\n"), lpbih->bV5Width);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Height:\t\t%d pixels\r\n"), lpbih->bV5Height);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Planes:\t\t%u\r\n"), lpbih->bV5Planes);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("BitCount:\t%u bpp\r\n"), lpbih->bV5BitCount);

		// Use the QUERYDIBSUPPORT escape function to determine whether GDI
		// can display this DIB. The Escape doesn't support OS/2-style DIBs.
		bIsDibSupported = IsDibSupported(lpbi);
		// Perform some additional sanity checks
		INT64 llBitsSize = WIDTHBYTES((INT64)lpbih->bV5Width * lpbih->bV5Planes * lpbih->bV5BitCount) * abs(lpbih->bV5Height);
		if (llBitsSize <= 0 || llBitsSize > 0x80000000L)
			bIsDibSupported = FALSE;

		//lpbih->bV5Compression &= ~BI_SRCPREROTATE;
		DWORD dwCompression = lpbih->bV5Compression;
		OutputText(hwndCtl, TEXT("Compression:\t"));
		if (isprint(dwCompression & 0xff) &&
			isprint((dwCompression >> 8) & 0xff) &&
			isprint((dwCompression >> 16) & 0xff) &&
			isprint((dwCompression >> 24) & 0xff))
		{ // biCompression contains a FourCC code
			// Not supported by GDI, but may be rendered by VfW DrawDibDraw
			bIsDibSupported = TRUE;
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("%hc%hc%hc%hc"),
				(char)(dwCompression & 0xff),
				(char)((dwCompression >> 8) & 0xff),
				(char)((dwCompression >> 16) & 0xff),
				(char)((dwCompression >> 24) & 0xff));
		}
		else if (dwDibHeaderSize == sizeof(BITMAPINFOHEADER2))
		{ // OS/2 2.0 bitmap
			switch (dwCompression)
			{
				case BCA_UNCOMP:
					OutputText(hwndCtl, TEXT("UNCOMP"));
					break;
				case BCA_RLE8:
					OutputText(hwndCtl, TEXT("RLE8"));
					break;
				case BCA_RLE4:
					OutputText(hwndCtl, TEXT("RLE4"));
					break;
				case BCA_HUFFMAN1D:
					OutputText(hwndCtl, TEXT("HUFFMAN1D"));
					break;
				case BCA_RLE24:
					OutputText(hwndCtl, TEXT("RLE24"));
					break;
				default:
					OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("%u"), dwCompression);
			}
		}
		else
		{
			switch (dwCompression)
			{
				case BI_RGB:
					OutputText(hwndCtl, TEXT("RGB"));
					break;
				case BI_RLE8:
					OutputText(hwndCtl, TEXT("RLE8"));
					break;
				case BI_RLE4:
					OutputText(hwndCtl, TEXT("RLE4"));
					break;
				case BI_BITFIELDS:
					OutputText(hwndCtl, TEXT("BITFIELDS"));
					break;
				case BI_JPEG:
					OutputText(hwndCtl, TEXT("JPEG"));
					break;
				case BI_PNG:
					OutputText(hwndCtl, TEXT("PNG"));
					break;
				case BI_ALPHABITFIELDS:
					OutputText(hwndCtl, TEXT("ALPHABITFIELDS"));
					break;
				case BI_FOURCC:
					OutputText(hwndCtl, TEXT("FOURCC"));
					break;
				case BI_CMYK:
					OutputText(hwndCtl, TEXT("CMYK"));
					break;
				case BI_CMYKRLE8:
					OutputText(hwndCtl, TEXT("CMYKRLE8"));
					break;
				case BI_CMYKRLE4:
					OutputText(hwndCtl, TEXT("CMYKRLE4"));
					break;
				default:
					OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("%u"), dwCompression);
			}
		}
		OutputText(hwndCtl, TEXT("\r\n"));

		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("SizeImage:\t%u bytes\r\n"), lpbih->bV5SizeImage);

		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("XPelsPerMeter:\t%d"), lpbih->bV5XPelsPerMeter);
		if (lpbih->bV5XPelsPerMeter >= 20)
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT(" (%d dpi)"),
				MulDiv(lpbih->bV5XPelsPerMeter, 127, 5000));
		OutputText(hwndCtl, TEXT("\r\n"));
			
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("YPelsPerMeter:\t%d"), lpbih->bV5YPelsPerMeter);
		if (lpbih->bV5YPelsPerMeter >= 20)
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT(" (%d dpi)"),
				MulDiv(lpbih->bV5YPelsPerMeter, 127, 5000));
		OutputText(hwndCtl, TEXT("\r\n"));
			
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("ClrUsed:\t%u\r\n"), lpbih->bV5ClrUsed);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("ClrImportant:\t%u\r\n"), lpbih->bV5ClrImportant);
	}

	// The size of the OS/2 2.0 header can be reduced from its full size of
	// 64 bytes down to 16 bytes. Omitted members are assumed to have a value
	// of zero. However, we only support bitmaps with full header here.
	if (dwDibHeaderSize == sizeof(BITMAPINFOHEADER2))
	{ // OS/2 Version 2.0 Bit-map
		LPBITMAPINFOHEADER2 lpbih2 = (LPBITMAPINFOHEADER2)lpbi;

		// Extensions to BITMAPINFOHEADER
		OutputText(hwndCtl, TEXT("Units:\t\t"));
		if (lpbih2->usUnits == BRU_METRIC)
			OutputText(hwndCtl, TEXT("METRIC"));
		else
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("%u"), lpbih2->usUnits);
		OutputText(hwndCtl, TEXT("\r\n"));

		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Reserved:\t%u\r\n"), lpbih2->usReserved);

		OutputText(hwndCtl, TEXT("Recording:\t"));
		if (lpbih2->usRecording == BRA_BOTTOMUP)
			OutputText(hwndCtl, TEXT("BOTTOMUP"));
		else
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("%u"), lpbih2->usRecording);
		OutputText(hwndCtl, TEXT("\r\n"));

		USHORT usRendering = lpbih2->usRendering;
		OutputText(hwndCtl, TEXT("Rendering:\t"));
		switch (usRendering)
		{
			case BRH_NOTHALFTONED:
				OutputText(hwndCtl, TEXT("NOTHALFTONED"));
				break;
			case BRH_ERRORDIFFUSION:
				OutputText(hwndCtl, TEXT("ERRORDIFFUSION"));
				break;
			case BRH_PANDA:
				OutputText(hwndCtl, TEXT("PANDA"));
				break;
			case BRH_SUPERCIRCLE:
				OutputText(hwndCtl, TEXT("SUPERCIRCLE"));
				break;
			default:
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("%u"), usRendering);
		}
		OutputText(hwndCtl, TEXT("\r\n"));

		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Size1:\t\t%u"), lpbih2->cSize1);
		if (usRendering == BRH_ERRORDIFFUSION && lpbih2->cSize1 <= 100)
			OutputText(hwndCtl, TEXT("%"));
		else if (usRendering == BRH_PANDA || usRendering == BRH_SUPERCIRCLE)
			OutputText(hwndCtl, TEXT(" pels"));
		OutputText(hwndCtl, TEXT("\r\n"));

		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Size2:\t\t%u"), lpbih2->cSize2);
		if (usRendering == BRH_PANDA || usRendering == BRH_SUPERCIRCLE)
			OutputText(hwndCtl, TEXT(" pels"));
		OutputText(hwndCtl, TEXT("\r\n"));

		OutputText(hwndCtl, TEXT("ColorEncoding:\t"));
		if (lpbih2->ulColorEncoding == BCE_RGB)
			OutputText(hwndCtl, TEXT("RGB"));
		else if (lpbih2->ulColorEncoding == BCE_PALETTE)
			OutputText(hwndCtl, TEXT("PALETTE"));
		else
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("%u"), lpbih2->ulColorEncoding);
		OutputText(hwndCtl, TEXT("\r\n"));

		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Identifier:\t%u\r\n"), lpbih2->ulIdentifier);

		// GDI doesn't support OS/2 2.0 bitmaps
		GlobalUnlock(hDib);
		GlobalFree(hDib);
		MarkAsUnsupported(hDlg);
		return TRUE;
	}

	if (dwDibHeaderSize == sizeof(BITMAPINFOHEADER))
	{ // Windows Version 3.0 Bitmap with Windows NT extension
		if (lpbih->bV5Compression == BI_BITFIELDS || lpbih->bV5Compression == BI_ALPHABITFIELDS)
		{
			if ((dwDibHeaderSize + ColorMasksSize(lpbi)) > dwDibSize)
			{
				GlobalUnlock(hDib);
				GlobalFree(hDib);
				return FALSE;
			}

			LPDWORD lpdwMasks = (LPDWORD)(lpbi + sizeof(BITMAPINFOHEADER));

			OutputText(hwndCtl, g_szSep1);
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("RedMask:\t%08X\r\n"), lpdwMasks[0]);
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("GreenMask:\t%08X\r\n"), lpdwMasks[1]);
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("BlueMask:\t%08X\r\n"), lpdwMasks[2]);

			// Windows CE extension
			if (lpbih->bV5Compression == BI_ALPHABITFIELDS)
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("AlphaMask:\t%08X\r\n"), lpdwMasks[3]);
		}
	}

	if (dwDibHeaderSize >= sizeof(BITMAPV2INFOHEADER))
	{ // Adobe Photoshop extension
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("RedMask:\t%08X\r\n"), lpbih->bV5RedMask);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("GreenMask:\t%08X\r\n"), lpbih->bV5GreenMask);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("BlueMask:\t%08X\r\n"), lpbih->bV5BlueMask);
	}

	if (dwDibHeaderSize >= sizeof(BITMAPV3INFOHEADER))
	{ // Adobe Photoshop extension
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("AlphaMask:\t%08X\r\n"), lpbih->bV5AlphaMask);
	}

	if (dwDibHeaderSize >= sizeof(BITMAPV4HEADER))
	{ // Win32 Version 4.0 Bitmap (DIBv4, ICM 1.0)
		DWORD dwCSType = lpbih->bV5CSType;
		OutputText(hwndCtl, TEXT("CSType:\t\t"));
		if (isprint(dwCSType & 0xff) &&
			isprint((dwCSType >>  8) & 0xff) &&
			isprint((dwCSType >> 16) & 0xff) &&
			isprint((dwCSType >> 24) & 0xff))
		{
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("%hc%hc%hc%hc"),
				(char)((dwCSType >> 24) & 0xff),
				(char)((dwCSType >> 16) & 0xff),
				(char)((dwCSType >>  8) & 0xff),
				(char)(dwCSType & 0xff));
		}
		else
		{
			switch (dwCSType)
			{
				case LCS_CALIBRATED_RGB:
					OutputText(hwndCtl, TEXT("CALIBRATED_RGB"));
					break;
				case LCS_DEVICE_RGB:
					OutputText(hwndCtl, TEXT("DEVICE_RGB"));
					break;
				case LCS_DEVICE_CMYK:
					OutputText(hwndCtl, TEXT("DEVICE_CMYK"));
					break;
				default:
					OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("%u"), dwCSType);
			}
		}
		OutputText(hwndCtl, TEXT("\r\n"));

		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput),
			TEXT("CIEXYZRed:\t%.4f, %.4f, %.4f\r\n"),
			(double)lpbih->bV5Endpoints.ciexyzRed.ciexyzX / 0x40000000,
			(double)lpbih->bV5Endpoints.ciexyzRed.ciexyzY / 0x40000000,
			(double)lpbih->bV5Endpoints.ciexyzRed.ciexyzZ / 0x40000000);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput),
			TEXT("CIEXYZGreen:\t%.4f, %.4f, %.4f\r\n"),
			(double)lpbih->bV5Endpoints.ciexyzGreen.ciexyzX / 0x40000000,
			(double)lpbih->bV5Endpoints.ciexyzGreen.ciexyzY / 0x40000000,
			(double)lpbih->bV5Endpoints.ciexyzGreen.ciexyzZ / 0x40000000);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput),
			TEXT("CIEXYZBlue:\t%.4f, %.4f, %.4f\r\n"),
			(double)lpbih->bV5Endpoints.ciexyzBlue.ciexyzX / 0x40000000,
			(double)lpbih->bV5Endpoints.ciexyzBlue.ciexyzY / 0x40000000,
			(double)lpbih->bV5Endpoints.ciexyzBlue.ciexyzZ / 0x40000000);

		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("GammaRed:\t%g\r\n"),
			(double)lpbih->bV5GammaRed / 0x10000);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("GammaGreen:\t%g\r\n"),
			(double)lpbih->bV5GammaGreen / 0x10000);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("GammaBlue:\t%g\r\n"),
			(double)lpbih->bV5GammaBlue / 0x10000);
	}

	if (dwDibHeaderSize >= sizeof(BITMAPV5HEADER))
	{ // Win32 Version 5.0 Bitmap (DIBv5, ICM 2.0)
		DWORD dwIntent = lpbih->bV5Intent;
		OutputText(hwndCtl, TEXT("Intent:\t\t"));
		switch (dwIntent)
		{
			case LCS_GM_BUSINESS:
				OutputText(hwndCtl, TEXT("GM_BUSINESS (Saturation)"));
				break;
			case LCS_GM_GRAPHICS:
				OutputText(hwndCtl, TEXT("GM_GRAPHICS (Relative Colorimetric)"));
				break;
			case LCS_GM_IMAGES:
				OutputText(hwndCtl, TEXT("GM_IMAGES (Perceptual)"));
				break;
			case LCS_GM_ABS_COLORIMETRIC:
				OutputText(hwndCtl, TEXT("GM_ABS_COLORIMETRIC (Absolute Colorimetric)"));
				break;
			default:
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("%u"), dwIntent);
		}
		OutputText(hwndCtl, TEXT("\r\n"));

		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("ProfileData:\t%u bytes\r\n"), lpbih->bV5ProfileData);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("ProfileSize:\t%u bytes\r\n"), lpbih->bV5ProfileSize);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Reserved:\t%u\r\n"), lpbih->bV5Reserved);
	}

	// Output the color table entries
	UINT uNumColors = min(DibNumColors(lpbi), 4096);
	if (uNumColors > 0)
	{
		if (DibBitsOffset(lpbi) > dwDibSize)
		{
			GlobalUnlock(hDib);
			GlobalFree(hDib);
			return FALSE;
		}

		OutputText(hwndCtl, g_szSep1);
		LPCTSTR lpszColorName = TEXT("");
		int nWidthDec = uNumColors > 1000 ? 4 : (uNumColors > 100 ? 3 : (uNumColors > 10 ? 2 : 1));
		int nWidthHex = uNumColors > 0x100 ? 3 : (uNumColors > 0x10 ? 2 : 1);

		if (IS_OS2PM_DIB(lpbi))
		{
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput),
				TEXT("%*c|   B   G   R |%-*c| B  G  R  |\r\n"), nWidthDec, 'I', nWidthHex, 'I');

			LPBITMAPCOREINFO lpbmc = (LPBITMAPCOREINFO)lpbi;
			for (UINT i = 0; i < uNumColors; i++)
			{
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput),
					TEXT("%*u| %3u %3u %3u |%0*X| %02X %02X %02X |"),
					nWidthDec, i,
					lpbmc->bmciColors[i].rgbtBlue,
					lpbmc->bmciColors[i].rgbtGreen,
					lpbmc->bmciColors[i].rgbtRed,
					nWidthHex, i,
					lpbmc->bmciColors[i].rgbtBlue,
					lpbmc->bmciColors[i].rgbtGreen,
					lpbmc->bmciColors[i].rgbtRed);

				if (GetColorName(RGB(
					lpbmc->bmciColors[i].rgbtRed,
					lpbmc->bmciColors[i].rgbtGreen,
					lpbmc->bmciColors[i].rgbtBlue),
					&lpszColorName))
					OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT(" %s"), lpszColorName);

				OutputText(hwndCtl, TEXT("\r\n"));
			}
		}
		else
		{
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput),
				TEXT("%*c|   B   G   R   X |%-*c| B  G  R  X  |\r\n"), nWidthDec, 'I', nWidthHex, 'I');

			LPRGBQUAD lprgbqColors = (LPRGBQUAD)FindDibPalette(lpbi);
			for (UINT i = 0; i < uNumColors; i++)
			{
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput),
					TEXT("%*u| %3u %3u %3u %3u |%0*X| %02X %02X %02X %02X |"),
					nWidthDec, i,
					lprgbqColors[i].rgbBlue,
					lprgbqColors[i].rgbGreen,
					lprgbqColors[i].rgbRed,
					lprgbqColors[i].rgbReserved,
					nWidthHex, i,
					lprgbqColors[i].rgbBlue,
					lprgbqColors[i].rgbGreen,
					lprgbqColors[i].rgbRed,
					lprgbqColors[i].rgbReserved,
					lpszColorName);

				if (GetColorName(RGB(
					lprgbqColors[i].rgbRed,
					lprgbqColors[i].rgbGreen,
					lprgbqColors[i].rgbBlue),
					&lpszColorName))
					OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT(" %s"), lpszColorName);

				OutputText(hwndCtl, TEXT("\r\n"));
			}
		}
	}

	// Check for a gap between color table and bitmap bits
	DWORD dwOffBits = dwFileHeaderSize + dwDibHeaderSize + ColorMasksSize(lpbi) + PaletteSize(lpbi);

	if (((bfh.bfOffBits != 0 ? bfh.bfOffBits : dwOffBits) - dwFileHeaderSize) > dwDibSize)
	{
		GlobalUnlock(hDib);
		GlobalFree(hDib);
		return FALSE;
	}

	if (bfh.bfOffBits > dwOffBits)
	{
		DWORD dwGap = bfh.bfOffBits - dwOffBits;

		// Declare the gap as (additional) color table entries to get a packed DIB
		if (dwDibHeaderSize >= sizeof(BITMAPINFOHEADER))
			lpbih->bV5ClrUsed += dwGap / sizeof(RGBQUAD);

		OutputText(hwndCtl, g_szSep1);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Gap to pixels:\t%u bytes\r\n"), dwGap);
	}
	else if (bfh.bfOffBits != 0 && dwOffBits > bfh.bfOffBits)
	{ // Color table overlaps bitmap bits
		DWORD dwOverlap = dwOffBits - bfh.bfOffBits;

		OutputText(hwndCtl, g_szSep1);
		OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Gap to pixels:\t-%u bytes\r\n"), dwOverlap);

		DWORD dwNumEntries = DibNumColors(lpbi);
		if (dwNumEntries > 0)
		{
			if (dwDibHeaderSize == sizeof(BITMAPCOREHEADER))
			{ // Add the missing color table entries to the DIB
				dwDibSize += dwOverlap;

				GlobalUnlock(hDib);
				HANDLE hTemp = GlobalReAlloc(hDib, dwDibSize, GHND);
				if (hTemp == NULL)
				{
					GlobalFree(hDib);
					return FALSE;
				}

				hDib = hTemp;
				lpbi = (LPSTR)GlobalLock(hDib);
				lpbih = (LPBITMAPV5HEADER)lpbi;
				if (lpbi == NULL)
				{
					GlobalFree(hDib);
					return FALSE;
				}

				LPSTR lpOld = lpbi + bfh.bfOffBits - dwFileHeaderSize;
				LPSTR lpNew = lpbi + dwOffBits - dwFileHeaderSize;

				__try
				{
					MoveMemory(lpNew, lpOld, dwFileHeaderSize + dwDibSize - dwOffBits);
					ZeroMemory(lpOld, dwOverlap);
					bfh.bfOffBits = dwOffBits;
				}
				__except (EXCEPTION_EXECUTE_HANDLER) { ; }
			}
			else
			{ // Adjust the number of color table entries
				DWORD dwAddEntries = dwOverlap / sizeof(RGBQUAD);

				if (dwNumEntries > dwAddEntries)
					lpbih->bV5ClrUsed = dwNumEntries - dwAddEntries;
			}
		}
	}

	// Output the profile data
	if (DibHasColorProfile(lpbi))
	{
		if ((lpbih->bV5ProfileData + lpbih->bV5ProfileSize) > dwDibSize)
		{
			GlobalUnlock(hDib);
			GlobalFree(hDib);
			return FALSE;
		}

		// Check for a gap between bitmap bits and profile data
		DWORD dwProfileData = DibBitsOffset(lpbi) + DibImageSize(lpbi);
		if (lpbih->bV5ProfileData > dwProfileData)
		{
			OutputText(hwndCtl, g_szSep1);
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Gap to profile:\t%u bytes\r\n"),
				lpbih->bV5ProfileData - dwProfileData);
		}

		if (lpbih->bV5CSType == PROFILE_LINKED)
		{
			char szPath[MAX_PATH];
			int nLen = min(min(lpbih->bV5ProfileSize + 1, dwDibSize - lpbih->bV5ProfileData + 1), _countof(szPath));

			// Output the file name of the ICC profile
			ZeroMemory(szPath, sizeof(szPath));
			if (MyStrNCpyA(szPath, lpbi + lpbih->bV5ProfileData, nLen) != NULL)
			{
				MultiByteToWideChar(CP_ACP, 0, szPath, -1, szOutput, _countof(szOutput) - 1);
				
				OutputText(hwndCtl, g_szSep1);
				OutputText(hwndCtl, szOutput);
				OutputText(hwndCtl, TEXT("\r\n"));
			}
		}
		else if (lpbih->bV5CSType == PROFILE_EMBEDDED)
		{
			if (lpbih->bV5ProfileSize < (sizeof(PROFILEHEADER) + sizeof(DWORD)))
			{
				GlobalUnlock(hDib);
				GlobalFree(hDib);
				return FALSE;
			}

			// Output the ICC profile header
			LPPROFILEHEADER lpph = (LPPROFILEHEADER)(lpbi + lpbih->bV5ProfileData);
			DWORD dwVersion = _byteswap_ulong(lpph->phVersion);

			if (_byteswap_ulong(lpph->phSignature) != 'acsp' && dwVersion != 0x00000100)
				goto Exit;

			OutputText(hwndCtl, g_szSep1);
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("ProfileSize:\t%u bytes\r\n"),
				_byteswap_ulong(lpph->phSize));

			PrintProfileSignature(hwndCtl, TEXT("CMMType:\t"), lpph->phCMMType);

			if (dwVersion == 0x00000100)
			{ // Apple ColorSync 1.0 profile
				OutputText(hwndCtl, TEXT("Version:\t1.0\r\n"));
				goto Exit; // No more identical header members
			}

			WORD wProfileVersion  = HIWORD(dwVersion);
			WORD wSubClassVersion = LOWORD(dwVersion);
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Version:\t%u.%u.%u\r\n"),
				HIBYTE(wProfileVersion), (LOBYTE(wProfileVersion) >> 4) & 0xF, LOBYTE(wProfileVersion) & 0xF);
			if (wProfileVersion >= 0x0500 && lpph->phSubClass != 0)
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("SubVersion:\t%u.%u\r\n"),
					HIBYTE(wSubClassVersion), LOBYTE(wSubClassVersion));

			PrintProfileSignature(hwndCtl, TEXT("Class:\t\t"), lpph->phClass);
			PrintProfileSignature(hwndCtl, TEXT("ColorSpace:\t"), lpph->phDataColorSpace);
			PrintProfileSignature(hwndCtl, TEXT("PCS:\t\t"), lpph->phConnectionSpace);

			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput),
				TEXT("DateTime:\t%02u-%02u-%02u %02u:%02u:%02u\r\n"),
				_byteswap_ushort(LOWORD(lpph->phDateTime[0])),
				_byteswap_ushort(HIWORD(lpph->phDateTime[0])),
				_byteswap_ushort(LOWORD(lpph->phDateTime[1])),
				_byteswap_ushort(HIWORD(lpph->phDateTime[1])),
				_byteswap_ushort(LOWORD(lpph->phDateTime[2])),
				_byteswap_ushort(HIWORD(lpph->phDateTime[2])));

			PrintProfileSignature(hwndCtl, TEXT("Signature:\t"), lpph->phSignature);
			PrintProfileSignature(hwndCtl, TEXT("Platform:\t"), lpph->phPlatform);

			DWORD dwProfileFlags = _byteswap_ulong(lpph->phProfileFlags);
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("ProfileFlags:\t%08X"), dwProfileFlags);
			if (dwProfileFlags != 0)
			{
				szOutput[0] = TEXT('\0');
				DWORD dwFlags = dwProfileFlags;
				if (dwProfileFlags & FLAG_EMBEDDEDPROFILE)
				{
					dwFlags ^= FLAG_EMBEDDEDPROFILE;
					AppendFlagName(szOutput, _countof(szOutput), TEXT("EMBEDDEDPROFILE"));
				}
				if (dwProfileFlags & FLAG_DEPENDENTONDATA)
				{
					dwFlags ^= FLAG_DEPENDENTONDATA;
					AppendFlagName(szOutput, _countof(szOutput), TEXT("DEPENDENTONDATA"));
				}
				if (dwProfileFlags & FLAG_MCSNEEDSSUBSET)
				{
					dwFlags ^= FLAG_MCSNEEDSSUBSET;
					AppendFlagName(szOutput, _countof(szOutput), TEXT("MCSNEEDSSUBSET"));
				}
				if (dwFlags != 0 && dwFlags != dwProfileFlags)
				{
					const int FLAGS_LEN = 32;
					TCHAR szFlags[FLAGS_LEN];
					_sntprintf(szFlags, _countof(szFlags), TEXT("%08X"), dwFlags);
					szFlags[FLAGS_LEN - 1] = TEXT('\0');
					AppendFlagName(szOutput, _countof(szOutput), szFlags);
				}
				if (szOutput[0] != TEXT('\0'))
				{
					OutputText(hwndCtl, TEXT(" ("));
					OutputText(hwndCtl, szOutput);
					OutputText(hwndCtl, TEXT(")"));
				}
			}
			OutputText(hwndCtl, TEXT("\r\n"));

			PrintProfileSignature(hwndCtl, TEXT("Manufacturer:\t"), lpph->phManufacturer);
			PrintProfileSignature(hwndCtl, TEXT("Model:\t\t"), lpph->phModel);

			UINT64 ullAttributes = _byteswap_ulong(lpph->phAttributes[0]);
			ullAttributes |= (UINT64)_byteswap_ulong(lpph->phAttributes[1]) << 32;
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("Attributes:\t%016llX"), ullAttributes);
			if (ullAttributes != 0)
			{
				szOutput[0] = TEXT('\0');
				UINT64 ullFlags = ullAttributes;
				if (ullAttributes & ATTRIB_TRANSPARENCY)
				{
					ullFlags ^= ATTRIB_TRANSPARENCY;
					AppendFlagName(szOutput, _countof(szOutput), TEXT("TRANSPARENCY"));
				}
				if (ullAttributes & ATTRIB_MATTE)
				{
					ullFlags ^= ATTRIB_MATTE;
					AppendFlagName(szOutput, _countof(szOutput), TEXT("MATTE"));
				}
				if (ullAttributes & ATTRIB_MEDIANEGATIVE)
				{
					ullFlags ^= ATTRIB_MEDIANEGATIVE;
					AppendFlagName(szOutput, _countof(szOutput), TEXT("MEDIANEGATIVE"));
				}
				if (ullAttributes & ATTRIB_MEDIABLACKANDWHITE)
				{
					ullFlags ^= ATTRIB_MEDIABLACKANDWHITE;
					AppendFlagName(szOutput, _countof(szOutput), TEXT("MEDIABLACKANDWHITE"));
				}
				if (ullAttributes & ATTRIB_NONPAPERBASED)
				{
					ullFlags ^= ATTRIB_NONPAPERBASED;
					AppendFlagName(szOutput, _countof(szOutput), TEXT("NONPAPERBASED"));
				}
				if (ullAttributes & ATTRIB_TEXTURED)
				{
					ullFlags ^= ATTRIB_TEXTURED;
					AppendFlagName(szOutput, _countof(szOutput), TEXT("TEXTURED"));
				}
				if (ullAttributes & ATTRIB_NONISOTROPIC)
				{
					ullFlags ^= ATTRIB_NONISOTROPIC;
					AppendFlagName(szOutput, _countof(szOutput), TEXT("NONISOTROPIC"));
				}
				if (ullAttributes & ATTRIB_SELFLUMINOUS)
				{
					ullFlags ^= ATTRIB_SELFLUMINOUS;
					AppendFlagName(szOutput, _countof(szOutput), TEXT("SELFLUMINOUS"));
				}
				if (ullFlags != 0 && ullFlags != ullAttributes)
				{
					const int FLAGS_LEN = 32;
					TCHAR szFlags[FLAGS_LEN];
					_sntprintf(szFlags, _countof(szFlags), TEXT("%016llX"), ullFlags);
					szFlags[FLAGS_LEN - 1] = TEXT('\0');
					AppendFlagName(szOutput, _countof(szOutput), szFlags);
				}
				if (szOutput[0] != TEXT('\0'))
				{
					OutputText(hwndCtl, TEXT(" ("));
					OutputText(hwndCtl, szOutput);
					OutputText(hwndCtl, TEXT(")"));
				}
			}
			OutputText(hwndCtl, TEXT("\r\n"));

			DWORD dwhRenderingIntent = _byteswap_ulong(lpph->phRenderingIntent);
			OutputText(hwndCtl, TEXT("Intent:\t\t"));
			switch (dwhRenderingIntent)
			{
				case INTENT_PERCEPTUAL:
					OutputText(hwndCtl, TEXT("PERCEPTUAL"));
					break;
				case INTENT_RELATIVE_COLORIMETRIC:
					OutputText(hwndCtl, TEXT("RELATIVE_COLORIMETRIC"));
					break;
				case INTENT_SATURATION:
					OutputText(hwndCtl, TEXT("SATURATION"));
					break;
				case INTENT_ABSOLUTE_COLORIMETRIC:
					OutputText(hwndCtl, TEXT("ABSOLUTE_COLORIMETRIC"));
					break;
				default:
					OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("%u"), dwhRenderingIntent);
			}
			OutputText(hwndCtl, TEXT("\r\n"));

			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput),
				TEXT("Illuminant:\t%.4f, %.4f, %.4f\r\n"),
				(double)(LONG32)_byteswap_ulong(lpph->phIlluminant.ciexyzX) / 0x10000,
				(double)(LONG32)_byteswap_ulong(lpph->phIlluminant.ciexyzY) / 0x10000,
				(double)(LONG32)_byteswap_ulong(lpph->phIlluminant.ciexyzZ) / 0x10000);

			PrintProfileSignature(hwndCtl, TEXT("Creator:\t"), lpph->phCreator);

			if (wProfileVersion >= 0x0400)
			{
				ZeroMemory(szOutput, sizeof(szOutput));
				for (SIZE_T i = 0; i < sizeof(lpph->phProfileID); i++)
					_sntprintf(szOutput, _countof(szOutput), TEXT("%s%02X"), (LPTSTR)szOutput, lpph->phProfileID[i]);

				OutputText(hwndCtl, TEXT("ProfileID:\t"));
				OutputText(hwndCtl, szOutput);
				OutputText(hwndCtl, TEXT("\r\n"));
			}

			if (wProfileVersion >= 0x0500)
			{
				PrintProfileSignature(hwndCtl, TEXT("SpectralPCS:\t"), lpph->phSpectralPCS);

				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput),
					TEXT("SpectralRange:\t%g nm - %g nm, %u steps\r\n"),
					Half2Float(_byteswap_ushort(lpph->phSpectralRange[0])),
					Half2Float(_byteswap_ushort(lpph->phSpectralRange[1])),
					_byteswap_ushort(lpph->phSpectralRange[2]));

				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput),
					TEXT("BiSpectrRange:\t%g nm - %g nm, %u steps\r\n"),
					Half2Float(_byteswap_ushort(lpph->phBiSpectralRange[0])),
					Half2Float(_byteswap_ushort(lpph->phBiSpectralRange[1])),
					_byteswap_ushort(lpph->phBiSpectralRange[2]));

				PrintProfileSignature(hwndCtl, TEXT("MCS:\t\t"), lpph->phMCS);
				PrintProfileSignature(hwndCtl, TEXT("SubClass:\t"), lpph->phSubClass);
			}

			// Output the tag table
			LPDWORD lpdwTagTable = (LPDWORD)(lpbi + lpbih->bV5ProfileData + sizeof(PROFILEHEADER));
			DWORD dwTagCount = _byteswap_ulong(*lpdwTagTable);

			OutputText(hwndCtl, g_szSep1);
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT("TagCount:\t%u\r\n"), dwTagCount);

			if (dwTagCount == 0)
				goto Exit;

			if (lpbih->bV5ProfileSize < (sizeof(PROFILEHEADER) + sizeof(DWORD) + 3 * sizeof(DWORD) * dwTagCount))
			{
				GlobalUnlock(hDib);
				GlobalFree(hDib);
				return FALSE;
			}

			// Note: The private tag 'ICC5' indicates that an iccMAX profile (tag type signature: 'ICCp') is
			// embedded in an ICC v2 or v4 profile, and the private tag 'MS00' indicates that WCS profile data
			// (type signature: 'MS10') is embedded in an (output) ICC profile

			OutputText(hwndCtl, g_szSep1);
			OutputText(hwndCtl, TEXT("Sig. | Element Offset | Element Size |\r\n"));

			LPDWORD lpdwTags = lpdwTagTable + 1;
			while (dwTagCount--)
			{
				PrintProfileSignature(hwndCtl, NULL, *lpdwTags++, FALSE);
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT(" | %8u bytes"), _byteswap_ulong(*lpdwTags++));
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput), TEXT(" | %6u bytes"), _byteswap_ulong(*lpdwTags++));
				OutputText(hwndCtl, TEXT(" |\r\n"));
			}
		}
	}

Exit:
	GlobalUnlock(hDib);

	if (bIsDibSupported)
		ReplaceThumbnail(hDlg, hDib);
	else
	{
		GlobalFree(hDib);
		MarkAsUnsupported(hDlg);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// IsDibSupported: Determines whether GDI can display a specific DIB image.
// Returns TRUE even if the info could not be retrieved (e.g. for a core DIB).

static BOOL IsDibSupported(LPCSTR lpbi)
{
	if (lpbi == NULL)
		return FALSE;

	return (QueryDibSupport(lpbi) & QDI_DIBTOSCREEN) != 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// The QUERYDIBSUPPORT escape function determines whether GDI supports a specific DIB image.
// It only checks if biCompression contains either BI_RGB, BI_RLE4, BI_RLE8 or BI_BITFIELDS
// and if the value of biBitCount matches the format. Core DIBs are therefore not supported.

static DWORD QueryDibSupport(LPCSTR lpbi)
{
	if (lpbi == NULL)
		return (DWORD)-1;

	HDC hdc = GetDC(NULL);
	if (hdc == NULL)
		return (DWORD)-1;

	DWORD dwFlags = 0;
	if (Escape(hdc, QUERYDIBSUPPORT, *(LPDWORD)lpbi, lpbi, (LPVOID)&dwFlags) <= 0)
		dwFlags = (DWORD)-1;

	ReleaseDC(NULL, hdc);

	return dwFlags;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Returns the color name if the color passed is one of the 20 static system palette colors

BOOL GetColorName(COLORREF rgbColor, LPCTSTR* lpszName)
{
	if (lpszName == NULL)
		return FALSE;

	*lpszName = TEXT("");
	size_t count = sizeof(g_aColorNames) / sizeof(ColorName);

	for (size_t i = 0; i < count; i++)
	{
		if (g_aColorNames[i].rgbColor == rgbColor)
		{
			*lpszName = g_aColorNames[i].lpszName;
			return TRUE;
		}
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Outputs an ICC profile signature given in big-endian format

void PrintProfileSignature(HWND hwndCtl, LPCTSTR lpszName, DWORD dwSignature, BOOL bAddCrLf)
{
	TCHAR szOutput[OUTPUT_LEN];

	if (hwndCtl == NULL)
		return;

	if (lpszName != NULL)
		OutputText(hwndCtl, lpszName);

	if (dwSignature == 0)
		OutputText(hwndCtl, TEXT("0"));
	else
		if (isprint(dwSignature & 0xff) &&
			isprint((dwSignature >>  8) & 0xff) &&
			isprint((dwSignature >> 16) & 0xff) &&
			isprint((dwSignature >> 24) & 0xff))
		{ // Sequence of four characters
			OutputTextFmt(hwndCtl, szOutput, _countof(szOutput),
				TEXT("%hc%hc%hc%hc"),
				(char)(dwSignature & 0xff),
				(char)((dwSignature >>  8) & 0xff),
				(char)((dwSignature >> 16) & 0xff),
				(char)((dwSignature >> 24) & 0xff));
		}
		else
			if (isprint(dwSignature & 0xff) &&
				isprint((dwSignature >> 8) & 0xff))
			{ // Extended colour space signature
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput),
					TEXT("%hc%hc%02X%02X"),
					(char)(dwSignature & 0xff),
					(char)((dwSignature >>  8) & 0xff),
					(BYTE)((dwSignature >> 16) & 0xff),
					(BYTE)((dwSignature >> 24) & 0xff));
			}
			else
				OutputTextFmt(hwndCtl, szOutput, _countof(szOutput),
					TEXT("%08X"), _byteswap_ulong(dwSignature));

	if (bAddCrLf)
		OutputText(hwndCtl, TEXT("\r\n"));
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Converts a half-precision floating-point number to a single-precision float.
// Special values like subnormals, infinities, NaN's and -0 are not supported.

float Half2Float(WORD h)
{
	DWORD f = h ? ((h & 0x8000) << 16) | (((h & 0x7C00) + 0x1C000) << 13) | ((h & 0x03FF) << 13) : 0;

	return *((float*)((void*)&f));
}

////////////////////////////////////////////////////////////////////////////////////////////////
