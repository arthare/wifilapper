#pragma once

#include <vector>
#include "ArtTools.h" // for FLOATRECT
#include "LapReceiver.h" // for TimePoint2D
#include "ArtUI.h" // for ArtOpenGLWindow
#include "LapData.h" // for CExtendedPoint
#include "resource.h"
#include "LapPainter.h"
#include "PitsideConsole.h"

struct SETSPLITSDLG_RESULT
{
public:
  SETSPLITSDLG_RESULT()
  {
    fCancelled = true;
  }
  bool fCancelled;
};

class CSetSplitsDlg : public IUI, public ILapSupplier
{
public:
  CSetSplitsDlg(ILapReceiver* pLapDB, CExtendedLap* pLap, SETSPLITSDLG_RESULT* pResults, int iRaceId, LAPSUPPLIEROPTIONS* i_sfLapOpts) : m_pLap(pLap), m_pResults(pResults), m_iRaceId(iRaceId), m_sfLapOpts(i_sfLapOpts), p_sfRefLapPainter(this,SUPPLIERID_SECTORDISPLAY)
  {
		m_pLapDB = pLapDB;
		m_pLap = pLap;
  };

  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_SETSPLITPOINTS;}

	// <-- returns which laps you want painted
  virtual vector<CExtendedLap*> GetLapsToShow() const
  {
    vector<CExtendedLap*> lstLaps;
    lstLaps.push_back(m_pLap);
    return lstLaps;
  }
  // <-- returns all the laps
  virtual vector<CExtendedLap*> GetAllLaps() const
  {
    vector<CExtendedLap*> lstLaps;
    lstLaps.push_back(m_pLap);
    return lstLaps;
  }
  // <-- returns how you want the laps shown: this can be a map or a data plot
  virtual LAPDISPLAYSTYLE GetLapDisplayStyle(int iSupplierId) const
  {
	  return LAPDISPLAYSTYLE_MAP;
  }
  //  <-- returns what data channel you want to use for the x-axis
  virtual DATA_CHANNEL GetXChannel() const
  {
	  return DATA_CHANNEL_X;
  }
  //  <-- returns what data channels you want to use for the y-axes (can be 1 or more)
  virtual vector<DATA_CHANNEL> GetYChannels() const
  {
	  return m_lstYChannels;
  }
  // <-- returns the actual data channel object for a given lap.
  virtual const IDataChannel* GetChannel(int iLapId, DATA_CHANNEL eChannel) const
  {
	  return m_pLapDB->GetDataChannel(iLapId, eChannel);
  }
  //  <-- returns the x and y bounds for all the laps
  virtual FLOATRECT GetAllLapsBounds() const
  {
    FLOATRECT rc;
    rc.left = 1e30;
    rc.top = -1e30;
    rc.bottom = 1e30;
    rc.right = -1e30;
    
    // changed this so it returns the bounds of the reference lap.  This way, data-viewing isn't ruined by errant points
    // it used to be based on all the laps, but if you had just one messed-up lap it would make viewing pointless
    if(m_pLap != NULL)
    {
      const vector<TimePoint2D>& lstPoints = m_pLap->GetPoints();
      for(int x = 0; x< lstPoints.size(); x++)
      {
        const TimePoint2D& p = lstPoints[x];
        rc.left = min(rc.left,p.flX);
        rc.top = max(rc.top,p.flY);
        rc.bottom = min(rc.bottom,p.flY);
        rc.right = max(rc.right,p.flX);
      }
    }
    return rc;
  }

  // guide-parameter functions - these configure the background horizontal/vertical lines
  //  <-- returns the position (in the units of whatever data channel you're plotting) of the first line we should draw
  virtual float GetGuideStart(DATA_CHANNEL eChannel, float flMin, float flMax)
  {
	  return 0.0f;
  }
  //  <-- returns the position (in the units of whatever data channel you're plotting) of the first line we should draw
  virtual float GetGuideStartX(DATA_CHANNEL eChannel, float flMin, float flMax)
  {
	  return 0.0f;
  }
  // <-- returns the distance between guidelines (in units of whatever data channel you're plotting)
  virtual float GetGuideStep(DATA_CHANNEL eChannel, float flMin, float flMax)
  {
	  return 0.0f;
  }
  // <-- returns the distance between guidelines (in units of whatever data channel you're plotting)
  virtual float GetGuideStepX(DATA_CHANNEL eChannel, float flMin, float flMax)
  {
	  return 0.0f;
  }
  // <-- returns the absolute lowest value we want to display for a given data channel type
  virtual float GetDataHardcodedMin(DATA_CHANNEL eChannel) const
  {
	  return 0.0f;
  }
  // <-- returns the absolute highest value we want to display for a given data channel type
  virtual float GetDataHardcodedMax(DATA_CHANNEL eChannel) const
  {
	  return 0.0f;
  }

  // highlighting functions
  //  <-- only call if you are a HighlightSource. Sets the time in milliseconds that should be highlighted.
  virtual void SetLapHighlightTime(const CExtendedLap* m_pLap, int iTimeMs)
  {
    m_mapLapHighlightTimes[m_pLap] = iTimeMs;
  }
  // <-- gets the time in milliseconds that should be highlighted.
  virtual int GetLapHighlightTime(const CExtendedLap* m_pLap) const
  {
    DASSERT(m_mapLapHighlightTimes.find(m_pLap) != m_mapLapHighlightTimes.end()); // this should have always ended up set from the "master" highlighter.  This function is only called by "slave" highlight-users
    return m_mapLapHighlightTimes.find(m_pLap)->second;
  }
  // returns whether the caller should be a lap highlighter (calling SetLapHighlightTime) or a lap highlight-receiver (calling GetLapHighlightTime)
  virtual bool IsHighlightSource(int iSupplierId) const
  {
		return true;	//	Allow the Set Split Sectors to be highlight source
  }
  // <-- gets more display options.
  virtual const LAPSUPPLIEROPTIONS& GetDisplayOptions() const
  {
    return *m_sfLapOpts;
  }
private:
	void GetSplitPoint(int x, SplitPoints szTempSplit, HWND hWnd);
	void ComputeSectors();
	SETSPLITSDLG_RESULT* m_pResults;
	int m_iRaceId;
	ILapReceiver* m_pLapDB;
	LAPSUPPLIEROPTIONS* m_sfLapOpts;
	CExtendedLap* m_pLap;
	ILapSupplier* m_sfSectorDisplay;
	vector<DATA_CHANNEL> m_lstYChannels;
	map<const CExtendedLap*,int> m_mapLapHighlightTimes; // stores the highlight times (in milliseconds since phone app start) for each lap.  Set from ILapSupplier calls
    int m_iSupplierId;
	CLapPainter p_sfRefLapPainter;

};
