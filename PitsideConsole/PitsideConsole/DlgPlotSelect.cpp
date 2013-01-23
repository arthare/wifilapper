#include "Stdafx.h"
#include "DlgPlotSelect.h"
#include "resource.h"
#include "PitsideConsole.h"
#include "LapReceiver.h"
#include "LapPainter.h"


LRESULT CPlotSelectDlg::DlgPlot
(
  HWND hWnd, 
  UINT uMsg, 
  WPARAM wParam, 
  LPARAM lParam
)
{
  int iPlotView(50);
  float fMinValue(50);
  float fMaxValue(50);
  bool fCancelled = false;
  TCHAR szTemp[512];

  switch(uMsg)
  {
	case IDC_PLOTTYPE_GRAPH1:
		{
			m_PlotPrefs[0].iPlotView = IDC_PLOTTYPE_GRAPH1;
			m_PlotPrefs[0].fMinValue = IDC_PLOTTYPE_LOWLIMIT1;
			m_PlotPrefs[0].fMaxValue = IDC_PLOTTYPE_HIGHLIMIT1;
//            szTemp = m_PlotPrefs[0].m_ArtListBox.GetSelectedItemsData.m_sfXAxis->lstColumnHeaders[0]->c_str();
          swprintf(szTemp, sizeof(szTemp),L"%s",szTemp);
          SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)szTemp);
		}

      break;

  
    case WM_COMMAND:
    {
      switch(LOWORD(wParam))
      {
        case IDOK:
        {
/*			 case IDC_DISPLAYTYPE_PLOT:
					  {
						switch(HIWORD(wParam))
						{
						case BN_CLICKED:
						  int m_eLapDisplayStyle = LAPDISPLAYSTYLE_PLOT;
						  UpdateUI(UPDATE_MAP | UPDATE_DASHBOARD);
						  break;
						}
						return TRUE;
					  }	*/
	        return TRUE;
        }
        case IDCANCEL:
//          m_pResults.fCancelled = true;
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
	return FALSE;
  }
}
void GetDataChannelName(DATA_CHANNEL eDC, LPTSTR lpszName, int cch, set<DATA_CHANNEL> setAvailable)
	{
	//  Initialize all Plot Settings
	for (int x=0; x<20; x++)
		{
//			const vector<wstring>& lstColumnHeaders
//			m_PlotPrefs[x].m_sfXAxis->Init.lstColumnHeaders = "";
			m_PlotPrefs[x].iPlotView = 0;	//	Default view for plot is as a graph
			m_PlotPrefs[x].fMinValue = NULL;
			m_PlotPrefs[x].fMaxValue = NULL;
		}

		TCHAR szDataChannelName[MAX_PATH];
		for(set<DATA_CHANNEL>::const_iterator i = setAvailable.begin(); i != setAvailable.end(); i++)
			{
				GetDataChannelName(*i, szDataChannelName, NUMCHARS(szDataChannelName));
					
					//			m_sfXAxis.AddString(szDataChannelName,*i);
					//			m_sfYAxis.AddString(szDataChannelName,*i);
				
			}

			/*
				  set<LPARAM> setXSelected,setYSelected;
				  setXSelected.insert(m_eXChannel);
				  m_sfXAxis.SetSelectedData(setXSelected);

				  for(int x = 0; x < m_lstYChannels.size(); x++)
				  {
					setYSelected.insert(m_lstYChannels[x]);
				  }
				  m_sfYAxis.SetSelectedData(setYSelected);
			*/
	}
/*  void InitAxes(set<DATA_CHANNEL> setAvailable)
  { 
    static set<DATA_CHANNEL> setLast;
    if(AreSetsEqual(setLast,setAvailable)) return; // nothing to do

    m_sfXAxis.Clear();
    m_sfYAxis.Clear();
    TCHAR szDataChannelName[MAX_PATH];
    for(set<DATA_CHANNEL>::const_iterator i = setAvailable.begin(); i != setAvailable.end(); i++)
    {
      GetDataChannelName(*i, szDataChannelName, NUMCHARS(szDataChannelName));
      m_sfXAxis.AddString(szDataChannelName,*i);
      m_sfYAxis.AddString(szDataChannelName,*i);
    }
    setLast = setAvailable;
  }	*/
// DlgPlotSelect.cpp : implementation file
//
/*
#include "stdafx.h"
#include "PitsideConsole.h"
#include "DlgPlotSelect.h"


// DlgPlotSelect dialog

IMPLEMENT_DYNAMIC(DlgPlotSelect, CDialogEx)

DlgPlotSelect::DlgPlotSelect(CWnd* pParent =NULL)
	: CDialogEx(DlgPlotSelect::IDD, pParent)
{

}

DlgPlotSelect::~DlgPlotSelect()
{
}

void DlgPlotSelect::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(DlgPlotSelect, CDialogEx)
	ON_STN_CLICKED(IDC_PLOTTYPE_CHANNEL1, &DlgPlotSelect::OnStnClickedPlottypeChannel1)
	ON_BN_CLICKED(IDC_PLOTTYPE_GRAPH1, &DlgPlotSelect::OnBnClickedPlottypeGraph1)
	ON_BN_CLICKED(IDC_PLOTTYPE_VALUE7, &DlgPlotSelect::OnBnClickedPlottypeValue7)
	ON_BN_CLICKED(IDC_PLOTTYPE_VALUE1, &DlgPlotSelect::OnBnClickedPlottypeValue1)
END_MESSAGE_MAP()


// DlgPlotSelect message handlers


void DlgPlotSelect::OnStnClickedPlottypeChannel1()
{
	// TODO: Add your control notification handler code here
}


void DlgPlotSelect::OnBnClickedPlottypeGraph1()
{
	// TODO: Add your control notification handler code here
}


void DlgPlotSelect::OnBnClickedPlottypeValue7()
{
	// TODO: Add your control notification handler code here
}


void DlgPlotSelect::OnBnClickedPlottypeValue1()
{
	// TODO: Add your control notification handler code here
}


      HWND hWndReference = GetDlgItem(m_hWnd, IDC_CURRENTREFERENCE);
      if(m_pReferenceLap)
      {
        TCHAR szRefString[512];
        TCHAR szLapString[512];
        m_pReferenceLap->GetString(szLapString, NUMCHARS(szLapString));
        swprintf(szRefString, NUMCHARS(szRefString), L"Reference Lap: %s", szLapString);
        SendMessage(hWndReference, WM_SETTEXT, 0, (LPARAM)szRefString);
      }
      else
      {
        SendMessage(hWndReference, WM_SETTEXT, 0, (LPARAM)L"No Reference Lap");
      }

      HWND hWndMessageStatus = GetDlgItem(m_hWnd, IDC_MESSAGESTATUS);
      SendMessage(hWndMessageStatus, WM_SETTEXT, 0, (LPARAM)m_szMessageStatus);

      InitAxes(setSelectedChannels);
    }
*/