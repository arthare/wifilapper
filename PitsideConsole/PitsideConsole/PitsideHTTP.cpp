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

bool PitsideHTTP::MakePage(HTTPREQUEST& pReq, ostream& out)
{
  if(pReq.strPage == "/") // main page - list all the laps the user can view
  {
    vector<CExtendedLap*> lstLaps = m_pLapSupplier->GetAllLaps();

    vector<wstring> lstHeaders;
    vector<int> lstWidths; // not really needed...
    CExtendedLap::GetStringHeaders(lstHeaders,lstWidths);

    out<<"<table border=1><tr><th>";
    for(int x = 0;x < lstHeaders.size(); x++)
    {
      char szHeaderA[200];
      _snprintf(szHeaderA,NUMCHARS(szHeaderA),"%S",lstHeaders[x].c_str());
      out<<"<th>"<<szHeaderA;
    }
    out<<"</tr>";
    for(int x = 0;x < lstLaps.size(); x++)
    {
      const CExtendedLap* pLap = lstLaps[x];
      vector<wstring> lstStrings;
      pLap->GetStrings(lstStrings);
      out<<"<tr><td><a href='lap?id="<<pLap->GetLap()->GetLapId()<<"'>Link</a>";
      for(int ixString = 0; ixString < lstStrings.size(); ixString++)
      {
        char szStringA[200];
        _snprintf(szStringA,NUMCHARS(szStringA),"%S",lstStrings[ixString].c_str());
        out<<"<td>"<<szStringA;
      }
      out<<"</tr>";
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
        out<<"<script type='text/javascript'>"<<endl;
        WriteLapDataInit(pLap,strLapId,out);
        WriteLapsAvailable(out);
        WriteLapDataScript(pLap,strLapId,out);
        
        WritePredoneJS(L"webside.js", out);
        out<<"</script>"<<endl;

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
  else if(pReq.strPage == "/lapdata")
  {
    // this usually occurs via XMLHttpRequests.  All they want here is just <script>...arrays...</script> for the specified lap
    string strLapId = pReq.mapParams["id"];

    int iLapId = atoi(strLapId.c_str());
    if(iLapId > 0)
    {
      const CLap* pLap = m_pLapsDB->GetLap(iLapId);

      if(pLap)
      {
        WriteLapDataScript(pLap,strLapId,out);
      }
    }
  }
  else
  {
    return false;
  }
}