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
  void WriteChannelHeaders(wofstream& out, map<DATA_CHANNEL, const IDataChannel*>& mapData)
  {
    // if you update this function, update the dashware.xml file too!
    out << L"Lap,Time,Comment";

    // column headers
    for(map<DATA_CHANNEL, const IDataChannel*>::iterator i = begin(mapData); i != end(mapData); i++)
    {
      TCHAR szDataChannelName[MAX_PATH];
      const DATA_CHANNEL eChannel = i->first;
      GetDataChannelName(eChannel, szDataChannelName, NUMCHARS(szDataChannelName));
      out << "," << szDataChannelName;
    }
    out << "," << endl;

  }

  //	Subprogram saves the chosen lap's data out to a CSV file for import into rendering programs like Dashware and RaceRender
  HRESULT SaveToDashware(LPCTSTR lpszFilename, const vector<const ILap*>& lstLaps, vector<CExtendedLap*> lstLaps1)
  {
    //	lstLaps: Vector of chosen laps to output, lpszFileaname: Filename to output CSV file to
	if(lstLaps.size() <= 0) return E_FAIL;

    vector<const ILap*> lstSortedLaps = lstLaps;	//	Get all of the chosen laps, lstLaps is used to remove Reference Lap from the list
    sort(begin(lstSortedLaps),end(lstSortedLaps),SortLapsByTime);	//	Sort all of the chosen laps by lap time

    wofstream out;
    out.open(lpszFilename);	//	Open up the CSV file for input

    map<DATA_CHANNEL,const IDataChannel*> dwMapChannels;	//	dwMapChannels is the list of Data Channels for all laps
	for(int ixLap = 0;ixLap < lstSortedLaps.size(); ixLap++)
    {
		//	Find all of the populated data channels for this lap
		set<DATA_CHANNEL> setLapData = lstLaps1[ixLap]->GetAvailableChannels();	//	Load the CExtendedLap data for the lap list
		for(set<DATA_CHANNEL>::iterator i = setLapData.begin(); i != setLapData.end(); i++)
		{
			const IDataChannel* pChannel = lstLaps1[ixLap]->GetChannel(*i);
			DASSERT(pChannel->IsValid());	//	Ensure that channel doesn't cause execution issues
//			DASSERT(pChannel->IsLocked() && pChannel->IsValid());
//			if(pChannel && pChannel->IsLocked() && pChannel->IsValid())
			if(pChannel && pChannel->IsValid())
			{
				dwMapChannels[*i] = pChannel;	//	Add Data Channel to the list
			}
			else
			{
				DASSERT(FALSE);
			}
		}
    }
	WriteChannelHeaders(out, dwMapChannels);	//	Put all Data Channel names into the header for the CSV file
    
	long long msLastLine = 0;	//	Indexer for pulling correct timestamp data for each data channel
    long long msTimeOffset = 0; // Time in ms to offset the Time in seconds column in the CSV offset file; used to fix phone reset issues
	//	Need to go through the lstSortedLaps sequentially and find each lap in lstLaps1 for output to file
    for (int ixLap = 0; ixLap < lstSortedLaps.size(); ixLap++)
    {
	  int iFoundLap;
	  //	First find this lap in the lstLaps1 array
	  for(iFoundLap = 0; iFoundLap < lstLaps1.size(); iFoundLap++)
	  {
		  if(lstLaps1[iFoundLap]->GetLap()->GetStartTime() == lstSortedLaps[ixLap]->GetStartTime())
		  {
			  //	We found the lap. Let's get all of the populated data channels for this lap

			  const ILap* pLap = lstLaps1[iFoundLap]->GetLap();	//	Get the chosen lap
			  long long msStartTime = INT_MAX; // start time and end time for this lap (gotten by looking at start and end time for data channels)
			  long long msEndTime = -INT_MAX;

			  const IDataChannel* pDC = lstLaps1[iFoundLap]->GetChannel(DATA_CHANNEL_VELOCITY);	//	Use the Velocity data channel to set time boundary conditions for this lap

			  msStartTime = pDC->GetStartTimeMs();
			  msEndTime = max(msEndTime, msStartTime + pLap->GetTime()*1000);
			  if ((msStartTime + 2000) < msLastLine)		//	Time stamp for this lap is less than previous lap; possible phone reset. 
			  //	Add an offset to following laps for RaceRender to work correctly
			  {
				  msTimeOffset = msLastLine + msTimeOffset + 5000 - msStartTime;
			  }

			  const vector<TimePoint2D>& lstPoints = pLap->GetPoints();	  //	List of points for this lap
			  float flRunningAverage[DATA_CHANNEL_COUNT] = {0.0f};
			  bool fUseRunningAverage[DATA_CHANNEL_COUNT] = {0};
			  fUseRunningAverage[DATA_CHANNEL_X_ACCEL] = true;
			  fUseRunningAverage[DATA_CHANNEL_Y_ACCEL] = true;
			  fUseRunningAverage[DATA_CHANNEL_Z_ACCEL] = true;

//			  for(int msQuery = msStartTime; msQuery < msEndTime; msQuery += 2000)	//	Increased time movement for debug purposes
			  for(int msQuery = msStartTime; msQuery < msEndTime; msQuery += 100)
			  {
				  //	Lap counter
				  out << ixLap << ",";
				  //	Time
				  TCHAR szTemp[100] = L"0";
				  _snwprintf( szTemp, NUMCHARS(szTemp), L"%6.3f", ((float)(msQuery + msTimeOffset)/1000.0f) );
				  out << szTemp << ",";
				  //	Comment
				  out << pLap->GetComment();
				  //	For all of the hard coded channels, loop through them to get the data channel value at this point in time
				  for(map<DATA_CHANNEL,const IDataChannel*>::iterator i = begin(dwMapChannels); i != end(dwMapChannels); i++)
				  {
					const IDataChannel* pDC = lstLaps1[iFoundLap]->GetChannel(i->first);;	//	Get the data for this lap/channel
					if(pDC)
					{
					  float flValue = pDC->GetValue(msQuery);
					  if(fUseRunningAverage[i->first])
					  {
						flRunningAverage[i->first] = 0.7*flValue + 0.3*flRunningAverage[i->first];
					  }
					  _snwprintf(szTemp,NUMCHARS(szTemp),L"%5.6f",fUseRunningAverage[i->first] ? flRunningAverage[i->first] : flValue);
		//              _snwprintf(szTemp, NUMCHARS(szTemp), L"%5.6f", flValue);	//	Used for debug purposes
					  out << "," << szTemp;	//	Output the value to the CSV file
					}
					else
					{
					  out << "," ; // if this lap didn't include the data channel, skip it
					}
				  }
				  out << "," << endl << flush;
				  msLastLine = msQuery;	//	Index the time in preparation for the next lap
			  }
			  break;	//	We have our informtation, let's get out of the loop and move to the next lap
		  }
		  else
		  {
			  //	Lap was not found, skip and move on
		  }
	  }
    }

    out.close();

    return S_OK;
  }
}