#pragma once

#include "ArtUI.h"
#include "LapData.h"

#include "ArtTools.h"
#include "LapReceiver.h"
#include "PitsideConsole.h"
#include "DlgRaceSelect.h"
//#include "LapPainter.h"

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

struct PITSIDE_SETTINGS
{
  void Default()
  {
    fRunHTTP = true;
    iHTTPPort = 80;
	iVelocity = 1;
	iMapLines = 1;
	iColorScheme = 0;
  }

  int fRunHTTP;
  int iHTTPPort;
  int iVelocity;
  int iMapLines;
  int iColorScheme;
};

struct LAPSUPPLIEROPTIONS
{
public:
  LAPSUPPLIEROPTIONS() : /*eUnitPreference(UNIT_PREFERENCE_MPH),*/fDrawSplitPoints(true),fDrawGuides(true),/*fDrawLines(true),fColorScheme(false),*/fIOIOHardcoded(true),flWindowShiftX(0),flWindowShiftY(0),iZoomLevels(0)
  {
  }
  UNIT_PREFERENCE eUnitPreference;
  bool fDrawSplitPoints;
  bool fDrawGuides;
  bool fDrawLines; // whether to draw lines between data points
  bool fColorScheme;
  bool fIOIOHardcoded;
  bool fElapsedTime;
  float flWindowShiftX;
  float flWindowShiftY;
  int iZoomLevels;
  PlotPrefs m_PlotPrefs[50];	// Pull in PlotPrefs data
};


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

class CPlotSelectDlg : public IUI
{
public:
  CPlotSelectDlg(ILapReceiver* pLapDB, PLOTSELECT_RESULT* pResults, int iRaceId, LAPSUPPLIEROPTIONS* i_sfLapOpts) : m_pPlotResults(pResults), m_iRaceId(iRaceId), m_sfLapOpts(i_sfLapOpts)
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
	LAPSUPPLIEROPTIONS* m_sfLapOpts;
};
