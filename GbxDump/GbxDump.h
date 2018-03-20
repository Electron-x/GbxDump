////////////////////////////////////////////////////////////////////////////////////////////////
// GbxDump.h - Copyright (c) 2010-2018 by Electron.
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

#pragma once

#include "resource.h"

#define WMU_FILEOPEN WM_APP + 42

#define OUTPUT_LEN  1024

#define UID_LENGTH  64
#define ENVI_LENGTH 64

#ifndef USER_DEFAULT_SCREEN_DPI
#define USER_DEFAULT_SCREEN_DPI 96
#endif

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

////////////////////////////////////////////////////////////////////////////////////////////////

const TCHAR g_szSep0[] = TEXT("........................................................................\r\n");
const TCHAR g_szSep1[] = TEXT("------------------------------------------------------------------------\r\n");
const TCHAR g_szSep2[] = TEXT("========================================================================\r\n");
const TCHAR g_szSep3[] = TEXT("\r\n-[%08X]-------------------------------------------------------------\r\n");

////////////////////////////////////////////////////////////////////////////////////////////////

extern const TCHAR g_szTitle[];
extern HINSTANCE g_hInstance;
extern HANDLE g_hDibThumb;
extern BOOL g_bGerUI;

////////////////////////////////////////////////////////////////////////////////////////////////
