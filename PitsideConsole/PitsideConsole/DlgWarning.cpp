#include "Stdafx.h"
#include "resource.h"
#include "pitsideconsole.h"
#include "DlgWarning.h"
#include "Hyperlinks.h"

LRESULT CWarningDlg::DlgProc(HWND c_hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
    {
		TCHAR szMessage[1024] = L"";
		swprintf(szMessage, NUMCHARS(szMessage), L"One or more of the alarm limits has been triggered\n\nCheck your Data Value parameters!!\n\nFailing Channel(s): \n%s", m_szYString);
		HWND hWndWarning = GetDlgItem(c_hWnd, IDC_WARNING1);
		SendMessage(hWndWarning, WM_SETTEXT, NUMCHARS(szMessage), (LPARAM)szMessage);
		MessageBeep(MB_OK);	//	Play a warning sound
		break;
    }
    case WM_COMMAND:
    {
      switch(LOWORD(wParam))
      {
        case IDOK:
        {
          m_sfResult->fCancelled = false;
          EndDialog(c_hWnd,0);
          return TRUE;
        }
      }
      break;
    } // end WM_COMMAND
    case WM_CLOSE:
    {
      m_sfResult->fCancelled = true;
      EndDialog(c_hWnd,0);
      break;
    }
  }
  return FALSE;
}