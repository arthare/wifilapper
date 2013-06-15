#pragma once
#include <Windows.h>
#include "artui.h"
#include "resource.h"

class CSplashDlg : public IUI
{
public:
  CSplashDlg() {};
  virtual ~CSplashDlg() {};

  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_DLGSPLASH;}
private:
};
