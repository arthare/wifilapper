#pragma once
#include <Windows.h>
#include "artui.h"
#include "ArtTools.h"
#include "resource.h"

struct RACERERUN_RESULT
{
public:
  RACERERUN_RESULT()
  {
    fCancelled = false;
	iStart = 0;
	iEnd = 0;
  }
  bool fCancelled;
  int iStart;
  int iEnd;
};

class CRaceRerunDlg : public IUI
{
public:
  CRaceRerunDlg(RACERERUN_RESULT* pResults) : m_sfResult(pResults) {};
  virtual ~CRaceRerunDlg() {};

  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_RACE_RERUN;}
public:
  RACERERUN_RESULT* m_sfResult;
private:
};