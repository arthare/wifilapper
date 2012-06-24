#include "ArtHTTPServer.h"
#include "LapPainter.h"

class PitsideHTTP : public ArtHTTPResponder
{
public:
  // since the HTTP guy is essentially a roundabout lap-painter, he should have access to the lap supplier
  PitsideHTTP(const ILapReceiver* pLapsDB, const ILapSupplier* pSupplier);
  virtual ~PitsideHTTP();

  virtual bool MakePage(HTTPREQUEST& strURL, ostream& out) override;

private:
  void WriteLapDataInit(const CLap* pLap, string strArrayName, ostream& out) const; // writes javascript that declares variables and initializes this
  void WriteLapsAvailable(ostream& out) const; // writes javascript that declares variables and initializes this
  void WriteLapDataScript(const CLap* pLap, string strArrayName, ostream& out) const; // writes javascript that fills up data arrays with data time/value pairs
private:
  const ILapSupplier* m_pLapSupplier;
  const ILapReceiver* m_pLapsDB;
};