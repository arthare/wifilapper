#include "Stdafx.h"
#include "resource.h"
#include "pitsideconsole.h"
#include "DlgSplash.h"
#include "Hyperlinks.h"

LRESULT CSplashDlg::DlgProc
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
      HBITMAP hBitmap = (HBITMAP)::LoadImage( GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SPLASHIMAGE), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);
	  ConvertStaticToHyperlink(hWnd, IDC_LBLMESSAGE6);
      HWND hWndSplash = GetDlgItem( hWnd, IDC_SPLASHIMAGE );
      SendMessage(hWndSplash,STM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hBitmap);
      break;
    }
    case WM_RBUTTONDOWN:
    case WM_LBUTTONDOWN:
    {
      EndDialog(hWnd,0);
      break;
    }
    case WM_COMMAND:
    {
      switch(LOWORD(wParam))
      {
        case IDOK:
        {
          EndDialog(hWnd,0);
          return TRUE;
        }
      }
      break;
    } // end WM_COMMAND
    case WM_CLOSE:
    {
      EndDialog(hWnd,0);
      return 0;
    }
  }
  return FALSE;
}