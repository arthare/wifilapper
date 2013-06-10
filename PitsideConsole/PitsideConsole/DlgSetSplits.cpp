#include "Stdafx.h"
#include "DlgSetSplits.h"
#include "resource.h"
#include "pitsideconsole.h"
#include "LapPainter.h"
#include "ArtTools.h" // for FLOATRECT
#include "ArtUI.h" // for ArtOpenGLWindow

POINT m_ptMouse;
bool m_fMouseValid;
bool GetMouse(POINT* ppt)
  {
    if(m_fMouseValid)
    {
      *ppt = m_ptMouse;
      return true;
    }
    return false;
  }

  HWND hWnd_OGL = NULL;

LRESULT CSetSplitsDlg::DlgProc
(
  HWND hWnd, 
  UINT uMsg, 
  WPARAM wParam, 
  LPARAM lParam
)
{
  float dBestLength = -1;
  float dTimeToHighlight = -1;
  TimePoint2D ptBest;

//  if(m_sfRefLapPainter->HandleMessage(hWnd_OGL,uMsg,wParam,lParam))
  if(m_sfRefLapPainter->HandleMessage(hWnd,uMsg,wParam,lParam))
  {
	return 0;
  }
  
  switch(uMsg)
  {

    case WM_INITDIALOG:
    {
        //  Initialize the send message parameters.
//		TCHAR szTime[100] = L"1";
//		HWND hWndTime = GetDlgItem(hWnd, IDC_EDTTIME);
//		SendMessage(hWndTime, WM_SETTEXT, NUMCHARS(szTime), (LPARAM)szTime);
//		HWND hWndMsg = GetDlgItem(hWnd,IDC_EDTMESSAGE);
//		SendMessage(hWndMsg, WM_GETTEXT, NUMCHARS(m_pResults->szMessage), (LPARAM)m_pResults->szMessage);
//		if(wcslen(m_pResults->szMessage) > 0)

		hWnd_OGL = GetDlgItem(hWnd,IDC_LBLSPLITMAP);
		m_sfRefLapPainter->Init(hWnd_OGL);

		m_sfRefLapPainter->DrawLapLines(*m_sfLapOpts); // draws laps as a map

		m_sfRefLapPainter->Refresh();

//		FLOATRECT rcAllLaps = m_sfSectorDisplay->GetAllLapsBounds();
		RECT rcClient;
		GetClientRect(hWnd_OGL, &rcClient);
		double dClientAspect = ((double)RECT_WIDTH(&rcClient)) / ((double)RECT_HEIGHT(&rcClient));
//		double dMapAspect = abs(RECT_WIDTH(&rcAllLaps)) / abs(RECT_HEIGHT(&rcAllLaps));

////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////

		
		
        return TRUE;
	}
	case WM_LBUTTONDOWN:
	{
		const vector<TimePoint2D>& lstPoints = m_pLap->GetPoints();
		for(int x = 0; x< lstPoints.size(); x++)
		{
			const TimePoint2D& p = lstPoints[x];
			glVertex2f(p.flX,p.flY);

			// for each lap, draw an indicator of the closest thing to the mouse
			
			//	Need ILapSupplier here to do this right.
/*
			if(!m_sfSectorDisplay->IsHighlightSource(m_iSupplierId))
			{
				// if we're not a source, use the given time to highlight
				dTimeToHighlight = m_sfSectorDisplay->GetLapHighlightTime(m_pLap);
			}
			else
			{
				int iTime = m_sfSectorDisplay->GetLapHighlightTime(m_pLap);
				if(abs(p.iTime - iTime) < dBestLength || dBestLength < 0)
				{
					dBestLength = abs(p.iTime - iTime);
					ptBest = p;
					dTimeToHighlight = iTime;
				}
			} */
		}	
        return TRUE;
	}
	case WM_LBUTTONDBLCLK:
	{
/////////////////////////////////////////////////////////////////////

		// project from unit-space (the map) to window-space so we know where we need to draw our highlight-box
		{
			GLdouble winx=100,winy=100,winz=0;
//				  gluProject(ptBest.flX, ptBest.flY, 0, rgModelviewMatrix, rgProjMatrix, rgViewport, &winx, &winy, &winz);
			MAPHIGHLIGHT mapPt;
			mapPt.pt.x = (int)winx;
			mapPt.pt.y = (int)winy;
			mapPt.pLap = m_pLap;

//				  lstMousePointsToDraw.push_back(mapPt);
		}
		glEnd();	

//////////////////////////////////////////////////////////////////////////
        return TRUE;
	}
	case WM_NOTIFY:
	{

		m_sfRefLapPainter->DrawLapLines(*m_sfLapOpts); // draws laps as a map

		NMHDR* notifyHeader = (NMHDR*)lParam;
		switch(wParam)
		{
		}
	}
    case WM_COMMAND:
    {
		switch(LOWORD(wParam))
		{
			case IDC_SETSPLIT1:
			{
				glVertex2f(m_ptMouse.x,m_ptMouse.y);
				return TRUE;
			}
			case IDC_SETSPLIT2:
			{
				glVertex2f(m_ptMouse.x,m_ptMouse.y);
				return TRUE;
			}
			case IDC_SETSPLIT3:
			{
				glVertex2f(m_ptMouse.x,m_ptMouse.y);
				return TRUE;
			}
			case IDOK:
			{
				m_sfLapOpts->fDrawSplitPoints = true;
				m_pResults->fCancelled = false;
				EndDialog(hWnd,0);
				return TRUE;
			}
			case IDRESET:
			{
				m_sfLapOpts->fDrawSplitPoints = false;
				m_pResults->fCancelled = true;
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
