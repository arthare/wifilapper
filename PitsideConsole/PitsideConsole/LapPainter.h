#pragma once
#include <vector>
#include "ArtTools.h" // for FLOATRECT
#include "LapReceiver.h" // for TimePoint2D
#include "ArtUI.h" // for ArtOpenGLWindow
#include "LapData.h" // for CExtendedPoint
using namespace std;


class CExtendedLap;

enum LAPDISPLAYSTYLE
{
  LAPDISPLAYSTYLE_MAP,
  LAPDISPLAYSTYLE_PLOT,
  LAPDISPLAYSTYLE_NOLAPS,

  LAPDISPLAYSTYLE_COUNT,
};
struct LAPSUPPLIEROPTIONS
{
public:
  LAPSUPPLIEROPTIONS() : eUnitPreference(UNIT_PREFERENCE_KMH),fDrawSplitPoints(true),fDrawGuides(true),flWindowShiftX(0),flWindowShiftY(0),iZoomLevels(0)
  {
  }
  UNIT_PREFERENCE eUnitPreference;
  bool fDrawSplitPoints;
  bool fDrawGuides;
  bool fDrawLines; // whether to draw lines between data points
  float flWindowShiftX;
  float flWindowShiftY;
  int iZoomLevels;
};

// LapSupplier interface - needed so that the lap painter knows what to paint
interface ILapSupplier
{
public:
  virtual vector<CExtendedLap*> GetLapsToShow() const = 0;
  virtual vector<CExtendedLap*> GetAllLaps() const = 0;
  virtual LAPDISPLAYSTYLE GetLapDisplayStyle(int iSupplierId) const = 0;
  virtual DATA_CHANNEL GetXChannel() const = 0;
  virtual vector<DATA_CHANNEL> GetYChannels() const = 0;
  virtual const IDataChannel* GetChannel(int iLapId, DATA_CHANNEL eChannel) const = 0;
  virtual FLOATRECT GetAllLapsBounds() const = 0;

  // guide-parameter functions
  virtual float GetGuideStart(DATA_CHANNEL eChannel, float flMin, float flMax) = 0;
  virtual float GetGuideStep(DATA_CHANNEL eChannel, float flMin, float flMax) = 0;

  // highlighting functions
  virtual void SetLapHighlightTime(const CExtendedLap* pLap, int iTimeMs) = 0;
  virtual int GetLapHighlightTime(const CExtendedLap* pLap) const = 0;
  virtual bool IsHighlightSource(int iSupplierId) const = 0; // returns whether the caller should be a lap highlighter (calling SetLapHighlightTime) or a lap highlight-receiver (calling GetLapHighlightTime)

  virtual const LAPSUPPLIEROPTIONS& GetDisplayOptions() const = 0;
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
private:
  ILapSupplier* m_pLapSupplier;

  //ILapHighlighter* m_pHighlighter;

  int m_iSupplierId;

  //IUI* m_pUI;
};
