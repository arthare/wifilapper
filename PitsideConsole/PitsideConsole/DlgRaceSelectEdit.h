#pragma once

#include "ArtUI.h"
#include "ArtTools.h"
#include "LapReceiver.h"
//#include "SQLiteLapDB.h"	//	Added by KDJ

struct RACESELECTEDIT_RESULT
{
public:
  RACESELECTEDIT_RESULT()
  {
    iRaceId = -1;
    fCancelled = false;
  }
  int iRaceId;
  bool fCancelled;
};

class CRaceSelectEditDlg : public IUI
{
public:
  CRaceSelectEditDlg(ILapReceiver* pLapDB, RACESELECTEDIT_RESULT* pResults) : m_pResults(pResults) 
  {
    m_pLapDB = pLapDB;
  };
  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_SELECTRACEEDIT;}
public:
  RACESELECTEDIT_RESULT* m_pResults;

private:
  ILapReceiver* m_pLapDB;
  ArtListBox sfListBox;
};

