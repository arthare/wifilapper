#include "stdafx.h"
#include "DlgProgress.h"
#include "commctrl.h"
#include "resource.h"

LRESULT CProgressDlg::DlgProc
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
    INITCOMMONCONTROLSEX InitCtrlEx;

    InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitCtrlEx.dwICC  = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&InitCtrlEx);
    m_hWnd = hWnd;
    m_hWndProgress = CreateWindowEx(0, PROGRESS_CLASS, NULL,
		               WS_CHILD | WS_VISIBLE,
			      5, 5, 200, 20,
			      hWnd, NULL, NULL, NULL);
    break;
  }
  case WM_NOTIFY_PROGRESS:
    UpdateProgress();
    return TRUE;
  case WM_NOTIFY_TOTAL:
    InitProgress();
    return TRUE;
  case WM_NOTIFY_DONE:
    EndDialog(m_hWnd, 0);
    return TRUE;
  }
  return FALSE;
}

void CProgressDlg::InitProgress()
{
  SendMessage(m_hWndProgress, PBM_SETRANGE, 0, MAKELPARAM(0,m_cTotal));
}

void CProgressDlg::UpdateProgress()
{
  SendMessage(m_hWndProgress, PBM_SETPOS, m_cProgress, 0);
}