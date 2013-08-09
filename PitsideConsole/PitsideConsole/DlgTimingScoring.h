#pragma once

#include "ArtUI.h"
#include "ArtTools.h"
#include "LapReceiver.h"
#include "DlgSelectSessions.h"

struct TS_RESULT
{
public:
  TS_RESULT()
  {
    iRaceId = -1;
    fCancelled = false;
  }
  int iRaceId;
  bool fCancelled;
};

class CDlgTimingScoring : public IUI
{
public:
  CDlgTimingScoring(ILapReceiver* pLapDB, TS_RESULT* pResults, TCHAR* szPath, SELECTSESSIONS_RESULT* sfResult) : m_pResults(pResults) 
  {
    m_pLapDB = pLapDB;
	_snwprintf(m_szPath, NUMCHARS(m_szPath), szPath);
	m_sfResult = sfResult;
  };
  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_TIMINGSCORING;}
public:
  TS_RESULT* m_pResults;
  DWORD* TimingScoringProc(LPVOID pv, HWND hWnd);

private:
  ILapReceiver* m_pLapDB;
  ArtListBox sfListBox;
  TCHAR m_szPath[MAX_PATH];
  SELECTSESSIONS_RESULT* m_sfResult;
};