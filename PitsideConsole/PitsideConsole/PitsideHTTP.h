#include "ArtHTTPServer.h"
#include "LapReceiver.h"

class PitsideHTTP : public ArtHTTPResponder
{
public:
  PitsideHTTP(const ILapReceiver* pReceiver);
  virtual ~PitsideHTTP();

  virtual bool MakePage(HTTPREQUEST& strURL, ostream& out) override;

private:
  void WriteLapDataScript(const CLap* pLap, string strArrayName, ostream& out) const;
private:
  const ILapReceiver* m_pLapsDB;
};