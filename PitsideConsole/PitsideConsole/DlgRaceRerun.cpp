#include "Stdafx.h"
#include "resource.h"
#include "pitsideconsole.h"
#include "DlgRaceRerun.h"

LRESULT CRaceRerunDlg::DlgProc(HWND c_hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
    {
		TCHAR szText[MAX_PATH];
		swprintf(szText, NUMCHARS(szText), L"%i", m_sfResult->iStart);
		SetDlgItemText(c_hWnd, IDC_RACE_START, szText);

		swprintf(szText, NUMCHARS(szText), L"%i", m_sfResult->iEnd);
		SetDlgItemText(c_hWnd, IDC_RACE_END, szText);
		m_sfResult->iEnd = _wtoi(szText);
		break;
    }
    case WM_COMMAND:
    {
      switch(LOWORD(wParam))
      {
        case IDOK:
        {
          m_sfResult->fCancelled = false;
		  TCHAR szText[MAX_PATH];
		  GetDlgItemText(c_hWnd, IDC_RACE_START, szText, NUMCHARS(szText));
		  m_sfResult->iStart = _wtoi(szText);

		  GetDlgItemText(c_hWnd, IDC_RACE_END, szText, NUMCHARS(szText));
		  m_sfResult->iEnd = _wtoi(szText);
          EndDialog(c_hWnd,0);
          return TRUE;
        }
        case IDCANCEL:
		{
			m_sfResult->fCancelled = true;
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