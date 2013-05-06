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
#include "DlgSplash.h"
#include "SQLiteLapDB.h"
#include "UnitTests.h"
#include <fstream>
#include "Winuser.h"


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

// supplier IDs - each lap painter is given a supplier ID, which it uses to identify itself when asking for more data
enum SUPPLIERID
{
  SUPPLIERID_MAINDISPLAY,
  SUPPLIERID_SUBDISPLAY,
};

class CMainUI : public IUI,public ILapSupplier
{
public:
  CMainUI() 
    : m_sfLapPainter(/*static_cast<IUI*>(this), */static_cast<ILapSupplier*>(this),SUPPLIERID_MAINDISPLAY), 
      m_sfSubDisplay(/*static_cast<IUI*>(this), */static_cast<ILapSupplier*>(this),SUPPLIERID_SUBDISPLAY), 
      m_eLapDisplayStyle(LAPDISPLAYSTYLE_PLOT),		//	Make data plot the default initial view
      m_fShowBests(false), 
      m_fShowDriverBests(false),
	  m_fShowReferenceLap(true),
      m_pReferenceLap(NULL),
      m_eXChannel(DATA_CHANNEL_DISTANCE),
      m_fdwUpdateNeeded(0),
      m_flShiftX(0),
      m_flShiftY(0),
      m_iRaceId(0)
  {
    m_lstYChannels.push_back(DATA_CHANNEL_VELOCITY);
    m_szCommentText[0] = '\0';
    m_szMessageStatus[0] = '\0';
    SetupMulticast();
  }
  void SetRaceId(int iRaceId)
  {
    m_iRaceId = iRaceId;
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

  LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    if(m_sfLapPainter.HandleMessage(hWnd,uMsg,wParam,lParam))
    {
      return 0;
    }
    switch(uMsg)
	  {
	    case WM_INITDIALOG:
      {
        m_hWnd = hWnd;
        
        {
          vector<wstring> lstCols;
          vector<int> lstWidths;
          CExtendedLap::GetStringHeadersXAxis(lstCols,lstWidths);
          m_sfXAxis.Init(GetDlgItem(m_hWnd, IDC_XAXIS),lstCols,lstWidths);
		}
        {
          vector<wstring> lstCols;
          vector<int> lstWidths;
          CExtendedLap::GetStringHeadersYAxis(lstCols,lstWidths);
          m_sfYAxis.Init(GetDlgItem(m_hWnd, IDC_YAXIS),lstCols,lstWidths);
        }
        {
          vector<wstring> lstCols;
          vector<int> lstWidths;
          CExtendedLap::GetStringHeaders(lstCols,lstWidths);
          m_sfLapList.Init(GetDlgItem(hWnd, IDC_LAPS), lstCols,lstWidths);
        }
        m_sfLapPainter.Init(GetDlgItem(hWnd,IDC_DISPLAY));
        m_sfSubDisplay.Init(GetDlgItem(hWnd,IDC_SUBDISPLAY));

        set<DATA_CHANNEL> setAvailable;
        InitAxes(setAvailable);
        LoadLaps(::g_pLapDB);
        UpdateUI(UPDATE_ALL);
        InitBaseWindowPos();

		return 0;
      }
      case WM_CLOSE:
        EndDialog(hWnd,0);
        return 0;
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

        if(IS_FLAG_SET(wParam, MK_LBUTTON))
        {
          // they're dragging!
          m_sfLapOpts.flWindowShiftX += moveX;
          m_sfLapOpts.flWindowShiftY -= moveY;
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
      case WM_LBUTTONDBLCLK:
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
		  //	Now display the closest values on the map.
/*			const float flDataX = MagicDeterminingFunction(sfLapOpts, fHighlightXAxis); // this part could be hard...
			vector<CExtendedLap*> lstLaps = GetLapsToShow();
			if(lstLaps.size() >= 2) // don't show anything if we've got multiple laps selected
			{
				set<DATA_CHANNEL> setChannels = lstLaps[0]->GetAvailableChannels();
				stringstream ss;
				for(set<DATA_CHANNEL>::const_iterator i = setChannels.begin(); i != setChannels.end(); i++)
				{
					const IDataChannel* pChannel = GetChannel(*i);
					if(pChannel)
					{
						const float flValue = pChannel->GetValue(flDataX);
						TCHAR szName[100];
						GetDataChannelName(*i,szName,NUMCHARS(szName));
						char szValue[100];
						GetChannelString(*i, m_sfLapOpts.eUnitPreference, flValue, szValue, NUMCHARS(szValue));
						ss<<szName;
						ss<<szValue;
						ss<<endl;
					}
				}
				MessageBox(NULL, ss.c_str(), NULL, NULL);

				}
*/          return TRUE;
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
              NMITEMACTIVATE* pDetails = (NMITEMACTIVATE*)notifyHeader;
              if(pDetails->iItem >= 0)
              {
                UpdateUI(UPDATE_MAP | UPDATE_DASHBOARD);
              }
              return TRUE;
            }
            break;
          case IDC_XAXIS:
            switch(notifyHeader->code)
            {
            case LVN_ITEMCHANGED:
              const set<LPARAM> sel = m_sfXAxis.GetSelectedItemsData();
              if(sel.size() == 1)
              {
                m_eXChannel = (DATA_CHANNEL)*sel.begin();
                NMITEMACTIVATE* pDetails = (NMITEMACTIVATE*)notifyHeader;
                if(pDetails->iItem >= 0)
                {
                  UpdateUI(UPDATE_MAP | UPDATE_DASHBOARD);
                }
              }
              return TRUE;
            }
            break;
          case IDC_YAXIS:
            switch(notifyHeader->code)
            {
            case LVN_ITEMCHANGED:
              const set<LPARAM> sel = m_sfYAxis.GetSelectedItemsData();
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
                  UpdateUI(UPDATE_MAP | UPDATE_DASHBOARD);
                }
              }
              return TRUE;
            }
            break;
        } // end switch on wParam
      } // end body of case WM_NOTIFY
	  case WM_COMMAND:
      {
	    switch(LOWORD(wParam)) // find out the control ID
	    {
          case IDC_SENDMESSAGE:
          {
            MESSAGEDLG_RESULT sfResult;
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
          case ID_OPTIONS_SHOWBESTS:
          {
            m_fShowBests = !m_fShowBests;
            UpdateUI(UPDATE_MENU | UPDATE_MAP | UPDATE_DASHBOARD);
            return TRUE;
          }
          case ID_OPTIONS_SHOWREFERENCELAP:
          {
            m_fShowReferenceLap = !m_fShowReferenceLap;
            UpdateUI(UPDATE_MENU | UPDATE_MAP | UPDATE_DASHBOARD);
            return TRUE;
          }
          case ID_OPTIONS_DRAWLINES:
          {
            m_sfLapOpts.fDrawLines = !m_sfLapOpts.fDrawLines;
            UpdateUI(UPDATE_MENU | UPDATE_MAP | UPDATE_DASHBOARD);
            return TRUE;
          }
          case ID_OPTIONS_BACKGROUND:
          {
            m_sfLapOpts.fColorScheme = !m_sfLapOpts.fColorScheme;
            UpdateUI(UPDATE_MENU | UPDATE_MAP | UPDATE_DASHBOARD);
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
            UpdateUI(UPDATE_MENU | UPDATE_MAP | UPDATE_DASHBOARD);
            return TRUE;
          }
          case ID_DATA_SWITCHSESSION:
          {
            RACESELECT_RESULT sfResult;
            CRaceSelectDlg dlgRace(g_pLapDB, &sfResult);
            ArtShowDialog<IDD_SELECTRACE>(&dlgRace);

            if(!sfResult.fCancelled)
            {
              m_iRaceId = sfResult.iRaceId;
              ClearUILaps();
              LoadLaps(g_pLapDB);
              UpdateUI(UPDATE_ALL);
            }
            return TRUE;
          }
          case ID_DATA_EDITSESSION:
          {
            RACESELECTEDIT_RESULT sfResult;
            CRaceSelectEditDlg dlgRace(g_pLapDB, &sfResult);
            ArtShowDialog<IDD_SELECTRACEEDIT>(&dlgRace);

            if(!sfResult.fCancelled)
            {
              m_iRaceId = sfResult.iRaceId;
              ClearUILaps();
              LoadLaps(g_pLapDB);
              UpdateUI(UPDATE_ALL);
            }
            return TRUE;
          }
		  case ID_OPTIONS_PLOTPREFS:
		  {
			PLOTSELECT_RESULT sfResult;
			CPlotSelectDlg dlgPlot(g_pLapDB, &sfResult, m_iRaceId, &m_sfLapOpts);
			ArtShowDialog<IDD_PLOTPREFS>(&dlgPlot);

			if(!sfResult.fCancelled)
			{
				UpdateUI(UPDATE_ALL);
			}
					
			return TRUE;
		  }		
          case ID_HELP_IPS:
          {
            ShowNetInfo();
            return TRUE;
          }
          case ID_HELP_ABOUT:
          {
			  ShowAbout();
			  return TRUE;
          }
          case ID_FILE_PRINT:
          {
			  HRESULT WINAPI PrintDlgEx(_Inout_  LPPRINTDLGEX lppd);
			  return TRUE;
          }
          case ID_FILE_EXIT:
          {
				DestroyWindow(hWnd);
				break;
		  }
          case ID_EDIT_COPY:
          {
			  return TRUE;
          }
          
		  case ID_DATA_OPENDB:
          {
            TCHAR szFilename[MAX_PATH];
            if(ArtGetOpenFileName(hWnd, L"Choose WFLP file", szFilename, NUMCHARS(szFilename),L"WifiLapper Files (*.wflp)\0*.WFLP\0\0"))
            {
              if(g_pLapDB->Init(szFilename))
              {
                RACESELECT_RESULT sfResult;
                CRaceSelectDlg dlgRace(g_pLapDB, &sfResult);
                ArtShowDialog<IDD_SELECTRACE>(&dlgRace);

                if(!sfResult.fCancelled)
                {
                  m_iRaceId = sfResult.iRaceId;
                  ClearUILaps();
                  LoadLaps(g_pLapDB);
                  UpdateUI(UPDATE_ALL);
                }
              }
            }
            return TRUE;
          }
          case ID_DATA_DASHWARE:
          {
            set<LPARAM> setSelectedData = m_sfLapList.GetSelectedItemsData();
            if(setSelectedData.size() > 0)
            {
              TCHAR szFilename[MAX_PATH];
              if(ArtGetSaveFileName(hWnd, L"Choose Output file", szFilename, NUMCHARS(szFilename),L"CSV Files (*.csv)\0*.CSV\0\0"))
              {
                vector<const ILap*> lstLaps;
                map<const ILap*, const IDataChannel*> mapData;
                for(set<LPARAM>::iterator i = setSelectedData.begin(); i != setSelectedData.end(); i++)
                {
                  CExtendedLap* pLap = (CExtendedLap*)*i;

                  for(int x = 0;x < DATA_CHANNEL_COUNT; x++)
                  {
                    const IDataChannel* pChannel = g_pLapDB->GetDataChannel(pLap->GetLap()->GetLapId(),(DATA_CHANNEL)x);
                    mapData[pLap->GetLap()] = pChannel;
                  }
                  lstLaps.push_back(pLap->GetLap());
                }
                // let's make sure there's a .csv suffix on that bugger.
				if(!str_ends_with(szFilename,L".csv"))
				{
					wcsncat(szFilename,L".csv", NUMCHARS(szFilename));
				}
				DashWare::SaveToDashware(szFilename, lstLaps);
              }
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
          case IDC_DISPLAYTYPE_RECEPTION:
          {
            switch(HIWORD(wParam))
            {
              case BN_CLICKED:
                m_eLapDisplayStyle = LAPDISPLAYSTYLE_RECEPTION;
                UpdateUI(UPDATE_MAP | UPDATE_DASHBOARD);
                break;
            }
            return TRUE;
          }
          case IDC_DISPLAYTYPE_PLOT:
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
            set<LPARAM> setSelected = m_sfLapList.GetSelectedItemsData();
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
            UpdateUI(UPDATE_MAP | UPDATE_DASHBOARD);
            return TRUE;
          }
          case IDC_SETDRIVER: // they want to set the driver of the selected laps
          {
            switch(HIWORD(wParam))
            {
            case BN_CLICKED:
              ApplyDriverNameToSelectedLaps(::g_pLapDB);
              UpdateUI(UPDATE_ALL); //the list causes everything to redraw
              break;
            }
            return TRUE;
          }
          case IDC_CLEARSELECTION:
          {
            switch(HIWORD(wParam))
            {
				case BN_CLICKED:
				  m_sfLapList.Clear();
				  UpdateUI(UPDATE_ALL);
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
                UpdateUI(UPDATE_LIST | UPDATE_DASHBOARD);
                break;
              }
            }
            return TRUE;
		  }
        } // end switch for finding out what control WM_COMMAND hit
        break; // break out of WM_COMMAND handling
      }
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
          if(m_iRaceId < 0 || pLapDB->GetLapCount(m_iRaceId) <= 0 || // if we don't have a race or our current race has no laps (aka sucks)
            (pLapDB->IsActivelyReceiving(iLastRaceId) && !pLapDB->IsActivelyReceiving(m_iRaceId))) // or if the new race ID is receiving and the current race ID isn't...
          {
            m_iRaceId = pLapDB->GetLastReceivedRaceId(); // since we just got told there's a new lap, there must be a last-received-race
            ClearUILaps();
            LoadLaps(g_pLapDB);
            UpdateUI(UPDATE_ALL);
          }
          else
          {
            LoadLaps((ILapReceiver*)lParam);
            UpdateUI(UPDATE_LIST | UPDATE_MAP | UPDATE_DASHBOARD);
          }
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
              CRaceSelectDlg dlgRace(g_pLapDB, &sfResult);
              ArtShowDialog<IDD_SELECTRACE>(&dlgRace);
              if(!sfResult.fCancelled)
              {
                m_iRaceId = sfResult.iRaceId;
                
                ClearUILaps();
                LoadLaps(g_pLapDB);
                UpdateUI(UPDATE_ALL);
              }
            }
          }
          return TRUE;
          
        }
        case NOTIFY_NEWNETSTATUS:
        {
          ILapReceiver* pLaps = (ILapReceiver*)lParam;
          SendMessage(m_hWnd, WM_SETTEXT, 0, (LPARAM)pLaps->GetNetStatus(NETSTATUS_STATUS));

          TCHAR szTemp[512];
          HWND hWndIp = GetDlgItem(m_hWnd, IDC_CURRENTIP);
          swprintf(szTemp, NUMCHARS(szTemp), L"PC: %s", pLaps->GetNetStatus(NETSTATUS_THISIP));
          SendMessage(hWndIp, WM_SETTEXT, 0, (LPARAM)szTemp);

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

  const static DWORD UPDATE_ALL = 0xffffffff;
  //	Pull in PlotPrefs array as well as lines vs. dots and Painting color scheme settings from Settings.txt file
  void SetDisplayOptions(const LAPSUPPLIEROPTIONS& lapOpts)
  {
    m_sfLapOpts = lapOpts;
  }

  void UpdateUI(DWORD fdwUpdateFlags)
  {
    m_fdwUpdateNeeded|= fdwUpdateFlags;
    PostMessage(m_hWnd,WM_UPDATEUI,0,0);
  }
  void UpdateUI_Internal(DWORD fdwUpdateFlags)
  {
    set<LPARAM> setSelectedData = m_sfLapList.GetSelectedItemsData();
    vector<CExtendedLap*> laps = GetSortedLaps(); // translates our m_mapLaps into a vector sorted by time

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

    if(IS_FLAG_SET(fdwUpdateFlags, UPDATE_LIST))
    {
      int iPosition = m_sfLapList.GetPosition();
      m_sfLapList.Clear();
      vector<CExtendedLap*> laps = GetSortedLaps(); // translates our m_mapLaps into a vector sorted by time
      for(int x = 0;x < laps.size(); x++)
      {
        vector<wstring> lstStrings;
        laps[x]->GetStrings(lstStrings);
        m_sfLapList.AddStrings(lstStrings, (LPARAM)laps[x]);
      }
      m_sfLapList.SetSelectedData(setSelectedData);
      if(laps.size() > 0)
      {
        m_sfLapList.MakeVisible((LPARAM)laps[laps.size()-1]);
      }
    }
    if(IS_FLAG_SET(fdwUpdateFlags, UPDATE_MAP))
    {
      UpdateDisplays();
    }
    if(IS_FLAG_SET(fdwUpdateFlags, UPDATE_MENU))
    {
      UpdateMenus();
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
   void ShowAbout()
	{
        MessageBox(NULL,L"Piside Console for Wifilapper\n\nVersion 2.003.0015\n\nThis is an Open Source project. If you want to contribute\n\nhttp://sites.google.com/site/wifilapper",
			L"About Pitside Console",MB_OK);
		return;
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
    GET_WINDOWPOS(IDC_SUBDISPLAY);
    GET_WINDOWPOS(IDC_LAPS);
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
    HandleCtlResize(sNewSize, IDC_SUBDISPLAY, true, false); // sub display window
    HandleCtlResize(sNewSize, IDC_LAPS, false, true); // lap list
  }
	float Average(DATA_CHANNEL eChannel, const IDataChannel* pChannel, float flVal)
	{
		//	This function currently only returns the current value where the cursor is pointed (flValue).
		//	It needs an iterator type loop to calculate the average value across all of the points in this data channel for this lap
		char szAvg[MAX_PATH];
		float sum = 0.0f;
		int count; 
	//	set<DATA_CHANNEL> channels = pChannel->GetData(); // get all the points that this lap / channel has
	//	for(set<DATA_CHANNEL>::const_iterator i = channels.begin(); i != channels.end(); i++) // loop through them, insert them into our "all data channels" set
		for (count = 0; count < sizeof pChannel->GetData(); count++)
		{
			GetChannelValue(eChannel,m_sfLapOpts.eUnitPreference,flVal,szAvg,NUMCHARS(szAvg));
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
void UpdateDisplays()
  {
	//	Update the data channels that are being displayed as values
	//	List of highlighted laps
	set<LPARAM> setSelectedData = m_sfLapList.GetSelectedItemsData();
    if(setSelectedData.size() > 0 && setSelectedData.size() < 5)
    {
      const int cLabels = 5;	//	The maximum number of Value Data channels to display
	  bool m_Warning = false;	//	Flag for showing dialog of Value display to indicate statistics are outside of bounds
	  int w=0;	//	String variable counter for Vaue display
      TCHAR szLabel[cLabels][MAX_PATH];
	  for (int z = 0; z < cLabels; z++)
	  {
		  wcscpy(szLabel[z],(TCHAR*) L"");	//	Initialize the strings for Data Value Channels
	  }
      //   Loop through the selected Y-axis data channels for this lap
	  for(int x = 0; x < this->m_lstYChannels.size() && x < 49; x++)
	  {
			const DATA_CHANNEL eChannel = m_lstYChannels[x];
			if(!eChannel /*|| !eChannel->IsValid()*/) continue;
			float flMin, flMax, flAvg;
			//	First check if this data channel is one to be displayed as a Value (false) or Graph (true) 
			for (int u = 0; u < 30; u++)	//	This can be improved, upper limit should be the total number of data channels, TotalYChannels plus all of the derived ones
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
						flAvg = Average(eChannel, pChannel, flVal);
						//	See if the Minimum or Maximum are outside of the PlotPrefs setpoints
						if (flMax > m_sfLapOpts.m_PlotPrefs[u].fMaxValue)
						{
							m_Warning = true;	//	An alarm has been triggered!
						}
						else if (flMin < m_sfLapOpts.m_PlotPrefs[u].fMinValue)
						{
							m_Warning = true;
						}
						else
						{
						}
					  }
					  else
					  {
						  flVal=0.0f;
						  flMin=0.0f;
						  flMax=0.0f;
						  flAvg=0.0f;
					  }
					}
					//	Now assign these values to the Data Value variable for display
					TCHAR szChannelName[MAX_PATH];
					GetDataChannelName(eChannel,szChannelName,NUMCHARS(szChannelName));

					char szMin[MAX_PATH];
					char szMax[MAX_PATH];
					GetChannelValue(eChannel,m_sfLapOpts.eUnitPreference,flMin,szMin,NUMCHARS(szMin));
					GetChannelValue(eChannel,m_sfLapOpts.eUnitPreference,flMax,szMax,NUMCHARS(szMax));
					//	Now assemble the string to display (max of 5)
					if (w < cLabels)
					{
						swprintf(szLabel[w],NUMCHARS(szLabel[w]),L"%s: Min: %S, Max: %S, Mean: %3.1f",szChannelName,szMin,szMax,flAvg);
						w++;	//	Increment Value string counter
					}
					break;
				}
				else
				{
				}
			}
	  }
	  //	Display the Data Value Channels
	  for (int z = 0; z < cLabels; z++)
	  {
			HWND hWndLabel = GetDlgItem(m_hWnd, IDC_VALUE_CHANNEL1 + z);
			SendMessage(hWndLabel, WM_SETTEXT, 0, (LPARAM)szLabel[z]);
	  }
		if (m_Warning)	//	Pop up dialog saying the alarm has been triggered
		{
			static bool fWarnedOnce = false;
			if(!fWarnedOnce)
			{
				fWarnedOnce = true;
				MessageBox(NULL,L"One or more of the alarm limits has been triggered\n\nMove the cursor over to the Lap List area and \nhit the 'Enter' key to remove this dialog",L"***WARNING***",MB_OK);
				fWarnedOnce = false;
			}
		}
    }
    m_sfLapPainter.Refresh();
	m_sfSubDisplay.Refresh();

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
    HMENU hSubMenu = GetSubMenu(hWndMenu, 2);

    CheckMenuHelper(hSubMenu, ID_OPTIONS_KMH, m_sfLapOpts.eUnitPreference == UNIT_PREFERENCE_KMH);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_MPH, m_sfLapOpts.eUnitPreference == UNIT_PREFERENCE_MPH);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_MS, m_sfLapOpts.eUnitPreference == UNIT_PREFERENCE_MS);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_SHOWBESTS, m_fShowBests);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_SHOWDRIVERBESTS, m_fShowDriverBests);
	CheckMenuHelper(hSubMenu, ID_OPTIONS_SHOWREFERENCELAP, m_fShowReferenceLap);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_DRAWLINES, m_sfLapOpts.fDrawLines);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_BACKGROUND, m_sfLapOpts.fColorScheme);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_IOIO5VSCALE, m_sfLapOpts.fIOIOHardcoded);
    CheckMenuHelper(hSubMenu, ID_OPTIONS_ELAPSEDTIME, m_sfLapOpts.fElapsedTime);
  }
  vector<CExtendedLap*> GetSortedLaps()
  {
    vector<CExtendedLap*> lstLaps;
    for(map<int,CExtendedLap*>::iterator i = m_mapLaps.begin(); i != m_mapLaps.end(); i++)
    {
      lstLaps.push_back(i->second);
    }
    sort(lstLaps.begin(),lstLaps.end(), CExtendedLap_SortByTime);
    return lstLaps;
  }
  void LoadLaps(ILapReceiver* pReceiver)
  {
    vector<const ILap*> laps = pReceiver->GetLaps(m_iRaceId);
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
  void ApplyDriverNameToSelectedLaps(ILapReceiver* pLapDB)
  {
    set<LPARAM> setSelectedData = m_sfLapList.GetSelectedItemsData();
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
    default:
      DASSERT(FALSE);
      return false;
    }
  }
  
  virtual vector<CExtendedLap*> GetAllLaps() const override
  {
    set<LPARAM> setSelectedLaps = m_sfLapList.GetSelectedItemsData();
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
    set<LPARAM> setSelectedLaps = m_sfLapList.GetSelectedItemsData();
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
//	if(m_pReferenceLap != NULL)
	if(m_fShowReferenceLap && m_pReferenceLap != NULL)
    {
		lstLaps.push_back(m_pReferenceLap);
    }


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
    default:
      DASSERT(FALSE);
      break;
    }
    return m_eLapDisplayStyle;
  }
  virtual float GetDataHardcodedMin(DATA_CHANNEL eChannel) const override
  {
    if(eChannel >= DATA_CHANNEL_IOIOPIN_START && eChannel < DATA_CHANNEL_IOIOPIN_END ||
       eChannel >= DATA_CHANNEL_IOIOCUSTOM_START && eChannel < DATA_CHANNEL_IOIOCUSTOM_END)
    {
      return m_sfLapOpts.fIOIOHardcoded ? 0 : 1e30;
    }
    return 1e30;
  }
  virtual float GetDataHardcodedMax(DATA_CHANNEL eChannel) const override
  {
    if(eChannel >= DATA_CHANNEL_IOIOPIN_START && eChannel < DATA_CHANNEL_IOIOPIN_END ||
       eChannel >= DATA_CHANNEL_IOIOCUSTOM_START && eChannel < DATA_CHANNEL_IOIOCUSTOM_END)
    {
      return m_sfLapOpts.fIOIOHardcoded ? 5 : -1e30;
    }
    return -1e30;
  }

  virtual float GetGuideStartX(DATA_CHANNEL eChannel, float flMin, float flMax) override
  {
    CASSERT(DATA_CHANNEL_COUNT == 0x401);

    switch(eChannel)
    {
      case DATA_CHANNEL_X: return 1e30;
      case DATA_CHANNEL_Y: return 1e30; // we don't want guides for either latitude or longitude
	  case DATA_CHANNEL_VELOCITY:
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
            eChannel >= DATA_CHANNEL_IOIOCUSTOM_START && eChannel < DATA_CHANNEL_IOIOCUSTOM_END)
        {
          return m_sfLapOpts.fIOIOHardcoded ? 0 : 1e30;
        }
        return 1e30;
    }
  }

  virtual float GetGuideStart(DATA_CHANNEL eChannel, float flMin, float flMax) override
  {
    CASSERT(DATA_CHANNEL_COUNT == 0x401);

    switch(eChannel)
    {
      case DATA_CHANNEL_X: return 1e30;
      case DATA_CHANNEL_Y: return 1e30; // we don't want guides for either latitude or longitude
      case DATA_CHANNEL_VELOCITY: return 0;
      case DATA_CHANNEL_DISTANCE: return 1e30;
      case DATA_CHANNEL_TIME: return 1e30;
      case DATA_CHANNEL_ELAPSEDTIME: return 1e30;
      case DATA_CHANNEL_TIMESLIP:
      {
        int iMin = (int)(flMin/1000.0f);
        return (float)(iMin)*1000.0f;
      }
	  case DATA_CHANNEL_LAPTIME_SUMMARY:
      {
        int iMin = (int)(flMin);
        return (float)(iMin);
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
            eChannel >= DATA_CHANNEL_IOIOCUSTOM_START && eChannel < DATA_CHANNEL_IOIOCUSTOM_END)
        {
          return m_sfLapOpts.fIOIOHardcoded ? 0 : 1e30;
        }
        return 1e30;
    }
  }

  virtual float GetGuideStepX(DATA_CHANNEL eChannel, float flMin, float flMax) override
  {
  // Function sets up the spacing for the vertical guidelines on the data plots
	  CASSERT(DATA_CHANNEL_COUNT == 0x401);
    const float flSpread = flMax - flMin;
    switch(eChannel)
    {
    case DATA_CHANNEL_X: return 1e30;
    case DATA_CHANNEL_Y: return 1e30; // we don't want guides for either latitude or longitude
    case DATA_CHANNEL_VELOCITY:	// We need to fix the X-channel call before putting these back into the code.
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
		  if(flSpread < 1000) return 50.0f;		//	Added by KDJ to improve TS display
		  if(flSpread < 5000) return 100.0f;
		  if(flSpread < 10000) return 500.0f;
		  if(flSpread < 50000) return 2500.0f;
		  if(flSpread < 110000) return 5000.0f;
		  if(flSpread < 1100000) return 10000.0f;
		  if(flSpread < 10000000) return 100000.0f;
		  return 10000000;
		} 

    case DATA_CHANNEL_TIME:					
    case DATA_CHANNEL_ELAPSEDTIME:					
		{
		  if(flSpread < 1000) return 50.0f;		//	Added by KDJ to improve TS display
		  if(flSpread < 5000) return 100.0f;
		  if(flSpread < 10000) return 500.0f;
		  if(flSpread < 50000) return 2500.0f;
		  if(flSpread < 110000) return 5000.0f;
		  if(flSpread < 1100000) return 10000.0f;
		  if(flSpread < 10000000) return 100000.0f;
		  return 10000000;
		}
    case DATA_CHANNEL_LAPTIME_SUMMARY:					
		{
		  if(flSpread < 1) return 0.50f;		//	Added by KDJ to improve TS display
		  if(flSpread < 5) return 1.0f;
		  if(flSpread < 10) return 5.0f;
		  if(flSpread < 50) return 25.0f;
		  if(flSpread < 110) return 50.0f;
		  if(flSpread < 1100) return 100.0f;
		  if(flSpread < 10000) return 1000.0f;
		  return 100000;
		}
	default:
    return 1e30;
	}
  }

  virtual float GetGuideStep(DATA_CHANNEL eChannel, float flMin, float flMax) override
  {
    CASSERT(DATA_CHANNEL_COUNT == 0x401);
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
    case DATA_CHANNEL_DISTANCE: 
    {
	  if(flSpread < 0.001) return 0.0001f;		
	  if(flSpread < 0.005) return 0.0005f;		
	  if(flSpread < 0.010) return 0.0010f;		
      if(flSpread < 0.050) return 0.0050f;		
      if(flSpread < 1.000) return 0.1000f;
      if(flSpread < 10.00) return 1.0000f;
	  if(flSpread < 1000) return 100.0f;		//	Added by KDJ to improve TS display
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
	  if(flSpread < 10) return 1.0f;		//	Added by KDJ to improve TS display
	  if(flSpread < 100) return 10.0f;		//	Added by KDJ to improve TS display
	  if(flSpread < 1000) return 100.0f;		//	Added by KDJ to improve TS display
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
	  return 10000.0f;
    }
    case DATA_CHANNEL_X_ACCEL: return 0.5f;
    case DATA_CHANNEL_Y_ACCEL: return 0.5f;
    case DATA_CHANNEL_Z_ACCEL: return 0.5f;
    case DATA_CHANNEL_TEMP: return 10.0f;
    case (DATA_CHANNEL_PID_START+0x5): return 25;
    case (DATA_CHANNEL_PID_START+0xA): return 100;
    case (DATA_CHANNEL_PID_START+0xc): return 1000; // rpms
    case (DATA_CHANNEL_PID_START+0x5c): return 25;
    default: 
      if(eChannel >= DATA_CHANNEL_IOIOPIN_START && eChannel < DATA_CHANNEL_IOIOPIN_END ||
          eChannel >= DATA_CHANNEL_IOIOCUSTOM_START && eChannel < DATA_CHANNEL_IOIOCUSTOM_END)
      {
        if(flSpread < 1) return m_sfLapOpts.fIOIOHardcoded ? 0.1f : 1e30;	//	Added by KDJ
		if(flSpread < 10) return m_sfLapOpts.fIOIOHardcoded ? 1.0f : 1e30;	//	Added by KDJ
		if(flSpread < 25) return m_sfLapOpts.fIOIOHardcoded ? 2.5f : 1e30;	//	Added by KDJ
		if(flSpread < 50) return m_sfLapOpts.fIOIOHardcoded ? 5.0f : 1e30;	//	Added by KDJ
		if(flSpread < 150) return m_sfLapOpts.fIOIOHardcoded ? 20.0f : 1e30;	//	Added by KDJ
		if(flSpread < 500) return m_sfLapOpts.fIOIOHardcoded ? 50.0f : 1e30;	//	Added by KDJ
		if(flSpread < 10000) return m_sfLapOpts.fIOIOHardcoded ? 1000.0f : 1e30;	//	Added by KDJ
		if(flSpread < 100000) return m_sfLapOpts.fIOIOHardcoded ? 5000.0f : 1e30;	//	Added by KDJ
		if(flSpread < 1000000) return m_sfLapOpts.fIOIOHardcoded ? 50000.0f : 1e30;	//	Added by KDJ
        return m_sfLapOpts.fIOIOHardcoded ? 1.0f : 1e30;		// Original code, and default for non-transformed IOIO data
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
  ArtListBox m_sfYAxis;
  ArtListBox m_sfLapList;
private:
  ArtListBox m_sfXAxis;

  CLapPainter m_sfLapPainter;
  CLapPainter m_sfSubDisplay;

  // lap display style data
  map<const CExtendedLap*,int> m_mapLapHighlightTimes; // stores the highlight times (in milliseconds since phone app start) for each lap.  Set from ILapSupplier calls

  LAPDISPLAYSTYLE m_eLapDisplayStyle;
  DATA_CHANNEL m_eXChannel;
//  vector<DATA_CHANNEL> m_lstYChannels;
  bool m_fShowBests;
  bool m_fShowDriverBests;
  bool m_fShowReferenceLap;

  CExtendedLap* m_pReferenceLap;
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

  int m_iRaceId;
  ILapSupplier* z_ILapSupplier;
};

DWORD ReceiveThreadProc(LPVOID param)
{
  ILapReceiver* pLaps = (ILapReceiver*)param;
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
	}
  }

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)

{
//  Show Splash screen as first screen
  CSplashDlg splash;
  ArtShowDialog<IDD_DLGSPLASH>(&splash);

  if(strcmp(lpCmdLine,"unit") == 0)
  {
    return UnitTests();
  }
  INITCOMMONCONTROLSEX initCtrls;
  initCtrls.dwICC = ICC_LISTVIEW_CLASSES;
  initCtrls.dwSize = sizeof(initCtrls);

  InitCommonControlsEx(&initCtrls);

  CMainUI sfUI;
  g_pUI = &sfUI;
  //CLapReceiver sfLaps(&sfUI);

  
  CSQLiteLapDB sfLaps(&sfUI);
  bool fDBOpened = false;

  int iRaceId = 0;
  TCHAR szDBPath[MAX_PATH];
  if(ArtGetSaveFileName(NULL,L"Select .wflp to open or save to",szDBPath,NUMCHARS(szDBPath),L"WifiLapper Files (*.wflp)\0*.WFLP\0\0"))
  {
    const bool fFileIsNew = !DoesFileExist(szDBPath);
    if(fFileIsNew)
    {
      // let's make sure there's a .wflp suffix on that bugger.
      if(!str_ends_with(szDBPath,L".wflp"))
      {
        wcsncat(szDBPath,L".wflp", NUMCHARS(szDBPath));
      }
    }
    // they chose one to open, so open it.
    if(sfLaps.Init(szDBPath))
    {
      if(!fFileIsNew)
      {
        // show the race-selection dialog
        RACESELECT_RESULT sfRaceResult;
        CRaceSelectDlg sfRaceSelect(&sfLaps,&sfRaceResult);
        ::ArtShowDialog<IDD_SELECTRACE>(&sfRaceSelect);
        if(!sfRaceResult.fCancelled)
        {
          iRaceId = sfRaceResult.iRaceId;
          fDBOpened = true;
        }
        else
        {
          iRaceId = -1;
          fDBOpened = true;
        }
      }
      else
      {
        iRaceId = -1;
        fDBOpened = true;
      }
    }
    
  }
  else
  {
    return 0;
  }
  
  if(!fDBOpened)
  {
    // they didn't choose a file, so just use a temp DB.
    TCHAR szTempPath[MAX_PATH];
    GetTempPath(NUMCHARS(szTempPath),szTempPath);
    wcscat(szTempPath,L"\\pitsidetemp.wflp");
    if(sfLaps.Init(szTempPath))
    {
      // success!
    }
  }
  if(!fDBOpened)
  {
    // disaster.
    MessageBox(NULL,L"Pitside was unable to create a database to save data to.  Is your hard drive full?",L"Failed to create DB",MB_ICONERROR);
    exit(0);
  }
  sfUI.SetRaceId(iRaceId);


  g_pLapDB = &sfLaps;

  
  LAPSUPPLIEROPTIONS x_sfLapOpts; //sfLapOpts contains all lap display options
  InitPlotPrefs(x_sfLapOpts);	//	Initialize all PlotPrefs variables before displaying anything
//  sfUI.SetDisplayOptions(x_sfLapOpts);

  PITSIDE_SETTINGS sfSettings;
  LoadPitsideSettings(&sfSettings);		//	Load preferences from "Settings.txt" file

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
  sfUI.SetDisplayOptions(x_sfLapOpts);

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
        TCHAR szMsg[200];
        _snwprintf(szMsg,NUMCHARS(szMsg),L"Pitside was unable to start the HTTP server on port %d.  Do you want to open settings.txt to try another port or disable the server?",sfSettings.iHTTPPort);
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

  HANDLE hRecvThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&ReceiveThreadProc, (LPVOID)&sfLaps, 0, NULL);

  ArtShowDialog<IDD_DLGFIRST>(&sfUI);
  exit(0);
}

