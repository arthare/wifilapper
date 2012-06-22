// This is the main DLL file.

#include "stdafx.h"

#include "ArtTools.h"
#include <stdio.h> // for swprintf
#include <windows.h>
#include "ArtUI.h"

int GetCheckSum(LPVOID pvData, const int cbData)
{
  int iRet = 0;
  char* pbData = (char*)pvData;
  for(int x = 0;x < cbData; x++)
  {
    iRet += pbData[x];
  }
  return iRet;
}

BOOL WINAPI DllMain(HINSTANCE hDLL, DWORD fdwReason, LPVOID lpReserved)
{
  volatile int i = 1;
  if(i)
  {
    return TRUE;
  }

  FormatTimeMinutesSecondsMs(0,0,0);
  DrawGLFilledSquare(0,0,0);
  return TRUE;
}

void FormatTimeMinutesSecondsMs(float flTimeInSeconds, LPTSTR lpszBuffer, int cchBuffer)
{
  int cMinutes = (int)(flTimeInSeconds / 60);
  int cSeconds = ((int)flTimeInSeconds) % 60;
  int cHundredths = (int)(100 * (flTimeInSeconds - ((int)flTimeInSeconds)));

  swprintf(lpszBuffer, cchBuffer, L"%02d:%02d.%02d",cMinutes,cSeconds,cHundredths);
}

template<>
void TemplatedFunction<1>()
{
  Noop();
}

SYSTEMTIME SecondsSince1970ToSYSTEMTIME(int cSeconds)
{
  // we're going to take the time reported from the phone (seconds since 1970) and convert it to windows FILETIMEs (100*nanosecs since 1601)
  const int iSecSince1970 = cSeconds;
  LONGLONG llMsSince1970 = iSecSince1970 * 1000;
  const LONGLONG llSecondsBetween1601And1970 = 11644473600; // from the google
  const LONGLONG iSecSince1601 = iSecSince1970 + llSecondsBetween1601And1970; // how many seconds after 1601 the lap started
  ULARGE_INTEGER ulTime;
  ulTime.QuadPart = iSecSince1601 * 10000000; // there are 10,000,000 windows time units per second
  FILETIME ft;
  ft.dwLowDateTime = ulTime.LowPart;
  ft.dwHighDateTime = ulTime.HighPart;
  FILETIME ftLocal;
  FileTimeToLocalFileTime(&ft,&ftLocal);
  
  SYSTEMTIME stResult;
  FileTimeToSystemTime(&ftLocal,&stResult);
  return stResult;
}

bool SaveBufferToFile(LPCTSTR lpszPath, void* pvData, int cbData)
{
  HANDLE hFile = CreateFile(lpszPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
  if(hFile != INVALID_HANDLE_VALUE)
  {
    DWORD cbWritten = 0;
    WriteFile(hFile,pvData,cbData,&cbWritten,NULL);
    CloseHandle(hFile);
    return true;
  }
  return false;
}