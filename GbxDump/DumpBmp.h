////////////////////////////////////////////////////////////////////////////////////////////////
// DumpBmp.h - Copyright (c) 2023 by Electron.
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

BOOL DumpBitmap(HWND hwndCtl, HANDLE hFile, DWORD dwFileSize);

////////////////////////////////////////////////////////////////////////////////////////////////
// Constants and structures

#pragma pack(push,1)

////////////////////////////////////////////////////////////////////////////////////////////////

#define BI_ALPHABITFIELDS   6L          // Windows CE
#define BI_CMYK             11L         // Windows Metafile
#define BI_CMYKRLE8         12L         // Windows Metafile
#define BI_CMYKRLE4         13L         // Windows Metafile

#ifndef LCS_DEVICE_RGB
#define LCS_DEVICE_RGB                  0x00000001L
#endif
#ifndef LCS_DEVICE_CMYK
#define LCS_DEVICE_CMYK                 0x00000002L
#endif

#ifndef LCS_GM_ABS_COLORIMETRIC
#define LCS_GM_ABS_COLORIMETRIC         0x00000008L
#endif

////////////////////////////////////////////////////////////////////////////////////////////////
// OS/2 Presentation Manager Bitmap type declarations

#define BFT_BITMAPARRAY     0x4142 // 'BA'
#define BFT_BMAP            0x4d42 // 'BM'

#define BCA_UNCOMP          0L
#define BCA_RLE8            1L
#define BCA_RLE4            2L
#define BCA_HUFFMAN1D       3L
#define BCA_RLE24           4L

#define BRU_METRIC          0L

#define BRA_BOTTOMUP        0L

#define BRH_NOTHALFTONED    0L
#define BRH_ERRORDIFFUSION  1L
#define BRH_PANDA           2L
#define BRH_SUPERCIRCLE     3L

#define BCE_PALETTE         (-1L)
#define BCE_RGB             0L

typedef struct
{
    USHORT usType;
    ULONG  cbSize;
    ULONG  offNext;
    USHORT cxDisplay;
    USHORT cyDisplay;
    BITMAPFILEHEADER bfh;
} BITMAPARRAYFILEHEADER, FAR* LPBITMAPARRAYFILEHEADER, *PBITMAPARRAYFILEHEADER;

typedef struct
{
    ULONG  cbFix;
    ULONG  cx;
    ULONG  cy;
    USHORT cPlanes;
    USHORT cBitCount;
    ULONG  ulCompression;
    ULONG  cbImage;
    ULONG  cxResolution;
    ULONG  cyResolution;
    ULONG  cclrUsed;
    ULONG  cclrImportant;
    USHORT usUnits;
    USHORT usReserved;
    USHORT usRecording;
    USHORT usRendering;
    ULONG  cSize1;
    ULONG  cSize2;
    ULONG  ulColorEncoding;
    ULONG  ulIdentifier;
} BITMAPINFOHEADER2, FAR* LPBITMAPINFOHEADER2, *PBITMAPINFOHEADER2;

////////////////////////////////////////////////////////////////////////////////////////////////
// Adobe Photoshop

typedef struct
{
    DWORD  biSize;
    LONG   biWidth;
    LONG   biHeight;
    WORD   biPlanes;
    WORD   biBitCount;
    DWORD  biCompression;
    DWORD  biSizeImage;
    LONG   biXPelsPerMeter;
    LONG   biYPelsPerMeter;
    DWORD  biClrUsed;
    DWORD  biClrImportant;
    DWORD  biRedMask;
    DWORD  biGreenMask;
    DWORD  biBlueMask;
} BITMAPV2INFOHEADER, FAR* LPBITMAPV2INFOHEADER, *PBITMAPV2INFOHEADER;

typedef struct
{
    DWORD  biSize;
    LONG   biWidth;
    LONG   biHeight;
    WORD   biPlanes;
    WORD   biBitCount;
    DWORD  biCompression;
    DWORD  biSizeImage;
    LONG   biXPelsPerMeter;
    LONG   biYPelsPerMeter;
    DWORD  biClrUsed;
    DWORD  biClrImportant;
    DWORD  biRedMask;
    DWORD  biGreenMask;
    DWORD  biBlueMask;
    DWORD  biAlphaMask;
} BITMAPV3INFOHEADER, FAR* LPBITMAPV3INFOHEADER, *PBITMAPV3INFOHEADER;

////////////////////////////////////////////////////////////////////////////////////////////////

#pragma pack(pop)

////////////////////////////////////////////////////////////////////////////////////////////////
