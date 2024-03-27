////////////////////////////////////////////////////////////////////////////////////////////////
// DumpDds.h - Copyright (c) 2010-2024 by Electron.
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

// Displays file header information of a DirectDraw Surface file
BOOL DumpDDS(HWND hwndCtl, HANDLE hFile, DWORD dwFileSize);

////////////////////////////////////////////////////////////////////////////////////////////////
// Constants and structures from dds.h and ddraw.h

#pragma pack(push,1)

#define DDPF_ALPHAPIXELS            0x00000001
#define DDPF_ALPHA                  0x00000002
#define DDPF_FOURCC                 0x00000004
#define DDPF_PALETTEINDEXED4        0x00000008
#define DDPF_PALETTEINDEXEDTO8      0x00000010
#define DDPF_PALETTEINDEXED8        0x00000020
#define DDPF_RGB                    0x00000040
#define DDPF_COMPRESSED             0x00000080
#define DDPF_RGBTOYUV               0x00000100
#define DDPF_YUV                    0x00000200
#define DDPF_ZBUFFER                0x00000400
#define DDPF_PALETTEINDEXED1        0x00000800
#define DDPF_PALETTEINDEXED2        0x00001000
#define DDPF_ZPIXELS                0x00002000
#define DDPF_STENCILBUFFER          0x00004000
#define DDPF_ALPHAPREMULT           0x00008000
#define DDPF_LUMINANCE              0x00020000
#define DDPF_BUMPLUMINANCE          0x00040000
#define DDPF_BUMPDUDV               0x00080000
#define DDPF_SRGB                   0x40000000	// NVTT
#define DDPF_NORMAL                 0x80000000	// NVTT

#define DDSD_CAPS                   0x00000001
#define DDSD_HEIGHT                 0x00000002
#define DDSD_WIDTH                  0x00000004
#define DDSD_PITCH                  0x00000008
#define DDSD_PIXELFORMAT            0x00001000
#define DDSD_MIPMAPCOUNT            0x00020000
#define DDSD_LINEARSIZE             0x00080000
#define DDSD_DEPTH                  0x00800000

#define DDSCAPS_ALPHA               0x00000002
#define DDSCAPS_COMPLEX             0x00000008
#define DDSCAPS_TEXTURE             0x00001000
#define DDSCAPS_MIPMAP              0x00400000

#define DDSCAPS2_CUBEMAP            0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX  0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX  0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY  0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY  0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ  0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ  0x00008000
#define DDSCAPS2_VOLUME             0x00200000

////////////////////////////////////////////////////////////////////////////////////////////////

enum DDS_RESOURCE_DIMENSION
{
    DDS_DIMENSION_UNKNOWN	= 0,
    DDS_DIMENSION_BUFFER	= 1,
    DDS_DIMENSION_TEXTURE1D	= 2,
    DDS_DIMENSION_TEXTURE2D	= 3,
    DDS_DIMENSION_TEXTURE3D	= 4,
};

enum DDS_RESOURCE_MISC_FLAG
{
    DDS_RESOURCE_MISC_GENERATE_MIPS	= 0x1L,
    DDS_RESOURCE_MISC_SHARED		= 0x2L,
    DDS_RESOURCE_MISC_TEXTURECUBE	= 0x4L,
};

enum DDS_MISC_FLAGS2
{
    DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7L,
};

enum DDS_ALPHA_MODE
{
    DDS_ALPHA_MODE_UNKNOWN			= 0,
    DDS_ALPHA_MODE_STRAIGHT			= 1,
    DDS_ALPHA_MODE_PREMULTIPLIED	= 2,
    DDS_ALPHA_MODE_OPAQUE			= 3,
    DDS_ALPHA_MODE_CUSTOM			= 4,
};

////////////////////////////////////////////////////////////////////////////////////////////////

struct DDS_PIXELFORMAT
{
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwFourCC;
    DWORD dwRGBBitCount;
    DWORD dwRBitMask;
    DWORD dwGBitMask;
    DWORD dwBBitMask;
    DWORD dwABitMask;
};

struct DDS_HEADER
{
    DWORD dwSize;
    DWORD dwHeaderFlags;
    DWORD dwHeight;
    DWORD dwWidth;
    DWORD dwPitchOrLinearSize;
    DWORD dwDepth;
    DWORD dwMipMapCount;
    DWORD dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    DWORD dwCaps;
    DWORD dwCaps2;
	DWORD dwCaps3;
	DWORD dwCaps4;
	DWORD dwReserved2;
};

struct DDS_HEADER_DXT10
{
    DWORD dxgiFormat;
    DWORD resourceDimension;
    DWORD miscFlag;
    DWORD arraySize;
    DWORD miscFlags2;
};

struct DDS_HEADER_XBOX
{
    DWORD dxgiFormat;
    DWORD resourceDimension;
    DWORD miscFlag;
    DWORD arraySize;
    DWORD miscFlags2;
    DWORD tileMode;
    DWORD baseAlignment;
    DWORD dataSize;
    DWORD xdkVer;
};

#pragma pack(pop)

////////////////////////////////////////////////////////////////////////////////////////////////
