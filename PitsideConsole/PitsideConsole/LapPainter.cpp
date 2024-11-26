#include "Stdafx.h"
#include "resource.h"
#include "LapPainter.h"
#include "LapData.h"
#include "ArtUI.h"
#include "ArtTools.h"
#include <math.h>

struct HIGHLIGHTDATA
{
public:
  const IDataChannel* m_pDataX;
  const IDataChannel* m_pDataY;
  POINT m_ptWindow;
  const CExtendedLap* m_pLap;
  DATA_CHANNEL m_eChannelY;
};

CLapPainter::CLapPainter(/*IUI* pUI, */ILapSupplier* pLapSupplier, int iSupplierId) : ArtOpenGLWindow(), /*m_pUI(pUI),*/ m_pLapSupplier(pLapSupplier), m_iSupplierId(iSupplierId)
{
}
CLapPainter::~CLapPainter()
{
}


void CLapPainter::OGL_Paint()
{
  if (m_pLapSupplier->GetDisplayOptions().fColorScheme)
  {
		glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );  //  Background color is black.
  }
  else
  {
		glClearColor( 0.95f, 0.95f, 0.95f, 0.95f );  //  Background color is grey.
  }
  glClear( GL_COLOR_BUFFER_BIT );
  
  RECT rcClient;
  GetClientRect(OGL_GetHWnd(), &rcClient);
  
  LAPDISPLAYSTYLE eDisplayStyle = m_pLapSupplier->GetLapDisplayStyle(m_iSupplierId);
  const LAPSUPPLIEROPTIONS& sfLapOpts = m_pLapSupplier->GetDisplayOptions();
//  vector<CExtendedLap*> lstLaps = GetLapsToShow();
  const CExtendedLap* pLap = NULL;
  switch(eDisplayStyle)
  {
	  case LAPDISPLAYSTYLE_MAP:
		glViewport(0,0,RECT_WIDTH(&rcClient), RECT_HEIGHT(&rcClient));
		DrawLapLines(sfLapOpts);	//	Draws the lap as a map on primary display
		break;
	  case LAPDISPLAYSTYLE_TRACTIONCIRCLE:
		glViewport(0,0,RECT_WIDTH(&rcClient), RECT_HEIGHT(&rcClient));
		DrawTractionCircle(sfLapOpts, true);	//	Draw the traction circle graph if the window is active
		break;
	  case LAPDISPLAYSTYLE_PLOT:
		DrawGeneralGraph(sfLapOpts, true);	//	Draws the data graphs on the primary display
		break;
	  case LAPDISPLAYSTYLE_RECEPTION:
		DrawReceptionMap(sfLapOpts);	//	Draws the reception map on the primary display
		break;
	  case LAPDISPLAYSTYLE_NOLAPS:
		// user doesn't have any laps selected, so we should tell them to select some
		DrawSelectLapsPrompt();
		break;
  }
  SwapBuffers( OGL_GetDC() );
}
  
void CLapPainter::DrawReceptionMap(const LAPSUPPLIEROPTIONS& sfLapOpts) const
{
  // we need to get all the DATA_CHANNEL_RECEPTION channels for each of the selected laps, then draw them
  FLOATRECT rcAllLaps = m_pLapSupplier->GetAllLapsBounds();

  RECT rcClient;
  GetClientRect(OGL_GetHWnd(), &rcClient);
  double dClientAspect = ((double)RECT_WIDTH(&rcClient)) / ((double)RECT_HEIGHT(&rcClient));
  double dMapAspect = abs(RECT_WIDTH(&rcAllLaps)) / abs(RECT_HEIGHT(&rcAllLaps));

  if(dClientAspect > dMapAspect)
  {
    // the window is wider than the map is.  we'll want to widen the map space we show appropriately
    const double dCentreX = (rcAllLaps.left + rcAllLaps.right)/2;
    rcAllLaps.left -= dCentreX;
    rcAllLaps.right -= dCentreX;
    rcAllLaps.left *= (dClientAspect / dMapAspect);
    rcAllLaps.right *= (dClientAspect / dMapAspect);
    rcAllLaps.left += dCentreX;
    rcAllLaps.right += dCentreX;
  }
  else
  {
    // the window is taller than the map points are.  we'll want to tallen map bounds
    const double dCentreY = (rcAllLaps.top + rcAllLaps.bottom)/2;
    rcAllLaps.top -= dCentreY;
    rcAllLaps.bottom -= dCentreY;
    rcAllLaps.top *= (dMapAspect / dClientAspect);
    rcAllLaps.bottom *= (dMapAspect / dClientAspect);
    rcAllLaps.top += dCentreY;
    rcAllLaps.bottom += dCentreY;

  }
  // we have now determined the bounds of the thing we're going to draw
  glPushMatrix();
  glLoadIdentity();
  glScalef(1.0f, 0.90f, 1.0f);	//	Keep the same scaling - KDJ
  glOrtho(rcAllLaps.right, rcAllLaps.left, rcAllLaps.top, rcAllLaps.bottom,-1.0,1.0);
  
  GLdouble rgModelviewMatrix[16];
  GLdouble rgProjMatrix[16];
  GLint rgViewport[4];

  glGetDoublev(GL_MODELVIEW_MATRIX, rgModelviewMatrix);
  glGetDoublev(GL_PROJECTION_MATRIX, rgProjMatrix);
  glGetIntegerv(GL_VIEWPORT, rgViewport);

  
  vector<CExtendedLap*> lstLaps = m_pLapSupplier->GetLapsToShow();
  for(int x = 0; x < lstLaps.size(); x++)
  {
    CExtendedLap* pLap = lstLaps[x];
    srand((int)pLap);

    const IDataChannel* pReceptionX = pLap->GetChannel(DATA_CHANNEL_RECEPTION_X);
    const IDataChannel* pReceptionY = pLap->GetChannel(DATA_CHANNEL_RECEPTION_Y);
    if(pReceptionX && pReceptionY)
    {
      glPointSize(4.0f);
      glBegin(GL_POINTS);

      const vector<DataPoint>& lstPointsX = pReceptionX->GetData();
      const vector<DataPoint>& lstPointsY = pReceptionY->GetData();
      if(lstPointsX.size() == lstPointsY.size())
      {
        for(int ix = 0; ix < lstPointsX.size(); ix++)
        {
          glVertex2f(lstPointsX[ix].flValue,lstPointsY[ix].flValue);
        }
      }
		  glEnd();
    }

  }



	glPopMatrix(); // popping us out of map-coords space.

}

void CLapPainter::DrawSelectLapsPromptShort() const
{
  RECT rcClient;
  GetClientRect(OGL_GetHWnd(),&rcClient);
  if (m_pLapSupplier->GetDisplayOptions().fColorScheme)	//	Background color is black, make sure there is enough contrast with the lines
  {
	glColor3d( 1.0, 1.0, 1.0 ); // Final color to use.  Tells opengl to draw the following in the colour we just made up
  }
  else
  {
	glColor3d( 0.0, 0.0, 0.0 ); // Final color to use.  Tells opengl to draw the following in the colour we just made up
  }
  glPushMatrix();
  glLoadIdentity();
  glScalef(1.0f, 0.90f, 1.0f);	//	Keep the same scaling - KDJ
  glOrtho(0, RECT_WIDTH(&rcClient),0, RECT_HEIGHT(&rcClient),-1.0,1.0);

  DrawText(0.0, 80, "Click ");
  DrawText(0.0, 50, "X/Y/Z-Accel");
  DrawText(0.0, 20, "to display");

  glPopMatrix();
}

void CLapPainter::DrawSelectLapsPrompt() const
{
  RECT rcClient;
  GetClientRect(OGL_GetHWnd(),&rcClient);

  glPushMatrix();
  glLoadIdentity();
  glScalef(1.0f, 0.90f, 1.0f);	//	Keep the same scaling - KDJ
  glOrtho(0, RECT_WIDTH(&rcClient),0, RECT_HEIGHT(&rcClient),-1.0,1.0);

  DrawText(20.0, 20, "No laps selected.  Select some laps in the lap list");

  glPopMatrix();
}

void UpdateHighlightPointList(vector<HIGHLIGHTDATA>& lst, const CExtendedLap* pLap, GLdouble* rgModelviewMatrix, GLdouble* rgProjMatrix, GLint* rgViewport, float dTimeToHighlight, const IDataChannel* pDataX, const IDataChannel* pDataY)
{
  const double dPointX = pDataX->GetValue(dTimeToHighlight);
  const double dPointY = pDataY->GetValue(dTimeToHighlight);

  GLdouble winx,winy,winz;
  gluProject(dPointX, dPointY, 0, rgModelviewMatrix, rgProjMatrix, rgViewport, &winx, &winy, &winz);
  POINT pt;
  pt.x = (int)winx;
  pt.y = (int)winy;

  HIGHLIGHTDATA aHighlight;
  aHighlight.m_ptWindow = pt;
  aHighlight.m_pDataX = pDataX;
  aHighlight.m_pDataY = pDataY;
  aHighlight.m_pLap = pLap;
  aHighlight.m_eChannelY = pDataY->GetChannelType();
  lst.push_back(aHighlight);
}

void CLapPainter::DrawGeneralGraph(const LAPSUPPLIEROPTIONS& sfLapOpts, bool fHighlightXAxis)
{
  vector<CExtendedLap*> lstLaps = m_pLapSupplier->GetLapsToShow();

  DATA_CHANNEL eX;
  eX = DATA_CHANNEL_DISTANCE;

  set<DATA_CHANNEL> setY;
  map<DATA_CHANNEL,float> mapMinY, mapMinYTemp;
  map<DATA_CHANNEL,float> mapMaxY, mapMaxYTemp;
  float dMaxX = -1e30;
  float dMinX = 1e30;
  float dCenterOvalX = 0;
  float dCenterOvalY = 0;
  const int cSectors = 9;	//	The maximum number of Sectors to display, gated by display area
/////////////////////////////////////////
  bool b_TransformY = false;	//	Flag if we should transform this channel using PlotPrefs polynomial function false = no, true = yes
  int i_TransInt = 0;		//	PlotPrefs index for transformation function coefficients
/////////////////////////////////////////
  // figuring out bounds and getting matrices all set up
  //	First lets load up all of the data into an array and determine its size
  for (int x = 0; x < lstLaps.size(); x++)
  {
      CExtendedLap* pLap = lstLaps[x];
      DATA_CHANNEL eDataX = m_pLapSupplier->GetXChannel();
      const IDataChannel* pDataX = pLap->GetChannel(eDataX);
      if(!pDataX || !pDataX->IsValid() || pDataX->GetData().size() <= 0) continue;

      vector<DATA_CHANNEL> lstDataY = m_pLapSupplier->GetYChannels();
      for (int y = 0; y < lstDataY.size(); y++)	//	Loop through the data channels and load their data
      {
        bool ValueDisplay = false;	//	Flag if we should display this channel as a graph or not false = yes, true = no
//////////////////////////////////
		b_TransformY = false;
		i_TransInt = 0;
//////////////////////////////////
		const IDataChannel* pChannel = pLap->GetChannel(lstDataY[y]);
        if (!pChannel || !pChannel->IsValid()) continue;

        const DATA_CHANNEL eType = lstDataY[y];

		//	Determine if this Data Channel is one that we only want to display the values for
			for (int u = 0; u < sizeof lstDataY; u++)
			{
				if (eType == m_pLapSupplier->GetDisplayOptions().m_PlotPrefs[u].iDataChannel)
				{
					//	We have found the appropriate DATA_CHANNEL, let's do some tests on it
////////////////////////////////////////
					//	Let's check here if the Y values need to be transformed or not
					if (m_pLapSupplier->GetDisplayOptions().m_PlotPrefs[u].iTransformYesNo == true)
					{
						//	Let's set the flags to transform the Y values for this data channel
						b_TransformY = true;
						i_TransInt = u;
					}
					else
					{
						b_TransformY = false;
						i_TransInt = 0;
					}
////////////////////////////////////////
					if (eType == m_pLapSupplier->GetDisplayOptions().m_PlotPrefs[u].iDataChannel && m_pLapSupplier->GetDisplayOptions().m_PlotPrefs[u].iPlotView == false)
					{	//	We have found a display only channel. Let's prevent the graph from displaying
						ValueDisplay = true;
					}
					break;
				}
			}
        if(mapMinYTemp.find(eType) == mapMinYTemp.end())
        {
          mapMinYTemp[eType] = min(pChannel->GetMin(),m_pLapSupplier->GetDataHardcodedMin(eType));
          mapMaxYTemp[eType] = max(pChannel->GetMax(),m_pLapSupplier->GetDataHardcodedMax(eType));
        }
        else
		{
          mapMinYTemp[eType] = min(pChannel->GetMin(),mapMinYTemp[eType]);	//	Make the minimum the least of the min of this lap, or the previous ones
          mapMaxYTemp[eType] = max(pChannel->GetMax(),mapMaxYTemp[eType]);	//	Make the maximum the greater of the max of this lap, or the previous ones
		}
////////////////////////////////
		//	Adding transformation functions here for Min/MaxY
		if (b_TransformY == true && (float)PolynomialFilter(mapMinYTemp[eType], sfLapOpts.m_PlotPrefs[i_TransInt].fTransAValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransBValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransCValue) > (float)PolynomialFilter(mapMaxYTemp[eType], sfLapOpts.m_PlotPrefs[i_TransInt].fTransAValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransBValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransCValue))		
		{
			mapMinY[eType] = (float)PolynomialFilter(mapMaxYTemp[eType], sfLapOpts.m_PlotPrefs[i_TransInt].fTransAValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransBValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransCValue);
			mapMaxY[eType] = (float)PolynomialFilter(mapMinYTemp[eType], sfLapOpts.m_PlotPrefs[i_TransInt].fTransAValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransBValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransCValue);
		}
		else if (b_TransformY == true)
		{
			mapMinY[eType] = (float)PolynomialFilter(mapMinYTemp[eType], sfLapOpts.m_PlotPrefs[i_TransInt].fTransAValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransBValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransCValue);
			mapMaxY[eType] = (float)PolynomialFilter(mapMaxYTemp[eType], sfLapOpts.m_PlotPrefs[i_TransInt].fTransAValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransBValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransCValue);
		}
		else
		{
			mapMinY[eType] = mapMinYTemp[eType];
			mapMaxY[eType] = mapMaxYTemp[eType];
		}

////////////////////////////////
        if (ValueDisplay == false)
	    {
		  setY.insert(eType);
	    }
      }
      if(pDataX)
      {
        dMaxX = max(dMaxX, pDataX->GetMax());
        dMinX = min(dMinX, pDataX->GetMin());

        eX = pDataX->GetChannelType();
      }
	  b_TransformY = false;	//	Reset the flag for transforming the Data Channel
  }	//	End of finding bounds and loading laps loop
	 
  if(setY.size() <= 0)
  {
    DrawSelectLapsPrompt();
    return;
  }
  
  RECT rcSpot;
  int iSegmentHeight=0;
  {
    RECT rcClient;
    GetClientRect(OGL_GetHWnd(), &rcClient);
  
    iSegmentHeight = RECT_HEIGHT(&rcClient) / setY.size();
    rcSpot.left = 0;
    rcSpot.top = 0;
    rcSpot.right = RECT_WIDTH(&rcClient);
    rcSpot.bottom = iSegmentHeight;
  }
  int iPos = 0;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // y-channel graphing loop start
  for(set<DATA_CHANNEL>::iterator i = setY.begin(); i != setY.end(); i++)
  {
    vector<HIGHLIGHTDATA> lstMousePointsToDraw;
    glViewport(rcSpot.left,rcSpot.top,RECT_WIDTH(&rcSpot),RECT_HEIGHT(&rcSpot));
	int y = 0;	//	Tracker for PlotPrefs array variable
	for (int z = 0; z < 49; z++)
	{
		if (sfLapOpts.m_PlotPrefs[z].iDataChannel == *i)
		{
			y = z;
			break;
		}
	}
	// now we have the bounds of all the laps we've looked at, so let's draw them
    glPushMatrix();
    glLoadIdentity();
    glScalef(1.0f, 0.90f, 1.0f);	// Let's scale it so that graphs don't touch each other.
    glOrtho(dMinX, dMaxX, mapMinY[*i], mapMaxY[*i], -1.0, 1.0);

    // draw horizontal guide lines and text on the background.
	for(float flLine = m_pLapSupplier->GetGuideStart(*i, mapMinY[*i], mapMaxY[*i]); flLine < mapMaxY[*i]; flLine += m_pLapSupplier->GetGuideStep(*i, mapMinY[*i], mapMaxY[*i]))
    {
      char szText[256];
      GetChannelString(*i, sfLapOpts.eUnitPreference, flLine, szText, NUMCHARS(szText));
	  LineColor();	//	Pick guideline color, based upon chosen color scheme
	  DrawHorizontalLine(flLine, dMinX, dMaxX, szText);
    }
	//	End of Horizontal line drawing routine

	//	Set up the non-zoomed/panned view for the map
    GLdouble rgModelviewMatrix[16];
    GLdouble rgProjMatrix[16];
    GLint rgViewport[4];
    
	{
		//	Now that the matrices are correct, let's graph them.    
		glGetDoublev(GL_MODELVIEW_MATRIX, rgModelviewMatrix);
		glGetDoublev(GL_PROJECTION_MATRIX, rgProjMatrix);
		glGetIntegerv(GL_VIEWPORT, rgViewport);
		static int OldiZoomLevels;
		POINT ptMouse;
		double dScaleAmt = pow(1.1, sfLapOpts.iZoomLevels);
		static GLdouble dXShift, dYShift, dZShift;
		if(GetMouse(&ptMouse) && m_pLapSupplier->IsHighlightSource(m_iSupplierId))
		{
			//		The mouse is in our window, so panning and zooming are active!
			//		Project the window shift stuff so we know how far to translate the view
			dYShift = 0;	// No Y shift or zoom for Map Plot.
			gluUnProject(sfLapOpts.flWindowShiftX / dScaleAmt, 0, 0, rgModelviewMatrix, rgProjMatrix, rgViewport, &dXShift, &dYShift, &dZShift);
			//		Set up to perform the ZOOM function for DATA PLOT.   
			static double dTranslateShiftX;
			static GLdouble dX,dY,dZ;
			CExtendedLap* pLap;
			if (lstLaps.size() > 0) 
			{
				pLap = lstLaps[lstLaps.size() - 1];	//	Get the Reference Lap for scaling
			}
			else
			{
				return;
			}
			const IDataChannel* pDataX = pLap->GetChannel(m_pLapSupplier->GetXChannel());	//	Get the list of X-Axis points for the Reference Lap
			const vector<DataPoint> &lstPointsX = pDataX->GetData();
			float dBestLength = -1;
			DataPoint ptBest;					//	Best highlighted point where the mouse is on the Reference Lap
			for(int x = 0; x< lstPointsX.size(); x++)
			{
				const DataPoint& p = lstPointsX[x];
				int iTime = m_pLapSupplier->GetLapHighlightTime(pLap);
				if(abs(p.iTimeMs - iTime) < dBestLength || dBestLength < 0)
				{
				  dBestLength = abs(p.iTimeMs - iTime);
				  ptBest = p;					//	ptBest contains the X/Y values for the highlighted point of the Reference Lap
				}
			}

			if(dScaleAmt > 1)
			{
				dTranslateShiftX = ptBest.flValue - dXShift;	//	Calculate the shift amount for the highlighted point in the Reference Lap if we are zoomed in
			}
			OldiZoomLevels = sfLapOpts.iZoomLevels;			//	Save the zoom level
				
			glTranslated(dTranslateShiftX, 0, 0);	// Translate the map to origin on x-axis only
			glScaled(dScaleAmt, 1.0, 1.0);	//	No scaling of Y-axis on Data Plot, only scale the X-axis.
			glTranslated(-dTranslateShiftX, 0, 0);	// Now put the map back in its place
			//	Panning functionality
			glTranslated(dXShift - dMinX, 0, 0);	//	Shift the graph based upon the mouse movement

			//		Now having shifted, let's get our new model matrices
			glGetDoublev(GL_MODELVIEW_MATRIX, rgModelviewMatrix);
			glGetDoublev(GL_PROJECTION_MATRIX, rgProjMatrix);
			glGetIntegerv(GL_VIEWPORT, rgViewport);
		}

		//	Draw Vertical guidelines for the graph
		if (sfLapOpts.fDrawSplitPoints && eX == DATA_CHANNEL_DISTANCE)
		{
			//	Draw Split Points if they are selected
			for(int z = 0; z < cSectors; z++)
			{
				if (sfLapOpts.m_SplitPoints[z].m_sfXPoint != 0.0f)
				{
					CExtendedLap* pLap;
					if (lstLaps.size() > 0) 
					{
						pLap = lstLaps[lstLaps.size() - 1];	//	Get the Reference Lap for scaling
					}
					else
					{
						return;
					}
					const IDataChannel* pDistance = pLap->GetChannel(DATA_CHANNEL_DISTANCE);
					const double dDistance = pDistance->GetValue(sfLapOpts.m_SplitPoints[z].m_sfSectorTime) - pDistance->GetValue(sfLapOpts.m_SplitPoints[0].m_sfSectorTime);
					double flLine = dDistance;
					char szText[256] = {NULL};
					sprintf(szText, "S%i",z);
					glColor3d(1.0,0.0,0.0);	//	Split Point guides are in red
					DrawVerticalLine(flLine, mapMinY[*i], mapMaxY[*i], szText);
				}
			}
		}
		//		If this is for drawing the Traction Circle, let's draw a bunch of circles (Ovals really)
		else if (eX == DATA_CHANNEL_X_ACCEL || eX == DATA_CHANNEL_Y_ACCEL || eX == DATA_CHANNEL_Z_ACCEL)
		{
			drawOval (dCenterOvalX, dCenterOvalY, 3.0f, 3.0f);	//	Draw 1.5G circle
			drawOval (dCenterOvalX, dCenterOvalY, 2.0f, 2.0f);	//	Draw 1.0G circle
			drawOval (dCenterOvalX, dCenterOvalY, 1.0f, 1.0f);	//	Draw 0.5G circle
			//	Now let's draw a vertical line at the origin
			DrawVerticalLine(0.0f, mapMinY[*i], mapMaxY[*i], NULL);
		}

		//	Default is to draw the standard distance markers
		else
		{
			for(float flLine = m_pLapSupplier->GetGuideStartX(eX, dMinX, dMaxX); flLine < dMaxX; flLine += m_pLapSupplier->GetGuideStepX(eX, dMinX, dMaxX))
			{
				LineColor();	//	Pick guideline color, based upon chosen color scheme
				char szText[256];
				GetChannelString(eX, sfLapOpts.eUnitPreference, flLine, szText, NUMCHARS(szText));
				DrawVerticalLine(flLine, mapMinY[*i], mapMaxY[*i], szText);
			}
		}	//	End of Guideline drawing loop

	}

    Vector2D ptHighlight; // the (x,y) coords in unit-space that we want to highlight.  Example: for a speed-distance graph, x would be in distance units, y in velocities.
    POINT ptMouse;
    if(GetMouse(&ptMouse) && m_pLapSupplier->IsHighlightSource(m_iSupplierId))
    {
      //		The mouse is in our window... we make our own highlighter, ignoring anything that got sent to us
      GLdouble dX,dY,dZ;
      gluUnProject(ptMouse.x, ptMouse.y, 0, rgModelviewMatrix, rgProjMatrix, rgViewport, &dX, &dY, &dZ);
      ptHighlight = V2D(dX,0);
    }
	GLushort pattern;	//	Stippling pattern
	GLint factor = 1;	// Stippling factor
    for(int x = 0; x < lstLaps.size(); x++)
    {
      CExtendedLap* pLap = lstLaps[x];
      const IDataChannel* pDataX = pLap->GetChannel(m_pLapSupplier->GetXChannel());
      const IDataChannel* pDataY = pLap->GetChannel(*i);
      //	If we are dealing with accelerometer data, pull and smooth that data
	  IDataChannel* pDataX_ACCEL;
	  IDataChannel* pDataY_ACCEL;
	  IDataChannel* pDataZ_ACCEL;
	  pDataX_ACCEL = (IDataChannel*) pLap->GetChannel(DATA_CHANNEL_X_ACCEL);
	  pDataY_ACCEL = (IDataChannel*) pLap->GetChannel(DATA_CHANNEL_Y_ACCEL);
	  pDataZ_ACCEL = (IDataChannel*) pLap->GetChannel(DATA_CHANNEL_Z_ACCEL);

	  float r;
	  float g;
	  float b;
	  if (x == lstLaps.size() - 1)	//	Means that this lap is the Reference Lap, so let's color it special
	  {
		pattern = 0xFFFF;  // Stipple pattern is a line for Reference Lap
		MakeColor ( pLap, true, &r, &g, &b ); // Function picks color to use and tells opengl to draw the following in the colour we just made up
	  }
	  else
	  {
		pattern = 0xEEEE;  // Stipple pattern
		MakeColor ( pLap, false, &r, &g, &b ); // Function picks color to use and tells opengl to draw the following in the colour we just made up
	  }

	  if(pDataX && pDataY)
	  {
		// tracking what we want to highlight
		int w = 4;	// * Default setting. w is the size of the smoothing window, taken on each side of sample
        float dBestLength = -1;
        float dTimeToHighlight = -1;
//		const vector<DataPoint> &lstPointsX = (vector<DataPoint>&) pDataX->GetData();
//      const vector<DataPoint> &lstPointsY = (vector<DataPoint>&) pDataY->GetData();
		//	Changed to non-constant as we want to smooth the data sometimes
		vector<DataPoint> &lstPointsX = (vector<DataPoint>&) pDataX->GetData();
        vector<DataPoint> &lstPointsY = (vector<DataPoint>&) pDataY->GetData();
		vector<DataPoint> lstPointsX_Accel;
		lstPointsX_Accel.begin();
		vector<DataPoint> lstPointsY_Accel;
		lstPointsY_Accel.begin();
		vector<DataPoint> lstPointsZ_Accel;
		lstPointsZ_Accel.begin();
		if ( eX == DATA_CHANNEL_X_ACCEL && sfLapOpts.bSmoothYesNo == true )
		{
			lstPointsX_Accel.clear();
			lstPointsX_Accel = pDataX_ACCEL->GetData();	//	pDataY->GetData()
			w = lstPointsX_Accel.size() / 400;	//	Sets the BoxAverage smoothing width, based upon the number of data points
		}
		if ( eX == DATA_CHANNEL_Y_ACCEL && sfLapOpts.bSmoothYesNo == true )
		{
			lstPointsY_Accel.clear();
			lstPointsY_Accel = pDataY_ACCEL->GetData();	//	pDataY->GetData()
			w = lstPointsY_Accel.size() / 400;	//	Sets the BoxAverage smoothing width, based upon the number of data points
		}
		if ( eX == DATA_CHANNEL_Z_ACCEL && sfLapOpts.bSmoothYesNo == true )
		{
			lstPointsZ_Accel.clear();
			lstPointsZ_Accel = pDataZ_ACCEL->GetData();	//	pDataY->GetData()
			w = lstPointsZ_Accel.size() / 400;	//	Sets the BoxAverage smoothing width, based upon the number of data points
		}

		if ( pDataY->GetChannelType() == DATA_CHANNEL_X_ACCEL && sfLapOpts.bSmoothYesNo == true )
		{
			lstPointsX_Accel.clear();
			lstPointsX_Accel = pDataX_ACCEL->GetData();	//	pDataY->GetData()
			w = lstPointsX_Accel.size() / 400;	//	Sets the BoxAverage smoothing width, based upon the number of data points
		}
		if ( pDataY->GetChannelType() == DATA_CHANNEL_Y_ACCEL && sfLapOpts.bSmoothYesNo == true )
		{
			lstPointsY_Accel.clear();
			lstPointsY_Accel = pDataY_ACCEL->GetData();	//	pDataY->GetData()
			w = lstPointsY_Accel.size() / 400;	//	Sets the BoxAverage smoothing width, based upon the number of data points
		}
		if ( pDataY->GetChannelType() == DATA_CHANNEL_Z_ACCEL && sfLapOpts.bSmoothYesNo == true )
		{
			lstPointsZ_Accel.clear();
			lstPointsZ_Accel = pDataZ_ACCEL->GetData();	//	pDataY->GetData()
			w = lstPointsZ_Accel.size() / 400;	//	Sets the BoxAverage smoothing width, based upon the number of data points
		}

		vector<DataPoint>& lstSmoothPts = (vector<DataPoint>) pDataX->GetData();
		if( (eX == DATA_CHANNEL_X_ACCEL || eX == DATA_CHANNEL_Y_ACCEL || eX == DATA_CHANNEL_Z_ACCEL) && sfLapOpts.bSmoothYesNo == true )
		{
			//	Smooth out the accerlometer data for all axes before displaying them on the X/Y-axes or in the Traction Circle display
			if (lstPointsX_Accel.size() ) 
			{
				lstSmoothPts.clear();
				fBoxMovingAvg( lstPointsX_Accel.size(), lstPointsX_Accel, w, lstSmoothPts, false );
				lstPointsX_Accel = lstSmoothPts;	//	Copy the smoothed data points over to the original data set
				lstSmoothPts.clear();
			}
			if (lstPointsY_Accel.size() ) 
			{
				lstSmoothPts.clear();
				fBoxMovingAvg( lstPointsY_Accel.size(), lstPointsY_Accel, w, lstSmoothPts, false );
				lstPointsY_Accel = lstSmoothPts;	//	Copy the smoothed data points over to the original data set
				lstSmoothPts.clear();
			}
			if (lstPointsZ_Accel.size() ) 
			{
				lstSmoothPts.clear();
				fBoxMovingAvg( lstPointsZ_Accel.size(), lstPointsZ_Accel, w, lstSmoothPts, false );
				lstPointsZ_Accel = lstSmoothPts;	//	Copy the smoothed data points over to the original data set
				lstSmoothPts.clear();
			}
		}

		if( (*i == DATA_CHANNEL_X_ACCEL || *i == DATA_CHANNEL_Y_ACCEL || *i == DATA_CHANNEL_Z_ACCEL) && sfLapOpts.bSmoothYesNo == true )
		{
			//	Smooth out the accerlometer data for all axes before displaying them on the X/Y-axes or in the Traction Circle display
			if (lstPointsX_Accel.size() ) 
			{
				lstSmoothPts.clear();
				fBoxMovingAvg( lstPointsX_Accel.size(), lstPointsX_Accel, w, lstSmoothPts, false );
				lstPointsX_Accel = lstSmoothPts;	//	Copy the smoothed data points over to the original data set
				lstSmoothPts.clear();
			}
			if (lstPointsY_Accel.size() ) 
			{
				lstSmoothPts.clear();
				fBoxMovingAvg( lstPointsY_Accel.size(), lstPointsY_Accel, w, lstSmoothPts, false );
				lstPointsY_Accel = lstSmoothPts;	//	Copy the smoothed data points over to the original data set
				lstSmoothPts.clear();
			}
			if (lstPointsZ_Accel.size() ) 
			{
				lstSmoothPts.clear();
				fBoxMovingAvg( lstPointsZ_Accel.size(), lstPointsZ_Accel, w, lstSmoothPts, false );
				lstPointsZ_Accel = lstSmoothPts;	//	Copy the smoothed data points over to the original data set
				lstSmoothPts.clear();
			}
	    }

		glEnable(GL_LINE_STIPPLE);
		glLineStipple(factor, pattern);	//	Set the line dash/dot characteristics
		if(sfLapOpts.fDrawLines == false)
		{
			glPointSize(4.0f);
			glBegin(GL_POINTS);
        }
		else if ( (eX == DATA_CHANNEL_X_ACCEL || eX == DATA_CHANNEL_Y_ACCEL || eX == DATA_CHANNEL_Z_ACCEL) && (*i == DATA_CHANNEL_X_ACCEL || *i == DATA_CHANNEL_Y_ACCEL || *i == DATA_CHANNEL_Z_ACCEL) )
		{
			//	Don't show lines for Traction Circle plots by default
			glPointSize(4.0f);
			glBegin(GL_POINTS);
		}
		else
		{
			glLineWidth(2);	// Added by KDJ. Sets the width of the line to draw.
			glBegin(GL_LINE_STRIP);
		}

		vector<DataPoint>::const_iterator iX ;
		if ( eX == DATA_CHANNEL_X_ACCEL && lstPointsX_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			iX = lstPointsX_Accel.begin(); 
		else if ( eX == DATA_CHANNEL_Y_ACCEL && lstPointsY_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			iX = lstPointsY_Accel.begin(); 
		else if ( eX == DATA_CHANNEL_Z_ACCEL && lstPointsZ_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			iX = lstPointsZ_Accel.begin(); 
		else
			iX = lstPointsX.begin();

		vector<DataPoint>::const_iterator iXend;
		if ( eX == DATA_CHANNEL_X_ACCEL && lstPointsX_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			iXend = lstPointsX_Accel.end(); 
		else if ( eX == DATA_CHANNEL_Y_ACCEL && lstPointsY_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			iXend = lstPointsY_Accel.end(); 
		else if ( eX == DATA_CHANNEL_Z_ACCEL && lstPointsZ_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			iXend = lstPointsZ_Accel.end(); 
		else
			iXend = lstPointsX.end();

		vector<DataPoint>::const_iterator iY;
		if ( *i == DATA_CHANNEL_X_ACCEL && lstPointsX_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			iY = lstPointsX_Accel.begin();
		else if ( *i == DATA_CHANNEL_Y_ACCEL && lstPointsY_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			iY = lstPointsY_Accel.begin(); 
		else if ( *i == DATA_CHANNEL_Z_ACCEL && lstPointsZ_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			iY = lstPointsZ_Accel.begin();
		else
			iY = lstPointsY.begin();

		vector<DataPoint>::const_iterator  iYend;
		if ( *i == DATA_CHANNEL_X_ACCEL && lstPointsX_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			iYend = lstPointsX_Accel.end(); 
		else if ( *i == DATA_CHANNEL_Y_ACCEL && lstPointsY_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			iYend = lstPointsY_Accel.end();
		else if ( *i == DATA_CHANNEL_Z_ACCEL && lstPointsZ_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			iYend = lstPointsZ_Accel.end();
		else
			iYend = lstPointsY.end();

		while(iX != iXend && iY != iYend)
        {
          float dX;
          float dY;
          const DataPoint& ptX = *iX;
          const DataPoint& ptY = *iY;
          int iTimeUsed = 0;
          if(ptX.iTimeMs < ptY.iTimeMs)
          {
            iTimeUsed = ptX.iTimeMs;
			if ( *i == DATA_CHANNEL_X_ACCEL && lstPointsX_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			{
				for(vector<DataPoint>::iterator t = lstPointsX_Accel.begin(); t != lstPointsX_Accel.end(); t++)
				{
					DataPoint& ptTemp = *t;
					if ( ptTemp.iTimeMs >= ptX.iTimeMs ) //	Found smoothed point, let's load the Y value
					{
						dY = ptTemp.flValue;
						break;
					}
				}
			}

			else if ( *i == DATA_CHANNEL_Y_ACCEL && lstPointsY_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			{
				for(vector<DataPoint>::iterator t = lstPointsY_Accel.begin(); t != lstPointsY_Accel.end(); t++)
				{
					DataPoint& ptTemp = *t;
					if ( ptTemp.iTimeMs >= ptX.iTimeMs ) //	Found smoothed point, let's load the Y value
					{
						dY = ptTemp.flValue;
						break;
					}
				}
			}

			else if ( *i == DATA_CHANNEL_Z_ACCEL && lstPointsZ_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			{
				for(vector<DataPoint>::iterator t = lstPointsY_Accel.begin(); t != lstPointsY_Accel.end(); t++)
				{
					DataPoint& ptTemp = *t;
					if ( ptTemp.iTimeMs >= ptX.iTimeMs ) //	Found smoothed point, let's load the Y value
					{
						dY = ptTemp.flValue;
						break;
					}
				}
			}
			else
			{
	            dX = ptX.flValue;
			}
			dY = pDataY->GetValue(ptX.iTimeMs, iY);
            iX++;
          }
          else if(ptX.iTimeMs > ptY.iTimeMs)
          {
            iTimeUsed = ptY.iTimeMs;

			if ( eX == DATA_CHANNEL_X_ACCEL && lstPointsX_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			{
				for(vector<DataPoint>::iterator t = lstPointsX_Accel.begin(); t != lstPointsX_Accel.end(); t++)
				{
					DataPoint& ptTemp = *t;
					if ( ptTemp.iTimeMs >= ptX.iTimeMs ) //	Found smoothed point, let's load the Y value
					{
						dX = ptTemp.flValue;
						break;
					}
				}
			}

			else if ( eX == DATA_CHANNEL_Y_ACCEL && lstPointsY_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			{
				for(vector<DataPoint>::iterator t = lstPointsY_Accel.begin(); t != lstPointsY_Accel.end(); t++)
				{
					DataPoint& ptTemp = *t;
					if ( ptTemp.iTimeMs >= ptX.iTimeMs ) //	Found smoothed point, let's load the Y value
					{
						dX = ptTemp.flValue;
						break;
					}
				}
			}

			else if ( eX == DATA_CHANNEL_Z_ACCEL && lstPointsZ_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			{
				for(vector<DataPoint>::iterator t = lstPointsY_Accel.begin(); t != lstPointsY_Accel.end(); t++)
				{
					DataPoint& ptTemp = *t;
					if ( ptTemp.iTimeMs >= ptX.iTimeMs ) //	Found smoothed point, let's load the Y value
					{
						dX = ptTemp.flValue;
						break;
					}
				}
			}
			else
			{
				dX = pDataX->GetValue(ptY.iTimeMs, iX);
			}
            dY = ptY.flValue;
            iY++;
          }
          else
          {
            iTimeUsed = ptY.iTimeMs;
            DASSERT(ptX.iTimeMs == ptY.iTimeMs);
            dX = ptX.flValue;
            dY = ptY.flValue;
            iX++;
            iY++;
          }
//////////////////////////////////////////
		  //	Can add transformation function here for Y
		  if (sfLapOpts.m_PlotPrefs[y].iTransformYesNo == true)
		  {
//			  dY = PolynomialFilter(ptY.flValue, sfLapOpts.m_PlotPrefs[y].fTransAValue, sfLapOpts.m_PlotPrefs[y].fTransBValue, sfLapOpts.m_PlotPrefs[y].fTransCValue);
			  dY = PolynomialFilter(dY, sfLapOpts.m_PlotPrefs[y].fTransAValue, sfLapOpts.m_PlotPrefs[y].fTransBValue, sfLapOpts.m_PlotPrefs[y].fTransCValue);
		  }
//		  else
//		  {
//			  dY = ptY.flValue;
//		  }
//////////////////////////////////////////
          glVertex2f(dX,dY);

          // if we're a highlight source, try to figure out the closest point for this lap
          if(m_pLapSupplier->IsHighlightSource(m_iSupplierId))
          {
            Vector2D vPt = V2D(dX,0);
            Vector2D vDiff = vPt - ptHighlight;
            if(vDiff.Length() < dBestLength || dBestLength < 0)
            {
              dBestLength = vDiff.Length();
              dTimeToHighlight = iTimeUsed;
            }
          }
        }
	    factor++;	//	Increment the line patterning
		glEnd();
		glDisable(GL_LINE_STIPPLE);
        // for each lap, draw an indicator of the closest thing to the mouse
        if(!m_pLapSupplier->IsHighlightSource(m_iSupplierId))
        {
          // if we're not a source, use the given time to highlight
          dTimeToHighlight = m_pLapSupplier->GetLapHighlightTime(pLap);
        }
        else
        {
          m_pLapSupplier->SetLapHighlightTime(pLap, (int)dTimeToHighlight);
        }
        UpdateHighlightPointList(lstMousePointsToDraw, pLap, rgModelviewMatrix, rgProjMatrix, rgViewport, dTimeToHighlight, pDataX, pDataY);
      }
    } // end lap loop

    if(lstMousePointsToDraw.size() > 0)
    {
      glPushMatrix(); // <-- pushes a matrix onto the opengl matrix stack.
      glLoadIdentity();	//  <-- makes it so that the matrix stack just converts all our coordinates directly to window coordinates
      glOrtho(0, RECT_WIDTH(&rcSpot),0, RECT_HEIGHT(&rcSpot),-1.0,1.0);
	  //  <-- tells OpenGL that it should show us the part of the openGL "world" that corresponds to (0...window width, 0 ... window height).  
	  //	This completes the "hey opengl, just draw where we tell you to plz" part of the function
      for(int x = 0; x < lstMousePointsToDraw.size(); x++)	// <-- loops through all the stupid boxes/lines we want to draw
      {
        const CExtendedLap* pLap = lstMousePointsToDraw[x].m_pLap;	//  <-- gets the lap data we want to draw
        const POINT& ptWindow = lstMousePointsToDraw[x].m_ptWindow;	// <-- gets info about where in the window we want to draw the box
        const IDataChannel* pDataX = lstMousePointsToDraw[x].m_pDataX;	//  <-- gets the x channel data
        const IDataChannel* pDataY = lstMousePointsToDraw[x].m_pDataY;	// <-- gets the y channel data
		POINT Temp_ptWindow;	//	Point for transforming Y values in highlight loop

		float r;
		float g;
		float b;
		MakeColor ( pLap, x == (lstLaps.size() - 1), &r, &g, &b ); // Function picks color to use and tells opengl to draw the following in the colour we just made up

		//	For TIME displayed on X-axis, remove all data channel text so that user can see the trends more clearly.
		if (pDataX->GetChannelType() != DATA_CHANNEL_TIME)
		{

			// if we're the main screen, we want to draw some text data for each point
			TCHAR szLapName[256];
			pLap->GetString(szLapName, NUMCHARS(szLapName));	// <-- gets the string "10:11:12 - 1:40.59 - Keith", aka the "lap name"

			float dTimeToHighlight = m_pLapSupplier->GetLapHighlightTime(pLap);	//  <-- asks the ILapSupplier interface what we should highlight

			TCHAR szTypeX[256];
			::GetDataChannelName(eX,szTypeX,NUMCHARS(szTypeX));	// <-- converts the data channel type into a string, like "Oil Temperature"

			TCHAR szTypeY[256];
			::GetDataChannelName(lstMousePointsToDraw[x].m_eChannelY, szTypeY, NUMCHARS(szTypeY));	// <-- converts the y channel into a string

			char szYString[256];
			float TempY;
//////////////////////////////////////////
//			Adding transformation functions here for Y
			if (sfLapOpts.m_PlotPrefs[y].iTransformYesNo == true)
			{
				TempY = PolynomialFilter(pDataY->GetValue(dTimeToHighlight), sfLapOpts.m_PlotPrefs[y].fTransAValue, sfLapOpts.m_PlotPrefs[y].fTransBValue, sfLapOpts.m_PlotPrefs[y].fTransCValue);
				//	Transform highlight points into OpenGL space
				GLdouble winx,winy,winz;
				gluProject(0, (GLdouble)TempY, 0, rgModelviewMatrix, rgProjMatrix, rgViewport, &winx, &winy, &winz);
				Temp_ptWindow.y = (int)winy;
			}
			else
			{
				TempY = pDataY->GetValue(dTimeToHighlight);
				Temp_ptWindow.y = ptWindow.y;
			}
//////////////////////////////////////////
			GetChannelString(lstMousePointsToDraw[x].m_eChannelY, sfLapOpts.eUnitPreference, TempY, szYString, NUMCHARS(szYString));
//			GetChannelString(lstMousePointsToDraw[x].m_eChannelY, sfLapOpts.eUnitPreference, pDataY->GetValue(dTimeToHighlight), szYString, NUMCHARS(szYString));
			// <-- gets the actual unit string for the data channel.  For speed, this might be "100.0km/h"

			char szXString[256];
			GetChannelString(eX, sfLapOpts.eUnitPreference, pDataX->GetValue(dTimeToHighlight), szXString, NUMCHARS(szXString));
			// <-- same for x channel

			char szText[256];
			sprintf(szText, "%S - (%S @ %S) %s @ %s", szLapName, szTypeY, szTypeX, szYString, szXString);

			DrawText(100.0,(x+2)*GetWindowFontSize(),szText);	// <-- draws the text from the bottom of the window, working upwards

			// we also want to draw a highlighted square
//			DrawGLFilledSquare(ptWindow.x, ptWindow.y, 3);	// <-- draws the stupid little box at ptWindow.x.
			DrawGLFilledSquare((double)ptWindow.x, (double)(Temp_ptWindow.y - rcSpot.top), 3);	// <-- draws the stupid little box at ptWindow.x.
			// we also want to draw a highlighted LINE for that individual lap/graph combination
			glLineWidth(1);								// Skinny vertical line for mouse location markers.
			glBegin(GL_LINE_STRIP);
			glVertex3f(ptWindow.x, 0, 0);
			glVertex3f(ptWindow.x,rcSpot.bottom,0);
			glEnd();
		}
	  }
      glPopMatrix();
      glPopMatrix();	//	Pops us out of windows space to OGL space
    }
    rcSpot.top += iSegmentHeight;
    rcSpot.bottom += iSegmentHeight;
  } // end y-channel data channel loop
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	
}

void CLapPainter::DrawTractionCircle(const LAPSUPPLIEROPTIONS& sfLapOpts, bool fHighlightXAxis)
{
  vector<CExtendedLap*> lstLaps = m_pLapSupplier->GetLapsToShow();
  //	Let's check on which orientation the phone is in so that the Traction Circle is displayed correctly
  //	Vertical/Landscape: X = X-Accel, Y=Y-Accel; Vertical/Portrait: X = Z-Accel, Y=Y-Accel; Flat/Landscape: X=X-Accel, Y=Z-Accel; Flat/Portrait: X=Z-Axis, Y=X-Axis
  DATA_CHANNEL eX;
  DATA_CHANNEL DataY;
  switch (sfLapOpts.e_Orientation)
  {
	  case VERTICAL_LANDSCAPE:
	  {
		eX = DATA_CHANNEL_X_ACCEL;
		DataY = DATA_CHANNEL_Y_ACCEL;
		break;
	  }
	  case VERTICAL_PORTRAIT:
	  {
		eX = DATA_CHANNEL_Z_ACCEL;
		DataY = DATA_CHANNEL_Y_ACCEL;
		break;
	  }
	  case FLAT_LANDSCAPE:
	  {
		eX = DATA_CHANNEL_X_ACCEL;
		DataY = DATA_CHANNEL_Z_ACCEL;
		break;
	  }
	  case FLAT_PORTRAIT:
	  {
		eX = DATA_CHANNEL_Z_ACCEL;
		DataY = DATA_CHANNEL_X_ACCEL;
		break;
	  }
  }
  set<DATA_CHANNEL> setY;
  map<DATA_CHANNEL,float> mapMinY, mapMinYTemp;
  map<DATA_CHANNEL,float> mapMaxY, mapMaxYTemp;
  float dMaxX = -1e30;
  float dMinX = 1e30;
  float dCenterOvalX = 0;
  float dCenterOvalY = 0;
/////////////////////////////////////////
  bool b_TransformY = false;	//	Flag if we should transform this channel using PlotPrefs polynomial function false = no, true = yes
  int i_TransInt = 0;		//	PlotPrefs index for transformation function coefficients
/////////////////////////////////////////
  // figuring out bounds and getting matrices all set up
  //	First lets load up all of the data into an array and determine its size
  for (int x = 0; x < lstLaps.size(); x++)
  {
      CExtendedLap* pLap = lstLaps[x];
      DATA_CHANNEL eDataX = eX;
      const IDataChannel* pDataX = pLap->GetChannel(eDataX);
      if(!pDataX || !pDataX->IsValid() || pDataX->GetData().size() <= 0) continue;
      {
//////////////////////////////////
		b_TransformY = false;
		i_TransInt = 0;
//////////////////////////////////
		const IDataChannel* pChannel = pLap->GetChannel(DataY);
        if (!pChannel || !pChannel->IsValid()) continue;

        const DATA_CHANNEL eType = DataY;

      if(mapMinYTemp.find(eType) == mapMinYTemp.end())
        {
          mapMinYTemp[eType] = min(pChannel->GetMin(),m_pLapSupplier->GetDataHardcodedMin(eType));
          mapMaxYTemp[eType] = max(pChannel->GetMax(),m_pLapSupplier->GetDataHardcodedMax(eType));
        }
        else
        {
          mapMinYTemp[eType] = min(pChannel->GetMin(),mapMinYTemp[eType]);
          mapMaxYTemp[eType] = max(pChannel->GetMax(),mapMaxYTemp[eType]);
		}
////////////////////////////////
		//	Adding transformation functions here for Min/MaxY
		if (b_TransformY == true && sfLapOpts.m_PlotPrefs[i_TransInt].fTransBValue < 0)
		{
			mapMaxY[eType] = (float)PolynomialFilter(mapMinYTemp[eType], sfLapOpts.m_PlotPrefs[i_TransInt].fTransAValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransBValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransCValue);
			mapMinY[eType] = (float)PolynomialFilter(mapMaxYTemp[eType], sfLapOpts.m_PlotPrefs[i_TransInt].fTransAValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransBValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransCValue);
		}
		else if (b_TransformY == true)
		{
			mapMinY[eType] = (float)PolynomialFilter(mapMinYTemp[eType], sfLapOpts.m_PlotPrefs[i_TransInt].fTransAValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransBValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransCValue);
			mapMaxY[eType] = (float)PolynomialFilter(mapMaxYTemp[eType], sfLapOpts.m_PlotPrefs[i_TransInt].fTransAValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransBValue, sfLapOpts.m_PlotPrefs[i_TransInt].fTransCValue);
		}
		else
		{
			mapMinY[eType] = mapMinYTemp[eType];
			mapMaxY[eType] = mapMaxYTemp[eType];
		}
////////////////////////////////
		setY.insert(eType);
      }
      if(pDataX)
      {
        dMaxX = max(dMaxX, pDataX->GetMax());
        dMinX = min(dMinX, pDataX->GetMin());

//        eX = pDataX->GetChannelType();
      }
	  b_TransformY = false;	//	Reset the flag for transforming the Data Channel
  }	//	End of finding bounds and loading laps loop
  
  if (m_pLapSupplier->GetDisplayOptions().fColorScheme)
  {
		glClearColor( 0.03, 0.03f, 0.03f, 0.05f );  //  Background color is black.
  }
  else
  {
		glClearColor( 0.97, 0.97f, 0.97f, 0.95f );  //  Background color is grey.
  }
  glClear( GL_COLOR_BUFFER_BIT );


  RECT rcSpot;
  {
    RECT rcClient;
    GetClientRect(OGL_GetHWnd(), &rcClient);
    glViewport(0,0,RECT_WIDTH(&rcClient), RECT_HEIGHT(&rcClient));
  
    rcSpot.left = 0;
    rcSpot.top = 0;
    rcSpot.right = RECT_WIDTH(&rcClient);
    rcSpot.bottom = RECT_HEIGHT(&rcClient);
  }

    if(setY.size() <= 0)
  {
    DrawSelectLapsPromptShort();
    return;
  }

  int iPos = 0;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // y-channel graphing loop start

  for(set<DATA_CHANNEL>::iterator i = setY.begin(); i != setY.end(); i++)
  {
    vector<HIGHLIGHTDATA> lstMousePointsToDraw;
    glViewport(rcSpot.left,rcSpot.top,RECT_WIDTH(&rcSpot),RECT_HEIGHT(&rcSpot));
	int y = 0;	//	Tracker for PlotPrefs array variable
	for (int z = 0; z < 49; z++)
	{
		if (sfLapOpts.m_PlotPrefs[z].iDataChannel == *i)
		{
			y = z;
			break;
		}
	}
	// now we have the bounds of all the laps we've looked at, so let's draw them
    glPushMatrix();
    glLoadIdentity();
    glScalef(1.0f, 1.0f, 1.0f);	// Let's scale it so that graphs don't touch each other.
    glOrtho(dMinX, dMaxX, mapMinY[*i], mapMaxY[*i], -1.0, 1.0);

	//	Set up the non-zoomed/panned view for the map
    GLdouble rgModelviewMatrix[16];
    GLdouble rgProjMatrix[16];
    GLint rgViewport[4];
    
	{
		//	Now that the matrices are correct, let's graph them.    
		glGetDoublev(GL_MODELVIEW_MATRIX, rgModelviewMatrix);
		glGetDoublev(GL_PROJECTION_MATRIX, rgProjMatrix);
		glGetIntegerv(GL_VIEWPORT, rgViewport);
		if (m_pLapSupplier->GetDisplayOptions().fColorScheme)
		{
			glColor3d(1.0,1.0,1.0);	//	Background is black, lines are white
		}
		else
		{
			glColor3d(0.0,0.0,0.0);	//	Background is grey, lines are black
		}
		drawOval (dCenterOvalX, dCenterOvalY, 3.0f, 3.0f);	//	Draw 1.5G circle
		drawOval (dCenterOvalX, dCenterOvalY, 2.0f, 2.0f);	//	Draw 1.0G circle
		drawOval (dCenterOvalX, dCenterOvalY, 1.0f, 1.0f);	//	Draw 0.5G circle
	}

    Vector2D ptHighlight; // the (x,y) coords in unit-space that we want to highlight.  Example: for a speed-distance graph, x would be in distance units, y in velocities.
    for(int x = 0; x < lstLaps.size(); x++)
    {
      CExtendedLap* pLap = lstLaps[x];
      const IDataChannel* pDataX = pLap->GetChannel(eX);	//	Only pull the X-Accel data for this graph
      const IDataChannel* pDataY = pLap->GetChannel(*i);

	  if(pDataX && pDataY)
	  {
		  // tracking what we want to highlight
		int w = 4;	// * Default setting. w is the size of the smoothing window, taken on each side of sample
        float dBestLength = -1;
        float dTimeToHighlight = -1;
        //	Changed to non-constant as we want to smooth the data sometimes
		vector<DataPoint>& lstPointsX = (vector<DataPoint>&) pDataX->GetData();
        vector<DataPoint>& lstPointsY = (vector<DataPoint>&) pDataY->GetData();
		//	If we are dealing with accelerometer data, pull and smooth that data
		IDataChannel* pDataX_ACCEL;
		IDataChannel* pDataY_ACCEL;
		IDataChannel* pDataZ_ACCEL;
		pDataX_ACCEL = (IDataChannel*) pLap->GetChannel(DATA_CHANNEL_X_ACCEL);
		pDataY_ACCEL = (IDataChannel*) pLap->GetChannel(DATA_CHANNEL_Y_ACCEL);
		pDataZ_ACCEL = (IDataChannel*) pLap->GetChannel(DATA_CHANNEL_Z_ACCEL);

		float r;
		float g;
		float b;
		if (x == lstLaps.size() - 1)	//	We color the last lap, or Reference Lap if it's chosen, as black
		{
			MakeColor ( pLap, true, &r, &g, &b ); // Function picks color to use and tells opengl to draw the following in the colour we just made up
		}
		else
		{
			MakeColor ( pLap, false, &r, &g, &b ); // Function picks color to use and tells opengl to draw the following in the colour we just made up
		}

		glPointSize(2.0f);
		glBegin(GL_POINTS);
		
		vector<DataPoint> lstPointsX_Accel;
		lstPointsX_Accel.begin();
		vector<DataPoint> lstPointsY_Accel;
		lstPointsY_Accel.begin();
		vector<DataPoint> lstPointsZ_Accel;
		lstPointsZ_Accel.begin();
		if ( eX == DATA_CHANNEL_X_ACCEL && sfLapOpts.bSmoothYesNo == true )
		{
			lstPointsX_Accel.clear();
			lstPointsX_Accel = pDataX_ACCEL->GetData();	//	pDataY->GetData()
			w = lstPointsX_Accel.size() / 400;	//	Sets the BoxAverage smoothing width, based upon the number of data points
		}
		if ( pDataY->GetChannelType() == DATA_CHANNEL_Y_ACCEL && sfLapOpts.bSmoothYesNo == true )
		{
			lstPointsY_Accel.clear();
			lstPointsY_Accel = pDataY_ACCEL->GetData();	//	pDataY->GetData()
			w = lstPointsY_Accel.size() / 400;	//	Sets the BoxAverage smoothing width, based upon the number of data points
		}
		vector<DataPoint>& lstSmoothPts = (vector<DataPoint>) pDataX->GetData();
		//	Smooth out the accerlometer data for X-Axis before displaying them on the Traction Circle display
		if (lstPointsX_Accel.size() ) 
		{
			lstSmoothPts.clear();
			fBoxMovingAvg( lstPointsX_Accel.size(), lstPointsX_Accel, w, lstSmoothPts, false );
			lstPointsX_Accel = lstSmoothPts;	//	Copy the smoothed data points over to the original data set
			lstSmoothPts.clear();
		}
		if (lstPointsY_Accel.size() ) 
		{
			lstSmoothPts.clear();
			fBoxMovingAvg( lstPointsY_Accel.size(), lstPointsY_Accel, w, lstSmoothPts, false );
			lstPointsY_Accel = lstSmoothPts;	//	Copy the smoothed data points over to the original data set
			lstSmoothPts.clear();
		}
		vector<DataPoint>::const_iterator iX ;
		if ( eX == DATA_CHANNEL_X_ACCEL && lstPointsX_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			iX = lstPointsX_Accel.begin(); 
		else
			iX = lstPointsX.begin();

		vector<DataPoint>::const_iterator iXend;
		if ( eX == DATA_CHANNEL_X_ACCEL && lstPointsX_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			iXend = lstPointsX_Accel.end(); 
		else
			iXend = lstPointsX.end();

		vector<DataPoint>::const_iterator iY;
		if ( *i == DATA_CHANNEL_Y_ACCEL && lstPointsY_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			iY = lstPointsY_Accel.begin(); 
		else
			iY = lstPointsY.begin();

		vector<DataPoint>::const_iterator  iYend;
		if ( *i == DATA_CHANNEL_Y_ACCEL && lstPointsY_Accel.size() && sfLapOpts.bSmoothYesNo == true ) 
			iYend = lstPointsY_Accel.end();
		else
			iYend = lstPointsY.end();

        dTimeToHighlight = m_pLapSupplier->GetLapHighlightTime(pLap);
		while(iX != iXend && iY != iYend)
        {
          float dX;
          float dY;
          const DataPoint& ptX = *iX;
          const DataPoint& ptY = *iY;
          int iTimeUsed = 0;
          if(ptX.iTimeMs < ptY.iTimeMs)
          {
            iTimeUsed = ptX.iTimeMs;
            dX = ptX.flValue;
            dY = pDataY->GetValue(ptX.iTimeMs, iY);
            iX++;
          }
          else if(ptX.iTimeMs > ptY.iTimeMs)
          {
            iTimeUsed = ptY.iTimeMs;
            dX = pDataX->GetValue(ptY.iTimeMs, iX);
            dY = ptY.flValue;
            iY++;
          }
          else
          {
            iTimeUsed = ptY.iTimeMs;
            DASSERT(ptX.iTimeMs == ptY.iTimeMs);
            dX = ptX.flValue;
            dY = ptY.flValue;
            iX++;
            iY++;
          }
//////////////////////////////////////////
		  //	Can add transformation function here for Y
		  if (sfLapOpts.m_PlotPrefs[y].iTransformYesNo == true)
		  {
			  dY = PolynomialFilter(dY, sfLapOpts.m_PlotPrefs[y].fTransAValue, sfLapOpts.m_PlotPrefs[y].fTransBValue, sfLapOpts.m_PlotPrefs[y].fTransCValue);
		  }
//////////////////////////////////////////
          if (sfLapOpts.bTractionCircle)	//	User wants dynamic traction circle, not full lap to be displayed
		  {
			  if (iTimeUsed > (dTimeToHighlight - 1500) && iTimeUsed <= (dTimeToHighlight))	//	Only display traction circle points near the highlighted point
			  {
				  glVertex2f(dX,dY);
			  }
		  }
		  else
		  {
			  glVertex2f(dX,dY);
		  }
        }
		glEnd();
        // for each lap, draw an indicator of the closest thing to the mouse
          // if we're not a source, use the given time to highlight
        UpdateHighlightPointList(lstMousePointsToDraw, pLap, rgModelviewMatrix, rgProjMatrix, rgViewport, dTimeToHighlight, pDataX, pDataY);
      }
    } // end lap loop

    if(lstMousePointsToDraw.size() > 0)
    {
      glPushMatrix(); // <-- pushes a matrix onto the opengl matrix stack.
      glLoadIdentity();	//  <-- makes it so that the matrix stack just converts all our coordinates directly to window coordinates
      glOrtho(0, RECT_WIDTH(&rcSpot),0, RECT_HEIGHT(&rcSpot),-1.0,1.0);
//	  <-- tells OpenGL that it should show us the part of the openGL "world" that corresponds to 
//	  (0...window width, 0 ... window height).  This completes the "hey opengl, just draw where we 
//	  tell you to plz" part of the function

      for(int x = 0; x < lstMousePointsToDraw.size(); x++)	// <-- loops through all the stupid boxes/lines we want to draw
      {
        const CExtendedLap* pLap = lstMousePointsToDraw[x].m_pLap;	//  <-- gets the lap data we want to draw
        const POINT& ptWindow = lstMousePointsToDraw[x].m_ptWindow;	// <-- gets info about where in the window we want to draw the box

		float r;
		float g;
		float b;
		MakeColor ( pLap, x == (lstLaps.size() - 1), &r, &g, &b ); // Function picks color to use and tells opengl to draw the following in the colour we just made up

		// Let's draw a highlighted square
		DrawGLFilledSquare(ptWindow.x, ptWindow.y, 4);	// <-- draws the stupid little box at ptWindow.x.
		// we also want to draw a highlighted horizontal and vertical LINE for that individual lap/graph combination
		glLineWidth(1);
		glBegin(GL_LINE_STRIP);
		glVertex3f(ptWindow.x, rcSpot.top, 0);
		glVertex3f(ptWindow.x,rcSpot.bottom,0);
		glEnd();
		glBegin(GL_LINE_STRIP);
		glVertex3f(rcSpot.left, ptWindow.y, 0);
		glVertex3f(rcSpot.right, ptWindow.y, 0);
		glEnd();
	  }
      glPopMatrix();
      glPopMatrix();
    }
  } // end y-channel data channel loop
}

//  Draws an oval centered at (x_center, y_center) and is is bound inside a rectangle whose width is w and height is h.
void CLapPainter::drawOval (float x_center, float y_center, float w, float h)
{
    int n = 50;	// n represents the number of line segments used to draw the oval.
	float PI_2 = 3.14159 * 2;
	float theta, angle_increment;
    float x, y;
    if (n <= 0)
        n = 1;
    angle_increment = PI_2 / n;
    //  center the oval at x_center, y_center
    //  draw the oval using line segments
    glBegin (GL_LINE_LOOP);
 
    for (theta = 0.0f; theta < PI_2; theta += angle_increment)
    {
        x = x_center + w/2 * cos (theta);
        y = y_center + h/2 * sin (theta);
		//	Draw the vertices  
        glVertex2f (x, y);
    }
    glEnd ();
}
// Function for setting highlighting color, making sure that there is enough contrast to a black background
void CLapPainter::MakeColor(const CExtendedLap* pLap, bool RefLapFlag, float* pR, float* pG, float* pB) 
{ 
	srand((int)pLap);	//  <-- makes sure that we randomize the colours consistently, so that lap plots don't change colour from draw to draw... 

		//	Background color options: 0 (00),0,0 (Black) or 0.95 (F2),0.95.0.95 (Lt. Grey)
		//	Need to check contrast ratio before allowing the color
		//	Contrast ratio is defined as Luminace ratio L1/L2
		//  Luminance = (0.2126 × red) + (0.7152 × green) + (0.0722 × blue) 
		//  Hexadecimal value of colour component (00-FF)

	if (m_pLapSupplier->GetDisplayOptions().fColorScheme)	//	Background color is black, make sure there is enough contrast with the lines
	{
		float fLBackground = (0.2126 * 0.05) + (0.7152 * 0.05) + (0.0722 * 0.05);	//	Background Luminace value is zero (black)
		float fLForeground;
		float fContrastRatio;
		
		do 
		{ 
			*pR = RandDouble(); 
			*pG = RandDouble(); 
			*pB = RandDouble(); 
			//	Contrast Ratio is (L1 + 0.05) / (L2 + 0.05) where L1 is the relative luminance of the lighter of the colors, 
			//	L2 is the relative luminance of the darker of the colors. Contrast ratios can range from 1 to 21 (commonly written 1:1 to 21:1)
			fLForeground = (0.2126 * *pR) + (0.7152 * *pG) + (0.0722 * *pB);
			fContrastRatio = (fLForeground + 0.05) / (fLBackground + 0.05);
		} 
		while(fContrastRatio < 4.5 ); //	Cinema standard is for greater than 4.5 to 1 contrast ratio
		glColor3d( *pR, *pG, *pB ); // Final color to use.  Tells opengl to draw the following in the colour we just made up
	}
	else	//	Background color is light grey, keep these colors low enough alpha to provide contrast
	{
		//	Calculate the Background Luminace value
		float fLBackground = (0.2126 * 0.95) + (0.7152 * 0.95) + (0.0722 * 0.95);
		float fLForeground;
		float fContrastRatio;
		do 
		{ 
			*pR = RandDouble(); 
			*pG = RandDouble(); 
			*pB = RandDouble(); 
			//	Contrast Ratio is (L1 + 0.05) / (L2 + 0.05) where L1 is the relative luminance of the lighter of the colors, 
			//	L2 is the relative luminance of the darker of the colors. Contrast ratios can range from 1 to 21 (commonly written 1:1 to 21:1)
			fLForeground = (0.2126 * *pR) + (0.7152 * *pG) + (0.0722 * *pB);
			fContrastRatio = (fLBackground + 0.05) / (fLForeground + 0.05);
		} 
		while(fContrastRatio < 3.5 ); //	Cinema standard is for greater than 4.5 to 1 contrast ratio
		glColor3d( *pR, *pG, *pB ); // Final color to use.  Tells opengl to draw the following in the colour we just made up
	}
	//	Check if this the is the Reference Lap. If so, change the color to full White/Black
	if (RefLapFlag && m_pLapSupplier->GetDisplayOptions().fColorScheme)	//	Background color is black, make Reference Lap white
	{
		*pR = 0.90; *pG = 0.90; *pB = 0.90;
	}
	else if (RefLapFlag)	//	Background color is Grey, make Reference Lap Black
	{
		*pR = 0.0; *pG = 0.0; *pB = 0.0;
	}
	glColor3d( *pR, *pG, *pB ); // Final color to use.  Tells opengl to draw the following in the colour we just made up
}
// Function converts the Y-value for a data channel to a transformed polynomial value
double CLapPainter::PolynomialFilter(double flValue, double fTransAValue, double fTransBValue, double fTransCValue)
{
  return fTransAValue + flValue * fTransBValue + flValue * flValue * fTransCValue;
}

void CLapPainter::DrawHorizontalLine(float flLine, float dMinX, float dMaxX, char szText[512])
{
	glLineWidth(1);			// Added by KDJ. Skinny lines for guidelines.
	glBegin(GL_LINE_STRIP);
	glVertex2f(dMinX,flLine);
	glVertex2f(dMaxX,flLine);
	glEnd();
	DrawText(dMinX, flLine, szText);	//	Draw the text information on the graph as well
}

void CLapPainter::DrawVerticalLine(double flLine, float mapMinY, float mapMaxY, char szText[512])
{
	glLineWidth(1);      
	glBegin(GL_LINE_STRIP);
	glVertex3f(flLine,mapMinY,0);
	glVertex3f(flLine,mapMaxY,0);
	glEnd();
	DrawText(flLine, mapMinY, szText);
}

void CLapPainter::LineColor() 
{ 
	if (m_pLapSupplier->GetDisplayOptions().fColorScheme)	//	Background color is black
	{
		glColor3d(0.7,0.7,0.7);	//	Make guidelines grey to show up on black background
	}
	else
	{
		glColor3d(0.1,0.1,0.5);	// Reduced the brightness of the guidelines to match text, and to better see the data lines
	}
}

//	Function for showing the map display
void CLapPainter::DrawLapLines(const LAPSUPPLIEROPTIONS& sfLapOpts)
{
  FLOATRECT rcAllLaps = m_pLapSupplier->GetAllLapsBounds();

  RECT rcClient;
  GetClientRect(OGL_GetHWnd(), &rcClient);
  double dClientAspect = ((double)RECT_WIDTH(&rcClient)) / ((double)RECT_HEIGHT(&rcClient));
  double dMapAspect = abs(RECT_WIDTH(&rcAllLaps)) / abs(RECT_HEIGHT(&rcAllLaps));
  const int cSectors = 9;	//	Max. number of Split Points

  if(dClientAspect > dMapAspect)
  {
    // the window is wider than the map is.  we'll want to widen the map space we show appropriately
    const double dCentreX = (rcAllLaps.left + rcAllLaps.right)/2;
    rcAllLaps.left -= dCentreX;
    rcAllLaps.right -= dCentreX;
    rcAllLaps.left *= (dClientAspect / dMapAspect);
    rcAllLaps.right *= (dClientAspect / dMapAspect);
    rcAllLaps.left += dCentreX;
    rcAllLaps.right += dCentreX;
  }
  else
  {
    // the window is taller than the map points are.  we'll want to tallen map bounds
    const double dCentreY = (rcAllLaps.top + rcAllLaps.bottom)/2;
    rcAllLaps.top -= dCentreY;
    rcAllLaps.bottom -= dCentreY;
    rcAllLaps.top *= (dMapAspect / dClientAspect);
    rcAllLaps.bottom *= (dMapAspect / dClientAspect);
    rcAllLaps.top += dCentreY;
    rcAllLaps.bottom += dCentreY;

  }
  // we have now determined the bounds of the thing we're going to draw, let's draw it
  glPushMatrix();
  glLoadIdentity();
  glScalef(1.0f, 1.0f, 1.0f);	// Scale it so that it fills up the window.
  glOrtho(rcAllLaps.left, rcAllLaps.right,rcAllLaps.bottom,rcAllLaps.top, -1.0,1.0);
  
  //		Set up the non-zoomed/panned view for the map
  GLdouble rgModelviewMatrix[16];
  GLdouble rgProjMatrix[16];
  GLint rgViewport[4];
  vector<CExtendedLap*> lstLaps = m_pLapSupplier->GetLapsToShow();		//	Moved up-code to allow for Reference lap zooming

  {
	//	Now that the matrices are correct, let's graph them.    
	glGetDoublev(GL_MODELVIEW_MATRIX, rgModelviewMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, rgProjMatrix);
    glGetIntegerv(GL_VIEWPORT, rgViewport);
  
    const double dCenterX = (rcAllLaps.left + rcAllLaps.right)/2;
    const double dCenterY = (rcAllLaps.top + rcAllLaps.bottom)/2;
    double dScaleAmt = pow(1.1,sfLapOpts.iZoomLevels);

	POINT ptMouse;
	if(GetMouse(&ptMouse) && m_pLapSupplier->IsHighlightSource(m_iSupplierId))
	{
		// the mouse is in our window, so let's enable panning and zooming!
		const double dTranslateShiftX = (rcAllLaps.left + rcAllLaps.right)/2;
		const double dTranslateShiftY = (rcAllLaps.top + rcAllLaps.bottom)/2;
		GLdouble dXShift,dYShift,dZ;

		//		Project the window shift stuff so we know how far to translate the view
		gluUnProject(sfLapOpts.flWindowShiftX/dScaleAmt,sfLapOpts.flWindowShiftY/dScaleAmt,0,rgModelviewMatrix,rgProjMatrix,rgViewport,&dXShift,&dYShift,&dZ);

		//		Set up to perform the ZOOM function for MAP.   
		glTranslated(dTranslateShiftX,dTranslateShiftY,0);	// Translate the map to origin
		glScaled(dScaleAmt,dScaleAmt,dScaleAmt);	//	Scale the sucker
		glTranslated(-dTranslateShiftX,-dTranslateShiftY,0);	// Now put the map back in its place

		glTranslated(dXShift-rcAllLaps.left,dYShift-rcAllLaps.bottom,0);	// Now let's allow it to pan
		// now having shifted, let's get our new model matrices
		glGetDoublev(GL_MODELVIEW_MATRIX, rgModelviewMatrix);
		glGetDoublev(GL_PROJECTION_MATRIX, rgProjMatrix);
		glGetIntegerv(GL_VIEWPORT, rgViewport);
	}
	else if(dScaleAmt > 1)	//	If image is Zoomed In, center the panning of the secondary display (map) to the highlighted point of the Reference Lap
	{
		// the mouse outside our window, let's match the zooming of the main display and pan to the highlighted location
		const double dTranslateShiftX = (rcAllLaps.left + rcAllLaps.right)/2;
		const double dTranslateShiftY = (rcAllLaps.top + rcAllLaps.bottom)/2;
		CExtendedLap* pLap;
		if (lstLaps.size() > 0) 
		{
			pLap = lstLaps[lstLaps.size() - 1];	//	Get the Reference Lap for scaling
		}
		else
		{
			return;
		}
		const vector<TimePoint2D>& lstPoints = pLap->GetPoints();	//	Get the list of point for the Reference Lap
		float dBestLength = -1;
		TimePoint2D ptBest;					//	Best highlighted point where the mouse is on the Reference Lap
		for(int x = 0; x< lstPoints.size(); x++)
		{
			const TimePoint2D& p = lstPoints[x];
			glVertex2f(p.flX,p.flY);
			int iTime = m_pLapSupplier->GetLapHighlightTime(pLap);
			if(abs(p.iTime - iTime) < dBestLength || dBestLength < 0)
			{
			  dBestLength = abs(p.iTime - iTime);
			  ptBest = p;					//	ptBest contains the X/Y values for the highlighted point of the Reference Lap
			}
		}
		//		Set up to perform the ZOOM function for MAP.   
		glTranslated(dTranslateShiftX, dTranslateShiftY, 0);	// Translate the map to origin
		glScaled(dScaleAmt*1.5, dScaleAmt*1.5, 1.0f);	//	Scale the sucker
		glTranslated(-dTranslateShiftX, -dTranslateShiftY, 0);	// Now put the map back in its place

		glTranslated(dTranslateShiftX-(double)ptBest.flX, dTranslateShiftY-(double)ptBest.flY, 0);	// Now move the map to the highlighted location
		// now having shifted, let's get our new model matrices
		glGetDoublev(GL_MODELVIEW_MATRIX, rgModelviewMatrix);
		glGetDoublev(GL_PROJECTION_MATRIX, rgProjMatrix);
		glGetIntegerv(GL_VIEWPORT, rgViewport);
	}
  }
  POINT ptMouse;
  Vector2D vHighlight;
  if(GetMouse(&ptMouse) && m_pLapSupplier->IsHighlightSource(m_iSupplierId))
  {
    // the mouse is in our window... we make our own highlighter
    GLdouble dX,dY,dZ;

    gluUnProject(ptMouse.x,ptMouse.y,0,rgModelviewMatrix,rgProjMatrix,rgViewport,&dX,&dY,&dZ);
    vHighlight = V2D((float)dX,(float)dY);
  }
  
  vector<MAPHIGHLIGHT> lstMousePointsToDraw;
  for(int x = 0; x < lstLaps.size(); x++)
  {
    CExtendedLap* pLap = lstLaps[x];
    srand((int)pLap);	//  <-- makes sure that we randomize the colours consistently, so that lap plots don't change colour from draw to draw...

    float dBestLength = -1;
    float dTimeToHighlight = -1;
    TimePoint2D ptBest;

    if(sfLapOpts.fDrawLines)
    {
      glLineWidth(2);	// Added by KDJ. Sets the width of the line to draw.
      glBegin(GL_LINE_STRIP);
    }
    else
    {
      glPointSize(4.0f);
      glBegin(GL_POINTS);
    }
	float r;
	float g;
	float b;
	MakeColor ( pLap, x == (lstLaps.size() - 1), &r, &g, &b ); // Function picks color to use and tells opengl to draw the following in the colour we just made up

	const vector<TimePoint2D>& lstPoints = pLap->GetPoints();
    for(int x = 0; x< lstPoints.size(); x++)
    {
      const TimePoint2D& p = lstPoints[x];
      glVertex2f(p.flX,p.flY);

      // if we're a highlight source, try to figure out the closest point for this lap
      if(m_pLapSupplier->IsHighlightSource(m_iSupplierId))
      {
        Vector2D vPt = V2D(p.flX,p.flY);
        Vector2D vDiff = vPt - vHighlight;
        if(vDiff.Length() < dBestLength || dBestLength < 0)
        {
          dBestLength = vDiff.Length();
          dTimeToHighlight = p.iTime;
          ptBest = p;
        }
      }
      else
      {
        int iTime = m_pLapSupplier->GetLapHighlightTime(pLap);
        if(abs(p.iTime - iTime) < dBestLength || dBestLength < 0)
        {
          dBestLength = abs(p.iTime - iTime);
          ptBest = p;
          dTimeToHighlight = iTime;
        }
      }
    }

    // for each lap, draw an indicator of the closest thing to the mouse
    if(!m_pLapSupplier->IsHighlightSource(m_iSupplierId))
    {
      // if we're not a source, use the given time to highlight
      dTimeToHighlight = m_pLapSupplier->GetLapHighlightTime(pLap);
    }
    else
    {
      m_pLapSupplier->SetLapHighlightTime(pLap, (int)dTimeToHighlight);
    }

    // project from unit-space (the map) to window-space so we know where we need to draw our highlight-box
    {
      GLdouble winx,winy,winz;
      gluProject(ptBest.flX, ptBest.flY, 0, rgModelviewMatrix, rgProjMatrix, rgViewport, &winx, &winy, &winz);
      MAPHIGHLIGHT mapPt;
      mapPt.pt.x = (int)winx;
      mapPt.pt.y = (int)winy;
      mapPt.pLap = pLap;

      lstMousePointsToDraw.push_back(mapPt);
    }
	glEnd();

  }
	  // draw the start-finish and segment lines
	  if(lstLaps.size() > 0 && sfLapOpts.fDrawSplitPoints)
	  {
		const CExtendedLap* pReferenceLap = lstLaps[lstLaps.size()-1];
		const StartFinish* pSF = pReferenceLap->GetLap()->GetSF();
		for(int x = 0;x < cSectors; x++)
		{
			if (sfLapOpts.m_SplitPoints[x].m_sfXPoint != 0.0f)
			{
			  Vector2D pt1 = pSF[x].GetPt1();
			  Vector2D pt2 = pSF[x].GetPt2();
			  glLineWidth(1);			// Added by KDJ. Skinny lines for Start/Finish.
			  glColor3d(1.0,0.0,0.0);	// Red for S/F line color
			  Vector2D vD;

			  for(int z = 0; z < pReferenceLap->m_lstPoints.size(); z++)
			  {
				  //	Find the point in the lap where the Split Point is and get the vector			
				  if (pt1.m_v[0] == pReferenceLap->m_lstPoints[z].flX && pt1.m_v[1] == pReferenceLap->m_lstPoints[z].flY)
				  {
					vD = V2D(pReferenceLap->m_lstPoints[z].flX, pReferenceLap->m_lstPoints[z].flY) - V2D(pReferenceLap->m_lstPoints[z+1].flX, pReferenceLap->m_lstPoints[z+1].flY);
					break;
				  }
			  }
			  //	We found our point and have the difference vector. Now let's rotate it 90 degrees.
			  Vector2D vPerp = vD.RotateAboutOrigin(PI/2);
			  //	Now let's create the vertices for plotting the split point line
			  pt2 = pt2 + vPerp;
			  pt1 = pt1 - vPerp;
			  glBegin(GL_LINE_STRIP);
			  glVertex2f(pt1.m_v[0], pt1.m_v[1]);
			  glVertex2f(pt2.m_v[0], pt2.m_v[1]);
			  glEnd();

			  glColor3d(1.0,0.0,0.0);
			  LPCSTR lpszText = "";
			  if(x == 0) lpszText = "S/F";	// Start/Finish Line
			  if(x == 1) lpszText = "S1";	// Segment 1
			  if(x == 2) lpszText = "S2";	// Segment 2
			  if(x == 3) lpszText = "S3";	// Segment 3
			  if(x == 4) lpszText = "S4";	// Segment 4
			  if(x == 5) lpszText = "S5";	// Segment 5
			  if(x == 6) lpszText = "S6";	// Segment 6
			  if(x == 7) lpszText = "S7";	// Segment 7
			  if(x == 8) lpszText = "S8";	// Segment 8
			  DrawText(pt2.m_v[0],pt2.m_v[1], lpszText);	//	Only draw text at one end of the line
			}
		}
	  }

	glPopMatrix(); // popping us out of map-coords space.

  if(lstMousePointsToDraw.size() > 0)
  {
    RECT rcClient;
    GetClientRect(OGL_GetHWnd(),&rcClient);

    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, RECT_WIDTH(&rcClient),0, RECT_HEIGHT(&rcClient),-1.0,1.0);

    for(int x = 0; x < lstMousePointsToDraw.size(); x++)
    {
      const CExtendedLap* pLap = lstMousePointsToDraw[x].pLap;
      const POINT& ptWindow = lstMousePointsToDraw[x].pt;
	  float r;
	  float g;
	  float b;
	  MakeColor ( pLap, x == (lstLaps.size() - 1), &r, &g, &b ); // Function picks color to use and tells opengl to draw the following in the colour we just made up
      
      // we also want to draw a highlighted square
      DrawGLFilledSquare(ptWindow.x, ptWindow.y, 5);
    }
    glPopMatrix(); // pop us from window space back to the identity
  }
}

// * Exponential moving average
// * n is number of samples
// * v is array of size n
// * alpha is between 0 and 1, low values give more weight to past values
// * out is the output array of size n
//

void CLapPainter::fExpMovingAvg( int n, vector<DataPoint>& lstPoints, double alpha, vector<DataPoint>& lstSmoothPts )
{
  lstSmoothPts[0] = lstPoints[0];
  for ( int i = 1; i < n; i++ )
  {
    lstSmoothPts[i].flValue = ( 1.0 - alpha ) * lstSmoothPts[i - 1].flValue + alpha * lstPoints[i].flValue;
  }
}

// * Moving average function
// * n is the number of input samples
// * v is an array of values of size n
// * w is the size of the window, taken on each side of sample
// * out is output array of size n
// *
//
void CLapPainter::fBoxMovingAvg( int n, vector<DataPoint>& lstPoints, int w, vector<DataPoint>& lstSmoothPts, bool bSmoothFlag )
{
  if (bSmoothFlag == true) return;
  int s;
  for(s=0; s < n; s++)
  {
    float t = 0.0;
	int aTemp = 0;
    for ( int a = s - w; a <= s + w; a++ )
    {
		if (a < 0) aTemp = 0; else if (a >= n) aTemp = n - 1; else aTemp = a;
		t += lstPoints[aTemp].flValue;
    }
	DataPoint tempDataPoint;
	tempDataPoint.flValue = (t / ( 2 * w + 1 ));
	tempDataPoint.iTimeMs = lstPoints[s].iTimeMs;
	lstSmoothPts.push_back( tempDataPoint );
  }
}
