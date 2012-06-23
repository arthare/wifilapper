#include "stdafx.h"
#include "PitsideHTTP.h"
#include "LapData.h"

PitsideHTTP::PitsideHTTP(const ILapReceiver* pReceiver) : m_pLapsDB(pReceiver) {}
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

void WritePredoneJS(LPCTSTR lpszFile, ostream& out)
{
  TCHAR szModule[MAX_PATH];
  GetModuleFileName(NULL,szModule,NUMCHARS(szModule));
  // now we should have a path like "c:\blah\pitsideconsole.exe".  We want to remove the PitsideConsole.exe and append the user's lpszFile (which will probably be WebSide.js)
  int cchEnd = wcslen(szModule);
  for(;cchEnd >= 0;cchEnd--)
  {
    if(szModule[cchEnd] == '\\') break;
  }
  if(cchEnd > 0)
  {
    // found the slash
    szModule[cchEnd+1] = 0; // kill the string after the slash
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
          break; // done!
      }
      else
      {
        break;
      }
    }

    CloseHandle(hFile);
  }
  
}

void PitsideHTTP::WriteLapDataScript(const CLap* pLap, string strArrayName, ostream& out) const
{
  set<DATA_CHANNEL> setChannels = m_pLapsDB->GetAvailableChannels(pLap->GetLapId());
  out<<"<script type='text/javascript'>"<<endl;

  out<<"var lapArrayTime = new Array("<<setChannels.size()<<");"<<endl;
  out<<"var lapArrayData = new Array("<<setChannels.size()<<");"<<endl;
  out<<"var lapArrayDataMinMax = new Array("<<setChannels.size()<<");"<<endl;
  out<<"var lapArrayTimeMinMax = new Array("<<setChannels.size()<<");"<<endl;

  // write out an array of the indices for the data channels we have
  out<<"var ixData = [";
  for(set<DATA_CHANNEL>::iterator i = setChannels.begin(); i != setChannels.end(); i++)
  {
    out<<(*i)<<",";
  }
  out<<"];"<<endl;
  out<<"var strData = [";
  for(set<DATA_CHANNEL>::iterator i = setChannels.begin(); i != setChannels.end(); i++)
  {
    TCHAR szChannelName[MAX_PATH];
    GetDataChannelName(*i,szChannelName, NUMCHARS(szChannelName));

    char szChannelCStr[MAX_PATH];
    _snprintf(szChannelCStr,sizeof(szChannelCStr),"%S",szChannelName);
    out<<"'"<<szChannelCStr<<"',";
  }
  out<<"];"<<endl;

  set<DATA_CHANNEL>::iterator i  =setChannels.begin();
  while(i != setChannels.end())
  {
    DATA_CHANNEL eChannelType = *i;
    const CDataChannel* pChannel = m_pLapsDB->GetDataChannel(pLap->GetLapId(),eChannelType);
    if(pChannel)
    {
      float flMinTime,flMaxTime;
      float flMinData,flMaxData;
      const vector<DataPoint>& data = pChannel->GetData();
      out<<"lapArrayTime["<<(int)(eChannelType)<<"] = ";
      WriteData(data,out,true,&flMinTime,&flMaxTime);
      out<<endl;

      out<<"lapArrayData["<<(int)(eChannelType)<<"] = ";
      WriteData(data,out,false,&flMinData,&flMaxData);
      out<<endl;
      out<<"lapArrayDataMinMax["<<(int)(eChannelType)<<"] = ["<<flMinData<<","<<flMaxData<<"];"<<endl;
      out<<"lapArrayTimeMinMax["<<(int)(eChannelType)<<"] = ["<<flMinTime<<","<<flMaxTime<<"];"<<endl;
    }
    i++;
  }
  WritePredoneJS(L"webside.js", out);
  out<<"</script>"<<endl;
}

bool PitsideHTTP::MakePage(HTTPREQUEST& pReq, ostream& out)
{
  if(pReq.strPage == "/") // main page - list all the laps the user can view
  {
    vector<const CLap*> lstLaps = m_pLapsDB->GetLaps();

    out<<"<table border=1><th><td>Laptime</tr>";
    for(int x = 0;x < lstLaps.size(); x++)
    {
      const CLap* pLap = lstLaps[x];
      out<<"<tr><td><a href='lap?id="<<pLap->GetLapId()<<"'>Link</a></td><td>"<<pLap->GetTime()<<"</td></tr>";
    }
    out<<"</table>";
    return true;
  }
  else if(pReq.strPage == "/lap")
  {
    string strLapId = pReq.mapParams["id"];

    int iLapId = atoi(strLapId.c_str());
    if(iLapId > 0)
    {
      const CLap* pLap = m_pLapsDB->GetLap(iLapId);

      if(pLap)
      {
        WriteLapDataScript(pLap,"mainLap",out);

        out<<"<body onload='start()'>"<<endl;
        out<<"Lap Id: "<<pLap->GetLapId()<<" has "<<pLap->GetPoints().size()<<" points<br/>"<<endl;
        out<<"</body>"<<endl;
       
      }
      else
      {
        out<<"Lap not found"<<endl;
      }
    }
    return true;
  }
  else
  {
    return false;
  }
}