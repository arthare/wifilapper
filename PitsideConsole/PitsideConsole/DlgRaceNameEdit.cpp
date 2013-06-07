#include "Stdafx.h"
#include "winsock2.h"
#include "DlgRaceNameEdit.h"
#include "AutoCS.h"
#include "resource.h"
#include "pitsideconsole.h"

LRESULT CRenameDlg::DlgProc
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
        return TRUE;
	}
    case WM_COMMAND:
    {
      switch(LOWORD(wParam))
      {
        case IDOK:
        {
          HWND hWndMsg = GetDlgItem(hWnd,IDC_EDTMESSAGE2);
          SendMessage(hWndMsg, WM_GETTEXT, NUMCHARS(m_pResults->szName), (LPARAM)m_pResults->szName);
          if(wcslen(m_pResults->szName) > 0)
          {
			  //	User actually entered something. Let's return this for updating.
	          m_pResults->fCancelled = false;
          }
		  else
		  {
			  //	User did not enter anything. Let's not make any changes.
	          m_pResults->fCancelled = true;
		  }
          EndDialog(hWnd,0);
          return TRUE;
        }
        case IDCANCEL:
		{
          m_pResults->fCancelled = true;
          EndDialog(hWnd,0);
          return TRUE;
		}
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
