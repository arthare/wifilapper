#include "Stdafx.h"
#include "DlgRaceSelect.h"
#include "resource.h"
#include "pitsideconsole.h"
#include "ArtSQL/ArtSQLite.h"
LRESULT CRaceSelectDlg::DlgProc
(
  HWND hWnd, 
  UINT uMsg, 
  WPARAM wParam, 
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
    {
      vector<wstring> lstCols;
      vector<int> lstWidths;
      lstCols.push_back(L"Date");
      lstCols.push_back(L"Race Name");
      lstCols.push_back(L"Laps");
      lstWidths.push_back(75);
      lstWidths.push_back(160);
      lstWidths.push_back(40);
      sfListBox.Init(GetDlgItem(hWnd,IDC_RACE),lstCols,lstWidths);

      // gotta set up the list
      CSfArtSQLiteDB sfDB;
      HRESULT hr = sfDB.Open(m_szSQL);
      if(SUCCEEDED(hr))
      {
        CSfArtSQLiteQuery sfQuery(sfDB);
        if(sfQuery.Init(L"select races._id,races.name,count(laps._id),unixtime from races left join laps on races._id = laps.raceid group by races._id order by races._id"))
        {
          while(sfQuery.Next())
          {
            int id = 0;
            TCHAR szName[MAX_PATH];
            int cCount = 0;
            int tmUnix = 0;
            if(sfQuery.GetCol(0,&id) && sfQuery.GetCol(1,szName,NUMCHARS(szName)) && sfQuery.GetCol(2,&cCount) && sfQuery.GetCol(3,&tmUnix))
            {
              vector<wstring> lstCols;

              SYSTEMTIME stStart = SecondsSince1970ToSYSTEMTIME(tmUnix);
              TCHAR szDate[100];
              _snwprintf(szDate,NUMCHARS(szDate),L"%d/%d/%4d",stStart.wMonth,stStart.wDay,stStart.wYear);
              lstCols.push_back(szDate);
              
              lstCols.push_back(szName);

              TCHAR szTemp[MAX_PATH];
              _snwprintf(szTemp, NUMCHARS(szTemp), L"%d",cCount);
              lstCols.push_back(szTemp);
              sfListBox.AddStrings(lstCols,id);
            }
          }
        }
      }
      break;
    }
    case WM_COMMAND:
    {
      switch(LOWORD(wParam))
      {
        case IDOK:
        {
          set<LPARAM> set = sfListBox.GetSelectedItemsData();
          if(set.size() == 1)
          {
            m_pResults->iRaceId = *set.begin();
            m_pResults->fCancelled = false;
            EndDialog(hWnd,0);
          }
          else
          {

          }
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

