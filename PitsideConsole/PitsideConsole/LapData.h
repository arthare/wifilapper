#pragma once
#include <vector>
#include "LapReceiver.h"
#include "ArtTools.h" // for FormatTimeMinutesSecondsMs
using namespace std;

enum DATA_CHANNEL
{
  DATA_CHANNEL_START = 0,
  DATA_CHANNEL_X = DATA_CHANNEL_START,
  DATA_CHANNEL_Y = 1,

  DATA_CHANNEL_PLOTTABLE_START = 2,

  DATA_CHANNEL_DISTANCE = DATA_CHANNEL_PLOTTABLE_START,
  DATA_CHANNEL_TIME = 11,
  DATA_CHANNEL_ELAPSEDTIME = 12,
  DATA_CHANNEL_LAPTIME_SUMMARY = 13,
  DATA_CHANNEL_VELOCITY = 3,
  DATA_CHANNEL_TIMESLIP = 4,
  DATA_CHANNEL_X_ACCEL = 5,
  DATA_CHANNEL_Y_ACCEL = 6,
  DATA_CHANNEL_TEMP = 7,
  DATA_CHANNEL_RECEPTION_X = 8,
  DATA_CHANNEL_RECEPTION_Y = 9,
  DATA_CHANNEL_Z_ACCEL = 10,

  DATA_CHANNEL_PID_START = 0x100,
  DATA_CHANNEL_PID_END = 0x200,

  DATA_CHANNEL_IOIOPIN_START = 0x200,
  DATA_CHANNEL_IOIOPIN_END = 0x300,
  DATA_CHANNEL_IOIOCUSTOM_START = 0x301,
  DATA_CHANNEL_IOIOCUSTOM_END = 0x400,
  DATA_CHANNEL_COUNT,
};

enum UNIT_PREFERENCE
{
  UNIT_PREFERENCE_KMH,
  UNIT_PREFERENCE_MPH,
  UNIT_PREFERENCE_MS,

  UNIT_PREFERENCE_COUNT,
};
void GetDataChannelName(DATA_CHANNEL eDC, LPTSTR lpszName, int cch);
float ConvertSpeed(UNIT_PREFERENCE eConvertTo, float flVelocityInMetersPerSecond);
LPCSTR GetUnitText(UNIT_PREFERENCE eUnits);
void GetChannelString(DATA_CHANNEL eX, UNIT_PREFERENCE eUnits, float flValue, LPSTR lpsz, int cch);
void GetChannelValue(DATA_CHANNEL eX, UNIT_PREFERENCE eUnits, float flValue, LPSTR lpsz, int cch);

bool FindClosestTwoPoints(const TimePoint2D& p, double dInputPercentage, const vector<TimePoint2D>& lstPoints, TimePoint2D* pt1, TimePoint2D* pt2);

// this class contains the UI representation of a lap.  It contains, in memory, all the data channels for a lap, pre-aligned, and ready for display.
// it is built by the main display dialog from a lap, using a reference lap.  If you're displaying UI, you shouldn't directly access the database for lap data, you should use a CExtendedLap instead
class CExtendedLap
{
public:
  CExtendedLap(const ILap* pLap, CExtendedLap* pReferenceLap, ILapReceiver* pLapDB, bool fComputeTimeSlip) : m_pLap(pLap), m_fComputeTimeSlip(fComputeTimeSlip)
  {
    DASSERT(pReferenceLap);
    GetLocalTime(&m_tmRecv);
    m_tmStart = SecondsSince1970ToSYSTEMTIME(pLap->GetStartTime());

    m_pReferenceLap = pReferenceLap;
    m_pLapDB = pLapDB;
  }
  virtual ~CExtendedLap()
  {
    for(map<DATA_CHANNEL,const IDataChannel*>::iterator i = m_mapChannels.begin(); i != m_mapChannels.end(); i++)
    {
      this->m_pLapDB->FreeDataChannel((IDataChannel*)i->second);
    }
    m_mapChannels.clear();
  }
  void Compact()
  {
    if(m_lstPoints.size() > 0)
    {
      for(map<DATA_CHANNEL,const IDataChannel*>::iterator i = m_mapChannels.begin(); i != m_mapChannels.end(); i++)
      {
        this->m_pLapDB->FreeDataChannel((IDataChannel*)i->second);
      }
      m_mapChannels.clear();
      m_lstPoints.clear();
    }
  }
  const ILap* GetLap() const {return m_pLap;}
  void GetString(LPTSTR lpszBuffer, int cchBuffer) const
  {
      TCHAR szTime[100];
      FormatTimeMinutesSecondsMs(m_pLap->GetTime(),szTime,NUMCHARS(szTime));
      if(GetLap()->GetComment().size() > 0)
      {
        swprintf(lpszBuffer, cchBuffer, L"%02d:%02d:%02d - Laptime: %s - \"%s\"", m_tmStart.wHour, m_tmStart.wMinute, m_tmStart.wSecond, szTime, GetLap()->GetComment().c_str());
      }
      else
      {
        swprintf(lpszBuffer, cchBuffer, L"%02d:%02d:%02d - Laptime: %s", m_tmStart.wHour, m_tmStart.wMinute, m_tmStart.wSecond, szTime);
      }
  }
  static void GetStringHeaders(vector<wstring>& lstCols, vector<int>& lstWidths)
  {
    lstCols.push_back(L"Time");
    lstWidths.push_back(60);
    lstCols.push_back(L"Laptime");
    lstWidths.push_back(60);
    lstCols.push_back(L"Comment");
    lstWidths.push_back(75);
  }
  static void GetStringHeadersXAxis(vector<wstring>& lstCols, vector<int>& lstWidths)
  {
    lstCols.push_back(L"X-Axis");
    lstWidths.push_back(90);
  }
  static void GetStringHeadersYAxis(vector<wstring>& lstCols, vector<int>& lstWidths)
  {
    lstCols.push_back(L"Y-Axis");
    lstWidths.push_back(90);
  }
  void GetStrings(vector<wstring>& lstStrings) const
  {
    TCHAR szTime[100];
    swprintf(szTime, NUMCHARS(szTime), L"%02d:%02d:%02d", m_tmStart.wHour, m_tmStart.wMinute, m_tmStart.wSecond);
    lstStrings.push_back(szTime); // pushes the time-of-day

    FormatTimeMinutesSecondsMs(m_pLap->GetTime(),szTime,NUMCHARS(szTime));
    lstStrings.push_back(szTime); // pushes the laptime

    if(GetLap()->GetComment().size() > 0)
    {
      lstStrings.push_back(GetLap()->GetComment());
    }
    else
    {
      lstStrings.push_back(L"");
    }
  }
  const vector<TimePoint2D>& GetPoints() 
  {
    CheckCompute();

    return m_lstPoints;
  }

  set<DATA_CHANNEL> GetAvailableChannels() const
  {
    set<DATA_CHANNEL> setRet;
    for(map<DATA_CHANNEL,const IDataChannel*>::iterator i = m_mapChannels.begin(); i != m_mapChannels.end(); i++)
    {
      setRet.insert(i->first);
    }
    return setRet;
  }
  const IDataChannel* GetChannel(DATA_CHANNEL eChan)
  {
    CheckCompute();
    return m_mapChannels[eChan];
  }

  // this is telling us to recompute our distances based on this reference lap and our already-owned CLap
  void ComputeDistances(CExtendedLap* pReferenceLap, ILapReceiver* pReceiver)
  {
    DASSERT(pReferenceLap);
    m_lstPoints.clear();
    m_pLapDB = pReceiver;
    m_pReferenceLap = pReferenceLap;
  }
private:
  // function to properly compute distances, and not just do the sum of the changes in position.
  // pReceiver allows us to add data channels as they are computed
  void ComputeLapData(const vector<TimePoint2D>& lstPoints, CExtendedLap* pReferenceLap, const ILapReceiver* pReceiver, bool fDoTimeSlip);
  void CheckCompute()
  {
    if(m_lstPoints.size() <= 0)
    {
      ComputeLapData(m_pLap->GetPoints(),m_pReferenceLap,m_pLapDB, m_fComputeTimeSlip);
    }
  }
  void AddChannel(const IDataChannel* pChan)
  {
    DASSERT(pChan->IsLocked());
    if(m_mapChannels[pChan->GetChannelType()])
    {
      m_pLapDB->FreeDataChannel((IDataChannel*)m_mapChannels[pChan->GetChannelType()]);
    }
    m_mapChannels[pChan->GetChannelType()] = pChan;
  }
private:
  ILapReceiver* m_pLapDB;
  CExtendedLap* m_pReferenceLap;
  const ILap* m_pLap;
  SYSTEMTIME m_tmRecv; // when was this thing constructed?
  SYSTEMTIME m_tmStart; // when was this thing started on-track?
public:
  vector<TimePoint2D> m_lstPoints;	//	Made public by KDJ
private:  
  bool m_fComputeTimeSlip; // time-slip is the most expensive data channel, so let's allow the caller to choose not to compute it

  mutable map<DATA_CHANNEL,const IDataChannel*> m_mapChannels; // we own these pointers.  We get them allocated in ComputeLapData, and it is our responsibility to get them de-allocated
};
const TimePoint2D GetPointAtTime(const vector<TimePoint2D>& lstPoints, int iTimeMs);
