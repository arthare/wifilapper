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
int GetSecondsSince1970()
{
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  ULARGE_INTEGER ulTime;
  ulTime.LowPart = ft.dwLowDateTime;
  ulTime.HighPart = ft.dwHighDateTime;
  const LONGLONG llSecondsBetween1601And1970 = 11644473600; // from the google

  const LONGLONG iSecsSince1601 = (ulTime.QuadPart / 10000000);
  const LONGLONG iSecsSince1970 = iSecsSince1601 - llSecondsBetween1601And1970;
  return (int)iSecsSince1970;
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

// a very simple integer conversion function.  Doesn't support negatives, but fails properly instead of returning 0
bool ArtAtoi(LPCSTR lpsz, int cch, int* pOut)
{
  for(int x = 0;x < cch; x++)
  {
    if(lpsz[x] >= '0' && lpsz[x] <= '9')
    {
    }
    else
    {
      return false;
    }
  }
  *pOut = atoi(lpsz);
  return true;
}

bool GetAppFolder(LPTSTR lpszBuf, const int cch)
{
  GetModuleFileName(NULL,lpszBuf,cch);
  // now we should have a path like "c:\blah\pitsideconsole.exe".  We want to remove the PitsideConsole.exe and append the user's lpszFile (which will probably be WebSide.js)
  int cchEnd = wcslen(lpszBuf)-1;
  for(;cchEnd >= 0;cchEnd--)
  {
    if(lpszBuf[cchEnd] == '\\') break;
  }
  lpszBuf[cchEnd+1] = (TCHAR)0; // kill the string after the slash

  return true;
}

bool DoesFileExist(LPCTSTR lpsz)
{
  HANDLE hFile = CreateFile(lpsz,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if(hFile != INVALID_HANDLE_VALUE)
  {
    CloseHandle(hFile);
    return true;
  }
  return false;
}


#define LOWCASE(x) ((x)& (~0x20))

int nospacecompare(LPCTSTR lpsz1, LPCTSTR lpsz2)
{
  int ix1 = 0;
  int ix2 = 0;
  while(true)
  {
    if(lpsz1[ix1] == ' ')
    {
      ix1++; // skip the space
      continue;
    }
    if(lpsz2[ix2] == ' ') 
    {
      ix2++; // skip the space
      continue;
    }
    TCHAR c1 = LOWCASE(lpsz1[ix1]);
    TCHAR c2 = LOWCASE(lpsz2[ix2]);
    if(c1 != c2) 
      return 1;
    DASSERT(lpsz1[ix1] == lpsz2[ix2]); // they must match for this character...

    if(lpsz1[ix1] == 0) 
      return 0; // we found the end of the string.  we know that lpsz2[ix2] is the same, so this string matches!
    DASSERT(lpsz1[ix1] != 0 && lpsz2[ix2] != 0); // both these characters are nonzero
    ix1++;
    ix2++;
  }
  return 0;
}