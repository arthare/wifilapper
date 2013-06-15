#include "ArtHTTPServer.h"
#include "LapPainter.h"

class PitsideHTTP : public ArtHTTPResponder
{
public:
  // since the HTTP guy is essentially a roundabout lap-painter, he should have access to the lap supplier
  PitsideHTTP(ILapReceiver* pLapsDB, const ILapSupplier* pSupplier);
  virtual ~PitsideHTTP();

  virtual bool MakePage(HTTPREQUEST& strURL, ostream& out) override;

private:
  const ILapSupplier* m_pLapSupplier;
  ILapReceiver* m_pLapsDB;
};