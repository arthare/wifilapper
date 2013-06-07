#pragma once
#include <Windows.h>
#include "artui.h"
#include "ArtTools.h"
#include "resource.h"

struct WARNING_RESULT
{
public:
  WARNING_RESULT()
  {
    fCancelled = false;
  }
  bool fCancelled;
  TCHAR szYString[512];
};

class CWarningDlg : public IUI
{
public:
  CWarningDlg(WARNING_RESULT* pResults, TCHAR szYString[512]) : m_sfResult(pResults), m_szYString(szYString)
  {
  };
  virtual ~CWarningDlg() {};

  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_WARNING;}
public:
  WARNING_RESULT* m_sfResult;
  TCHAR* m_szYString;
private:
};