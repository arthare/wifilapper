#pragma once
#include <vector>
#include "ArtTools.h" // for FLOATRECT
#include "LapReceiver.h" // for TimePoint2D
#include "ArtUI.h" // for ArtOpenGLWindow
#include "LapData.h" // for CExtendedPoint
//#include "DlgPlotSelect.h"	//	For Value display and Alarms
using namespace std;


class CExtendedLap;

enum CHANNELDISPLAYSTYLE
{
  CHANNELDISPLAYSTYLE_VALUE,
  CHANNELDISPLAYSTYLE_GRAPH,
};

enum LAPDISPLAYSTYLE
{
  LAPDISPLAYSTYLE_MAP,
  LAPDISPLAYSTYLE_PLOT,
  LAPDISPLAYSTYLE_NOLAPS, // what we display if there are no laps selected
  LAPDISPLAYSTYLE_RECEPTION, // display a map of wireless reception data

  LAPDISPLAYSTYLE_COUNT,
};
struct LAPSUPPLIEROPTIONS
{
public:
  LAPSUPPLIEROPTIONS() : eUnitPreference(UNIT_PREFERENCE_MPH),fDrawSplitPoints(true),fDrawGuides(true),fDrawLines(true),fIOIOHardcoded(true), flWindowShiftX(0),flWindowShiftY(0),iZoomLevels(0)
  {
  }
  UNIT_PREFERENCE eUnitPreference;
  bool fDrawSplitPoints;
  bool fDrawGuides;
  bool fDrawLines; // whether to draw lines between data points
  bool fIOIOHardcoded;
  bool fElapsedTime;
  float flWindowShiftX;
  float flWindowShiftY;
  int iZoomLevels;
};

// LapSupplier interface - needed so that the lap painter knows what to paint
interface ILapSupplier
{
public:
  virtual vector<CExtendedLap*> GetLapsToShow() const = 0;	// <-- returns which laps you want painted
  virtual vector<CExtendedLap*> GetAllLaps() const = 0;		// <-- returns all the laps
  virtual LAPDISPLAYSTYLE GetLapDisplayStyle(int iSupplierId) const = 0;	// <-- returns how you want the laps shown: this can be a map or a data plot
  virtual DATA_CHANNEL GetXChannel() const = 0;				//  <-- returns what data channel you want to use for the x-axis
  virtual vector<DATA_CHANNEL> GetYChannels() const = 0;	//  <-- returns what data channels you want to use for the y-axes (can be 1 or more)
  virtual const IDataChannel* GetChannel(int iLapId, DATA_CHANNEL eChannel) const = 0;	// <-- returns the actual data channel object for a given lap.
  virtual FLOATRECT GetAllLapsBounds() const = 0;			//  <-- returns the x and y bounds for all the laps

  // guide-parameter functions - these configure the background horizontal/vertical lines
  virtual float GetGuideStart(DATA_CHANNEL eChannel, float flMin, float flMax) = 0;	//  <-- returns the position (in the units of whatever data channel you're plotting) of the first line we should draw
  virtual float GetGuideStartX(DATA_CHANNEL eChannel, float flMin, float flMax) = 0;	//  <-- returns the position (in the units of whatever data channel you're plotting) of the first line we should draw
  virtual float GetGuideStep(DATA_CHANNEL eChannel, float flMin, float flMax) = 0;	// <-- returns the distance between guidelines (in units of whatever data channel you're plotting)
  virtual float GetGuideStepX(DATA_CHANNEL eChannel, float flMin, float flMax) = 0;	// <-- returns the distance between guidelines (in units of whatever data channel you're plotting)
  virtual float GetDataHardcodedMin(DATA_CHANNEL eChannel) const = 0;		// <-- returns the absolute lowest value we want to display for a given data channel type
  virtual float GetDataHardcodedMax(DATA_CHANNEL eChannel) const = 0;		// <-- returns the absolute highest value we want to display for a given data channel type

  // highlighting functions
  virtual void SetLapHighlightTime(const CExtendedLap* pLap, int iTimeMs) = 0;	//  <-- only call if you are a HighlightSource. Sets the time in milliseconds that should be highlighted.
  virtual int GetLapHighlightTime(const CExtendedLap* pLap) const = 0;		// <-- gets the time in milliseconds that should be highlighted.
  virtual bool IsHighlightSource(int iSupplierId) const = 0; // returns whether the caller should be a lap highlighter (calling SetLapHighlightTime) or a lap highlight-receiver (calling GetLapHighlightTime)

  virtual const LAPSUPPLIEROPTIONS& GetDisplayOptions() const = 0;		// <-- gets more display options.
};

class CLapPainter : public ArtOpenGLWindow
{
public:
  CLapPainter(ILapSupplier* pLapSupplier, int iSupplierId);
  virtual ~CLapPainter();

  // paints all the laps supplied by our ILapSupplier
  virtual void OGL_Paint() override;
  
  //void SetHighlighter(ILapHighlighter* pHighlighter);
  //ILapHighlighter* GetHighlighter() {return m_pHighlighter;}
private:
  void DrawGeneralGraph(const LAPSUPPLIEROPTIONS& sfLapOpts, bool fHighlightXAxis);
  void DrawLapLines(const LAPSUPPLIEROPTIONS& sfLapOpts); // draws laps as a map
  void DrawSelectLapsPrompt() const;
  void DrawReceptionMap(const LAPSUPPLIEROPTIONS& sfLapOpts) const;
  void MakeColor(const CExtendedLap* pLap, float* pR, float* pG, float*pB); 
//  void MagicDeterminingFunction(const LAPSUPPLIEROPTIONS& sfLapOpts, bool fHighlightXAxis);
private:
  ILapSupplier* m_pLapSupplier;

  //ILapHighlighter* m_pHighlighter;

  int m_iSupplierId;

  //IUI* m_pUI;
};
