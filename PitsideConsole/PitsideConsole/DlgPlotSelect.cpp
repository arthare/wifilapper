#include "Stdafx.h"
#include "DlgPlotSelect.h"
#include "resource.h"

#include "PitsideConsole.h"
#include "LapReceiver.h"
#include "ArtSQL/ArtSQLite.h"
#include "LapData.h"
#include "DlgRaceSelect.h"
#include "LapPainter.h"

bool fCancelled = false;
TCHAR szTemp[512];
map<int,CExtendedLap*> m_mapLaps; // maps from iLapId to a lap object
CExtendedLap* m_pReferenceLap;
ArtListBox m_LapList;
vector<CExtendedLap*> GetAllLaps();
int TotalYChannels;

void LoadLaps(ILapReceiver* pReceiver, int m_iRaceId)
  {
    vector<const ILap*> laps = pReceiver->GetLaps(m_iRaceId);
    m_pReferenceLap = NULL;
    for(int x = 0;x < laps.size(); x++)
    {
		const ILap* pLap = laps[x];
		// we don't have this lap yet, so let's put it in
		CExtendedLap* pNewLap = new CExtendedLap(pLap, m_pReferenceLap, pReceiver, true);
		m_mapLaps[pLap->GetLapId()] = pNewLap;
    }
  }

vector<CExtendedLap*> GetAllLaps()
  {
    set<LPARAM> setSelectedLaps = m_LapList.GetSelectedItemsData();
    vector<CExtendedLap*> lstLaps;
    for(map<int,CExtendedLap*>::const_iterator i = m_mapLaps.begin(); i != m_mapLaps.end(); i++)
    {
      CExtendedLap* pLap = i->second;
      lstLaps.push_back(pLap);
    }

    return lstLaps;
  }

    int CPlotSelectDlg::InitPlotPrefs(HWND hWnd, LPARAM lParam)
	{
		//  We need to get all of the Y-axis data channels being displayed
		// First load all laps and get their channels
		vector<RACEDATA> lstRaces = m_pLapDB->GetRaces();
		//	Pull the data channels for each lap from this race instance m_iRaceId
		m_pPlotResults->iPlotId = m_iRaceId;	//	Get the Selected Race ID.
		m_LapList.Clear();	//  Clear list of laps in memory and reload them.
		if (m_pLapDB->GetLapCount(m_pPlotResults->iPlotId) > 1)	//	If race has more than 1 lap, let's get the laps / channels
		{
			LoadLaps(m_pLapDB, m_pPlotResults->iPlotId);	//	Load all of the laps for this Race ID
			GetAllLaps();	//	Load up lstLaps with all available laps
		}
		//	Now that we have the lap list from the DB, let's get the data channels in them

		m_sfYAxis.Clear();
		set<LPARAM> setYSelected;
		vector<const ILap*> lstLaps = m_pLapDB->GetLaps(m_pPlotResults->iPlotId);
		for(int x = 0; x < lstLaps.size(); x++)
		{
			// build up the data channel set
			TotalYChannels = 1;
			TCHAR szDataChannelName[MAX_PATH];
			set<DATA_CHANNEL> channels = m_pLapDB->GetAvailableChannels(lstLaps[x]->GetLapId()); // get all the channels that this lap has
			for(set<DATA_CHANNEL>::const_iterator i = channels.begin(); i != channels.end(); i++) // loop through them, insert them into our "all data channels" set
			{
				setYSelected.insert(*i);
				GetDataChannelName(*i, szDataChannelName, NUMCHARS(szDataChannelName));
				m_sfYAxis.AddString(szDataChannelName,*i);
				wcscpy(m_PlotPrefs[TotalYChannels].m_ChannelName, szDataChannelName);
				m_PlotPrefs[TotalYChannels].iDataChannel = *i;	//	Add the DATA_CHANNEL enum into the PP array
				TotalYChannels = TotalYChannels + 1;
			}
		}
		for (int i=1; i <= setYSelected.size(); i++)
		{
			m_PlotPrefs[i].iPlotView = true;  //  Default to display as a graph
			m_PlotPrefs[i].fMinValue = -1.0;    //  Set all lower limits to -1.0
			m_PlotPrefs[i].fMaxValue = 1000000.0;  //  Set all upper limits to 1000000.0
		}
		//  Display all of the data channels.
			HWND p_hWnd;
			TCHAR szTemp[512];

			//	Initialize the Plot Prefs dialog box
			for (int z=1; z <= setYSelected.size(); z++)
			{
				wcscpy(szTemp, m_PlotPrefs[z].m_ChannelName);	//	Load the dialog with Channel names
				p_hWnd = GetDlgItem(hWnd, IDC_PLOTTYPE_CHANNEL0 + z);
				SendMessage(p_hWnd, WM_SETTEXT, 0, (LPARAM)szTemp);

				if (m_PlotPrefs[z].iPlotView)	//	Load the dialog with the radio buttons
				{
					CheckRadioButton(hWnd, IDC_PLOTTYPE_GRAPH0 + z, IDC_PLOTTYPE_VALUE0 + z, IDC_PLOTTYPE_GRAPH0 + z);
				}
				else
				{
					CheckRadioButton(hWnd, IDC_PLOTTYPE_GRAPH0 + z, IDC_PLOTTYPE_VALUE0 + z, IDC_PLOTTYPE_VALUE0 + z);
				}

				TCHAR szText[MAX_PATH];	//	Load the dialog with the alarm limits
				swprintf (szText, NUMCHARS(szText), L"%9.2f%", m_PlotPrefs[z].fMinValue);
				SetDlgItemText(hWnd, IDC_PLOTTYPE_LOWLIMIT0 + z, LPCWSTR(&szText));       
				swprintf (szText, NUMCHARS(szText), L"%9.2f%", m_PlotPrefs[z].fMaxValue);
				SetDlgItemText(hWnd, IDC_PLOTTYPE_HIGHLIMIT0 + z, LPCWSTR(&szText));       
			}
		return 0;
    }

      int CPlotSelectDlg::SetPlotPrefs(HWND hWnd, set<DATA_CHANNEL> setAvailable)
	  {
		   //  Display all of the limits for all data channels

			HWND p_hWnd;
			TCHAR szTemp[512];
			for (int z=1; z <= TotalYChannels - 1; z++)
			{
				wcscpy(szTemp, m_PlotPrefs[z].m_ChannelName);	//	Load the dialog with Channel names
				p_hWnd = GetDlgItem(hWnd, IDC_PLOTTYPE_CHANNEL0 + z);
				SendMessage(p_hWnd, WM_SETTEXT, 0, (LPARAM)szTemp);

				TCHAR szText[MAX_PATH];	//	Load the dialog with the alarm limits
				swprintf (szText, NUMCHARS(szText), L"%9.2f%", m_PlotPrefs[z].fMinValue);
				SetDlgItemText(hWnd, IDC_PLOTTYPE_LOWLIMIT0 + z, LPCWSTR(&szText));       
				swprintf (szText, NUMCHARS(szText), L"%9.2f%", m_PlotPrefs[z].fMaxValue);
				SetDlgItemText(hWnd, IDC_PLOTTYPE_HIGHLIMIT0 + z, LPCWSTR(&szText));       

				if (m_PlotPrefs[z].iPlotView)
				{
					CheckRadioButton(hWnd, IDC_PLOTTYPE_GRAPH0 + z, IDC_PLOTTYPE_VALUE0 + z, IDC_PLOTTYPE_GRAPH0 + z);
				}
				else
				{
					CheckRadioButton(hWnd, IDC_PLOTTYPE_GRAPH0 + z, IDC_PLOTTYPE_VALUE0 + z, IDC_PLOTTYPE_VALUE0 + z);
				}
			}
		  return 0;
	  }

    LRESULT CPlotSelectDlg::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
      switch(uMsg)
      {
        case WM_INITDIALOG:
        {
          //  Initialize all data channels, if not already set by user.
          //  Assumes first data channel will always be a graph
          if (m_PlotPrefs[1].iPlotView == false)
          {
            InitPlotPrefs(hWnd, lParam);
          }
          else
          {
            set<DATA_CHANNEL> setAvailable;
            SetPlotPrefs(hWnd, setAvailable);
          }
          return TRUE;
        }

        case WM_COMMAND:
        {
          switch(LOWORD(wParam))
          {
          case IDC_PLOTTYPE_GRAPH1:
          {
            m_PlotPrefs[1].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE1:
          {
            m_PlotPrefs[1].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH2:
          {
            m_PlotPrefs[2].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE2:
          {
            m_PlotPrefs[2].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH3:
          {
            m_PlotPrefs[3].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE3:
          {
            m_PlotPrefs[3].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH4:
          {
            m_PlotPrefs[4].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE4:
          {
            m_PlotPrefs[4].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH5:
          {
            m_PlotPrefs[5].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE5:
          {
            m_PlotPrefs[5].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH6:
          {
            m_PlotPrefs[6].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE6:
          {
            m_PlotPrefs[6].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH7:
          {
            m_PlotPrefs[7].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE7:
          {
            m_PlotPrefs[7].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH8:
          {
            m_PlotPrefs[8].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE8:
          {
            m_PlotPrefs[8].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH9:
          {
            m_PlotPrefs[9].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE9:
          {
            m_PlotPrefs[9].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH10:
          {
            m_PlotPrefs[10].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE10:
          {
            m_PlotPrefs[10].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH11:
          {
            m_PlotPrefs[11].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE11:
          {
            m_PlotPrefs[11].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH12:
          {
            m_PlotPrefs[12].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE12:
          {
            m_PlotPrefs[12].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH13:
          {
            m_PlotPrefs[13].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE13:
          {
            m_PlotPrefs[13].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH14:
          {
            m_PlotPrefs[14].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE14:
          {
            m_PlotPrefs[14].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH15:
          {
            m_PlotPrefs[15].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE15:
          {
            m_PlotPrefs[15].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH16:
          {
            m_PlotPrefs[16].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE16:
          {
            m_PlotPrefs[16].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH17:
          {
            m_PlotPrefs[17].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE17:
          {
            m_PlotPrefs[17].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH18:
          {
            m_PlotPrefs[18].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE18:
          {
            m_PlotPrefs[18].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH19:
          {
            m_PlotPrefs[19].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE19:
          {
            m_PlotPrefs[19].iPlotView = false;
            break;
          }
          case IDC_PLOTTYPE_GRAPH20:
          {
            m_PlotPrefs[20].iPlotView = true;
            break;
          }
          case IDC_PLOTTYPE_VALUE20:
          {
            m_PlotPrefs[20].iPlotView = false;
            break;
          }
		  case IDC_PLOTTYPE_RESCAN:
		  {
			  //  Initialize all data channels, if not already set by user.
			  //  Assumes first data channel (LONG X) will always be a graph
			  InitPlotPrefs(hWnd, lParam);
			  return TRUE;
		  }
		  case IDOK:
          {
            //  Let's get the values for each channel and store it for program execution
            TCHAR szText[MAX_PATH];
            int len;
            float flValue;

			for (int z=1; z <= TotalYChannels -1; z++)
			{
				len = GetWindowTextLength(GetDlgItem(hWnd, IDC_PLOTTYPE_LOWLIMIT0 + z));
				GetDlgItemText(hWnd, IDC_PLOTTYPE_LOWLIMIT0 + z, szText, len+1);
				flValue = _wtof(szText);
				m_PlotPrefs[z].fMinValue = flValue;
				len = GetWindowTextLength(GetDlgItem(hWnd, IDC_PLOTTYPE_HIGHLIMIT0 + z));
				GetDlgItemText(hWnd, IDC_PLOTTYPE_HIGHLIMIT0 + z, szText, len+1);
				flValue = _wtof(szText);
				m_PlotPrefs[z].fMaxValue = flValue;
			}
						
            m_pPlotResults->fCancelled = false;
            EndDialog(hWnd,0);
            return TRUE;
          }
          case IDCANCEL:
                m_pPlotResults->fCancelled = true;
            EndDialog(hWnd,0);
            return TRUE;
          }
          break;
        } // end WM_COMMAND
        case WM_CLOSE:
        {
            m_pPlotResults->fCancelled = true;
          EndDialog(hWnd,0);
          break;
        }
		default:
      return FALSE;
      }
    }
