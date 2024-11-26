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
        //  Initialize the send message parameters, and fill in the dialog with the old item for editing.
        HWND hWndMsg = GetDlgItem(hWnd,IDC_EDTMESSAGE2);
        SendMessageW(hWndMsg, WM_SETTEXT, (WPARAM)m_pResults->szName, (LPARAM)m_pResults->szName);
        return TRUE;
	}
    case WM_COMMAND:
    {
      switch(LOWORD(wParam))
      {
        case IDOK:
        {
          HWND hWndMsg = GetDlgItem(hWnd,IDC_EDTMESSAGE2);
		  TCHAR szTemp[MAX_PATH] = {NULL};
          SendMessage(hWndMsg, WM_GETTEXT, NUMCHARS(szTemp), (LPARAM)szTemp);
          if(wcslen(szTemp) > 0 && wcscmp( m_pResults->szName, szTemp ) != 0 )
          {
			  //	User actually entered something. Let's return this for updating.
			  wcscpy_s( m_pResults->szName, NUMCHARS(m_pResults->szName), szTemp );	//	Copy the string to the sfResult pointer
	          m_pResults->fCancelled = false;
          }
		  else
		  {
			  //	User did not enter or change anything. Let's not make any changes.
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
