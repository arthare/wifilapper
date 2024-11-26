// PitsideConsole.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "pitsideconsole.h"
#include "LapReceiver.h"
#include <vector>
#include "AutoCS.h"
#include <string>
#include <sstream>
//#include "sdl.h"
#include "resource.h"
#include "ArtUI.h"
#include "ArtTools.h"
#include "Windowsx.h"
#include <map>
#include <algorithm>
#include "ArtVector.h"
#include "LapPainter.h"
#include "LapData.h"
#include "DlgMessage.h"
#include "DlgRaceSelect.h"
#include "DlgRaceSelectEdit.h"
#include "DlgPlotSelect.h"	//	Added by KDJ for Preferences menu
#include "Iphlpapi.h"
#include "ArtSQL/ArtSQLite.h"
#include <stdio.h>
#include "DashWare.h"
#include "Multicast.h"
#include "PitsideHTTP.h"
#include "SQLiteLapDB.h"
#include "UnitTests.h"
#include <fstream>
#include "Winuser.h"
#include "Hyperlinks.h"
#include "DlgAbout.h"
#include <Winspool.h>
#include "DlgProgress.h"
#include "DlgWarning.h"
#include "DlgSetSplits.h"
#include "jpge.h"
#include <CommCtrl.h>	//	For Listview sorting routines
#include "DlgSelectSessions.h"
#include "DlgTimingScoring.h"
#include "LapReceiver.h"

//#pragma comment(lib,"sdl.lib")
using namespace std;

ILapReceiver* g_pLapDB = NULL;

SimpleHTTPServer* g_pHTTPServer = NULL;

struct COMPUTERDESC
{
public:
  char szDesc[100]; // computer name
};

class MCResponder : public MulticastResponseGenerator
{
public:
  virtual void GetResponse(const char* pbReceived, int cbReceived, char** ppbResponse, int* pcbResponse) override
  {
    COMPUTERDESC* pResp = new COMPUTERDESC();

    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH +1] = L"";
    DWORD cchComputerName = MAX_COMPUTERNAME_LENGTH +1;
    BOOL fSuccess = GetComputerName(szComputerName, &cchComputerName);
    
    if(fSuccess)
    {
      sprintf(pResp->szDesc,"%S",szComputerName);
    }
    else
    {
      sprintf(pResp->szDesc,"Unknown Computer");
    }
    *ppbResponse = (char*)pResp;
    *pcbResponse = sizeof(*pResp);
  }
};

bool CLap_SortByTime(const ILap* p1, const ILap* p2)
{
  return p1->GetStartTime() < p2->GetStartTime();
}

// this object takes the laps received on the net thread, stores them, and notifies the UI of the new laps
class CLapReceiver : public ILapReceiver
{
public:
  CLapReceiver(IUI* pUI) : m_pUI(pUI) 
  { 
    for(int x = 0; x < NETSTATUS_COUNT; x++)
    {
      szLastNetStatus[x][0] = '\0';
    }
  }
  virtual ~CLapReceiver() {};

  void Clear() override
  {
    AutoLeaveCS _cs(&m_cs);

    for(int x = 0;x < m_lstLaps.size(); x++)
    {
      delete m_lstLaps[x];
    }
    m_lstLaps.clear();

    ChannelMap::iterator i = m_mapChannels.begin();
    while(i != m_mapChannels.end())
    {
      map<DATA_CHANNEL,const IDataChannel*>::iterator i2 = i->second.begin();
      while(i2 != i->second.end())
      {
        FreeDataChannel((IDataChannel*)(i2->second));
        i2++;
      }
      i++;
    }
    m_mapChannels.clear();
    m_pUI->NotifyChange(NOTIFY_NEWDATA,(LPARAM)this);
  }

  void AddLap(const ILap* pLap, int iRaceId) override
  {
    {
      AutoLeaveCS _cs(&m_cs);
      m_lstLaps.push_back(pLap);
    }
  m_pUI->NotifyChange(NOTIFY_NEWLAP,(LPARAM)this);
  }
  void AddDataChannel(const IDataChannel* pDataChannel) override
  {
    DASSERT(pDataChannel->IsLocked());

    bool fFoundHome = false;
    {
      AutoLeaveCS _cs(&m_cs);
      map<DATA_CHANNEL,const IDataChannel*>& mapChannels = m_mapChannels[pDataChannel->GetLapId()];
      map<DATA_CHANNEL,const IDataChannel*>::iterator i = mapChannels.find(pDataChannel->GetChannelType());
      if(i != mapChannels.end())
      {
        // we already had one.  The correct thing to do would be to free it.
        IDataChannel* pChannel = const_cast<IDataChannel*>(i->second);
      }

      mapChannels[pDataChannel->GetChannelType()] = pDataChannel;
    }

    m_pUI->NotifyChange(NOTIFY_NEWDATA,(LPARAM)this);
  }
	ILap* AllocateLap(bool fMemory) override
	{
		return new CMemoryLap();
	}
	IDataChannel* AllocateDataChannel() const override
	{
		return new CDataChannel();
	}
	void FreeDataChannel(IDataChannel* pInput) const override
	{
		delete pInput;
	}
  void SetNetStatus(NETSTATUSSTRING eString, LPCTSTR sz) override
  {
    wcscpy(szLastNetStatus[eString], sz);
    m_pUI->NotifyChange(NOTIFY_NEWNETSTATUS,(LPARAM)this);
  }
  void NotifyDBArrival(LPCTSTR szPath)
  {
    wcscpy(szLastNetStatus[NETSTATUS_DB],szPath);
    m_pUI->NotifyChange(NOTIFY_NEWDATABASE,(LPARAM)szLastNetStatus[NETSTATUS_DB]);
  }
  LPCTSTR GetNetStatus(NETSTATUSSTRING eString) const 
  {
    return szLastNetStatus[eString];
  }
  virtual vector<const ILap*> GetLaps(int iRaceId) override
  {
    AutoLeaveCS _cs(&m_cs);
    vector<const ILap*> ret;
    for(int x = 0;x < m_lstLaps.size();x++)
    {
      ret.push_back(m_lstLaps[x]);
    }
    return ret;
  }
  virtual const ILap* GetLap(int iLapId) override
  {
    AutoLeaveCS _cs(&m_cs);
    for(int x = 0; x < m_lstLaps.size(); x++)
    {
      if(m_lstLaps[x]->GetLapId() == iLapId)
      {
        return m_lstLaps[x];
      }
    }
    return NULL;
  }
  virtual const IDataChannel* GetDataChannel(int iLapId,DATA_CHANNEL eChannel) const override
  {
    AutoLeaveCS _cs(&m_cs);
    ChannelMap::const_iterator i = m_mapChannels.find(iLapId);
    if(i != m_mapChannels.end())
    {
      // ok, we've got stuff about that lap...
      map<DATA_CHANNEL,const IDataChannel*>::const_iterator i2 = i->second.find(eChannel);
      if(i2 != i->second.end())
      {
        return i2->second;
      }
    }
    return NULL;
  }
  virtual set<DATA_CHANNEL> GetAvailableChannels(int iLapId) const override
  {
    AutoLeaveCS _cs(&m_cs);
    set<DATA_CHANNEL> setRet;

    ChannelMap::const_iterator i = m_mapChannels.find(iLapId);
    if(i != m_mapChannels.end())
    {
      // ok, we've got stuff about that lap...
      map<DATA_CHANNEL,const IDataChannel*>::const_iterator i2 = i->second.begin();
      while(i2 != i->second.end())
      {
        setRet.insert(i2->first);
        i2++;
      }
    }
    return setRet;
  };
private:
  vector<const ILap*> m_lstLaps;

  typedef map<int,map<DATA_CHANNEL,const IDataChannel*> > ChannelMap; // for each lapid, defines a map from channeltype to channel
  mutable ChannelMap m_mapChannels; // maps from a lapid to a list of data channels for that lap
  
  IUI* m_pUI;
  TCHAR szLastNetStatus[NETSTATUS_COUNT][200];
  mutable ManagedCS m_cs;
};

IUI* g_pUI = NULL;

INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if(g_pUI)
  {
    return g_pUI->DlgProc(hWnd,uMsg,wParam,lParam);
  }
  return FALSE;
}

#define WM_NOTIFYUPDATE (WM_USER+0x1)
#define WM_UPDATEUI (WM_USER+0x2)

bool CExtendedLap_SortByTime(const CExtendedLap* p1, const CExtendedLap* p2)
{
  return p1->GetLap()->GetStartTime() < p2->GetLap()->GetStartTime();
}
bool CExtendedLap_SortByLapTime(const CExtendedLap* p1, const CExtendedLap* p2)
{
  return p1->GetLap()->GetTime() < p2->GetLap()->GetTime(); // GetTime() or whatever the function is that gets lap time
}

class CMainUI : public IUI,public ILapSupplier
{
public:
  CMainUI() 
    : m_sfLapPainter(static_cast<ILapSupplier*>(this),SUPPLIERID_MAINDISPLAY), 
      m_sfSubDisplay(static_cast<ILapSupplier*>(this),SUPPLIERID_SUBDISPLAY), 
//	  m_sfSubWindow(static_cast<ILapSupplier*>(this),SUPPLIERID_SUBWINDOW), 
      m_sfTractionDisplay(static_cast<ILapSupplier*>(this),SUPPLIERID_TRACTIONCIRCLEDISPLAY), 
	  m_eLapDisplayStyle(LAPDISPLAYSTYLE_PLOT),		//	Make data plot the default initial view
      m_fShowTractionCircle(false),
	  m_fSmooth(false),
	  m_fShowBests(false), 
      m_fShowDriverBests(false),
	  m_fShowReferenceLap(true),
      m_pReferenceLap(NULL),
      m_eXChannel(DATA_CHANNEL_DISTANCE),
      m_fdwUpdateNeeded(0),
      m_flShiftX(0),
      m_flShiftY(0)
  {
	m_iRaceId[0] = 0;
    m_lstYChannels.push_back(DATA_CHANNEL_VELOCITY);
    m_szCommentText[0] = '\0';
    m_szMessageStatus[0] = '\0';
    SetupMulticast();
  }
  	DWORD tmNow, tmLast;	//	Variables for setting up receive time / live car position

//////////////////////////////////////////////////////////////////////////////////
	RECT rect;
	HBITMAP hBitmap;
	BITMAP bitmap;
	int bxWidth, bxHeight, flag;
	HDC hdc,hdcMem;
	HMENU menu;
	HPALETTE hpal;
	int cxsize, cxpage;
	int cysize, cypage;
//////////////////////////////////////////////////////////////////////////////////

void SetRaceId(int iRaceId[50])
  {
    for (int z = 0; z < 50; z++)
	{
	  m_iRaceId[z] = iRaceId[z];	//	Load all of the race sessions chosen
	}
  }
  void NotifyChange(WPARAM wParam, LPARAM lParam) override
  {
    if(m_hWnd != NULL)
    {
      PostMessage(m_hWnd,WM_NOTIFYUPDATE,wParam,(LPARAM)lParam);
    }
  }

  int str_ends_with(const TCHAR * str, const TCHAR * suffix) 
  {
    if( str == NULL || suffix == NULL )
      return 0;

    size_t str_len = wcslen(str);
    size_t suffix_len = wcslen(suffix);

    if(suffix_len > str_len)
      return 0;

    return 0 == wcsncmp( str + str_len - suffix_len, suffix, suffix_len );
  }
  LAPSUPPLIEROPTIONS m_sfLapOpts;
  TCHAR m_szPath[MAX_PATH];

/////////////////////////////////////////////////////////////////////////////////
  //	Functions for enabling Printing of OpenGL graphs
  HDC GetPrinterDC (HWND Hwnd)
	{
	// Initialize a PRINTDLG structure's size and set the PD_RETURNDC flag set the Owner flag to hwnd.
	// The PD_RETURNDC flag tells the dialog to return a printer device context.
		PRINTDLG pd = {0};
		pd.lStructSize = sizeof( pd );
		pd.hwndOwner = Hwnd;
		pd.Flags = PD_RETURNDC;

	// Retrieves the printer DC
		PrintDlg(&pd);
		hdc =pd.hDC;
		return hdc ;
	}

int copyBitmapToClipboard(char *bitmapBuffer, size_t buflen)
{
 HGLOBAL hResult;
 if (!OpenClipboard(NULL)) return 1;//PASTE_OPEN_ERROR;
 if (!EmptyClipboard()) return 2;//PASTE_CLEAR_ERROR;

 //buflen -= sizeof(BITMAPFILEHEADER);
 hResult = GlobalAlloc(GMEM_MOVEABLE, buflen);
//						buflen -= sizeof(BITMAPFILEHEADER);
//						hResult = GlobalAlloc(GMEM_MOVEABLE, dwBmpSize);
 if (hResult == NULL) return 3;//PASTE_DATA_ERROR;

 /*
			 WORD cClrBits;
			// Convert the color format to a count of bits.
				cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
				if (cClrBits == 1)
					cClrBits = 1;
				else if (cClrBits <= 4)
					cClrBits = 4;
				else if (cClrBits <= 8)
					cClrBits = 8;
				else if (cClrBits <= 16)
					cClrBits = 16;
				else if (cClrBits <= 24)
					cClrBits = 24;
				else cClrBits = 32;

			// Allocate memory for the BITMAPINFO structure. (This structure
			// contains a BITMAPINFOHEADER structure and an array of RGBQUAD
			// data structures.)

				if (cClrBits != 24)
				{
					pbmi = (PBITMAPINFO) LocalAlloc(LPTR,sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1<< cClrBits));
				}
			// There is no RGBQUAD array for the 24-bit-per-pixel format.
				else
					pbmi = (PBITMAPINFO) LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER));
*/

//	memcpy(GlobalLock(hResult), bitmapBuffer + sizeof(BITMAPFILEHEADER), buflen);
	memcpy(GlobalLock(hResult), bitmapBuffer, buflen);
// memcpy(GlobalLock(hResult), bitmapBuffer + sizeof(BITMAPFILEHEADER), buflen);
 GlobalUnlock(hResult);

 if (SetClipboardData(CF_DIB, hResult) == NULL) {
  CloseClipboard();
  return 4;//PASTE_PASTE_ERROR;
 }

 CloseClipboard();
 GlobalFree(hResult);
 return 5;//PASTE_WE_DID_IT_YAY;
}

//	Function that sets the printer default to Landscape Mode and Double-Sided printing, if available
LPDEVMODE GetLandscapeDevMode(HWND hWnd, wchar_t *pDevice, HANDLE hPrinter)
{
//  HANDLE      hPrinter;
  LPDEVMODE   pDevMode;
  DWORD       dwNeeded, dwRet;

  // Start by opening the printer 
  if (!OpenPrinter(pDevice, &hPrinter, NULL))
  {
	  MessageBox(hWnd, L"Printer not found, Landscape mode disabled",L"Error", MB_OK);
      return NULL;
  }

  // Step 1:
  // Allocate a buffer of the correct size.
  dwNeeded = DocumentProperties(hWnd,
       hPrinter,       /* Handle to our printer. */ 
       pDevice,        /* Name of the printer. */ 
       NULL,           /* Asking for size, so */ 
       NULL,           /* these are not used. */ 
       0);             /* Zero returns buffer size. */ 
  pDevMode = (LPDEVMODE)malloc(dwNeeded);

  // Step 2:
  // Get the default DevMode for the printer and
  // modify it for your needs.
  dwRet = DocumentProperties(hWnd,
       hPrinter,
       pDevice,
       pDevMode,       // The address of the buffer to fill.
       NULL,           // Not using the input buffer.
       DM_OUT_BUFFER); // Have the output buffer filled. 
  if (dwRet != IDOK)
  {
       // If failure, cleanup and return failure. 
       free(pDevMode);
       ClosePrinter(hPrinter);
       return NULL;
  }

  //	Make changes to the DevMode which are supported.
  if (pDevMode->dmFields & DM_ORIENTATION)
  {
       // If the printer supports paper orientation, set it.
       pDevMode->dmOrientation = DMORIENT_LANDSCAPE;
  }

  if (pDevMode->dmFields & DM_DUPLEX)
  {
       // If it supports duplex printing, use it.  
       pDevMode->dmDuplex = DMDUP_HORIZONTAL;
  }

  // Step 3:
  // Merge the new settings with the old.
  // This gives the driver an opportunity to update any private
  // portions of the DevMode structure.
  dwRet = DocumentProperties(hWnd,
       hPrinter,
       pDevice,
       pDevMode,       // Reuse our buffer for output.
       pDevMode,       // Pass the driver our changes. 
       DM_IN_BUFFER |  // Commands to Merge our changes and 
       DM_OUT_BUFFER); // write the result. 

  // Finished with the printer
  ClosePrinter(hPrinter);

  if (dwRet != IDOK)
  {
       // If failure, cleanup and return failure.
       free(pDevMode);
       return NULL;
  }

  // Return the modified DevMode structure.
  return pDevMode;
}
///////////////////////////////////////////////////////////////////////////////////
//	Tentative code for Custom Draw of ListViews implementation
	LRESULT ProcessCustomDraw (LPARAM lParam, WPARAM wParam, INT i_Color)
	{
		LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

		switch(lplvcd->nmcd.dwDrawStage) 
		{
			case CDDS_PREPAINT : //	Before the paint cycle begins
				return CDRF_NOTIFYITEMDRAW;	//	Request notifications for individual listview items
            
			case CDDS_ITEMPREPAINT: //	Before an item is drawn
				{
					return CDRF_NOTIFYSUBITEMDRAW;	//	Request notifications for individual listview SubItems
				}
				break;
    
			case CDDS_SUBITEM | CDDS_ITEMPREPAINT: //	Before a subitem is drawn
				{
					//	switch(lplvcd->iSubItem)
					switch(i_Color)
					{
						case 0:
						{
							//	Black letters, clear background (Default)
							lplvcd->clrText   = RGB(0,0,0);
							lplvcd->clrTextBk = RGB(255,255,255);
							return CDRF_NEWFONT;
						}
						break;
                    
						case 1:
						{
							//	Red background, white letters
							lplvcd->clrText   = RGB(255,255,255);
							lplvcd->clrTextBk = RGB(240,55,23);
							return CDRF_NEWFONT;
						}
						break;  

						case 2:
						{
							//	Green background, black letters
							lplvcd->clrText   = RGB(0,0,0);
							lplvcd->clrTextBk = RGB(155,255,80);
							return CDRF_NEWFONT;
						}
						break;
						case 3:
						{
							//	Blue background, white letters
							lplvcd->clrText   = RGB(255,255,255);
							lplvcd->clrTextBk = RGB(20,20,220);
							return CDRF_NEWFONT;
						}
						break;
						case 4:
						{
							//	Black letters, Light Grey background (Default)
							lplvcd->clrText   = RGB(0,0,0);
							lplvcd->clrTextBk = RGB(240,240,255);
							return CDRF_NEWFONT;
						}
						break;
					}
				}
		}
		return CDRF_DODEFAULT;
	}

/////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////
	HWND hWnd_AllData;			//	AllData window control handle
	HWND AD_hWnd;				//	AllData listview control handle
	LVITEM p_ADlvi;				//	Listview global pointer for Hot Laps
	HWND hWndShowSplits;		//	Show Sectors window control handle
	HWND HC_ShowSplits;			//	Handle to the Splits listview control
//	HWND sub_hWnd;				//	Handle to the SubWindow, to get to the SubDisplay control
	BOOL b_GreyColor;			//	Switch for painting listviews with alternating lines of color
	INT i_SubItemBest, i_SubItemWorst;
	BOOL b_Button_LiveData;

  LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    if(m_sfLapPainter.HandleMessage(hWnd,uMsg,wParam,lParam))
    {
      return 0;
    }

	//	Update and show Current Lap Time
    TCHAR szTemp[512], szLap[512];
    HWND hWndIp = GetDlgItem(m_hWnd, IDC_LIVELAPTIME);
	static WINDOWPLACEMENT w_AllDataWindow;	//	Save the location for the AllData display window
	static WINDOWPLACEMENT w_SectorTimesWindow;	//	Save the location for the Sector Times display window
    ::FormatTimeMinutesSecondsMs((float)(timeGetTime() - tmLast) / 1000, szLap, NUMCHARS(szLap) );
	swprintf(szLap, _tcslen(szLap) - 2, L"%s", szLap);	//	Remove the fractional time
    swprintf(szTemp, NUMCHARS(szTemp), L"Current Lap: %s", szLap);	//	Final string to show current lap time
    SendMessage(hWndIp, WM_SETTEXT, 0, (LPARAM)szTemp);

	switch(uMsg)
	{
	  case WM_INITDIALOG:
      {
        m_hWnd = hWnd;
		hWndShowSplits = NULL;
        {
          vector<wstring> lstCols;
          vector<int> lstWidths;
          CExtendedLap::GetStringHeadersXAxis(lstCols,lstWidths);
          m_sfXAxis.Init(GetDlgItem(m_hWnd, IDC_XAXIS),lstCols,lstWidths);
          CExtendedLap::SetDistance(m_sfLapOpts.bXAxis_KM);	//	Set the X Axis as read from SETTINGS.TXT file
		}
        {
          vector<wstring> lstCols;
          vector<int> lstWidths;
          CExtendedLap::GetStringHeadersYAxis(lstCols,lstWidths);
          m_sfYAxis.Init2(GetDlgItem(m_hWnd, IDC_YAXIS),lstCols,lstWidths);
        }
        {
          vector<wstring> lstCols;
          vector<int> lstWidths;
          CExtendedLap::GetStringHeadersDataValues(lstCols,lstWidths);
          m_sfLapList.Init(GetDlgItem(m_hWnd, IDC_DATAVALUES), lstCols,lstWidths);
        }
        {
          vector<wstring> lstCols;
          vector<int> lstWidths;
          CExtendedLap::GetStringHeaders(lstCols,lstWidths);
          m_sfLapList.Init(GetDlgItem(m_hWnd, IDC_LAPS), lstCols,lstWidths);
        }
        m_sfLapPainter.Init(GetDlgItem(hWnd,IDC_DISPLAY));
//        ArtShowDialog<IDD_DLGSUBDISPLAY>(s_pUI);
//		sub_hWnd = GetDlgItem(m_hWnd,IDD_DLGSUBDISPLAY);
//		sub_hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, L"SubDisplay", L"SubDisplay",  WS_POPUP | WS_SYSMENU | WS_THICKFRAME | WS_CAPTION, 586, 0, 87, 63, m_hWnd, NULL, GetModuleHandle(0), NULL);
//		m_sfSubDisplay.Init(GetDlgItem(sub_hWnd,IDC_SUBDISPLAY));
		m_sfSubDisplay.Init(GetDlgItem(m_hWnd,IDC_SUBDISPLAY));
        m_sfTractionDisplay.Init(GetDlgItem(m_hWnd,IDC_TRACTIONCIRCLEMAP));

		LoadLaps(g_pLapDB);
		if (m_sfLapOpts.LapId != 0 && m_mapLaps.size() > 0)	//	Reference lap needs to be loaded. Identify which one it is and set it
		{
            for(map<int,CExtendedLap*>::iterator i = m_mapLaps.begin(); i != m_mapLaps.end(); i++)
            {
                if(i->second->GetLap()->GetLapId() == m_sfLapOpts.LapId)
                {
					m_pReferenceLap = i->second;	//	Set the Reference lap to this lap                    
                }
            }
            m_pReferenceLap->ComputeDistances(NULL, g_pLapDB);	//	Now recompute distances, based upon this Reference Lap
            for(map<int,CExtendedLap*>::iterator i = m_mapLaps.begin(); i != m_mapLaps.end(); i++)
            {
				if(i->second != m_pReferenceLap)
				{
					i->second->ComputeDistances(m_pReferenceLap, g_pLapDB); // tell this lap to recompute using pLap as the reference lap
				}
            }
		}

		
		set<DATA_CHANNEL> setAvailable;
        InitAxes(setAvailable);
        LoadLaps(::g_pLapDB);
        UpdateUI(UPDATE_ALL);
        InitBaseWindowPos();

		tmLast = timeGetTime();	//	Initialize time lap was received

		// Find all of the Names of the selected Race Sessions for addition to title bar
		vector<RACEDATA> lstRaces = g_pLapDB->GetRaces();
		TCHAR lstSessions[MAX_PATH] = {NULL};
		for(int x = 0;x < lstRaces.size(); x++)
		{
			for (int z = 0; z < 50; z++)
			{
				if ( m_iRaceId[z] == lstRaces[x].raceId )
				{
					_snwprintf(lstSessions, NUMCHARS(lstSessions), L"%s | %s", lstSessions, lstRaces[x].strName.c_str());
					break;
				}
			}
		}
		
		TCHAR szTemp[MAX_PATH] = {NULL};
	    _snwprintf(szTemp, NUMCHARS(szTemp), L"Pitside - %s%s", m_szPath, lstSessions);
		SetWindowText(m_hWnd, szTemp );	//	Change the title bar to show the file name and Session(s) opened

		if (m_sfLapOpts.fDrawSplitPoints == true)	//	Initialize the Sector Times window
		{
			//	Create non-modal dialog to display the sector times window if DrawSplitPoints is TRUE
			DLGPROC ShowSplits = NULL;
			//	Create the window for displaying sector times for the selected laps
			INITCOMMONCONTROLSEX InitCtrlEx;
			InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
			InitCtrlEx.dwICC = ICC_PROGRESS_CLASS;
			InitCommonControlsEx(&InitCtrlEx);
			hWndShowSplits = CreateDialog(NULL, MAKEINTRESOURCE (IDD_SHOWSECTORS), hWnd, (DLGPROC)ShowSplits);	//	Create resource
			HC_ShowSplits = GetDlgItem(hWndShowSplits, IDC_SHOW_SECTORS);	//	Let's get the handle for the display control in this window
			SetWindowPlacement(hWndShowSplits, &w_SectorTimesWindow);	//	Maintains the location of the Sector Times window

			//	Set up the Sector Times list box columns
			vector<wstring> lstCols;
			vector<int> lstWidths;
			lstCols.push_back(L"Lap ID");
			lstCols.push_back(L"1");
			lstCols.push_back(L"2");
			lstCols.push_back(L"3");
			lstCols.push_back(L"4");
			lstCols.push_back(L"5");
			lstCols.push_back(L"6");
			lstCols.push_back(L"7");
			lstCols.push_back(L"8");
			lstCols.push_back(L"9");
			lstWidths.push_back(195);
			lstWidths.push_back(40);
			lstWidths.push_back(40);
			lstWidths.push_back(40);
			lstWidths.push_back(40);
			lstWidths.push_back(40);
			lstWidths.push_back(40);
			lstWidths.push_back(40);
			lstWidths.push_back(40);
			lstWidths.push_back(40);

			m_sfListBox.Init(HC_ShowSplits,lstCols,lstWidths);	//	Initialize and show the Sector Splits window
			ShowWindow(hWndShowSplits, SW_SHOW); 
		}

		return 0;
      }
      case WM_CLOSE:
	  {
        DestroyWindow(hWnd);
		EndDialog(hWnd,0);
        return 0;
	  }
	  case WM_MOUSEWHEEL:
	  {
       short iDist = HIWORD(wParam);
        m_sfLapOpts.iZoomLevels += (iDist/WHEEL_DELTA);
        UpdateUI(UPDATE_MAP);
        return 0;
      }
      case WM_MOUSEMOVE:
      {
        const int INVALID_MOUSEPOS = 0x80000000;
        static int lastX = INVALID_MOUSEPOS;
        static int lastY = INVALID_MOUSEPOS;
        const int x = LOWORD(lParam);
        const int y = HIWORD(lParam);
        const int moveX = lastX == INVALID_MOUSEPOS ? 0 : x - lastX;
        const int moveY = lastY == INVALID_MOUSEPOS ? 0 : y - lastY;
        lastX = x;
        lastY = y;

		switch(wParam)
		{
			case MK_CONTROL:	//	Ctrl + Mouse movement
			{
			   short iDist = HIWORD(wParam);
				m_sfLapOpts.iZoomLevels += - moveY;
				UpdateUI(UPDATE_MAP);
				break;
			}
			case MK_LBUTTON:
			{
			  // they're dragging!
			  m_sfLapOpts.flWindowShiftX += moveX;
			  m_sfLapOpts.flWindowShiftY -= moveY;
			  break;
			}
			case MK_MBUTTON:
			{
			  // Dynamic update of All Data display
			  UpdateUI(UPDATE_ALLDATA);
			  break;
			}
		}
        UpdateUI(UPDATE_MAP);
        return 0;
      }
      case WM_LBUTTONDOWN:
     {
        const int x = LOWORD(lParam);
        const int y = HIWORD(lParam);
        // figure out if we should put focus on the main map
        RECT rcMap;
        HWND hWndMap = GetDlgItem(this->m_hWnd,IDC_DISPLAY);
        GetClientRect(hWndMap,&rcMap);
        if(x >= rcMap.left && x < rcMap.right && y >= rcMap.top && y < rcMap.bottom)
        {
          SetFocus(hWndMap);
          return TRUE;
        }
        return FALSE;
      }
		//	Left/Middle mouse button activates a modal window that dispalys all of the data for a given point in a table
	  case WM_MBUTTONDOWN:
	  case WM_MBUTTONDBLCLK:
	  case WM_LBUTTONDBLCLK:
	  {
		  if ( GetDlgItem(hWnd_AllData, IDC_ALLDATADISPLAY) && uMsg != WM_MBUTTONDOWN )	//	If the window showing all of the lap data is present, let's kill it
		  {
			if (GetWindowPlacement(hWnd_AllData, &w_AllDataWindow) )
			{
			  DestroyWindow(hWnd_AllData);
			  hWnd_AllData = NULL;
			  AD_hWnd = NULL;
			}
			return TRUE;
		  }
		  else	//	If the window showing all of the lap data isn't there, let's create it
		  {
			DLGPROC ShowAllData = NULL;
			if (!GetDlgItem(hWnd_AllData, IDC_ALLDATADISPLAY))	//	Make sure that the display isn't already showing
			{ 
				//	Create the window for displaying all of the data points for the selected laps
				INITCOMMONCONTROLSEX InitCtrlEx;
				InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
				InitCtrlEx.dwICC = ICC_PROGRESS_CLASS;
				InitCommonControlsEx(&InitCtrlEx);
				hWnd_AllData = CreateDialog(NULL, MAKEINTRESOURCE (IDD_ALLDATADISPLAY), hWnd, (DLGPROC)ShowAllData);
				AD_hWnd = GetDlgItem(hWnd_AllData,IDC_ALLDATADISPLAY);	//	All Data listview control
				SetWindowPlacement(hWnd_AllData, &w_AllDataWindow);

				//	Set up the AllData list box columns
				vector<wstring> lstCols;
				vector<int> lstWidths;
				lstCols.push_back(L"Data Channel");
				lstCols.push_back(L"Lap 1");
				lstCols.push_back(L"Lap 2");
				lstCols.push_back(L"Lap 3");
				lstCols.push_back(L"Lap 4");
				lstCols.push_back(L"Lap 5");
				lstCols.push_back(L"Lap 6");
				lstCols.push_back(L"Lap 7");
				lstCols.push_back(L"Lap 8");
				lstCols.push_back(L"Lap 9");
				lstCols.push_back(L"Lap 10");
				lstWidths.push_back(130);
				lstWidths.push_back(60);
				lstWidths.push_back(60);
				lstWidths.push_back(60);
				lstWidths.push_back(60);
				lstWidths.push_back(60);
				lstWidths.push_back(60);
				lstWidths.push_back(60);
				lstWidths.push_back(60);
				lstWidths.push_back(60);
				lstWidths.push_back(60);

				m_sfListBox.Init(AD_hWnd,lstCols,lstWidths);	//	Initialize the listview
				ShowWindow(hWnd_AllData, SW_SHOW);	//	Show the listview
			} 

			UpdateUI(UPDATE_MAP | UPDATE_ALLDATA);
			return TRUE;
		  }
	  }
      case WM_NOTIFY:
      {
        NMHDR* notifyHeader = (NMHDR*)lParam;
        switch(wParam)
        {
        case IDC_LAPS:
            switch(notifyHeader->code)
            {
				case LVN_ITEMCHANGED:
				{
				  NMITEMACTIVATE* pDetails = (NMITEMACTIVATE*)notifyHeader;
				  if(pDetails->iItem >= 0)
				  {
					  UpdateUI(UPDATE_MAP | UPDATE_DASHBOARD | UPDATE_VALUES);
				  }
				  return TRUE;
				}
				case  LVN_COLUMNCLICK:
				{
					LPNMLISTVIEW pLVInfo = (LPNMLISTVIEW)lParam;	
					//	User clicked the column header, let's re-sort the list
					if (pLVInfo->iSubItem == SORTSTYLE_BYTIMEOFRACE && m_sfLapOpts.eSortPreference != SORTSTYLE_BYTIMEOFRACE)
					{
						m_sfLapOpts.eSortPreference = SORTSTYLE_BYTIMEOFRACE;
						UpdateUI(UPDATE_DASHBOARD | UPDATE_LIST);
					}
					else if (pLVInfo->iSubItem == SORTSTYLE_BYLAPTIME && m_sfLapOpts.eSortPreference != SORTSTYLE_BYLAPTIME)
					{
						m_sfLapOpts.eSortPreference = SORTSTYLE_BYLAPTIME;
						UpdateUI(UPDATE_DASHBOARD | UPDATE_LIST);
					}
				}
			}
            break;
          case IDC_XAXIS:
            switch(notifyHeader->code)
            {
            case LVN_ITEMCHANGED:
              const set<LPARAM> sel = m_sfXAxis.GetSelectedItemsData();
              if(sel.size() == 1)
              {
                m_eXChannel = (DATA_CHANNEL)*sel.begin();	//	Just take the first selected item for our choice
                NMITEMACTIVATE* pDetails = (NMITEMACTIVATE*)notifyHeader;
                if(pDetails->iItem >= 0)	//	Single select Listview, requires special handling
                {
                  UpdateUI(UPDATE_MAP| UPDATE_DASHBOARD | UPDATE_VALUES);
                }
              }
			  return TRUE;
            }
            break;
          case IDC_YAXIS:
            switch(notifyHeader->code)
            {
            case LVN_ITEMCHANGED:
              const set<LPARAM> sel = m_sfYAxis.GetSelectedItemsData2();
              if(sel.size() >= 1)
              {
                m_lstYChannels.clear();
                for(set<LPARAM>::iterator i = sel.begin(); i != sel.end(); i++)
                {
                  m_lstYChannels.push_back((DATA_CHANNEL)*i);
                }
                NMITEMACTIVATE* pDetails = (NMITEMACTIVATE*)notifyHeader;
                if(pDetails->iItem >= 0)
                {
                  UpdateUI(UPDATE_MAP| UPDATE_DASHBOARD | UPDATE_VALUES);
                }
              }
              return TRUE;
            }
            break;
        } // end switch on wParam

		LPNMLISTVIEW  pnm  = (LPNMLISTVIEW)lParam;
		switch (pnm->hdr.code)	//	Switch for ListView coloring and painting routines
		{
			case NM_CUSTOMDRAW:
			{
				LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW)lParam;
				enum i_Color
				{
					CLEAR,
					RED,
					GREEN,
					BLUE,
					LTGREY
				};

				if( pnm->hdr.hwndFrom == AD_hWnd)	//	First see if this is from the All Data Display LV control
				{
					SetWindowLong(hWnd_AllData, DWL_MSGRESULT, (LONG)ProcessCustomDraw(lParam, wParam, GREEN));
					return TRUE;
				}
				else if( pnm->hdr.hwndFrom == HC_ShowSplits)	//	See if this is from the Sector Display LV control
				{
					SetWindowLong(hWndShowSplits, DWL_MSGRESULT, (LONG)ProcessCustomDraw(lParam, wParam, GREEN));
					return TRUE;
				}
				switch (wParam)
				{
					case IDC_XAXIS:
					case IDC_YAXIS:
					{
						SetWindowLong(hWnd, DWL_MSGRESULT, (LONG)ProcessCustomDraw(lParam, wParam, LTGREY));
						return TRUE;
					}
					case IDC_LAPS:
					{
//						SetWindowLong(hWnd, DWL_MSGRESULT, (LONG)ProcessCustomDraw(lParam, wParam, CLEAR));	//	No LV coloring for Laps list
						return TRUE;
					}
					case IDC_DATAVALUES:
					{
						SetWindowLong(hWnd, DWL_MSGRESULT, (LONG)ProcessCustomDraw(lParam, wParam, CLEAR));	//	No LV coloring for Values Display
						return TRUE;
					}
					case IDC_ALLDATADISPLAY:	//	First see if this is from the All Data Display LV control
					{
						SetWindowLong(hWnd, DWL_MSGRESULT, (LONG)ProcessCustomDraw(lParam, wParam, GREEN));
						return TRUE;
					}
					case IDC_SHOW_SECTORS:	//	See if this is from the Sector Display LV control
					{
						SetWindowLong(hWnd, DWL_MSGRESULT, (LONG)ProcessCustomDraw(lParam, wParam, GREEN));
						return TRUE;
					}
					default:
					{
						SetWindowLong(hWnd, DWL_MSGRESULT, (LONG)ProcessCustomDraw(lParam, wParam, BLUE));	//	Color Blue if process gets here (unlikely)
						return TRUE;
					}
				}

				switch(lplvcd->nmcd.dwDrawStage) 
				{
					case CDDS_PREPAINT :
						return CDRF_NOTIFYITEMDRAW;

					case CDDS_ITEMPREPAINT:
						if ( b_GreyColor)	//	Let's paint it with grey background
						{
							lplvcd->clrText = RGB(0,0,0);	//	Black text
							lplvcd->clrTextBk = RGB(200,200,200);	//	Grey background
							b_GreyColor = !b_GreyColor;
						}
						else
						{
							lplvcd->clrText = RGB(0,0,0);	//	Black text
							lplvcd->clrTextBk = RGB(255,255,255);	//	White background
						}

	//					At this point, you can change the background colors for the item
	//					and any subitems and return CDRF_NEWFONT. If the list-view control
	//					is in report mode, you can simply return CDRF_NOTIFYSUBITEMDRAW
	//					to customize the item's subitems individually 

						if (pnm->hdr.hwndFrom == HC_ShowSplits || pnm->hdr.hwndFrom == AD_hWnd)
						{
							return CDRF_NOTIFYSUBITEMDRAW;	//	Returns this to start the SubItem painting
						}
						else
						{
							return CDRF_NEWFONT;	//	No subitem painting, reduce overhead
						}

					case CDDS_SUBITEM | CDDS_ITEMPREPAINT:	//	SubItem painting routines
						if ( lplvcd->iSubItem == i_SubItemBest)	//	Best time/value, let's paint it with green background
						{
							lplvcd->clrText = RGB(0,0,0);	//	Black text
							lplvcd->clrTextBk = RGB(20,230,20);	//	Green background
						}
						if ( lplvcd->iSubItem == i_SubItemWorst)	//	Worst time/value, let's paint it with green background
						{
							lplvcd->clrText = RGB(0,0,0);	//	Black text
							lplvcd->clrTextBk = RGB(230,20,20);	//	Red background
						}

	//					This notification is received only if you are in report mode and
	//					returned CDRF_NOTIFYSUBITEMDRAW in the previous step. At
	//					this point, you can change the background colors for the
	//					subitem and return CDRF_NEWFONT.

						return CDRF_NEWFONT;    
					default:
						return CDRF_DODEFAULT;
				}
			}
			default:
				return CDRF_DODEFAULT;
		}
      } // end body of case WM_NOTIFY
	  case WM_COMMAND:
      {
	    switch(LOWORD(wParam)) // find out the control ID
	    {
          case IDC_SENDMESSAGE:
          {
            static MESSAGEDLG_RESULT sfResult;
			if(wcslen(sfResult.szMessage) <= 0)
			{
				wcscpy_s( sfResult.szMessage, L"Pit Now");	//	Add default string if nothing is there
			}
            CMessageDlg dlgMessage(&sfResult);
            
            HWND hWndButton = GetDlgItem(this->m_hWnd,IDC_SENDMESSAGE);
            EnableWindow(hWndButton,FALSE);
            ArtShowDialog<IDD_DLGMESSAGE>(&dlgMessage);
            EnableWindow(hWndButton,TRUE);
            LPCTSTR lpsz = g_pLapDB->GetNetStatus(NETSTATUS_REMOTEIP);
            sprintf(sfResult.szIP, "%S", lpsz);

            if(!sfResult.fCancelled)
            {
				// now that we're done, we should have a result!
				SendMsg(sfResult, this);
            }
            return TRUE;
          }
		      case IDOK:
          {
			  return TRUE;
          }
          case ID_OPTIONS_KMH:
          {
            m_sfLapOpts.eUnitPreference = UNIT_PREFERENCE_KMH;
            UpdateUI(UPDATE_MAP | UPDATE_MENU);
            return TRUE;
          }
          case ID_OPTIONS_MPH:
          {
            m_sfLapOpts.eUnitPreference = UNIT_PREFERENCE_MPH;
            UpdateUI(UPDATE_MAP | UPDATE_MENU);
            return TRUE;
          }
          case ID_OPTIONS_MS:
          {
            m_sfLapOpts.eUnitPreference = UNIT_PREFERENCE_MS;
            UpdateUI(UPDATE_MAP | UPDATE_MENU);
            return TRUE;
          }
          case ID_OPTIONS_VERTICAL_LANDSCAPE:
          {
            m_sfLapOpts.e_Orientation = VERTICAL_LANDSCAPE;
            UpdateUI(UPDATE_MAP | UPDATE_MENU);
            return TRUE;
          }
          case ID_OPTIONS_VERTICAL_PORTRAIT:
          {
            m_sfLapOpts.e_Orientation = VERTICAL_PORTRAIT;
            UpdateUI(UPDATE_MAP | UPDATE_MENU);
            return TRUE;
          }
          case ID_OPTIONS_FLAT_LANDSCAPE:
          {
            m_sfLapOpts.e_Orientation = FLAT_LANDSCAPE;
            UpdateUI(UPDATE_MAP | UPDATE_MENU);
            return TRUE;
          }
          case ID_OPTIONS_FLAT_PORTRAIT:
          {
            m_sfLapOpts.e_Orientation = FLAT_PORTRAIT;
            UpdateUI(UPDATE_MAP | UPDATE_MENU);
            return TRUE;
          }
          case ID_OPTIONS_XAXIS_KM:
          {
            m_sfLapOpts.bXAxis_KM = XAXIS_PREFERENCE_KM;
            CExtendedLap::SetDistance(m_sfLapOpts.bXAxis_KM);	//	Set the X Axis as read from SETTINGS.TXT file
			MessageBox(hWnd, L"X-Axis will be displayed in KM's\n\nYou need to re-load the Race Session for this change to take effect!",L"NOTICE", MB_OK);
            UpdateUI(UPDATE_MAP | UPDATE_MENU);
            return TRUE;
          }
          case ID_OPTIONS_XAXIS_LAT:
          {
            m_sfLapOpts.bXAxis_KM = XAXIS_PREFERENCE_LAT;
            CExtendedLap::SetDistance(m_sfLapOpts.bXAxis_KM);	//	Set the X Axis as read from SETTINGS.TXT file
			MessageBox(hWnd, L"X-Axis will be displayed in Latitude/Longitudinal degrees\n\nYou need to re-load the Race Session for this change to take effect!",L"NOTICE", MB_OK);
            UpdateUI(UPDATE_MAP | UPDATE_MENU);
            return TRUE;
          }
          case ID_OPTIONS_SHOWBESTS:
          {
            m_fShowBests = !m_fShowBests;
            UpdateUI(UPDATE_MENU | UPDATE_MAP | UPDATE_DASHBOARD | UPDATE_VALUES);
            return TRUE;
          }
          case ID_OPTIONS_SHOWREFERENCELAP:
          {
            m_fShowReferenceLap = !m_fShowReferenceLap;
            m_sfLapOpts.bShowReferenceLap = m_fShowReferenceLap;
            UpdateUI(UPDATE_MENU | UPDATE_MAP | UPDATE_DASHBOARD | UPDATE_VALUES);
            return TRUE;
          }
          case ID_OPTIONS_DRAWLINES:
          {
            m_sfLapOpts.fDrawLines = !m_sfLapOpts.fDrawLines;
            UpdateUI(UPDATE_MENU | UPDATE_MAP);
            return TRUE;
          }
          case ID_OPTIONS_BACKGROUND:
          {
            m_sfLapOpts.fColorScheme = !m_sfLapOpts.fColorScheme;
            UpdateUI(UPDATE_MENU | UPDATE_MAP);
            return TRUE;
          }
          case ID_OPTIONS_IOIO5VSCALE:
          {
            m_sfLapOpts.fIOIOHardcoded = !m_sfLapOpts.fIOIOHardcoded;
            UpdateUI(UPDATE_MENU | UPDATE_MAP | UPDATE_DASHBOARD);
            return TRUE;
          }
          case ID_OPTIONS_SHOWDRIVERBESTS:
          {
            m_fShowDriverBests = !m_fShowDriverBests;
            UpdateUI(UPDATE_MENU | UPDATE_MAP | UPDATE_DASHBOARD | UPDATE_VALUES);
            return TRUE;
          }
          case ID_DATA_SWITCHSESSION:
          {
              static bool bSwitchSession;	//	Flag to prevent multiple windows from being opened
			  if (!bSwitchSession)
			  {
				RACESELECT_RESULT sfResult;
				bSwitchSession = true;
				//	Zero out the Race ID's before selecting them
				for (int z = 0; z < 50; z++)
				{
					m_iRaceId[z] = -1;	
					sfResult.iRaceId[z] = -1;
				}
			
				CRaceSelectDlg dlgRace(g_pLapDB, &sfResult);
				ArtShowDialog<IDD_SELECTRACE>(&dlgRace);

				if(!sfResult.fCancelled)
				{
				  for (int z = 0; z < 50; z++)
				  {
					m_iRaceId[z] = sfResult.iRaceId[z];	//	Load all of the race sessions chosen
				  }
				  ClearUILaps();
				  LoadLaps(g_pLapDB);
				  UpdateUI(UPDATE_ALL);

				  // Find all of the Names of the selected Race Sessions for addition to title bar
				  vector<RACEDATA> lstRaces = g_pLapDB->GetRaces();
				  TCHAR lstSessions[MAX_PATH] = {NULL};
				  for(int x = 0;x < lstRaces.size(); x++)
				  {
						for (int z = 0; z < 50; z++)
						{
							if ( m_iRaceId[z] == lstRaces[x].raceId )
							{
								_snwprintf(lstSessions, NUMCHARS(lstSessions), L"%s | %s", lstSessions, lstRaces[x].strName.c_str());
								break;
							}
						}
				  }
		
				  TCHAR szTemp[MAX_PATH] = {NULL};
				  _snwprintf(szTemp, NUMCHARS(szTemp), L"Pitside - %s%s", m_szPath, lstSessions);
				  SetWindowText(m_hWnd, szTemp );	//	Change the title bar to show the file name and Session(s) opened

				  //	Just loaded a new session. Let's reset the timer
				  tmLast = timeGetTime();	//	Save last time lap was received
				}
				bSwitchSession = false;
			  }
			  return TRUE;
          }
          case ID_DATA_EDITSESSION:
          {
            RACESELECTEDIT_RESULT sfResult;
            static bool bEditSessions;	//	Flag to prevent multiple windows from being opened
			if (!bEditSessions)
			{
				bEditSessions = true;
				CRaceSelectEditDlg dlgRace(g_pLapDB, &sfResult);
				ArtShowDialog<IDD_SELECTRACEEDIT>(&dlgRace);

				if(!sfResult.fCancelled)
				{
				  m_iRaceId[0] = sfResult.iRaceId;
				  ClearUILaps();
				  LoadLaps(g_pLapDB);
				  UpdateUI(UPDATE_MAP | UPDATE_MENU | UPDATE_DASHBOARD );
				}
				bEditSessions = false;
			}
            return TRUE;
          }
		  case ID_OPTIONS_PLOTPREFS:
		  {
			bool static bSetPlot;
			PLOTSELECT_RESULT sfResult;
			if (!bSetPlot)	//	Prevent duplicate windows from appearing
			{
				bSetPlot = true;
				CPlotSelectDlg dlgPlot(g_pLapDB, &sfResult, m_iRaceId[0], &m_sfLapOpts);
				ArtShowDialog<IDD_PLOTPREFS>(&dlgPlot);
				bSetPlot = false;
				UpdateUI(UPDATE_MENU | UPDATE_MAP | UPDATE_DASHBOARD | UPDATE_VALUES);
			}
			return TRUE;
		  }		
		  case ID_OPTIONS_SETSPLITS:
		  {
			bool static bSetSplits;
			SETSPLITSDLG_RESULT sfResult;
			if (!bSetSplits)	//	Prevent duplicate windows from appearing
			{
				bSetSplits = true;
				CSetSplitsDlg dlgSetSplits(g_pLapDB, m_pReferenceLap,  &sfResult, m_iRaceId[0], &m_sfLapOpts);
				ArtShowDialog<IDD_SETSPLITPOINTS>(&dlgSetSplits);
				bSetSplits = false;
			}
			else
			{
				return TRUE;
			}

			const int cSectors = 9;	//	Maximum numbers of Split Times
			const int MaxLaps = 8;	//	Maximum number of laps to display
			if (!IsWindow(hWndShowSplits) && m_sfLapOpts.fDrawSplitPoints)
			{
				//	Create non-modal dialog to display the sector times window if DrawSplitPoints is TRUE
				DLGPROC ShowSplits = NULL;
				//	Create the window for displaying sector times for the selected laps
				INITCOMMONCONTROLSEX InitCtrlEx;
				InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
				InitCtrlEx.dwICC = ICC_PROGRESS_CLASS;
				InitCommonControlsEx(&InitCtrlEx);
				hWndShowSplits = CreateDialog(NULL, MAKEINTRESOURCE (IDD_SHOWSECTORS), hWnd, (DLGPROC)ShowSplits);	//	Create resource
				HC_ShowSplits = GetDlgItem(hWndShowSplits, IDC_SHOW_SECTORS);	//	Let's get the handle for the display control in this window
				SetWindowPlacement(hWndShowSplits, &w_SectorTimesWindow);	//	Maintains the location of the Sector Times window

				//	Set up the Sector Times list box columns
				vector<wstring> lstCols;
				vector<int> lstWidths;
				lstCols.push_back(L"Lap ID");
				lstCols.push_back(L"1");
				lstCols.push_back(L"2");
				lstCols.push_back(L"3");
				lstCols.push_back(L"4");
				lstCols.push_back(L"5");
				lstCols.push_back(L"6");
				lstCols.push_back(L"7");
				lstCols.push_back(L"8");
				lstCols.push_back(L"9");
				lstWidths.push_back(195);
				lstWidths.push_back(40);
				lstWidths.push_back(40);
				lstWidths.push_back(40);
				lstWidths.push_back(40);
				lstWidths.push_back(40);
				lstWidths.push_back(40);
				lstWidths.push_back(40);
				lstWidths.push_back(40);
				lstWidths.push_back(40);

				m_sfListBox.Init(HC_ShowSplits,lstCols,lstWidths);	//	Initialize and show the Sector Splits window
				ShowWindow(hWndShowSplits, SW_SHOW); 
			}
			else if (!m_sfLapOpts.fDrawSplitPoints)
			{
				  if (HC_ShowSplits)	//	If the window showing all of the lap data is present, let's kill it
				  {
					  if (GetWindowPlacement(hWndShowSplits, &w_SectorTimesWindow) )
					  {
						  DestroyWindow(hWndShowSplits);
						  hWndShowSplits = NULL;
					  }
				  }
			}
			if(!sfResult.fCancelled)
            {
			  UpdateUI(UPDATE_MENU | UPDATE_MAP | UPDATE_TRACTIONCIRCLE | UPDATE_DASHBOARD);
			}
            return TRUE;
		  }		
		  case ID_OPTIONS_CANCELSPLITS:
		  {
				if (HC_ShowSplits)	//	If the window showing all of the lap data is present, let's kill it
				{
					if (GetWindowPlacement(hWndShowSplits, &w_SectorTimesWindow) )
					{
						DestroyWindow(hWndShowSplits);
						hWndShowSplits = NULL;
					}
				}
				m_sfLapOpts.fDrawSplitPoints = false;
				return TRUE;
		  }
		  case ID_HELP_SHOWHELP:
          {
            ShowHelp(hWnd);
            return TRUE;
          }
          case ID_HELP_SHOWWFLHELP:
          {
            ShowWFLHelp(hWnd);
            return TRUE;
          }
          case ID_HELP_IPS:
          {
            ShowNetInfo();
            return TRUE;
          }
          case ID_HELP_ABOUT:
		  {
			ABOUT_RESULT sfResult;
			CAboutDlg dlgAbout(&sfResult);
			ArtShowDialog<IDD_ABOUT>(&dlgAbout);
			UpdateUI(UPDATE_MENU | UPDATE_MAP | UPDATE_TRACTIONCIRCLE | UPDATE_DASHBOARD);
			return TRUE;
		  }		
          case ID_OPTIONS_TRACTIONCIRCLE:
		  {
            m_fShowTractionCircle = !m_fShowTractionCircle;
			m_sfLapOpts.bTractionCircle = m_fShowTractionCircle;
			UpdateUI(UPDATE_MENU | UPDATE_MAP | UPDATE_TRACTIONCIRCLE);
            return TRUE;
		  }		
          case ID_OPTIONS_SMOOTH:
		  {
			m_sfLapOpts.bSmoothYesNo = !m_sfLapOpts.bSmoothYesNo;
			UpdateUI(UPDATE_MENU | UPDATE_MAP | UPDATE_TRACTIONCIRCLE);
            return TRUE;
		  }		
		  //	Nested loop for the following functions
          case ID_EDIT_COPY:
		  {
			//	Option is not working and causes crashes right now, so disable it
			return TRUE;
		  }
		  case IDM_PRINT_BM:
		  case IDM_SAVE_BM:
          {
			  int SaveFlag = false, PrintFlag = false;	
			  //	Set flag to sending image to Clipboard, if requested by user
			  if (LOWORD(wParam) == IDM_SAVE_BM)
				  SaveFlag = true;
			  if (LOWORD(wParam) == IDM_PRINT_BM)
				  PrintFlag = true;
          
				// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
				// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
				// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
				// PARTICULAR PURPOSE.
				//
				// Copyright (c) Microsoft Corporation. All rights reserved

				//
				//   FUNCTION: CaptureAnImage(HWND hWnd)
				//
				//   PURPOSE: Captures a screenshot into a window and then saves it in a .bmp file.
				//
				//   COMMENTS: 
				//
				//      Note: This sample will attempt to create a file called captureqwsx.bmp 
				//        

				HDC hdcSource;
				HDC hdcWindow;
				HDC hdcMemDC = NULL;
				HBITMAP hbmSource = NULL;
				BITMAP bmpSource;

				// Retrieve the handle to a display device context for the client area of the window. 
				hdcSource = GetDC(NULL);
				hdcWindow = GetDC(hWnd);

				// Create a compatible DC which is used in a BitBlt from the window DC
				hdcMemDC = CreateCompatibleDC(hdcWindow); 

				if(!hdcMemDC)
				{
					MessageBox(hWnd, L"CreateCompatibleDC has failed",L"Failed", MB_OK);
					break;
				}
						    
				// Get the client area for size calculation
				RECT rcClient;
				//	Get the windows coordinates for the Window, hWnd
				::GetWindowRect (hWnd,&rcClient); 
				//	Get the dimensions of the target image handle, hWnd
				RECT rc;
				::GetWindowRect (hWnd,&rc); 
				//This is the best stretch mode
				SetStretchBltMode(hdcWindow,HALFTONE);

				//The source DC is the current window and the destination DC is the current window (HWND)
				if(!StretchBlt(hdcWindow, 
							0,0,
							rcClient.right-rcClient.left, rcClient.bottom-rcClient.top,
							hdcSource, 
							rc.left+8, rc.top+50,	//	Adjustments to remove menu duplicate
							rc.right-rc.left, rc.bottom-rc.top, 
							SRCCOPY))
				{
					MessageBox(hWnd, L"StretchBlt has failed",L"Failed", MB_OK);
					break;
				}
    
				// Create a compatible bitmap from the Window DC
				hbmSource = CreateCompatibleBitmap(hdcWindow, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top);
    
				if(!hbmSource)
				{
					MessageBox(hWnd, L"CreateCompatibleBitmap Failed",L"Failed", MB_OK);
					break;
				}

				// Select the compatible bitmap into the compatible memory DC.
				SelectObject(hdcMemDC,hbmSource);
    
				// Bit block transfer into our compatible memory DC.
				if(!BitBlt(hdcMemDC, 
							0,0,
							rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, 
							hdcSource,
							rcClient.left,rcClient.top,
							SRCCOPY))
				{
					MessageBox(hWnd, L"BitBlt has failed", L"Failed", MB_OK);
					break;
				}

				// Get the BITMAP from the HBITMAP
				GetObject(hbmSource,sizeof(BITMAP),&bmpSource);
     
				BITMAPFILEHEADER   bmfHeader = {0};    
				BITMAPINFOHEADER   bi = {0};
     
				bi.biSize = sizeof(BITMAPINFOHEADER);    
				bi.biWidth = bmpSource.bmWidth;    
				bi.biHeight = bmpSource.bmHeight;  
				bi.biPlanes = 1;    
				bi.biBitCount = 32;    
				bi.biCompression = BI_RGB;    
				bi.biSizeImage = 0;  
				bi.biXPelsPerMeter = 0;    
				bi.biYPelsPerMeter = 0;    
				bi.biClrUsed = 0;    
				bi.biClrImportant = 0;

				DWORD dwBmpSize = ((bmpSource.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpSource.bmHeight;

				// Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
				// call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
				// have greater overhead than HeapAlloc.
				HANDLE hDIB = GlobalAlloc(GHND,dwBmpSize); 
				char *lpbitmap = (char *)GlobalLock(hDIB);    

				// Gets the "bits" from the bitmap and copies them into a buffer 
				// which is pointed to by lpbitmap.
				GetDIBits(hdcWindow, hbmSource, 0,
					(UINT)bmpSource.bmHeight,
					lpbitmap,
					(BITMAPINFO *)&bi, DIB_RGB_COLORS);

				//	If request is to save or print an image file, request name and save it.
				if (SaveFlag || PrintFlag)
				{

					//	Let's get the output file name from the user.
				    TCHAR szTempPath[MAX_PATH], szTempName[MAX_PATH];
					TCHAR szFileName[MAX_PATH] = {NULL};
					if (PrintFlag)
					{
					    GetTempPath(NUMCHARS(szTempPath),szTempPath);	//	Get the TEMP folder path
						swprintf(szTempName,NUMCHARS(szTempName), L"%sTmpFile.bmp", szTempPath);
					}
					else
					{

						wcscat(szFileName,L"Picture.jpg");
						while (true)
						{
							if(ArtGetSaveFileName(hWnd, L"Choose Filename to save as a JPEG File.", szFileName, NUMCHARS(szFileName),L"JPG Files (*.jpg)\0*.JPG\0\0"))
							{
								// let's make sure there's a .jpg suffix on that bugger.
								if(!str_ends_with(szFileName,L".jpg") && !str_ends_with(szFileName,L".JPG"))
								{
									wcsncat(szFileName,L".jpg", NUMCHARS(szFileName));
								}
								const bool fFileIsNew = !DoesFileExist(szFileName);
								if(fFileIsNew)
								{
								  break;	//	Exit loop, as file name is valid and new
								}
								else
								{
									DWORD dwRet = MessageBox(NULL,L"A file already exists with that name.\n\nAre you sure you want to overwrite it?",L"WARNING", MB_APPLMODAL | MB_ICONWARNING | MB_YESNO | MB_TOPMOST | MB_DEFBUTTON2);
									if (dwRet == IDYES)
									{
										break;	//	User wants to overwrite file, so exit loop and proceed
									}
								}
							}
							else
							{
								return 0;	//	User cancelled the save operation, so leave subroutine
							}
						}

						// Create a temporary BMP file, this is where we will save the screen capture.
						swprintf(szTempName, NUMCHARS(szTempName), L"%s.bmp", szFileName);
					}
					// Add the size of the headers to the size of the bitmap to get the total file size
					DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
 
					//Offset to where the actual bitmap bits start.
					bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER); 
    
					//Size of the file
					bmfHeader.bfSize = dwSizeofDIB; 
    
					//bfType must always be BM for Bitmaps
					bmfHeader.bfType = 0x4D42; //BM   

					//	Open a handle to the TEMP BMP file and write the DIB to it
					HANDLE hFile = CreateFile(szTempName,
						GENERIC_WRITE,
						0,
						NULL,
						CREATE_ALWAYS,
						FILE_ATTRIBUTE_NORMAL, NULL);   

					DWORD dwBytesWritten = 0;
					WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
					WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
					WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);
    
					//Close the handle for the file that was created
					CloseHandle(hFile);

					//Unlock and Free the DIB from the heap
//					GlobalUnlock(hDIB);    
//					GlobalFree(hDIB);

					//	Now let's convert this Bitmap into a JPEG file, if the user wants to save it.
					if (SaveFlag)
					{
						//	Load the BMP from a temporary file on the disk, and convert it
						  jpge::params params;
						  params.m_no_chroma_discrim_flag = 0;
						  params.m_quality = 95;
						  params.m_subsampling = jpge::subsampling_t::H2V2;
						  params.m_two_pass_flag = 0;

						  wstring strW(szFileName); // unicode -> const char* conversion
						  string strA(strW.begin(),strW.end()); // unicode -> const char* conversion

						  jpge::compress_image_to_jpeg_file(strA.c_str(),bi.biWidth,-bi.biHeight,4,(const jpge::uint8*)lpbitmap,params);

						  DeleteFile(szTempName);	//	Remove the TEMP file
						  //Unlock and Free the DIB from the heap
						  GlobalUnlock(hDIB);    
						  GlobalFree(hDIB);
					}
					else if (PrintFlag)
					{
						//Unlock and Free the DIB from the heap
						GlobalUnlock(hDIB);    
						GlobalFree(hDIB);

						////////////////////////////////////////////////////////////////////////////////
						cxsize=0, cxpage=0;
						cysize=0, cypage=0;
						PAINTSTRUCT ps;

						//	Let's get the Bitmap image for printing
						{
							ZeroMemory(&hBitmap, sizeof(HBITMAP));

							hBitmap = (HBITMAP)LoadImage(NULL,szTempName,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION|LR_DEFAULTSIZE|LR_LOADFROMFILE|LR_VGACOLOR);
							if(hBitmap)
							{
								cxpage = GetDeviceCaps (hdc, HORZRES);
								cypage = GetDeviceCaps (hdc, VERTRES);
								GetObject(hBitmap,sizeof(BITMAP),&bitmap);
								bxWidth = bitmap.bmWidth;
								bxHeight = bitmap.bmHeight;
 							}
				
							//	Let's paint the image into a Device Context
							hdc = BeginPaint(hWnd, &ps);
							hdcMem = CreateCompatibleDC(hdc);
							SelectObject(hdcMem, hBitmap);
							SetMapMode (hdc, MM_ISOTROPIC);
							SetWindowExtEx(hdc, cxpage,cypage, NULL);
							SetViewportExtEx(hdc, cxsize, cysize,NULL);
							SetViewportOrgEx(hdc, 0, 0, NULL);
							SetStretchBltMode(hdc,COLORONCOLOR);
							StretchBlt(hdc, 0, 0, bxWidth, bxHeight, hdcMem, 0, 0,bxWidth,bxHeight, SRCCOPY);

							EndPaint(hWnd, &ps);
							DeleteDC(hdcMem);

							//	Now let's print the  loaded image
							DOCINFO di= { sizeof (DOCINFO), TEXT ("Printing Picture...")};
							HDC prn = NULL;
							//	Open up the standard printer dialog and get our printer DC
							prn = GetPrinterDC(hWnd);

							//	Let's set up the printer for Landscape Mode printing
//							TCHAR pDevice[MAX_PATH];
//							swprintf(pDevice, NUMCHARS(pDevice), L"PrinterName, Job 0001");
//							GetLandscapeDevMode(hWnd, pDevice, (HANDLE)prn);

							if (prn)
							{
								cxpage = GetDeviceCaps (prn, HORZRES);
								cypage = GetDeviceCaps (prn, VERTRES);
								hdcMem = CreateCompatibleDC(prn);
								HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hBitmap);

								StartDoc (prn, &di);
								StartPage (prn) ;
								SetMapMode (prn, MM_ISOTROPIC);
								SetWindowExtEx(prn, cxpage,cypage, NULL);
								SetViewportExtEx(prn, cxpage, cypage,NULL);

								SetViewportOrgEx(prn, 0, 0, NULL);
								StretchBlt(prn, 0, 0, cxpage, cypage, hdcMem, 0, 0,bxWidth,bxHeight, SRCCOPY);
								EndPage (prn);
								EndDoc(prn);
								DeleteDC(prn);
								SelectObject(hdcMem, hbmOld);
								DeleteDC(hdcMem);
							}
							DeleteFile(szTempName);
							////////////////////////////////////////////////////////////////////////////////
						  }
					}
				}
				else
				{
					//	User wants to copy image to Clipboard.

					//Unlock and Free the DIB from the heap
					GlobalUnlock(hDIB);    
					GlobalFree(hDIB);

					// Add the size of the headers to the size of the bitmap to get the total DIB size
					DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
 
					//Offset to where the actual bitmap bits start.
					bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER); 
    
					//Size of the file
					bmfHeader.bfSize = dwSizeofDIB; 
    
					//bfType must always be BM for Bitmaps
					bmfHeader.bfType = 0x4D42; //BM   

					//	lpbitmap is the pointer to the BMP byte array
					{
						char* bitmapBuffer = lpbitmap;
						size_t buflen = dwSizeofDIB;
//						size_t buflen = dwBmpSize;

						copyBitmapToClipboard(bitmapBuffer, buflen);

//						DWORD dwBytesWritten = 0;
//						WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
//						WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
//						WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

					}
				}
				//Unlock and Free the DIB from the heap
				GlobalUnlock(hDIB);    
				GlobalFree(hDIB);
				DeleteObject(hbmSource);
				DeleteObject(hdcMemDC);
				ReleaseDC(NULL,hdcSource);
				ReleaseDC(hWnd,hdcWindow);
				UpdateUI(UPDATE_ALL);
//			    InitAxes(setSelectedChannels);
				return TRUE;
          }
		  case ID_FILE_EXIT:
          {
				DestroyWindow(hWnd);
				break;
		  }
		  case ID_TIMINGSCORING:
          {
				//	Let's set up for displaying the T&S page
				int m_RaceId[50] = {NULL};
				// Show the race-selection dialog and let the User pick which ones to use on T&S page
				static bool bSelectSessions;
				if (!bSelectSessions)	//	Prevent duplicate windows
				{
					bSelectSessions = true;
					SELECTSESSIONS_RESULT sfResult;
					CDlgSelectSessions dlgRace(g_pLapDB, &sfResult);
					ArtShowDialog<IDD_SELECTSESSIONS>(&dlgRace);
					if( !sfResult.fCancelled )	//	Allow the user to show the menu and start a race, even if no data has been collected from the racers yet
					{
						// Now display the T&S page and pass these RaceID's to this class
						TS_RESULT ts_sfResult;
						CDlgTimingScoring dlgTS(g_pLapDB, &ts_sfResult, m_szPath, &sfResult);
						ArtShowDialog<IDD_TIMINGSCORING>(&dlgTS);
					}
					bSelectSessions = false;
				}
				return TRUE;
		  }
		  case ID_DATA_OPENDB:
          {
            TCHAR szFilename[MAX_PATH];
            if(ArtGetOpenFileName(hWnd, L"Choose WFLP file", szFilename, NUMCHARS(szFilename),L"WifiLapper Files (*.wflp)\0*.WFLP\0\0"))
            {
              if(g_pLapDB->Init(szFilename))
              {
                _snwprintf(m_szPath, NUMCHARS(m_szPath), szFilename);
				RACESELECT_RESULT sfResult;

				//	Zero out the Race ID's before selecting them
				for (int z = 0; z < 50; z++)
				{
					m_iRaceId[z] = -1;	
					sfResult.iRaceId[z] = -1;
				}

                CRaceSelectDlg dlgRace(g_pLapDB, &sfResult);
                ArtShowDialog<IDD_SELECTRACE>(&dlgRace);

                if(!sfResult.fCancelled)
                {
				  for (int z = 0; z < 50; z++)
				  {
					m_iRaceId[z] = sfResult.iRaceId[z];	//	Load all of the race sessions chosen
				  }
                  ClearUILaps();
                  LoadLaps(g_pLapDB);
                  UpdateUI(UPDATE_ALL);
				  //	Let's reset the timer
				  tmLast = timeGetTime();	//	Save last time lap was received

				  // Find all of the Names of the selected Race Sessions for addition to title bar
				  vector<RACEDATA> lstRaces = g_pLapDB->GetRaces();
				  TCHAR lstSessions[MAX_PATH] = {NULL};
				  for(int x = 0;x < lstRaces.size(); x++)
				  {
					for (int z = 0; z < 50; z++)
					{
						if ( m_iRaceId[z] == lstRaces[x].raceId )
						{
							_snwprintf(lstSessions, NUMCHARS(lstSessions), L"%s | %s", lstSessions, lstRaces[x].strName.c_str());
							break;
						}
					}
				  }
				  TCHAR szTemp[MAX_PATH] = {NULL};
				  _snwprintf(szTemp, NUMCHARS(szTemp), L"Pitside - %s%s", m_szPath, lstSessions);
				  SetWindowText(hWnd, szTemp);	//	Change the title bar to show the file name opened
                }
              }
            }
            return TRUE;
          }
          case ID_DATA_DASHWARE:	//	Save the data to a .CSV file
          {
            set<LPARAM> setSelectedData = m_sfLapList.GetSelectedItemsData3();
            if(setSelectedData.size() > 0)
            {
			  TCHAR szFilename[MAX_PATH] = {NULL};
			  wcscat(szFilename,L"Export.csv");
			  while (true)
			  {
				if(ArtGetSaveFileName(hWnd, L"Choose Output file", szFilename, NUMCHARS(szFilename),L"CSV Files (*.csv)\0*.CSV\0\0"))
				{
					// let's make sure there's a .csv suffix on that bugger.
					if(!str_ends_with(szFilename,L".csv") && !str_ends_with(szFilename,L".CSV"))
					{
						wcsncat(szFilename,L".csv", NUMCHARS(szFilename));
					}
					const bool fFileIsNew = !DoesFileExist(szFilename);
					if(fFileIsNew)
					{
						break;	//	Exit loop, as file name is valid and new
					}
					else
					{
						DWORD dwRet = MessageBox(NULL,L"A file already exists with that name.\n\nAre you sure you want to overwrite it?",L"WARNING", MB_APPLMODAL | MB_ICONWARNING | MB_YESNO | MB_TOPMOST | MB_DEFBUTTON2);
						if (dwRet == IDYES)
						{
							break;	//	User wants to overwrite file, so exit loop and proceed
						}
					}
				}
				else
				{
					return 0;	//	User cancelled the save operation, so leave subroutine
				}
			  }

				//	Display the "Working...." dialog, as this is going to take some time.
				DLGPROC working = NULL;
				HWND hwndGoto = NULL;  // Window handle of dialog box  
				if (!IsWindow(hwndGoto)) 
				{ 
					hwndGoto = CreateDialog(NULL, MAKEINTRESOURCE (IDD_PROGRESS), hWnd, working); 
					ShowWindow(hwndGoto, SW_SHOW); 
				} 

				vector<const ILap*> lstLaps;
				vector <CExtendedLap*> lstLaps1 = GetLapsToShow();
                for(set<LPARAM>::iterator i = setSelectedData.begin(); i != setSelectedData.end(); i++)
                {
                  CExtendedLap* pLap = (CExtendedLap*)*i;
                  lstLaps.push_back(pLap->GetLap());
                }
				DashWare::SaveToDashware(szFilename, lstLaps, lstLaps1);	//	lstLaps does not have the reference lap in it, lstLaps1 may have it
				DestroyWindow(hwndGoto); //	Close the "Working..." dialog
                hwndGoto = NULL; 
            }
            else
            {
              MessageBox(NULL,L"Please Select Laps",L"You must select some laps first!",MB_OK | MB_ICONWARNING);
            }
            return TRUE;
          }
          case IDC_DISPLAYTYPE_LINE:
          {
            switch(HIWORD(wParam))
            {
            case BN_CLICKED:
              m_eLapDisplayStyle = LAPDISPLAYSTYLE_MAP;
              UpdateUI(UPDATE_MAP | UPDATE_DASHBOARD);
              break;
            }
            return TRUE;
          }
/*          case IDC_DISPLAYTYPE_RECEPTION:
          {
            switch(HIWORD(wParam))
            {
              case BN_CLICKED:
                m_eLapDisplayStyle = LAPDISPLAYSTYLE_RECEPTION;
                UpdateUI(UPDATE_MAP | UPDATE_DASHBOARD | UPDATE_VALUES);
                break;
            }
            return TRUE;
          }
*/          case IDC_DISPLAYTYPE_PLOT:
          {
            switch(HIWORD(wParam))
            {
              case BN_CLICKED:
                m_eLapDisplayStyle = LAPDISPLAYSTYLE_PLOT;
                UpdateUI(UPDATE_MAP | UPDATE_DASHBOARD);
                break;
            }
            return TRUE;
          }
          case IDC_SETREFERENCE:
          {
            // they want to set a given lap as a reference lap
            set<LPARAM> setSelected = m_sfLapList.GetSelectedItemsData3();
            if(setSelected.size() == 1)
            {
              CExtendedLap* pNewRefLap = (CExtendedLap*)*(setSelected.begin());
              pNewRefLap->ComputeDistances(NULL, g_pLapDB);
              for(map<int,CExtendedLap*>::iterator i = m_mapLaps.begin(); i != m_mapLaps.end(); i++)
              {
                if(i->second != pNewRefLap)
                {
                  i->second->ComputeDistances(pNewRefLap, g_pLapDB); // tell this lap to recompute using pLap as the reference lap
                }
              }
              m_pReferenceLap = pNewRefLap;
            }
            else
            {
              // what's going on?  This should've been disabled
            }
            UpdateUI(UPDATE_MAP | UPDATE_DASHBOARD | UPDATE_VALUES);
            return TRUE;
          }
           case IDC_LIVEDATA:
          {
            // Let's toggle all live data collection threads on/off to allow for lap analysis
            switch(HIWORD(wParam))
            {
            case BN_CLICKED:
				{
					b_Button_LiveData = !b_Button_LiveData;
					m_sfLapOpts.b_Button_LiveData = b_Button_LiveData;
					UpdateUI(UPDATE_DASHBOARD); // Redraw the laplist and buttons
	              break;
				}
            }
            return TRUE;
          }
         case IDC_SETDRIVER: // they want to set the driver of the selected laps
          {
            switch(HIWORD(wParam))
            {
            case BN_CLICKED:
              ApplyDriverNameToSelectedLaps(::g_pLapDB);
              UpdateUI(UPDATE_LIST | UPDATE_DASHBOARD); // Redraw the laplist and buttons
              break;
            }
            return TRUE;
          }
          case IDC_CLEARCOMMENT:	//	This button clears the comment field
          {
            switch(HIWORD(wParam))
            {
				case BN_CLICKED:
				  m_szCommentText[0] = '\0';
				  SendMessage(GetDlgItem(m_hWnd, IDC_COMMENTTEXT), WM_SETTEXT, NUMCHARS(m_szCommentText), (LPARAM)m_szCommentText);
//				  UpdateUI(UPDATE_LIST | UPDATE_DASHBOARD);
				  break;
            }
            return TRUE;
          }
          case IDC_COMMENTTEXT:
          {
            switch(HIWORD(wParam))
            {
              case EN_CHANGE:
              {
                SendMessage(GetDlgItem(m_hWnd, IDC_COMMENTTEXT), WM_GETTEXT, NUMCHARS(m_szCommentText), (LPARAM)m_szCommentText);
//                UpdateUI(UPDATE_LIST | UPDATE_DASHBOARD);
                break;
              }
            }
            return TRUE;
		  }
        } // end switch for finding out what control WM_COMMAND hit
        break; // break out of WM_COMMAND handling
      }	//	End of COMMAND loop
      case WM_UPDATEUI:
      {
        DWORD dwCurrentUpdate = m_fdwUpdateNeeded;
        UpdateUI_Internal(dwCurrentUpdate);
        m_fdwUpdateNeeded &= ~dwCurrentUpdate;
        return TRUE;
      }
      case WM_NOTIFYUPDATE:
      {
        switch(wParam)
        {
        case NOTIFY_NEEDRECVCONFIG:
        {
          // a lap has been received, but the database isn't set up to receive (aka it's pointing at a high-quality race).
          // let's tell the user about it and ask them for input.
          static bool fWarnedOnce = false;
          if(!fWarnedOnce)
          {
            fWarnedOnce = true;
            MessageBox(NULL,L"You just received a lap from a car, but you're looking at old data.  Hit data->'new race session' or else Pitside will keep ignoring it",L"Not ready to receive",0);
          }
          return TRUE;
        }
        case NOTIFY_NEWLAP:
        {
          ILapReceiver* pLapDB = (ILapReceiver*)lParam;
          int iLastRaceId = pLapDB->GetLastReceivedRaceId();
          if(m_iRaceId[0] < 0 || pLapDB->GetLapCount(m_iRaceId[0]) <= 0 || // if we don't have a race or our current race has no laps (aka sucks)
            (pLapDB->IsActivelyReceiving(iLastRaceId) && !pLapDB->IsActivelyReceiving(m_iRaceId[0]))) // or if the new race ID is receiving and the current race ID isn't...
          {
            m_iRaceId[0] = pLapDB->GetLastReceivedRaceId(); // since we just got told there's a new lap, there must be a last-received-race
            ClearUILaps();
            LoadLaps(g_pLapDB);
            UpdateUI(UPDATE_ALL);
          }
          else
          {
            LoadLaps((ILapReceiver*)lParam);
			UpdateUI(UPDATE_ADD2LIST);	// Add the recently received lap to the laplist window
          }
		  //	Just loaded a new lap. Let's reset the timer
		  tmLast = timeGetTime();	//	Save last time lap was received
          return TRUE;
        }
        case NOTIFY_NEWDATABASE:
        {
          // a new database has shown up!

          // first, check if the user cares...
          DWORD dwRet = MessageBox(NULL,L"A new database has been sent from a phone.  Do you want to load it?",L"New database",MB_YESNO);
          if(dwRet == IDYES)
          {
            LPCTSTR lpszFilename = (LPCTSTR)lParam;
            if(g_pLapDB->Init(lpszFilename))
            {
              RACESELECT_RESULT sfResult;

			  //	Zero out the Race ID's before selecting them
			  for (int z = 0; z < 50; z++)
			  {
				  m_iRaceId[z] = -1;	
				  sfResult.iRaceId[z] = -1;
			  }

              CRaceSelectDlg dlgRace(g_pLapDB, &sfResult);
              ArtShowDialog<IDD_SELECTRACE>(&dlgRace);
              if(!sfResult.fCancelled)
              {
				for (int z = 0; z < 50; z++)
				{
					m_iRaceId[z] = sfResult.iRaceId[z];	//	Load all of the race sessions chosen
				}
                
                ClearUILaps();
				LoadLaps(g_pLapDB);
                UpdateUI(UPDATE_ALL);
              }
            }
		  //	Let's reset the timer
		  tmLast = timeGetTime();	//	Save last time lap was received
          }
          return TRUE;
          
        }
        case NOTIFY_NEWNETSTATUS:
        {
          ILapReceiver* pLaps = (ILapReceiver*)lParam;
          SendMessage(m_hWnd, WM_SETTEXT, 0, (LPARAM)pLaps->GetNetStatus(NETSTATUS_STATUS));

          TCHAR szTemp[512];
//          HWND hWndIp = GetDlgItem(m_hWnd, IDC_CURRENTIP);
//          swprintf(szTemp, NUMCHARS(szTemp), L"PC: %s", pLaps->GetNetStatus(NETSTATUS_THISIP));
//          SendMessage(hWndIp, WM_SETTEXT, 0, (LPARAM)szTemp);

          HWND hWndRemoteIp = GetDlgItem(m_hWnd, IDC_CURRENTREMOTEIP);
          swprintf(szTemp, NUMCHARS(szTemp), L"Phone: %s", pLaps->GetNetStatus(NETSTATUS_REMOTEIP));
          SendMessage(hWndRemoteIp, WM_SETTEXT, 0, (LPARAM)szTemp);
          return TRUE;
        }
        case NOTIFY_NEWMSGDATA:
        {
          CMsgThread* pThd = (CMsgThread*)lParam;
          pThd->GetStatusMessage(m_szMessageStatus, NUMCHARS(m_szMessageStatus));
          UpdateUI(UPDATE_DASHBOARD);
          return TRUE;
        }
      }
        
      return 0;
    }
    case WM_RBUTTONUP:
    {
		m_sfLapOpts.flWindowShiftX = 0;
		m_sfLapOpts.flWindowShiftY = 0;
		m_sfLapOpts.iZoomLevels = 0;
		UpdateUI(UPDATE_MAP);
		return TRUE;
    }
    case WM_PAINT:
    {
		UpdateDisplays();
		return FALSE;
    }
    case WM_SIZE:
    {
		SIZE sNewSize;
		sNewSize.cx = LOWORD(lParam);
		sNewSize.cy = HIWORD(lParam);
		HandleResize(sNewSize);
		return TRUE;
    }
  }

  return FALSE;
}
  DWORD GetDlgId() const {return IDD_DLGFIRST;}

  const static DWORD UPDATE_MAP = 0x1;
  const static DWORD UPDATE_LIST = 0x2;
  const static DWORD UPDATE_DASHBOARD = 0x4;
  const static DWORD UPDATE_MENU = 0x8;
  const static DWORD UPDATE_VALUES = 0x10;
  const static DWORD UPDATE_TRACTIONCIRCLE = 0x20;
  const static DWORD UPDATE_ALLDATA = 0x40;

  const static DWORD UPDATE_ALL = 0xffff;
  const static DWORD UPDATE_ADD2LIST = 0x10000;

  //	Pull in PlotPrefs array as well as lines vs. dots and Painting color scheme settings from Settings.txt file
  void SetDisplayOptions(const LAPSUPPLIEROPTIONS& lapOpts)
  {
    m_sfLapOpts = lapOpts;
  }
  void SetDBPath(const TCHAR szPath[MAX_PATH])
  {
	  _snwprintf(m_szPath, NUMCHARS(m_szPath), szPath);
  }

  void UpdateUI(DWORD fdwUpdateFlags)
  {
    m_fdwUpdateNeeded|= fdwUpdateFlags;
    PostMessage(m_hWnd,WM_UPDATEUI,0,0);
  }


  void UpdateUI_Internal(DWORD fdwUpdateFlags)
  {
    set<LPARAM> setSelectedData = m_sfLapList.GetSelectedItemsData3();
	vector<CExtendedLap*> laps = GetSortedLaps(m_sfLapOpts.eSortPreference); // translates our m_mapLaps into a vector sorted by time
	// do some memory cleanup
	for(int x = 0;x < laps.size(); x++)
	{
		if(setSelectedData.find((LPARAM)laps[x]) != setSelectedData.end() || laps[x] == m_pReferenceLap)
		{
		// this lap is still selected, as it is in the set of selected items
		}
		else
		{
		// this lap is not selected.  we should compact it so that it doesn't gobble memory.
		laps[x]->Compact();
		}
	}

    if(IS_FLAG_SET(fdwUpdateFlags, UPDATE_LIST))	// rebuild the laplist from scratch
    {
		m_sfLapList.Clear();
		for(int x = 0;x < laps.size(); x++)
		{
			vector<wstring> lstStrings;
			laps[x]->GetStrings(lstStrings);
			m_sfLapList.AddStrings(lstStrings, (LPARAM)laps[x]);
		}

		m_sfLapList.SetSelectedData(setSelectedData);
		if(laps.size() > 0)
		{
			m_sfLapList.MakeVisible((LPARAM)laps[laps.size()-1]);	//	Always set the last lap as visible in the Lap List
		}
    }

	if(IS_FLAG_SET(fdwUpdateFlags, UPDATE_ADD2LIST))
    {
		int iLapId;
		int x;
		const ILap *iLap;

		// Loop through the list of laps to add, making sure avoid duplicates.  Find the row to add it
		while( m_sfLapList.GetCount() < laps.size() && (iLap = g_pLapDB->GetLastLap()) != NULL )  {
			iLapId = iLap->GetLapId();

			// Find which row this lap needs to appear
			for(x = 0;x < laps.size(); x++) {
				if( laps[x]->GetLap()->GetLapId() == iLapId ) break;
			}
			if(x < laps.size())
			{
				// x is the index which matches our most recent lap.  We need to insert this one into m_sfLapList
				vector<wstring> lstStrings;
				laps[x]->GetStrings(lstStrings);
				m_sfLapList.InsertStrings(lstStrings, (LPARAM)laps[x], x);
				m_sfLapList.MakeVisible((LPARAM)laps[x]);	//	Always set the last lap as visible in the Lap List
			}
			else 
				DASSERT(1);// we didn't find the lap ID from the list of laps to add in the laps vector !!
		}
		m_sfLapList.SetSelectedData(setSelectedData);
    }

	if(IS_FLAG_SET(fdwUpdateFlags, UPDATE_VALUES))
    {
      UpdateValues();
	  UpdateSectors();
    }
    if(IS_FLAG_SET(fdwUpdateFlags, UPDATE_MAP))
    {
      UpdateDisplays();
    }
    if(IS_FLAG_SET(fdwUpdateFlags, UPDATE_MENU))
    {
      UpdateMenus();
    }
    if(IS_FLAG_SET(fdwUpdateFlags, UPDATE_ALLDATA))
    {
      UpdateAllData();
    }
	
    if(IS_FLAG_SET(fdwUpdateFlags, UPDATE_DASHBOARD))
    {
      set<DATA_CHANNEL> setSelectedChannels;

      for(set<LPARAM>::const_iterator i = setSelectedData.begin(); i != setSelectedData.end(); i++)
      {
        CExtendedLap* pLap = (CExtendedLap*)(*i);
        set<DATA_CHANNEL> setLapData = pLap->GetAvailableChannels();
        for(set<DATA_CHANNEL>::iterator i = setLapData.begin(); i != setLapData.end(); i++)
        {
          setSelectedChannels.insert(*i);
        }
      }

      CASSERT(LAPDISPLAYSTYLE_COUNT == 4);
      switch(m_eLapDisplayStyle)
      {
		  case LAPDISPLAYSTYLE_MAP:            CheckRadioButton(m_hWnd, IDC_DISPLAYTYPE_LINE, IDC_DISPLAYTYPE_LAST, IDC_DISPLAYTYPE_LINE); break;
		  case LAPDISPLAYSTYLE_PLOT:           CheckRadioButton(m_hWnd, IDC_DISPLAYTYPE_LINE, IDC_DISPLAYTYPE_LAST, IDC_DISPLAYTYPE_PLOT); break;
		  case LAPDISPLAYSTYLE_RECEPTION:      CheckRadioButton(m_hWnd, IDC_DISPLAYTYPE_LINE, IDC_DISPLAYTYPE_LAST, IDC_DISPLAYTYPE_RECEPTION); break;
		  default: DASSERT(FALSE); break;
      }
    
      set<LPARAM> setXSelected,setYSelected;
      setXSelected.insert(m_eXChannel);
      m_sfXAxis.SetSelectedData(setXSelected);

      for(int x = 0; x < m_lstYChannels.size(); x++)
      {
        setYSelected.insert(m_lstYChannels[x]);
      }
      m_sfYAxis.SetSelectedData(setYSelected);

      HWND hDriverButton = GetDlgItem(m_hWnd, IDC_SETDRIVER);
      Button_Enable(hDriverButton, setSelectedData.size() > 0);
      HWND hReferenceButton = GetDlgItem(m_hWnd, IDC_SETREFERENCE);
      Button_Enable(hReferenceButton, setSelectedData.size() == 1);

      HWND hWndReference = GetDlgItem(m_hWnd, IDC_CURRENTREFERENCE);
      if(m_pReferenceLap)
      {
        TCHAR szRefString[512] = L"";
        TCHAR szLapString[512] = L"";
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
  }
private:
  void ClearUILaps()
  {
    m_mapLapHighlightTimes.clear();
    m_pReferenceLap = NULL;
    m_mapLaps.clear();
    m_sfLapList.Clear();
  }
   bool ShowHelp(HWND hWnd)
	{
		TCHAR lpOpen[MAX_PATH] = L"open";
		
		TCHAR lpFile[MAX_PATH] = L"PitsideHelp.pdf";
		TCHAR lpDir[MAX_PATH];
		if(GetAppFolder(lpDir,NUMCHARS(lpDir)))
		{
			//	Set up the Filename string for the Help PDF file.
			wcsncat(lpDir,L"", NUMCHARS(lpDir)-1);
		}
		else
		{
			// trouble.  just bail.
			return false;
		}
		int nShowCmd = SW_RESTORE;	//	Restore the Help document, if it is minimized or whatever.

		//	Shell to the Help PDF file
		HINSTANCE Check = ShellExecuteW(hWnd, lpOpen, lpFile, NULL, lpDir, nShowCmd);
		if ((int)Check <= 32)
          MessageBox(NULL, L"The Help file requires Acrobat Reader\n\nPlease install Reader and try again", L"Acrobat Reader Not Found", MB_OK);
		return true;
	}
   bool ShowWFLHelp(HWND hWnd)
	{
		TCHAR lpOpen[MAX_PATH] = L"open";
		
		TCHAR lpFile[MAX_PATH] = L"SpeedFreqHelp.pdf";
		TCHAR lpDir[MAX_PATH];
		if(GetAppFolder(lpDir,NUMCHARS(lpDir)))
		{
			//	Set up the Filename string for the Help PDF file.
			wcsncat(lpDir,L"", NUMCHARS(lpDir)-1);
		}
		else
		{
			// trouble.  just bail.
			return false;
		}
		int nShowCmd = SW_RESTORE;	//	Restore the Help document, if it is minimized or whatever.

		//	Shell to the Help PDF file
		HINSTANCE Check = ShellExecuteW(hWnd, lpOpen, lpFile, NULL, lpDir, nShowCmd);
		if ((int)Check <= 32)
          MessageBox(NULL, L"The Help file requires Acrobat Reader\n\nPlease install Reader and try again", L"Acrobat Reader Not Found", MB_OK);
		return true;
	}
  void ShowNetInfo()
  {
    while(true)
    {
      TCHAR szBuf[1000];
      szBuf[0] = '\0';
       _snwprintf(szBuf, NUMCHARS(szBuf), L"PC IP Address[es] are displayed below.  You will need one of these to put in the phone app.\n\n");

      IP_ADAPTER_INFO sfAdapter[255] = {0};
      ULONG cbAdapter = sizeof(sfAdapter);
      ULONG ret = GetAdaptersInfo(sfAdapter, &cbAdapter);
      int cConnected = 0;
      if(ret == NO_ERROR)
      {
        IP_ADAPTER_INFO* pInfo = &sfAdapter[0];
        while(pInfo)
        {
          TCHAR szLine[255];
          if(strcmp(pInfo->IpAddressList.IpAddress.String, "0.0.0.0") != 0)
          {
            _snwprintf(szLine, NUMCHARS(szLine), L"%S - %S\n", pInfo->Description, pInfo->IpAddressList.IpAddress.String);
            wcsncat(szBuf,szLine, NUMCHARS(szBuf));
            cConnected++;
          }

          pInfo = pInfo->Next;
        }

        if(cConnected > 0)
        {
          MessageBox(NULL, szBuf, L"Ip address info", MB_OK);
          return;
        }
        else
        {
          int iResult = MessageBox(NULL, L"WifiLapper Pitside cannot find any network devices connected.  Please connect to a network and try again.", L"Could not find IP addresses", MB_ABORTRETRYIGNORE | MB_ICONWARNING);
          if(iResult == IDABORT)
          {
            exit(0);
          }
          else if(iResult == IDRETRY)
          {
            // just let the loop roll
          }
          else if(iResult == IDIGNORE)
          {
            // ok, they just want to get to the app.
            return;
          }
        }
      }
    } // loop for retrying
  }
  void InitAxes(set<DATA_CHANNEL> setAvailable)
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
  }
  void InitBaseWindowPos()
  {
#define GET_WINDOWPOS(idc) \
    { \
      WINDOWPLACEMENT wp; \
      wp.length = sizeof(wp); \
      HWND hWnd = GetDlgItem(m_hWnd,idc); \
      GetWindowPlacement(hWnd, &wp); \
      m_baseWindowPos[idc] = wp.rcNormalPosition; \
    }
    WINDOWPLACEMENT wp;
    wp.length = sizeof(wp);
    HWND hWnd = m_hWnd;
    GetWindowPlacement(hWnd, &wp);
    m_baseWindowPos[IDD_DLGFIRST] = wp.rcNormalPosition;

    GET_WINDOWPOS(IDC_DISPLAY);
//	GET_WINDOWPOS(IDD_DLGSUBDISPLAY);
    GET_WINDOWPOS(IDC_SUBDISPLAY);
    GET_WINDOWPOS(IDC_LAPS);
    GET_WINDOWPOS(IDC_TRACTIONCIRCLEMAP);
	GET_WINDOWPOS(IDC_ALLDATADISPLAY);

  }
  void HandleCtlResize(SIZE sNewSize, int idc, bool fResizeX, bool fResizeY)
  {
    HWND hwndMainView = GetDlgItem(m_hWnd, idc);
    RECT rcBasePos = m_baseWindowPos[idc];
    SIZE sNewCtlSize = {fResizeX ? (sNewSize.cx - rcBasePos.left) : RECT_WIDTH(&rcBasePos), fResizeY ? (sNewSize.cy - rcBasePos.top) : RECT_HEIGHT(&rcBasePos)};
    MoveWindow(hwndMainView, rcBasePos.left, rcBasePos.top, sNewCtlSize.cx, sNewCtlSize.cy, TRUE);
  }
  void HandleResize(SIZE sNewSize)
  {
    HandleCtlResize(sNewSize, IDC_DISPLAY, true, true); // main display window
//    HandleCtlResize(sNewSize, IDD_DLGSUBDISPLAY, true, true); // sub display window
	HandleCtlResize(sNewSize, IDC_SUBDISPLAY, true, false); // sub display control
    HandleCtlResize(sNewSize, IDC_LAPS, false, true); // lap list
    HandleCtlResize(sNewSize, IDC_TRACTIONCIRCLEMAP, false, false); // Traction circle window
  }

  //	Function updates the data points in the window showing all of the data for the selected laps, activated by the middle mouse button
  void UpdateAllData()
  {
		//	hWnd_AllData is the handle for the window we want. Let's make sure that it is displayed
		//	Let's define some colors for the List Views to use
		COLORREF d_BlackColor = RGB(0, 0, 0);	//	Black
		COLORREF d_WhiteColor = RGB(255, 255, 255);	//	White
		COLORREF d_RedColor = RGB(230, 20, 20);	//	Red
		COLORREF d_GreenColor = RGB(20, 230, 20);	//	Green
		COLORREF d_LGreyColor = RGB(200, 200, 200);	//	Light Grey
		HDC AD_Rect = GetDC(AD_hWnd);	//	HDC for the display control

		if (GetDlgItem(hWnd_AllData, IDC_ALLDATADISPLAY))	//	Only execute data display functions if window is showing
		{
			//	First let's get all of the information about the data points for this lap/location
			ListView_DeleteAllItems(AD_hWnd);	//	Clear the list before displaying the update
			vector<CExtendedLap*> lstLaps = GetLapsToShow();	//	Load the CExtendedLap data for the lap list
			if(lstLaps.size() > 0)
			{
				const int cLaps = 10;	//	The maximum number of Laps to display Data channels for, limited by the window form
				int iLap = 0;	//	Tracking variable for which Lap we are on in the loop
				int iChannel = 0;	//	Tracking variable for which Y-Channel we are on within each lap
				int iTotChannel = 0;	//	The number of Y Data channels in the Listview
				//	Loop through each lap and pick up all of their values for each data channel
				for(vector<CExtendedLap*>::iterator a = lstLaps.begin(); a != lstLaps.end(); a++)
				{
					CExtendedLap* pLap = (CExtendedLap*)*a;
					//	For each lap let's get all of the available data channels for display
					int TotalYChannels = 0;	//	Total numbber of Data Channels availalbe for this lap
					int iLapId = pLap->GetLap()->GetLapId();
					set<DATA_CHANNEL> channels = pLap->GetAvailableChannels();	//	Get all of the data channels for this lap
					if ( !iTotChannel ) iTotChannel = channels.size();	//	Only update the first time through the LV creation loop
					for(set<DATA_CHANNEL>::const_iterator i = channels.begin(); i != channels.end(); i++) // Loop through them, insert them into our "all data channels" set
					{
						TCHAR m_szYString[512] = {NULL};
						TCHAR szLabel[50][MAX_PATH] = {NULL};	//	Assumes that we will have no more than 50 Y-Data channels
						TotalYChannels = channels.size();	//	The number of Data Channels available for this lap
						DATA_CHANNEL eChannel = *i;
						if( !eChannel ) continue;
						float flVal;
						//	Let's get the value for this data channel at this highlight point
						const IDataChannel* pChannel = pLap->GetChannel(eChannel);
						if (pChannel)	//	Check if pointer is valid
						{
							flVal = pChannel->GetValue(m_mapLapHighlightTimes[pLap]);	//	The value that we are looking for
				//////////////////////////////////////////
							//		Adding transformation functions here for Y
							if (m_sfLapOpts.m_PlotPrefs[iChannel].iTransformYesNo == true)
							{
								if (m_sfLapOpts.m_PlotPrefs[iChannel].fTransBValue < 0)
								{
									flVal = m_sfLapOpts.m_PlotPrefs[iChannel].fTransAValue + flVal * m_sfLapOpts.m_PlotPrefs[iChannel].fTransBValue + flVal * flVal *  m_sfLapOpts.m_PlotPrefs[iChannel].fTransCValue;
								}
								else
								{
									flVal = m_sfLapOpts.m_PlotPrefs[iChannel].fTransAValue + flVal * m_sfLapOpts.m_PlotPrefs[iChannel].fTransBValue + flVal * flVal *  m_sfLapOpts.m_PlotPrefs[iChannel].fTransCValue;
								}
							}
				//////////////////////////////////////////
							//	Now assign these values to the Data Value variable for display
							TCHAR szChannelName[MAX_PATH];
							GetDataChannelName(eChannel,szChannelName,NUMCHARS(szChannelName));

							char szVal[MAX_PATH];
							GetChannelValue(eChannel,m_sfLapOpts.eUnitPreference,flVal,szVal,NUMCHARS(szVal));	//	Value for data channels

							TCHAR szDate[100];
							pLap->GetString(szDate, 9); //   Truncated timestamp of this lap, to used to name it

							//	Let's load the Listview with this result
							TCHAR lstVal[MAX_PATH] ;
							swprintf(lstVal,NUMCHARS(lstVal), L"%S", szVal);
							wchar_t result[MAX_PATH] ;
							bool b_LV_ChannelFound = false;	//	ListView boolean for Data Channel searches
							//	First set up the Main listview item (Data Channel Name) for the first lap
							int i_TextResult = 0;	//	Data channel matching integer
							if (iLap == 0 && iChannel == 0 )
							{
								p_ADlvi.mask = LVIF_TEXT | LVIF_PARAM;
								p_ADlvi.iItem = 0;	//	Which Data Channel subscript
								p_ADlvi.iSubItem = 0;	//	Which Lap subscript
								p_ADlvi.lParam = 0;
								swprintf(result,NUMCHARS(result), L"Time");
								p_ADlvi.pszText = result;
								p_ADlvi.cchTextMax = wcslen(result);
								//	ListView_InsertItem(AD_hWnd, &p_ADlvi);	//	For the first item line let's use the Lap Start Time for Identification
								SendMessage(AD_hWnd, LVM_INSERTITEM, 0, (LPARAM)&p_ADlvi);

								p_ADlvi.mask = LVIF_TEXT | LVIF_PARAM;
								p_ADlvi.iItem = iChannel + 1;	//	Which Data Channel subscript
								p_ADlvi.iSubItem = 0;	//	Which Lap subscript
								p_ADlvi.lParam = 0;
								swprintf(result,NUMCHARS(result), szChannelName);
								p_ADlvi.pszText = result;
								p_ADlvi.cchTextMax = wcslen(result);
								//	ListView_InsertItem(AD_hWnd, &p_ADlvi);	//	Put in the Data Channel name next
								SendMessage(AD_hWnd, LVM_INSERTITEM, 0, (LPARAM)&p_ADlvi);
								b_LV_ChannelFound = true;
							}
							else if (iLap == 0 && !iChannel == 0 )
							{
								p_ADlvi.mask = LVIF_TEXT | LVIF_PARAM;
								p_ADlvi.iItem = iChannel + 1;	//	Which Data Channel subscript
								p_ADlvi.iSubItem = 0;	//	Which Lap subscript
								p_ADlvi.lParam = 0;
								swprintf(result,NUMCHARS(result), szChannelName);
								p_ADlvi.pszText = result;
								p_ADlvi.cchTextMax = wcslen(result);
								//	ListView_InsertItem(AD_hWnd, &p_ADlvi);	//	Put in the Data Channel name next
								SendMessage(AD_hWnd, LVM_INSERTITEM, 0, (LPARAM)&p_ADlvi);
								b_LV_ChannelFound = true;
							}
							else	//	Let's match up the Data Channel Name with those in the first lap for displaying in-line
							{
								//	First let's get the channel name for this data channel
								wchar_t result2[MAX_PATH];
								p_ADlvi.iItem = iChannel + 1;	//	The first lap's data channel name
								p_ADlvi.iSubItem = 0;	//	The first lap's subscript
								p_ADlvi.lParam = 0;
								p_ADlvi.mask = LVIF_TEXT;
								p_ADlvi.cchTextMax = MAX_PATH;
								ListView_GetItem(AD_hWnd, &p_ADlvi);

								swprintf(result2,NUMCHARS(szChannelName), szChannelName);	//	szChannelName is the current Data Channel name we want to add the value to the LV
								i_TextResult = wcscmp(p_ADlvi.pszText, result2);	//	Compare the wide strings to see if they match
								if ( i_TextResult )	//	0 if they match, <>0 if they are different
								{
									//	Items don't match. Let's see if the data channel exists in the array
									for (int chan = 0; chan < iTotChannel; chan++)
									{
										//	First let's get the channel name for this data channel
										p_ADlvi.iItem = chan + 1;	//	The first lap's data channel name
										p_ADlvi.iSubItem = 0;	//	The first lap's subscript
										p_ADlvi.lParam = 0;
										p_ADlvi.mask = LVIF_TEXT;
										p_ADlvi.cchTextMax = MAX_PATH;

										if (ListView_GetItem(AD_hWnd, &p_ADlvi) )	//	Returns TRUE if it gets an item
										{
											i_TextResult = wcscmp((LPWSTR)p_ADlvi.pszText, result2);	//	Compare the wide strings to see if they match
											if ( !i_TextResult )	//	0 if they match, <>0 if they are different. Break loop if we found a match
											{
												iChannel = chan;	//	Set the LV Item subsript to the channel's number
												b_LV_ChannelFound = true;	//	Set the "Channel Found" flag
												break;
											}
										}
									}	//	End of Data Channel searching loop
									if ( b_LV_ChannelFound )	//	Data Channel was found, now add the value to it
									{
										//	Now populate the listview subitem with the datapoint value, if it's valid
										p_ADlvi.mask = LVIF_TEXT;
										p_ADlvi.iItem = iChannel + 1;	//	Which Data Channel subscript, +1 for "Lap" row
										p_ADlvi.iSubItem = iLap + 1;	//	Which Lap subscript incremented to be positioned correctly
										p_ADlvi.lParam = iLap + 1;
										swprintf(result,NUMCHARS(result), lstVal);
										p_ADlvi.pszText = result;
										p_ADlvi.cchTextMax = wcslen(result);
										//	ListView_SetItem(AD_hWnd, &p_ADlvi);
										SendMessage(AD_hWnd, LVM_SETITEM, 0, (LPARAM)&p_ADlvi);
									}
									else	//	Data Channel doesn't exist in the list. Let's add it and the value.
									{
										p_ADlvi.mask = LVIF_TEXT | LVIF_PARAM;
										p_ADlvi.iItem = iTotChannel + 1;	//	Which Data Channel subscript
										p_ADlvi.iSubItem = 0;	//	Which Lap subscript
										p_ADlvi.lParam = 0;
										swprintf(result2,NUMCHARS(szChannelName), szChannelName);
										p_ADlvi.pszText = result2;
										p_ADlvi.cchTextMax = wcslen(result2);
										//	ListView_InsertItem(AD_hWnd, &p_ADlvi);
										SendMessage(AD_hWnd, LVM_INSERTITEM, 0, (LPARAM)&p_ADlvi);

										//	Now populate the listview subitem with the datapoint value, if it's valid
										p_ADlvi.mask = LVIF_TEXT;
										p_ADlvi.iItem = iTotChannel + 1;	//	Which Data Channel subscript
										p_ADlvi.iSubItem = iLap + 1;	//	Which Lap subscript incremented to be positioned correctly
										p_ADlvi.lParam = iLap + 1;
										swprintf(result,NUMCHARS(result), lstVal);
										p_ADlvi.pszText = result;
										p_ADlvi.cchTextMax = wcslen(result);
										//	ListView_SetItem(AD_hWnd, &p_ADlvi);
										SendMessage(AD_hWnd, LVM_SETITEM, 0, (LPARAM)&p_ADlvi);

										iTotChannel = iTotChannel + 1;	//	Increment the total number of data channels in the LV
									}	//	End of data channel add test

									b_LV_ChannelFound = false;	//	Set the channel search flag to not found
								}	//	End of Data Channel not matched test
								else
								{
									b_LV_ChannelFound = true;	//	The channels match
								}
							}	//	End of Sub Item addition test
							if (b_LV_ChannelFound )
							{
								//	Now populate the listview subitem with the datapoint value, if it's valid
								if (iChannel == 0)	//	Put in Lap Name if it's the first row
								{
									p_ADlvi.mask = LVIF_TEXT;
									p_ADlvi.iItem = 0;	//	Which Data Channel subscript
									p_ADlvi.iSubItem = iLap + 1;	//	Which Lap subscript incremented to be positioned correctly
									p_ADlvi.lParam = iLap + 1;
									swprintf(result,NUMCHARS(szDate), szDate);
									p_ADlvi.pszText = result;
									p_ADlvi.cchTextMax = wcslen(result);
									//	ListView_SetItem(AD_hWnd, &p_ADlvi);
									SendMessage(AD_hWnd, LVM_SETITEM, 0, (LPARAM)&p_ADlvi);

									p_ADlvi.mask = LVIF_TEXT;
									p_ADlvi.iItem = iChannel + 1;	//	Which Data Channel subscript
									p_ADlvi.iSubItem = iLap + 1;	//	Which Lap subscript incremented to be positioned correctly
									p_ADlvi.lParam = iLap + 1;
									swprintf(result,NUMCHARS(result), lstVal);
									p_ADlvi.pszText = result;
									p_ADlvi.cchTextMax = wcslen(result);
									//	ListView_SetItem(AD_hWnd, &p_ADlvi);
									SendMessage(AD_hWnd, LVM_SETITEM, 0, (LPARAM)&p_ADlvi);
								}
								else	//	Populate with the data channel value
								{
									p_ADlvi.mask = LVIF_TEXT;
									p_ADlvi.iItem = iChannel + 1;	//	Which Data Channel subscript
									p_ADlvi.iSubItem = iLap + 1;	//	Which Lap subscript incremented to be positioned correctly
									p_ADlvi.lParam = iLap + 1;
									swprintf(result,NUMCHARS(result), lstVal);
									p_ADlvi.pszText = result;
									p_ADlvi.cchTextMax = wcslen(result);
									//	ListView_SetItem(AD_hWnd, &p_ADlvi);
									SendMessage(AD_hWnd, LVM_SETITEM, 0, (LPARAM)&p_ADlvi);
								}
							}
						}	//	End Data Channel checking loop
						iChannel++;	//	Increment the Data Channel counter and try to pull the next Data Channel information
					}	//	End Data Channel Loop
					//	Increment the lap counter and get info from the next lap if we are less than our max. laps
					if (iLap < cLaps)
					{
						iLap++;
						iChannel = 0;	//	Reset the Data Channel Listview array counter for the next lap
					}
					else
					{
						break;
					}
				}	//	End Lap Loop
			}	//	End Test Loop
		}	//	End of Dialog present conditional
		return;
  }
  
  float fAverage(DATA_CHANNEL eChannel, const IDataChannel* pChannel, float flVal)
	{
		//	This function returns the average value for the data channel across all data points from this lap.
		char szAvg[MAX_PATH];
		float sum = 0.0f;
		int count; 
		vector<DataPoint> channels = pChannel->GetData(); // get the values for all of the data points
		for (count = 0; count < channels.size(); count++)
		{
			GetChannelValue(eChannel,m_sfLapOpts.eUnitPreference,channels[count].flValue,szAvg,NUMCHARS(szAvg));
			sum = sum + atof(szAvg); 
		}
		if (count != 0) 
		{
			return sum / count; 
		}
		else
		{
			return sum;
		}
	}

void UpdateSectors()
  {
	//	Update the Sector Times display
	//	The idea here is to get the sector positions and iTime from sfLapOpts, then for each highlighted
	//  Lap run through the Ref Lap Time/Distance array and interpolate the iTime at the equivalent distance
	//  Coding is similar to TimeSlip
	//	Let's define some colors for the List Views to use
	COLORREF d_BlackColor = RGB(0, 0, 0);	//	Black
	COLORREF d_WhiteColor = RGB(255, 255, 255);	//	White
	COLORREF d_RedColor = RGB(230, 20, 20);	//	Red
	COLORREF d_GreenColor = RGB(20, 230, 20);	//	Green
	COLORREF d_LGreyColor = RGB(200, 200, 200);	//	Light Grey
	LVBKIMAGE plvbki={0};	//	Set background image to black

	if ( m_pReferenceLap != NULL && m_sfLapOpts.fDrawSplitPoints )	//	First, let's make sure that we have a Reference Lap, or let's not perform this
	{
		HC_ShowSplits = GetDlgItem(hWndShowSplits, IDC_SHOW_SECTORS);	//	Let's get the handle for the display control in this window
		HDC HC_Rect = GetDC(hWndShowSplits);	//	HDC for the display control
		ListView_DeleteAllItems(HC_ShowSplits);	//	Clear the list before displaying the update
		const int cSectors = 9;	//	The maximum number of Sectors to display, gated by display area
		const int MaxLaps = 8;	//	Maximum number of laps to display
		struct i_BestSectors
		{
			int iItem;
			int iSubItem;
		}
		i_BestSectors[cSectors + 1] = {NULL};	//	Initialize the Best Sectors array
		float i_TheoreticalBestLap[cSectors + 1];	//	+1 For the final Split Point
		for (int a = 0; a < cSectors + 1; a++)
		{
			i_TheoreticalBestLap[a] = 999999.0f;	//	Initialize the Theorectical Best Lap
		}
		int w = 0;	//	Lap tracker for Sector display
		int s = 0;	//	Sector tracker for Listview

		set<LPARAM> setSelected = m_sfLapList.GetSelectedItemsData3();	//	Get the list of highlighted lap time ID's
		vector<CExtendedLap*> lstLaps = GetLapsToShow();	//	Load the CExtendedLap data for the lap list

		//	Get the points from the Ref Lap for computation
		const vector<TimePoint2D>& lstRefPoints = m_pReferenceLap->GetPoints();	// For iTime
		const IDataChannel* pReferenceDistance = m_pReferenceLap->GetChannel(DATA_CHANNEL_DISTANCE);

		//	Strings for building the Sector Times output for each lap
		TCHAR szLapString[50][512] = {NULL};
		TCHAR szString[50][512] = {NULL};
		TCHAR szSectorTime[MAX_PATH] = {NULL};	//	String containing the formatted sector time
		float dSectorTime = 0;	//	Initialize the Sector Times variable
		BOOL b_LineColor = false;	//	Flag for making alternating rows different colors

		SendMessage(HC_ShowSplits,LVM_SETBKIMAGE,0,(LPARAM)(LPLVBKIMAGE)&plvbki);	//	Set background color of LV
		SendMessage(HC_ShowSplits,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT); // Set style

		//	Lap Loop
		//	Now loop through the lap list, compute the sector times and store them in SplitPoints[]
		for(vector<CExtendedLap*>::iterator i = lstLaps.begin(); i != lstLaps.end(); i++)
		{
			CExtendedLap* pLap = *i;	//	Get the data points for this lap, and compare the sector times to the Reference Lap (m_pReferenceLap)
			const vector<TimePoint2D>& lstLapPoints = pLap->GetPoints();	//	Get the points from the Selected Lap for computation
			pLap->GetString(szLapString[w], NUMCHARS(szLapString)); //   Timestamp of this lap, to used to name it

			//	Let's load the Listview row titles with this result
			wchar_t result[MAX_PATH] ;
			p_ADlvi.mask = LVIF_TEXT | LVIF_PARAM;
			p_ADlvi.iItem = w;	//	Which Lap subscript
			p_ADlvi.iSubItem = 0;	//	Which Sector subscript (0 = Lap Name string)
			p_ADlvi.lParam = 0;
			swprintf(result,NUMCHARS(result), szLapString[w]);
			p_ADlvi.pszText = result;
			p_ADlvi.cchTextMax = wcslen(result);
			ListView_InsertItem(HC_ShowSplits, &p_ADlvi);	//	Using the lap time stamp/string for its name

			const IDataChannel* pDistance = pLap->GetChannel(DATA_CHANNEL_DISTANCE);
			int iLapStartTime = lstLapPoints[0].iTime;

			//	Sector Loop
			//	Now loop through the split points and determine lap times for each sector
			for(s = 1; s <= cSectors; s++)
			{
				const int SectorStartTime =  m_sfLapOpts.m_SplitPoints[s].m_sfSectorTime;	//	Get the Split Point iTime and it's distance value
				const double dSectorDistance = pReferenceDistance->GetValue((int)SectorStartTime);	//	First iTime for the lap array
				bool b_SectorFlag = false;
				double dLastLapDist = 0;

				//	Interpolation Loop
				//	Now go through the lap array and find the 2 points that span the dSectorDistance distance
				for (int x = 1; x < lstLapPoints.size(); x++)
				{
					const int iElapsedTime = lstLapPoints[x].iTime - iLapStartTime;
					const double dDistance = pDistance->GetValue(lstLapPoints[x].iTime);
					dLastLapDist = pDistance->GetValue(lstLapPoints[x-1].iTime);

					TimePoint2D pLapPoint = lstLapPoints[x];
					// this lap's time at {dDistance} was {iElapsedTime}.
					// we now need to estimate what the lap time at {dDistance} was, and then we can get our sector time
					const int cLapSize = lstLapPoints.size();
					if(dDistance >= dSectorDistance && dLastLapDist <= dSectorDistance)
					{
						// we have found two points straddling the distance we're curious about, dSectorDistance
						const double dOffset = dSectorDistance - dLastLapDist; // how far into the {dLastRefDist,dRefDist} x axis we are
						const double dWidth = dDistance - dLastLapDist; // how far apart {dLastRefDist,dRefDist} are
						double dFraction = 0;
						if(dWidth != 0)
						{
							dFraction = dOffset / dWidth; // the fraction that dDistance is between dLastLapDist and dDistance
							if(dFraction >= 0.0 && dFraction <= 1.0)
							{
								const int iLastTime = lstLapPoints[x-1].iTime;
								const int iThisTime = lstLapPoints[x].iTime;
								const double dEstimatedElapsedTime = dFraction * (iThisTime - iLastTime) + (double)iLastTime; 
								// this is the estimated time for the previous lap at this position
								if(dEstimatedElapsedTime >= 0)
								{
									dSectorTime = dEstimatedElapsedTime - (double)iLapStartTime;
									//	Now that we have computed the Sector Time, let's put it in the Sector times string
									if ( dSectorTime > 1 )	//	1 used for roundoff error
									{
										swprintf(szSectorTime, NUMCHARS(szSectorTime), L"%4.2f", dSectorTime/1000);
									}
									else
									{
										swprintf(szSectorTime, NUMCHARS(szSectorTime), L"", NULL);
									}
									iLapStartTime = dEstimatedElapsedTime;
									dLastLapDist = dSectorDistance;
									break;
								}
							}
						}
						else
						{
							const int iLastTime = lstLapPoints[x-1].iTime;
							dSectorTime = iLastTime - (double)iLapStartTime;
							//	Now that we have computed the Sector Time, let's put it in the Sector times string
							if ( dSectorTime > 1 )	//	1 used for roundoff error
							{
								swprintf(szSectorTime, NUMCHARS(szSectorTime), L"%4.2f", dSectorTime/1000);
							}
							else
							{
								swprintf(szSectorTime, NUMCHARS(szSectorTime), L"", NULL);
							}
							iLapStartTime = iLastTime;
							dLastLapDist = dSectorDistance;
							break;
						}
					}
					if (x == lstLapPoints.size()-1)
					{
						//	We've reached the end of the loop. Dump the last point as the last sector time, if other conditions failed
						const int iLastTime = lstLapPoints[lstLapPoints.size()-1].iTime;
						dSectorTime = iLastTime - (double)iLapStartTime;
						//	Now that we have computed the Sector Time, let's put it in the Sector times string
						if ( dSectorTime > 1 )	//	1 used for roundoff error
						{
							swprintf(szSectorTime, NUMCHARS(szSectorTime), L"%4.2f", dSectorTime/1000);
						}
						else
						{
							swprintf(szSectorTime, NUMCHARS(szSectorTime), L"", NULL);
						}
						iLapStartTime = iLastTime;
						dLastLapDist = dSectorDistance;
						break;
					}
				}	//	End Interpolation Loop

				//	Insert the item into the Listview
				p_ADlvi.mask = LVIF_TEXT;
				p_ADlvi.iItem = w;	//	Which Lap subscript
				p_ADlvi.iSubItem = s;	//	Which Sector subscript incremented to be positioned correctly
				p_ADlvi.lParam = s;
				swprintf(result,NUMCHARS(result), szSectorTime);
				p_ADlvi.pszText = result;
				p_ADlvi.cchTextMax = wcslen(result);
				ListView_SetItem(HC_ShowSplits, &p_ADlvi);
				if (dSectorTime < i_TheoreticalBestLap[s] )	//	Save sector information if this is the fastest
				{
					i_BestSectors[s].iItem = w;
					i_BestSectors[s].iSubItem = s;
					i_TheoreticalBestLap[s] = dSectorTime;
				}

			}	//	End Sector Loop
			w++;	//	Increment "w" counter and do the next lap
			if (w >= MaxLaps) break;	//	Stop building these if we already have as many as we need.
		}	//	Lap Loop end

		//	Now let's load the Listview with the Best Theorectical Lap information
		float f_TheoreticalBestLap = 0.0f;
		for (int t = 1; t <= cSectors; t++)
		{
			f_TheoreticalBestLap = f_TheoreticalBestLap + i_TheoreticalBestLap[t];
		}
		TCHAR szLap[MAX_PATH];
		wchar_t result[MAX_PATH] ;
		p_ADlvi.mask = LVIF_TEXT | LVIF_PARAM;
		p_ADlvi.iItem = w;	//	Which Lap subscript
		p_ADlvi.iSubItem = 0;	//	Which Sector subscript (0 = Lap Name string)
		p_ADlvi.lParam = 0;
	    ::FormatTimeMinutesSecondsMs(f_TheoreticalBestLap/1000, szLap, NUMCHARS(szLap) );
		swprintf(result,NUMCHARS(result), L"Theoretical Best: %s", szLap );
		p_ADlvi.pszText = result;
		p_ADlvi.cchTextMax = wcslen(result);
		ListView_InsertItem(HC_ShowSplits, &p_ADlvi);	//	Using the lap time stamp/string for its name

		for (int c = 1; c <= cSectors; c++)	//	Now let's populate the sector times for Best Theoretical Lap
		{
			//	Insert the item into the Listview
			p_ADlvi.mask = LVIF_TEXT;
			p_ADlvi.iItem = w;	//	Which Lap subscript
			p_ADlvi.iSubItem = c;	//	Which Sector subscript incremented to be positioned correctly
			p_ADlvi.lParam = c;
			if ( i_TheoreticalBestLap[c] > 1 )	//	1 used for roundoff error
			{
				swprintf( result,NUMCHARS(result), L"%4.2f", i_TheoreticalBestLap[c] / 1000 );
			}
			else
			{
				swprintf( result,NUMCHARS(result), L"", NULL);
			}
			p_ADlvi.pszText = result;
			p_ADlvi.cchTextMax = wcslen(result);
			ListView_SetItem(HC_ShowSplits, &p_ADlvi);
		}
	  }
  }

  void UpdateValues()
  {
	//	Update the data channels that are being displayed as values
	//	List of highlighted laps
	set<LPARAM> setSelectedData = m_sfLapList.GetSelectedItemsData3();
    if(setSelectedData.size() > 0)
    {
	  HWND hWndDataValues = GetDlgItem(m_hWnd, IDC_DATAVALUES);	//	Get the handle for the control
	  ListView_DeleteAllItems(hWndDataValues);	//	Clear the list before displaying the update
      const int cLabels = 10;	//	The maximum number of Value Data channels to display, gated by display area
	  int i_WarningCoord[cLabels][3] = {NULL};	//	Coordinates in the Data Channels display ListView for painting routines
	  bool m_Warning = false;	//	Flag for showing dialog of Value display to indicate statistics are outside of bounds
	  TCHAR m_szYString[512] = {NULL};
	  TCHAR m_szWarningChannel[MAX_PATH] = {NULL};
	  int w=0;	//	String variable counter for Vaue display
      TCHAR szLabel[cLabels][MAX_PATH] = {NULL};
      //   Loop through the selected Y-axis data channels for this lap
	  for(int x = 0; x < this->m_lstYChannels.size() && x < 49; x++)
	  {
			const DATA_CHANNEL eChannel = m_lstYChannels[x];
			if(!eChannel /*|| !eChannel->IsValid()*/) continue;
			float flMin, flMax, flAvg, flMinTemp, flMaxTemp;
			//	First check if this data channel is one to be displayed as a Value (false) or Graph (true) 
			for (int u = 0; u < sizeof m_lstYChannels; u++)
			{
				if (m_lstYChannels[x] == m_sfLapOpts.m_PlotPrefs[u].iDataChannel && m_sfLapOpts.m_PlotPrefs[u].iPlotView == true)
				{
						break;	//	Data channel is requested to be displayed as a graph, do nothing here
				}
				else if	(m_lstYChannels[x] == m_sfLapOpts.m_PlotPrefs[u].iDataChannel && m_sfLapOpts.m_PlotPrefs[u].iPlotView == false)
				{
					//	Let's get the statistical values for this channel for display
					// go through all the laps we have selected to figure out min/max
					flMin = 1e30;
					flMax = -1e30;
					float flVal;
					for(set<LPARAM>::const_iterator i = setSelectedData.begin(); i != setSelectedData.end(); i++)
					{
					  CExtendedLap* pLap = (CExtendedLap*)*i;
					  const IDataChannel* pChannel = pLap->GetChannel(eChannel);
					  if (pChannel)	//	Check if pointer is valid
					  {
						flVal = pChannel->GetValue(m_mapLapHighlightTimes[pLap]);
						flMin = pChannel->GetMin();
						flMax = pChannel->GetMax();
						// 951turbo: do more math here like averages, median, etc.
						flAvg = fAverage(eChannel, pChannel, flVal);
						//	See if the Minimum or Maximum are outside of the PlotPrefs setpoints
//////////////////////////////////////////
						//		Adding transformation functions here for Y
						if (m_sfLapOpts.m_PlotPrefs[u].iTransformYesNo == true)
						{
							if (m_sfLapOpts.m_PlotPrefs[u].fTransBValue < 0)
							{
								flAvg = m_sfLapOpts.m_PlotPrefs[u].fTransAValue + flAvg * m_sfLapOpts.m_PlotPrefs[u].fTransBValue + flAvg * flAvg *  m_sfLapOpts.m_PlotPrefs[u].fTransCValue;
								flMaxTemp = m_sfLapOpts.m_PlotPrefs[u].fTransAValue + flMin * m_sfLapOpts.m_PlotPrefs[u].fTransBValue + flMin * flMin *  m_sfLapOpts.m_PlotPrefs[u].fTransCValue;
								flMinTemp = m_sfLapOpts.m_PlotPrefs[u].fTransAValue + flMax * m_sfLapOpts.m_PlotPrefs[u].fTransBValue + flMax * flMax *  m_sfLapOpts.m_PlotPrefs[u].fTransCValue;
							}
							else
							{
								flAvg = m_sfLapOpts.m_PlotPrefs[u].fTransAValue + flAvg * m_sfLapOpts.m_PlotPrefs[u].fTransBValue + flAvg * flAvg *  m_sfLapOpts.m_PlotPrefs[u].fTransCValue;
								flMinTemp = m_sfLapOpts.m_PlotPrefs[u].fTransAValue + flMin * m_sfLapOpts.m_PlotPrefs[u].fTransBValue + flMin * flMin *  m_sfLapOpts.m_PlotPrefs[u].fTransCValue;
								flMaxTemp = m_sfLapOpts.m_PlotPrefs[u].fTransAValue + flMax * m_sfLapOpts.m_PlotPrefs[u].fTransBValue + flMax * flMax *  m_sfLapOpts.m_PlotPrefs[u].fTransCValue;
							}
							flMin = flMinTemp;
							flMax = flMaxTemp;
						}
//////////////////////////////////////////
						if (flMax > m_sfLapOpts.m_PlotPrefs[u].fMaxValue)
						{
							m_Warning = true;	//	An alarm has been triggered! Save the channel name and post a warning dialog.
							//	Save the ListView coordinates for painting of this value
							i_WarningCoord[w][2] = 1;	//	Maximum value exceeded
							GetDataChannelName(eChannel,m_szWarningChannel,NUMCHARS(m_szWarningChannel));	//	Get the failing channel name
							//	Build the failing channels string for output
							swprintf(m_szYString,NUMCHARS(m_szYString),L"%s\n%s",m_szYString, m_szWarningChannel);
						}
						else if (flMin < m_sfLapOpts.m_PlotPrefs[u].fMinValue)
						{
							m_Warning = true;	//	An alarm has been triggered! Save the channel name and post a warning dialog.
							//	Save the ListView coordinates for painting of this value
							i_WarningCoord[w][1] = -1;	//	Minimum value exceeded
							GetDataChannelName(eChannel,m_szWarningChannel,NUMCHARS(m_szWarningChannel));	//	Get the failing channel name
							//	Build the failing channels string for output
							swprintf(m_szYString,NUMCHARS(m_szYString),L"%s\n%s",m_szYString, m_szWarningChannel);
						}
						else
						{
							//	Reset the ListView coordinates for painting of this value
							i_WarningCoord[w][1] = 0;	//	Reset Minimum value
							i_WarningCoord[w][2] = 0;	//	Reset Maximum value
						}
					  }
					  else
					  {
						  flVal=0.0f;
						  flMin=0.0f;
						  flMax=0.0f;
						  flAvg=0.0f;
						  //	Reset the ListView coordinates for painting of this value
						  i_WarningCoord[w][1] = 0;	//	Reset Minimum value
						  i_WarningCoord[w][2] = 0;	//	Reset Maximum value
					  }
					}

					//	Now assign these values to the Data Value variable for display
					TCHAR szChannelName[MAX_PATH];
					TCHAR w_szMin[MAX_PATH];
					TCHAR w_szMax[MAX_PATH];
					GetDataChannelName(eChannel,szChannelName,NUMCHARS(szChannelName));

					char szMin[MAX_PATH];
					char szMax[MAX_PATH];
					GetChannelValue(eChannel,m_sfLapOpts.eUnitPreference,flMin,szMin,NUMCHARS(szMin));
					GetChannelValue(eChannel,m_sfLapOpts.eUnitPreference,flMax,szMax,NUMCHARS(szMax));
					//	Need to convert these from char to wchar_t
					swprintf(w_szMin, NUMCHARS(w_szMin), L"%S", szMin);
					swprintf(w_szMax, NUMCHARS(w_szMax), L"%S", szMax);

					//	Now display the results in the ListView (max of cLabels)
					if (w < cLabels)
					{
						//	Let's load the Listview row titles with this result
						p_ADlvi.mask = LVIF_TEXT | LVIF_PARAM;
						p_ADlvi.iItem = w;	//	Which Data Value subscript (Max = cLabels)
						p_ADlvi.iSubItem = 0;	//	Which Sector subscript (0 = Lap Name string)
						p_ADlvi.lParam = 0;
						p_ADlvi.pszText = szChannelName;
						p_ADlvi.cchTextMax = wcslen(szChannelName);
						ListView_InsertItem(hWndDataValues, &p_ADlvi);

						wchar_t result[MAX_PATH] = {NULL};	//	Null string
						//	Insert the item into the Listview
						p_ADlvi.mask = LVIF_TEXT;
						p_ADlvi.iItem = w;	//	Which Data Value subscript (Max = cLabels)
						p_ADlvi.iSubItem = 1;	//	Which Subitem subscript
						p_ADlvi.lParam = 1;
						swprintf(result,NUMCHARS(result), L"%S", szMin);
						p_ADlvi.pszText = result;
						p_ADlvi.cchTextMax = wcslen(result);
						ListView_SetItem(hWndDataValues, &p_ADlvi);

						//	Insert the item into the Listview
						p_ADlvi.mask = LVIF_TEXT;
						p_ADlvi.iItem = w;	//	Which Data Value subscript (Max = cLabels)
						p_ADlvi.iSubItem = 2;	//	Which Subitem subscript
						p_ADlvi.lParam = 2;
						swprintf(result,NUMCHARS(result), L"%S", szMax);
						p_ADlvi.pszText = result;
						p_ADlvi.cchTextMax = wcslen(result);
						ListView_SetItem(hWndDataValues, &p_ADlvi);

						//	Insert the item into the Listview
						p_ADlvi.mask = LVIF_TEXT;
						p_ADlvi.iItem = w;	//	Which Data Value subscript (Max = cLabels)
						p_ADlvi.iSubItem = 3;	//	Which Subitem subscript
						p_ADlvi.lParam = 3;
						swprintf(result,NUMCHARS(result), L"%3.1f", flAvg);
						p_ADlvi.pszText = result;
						p_ADlvi.cchTextMax = wcslen(result);
						ListView_SetItem(hWndDataValues, &p_ADlvi);

						w++;	//	Increment Value string counter
					}
					break;
				}
				else
				{
				}
			}
	  }
	  if (m_Warning)	//	Pop up dialog saying the alarm has been triggered
		{
			static bool fWarnedOnce;	//	Prevent multiple windows from appearing
			if(!fWarnedOnce)
			{
				//	Display a warning dialog box about an alarm being triggered.
				fWarnedOnce = true;
				WARNING_RESULT sfResult;
				CWarningDlg dlgWarning(&sfResult, m_szYString);
				ArtShowDialog<IDD_WARNING>(&dlgWarning);
				fWarnedOnce = false;
			}
		}
    }
  }
void UpdateDisplays()
  {
    m_sfLapPainter.Refresh();
	m_sfSubDisplay.Refresh();
//	if (m_sfLapOpts.bTractionCircle)
//	{
		m_sfTractionDisplay.Refresh();
//	}
  }
  void CheckMenuHelper(HMENU hMainMenu, int id, bool fChecked)
  {
    DWORD dwFlags = fChecked ? (MF_BYCOMMAND | MF_CHECKED) : (MF_BYCOMMAND | MF_UNCHECKED);
    DWORD dwRet = CheckMenuItem(hMainMenu, id, dwFlags);
    DASSERT(dwRet != -1);
  }
  void UpdateMenus()
  {
    HMENU hWndMenu = GetMenu(m_hWnd);
    HMENU hSubMenu = GetSubMenu(hWndMenu, 2);	//	Graphing... menu
    HMENU hSubMenu2 = GetSubMenu(hWndMenu, 3);	//	Phone Orientation... menu

    CheckMenuHelper(hSubMenu, ID_OPTIONS_KMH, m_sfLapOpts.eUnitPreference == UNIT_PREFERENCE_KMH);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_MPH, m_sfLapOpts.eUnitPreference == UNIT_PREFERENCE_MPH);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_MS, m_sfLapOpts.eUnitPreference == UNIT_PREFERENCE_MS);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_XAXIS_KM, m_sfLapOpts.bXAxis_KM == XAXIS_PREFERENCE_KM);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_XAXIS_LAT, m_sfLapOpts.bXAxis_KM == XAXIS_PREFERENCE_LAT);
	CheckMenuHelper(hSubMenu, ID_OPTIONS_TRACTIONCIRCLE, m_sfLapOpts.bTractionCircle);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_SHOWBESTS, m_fShowBests);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_SHOWDRIVERBESTS, m_fShowDriverBests);
	CheckMenuHelper(hSubMenu, ID_OPTIONS_SHOWREFERENCELAP, m_fShowReferenceLap);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_DRAWLINES, m_sfLapOpts.fDrawLines);
	CheckMenuHelper(hSubMenu, ID_OPTIONS_SMOOTH, m_sfLapOpts.bSmoothYesNo);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_BACKGROUND, m_sfLapOpts.fColorScheme);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_IOIO5VSCALE, m_sfLapOpts.fIOIOHardcoded);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_ELAPSEDTIME, m_sfLapOpts.fElapsedTime);
	CheckMenuHelper(hSubMenu2, ID_OPTIONS_VERTICAL_LANDSCAPE, m_sfLapOpts.e_Orientation == VERTICAL_LANDSCAPE);
	CheckMenuHelper(hSubMenu2, ID_OPTIONS_VERTICAL_PORTRAIT, m_sfLapOpts.e_Orientation == VERTICAL_PORTRAIT);
	CheckMenuHelper(hSubMenu2, ID_OPTIONS_FLAT_LANDSCAPE, m_sfLapOpts.e_Orientation == FLAT_LANDSCAPE);
	CheckMenuHelper(hSubMenu2, ID_OPTIONS_FLAT_PORTRAIT, m_sfLapOpts.e_Orientation == FLAT_PORTRAIT);
  }

  vector<CExtendedLap*> GetSortedLaps(LAPSORTSTYLE eSortStyle)
  {
    vector<CExtendedLap*> lstLaps;
    for(map<int,CExtendedLap*>::iterator i = m_mapLaps.begin(); i != m_mapLaps.end(); i++)
    {
      lstLaps.push_back(i->second);
    }
    switch(eSortStyle)
    {
    case SORTSTYLE_BYTIMEOFRACE:
      sort(lstLaps.begin(),lstLaps.end(), CExtendedLap_SortByTime);
      break;
    case SORTSTYLE_BYLAPTIME:
      sort(lstLaps.begin(),lstLaps.end(), CExtendedLap_SortByLapTime);
      break;
    }
    return lstLaps;
  }
  
  void LoadLaps(ILapReceiver* pReceiver)
  {
	  int z_iRaceId = 0;
	  for (int z = 0; z < 50; z++)
	  {
			if (m_iRaceId[z] <= 0) break;	//	Only load the valid Race Id's
			z_iRaceId = m_iRaceId[z];
			vector<const ILap*> laps = pReceiver->GetLaps(z_iRaceId);
			for(int x = 0;x < laps.size(); x++)
			{
				const ILap* pLap = laps[x];
				// let's see if we already have this lap
				if(m_mapLaps.count(pLap->GetLapId()) != 0)
				{
					// we've already got this lap.  THere is nothing to be added from this lap
					((ILap*)pLap)->Free();
					laps[x] = NULL;
				}
				else
				{
					// we don't have this lap yet, so let's put it in
					CExtendedLap* pNewLap = new CExtendedLap(pLap, m_pReferenceLap, pReceiver, true);
					if(m_pReferenceLap == NULL)		// If there is no reference lap currently
					{
						m_pReferenceLap = pNewLap; // by default, make the first lap received the reference lap
					}
					if(pLap->GetComment().size() <= 0)
					{
						pLap->SetComment(m_szCommentText);
					}
					m_mapLaps[pLap->GetLapId()] = pNewLap;
				}
			}
	  }
  }

  void ApplyDriverNameToSelectedLaps(ILapReceiver* pLapDB)
  {
    set<LPARAM> setSelectedData = m_sfLapList.GetSelectedItemsData3();
    for(set<LPARAM>::iterator i = setSelectedData.begin(); i != setSelectedData.end(); i++)
    {
      // the ints of this set are actually pointers to CExtendedLap objects
      CExtendedLap* pLap = (CExtendedLap*)(*i);
      pLap->GetLap()->SetComment(m_szCommentText);
    }
  }

  virtual void SetLapHighlightTime(const CExtendedLap* pLap, int iTimeMs) override
  {
    m_mapLapHighlightTimes[pLap] = iTimeMs;
  }
  virtual int GetLapHighlightTime(const CExtendedLap* pLap) const override
  {
    DASSERT(m_mapLapHighlightTimes.find(pLap) != m_mapLapHighlightTimes.end()); // this should have always ended up set from the "master" highlighter.  This function is only called by "slave" highlight-users
    return m_mapLapHighlightTimes.find(pLap)->second;
  }
  virtual bool IsHighlightSource(int iSupplierId) const override
  {
    switch(iSupplierId)
    {
    case SUPPLIERID_MAINDISPLAY:
      return true; // main display is always the driver of highlight data
    case SUPPLIERID_SUBDISPLAY:
      return false;
	case SUPPLIERID_SECTORDISPLAY:
		return true;	//	Allow the Set Split Sectors to be highlight source
	case SUPPLIERID_TRACTIONCIRCLEDISPLAY:
	case SUPPLIERID_ALLDATADISPLAY:
		return false;	//	main display is always the driver of highlight data
    default:
      DASSERT(FALSE);
      return false;
    }
  }
  
  virtual vector<CExtendedLap*> GetAllLaps() const override
  {
    set<LPARAM> setSelectedLaps = m_sfLapList.GetSelectedItemsData3();
    vector<CExtendedLap*> lstLaps;
    for(map<int,CExtendedLap*>::const_iterator i = m_mapLaps.begin(); i != m_mapLaps.end(); i++)
    {
      CExtendedLap* pLap = i->second;
      lstLaps.push_back(pLap);
    }

    return lstLaps;
  }
  virtual vector<CExtendedLap*> GetLapsToShow() const override
  {
    set<LPARAM> setSelectedLaps = m_sfLapList.GetSelectedItemsData3();
    vector<CExtendedLap*> lstLaps;
    map<wstring,CExtendedLap*> mapFastestDriver;
    CExtendedLap* pFastest = NULL;
    CExtendedLap* pReference = NULL;		// Added to show Reference Lap - KDJ
    for(set<LPARAM>::iterator i = setSelectedLaps.begin(); i != setSelectedLaps.end(); i++)
    {
      CExtendedLap* pLap = (CExtendedLap*)*i;

      lstLaps.push_back(pLap);
    }

    for(map<int,CExtendedLap*>::const_iterator i = m_mapLaps.begin(); i != m_mapLaps.end(); i++)
    {
      CExtendedLap* pLap = i->second;
      if(m_fShowDriverBests && (mapFastestDriver.count(pLap->GetLap()->GetComment()) == 0 || pLap->GetLap()->GetTime() < mapFastestDriver[pLap->GetLap()->GetComment()]->GetLap()->GetTime()))
      {
        mapFastestDriver[pLap->GetLap()->GetComment()] = pLap;
      }
      if(m_fShowBests)
      {
        if(pFastest == NULL || pLap->GetLap()->GetTime() < pFastest->GetLap()->GetTime())
        {
          pFastest = pLap;
        }
      }
    }

    if(m_fShowBests && pFastest)
    {
      lstLaps.push_back(pFastest);
    }
    for(map<wstring,CExtendedLap*>::iterator i = mapFastestDriver.begin(); i != mapFastestDriver.end(); i++)
    {
      lstLaps.push_back(i->second);
    }

	//	Set up for showing Reference lap similar to how we show Fastest Lap. 
	if(m_fShowReferenceLap && m_pReferenceLap != NULL)
    {
		lstLaps.push_back(m_pReferenceLap);
    }
/*	else
	{
		lstLaps.push_back(m_pReferenceLap);	//	Push the reference lap anyway, so that we can get the lap Split Points
	}
*/

    return lstLaps;
  }
  virtual FLOATRECT GetAllLapsBounds() const override
  {
    FLOATRECT rc;
    rc.left = 1e30;
    rc.top = -1e30;
    rc.bottom = 1e30;
    rc.right = -1e30;
    
    // changed this so it returns the bounds of the reference lap.  This way, data-viewing isn't ruined by errant points
    // it used to be based on all the laps, but if you had just one messed-up lap it would make viewing pointless
    if(m_pReferenceLap != NULL)
    {
      const vector<TimePoint2D>& lstPoints = m_pReferenceLap->GetPoints();
      for(int x = 0; x< lstPoints.size(); x++)
      {
        const TimePoint2D& p = lstPoints[x];
        rc.left = min(rc.left,p.flX);
        rc.top = max(rc.top,p.flY);
        rc.bottom = min(rc.bottom,p.flY);
        rc.right = max(rc.right,p.flX);
      }

      rc.left += m_flShiftX;
      rc.top += m_flShiftY;
      rc.right += m_flShiftX;
      rc.bottom += m_flShiftY;
    }
    return rc;
  }
  virtual LAPDISPLAYSTYLE GetLapDisplayStyle(int iSupplierId) const override
  {
    switch(iSupplierId)
    {
    case SUPPLIERID_MAINDISPLAY:
    {
      vector<CExtendedLap*> lstLaps = GetLapsToShow();
      if(lstLaps.size() <= 0) return LAPDISPLAYSTYLE_NOLAPS;
      return m_eLapDisplayStyle;
    }
    case SUPPLIERID_SUBDISPLAY:
      switch(m_eLapDisplayStyle)
      {
      case LAPDISPLAYSTYLE_MAP: return LAPDISPLAYSTYLE_PLOT;
      default: return LAPDISPLAYSTYLE_MAP;
      }
	case SUPPLIERID_TRACTIONCIRCLEDISPLAY:
	  {
		return LAPDISPLAYSTYLE_TRACTIONCIRCLE;
	  }
	case SUPPLIERID_ALLDATADISPLAY:
	  {
		return LAPDISPLAYSTYLE_ALLDATADISPLAY;
	  }
	case SUPPLIERID_SECTORDISPLAY:
	  {
		return LAPDISPLAYSTYLE_MAP;
	  }
	default:
      DASSERT(FALSE);
      break;
    }
    return m_eLapDisplayStyle;
  }
  virtual float GetDataHardcodedMin(DATA_CHANNEL eChannel) const override
  {
    if(eChannel >= DATA_CHANNEL_IOIOPIN_START && eChannel < DATA_CHANNEL_IOIOPIN_END ||
       eChannel >= DATA_CHANNEL_IOIOCUSTOM_START && eChannel < DATA_CHANNEL_IOIOCUSTOM_END ||
	   eChannel >= DATA_CHANNEL_RACEDAC_START && eChannel < DATA_CHANNEL_RACEDAC_END ||
       eChannel >= DATA_CHANNEL_RACEDACCUSTOM_START && eChannel < DATA_CHANNEL_RACEDACCUSTOM_END)
    {
      return m_sfLapOpts.fIOIOHardcoded ? 0 : 1e30;
    }
    return 1e30;
  }
  virtual float GetDataHardcodedMax(DATA_CHANNEL eChannel) const override
  {
    if(eChannel >= DATA_CHANNEL_IOIOPIN_START && eChannel < DATA_CHANNEL_IOIOPIN_END ||
       eChannel >= DATA_CHANNEL_IOIOCUSTOM_START && eChannel < DATA_CHANNEL_IOIOCUSTOM_END ||
	   eChannel >= DATA_CHANNEL_RACEDAC_START && eChannel < DATA_CHANNEL_RACEDAC_END ||
       eChannel >= DATA_CHANNEL_RACEDACCUSTOM_START && eChannel < DATA_CHANNEL_RACEDACCUSTOM_END)
    {
      return m_sfLapOpts.fIOIOHardcoded ? 5 : -1e30;
    }
    return -1e30;
  }

  virtual float GetGuideStartX(DATA_CHANNEL eChannel, float flMin, float flMax) override
  {
    CASSERT(DATA_CHANNEL_COUNT == 0x601);

    switch(eChannel)
    {
      case DATA_CHANNEL_X: return 1e30;
      case DATA_CHANNEL_Y: return 1e30; // we don't want guides for either latitude or longitude
	  case DATA_CHANNEL_VELOCITY:
	  case DATA_CHANNEL_VELOCITYDELTA:
      {
        int iMin = (int)(flMin);
        return (float)(iMin);
      }
      case DATA_CHANNEL_DISTANCE:
      {
        int iMin = (int)(flMin);
        return (float)(iMin);
      }
	  case DATA_CHANNEL_TIME:
	  case DATA_CHANNEL_ELAPSEDTIME:
	  case DATA_CHANNEL_TIMESLIP:
	  case DATA_CHANNEL_LAPTIME_SUMMARY:
	  {
        int iMin = (int)(flMin/1000.0f);
        return (float)(iMin)*1000.0f;
      }
      case DATA_CHANNEL_X_ACCEL:
      case DATA_CHANNEL_Y_ACCEL:
      case DATA_CHANNEL_Z_ACCEL:
      {
        int iMin = (int)(flMin);
        return (float)(iMin);
      }
      case DATA_CHANNEL_TEMP: return 0;
      case (DATA_CHANNEL_PID_START+0x5): return -40;
      case (DATA_CHANNEL_PID_START+0xc): return 0;
      case (DATA_CHANNEL_PID_START+0xA): return 0;
      case (DATA_CHANNEL_PID_START+0x5c): return -40;
      default: 
        if(eChannel >= DATA_CHANNEL_IOIOPIN_START && eChannel < DATA_CHANNEL_IOIOPIN_END ||
            eChannel >= DATA_CHANNEL_IOIOCUSTOM_START && eChannel < DATA_CHANNEL_IOIOCUSTOM_END ||
			eChannel >= DATA_CHANNEL_RACEDAC_START && eChannel < DATA_CHANNEL_RACEDAC_END ||
            eChannel >= DATA_CHANNEL_RACEDACCUSTOM_START && eChannel < DATA_CHANNEL_RACEDACCUSTOM_END)
        {
          return m_sfLapOpts.fIOIOHardcoded ? 0 : 1e30;
        }
        return 1e30;
    }
  }

  virtual float GetGuideStart(DATA_CHANNEL eChannel, float flMin, float flMax) override
  {
    CASSERT(DATA_CHANNEL_COUNT == 0x601);

    switch(eChannel)
    {
      case DATA_CHANNEL_X: return 1e30;
      case DATA_CHANNEL_Y: return 1e30; // we don't want guides for either latitude or longitude
      case DATA_CHANNEL_VELOCITY: return 0;
      case DATA_CHANNEL_DISTANCE: return 1e30;
      case DATA_CHANNEL_TIME:
      case DATA_CHANNEL_ELAPSEDTIME:
      case DATA_CHANNEL_TIMESLIP:
      {
        int iMin = (int)(flMin/1000.0f);
        return (float)(iMin)*1000.0f;
      }
      case DATA_CHANNEL_STRENGTH: return 0.0f;
	  case DATA_CHANNEL_LAPTIME_SUMMARY:
      case DATA_CHANNEL_X_ACCEL:
      case DATA_CHANNEL_Y_ACCEL:
      case DATA_CHANNEL_Z_ACCEL:
      {
        int iMin = (int)(flMin);
        return (float)(iMin);
      }
 	  case DATA_CHANNEL_VELOCITYDELTA:
      {
        int iMin = (int)(flMin);
        return (float)(iMin);
      }
      case DATA_CHANNEL_TEMP: return 0;

	  default: 
        if(eChannel >= DATA_CHANNEL_IOIOPIN_START && eChannel < DATA_CHANNEL_IOIOPIN_END ||
            eChannel >= DATA_CHANNEL_IOIOCUSTOM_START && eChannel < DATA_CHANNEL_IOIOCUSTOM_END ||
			eChannel >= DATA_CHANNEL_RACEDAC_START && eChannel < DATA_CHANNEL_RACEDAC_END ||
            eChannel >= DATA_CHANNEL_RACEDACCUSTOM_START && eChannel < DATA_CHANNEL_RACEDACCUSTOM_END)
        {
          return m_sfLapOpts.fIOIOHardcoded ? 0 : 1e30;
        }
        else if(eChannel >= DATA_CHANNEL_PID_START && eChannel < DATA_CHANNEL_PID_END)
        {
			int iMin = (int)(flMin);
			return (float)(iMin);
        }
        return 1e30;
    }
  }

  virtual float GetGuideStepX(DATA_CHANNEL eChannel, float flMin, float flMax) override
  {
  // Function sets up the spacing for the vertical guidelines on the data plots
	  CASSERT(DATA_CHANNEL_COUNT == 0x601);
    const float flSpread = flMax - flMin;
    switch(eChannel)
    {
    case DATA_CHANNEL_X: return 1e30;
    case DATA_CHANNEL_Y: return 1e30; // we don't want guides for either latitude or longitude
    case DATA_CHANNEL_VELOCITY:	// We need to fix the X-channel call before putting these back into the code.
    case DATA_CHANNEL_VELOCITYDELTA:
		{
		  switch(m_sfLapOpts.eUnitPreference)
		  {
		  case UNIT_PREFERENCE_KMH: return KMH_TO_MS(25.0);
		  case UNIT_PREFERENCE_MPH: return MPH_TO_MS(20.0);		//	Adjusted by KDJ
		  case UNIT_PREFERENCE_MS: return 5;
		  }
		  return 10.0;
		}
    case DATA_CHANNEL_DISTANCE: 
		{
		  if(flSpread < 0.001) return 0.0001f;		
		  if(flSpread < 0.005) return 0.0005f;		
		  if(flSpread < 0.010) return 0.0010f;		
		  if(flSpread < 0.050) return 0.0050f;		
		  if(flSpread < 1.000) return 0.1000f;
		  if(flSpread < 10.00) return 1.0000f;
		  if(flSpread < 500) return 50.0f;
		  if(flSpread < 1000) return 100.0f;
		  if(flSpread < 5000) return 500.0f;
		  if(flSpread < 10000) return 1000.0f;
		  if(flSpread < 50000) return 2500.0f;
		  if(flSpread < 110000) return 5000.0f;
		  if(flSpread < 1100000) return 10000.0f;
		  if(flSpread < 10000000) return 100000.0f;
		  return 10000000;
		} 

    case DATA_CHANNEL_TIME:					
    case DATA_CHANNEL_ELAPSEDTIME:					
		{
		  if(flSpread < 1000) return 50.0f;
		  if(flSpread < 5000) return 100.0f;
		  if(flSpread < 10000) return 500.0f;
		  if(flSpread < 50000) return 2500.0f;
		  if(flSpread < 110000) return 5000.0f;
		  if(flSpread < 1100000) return 10000.0f;
		  if(flSpread < 10000000) return 100000.0f;
		  if(flSpread < 100000000) return 1000000.0f;
		  return 10000000.0f;
		}
    case DATA_CHANNEL_LAPTIME_SUMMARY:					
		{
		  if(flSpread < 1) return 0.50f;
		  if(flSpread < 5) return 1.0f;
		  if(flSpread < 10) return 5.0f;
		  if(flSpread < 50) return 25.0f;
		  if(flSpread < 110) return 50.0f;
		  if(flSpread < 1100) return 100.0f;
		  if(flSpread < 10000) return 1000.0f;
		  if(flSpread < 50000) return 2500.0f;
		  if(flSpread < 110000) return 5000.0f;
		  if(flSpread < 1100000) return 10000.0f;
		  if(flSpread < 10000000) return 100000.0f;
		  if(flSpread < 100000000) return 1000000.0f;
		  return 10000000.0f;
		}
	default:
    return 1e30;
	}
  }

  virtual float GetGuideStep(DATA_CHANNEL eChannel, float flMin, float flMax) override
  {
    CASSERT(DATA_CHANNEL_COUNT == 0x601);
    const float flSpread = flMax - flMin;
    switch(eChannel)
    {
		case DATA_CHANNEL_X: return 1e30;
		case DATA_CHANNEL_Y: return 1e30; // we don't want guides for either latitude or longitude
		case DATA_CHANNEL_VELOCITY: 
		{
		  switch(m_sfLapOpts.eUnitPreference)
		  {
			  case UNIT_PREFERENCE_KMH: return KMH_TO_MS(25.0);
			  case UNIT_PREFERENCE_MPH: return MPH_TO_MS(20.0);		//	Adjusted by KDJ
			  case UNIT_PREFERENCE_MS: return 5;
		  }
		  return 10.0;
		}
		case DATA_CHANNEL_VELOCITYDELTA: 
		{
		  if(flSpread < 1) return 0.5f;
		  if(flSpread < 5) return 1.0f;
		  if(flSpread < 10) return 2.0f;
		  if(flSpread < 20) return 5.0f;
		  if(flSpread < 40) return 10.0f;
		  if(flSpread < 60) return 15.0f;
		  if(flSpread < 100) return 20.0f;
		  return 2.0;
		}
		case DATA_CHANNEL_DISTANCE: 
		{
		  if(flSpread < 0.001) return 0.0001f;		
		  if(flSpread < 0.005) return 0.0005f;		
		  if(flSpread < 0.010) return 0.0010f;		
		  if(flSpread < 0.050) return 0.0050f;		
		  if(flSpread < 1.000) return 0.1000f;
		  if(flSpread < 10.00) return 1.0000f;
		  if(flSpread < 1000) return 100.0f;
		  if(flSpread < 5000) return 500.0f;
		  if(flSpread < 10000) return 1000.0f;
		  if(flSpread < 50000) return 5000.0f;
		  if(flSpread < 110000) return 10000.0f;
		  if(flSpread < 1100000) return 100000.0f;
		  if(flSpread < 10000000) return 1000000.0f;
		  return 10000000;
		}

		case DATA_CHANNEL_TIME: return 1e30;		//	No guidelines for Y-axis
		case DATA_CHANNEL_TIMESLIP:
		case DATA_CHANNEL_ELAPSEDTIME:
		{
		  if(flSpread < 10) return 1.0f;
		  if(flSpread < 100) return 10.0f;
		  if(flSpread < 1000) return 100.0f;
		  if(flSpread < 5000) return 500.0f;
		  if(flSpread < 10000) return 1000.0f;
		  if(flSpread < 50000) return 5000.0f;
		  if(flSpread < 110000) return 10000.0f;
		  if(flSpread < 1100000) return 100000.0f;
		  if(flSpread < 10000000) return 1000000.0f;
		  return 10000000.0f;
		}
		case DATA_CHANNEL_LAPTIME_SUMMARY:
		{
		  if(flSpread < 5) return 0.5f;
		  if(flSpread < 10) return 1.0f;
		  if(flSpread < 50) return 5.0f;
		  if(flSpread < 100) return 10.0f;
		  if(flSpread < 1100) return 100.0f;
		  if(flSpread < 10000) return 1000.0f;
		  if(flSpread < 50000) return 5000.0f;
		  if(flSpread < 110000) return 10000.0f;
		  if(flSpread < 1100000) return 100000.0f;
		  if(flSpread < 10000000) return 1000000.0f;
		  return 10000000.0f;
		}
		case DATA_CHANNEL_X_ACCEL: return 0.5f;
		case DATA_CHANNEL_Y_ACCEL: return 0.5f;
		case DATA_CHANNEL_Z_ACCEL: return 0.5f;
		case DATA_CHANNEL_TEMP: return 10.0f;
		case DATA_CHANNEL_STRENGTH: return 1.0f;

		default: 
		  if(eChannel >= DATA_CHANNEL_IOIOPIN_START && eChannel < DATA_CHANNEL_IOIOPIN_END ||
			  eChannel >= DATA_CHANNEL_IOIOCUSTOM_START && eChannel < DATA_CHANNEL_IOIOCUSTOM_END ||
			  eChannel >= DATA_CHANNEL_RACEDAC_START && eChannel < DATA_CHANNEL_RACEDAC_END ||
			  eChannel >= DATA_CHANNEL_RACEDACCUSTOM_START && eChannel < DATA_CHANNEL_RACEDACCUSTOM_END)
		  {
			if(flSpread < 1) return m_sfLapOpts.fIOIOHardcoded ? 0.1f : 1e30;
			if(flSpread < 10) return m_sfLapOpts.fIOIOHardcoded ? 1.0f : 1e30;
			if(flSpread < 25) return m_sfLapOpts.fIOIOHardcoded ? 2.5f : 1e30;
			if(flSpread < 50) return m_sfLapOpts.fIOIOHardcoded ? 5.0f : 1e30;
			if(flSpread < 150) return m_sfLapOpts.fIOIOHardcoded ? 20.0f : 1e30;
			if(flSpread < 500) return m_sfLapOpts.fIOIOHardcoded ? 50.0f : 1e30;
			if(flSpread < 10000) return m_sfLapOpts.fIOIOHardcoded ? 1000.0f : 1e30;
			if(flSpread < 100000) return m_sfLapOpts.fIOIOHardcoded ? 5000.0f : 1e30;
			if(flSpread < 1000000) return m_sfLapOpts.fIOIOHardcoded ? 50000.0f : 1e30;
			return m_sfLapOpts.fIOIOHardcoded ? 1.0f : 1e30;		// Original code, and default for non-transformed IOIO data
		  }
		  else if(eChannel >= DATA_CHANNEL_PID_START && eChannel < DATA_CHANNEL_PID_END)
		  {
			if(flSpread < 1) return 0.1f;
			if(flSpread < 10) return 1.0f;
			if(flSpread < 25) return 2.5f;
			if(flSpread < 50) return 5.0f;
			if(flSpread < 150) return 20.0f;
			if(flSpread < 500) return 50.0f;
			if(flSpread < 10000) return 1000.0f;
			if(flSpread < 100000) return 5000.0f;
			if(flSpread < 1000000) return 50000.0f;
			return 1e30;
		  }
		  return 1e30;
		}
  }
  virtual const LAPSUPPLIEROPTIONS& GetDisplayOptions() const override
  {
    return m_sfLapOpts;
  }
  virtual DATA_CHANNEL GetXChannel() const override
  {
    return m_eXChannel;
  }
  virtual const IDataChannel* GetChannel(int iLapId, DATA_CHANNEL eChannel) const override
  {
    return g_pLapDB->GetDataChannel(iLapId, eChannel);
  }
  virtual vector<DATA_CHANNEL> GetYChannels() const override
  {
    return m_lstYChannels;
  }
  virtual void GetResponse(const char* pbData, int cbData, char** ppbResponse, int* pcbResponse)
  {

  }
  void SetupMulticast()
  {
    IP_ADAPTER_INFO sfAdapter[255] = {0};
    ULONG cbAdapter = sizeof(sfAdapter);
    ULONG ret = GetAdaptersInfo(sfAdapter, &cbAdapter);
    int cConnected = 0;
    if(ret == NO_ERROR)
    {
      IP_ADAPTER_INFO* pInfo = &sfAdapter[0];
      while(pInfo)
      {
        if(strcmp(pInfo->IpAddressList.IpAddress.String, "0.0.0.0") != 0)
        {
          // found an adapter.  That means we need a multicast checker
          MulticastListener* pListener = new MulticastListener(&m_sfResponder, pInfo->IpAddressList.IpAddress.String);
          if(pListener->Start())
          {
            m_lstMulticast.push_back(pListener);
          }
          else
          {
            delete pListener;
          }
        }

        pInfo = pInfo->Next;
      }
    }
  }
public:
  vector<DATA_CHANNEL> m_lstYChannels;
  DATA_CHANNEL m_eXChannel;	//	Made public by KDJ
  ArtListBox m_sfYAxis;
  CExtendedLap* m_pReferenceLap;
  ArtListBox m_sfLapList;
  ArtListBox m_sfListBox;
  int m_iRaceId[50];	//	Made public by KDJ
private:
  ArtListBox m_sfXAxis;

  CLapPainter m_sfLapPainter;
//  CLapPainter m_sfSubWindow;
  CLapPainter m_sfSubDisplay;
  CLapPainter m_sfTractionDisplay;

  // lap display style data
  map<const CExtendedLap*,int> m_mapLapHighlightTimes; // stores the highlight times (in milliseconds since phone app start) for each lap.  Set from ILapSupplier calls

  LAPDISPLAYSTYLE m_eLapDisplayStyle;
//  DATA_CHANNEL m_eXChannel;
//  vector<DATA_CHANNEL> m_lstYChannels;
  bool m_fShowTractionCircle;
  bool m_fSmooth;
  bool m_fShowBests;
  bool m_fShowDriverBests;
  bool m_fShowReferenceLap;

//  CExtendedLap* m_pReferenceLap;	//	Made public by KDJ
  map<int,CExtendedLap*> m_mapLaps; // maps from iLapId to a lap object
  HWND m_hWnd;

  map<int,RECT> m_baseWindowPos;

  TCHAR m_szCommentText[512];
  TCHAR m_szMessageStatus[512];

  // what updates are needed.  When we call UpdateUI, it will |= the requested flags onto this, and then do a PostMessage with a custom UPDATEUI message
  // when the UPDATEUI message is received, it will do an UpdateUI_Internal call using all the flags that have been built up, and clear the ones it handled.
  DWORD m_fdwUpdateNeeded;

  // panning/zooming
  float m_flShiftX;
  float m_flShiftY;

  // multicast responses for each network device detected
  vector<MulticastListener*> m_lstMulticast;
  MCResponder m_sfResponder;

//  int m_iRaceId[50];
  ILapSupplier* z_ILapSupplier;
};

DWORD ReceiveThreadProc(LPVOID param)
{
  ILapReceiver* pLaps = (ILapReceiver*)param;
//  bool b_receive_data = pLaps->m_Button_LiveData();
  while(true)
  {
    ReceiveLaps(63939, pLaps);
  }
  return 0;
}

int str_ends_with(const TCHAR * str, const TCHAR * suffix) 
{
  if( str == NULL || suffix == NULL )
    return 0;

  size_t str_len = wcslen(str);
  size_t suffix_len = wcslen(suffix);

  if(suffix_len > str_len)
    return 0;

  return 0 == wcsncmp( str + str_len - suffix_len, suffix, suffix_len );
}

void LoadPitsideSettings(PITSIDE_SETTINGS* pSettings)
{
  pSettings->Default();

  TCHAR szModule[MAX_PATH];
  if(GetAppFolder(szModule,NUMCHARS(szModule)))
  {
    wcsncat(szModule,L"settings.txt", NUMCHARS(szModule));

    ifstream in;
    in.open(szModule);
    if(!in.eof() && !in.fail())
    {
      in>>pSettings->fRunHTTP;
      in>>pSettings->iHTTPPort;
      in>>pSettings->iVelocity;
      in>>pSettings->iMapLines;
	  in>>pSettings->iColorScheme;
	  in>>pSettings->bSmoothYesNo;
	  in>>pSettings->bXAxis_KM;
      in.close();
    }
  }
  else
  {
    // trouble.  just bail.
    return;
  }
}

void InitPlotPrefs(LAPSUPPLIEROPTIONS &p_sfLapOpts)
  {
	for (int i=0; i < 50; i++)
	{
		swprintf(p_sfLapOpts.m_PlotPrefs[i].m_ChannelName, L"Velocity");
		p_sfLapOpts.m_PlotPrefs[i].iDataChannel = DATA_CHANNEL_VELOCITY;
		p_sfLapOpts.m_PlotPrefs[i].iPlotView = true;  //  Default to dsplay as a graph
		p_sfLapOpts.m_PlotPrefs[i].fMinValue = -3.0;    //  Set all lower limits to -3.0
		p_sfLapOpts.m_PlotPrefs[i].fMaxValue = 1000000.0;  //  Set all upper limits to 1000000.0
		p_sfLapOpts.m_PlotPrefs[i].iTransformYesNo = false;  //  Default to display as a graph
		p_sfLapOpts.m_PlotPrefs[i].fTransAValue = 0.0;  //  Set all A constants to 0.0
		p_sfLapOpts.m_PlotPrefs[i].fTransBValue = 1.0;  //  Set all B constants to 1.0
		p_sfLapOpts.m_PlotPrefs[i].fTransCValue = 0.0;  //  Set all C constants to 0.0
		p_sfLapOpts.m_SplitPoints[i].m_sfXPoint = 0.0f;	//	Initialize all split points
		p_sfLapOpts.m_SplitPoints[i].m_sfYPoint = 0.0f;	//	Initialize all split points
		p_sfLapOpts.m_SplitPoints[i].m_sfSectorTime = 0;	//	Initialize all sector times
		p_sfLapOpts.m_SplitPoints[i].m_sfSplitTime = 0.0f;
		p_sfLapOpts.fDrawSplitPoints = false;	//	Default to not show split points
		p_sfLapOpts.m_Tranformations[i].f_CoeffA= -1.0;
		p_sfLapOpts.m_Tranformations[i].f_CoeffB= -1.0;
		p_sfLapOpts.m_Tranformations[i].f_CoeffC= -1.0;
		swprintf(p_sfLapOpts.m_Tranformations[i].c_Name, L"");
		p_sfLapOpts.m_Tranformations[i].b_LoadTrans = false;
	}
  }

void SaveSettings(TCHAR szDBPath[MAX_PATH], PITSIDE_SETTINGS* sfSettings, CMainUI* sfUI)
{
  TCHAR szTempPath[MAX_PATH];
  if(GetTempPathW(NUMCHARS(szTempPath),szTempPath))
  {
	wcscat(szTempPath,L"LatestSettings.txt");

    wofstream out;
    out.open(szTempPath);
    if(!out.eof() && !out.fail())
    {
	  out << sfUI->m_szPath << endl;	//	The file name for the database
	  for (int t=0; t<50; t++)	//	Save the RaceID's currently displayed
	  {
		  out << sfUI->m_iRaceId[t] << endl;
	  }
	  out << sfSettings->fRunHTTP << endl;
      out << sfSettings->iHTTPPort << endl;
      out << sfSettings->iVelocity << endl;
      out << sfSettings->iMapLines << endl;
	  out << sfSettings->iColorScheme << endl;
	  out << sfSettings->bSmoothYesNo << endl;
	  out << sfSettings->bXAxis_KM << endl;

	  out << sfUI->m_sfLapOpts.bShowReferenceLap << endl;
	  out << sfUI->m_sfLapOpts.bSmoothYesNo << endl;
	  out << sfUI->m_sfLapOpts.bTractionCircle << endl;
	  out << sfUI->m_sfLapOpts.bXAxis_KM << endl;
	  out << sfUI->m_sfLapOpts.eSortPreference << endl;
	  out << sfUI->m_sfLapOpts.eUnitPreference << endl;
	  out << sfUI->m_sfLapOpts.e_Orientation << endl;
	  out << sfUI->m_sfLapOpts.fColorScheme << endl;
	  out << sfUI->m_sfLapOpts.fDrawGuides << endl;
	  out << sfUI->m_sfLapOpts.fDrawLines << endl;
	  out << sfUI->m_sfLapOpts.fDrawSplitPoints << endl;
	  out << sfUI->m_sfLapOpts.fElapsedTime << endl;
	  out << sfUI->m_sfLapOpts.fIOIOHardcoded << endl;
	  out << sfUI->m_sfLapOpts.flWindowShiftX << endl;
	  out << sfUI->m_sfLapOpts.flWindowShiftY << endl;
	  out << sfUI->m_sfLapOpts.iZoomLevels << endl;

	  for (int i=0; i < 50; i++)
	  {
		out << sfUI->m_sfLapOpts.m_PlotPrefs[i].iDataChannel << endl;	//	Save the Data Channel number
		out << sfUI->m_sfLapOpts.m_PlotPrefs[i].m_ChannelName << endl;	//	Save the Data Channel Name
		out << sfUI->m_sfLapOpts.m_PlotPrefs[i].iPlotView << endl;  //  Save how channel is displayed (graph/value)
		out << sfUI->m_sfLapOpts.m_PlotPrefs[i].fMinValue << endl;    //  Save the lower limit for the Alarm
		out << sfUI->m_sfLapOpts.m_PlotPrefs[i].fMaxValue << endl;  //  Save the upper limit for the Alarm
		out << sfUI->m_sfLapOpts.m_PlotPrefs[i].iTransformYesNo << endl;  //  Save whether channel is transformed
		out << sfUI->m_sfLapOpts.m_PlotPrefs[i].fTransAValue << endl;  //  Save A constants
		out << sfUI->m_sfLapOpts.m_PlotPrefs[i].fTransBValue << endl;  //  Save B constants
		out << sfUI->m_sfLapOpts.m_PlotPrefs[i].fTransCValue << endl;  //  Save C constants
		out << sfUI->m_sfLapOpts.m_SplitPoints[i].m_sfXPoint << endl;	//	Save split points
		out << sfUI->m_sfLapOpts.m_SplitPoints[i].m_sfYPoint << endl;	//	Save split points
		out << sfUI->m_sfLapOpts.m_SplitPoints[i].m_sfSectorTime << endl;	//	Save sector times
		out << sfUI->m_sfLapOpts.m_SplitPoints[i].m_sfSplitTime << endl;	//	Save best sector time
	  }
		out << "End of Plot Prefs" << endl;

	  out << sfUI->m_sfLapOpts.fDrawSplitPoints << endl;	//	Save setting to show/hide split points
	  if ( sfUI->m_pReferenceLap )
		out << sfUI->m_pReferenceLap->m_pLap->GetLapId() << endl;	//	Save the Reference Lap ID
	  else
		out << "0" << endl;

	  out << sfUI->m_eXChannel << endl;	//	Save X-Axis data channel
	  for (int i=0; i < sfUI->m_lstYChannels.size(); i++)	//	Save the list of selected Y-Axis channels
	  {
		  out << sfUI->m_lstYChannels[i] << endl;	//	Save the list of selected Y-Axis channels
	  }
	  out << "//" << endl;	//	End of file marker
	  out.close();
    }
  
  }
}

int LoadSettings(TCHAR szDBPath[MAX_PATH], PITSIDE_SETTINGS* sfSettings, CMainUI* sfUI)
{
  TCHAR szTempPath[MAX_PATH];
  if(GetTempPathW(NUMCHARS(szTempPath),szTempPath))
  {
	wcscat(szTempPath,L"LatestSettings.txt");

    vector<string> lines;
    ifstream in;
    in.open(szTempPath);
	//	Read in all of the lines of the "LatestSettings.txt" file at once, or it doesn't work right
	//	Store them in the 'lines' vector
	for (int i = 0; i < 1000; i++)
	{
		string Line;
		if(!in.eof() && !in.fail())
		{
			getline(in, Line);
			if (Line != "//")
			{
				lines.push_back(Line);
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
    }
	in.close();

	//	Now let's process the information and load it in the sfUI data structure
	string Line = lines[0];
	int t;
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	swprintf(sfUI->m_szPath, NUMCHARS(sfUI->m_szPath), c_Name);	//	Load name of last-processed file in sfUI Pointer for szDBPath
	}
	for (t=1; t<=50; t++)	//	Load the RaceID's to be displayed
	{
		Line = lines[t];
		{
		TCHAR *c_Name = new TCHAR[Line.size()+1];
		c_Name[Line.size()] = 0;
		copy(Line.begin(), Line.end(), c_Name);
		sfUI->m_iRaceId[t-1] = _wtoi(c_Name);	//	Load the last Race session ID's into Pitside Console for loading and display
		}
	}
	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfSettings->fRunHTTP = _wtoi(c_Name);	//	Load the boolean on whether to run the HTTP server into sfSettings data structure
	}
	
	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfSettings->iHTTPPort = _wtoi(c_Name);	//	Load the HTTP port number into sfSettings data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfSettings->iVelocity = _wtoi(c_Name);	//	Load the Velocity (km/mi/mps) into sfSettings data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfSettings->iMapLines = _wtoi(c_Name);	//	Load the boolean for map lines display into sfSettings data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfSettings->iColorScheme = _wtoi(c_Name);	//	Load the color scheme into sfSettings data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfSettings->bSmoothYesNo = _wtoi(c_Name);	//	Load the boolean for accel smoothing into sfSettings data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfSettings->bXAxis_KM = _wtoi(c_Name);	//	Load the Distance setting into sfSettings data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.bShowReferenceLap = _wtoi(c_Name);	//	Load the booelean to display Ref Lap into LapOpts data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.bSmoothYesNo = _wtoi(c_Name);	//	Load the boolean for accel smoothing into LapOpts data structuree
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.bTractionCircle = _wtoi(c_Name);	//	Load the boolean to display Traction Circle into LapOpts data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.bXAxis_KM = _wtoi(c_Name);	//	Load the Distance setting into LapOpts data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.eSortPreference = (LAPSORTSTYLE)_wtoi(c_Name);	//	Load the Lap Sort pref into LapOpts data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.eUnitPreference = (UNIT_PREFERENCE)_wtoi(c_Name);	//	Load the kph/mph/mps seeting into LapOpts data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.e_Orientation = (ORIENTATION)_wtoi(c_Name);	//	Load the Phone Orientation into LapOpts data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.fColorScheme = _wtoi(c_Name);	//	Load the Color Scheme into LapOpts data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.fDrawGuides = _wtoi(c_Name);	//	Load the Guidelines Drawing boolean into LapOpts data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.fDrawLines = _wtoi(c_Name);	//	Load the Lines/Dots Drawing boolean into LapOpts data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.fDrawSplitPoints = _wtoi(c_Name);	//	Load the Split Points boolean into LapOpts data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.fElapsedTime = _wtoi(c_Name);	//	Load the Elapsed Time boolean into LapOpts data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.fIOIOHardcoded = _wtoi(c_Name);	//	Load the IOIO Hardcoded boolean into LapOpts data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.flWindowShiftX = _wtof(c_Name);	//	Load the Window ShiftX into LapOpts data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.flWindowShiftY = _wtof(c_Name);	//	Load the Window ShiftY into LapOpts data structure
	}

	Line = lines[t++];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.iZoomLevels = _wtoi(c_Name);	//	Load the Zoom Level into LapOpts data structure
	}

	int arraycounter = 0;	//	Now load the Plotting Preferences into the m_sfLapOpts.PlotPrefs array
	for (int i = t; i < lines.size() && arraycounter < 50; i+=13)
	{
		Line = lines[i];
		{
		TCHAR *c_Name = new TCHAR[Line.size()+1];
		c_Name[Line.size()] = 0;
		copy(Line.begin(), Line.end(), c_Name);
		sfUI->m_sfLapOpts.m_PlotPrefs[arraycounter].iDataChannel = (DATA_CHANNEL)_wtoi(c_Name);    //  Load the data channel
		}

		Line = lines[i+1];
		{
		TCHAR *c_Name = new TCHAR[Line.size()+1];
		c_Name[Line.size()] = 0;
		copy(Line.begin(), Line.end(), c_Name);
		swprintf(sfUI->m_sfLapOpts.m_PlotPrefs[arraycounter].m_ChannelName, NUMCHARS(sfUI->m_sfLapOpts.m_PlotPrefs[arraycounter].m_ChannelName), c_Name);    //  Load the data channel
		}

		Line = lines[i+2];
		{
		TCHAR *c_Name = new TCHAR[Line.size()+1];
		c_Name[Line.size()] = 0;
		copy(Line.begin(), Line.end(), c_Name);
		sfUI->m_sfLapOpts.m_PlotPrefs[arraycounter].iPlotView = _wtoi(c_Name);    //  Load current display mode for channel
		}

		Line = lines[i+3];
		{
		TCHAR *c_Name = new TCHAR[Line.size()+1];
		c_Name[Line.size()] = 0;
		copy(Line.begin(), Line.end(), c_Name);
		sfUI->m_sfLapOpts.m_PlotPrefs[arraycounter].fMinValue = _wtof(c_Name);    //  Load the lower limit for channel
		}

		Line = lines[i+4];
		{
		TCHAR *c_Name = new TCHAR[Line.size()+1];
		c_Name[Line.size()] = 0;
		copy(Line.begin(), Line.end(), c_Name);
		sfUI->m_sfLapOpts.m_PlotPrefs[arraycounter].fMaxValue = _wtof(c_Name);    //  Load the upper limit for channel
		}

		Line = lines[i+5];
		{
		TCHAR *c_Name = new TCHAR[Line.size()+1];
		c_Name[Line.size()] = 0;
		copy(Line.begin(), Line.end(), c_Name);
		sfUI->m_sfLapOpts.m_PlotPrefs[arraycounter].iTransformYesNo = _wtoi(c_Name);    //  Load the transform flag for channel
		}

		Line = lines[i+6];
		{
		TCHAR *c_Name = new TCHAR[Line.size()+1];
		c_Name[Line.size()] = 0;
		copy(Line.begin(), Line.end(), c_Name);
		sfUI->m_sfLapOpts.m_PlotPrefs[arraycounter].fTransAValue = _wtof(c_Name);    //  Load the A constant for channel
		}

		Line = lines[i+7];
		{
		TCHAR *c_Name = new TCHAR[Line.size()+1];
		c_Name[Line.size()] = 0;
		copy(Line.begin(), Line.end(), c_Name);
		sfUI->m_sfLapOpts.m_PlotPrefs[arraycounter].fTransBValue = _wtof(c_Name);    //  Load the B constant for channel
		}

		Line = lines[i+8];
		{
		TCHAR *c_Name = new TCHAR[Line.size()+1];
		c_Name[Line.size()] = 0;
		copy(Line.begin(), Line.end(), c_Name);
		sfUI->m_sfLapOpts.m_PlotPrefs[arraycounter].fTransCValue = _wtof(c_Name);    //  Load the C constant for channel
		}

		Line = lines[i+9];
		{
		TCHAR *c_Name = new TCHAR[Line.size()+1];
		c_Name[Line.size()] = 0;
		copy(Line.begin(), Line.end(), c_Name);
		sfUI->m_sfLapOpts.m_SplitPoints[arraycounter].m_sfXPoint = _wtof(c_Name);    //  Load the split points
		}

		Line = lines[i+10];
		{
		TCHAR *c_Name = new TCHAR[Line.size()+1];
		c_Name[Line.size()] = 0;
		copy(Line.begin(), Line.end(), c_Name);
		sfUI->m_sfLapOpts.m_SplitPoints[arraycounter].m_sfYPoint = _wtof(c_Name);    //  Load the split points
		}

		Line = lines[i+11];
		{
		TCHAR *c_Name = new TCHAR[Line.size()+1];
		c_Name[Line.size()] = 0;
		copy(Line.begin(), Line.end(), c_Name);
		sfUI->m_sfLapOpts.m_SplitPoints[arraycounter].m_sfSectorTime = _wtoi(c_Name);    //  Load all sector times
		}

		Line = lines[i+12];
		{
		TCHAR *c_Name = new TCHAR[Line.size()+1];
		c_Name[Line.size()] = 0;
		copy(Line.begin(), Line.end(), c_Name);
		sfUI->m_sfLapOpts.m_SplitPoints[arraycounter].m_sfSplitTime = _wtof(c_Name);    //  Load the lower limit for channel
		}
		arraycounter++;
	}
	t = t + (arraycounter) * 13;	//	Reset the line counter to collect the next set of data

	Line = lines[t];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
//	sfUI->m_sfLapOpts.fDrawSplitPoints = _wtoi(c_Name);    //  End of Data Channels
	}

	t++;
	Line = lines[t];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.fDrawSplitPoints = _wtoi(c_Name);    //  Load the setting to show split points
	}

	t++;
	Line = lines[t];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_sfLapOpts.LapId = _wtoi(c_Name);	//	Get the LapId for the reference lap
	}

	t++; 
	Line = lines[t];
	{
	TCHAR *c_Name = new TCHAR[Line.size()+1];
	c_Name[Line.size()] = 0;
	copy(Line.begin(), Line.end(), c_Name);
	sfUI->m_eXChannel = (DATA_CHANNEL)_wtoi(c_Name);	//	Load the X-Axis data channel
	}
	t++; 
	
	sfUI->m_lstYChannels.clear();	//	Clear the current list of Data channels
	for (t; t < lines.size(); t++ )
	{
		Line = lines[t];
		{
		TCHAR *c_Name = new TCHAR[Line.size()+1];
		c_Name[Line.size()] = 0;
		copy(Line.begin(), Line.end(), c_Name);
		sfUI->m_lstYChannels.push_back( (DATA_CHANNEL)_wtoi(c_Name) );	//	Load the Y-Axis data channels
		}
	}
	
	in.close();
  }
  return 1;
}

//	Converts ANSI strings to UNICODE strings
BOOL AnsiToUnicode( LPSTR  pszAnsiString, LPWSTR pszwUniBuff, DWORD dwUniBuffSize )
{
	int iRet = 0;
    iRet = MultiByteToWideChar(CP_ACP, 0, pszAnsiString, -1, pszwUniBuff, dwUniBuffSize);
	return ( 0 != iRet );
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)

{
  INITCOMMONCONTROLSEX initCtrls;
  initCtrls.dwICC = ICC_LISTVIEW_CLASSES;
  initCtrls.dwSize = sizeof(initCtrls);
  LoadIcon(hInstance,_T("IDI_EXPLORERICON"));
  InitCommonControlsEx(&initCtrls);

  CMainUI sfUI;
  g_pUI = &sfUI;
  //CLapReceiver sfLaps(&sfUI);

  CSQLiteLapDB sfLaps(&sfUI);
  bool fDBOpened = false;

  int iRaceId[50] = {0};
  TCHAR szDBPath[MAX_PATH] = {NULL};	//	Filename and path are contained in this string
  TCHAR szTemp[512] = {NULL};
  TCHAR szLoadFileName[MAX_PATH] = {NULL};
  TCHAR wCmdLine[MAX_PATH] = {NULL};
  AnsiToUnicode(lpCmdLine, wCmdLine, sizeof(wCmdLine));	//	Convert ANSI command line to UNICODE command

  PITSIDE_SETTINGS sfSettings;
  LoadPitsideSettings(&sfSettings);		//	Load preferences from "Settings.txt" file
  LAPSUPPLIEROPTIONS x_sfLapOpts; //sfLapOpts contains all lap display options
  InitPlotPrefs(x_sfLapOpts);	//	Initialize all PlotPrefs variables before displaying anything
  sfUI.SetDisplayOptions(x_sfLapOpts);	//	Link x_sfLapOpts with CMainUI pointer
  sfUI.SetRaceId(&iRaceId[0]);
  x_sfLapOpts.LapId = 0;


  if(strcmp(lpCmdLine,"unit") == 0)	//	Check for special test command line
  {
    return UnitTests();
	DWORD dwRet = MessageBox(NULL,wCmdLine,L"Unit", MB_APPLMODAL | MB_ICONWARNING | MB_OK | MB_TOPMOST | MB_DEFBUTTON2);
  }
  else if( str_ends_with( wCmdLine, L".wflp\"") )	//	Check if user clicked on an associated WFLP file and load it
  {
	  //	First we must remove the quote marks from the beginning and end of the file name in the command line
	  int start = 1;
	  int end = NUMCHARS(wCmdLine);
	  for (int q=start; q<end; q++)
	  {
		  szLoadFileName[q-1] = wCmdLine[q];
		  if (wCmdLine[q] == NULL)
		  {
			  szLoadFileName[q-2] = NULL;	//	Remove the final "
			  break;
		  }
	  }
	  //	Now let's put the file name/path into our loading string, szDBPath
	  wcsncat(szDBPath, szLoadFileName, NUMCHARS(szLoadFileName));
  }
  else
  {
	wcscat(szDBPath,L"NewDatabase.wflp");	//	Retrieve last file failed. Load a new database
	if(ArtGetSaveFileName(NULL,L"Select .wflp to open or save to",szDBPath,NUMCHARS(szDBPath),L"WifiLapper Files (*.wflp)\0*.WFLP\0\0"))
	{
		// let's make sure there's a .wflp suffix on that bugger.
		if(!str_ends_with(szDBPath,L".wflp") && !str_ends_with(szDBPath,L".WFLP"))
		{
			wcsncat(szDBPath,L".wflp", NUMCHARS(szDBPath));
		}
	}
	else
	{
		return 0;	//	User cancelled load file operation, just exit the program
	}
  }

  const bool fFileIsNew = !DoesFileExist(szDBPath);
  // they chose one to open, so open it.
  if(sfLaps.Init(szDBPath))
  {
	if(!fFileIsNew)
	{
		  TCHAR szTempPath[MAX_PATH];
		  if(GetTempPathW(NUMCHARS(szTempPath),szTempPath))
		  {
			wcscat(szTempPath,L"LatestSettings.txt");
			vector<string> lines;
			ifstream in;
			if ( DoesFileExist(szTempPath) )	//	Make sure that the file exists before opening it
			{
				in.open(szTempPath);
				//	Read in the first line of the "LatestSettings.txt" file
				//	Store them in the 'lines' vector
				string Line;
				if(!in.eof() && !in.fail())
				{
					getline(in, Line);
					if (Line != "//")
					{
						lines.push_back(Line);
					}
				}
				in.close();
				TCHAR szText[MAX_PATH];

				//	Now let's process the information and load it in the sfUI data structure
				{
					string Line = lines[0];
					TCHAR *c_Name = new TCHAR[Line.size()+1];
					c_Name[Line.size()] = 0;
					copy(Line.begin(), Line.end(), c_Name);
					swprintf(szText, NUMCHARS(szText), c_Name);	//	Load name of last-processed file in szDBPath
				}
				if ( _wcsnicmp( szText, szDBPath, NUMCHARS(szDBPath) ) == 0 )	//	Opening the same file, so load see if user wants to return to previous session
				{
					int iRet = MessageBoxW(NULL,L"Do you want to pick up where you left off?",L"Database loaded previously",MB_ICONERROR | MB_YESNO);
					if(iRet == IDYES)
					{
							LoadSettings(szDBPath, &sfSettings, &sfUI);	//	Let's overwrite the default preferences with the settings from the last instance run
							for (int t=0; t<50; t++)
							{
								iRaceId[t]=sfUI.m_iRaceId[t];
							}
							fDBOpened = true;
					}
					else
					{
						// show the race-selection dialog
						RACESELECT_RESULT sfRaceResult;
						CRaceSelectDlg sfRaceSelect(&sfLaps,&sfRaceResult);
						::ArtShowDialog<IDD_SELECTRACE>(&sfRaceSelect);
						if(!sfRaceResult.fCancelled)
						{
							for (int z = 0; z < 50; z++)
							{
								iRaceId[z] = sfRaceResult.iRaceId[z];	//	Load the first selected race session
								sfUI.m_iRaceId[z] = sfRaceResult.iRaceId[z];	//	Load these RaceId's into the global pointer
							}
							fDBOpened = true;
						}
						else
						{
							iRaceId[0] = -1;
							sfUI.m_iRaceId[0] = -1;
							fDBOpened = true;
						}
					}
				}
				else
				{
					// show the race-selection dialog
					RACESELECT_RESULT sfRaceResult;
					CRaceSelectDlg sfRaceSelect(&sfLaps,&sfRaceResult);
					::ArtShowDialog<IDD_SELECTRACE>(&sfRaceSelect);
					if(!sfRaceResult.fCancelled)
					{
						for (int z = 0; z < 50; z++)
						{
							iRaceId[z] = sfRaceResult.iRaceId[z];	//	Load the first selected race session
							sfUI.m_iRaceId[z] = sfRaceResult.iRaceId[z];	//	Load these RaceId's into the global pointer
						}
						fDBOpened = true;
					}
					else
					{
						iRaceId[0] = -1;
						sfUI.m_iRaceId[0] = -1;
						fDBOpened = true;
					}
				}
			}
			else
			{
				// show the race-selection dialog
				RACESELECT_RESULT sfRaceResult;
				CRaceSelectDlg sfRaceSelect(&sfLaps,&sfRaceResult);
				::ArtShowDialog<IDD_SELECTRACE>(&sfRaceSelect);
				if(!sfRaceResult.fCancelled)
				{
					for (int z = 0; z < 50; z++)
					{
						iRaceId[z] = sfRaceResult.iRaceId[z];	//	Load the first selected race session
						sfUI.m_iRaceId[z] = sfRaceResult.iRaceId[z];	//	Load these RaceId's into the global pointer
					}
					fDBOpened = true;
				}
				else
				{
					iRaceId[0] = -1;
					sfUI.m_iRaceId[0] = -1;
					fDBOpened = true;
				}
			}
		  }
	}
	else
	{
		iRaceId[0] = -1;
		sfUI.m_iRaceId[0] = -1;
		fDBOpened = true;
	}
  }
  
  if(!fDBOpened)
  {
    // they didn't choose a file, so just use a temp DB.
    TCHAR szTempPath[MAX_PATH];
    GetTempPath(NUMCHARS(szTempPath),szTempPath);
    wcscat(szTempPath,L"pitsidetemp.wflp");
    if(sfLaps.Init(szTempPath))
    {
      // success!
      wcscpy(szDBPath,szTempPath);
	  iRaceId[0] = -1;
	  sfUI.m_iRaceId[0] = -1;
	  fDBOpened = true;
    }
  }

  if(!fDBOpened)
  {
    // disaster.
    MessageBox(NULL,L"Pitside was unable to create a database to save data to.  Is your hard drive full?",L"Failed to create DB",MB_ICONERROR);
    exit(0);
  }
  g_pLapDB = &sfLaps;

  switch (sfSettings.iVelocity)
  {
  case 0:
          {
            x_sfLapOpts.eUnitPreference = UNIT_PREFERENCE_KMH;
			break;
          }
  case 1:
          {
            x_sfLapOpts.eUnitPreference = UNIT_PREFERENCE_MPH;
			break;
          }
  case 2:
          {
            x_sfLapOpts.eUnitPreference = UNIT_PREFERENCE_MS;
			break;
          }
  default:
          {
            x_sfLapOpts.eUnitPreference = UNIT_PREFERENCE_KMH;
          }
  }
  switch (sfSettings.iMapLines)
  {
  case 0:
  case 1:
          {
            x_sfLapOpts.fDrawLines = sfSettings.iMapLines;
			break;
          }
  default:
          {
            x_sfLapOpts.fDrawLines = true;
          }
  }
  switch (sfSettings.iColorScheme)
  {
  case 0:
  case 1:
          {
            x_sfLapOpts.fColorScheme = sfSettings.iColorScheme;	//	Assign color scheme from Settings.txt file
			break;
          }
  default:
          {
            x_sfLapOpts.fColorScheme = false;	//	Grey background as a default, true = black
          }
  }
  switch (sfSettings.bSmoothYesNo)
  {
  case 0:
  case 1:
          {
            x_sfLapOpts.bSmoothYesNo = sfSettings.bSmoothYesNo;	//	Assign smoothing setting from Settings.txt file
			break;
          }
  }
  switch (sfSettings.bXAxis_KM)
  {
  case 0:
  case 1:
          {
			x_sfLapOpts.bXAxis_KM = sfSettings.bXAxis_KM;	//	Assign choice of LAT or KM for X-Axis setting from Settings.txt file
			break;
          }
  }
  x_sfLapOpts.eSortPreference = SORTSTYLE_BYTIMEOFRACE;		//	Default sort Lap List by time of lap
  swprintf(sfUI.m_szPath, NUMCHARS(sfUI.m_szPath), szDBPath);	//	Load name of the chosen file into the sfUI pointer

  PitsideHTTP aResponder(g_pLapDB,&sfUI);
  if(sfSettings.fRunHTTP && sfSettings.iHTTPPort > 0 && sfSettings.iHTTPPort < 65536)
  {
    g_pHTTPServer = new SimpleHTTPServer();

    bool fTryAgain = false;
    do
    {
      fTryAgain = false;
      if(!g_pHTTPServer->Init(sfSettings.iHTTPPort,&aResponder))
      {
        TCHAR szMsg[512];
        _snwprintf(szMsg,NUMCHARS(szMsg),L"Pitside was unable to start the HTTP server on port %d. Do you want to open settings.txt to try another port or disable the server?\n\nNOTE: This will also stop all communication from the phone to Pitside",sfSettings.iHTTPPort);
        int iRet = MessageBox(NULL,szMsg,L"Failed to start HTTP server",MB_ICONERROR | MB_YESNO);
        if(iRet == IDYES)
        {
          TCHAR szPath[MAX_PATH];
          if(GetAppFolder(szPath,NUMCHARS(szPath)))
          {
            wcscat(szPath,L"settings.txt");
            _wsystem(szPath);

            LoadPitsideSettings(&sfSettings);
            fTryAgain = sfSettings.fRunHTTP;
          }
        }
      }
    }
    while(fTryAgain);
  }
  else
  {
    g_pHTTPServer = NULL;
  }
  if(sfSettings.fRunHTTP)	//	Only try to receive phone information when the HTTP server is turned on
  {
	  HANDLE hRecvThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&ReceiveThreadProc, (LPVOID)&sfLaps, 0, NULL);
  }
  //	Show the Open Dialog and start Pitside Console
  ArtShowDialog<IDD_DLGFIRST>(&sfUI);

  //	Exiting the program. 
  SaveSettings(szDBPath, &sfSettings, &sfUI);	//	Let's save as many preferences and current settings as we can for next execution cycle before closing

  exit(0);	//
}

