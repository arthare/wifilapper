#include "stdafx.h"
#include "ArtNet.h"
#include <mmsystem.h>

int TimeoutRead(SOCKET s, char* buf, int cbBuf, int flags, int timeout, bool* pfConnectionLost)
{
  *pfConnectionLost = false;

  DWORD tmNow = timeGetTime();
  while(true)
  {
    unsigned long cbWaiting = 0;
    int iRet = ioctlsocket(s, FIONREAD, &cbWaiting);
    if(iRet == 0)
    {
      if(cbWaiting > 0)
      {
        return recv(s, buf, cbBuf, flags);
      }
      else
      {
        if(timeGetTime() - tmNow > (UINT)timeout)
        {
          // timed out
          *pfConnectionLost = true;
          break;
        }
        else
        {
          // not timed-out yet
          Sleep(10);
        }
      }
    }
    else
    {
      // network failure
      *pfConnectionLost = true;
      break;
    }
  }
  return 0;
}