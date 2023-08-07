// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 95 or later.
#define _WIN32_WINDOWS 0x0400 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 SP2 or later.
#define _WIN32_IE 0x0603	// Change this to the appropriate value to target other versions of IE.
#endif

#if defined (_MSC_VER) && (_MSC_VER <= 1500)
#else
#include <SDKDDKVer.h>
#endif

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#define _CRT_SECURE_NO_WARNINGS

#ifndef STRICT
#define STRICT
#endif

// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
#include <wininet.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <vfw.h>
#include <ole2.h>
#include <xmllite.h>
#include <Uxtheme.h>

// C RunTime Header Files
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <mbstring.h>
#include <setjmp.h>

// TODO: reference additional headers your program requires here
#include "gbxdump.h"
#include "misc.h"
