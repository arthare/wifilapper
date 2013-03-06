#pragma once

#include "ArtUI.h"
#include "ArtTools.h"
#include "LapReceiver.h"
#include "PitsideConsole.h"
#include "LapData.h"
#include "DlgRaceSelect.h"
#include "LapPainter.h"

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
//  CPlotSelectDlg(ILapReceiver* pLapDB, PLOTSELECT_RESULT* pResults, int iRaceId) : m_pPlotResults(pResults), m_iRaceId(iRaceId)
  CPlotSelectDlg(ILapReceiver* pLapDB, PLOTSELECT_RESULT* pResults, int iRaceId, ILapSupplier* ILapSupplier) : m_pPlotResults(pResults), m_iRaceId(iRaceId), m_ILapSupplier(ILapSupplier)
  {
		m_pLapDB = pLapDB;
  };
  int InitPlotPrefs (HWND hWnd, LPARAM lParam);
//  int SetPlotPrefs(HWND hWnd, set<DATA_CHANNEL> setAvailable);
  void InitPlotChannels(set<DATA_CHANNEL> setAvailable);

  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_PLOTPREFS;}
private:
	PLOTSELECT_RESULT* m_pPlotResults;
	int m_iRaceId;
	ILapReceiver* m_pLapDB;
	ILapSupplier* m_ILapSupplier;
	ArtListBox m_sfYAxis;
	RACESELECT_RESULT* m_pResults;
	virtual vector<DATA_CHANNEL> GetYChannels()
	{
		return m_lstYChannels;
	};
	vector<DATA_CHANNEL> m_lstYChannels;
};
