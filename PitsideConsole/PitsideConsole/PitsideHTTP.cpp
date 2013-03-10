#include "stdafx.h"
#include "PitsideHTTP.h"
#include "LapData.h"

PitsideHTTP::PitsideHTTP(ILapReceiver* pReceiver, const ILapSupplier* pSupplier) : m_pLapsDB(pReceiver),m_pLapSupplier(pSupplier) {}
PitsideHTTP::~PitsideHTTP() {}

// writes the javascript array for data: "{1,2,3};" for example.  if fTime, writes the timings.  If !fTime, writes the data values
void WriteData(const vector<DataPoint>& data, ostream& out, bool fTime, float* pflMin, float* pflMax)
{
  float flMin = 1e30;
  float flMax = -1e30;
  out.precision(10);
  out<<"[";
  for(int x = 0; x < data.size(); x++)
  {
    if(fTime)
    {
      flMin = min(flMin, data[x].iTimeMs);
      flMax = max(flMax,data[x].iTimeMs);
      out<<data[x].iTimeMs<<",";
    }
    else
    {
      flMin = min(flMin, data[x].flValue);
      flMax = max(flMax, data[x].flValue);
      out<<data[x].flValue<<",";
    }
  }
  out<<"];"<<endl;

  *pflMin = flMin;
  *pflMax = flMax;
}

bool WriteFile(LPCTSTR lpszFile, ostream& out)
{
  TCHAR szModule[MAX_PATH];
  if(GetAppFolder(szModule,NUMCHARS(szModule)))
  {
    // found the slash
    wcsncat(szModule,lpszFile, NUMCHARS(szModule));
  }

  HANDLE hFile = CreateFile(szModule,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if(hFile != INVALID_HANDLE_VALUE)
  {
    while(true)
    {
      char buf[1000];
      const int cbDesiredRead = 999;
      DWORD cbRead = 0;
      BOOL fSuccess = ReadFile(hFile,buf,cbDesiredRead,&cbRead,NULL);
      if(fSuccess)
      {
//        buf[cbRead] = 0;
//        out<<buf;
		out.write(buf,cbRead);
        if(cbRead < cbDesiredRead)
        {
          CloseHandle(hFile);
          return true;
        }
      }
      else
      {
        CloseHandle(hFile);
        return true;
      }
    }

    CloseHandle(hFile);
  }
  else
  {
    return false;
  }
  return false;
}

string RGBToNetColor(float r, float g, float b)
{
  char szResult[255];

  int iR = (int)(r * 255);
  int iG = (int)(g * 255);
  int iB = (int)(b * 255);
  sprintf(szResult,"#%X%X%X",iR,iG,iB);
  string strRet = string(szResult);
  return string(szResult);
}

// CSL = comma-separated-list
vector<int> ParseCSL(string strCSL)
{
  vector<int> lstRet;

  char szParse[1000];
  strncpy(szParse,strCSL.c_str(),strCSL.length());
  char* pszCurPos = &szParse[0];
  do
  {
    char* pszComma = strstr(pszCurPos,",");
    if(!pszComma)
    {
      // we're at the last thing in the list
      int iVal = atoi(pszCurPos);
      if(iVal > 0)
      {
        lstRet.push_back(iVal);
      }
      break;
    }
    else
    {
      // found the next comma
      pszComma[0] = 0;
      int iVal = atoi(pszCurPos);
      if(iVal > 0)
      {
        lstRet.push_back(iVal);
      }
      pszCurPos = pszComma+1;
    }
  } while(true);
  return lstRet;

}

bool PitsideHTTP::MakePage(HTTPREQUEST& pReq, ostream& out)
{
  pReq.strResponseType = "text/html";

  if(pReq.strPage == "/") // main page - list all the laps the user can view
  {
    return WriteFile(L"graphPage.html",out);
  }
  else if(pReq.strPage == "/getlaps.php")
  {
    // getlaps needs to return "id, name" for each available race.  In the case of pitside, that'll be one race
    if(pReq.mapParams["action"] == "getLaps" && pReq.mapParams["raceId"].length() > 0)
    {
      const int iRaceId = atoi(pReq.mapParams["raceId"].c_str());
      vector<const ILap*> lstLaps = m_pLapsDB->GetLaps(iRaceId);
      for(int x = 0;x < lstLaps.size(); x++)
      {
        const ILap* pLap = lstLaps[x];
        out<<pLap->GetLapId()<<", "<<x<<", "<<pLap->GetTime()<<endl;
      }
    }
  }
  else if(pReq.strPage == "/lapdistance.php")
  {
    pReq.strResponseType = "text/plain";

    const int lapId1 = atoi(pReq.mapParams["lap"].c_str());
    const int lapId2 = atoi(pReq.mapParams["refLap"].c_str());

    const ILap* pLap1 = m_pLapsDB->GetLap(lapId1);
    const ILap* pLap2 = m_pLapsDB->GetLap(lapId2);
    
    bool fShowAll = pReq.mapParams["allCols"].length() > 0;
    // distance, lap 1 vel, lap 2 vel
    out<<"Distance, Lap "<<lapId1<<", Lap "<<lapId2;
    if(fShowAll)
    {
      out<<", x, y";
    }
    out<<endl;

    const IDataChannel* pDist1 = this->m_pLapSupplier->GetChannel(lapId1,DATA_CHANNEL_DISTANCE);
    const IDataChannel* pDist2 = this->m_pLapSupplier->GetChannel(lapId2,DATA_CHANNEL_DISTANCE);

    const IDataChannel* pTime1 = this->m_pLapSupplier->GetChannel(lapId1,DATA_CHANNEL_TIME);
    const IDataChannel* pTime2 = this->m_pLapSupplier->GetChannel(lapId2,DATA_CHANNEL_TIME);

    const IDataChannel* pLapTime1 = this->m_pLapSupplier->GetChannel(lapId1,DATA_CHANNEL_ELAPSEDTIME);
    const IDataChannel* pLapTime2 = this->m_pLapSupplier->GetChannel(lapId2,DATA_CHANNEL_ELAPSEDTIME);

    const IDataChannel* pLapTimeSummary1 = this->m_pLapSupplier->GetChannel(lapId1,DATA_CHANNEL_LAPTIME_SUMMARY);
    const IDataChannel* pLapTimeSummary2 = this->m_pLapSupplier->GetChannel(lapId2,DATA_CHANNEL_LAPTIME_SUMMARY);

    const IDataChannel* pVel1 = this->m_pLapSupplier->GetChannel(lapId1,DATA_CHANNEL_VELOCITY);
    const IDataChannel* pVel2 = this->m_pLapSupplier->GetChannel(lapId2,DATA_CHANNEL_VELOCITY);
    
    const IDataChannel* pX1 = m_pLapSupplier->GetChannel(lapId1,DATA_CHANNEL_X);
    const IDataChannel* pY1 = m_pLapSupplier->GetChannel(lapId1,DATA_CHANNEL_Y);
    const IDataChannel* pX2 = m_pLapSupplier->GetChannel(lapId2,DATA_CHANNEL_X);
    const IDataChannel* pY2 = m_pLapSupplier->GetChannel(lapId2,DATA_CHANNEL_Y);

    if(pDist1 && pDist2 && pVel1 && pVel2)
    {
      const vector<DataPoint>& lstData1 = pDist1->GetData();
      const vector<DataPoint>& lstData2 = pDist2->GetData();
      int ix1 = 0;
      int ix2 = 0;
      while(true)
      {
        const IDataChannel* pX,*pY;
        DataPoint ptRef;
        if(ix1 < lstData1.size() && ix2 < lstData2.size() && lstData1[ix1].flValue < lstData2[ix2].flValue)
        {
          // lap 1 has the next distance data point
          float flDist = lstData1[ix1].flValue;
          float flVel = pVel1->GetValue(lstData1[ix1].iTimeMs);
          out<<flDist<<", "<<flVel<<", ";
          ptRef = lstData1[ix1];
          ix1++;
          pX = pX1;
          pY = pY1;
          
        }
        else if(ix2 < lstData2.size() && ix1 < lstData1.size())
        {
          // lap 2 has the next distance data point
          float flDist = lstData2[ix2].flValue;
          float flVel = pVel2->GetValue(lstData2[ix2].iTimeMs);
          out<<flDist<<", , "<<flVel;
          ptRef = lstData2[ix2];
          ix2++;
          pX = pX2;
          pY = pY2;
        }
        else
        {
          break; // all done
        }
        if(fShowAll)
        {
          // we also need to spit out x and y coords for the ref lap, presumably
          if(pX && pY)
          {
            float flX = pX->GetValue(ptRef.iTimeMs);
            float flY = pY->GetValue(ptRef.iTimeMs);
            out.precision(8);
            out<<", "<<flX<<", "<<flY;
          }
          else
          {
            out<<", 0, 0";
          }
        }
        out<<endl;
      }
    }
    else
    {
      out<<"no laps selected"<<endl<<"error"<<endl;
    }
  }
  else if(pReq.strPage == "/lastlaptimestamp")
  {
    vector<int> carNumbers = ParseCSL(pReq.mapParams["cars"]);
    vector<unsigned int> lstTimeStamps;
    m_pLapsDB->GetLastLapTimeStamp(carNumbers,lstTimeStamps);

    if(lstTimeStamps.size() == carNumbers.size())
    {
      for(int x = 0;x < carNumbers.size(); x++)
      {
        out<<carNumbers[x]<<",";
      }
      out<<endl;
      for(int x = 0; x < lstTimeStamps.size(); x++)
      {
        out<<lstTimeStamps[x]<<",";
      }
      out<<endl;
    }
    return true;
  }
  else if(pReq.strPage == "/getdata")
  {
    out.precision(8);
    out<<fixed;
    pReq.strResponseType = "text/plain";
    // they have called the general "getdata" page
    // params:
    // table: which table are they interested in? (races, laps, points, channels, or data will be supported in round 1)
    // parentId: what is the value of the logical parentId? (example: for laps, it would be the raceId we care about)
    

    string strRaceId = pReq.mapParams["raceid"];
    int iRaceId = 0;
    int iLapId = 0;
    if(pReq.mapParams["table"] == "races")
    {
      if(ArtAtoi(strRaceId.c_str(),strRaceId.size(),&iRaceId))
      {
        out<<"id,racename,date,lapcount"<<endl; // for the time being, pitside only supports one race/car/whatever at once
        
        vector<RACEDATA> lstRaces = m_pLapsDB->GetRaces();
        for(int x = 0;x < lstRaces.size(); x++)
        {
          wstring strWRace = lstRaces[x].strName;
          char szRaceName[1000];
          _snprintf(szRaceName,NUMCHARS(szRaceName),"%S",strWRace.c_str());

          out<<lstRaces[x].raceId<<","<<szRaceName<<","<<lstRaces[x].unixtime<<","<<lstRaces[x].laps<<endl;
        }

        return true;
      }
    }
    else if(pReq.mapParams["table"] == "laps")
    {
      if(ArtAtoi(strRaceId.c_str(),strRaceId.size(),&iRaceId))
      {
        vector<const ILap*> lstLaps = m_pLapsDB->GetLaps(iRaceId);
        out<<"id,Lap #,Time of Day, Lap Time"<<endl;
        for(int x = 0;x < lstLaps.size(); x++)
        {
          const ILap* pLap = lstLaps[x];
          out<<pLap->GetLapId()<<","<<x<<","<<pLap->GetStartTime()<<","<<pLap->GetTime()<<endl;
          ((ILap*)lstLaps[x])->Free();
        }
        return true;
      }
    }
    else if(pReq.mapParams["table"] == "points")
    {
      vector<int> lstLaps = ParseCSL(pReq.mapParams["parentId"]);
      if(lstLaps.size() > 0)
      {
        string strRefLap = pReq.mapParams["refLap"];
        int iRefLapId = 0;
        if(!ArtAtoi(strRefLap.c_str(),strRefLap.size(),&iRefLapId))
        {
          iRefLapId = lstLaps[0];
        }
        if(iRefLapId >= 0)
        {
          const ILap* pMainLap = m_pLapsDB->GetLap(lstLaps[0]);
          CExtendedLap* pRefLap = new CExtendedLap(pMainLap,NULL,m_pLapsDB, false);
          if(pMainLap)
          {
            out<<"Lapid,Time,Longitude,Latitude,Velocity,Distance"<<endl;
            for(int x = 0;x < lstLaps.size(); x++)
            {
              const ILap* pThisLap = m_pLapsDB->GetLap(lstLaps[x]);
              if(pThisLap)
              {
                CExtendedLap* pExtLap = new CExtendedLap(pThisLap,x > 0 ? pRefLap : NULL,m_pLapsDB, false);
                const IDataChannel* pDistance = pExtLap->GetChannel(DATA_CHANNEL_DISTANCE);
                if(pDistance)
                {
                  const vector<TimePoint2D>& lstPoints = pExtLap->GetPoints();
                  if(lstPoints.size() > 0)
                  {
                    TimePoint2D ptLast = lstPoints[0];
                    for(int x = 0;x < lstPoints.size(); x++)
                    {
                      TimePoint2D pt = lstPoints[x];
              
                      out<<pThisLap->GetLapId()<<","<<pt.iTime<<","<<pt.flX<<","<<pt.flY<<","<<pt.flVelocity<<","<<pDistance->GetValue(pt.iTime)<<endl;
                    }
                  }
                }

                delete pExtLap;
              }
            }
          }
          delete pRefLap;
          return true;
        }
      }
    }
    else if(pReq.mapParams["table"] == "channels")
    {
      // querying what data channels are available for a given lap
      // we need to know the lap ID
      string strLapId = pReq.mapParams["parentId"];
      int iLapId = 0;
      if(ArtAtoi(strLapId.c_str(),strLapId.size(),&iLapId))
      {
        out<<"id,Name"<<endl;
        set<DATA_CHANNEL> setChans = m_pLapsDB->GetAvailableChannels(iLapId);
        for(set<DATA_CHANNEL>::iterator i = setChans.begin(); i != setChans.end(); i++)
        {
          TCHAR szName[MAX_PATH];
          GetDataChannelName(*i,szName,NUMCHARS(szName));

          char szSmallName[MAX_PATH];
          _snprintf(szSmallName,NUMCHARS(szSmallName),"%S",szName);

          out<<*i<<","<<szSmallName<<endl;
        }
        return true;
      }
    }
    else if(pReq.mapParams["table"] == "data")
    {
      // querying for actual data from a data channel.
      // this will require knowing a lapid and a channel type
      string strLapId = pReq.mapParams["lapId"];
      int iLapId = 0;
      if(ArtAtoi(strLapId.c_str(),strLapId.size(),&iLapId))
      {
        vector<int> lstChannelTypes = ParseCSL(pReq.mapParams["dataType"]);
        if(lstChannelTypes.size() > 0)
        {
          vector<const IDataChannel*> lstChannels;
          for(int x = 0;x < lstChannelTypes.size(); x++)
          {
            lstChannels.push_back(m_pLapsDB->GetDataChannel(iLapId,(DATA_CHANNEL)lstChannelTypes[x]));
          }

          int msStart = 0x7fffffff;
          int msEnd = 0x80000000;

          for(int x = 0;x < lstChannels.size();x++)
          {
            const IDataChannel* pChannel = lstChannels[x];
            if(pChannel)
            {
              msStart = min(msStart,pChannel->GetStartTimeMs());
              msEnd = max(msEnd,pChannel->GetEndTimeMs());
            }
          }
          out<<"lapid,time";
          for(int x = 0; x < lstChannels.size(); x++)
          {
            if(lstChannels[x])
            {
              out<<","<<lstChannelTypes[x];
            }
          }
          out<<endl;
          for(int ms = msStart; ms < msEnd; ms += 250)
          {
            out<<strLapId<<","<<ms;
            for(int chan = 0; chan < lstChannels.size(); chan++)
            {
              if(lstChannels[chan])
              {
                out<<","<<lstChannels[chan]->GetValue(ms);
              }
            }
            out<<endl;
          }
          
          return true;
        }
      }
    }
  }
  else
  {
    if(pReq.strPage.find(".css") != string::npos)
    {
      pReq.strResponseType = "text/css";
    }
    else if(pReq.strPage.find(".js") != string::npos)
    {
      pReq.strResponseType = "text/javascript";
    }
	else if(pReq.strPage.find(".png") != string::npos)
    {
      pReq.strResponseType = "image/png";
    }
	else if(pReq.strPage.find(".jpg") != string::npos)
    {
      pReq.strResponseType = "image/jpeg";
    }
	else if(pReq.strPage.find(".gif") != string::npos)
    {
      pReq.strResponseType = "image/gif";
    }
    wstring widePage(pReq.strPage.length(),0);
    std::copy(pReq.strPage.begin(),pReq.strPage.end(),widePage.begin());

    // just load a local file
    return WriteFile(&widePage.c_str()[1],out);
  }
  return false;
}