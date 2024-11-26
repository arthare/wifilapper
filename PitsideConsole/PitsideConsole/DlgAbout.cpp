#include "Stdafx.h"
#include "resource.h"
#include "pitsideconsole.h"
#include "DlgAbout.h"
#include "Hyperlinks.h"

LRESULT CAboutDlg::DlgProc(HWND c_hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
    {
		HBITMAP hBitmap = (HBITMAP)::LoadImage( GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SPLASHIMAGE), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);
		ConvertStaticToHyperlink(c_hWnd, IDC_LBLMESSAGE6);
		HWND hWndSplash = GetDlgItem( c_hWnd, IDC_SPLASHIMAGE );
		SendMessage(hWndSplash,STM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hBitmap);
		break;
    }
    case WM_RBUTTONDOWN:
    case WM_LBUTTONDOWN:
    {
      EndDialog(c_hWnd,0);
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