#pragma once
#include <Windows.h>
#include "artui.h"
#include "ArtTools.h"
#include "resource.h"

struct ABOUT_RESULT
{
public:
  ABOUT_RESULT()
  {
    fCancelled = false;
  }
  bool fCancelled;
};

class CAboutDlg : public IUI
{
public:
  CAboutDlg(ABOUT_RESULT* pResults) : m_sfResult(pResults) {};
  virtual ~CAboutDlg() {};

  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_ABOUT;}
public:
  ABOUT_RESULT* m_sfResult;
private:
};