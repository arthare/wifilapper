#pragma once
#include <vector>
#include "ArtTools.h" // for FLOATRECT
#include "LapReceiver.h" // for TimePoint2D
#include "ArtUI.h" // for ArtOpenGLWindow
#include "LapData.h" // for CExtendedPoint
#include "DlgPlotSelect.h" // For PlotPrefs structure

using namespace std;

struct MAPHIGHLIGHT
{
  const CExtendedLap* pLap;
  POINT pt;
};

class CExtendedLap;

enum CHANNELDISPLAYSTYLE
{
  CHANNELDISPLAYSTYLE_VALUE,
  CHANNELDISPLAYSTYLE_GRAPH,
};

// supplier IDs - each lap painter is given a supplier ID, which it uses to identify itself when asking for more data
enum SUPPLIERID
{
  SUPPLIERID_MAINDISPLAY,
  SUPPLIERID_SUBDISPLAY,
  SUPPLIERID_SECTORDISPLAY,
  SUPPLIERID_TRACTIONCIRCLEDISPLAY,	//	Draw the traction 
};

enum LAPDISPLAYSTYLE
{
  LAPDISPLAYSTYLE_MAP,
  LAPDISPLAYSTYLE_PLOT,
  LAPDISPLAYSTYLE_NOLAPS, // what we display if there are no laps selected
  LAPDISPLAYSTYLE_RECEPTION, // display a map of wireless reception data
  LAPDISPLAYSTYLE_COUNT,
  LAPDISPLAYSTYLE_TRACTIONCIRCLE,	//	Draw the traction 
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
  void DrawLapLines(const LAPSUPPLIEROPTIONS& sfLapOpts); // draws laps as a map	Made public by KDJ
  void DrawTractionCircle(const LAPSUPPLIEROPTIONS& sfLapOpts, bool fHighlightXAxis);	//	Made public by KDJ
  double CLapPainter::PolynomialFilter(double flValue, double fTransAValue, double fTransBValue, double fTransCValue);
  //void SetHighlighter(ILapHighlighter* pHighlighter);
  //ILapHighlighter* GetHighlighter() {return m_pHighlighter;}
private:
  void DrawGeneralGraph(const LAPSUPPLIEROPTIONS& sfLapOpts, bool fHighlightXAxis);	
//  void DrawLapLines(const LAPSUPPLIEROPTIONS& sfLapOpts); // draws laps as a map	Made public by KDJ
  void DrawSelectLapsPrompt() const;
  void DrawSelectLapsPromptShort() const;
  void DrawReceptionMap(const LAPSUPPLIEROPTIONS& sfLapOpts) const;
  void MakeColor(const CExtendedLap* pLap,  bool RefLapFlag, float* pR, float* pG, float*pB); 
  void LineColor();
  void drawOval (float x_center, float y_center, float w, float h);
  void DrawHorizontalLine(float flLine, float dMinX, float dMaxX, char szText[256]);
  void DrawVerticalLine(double flLine, float mapMinY, float mapMaxY, char szText[512]);
  void fExpMovingAvg( int n, vector<DataPoint>& lstPointsX, double alpha, vector<DataPoint>& lstSmoothPts );
  void fBoxMovingAvg( int n, vector<DataPoint>& lstPoints, int w, vector<DataPoint>& lstSmoothPts, bool bSmoothFlag );
  //  void MagicDeterminingFunction(const LAPSUPPLIEROPTIONS& sfLapOpts, bool fHighlightXAxis);
private:
  ILapSupplier* m_pLapSupplier;

  //ILapHighlighter* m_pHighlighter;

  int m_iSupplierId;
  static int d;	//	For Value table counting
  //IUI* m_pUI;
};
