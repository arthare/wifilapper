#include "Stdafx.h"
#include "DlgTimingScoring.h"
#include "resource.h"
#include "PitsideConsole.h"
#include "LapReceiver.h"
#include "ArtSQL/ArtSQLite.h"
#include <string.h>

LRESULT CDlgTimingScoring::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
    {
		//	Set up the Hot Lap timing list box
		vector<wstring> lstCols;
		vector<int> lstWidths;
		lstCols.push_back(L"Pos");
		lstCols.push_back(L"Name");
		lstCols.push_back(L"Comment");
		lstCols.push_back(L"Laptime");
		lstWidths.push_back(40);
		lstWidths.push_back(210);
		lstWidths.push_back(85);
		lstWidths.push_back(85);
		HWND Dlg_hWnd = GetDlgItem(hWnd,IDC_TIMINGSCORING);
		sfListBox.Init(Dlg_hWnd,lstCols,lstWidths);


		//	Now set up the Scoring list box
		vector<wstring> scoringLstCols;
		vector<int> scoringLstWidths;
		scoringLstCols.push_back(L"Pos");
		scoringLstCols.push_back(L"Name");
		scoringLstCols.push_back(L"Laps/Time Ahead");
		scoringLstWidths.push_back(30);
		scoringLstWidths.push_back(145);
		scoringLstWidths.push_back(102);
		HWND DlgScoring_hWnd = GetDlgItem(hWnd,IDC_RACESCORING);
		sfListBox.Init(DlgScoring_hWnd,scoringLstCols,scoringLstWidths);
		tmStartRace = NULL;	//	No races started at window initialization

		TimingScoringProc((LPVOID)&m_szPath, hWnd);
		break;
    }
    case WM_COMMAND:
    {
      switch(LOWORD(wParam))
      {
        case IDRESCAN:
        {
			TimingScoringProc((LPVOID)&m_szPath, hWnd);
			if (tmStartRace)
			{
				CRaceScoring((LPVOID) &m_szPath, hWnd);
			}
            m_pResults->fCancelled = false;
			return TRUE;
        }
        case IDCANCEL:
		{
          m_pResults->fCancelled = true;
		  for (int i=0;i<50;i++)
		  {
				m_sfResult->m_RaceId[i] = -1;
		  }
          EndDialog(hWnd,0);
          return TRUE;
		}
        case IDSTARTRACE:
		{
			TCHAR szText[MAX_PATH] = {NULL};			
			TCHAR szTemp[MAX_PATH] = {NULL};			
			tmEndRace = NULL;	//	Remove any end of race marker when new race begins. DWORD format
//			tmStartRace = timeGetTime();	//	Set the start time for this race session. Unixtime in DWORD format
			tmStartRace = GetSecondsSince1970();
			MessageBox(hWnd, L"New race started!", MB_OK,NULL);
			CRaceScoring((LPVOID) &m_szPath, hWnd);	//	Update the scoring list view
			m_pResults->fCancelled = false;
			return TRUE;
		}
        case IDENDRACE:
		{
			TCHAR szText[MAX_PATH] = {NULL};			
			TCHAR szTemp[MAX_PATH] = {NULL};			
//			tmEndRace = timeGetTime();		//	Set the end time for this race session. Unixtime in DWORD format
			tmEndRace = GetSecondsSince1970();
		    ::FormatTimeMinutesSecondsMs((tmEndRace - tmStartRace), szText, NUMCHARS(szText) );
			swprintf(szText, _tcslen(szText) - 2, L"%s", szText);	//	Remove the fractional time
			swprintf(szTemp, NUMCHARS(szTemp), szText);
			_snwprintf(szText, NUMCHARS(szText), L"Race has ended\n\nRace duration: %s", szTemp);
			MessageBox(hWnd, szText, MB_OK,NULL);
				m_pResults->fCancelled = false;
				return TRUE;
		}
      }
      break;
    } // end WM_COMMAND
    case WM_CLOSE:
    {
      m_pResults->fCancelled = true;
      for (int i=0;i<50;i++)
	  {
			m_sfResult->m_RaceId[i] = -1;
	  }
	  EndDialog(hWnd,0);
      break;
    }
  }
  return FALSE;
}

//	Function calculates the current laps and elapsed time since the race started
DWORD* CDlgTimingScoring::CRaceScoring(LPVOID pv, HWND hWnd)
{
  LPCTSTR lpszPath = (LPCTSTR)pv;
  CSfArtSQLiteDB sfDB;
  SCORINGDATA m_ScoringData[50];	//	Save up to 50 racer's results in memory
  vector<wstring> lstTables;
  HWND DlgScoring_hWnd = GetDlgItem(hWnd, IDC_RACESCORING);
  if(SUCCEEDED(sfDB.Open(lpszPath, lstTables, true)))
  {
	  //	Race ID's are stored in the sfResult.m_RaceId structure
	  int z = 0;
	  while (m_sfResult->m_RaceId[z] >= 0 && z < 50)
	  {
		m_ScoringData[z].db_iRaceId = m_sfResult->m_RaceId[z];
		z++;
	  }

	  //	First query for the total number of laps for each car
	  TCHAR szTmStartRace[MAX_PATH] = {NULL};
	  TCHAR szTmEndRace[MAX_PATH] = {NULL};

	  //	First let's make the strings for Start and End times
	  long long iTmStartRace = 0;
	  long long iTmEndRace = 0;
//	  DWORD fTmStartRace = 1376100527;	//	Used for the TestRaces database only
	  DWORD fTmStartRace = tmStartRace;
	  iTmStartRace = (int)fTmStartRace;
	  swprintf(szTmStartRace, NUMCHARS(szTmEndRace), L"%d", iTmStartRace); 
//	  DWORD fTmEndRace = 1376100699;	//	Used for the TestRaces database only
	  DWORD fTmEndRace = tmEndRace;
	  iTmEndRace = (int)fTmEndRace;
	  if (iTmEndRace <= 0)
	  {
			swprintf(szTmEndRace, NUMCHARS(szTmEndRace), L"5555555555"); 
	  }
	  else
	  {
			swprintf(szTmEndRace, NUMCHARS(szTmEndRace), L"%d", iTmEndRace); 
	  }

	  //	Now cycle through all selected RaceId's and get the number of laps completed
      CSfArtSQLiteQuery sfQuery(sfDB);
	  TCHAR szTmp[2080] = {NULL};
	  TCHAR szTemp[1080] = L"select count(laps._id) from laps,races where laps.raceid=races._id and races._id = "; //? and laps.unixtime > ? and laps.unixtime < ?";
	  for (int y = 0; y < z; y++)
	  {
		    _snwprintf(szTmp, NUMCHARS(szTmp), L"%s%i and laps.unixtime > %s and laps.unixtime < %s", szTemp, m_ScoringData[y].db_iRaceId, szTmStartRace, szTmEndRace);
			if(sfQuery.Init(szTmp))
			{
				while(sfQuery.Next())
					{
//					sfQuery.BindValue(m_ScoringData[y].db_iRaceId);
//					sfQuery.BindValue((int)iTmStartRace);
//					sfQuery.BindValue((int)iTmEndRace);
					int cLaps = 0;
					if(sfQuery.GetCol(0,&cLaps))
					{
						m_ScoringData[y].db_iTotLaps = cLaps;	//	Store the lap count value in the data structure
//						lstLaps.push_back(cLaps);
					}
				}
			}
	  }

	  //	Now let's get their lap time information, so that we can figure out between-lap time differences
	  //	Right now we are just getting the time difference between EndRace time and the last lap time
	  //	Now cycle through all selected RaceId's and get all of their laptimes and sort them by race name and then by time collected in the given timespan
	  _snwprintf(szTmp, NUMCHARS(szTmp), L"");
	  _snwprintf(szTemp, NUMCHARS(szTemp), L"select races.name,laps.unixtime,laps._id from laps,races where laps.raceid=races._id and (");
	  for (int y = 0; y < z; y++)
	  {
			int s_RaceId = m_ScoringData[y].db_iRaceId;
			if (s_RaceId != 0)
			{
				_snwprintf(szTemp, NUMCHARS(szTemp), L"%sraces._id = %i or ", szTemp, s_RaceId);
			}
	  }
	  swprintf(szTmp, wcslen(szTemp) - 3, L"%s", szTemp);
	  _snwprintf(szTemp, NUMCHARS(szTemp), L"%s) and laps.unixtime > %s and laps.unixtime < %s order by name and unixtime", szTmp, szTmStartRace, szTmEndRace);
	  int rec = 0, recFlag = 0;
	  if(sfQuery.Init(szTemp))
	  {
			int db_iUnixFirstTimeStart = -1;
			int db_iUnixLastTimeStart = -1;
			TCHAR szLapsIdStart[MAX_PATH] = {NULL};
			TCHAR szRaceNameStart[300] = {NULL};
			long long flStartLapTime = -1;
			long long flFinishLapTime = -1;
			TCHAR szPos[MAX_PATH] = {NULL};
			TCHAR szRaceName[300] = {NULL};
			TCHAR szLap[300] = {NULL};
			long long flLapTime = 0, flLapTemp = 0;
			while(sfQuery.Next())
			{
			  if (sfQuery.GetCol(0,szRaceName,NUMCHARS(szRaceName)))
			  {
//				  lstRaceName.push_back(szRaceName);
				  if (recFlag == 0) swprintf(szRaceNameStart, NUMCHARS(szRaceNameStart), szRaceName);
			  }

			  if (sfQuery.GetCol(1,&flLapTemp))
			  {
				  int flTemp = _wtoi(szTmStartRace);
				  flLapTime = flLapTemp - (long long)flTemp;
				  if (recFlag == 0)
				  {
					  //	Initialize times for first data point
					  flStartLapTime = flLapTime;
					  flFinishLapTime = flLapTime;
					  recFlag = 1;
				  }
				  else if (flLapTemp > flFinishLapTime && wcscmp(szRaceNameStart, szRaceName) == 0)
				  {
					  flFinishLapTime = flLapTemp;
				  }
				  else
				  {
					  //	We have gone past the last lap for this Race_Id
					  //	Put the final time difference into the data structure
//					  flLapTime = iTmStartRace - flFinishLapTime;
					  //	Store the results in the data structure and reset the indicies
					  m_ScoringData[rec].db_iUnixFirstTime = (int)flStartLapTime;
					  swprintf(m_ScoringData[rec].db_strRaceName, NUMCHARS(m_ScoringData[rec].db_strRaceName), szRaceNameStart);
					  flFinishLapTime = iTmEndRace - flFinishLapTime;
					  m_ScoringData[rec].db_iUnixLastTime = (int)flFinishLapTime;
					  int iTotTime = m_ScoringData[rec].db_iUnixFirstTime + m_ScoringData[rec].db_iUnixLastTime;
					  TCHAR szText[MAX_PATH] = {NULL};
					  ::FormatTimeMinutesSecondsMs((float)iTotTime,szText,NUMCHARS(szText));
					  _snwprintf(m_ScoringData[rec].db_szTotTime, NUMCHARS(m_ScoringData[rec].db_szTotTime), L"%i / %s", m_ScoringData[rec].db_iTotLaps, szText);

					  swprintf(szRaceNameStart, NUMCHARS(szRaceNameStart), szRaceName);
					  flStartLapTime = flLapTime;
					  flFinishLapTime = flLapTime;
					  rec++;	//	Go to the next summary record and continue
				  }
			  }
			}

			//	We have gone to the last lap data record
			//	Put the final time difference into the data structure
			m_ScoringData[rec].db_iUnixFirstTime = (int)flStartLapTime;
			swprintf(m_ScoringData[rec].db_strRaceName, NUMCHARS(m_ScoringData[rec].db_strRaceName), szRaceNameStart);
			flFinishLapTime = iTmEndRace - flFinishLapTime;
			m_ScoringData[rec].db_iUnixLastTime = (int)flFinishLapTime;
			int iTotTime = m_ScoringData[rec].db_iUnixFirstTime + m_ScoringData[rec].db_iUnixLastTime;
			TCHAR szText[MAX_PATH] = {NULL};
			::FormatTimeMinutesSecondsMs((float)iTotTime,szText,NUMCHARS(szText));
			_snwprintf(m_ScoringData[rec].db_szTotTime, NUMCHARS(m_ScoringData[rec].db_szTotTime), L"%i / %s", m_ScoringData[rec].db_iTotLaps, szText);
			rec++;	//	Keep the counter correct
	  }

	  // set up List view items
			vector<wstring> lstPos;
			vector<wstring> lstRaceName;
			vector<wstring> lstLapTimes;

			ListView_DeleteAllItems(DlgScoring_hWnd);	//	Clear the list before displaying the update
			TCHAR szText[MAX_PATH] = {NULL};
			int nItem;
			LVITEM lvi;
			LPWSTR result;
			for (nItem = 0; nItem < rec; ++nItem)
			{
				lvi.mask = LVIF_TEXT | LVIF_PARAM;
				lvi.iItem = nItem;
				lvi.iSubItem = 0;
				lvi.lParam = nItem;
				swprintf(szTemp, NUMCHARS(szTemp), L"%i", m_ScoringData[nItem].db_iRaceId);
				std::wstring strTemp(szTemp);
				result = (LPWSTR)strTemp.c_str();	
				lvi.pszText = result;
				lvi.cchTextMax = wcslen(result);
				ListView_InsertItem(DlgScoring_hWnd, &lvi);

				// set up subitems
				lvi.mask = LVIF_TEXT;
				lvi.iItem = nItem;

				lvi.iSubItem = 1;
				std::wstring strRace(m_ScoringData[nItem].db_strRaceName);
				result = (LPWSTR)strRace.c_str();		  
				lvi.pszText = result;
				lvi.cchTextMax = wcslen(result);
				ListView_SetItem(DlgScoring_hWnd, &lvi);

				lvi.iSubItem = 2;
				std::wstring strTotTimes(m_ScoringData[nItem].db_szTotTime);
				result = (LPWSTR)strTotTimes.c_str();		  
				lvi.pszText = result;
				lvi.cchTextMax = wcslen(result);
				ListView_SetItem(DlgScoring_hWnd, &lvi);
			}
	  lstPos.clear();
	  lstRaceName.clear();
	  lstLapTimes.clear();
  }
  return 0;
}


//	Function updates the T&S screen for HPDE's and track days, based upon user choices for Race Sessions selected
DWORD* CDlgTimingScoring::TimingScoringProc(LPVOID pv, HWND hWnd)
{
  LPCTSTR lpszPath = (LPCTSTR)pv;
  CSfArtSQLiteDB sfDB;
  vector<wstring> lstTables;
  HWND Dlg_hWnd = GetDlgItem(hWnd, IDC_TIMINGSCORING);
  if(SUCCEEDED(sfDB.Open(lpszPath, lstTables, true)))
  {
	  //	Race ID's are stored in the sfResult.m_RaceId structure
	  int z = 0;
	  int z_RaceId[50] = {-1};
	  while (m_sfResult->m_RaceId[z] >= 0 && z < 50)
	  {
		z_RaceId[z] = m_sfResult->m_RaceId[z];
		z++;
	  }
	  vector<wstring> lstPos;
      vector<wstring> lstRaceName;
      vector<wstring> lstComment;
	  vector<wstring> lstLapTimes;
      CSfArtSQLiteQuery sfQuery(sfDB);
	  TCHAR szTmp[1080] = {NULL};
	  //	Now cycle through all selected RaceId's and get their laptimes and sort them
	  TCHAR szTemp[1080] = L"select races.name,laps.laptime,extras.comment from laps,races left join extras on extras.lapid = laps._id where laps.raceid=races._id and (";
	  for (int y = 0; y < z; y++)
	  {
			int s_RaceId = z_RaceId[y];
			if (s_RaceId != 0)
			{
				_snwprintf(szTemp, NUMCHARS(szTemp), L"%sraces._id = %i or ", szTemp, s_RaceId);
			}
	  }
	  swprintf(szTmp, wcslen(szTemp) - 3, L"%s", szTemp);
	  _snwprintf(szTemp, NUMCHARS(szTemp), L"%s) order by laptime asc limit 40", szTmp);
	  if(sfQuery.Init(szTemp))
	  {
			SYSTEMTIME st;
			GetSystemTime(&st);
			int z = 1;
			TCHAR szPos[MAX_PATH] = {NULL};
			TCHAR szRaceName[300] = {NULL};
			TCHAR szComment[300] = {NULL};
			TCHAR szLap[300] = {NULL};
			while(sfQuery.Next())
			{
			  float flLapTime = 0;
			  sfQuery.GetCol(0,szRaceName,NUMCHARS(szRaceName));
			  sfQuery.GetCol(1,&flLapTime);
			  sfQuery.GetCol(2,szComment,NUMCHARS(szComment));

			  ::FormatTimeMinutesSecondsMs(flLapTime,szLap,NUMCHARS(szLap));
			  _snwprintf(szPos,NUMCHARS(szPos),L"%i",z);
			  lstPos.push_back(szPos);
			  lstRaceName.push_back(szRaceName);
			  lstComment.push_back(szComment);
			  lstLapTimes.push_back(szLap);
			  z++;
			}
			HWND Dlg_hWnd = GetDlgItem(hWnd, IDC_TIMINGSCORING);
			ListView_DeleteAllItems(Dlg_hWnd);	//	Clear the list before displaying the update
			TCHAR szText[MAX_PATH] = {NULL};

			// set up list view items
			int nItem;
			LVITEM lvi;
			LPWSTR result;
			for (nItem = 0; nItem < z - 1; ++nItem)
			{
				lvi.mask = LVIF_TEXT | LVIF_PARAM;
				lvi.iItem = nItem;
				lvi.iSubItem = 0;
				lvi.lParam = nItem;
				std::wstring strPos(lstPos[nItem]);
				result = (LPWSTR)strPos.c_str();		  
				lvi.pszText = result;
				lvi.cchTextMax = wcslen(result);
				ListView_InsertItem(Dlg_hWnd, &lvi);

				// set up subitems
				lvi.mask = LVIF_TEXT;
				lvi.iItem = nItem;

				lvi.iSubItem = 1;
				std::wstring strRace(lstRaceName[nItem]);
				result = (LPWSTR)strRace.c_str();		  
				lvi.pszText = result;
				lvi.cchTextMax = wcslen(result);
				ListView_SetItem(Dlg_hWnd, &lvi);

				lvi.iSubItem = 2;
				std::wstring strComment(lstComment[nItem]);
				result = (LPWSTR)strComment.c_str();		  
				lvi.pszText = result;
				lvi.cchTextMax = wcslen(result);
				ListView_SetItem(Dlg_hWnd, &lvi);

				lvi.iSubItem = 3;
//				lvi.pszText = (LPWSTR)&lstLapTimes[nItem];
				std::wstring strLapTimes(lstLapTimes[nItem]);
				result = (LPWSTR)strLapTimes.c_str();		  
				lvi.pszText = result;
				lvi.cchTextMax = wcslen(result);
				ListView_SetItem(Dlg_hWnd, &lvi);
			}
	  }
	  lstPos.clear();
	  lstRaceName.clear();
	  lstComment.clear();
	  lstLapTimes.clear();
  }
  return 0;
}
