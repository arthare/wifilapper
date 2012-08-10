#include "LapReceiver.h"

class CSQLiteLapDB : public ILapReceiver
{
public:
  CSQLiteLapDB(IUI* pUI) : m_iReceiveId(-1),cChannels(0), m_pUI(pUI) {};
  virtual ~CSQLiteLapDB() {};

  virtual bool Init(LPCTSTR lpszPath) override;
  virtual bool InitRaceSession(int* piRaceId, LPCTSTR lpszRaceName) override;
  void DeInit();

  // memory management
	virtual ILap* AllocateLap(bool fMemory) override;

  virtual IDataChannel* AllocateDataChannel() const override;
  virtual void FreeDataChannel(IDataChannel* pChannel) const override;

  // data access
  // I chose to access all the laps at once to avoid race condition issues if the network thread updates
  // the databank while the UI is displaying it
  virtual vector<RACEDATA> GetRaces() override;
  virtual vector<const ILap*> GetLaps(int iRaceId) override;
  virtual const ILap* GetLap(int iLapId) override;
  virtual const IDataChannel* GetDataChannel(int iLapId, DATA_CHANNEL eChannel) const override;
  virtual set<DATA_CHANNEL> GetAvailableChannels(int iLapId) const override;

  // modifying data
	virtual void AddLap(const ILap* pLap, int iRaceId) override;
  virtual void AddDataChannel(const IDataChannel* pChannel) override;
  virtual void Clear() override;

  // status strings
  virtual void SetReceiveId(int iRaceId) {m_iReceiveId = iRaceId;}
  virtual void NotifyDBArrival(LPCTSTR lpszPath) override;
  virtual void SetNetStatus(NETSTATUSSTRING eString, LPCTSTR szData) override; // network man tells us the latest status
  virtual LPCTSTR GetNetStatus(NETSTATUSSTRING eString) const override;
private:
  mutable CSfArtSQLiteDB m_sfDB;
  StartFinish m_rgSF[3];

  mutable int cChannels;

  IUI* m_pUI;
  int m_iReceiveId; // what race ID should we apply to any negative-raceid laps we receive in AddLap()?
  TCHAR szLastNetStatus[NETSTATUS_COUNT][200];
};