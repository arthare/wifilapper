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

struct V1InputLapRaw
{
	int iLapId;
	int cCount;
	float dTime;
  int iStartTime; // start time of the lap in unix time (seconds since 1970)
  float rgSF[12]; // the 6 points (in 3 pairs) designating split 1, split 2, and start/finish.
  float rgSFDir[6]; // the 3 vectors (x1,y1,x2,y2,x3,y3) showing direction of the SF lines
	TimePoint2D rgPoints[1];
};
struct V2InputLapRaw
{
  int iVersion_1; // redundant versions.  we don't want to get it wrong
  int iVersion_2;
  int iCarNumber;
  int iSecondaryCarNumber;
	int iLapId;
	int cCount;
	float dTime;
  int iStartTime; // start time of the lap in unix time (seconds since 1970)
  float rgSF[12]; // the 6 points (in 3 pairs) designating split 1, split 2, and start/finish.
  float rgSFDir[6]; // the 3 vectors (x1,y1,x2,y2,x3,y3) showing direction of the SF lines
	TimePoint2D rgPoints[1];
};

class IDataChannel
{
public:
  virtual void Load(InputChannelRaw* pData) = 0;
  virtual bool Load(CSfArtSQLiteDB& db, CSfArtSQLiteQuery& dc, bool fLazyLoad) = 0;
  virtual void Init(int iLapId, DATA_CHANNEL eChannel) = 0;
  virtual bool IsValid() const = 0;
  virtual bool IsSameChannel(const IDataChannel* pOther) const = 0;

  virtual int GetLapId() const = 0;

  // random-access version: slower
  virtual float GetValue(int iTime) const = 0;
  // the iterator points to the DataPoint with time greater than the current time
  virtual float GetValue(int iTime, const vector<DataPoint>::const_iterator& i) const = 0;
  virtual float GetMin() const = 0;
  virtual float GetMax() const = 0;
  virtual int GetEndTimeMs() const = 0;
  virtual int GetStartTimeMs() const = 0;

  virtual const vector<DataPoint>& GetData() const = 0;

  virtual void AddPoint(int iTime, float flValue) = 0;
  virtual DATA_CHANNEL GetChannelType() const = 0;
  // when you lock a data channel, it means that no more points may be added to it ever.
  // it also sorts the vector
  virtual void Lock() = 0;
  virtual bool IsLocked() const = 0;
};
class CDataChannel : public IDataChannel
{
public:
  CDataChannel();
  virtual ~CDataChannel();

  void Load(InputChannelRaw* pData) override;
  bool Load(CSfArtSQLiteDB& db, CSfArtSQLiteQuery& dc, bool fLazyLoad) override;
  void Init(int iLapId, DATA_CHANNEL eChannel) override;
  bool IsValid() const override;
  bool IsSameChannel(const IDataChannel* pOther) const override;

  int GetLapId() const override;

  // random-access version: slower
  float GetValue(int iTime) const override;
  // the iterator points to the DataPoint with time greater than the current time
  float GetValue(int iTime, const vector<DataPoint>::const_iterator& i) const override;
  float GetMin() const override;
  float GetMax() const override;
  int GetEndTimeMs() const override;
  int GetStartTimeMs() const override;

  const vector<DataPoint>& GetData() const override {return lstData;}

  void AddPoint(int iTime, float flValue) override; // iTime must be in milliseconds since the phone app started.  flValue can be whatever you want
  DATA_CHANNEL GetChannelType() const override {return eChannelType;}
  // when you lock a data channel, it means that no more points may be added to it ever.
  // it also sorts the vector
  void Lock() override;
  bool IsLocked() const override {return fLocked;}
private:
  void DoLoad(CSfArtSQLiteDB& db, int channelId);
  void CheckLazyLoad() const
  {
    CDataChannel* pChan = (CDataChannel*)this; // gross....
    if(m_fLazyLoad && !fLocked)
    {
      pChan->DoLoad(*m_db,m_iChannelId);
      pChan->Lock();
    }
  }
private:
  int m_iChannelId;

  bool fLocked;
  int iLapId;
  DATA_CHANNEL eChannelType;
  mutable vector<DataPoint> lstData;

  float m_dMin;
  float m_dMax;

  int m_msMin; // start time (milliseconds since app start)
  int m_msMax; // end time (milliseconds since app start)

  bool m_fLazyLoad;
  mutable CSfArtSQLiteDB* m_db;
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
//  Vector2D& GetPt1() const {return m_pt1;}
//  Vector2D& GetPt2() const {return m_pt2;}
//private:		//		Made public by KDJ
  Vector2D m_pt1;
  Vector2D m_pt2;
};

struct CARNUMBERCOMBO
{
  int iCarNumber;
  int iSecondaryCarNumber;
  bool IsOldVersion() const {return iCarNumber == -1 && iSecondaryCarNumber == -1;}
  bool operator <(const CARNUMBERCOMBO& sfOther) const
  {
    return Hash() < sfOther.Hash();
  }
private:
  int Hash() const
  {
    return (iCarNumber<<16) | iSecondaryCarNumber;
  }
};

// a more civilized lap, constructed from InputLapRaw and sent to the user's ILapReceiver
interface ILap
{
public:
  virtual void Load(V1InputLapRaw* pLap) = 0;
  virtual void Load(V2InputLapRaw* pLap) = 0;
  virtual bool Load(CSfArtSQLiteDB& db, StartFinish* rgSF, CSfArtSQLiteQuery& line) = 0;
  virtual void Free() = 0; // delete the lap

  virtual bool IsValid() const = 0;
  virtual int GetStartTime() const = 0;
  virtual int GetLapId() const = 0;
  virtual float GetTime() const = 0;
  virtual const vector<TimePoint2D> GetPoints() const = 0;
  virtual const StartFinish* GetSF() const = 0;
  virtual wstring GetComment() const = 0;
  virtual void SetComment(wstring strComment) const = 0;
  
  virtual CARNUMBERCOMBO GetCarNumbers() const = 0;
private:
};
class CMemoryLap : public ILap
{
public:
	CMemoryLap()
	{

	}
  void Load(V1InputLapRaw* pLap);
  void Load(V2InputLapRaw* pLap);
  bool Load(CSfArtSQLiteDB& db, StartFinish* rgSF, CSfArtSQLiteQuery& line);
  virtual void Free()override {delete this;};

	bool IsValid() const
	{
		return dTime < 3600 && dTime > 3.0 && lstPoints.size() > 0;
	}
  int GetStartTime() const {return iStartTime;} // returns the start time in unix time (seconds since 1970)
  int GetLapId() const {return iLapId;}
  float GetTime() const {return dTime;}
  const vector<TimePoint2D> GetPoints() const {return lstPoints;}
  const StartFinish* GetSF() const {return rgSF;}
  wstring GetComment() const {return strComment;}
  void SetComment(wstring strComment) const override {this->strComment = strComment;}

  virtual CARNUMBERCOMBO GetCarNumbers() const override
  {
    return sfCarNumbers;
  }
private:
	vector<TimePoint2D> lstPoints;
  StartFinish rgSF[3];
  Vector2D vDir[3];
	float dTime;
	int iLapId;
  int iStartTime; // seconds since Jan 1 1970
  mutable wstring strComment;
  CARNUMBERCOMBO sfCarNumbers;
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

struct RACEDATA
{
  RACEDATA()
  {
    unixtime = 0;
    laps = 0;
    raceId = -1;
  }
  wstring strName;
  int unixtime;
  int laps;
  int raceId;
};

interface ILapReceiver
{
public:
  // loading from file
  virtual bool Init(LPCTSTR lpszSQL) = 0;
  virtual bool InitRaceSession(int* iRaceId, LPCTSTR lpszName) = 0;

  // memory management
	virtual ILap* AllocateLap(bool fMemory) = 0;

  virtual IDataChannel* AllocateDataChannel() const = 0;
  virtual void FreeDataChannel(IDataChannel* pChannel) const = 0;

  // data access
  // I chose to access all the laps at once to avoid race condition issues if the network thread updates
  // the databank while the UI is displaying it
  virtual bool IsActivelyReceiving(int iRaceId) const = 0; // returns whether a given raceId is receiving new laps this session
  virtual int GetLastReceivedRaceId() const = 0; // gets the race ID of the last race that received a lap
  virtual void GetLastLapTimeStamp(const vector<int>& lstCarNumbers, vector<unsigned int>& lstTimeStamps) const = 0;
  virtual int GetLapCount(int iRaceId) const = 0; // gets the lap count for a given race
  virtual vector<RACEDATA> GetRaces() = 0;
  virtual vector<const ILap*> GetLaps(int iRaceId) = 0;
  virtual vector<const ILap*> GetScoring(int iRaceId) = 0;
  virtual const ILap* GetLap(int iLapId) = 0;
  virtual const IDataChannel* GetDataChannel(int iLapId, DATA_CHANNEL eChannel) const = 0;
  virtual set<DATA_CHANNEL> GetAvailableChannels(int iLapId) const = 0;
  virtual void GetComments(int iLapId, vector<wstring>& lstComments) const = 0;

  // modifying data
  virtual void AddLap(const ILap* pLap, int iRaceId) = 0;
  virtual void AddDataChannel(const IDataChannel* pChannel) = 0;
  virtual void Clear() = 0;
  virtual void AddComment(int iLapId, LPCTSTR strComment) = 0;

  // status strings
  virtual void NotifyDBArrival(LPCTSTR lpszPath) = 0;
  virtual void SetNetStatus(NETSTATUSSTRING eString, LPCTSTR szData) = 0; // network man tells us the latest status
  virtual LPCTSTR GetNetStatus(NETSTATUSSTRING eString) const = 0;
  virtual bool MergeLaps(int m_iRaceId1, int m_iRaceId2) = 0;
  virtual bool RenameLaps(TCHAR szName[260], int m_RaceId1) = 0;
};

// net-thread entry point.  Fills a ILapReceiver from the network
bool ReceiveLaps(int iPort, ILapReceiver* pLaps);

// fills an ILapReceiver from an SQLite database
void LoadFromSQLite(LPCTSTR lpszSQL, int iRaceId, ILapReceiver* pRecv);
