#include "Stdafx.h"
#include "winsock2.h"
#include "DlgMessage.h"
#include "AutoCS.h"
#include "resource.h"
#include "pitsideconsole.h"

LRESULT CMessageDlg::DlgProc
(
  HWND hWnd, 
  UINT uMsg, 
  WPARAM wParam, 
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
    {
        //  Initialize the send message parameters.
		TCHAR szTime[100] = L"1";
		HWND hWndTime = GetDlgItem(hWnd, IDC_EDTTIME);
		SendMessage(hWndTime, WM_SETTEXT, NUMCHARS(szTime), (LPARAM)szTime);
		int iTime = _wtoi(szTime);
		HWND hWndAttemptTime = GetDlgItem(hWnd, IDC_EDTATTEMPTTIME);
		SendMessage(hWndAttemptTime, WM_SETTEXT, NUMCHARS(szTime), (LPARAM)szTime);
		int iAttemptTime = _wtoi(szTime);
        return TRUE;
	}
    case WM_COMMAND:
    {
      switch(LOWORD(wParam))
      {
        case IDOK:
        {
          HWND hWndMsg = GetDlgItem(hWnd,IDC_EDTMESSAGE);
          SendMessage(hWndMsg, WM_GETTEXT, NUMCHARS(m_pResults->szMessage), (LPARAM)m_pResults->szMessage);
          if(wcslen(m_pResults->szMessage) > 0)
          {
            TCHAR szTime[100];
            HWND hWndTime = GetDlgItem(hWnd, IDC_EDTTIME);
            SendMessage(hWndTime, WM_GETTEXT, NUMCHARS(szTime), (LPARAM)szTime);
            int iTime = _wtoi(szTime);
            if(iTime > 0)
            {
              HWND hWndAttemptTime = GetDlgItem(hWnd, IDC_EDTATTEMPTTIME);
              SendMessage(hWndAttemptTime, WM_GETTEXT, NUMCHARS(szTime), (LPARAM)szTime);
              int iAttemptTime = _wtoi(szTime);
              if(iAttemptTime > 0)
              {
                m_pResults->iAttemptTime = iAttemptTime;
                m_pResults->iTime = iTime;
                m_pResults->fCancelled = false;
                EndDialog(hWnd,0);
                return TRUE;
              }
            }
          }
          return TRUE;
        }
        case IDCANCEL:
          m_pResults->fCancelled = true;
          EndDialog(hWnd,0);
          return TRUE;
      }
      break;
    } // end WM_COMMAND
    case WM_CLOSE:
    {
      m_pResults->fCancelled = true;
      EndDialog(hWnd,0);
      break;
    }
  }
  return FALSE;
}


DWORD MessageSendProc(LPVOID param)
{
  CMsgThread* pThread = (CMsgThread*)param;
  return pThread->MessageThreadProc();
}
DWORD MessageRecvProc(LPVOID param)
{
  CMsgThread* pThread = (CMsgThread*)param;
  return pThread->MessageRecvThreadProc();
}

CMsgThread::CMsgThread(IUI* pUI)
{
  m_pUI = pUI;
  m_hExit = NULL;
  m_hThread = NULL;
  m_fContinue = false;

  SetStatusMessage(L"Starting");

}
void CMsgThread::Start(const MESSAGEDLG_RESULT& msg)
{
  Shutdown(); // won't return until we're done
  
  AutoLeaveCS _cs(&m_cs);
  DASSERT(m_hThread == NULL && m_hExit == NULL);
  m_msg = msg;

  m_fReceived = false;
  m_fContinue = true;
  
  
  m_hExit = CreateEvent(NULL, FALSE, FALSE, NULL);
  m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&MessageSendProc, (LPVOID)this, 0, NULL);
  m_hRecvThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)&MessageRecvProc,(LPVOID)this,0,NULL);
}
void CMsgThread::Shutdown()
{
  m_fContinue = false;
  //	Wait a maximum of 1 minute, then end the thread process.
  WaitForSingleObject(m_hExit,60*1000);
  WaitForSingleObject(m_hThread,60*1000);
  WaitForSingleObject(m_hRecvThread,60*1000);
  {
    AutoLeaveCS _cs(&m_cs);
    if(m_hThread == NULL && m_hExit == NULL) return; // nothing is going on
    DASSERT(m_hThread && m_hExit);
  }

  {
    AutoLeaveCS _cs(&m_cs);
    CloseHandle(m_hExit);
    CloseHandle(m_hThread);
    CloseHandle(m_hRecvThread);
    m_hExit = NULL;
    m_hThread = NULL;
  }
}

// the LPVOID will be a new'd MESSAGEDLG_RESULT.
// MessageSendProc will delete the MESSAGEDLG_RESULT
DWORD CMsgThread::MessageThreadProc()
{
  SOCKET sOutgoing = INVALID_SOCKET;
  MESSAGEDLG_RESULT msg;
  {
    AutoLeaveCS _cs(&m_cs);
    msg = m_msg;
  }
  sOutgoing = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  DWORD tmEnd = timeGetTime() + msg.iAttemptTime*60*1000; // figures out the current time plus how long we're supposed to send that message
  while(true)
  {
    if(timeGetTime() > tmEnd) 
    {
      SetStatusMessage(L"Gave up sending message");  
      break;
    }
    if(!m_fContinue) 
    {
      break;
    }

    if(sOutgoing != INVALID_SOCKET)
    {
      sockaddr_in  dest;
      dest.sin_family = AF_INET;
      dest.sin_addr.s_addr = inet_addr(msg.szIP);
      dest.sin_port = htons(63940);

      WIFILAPPER_MESSAGE sfMsg;
      sfMsg.iTime = FLIPBITS(msg.iTime);
      sprintf(sfMsg.szMessage, "%S", msg.szMessage);

      const int cbChecksum = GetCheckSum(&sfMsg, sizeof(sfMsg));

      const int cbSent = sendto(sOutgoing, (const char*)&sfMsg, sizeof(sfMsg), 0, (sockaddr*)&dest, sizeof(dest));
      

      // tell the user how we're doing...
      {
        const int msTimeLeft = tmEnd - timeGetTime();
        TCHAR szBlah[200];
        _snwprintf(szBlah,NUMCHARS(szBlah),L"Trying to send '%s' (%ds)",this->m_msg.szMessage,msTimeLeft/1000);
        SetStatusMessage(szBlah);
      }

      Sleep(250);
    }
    else
    {
      SetStatusMessage(L"Network error");
      break;
    }
  }
  SetEvent(m_hExit);
  
  if(sOutgoing != INVALID_SOCKET)
  {
    closesocket(sOutgoing);
    sOutgoing= INVALID_SOCKET;
  }
  
  if(m_fReceived)
  {
    TCHAR szBlah[200];
    _snwprintf(szBlah,NUMCHARS(szBlah),L"Phone received '%s'",this->m_msg.szMessage);
    SetStatusMessage(szBlah);
  }
  else
  {
    SetStatusMessage(L"Message attempt aborted");
  }
  return 0;
}
// the LPVOID will be a new'd MESSAGEDLG_RESULT.
// MessageSendProc will delete the MESSAGEDLG_RESULT
DWORD CMsgThread::MessageRecvThreadProc()
{
  MESSAGEDLG_RESULT msg;
  {
    AutoLeaveCS _cs(&m_cs);
    msg = m_msg;
  }

  int iRequiredCS = 0;
  {
    WIFILAPPER_MESSAGE sfMsg;
    sfMsg.iTime = FLIPBITS(msg.iTime);
    sprintf(sfMsg.szMessage, "%S", msg.szMessage);
    iRequiredCS = GetCheckSum(&sfMsg, sizeof(sfMsg));
  }

  SOCKET sIncoming = INVALID_SOCKET;
  sockaddr_in RecvAddr;

  sIncoming = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  
  RecvAddr.sin_family = AF_INET;
  RecvAddr.sin_port = htons(63941);
  RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  int iRet = bind(sIncoming, (SOCKADDR *) & RecvAddr, sizeof (RecvAddr));
  if (iRet != 0) 
  {
      wprintf(L"bind failed with error %d\n", WSAGetLastError());
      return 1;
  }
  DWORD tmEnd = timeGetTime() + msg.iAttemptTime*60*1000; // figures out the current time plus how long we're supposed to send that message
  while(true)
  {
     if(timeGetTime() > tmEnd) 
     {
       break;
     }
     if(!m_fContinue) 
     {
       break;
     }

    if(sIncoming != INVALID_SOCKET)
    {
      fd_set socks;
      struct timeval t;
      FD_ZERO(&socks);
      FD_SET(sIncoming, &socks);
      t.tv_sec = 1;
      t.tv_usec = 0;

      if(select(1,&socks,NULL,NULL,&t))
      {
        char szBuf[1000];
        const int cbSent = recv(sIncoming, szBuf, sizeof(szBuf), 0);
        if(cbSent >= 8)
        {
          // we expect the response to be "wflp<integer checksum>"
          if(strncmp(szBuf,"wflp",4) == 0)
          {
            int* prgChecksum = (int*)&szBuf[4];
            if(FLIPBITS(prgChecksum[0]) == iRequiredCS)
            {
              m_fReceived = true;
              m_fContinue = false;
              break;
            }
          }
        }
      }

      Sleep(250);
    }
    else
    {
      SetStatusMessage(L"Network error");
      break;
    }
  }
  SetEvent(m_hExit);
  
  if(sIncoming != INVALID_SOCKET)
  {
    closesocket(sIncoming);
    sIncoming= INVALID_SOCKET;
  }

  return 0;
}
void CMsgThread::SetStatusMessage(LPCTSTR psz)
{
  {
    AutoLeaveCS _cs(&m_cs);
    wcsncpy(m_szStatusMessage, psz, NUMCHARS(m_szStatusMessage));
  }
  m_pUI->NotifyChange(NOTIFY_NEWMSGDATA,(LPARAM)this);
}
void CMsgThread::GetStatusMessage(LPTSTR psz, int cch) const
{
  AutoLeaveCS _cs(&m_cs);
  wcsncpy(psz, m_szStatusMessage, cch);
}


CMsgThread* g_pMsgThread = NULL;

void SendMsg(const MESSAGEDLG_RESULT& msg, IUI* pUI)
{
  if(g_pMsgThread == NULL)
  {
    g_pMsgThread = new CMsgThread(pUI);
  }
  g_pMsgThread->Start(msg);
}