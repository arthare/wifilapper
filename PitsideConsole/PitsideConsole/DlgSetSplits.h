#pragma once

#include "ArtUI.h"
#include "ArtTools.h"
#include "resource.h"
#include "LapPainter.h"
#include "PitsideConsole.h"
//#include "LapPainter.h"

struct SETSPLITSDLG_RESULT
{
public:
  SETSPLITSDLG_RESULT()
  {
    fCancelled = true;
  }
  bool fCancelled;
};

class CSetSplitsDlg : public IUI//, public ILapSupplier
{
public:
  CSetSplitsDlg(ILapReceiver* pLapDB, CExtendedLap* pLap, SETSPLITSDLG_RESULT* pResults, int iRaceId, LAPSUPPLIEROPTIONS* i_sfLapOpts, CLapPainter* sfRefLapPainter) : m_pLap(pLap), m_pResults(pResults), m_iRaceId(iRaceId), m_sfLapOpts(i_sfLapOpts), m_sfRefLapPainter(sfRefLapPainter)
//  CSetSplitsDlg(ILapReceiver* pLapDB, SETSPLITSDLG_RESULT* pResults, int iRaceId, LAPSUPPLIEROPTIONS* i_sfLapOpts) : m_pResults(pResults), m_iRaceId(iRaceId), m_sfLapOpts(i_sfLapOpts)
  {
		m_pLapDB = pLapDB;
		m_pLap = pLap;
  };

  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_SETSPLITPOINTS;}
private:
	SETSPLITSDLG_RESULT* m_pResults;
	int m_iRaceId;
	ILapReceiver* m_pLapDB;
	LAPSUPPLIEROPTIONS* m_sfLapOpts;
	CExtendedLap* m_pLap;
	CLapPainter* m_sfRefLapPainter;
	ILapSupplier* m_sfSectorDisplay;

//	ILapSupplier* m_pLapSupplier;
    int m_iSupplierId;
  HDC OGL_GetDC() {return m_hdc;}
  HGLRC OGL_GetRC() {return m_hRC;}
  HWND OGL_GetHWnd() const {return m_hWnd;}
  HDC m_hdc;
  HWND m_hWnd;
  HGLRC m_hRC;

};
