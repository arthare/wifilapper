#pragma once

#include "stdafx.h"
#include "WinSock2.h"
#include <vector>
#include <iostream>
#include <windows.h>
#include "LapReceiver.h"
#include "ArtVector.h"
#include "ArtTools.h"
#include "LapData.h"
#include <algorithm>
#include "DlgProgress.h"
#include "dlgmessage.h"
#include "zlib.h"

using namespace std;

// sorts a vector of DataPoints by time
bool DataChannelSorter (const DataPoint& pt1, const DataPoint& pt2) 
{
  return pt1.iTimeMs < pt2.iTimeMs;
}

CDataChannel::CDataChannel() : iLapId(-1),fLocked(false),eChannelType(DATA_CHANNEL_COUNT)
{
};
CDataChannel::~CDataChannel() 
{
  m_mapCachedData.clear();
  lstData.clear();
}
void CDataChannel::Load(InputChannelRaw* pData)
{
  DASSERT(!fLocked);
  if(fLocked) return;

  FLIP(pData->iLapId);
  FLIP(pData->eChannelType);
  FLIP(pData->cPoints);

  iLapId = pData->iLapId;
  eChannelType=  (DATA_CHANNEL)pData->eChannelType;

  int cPoints = pData->cPoints;
  for(int x = 0;x < cPoints; x++)
  {
    DataPoint& pt = pData->rgPoints[x];
    pt.flValue = FLIPBITS(pt.flValue);
    pt.iTimeMs = FLIPBITS(pt.iTimeMs);
    lstData.push_back(pt);
  }
}
bool CDataChannel::LoadZipped(const char* pszData, int cbData)
{
  bool fRet = false;

  int cbUncomp = min(10000,cbData*100);
  Bytef* pbUncompressed = new Bytef[cbUncomp];
  if(pbUncompressed)
  {
    uLongf outLen = cbUncomp;
    int iRet = uncompress(pbUncompressed, &outLen, (const Bytef*)pszData, cbData);
    if(iRet == 0)
    {
      // successful decompress!
      InputChannelRaw* pData = (InputChannelRaw*)pbUncompressed;
      Load(pData);
      fRet = true;
    }

    delete[] pbUncompressed;
  }
  return fRet;
}
bool CDataChannel::Load(CSfArtSQLiteDB& db, CSfArtSQLiteQuery& dc)
{
  DASSERT(!fLocked);
  if(fLocked) return false;

  // _id = 0
  // lapid = 1
  // channeltype = 2

  int _id = 0;
  if(!dc.GetCol(0,&_id)) return false;
  if(!dc.GetCol(1,&iLapId)) return false;
  if(!dc.GetCol(2,(int*)&eChannelType)) return false;

  CSfArtSQLiteQuery sfData(db);
  TCHAR szQuery[MAX_PATH];

  _snwprintf(szQuery,NUMCHARS(szQuery), L"select time,value,channelid from data where channelid = %d",_id);
  if(sfData.Init(szQuery))
  {
    while(sfData.Next())
    {
      int iTimeMs = 0;
      float flValue = 0;
      if(sfData.GetCol(0, &iTimeMs) && sfData.GetCol(1, &flValue))
      {
        lstData.push_back(DataPoint(iTimeMs, flValue));
      }
    }
  }
  return true;
}

void CDataChannel::Init(int iLapId, DATA_CHANNEL eType)
{
  DASSERT(!fLocked);
  if(fLocked) return;

  this->iLapId = iLapId;
  eChannelType = eType;
}
bool CDataChannel::IsValid() const
{
  return iLapId != -1 && eChannelType >= 0 && eChannelType < DATA_CHANNEL_COUNT;
}
bool CDataChannel::IsSameChannel(const CDataChannel* pOther) const
{
  return iLapId == pOther->iLapId && eChannelType == pOther->eChannelType;
}
int CDataChannel::GetLapId() const
{
  return iLapId;
}
float CDataChannel::GetValue(int iTime) const
{
  if(lstData.size() <= 0) return 0;

  int iBegin = 0;
  int iEnd = lstData.size();

  // this binary search will find the first and second points that we should use for interpolation.
  DataPoint dataFirst;
  DataPoint dataSecond;
  while(true)
  {
    const int iCheck = (iBegin + iEnd)/2;
    if(iEnd - iBegin <= 1)
    {
      // ok, we've narrowed it down to one data point, which is probably the closest
      if(iTime > lstData[iBegin].iTimeMs)
      {
        dataFirst = lstData[iBegin];
        // we need to interpolate with the data point after
        if(iBegin >= lstData.size() - 1)
        {
          // but we're right at the end of the list, so just return this value
          dataSecond = lstData[iBegin];
        }
        else
        {
          dataSecond = lstData[iBegin+1];
        }
      }
      else
      {
        dataSecond = lstData[iBegin];
        if(iBegin > 0)
        {
          // we didn't find the first point, so we can use the previous point
          dataFirst = lstData[iBegin-1];
        }
        else
        {
          dataFirst = lstData[iBegin];
        }
      }
      DASSERT(dataFirst.iTimeMs <= dataSecond.iTimeMs);

      const float flFirst = dataFirst.flValue;
      const float flNext = dataSecond.flValue;
      const float flOffset = iTime - dataFirst.iTimeMs;
      const float flWidth = dataSecond.iTimeMs - dataFirst.iTimeMs;
      if(flWidth == 0) return flFirst;
      const float flPct = flOffset / flWidth;
      return (1-flPct)*flFirst + (flPct)*flNext;
    }
    else if(lstData[iCheck].iTimeMs > iTime)
    {
      iEnd = iCheck;
    }
    else if(lstData[iCheck].iTimeMs < iTime)
    {
      iBegin = iCheck;
    }
    else if(lstData[iCheck].iTimeMs == iTime)
    {
      return lstData[iCheck].flValue;
    }
  }
  return 0;
}
float CDataChannel::GetValue(int iTime, const vector<DataPoint>::const_iterator& i) const
{
  DASSERT(fLocked); // you should only be getting data after the channel is all loaded up!
  const DataPoint& data = (*i);
  DASSERT(data.iTimeMs >= iTime);
  if(i != lstData.begin())
  {
    // this iterator has been moved forward from the start of the vector.
    // back up one from it, then interpolate between those two points
    vector<DataPoint>::const_iterator iBack = i;
    iBack--;
    const DataPoint& dataLast = *iBack;
    if(dataLast.iTimeMs == data.iTimeMs) return dataLast.flValue;

    float flWidth = data.iTimeMs - dataLast.iTimeMs;
    float flOffset = iTime - dataLast.iTimeMs;
    float flPct = flOffset / flWidth;
    DASSERT(flPct >= 0.0f && flPct <= 1.0f);
    return (1-flPct)*dataLast.flValue + flPct * data.flValue;
  }
  else
  {
    // this iterator is actually the first element in our vector, so just return it's value
    return data.flValue;
  }
}
float CDataChannel::GetMin() const
{
  return m_dMin;
}
float CDataChannel::GetMax() const
{
  return m_dMax;
}
int CDataChannel::GetEndTimeMs() const
{
  return m_msMax;
}
int CDataChannel::GetStartTimeMs() const
{
  return m_msMin;
}
void CDataChannel::AddPoint(int iTime, float flValue)
{
  DASSERT(eChannelType >= 0 && eChannelType < DATA_CHANNEL_COUNT);
  DASSERT(!fLocked);
  if(fLocked) return;

  DataPoint pt;
  pt.flValue = flValue;
  pt.iTimeMs = iTime;
  lstData.push_back(pt);
}
void CDataChannel::Lock()
{
  if(fLocked) return;

  fLocked = true;
  sort(lstData.begin(), lstData.end(), DataChannelSorter);
  m_dMin = 1e30;
  m_dMax = -1e30;
  m_msMin = INT_MAX;
  m_msMax = -INT_MAX;

  for(int x = 0; x < lstData.size(); x++)
  {
    m_dMin = min(lstData[x].flValue, m_dMin);
    m_dMax = max(lstData[x].flValue, m_dMax);
    m_msMin = min(lstData[x].iTimeMs,m_msMin);
    m_msMax = max(lstData[x].iTimeMs,m_msMax);

    m_mapCachedData[lstData[x].iTimeMs] = lstData[x].flValue;
  }
}

bool CLap::LoadZipped(const char* pszInput, int cbInput)
{
  bool fRet = false;

  int cbUncomp = min(10000,cbInput*100);
  Bytef* pbUncompressed = new Bytef[cbUncomp];
  if(pbUncompressed)
  {
    uLongf outLen = cbUncomp;
    int iRet = uncompress(pbUncompressed, &outLen, (const Bytef*)pszInput, cbInput);
    if(iRet == 0)
    {
      // successful decompress!
      InputLapRaw* pData = (InputLapRaw*)pbUncompressed;
      Load(pData);
      fRet = true;
    }

    delete[] pbUncompressed;
  }
  return fRet;
}
void CLap::Load(InputLapRaw* pLap)
{
  FLIP(pLap->cCount);
  FLIP(pLap->dTime);
  FLIP(pLap->iLapId);
  FLIP(pLap->iStartTime);
	for(int x = 0;x < pLap->cCount; x++)
	{
    TimePoint2D newPt(&pLap->rgPoints[x]);
    if(newPt.IsValid())
    {
			lstPoints.push_back(newPt);
    }
    else
    {
      newPt.flX++;
      newPt.flX--;
    }
	}
  for(int x = 0;x < NUMITEMS(pLap->rgSF); x++)
  {
    FLIP(pLap->rgSF[x]);
  }
  for(int x = 0; x < NUMITEMS(pLap->rgSFDir); x++)
  {
    FLIP(pLap->rgSFDir[x]);
  }
  for(int x = 0;x < NUMITEMS(rgSF); x++)
  {
    // pass each waypoint a block of 4 floats indicating its position
    rgSF[x] = StartFinish(&pLap->rgSF[x*4]);
    vDir[x] = V2D(pLap->rgSFDir[x*2], pLap->rgSFDir[x*2+1]);
  }
	dTime = pLap->dTime;
	iLapId = pLap->iLapId;
  iStartTime = pLap->iStartTime;
}
bool CLap::Load(CSfArtSQLiteDB& db, StartFinish* rgSF, CSfArtSQLiteQuery& line)
{
  bool fSuccess = true;
  fSuccess &= (line.GetCol(0, &iLapId));
  fSuccess &= (line.GetCol(1, &dTime));
  fSuccess &= (line.GetCol(2, &iStartTime));
  memcpy(this->rgSF, rgSF, sizeof(this->rgSF));

  // now we need to get all the datapoints for this lap (data channels will come later)
  TCHAR szQuery[MAX_PATH];
  _snwprintf(szQuery, NUMCHARS(szQuery), L"select points.x,points.y,points.time,points.velocity from points where points.lapid = %d", iLapId);
  CSfArtSQLiteQuery sfPointQuery(db);
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
  return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////
bool ProcessLapData(vector<char>& buf, int* piLapId, CLap* pOutputLap)
{
	InputLapRaw* pLap = (InputLapRaw*)&buf[0];
	pOutputLap->Load(pLap);
	return pOutputLap->IsValid();
}
//////////////////////////////////////////////////////////////////////////////
bool ProcessZippedLapData(vector<char>& buf, int* piLapId, CLap* pOutputLap)
{
	const char* pbData = (const char*)&buf[0];
	pOutputLap->LoadZipped(pbData,buf.size());
	return pOutputLap->IsValid();
}
bool ProcessDataChannel(vector<char>& buf, int* piLapId, CDataChannel* pDataChannel)
{
  InputChannelRaw* pData = (InputChannelRaw*)&buf[0];
  pDataChannel->Load(pData);
  pDataChannel->Lock();
  return pDataChannel->IsValid();
}
bool ProcessZippedDataChannel(vector<char>& buf, int* piLapId, CDataChannel* pDataChannel)
{
  const char* pData = (const char*)&buf[0];
  pDataChannel->LoadZipped(pData, buf.size()-8);
  pDataChannel->Lock();
  return pDataChannel->IsValid();
}
void GetIPString(DWORD ip, LPTSTR lpsz, int cchBuf)
{
  unsigned char* pBytes = (unsigned char*)&ip;
  swprintf(lpsz, cchBuf, L"%d.%d.%d.%d",pBytes[0],pBytes[1],pBytes[2],pBytes[3]);
}

int TimeoutRead(SOCKET s, char* buf, int cbBuf, int flags, int timeout, bool* pfConnectionLost)
{
  *pfConnectionLost = false;

  DWORD tmNow = timeGetTime();
  while(true)
  {
    unsigned long cbWaiting = 0;
    int iRet = ioctlsocket(s, FIONREAD, &cbWaiting);
    if(iRet == 0)
    {
      if(cbWaiting > 0)
      {
        return recv(s, buf, cbBuf, flags);
      }
      else
      {
        if(timeGetTime() - tmNow > timeout)
        {
          // timed out
          *pfConnectionLost = true;
          break;
        }
        else
        {
          // not timed-out yet
          Sleep(10);
        }
      }
    }
    else
    {
      // network failure
      *pfConnectionLost = true;
      break;
    }
  }
  return 0;
}

bool ReceiveLaps(int iPort, ILapReceiver* pLaps)
{
  WSADATA wsaData;
  SOCKET aSocket = NULL;
  SOCKET sDataSocket = NULL;
  int err;

  WORD wVersionRequested = MAKEWORD(2, 2);

  err = WSAStartup(wVersionRequested, &wsaData);
  if(err == 0)
  {
    aSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(aSocket != INVALID_SOCKET)
    {
      SOCKADDR_IN        ReceiverAddr;
      // The IPv4 family
      ReceiverAddr.sin_family = AF_INET;
      // Port no. 5150
      ReceiverAddr.sin_port = htons(iPort);
      // From all interface (0.0.0.0)
      ReceiverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
      err = bind(aSocket, (SOCKADDR*)&ReceiverAddr, sizeof(ReceiverAddr));
      if(err == 0)
      {
        // socket is ready to listen
		    err = listen(aSocket, SOMAXCONN);
		    if(err == SOCKET_ERROR)
		    {
			    closesocket(aSocket);
			    aSocket = INVALID_SOCKET;
		    }
		    else
		    {
			    sockaddr sfAddr = {0};
			    int cbAddr = sizeof(sfAddr);
			    pLaps->SetNetStatus(NETSTATUS_STATUS, L"Waiting for incoming connection to accept");
			    sDataSocket = accept(aSocket, &sfAddr, &cbAddr);
			    if(sDataSocket != INVALID_SOCKET)
			    {
            sockaddr_in* pIn = (sockaddr_in*)&sfAddr;
				    pLaps->SetNetStatus(NETSTATUS_STATUS, L"Connected");

            TCHAR szIPString[512];
            GetIPString(pIn->sin_addr.S_un.S_addr, szIPString, NUMCHARS(szIPString));
            pLaps->SetNetStatus(NETSTATUS_REMOTEIP, szIPString);

            
            {
              sockaddr sfName = {0};
              int cbName = sizeof(sfName);
              getsockname(sDataSocket, &sfName, &cbName);
              
              pIn = (sockaddr_in*)&sfName;
              GetIPString(pIn->sin_addr.S_un.S_addr, szIPString, NUMCHARS(szIPString));
              pLaps->SetNetStatus(NETSTATUS_THISIP, szIPString);
            }
			    }
		    }
      }
    }
  }

  vector<char> lstLapBuf; // for incoming lap-data stuff
  vector<char> lstDataBuf; // for incoming data-channel stuff
  vector<char> lstDBBuf;
  TextMatcher<8> aStart("jndadere");
  TextMatcher<8> aEnd("donelap_");
  TextMatcher<4> aHTBT("htbt");
  TextMatcher<8> aDataStart("datachan");
  TextMatcher<8> aDataEnd("donedata");
  TextMatcher<8> aZipDataStart("datachaz");
  TextMatcher<8> aZipDataEnd("donedatz");
  TextMatcher<8> aZipLapStart("jndazipz");
  TextMatcher<8> aZipLapEnd("donezipz");
  TextMatcher<16> aDBIncoming("racingdbincoming");
  TextMatcher<16> aDBDone("racingdbcomplete");

  enum CURRENTRECV {RECV_NONE,RECV_LAP,RECV_DATA,RECV_DB}; // keeps track of which type of data we're currently receiving so we can report progress about it.
  CURRENTRECV eRecv = RECV_NONE;

  while(true)
  {
    bool fConsuming = false;
	  char buf[1024];
    bool fConnectionLost = false;
	  int cbRead = TimeoutRead(sDataSocket,buf,sizeof(buf),0,10000, &fConnectionLost);
	  if(cbRead <= 0 || fConnectionLost)
	  {
		  pLaps->SetNetStatus(NETSTATUS_STATUS, L"Probably lost connection");
      pLaps->SetNetStatus(NETSTATUS_REMOTEIP, L"");
		  break;
	  }
	  else
	  {
		  for(int x = 0;x < cbRead; x++)
		  {
        lstLapBuf.push_back(buf[x]);
			  if(aStart.Process(buf[x]))
			  {
          eRecv = RECV_LAP;
				  aStart.Reset();
				  // we have detected the start of a new lap
				  lstLapBuf.clear();
			  }
			  if(aEnd.Process(buf[x]))
			  {
          eRecv = RECV_NONE;
				  aEnd.Reset();
				  // we have found the end of a lap
				  int iLapId = 0;
				  CLap* pLap = pLaps->AllocateLap();
				  if(ProcessLapData(lstLapBuf, &iLapId, pLap))
				  {
					  pLaps->AddLap(pLap);
				  }
				  else
				  {
					  pLaps->FreeLap(pLap);
				  }
          lstLapBuf.clear();
			  }
        if(aZipLapStart.Process(buf[x]))
        {
          eRecv = RECV_LAP;
          aStart.Reset();
				  // we have detected the start of a new lap
				  lstLapBuf.clear();
        }
        if(aZipLapEnd.Process(buf[x]))
        {
          eRecv = RECV_NONE;
          aEnd.Reset();
				  // we have found the end of a lap
				  int iLapId = 0;
				  CLap* pLap = pLaps->AllocateLap();
				  if(ProcessZippedLapData(lstLapBuf, &iLapId, pLap))
				  {
					  pLaps->AddLap(pLap);
				  }
				  else
				  {
					  pLaps->FreeLap(pLap);
				  }
          lstLapBuf.clear();
        }

        lstDataBuf.push_back(buf[x]);
        if(aDataStart.Process(buf[x]))
        {
          eRecv = RECV_DATA;
          aDataStart.Reset();
          lstDataBuf.clear();
        }
        if(aDataEnd.Process(buf[x]))
        {
          eRecv = RECV_NONE;
          int iLapId = 0;
          aDataEnd.Reset();
          CDataChannel* pDataChannel = pLaps->AllocateDataChannel();
          if(ProcessDataChannel(lstDataBuf,&iLapId, pDataChannel))
          {
            pLaps->AddDataChannel(pDataChannel);
          }
          lstDataBuf.clear();
        }
        if(aZipDataStart.Process(buf[x]))
        {
          eRecv = RECV_DATA;
          aDataStart.Reset();
          lstDataBuf.clear();
        }
        if(aZipDataEnd.Process(buf[x]))
        {
          eRecv = RECV_DATA;
          int iLapId = 0;
          aDataEnd.Reset();
          CDataChannel* pDataChannel = pLaps->AllocateDataChannel();
          if(ProcessZippedDataChannel(lstDataBuf,&iLapId, pDataChannel))
          {
            pLaps->AddDataChannel(pDataChannel);

            TCHAR szBuf[200];
          }
          lstDataBuf.clear();
        }
        
			  if(aHTBT.Process(buf[x]))
			  {
          eRecv = RECV_NONE;
				  aHTBT.Reset();
				  const char* pbResponse = "HTBT";
				  send(sDataSocket, pbResponse, 4, 0);

          lstLapBuf.clear();
          lstDataBuf.clear();
			  }
        lstDBBuf.push_back(buf[x]);

        if(aDBIncoming.Process(buf[x]))
        {
          eRecv = RECV_DB;
          TCHAR szBuf[200];
          lstDBBuf.clear();
        }
        if(aDBDone.Process(buf[x]))
        {
          eRecv = RECV_NONE;
          // we have received a raw database.  Save it to a temp folder, then send the path to the pitside UI so it can decide what to do
          TCHAR szTemp[MAX_PATH];
          if(GetTempPath(MAX_PATH, szTemp))
          {
            wcscat(szTemp,L"TempPitsideDB.wflp");
            if(SaveBufferToFile(szTemp, &lstDBBuf[0], lstDBBuf.size()-aDBDone.GetSize()))
            {
              pLaps->NotifyDBArrival(szTemp);
            }
          }
          
        }
        
		  } // end processing loop
      
      bool fSendUpdate = true;
      int cbRecved = 0;
      TCHAR szStatus[MAX_PATH];
      switch(eRecv)
      {
      case RECV_LAP:   
        cbRecved = lstLapBuf.size(); 
        swprintf(szStatus,L"Receiving lap: %dkb",cbRecved/1024);
        break;
      case RECV_DATA:  
        cbRecved = lstDataBuf.size(); 
        swprintf(szStatus,L"Receiving data: %dkb",cbRecved/1024);
        break;
      case RECV_DB:    
      {
        static int cUpdates = 0;
        cUpdates++;
        if(cUpdates % 100 == 0)
        {
          cbRecved = lstDBBuf.size(); 
          swprintf(szStatus,L"Receiving db: %dkb",cbRecved/1024);
        }
        else
        {
          fSendUpdate = false;
        }
        break;
      }
      default:
      case RECV_NONE:  
        cbRecved = 0;
        swprintf(szStatus,L"Connected");
        break;
      }
      if(fSendUpdate)
      {
        pLaps->SetNetStatus(NETSTATUS_STATUS, szStatus);
      }

      if(cbRead < sizeof(buf))
      {
        Sleep(1); // let's let the buffer fill up
      }
	  }
  }

  if(sDataSocket != INVALID_SOCKET)
  {
	  closesocket(sDataSocket);
	  sDataSocket = INVALID_SOCKET;
  }
  if(aSocket != INVALID_SOCKET)
  {
	  closesocket(aSocket);
	  aSocket = INVALID_SOCKET;
  }

  WSACleanup();

  return true;
}

struct LOADFROMSQLITE_PARAMS
{
public:
  LOADFROMSQLITE_PARAMS(LPCTSTR lpszSQL, int iRaceId, ILapReceiver* pRecv,IProgress* pProgress)
    : lpszSQL(lpszSQL),
      iRaceId(iRaceId),
      pRecv(pRecv),
      pProgress(pProgress)
  {

  }
  LPCTSTR lpszSQL;
  int iRaceId;
  ILapReceiver* pRecv;
  IProgress* pProgress;
};

DWORD LoadFromSQLiteThreadProc(LPVOID pvParam)
{
  LOADFROMSQLITE_PARAMS* pParams = (LOADFROMSQLITE_PARAMS*)pvParam;

  CSfArtSQLiteDB sfDB;
  HRESULT hr = sfDB.Open(pParams->lpszSQL);
  if(SUCCEEDED(hr))
  {
    sfDB.StartTransaction();

    StartFinish rgSF[3];
    {
      // annoyingly, we need to fetch the x1/y1 data out of this race
      CSfArtSQLiteQuery sfQuery(sfDB);
      TCHAR szRace[MAX_PATH];
      _snwprintf(szRace, NUMCHARS(szRace), L"select races.x1,races.y1,races.x2,races.y2,races.x3,races.y3,races.x4,races.y4,races.x5,races.y5,races.x6,races.y6 from races where _id = %d",pParams->iRaceId);

      if(sfQuery.Init(szRace))
      {
        if(sfQuery.Next())
        {
          float rgData[12];
          for(int x = 0;x < 12; x++)
          {
            VERIFY(sfQuery.GetCol(x,&rgData[x]));
          }
          for(int x = 0;x < 3; x++)
          {
            rgSF[x] = StartFinish(&rgData[x*4]);
          }
        }
        DASSERT(!sfQuery.Next());
      }
    }
    int cLaps = 0;
    {
      CSfArtSQLiteQuery sfQuery(sfDB);
      TCHAR szQuery[MAX_PATH];
      _snwprintf(szQuery, NUMCHARS(szQuery), L"Select count(laps._id) from laps where laps.raceid = %d", pParams->iRaceId);
      if(sfQuery.Init(szQuery))
      {
        if(sfQuery.Next())
        {
          sfQuery.GetCol(0,&cLaps);
          pParams->pProgress->SetTotal(cLaps);
          pParams->pProgress->SetProgress(0);
        }
      }
    }


    CSfArtSQLiteQuery sfQuery(sfDB);
    TCHAR szQuery[MAX_PATH];
    _snwprintf(szQuery, NUMCHARS(szQuery), L"Select laps._id,laps.laptime, laps.unixtime from laps where laps.raceid = %d", pParams->iRaceId);
    if(sfQuery.Init(szQuery))
    {
      int cFinished = 0;
      while(sfQuery.Next())
      {
        // going through the laps...
        CLap* pLap = pParams->pRecv->AllocateLap();
        if(pLap->Load(sfDB, rgSF, sfQuery))
        {
          // yay, the lap loaded!
          pParams->pRecv->AddLap(pLap);

          // now let's load any data channels associated with it
          CSfArtSQLiteQuery sfQueryDC(sfDB);
          _snwprintf(szQuery, NUMCHARS(szQuery), L"Select _id,lapid,channeltype from channels where lapid = %d", pLap->GetLapId());
          if(sfQueryDC.Init(szQuery))
          {
            int cFinished = 0;
            while(sfQueryDC.Next())
            {
              // going through the laps...
              CDataChannel* pDC = pParams->pRecv->AllocateDataChannel();
              if(pDC->Load(sfDB, sfQueryDC))
              {
                pDC->Lock();
                // yay, the lap loaded!
                pParams->pRecv->AddDataChannel(pDC);
              }
            }
          }
        }

        cFinished++;
        pParams->pProgress->SetProgress(cFinished);
      }
    }

    sfDB.StopTransaction();
  }

  pParams->pProgress->SetDone();
  return 0;
}

void LoadFromSQLite
(
  LPCTSTR lpszSQL, 
  int iRaceId, 
  ILapReceiver* pRecv
)
{
  DASSERT(iRaceId >= 0);

  CProgressDlg dlgProgress;
  LOADFROMSQLITE_PARAMS sfParams(lpszSQL,iRaceId,pRecv, &dlgProgress);

  DWORD dwThreadId = 0;
  HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&LoadFromSQLiteThreadProc, &sfParams, 0, &dwThreadId);
  if(hThread)
  {
    ArtShowDialog<IDD_PROGRESS>(&dlgProgress);
    WaitForSingleObject(hThread, INFINITE);
  }
}
