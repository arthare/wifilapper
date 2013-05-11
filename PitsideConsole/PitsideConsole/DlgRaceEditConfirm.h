#pragma once
#include <Windows.h>
#include "artui.h"
#include "ArtTools.h"
#include "resource.h"

struct RACEEDITCONFIRM_RESULT
{
public:
  RACEEDITCONFIRM_RESULT()
  {
    fCancelled = false;
  }
  bool fCancelled;
};

class CRaceEditConfirmDlg : public IUI
{
public:
  CRaceEditConfirmDlg(RACEEDITCONFIRM_RESULT* pResults) : m_sfResult(pResults) {};
  virtual ~CRaceEditConfirmDlg() {};

  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_RACEEDITCONFIRM;}
public:
  RACEEDITCONFIRM_RESULT* m_sfResult;
private:
};