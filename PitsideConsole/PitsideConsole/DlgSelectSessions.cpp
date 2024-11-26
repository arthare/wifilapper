#include "Stdafx.h"
#include "DlgSelectSessions.h"
#include "resource.h"
#include "PitsideConsole.h"
#include "LapReceiver.h"
#include "ArtSQL/ArtSQLite.h"
LRESULT CDlgSelectSessions::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
      lstWidths.push_back(85);
      lstWidths.push_back(160);
      lstWidths.push_back(40);
      sfListBox.Init2(GetDlgItem(hWnd,IDC_RACE),lstCols,lstWidths);

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
          set<LPARAM> setSelected = sfListBox.GetSelectedItemsData2();
          if(setSelected.size() > 0)
          {
			//   Need to find all Race Sessions selected
			int z = 0;
			for(set<LPARAM>::const_iterator i = setSelected.begin(); i != setSelected.end(); i++)
			{
				m_pResults->m_RaceId[z] = *i;
				z++;
				if (z >= 50) break;	//	Maximum of 50 race sessions to display
			}
			m_pResults->fCancelled = false;
			EndDialog(hWnd,0);
		  }
          else
          {
			//	Do nothing, no race sessions chosen
			int iRet = MessageBox(NULL,L"No race sessions selected\n\nDo you want to start a race without any racers?\n\n(Do this if you want to capture the Start and End time markers\nfor Re-Running the race scenario once all data is collected)",L"", MB_YESNO);
			if(iRet == IDYES)
			{
				m_pResults->m_RaceId[0] = -1;	//	No Race_Id stored, so let's just save the start/end markers
				m_pResults->fCancelled = false;
				EndDialog(hWnd,0);
			}
          }
          return TRUE;
        }
        case IDCANCEL:
		{
          m_pResults->fCancelled = true;
          EndDialog(hWnd,0);
          return TRUE;
		}
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

