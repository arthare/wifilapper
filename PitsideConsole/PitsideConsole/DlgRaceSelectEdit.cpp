#include "Stdafx.h"
#include "DlgRaceSelectEdit.h"
#include "resource.h"
#include "PitsideConsole.h"
#include "LapReceiver.h"
#include "ArtSQL/ArtSQLite.h"
#include "SQLiteLapDB.h"	//	Added by KDJ
#include "DlgRaceEditConfirm.h"

extern ILapReceiver* g_pLapDB;

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
			MessageBox(NULL,L"Only 1 race session selected\n\nNo changes were made",L"", MB_OK);
            m_pResults->iRaceId = *set.begin();
            m_pResults->fCancelled = false;
            EndDialog(hWnd,0);
          }
          else
          {
              if(set.size() >= 1)
              {
					MessageBox(NULL,L"No changes were made",L"", MB_OK);
					m_pResults->iRaceId = *set.begin();
					m_pResults->fCancelled = false;
					EndDialog(hWnd,0);
              }
          }
          return TRUE;
        }
        case IDC_RACEEDIT_MERGE:
        {
		  //	Let's make sure that the user really wants to do this.
		  RACEEDITCONFIRM_RESULT sfResult;
		  CRaceEditConfirmDlg dlgRaceEditConfirm(&sfResult);
		  ArtShowDialog<IDD_RACEEDITCONFIRM>(&dlgRaceEditConfirm);
		  if(!sfResult.fCancelled)
		  {
			  // Okay they are serious and really want to merge these race sessions
			  set<LPARAM> setSelected = sfListBox.GetSelectedItemsData();
			  if(setSelected.size() == 1)
			  {
				//	Do nothing, only 1 race session chosen
				MessageBox(NULL,L"Only 1 race session selected\n\nNo changes were made",L"", MB_OK);
			  }
			  else if(setSelected.size() >= 1)
			  {
				//   Need to find all Race Sessions selected, and then merge them into a single RaceID
				int iFirstRaceId = -1;
				for(set<LPARAM>::const_iterator i = setSelected.begin(); i != setSelected.end(); i++)
				{
				  if(iFirstRaceId == -1)
				  {
					iFirstRaceId = *i;
					continue; // don't need to merge this lap with itself
				  }
				  else
				  {
					bool RaceCheck = g_pLapDB->MergeLaps(iFirstRaceId, *i); // merges the current race with the first race.
					if (RaceCheck == false)
					{
					  MessageBox(NULL,L"Race session merging failed!\n\nPost on Wifilapper Forum about this issue",L"", MB_OK);
					}
				  }
				}
				//	Finally, let's load this new combined Race Session as current and close the dialog box.
				if (iFirstRaceId != -1) m_pResults->iRaceId = iFirstRaceId;
				m_pResults->fCancelled = false;
				EndDialog(hWnd,0);
			  }
			  return TRUE;
		  }
          m_pResults->fCancelled = true;	//	User cancelled the operation at the warning/confirm screen
          EndDialog(hWnd,0);
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
