#include "Stdafx.h"
#include "DlgTimingScoring.h"
#include "resource.h"
#include "PitsideConsole.h"
#include "LapReceiver.h"
#include "ArtSQL/ArtSQLite.h"

LRESULT CDlgTimingScoring::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
    {
      vector<wstring> lstCols;
      vector<int> lstWidths;
      lstCols.push_back(L"Pos");
      lstCols.push_back(L"Name");
      lstCols.push_back(L"Laptime");
      lstWidths.push_back(80);
      lstWidths.push_back(250);
      lstWidths.push_back(90);
	  HWND Dlg_hWnd = GetDlgItem(hWnd,IDC_TIMINGSCORING);
      sfListBox.Init(Dlg_hWnd,lstCols,lstWidths);

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
            m_pResults->fCancelled = false;
			return TRUE;
        }
        case IDCANCEL:
          m_pResults->fCancelled = true;
          EndDialog(hWnd,0);
          return TRUE;
      }
      break;
    } // end WM_COMMAND
    case WM_CLOSE:
    {
      m_pResults->fCancelled = true;
      EndDialog(hWnd,0);
      break;
    }
  }
  return FALSE;
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
	  vector<wstring> lstLapTimes;
      CSfArtSQLiteQuery sfQuery(sfDB);
	  TCHAR szTmp[512] = {NULL};
	  TCHAR szTemp[512] = L"select races.name,laps.laptime from laps,races where laps.raceid=races._id and (";
	  //	Now cycle through all selected RaceId's and get their laptimes and sort them
	  for (int y = 0; y < z; y++)
	  {
			int s_RaceId = z_RaceId[y];
			if (s_RaceId != 0)
			{
				_snwprintf(szTemp, NUMCHARS(szTemp), L"%sraces._id = %i or ", szTemp, s_RaceId);
			}
	  }
	  _snwprintf(szTmp, wcslen(szTemp) - 4, L"%s", szTemp);
	  _snwprintf(szTemp, NUMCHARS(szTemp), L"%s) order by laptime asc limit 40", szTmp);
	  if(sfQuery.Init(szTemp))
	  {
			SYSTEMTIME st;
			GetSystemTime(&st);
			int z = 1;
			TCHAR szPos[MAX_PATH] = {NULL};
			TCHAR szRaceName[300] = {NULL};
			TCHAR szLap[300] = {NULL};
			while(sfQuery.Next())
			{
			  float flLapTime = 0;
			  sfQuery.GetCol(0,szRaceName,NUMCHARS(szRaceName));
			  sfQuery.GetCol(1,&flLapTime);

			  ::FormatTimeMinutesSecondsMs(flLapTime,szLap,NUMCHARS(szLap));
			  _snwprintf(szPos,NUMCHARS(szPos),L"%i",z);
			  lstPos.push_back(szPos);
			  lstRaceName.push_back(szRaceName);
			  lstLapTimes.push_back(szLap);
			  z++;
			}
			HWND Dlg_hWnd = GetDlgItem(hWnd, IDC_TIMINGSCORING);
			ListView_DeleteAllItems(Dlg_hWnd);
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
				lvi.pszText = (LPWSTR)&lstLapTimes[nItem];
				std::wstring strLapTimes(lstLapTimes[nItem]);
				result = (LPWSTR)strLapTimes.c_str();		  
				lvi.pszText = result;
				lvi.cchTextMax = wcslen(result);
				ListView_SetItem(Dlg_hWnd, &lvi);
			}
	  }
  }
  return 0;
}
