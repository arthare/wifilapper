#include "Stdafx.h"
#include "DlgRaceSelectEdit.h"
#include "resource.h"
#include "PitsideConsole.h"
#include "LapReceiver.h"
#include "ArtSQL/ArtSQLite.h"
LRESULT CRaceSelectEditDlg::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
    {
      vector<wstring> lstCols;
      vector<int> lstWidths;
      lstCols.push_back(L"Date");
      lstCols.push_back(L"Race Name");
      lstCols.push_back(L"Laps");
      lstWidths.push_back(75);
      lstWidths.push_back(160);
      lstWidths.push_back(40);
      sfListBox.Init(GetDlgItem(hWnd,IDC_RACE),lstCols,lstWidths);

      // gotta set up the list
      vector<RACEDATA> lstRaces = m_pLapDB->GetRaces();
      
      for(int x = 0;x < lstRaces.size(); x++)
      {
        vector<wstring> lstCols;

        SYSTEMTIME stStart = SecondsSince1970ToSYSTEMTIME(lstRaces[x].unixtime);
        TCHAR szDate[100];
        _snwprintf(szDate,NUMCHARS(szDate),L"%d/%d/%4d",stStart.wMonth,stStart.wDay,stStart.wYear);
        lstCols.push_back(szDate);
              
        lstCols.push_back(lstRaces[x].strName.c_str());

        TCHAR szTemp[MAX_PATH];
        _snwprintf(szTemp, NUMCHARS(szTemp), L"%d",lstRaces[x].laps);
        lstCols.push_back(szTemp);

        sfListBox.AddStrings(lstCols,lstRaces[x].raceId);
      }

      break;
    }
    case WM_COMMAND:
    {
      switch(LOWORD(wParam))
      {
        case IDOK:
        {
          set<LPARAM> set = sfListBox.GetSelectedItemsData();
          if(set.size() == 1)
          {
            m_pResults->iRaceId = *set.begin();
            m_pResults->fCancelled = false;
            EndDialog(hWnd,0);
          }
          else
          {
              if(set.size() >= 1)
              {
//                for(set<LPARAM> set::iterator i = set.begin(); i != set.end(); i++)
                {
					m_pResults->iRaceId = *set.begin();
					m_pResults->fCancelled = false;
					EndDialog(hWnd,0);
                }
              }

          }
          return TRUE;
        }
        case IDC_RACEEDIT_MERGE:
        {
          set<LPARAM> set = sfListBox.GetSelectedItemsData();
          if(set.size() == 1)
          {
			//	Do nothing, only 1 race session chosen
		  }
          else if(set.size() >= 1)
		  {
			  //	Need to find all Race Sessions selected, and then merge them into a single RaceID
			  //	int m_iRaceId(100), x;
			  //	x = 0;
			  //	for(set<LPARAM> set::iterator i = set.begin(); i != set.end(); i++)
			  //		{
			  //			m_iRaceId(x) = *i;
				//			x++;
				//		}
			  //	Now let's change the SQL RaceId so that they are all the same as set.beging()

				{
					m_pResults->iRaceId = *set.begin();
					m_pResults->fCancelled = false;
					EndDialog(hWnd,0);
				}
		  }
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

