#include "Stdafx.h"
#include "DlgTimingScoring.h"
#include "resource.h"
#include "PitsideConsole.h"
#include "LapReceiver.h"
#include "ArtSQL/ArtSQLite.h"
#include <string.h>
#include <fstream>
#include "DlgRaceRerun.h"

//	Set up global variables for listview processing
static TCHAR szTitle[MAX_PATH];
SCORINGDATA m_ScoringData[50];
HWND HL_hWnd;
HWND TS_hWnd;
// HWND click_hWnd;
// HWND hWnd;
LVITEM p_HLlvi;				//	Listview global pointer for Hot Laps
LVITEM p_TSlvi;				//	Listview global pointer for Race Scoring

//	Routines for sorting list views by column headers
int CALLBACK CompareHLListItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
    // Get the text of the list items for comparison
	LVITEM pitem1 = {NULL};
	LVITEM pitem2 = {NULL};
	TCHAR sz_Result1[512] = {NULL};
	TCHAR sz_Result2[512] = {NULL};
	BOOL result;
	BOOL b_TextResult;
	BOOL bSortAscending = (lParamSort > 0);	//	Determines which way to sort the column
    int nColumn = abs(lParamSort) - 1;	//	Then pulls the column number from the same variable

	if ( HL_hWnd )	//	Hot Laps listview
	{
		//	Get the text value for the given items and compare them
		p_HLlvi.iItem = lParam1;
		p_HLlvi.iSubItem =nColumn;
		ListView_GetItem(HL_hWnd, (LVITEM*)&p_HLlvi);
		swprintf(sz_Result1, p_HLlvi.cchTextMax, L"%s", p_HLlvi.pszText);
		p_HLlvi.iItem = lParam2;
		p_HLlvi.iSubItem = nColumn;
		ListView_GetItem(HL_hWnd, (LVITEM*)&p_HLlvi);
		swprintf(sz_Result2, p_HLlvi.cchTextMax, L"%s", p_HLlvi.pszText);
	}

//	b_TextResult = wcscmp(sz_Result2, sz_Result1);
	b_TextResult = _wcsicmp(sz_Result1, sz_Result2);
	if (bSortAscending > 0 && b_TextResult < 0)
//	if (bSortAscending && b_TextResult < 0)
	{
		result = -1 * abs((lParam1 - lParam2));
//		result = (lParam2 - lParam1);
	}
	else if (bSortAscending > 0 && b_TextResult >= 0)
//	else if (bSortAscending && b_TextResult >= 0)
	{
		result = 1 * abs((lParam1 - lParam2));
//		result = (lParam1 - lParam2);
	}
	else if (b_TextResult < 0)
//	else if (b_TextResult < 0)
	{
		result = 1 * abs(lParam2 - lParam1);
//		result = (lParam1 - lParam2);
	}
	else
	{
		result = -1 * abs(lParam2 - lParam1);
//		result = (lParam2 - lParam1);
	}

//	result = (bSortAscending && b_TextResult < 0) ? -1 : 1;
	if (nColumn == 0) 
	{
		result = bSortAscending ? (lParam1 - lParam2) : (lParam2 - lParam1);
	}
	return result;
}

int CALLBACK CompareTSListItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
    // Get the text of the list items for comparison
	LVITEM pitem1 = {NULL};
	LVITEM pitem2 = {NULL};
	TCHAR sz_Result1[512] = {NULL};
	TCHAR sz_Result2[512] = {NULL};
	BOOL result;
	BOOL b_TextResult;
	BOOL bSortAscending = (lParamSort > 0);	//	Determines which way to sort the column
    int nColumn = abs(lParamSort) - 1;	//	Then pulls the column number from the same variable

	if ( TS_hWnd )
	{
		//	Get the text value for the given items and compare them
		p_TSlvi.iItem = lParam1;
		p_TSlvi.iSubItem =nColumn;
		p_TSlvi.iItem = lParam2;
		p_TSlvi.iSubItem = nColumn;
		ListView_GetItem(TS_hWnd, (LVITEM*)&p_TSlvi);
		swprintf(sz_Result1, p_TSlvi.cchTextMax, L"%s", p_TSlvi.pszText, p_TSlvi.iSubItem);
		ListView_GetItem(TS_hWnd, (LVITEM*)&p_TSlvi);
		swprintf(sz_Result2, p_TSlvi.cchTextMax, L"%s", p_TSlvi.pszText, p_TSlvi.iSubItem);
	}
	else
	{
		return 0;	//	Should not hit this
	}

//	b_TextResult = wcscmp(sz_Result2, sz_Result1);
	b_TextResult = wcscmp(sz_Result1, sz_Result2);
	if (bSortAscending > 0 && b_TextResult < 0)
	{
		result = -1 * abs((lParam1 - lParam2));
	}
	else if (bSortAscending > 0 && b_TextResult >= 0)
	{
		result = 1 * abs((lParam1 - lParam2));
	}
	else if (b_TextResult < 0)
	{
		result = 1 * abs(lParam2 - lParam1);
	}
	else
	{
		result = -1 * abs(lParam2 - lParam1);
	}

//	result = (bSortAscending && b_TextResult < 0) ? -1 : 1;
	if (nColumn == 0) 
	{
		return bSortAscending ? (lParam1 - lParam2) : (lParam2 - lParam1);
	}
	else
		return result;
}

void OnColumnClick(LPNMLISTVIEW pLVInfo, HWND hWnd)
{
    static int nSortColumn = 0;
    static BOOL bSortAscending = TRUE;
    LPARAM lParamSort;

    // get new sort parameters
    if (pLVInfo->iSubItem == nSortColumn)
        bSortAscending = !bSortAscending;
    else
    {
        nSortColumn = pLVInfo->iSubItem;
        bSortAscending = !bSortAscending;
    }

    // combine sort info into a single value we can send to our sort function
    lParamSort = 1 + nSortColumn;
    if (!bSortAscending)
        lParamSort = -lParamSort;

//	click_hWnd = pLVInfo->hdr.hwndFrom;
//	HL_hWnd = hWnd;
//	TS_hWnd = hWnd;
//	HL_hWnd = GetDlgItem(hWnd, IDC_RACESCORING);
//	TS_hWnd = GetDlgItem(hWnd, IDC_TIMINGSCORING);

    // sort list
	if ( HL_hWnd && pLVInfo->hdr.hwndFrom == HL_hWnd)
		ListView_SortItems(pLVInfo->hdr.hwndFrom, CompareHLListItems, lParamSort);
	else
		ListView_SortItems(pLVInfo->hdr.hwndFrom, CompareTSListItems, lParamSort);

}

LRESULT CDlgTimingScoring::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static int tmStartRace, tmEndRace;	//	Variables for setting up receive time / live car position
	HL_hWnd = GetDlgItem(hWnd,IDC_TIMINGSCORING);	//	Hot Laps listview
	TS_hWnd = GetDlgItem(hWnd,IDC_RACESCORING);	//	Race Timing listview
	switch(uMsg)
  {
    case WM_INITDIALOG:
    {
		//	Initialize the results array
		for (int i=0;i<50;i++)
		{
			swprintf(m_ScoringData[i].db_strRaceName, NUMCHARS(m_ScoringData[i].db_strRaceName),L"");
			swprintf(m_ScoringData[i].db_szTotTime, NUMCHARS(m_ScoringData[i].db_szTotTime),L"");
		}
//		tmStartRace = NULL;
//		tmEndRace = NULL;	//	Set the initial End Race time to NULL
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
//		HL_hWnd = GetDlgItem(hWnd,IDC_TIMINGSCORING);	//	Hot Laps listview
		sfListBox.Init(HL_hWnd,lstCols,lstWidths);


		//	Now set up the Scoring list box
		vector<wstring> scoringLstCols;
		vector<int> scoringLstWidths;
		scoringLstCols.push_back(L"Pos");
		scoringLstCols.push_back(L"Name");
		scoringLstCols.push_back(L"Lap/Time Ahead");
		scoringLstWidths.push_back(30);
		scoringLstWidths.push_back(145);
		scoringLstWidths.push_back(95);
		sfListBox.Init(TS_hWnd,scoringLstCols,scoringLstWidths);
		HWND hWnd_Comment = GetDlgItem(hWnd, IDC_RACE_COMMENT);
		SetDlgItemText(hWnd, IDC_RACE_COMMENT, szTitle);

		TimingScoringProc((LPVOID)&m_szPath, hWnd);
		break;
    }
	case WM_NOTIFY:
	{
		// check for column click notification and sort the list view accordingly
		if ( ( ((LPNMHDR)lParam)->idFrom == IDC_RACESCORING) && ( ((LPNMHDR)lParam)->code == LVN_COLUMNCLICK) )
		{
//			TS_hWnd = GetDlgItem(hWnd,IDC_RACESCORING);
			OnColumnClick((LPNMLISTVIEW)lParam, TS_hWnd );
		}
		else if ( ( ((LPNMHDR)lParam)->idFrom == IDC_TIMINGSCORING) && ( ((LPNMHDR)lParam)->code == LVN_COLUMNCLICK) )
		{
//			HL_hWnd = GetDlgItem(hWnd,IDC_TIMINGSCORING);
			OnColumnClick((LPNMLISTVIEW)lParam, HL_hWnd );
		}
		return TRUE;
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
				CRaceScoring((LPVOID) &m_szPath, hWnd, tmStartRace, tmEndRace);
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
			if (tmEndRace == NULL && tmStartRace != NULL)
			{
				MessageBox(hWnd, L"Race already in progress!\n\nYou must end that race before starting a new one", L"***ERROR***", MB_OK);
			}
			else
			{
				if (tmStartRace > 0)	//	There is already a time marker stored, verify user wants to change this
				{
					DWORD dRet = MessageBox(hWnd, L"You already have a race stored!\n\nAre you sure you want to start a new race?\n\nPrevious race results will be lost if you haven't saved them", L"WARNING", MB_YESNO);
					if(dRet == IDNO)
					{
						//	Do nothing
						m_pResults->fCancelled = true;
						return TRUE;
					}
				}
				tmEndRace = NULL;	//	Remove any end of race marker when new race begins. INT format
				tmStartRace = GetSecondsSince1970();		//	Set the start time for this race session. Unixtime in INT format
//				tmStartRace = 1376100527;	// Used for the TestRaces database only
//				swprintf(szText, NUMCHARS(szText), L"Race Started\n\nTime = %i", tmStartRace);
//				MessageBox(hWnd, szText, L"Started", MB_OK);
				MessageBox(hWnd, L"Race started", L"Started", MB_OK);

				HWND prog_hWnd = GetDlgItem(hWnd, IDC_RACE_PROGRESS);
				swprintf(szText, NUMCHARS(szText), L"<<<< Race In Progress >>>>");
				SendMessage(prog_hWnd, WM_SETTEXT, 0, (LPARAM)szText);
			}
			m_pResults->fCancelled = false;
			return TRUE;
		}
		case IDENDRACE:
		{
			if (tmStartRace != NULL)
			{
				TCHAR szText[MAX_PATH] = {NULL};			
				TCHAR szTemp[MAX_PATH] = {NULL};			
				if (tmEndRace > 0)	//	There is already a time marker stored, verify user wants to change this
				{
					DWORD dRet = MessageBox(hWnd, L"You already have a race stored!\n\nAre you sure you want to change the end time for this race?\n\nPrevious race results will be lost if you haven't saved them", L"WARNING", MB_YESNO);
					if(dRet == IDNO)
					{
						//	Do nothing
						m_pResults->fCancelled = true;
						return TRUE;
					}
				}

				tmEndRace = GetSecondsSince1970();		//	Set the end time for this race session. Unixtime in INT format
//				tmEndRace = 1376100699;	// Used for the TestRaces database only
//				swprintf(szText, NUMCHARS(szText), L"Race End = %i", tmEndRace);
//				MessageBox(hWnd, szText, L"Ended", MB_OK);

				HWND prog_hWnd = GetDlgItem(hWnd, IDC_RACE_PROGRESS);
				swprintf(szText, NUMCHARS(szText), L">>>> Race Ended <<<<");
				SendMessage(prog_hWnd, WM_SETTEXT, 0, (LPARAM)szText);

				::FormatTimeMinutesSecondsMs((tmEndRace - tmStartRace), szText, NUMCHARS(szText) );
				TimingScoringProc((LPVOID)&m_szPath, hWnd);	//	Refresh the results one last time
				if (tmStartRace)
				{
					CRaceScoring((LPVOID) &m_szPath, hWnd, tmStartRace, tmEndRace);
				}

				swprintf(szTemp, NUMCHARS(szTemp), szText);
				_snwprintf(szText, NUMCHARS(szText), L"Race has ended\n\nRace duration: %s", szTemp);
				MessageBox(hWnd, szText, L"Ended", MB_OK);
			}
			else
			{
				MessageBox(hWnd, L"Race scoring has not been started", L"***ERROR***", MB_OK);
			}
			m_pResults->fCancelled = false;
			return TRUE;
		}
		case IDC_RACE_RERUN:
        {
			//	Let's set up for displaying the T&S page
			int m_RaceId[50] = {NULL};
			// Show the race-selection dialog and let the User pick which ones to use on T&S page
			RACERERUN_RESULT sfResult;
			if (sfResult.iStart <= 0 || tmStartRace > 0) sfResult.iStart = tmStartRace;
			if (sfResult.iEnd <= 0 || tmEndRace > 0) sfResult.iEnd = tmEndRace;

			CRaceRerunDlg dlgRace(&sfResult);
			ArtShowDialog<IDD_RACE_RERUN>(&dlgRace);

			if(!sfResult.fCancelled)
			{
				// Get the Start and End race markers and store them for race scoring
				tmStartRace = sfResult.iStart;
				tmEndRace = sfResult.iEnd;
				TimingScoringProc((LPVOID)&m_szPath, hWnd);	//	Refresh the results one last time
				if (tmStartRace)
				{
					CRaceScoring((LPVOID) &m_szPath, hWnd, tmStartRace, tmEndRace);
				}
			}
			return TRUE;
		}
        case IDC_RACE_SAVE:
		{
			if (tmEndRace == NULL && tmStartRace != NULL)
			{
				MessageBox(hWnd, L"Race in progress!\n\nYou must end the race before saving your results", L"***ERROR***", MB_OK);
			}
			else if (tmEndRace == NULL && tmStartRace == NULL)
			{
				MessageBox(hWnd, L"No race data to save", L"***ERROR***", MB_OK);
			}
			else
			{
				TCHAR szText[MAX_PATH] = {NULL};			
				TCHAR szTemp[MAX_PATH] = {NULL};			
				//	Save the results into a text file
				if(tmEndRace > 0)
				{
//				  swprintf(szText, NUMCHARS(szText), L"Race Start = %i\n\nRace End = %i", tmStartRace, tmEndRace);
//				  MessageBox(hWnd, szText, L"Saving", MB_OK);
				  TCHAR szFilename[MAX_PATH] = {NULL};
				  wcscat(szFilename,L"TimingScoring.txt");
				  while (true)
				  {
					if(ArtGetSaveFileName(hWnd, L"Choose Output file", szFilename, NUMCHARS(szFilename),L"TXT Files (*.txt)\0*.TXT\0\0"))
					{
						// let's make sure there's a .txt suffix on that bugger.
						if(!str_ends_with(szFilename,L".txt") && !str_ends_with(szFilename,L".TXT") )
						{
							wcsncat(szFilename,L".txt", NUMCHARS(szFilename));
						}
						const bool fFileIsNew = !DoesFileExist(szFilename);
						if(fFileIsNew)
						{
							break;	//	Exit loop, as file name is valid and new
						}
						else
						{
							DWORD dwRet = MessageBox(NULL,L"A file already exists with that name.\n\nAre you sure you want to overwrite it?",L"WARNING", MB_APPLMODAL | MB_ICONWARNING | MB_YESNO | MB_TOPMOST | MB_DEFBUTTON2);
							if (dwRet == IDYES)
							{
								break;	//	User wants to overwrite file, so exit loop and proceed
							}
						}
					}
					else
					{
						return 0;	//	User cancelled the save operation, so leave subroutine
					}
				  }

					//	Open up the file and write the information to it
					wofstream out;
					out.open(szFilename);

					//	First the race title information
					HWND hWnd_Comment = GetDlgItem(hWnd, IDC_RACE_COMMENT);
					int len;
					len = GetWindowTextLength(hWnd_Comment);
					GetDlgItemText(hWnd, IDC_RACE_COMMENT, szTitle, len+1);
					out<<szTitle<<endl<<endl;

					out<<L"File Name: "<<m_szPath<<endl;
					//	Let's save the start/end markers so the race can be recreated if needed
					_snwprintf(szText, NUMCHARS(szText), L"Race start marker:\t%i", tmStartRace);
					out<<szText<<endl;
					_snwprintf(szText, NUMCHARS(szText), L"Race ending marker:\t%i", tmEndRace);
					out<<szText<<endl;
					//	Now the race duration
					::FormatTimeMinutesSecondsMs((tmEndRace - tmStartRace), szText, NUMCHARS(szText) );
	//				swprintf(szText, _tcslen(szText) - 2, L"%s", szText);	//	Remove the fractional time
					swprintf(szTemp, NUMCHARS(szTemp), szText);
					_snwprintf(szText, NUMCHARS(szText), L"Race duration:\t%s", szTemp);

					out<<szText<<endl<<endl;
					out<<L"ID\tName\t\t\t\t\tLap/Time Ahead"<<endl;
					out<<L"====================================================================="<<endl;

					for(int i = 0; i < 25; i++)
					{
						if (m_ScoringData[i].db_iRaceId <= 0 || m_ScoringData[i].db_iUnixFirstTime == 0) break;
						int Temp = m_ScoringData[i].db_iUnixFirstTime + m_ScoringData[i].db_iUnixLastTime;
						::FormatTimeMinutesSecondsMs(Temp, szTemp, NUMCHARS(szTemp) );
						out << m_ScoringData[i].db_iRaceId << L"\t";
						out << m_ScoringData[i].db_strRaceName << L"\t";
						out << m_ScoringData[i].db_szTotTime << endl;
					}
					out<<endl;

					//	Now let's push out all of the Top 40 Hot Laps to the file

					out<<endl<<endl;
					out<<L"\t\tTop 40 Hot Laps"<<endl;
					out<<L"Pos\tName\t\t\t\t\tComment\tLap Time"<<endl;
					out<<L"====================================================================="<<endl;
					for(int i = 0; i < 40; i++)
					{
						if (_wtoi(m_ScoringData[i].lstPos) <= 0) break;
						out << m_ScoringData[i].lstPos << L"\t";
						out << m_ScoringData[i].lstRaceName << L"\t";
						out << m_ScoringData[i].lstComment << L"\t\t";
						out << m_ScoringData[i].lstLapTimes << endl;
					}
					out<<endl;

					out.close();	//	Close the file
					MessageBox(hWnd, L"Race Results Saved", L"Saved", MB_OK);
	
					m_pResults->fCancelled = false;
					return TRUE;
				}
				else
				{
					swprintf(szText, NUMCHARS(szText), L"Race Start = %i\n\nRace End = %i", tmStartRace, tmEndRace);
					MessageBox(hWnd, szText, MB_OK, NULL);
				}
			}
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

// Function calculates the current laps and elapsed time since the race started
DWORD* CDlgTimingScoring::CRaceScoring(LPVOID pv, HWND hWnd, int tmStartRace, int tmEndRace)
{
  LPCTSTR lpszPath = (LPCTSTR)pv;
  CSfArtSQLiteDB sfDB;
  vector<wstring> lstTables;
//  HWND DlgScoring_hWnd = GetDlgItem(hWnd, IDC_RACESCORING);
//  TS_hWnd = GetDlgItem(hWnd, IDC_RACESCORING);
  if(SUCCEEDED(sfDB.Open(lpszPath, lstTables, true)))
  {
		// Race ID's are stored in the sfResult.m_RaceId structure
		int z = 0;
		while (m_sfResult->m_RaceId[z] >= 0 && z < 50)
		{
			m_ScoringData[z].db_iRaceId = m_sfResult->m_RaceId[z];
			z++;
		}

		// First query for the total number of laps for each car
		TCHAR szTmStartRace[MAX_PATH] = {NULL};
		TCHAR szTmEndRace[MAX_PATH] = {NULL};

		// First let's make the strings for Start and End times
		long long iTmStartRace = 0;
		long long iTmEndRace = 0;
		iTmStartRace = tmStartRace;
		swprintf(szTmStartRace, NUMCHARS(szTmEndRace), L"%d", iTmStartRace);
		iTmEndRace = tmEndRace;
		if (iTmEndRace <= 0)
		{
			swprintf(szTmEndRace, NUMCHARS(szTmEndRace), L"5555555555");
		}
		else
		{
			swprintf(szTmEndRace, NUMCHARS(szTmEndRace), L"%d", iTmEndRace);
		}

		// Now cycle through all selected RaceId's and get the number of laps completed
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
					// sfQuery.BindValue(m_ScoringData[y].db_iRaceId);
					// sfQuery.BindValue((int)iTmStartRace);
					// sfQuery.BindValue((int)iTmEndRace);
					int cLaps = 0;
					if(sfQuery.GetCol(0,&cLaps))
					{
						m_ScoringData[y].db_iTotLaps = cLaps;	// Store the lap count value in the data structure
					}
				}
			}
		}

		// Now let's get their lap time information, so that we can figure out between-lap time differences
		// Right now we are just getting the time difference between EndRace time and the last lap time

		// First, cycle through all selected RaceId's and get all of their laptimes and sort them by race name and then by time collected in the given timespan
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
					if (recFlag == 0) swprintf(szRaceNameStart, NUMCHARS(szRaceNameStart), szRaceName);
				}

				if (sfQuery.GetCol(1,&flLapTemp))
				{
					int flTemp = _wtoi(szTmStartRace);
					flLapTime = flLapTemp - (long long)flTemp;
					if (recFlag == 0)
					{
						// Initialize times for first data point
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
						// We have gone past the last lap for this Race_Id
						// Store the results in the data structure and reset the indicies
						m_ScoringData[rec].db_iUnixFirstTime = (int)flStartLapTime;
						swprintf(m_ScoringData[rec].db_strRaceName, NUMCHARS(m_ScoringData[rec].db_strRaceName), szRaceNameStart);
						flFinishLapTime = iTmEndRace - flFinishLapTime;
						m_ScoringData[rec].db_iUnixLastTime = (int)flFinishLapTime;
						int iTotTime = m_ScoringData[rec].db_iUnixFirstTime + m_ScoringData[rec].db_iUnixLastTime;
						TCHAR szText[MAX_PATH] = {NULL};
						::FormatTimeMinutesSecondsMs((float)iTotTime,szText,NUMCHARS(szText));
						if (iTmEndRace > 0)
						{
							_snwprintf(m_ScoringData[rec].db_szTotTime, NUMCHARS(m_ScoringData[rec].db_szTotTime), L"%i / %s", m_ScoringData[rec].db_iTotLaps, szText);
						}
						else
						{
							_snwprintf(m_ScoringData[rec].db_szTotTime, NUMCHARS(m_ScoringData[rec].db_szTotTime), L"%i / TBD", m_ScoringData[rec].db_iTotLaps);
						}
						swprintf(szRaceNameStart, NUMCHARS(szRaceNameStart), szRaceName);
						flStartLapTime = flLapTime;
						flFinishLapTime = flLapTime;
						rec++;	// Go to the next summary record and continue
					}
				}
			}

			// We have gone to the last lap data record
			// Put the final time difference into the data structure
			m_ScoringData[rec].db_iUnixFirstTime = (int)flStartLapTime;
			swprintf(m_ScoringData[rec].db_strRaceName, NUMCHARS(m_ScoringData[rec].db_strRaceName), szRaceNameStart);
			flFinishLapTime = iTmEndRace - flFinishLapTime;
			m_ScoringData[rec].db_iUnixLastTime = (int)flFinishLapTime;
			int iTotTime = m_ScoringData[rec].db_iUnixFirstTime + m_ScoringData[rec].db_iUnixLastTime;
			TCHAR szText[MAX_PATH] = {NULL};
			::FormatTimeMinutesSecondsMs((float)iTotTime,szText,NUMCHARS(szText));
			if (iTmEndRace > 0)
			{
				_snwprintf(m_ScoringData[rec].db_szTotTime, NUMCHARS(m_ScoringData[rec].db_szTotTime), L"%i / %s", m_ScoringData[rec].db_iTotLaps, szText);
			}
			else
			{
				_snwprintf(m_ScoringData[rec].db_szTotTime, NUMCHARS(m_ScoringData[rec].db_szTotTime), L"%i / TBD", m_ScoringData[rec].db_iTotLaps);
			}
			rec++;	// Keep the counter correct
		}
		sfDB.Close();	//	Close the file

		// set up List view items
		ListView_DeleteAllItems(TS_hWnd);	// Clear the list before displaying the update
		TCHAR szText[MAX_PATH] = {NULL};
		int nItem;
//		LVITEM lvi;
		LPWSTR result;
		for (nItem = 0; nItem < rec; ++nItem)
		{
		p_TSlvi.mask = LVIF_TEXT | LVIF_PARAM;
		p_TSlvi.iItem = nItem;
		p_TSlvi.iSubItem = 0;
		p_TSlvi.lParam = nItem;
		swprintf(szTemp, NUMCHARS(szTemp), L"%i", m_ScoringData[nItem].db_iRaceId);
		std::wstring strTemp(szTemp);
		result = (LPWSTR)strTemp.c_str();	
		p_TSlvi.pszText = result;
		p_TSlvi.cchTextMax = wcslen(result);
		ListView_InsertItem(TS_hWnd, &p_TSlvi);

		// set up subitems
		p_TSlvi.mask = LVIF_TEXT;
		p_TSlvi.iItem = nItem;

		p_TSlvi.iSubItem = 1;
		std::wstring strRace(m_ScoringData[nItem].db_strRaceName);
		result = (LPWSTR)strRace.c_str();	
//		p_TSlvi.lParam = (LPARAM) result;	//	Try this for LPARAM
		p_TSlvi.lParam = nItem;
		p_TSlvi.pszText = result;
		p_TSlvi.cchTextMax = wcslen(result);
		ListView_SetItem(TS_hWnd, &p_TSlvi);

		p_TSlvi.iSubItem = 2;
		std::wstring strTotTimes(m_ScoringData[nItem].db_szTotTime);
		result = (LPWSTR)strTotTimes.c_str();	
//		p_TSlvi.lParam = (LPARAM) result;	//	Try this for LPARAM
		p_TSlvi.lParam = nItem;
		p_TSlvi.pszText = result;
		p_TSlvi.cchTextMax = wcslen(result);
		ListView_SetItem(TS_hWnd, &p_TSlvi);
	}
  }
  return 0;
}



//	Function updates the HotLap screen for HPDE's and track days, based upon user choices for Race Sessions selected
DWORD* CDlgTimingScoring::TimingScoringProc(LPVOID pv, HWND hWnd)
{
  LPCTSTR lpszPath = (LPCTSTR)pv;
  CSfArtSQLiteDB sfDB;
  vector<wstring> lstTables;
//  HL_hWnd = GetDlgItem(hWnd, IDC_TIMINGSCORING);
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
			//	Load up all of the HL vectors with data from the database
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
//			HWND HL_hWnd = GetDlgItem(hWnd, IDC_TIMINGSCORING);
			ListView_DeleteAllItems(HL_hWnd);	//	Clear the list before displaying the update
			ClearHotLaps();	//	Clear the Top 40 Hot Laps list before updating
			TCHAR szText[MAX_PATH] = {NULL};

			// set up list view items
			int nItem;
//			LVITEM p_HLlvi;
			LPWSTR result;
			for (nItem = 0; nItem < z - 1; ++nItem)
			{
				p_HLlvi.mask = LVIF_TEXT | LVIF_PARAM;
				p_HLlvi.iItem = nItem;
				p_HLlvi.iSubItem = 0;
				p_HLlvi.lParam = nItem;
				std::wstring strPos(lstPos[nItem]);
				result = (LPWSTR)strPos.c_str();		  
				p_HLlvi.pszText = result;
				p_HLlvi.cchTextMax = wcslen(result);
				ListView_InsertItem(HL_hWnd, &p_HLlvi);

				// set up subitems
				p_HLlvi.mask = LVIF_TEXT;
				p_HLlvi.iItem = nItem;

				p_HLlvi.iSubItem = 1;
				std::wstring strRace(lstRaceName[nItem]);
				result = (LPWSTR)strRace.c_str();		  
				//From TCHAR to DWORD.
//				DWORD dwSomeNum;
//				dwSomeNum = wcstod(result, _T('\0'));
//				p_HLlvi.lParam = (LPARAM) dwSomeNum;	//	Try this for LPARAM
				p_HLlvi.lParam = nItem;
				p_HLlvi.pszText = result;
				p_HLlvi.cchTextMax = wcslen(result);
				ListView_SetItem(HL_hWnd, &p_HLlvi);

				p_HLlvi.iSubItem = 2;
				std::wstring strComment(lstComment[nItem]);
				result = (LPWSTR)strComment.c_str();		  
				//From TCHAR to DWORD.
//				dwSomeNum = wcstod(result, _T('\0'));
//				p_HLlvi.lParam = (LPARAM) dwSomeNum;	//	Try this for LPARAM
				p_HLlvi.lParam = nItem;
				p_HLlvi.pszText = result;
				p_HLlvi.cchTextMax = wcslen(result);
				ListView_SetItem(HL_hWnd, &p_HLlvi);

				p_HLlvi.iSubItem = 3;
				p_HLlvi.pszText = (LPWSTR)&lstLapTimes[nItem];
				std::wstring strLapTimes(lstLapTimes[nItem]);
				result = (LPWSTR)strLapTimes.c_str();		  
				//From TCHAR to DWORD.
//				dwSomeNum = wcstod(result, _T('\0'));
//				p_HLlvi.lParam = (LPARAM) dwSomeNum;	//	Try this for LPARAM
				p_HLlvi.lParam = nItem;
				p_HLlvi.pszText = result;
				p_HLlvi.cchTextMax = wcslen(result);
				ListView_SetItem(HL_hWnd, &p_HLlvi);
			}
	  
			//	Now load the RACERESULTS vectors with the Top 40 Hot Laps for saving to a text file
			for (int y = 0; y < z -1; y++)
			{
				std::wstring strPos(lstPos[y]);
				result = (LPWSTR)strPos.c_str();		  
				_snwprintf(m_ScoringData[y].lstPos, NUMCHARS(m_ScoringData[y].lstPos), result);

				std::wstring strRace(lstRaceName[y]);
				result = (LPWSTR)strRace.c_str();		  
				_snwprintf(m_ScoringData[y].lstRaceName, NUMCHARS(m_ScoringData[y].lstRaceName), result);

				std::wstring strComment(lstComment[y]);
				result = (LPWSTR)strComment.c_str();		  
				_snwprintf(m_ScoringData[y].lstComment, NUMCHARS(m_ScoringData[y].lstComment), result);

				std::wstring strLapTimes(lstLapTimes[y]);
				result = (LPWSTR)strLapTimes.c_str();		  
				_snwprintf(m_ScoringData[y].lstLapTimes, NUMCHARS(m_ScoringData[y].lstLapTimes), result);
			}
	  }
	  lstPos.clear();
	  lstRaceName.clear();
	  lstComment.clear();
	  lstLapTimes.clear();
	  sfDB.Close();
  }
  return 0;
}

int CDlgTimingScoring::str_ends_with(const TCHAR * str, const TCHAR * suffix) 
{
	if( str == NULL || suffix == NULL )
		return 0;

	size_t str_len = wcslen(str);
	size_t suffix_len = wcslen(suffix);

	if(suffix_len > str_len)
		return 0;

	return 0 == wcsncmp( str + str_len - suffix_len, suffix, suffix_len );
}

void CDlgTimingScoring::ClearHotLaps()
{
	for (int x = 0; x < 50; x++)
	{
		swprintf(m_ScoringData[x].lstPos, NUMCHARS(m_ScoringData[x].lstPos), L"");
		swprintf(m_ScoringData[x].lstRaceName, NUMCHARS(m_ScoringData[x].lstRaceName), L"");
		swprintf(m_ScoringData[x].lstComment, NUMCHARS(m_ScoringData[x].lstComment), L"");
		swprintf(m_ScoringData[x].lstLapTimes, NUMCHARS(m_ScoringData[x].lstLapTimes), L"");
	}
}