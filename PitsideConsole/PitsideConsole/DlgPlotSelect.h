#pragma once

#include "ArtUI.h"
#include "ArtTools.h"
#include "LapReceiver.h"
#include "LapPainter.h"		//	Added as I think this will be needed to pull channels
#include "LapData.h"
#include "resource.h"
#include "PitsideConsole.h"


struct PLOTSELECT_RESULT
{
public:
  PLOTSELECT_RESULT()
  {
    iPlotId = -1;
    fCancelled = false;
  }
  int iPlotId;
  bool fCancelled;
};

static struct PlotPrefs 
{
	ArtListBox m_ArtListBox;
	int iPlotView;
	float fMinValue;
	float fMaxValue;
} m_PlotPrefs [20];

class CPlotSelectDlg : public IUI
{

    bool fCancelled;
	int x;

	public:
//	void InitPlotPrefs (PlotPrefs& m_PlotPrefs, int x);
	void GetDataChannelName(DATA_CHANNEL eDC, LPTSTR lpszName, int cch, set<DATA_CHANNEL> setAvailable);
//    void InitAxes(set<DATA_CHANNEL> setAvailable);
	void GetPlotPrefs (PlotPrefs, DATA_CHANNEL eDC, int x);

	CPlotSelectDlg(ILapReceiver* pLapDB, PLOTSELECT_RESULT* pResults) : m_pResults(pResults)
	{
	};
	virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
	virtual LRESULT DlgPlot(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual DWORD GetDlgId() const {return IDD_PLOTPREFS;}
	private:
		PLOTSELECT_RESULT* m_pResults;
		ILapReceiver* m_pLapDB;
		ArtListBox sfListBox;
};
#pragma once


// DlgPlotSelect dialog
/*
class DlgPlotSelect : public CDialogEx
{
	DECLARE_DYNAMIC(DlgPlotSelect)

public:
	DlgPlotSelect(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgPlotSelect();

// Dialog Data
	enum { IDD = IDD_PLOTPREFS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnStnClickedPlottypeChannel1();
	afx_msg void OnBnClickedPlottypeGraph1();
	afx_msg void OnBnClickedPlottypeValue7();
	afx_msg void OnBnClickedPlottypeValue1();
};
*/