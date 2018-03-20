// stdafx.h : Includedatei für Standardsystem-Includedateien
// oder häufig verwendete projektspezifische Includedateien,
// die nur in unregelmäßigen Abständen geändert werden.
//

#pragma once

// Ändern Sie folgende Definitionen für Plattformen, die älter als die unten angegebenen sind.
// In MSDN finden Sie die neuesten Informationen über die entsprechenden Werte für die unterschiedlichen Plattformen.
#ifndef WINVER				// Lassen Sie die Verwendung spezifischer Features von Windows NT oder später zu.
#define WINVER 0x0400		// Ändern Sie dies in den geeigneten Wert für andere Versionen von Windows.
#endif

#ifndef _WIN32_WINNT		// Lassen Sie die Verwendung spezifischer Features von Windows NT oder später zu.                   
#define _WIN32_WINNT 0x0400	// Ändern Sie dies in den geeigneten Wert für andere Versionen von Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Lassen Sie die Verwendung spezifischer Features von Windows 95 oder später zu.
#define _WIN32_WINDOWS 0x0400 // Ändern Sie dies in den geeigneten Wert für Windows Me oder höher.
#endif

#ifndef _WIN32_IE			// Lassen Sie die Verwendung spezifischer Features von IE 6.0 oder später zu.
#define _WIN32_IE 0x0600	// Ändern Sie dies in den geeigneten Wert für andere Versionen von IE.
#endif

#define WIN32_LEAN_AND_MEAN		// Selten verwendete Teile der Windows-Header nicht einbinden.

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#ifndef STRICT
#define STRICT
#endif

// Windows-Headerdateien:
#include <windows.h>
#include <windowsx.h>
#include <wininet.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shellapi.h>
#include <mmsystem.h>

// C RunTime-Headerdateien
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <mbstring.h>
#include <setjmp.h>

// TODO: Hier auf zusätzliche Header, die das Programm erfordert, verweisen.
#include "gbxdump.h"
#include "misc.h"
