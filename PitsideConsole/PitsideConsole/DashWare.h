#include <Windows.h>
#include <vector>
#include <map>
#include "LapData.h"

using namespace std;
#pragma once

namespace DashWare
{
  HRESULT SaveToDashware(LPCTSTR lpszFilename, const vector<const ILap*>& lstLaps);
}