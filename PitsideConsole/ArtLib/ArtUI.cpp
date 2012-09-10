#include "stdafx.h"
#include "artui.h"
#include "gl/gl.h"
#include "CommDlg.h"
#include <map>
using namespace std;

GLvoid DrawGLText(GLuint base, float x, float y, const char* fmt, ... ); // note: this is a private function.  People wanting to call it should call ArtOpenGLWindow::DrawText from within their window

void DrawGLFilledSquare(double dX, double dY, double dRadius)
{
  glBegin(GL_POLYGON);

  glVertex2d(dX-dRadius,dY-dRadius);
  glVertex2d(dX-dRadius,dY+dRadius);
  glVertex2d(dX+dRadius,dY+dRadius);
  glVertex2d(dX+dRadius,dY-dRadius);

  glEnd();
}

void ArtOpenGLWindow::DrawText(float flX, float flY, const char* pText) const
{
  DrawGLText(m_uFontBase, flX, flY, pText);
}


HRESULT InitGLFont(GLuint* pBase, int iFontSize, HDC hDC)								// Build Our Bitmap Font
{
  HRESULT hr = S_OK;

	HFONT	font;										// Windows Font ID
	HFONT	oldfont;									// Used For Good House Keeping

	GLuint base = glGenLists(96);								// Storage For 96 Characters

	font = CreateFont(	-iFontSize,							// Height Of Font
						0,								// Width Of Font
						0,								// Angle Of Escapement
						0,								// Orientation Angle
						FW_BOLD,						// Font Weight
						FALSE,							// Italic
						FALSE,							// Underline
						FALSE,							// Strikeout
						ANSI_CHARSET,					// Character Set Identifier
						OUT_TT_PRECIS,					// Output Precision
						CLIP_DEFAULT_PRECIS,			// Clipping Precision
						ANTIALIASED_QUALITY,			// Output Quality
						FF_DONTCARE|DEFAULT_PITCH,		// Family And Pitch
						L"Verdana");					// Font Name

	oldfont = (HFONT)SelectObject(hDC, font);           // Selects The Font We Want
	BOOL fSuccess = wglUseFontBitmaps(hDC, 32, 96, base);				// Builds 96 Characters Starting At Character 32
  hr = fSuccess ? S_OK : HRESULT_FROM_WIN32(GetLastError());
	SelectObject(hDC, oldfont);							// Selects The Font We Want
	DeleteObject(font);									// Delete The Font

  if(SUCCEEDED(hr))
  {
    *pBase = base;
  }
  else
  {
    *pBase = 0;
  }

  return hr;
}

GLvoid DrawGLText(GLuint base, float x, float y, const char* fmt, ... )					// Custom GL "Print" Routine
{

	char		text[256];								// Holds Our String
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing

	va_start(ap, fmt);									// Parses The String For Variables
	    vsprintf(text, fmt, ap);						// And Converts Symbols To Actual Numbers
	va_end(ap);											// Results Are Stored In Text

	// Position The Text On The Screen
	glRasterPos2f(x, y);

	glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
	glListBase(base - 32);								// Sets The Base Character to 32
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);	// Draws The Display List Text
	glPopAttrib();										// Pops The Display List Bits
}

void KillFont(GLuint base)									// Delete The Font List
{
	glDeleteLists(base, 96);							// Delete All 96 Characters
}

// shows the open file dialog, sticks result in szPath...
// true -> path is good
// false -> was cancelled
bool ArtGetOpenFileName(HWND hWndOwner, LPCTSTR lpszTitle, LPTSTR lpszPath, int cchPath, LPCTSTR lpszFilter)
{
  lpszPath[0] = '\0';
  OPENFILENAME ofn;
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hWndOwner;
  ofn.hInstance = NULL;
  ofn.lpstrFilter = lpszFilter;
  ofn.lpstrCustomFilter = NULL;
  ofn.nMaxCustFilter = 0;
  ofn.nFilterIndex = 0;
  ofn.lpstrFile = lpszPath;
  ofn.nMaxFile = cchPath;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrTitle = lpszTitle;
  ofn.Flags = 0;
  ofn.nFileOffset = 0;
  ofn.nFileExtension = 0;
  ofn.lpstrDefExt = NULL;
  ofn.lpfnHook = NULL;
  ofn.lpTemplateName = NULL;
  ofn.pvReserved = 0;
  ofn.dwReserved = 0;
  ofn.FlagsEx = 0;
  BOOL fSuccess =  GetOpenFileName(&ofn);
  return fSuccess;
}
bool ArtGetSaveFileName(HWND hWndOwner, LPCTSTR lpszTitle, LPTSTR lpszPath, int cchPath, LPCTSTR lpszFilter)
{
  lpszPath[0] = '\0';
  OPENFILENAME ofn;
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hWndOwner;
  ofn.hInstance = NULL;
  ofn.lpstrFilter = lpszFilter;
  ofn.lpstrCustomFilter = NULL;
  ofn.nMaxCustFilter = 0;
  ofn.nFilterIndex = 0;
  ofn.lpstrFile = lpszPath;
  ofn.nMaxFile = cchPath;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrTitle = lpszTitle;
  ofn.Flags = 0;
  ofn.nFileOffset = 0;
  ofn.nFileExtension = 0;
  ofn.lpstrDefExt = NULL;
  ofn.lpfnHook = NULL;
  ofn.lpTemplateName = NULL;
  ofn.pvReserved = 0;
  ofn.dwReserved = 0;
  ofn.FlagsEx = 0;
  BOOL fSuccess =  GetSaveFileName(&ofn);
  return fSuccess;
}
map<int,IUI*> mapDialogs;