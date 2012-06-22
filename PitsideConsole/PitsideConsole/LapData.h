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
  DATA_CHANNEL_VELOCITY = 3,
  DATA_CHANNEL_TIMESLIP = 4,
  DATA_CHANNEL_X_ACCEL = 5,
  DATA_CHANNEL_Y_ACCEL = 6,
  DATA_CHANNEL_TEMP = 7,
  DATA_CHANNEL_SIGNAL_STRENGTH = 8,

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

bool FindClosestTwoPoints(const TimePoint2D& p, double dInputPercentage, const vector<TimePoint2D>& lstPoints, TimePoint2D* pt1, TimePoint2D* pt2);

class CExtendedLap
{
public:
  CExtendedLap(const CLap* pLap, const CExtendedLap* pReferenceLap, ILapReceiver* pLapDB) : m_pLap(pLap)
  {
    m_szComment[0] = '\0';
    GetLocalTime(&m_tmRecv);
    ComputeLapData(pLap->GetPoints(),pReferenceLap,pLapDB);
  }
  const CLap* GetLap() const {return m_pLap;}
  void GetString(LPTSTR lpszBuffer, int cchBuffer) const
  {
      TCHAR szTime[100];
      FormatTimeMinutesSecondsMs(m_pLap->GetTime(),szTime,NUMCHARS(szTime));
      if(m_szComment[0] != '\0')
      {
        swprintf(lpszBuffer, cchBuffer, L"%02d:%02d:%02d - Laptime: %s - \"%s\"", m_tmStart.wHour, m_tmStart.wMinute, m_tmStart.wSecond, szTime, m_szComment);
      }
      else
      {
        swprintf(lpszBuffer, cchBuffer, L"%02d:%02d:%02d - Laptime: %s", m_tmStart.wHour, m_tmStart.wMinute, m_tmStart.wSecond, szTime, m_szComment);
      }
  }
  static void GetStringHeaders(vector<wstring>& lstCols, vector<int>& lstWidths)
  {
    lstCols.push_back(L"Time");
    lstWidths.push_back(80);
    lstCols.push_back(L"Laptime");
    lstWidths.push_back(80);
    lstCols.push_back(L"Comment");
    lstWidths.push_back(255);
  }
  void GetStrings(vector<wstring>& lstStrings) const
  {
    TCHAR szTime[100];
    swprintf(szTime, NUMCHARS(szTime), L"%02d:%02d:%02d", m_tmStart.wHour, m_tmStart.wMinute, m_tmStart.wSecond);
    lstStrings.push_back(szTime); // pushes the time-of-day

    FormatTimeMinutesSecondsMs(m_pLap->GetTime(),szTime,NUMCHARS(szTime));
    lstStrings.push_back(szTime); // pushes the laptime

    lstStrings.push_back(m_szComment);
  }
  wstring GetComment() const
  {
    return wstring(m_szComment);
  }
  void SetComment(LPCTSTR lpsz)
  {
    wcscpy_s(m_szComment, NUMCHARS(m_szComment), lpsz);
  }
  const vector<TimePoint2D>& GetPoints() const {return m_lstPoints;}

  // this is telling us to recompute our distances based on this reference lap and our already-owned CLap
  void ComputeDistances(const CExtendedLap* pReferenceLap, ILapReceiver* pReceiver)
  {
    m_lstPoints.clear();
    ComputeLapData(GetLap()->GetPoints(), pReferenceLap, pReceiver);
  }
private:
  // function to properly compute distances, and not just do the sum of the changes in position.
  // pReceiver allows us to add data channels as they are computed
  void ComputeLapData(const vector<TimePoint2D>& lstPoints, const CExtendedLap* pReferenceLap, ILapReceiver* pReceiver);
private:
  const CLap* m_pLap;
  TCHAR m_szComment[512];
  SYSTEMTIME m_tmRecv; // when was this thing constructed?
  SYSTEMTIME m_tmStart; // when was this thing started on-track?
  vector<TimePoint2D> m_lstPoints;
};

const TimePoint2D GetPointAtTime(const vector<TimePoint2D>& lstPoints, int iTimeMs);