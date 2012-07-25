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
  GetModuleFileName(NULL,szModule,NUMCHARS(szModule));
  // now we should have a path like "c:\blah\pitsideconsole.exe".  We want to remove the PitsideConsole.exe and append the user's lpszFile (which will probably be WebSide.js)
  int cchEnd = wcslen(szModule)-1;
  for(;cchEnd >= 0;cchEnd--)
  {
    if(szModule[cchEnd] == '\\') break;
  }
  if(cchEnd > 0)
  {
    // found the slash
    szModule[cchEnd+1] = (TCHAR)0; // kill the string after the slash
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
        buf[cbRead] = 0;
        out<<buf;
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
          m_pLapsDB->FreeLap((ILap*)lstLaps[x]);
        }
        return true;
      }
    }
    else if(pReq.mapParams["table"] == "points")
    {
      string strLapId = pReq.mapParams["parentId"];
      int iLapId = 0;
      if(ArtAtoi(strLapId.c_str(),strLapId.size(),&iLapId))
      {
        const ILap* pLap = m_pLapsDB->GetLap(iLapId);
        if(pLap)
        {
          double dDist = 0;
          const vector<TimePoint2D>& lstPoints = pLap->GetPoints();
          out<<"Time,Longitude,Latitude,Velocity,Distance"<<endl;
          if(lstPoints.size() > 0)
          {
            TimePoint2D ptLast = lstPoints[0];
            for(int x = 0;x < lstPoints.size(); x++)
            {
              TimePoint2D pt = lstPoints[x];
              const double dX = ptLast.flX - pt.flX;
              const double dY = ptLast.flY - pt.flY;
              dDist += sqrt(dX*dX+dY*dY);
              
              out<<pt.iTime<<","<<pt.flX<<","<<pt.flY<<","<<pt.flVelocity<<","<<dDist<<endl;
            }
          }
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
        string strChannelType = pReq.mapParams["dataType"];
        int iChannelId = 0;
        if(ArtAtoi(strChannelType.c_str(), strChannelType.size(),&iChannelId))
        {
          DATA_CHANNEL eChannel = (DATA_CHANNEL)iChannelId;
          const IDataChannel* pChannel = m_pLapsDB->GetDataChannel(iLapId,eChannel);
          if(pChannel)
          {
            out<<"time,value"<<endl;
            vector<DataPoint> lstData = pChannel->GetData();
            for(int x = 0;x < lstData.size(); x++)
            {
              const DataPoint& pt = lstData[x];
              out<<pt.iTimeMs<<","<<pt.flValue<<endl;
            }
            return true;
          }
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
    wstring widePage(pReq.strPage.length(),0);
    std::copy(pReq.strPage.begin(),pReq.strPage.end(),widePage.begin());

    // just load a local file
    return WriteFile(&widePage.c_str()[1],out);
  }
  return false;
}