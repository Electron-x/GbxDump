////////////////////////////////////////////////////////////////////////////////////////////////
// DumpDds.cpp - Copyright (c) 2010-2018 by Electron.
//
// Licensed under the EUPL, Version 1.2 or – as soon they will be approved by
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
#include "dumpdds.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// DumpDDS is called by DumpFile from GbxDump.cpp

BOOL DumpDDS(HWND hwndCtl, HANDLE hFile)
{
	DWORD dwFlags;
	TCHAR szFlags[32];
	TCHAR szOutput[OUTPUT_LEN];

	const TCHAR chNil = TEXT('\0');
	const TCHAR szOR[] = TEXT("|");
	const TCHAR szCRLF[] = TEXT("\r\n");
	const TCHAR szUnknown[] = TEXT("Unknown");

	if (hwndCtl == NULL || hFile == NULL)
		return FALSE;

	// Jump to header (skip file signature)
	if (!FileSeekBegin(hFile, 4))
		return FALSE;

	// Read header
	DDS_HEADER ddsh;
	ZeroMemory(&ddsh, sizeof(ddsh));
	if (!ReadData(hFile, (LPVOID)&ddsh, sizeof(ddsh)))
		return FALSE;

	// Output data
	OutputText(hwndCtl, g_szSep1);

	OutputText(hwndCtl, TEXT("Header Size:\t"));
	FormatByteSize(ddsh.dwSize, szOutput, _countof(szOutput));
	OutputText(hwndCtl, szOutput);
	if (ddsh.dwSize != 124) OutputText(hwndCtl, TEXT("!"));
	OutputText(hwndCtl, szCRLF);

	OutputTextFmt(hwndCtl, szOutput, TEXT("Header Flags:\t%08X"), ddsh.dwHeaderFlags);
	if (ddsh.dwHeaderFlags != 0)
	{
		szOutput[0] = chNil;
		dwFlags = ddsh.dwHeaderFlags;
		if (ddsh.dwHeaderFlags & DDSD_HEIGHT)
		{
			dwFlags ^= DDSD_HEIGHT;
			if (szOutput[0] != chNil)
				_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("HEIGHT"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}
		if (ddsh.dwHeaderFlags & DDSD_WIDTH)
		{
			dwFlags ^= DDSD_WIDTH;
			if (szOutput[0] != chNil)
				_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("WIDTH"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}
		if (ddsh.dwHeaderFlags & DDSD_PITCH)
		{
			dwFlags ^= DDSD_PITCH;
			if (szOutput[0] != chNil)
				_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("PITCH"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}
		if (ddsh.dwHeaderFlags & DDSD_LINEARSIZE)
		{
			dwFlags ^= DDSD_LINEARSIZE;
			if (szOutput[0] != chNil)
				_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("LINEARSIZE"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}
		if (ddsh.dwHeaderFlags & DDSD_DEPTH)
		{
			dwFlags ^= DDSD_DEPTH;
			if (szOutput[0] != chNil)
				_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("DEPTH"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}
		if (ddsh.dwHeaderFlags & DDSD_MIPMAPCOUNT)
		{
			dwFlags ^= DDSD_MIPMAPCOUNT;
			if (szOutput[0] != chNil)
				_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("MIPMAPCOUNT"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}
		if (ddsh.dwHeaderFlags & DDSD_PIXELFORMAT)
		{
			dwFlags ^= DDSD_PIXELFORMAT;
			if (szOutput[0] != chNil)
				_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("PIXELFORMAT"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}
		if (ddsh.dwHeaderFlags & DDSD_CAPS)
		{
			dwFlags ^= DDSD_CAPS;
			if (szOutput[0] != chNil)
				_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_tcsncat(szOutput, TEXT("CAPS"), _countof(szOutput) - _tcslen(szOutput) - 1);
		}
		if (dwFlags != 0 && dwFlags != ddsh.dwHeaderFlags)
		{
			if (szOutput[0] != chNil)
				_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
			_sntprintf(szFlags, _countof(szFlags), TEXT("%08X"), dwFlags);
			_tcsncat(szOutput, szFlags, _countof(szOutput) - _tcslen(szOutput) - 1);
		}

		if (szOutput[0] != chNil)
		{
			OutputText(hwndCtl, TEXT(" ("));
			OutputText(hwndCtl, szOutput);
			OutputText(hwndCtl, TEXT(")"));
		}
	}
	OutputText(hwndCtl, szCRLF);

	if ((ddsh.dwHeaderFlags & DDSD_HEIGHT) || ddsh.dwHeight != 0)
		OutputTextFmt(hwndCtl, szOutput, TEXT("Surface Height:\t%u pixels\r\n"), ddsh.dwHeight);

	if ((ddsh.dwHeaderFlags & DDSD_WIDTH) || ddsh.dwWidth != 0)
		OutputTextFmt(hwndCtl, szOutput, TEXT("Surface Width:\t%u pixels\r\n"), ddsh.dwWidth);

	if ((ddsh.dwHeaderFlags & DDSD_PITCH) || (ddsh.dwHeaderFlags & DDSD_LINEARSIZE))
	{
		if (ddsh.dwHeaderFlags & DDSD_PITCH)
			OutputTextFmt(hwndCtl, szOutput, TEXT("Pitch:\t\t%u bytes\r\n"), ddsh.dwPitchOrLinearSize);

		if (ddsh.dwHeaderFlags & DDSD_LINEARSIZE)
			OutputTextFmt(hwndCtl, szOutput, TEXT("Linear Size:\t%u bytes\r\n"), ddsh.dwPitchOrLinearSize);
	}
	else if (ddsh.dwPitchOrLinearSize != 0)
		OutputTextFmt(hwndCtl, szOutput, TEXT("Pitch Or Size:\t%u bytes\r\n"), ddsh.dwPitchOrLinearSize);

	if ((ddsh.dwHeaderFlags & DDSD_DEPTH) || ddsh.dwDepth != 0)
		OutputTextFmt(hwndCtl, szOutput, TEXT("Depth:\t\t%u pixels\r\n"), ddsh.dwDepth);

	if ((ddsh.dwHeaderFlags & DDSD_MIPMAPCOUNT) || ddsh.dwMipMapCount != 0)
		OutputTextFmt(hwndCtl, szOutput, TEXT("Mipmap Levels:\t%u\r\n"), ddsh.dwMipMapCount);

	if ((ddsh.dwHeaderFlags & DDSD_PIXELFORMAT) || ddsh.ddspf.dwSize > 0)
	{
		OutputText(hwndCtl, TEXT("PixelFmt Size:\t"));
		FormatByteSize(ddsh.ddspf.dwSize, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
		if (ddsh.ddspf.dwSize != 32) OutputText(hwndCtl, TEXT("!"));
		OutputText(hwndCtl, szCRLF);

		OutputTextFmt(hwndCtl, szOutput, TEXT("PixelFmt Flags:\t%08X"), ddsh.ddspf.dwFlags);
		if (ddsh.ddspf.dwFlags != 0)
		{
			szOutput[0] = chNil;
			dwFlags = ddsh.ddspf.dwFlags;
			if (ddsh.ddspf.dwFlags & DDPF_FOURCC)
			{
				dwFlags ^= DDPF_FOURCC;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("FOURCC"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.ddspf.dwFlags & DDPF_RGB)
			{
				dwFlags ^= DDPF_RGB;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("RGB"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.ddspf.dwFlags & DDPF_YUV)
			{
				dwFlags ^= DDPF_YUV;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("YUV"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.ddspf.dwFlags & DDPF_LUMINANCE)
			{
				dwFlags ^= DDPF_LUMINANCE;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("LUMINANCE"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.ddspf.dwFlags & DDPF_PALETTEINDEXED8)
			{
				dwFlags ^= DDPF_PALETTEINDEXED8;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("PAL8"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.ddspf.dwFlags & DDPF_ALPHAPIXELS)
			{
				dwFlags ^= DDPF_ALPHAPIXELS;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("ALPHAPIXELS"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.ddspf.dwFlags & DDPF_ALPHA)
			{
				dwFlags ^= DDPF_ALPHA;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("ALPHA"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.ddspf.dwFlags & DDPF_ALPHAPREMULT)
			{
				dwFlags ^= DDPF_ALPHAPREMULT;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("ALPHAPREMULT"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.ddspf.dwFlags & DDPF_BUMPLUMINANCE)
			{
				dwFlags ^= DDPF_BUMPLUMINANCE;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("BUMPLUMINANCE"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.ddspf.dwFlags & DDPF_BUMPDUDV)
			{
				dwFlags ^= DDPF_BUMPDUDV;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("BUMPDUDV"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.ddspf.dwFlags & DDPF_NORMAL)
			{
				dwFlags ^= DDPF_NORMAL;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("NORMAL"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.ddspf.dwFlags & DDPF_SRGB)
			{
				dwFlags ^= DDPF_SRGB;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("SRGB"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (dwFlags != 0 && dwFlags != ddsh.ddspf.dwFlags)
			{
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_sntprintf(szFlags, _countof(szFlags), TEXT("%08X"), dwFlags);
				_tcsncat(szOutput, szFlags, _countof(szOutput) - _tcslen(szOutput) - 1);
			}

			if (szOutput[0] != chNil)
			{
				OutputText(hwndCtl, TEXT(" ("));
				OutputText(hwndCtl, szOutput);
				OutputText(hwndCtl, TEXT(")"));
			}
		}
		OutputText(hwndCtl, szCRLF);

		if (ddsh.ddspf.dwFlags & DDPF_FOURCC)
		{
			BYTE achFourCC[4];
			CopyMemory(achFourCC, &ddsh.ddspf.dwFourCC, 4);

			OutputText(hwndCtl, TEXT("FourCC Tag:\t"));
			if (isprint(achFourCC[0]) && isprint(achFourCC[1]) &&
				isprint(achFourCC[2]) && isprint(achFourCC[3]))
				OutputTextFmt(hwndCtl, szOutput, TEXT("%hc%hc%hc%hc"),
					achFourCC[0], achFourCC[1], achFourCC[2], achFourCC[3]);
			else
			{
				if (ddsh.ddspf.dwFourCC != 0)
				{
					OutputTextFmt(hwndCtl, szOutput, TEXT("%u"), ddsh.ddspf.dwFourCC);

					switch (ddsh.ddspf.dwFourCC)
					{
						case 36:
							OutputText(hwndCtl, TEXT(" (A16B16G16R16)"));
							break;
						case 110:
							OutputText(hwndCtl, TEXT(" (Q16W16V16U16)"));
							break;
						case 111:
							OutputText(hwndCtl, TEXT(" (R16F)"));
							break;
						case 112:
							OutputText(hwndCtl, TEXT(" (G16R16F)"));
							break;
						case 113:
							OutputText(hwndCtl, TEXT(" (A16B16G16R16F)"));
							break;
						case 114:
							OutputText(hwndCtl, TEXT(" (R32F)"));
							break;
						case 115:
							OutputText(hwndCtl, TEXT(" (G32R32F)"));
							break;
						case 116:
							OutputText(hwndCtl, TEXT(" (A32B32G32R32F)"));
							break;
						case 117:
							OutputText(hwndCtl, TEXT(" (CxV8U8)"));
							break;
					}
				}
				else
					OutputText(hwndCtl, szUnknown);
			}
			OutputText(hwndCtl, szCRLF);
		}

		if ((ddsh.ddspf.dwFlags & DDPF_PALETTEINDEXED8) || (ddsh.ddspf.dwFlags & DDPF_RGB) ||
			(ddsh.ddspf.dwFlags & DDPF_LUMINANCE) || (ddsh.ddspf.dwFlags & DDPF_YUV) ||
			(ddsh.ddspf.dwFlags & DDPF_ALPHA) || (ddsh.ddspf.dwFlags & DDPF_BUMPDUDV) ||
			(ddsh.ddspf.dwFlags & DDPF_BUMPLUMINANCE))
			OutputTextFmt(hwndCtl, szOutput, TEXT("Bit Count:\t%u bits\r\n"), ddsh.ddspf.dwRGBBitCount);

		if ((ddsh.ddspf.dwFlags & DDPF_RGB) || (ddsh.ddspf.dwFlags & DDPF_YUV) ||
			(ddsh.ddspf.dwFlags & DDPF_LUMINANCE || (ddsh.ddspf.dwFlags & DDPF_BUMPDUDV) ||
			(ddsh.ddspf.dwFlags & DDPF_BUMPLUMINANCE)))
			OutputTextFmt(hwndCtl, szOutput, TEXT("Red Mask:\t%08X\r\n"), ddsh.ddspf.dwRBitMask);

		if ((ddsh.ddspf.dwFlags & DDPF_RGB) || (ddsh.ddspf.dwFlags & DDPF_YUV) ||
			(ddsh.ddspf.dwFlags & DDPF_BUMPDUDV) || (ddsh.ddspf.dwFlags & DDPF_BUMPLUMINANCE))
		{
			OutputTextFmt(hwndCtl, szOutput, TEXT("Green Mask:\t%08X\r\n"), ddsh.ddspf.dwGBitMask);
			OutputTextFmt(hwndCtl, szOutput, TEXT("Blue Mask:\t%08X\r\n"), ddsh.ddspf.dwBBitMask);
		}

		if ((ddsh.ddspf.dwFlags & DDPF_ALPHAPIXELS) || (ddsh.ddspf.dwFlags & DDPF_ALPHA) ||
			(ddsh.ddspf.dwFlags & DDPF_BUMPDUDV))
			OutputTextFmt(hwndCtl, szOutput, TEXT("Alpha Mask:\t%08X\r\n"), ddsh.ddspf.dwABitMask);
	}

	if ((ddsh.dwHeaderFlags & DDSD_CAPS) || ddsh.dwSurfaceFlags != 0 || ddsh.dwCubemapFlags != 0)
	{
		OutputTextFmt(hwndCtl, szOutput, TEXT("Surface Flags:\t%08X"), ddsh.dwSurfaceFlags);
		if (ddsh.dwSurfaceFlags != 0)
		{
			szOutput[0] = chNil;
			dwFlags = ddsh.dwSurfaceFlags;
			if (ddsh.dwSurfaceFlags & DDSCAPS_TEXTURE)
			{
				dwFlags ^= DDSCAPS_TEXTURE;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("TEXTURE"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.dwSurfaceFlags & DDSCAPS_MIPMAP)
			{
				dwFlags ^= DDSCAPS_MIPMAP;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("MIPMAP"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.dwSurfaceFlags & DDSCAPS_COMPLEX)
			{
				dwFlags ^= DDSCAPS_COMPLEX;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("COMPLEX"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.dwSurfaceFlags & DDSCAPS_ALPHA)
			{
				dwFlags ^= DDSCAPS_ALPHA;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("ALPHA"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (dwFlags != 0 && dwFlags != ddsh.dwSurfaceFlags)
			{
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_sntprintf(szFlags, _countof(szFlags), TEXT("%08X"), dwFlags);
				_tcsncat(szOutput, szFlags, _countof(szOutput) - _tcslen(szOutput) - 1);
			}

			if (szOutput[0] != chNil)
			{
				OutputText(hwndCtl, TEXT(" ("));
				OutputText(hwndCtl, szOutput);
				OutputText(hwndCtl, TEXT(")"));
			}
		}
		OutputText(hwndCtl, szCRLF);

		if (ddsh.dwCubemapFlags != 0)
		{
			OutputTextFmt(hwndCtl, szOutput, TEXT("Cubemap Flags:\t%08X"), ddsh.dwCubemapFlags);

			szOutput[0] = chNil;
			dwFlags = ddsh.dwCubemapFlags;
			if (ddsh.dwCubemapFlags & DDSCAPS2_CUBEMAP)
			{
				dwFlags ^= DDSCAPS2_CUBEMAP;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("CUBEMAP"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.dwCubemapFlags & DDSCAPS2_CUBEMAP_POSITIVEX)
			{
				dwFlags ^= DDSCAPS2_CUBEMAP_POSITIVEX;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("POSX"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.dwCubemapFlags & DDSCAPS2_CUBEMAP_NEGATIVEX)
			{
				dwFlags ^= DDSCAPS2_CUBEMAP_NEGATIVEX;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("NEGX"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.dwCubemapFlags & DDSCAPS2_CUBEMAP_POSITIVEY)
			{
				dwFlags ^= DDSCAPS2_CUBEMAP_POSITIVEY;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("POSY"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.dwCubemapFlags & DDSCAPS2_CUBEMAP_NEGATIVEY)
			{
				dwFlags ^= DDSCAPS2_CUBEMAP_NEGATIVEY;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("NEGY"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.dwCubemapFlags & DDSCAPS2_CUBEMAP_POSITIVEZ)
			{
				dwFlags ^= DDSCAPS2_CUBEMAP_POSITIVEZ;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("POSZ"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.dwCubemapFlags & DDSCAPS2_CUBEMAP_NEGATIVEZ)
			{
				dwFlags ^= DDSCAPS2_CUBEMAP_NEGATIVEZ;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("NEGZ"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsh.dwCubemapFlags & DDSCAPS2_VOLUME)
			{
				dwFlags ^= DDSCAPS2_VOLUME;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("VOLUME"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (dwFlags != 0 && dwFlags != ddsh.dwCubemapFlags)
			{
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_sntprintf(szFlags, _countof(szFlags), TEXT("%08X"), dwFlags);
				_tcsncat(szOutput, szFlags, _countof(szOutput) - _tcslen(szOutput) - 1);
			}

			if (szOutput[0] != chNil)
			{
				OutputText(hwndCtl, TEXT(" ("));
				OutputText(hwndCtl, szOutput);
				OutputText(hwndCtl, TEXT(")"));
			}

			OutputText(hwndCtl, szCRLF);
		}
	}

	// DX10 or XBOX header present?
	if ((ddsh.ddspf.dwFlags & DDPF_FOURCC) &&
		((ddsh.ddspf.dwFourCC == MAKEFOURCC('D', 'X', '1', '0')) ||
		(ddsh.ddspf.dwFourCC == MAKEFOURCC('X', 'B', 'O', 'X'))))
	{
		BOOL bIsXbox = (ddsh.ddspf.dwFourCC == MAKEFOURCC('X', 'B', 'O', 'X'));

		// Read header
		DDS_HEADER_XBOX ddsexth;	// Enhanced version of DDS_HEADER_DXT10
		ZeroMemory(&ddsexth, sizeof(ddsexth));
		if (!ReadData(hFile, (LPVOID)&ddsexth,
			bIsXbox ? sizeof(DDS_HEADER_XBOX) : sizeof(DDS_HEADER_DXT10)))
			return FALSE;

		// Output data
		OutputText(hwndCtl, g_szSep1);

		OutputText(hwndCtl, TEXT("Pixel Format:\t"));
		if (ddsexth.dxgiFormat != 0)
		{
			OutputTextFmt(hwndCtl, szOutput, TEXT("%u"), ddsexth.dxgiFormat);

			switch (ddsexth.dxgiFormat)
			{
				case 10:
					OutputText(hwndCtl, TEXT(" (R16G16B16A16 FLOAT)"));
					break;
				case 24:
					OutputText(hwndCtl, TEXT(" (R10G10B10A2 UNORM)"));
					break;
				case 26:
					OutputText(hwndCtl, TEXT(" (R11G11B10 FLOAT)"));
					break;
				case 66:
					OutputText(hwndCtl, TEXT(" (R1 UNORM)"));
					break;
				case 67:
					OutputText(hwndCtl, TEXT(" (R9G9B9E5 SHAREDEXP)"));
					break;
				case 70:
					OutputText(hwndCtl, TEXT(" (BC1 TYPELESS)"));
					break;
				case 71:
					OutputText(hwndCtl, TEXT(" (BC1 UNORM)"));
					break;
				case 72:
					OutputText(hwndCtl, TEXT(" (BC1 UNORM SRGB)"));
					break;
				case 73:
					OutputText(hwndCtl, TEXT(" (BC2 TYPELESS)"));
					break;
				case 74:
					OutputText(hwndCtl, TEXT(" (BC2 UNORM)"));
					break;
				case 75:
					OutputText(hwndCtl, TEXT(" (BC2 UNORM SRGB)"));
					break;
				case 76:
					OutputText(hwndCtl, TEXT(" (BC3 TYPELESS)"));
					break;
				case 77:
					OutputText(hwndCtl, TEXT(" (BC3 UNORM)"));
					break;
				case 78:
					OutputText(hwndCtl, TEXT(" (BC3 UNORM SRGB)"));
					break;
				case 79:
					OutputText(hwndCtl, TEXT(" (BC4 TYPELESS)"));
					break;
				case 80:
					OutputText(hwndCtl, TEXT(" (BC4 UNORM)"));
					break;
				case 81:
					OutputText(hwndCtl, TEXT(" (BC4 SNORM)"));
					break;
				case 82:
					OutputText(hwndCtl, TEXT(" (BC5 TYPELESS)"));
					break;
				case 83:
					OutputText(hwndCtl, TEXT(" (BC5 UNORM)"));
					break;
				case 84:
					OutputText(hwndCtl, TEXT(" (BC5 SNORM)"));
					break;
				case 89:
					OutputText(hwndCtl, TEXT(" (R10G10B10 XR BIAS A2 UNORM)"));
					break;
				case 94:
					OutputText(hwndCtl, TEXT(" (BC6H TYPELESS)"));
					break;
				case 95:
					OutputText(hwndCtl, TEXT(" (BC6H UF16)"));
					break;
				case 96:
					OutputText(hwndCtl, TEXT(" (BC6H SF16)"));
					break;
				case 97:
					OutputText(hwndCtl, TEXT(" (BC7 TYPELESS)"));
					break;
				case 98:
					OutputText(hwndCtl, TEXT(" (BC7 UNORM)"));
					break;
				case 99:
					OutputText(hwndCtl, TEXT(" (BC7 UNORM SRGB)"));
					break;
			}
		}
		else
			OutputText(hwndCtl, szUnknown);
		OutputText(hwndCtl, szCRLF);

		OutputText(hwndCtl, TEXT("Dimension:\t"));
		switch (ddsexth.resourceDimension)
		{
			case DDS_DIMENSION_UNKNOWN:
				OutputText(hwndCtl, szUnknown);
				break;
			case DDS_DIMENSION_BUFFER:
				OutputText(hwndCtl, TEXT("Buffer"));
				break;
			case DDS_DIMENSION_TEXTURE1D:
				OutputText(hwndCtl, TEXT("Texture1D"));
				break;
			case DDS_DIMENSION_TEXTURE2D:
				OutputText(hwndCtl, TEXT("Texture2D"));
				break;
			case DDS_DIMENSION_TEXTURE3D:
				OutputText(hwndCtl, TEXT("Texture3D"));
				break;
			default:
				OutputTextFmt(hwndCtl, szOutput, TEXT("%u"), ddsexth.resourceDimension);
		}
		OutputText(hwndCtl, szCRLF);

		OutputTextFmt(hwndCtl, szOutput, TEXT("Resource Flags:\t%08X"), ddsexth.miscFlag);
		if (ddsexth.miscFlag != 0)
		{
			szOutput[0] = chNil;
			dwFlags = ddsexth.miscFlag;
			if (ddsexth.miscFlag & DDS_RESOURCE_MISC_GENERATE_MIPS)
			{
				dwFlags ^= DDS_RESOURCE_MISC_GENERATE_MIPS;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("GENERATEMIPS"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsexth.miscFlag & DDS_RESOURCE_MISC_SHARED)
			{
				dwFlags ^= DDS_RESOURCE_MISC_SHARED;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("SHARED"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (ddsexth.miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE)
			{
				dwFlags ^= DDS_RESOURCE_MISC_TEXTURECUBE;
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_tcsncat(szOutput, TEXT("TEXTURECUBE"), _countof(szOutput) - _tcslen(szOutput) - 1);
			}
			if (dwFlags != 0 && dwFlags != ddsexth.miscFlag)
			{
				if (szOutput[0] != chNil)
					_tcsncat(szOutput, szOR, _countof(szOutput) - _tcslen(szOutput) - 1);
				_sntprintf(szFlags, _countof(szFlags), TEXT("%08X"), dwFlags);
				_tcsncat(szOutput, szFlags, _countof(szOutput) - _tcslen(szOutput) - 1);
			}

			if (szOutput[0] != chNil)
			{
				OutputText(hwndCtl, TEXT(" ("));
				OutputText(hwndCtl, szOutput);
				OutputText(hwndCtl, TEXT(")"));
			}
		}
		OutputText(hwndCtl, szCRLF);

		OutputTextFmt(hwndCtl, szOutput, TEXT("Array Size:\t%u\r\n"), ddsexth.arraySize);

		OutputText(hwndCtl, TEXT("Alpha Mode:\t"));
		DWORD dwAlphaMode = ddsexth.miscFlags2 & DDS_MISC_FLAGS2_ALPHA_MODE_MASK;
		switch (dwAlphaMode)
		{
			case DDS_ALPHA_MODE_UNKNOWN:
				OutputText(hwndCtl, szUnknown);
				break;
			case DDS_ALPHA_MODE_STRAIGHT:
				OutputText(hwndCtl, TEXT("Straight"));
				break;
			case DDS_ALPHA_MODE_PREMULTIPLIED:
				OutputText(hwndCtl, TEXT("Premultiplied"));
				break;
			case DDS_ALPHA_MODE_OPAQUE:
				OutputText(hwndCtl, TEXT("Opaque"));
				break;
			case DDS_ALPHA_MODE_CUSTOM:
				OutputText(hwndCtl, TEXT("Custom"));
				break;
			default:
				OutputTextFmt(hwndCtl, szOutput, TEXT("%u"), dwAlphaMode);
		}
		OutputText(hwndCtl, szCRLF);

		if (bIsXbox)
		{
			OutputTextFmt(hwndCtl, szOutput, TEXT("Tile Mode:\t%u\r\n"), ddsexth.tileMode);
			OutputTextFmt(hwndCtl, szOutput, TEXT("Base Alignment:\t%u\r\n"), ddsexth.baseAlignment);
			OutputTextFmt(hwndCtl, szOutput, TEXT("Data Size:\t%u\r\n"), ddsexth.dataSize);
			OutputTextFmt(hwndCtl, szOutput, TEXT("XDK Version:\t%u\r\n"), ddsexth.xdkVer);
		}
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
