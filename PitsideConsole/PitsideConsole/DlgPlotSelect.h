#pragma once

#include "ArtUI.h"
#include "ArtTools.h"
#include "LapReceiver.h"

struct PLOTSELECT_RESULT
{
public:
  PLOTSELECT_RESULT()
  {
    iPlotId = -1;
    fCancelled = false;
  }
  int iPlotId;
  bool fCancelled;
};


//	Create a data structure containing all of the Plotting preferences and make it available to entire program.
//	Use it to create a 50 term array to store these values
static struct PlotPrefs 
{
	LPCWSTR m_ChannelName[512];
	bool iPlotView;
	double fMinValue;
	double fMaxValue;
} m_PlotPrefs[50];


class CPlotSelectDlg : public IUI
{
public:
  CPlotSelectDlg(ILapReceiver* pLapDB, PLOTSELECT_RESULT* pResults) : m_pResults(pResults) 
  {
  m_pLapDB = pLapDB;
  int InitPlotPrefs (HWND hWnd, set<DATA_CHANNEL> setAvailable);
//  void InitPlotChannels(set<DATA_CHANNEL> setAvailable);
//  void GetDataChannelName(DATA_CHANNEL eDC, LPTSTR lpszName, int cch, set<DATA_CHANNEL> setAvailable);
  };
  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgPlot(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_PLOTPREFS;}
private:
	PLOTSELECT_RESULT* m_pResults;
	ILapReceiver* m_pLapDB;
	ArtListBox sfListBox;
};
