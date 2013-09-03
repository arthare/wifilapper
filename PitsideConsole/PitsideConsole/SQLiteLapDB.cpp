#include "stdafx.h"
#include "LapReceiver.h"
#include "PitsideConsole.h"
#include "ArtUI.h"
#include "SQLiteLapDB.h"
#include "LapData.h"
class CSQLiteLap : public ILap
{
public:
  CSQLiteLap(CSfArtSQLiteDB& sfDB, ILapReceiver* pWriteBack) : m_sfDB(sfDB),m_pWriteBack(pWriteBack), m_fLoaded(false) {};
  virtual ~CSQLiteLap() {};

public: // ILap overrides
  virtual void Load(V1InputLapRaw* pLap) override
  {
    DASSERT(false); // this should never be called - memory laps should be loaded instead.
  }
  virtual void Load(V2InputLapRaw* pLap) override
  {
    DASSERT(false); // this should never be called - memory laps should be loaded instead.
  }
  virtual bool Load(CSfArtSQLiteDB& db, StartFinish* rgSF, CSfArtSQLiteQuery& line) override
  {
    bool fSuccess = true;
    fSuccess &= (line.GetCol(0, &m_iLapId));
    fSuccess &= (line.GetCol(1, &m_flLapTime));
    fSuccess &= (line.GetCol(2, &m_iStartTime));

    TCHAR szComment[200];
    szComment[0] = 0;
    fSuccess &= (line.GetCol(3, szComment, NUMCHARS(szComment)));
    m_strComment = wstring(szComment);

    memcpy(this->m_rgSF, rgSF, sizeof(this->m_rgSF));

    return fSuccess;
  }
  virtual void Free()override {delete this;};

  virtual bool IsValid() const override {return m_flLapTime < 3600 && m_flLapTime > 3.0 && m_iLapId != -1;};
  virtual int GetStartTime() const override {return m_iStartTime;}
  virtual int GetLapId() const override {return m_iLapId;}
  virtual float GetTime() const override {return m_flLapTime;}
  virtual wstring GetComment() const override {return m_strComment;}
  virtual void SetComment(wstring strComment) const override 
  {
    m_strComment = strComment;
    m_pWriteBack->AddComment(m_iLapId,strComment.c_str());
  }
  virtual const vector<TimePoint2D> GetPoints() const override
  {
    vector<TimePoint2D> lstPoints;
    bool fSuccess = false;
    // they're asking for points.  This is where stuff gets expensive
    // now we need to get all the datapoints for this lap (data channels will come later)
    TCHAR szQuery[MAX_PATH];
    _snwprintf(szQuery, NUMCHARS(szQuery), L"select points.x,points.y,points.time,points.velocity from points where points.lapid = %d", m_iLapId);
    CSfArtSQLiteQuery sfPointQuery(m_sfDB);
    if(sfPointQuery.Init(szQuery))
    {
      while(sfPointQuery.Next())
      {
        TimePoint2D pt;
        fSuccess &= sfPointQuery.GetCol(0,&pt.flX);
        fSuccess &= sfPointQuery.GetCol(1,&pt.flY);
        fSuccess &= sfPointQuery.GetCol(2,&pt.iTime);
        fSuccess &= sfPointQuery.GetCol(3,&pt.flVelocity);
        pt.flSum = pt.flX + pt.flY;
        DASSERT(pt.IsValid());
        if(pt.IsValid())
        {
          lstPoints.push_back(pt);
        }
      }
    }
    return lstPoints;
  }
  virtual const StartFinish* GetSF() const override {return m_rgSF;}
  virtual CARNUMBERCOMBO GetCarNumbers() const override
  {
    DASSERT(FALSE); // nobody should be calling this on an SQLite lap
    CARNUMBERCOMBO sfCars;
    sfCars.iCarNumber = -1;
    sfCars.iSecondaryCarNumber = -1;
    return sfCars;
  }
public: // general CSQLiteLap functioning

private:
  CSfArtSQLiteDB& m_sfDB;
  mutable ILapReceiver* m_pWriteBack;

  bool m_fLoaded;

  int m_iStartTime;
  int m_iLapId;
  float m_flLapTime;
  mutable wstring m_strComment;
//  StartFinish m_rgSF[3];
  StartFinish m_rgSF[50];	//		Increased by KDJ
};

//////////////////////////////////////////////////////////////
static LPCTSTR CREATE_RACE_SQL_V20 =  L"create table races (	_id integer primary key asc autoincrement, " \
																						L"\"name\" string, " \
																						L"\"date\" string, " \
																						L"\"testmode\" integer, " \
																						L"x1 real, " \
																						L"y1 real, " \
																						L"x2 real, " \
																						L"y2 real, " \
																						L"x3 real, " \
																						L"y3 real, " \
																						L"x4 real, " \
																						L"y4 real, " \
																						L"x5 real, " \
																						L"y5 real, " \
																						L"x6 real, " \
																						L"y6 real, " \
																						L"vx1 real," \
																						L"vy1 real," \
																						L"vx2 real," \
																						L"vy2 real," \
																						L"vx3 real," \
																						L"vy3 real)";

static LPCTSTR CREATE_RACE_SQL_V21 =  L"create table races (	_id integer primary key asc autoincrement, " \
																						L"\"name\" string, " \
																						L"\"date\" string, " \
																						L"\"testmode\" integer, " \
																						L"x1 real, " \
																						L"y1 real, " \
																						L"x2 real, " \
																						L"y2 real, " \
																						L"x3 real, " \
																						L"y3 real, " \
																						L"x4 real, " \
																						L"y4 real, " \
																						L"x5 real, " \
																						L"y5 real, " \
																						L"x6 real, " \
																						L"y6 real, " \
																						L"vx1 real," \
																						L"vy1 real," \
																						L"vx2 real," \
																						L"vy2 real," \
																						L"vx3 real," \
																						L"vy3 real," \
                                            L"p2p integer not null default 0)";

static LPCTSTR CREATE_RACE_SQL_V22 =  L"create table races (	_id integer primary key asc autoincrement, " \
																						L"\"name\" string, " \
																						L"\"date\" string, " \
																						L"\"testmode\" integer, " \
																						L"x1 real, " \
																						L"y1 real, " \
																						L"x2 real, " \
																						L"y2 real, " \
																						L"x3 real, " \
																						L"y3 real, " \
																						L"x4 real, " \
																						L"y4 real, " \
																						L"x5 real, " \
																						L"y5 real, " \
																						L"x6 real, " \
																						L"y6 real, " \
																						L"vx1 real," \
																						L"vy1 real," \
																						L"vx2 real," \
																						L"vy2 real," \
																						L"vx3 real," \
																						L"vy3 real," \
                                            L"p2p integer not null default 0," \
                                            L"finishcount integer not null default 1)";
	
static LPCTSTR CREATE_LAPS_SQL = L"create table laps " \
												L"(_id integer primary key asc autoincrement, " \
												L"laptime real, " \
												L"unixtime integer, " \
												L"transmitted integer, " \
												L"raceid integer," \
												L"foreign key (raceid) references races(_id))";
	
static LPCTSTR CREATE_POINTS_SQL = L"create table points " \
												L"(_id integer primary key asc autoincrement, " \
												L"x real," \
												L"y real," \
												L"time integer," \
												L"velocity real," \
												L"lapid integer," \
												L"foreign key (lapid) references laps(_id))";
	
static LPCTSTR CREATE_CHANNELS_SQL = L"create table channels" \
													L"(_id integer primary key asc autoincrement, " \
													L"lapid integer NOT NULL," \
													L"channeltype integer NOT NULL," \
													L"foreign key(lapid) references laps(_id))";
	
static LPCTSTR CREATE_DATA_SQL =L"create table data " \
														L"(_id integer primary key asc autoincrement," \
														L"time integer NOT NULL," \
														L"value real NOT NULL," \
														L"channelid integer NOT NULL," \
														L"foreign key (channelid) references channels(_id))";

static LPCTSTR CREATE_EXTRA_SQL =L"create table extras " \
														L"(_id integer primary key asc autoincrement," \
														L"comment string," \
                            L"lapid integer NOT NULL unique on conflict fail," \
														L"foreign key (lapid) references laps(_id))";

static LPCTSTR CREATE_INDICES =L"create index data_channelid on data(channelid);" \
											L"create index if not exists points_lapid on points(lapid);" \
											L"create index if not exists laps_raceid on laps(raceid)";

static LPCTSTR rgRequiredTables20[] = {CREATE_RACE_SQL_V20, CREATE_LAPS_SQL, CREATE_POINTS_SQL, CREATE_CHANNELS_SQL, CREATE_DATA_SQL, NULL};
static LPCTSTR rgRequiredTables21[] = {CREATE_RACE_SQL_V21, CREATE_LAPS_SQL, CREATE_POINTS_SQL, CREATE_CHANNELS_SQL, CREATE_DATA_SQL, NULL};
static LPCTSTR rgRequiredTables23[] = {CREATE_RACE_SQL_V22, CREATE_LAPS_SQL, CREATE_POINTS_SQL, CREATE_CHANNELS_SQL, CREATE_DATA_SQL, CREATE_EXTRA_SQL};

static LPCTSTR* rgSchemaList[] = 
{
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables20,
  rgRequiredTables21,
  rgRequiredTables21,
  rgRequiredTables23
};

const static int iMinSupportedVersion = 0;
const static int iMaxSupportedVersion = 23;
//////////////////////////////////////////////////////////////
bool CSQLiteLapDB::Init(LPCTSTR lpszPath)
{
  if(!DoesFileExist(lpszPath))
  {
    vector<wstring> lstSQL;
    HRESULT hr = m_sfDB.Open(lpszPath, lstSQL);
    if(SUCCEEDED(hr))
    {
      for(int x = 0;x < 6; x++)
      {
        hr = E_FAIL;
        CSfArtSQLiteQuery sfInsert(m_sfDB);
        if(sfInsert.Init(rgSchemaList[iMaxSupportedVersion][x]))
        {
          if(sfInsert.Next())
          {
            hr = S_OK;
          }
          if(sfInsert.IsDone())
          {
            hr = S_OK;
          }
        }
        if(FAILED(hr))
          break;
      }
    }
    return SUCCEEDED(hr);
  }
  else
  {
    int iFindFailures = 0;
    int ixFoundVersion = -1;
    vector<wstring> lstSQL;
    HRESULT hr = m_sfDB.Open(lpszPath, lstSQL);
    if(SUCCEEDED(hr))
    {
      // find a schema version that matches
      for(int ixVersion = iMaxSupportedVersion; ixVersion >= iMinSupportedVersion; ixVersion--)
      {
        bool fFoundAll = true;
        for(int x = 0; x < 6 && fFoundAll; x++)
        {
          // we want to find rgSchemaList[ixVersion][x] in the lstSQL that we got from the DB.
          if(!rgSchemaList[ixVersion][x]) continue; // don't need to find this one

          bool fFound = false;
          for(int y = 0; y < lstSQL.size(); y++)
          {
            if(wcsstr(lstSQL[y].c_str(),L"android_metadata") != NULL) continue; // don't care about this table
            if(nospacecompare(lstSQL[y].c_str(),rgSchemaList[ixVersion][x]) == 0)
            {
              fFound = true;
              break;
            }
          }
          if(!fFound)
          {
            iFindFailures++;
          }
        
          fFoundAll = fFoundAll && fFound;
        }
        if(fFoundAll)
        {
          ixFoundVersion = ixVersion;
          break;
        }
      }

      if(ixFoundVersion < 0)
      {
        DASSERT(FALSE);
        // we didn't find or create all our required tables...
        hr = E_FAIL;
      }


      if(SUCCEEDED(hr) && ixFoundVersion < 23)
      {
        // upgrade!
        CSfArtSQLiteQuery sfQuery(m_sfDB);
        if(sfQuery.Init(CREATE_EXTRA_SQL))
        {
          while(sfQuery.Next())
          {
          }
          DASSERT(sfQuery.IsDone());
        }
      }
      /*if(ixFoundVersion < ...) future upgrades...
      {
      }*/
    }
    return SUCCEEDED(hr);
  }
  
}
//////////////////////////////////////////////////////////////
bool CSQLiteLapDB::InitRaceSession(int* piRaceId, LPCTSTR lpszRaceName)
{
  AutoLeaveCS _cs(&m_cs);
  bool fSuccess = true;
  CSfArtSQLiteQuery sfQuery(m_sfDB);
  if(sfQuery.Init(L"insert into races (name,date) values (?,?)"))
  {
    sfQuery.BindValue(lpszRaceName);
    sfQuery.BindValue(GetSecondsSince1970());
    if(sfQuery.Next() || sfQuery.IsDone())
    {
      // inserted new data, hooray!
      long long lastInsert = m_sfDB.GetLastInsertId();
      *piRaceId = lastInsert;
      return true;
    }
  }
  return false;
}
//////////////////////////////////////////////////////////////
void CSQLiteLapDB::DeInit()
{
  AutoLeaveCS _cs(&m_cs);
  m_sfDB.Close();
}
//////////////////////////////////////////////////////////////
ILap* CSQLiteLapDB::AllocateLap(bool fMemory)
{
  if(fMemory)
  {
    return new CMemoryLap();
  }
  else
  {
    return new CSQLiteLap(m_sfDB, this);
  }
}
//////////////////////////////////////////////////////////////
IDataChannel* CSQLiteLapDB::AllocateDataChannel() const
{
  AutoLeaveCS _cs(&m_cs);
  cChannels++;
  return new CDataChannel();
}
//////////////////////////////////////////////////////////////
void CSQLiteLapDB::FreeDataChannel(IDataChannel* pChannel) const
{
  AutoLeaveCS _cs(&m_cs);
  cChannels--;
  delete pChannel;
}
//////////////////////////////////////////////////////////////
int CSQLiteLapDB::GetLastReceivedRaceId() const
{
  AutoLeaveCS _cs(&m_cs);
  DASSERT(m_iLastRaceId >= 0); // you shouldn't be calling this unless there's evidence we've received a lap!
  return m_iLastRaceId;
}
//////////////////////////////////////////////////////////////
bool CSQLiteLapDB::IsActivelyReceiving(int iRaceId) const
{
  AutoLeaveCS _cs(&m_cs);
  return this->m_setReceivingIds.find(iRaceId) != m_setReceivingIds.end();
}
//////////////////////////////////////////////////////////////
void CSQLiteLapDB::GetLastLapTimeStamp(const vector<int>& lstCarNumbers, vector<unsigned int>& lstTimeStamps) const
{
  AutoLeaveCS _cs(&m_cs);
  for(int x = 0;x < lstCarNumbers.size(); x++)
  {
    const int iCar = lstCarNumbers[x];
    unsigned int msLatestTime = 0;
    {
      for(map<CARNUMBERCOMBO,int>::const_iterator i = mapCarNumberRaceIds.begin(); i != mapCarNumberRaceIds.end(); i++)
      {
        if(i->first.iCarNumber == iCar)
        {
          const int iRaceId = i->second;
          map<int,unsigned int>::const_iterator found = mapLastRaceTimes.find(iRaceId);
          if(found != mapLastRaceTimes.end())
          {
            const int msLastLapTime = found->second;
            msLatestTime = max(msLatestTime, msLastLapTime); // figures out which one came later
          }
          else
          {
            // nothing to do, this race doesn't have a last lap time
          }
        }
      }
      lstTimeStamps.push_back(msLatestTime);
    }
  }
}
//////////////////////////////////////////////////////////////
vector<const ILap*> CSQLiteLapDB::GetScoring(int iRaceId)
{
  AutoLeaveCS _cs(&m_cs);
  vector<const ILap*> lstLaps;
  // gotta load all the laps that are in the DB, but we don't want to fully load them, just their laptimes and other directly lap-related data
  CSfArtSQLiteQuery sfQuery(m_sfDB);
  TCHAR szQuery[MAX_PATH];
  _snwprintf(szQuery, NUMCHARS(szQuery), L"Select laps._id,laps.laptime, laps.unixtime,extras.comment from laps left join extras on extras.lapid = laps._id where laps.raceid=%d", iRaceId);
  if(sfQuery.Init(szQuery))
  {
    while(sfQuery.Next())
    {
      CSQLiteLap* pLap = (CSQLiteLap*)AllocateLap(false);
      pLap->Load(m_sfDB,m_rgSF,sfQuery);
      if(pLap->IsValid())
      {
        lstLaps.push_back(pLap);
      }
    }
  }
  return lstLaps;
}
//////////////////////////////////////////////////////////////
int CSQLiteLapDB::GetLapCount(int iRaceId) const
{
  AutoLeaveCS _cs(&m_cs);
  CSfArtSQLiteQuery sfQuery(m_sfDB);
  TCHAR szQuery[MAX_PATH];
  _snwprintf(szQuery, NUMCHARS(szQuery), L"select count(laps._id) from races,laps where races._id = %d and laps.raceid=races._id group by races._id", iRaceId);

  if(sfQuery.Init(szQuery))
  {
    while(sfQuery.Next())
    {
      int cLaps = 0;
      if(sfQuery.GetCol(0,&cLaps))
      {
        return cLaps;
      }
    }
  }
  return 0;
}
//////////////////////////////////////////////////////////////
vector<RACEDATA> CSQLiteLapDB::GetRaces()
{
  AutoLeaveCS _cs(&m_cs);
  vector<RACEDATA> lstRaces;
  CSfArtSQLiteQuery sfQuery(m_sfDB);
  TCHAR szQuery[MAX_PATH];
  _snwprintf(szQuery, NUMCHARS(szQuery), L"select races.name,races.date,count(laps._id),races._id from races,laps where laps.raceid=races._id group by races._id");

  if(sfQuery.Init(szQuery))
  {
    while(sfQuery.Next())
    {
      RACEDATA raceData;
      TCHAR szData[1000];
      if(sfQuery.GetCol(0,szData,NUMCHARS(szData)))
      {
        raceData.strName = szData;
        int iRaceDate = 0;
        if(sfQuery.GetCol(1,&iRaceDate))
        {
          raceData.unixtime = iRaceDate;
        }
        int cLaps = 0;
        if(sfQuery.GetCol(2,&cLaps))
        {
          raceData.laps = cLaps;
        }
        int iRaceId = 0;
        if(sfQuery.GetCol(3,&iRaceId))
        {
          raceData.raceId = iRaceId;
        }
        lstRaces.push_back(raceData);
      }
    }
  }
  return lstRaces;
}
//////////////////////////////////////////////////////////////
bool CSQLiteLapDB::MergeLaps(int m_iRaceId1, int m_iRaceId2)
{
  CSfArtSQLiteQuery sfQuery(m_sfDB);
  if(sfQuery.Init(L"Update laps set raceid = ? where raceid = ?"))
  {
    sfQuery.BindValue(m_iRaceId1);
    sfQuery.BindValue(m_iRaceId2);
    if(sfQuery.Next() || sfQuery.IsDone())
    {
      // data update complete, hooray!
      return true;
    }
  }
  return false;
}
//////////////////////////////////////////////////////////////
bool CSQLiteLapDB::RenameLaps(TCHAR szName[MAX_PATH], int m_iRaceId1)
{
  CSfArtSQLiteQuery sfQuery(m_sfDB);
  if(sfQuery.Init(L"update races set name = ? where _id = ?"))
  {
    sfQuery.BindValue(szName);
    sfQuery.BindValue(m_iRaceId1);
    if(sfQuery.Next() || sfQuery.IsDone())
    {
      // data update complete, hooray!
      return true;
    }
  }
  return false;
}
//////////////////////////////////////////////////////////////
vector<const ILap*> CSQLiteLapDB::GetLaps(int iRaceId)
{
  AutoLeaveCS _cs(&m_cs);
  vector<const ILap*> lstLaps;
  // gotta load all the laps that are in the DB, but we don't want to fully load them, just their laptimes and other directly lap-related data
  CSfArtSQLiteQuery sfQuery(m_sfDB);
  TCHAR szQuery[MAX_PATH];
  _snwprintf(szQuery, NUMCHARS(szQuery), L"Select laps._id,laps.laptime, laps.unixtime,extras.comment from laps left join extras on extras.lapid = laps._id where laps.raceid=%d", iRaceId);
  if(sfQuery.Init(szQuery))
  {
    while(sfQuery.Next())
    {
      CSQLiteLap* pLap = (CSQLiteLap*)AllocateLap(false);
      pLap->Load(m_sfDB,m_rgSF,sfQuery);
      if(pLap->IsValid())
      {
        lstLaps.push_back(pLap);
      }
    }
  }
  return lstLaps;
}
//////////////////////////////////////////////////////////////
const ILap* CSQLiteLapDB::GetLap(int iLapId)
{
  AutoLeaveCS _cs(&m_cs);
  CSfArtSQLiteQuery sfQuery(m_sfDB);
  TCHAR szQuery[MAX_PATH];
  _snwprintf(szQuery, NUMCHARS(szQuery), L"Select laps._id,laps.laptime, laps.unixtime from laps where laps._id = %d", iLapId);
  if(sfQuery.Init(szQuery))
  {
    if(sfQuery.Next())
    {
      CSQLiteLap* pLap = (CSQLiteLap*)AllocateLap(false);
      pLap->Load(m_sfDB,m_rgSF,sfQuery);

      return pLap;
    }
  }
  return NULL;
}
//////////////////////////////////////////////////////////////
const IDataChannel* CSQLiteLapDB::GetDataChannel(int iLapId, DATA_CHANNEL eChannel) const
{
  AutoLeaveCS _cs(&m_cs);
  if(eChannel == DATA_CHANNEL_VELOCITY)
  {
    // since velocity is stored as part of the points table, it is a bit different than the normal data channels
    CSfArtSQLiteQuery sfQuery(m_sfDB);
    TCHAR szQuery[MAX_PATH];
    _snwprintf(szQuery, NUMCHARS(szQuery), L"select points.time, points.velocity from points where lapid=%d", iLapId);
    if(sfQuery.Init(szQuery))
    {
      IDataChannel* pChannel = AllocateDataChannel();
      pChannel->Init(iLapId,DATA_CHANNEL_VELOCITY);
      while(sfQuery.Next())
      {
        int iTime = 0;
        double dVel = 0;
        if(sfQuery.GetCol(0,&iTime) && sfQuery.GetCol(1,&dVel))
        {
          pChannel->AddPoint(iTime, dVel);
        }
      }
      pChannel->Lock();
      return pChannel;
    }
  }
  else
  {
    CSfArtSQLiteQuery sfQuery(m_sfDB);
    TCHAR szQuery[MAX_PATH];
    _snwprintf(szQuery, NUMCHARS(szQuery), L"select channels._id, channels.lapid, channels.channeltype from channels where channels.lapid = %d and channels.channeltype=%d", iLapId, eChannel);
    if(sfQuery.Init(szQuery))
    {
      if(sfQuery.Next())
      {
        IDataChannel* pChannel = AllocateDataChannel();
        pChannel->Load(m_sfDB,sfQuery,true);
        // warning: massive memory leaks here
        return pChannel;
      }
    }
  }
  return NULL;
}
//////////////////////////////////////////////////////////////
set<DATA_CHANNEL> CSQLiteLapDB::GetAvailableChannels(int iLapId) const
{
  AutoLeaveCS _cs(&m_cs);
  set<DATA_CHANNEL> setRet;

  CSfArtSQLiteQuery sfQuery(m_sfDB);
  TCHAR szQuery[MAX_PATH];
  _snwprintf(szQuery, NUMCHARS(szQuery), L"select channels.channeltype from channels where channels.lapid = %d", iLapId);
  if(sfQuery.Init(szQuery))
  {
    while(sfQuery.Next())
    {
      DATA_CHANNEL eType;
      if(sfQuery.GetCol(0,(int*)&eType))
      {
        setRet.insert(eType);
      }
    }
  }
  return setRet;
}
//////////////////////////////////////////////////////////////
void CSQLiteLapDB::GetComments(int iLapId, vector<wstring>& lstComments) const
{
  AutoLeaveCS _cs(&m_cs);
  CSfArtSQLiteQuery sfQuery(m_sfDB);
  TCHAR szQuery[MAX_PATH];
  _snwprintf(szQuery, NUMCHARS(szQuery), L"select extras.comment from extras where extras.lapid = %d", iLapId);
  if(sfQuery.Init(szQuery))
  {
    while(sfQuery.Next())
    {
      TCHAR szComment[500];
      if(sfQuery.GetCol(0,szComment, NUMCHARS(szComment)))
      {
        lstComments.push_back(szComment);
      }
    }
  }
}
//////////////////////////////////////////////////////////////
void CSQLiteLapDB::AddLap(const ILap* pLap, int _iRaceId)
{
  AutoLeaveCS _cs(&m_cs);
  CARNUMBERCOMBO sfCarNumber = pLap->GetCarNumbers();
  int iSaveRaceId = -1;
  if(mapCarNumberRaceIds.find(sfCarNumber) == mapCarNumberRaceIds.end())
  {
    // we've never seen this car number combo before.  Time for a new race.
    TCHAR szRaceName[200];
    if(sfCarNumber.IsOldVersion())
    {
      _snwprintf(szRaceName,NUMCHARS(szRaceName),L"Received from old wifilappers");
    }
    else
    {
      _snwprintf(szRaceName,NUMCHARS(szRaceName),L"Received laps (car %d) - (%d)", sfCarNumber.iCarNumber, sfCarNumber.iSecondaryCarNumber);
    }
    int iNewRaceId = -1;
    if(InitRaceSession(&iNewRaceId,szRaceName))
    {
      mapCarNumberRaceIds[sfCarNumber] = iNewRaceId;
      iSaveRaceId = iNewRaceId;
    }
    else
    {
      return;
    }
  }
  else
  {
    iSaveRaceId = mapCarNumberRaceIds[sfCarNumber];
  }

  m_setReceivingIds.insert(iSaveRaceId); // this race ID received a lap
  m_iLastRaceId = iSaveRaceId;

  m_sfDB.StartTransaction();

  bool fSuccess = true;
  CSfArtSQLiteQuery sfQuery(m_sfDB);
  if(sfQuery.Init(L"insert into laps (_id,laptime,unixtime,transmitted,raceid) values (?,?,?,?,?)"))
  {
    sfQuery.BindValue(pLap->GetLapId());
    sfQuery.BindValue(pLap->GetTime());
    sfQuery.BindValue(pLap->GetStartTime());
    sfQuery.BindValue(0);
    sfQuery.BindValue(iSaveRaceId);
    if(sfQuery.Next() || sfQuery.IsDone())
    {
      // inserted new data, hooray!

      vector<TimePoint2D> lstPoints = pLap->GetPoints();
      for(int x = 0;x < lstPoints.size(); x++)
      {
        const TimePoint2D& pt = lstPoints[x];
        sfQuery.DeInit();
        if(sfQuery.Init(L"insert into points (x,y,time,velocity,lapid) values (?,?,?,?,?)"))
        {
          sfQuery.BindValue(pt.flX);
          sfQuery.BindValue(pt.flY);
          sfQuery.BindValue(pt.iTime);
          sfQuery.BindValue(pt.flVelocity);
          sfQuery.BindValue(pLap->GetLapId());
          if(!sfQuery.Next() && !sfQuery.IsDone())
          {
            fSuccess = false;
            break;
          }
        }
      }
    }
    else
    {
      fSuccess = false;
    }
  }

  if(fSuccess)
  {
    mapLastRaceTimes[iSaveRaceId] = timeGetTime();
    m_pUI->NotifyChange(NOTIFY_NEWLAP,(LPARAM)this);
  }

  m_sfDB.StopTransaction();
}
//////////////////////////////////////////////////////////////
void CSQLiteLapDB::AddDataChannel(const IDataChannel* pChannel)
{
  AutoLeaveCS _cs(&m_cs);
  m_sfDB.StartTransaction();

  bool fSuccess = true;
  CSfArtSQLiteQuery sfQuery(m_sfDB);
  if(sfQuery.Init(L"insert into channels (lapid,channeltype) values (?,?)"))
  {
    sfQuery.BindValue(pChannel->GetLapId());
    sfQuery.BindValue((int)pChannel->GetChannelType());
    if(sfQuery.Next() || sfQuery.IsDone())
    {
      // inserted new data, hooray!
      long long lastInsert = m_sfDB.GetLastInsertId();


      vector<DataPoint> lstPoints = pChannel->GetData();
      for(int x = 0;x < lstPoints.size(); x++)
      {
        const DataPoint& pt = lstPoints[x];
        sfQuery.DeInit();
        if(sfQuery.Init(L"insert into data (time,value,channelid) values (?,?,?)"))
        {
          sfQuery.BindValue(pt.iTimeMs);
          sfQuery.BindValue(pt.flValue);
          sfQuery.BindValue(lastInsert);
          if(!sfQuery.Next() && !sfQuery.IsDone())
          {
            fSuccess = false;
            break;
          }
        }
      }
    }
    else
    {
      fSuccess = false;
    }
  }
  if(fSuccess)
  {
    m_pUI->NotifyChange(NOTIFY_NEWDATA,(LPARAM)this);
  }

  m_sfDB.StopTransaction();
}
//////////////////////////////////////////////////////////////
void CSQLiteLapDB::Clear()
{
}
//////////////////////////////////////////////////////////////
void CSQLiteLapDB::AddComment(int iLapId, LPCTSTR strComment)
{
  CSfArtSQLiteQuery sfQuery(m_sfDB);
  if(sfQuery.Init(L"insert or replace into extras (comment,lapid) values (?,?)"))
  {
    sfQuery.BindValue(strComment);
    sfQuery.BindValue(iLapId);
    if(!sfQuery.Next() && !sfQuery.IsDone())
    {
      DASSERT(FALSE);
    }
  }
}
//////////////////////////////////////////////////////////////
void CSQLiteLapDB::SetNetStatus(NETSTATUSSTRING eString, LPCTSTR sz)
{
  AutoLeaveCS _cs(&m_cs);
  wcscpy(szLastNetStatus[eString], sz);
  m_pUI->NotifyChange(NOTIFY_NEWNETSTATUS,(LPARAM)this);
}
//////////////////////////////////////////////////////////////
void CSQLiteLapDB::NotifyDBArrival(LPCTSTR szPath)
{
  AutoLeaveCS _cs(&m_cs);
  wcscpy(szLastNetStatus[NETSTATUS_DB],szPath);
  m_pUI->NotifyChange(NOTIFY_NEWDATABASE,(LPARAM)szLastNetStatus[NETSTATUS_DB]);
}
//////////////////////////////////////////////////////////////
LPCTSTR CSQLiteLapDB::GetNetStatus(NETSTATUSSTRING eString) const 
{
  return szLastNetStatus[eString];
}