#include "stdafx.h"
#include "DashWare.h"
#include <fstream>
#include "arttools.h"
#include <algorithm>

extern ILapReceiver* g_pLapDB;

namespace DashWare
{
  bool SortLapsByTime(const ILap* lap1, const ILap* lap2)
  {
    return lap1->GetStartTime() < lap2->GetStartTime();
  }
  void WriteChannelHeaders(wofstream& out, const vector<const ILap*>& lstLaps, map<DATA_CHANNEL, const IDataChannel*>& mapData)
  {
    // if you update this function, update the dashware.xml file too!
    out<<L"Lap,Time,x,y";

    // column headers
    for(map<DATA_CHANNEL, const IDataChannel*>::iterator i = begin(mapData); i != end(mapData); i++)
    {
      TCHAR szDataChannelName[MAX_PATH];
      const DATA_CHANNEL eChannel = i->first;
      GetDataChannelName(eChannel, szDataChannelName, NUMCHARS(szDataChannelName));
      out<<","<<szDataChannelName;
    }
    out<<","<<endl;

  }

  HRESULT SaveToDashware(LPCTSTR lpszFilename, const vector<const ILap*>& lstLaps)
  {
    if(lstLaps.size() <= 0) return E_FAIL;

    vector<const ILap*> lstSortedLaps = lstLaps;
    sort(begin(lstSortedLaps),end(lstSortedLaps),SortLapsByTime);

    wofstream out;
    out.open(lpszFilename);

    map<DATA_CHANNEL,const IDataChannel*> mapChannels;

    for(int ixLap = 0;ixLap < lstLaps.size(); ixLap++)
    {
	  for(int y = 0; y < DATA_CHANNEL_COUNT; y++)
      {
        const IDataChannel* pChannel = g_pLapDB->GetDataChannel(lstLaps[ixLap]->GetLapId(),(DATA_CHANNEL)y);
        DASSERT(pChannel->IsLocked() && pChannel->IsValid());
//        if(pChannel && pChannel->IsLocked() && pChannel->IsValid())
        if(pChannel && pChannel->IsValid())
        {
          mapChannels[(DATA_CHANNEL)y] = pChannel;
        }
        else
        {
          DASSERT(FALSE);
        }
      }
    }

    WriteChannelHeaders(out, lstSortedLaps, mapChannels);


    int msLastLine = 0;
    float flStartTime = 0; // start time in seconds;
    for(int ixLap = 0; ixLap < lstSortedLaps.size(); ixLap++)
    {
      const ILap* pLap = lstSortedLaps[ixLap];
      int msStartTime = INT_MAX; // start time and end time for this lap (gotten by looking at start and end time for data channels)
      int msEndTime = -INT_MAX;

      for(int y = 0; y < DATA_CHANNEL_COUNT; y++)
      {
        const IDataChannel* pChannel = g_pLapDB->GetDataChannel(pLap->GetLapId(),(DATA_CHANNEL)y);
        DASSERT(pChannel->IsLocked() && pChannel->IsValid() && pChannel->GetChannelType() == (DATA_CHANNEL)y);
//        if(pChannel && pChannel->IsLocked() && pChannel->IsValid())
        if(pChannel && pChannel->IsValid())
        {
          mapChannels[(DATA_CHANNEL)y] = pChannel;
        }
        else
        {
          DASSERT(FALSE);
        }
      }

      for(map<DATA_CHANNEL, const IDataChannel*>::iterator i = begin(mapChannels); i != end(mapChannels); i++)
      {
        const IDataChannel* pDC = mapChannels[i->first];
        if(pDC)
        {
          msStartTime = min(pDC->GetStartTimeMs(),msStartTime);
          msEndTime = max(pDC->GetEndTimeMs(),msEndTime);
        }
      }
      msEndTime = max(msEndTime, msStartTime + pLap->GetTime()*1000);

      const vector<TimePoint2D>& lstPoints = pLap->GetPoints();
	  float flRunningAverage[DATA_CHANNEL_COUNT] = {0.0f};
      bool fUseRunningAverage[DATA_CHANNEL_COUNT] = {0};
      fUseRunningAverage[DATA_CHANNEL_X_ACCEL] = true;
      fUseRunningAverage[DATA_CHANNEL_Y_ACCEL] = true;
      fUseRunningAverage[DATA_CHANNEL_Z_ACCEL] = true;

      for(int msQuery = msStartTime; msQuery < msEndTime; msQuery += 100)
      {
        if(msQuery > msLastLine)
        {
          out<<ixLap<<",";

          TCHAR szTemp[100];
          _snwprintf(szTemp,NUMCHARS(szTemp),L"%6.3f",((float)msQuery/1000.0f)+flStartTime);
          out<<szTemp;

          TimePoint2D pt = ::GetPointAtTime(lstPoints,msQuery);
          _snwprintf(szTemp,NUMCHARS(szTemp),L"%4.6f",pt.flX);
          out<<","<<szTemp;
          _snwprintf(szTemp,NUMCHARS(szTemp),L"%4.6f",pt.flY);
          out<<","<<szTemp;

          for(map<DATA_CHANNEL,const IDataChannel*>::iterator i = begin(mapChannels); i != end(mapChannels); i++)
          {
            const IDataChannel* pDC = i->second;
            if(pDC)
            {
              float flValue = pDC->GetValue(msQuery);
              if(fUseRunningAverage[i->first])
              {
                flRunningAverage[i->first] = 0.7*flValue + 0.3*flRunningAverage[i->first];
              }
              _snwprintf(szTemp,NUMCHARS(szTemp),L"%5.6f",fUseRunningAverage[i->first] ? flRunningAverage[i->first] : flValue);
              out<<","<<szTemp;
            }
            else
            {
              out<<","; // if this lap didn't include the data channel, skip it
            }
          }
          out<<","<<endl;
          msLastLine = msQuery;
        }
      }
    }

    out.close();

    return S_OK;
  }
}