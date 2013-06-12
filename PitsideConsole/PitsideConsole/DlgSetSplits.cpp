#include "Stdafx.h"
#include "DlgSetSplits.h"
#include "resource.h"
#include "pitsideconsole.h"
#include "LapPainter.h"
#include "ArtTools.h" // for FLOATRECT
#include "ArtUI.h" // for ArtOpenGLWindow

//  HWND hWnd_OGL = NULL;

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

  GLdouble rgModelviewMatrix[16];
  GLdouble rgProjMatrix[16];
  GLint rgViewport[4];

  if(m_sfRefLapPainter->HandleMessage(hWnd_OGL,uMsg,wParam,lParam))
//  if(m_sfRefLapPainter->HandleMessage(hWnd,uMsg,wParam,lParam))
  {
	return 0;
  }
  glGetDoublev(GL_MODELVIEW_MATRIX, rgModelviewMatrix);
  glGetDoublev(GL_PROJECTION_MATRIX, rgProjMatrix);
  glGetIntegerv(GL_VIEWPORT, rgViewport);
  
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

		m_sfRefLapPainter->Init(hWnd_OGL);

		m_sfRefLapPainter->DrawLapLines(*m_sfLapOpts); // draws laps as a map


////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////

		
		
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

		int iTime;
		POINT ptMouse;
		Vector2D vHighlight;
		const vector<TimePoint2D>& lstPoints = m_pLap->GetPoints();

		if (m_sfRefLapPainter->GetMouse(&ptMouse))
		{
			// We need to convert from Mouse space to Map space and find closest point
			iTime = GetLapHighlightTime((const CExtendedLap *)m_pLap);
		}
		GLdouble dX,dY,dZ;

		gluUnProject(ptMouse.x,ptMouse.y,0,rgModelviewMatrix,rgProjMatrix,rgViewport,&dX,&dY,&dZ);
		vHighlight = V2D((float)dX,(float)dY);
		for(int x = 0; x< lstPoints.size()/3; x++)
		{
			SplitPoints sz_SplitTemp;
			const TimePoint2D& p = lstPoints[x];
			if (p.iTime == iTime)
			{
				ptBest = p;
				sz_SplitTemp.m_sfSectorTime = ptBest.iTime;
				sz_SplitTemp.m_sfXPoint = ptBest.flX;
				sz_SplitTemp.m_sfYPoint = ptBest.flY;
				m_sfLapOpts->m_SplitPoints[49] = sz_SplitTemp;
			}
			else
			{
				ptBest = p;
				sz_SplitTemp.m_sfSectorTime = ptBest.iTime;
				sz_SplitTemp.m_sfXPoint = ptBest.flX;
				sz_SplitTemp.m_sfYPoint = ptBest.flY;
				m_sfLapOpts->m_SplitPoints[49] = sz_SplitTemp;
			}
		}	
        return TRUE;
	}
	case WM_LBUTTONDBLCLK:
	{
/////////////////////////////////////////////////////////////////////
		int iTime;
		POINT ptMouse;
		Vector2D vHighlight;
		const vector<TimePoint2D>& lstPoints = m_pLap->GetPoints();

		if (m_sfRefLapPainter->GetMouse(&ptMouse))
		{
			// We need to convert from Mouse space to Map space and find closest point
			iTime = GetLapHighlightTime((const CExtendedLap *)m_pLap);
		}
		GLdouble dX,dY,dZ;

		gluUnProject(ptMouse.x,ptMouse.y,0,rgModelviewMatrix,rgProjMatrix,rgViewport,&dX,&dY,&dZ);
		vHighlight = V2D((float)dX,(float)dY);
		for(int x = 0; x< lstPoints.size()*2/3; x++)
		{
			SplitPoints sz_SplitTemp;
			const TimePoint2D& p = lstPoints[x];
			if (p.iTime == iTime)
			{
				ptBest = p;
				sz_SplitTemp.m_sfSectorTime = ptBest.iTime;
				sz_SplitTemp.m_sfXPoint = ptBest.flX;
				sz_SplitTemp.m_sfYPoint = ptBest.flY;
				m_sfLapOpts->m_SplitPoints[49] = sz_SplitTemp;
			}
			else
			{
				ptBest = p;
				sz_SplitTemp.m_sfSectorTime = ptBest.iTime;
				sz_SplitTemp.m_sfXPoint = ptBest.flX;
				sz_SplitTemp.m_sfYPoint = ptBest.flY;
				m_sfLapOpts->m_SplitPoints[49] = sz_SplitTemp;
			}
		}	
//////////////////////////////////////////////////////////////////////////
        return TRUE;
	}
	case WM_NOTIFY:
	{

		m_sfRefLapPainter->DrawLapLines(*m_sfLapOpts); // draws laps as a map

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
			case IDC_SETSPLIT2:
			{
				const vector<TimePoint2D>& lstPoints = m_pLap->GetPoints();
				StartFinish* pSF = (StartFinish*)m_pLap->GetLap()->GetSF();
				//	Fill in the S/F vectors for this lap
				{
				  int x = 1;
				  int iTime = m_sfLapOpts->m_SplitPoints[x].m_sfSectorTime;
				  m_sfLapOpts->m_SplitPoints[x] = m_sfLapOpts->m_SplitPoints[49];
/*				  const TimePoint2D& p = lstPoints[iTime];
				  const TimePoint2D& q = lstPoints[iTime+1];
				  Vector2D v_Vector, v_Ortho;
				  v_Vector.m_v[0] = q.flX-p.flX, q.flY-p.flY;
				  pSF[x].m_pt1 = V2D(p.flX,p.flX);
				  pSF[x].m_pt2 = V2D(q.flX,q.flX);
				  v_Ortho = FLIP(pSF[x].m_pt1);		*/
				  pSF[x].m_pt1 = V2D(m_sfLapOpts->m_SplitPoints[x].m_sfXPoint,m_sfLapOpts->m_SplitPoints[x].m_sfYPoint);
				  pSF[x].m_pt2 = V2D(m_sfLapOpts->m_SplitPoints[x].m_sfXPoint,m_sfLapOpts->m_SplitPoints[x].m_sfYPoint);
				}
				m_sfLapOpts->fDrawSplitPoints = true;

				return TRUE;
			}
			case IDC_SETSPLIT1:
			{
				int x=0;
				StartFinish* pSF = (StartFinish*)m_pLap->GetLap()->GetSF();
				const vector<TimePoint2D>& lstPoints = m_pLap->GetPoints();
				const TimePoint2D& p = lstPoints[x];
				m_sfLapOpts->m_SplitPoints[x].m_sfSectorTime = p.iTime;
				m_sfLapOpts->m_SplitPoints[x].m_sfXPoint = p.flX;
				m_sfLapOpts->m_SplitPoints[x].m_sfYPoint = p.flY;
				pSF[x].m_pt1 = V2D(m_sfLapOpts->m_SplitPoints[x].m_sfXPoint,m_sfLapOpts->m_SplitPoints[x].m_sfYPoint);
				pSF[x].m_pt2 = V2D(m_sfLapOpts->m_SplitPoints[x].m_sfXPoint,m_sfLapOpts->m_SplitPoints[x].m_sfYPoint);
				m_sfLapOpts->fDrawSplitPoints = true;

				return TRUE;
			}
			case IDC_SETSPLIT3:
			{
				const vector<TimePoint2D>& lstPoints = m_pLap->GetPoints();
				StartFinish* pSF = (StartFinish*)m_pLap->GetLap()->GetSF();
				//	Fill in the S/F vectors for this lap
				{
				  int x = 2;
				  int iTime = m_sfLapOpts->m_SplitPoints[x].m_sfSectorTime;
				  m_sfLapOpts->m_SplitPoints[x] = m_sfLapOpts->m_SplitPoints[49];
/*				  const TimePoint2D& p = lstPoints[iTime];
				  const TimePoint2D& q = lstPoints[iTime+1];
				  Vector2D v_Vector, v_Ortho;
				  v_Vector.m_v[0] = q.flX-p.flX, q.flY-p.flY;
				  pSF[x].m_pt1 = V2D(p.flX,p.flX);
				  pSF[x].m_pt2 = V2D(q.flX,q.flX);
				  v_Ortho = FLIP(pSF[x].m_pt1);		*/
				  pSF[x].m_pt1 = V2D(m_sfLapOpts->m_SplitPoints[x].m_sfXPoint,m_sfLapOpts->m_SplitPoints[x].m_sfYPoint);
				  pSF[x].m_pt2 = V2D(m_sfLapOpts->m_SplitPoints[x].m_sfXPoint,m_sfLapOpts->m_SplitPoints[x].m_sfYPoint);
				}
				m_sfLapOpts->fDrawSplitPoints = true;

				return TRUE;
			}
			case IDOK:
			{

/*				const vector<TimePoint2D>& lstPoints = m_pLap->GetPoints();
				StartFinish* pSF = (StartFinish*)m_pLap->GetLap()->GetSF();
				//	Fill in the S/F vectors for this lap
				for(int x = 0;x < 3; x++)
				{
				  const TimePoint2D& p = lstPoints[x*20];
				  const TimePoint2D& q = lstPoints[x*lstPoints.size()/3];
				  pSF[x].m_pt1.m_v[0] = p.flX;
				  pSF[x].m_pt1.m_v[1] = p.flY;
				  pSF[x].m_pt2.m_v[0] = q.flX;
				  pSF[x].m_pt2.m_v[1] = q.flY;
				}
*/
				m_pResults->fCancelled = false;
				EndDialog(hWnd,0);
				return TRUE;
			}
			case IDRESET:
			{
				m_sfLapOpts->fDrawSplitPoints = false;
//				m_pResults->fCancelled = false;
//				EndDialog(hWnd,0);
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
  m_sfRefLapPainter->Refresh();	//	Allow the Set Split Point window its own highlighter
  return FALSE;
}
