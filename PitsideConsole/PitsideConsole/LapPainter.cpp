#include "stdafx.h"
#include "LapPainter.h"
#include "LapData.h"
#include "ArtUI.h"

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
  switch(eDisplayStyle)
  {
  case LAPDISPLAYSTYLE_MAP:
    glViewport(0,0,RECT_WIDTH(&rcClient), RECT_HEIGHT(&rcClient));
    DrawLapLines(sfLapOpts);	//	Draws the lap as a map on primary display
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
  map<DATA_CHANNEL,float> mapMinY;
  map<DATA_CHANNEL,float> mapMaxY;
  float dMaxX = -1e30;
  float dMinX = 1e30;
  { // figuring out bounds and getting matrices all set up
    //	First lets load up all of the data into an array and determine its size
    for(int x = 0;x < lstLaps.size(); x++)
    {
      CExtendedLap* pLap = lstLaps[x];
      DATA_CHANNEL eDataX = m_pLapSupplier->GetXChannel();
      const IDataChannel* pDataX = pLap->GetChannel(eDataX);
      if(!pDataX || !pDataX->IsValid() || pDataX->GetData().size() <= 0) continue;

      vector<DATA_CHANNEL> lstDataY = m_pLapSupplier->GetYChannels();
      for(int y = 0; y < lstDataY.size(); y++)	//	Loop through the data channels and display them
      {
        int ValueDisplay = 0;	//	Flag if we should display this channel as a graph or not 0 = yes, 1 = no
		const IDataChannel* pChannel = pLap->GetChannel(lstDataY[y]);
        if(!pChannel || !pChannel->IsValid()) continue;

        const DATA_CHANNEL eType = lstDataY[y];

		//	Determine if this Data Channel is one that we only want to display the values for
			for (int u=0;u<49;u++)
			{
				if (eType == m_pLapSupplier->GetDisplayOptions().m_PlotPrefs[u].iDataChannel && m_pLapSupplier->GetDisplayOptions().m_PlotPrefs[u].iPlotView == false)
				{	//	We have found a display only channel. Let's prevent the graph from displaying
					ValueDisplay = 1;
					break;
				}
			}
      if(mapMinY.find(eType) == mapMinY.end())
        {
          mapMinY[eType] = min(pChannel->GetMin(),m_pLapSupplier->GetDataHardcodedMin(eType));
          mapMaxY[eType] = max(pChannel->GetMax(),m_pLapSupplier->GetDataHardcodedMax(eType));
        }
        else
        {
          mapMinY[eType] = min(pChannel->GetMin(),mapMinY[eType]);
          mapMaxY[eType] = max(pChannel->GetMax(),mapMaxY[eType]);
        }
        
        if (ValueDisplay == 0)
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
    }
  }
	 
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

  // y-channel graphing loop start
  for(set<DATA_CHANNEL>::iterator i = setY.begin(); i != setY.end(); i++)
  {
    vector<HIGHLIGHTDATA> lstMousePointsToDraw;
    glViewport(rcSpot.left,rcSpot.top,RECT_WIDTH(&rcSpot),RECT_HEIGHT(&rcSpot));

    // now we have the bounds of all the laps we've looked at, so let's draw them
    glPushMatrix();
    glLoadIdentity();
    glScalef(1.0f, 0.90f, 1.0f);	// Let's scale it so that graphs don't touch each other.
    glOrtho(dMinX, dMaxX,mapMinY[*i], mapMaxY[*i],-1.0,1.0);

    // draw horizontal guide lines and text on the background. Yes this should probably go into a function, Art ;)
    // first draw the starting guideline
	{	
		float flLine = m_pLapSupplier->GetGuideStart(*i, mapMinY[*i], mapMaxY[*i]);
		LineColor();	//	Pick guideline color, based upon chosen color scheme
		glLineWidth(1);			// Added by KDJ. Skinny lines for guidelines.
		glBegin(GL_LINE_STRIP);
		glVertex2f(dMinX,flLine);
		glVertex2f(dMaxX,flLine);
		glEnd();

		LineColor();	//	Pick guideline color, based upon chosen color scheme
		char szText[256];
		GetChannelString(*i, sfLapOpts.eUnitPreference, flLine, szText, NUMCHARS(szText));
		DrawText(dMinX, flLine, szText);
	}
	// now draw the rest of them
	for(float flLine = m_pLapSupplier->GetGuideStart(*i, mapMinY[*i], mapMaxY[*i]) + m_pLapSupplier->GetGuideStep(*i, mapMinY[*i], mapMaxY[*i]); flLine < mapMaxY[*i]; flLine += m_pLapSupplier->GetGuideStep(*i, mapMinY[*i], mapMaxY[*i]))
    {
	  LineColor();	//	Pick guideline color, based upon chosen color scheme
      glLineWidth(1);			// Added by KDJ. Skinny lines for guidelines.
      glBegin(GL_LINE_STRIP);
      glVertex2f(dMinX,flLine);
      glVertex2f(dMaxX,flLine);
      glEnd();
      
	  LineColor();	//	Pick guideline color, based upon chosen color scheme
      char szText[256];
      GetChannelString(*i, sfLapOpts.eUnitPreference, flLine, szText, NUMCHARS(szText));
      DrawText(dMinX, flLine, szText);
    }


	// draw vertical guide lines and text on the background only if at 1.0X magnification
	if (sfLapOpts.iZoomLevels != 0)
	{
	}
	else
	{
		// first draw the starting guideline
		{
			float flLine = m_pLapSupplier->GetGuideStartX(eX, dMinX, dMaxX);
			LineColor();	//	Pick guideline color, based upon chosen color scheme
			glLineWidth(1);      
			glBegin(GL_LINE_STRIP);
			glVertex3f(flLine,mapMinY[*i],0);
			glVertex3f(flLine,mapMaxY[*i],0);
			glEnd();
			LineColor();	//	Pick guideline color, based upon chosen color scheme
			char szText[256];
			GetChannelString(eX, sfLapOpts.eUnitPreference, flLine, szText, NUMCHARS(szText));
			DrawText(flLine, mapMinY[*i]-12, szText);
		}
	// now draw the rest of them
		for(float flLine = m_pLapSupplier->GetGuideStartX(eX, dMinX, dMaxX) + m_pLapSupplier->GetGuideStepX(eX, dMinX, dMaxX); flLine < dMaxX; flLine += m_pLapSupplier->GetGuideStepX(eX, dMinX, dMaxX))
		{
			LineColor();	//	Pick guideline color, based upon chosen color scheme
			glLineWidth(1);      
			glBegin(GL_LINE_STRIP);
			glVertex3f(flLine,mapMinY[*i],0);
			glVertex3f(flLine,mapMaxY[*i],0);
			glEnd();

			LineColor();	//	Pick guideline color, based upon chosen color scheme
			char szText[256];
			GetChannelString(eX, sfLapOpts.eUnitPreference, flLine, szText, NUMCHARS(szText));
		
			DrawText(flLine, mapMinY[*i]-12, szText);
		}
	}



//		Set up the non-zoomed/panned view for the map
    GLdouble rgModelviewMatrix[16];
    GLdouble rgProjMatrix[16];
    GLint rgViewport[4];
    
	{
//	Now that the matrices are correct, let's graph them.    
		glGetDoublev(GL_MODELVIEW_MATRIX, rgModelviewMatrix);
		glGetDoublev(GL_PROJECTION_MATRIX, rgProjMatrix);
		glGetIntegerv(GL_VIEWPORT, rgViewport);

		POINT ptMouse;
		if(GetMouse(&ptMouse) && m_pLapSupplier->IsHighlightSource(m_iSupplierId))
		{
		//		The mouse is in our window, so panning and zooming are active!
			const double dCenterX = (dMinX + dMaxX)/2;		//	Center of X for scaling transformation
			double dScaleAmt = pow(1.08,sfLapOpts.iZoomLevels);
			GLdouble dXShift,dYShift,dZShift;
		//		Project the window shift stuff so we know how far to translate the view
			dYShift = 0;	// No Y shift or zoom for Map Plot.
			gluUnProject(sfLapOpts.flWindowShiftX/dScaleAmt,0,0,rgModelviewMatrix,rgProjMatrix,rgViewport,&dXShift,&dYShift,&dZShift);
		//		Set up to perform the ZOOM function for DATA PLOT.   
		double dTranslateShiftX;
		dTranslateShiftX= dCenterX;
			glTranslated(dTranslateShiftX,0,0);	// Translate the map to origin on x-axis only
			glScaled(dScaleAmt,1.0,1.0);	//	No scaling of Y-axis on Data Plot.
			glTranslated(-dTranslateShiftX,0,0);	// Now put the map back in its place
		//	Panning functionality
			glTranslated(dXShift-dMinX,0,0);	//	Offset for this is still slight wrong, but the best for now.

		//		Now having shifted, let's get our new model matrices
		  glGetDoublev(GL_MODELVIEW_MATRIX, rgModelviewMatrix);
		  glGetDoublev(GL_PROJECTION_MATRIX, rgProjMatrix);
		  glGetIntegerv(GL_VIEWPORT, rgViewport);
		}
	}

    Vector2D ptHighlight; // the (x,y) coords in unit-space that we want to highlight.  Example: for a speed-distance graph, x would be in distance units, y in velocities.
    POINT ptMouse;
    if(GetMouse(&ptMouse) && m_pLapSupplier->IsHighlightSource(m_iSupplierId))
    {
      //		The mouse is in our window... we make our own highlighter, ignoring anything that got sent to us
      GLdouble dX,dY,dZ;
      gluUnProject(ptMouse.x,ptMouse.y,0,rgModelviewMatrix,rgProjMatrix,rgViewport,&dX,&dY,&dZ);
      ptHighlight = V2D(dX,0);
    }
    for(int x = 0; x < lstLaps.size(); x++)
    {
      CExtendedLap* pLap = lstLaps[x];
      const IDataChannel* pDataX = pLap->GetChannel(m_pLapSupplier->GetXChannel());
      const IDataChannel* pDataY = pLap->GetChannel(*i);

	  if(pDataX && pDataY)
	  {
		  // tracking what we want to highlight
        float dBestLength = -1;
        float dTimeToHighlight = -1;
        const vector<DataPoint>& lstPointsX = pDataX->GetData();
        const vector<DataPoint>& lstPointsY = pDataY->GetData();
//        srand((int)pLap);	//  <-- makes sure that we randomize the colours consistently, so that lap plots don't change colour from draw to draw...
		float r;
		float g;
		float b;
		MakeColor ( pLap, &r, &g, &b ); // Function picks color to use and tells opengl to draw the following in the colour we just made up
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

        vector<DataPoint>::const_iterator iX = lstPointsX.begin();
        vector<DataPoint>::const_iterator iY = lstPointsY.begin();
        while(iX != lstPointsX.end() && iY != lstPointsY.end())
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
		    glEnd();
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
	  /*  <-- tells OpenGL that it should show us the part of the openGL "world" that corresponds to 
	  (0...window width, 0 ... window height).  This completes the "hey opengl, just draw where we 
	  tell you to plz" part of the function */

      for(int x = 0; x < lstMousePointsToDraw.size(); x++)	// <-- loops through all the stupid boxes/lines we want to draw
      {
        const CExtendedLap* pLap = lstMousePointsToDraw[x].m_pLap;	//  <-- gets the lap data we want to draw
        const POINT& ptWindow = lstMousePointsToDraw[x].m_ptWindow;	// <-- gets info about where in the window we want to draw the box
        const IDataChannel* pDataX = lstMousePointsToDraw[x].m_pDataX;	//  <-- gets the x channel data
        const IDataChannel* pDataY = lstMousePointsToDraw[x].m_pDataY;	// <-- gets the y channel data

		float r;
		float g;
		float b;
		MakeColor ( pLap, &r, &g, &b ); // Function picks color to use and tells opengl to draw the following in the colour we just made up

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
			GetChannelString(lstMousePointsToDraw[x].m_eChannelY, sfLapOpts.eUnitPreference, pDataY->GetValue(dTimeToHighlight), szYString, NUMCHARS(szYString));
			// <-- gets the actual unit string for the data channel.  For speed, this might be "100.0km/h"

			char szXString[256];
			GetChannelString(eX, sfLapOpts.eUnitPreference, pDataX->GetValue(dTimeToHighlight), szXString, NUMCHARS(szXString));
			// <-- same for x channel

			char szText[256];
			sprintf(szText, "%S - (%S @ %S) %s @ %s", szLapName, szTypeY, szTypeX, szYString, szXString);

			DrawText(100.0,(x+1)*GetWindowFontSize(),szText);	// <-- draws the text from the bottom of the window, working upwards

			// we also want to draw a highlighted square
			DrawGLFilledSquare(ptWindow.x, ptWindow.y, 3);	// <-- draws the stupid little box at ptWindow.x.
			// we also want to draw a highlighted LINE for that individual lap/graph combination
			glLineWidth(1);								// Added by KDJ. Skinny line for Distance markers.
			glBegin(GL_LINE_STRIP);						// Added by KDJ
			glVertex3f(ptWindow.x, 0, 0);				// Added by KDJ, modified by Chas
			glVertex3f(ptWindow.x,rcSpot.bottom,0);		// Added by KDJ
		}
			glEnd();									// Added by KDJ
	  }
      glPopMatrix();
      glPopMatrix();	//	Should there be two of these here?
    }
    rcSpot.top += iSegmentHeight;
    rcSpot.bottom += iSegmentHeight;
  } // end y-channel data channel loop

	
}
/*
void CLapPainter::MagicDeterminingFunction(const LAPSUPPLIEROPTIONS& sfLapOpts, bool fHighlightXAxis)
{
      for(int x = 0; x < lstMousePointsToDraw.size(); x++)	// <-- loops through all the stupid boxes/lines we want to draw
      {
        const CExtendedLap* pLap = lstMousePointsToDraw[x].m_pLap;	//  <-- gets the lap data we want to draw
        const POINT& ptWindow = lstMousePointsToDraw[x].m_ptWindow;	// <-- gets info about where in the window we want to draw the box
        const IDataChannel* pDataX = lstMousePointsToDraw[x].m_pDataX;	//  <-- gets the x channel data
        const IDataChannel* pDataY = lstMousePointsToDraw[x].m_pDataY;	// <-- gets the y channel data

		float r;
		float g;
		float b;
		MakeColor ( pLap, &r, &g, &b ); // Function picks color to use and tells opengl to draw the following in the colour we just made up

		// if we're the main screen, we want to draw some text data for each point
        TCHAR szLapName[256];
        pLap->GetString(szLapName, NUMCHARS(szLapName));	// <-- gets the string "10:11:12 - 1:40.59 - Keith", aka the "lap name"

        float dTimeToHighlight = m_pLapSupplier->GetLapHighlightTime(pLap);	//  <-- asks the ILapSupplier interface what we should highlight

        TCHAR szTypeX[256];
        ::GetDataChannelName(eX,szTypeX,NUMCHARS(szTypeX));	// <-- converts the data channel type into a string, like "Oil Temperature"

        TCHAR szTypeY[256];
        ::GetDataChannelName(lstMousePointsToDraw[x].m_eChannelY, szTypeY, NUMCHARS(szTypeY));	// <-- converts the y channel into a string

        char szYString[256];
        GetChannelString(lstMousePointsToDraw[x].m_eChannelY, sfLapOpts.eUnitPreference, pDataY->GetValue(dTimeToHighlight), szYString, NUMCHARS(szYString));
		// <-- gets the actual unit string for the data channel.  For speed, this might be "100.0km/h"

        char szXString[256];
        GetChannelString(eX, sfLapOpts.eUnitPreference, pDataX->GetValue(dTimeToHighlight), szXString, NUMCHARS(szXString));
		// <-- same for x channel

        char szText[256];
        sprintf(szText, "%S - (%S @ %S) %s @ %s", szLapName, szTypeY, szTypeX, szYString, szXString);

        DrawText(100.0,(x+1)*GetWindowFontSize(),szText);	// <-- draws the text from the bottom of the window, working upwards

        // we also want to draw a highlighted square
//        DrawGLFilledSquare(ptWindow.x, ptWindow.y, 5);	// <-- draws the stupid little box at ptWindow.x. Commented out by KDJ
        // we also want to draw a highlighted LINE for that individual lap/graph combination
				glLineWidth(1);								// Added by KDJ. Skinny line for Distance markers.
				glBegin(GL_LINE_STRIP);						// Added by KDJ
				glVertex3f(ptWindow.x, 0, 0);				// Added by KDJ, modified by Chas
				glVertex3f(ptWindow.x,rcSpot.bottom,0);		// Added by KDJ
				glEnd();									// Added by KDJ
	  }
}
*/

struct MAPHIGHLIGHT
{
  const CExtendedLap* pLap;
  POINT pt;
};

// Function for setting highlighting color, making sure that there is enough contrast to a black background
void CLapPainter::MakeColor(const CExtendedLap* pLap, float* pR, float* pG, float* pB) 
{ 
	srand((int)pLap);	//  <-- makes sure that we randomize the colours consistently, so that lap plots don't change colour from draw to draw... 
	if (m_pLapSupplier->GetDisplayOptions().fColorScheme)	//	Background color is black, make sure there is enough contrast with the lines
	{
		do 
		{ 
			*pR = RandDouble(); 
			*pG = RandDouble(); 
			*pB = RandDouble(); 
		} 
		while(*pR + *pG + *pB < 0.5); 
		glColor3d( *pR, *pG, *pB ); // Final color to use.  Tells opengl to draw the following in the colour we just made up
	}
	else
	{
		do 
		{ 
			*pR = RandDouble(); 
			*pG = RandDouble(); 
			*pB = RandDouble(); 
		} 
		while(*pR + *pG + *pB > 2.5); 
		glColor3d( *pR, *pG, *pB ); // Final color to use.  Tells opengl to draw the following in the colour we just made up
	}
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

void CLapPainter::DrawLapLines(const LAPSUPPLIEROPTIONS& sfLapOpts)
{
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
  // we have now determined the bounds of the thing we're going to draw, let's draw it
  glPushMatrix();
  glLoadIdentity();
  glScalef(1.0f, 1.0f, 1.0f);	// Scale it so that it fills up the window.
  glOrtho(rcAllLaps.left, rcAllLaps.right,rcAllLaps.bottom,rcAllLaps.top, -1.0,1.0);
  
  //		Set up the non-zoomed/panned view for the map
  GLdouble rgModelviewMatrix[16];
  GLdouble rgProjMatrix[16];
  GLint rgViewport[4];

  {
	//	Now that the matrices are correct, let's graph them.    
	glGetDoublev(GL_MODELVIEW_MATRIX, rgModelviewMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, rgProjMatrix);
    glGetIntegerv(GL_VIEWPORT, rgViewport);
  
    const double dCenterX = (rcAllLaps.left + rcAllLaps.right)/2;
    const double dCenterY = (rcAllLaps.top + rcAllLaps.bottom)/2;
    double dScaleAmt = pow(1.06,sfLapOpts.iZoomLevels);

	POINT ptMouse;
	if(GetMouse(&ptMouse) && m_pLapSupplier->IsHighlightSource(m_iSupplierId))
	{
		// the mouse is in our window, so let's enable panning and zooming!
		const double dTranslateShiftX = (rcAllLaps.left + rcAllLaps.right)/2;
		const double dTranslateShiftY = (rcAllLaps.top + rcAllLaps.bottom)/2;
		double dScaleAmt = pow(1.06,sfLapOpts.iZoomLevels);
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
  vector<CExtendedLap*> lstLaps = m_pLapSupplier->GetLapsToShow();
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
	MakeColor ( pLap, &r, &g, &b ); // Function picks color to use and tells opengl to draw the following in the colour we just made up

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
//  if (pLap == m_pReferenceLap)	// If this lap is the reference lap, draw the segment lines
//  {
	  // draw the start-finish and segment lines
	  if(lstLaps.size() > 0)
	  {
		const CExtendedLap* pReferenceLap = lstLaps[lstLaps.size()-1];
		const StartFinish* pSF = pReferenceLap->GetLap()->GetSF();
		for(int x = 0;x < 3; x++)
		{
		  Vector2D pt1 = pSF[x].GetPt1();
		  Vector2D pt2 = pSF[x].GetPt2();
		  glLineWidth(1);			// Added by KDJ. Skinny lines for Start/Finish.
		  glBegin(GL_LINE_STRIP);
		  glColor3d(1.0,0.0,0.0);	// Red for S/F line color
		  glVertex2f(pt1.m_v[0],pt1.m_v[1]);
		  glVertex2f(pt2.m_v[0],pt2.m_v[1]);
		  glEnd();

		  glColor3d(1.0,0.0,0.0);
		  LPCSTR lpszText = "";
		  if(x == 0) lpszText = "S1";	// Segment 1
		  if(x == 1) lpszText = "S2";	// Segment 2
		  if(x == 2) lpszText = "S/F";	// Segment 3, Start/Finish Line
		  DrawText(pt1.m_v[0],pt1.m_v[1], lpszText);	//	Need to add offsets to these for them to be on the screen
		  DrawText(pt2.m_v[0],pt2.m_v[1], lpszText);	//	Need to add offsets to these for them to be on the screen
		}
	  }
//  }

	glPopMatrix(); // popping us out of map-coords space.

/*
	//	The idea here is to get the mouse location and find the closest reference lap point to set the sector location
	POINT ptMouse;
	Vector2D vHighlight;
	if(GetMouse(&ptMouse) && m_pLapSupplier->IsHighlightSource(m_iSupplierId))
	{
		// the mouse is in our window... we make our own highlighter
		GLdouble dX,dY,dZ;

		gluUnProject(ptMouse.x,ptMouse.y,0,rgModelviewMatrix,rgProjMatrix,rgViewport,&dX,&dY,&dZ);
		vHighlight = V2D((float)dX,(float)dY);
	}

    float dBestLength = -1;
    float dTimeToHighlight = -1;
    TimePoint2D ptBest;

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
*/

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
	  MakeColor ( pLap, &r, &g, &b ); // Function picks color to use and tells opengl to draw the following in the colour we just made up
      
      // we also want to draw a highlighted square
      DrawGLFilledSquare(ptWindow.x, ptWindow.y, 5);
    }
    glPopMatrix(); // pop us from window space back to the identity
  }
}