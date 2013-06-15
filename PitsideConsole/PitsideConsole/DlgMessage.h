#pragma once

#include "ArtUI.h"
#include "ArtTools.h"
#include "AutoCS.h"
#include "resource.h"
struct MESSAGEDLG_RESULT
{
public:
  MESSAGEDLG_RESULT()
  {
    fCancelled = true;
    iTime = 1;
    iAttemptTime = 1;
    szMessage[0] = '\0';
  }
  bool fCancelled;
  int iTime; // number of minutes to show message
  int iAttemptTime; // number of minutes to attempt to send message
  char szIP[100]; // string version of the target IP
  TCHAR szMessage[260];
};

struct WIFILAPPER_MESSAGE
{
  WIFILAPPER_MESSAGE() : WFLP('WFLP'),iTime(1)
  {
    memset(szMessage,0,sizeof(szMessage));
  }
  const int WFLP;
  int iTime; // time to show the message
  char szMessage[260];
};

class CMessageDlg : public IUI
{
public:
  CMessageDlg(MESSAGEDLG_RESULT* pResults) : m_pResults(pResults) {};

  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_DLGMESSAGE;}
private:
  MESSAGEDLG_RESULT* m_pResults;
};

class CMsgThread
{
public:
  CMsgThread(IUI* pUI);
  void Start(const MESSAGEDLG_RESULT& msg);
  void Shutdown();
  DWORD MessageThreadProc();
  DWORD MessageRecvThreadProc();
  
  void GetStatusMessage(LPTSTR psz, int cch) const;
private:
  void SetStatusMessage(LPCTSTR psz);
private:
  mutable ManagedCS m_cs;
  MESSAGEDLG_RESULT m_msg;
  bool m_fContinue;
  bool m_fReceived;
  HANDLE m_hExit;
  HANDLE m_hThread;
  HANDLE m_hRecvThread;
  IUI* m_pUI;
  TCHAR m_szStatusMessage[256];
};

// the LPVOID will be a new'd MESSAGEDLG_RESULT.
// MessageSendProc will delete the MESSAGEDLG_RESULT
void SendMsg(const MESSAGEDLG_RESULT& msg, IUI* pUI);