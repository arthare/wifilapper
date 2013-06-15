// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include "resource.h"


// TODO: reference additional headers your program requires here
#pragma warning(disable:4018) // signed/unsigned mismatch
#pragma warning(disable:4305) // double to float warning
#pragma warning(disable:4244) // conversion from double to flaot
#pragma warning(disable:4996) // stupid crap about swprintf having been modified
#pragma warning(disable:4355)
#pragma warning(disable:4800)