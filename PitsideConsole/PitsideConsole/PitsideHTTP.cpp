#include "stdafx.h"
#include "PitsideHTTP.h"
#include "LapData.h"

PitsideHTTP::PitsideHTTP(const ILapReceiver* pReceiver, const ILapSupplier* pSupplier) : m_pLapsDB(pReceiver),m_pLapSupplier(pSupplier) {}
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
          return true;
      }
      else
      {
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

void PitsideHTTP::WriteLapsAvailable(ostream& out) const
{
  vector<CExtendedLap*> lstLaps = m_pLapSupplier->GetAllLaps();

  out<<"var lapsAvailable = new Array();"<<endl;
  out<<"lapsAvailable = [";
  for(int x = 0;x < lstLaps.size(); x++)
  {
    out<<lstLaps[x]->GetLap()->GetLapId()<<",";
  }
  out<<"];"<<endl;

  out<<"var lapColors = new Array();"<<endl;
  out<<"lapColors = [";
  for(int x = 0;x < lstLaps.size(); x++)
  {
    srand((int)lstLaps[x]);
    const float r = RandDouble()/2 + 0.5;
    const float g = RandDouble()/2 + 0.5;
    const float b = RandDouble()/2 + 0.5;
    out<<"'"<<RGBToNetColor(r,g,b).c_str()<<"',";
  }
  out<<"];"<<endl;

  out<<"var lapTexts = new Array();"<<endl;
  out<<"lapTexts = [";
  for(int x = 0;x < lstLaps.size(); x++)
  {
    TCHAR szLapW[200];
    lstLaps[x]->GetString(szLapW,NUMCHARS(szLapW));
    char szLapA[200];
    _snprintf(szLapA,NUMCHARS(szLapA),"%S",szLapW);

    out<<"'"<<szLapA<<"',";
  }
  out<<"];"<<endl;
}

void PitsideHTTP::WriteLapDataInit(const CLap* pLap, string strArrayName, ostream& out) const
{
  set<DATA_CHANNEL> setChannels = m_pLapsDB->GetAvailableChannels(pLap->GetLapId());
  
  out<<"var defaultLap = "<<pLap->GetLapId()<<";"<<endl;

  out<<"var lapArrayTime = new Array("<<setChannels.size()<<");"<<endl;
  out<<"var lapArrayData = new Array("<<setChannels.size()<<");"<<endl;
  out<<"var lapArrayDataMinMax = new Array("<<setChannels.size()<<");"<<endl;
  out<<"var lapArrayTimeMinMax = new Array("<<setChannels.size()<<");"<<endl;

  // write out an array of the indices for the data channels we have
  out<<"var ixData = new Array();"<<endl;
  

  out<<"var strData = new Array();"<<endl;
}
void PitsideHTTP::WriteLapDataScript(const CLap* pLap, string strArrayName, ostream& out) const
{
  out<<"lapArrayTime["<<strArrayName<<"] = new Array();"<<endl;
  out<<"lapArrayData["<<strArrayName<<"] = new Array();"<<endl;
  out<<"lapArrayDataMinMax["<<strArrayName<<"] = new Array();"<<endl;
  out<<"lapArrayTimeMinMax["<<strArrayName<<"] = new Array();"<<endl;

  set<DATA_CHANNEL> setChannels = m_pLapsDB->GetAvailableChannels(pLap->GetLapId());

  out<<"strData["<<strArrayName<<"] = [";
  for(set<DATA_CHANNEL>::iterator i = setChannels.begin(); i != setChannels.end(); i++)
  {
    TCHAR szChannelName[MAX_PATH];
    GetDataChannelName(*i,szChannelName, NUMCHARS(szChannelName));

    char szChannelCStr[MAX_PATH];
    _snprintf(szChannelCStr,sizeof(szChannelCStr),"%S",szChannelName);
    out<<"'"<<szChannelCStr<<"',";
  }
  out<<"];"<<endl;

  out<<"ixData["<<strArrayName<<"] = [";
  for(set<DATA_CHANNEL>::iterator i = setChannels.begin(); i != setChannels.end(); i++)
  {
    out<<(*i)<<",";
  }
  out<<"];"<<endl;

  set<DATA_CHANNEL>::iterator i = setChannels.begin();
  while(i != setChannels.end())
  {
    DATA_CHANNEL eChannelType = *i;
    const CDataChannel* pChannel = m_pLapsDB->GetDataChannel(pLap->GetLapId(),eChannelType);
    if(pChannel)
    {
      float flMinTime,flMaxTime;
      float flMinData,flMaxData;
      const vector<DataPoint>& data = pChannel->GetData();
      out<<"lapArrayTime["<<strArrayName<<"]["<<(int)(eChannelType)<<"] = ";
      WriteData(data,out,true,&flMinTime,&flMaxTime);
      out<<endl;

      out<<"lapArrayData["<<strArrayName<<"]["<<(int)(eChannelType)<<"] = ";
      WriteData(data,out,false,&flMinData,&flMaxData);
      out<<endl;
      out<<"lapArrayDataMinMax["<<strArrayName<<"]["<<(int)(eChannelType)<<"] = ["<<flMinData<<","<<flMaxData<<"];"<<endl;
      out<<"lapArrayTimeMinMax["<<strArrayName<<"]["<<(int)(eChannelType)<<"] = ["<<flMinTime<<","<<flMaxTime<<"];"<<endl;
    }
    i++;
  }
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
      vector<CExtendedLap*> lstLaps = m_pLapSupplier->GetAllLaps();
      for(int x = 0;x < lstLaps.size(); x++)
      {
        const CExtendedLap* pLap = lstLaps[x];
        out<<pLap->GetLap()->GetLapId()<<", "<<x<<", "<<pLap->GetLap()->GetTime()<<endl;
      }
    }
  }
  else if(pReq.strPage == "/lapdistance.php")
  {
    pReq.strResponseType = "text/plain";

    const int lapId1 = atoi(pReq.mapParams["lap"].c_str());
    const int lapId2 = atoi(pReq.mapParams["refLap"].c_str());

    const CLap* pLap1 = m_pLapsDB->GetLap(lapId1);
    const CLap* pLap2 = m_pLapsDB->GetLap(lapId2);
    
    bool fShowAll = pReq.mapParams["allCols"].length() > 0;
    // distance, lap 1 vel, lap 2 vel
    out<<"Distance, Lap "<<lapId1<<", Lap "<<lapId2;
    if(fShowAll)
    {
      out<<", x, y";
    }
    out<<endl;

    const CDataChannel* pDist1 = this->m_pLapSupplier->GetChannel(lapId1,DATA_CHANNEL_DISTANCE);
    const CDataChannel* pDist2 = this->m_pLapSupplier->GetChannel(lapId2,DATA_CHANNEL_DISTANCE);

    const CDataChannel* pVel1 = this->m_pLapSupplier->GetChannel(lapId1,DATA_CHANNEL_VELOCITY);
    const CDataChannel* pVel2 = this->m_pLapSupplier->GetChannel(lapId2,DATA_CHANNEL_VELOCITY);
    
    const CDataChannel* pX1 = m_pLapSupplier->GetChannel(lapId1,DATA_CHANNEL_X);
    const CDataChannel* pY1 = m_pLapSupplier->GetChannel(lapId1,DATA_CHANNEL_Y);
    const CDataChannel* pX2 = m_pLapSupplier->GetChannel(lapId2,DATA_CHANNEL_X);
    const CDataChannel* pY2 = m_pLapSupplier->GetChannel(lapId2,DATA_CHANNEL_Y);

    if(pDist1 && pDist2 && pVel1 && pVel2)
    {
      const vector<DataPoint>& lstData1 = pDist1->GetData();
      const vector<DataPoint>& lstData2 = pDist2->GetData();
      int ix1 = 0;
      int ix2 = 0;
      while(true)
      {
        const CDataChannel* pX,*pY;
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
    pReq.strResponseType = "text/plain";
    // they have called the general "getdata" page
    // params:
    // table: which table are they interested in? (races, laps, points, channels, or data will be supported in round 1)
    // parentId: what is the value of the logical parentId? (example: for laps, it would be the raceId we care about)
    
    if(pReq.mapParams["table"] == "races")
    {
      out<<"id,racename,date"<<endl; // for the time being, pitside only supports one race/car/whatever at once
      out<<"1,OnlyRace,1"<<endl;
      return true;
    }
    else if(pReq.mapParams["table"] == "laps")
    {
      // there should also be a raceId parameter here, but since we only support one race we don't care.  Sometime in the future...
      vector<const CLap*> lstLaps = m_pLapsDB->GetLaps();
      out<<"id,Lap #,Time of Day, Lap Time"<<endl;
      for(int x = 0;x < lstLaps.size(); x++)
      {
        const CLap* pLap = lstLaps[x];
        out<<pLap->GetLapId()<<","<<x<<","<<pLap->GetStartTime()<<","<<pLap->GetTime()<<endl;
      }
      return true;
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
      out.precision(8);

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
          const CDataChannel* pChannel = m_pLapsDB->GetDataChannel(iLapId,eChannel);
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