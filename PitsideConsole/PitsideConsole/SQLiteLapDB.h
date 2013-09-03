#include "LapReceiver.h"
#include "ArtUI.h"
#include "AutoCS.h"

class CSQLiteLapDB : public ILapReceiver
{
public:
  CSQLiteLapDB(IUI* pUI) : cChannels(0), m_pUI(pUI),m_iLastRaceId(-1) {};
  virtual ~CSQLiteLapDB() {};

  virtual bool Init(LPCTSTR lpszPath) override;
  void DeInit();

  // memory management
	virtual ILap* AllocateLap(bool fMemory) override;

  virtual IDataChannel* AllocateDataChannel() const override;
  virtual void FreeDataChannel(IDataChannel* pChannel) const override;

  // data access
  // I chose to access all the laps at once to avoid race condition issues if the network thread updates
  // the databank while the UI is displaying it
  virtual int GetLastReceivedRaceId() const override; // gets the race ID of the last race that received a lap
  virtual bool IsActivelyReceiving(int iRaceId) const override; // returns whether a given raceId is receiving new laps this session
  virtual void GetLastLapTimeStamp(const vector<int>& lstCarNumbers, vector<unsigned int>& lstTimeStamps) const;
  virtual int GetLapCount(int iRaceId) const override; // gets the lap count for a given race
  virtual vector<RACEDATA> GetRaces() override;
  virtual vector<const ILap*> GetLaps(int iRaceId) override;
  virtual vector<const ILap*> GetScoring(int iRaceId) override;
  virtual bool MergeLaps(int m_iRaceId1, int m_iRaceId2);
  virtual bool RenameLaps(TCHAR szName[260], int m_RaceId1);
  virtual const ILap* GetLap(int iLapId) override;
  virtual const IDataChannel* GetDataChannel(int iLapId, DATA_CHANNEL eChannel) const override;
  virtual set<DATA_CHANNEL> GetAvailableChannels(int iLapId) const override;
  virtual void GetComments(int iLapId, vector<wstring>& lstComments) const override;

  // modifying data
	virtual void AddLap(const ILap* pLap, int iRaceId) override;
  virtual void AddDataChannel(const IDataChannel* pChannel) override;
  virtual void Clear() override;
  virtual void AddComment(int iLapId, LPCTSTR strComment) override;

  virtual void NotifyDBArrival(LPCTSTR lpszPath) override;
  virtual void SetNetStatus(NETSTATUSSTRING eString, LPCTSTR szData) override; // network man tells us the latest status
  virtual LPCTSTR GetNetStatus(NETSTATUSSTRING eString) const override;
private:
  virtual bool InitRaceSession(int* piRaceId, LPCTSTR lpszRaceName) override;
private:
  mutable ManagedCS m_cs;

  mutable CSfArtSQLiteDB m_sfDB;
//  StartFinish m_rgSF[3];
  StartFinish m_rgSF[50];	//	Increased by KDJ

  set<int> m_setReceivingIds; // which race IDs have actually received laps from afar this session?
  int m_iLastRaceId; // what was the last race ID to receive a lap?

  mutable int cChannels;

  IUI* m_pUI;
  TCHAR szLastNetStatus[NETSTATUS_COUNT][200];
  map<CARNUMBERCOMBO,int> mapCarNumberRaceIds; // a mapping from car numbers to the raceIDs we assign for them.
  map<int,unsigned int> mapLastRaceTimes; // remembers the timestamp for the last lap received for any race
};