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
#include "file.h"
#include "archive.h"
#include "dumpbmp.h"

////////////////////////////////////////////////////////////////////////////////////////////////

static void MarkAsUnsupported(HWND hwndCtl)
{
	// Draw "UNSUPPORTED FORMAT" lettering over the default thumbnail image
	HWND hwndThumb = GetDlgItem(GetParent(hwndCtl), IDC_THUMB);
	if (hwndThumb != NULL)
	{
		TCHAR szText[256];
		if (LoadString(g_hInstance, g_bGerUI ? IDS_GER_UNSUPPORTED : IDS_ENG_UNSUPPORTED,
			szText, _countof(szText)) > 0)
		{
			SetWindowText(hwndThumb, szText);
			if (InvalidateRect(hwndThumb, NULL, FALSE))
				UpdateWindow(hwndThumb);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
// DumpBitmap is called by DumpFile from GbxDump.cpp

BOOL DumpBitmap(HWND hwndCtl, HANDLE hFile, DWORD dwFileSize)
{
	TCHAR szOutput[OUTPUT_LEN];
	BITMAPFILEHEADER bfh;
	DWORD dwFileHeaderSize = sizeof(bfh);

	if (hwndCtl == NULL || hFile == NULL || dwFileSize < dwFileHeaderSize)
		return FALSE;

	// Jump to the beginning of the file
	if (!FileSeekBegin(hFile, 0))
		return FALSE;

	// Read the file header
	ZeroMemory(&bfh, sizeof(bfh));
	if (!ReadData(hFile, (LPVOID)&bfh, sizeof(bfh)))
		return FALSE;

	if (bfh.bfType == BFT_BITMAPARRAY)
	{ // OS/2 bitmap array
		LPBITMAPARRAYFILEHEADER lpbafh = (LPBITMAPARRAYFILEHEADER)&bfh;

		OutputText(hwndCtl, g_szSep1);
		OutputTextFmt(hwndCtl, szOutput, TEXT("Type:\t\t%hc%hc\r\n"),
			LOBYTE(lpbafh->usType), HIBYTE(lpbafh->usType));
		OutputTextFmt(hwndCtl, szOutput, TEXT("Size:\t\t%u bytes\r\n"), lpbafh->cbSize);
		OutputTextFmt(hwndCtl, szOutput, TEXT("OffNext:\t%u bytes\r\n"), lpbafh->offNext);
		OutputTextFmt(hwndCtl, szOutput, TEXT("CxDisplay:\t%u\r\n"), lpbafh->cxDisplay);
		OutputTextFmt(hwndCtl, szOutput, TEXT("CyDisplay:\t%u\r\n"), lpbafh->cyDisplay);

		// Proceed only if the array contains only one bitmap
		if (lpbafh->offNext != 0)
		{
			MarkAsUnsupported(hwndCtl);
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
		{
			MarkAsUnsupported(hwndCtl);
			return TRUE;
		}
	}

	OutputText(hwndCtl, g_szSep1);
	OutputTextFmt(hwndCtl, szOutput, TEXT("Type:\t\t%hc%hc\r\n"),
		LOBYTE(bfh.bfType), HIBYTE(bfh.bfType));
	OutputTextFmt(hwndCtl, szOutput, TEXT("Size:\t\t%u bytes\r\n"), bfh.bfSize);
	OutputTextFmt(hwndCtl, szOutput, TEXT("Reserved1:\t%u\r\n"), bfh.bfReserved1);
	OutputTextFmt(hwndCtl, szOutput, TEXT("Reserved2:\t%u\r\n"), bfh.bfReserved2);
	OutputTextFmt(hwndCtl, szOutput, TEXT("OffBits:\t%u bytes\r\n"), bfh.bfOffBits);

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

	// Read the DIB
	if (!ReadData(hFile, lpbi, dwDibSize))
	{
		GlobalUnlock(hDib);
		GlobalFree(hDib);
		return FALSE;
	}

	DWORD dwDibHeaderSize = *(LPDWORD)lpbi;
	LPBITMAPV5HEADER lpbih = (LPBITMAPV5HEADER)lpbi;
	LPBITMAPCOREHEADER lpbch = (LPBITMAPCOREHEADER)lpbi;
	LPBITMAPINFOHEADER2 lpbih2 = (LPBITMAPINFOHEADER2)lpbi;

	OutputText(hwndCtl, g_szSep1);
	OutputTextFmt(hwndCtl, szOutput, TEXT("Size:\t\t%u bytes\r\n"), dwDibHeaderSize);

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
		GlobalUnlock(hDib);
		GlobalFree(hDib);
		MarkAsUnsupported(hwndCtl);
		return TRUE;
	}

	// Since we don't perform format conversions here, all formats
	// that the GDI cannot directly display are marked as unsupported
	BOOL bIsUnsupportedFormat = FALSE;

	if (dwDibHeaderSize == sizeof(BITMAPCOREHEADER))
	{ // OS/2 Version 1.x Bitmap
		OutputTextFmt(hwndCtl, szOutput, TEXT("Width:\t\t%u pixels\r\n"), lpbch->bcWidth);
		OutputTextFmt(hwndCtl, szOutput, TEXT("Height:\t\t%u pixels\r\n"), lpbch->bcHeight);
		OutputTextFmt(hwndCtl, szOutput, TEXT("Planes:\t\t%u\r\n"), lpbch->bcPlanes);
		OutputTextFmt(hwndCtl, szOutput, TEXT("BitCount:\t%u bpp\r\n"), lpbch->bcBitCount);
		
		UINT64 ullBitsSize = (UINT64)((((lpbch->bcWidth * lpbch->bcPlanes *
			lpbch->bcBitCount) + 31) / 32) * 4) * lpbch->bcHeight;

		// Windows also supports the Windows Version 2.0 variant with 16 bpp and 32 bpp
		if (ullBitsSize == 0 || ullBitsSize > 0xFFFFFFFF ||
			lpbch->bcPlanes > 4 || lpbch->bcBitCount > 32)
			bIsUnsupportedFormat = TRUE;
	}

	if (dwDibHeaderSize >= sizeof(BITMAPINFOHEADER))
	{ // Windows Version 3.0 Bitmap
		OutputTextFmt(hwndCtl, szOutput, TEXT("Width:\t\t%d pixels\r\n"), lpbih->bV5Width);
		OutputTextFmt(hwndCtl, szOutput, TEXT("Height:\t\t%d pixels\r\n"), lpbih->bV5Height);
		OutputTextFmt(hwndCtl, szOutput, TEXT("Planes:\t\t%u\r\n"), lpbih->bV5Planes);
		OutputTextFmt(hwndCtl, szOutput, TEXT("BitCount:\t%u bpp\r\n"), lpbih->bV5BitCount);

		INT64 llBitsSize = (INT64)((((lpbih->bV5Width * lpbih->bV5Planes *
			lpbih->bV5BitCount) + 31) / 32) * 4) * abs(lpbih->bV5Height);

		// BitCount == 2 is valid for Windows CE, but is not supported by the PC version of Windows
		if (llBitsSize <= 0 || llBitsSize > 0xFFFFFFFFL || lpbih->bV5Planes > 4 ||
			lpbih->bV5BitCount == 2 || lpbih->bV5BitCount > 32)
			bIsUnsupportedFormat = TRUE;

		DWORD dwCompression = lpbih->bV5Compression;
		OutputText(hwndCtl, TEXT("Compression:\t"));
		if (dwDibHeaderSize == sizeof(BITMAPINFOHEADER2))
		{ // All unsupported, see special handling of OS/2 2.0 bitmaps below
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
					OutputTextFmt(hwndCtl, szOutput, TEXT("%u"), dwCompression);
			}
		}
		else
		{
			if (isprint(dwCompression & 0xff) &&
				isprint((dwCompression >>  8) & 0xff) &&
				isprint((dwCompression >> 16) & 0xff) &&
				isprint((dwCompression >> 24) & 0xff))
			{ // All video formats are unsupported
				bIsUnsupportedFormat = TRUE;
				OutputTextFmt(hwndCtl, szOutput, TEXT("%hc%hc%hc%hc"),
					(char)(dwCompression & 0xff),
					(char)((dwCompression >>  8) & 0xff),
					(char)((dwCompression >> 16) & 0xff),
					(char)((dwCompression >> 24) & 0xff));
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
						bIsUnsupportedFormat = TRUE;
						break;
					case BI_PNG:
						OutputText(hwndCtl, TEXT("PNG"));
						bIsUnsupportedFormat = TRUE;
						break;
					case BI_ALPHABITFIELDS:
						OutputText(hwndCtl, TEXT("ALPHABITFIELDS"));
						bIsUnsupportedFormat = TRUE;
						break;
					case BI_CMYK:
						OutputText(hwndCtl, TEXT("CMYK"));
						bIsUnsupportedFormat = TRUE;
						break;
					case BI_CMYKRLE8:
						OutputText(hwndCtl, TEXT("CMYKRLE8"));
						bIsUnsupportedFormat = TRUE;
						break;
					case BI_CMYKRLE4:
						OutputText(hwndCtl, TEXT("CMYKRLE4"));
						bIsUnsupportedFormat = TRUE;
						break;
					default:
						OutputTextFmt(hwndCtl, szOutput, TEXT("%u"), dwCompression);
						bIsUnsupportedFormat = TRUE;
				}
			}
		}
		OutputText(hwndCtl, TEXT("\r\n"));

		OutputTextFmt(hwndCtl, szOutput, TEXT("SizeImage:\t%u bytes\r\n"), lpbih->bV5SizeImage);

		OutputTextFmt(hwndCtl, szOutput, TEXT("XPelsPerMeter:\t%d"), lpbih->bV5XPelsPerMeter);
		if (lpbih->bV5XPelsPerMeter > 0)
			OutputTextFmt(hwndCtl, szOutput, TEXT(" (%d dpi)"),
				MulDiv(lpbih->bV5XPelsPerMeter, 127, 5000));
		OutputText(hwndCtl, TEXT("\r\n"));
			
		OutputTextFmt(hwndCtl, szOutput, TEXT("YPelsPerMeter:\t%d"), lpbih->bV5YPelsPerMeter);
		if (lpbih->bV5YPelsPerMeter > 0)
			OutputTextFmt(hwndCtl, szOutput, TEXT(" (%d dpi)"),
				MulDiv(lpbih->bV5YPelsPerMeter, 127, 5000));
		OutputText(hwndCtl, TEXT("\r\n"));
			
		OutputTextFmt(hwndCtl, szOutput, TEXT("ClrUsed:\t%u\r\n"), lpbih->bV5ClrUsed);
		OutputTextFmt(hwndCtl, szOutput, TEXT("ClrImportant:\t%u\r\n"), lpbih->bV5ClrImportant);
	}

	// The size of the OS/2 2.0 header can be reduced from its full size of
	// 64 bytes down to 16 bytes. Omitted fields are assumed to have a value
	// of zero. However, we only support bitmaps with full header here.
	if (dwDibHeaderSize == sizeof(BITMAPINFOHEADER2))
	{ // OS/2 Version 2.0 Bitmap
		OutputText(hwndCtl, TEXT("Units:\t\t"));
		if (lpbih2->usUnits == BRU_METRIC)
			OutputText(hwndCtl, TEXT("METRIC"));
		else
			OutputTextFmt(hwndCtl, szOutput, TEXT("%u"), lpbih2->usUnits);
		OutputText(hwndCtl, TEXT("\r\n"));

		OutputTextFmt(hwndCtl, szOutput, TEXT("Reserved:\t%u\r\n"), lpbih2->usReserved);

		OutputText(hwndCtl, TEXT("Recording:\t"));
		if (lpbih2->usRecording == BRA_BOTTOMUP)
			OutputText(hwndCtl, TEXT("BOTTOMUP"));
		else
			OutputTextFmt(hwndCtl, szOutput, TEXT("%u"), lpbih2->usRecording);
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
				OutputTextFmt(hwndCtl, szOutput, TEXT("%u"), usRendering);
		}
		OutputText(hwndCtl, TEXT("\r\n"));

		OutputTextFmt(hwndCtl, szOutput, TEXT("Size1:\t\t%u\r\n"), lpbih2->cSize1);
		OutputTextFmt(hwndCtl, szOutput, TEXT("Size2:\t\t%u\r\n"), lpbih2->cSize2);

		OutputText(hwndCtl, TEXT("ColorEncoding:\t"));
		if (lpbih2->ulColorEncoding == BCE_RGB)
			OutputText(hwndCtl, TEXT("RGB"));
		else if (lpbih2->ulColorEncoding == BCE_PALETTE)
			OutputText(hwndCtl, TEXT("PALETTE"));
		else
			OutputTextFmt(hwndCtl, szOutput, TEXT("%u"), lpbih2->ulColorEncoding);
		OutputText(hwndCtl, TEXT("\r\n"));

		OutputTextFmt(hwndCtl, szOutput, TEXT("Identifier:\t%u\r\n"), lpbih2->ulIdentifier);

		// OS/2 2.0 bitmaps are not supported by Windows
		GlobalUnlock(hDib);
		GlobalFree(hDib);
		MarkAsUnsupported(hwndCtl);

		return TRUE;
	}

	if (dwDibHeaderSize >= sizeof(BITMAPV2INFOHEADER))
	{ // Adobe Photoshop
		OutputTextFmt(hwndCtl, szOutput, TEXT("RedMask:\t%08X\r\n"), lpbih->bV5RedMask);
		OutputTextFmt(hwndCtl, szOutput, TEXT("GreenMask:\t%08X\r\n"), lpbih->bV5GreenMask);
		OutputTextFmt(hwndCtl, szOutput, TEXT("BlueMask:\t%08X\r\n"), lpbih->bV5BlueMask);
	}

	if (dwDibHeaderSize >= sizeof(BITMAPV3INFOHEADER))
	{ // Adobe Photoshop
		OutputTextFmt(hwndCtl, szOutput, TEXT("AlphaMask:\t%08X\r\n"), lpbih->bV5AlphaMask);
	}

	if (dwDibHeaderSize >= sizeof(BITMAPV4HEADER))
	{ // Win32 Version 4.0 Bitmap
		DWORD dwCSType = lpbih->bV5CSType;
		OutputText(hwndCtl, TEXT("CSType:\t\t"));
		if (isprint(dwCSType & 0xff) &&
			isprint((dwCSType >>  8) & 0xff) &&
			isprint((dwCSType >> 16) & 0xff) &&
			isprint((dwCSType >> 24) & 0xff))
		{
			OutputTextFmt(hwndCtl, szOutput, TEXT("%hc%hc%hc%hc"),
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
					OutputTextFmt(hwndCtl, szOutput, TEXT("%u"), dwCSType);
			}
		}
		OutputText(hwndCtl, TEXT("\r\n"));

		_sntprintf(szOutput, _countof(szOutput),
			TEXT("CIEXYZRed:\t%.4f, %.4f, %.4f\r\n"),
			(double)lpbih->bV5Endpoints.ciexyzRed.ciexyzX / 0x40000000,
			(double)lpbih->bV5Endpoints.ciexyzRed.ciexyzY / 0x40000000,
			(double)lpbih->bV5Endpoints.ciexyzRed.ciexyzZ / 0x40000000);
		OutputText(hwndCtl, szOutput);
		_sntprintf(szOutput, _countof(szOutput),
			TEXT("CIEXYZGreen:\t%.4f, %.4f, %.4f\r\n"),
			(double)lpbih->bV5Endpoints.ciexyzGreen.ciexyzX / 0x40000000,
			(double)lpbih->bV5Endpoints.ciexyzGreen.ciexyzY / 0x40000000,
			(double)lpbih->bV5Endpoints.ciexyzGreen.ciexyzZ / 0x40000000);
		OutputText(hwndCtl, szOutput);
		_sntprintf(szOutput, _countof(szOutput),
			TEXT("CIEXYZBlue:\t%.4f, %.4f, %.4f\r\n"),
			(double)lpbih->bV5Endpoints.ciexyzBlue.ciexyzX / 0x40000000,
			(double)lpbih->bV5Endpoints.ciexyzBlue.ciexyzY / 0x40000000,
			(double)lpbih->bV5Endpoints.ciexyzBlue.ciexyzZ / 0x40000000);
		OutputText(hwndCtl, szOutput);

		_sntprintf(szOutput, _countof(szOutput), TEXT("GammaRed:\t%g\r\n"),
			(double)lpbih->bV5GammaRed / 0x10000);
		OutputText(hwndCtl, szOutput);
		_sntprintf(szOutput, _countof(szOutput), TEXT("GammaGreen:\t%g\r\n"),
			(double)lpbih->bV5GammaGreen / 0x10000);
		OutputText(hwndCtl, szOutput);
		_sntprintf(szOutput, _countof(szOutput), TEXT("GammaBlue:\t%g\r\n"),
			(double)lpbih->bV5GammaBlue / 0x10000);
		OutputText(hwndCtl, szOutput);
	}

	if (dwDibHeaderSize >= sizeof(BITMAPV5HEADER))
	{ // Win32 Version 5.0 Bitmap
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
				OutputTextFmt(hwndCtl, szOutput, TEXT("%u"), dwIntent);
		}
		OutputText(hwndCtl, TEXT("\r\n"));

		OutputTextFmt(hwndCtl, szOutput, TEXT("ProfileData:\t%u bytes\r\n"), lpbih->bV5ProfileData);

		OutputTextFmt(hwndCtl, szOutput, TEXT("ProfileSize:\t%u bytes"), lpbih->bV5ProfileSize);
		if (lpbih->bV5ProfileData != 0 && lpbih->bV5ProfileSize != 0 &&
			lpbih->bV5CSType == PROFILE_LINKED && dwDibSize > lpbih->bV5ProfileData)
		{
			char szPath[MAX_PATH] = { 0 };
			int nLen = min(min(lpbih->bV5ProfileSize, dwDibSize - lpbih->bV5ProfileData), _countof(szPath));
			lstrcpynA(szPath, (LPSTR)lpbih + lpbih->bV5ProfileData, nLen);
			OutputTextFmt(hwndCtl, szOutput, TEXT(" (%hs)"), szPath);
		}
		OutputText(hwndCtl, TEXT("\r\n"));

		OutputTextFmt(hwndCtl, szOutput, TEXT("Reserved:\t%u\r\n"), lpbih->bV5Reserved);
	}

	GlobalUnlock(hDib);

	if (bIsUnsupportedFormat)
	{
		GlobalFree(hDib);
		MarkAsUnsupported(hwndCtl);
		return TRUE;
	}

	if (g_hBitmapThumb != NULL)
		FreeBitmap(g_hBitmapThumb);
	if (g_hDibThumb != NULL)
		FreeDib(g_hDibThumb);

	g_hDibThumb = hDib;
	g_hBitmapThumb = CreatePremultipliedBitmap(hDib);

	// View the thumbnail immediately
	HWND hwndThumb = GetDlgItem(GetParent(hwndCtl), IDC_THUMB);
	if (hwndThumb != NULL)
	{
		if (InvalidateRect(hwndThumb, NULL, FALSE))
			UpdateWindow(hwndThumb);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
