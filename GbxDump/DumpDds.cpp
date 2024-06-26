////////////////////////////////////////////////////////////////////////////////////////////////
// DumpDds.cpp - Copyright (c) 2010-2024 by Electron.
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
#include "ImgFmt.h"
#include "Archive.h"
#include "DumpDds.h"

#define FLAGS_LEN 32

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module

// Decodes the DDS file and displays it as a thumbnail
BOOL DisplayDDS(HWND hwndEdit, HANDLE hFile, DWORD dwFileSize, BOOL bShowTextureDesc = FALSE);

////////////////////////////////////////////////////////////////////////////////////////////////
// String Constants

const TCHAR chNil = TEXT('\0');
const TCHAR szCRLF[] = TEXT("\r\n");

////////////////////////////////////////////////////////////////////////////////////////////////

static void PrintD3DFormat(HWND hwndEdit, DWORD dwFourCC)
{
	if (hwndEdit == NULL)
		return;

	switch (dwFourCC)
	{
		case 0:
			OutputText(hwndEdit, TEXT(" (Unknown)"));
			break;
		case 36:
			OutputText(hwndEdit, TEXT(" (A16B16G16R16)"));
			break;
		case 110:
			OutputText(hwndEdit, TEXT(" (Q16W16V16U16)"));
			break;
		case 111:
			OutputText(hwndEdit, TEXT(" (R16F)"));
			break;
		case 112:
			OutputText(hwndEdit, TEXT(" (G16R16F)"));
			break;
		case 113:
			OutputText(hwndEdit, TEXT(" (A16B16G16R16F)"));
			break;
		case 114:
			OutputText(hwndEdit, TEXT(" (R32F)"));
			break;
		case 115:
			OutputText(hwndEdit, TEXT(" (G32R32F)"));
			break;
		case 116:
			OutputText(hwndEdit, TEXT(" (A32B32G32R32F)"));
			break;
		case 117:
			OutputText(hwndEdit, TEXT(" (CxV8U8)"));
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

static void PrintDXGIFormat(HWND hwndEdit, DWORD dwDxgiFormat)
{
	if (hwndEdit == NULL)
		return;

	switch (dwDxgiFormat)
	{
		case 0:
			OutputText(hwndEdit, TEXT(" (Unknown)"));
			break;
		case 2:
			OutputText(hwndEdit, TEXT(" (R32G32B32A32 FLOAT)"));
			break;
		case 10:
			OutputText(hwndEdit, TEXT(" (R16G16B16A16 FLOAT)"));
			break;
		case 11:
			OutputText(hwndEdit, TEXT(" (R16G16B16A16 UNORM)"));
			break;
		case 13:
			OutputText(hwndEdit, TEXT(" (R16G16B16A16 SNORM)"));
			break;
		case 16:
			OutputText(hwndEdit, TEXT(" (R32G32 FLOAT)"));
			break;
		case 24:
			OutputText(hwndEdit, TEXT(" (R10G10B10A2 UNORM)"));
			break;
		case 26:
			OutputText(hwndEdit, TEXT(" (R11G11B10 FLOAT)"));
			break;
		case 28:
			OutputText(hwndEdit, TEXT(" (R8G8B8A8 UNORM)"));
			break;
		case 29:
			OutputText(hwndEdit, TEXT(" (R8G8B8A8 UNORM SRGB)"));
			break;
		case 31:
			OutputText(hwndEdit, TEXT(" (R8G8B8A8 SNORM)"));
			break;
		case 34:
			OutputText(hwndEdit, TEXT(" (R16G16 FLOAT)"));
			break;
		case 35:
			OutputText(hwndEdit, TEXT(" (R16G16 UNORM)"));
			break;
		case 37:
			OutputText(hwndEdit, TEXT(" (R16G16 SNORM)"));
			break;
		case 41:
			OutputText(hwndEdit, TEXT(" (R32 FLOAT)"));
			break;
		case 49:
			OutputText(hwndEdit, TEXT(" (R8G8 UNORM)"));
			break;
		case 51:
			OutputText(hwndEdit, TEXT(" (R8G8 SNORM)"));
			break;
		case 54:
			OutputText(hwndEdit, TEXT(" (R16 FLOAT)"));
			break;
		case 56:
			OutputText(hwndEdit, TEXT(" (R16 UNORM)"));
			break;
		case 61:
			OutputText(hwndEdit, TEXT(" (R8 UNORM)"));
			break;
		case 65:
			OutputText(hwndEdit, TEXT(" (A8 UNORM)"));
			break;
		case 66:
			OutputText(hwndEdit, TEXT(" (R1 UNORM)"));
			break;
		case 67:
			OutputText(hwndEdit, TEXT(" (R9G9B9E5 SHAREDEXP)"));
			break;
		case 68:
			OutputText(hwndEdit, TEXT(" (R8G8 B8G8 UNORM)"));
			break;
		case 69:
			OutputText(hwndEdit, TEXT(" (G8R8 G8B8 UNORM)"));
			break;
		case 70:
			OutputText(hwndEdit, TEXT(" (BC1 TYPELESS)"));
			break;
		case 71:
			OutputText(hwndEdit, TEXT(" (BC1 UNORM)"));
			break;
		case 72:
			OutputText(hwndEdit, TEXT(" (BC1 UNORM SRGB)"));
			break;
		case 73:
			OutputText(hwndEdit, TEXT(" (BC2 TYPELESS)"));
			break;
		case 74:
			OutputText(hwndEdit, TEXT(" (BC2 UNORM)"));
			break;
		case 75:
			OutputText(hwndEdit, TEXT(" (BC2 UNORM SRGB)"));
			break;
		case 76:
			OutputText(hwndEdit, TEXT(" (BC3 TYPELESS)"));
			break;
		case 77:
			OutputText(hwndEdit, TEXT(" (BC3 UNORM)"));
			break;
		case 78:
			OutputText(hwndEdit, TEXT(" (BC3 UNORM SRGB)"));
			break;
		case 79:
			OutputText(hwndEdit, TEXT(" (BC4 TYPELESS)"));
			break;
		case 80:
			OutputText(hwndEdit, TEXT(" (BC4 UNORM)"));
			break;
		case 81:
			OutputText(hwndEdit, TEXT(" (BC4 SNORM)"));
			break;
		case 82:
			OutputText(hwndEdit, TEXT(" (BC5 TYPELESS)"));
			break;
		case 83:
			OutputText(hwndEdit, TEXT(" (BC5 UNORM)"));
			break;
		case 84:
			OutputText(hwndEdit, TEXT(" (BC5 SNORM)"));
			break;
		case 85:
			OutputText(hwndEdit, TEXT(" (B5G6R5 UNORM)"));
			break;
		case 86:
			OutputText(hwndEdit, TEXT(" (B5G5R5A1 UNORM)"));
			break;
		case 87:
			OutputText(hwndEdit, TEXT(" (B8G8R8A8 UNORM)"));
			break;
		case 88:
			OutputText(hwndEdit, TEXT(" (B8G8R8X8 UNORM)"));
			break;
		case 89:
			OutputText(hwndEdit, TEXT(" (R10G10B10 XR BIAS A2 UNORM)"));
			break;
		case 94:
			OutputText(hwndEdit, TEXT(" (BC6H TYPELESS)"));
			break;
		case 95:
			OutputText(hwndEdit, TEXT(" (BC6H UF16)"));
			break;
		case 96:
			OutputText(hwndEdit, TEXT(" (BC6H SF16)"));
			break;
		case 97:
			OutputText(hwndEdit, TEXT(" (BC7 TYPELESS)"));
			break;
		case 98:
			OutputText(hwndEdit, TEXT(" (BC7 UNORM)"));
			break;
		case 99:
			OutputText(hwndEdit, TEXT(" (BC7 UNORM SRGB)"));
			break;
		case 115:
			OutputText(hwndEdit, TEXT(" (B4G4R4A4 UNORM)"));
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

static void PrintFourCC(HWND hwndEdit, DWORD dwFourCC)
{
	BYTE achFourCC[4];
	TCHAR szOutput[OUTPUT_LEN];

	if (hwndEdit == NULL)
		return;

	CopyMemory(achFourCC, &dwFourCC, 4);

	if (!isprint(achFourCC[0]) || !isprint(achFourCC[1]) ||
		!isprint(achFourCC[2]) || !isprint(achFourCC[3]))
	{
		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("%u"), dwFourCC);
		PrintD3DFormat(hwndEdit, dwFourCC);
	}
	else
	{
		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("%hc%hc%hc%hc"),
			achFourCC[0], achFourCC[1], achFourCC[2], achFourCC[3]);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
// DumpDDS is called by DumpFile from GbxDump.cpp

BOOL DumpDDS(HWND hwndEdit, HANDLE hFile, DWORD dwFileSize)
{
	DWORD dwFlags;
	TCHAR szFlags[FLAGS_LEN];
	TCHAR szOutput[OUTPUT_LEN];

	if (hwndEdit == NULL || hFile == NULL)
		return FALSE;

	// Skip the file signature (already checked in DumpFile())
	if (!FileSeekBegin(hFile, 4))
		return FALSE;

	// Read header
	DDS_HEADER ddsh;
	ZeroMemory(&ddsh, sizeof(ddsh));
	if (!ReadData(hFile, (LPVOID)&ddsh, sizeof(ddsh)))
		return FALSE;

	// Output data
	OutputText(hwndEdit, g_szSep1);

	OutputText(hwndEdit, TEXT("Header Size:\t"));
	FormatByteSize(ddsh.dwSize, szOutput, _countof(szOutput));
	OutputText(hwndEdit, szOutput);
	if (ddsh.dwSize != 124) OutputText(hwndEdit, TEXT("!"));
	OutputText(hwndEdit, szCRLF);

	OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Header Flags:\t%08X"), ddsh.dwHeaderFlags);
	if (ddsh.dwHeaderFlags != 0)
	{
		szOutput[0] = chNil;
		dwFlags = ddsh.dwHeaderFlags;
		if (ddsh.dwHeaderFlags & DDSD_CAPS)
		{
			dwFlags ^= DDSD_CAPS;
			AppendFlagName(szOutput, _countof(szOutput), TEXT("CAPS"));
		}
		if (ddsh.dwHeaderFlags & DDSD_HEIGHT)
		{
			dwFlags ^= DDSD_HEIGHT;
			AppendFlagName(szOutput, _countof(szOutput), TEXT("HEIGHT"));
		}
		if (ddsh.dwHeaderFlags & DDSD_WIDTH)
		{
			dwFlags ^= DDSD_WIDTH;
			AppendFlagName(szOutput, _countof(szOutput), TEXT("WIDTH"));
		}
		if (ddsh.dwHeaderFlags & DDSD_PITCH)
		{
			dwFlags ^= DDSD_PITCH;
			AppendFlagName(szOutput, _countof(szOutput), TEXT("PITCH"));
		}
		if (ddsh.dwHeaderFlags & DDSD_PIXELFORMAT)
		{
			dwFlags ^= DDSD_PIXELFORMAT;
			AppendFlagName(szOutput, _countof(szOutput), TEXT("PIXELFORMAT"));
		}
		if (ddsh.dwHeaderFlags & DDSD_MIPMAPCOUNT)
		{
			dwFlags ^= DDSD_MIPMAPCOUNT;
			AppendFlagName(szOutput, _countof(szOutput), TEXT("MIPMAPCOUNT"));
		}
		if (ddsh.dwHeaderFlags & DDSD_LINEARSIZE)
		{
			dwFlags ^= DDSD_LINEARSIZE;
			AppendFlagName(szOutput, _countof(szOutput), TEXT("LINEARSIZE"));
		}
		if (ddsh.dwHeaderFlags & DDSD_DEPTH)
		{
			dwFlags ^= DDSD_DEPTH;
			AppendFlagName(szOutput, _countof(szOutput), TEXT("DEPTH"));
		}
		if (dwFlags != 0 && dwFlags != ddsh.dwHeaderFlags)
		{
			_sntprintf(szFlags, _countof(szFlags), TEXT("%08X"), dwFlags);
			szFlags[FLAGS_LEN - 1] = chNil;
			AppendFlagName(szOutput, _countof(szOutput), szFlags);
		}

		if (szOutput[0] != chNil)
		{
			OutputText(hwndEdit, TEXT(" ("));
			OutputText(hwndEdit, szOutput);
			OutputText(hwndEdit, TEXT(")"));
		}
	}
	OutputText(hwndEdit, szCRLF);

	if ((ddsh.dwHeaderFlags & DDSD_WIDTH) || ddsh.dwWidth != 0)
		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Surface Width:\t%u pixels\r\n"), ddsh.dwWidth);

	if ((ddsh.dwHeaderFlags & DDSD_HEIGHT) || ddsh.dwHeight != 0)
		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Surface Height:\t%u pixels\r\n"), ddsh.dwHeight);

	if ((ddsh.dwHeaderFlags & DDSD_PITCH) || (ddsh.dwHeaderFlags & DDSD_LINEARSIZE))
	{
		if (ddsh.dwHeaderFlags & DDSD_PITCH)
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Pitch:\t\t%u bytes\r\n"), ddsh.dwPitchOrLinearSize);

		if (ddsh.dwHeaderFlags & DDSD_LINEARSIZE)
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Linear Size:\t%u bytes\r\n"), ddsh.dwPitchOrLinearSize);
	}
	else if (ddsh.dwPitchOrLinearSize != 0)
		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Pitch Or Size:\t%u bytes\r\n"), ddsh.dwPitchOrLinearSize);

	if ((ddsh.dwHeaderFlags & DDSD_DEPTH) || ddsh.dwDepth != 0)
		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Depth:\t\t%u pixels\r\n"), ddsh.dwDepth);

	if ((ddsh.dwHeaderFlags & DDSD_MIPMAPCOUNT) || ddsh.dwMipMapCount != 0)
		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Mipmap Levels:\t%u\r\n"), ddsh.dwMipMapCount);

	if (ddsh.dwReserved1[0] == MAKEFOURCC('G', 'I', 'M', 'P') &&
		ddsh.dwReserved1[1] == MAKEFOURCC('-', 'D', 'D', 'S'))
	{
		UINT uMajor = (ddsh.dwReserved1[2] >> 16) & 0xFF;
		UINT uMinor = (ddsh.dwReserved1[2] >> 8) & 0xFF;
		UINT uRevision = ddsh.dwReserved1[2] & 0xFF;

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("GIMP Plug-in:\t%u.%u.%u\r\n"), uMajor, uMinor, uRevision);

		if (ddsh.dwReserved1[3] != 0)
		{
			OutputText(hwndEdit, TEXT("Extra FourCC:\t"));
			PrintFourCC(hwndEdit, ddsh.dwReserved1[3]);
			OutputText(hwndEdit, szCRLF);
		}
	}

	if (ddsh.dwReserved1[9] == MAKEFOURCC('N', 'V', 'T', 'T'))
	{
		int nMajor = (ddsh.dwReserved1[10] >> 16) & 0xFF;
		int nMinor = (ddsh.dwReserved1[10] >> 8) & 0xFF;
		int nRevision = ddsh.dwReserved1[10] & 0xFF;

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("NVTT Version:\t%d.%d.%d\r\n"), nMajor, nMinor, nRevision);
	}

	if (ddsh.dwReserved1[7] == MAKEFOURCC('U', 'V', 'E', 'R'))
		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("User Version:\t%d\r\n"), ddsh.dwReserved1[8]);

	if ((ddsh.dwHeaderFlags & DDSD_PIXELFORMAT) || ddsh.ddspf.dwSize > 0)
	{
		OutputText(hwndEdit, TEXT("PixelFmt Size:\t"));
		FormatByteSize(ddsh.ddspf.dwSize, szOutput, _countof(szOutput));
		OutputText(hwndEdit, szOutput);
		if (ddsh.ddspf.dwSize != 32) OutputText(hwndEdit, TEXT("!"));
		OutputText(hwndEdit, szCRLF);

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("PixelFmt Flags:\t%08X"), ddsh.ddspf.dwFlags);
		if (ddsh.ddspf.dwFlags != 0)
		{
			szOutput[0] = chNil;
			dwFlags = ddsh.ddspf.dwFlags;
			if (ddsh.ddspf.dwFlags & DDPF_FOURCC)
			{
				dwFlags ^= DDPF_FOURCC;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("FOURCC"));
			}
			if (ddsh.ddspf.dwFlags & DDPF_RGB)
			{
				dwFlags ^= DDPF_RGB;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("RGB"));
			}
			if (ddsh.ddspf.dwFlags & DDPF_YUV)
			{
				dwFlags ^= DDPF_YUV;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("YUV"));
			}
			if (ddsh.ddspf.dwFlags & DDPF_LUMINANCE)
			{
				dwFlags ^= DDPF_LUMINANCE;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("LUMINANCE"));
			}
			if (ddsh.ddspf.dwFlags & DDPF_PALETTEINDEXED8)
			{
				dwFlags ^= DDPF_PALETTEINDEXED8;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("PAL8"));
			}
			if (ddsh.ddspf.dwFlags & DDPF_ALPHAPIXELS)
			{
				dwFlags ^= DDPF_ALPHAPIXELS;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("ALPHAPIXELS"));
			}
			if (ddsh.ddspf.dwFlags & DDPF_ALPHA)
			{
				dwFlags ^= DDPF_ALPHA;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("ALPHA"));
			}
			if (ddsh.ddspf.dwFlags & DDPF_ALPHAPREMULT)
			{
				dwFlags ^= DDPF_ALPHAPREMULT;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("ALPHAPREMULT"));
			}
			if (ddsh.ddspf.dwFlags & DDPF_BUMPLUMINANCE)
			{
				dwFlags ^= DDPF_BUMPLUMINANCE;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("BUMPLUMINANCE"));
			}
			if (ddsh.ddspf.dwFlags & DDPF_BUMPDUDV)
			{
				dwFlags ^= DDPF_BUMPDUDV;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("BUMPDUDV"));
			}
			if (ddsh.ddspf.dwFlags & DDPF_NORMAL)
			{
				dwFlags ^= DDPF_NORMAL;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("NORMAL"));
			}
			if (ddsh.ddspf.dwFlags & DDPF_SRGB)
			{
				dwFlags ^= DDPF_SRGB;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("SRGB"));
			}
			if (dwFlags != 0 && dwFlags != ddsh.ddspf.dwFlags)
			{
				_sntprintf(szFlags, _countof(szFlags), TEXT("%08X"), dwFlags);
				szFlags[FLAGS_LEN - 1] = chNil;
				AppendFlagName(szOutput, _countof(szOutput), szFlags);
			}

			if (szOutput[0] != chNil)
			{
				OutputText(hwndEdit, TEXT(" ("));
				OutputText(hwndEdit, szOutput);
				OutputText(hwndEdit, TEXT(")"));
			}
		}
		OutputText(hwndEdit, szCRLF);

		if (ddsh.ddspf.dwFlags & DDPF_FOURCC)
		{
			OutputText(hwndEdit, TEXT("FourCC Tag:\t"));
			PrintFourCC(hwndEdit, ddsh.ddspf.dwFourCC);
			OutputText(hwndEdit, szCRLF);
		}

		if (ddsh.ddspf.dwRGBBitCount > 0)
		{
			BYTE achFourCC[4];
			CopyMemory(achFourCC, &ddsh.ddspf.dwRGBBitCount, 4);

			if ((ddsh.ddspf.dwFlags & DDPF_FOURCC) &&
				isprint(achFourCC[0]) && isprint(achFourCC[1]) &&
				isprint(achFourCC[2]) && isprint(achFourCC[3]))
			{
				OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Swizzle:\t%hc%hc%hc%hc\r\n"),
					achFourCC[0], achFourCC[1], achFourCC[2], achFourCC[3]);
			}
			else
			{
				OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Bit Count:\t%u bits\r\n"), ddsh.ddspf.dwRGBBitCount);
				OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Red Mask:\t%08X\r\n"), ddsh.ddspf.dwRBitMask);
				OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Green Mask:\t%08X\r\n"), ddsh.ddspf.dwGBitMask);
				OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Blue Mask:\t%08X\r\n"), ddsh.ddspf.dwBBitMask);
				OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Alpha Mask:\t%08X\r\n"), ddsh.ddspf.dwABitMask);
			}
		}
	}

	if ((ddsh.dwHeaderFlags & DDSD_CAPS) ||
		ddsh.dwCaps != 0 || ddsh.dwCaps2 != 0 || ddsh.dwCaps3 != 0 || ddsh.dwCaps4 != 0)
	{
		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Surface Flags:\t%08X"), ddsh.dwCaps);
		if (ddsh.dwCaps != 0)
		{
			szOutput[0] = chNil;
			dwFlags = ddsh.dwCaps;
			if (ddsh.dwCaps & DDSCAPS_TEXTURE)
			{
				dwFlags ^= DDSCAPS_TEXTURE;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("TEXTURE"));
			}
			if (ddsh.dwCaps & DDSCAPS_MIPMAP)
			{
				dwFlags ^= DDSCAPS_MIPMAP;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("MIPMAP"));
			}
			if (ddsh.dwCaps & DDSCAPS_COMPLEX)
			{
				dwFlags ^= DDSCAPS_COMPLEX;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("COMPLEX"));
			}
			if (ddsh.dwCaps & DDSCAPS_ALPHA)
			{
				dwFlags ^= DDSCAPS_ALPHA;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("ALPHA"));
			}
			if (dwFlags != 0 && dwFlags != ddsh.dwCaps)
			{
				_sntprintf(szFlags, _countof(szFlags), TEXT("%08X"), dwFlags);
				szFlags[FLAGS_LEN - 1] = chNil;
				AppendFlagName(szOutput, _countof(szOutput), szFlags);
			}

			if (szOutput[0] != chNil)
			{
				OutputText(hwndEdit, TEXT(" ("));
				OutputText(hwndEdit, szOutput);
				OutputText(hwndEdit, TEXT(")"));
			}
		}
		OutputText(hwndEdit, szCRLF);

		if (ddsh.dwCaps2 != 0)
		{
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Cubemap Flags:\t%08X"), ddsh.dwCaps2);

			szOutput[0] = chNil;
			dwFlags = ddsh.dwCaps2;
			if (ddsh.dwCaps2 & DDSCAPS2_CUBEMAP)
			{
				dwFlags ^= DDSCAPS2_CUBEMAP;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("CUBEMAP"));
			}
			if (ddsh.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEX)
			{
				dwFlags ^= DDSCAPS2_CUBEMAP_POSITIVEX;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("POSX"));
			}
			if (ddsh.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEX)
			{
				dwFlags ^= DDSCAPS2_CUBEMAP_NEGATIVEX;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("NEGX"));
			}
			if (ddsh.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEY)
			{
				dwFlags ^= DDSCAPS2_CUBEMAP_POSITIVEY;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("POSY"));
			}
			if (ddsh.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEY)
			{
				dwFlags ^= DDSCAPS2_CUBEMAP_NEGATIVEY;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("NEGY"));
			}
			if (ddsh.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEZ)
			{
				dwFlags ^= DDSCAPS2_CUBEMAP_POSITIVEZ;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("POSZ"));
			}
			if (ddsh.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ)
			{
				dwFlags ^= DDSCAPS2_CUBEMAP_NEGATIVEZ;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("NEGZ"));
			}
			if (ddsh.dwCaps2 & DDSCAPS2_VOLUME)
			{
				dwFlags ^= DDSCAPS2_VOLUME;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("VOLUME"));
			}
			if (dwFlags != 0 && dwFlags != ddsh.dwCaps2)
			{
				_sntprintf(szFlags, _countof(szFlags), TEXT("%08X"), dwFlags);
				szFlags[FLAGS_LEN - 1] = chNil;
				AppendFlagName(szOutput, _countof(szOutput), szFlags);
			}

			if (szOutput[0] != chNil)
			{
				OutputText(hwndEdit, TEXT(" ("));
				OutputText(hwndEdit, szOutput);
				OutputText(hwndEdit, TEXT(")"));
			}

			OutputText(hwndEdit, szCRLF);
		}

		if (ddsh.dwCaps3 != 0)
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Caps3 Flags:\t%08X\r\n"), ddsh.dwCaps3);

		if (ddsh.dwCaps4 != 0)
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Caps4 Flags:\t%08X\r\n"), ddsh.dwCaps4);
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
		if (!ReadData(hFile, (LPVOID)&ddsexth, bIsXbox ? sizeof(DDS_HEADER_XBOX) : sizeof(DDS_HEADER_DXT10)))
			return FALSE;

		// Output data
		OutputText(hwndEdit, g_szSep1);

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Pixel Format:\t%u"), ddsexth.dxgiFormat);
		PrintDXGIFormat(hwndEdit, ddsexth.dxgiFormat);
		OutputText(hwndEdit, szCRLF);

		OutputText(hwndEdit, TEXT("Dimension:\t"));
		switch (ddsexth.resourceDimension)
		{
			case DDS_DIMENSION_UNKNOWN:
				OutputText(hwndEdit, TEXT("Unknown"));
				break;
			case DDS_DIMENSION_BUFFER:
				OutputText(hwndEdit, TEXT("Buffer"));
				break;
			case DDS_DIMENSION_TEXTURE1D:
				OutputText(hwndEdit, ddsexth.arraySize > 1 ? TEXT("1D Array") : TEXT("1D"));
				break;
			case DDS_DIMENSION_TEXTURE2D:
				if (ddsexth.miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE)
					OutputText(hwndEdit, ddsexth.arraySize > 1 ? TEXT("Cube Array") : TEXT("Cube"));
				else
					OutputText(hwndEdit, ddsexth.arraySize > 1 ? TEXT("2D Array") : TEXT("2D"));
				break;
			case DDS_DIMENSION_TEXTURE3D:
				OutputText(hwndEdit, TEXT("3D"));
				break;
			default:
				OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("%u"), ddsexth.resourceDimension);
		}
		OutputText(hwndEdit, szCRLF);

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Resource Flags:\t%08X"), ddsexth.miscFlag);
		if (ddsexth.miscFlag != 0)
		{
			szOutput[0] = chNil;
			dwFlags = ddsexth.miscFlag;
			if (ddsexth.miscFlag & DDS_RESOURCE_MISC_GENERATE_MIPS)
			{
				dwFlags ^= DDS_RESOURCE_MISC_GENERATE_MIPS;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("GENERATEMIPS"));
			}
			if (ddsexth.miscFlag & DDS_RESOURCE_MISC_SHARED)
			{
				dwFlags ^= DDS_RESOURCE_MISC_SHARED;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("SHARED"));
			}
			if (ddsexth.miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE)
			{
				dwFlags ^= DDS_RESOURCE_MISC_TEXTURECUBE;
				AppendFlagName(szOutput, _countof(szOutput), TEXT("TEXTURECUBE"));
			}
			if (dwFlags != 0 && dwFlags != ddsexth.miscFlag)
			{
				_sntprintf(szFlags, _countof(szFlags), TEXT("%08X"), dwFlags);
				szFlags[FLAGS_LEN - 1] = chNil;
				AppendFlagName(szOutput, _countof(szOutput), szFlags);
			}

			if (szOutput[0] != chNil)
			{
				OutputText(hwndEdit, TEXT(" ("));
				OutputText(hwndEdit, szOutput);
				OutputText(hwndEdit, TEXT(")"));
			}
		}
		OutputText(hwndEdit, szCRLF);

		OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Array Size:\t%u\r\n"), ddsexth.arraySize);

		OutputText(hwndEdit, TEXT("Alpha Mode:\t"));
		DWORD dwAlphaMode = ddsexth.miscFlags2 & DDS_MISC_FLAGS2_ALPHA_MODE_MASK;
		switch (dwAlphaMode)
		{
			case DDS_ALPHA_MODE_UNKNOWN:
				OutputText(hwndEdit, TEXT("Unknown"));
				break;
			case DDS_ALPHA_MODE_STRAIGHT:
				OutputText(hwndEdit, TEXT("Straight"));
				break;
			case DDS_ALPHA_MODE_PREMULTIPLIED:
				OutputText(hwndEdit, TEXT("Premultiplied"));
				break;
			case DDS_ALPHA_MODE_OPAQUE:
				OutputText(hwndEdit, TEXT("Opaque"));
				break;
			case DDS_ALPHA_MODE_CUSTOM:
				OutputText(hwndEdit, TEXT("Custom"));
				break;
			default:
				OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("%u"), dwAlphaMode);
		}
		OutputText(hwndEdit, szCRLF);

		if (bIsXbox)
		{
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Tile Mode:\t%u\r\n"), ddsexth.tileMode);
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Base Alignment:\t%u\r\n"), ddsexth.baseAlignment);
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("Data Size:\t%u\r\n"), ddsexth.dataSize);
			OutputTextFmt(hwndEdit, szOutput, _countof(szOutput), TEXT("XDK Version:\t%u\r\n"), ddsexth.xdkVer);
		}
	}

	// Decode and display the DDS image
	DisplayDDS(hwndEdit, hFile, dwFileSize, FALSE);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DisplayDDS(HWND hwndEdit, HANDLE hFile, DWORD dwFileSize, BOOL bShowTextureDesc)
{
	if (hwndEdit == NULL || hFile == NULL || dwFileSize == 0)
		return FALSE;

	HWND hDlg = GetParent(hwndEdit);

	// Jump to the beginning of the file
	if (!FileSeekBegin(hFile, 0))
		return FALSE;

	LPVOID lpData = MyGlobalAllocPtr(GHND, dwFileSize);
	if (lpData == NULL)
		return FALSE;

	if (!ReadData(hFile, lpData, dwFileSize))
	{
		MyGlobalFreePtr(lpData);
		return FALSE;
	}

	if (bShowTextureDesc)
		OutputText(hwndEdit, g_szSep1);

	HANDLE hDib = NULL;
	HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	// Decode the DDS image
	__try { hDib = DdsToDib(lpData, dwFileSize, FALSE, bShowTextureDesc); }
	__except (EXCEPTION_EXECUTE_HANDLER) { hDib = NULL; }

	SetCursor(hOldCursor);

	if (hDib != NULL)
		ReplaceThumbnail(hDlg, hDib);
	else
		MarkAsUnsupported(hDlg);

	MyGlobalFreePtr(lpData);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
