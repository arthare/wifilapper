#include "Stdafx.h"
#include "DlgSetSplits.h"
#include "resource.h"
#include "pitsideconsole.h"
#include "LapPainter.h"
#include "ArtTools.h" // for FLOATRECT
#include "ArtUI.h" // for ArtOpenGLWindow


LRESULT CSetSplitsDlg::DlgProc
(
  HWND hWnd, 
  UINT uMsg, 
  WPARAM wParam, 
  LPARAM lParam
)
{
  HWND hWnd_OGL = GetDlgItem(hWnd,IDC_LBLSPLITMAP);
  float dBestLength = -1;
  static float dTimeToHighlight = -1;
  static TimePoint2D ptBest;
  static MAPHIGHLIGHT mapPt;
  POINT m_ptMouse = {0};
  bool m_fMouseValid = false;
  static SplitPoints szTempSplit = {0};

  enum SPLITS
  {
	  START,
	  SPLIT1,
	  SPLIT2,
	  SPLIT3,
	  SPLIT4,
	  SPLIT5,
	  SPLIT6,
	  SPLIT7,
	  SPLIT8,
	  FINISH
  };

  if(p_sfRefLapPainter.HandleMessage(hWnd_OGL,uMsg,wParam,lParam))
  {
	return 0;
  }
  
  switch(uMsg)
  {

    case WM_INITDIALOG:
    {
		//	Get the Start time for the lap and store it
		int x = START;
		StartFinish* pSF = (StartFinish*)m_pLap->GetLap()->GetSF();
		const vector<TimePoint2D>& lstPoints = m_pLap->GetPoints();
		{
			const TimePoint2D& p = lstPoints[x];
			m_sfLapOpts->m_SplitPoints[x].m_sfSectorTime = p.iTime;
			m_sfLapOpts->m_SplitPoints[x].m_sfXPoint = p.flX;
			m_sfLapOpts->m_SplitPoints[x].m_sfYPoint = p.flY;
			pSF[x].m_pt1 = V2D(m_sfLapOpts->m_SplitPoints[x].m_sfXPoint,m_sfLapOpts->m_SplitPoints[x].m_sfYPoint);
			pSF[x].m_pt2 = V2D(m_sfLapOpts->m_SplitPoints[x].m_sfXPoint,m_sfLapOpts->m_SplitPoints[x].m_sfYPoint);
		}

		x = FINISH;
		{
			const TimePoint2D& p = lstPoints[lstPoints.size()-1];
			m_sfLapOpts->m_SplitPoints[x].m_sfSectorTime = p.iTime;
			m_sfLapOpts->m_SplitPoints[x].m_sfXPoint = p.flX;
			m_sfLapOpts->m_SplitPoints[x].m_sfYPoint = p.flY;
			pSF[x].m_pt1 = V2D(m_sfLapOpts->m_SplitPoints[x].m_sfXPoint,m_sfLapOpts->m_SplitPoints[x].m_sfYPoint);
			pSF[x].m_pt2 = V2D(m_sfLapOpts->m_SplitPoints[x].m_sfXPoint,m_sfLapOpts->m_SplitPoints[x].m_sfYPoint);
		}

		//	Show the split points
		m_sfLapOpts->fDrawSplitPoints = true;

		//	The the OGL handle
		p_sfRefLapPainter.Init(hWnd_OGL);

		p_sfRefLapPainter.DrawLapLines(*m_sfLapOpts); // draws laps as a map

        return TRUE;
	}
/*    case WM_MOUSEMOVE:
    {
      POINT ptLastMouse = m_ptMouse;
      RECT rcParent,rcThis;
      GetWindowRect(GetParent(hWnd_OGL),&rcParent);
      GetWindowRect(hWnd_OGL,&rcThis);
      m_ptMouse.x = LOWORD(wParam);
      m_ptMouse.y = HIWORD(wParam);

      m_ptMouse.x -= (rcThis.left - rcParent.left);
      m_ptMouse.y -= (rcThis.top - rcParent.top);

      RECT rcClient;
      GetClientRect(hWnd_OGL, &rcClient);

      RECT rcParentClient;
      GetClientRect(GetParent(hWnd_OGL), &rcParentClient);
      // the difference between rcParent and rcParentClient's height will be the height of the title bar
      m_ptMouse.y += (RECT_HEIGHT(&rcParent) - RECT_HEIGHT(&rcParentClient));
      m_ptMouse.y = RECT_HEIGHT(&rcClient) - m_ptMouse.y;
      bool fLastMouseValid = m_fMouseValid;
      m_fMouseValid = m_ptMouse.x >= 0 && m_ptMouse.y >= 0 && m_ptMouse.x < RECT_WIDTH(&rcClient) && m_ptMouse.y < RECT_HEIGHT(&rcClient);
      return FALSE;
    }	*/
	case WM_LBUTTONDOWN:
	{

		int iTime = -1;
		POINT ptMouse;
		Vector2D vHighlight;
		const vector<TimePoint2D>& lstPoints = m_pLap->GetPoints();

		if (p_sfRefLapPainter.GetMouse(&ptMouse))
		{
			// We need to convert from Window space to Map space and find closest point
			iTime = GetLapHighlightTime((const CExtendedLap *)m_pLap);
		}
		if (iTime > 0)
		{
			GLdouble dX=0,dY=0,dZ=0;
			for(int x = 0; x< lstPoints.size(); x++)
			{
				const TimePoint2D& p = lstPoints[x];
				if (p.iTime >= iTime)
				{
					dX = lstPoints[x].flX;
					dY = lstPoints[x].flY;
					ptBest = p;
					vHighlight = V2D((float)dX,(float)dY);
					szTempSplit.m_sfSectorTime = ptBest.iTime;
					szTempSplit.m_sfXPoint = ptBest.flX;
					szTempSplit.m_sfYPoint = ptBest.flY;
					break;
				}
			}	
		}
        return TRUE;
	}
	case WM_NOTIFY:
	{

		p_sfRefLapPainter.DrawLapLines(*m_sfLapOpts); // draws laps as a map

		NMHDR* notifyHeader = (NMHDR*)lParam;
		switch(wParam)
		{
	        case IDC_LAPS:
			{
				return TRUE;
			}
			default:
				return TRUE;
		}
	}
    case WM_COMMAND:
    {
		switch(LOWORD(wParam))
		{
			case IDC_SETSPLIT1:
			{
				//	Get the Finish time for the lap and store it and fill in the S/F vectors for this lap
				GetSplitPoint(SPLIT1, szTempSplit, hWnd);
				  
				m_sfLapOpts->fDrawSplitPoints = true;

				return TRUE;
			}
			case IDC_SETSPLIT2:
			{
				//	Get the Finish time for the lap and store it and fill in the S/F vectors for this lap
				GetSplitPoint(SPLIT2, szTempSplit, hWnd);

				m_sfLapOpts->fDrawSplitPoints = true;

				return TRUE;
			}
			case IDC_SETSPLIT3:
			{
				//	Get the Finish time for the lap and store it and fill in the S/F vectors for this lap
				GetSplitPoint(SPLIT3, szTempSplit, hWnd);

				m_sfLapOpts->fDrawSplitPoints = true;

				return TRUE;
			}
			case IDC_SETSPLIT4:
			{
				//	Get the Finish time for the lap and store it and fill in the S/F vectors for this lap
				GetSplitPoint(SPLIT4, szTempSplit, hWnd);

				m_sfLapOpts->fDrawSplitPoints = true;

				return TRUE;
			}
			case IDC_SETSPLIT5:
			{
				//	Get the Finish time for the lap and store it and fill in the S/F vectors for this lap
				GetSplitPoint(SPLIT5, szTempSplit, hWnd);

				m_sfLapOpts->fDrawSplitPoints = true;

				return TRUE;
			}
			case IDC_SETSPLIT6:
			{
				//	Get the Finish time for the lap and store it and fill in the S/F vectors for this lap
				GetSplitPoint(SPLIT6, szTempSplit, hWnd);

				m_sfLapOpts->fDrawSplitPoints = true;

				return TRUE;
			}
			case IDC_SETSPLIT7:
			{
				//	Get the Finish time for the lap and store it and fill in the S/F vectors for this lap
				GetSplitPoint(SPLIT7, szTempSplit, hWnd);

				m_sfLapOpts->fDrawSplitPoints = true;

				return TRUE;
			}
			case IDC_SETSPLIT8:
			{
				//	Get the Finish time for the lap and store it and fill in the S/F vectors for this lap
				GetSplitPoint(SPLIT8, szTempSplit, hWnd);

				m_sfLapOpts->fDrawSplitPoints = true;

				return TRUE;
			}
			case IDOK:
			{
				ComputeSectors();	//	Load Sector times into sfLapOpts
				m_pResults->fCancelled = false;
				EndDialog(hWnd,0);
				return TRUE;
			}
			case IDRESET:
			{
				for (int x=1; x < 50; x++)
				{
					m_sfLapOpts->m_SplitPoints[x].m_sfXPoint = 0.0f;
					m_sfLapOpts->m_SplitPoints[x].m_sfYPoint = 0.0f;
					m_sfLapOpts->m_SplitPoints[x].m_sfSectorTime = 0;
					m_sfLapOpts->m_SplitPoints[x].m_sfSplitTime = 0.0f;
				}
				StartFinish* pSF = (StartFinish*)m_pLap->GetLap()->GetSF();
				for (int x=0; x < 50; x++)
				{
				  pSF[x].m_pt1 = V2D(m_sfLapOpts->m_SplitPoints[x].m_sfXPoint,m_sfLapOpts->m_SplitPoints[x].m_sfYPoint);
				  pSF[x].m_pt2 = V2D(m_sfLapOpts->m_SplitPoints[x].m_sfXPoint,m_sfLapOpts->m_SplitPoints[x].m_sfYPoint);
				}

				m_sfLapOpts->fDrawSplitPoints = false;
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
  p_sfRefLapPainter.Refresh();	//	Allow the Set Split Point window its own highlighter
  return FALSE;
}

void CSetSplitsDlg::GetSplitPoint(int x, SplitPoints szTempSplit, HWND hWnd)
{
	//	Fill in the S/F vectors for this lap
	StartFinish* pSF = (StartFinish*)m_pLap->GetLap()->GetSF();
	if (szTempSplit.m_sfSectorTime >= m_sfLapOpts->m_SplitPoints[x-1].m_sfSectorTime && m_sfLapOpts->m_SplitPoints[x-1].m_sfSectorTime)
	{
		//	Assign the highlighted time into the latest SF data
		m_sfLapOpts->m_SplitPoints[x] = szTempSplit;
		//	Compute and load the Time Difference for this sector into the SF data
		m_sfLapOpts->m_SplitPoints[x].m_sfSplitTime = szTempSplit.m_sfSplitTime - m_sfLapOpts->m_SplitPoints[x-1].m_sfSplitTime;
		pSF[x].m_pt1 = V2D(m_sfLapOpts->m_SplitPoints[x].m_sfXPoint,m_sfLapOpts->m_SplitPoints[x].m_sfYPoint);
		pSF[x].m_pt2 = V2D(m_sfLapOpts->m_SplitPoints[x].m_sfXPoint,m_sfLapOpts->m_SplitPoints[x].m_sfYPoint);
	}
	else
	{
		MessageBox(hWnd, L"Sector point is earlier than previous.\nPlease Re-pick or re-assign",L"ERROR",MB_OK);
	}
		
}

void CSetSplitsDlg::ComputeSectors()
{
	//	Now, let's compute the Sector times for this lap.
	StartFinish* pSF = (StartFinish*)m_pLap->GetLap()->GetSF();
	for (int z=1; z < 50; z++)
	{
		if (m_sfLapOpts->m_SplitPoints[z].m_sfSectorTime > m_sfLapOpts->m_SplitPoints[z-1].m_sfSectorTime)
		{
			m_sfLapOpts->m_SplitPoints[z].m_sfSplitTime = m_sfLapOpts->m_SplitPoints[z].m_sfSectorTime - m_sfLapOpts->m_SplitPoints[z-1].m_sfSectorTime;		//	Assign the highlighted time into the latest SF data
		}
		else
		{
			m_sfLapOpts->m_SplitPoints[z].m_sfSplitTime = 0.0f;
			m_sfLapOpts->m_SplitPoints[z].m_sfSectorTime = m_sfLapOpts->m_SplitPoints[z-1].m_sfSectorTime;

		}
	}
		
}
