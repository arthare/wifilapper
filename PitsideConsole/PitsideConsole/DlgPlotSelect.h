#pragma once

#include "ArtUI.h"
#include "LapData.h"

#include "ArtTools.h"
#include "LapReceiver.h"
#include "PitsideConsole.h"
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
struct PlotPrefs 
{
	TCHAR m_ChannelName[512];
	DATA_CHANNEL iDataChannel;
	bool iPlotView;
	double fMinValue;
	double fMaxValue;
}; 
extern PlotPrefs m_PlotPrefs[50];	//	Declare the PlotPrefs array so it's of global scope

class CPlotSelectDlg : public IUI
{
public:
  CPlotSelectDlg(ILapReceiver* pLapDB, PLOTSELECT_RESULT* pResults, int iRaceId, ILapSupplier* ILapSupplier, PlotPrefs* i_PlotPrefs) : m_pPlotResults(pResults), m_iRaceId(iRaceId), m_ILapSupplier(ILapSupplier), p_PlotPrefs(i_PlotPrefs)
  {
		m_pLapDB = pLapDB;
  };
  int InitPlotPrefs (HWND hWnd, LPARAM lParam);
  int SetPlotPrefs(HWND hWnd, set<DATA_CHANNEL> setAvailable);
  void InitPlotChannels(set<DATA_CHANNEL> setAvailable);

  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_PLOTPREFS;}
private:
	PLOTSELECT_RESULT* m_pPlotResults;
	int m_iRaceId;
	ILapReceiver* m_pLapDB;
	ArtListBox m_sfYAxis;
	PlotPrefs* p_PlotPrefs;
	ILapSupplier* m_ILapSupplier;
};
