#pragma once

#include "ArtUI.h"
#include "LapData.h"

#include "ArtTools.h"
#include "LapReceiver.h"
#include "PitsideConsole.h"
#include "DlgRaceSelect.h"

//	Create a data structure containing all of the Plotting preferences and make it available to entire program.
//	Use it to create a 50 term array to store these values
struct PlotPrefs 
{
	TCHAR m_ChannelName[512];
	DATA_CHANNEL iDataChannel;
	bool iPlotView;
	double fMinValue;
	double fMaxValue;
	bool iTransformYesNo;
	double fTransAValue;  
	double fTransBValue;
	double fTransCValue;
};

struct SplitPoints
{
	double m_sfXPoint;
	double m_sfYPoint;
	long m_sfSectorTime;
	float m_sfSplitTime;
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
	bSmoothYesNo = false;
	bXAxis_KM = false;
  }

  int fRunHTTP;
  int iHTTPPort;
  int iVelocity;
  int iMapLines;
  int iColorScheme;
  bool bSmoothYesNo;
  bool bXAxis_KM;
};

struct TRANSFORMATION
{
  void Default()
  {
	c_Name[0] = (TCHAR)L"";
	f_CoeffA = 0;
	f_CoeffB = 1;
	f_CoeffC = 0;
	b_LoadTrans = false;
  }

  TCHAR c_Name[MAX_PATH];
    float f_CoeffA;
	float f_CoeffB;
	float f_CoeffC;
	bool b_LoadTrans;
};
/*
enum UNIT_PREFERENCE
{
  UNIT_PREFERENCE_KMH,
  UNIT_PREFERENCE_MPH,
  UNIT_PREFERENCE_MS,

  UNIT_PREFERENCE_COUNT,
};
*/
enum LAPSORTSTYLE
{
	SORTSTYLE_BYTIMEOFRACE, // sort by the time the lap was done: 2:31pm comes before 4:45pm (well... on the same day)
	SORTSTYLE_BYLAPTIME, // sort by lap time.  1:12.15 comes before 1:13.45
};

enum ORIENTATION	//	These are phone orientatino ENUMS, for setting up the Traction Circle display inside LapPainter.cpp
{
	VERTICAL_LANDSCAPE,
	VERTICAL_PORTRAIT,
	FLAT_LANDSCAPE,
	FLAT_PORTRAIT
};

struct LAPSUPPLIEROPTIONS
{
public:
  LAPSUPPLIEROPTIONS() : eUnitPreference(UNIT_PREFERENCE_MPH),bXAxis_KM(false),fDrawSplitPoints(false),fDrawGuides(true),fDrawLines(true),fColorScheme(false),fIOIOHardcoded(true),flWindowShiftX(0),flWindowShiftY(0),iZoomLevels(0),bTractionCircle(false),bShowReferenceLap(true),e_Orientation(VERTICAL_LANDSCAPE), b_Button_LiveData(true)
  {
  }
  UNIT_PREFERENCE eUnitPreference;
  bool bXAxis_KM;
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
  SplitPoints m_SplitPoints[50];	//	Pull in all Split points data
  TRANSFORMATION m_Tranformations[100];	//	Pull in all Transformations data
  HWND hWndLap[50];
  LAPSORTSTYLE eSortPreference;
  bool bTractionCircle;		//	Whether or not to display the Traction Circle display
  bool bSmoothYesNo;	//	Whether or not to smooth the accelerometer graphs
  bool bShowReferenceLap;	//	Whether or not the Reference Lap is displayed
  ORIENTATION e_Orientation;	//	Phone orientation when installed in the car
  BOOL b_Button_LiveData;		//	Button for choosing to pause live data collection to allow for data processing
  int LapId;
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
  void InitPlotPrefs (HWND hWnd, LPARAM lParam);
  void SetPlotPrefs(HWND hWnd, set<DATA_CHANNEL> setAvailable);
  void InitPlotChannels(set<DATA_CHANNEL> setAvailable);

  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_PLOTPREFS;}
  LAPSUPPLIEROPTIONS* m_sfLapOpts;
private:
	PLOTSELECT_RESULT* m_pPlotResults;
	int m_iRaceId;
	ILapReceiver* m_pLapDB;
	ArtListBox m_sfYAxis;
	void CPlotSelectDlg::Checkbox(HWND hWnd, UINT DLG_ITEM, UINT CHECKBOX_STATE);
	void LoadTransformations(LAPSUPPLIEROPTIONS &p_sfLapOpts);
	void LoadDropDown(HWND hWnd);
};
