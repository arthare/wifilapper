#pragma once

#include <vector>
#include <iostream>
#include <windows.h>
#include "ArtVector.h"
#include "ArtTools.h"
#include <map>
#include "ArtSQL/ArtSQLite.h"

using namespace std;

struct TimePoint2D
{
public:
  TimePoint2D() {}
	TimePoint2D(const TimePoint2D* pToFlip) // flips all the input bits
	{
		flX = FLIPBITS(pToFlip->flX);
		flY = FLIPBITS(pToFlip->flY);
		iTime = FLIPBITS(pToFlip->iTime);
		flVelocity = FLIPBITS(pToFlip->flVelocity);
    flSum = FLIPBITS(pToFlip->flSum);
	}
  bool IsValid()
  {
    float flComputedSum = flX + flY;
    if(flSum == flComputedSum) return true;
    return false;
  }
  bool operator != (const TimePoint2D& pt) const
  {
    return flX != pt.flX || flY != pt.flY || iTime != pt.iTime || flVelocity != pt.flVelocity;
  }

  // flWeight: 0.0 -> entirely pt1.  1.0 -> entirely pt2.  0.5 -> normal averaging
  static TimePoint2D Average(const TimePoint2D& pt1, const TimePoint2D& pt2, float flWeight)
  {
    float fl1Minus = 1.0f - flWeight;
    TimePoint2D ptRet;
    ptRet.flVelocity = fl1Minus*pt1.flVelocity + flWeight*pt2.flVelocity;
    ptRet.flX = fl1Minus*pt1.flX + flWeight*pt2.flX;
    ptRet.flY = fl1Minus*pt1.flY + flWeight*pt2.flY;
    ptRet.iTime = (int)(fl1Minus*pt1.iTime + flWeight*pt2.iTime);
    ptRet.flSum = (ptRet.flX + ptRet.flY);
    return ptRet;
  }
  Vector2D operator - (const TimePoint2D& pt) const
  {
    Vector2D ret;
    ret.m_v[0] = flX - pt.flX;
    ret.m_v[1] = flY - pt.flY;
    return ret;
  }
  Vector2D V2D() const
  {
    Vector2D ret;
    ret.m_v[0] = flX;
    ret.m_v[1] = flY;
    return ret;
  }

	float flX;
	float flY;
	int iTime; // the time since the phone app started that this point was received (in milliseconds)
	float flVelocity;
  float flSum;
};

struct DataPoint
{
  DataPoint() : flValue(0),iTimeMs(0) {}
  DataPoint(int iTimeMs, float flValue) : flValue(flValue),iTimeMs(iTimeMs) {}
  float flValue;
  int iTimeMs;
};

enum DATA_CHANNEL;
struct InputChannelRaw
{
  int iLapId;
  int eChannelType;
  int cPoints;
  DataPoint rgPoints[1];
};

struct InputLapRaw
{
	int iLapId;
	int cCount;
	float dTime;
  int iStartTime; // start time of the lap in unix time (seconds since 1970)
  float rgSF[12]; // the 6 points (in 3 pairs) designating split 1, split 2, and start/finish.
  float rgSFDir[6]; // the 3 vectors (x1,y1,x2,y2,x3,y3) showing direction of the SF lines
	TimePoint2D rgPoints[1];
};

class CDataChannel
{
public:
  CDataChannel();
  virtual ~CDataChannel();

  void Load(InputChannelRaw* pData);
  bool LoadZipped(const char* pbData, int cbData);
  bool Load(CSfArtSQLiteDB& db, CSfArtSQLiteQuery& dc);
  void Init(int iLapId, DATA_CHANNEL eChannel);
  bool IsValid() const;
  bool IsSameChannel(const CDataChannel* pOther) const;

  int GetLapId() const;

  // random-access version: slower
  float GetValue(int iTime) const;
  // the iterator points to the DataPoint with time greater than the current time
  float GetValue(int iTime, const vector<DataPoint>::const_iterator& i) const;
  float GetMin() const;
  float GetMax() const;
  int GetEndTimeMs() const;
  int GetStartTimeMs() const;

  const vector<DataPoint>& GetData() const {return lstData;}

  void AddPoint(int iTime, float flValue); // iTime must be in milliseconds since the phone app started.  flValue can be whatever you want
  DATA_CHANNEL GetChannelType() const {return eChannelType;}
  // when you lock a data channel, it means that no more points may be added to it ever.
  // it also sorts the vector
  void Lock();
  bool IsLocked() const {return fLocked;}
private:
  bool fLocked;
  int iLapId;
  DATA_CHANNEL eChannelType;
  vector<DataPoint> lstData;
  map<int,float> m_mapCachedData;

  float m_dMin;
  float m_dMax;

  int m_msMin; // start time (milliseconds since app start)
  int m_msMax; // end time (milliseconds since app start)
};


class StartFinish
{
public:
  StartFinish()
  { 
  }
  StartFinish(float* pPointData)
  {
    m_pt1.m_v[0] = pPointData[0];
    m_pt1.m_v[1] = pPointData[1];
    m_pt2.m_v[0] = pPointData[2];
    m_pt2.m_v[1] = pPointData[3];
  }
  const Vector2D& GetPt1() const {return m_pt1;}
  const Vector2D& GetPt2() const {return m_pt2;}
private:
  Vector2D m_pt1;
  Vector2D m_pt2;
};
// a more civilized lap, constructed from InputLapRaw and sent to the user's ILapReceiver
class CLap
{
public:
	CLap()
	{

	}
  void Load(InputLapRaw* pLap);
  bool LoadZipped(const char* pszInput, int cbInput);
  bool Load(CSfArtSQLiteDB& db, StartFinish* rgSF, CSfArtSQLiteQuery& line);

	bool IsValid() const
	{
		return dTime < 3600 && dTime > 3.0 && lstPoints.size() > 0;
	}
  int GetStartTime() const {return iStartTime;} // returns the start time in unix time (seconds since 1970)
  int GetLapId() const {return iLapId;}
  float GetTime() const {return dTime;}
  const vector<TimePoint2D>& GetPoints() const {return lstPoints;}
  const StartFinish* GetSF() const {return rgSF;}
private:
	vector<TimePoint2D> lstPoints;
  StartFinish rgSF[3];
  Vector2D vDir[3];
	float dTime;
	int iLapId;
  int iStartTime; // seconds since Jan 1 1970
};

template<int cToMatch>
struct TextMatcher
{
	char szToMatch[cToMatch];
	int cMatched;
	bool Process(char c)
	{
		if(cMatched == cToMatch)
		{
			cMatched = 0;
		}
		if(szToMatch[cMatched] == c)
		{
			cMatched++;
		}
		else
		{
			cMatched = szToMatch[0] == c;
		}
		return cMatched == cToMatch;
	}
  int GetSize() const {return cToMatch;}
	void Reset()
	{
		cMatched = 0;
	}
	TextMatcher(const char* pbMatch)
	{
		strcpy(szToMatch,pbMatch);
		cMatched = 0;
	}
};

enum NETSTATUSSTRING
{
  NETSTATUS_STATUS,
  NETSTATUS_THISIP,
  NETSTATUS_REMOTEIP,
  NETSTATUS_DB, // parameter is the file location of the transmitted database file

  NETSTATUS_COUNT,
};
interface ILapReceiver
{
public:
  // memory management
	virtual CLap* AllocateLap() const = 0;
	virtual void FreeLap(CLap* pLap) const = 0;

  virtual CDataChannel* AllocateDataChannel() const = 0;
  virtual void FreeDataChannel(CDataChannel* pChannel) const = 0;

  // data access
  // I chose to access all the laps at once to avoid race condition issues if the network thread updates
  // the databank while the UI is displaying it
  virtual vector<const CLap*> GetLaps() const = 0;
  virtual const CLap* GetLap(int iLapId) const = 0;
  virtual const CDataChannel* GetDataChannel(int iLapId, DATA_CHANNEL eChannel) const = 0;
  virtual set<DATA_CHANNEL> GetAvailableChannels(int iLapId) const = 0;

  // modifying data
	virtual void AddLap(const CLap* pLap) = 0;
  virtual void AddDataChannel(const CDataChannel* pChannel) = 0;
  virtual void Clear() = 0;

  // status strings
  virtual void NotifyDBArrival(LPCTSTR lpszPath) = 0;
  virtual void SetNetStatus(NETSTATUSSTRING eString, LPCTSTR szData) = 0; // network man tells us the latest status
  virtual LPCTSTR GetNetStatus(NETSTATUSSTRING eString) const = 0;
};

// net-thread entry point.  Fills a ILapReceiver from the network
bool ReceiveLaps(int iPort, ILapReceiver* pLaps);

// fills an ILapReceiver from an SQLite database
void LoadFromSQLite(LPCTSTR lpszSQL, int iRaceId, ILapReceiver* pRecv);