// Hyperlinks.h
//
// Copyright 2002 Neal Stublen
// All rights reserved.
//
// http://www.awesoftware.com
//
#pragma once
#include <Windows.h>
#include "artui.h"
#include "ArtTools.h"
#include "resource.h"

BOOL ConvertStaticToHyperlink(HWND hwndCtl);
BOOL ConvertStaticToHyperlink(HWND hwndParent, UINT uiCtlId);
