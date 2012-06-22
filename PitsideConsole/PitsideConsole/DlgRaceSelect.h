#pragma once

#include "ArtUI.h"
#include "ArtTools.h"

struct RACESELECT_RESULT
{
public:
  RACESELECT_RESULT()
  {
    iRaceId = -1;
    fCancelled = false;
  }
  int iRaceId;
  bool fCancelled;
};

class CRaceSelectDlg : public IUI
{
public:
  CRaceSelectDlg(LPCTSTR lpszSQL, RACESELECT_RESULT* pResults) : m_pResults(pResults) 
  {
    wcsncpy(m_szSQL, lpszSQL, NUMCHARS(m_szSQL));
  };

  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_SELECTRACE;}
private:
  RACESELECT_RESULT* m_pResults;
  TCHAR m_szSQL[MAX_PATH];
  ArtListBox sfListBox;
};