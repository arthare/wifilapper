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
		scoringLstCols.push_back(L"Lap/Time Behind");
		scoringLstWidths.push_back(30);
		scoringLstWidths.push_back(145);
		scoringLstWidths.push_back(95);
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
			tmEndRace = NULL;
			tmStartRace = timeGetTime();	//	Set the start time for this race session
			//	Let's format the time into hr:min
/*		    ::FormatTimeMinutesSecondsMs((tmStartRace - timeGetTime()) / 1000, szText, NUMCHARS(szText) );
			swprintf(szText, _tcslen(szText) - 2, L"%s", szText);	//	Remove the fractional time
			swprintf(szTemp, NUMCHARS(szTemp), szText);
			_snwprintf(szText, NUMCHARS(szText), L"Race started!\n%s", szTemp);
//			_snwprintf(szText, NUMCHARS(szText), L"Let the race begin!\n%12.0f", (float)tmStartRace);
*/			MessageBox(hWnd, L"New race started!", MB_OK,NULL);
			CRaceScoring((LPVOID) &m_szPath, hWnd);	//	Update the scoring list view
			m_pResults->fCancelled = false;
			return TRUE;
		}
        case IDENDRACE:
		{
			TCHAR szText[MAX_PATH] = {NULL};			
			TCHAR szTemp[MAX_PATH] = {NULL};			
			tmEndRace = timeGetTime();
		    ::FormatTimeMinutesSecondsMs((tmEndRace - tmStartRace) / 1000, szText, NUMCHARS(szText) );
			swprintf(szText, _tcslen(szText) - 2, L"%s", szText);	//	Remove the fractional time
			swprintf(szTemp, NUMCHARS(szTemp), szText);
			_snwprintf(szText, NUMCHARS(szText), L"Race has ended\n\nRace duration: %s", szTemp);
//			_snwprintf(szText, NUMCHARS(szText), L"Let the race end!\n%12.0f", (float)tmEndRace);
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
  vector<wstring> lstTables;
  HWND DlgScoring_hWnd = GetDlgItem(hWnd, IDC_RACESCORING);
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

	  //	First query for the total number of laps for each car
	  vector<wstring> lstPos;
      vector<wstring> lstRaceName;
	  vector<int> lstLaps;
	  vector<wstring> lstLapTimes;
	  TCHAR szTmStartRace[MAX_PATH] = {NULL};
	  TCHAR szTmEndRace[MAX_PATH] = {NULL};
      CSfArtSQLiteQuery sfQuery(sfDB);
	  TCHAR szTmp[2080] = {NULL};
	  //	Now cycle through all selected RaceId's and get the number of laptimes completed
	  TCHAR szTemp[1080] = L"select count(laps._id) from laps,races where laps.raceid=races._id and races._id = "; //? and laps.unixtime > ? and laps.unixtime < ?";
	  for (int y = 0; y < z; y++)
	  {
					long long iTmStartRace = 0;
					long long iTmEndRace = 0;
					double fTmStartRace = 1376100527;	//	Used for the TestRaces database only
//					double fTmStartRace = tmStartRace;
					iTmStartRace = (int)fTmStartRace;
					swprintf(szTmStartRace, NUMCHARS(szTmEndRace), L"%d", iTmStartRace); 
					double fTmEndRace = 1376100699;	//	Used for the TestRaces database only
//					double fTmEndRace = tmEndRace;
					iTmEndRace = (int)fTmEndRace;
					if (iTmEndRace <= 0)
					{
						swprintf(szTmEndRace, NUMCHARS(szTmEndRace), L"55555555555"); 
//						iTmEndRace = (long long)55555555555;
					}
					else
					{
						swprintf(szTmEndRace, NUMCHARS(szTmEndRace), L"%d", iTmEndRace); 
//						iTmEndRace = (long long)fTmEndRace;
					}
		    _snwprintf(szTmp, NUMCHARS(szTmp), L"%s%i and laps.unixtime > %s and laps.unixtime < %s", szTemp, z_RaceId[y], szTmStartRace, szTmEndRace/*iTmEndRace*/);
			if(sfQuery.Init(szTmp))
			{
				while(sfQuery.Next())
					{
//					sfQuery.BindValue(z_RaceId[y]);
//					sfQuery.BindValue((int)iTmStartRace);
//					sfQuery.BindValue((int)iTmEndRace);
					int cLaps = 0;
					if(sfQuery.GetCol(0,&cLaps))
					{
						lstLaps.push_back(cLaps);
					}
				}
			}
	  }



	  //	Now let's get their lap time information, so that we can figure out between-lap time differences

	  _snwprintf(szTmp, NUMCHARS(szTmp), L"");
	  //	Now cycle through all selected RaceId's and get their laptimes and sort them by time collected
	  _snwprintf(szTemp, NUMCHARS(szTemp), L"select races.name,laps.unixtime,laps._id from laps,races where laps.raceid=races._id and (");
	  for (int y = 0; y < z; y++)
	  {
			int s_RaceId = z_RaceId[y];
			if (s_RaceId != 0)
			{
				_snwprintf(szTemp, NUMCHARS(szTemp), L"%sraces._id = %i or ", szTemp, s_RaceId);
			}
	  }
	  swprintf(szTmp, wcslen(szTemp) - 3, L"%s", szTemp);
	  _snwprintf(szTemp, NUMCHARS(szTemp), L"%s) and laps.unixtime > %s and laps.unixtime < %s order by name and unixtime", szTmp, szTmStartRace, szTmEndRace);
	  int rec = 1;
	  if(sfQuery.Init(szTemp))
	  {
			TCHAR szPos[MAX_PATH] = {NULL};
			TCHAR szRaceName[300] = {NULL};
			TCHAR szLap[300] = {NULL};
			while(sfQuery.Next())
			{
			  long long flLapTime = 0;
			  if (sfQuery.GetCol(0,szRaceName,NUMCHARS(szRaceName)))
			  {
				  lstRaceName.push_back(szRaceName);
			  }
			  sfQuery.GetCol(1,&flLapTime);
//			  flLapTime = flLapTime - (float)tmStartRace;
//					flValue = _wtof(szText);
			  int flTemp = _wtoi(szTmStartRace);
			  flLapTime = flLapTime - (long long)flTemp;
			  int iTemp = _wtoi(szTmEndRace) - _wtoi(szTmStartRace);
			  ::FormatTimeMinutesSecondsMs(flLapTime,szLap,NUMCHARS(szLap));
			  _snwprintf(szPos,NUMCHARS(szPos),L"%i",rec);
			  lstPos.push_back(szPos);
			  lstLapTimes.push_back(szLap);
			  rec++;
			}
	  }

			// set up List view items
			ListView_DeleteAllItems(DlgScoring_hWnd);	//	Clear the list before displaying the update
			TCHAR szText[MAX_PATH] = {NULL};
			int nItem;
			LVITEM lvi;
			LPWSTR result;
			for (nItem = 0; nItem < rec - 1; ++nItem)
			{
				lvi.mask = LVIF_TEXT | LVIF_PARAM;
				lvi.iItem = nItem;
				lvi.iSubItem = 0;
				lvi.lParam = nItem;
				std::wstring strPos(lstPos[nItem]);
				result = (LPWSTR)strPos.c_str();		  
				lvi.pszText = result;
				lvi.cchTextMax = wcslen(result);
				ListView_InsertItem(DlgScoring_hWnd, &lvi);

				// set up subitems
				lvi.mask = LVIF_TEXT;
				lvi.iItem = nItem;

				lvi.iSubItem = 1;
				std::wstring strRace(lstRaceName[nItem]);
				result = (LPWSTR)strRace.c_str();		  
				lvi.pszText = result;
				lvi.cchTextMax = wcslen(result);
				ListView_SetItem(DlgScoring_hWnd, &lvi);

				lvi.iSubItem = 2;
				lvi.pszText = (LPWSTR)&lstLapTimes[nItem];
				std::wstring strLapTimes(lstLapTimes[nItem]);
				result = (LPWSTR)strLapTimes.c_str();		  
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
				lvi.pszText = (LPWSTR)&lstLapTimes[nItem];
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
