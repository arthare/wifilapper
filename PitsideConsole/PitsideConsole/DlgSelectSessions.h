#pragma once

#include "ArtUI.h"
#include "ArtTools.h"
#include "LapReceiver.h"

struct SELECTSESSIONS_RESULT
{
public:
  SELECTSESSIONS_RESULT()
  {
	  m_RaceId[0] = -1;
	  fCancelled = false;
  }
  int m_RaceId[50];
  bool fCancelled;
};

class CDlgSelectSessions : public IUI
{
public:
  CDlgSelectSessions(ILapReceiver* pLapDB, SELECTSESSIONS_RESULT* pResults) : m_pResults(pResults) 
  {
    m_pLapDB = pLapDB;
  };
  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_SELECTSESSIONS;}
public:
  SELECTSESSIONS_RESULT* m_pResults;

private:
//  RACESELECT_RESULT* m_pResults;
  ILapReceiver* m_pLapDB;
  ArtListBox sfListBox;
};