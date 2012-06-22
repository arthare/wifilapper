#include "stdafx.h"
#include "DashWare.h"
#include <fstream>
#include "arttools.h"
#include <algorithm>

extern ILapReceiver* g_pLapDB;

namespace DashWare
{
  bool SortLapsByTime(const CLap* lap1, const CLap* lap2)
  {
    return lap1->GetStartTime() < lap2->GetStartTime();
  }
  void WriteChannelHeaders(wofstream& out, const vector<const CLap*>& lstLaps, set<DATA_CHANNEL>& setData)
  {
    // if you update this function, update the dashware.xml file too!
    out<<L"Time,x,y";

    // column headers
    for(set<DATA_CHANNEL>::iterator i = begin(setData); i != end(setData); i++)
    {
      TCHAR szDataChannelName[MAX_PATH];
      const DATA_CHANNEL eChannel = *i;
      GetDataChannelName(eChannel, szDataChannelName, NUMCHARS(szDataChannelName));
      out<<","<<szDataChannelName;
    }
    out<<","<<endl;

    /*// column mins
    out<<"0.0";
    for(set<DATA_CHANNEL>::iterator i = begin(setData); i != end(setData); i++)
    {
      const DATA_CHANNEL eChannel = *i;

      float flMin = 1e30;
      for(int ixLap = 0; ixLap < lstLaps.size(); ixLap++)
      {
        const CDataChannel* pDC = g_pLapDB->GetDataChannel(lstLaps[ixLap]->GetLapId(),eChannel);
        if(pDC)
        {
          flMin = min(flMin,pDC->GetMin());
        }
      }
      
      TCHAR szMinMax[100];
      _snwprintf(szMinMax,NUMCHARS(szMinMax),L"%5.2f",flMin);
      out<<","<<szMinMax;
    }
    out<<endl;

    // column maxes
    out<<"999999.99";
    for(set<DATA_CHANNEL>::iterator i = begin(setData); i != end(setData); i++)
    {
      const DATA_CHANNEL eChannel = *i;
      float flMax = -1e30;
      for(int ixLap = 0; ixLap < lstLaps.size(); ixLap++)
      {
        const CDataChannel* pDC = g_pLapDB->GetDataChannel(lstLaps[ixLap]->GetLapId(),eChannel);
        if(pDC)
        {
          flMax = max(flMax,pDC->GetMax());
        }
      }
      
      TCHAR szMinMax[100];
      _snwprintf(szMinMax,NUMCHARS(szMinMax),L"%5.2f",flMax);
      out<<","<<szMinMax;
    }
    out<<endl;*/

  }

  HRESULT SaveToDashware(LPCTSTR lpszFilename, const vector<const CLap*>& lstLaps)
  {
    if(lstLaps.size() <= 0) return E_FAIL;

    vector<const CLap*> lstSortedLaps = lstLaps;
    sort(begin(lstSortedLaps),end(lstSortedLaps),SortLapsByTime);

    wofstream out;
    out.open(lpszFilename);

    set<DATA_CHANNEL> setChannels;

    for(int ixLap = 0;ixLap < lstLaps.size(); ixLap++)
    {
      for(int y = 0; y < DATA_CHANNEL_COUNT; y++)
      {
        const CDataChannel* pChannel = g_pLapDB->GetDataChannel(lstLaps[ixLap]->GetLapId(),(DATA_CHANNEL)y);
        if(pChannel)
        {
          setChannels.insert((DATA_CHANNEL)y);
        }
      }
    }

    WriteChannelHeaders(out, lstSortedLaps, setChannels);

    float flStartTime = 0; // start time in seconds;
    for(int ixLap = 0; ixLap < lstSortedLaps.size(); ixLap++)
    {
      const CLap* pLap = lstSortedLaps[ixLap];
      int msStartTime = INT_MAX; // start time and end time for this lap (gotten by looking at start and end time for data channels)
      int msEndTime = -INT_MAX;

      for(set<DATA_CHANNEL>::iterator i = begin(setChannels); i != end(setChannels); i++)
      {
        const CDataChannel* pDC = g_pLapDB->GetDataChannel(pLap->GetLapId(),*i);
        msStartTime = min(pDC->GetStartTimeMs(),msStartTime);
        msEndTime = max(pDC->GetEndTimeMs(),msEndTime);
      }

      const vector<TimePoint2D>& lstPoints = pLap->GetPoints();
      for(int msQuery = msStartTime; msQuery < msEndTime; msQuery += 50)
      {
        TCHAR szTemp[100];
        _snwprintf(szTemp,NUMCHARS(szTemp),L"%5.3f",((float)msQuery/1000.0f)+flStartTime);
        out<<szTemp;

        TimePoint2D pt = ::GetPointAtTime(lstPoints,msQuery);
        _snwprintf(szTemp,NUMCHARS(szTemp),L"%4.6f",pt.flX);
        out<<","<<szTemp;
        _snwprintf(szTemp,NUMCHARS(szTemp),L"%4.6f",pt.flY);
        out<<","<<szTemp;

        for(set<DATA_CHANNEL>::iterator i = begin(setChannels); i != end(setChannels); i++)
        {
          const DATA_CHANNEL eDC = *i;
          const CDataChannel* pDC = g_pLapDB->GetDataChannel(pLap->GetLapId(),eDC);
          if(pDC)
          {
            _snwprintf(szTemp,NUMCHARS(szTemp),L"%5.2f",pDC->GetValue(msQuery));
            out<<","<<szTemp;
          }
          else
          {
            out<<","; // if this lap didn't include the data channel, skip it
          }
        }
        out<<","<<endl;
      }
    }

    out.close();

    return S_OK;
  }
}