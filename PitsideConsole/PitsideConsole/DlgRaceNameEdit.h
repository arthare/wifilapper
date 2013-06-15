#pragma once

#include "ArtUI.h"
#include "ArtTools.h"
#include "AutoCS.h"
#include "resource.h"

struct RENAMEDLG_RESULT
{
public:
  RENAMEDLG_RESULT()
  {
    fCancelled = true;
    szName[0] = '\0';
  }
  bool fCancelled;
  TCHAR szName[260];
};

class CRenameDlg : public IUI
{
public:
  CRenameDlg(RENAMEDLG_RESULT* pResults) : m_pResults(pResults) {};

  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_RACENAMECHANGE;}
private:
  RENAMEDLG_RESULT* m_pResults;
};
