#include "Stdafx.h"
#include "DlgTractionCircle.h"
#include "resource.h"
#include "pitsideconsole.h"
#include "LapPainter.h"
#include "ArtTools.h" // for FLOATRECT
#include "ArtUI.h" // for ArtOpenGLWindow



LRESULT CTractionCircleDlg::DlgProc
(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int width, height;

  HWND hWnd_OGL = GetDlgItem(hWnd,IDC_TRACTIONCIRCLEMAP2);
  float dBestLength = -1;
  static float dTimeToHighlight = -1;
  static TimePoint2D ptBest;
  static MAPHIGHLIGHT mapPt;
  POINT m_ptMouse = {0};
  bool m_fMouseValid = false;

  if(p_sfRefLapPainter.HandleMessage(hWnd_OGL,uMsg,wParam,lParam))
  {
	return 0;
  }
  
  switch(uMsg)
  {
    case WM_INITDIALOG:
	case WM_CREATE: //window being created
    {

//		hDC = GetDC(hWnd);  //get current windows device context
		//	SetupPixelFormat(hDC); //call our pixel format setup function

		//      Create rendering context and make it current
//		hRC = wglCreateContext(hDC);
//		wglMakeCurrent(hDC, hRC);

		//	Display the Traction Circle window
//		m_sfLapOpts->bTractionCircle = true;

		//	Get the OGL handle
		p_sfRefLapPainter.Init(hWnd_OGL);

//		p_sfRefLapPainter.DrawLapLines(*m_sfLapOpts); // draws laps as a map
        m_eXChannel = (DATA_CHANNEL)DATA_CHANNEL_X_ACCEL;
        m_lstYChannels.push_back((DATA_CHANNEL)DATA_CHANNEL_Y_ACCEL);
		p_sfRefLapPainter.DrawTractionCircle((const LAPSUPPLIEROPTIONS) *m_sfLapOpts, true); // draws Traction Circle as a separate map
        return TRUE;

		default:
			break;
        return TRUE;
	}
	case WM_NOTIFY:
	{

		p_sfRefLapPainter.DrawTractionCircle(*m_sfLapOpts, true); // draws laps as a map

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
        case WM_SIZE:

                //      Retrieve width and height
                height = HIWORD(lParam);
                width = LOWORD(lParam);

                //      Don't want a divide by 0
                if (height == 0)
                {
                        height = 1;
                }

                //      Reset the viewport to new dimensions
                glViewport(0, 0, width, height);

                //      Set current Matrix to projection
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity(); //reset projection matrix

                //      Time to calculate aspect ratio of our window.
                
                gluPerspective(54.0f, (GLfloat)width/(GLfloat)height, 1.0f, 1000.0f);

                glMatrixMode(GL_MODELVIEW); //set modelview matrix
                glLoadIdentity(); //reset modelview matrix

                return 0;
                break;

    case WM_COMMAND:
    {
      break;
    } // end WM_COMMAND
    case WM_CLOSE:
    {
		m_pResults->fCancelled = true;
		EndDialog(hWnd,0);
		break;
    }
  }
//  p_sfRefLapPainter.Refresh();	//	Allow the Set Split Point window its own highlighter
  return FALSE;
}
