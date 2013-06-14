#pragma once
#include <Windows.h>
#include "artui.h"
#include "ArtTools.h"
#include "resource.h"
#include "LapPainter.h"

struct SHOWSPLIT_RESULT
{
public:
  SHOWSPLIT_RESULT()
  {
    fCancelled = false;
  }
  bool fCancelled;
};

class CShowSplitsDlg : public IUI
{
public:
  CShowSplitsDlg(SHOWSPLIT_RESULT* pResults) : m_sfResult(pResults) {};
  virtual ~CShowSplitsDlg() {};

  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_SHOWSECTORS;}
public:
  SHOWSPLIT_RESULT* m_sfResult;
private:
};