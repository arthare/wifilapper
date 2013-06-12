#pragma once
#include <windows.h>
#include "ArtTools.h"
#include <set>
#include <vector>
#include <string>
#include "gl/gl.h"
#include <map>
#include "commctrl.h"

void DrawGLFilledSquare(double dX, double dY, double dRadius);

GLuint LoadTextureSDL( const char * filename);

HRESULT InitGLFont(GLuint* pBase, int iFontSize, HDC hdc);
void KillFont(GLuint base);


using namespace std;
class ArtListBox
{
public:
  ArtListBox()
  {
  }
  void Init(HWND hWnd, const vector<wstring>& lstColumnHeaders, const vector<int>& lstColWidths)
  {
    DASSERT(lstColumnHeaders.size() == lstColWidths.size());

    m_hWnd = hWnd;

    m_cColumns = lstColumnHeaders.size();
    if(lstColumnHeaders.size() <= 0)	//	No data to display
    {
      RECT rc;
      GetClientRect(m_hWnd,&rc);

      LVCOLUMN LvCol = {0};
      LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
      LvCol.pszText=L"";
      LvCol.cx=RECT_WIDTH(&rc) - 30;

      SendMessage(m_hWnd,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol); // Insert/Show the coloum
    }
    else								//	Data to display
    {
      LVCOLUMN LvCol = {0};
      LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
      
      for(unsigned int x = 0; x < lstColumnHeaders.size(); x++)
      {
        TCHAR szTemp[MAX_PATH];
        wcsncpy_s(szTemp,lstColumnHeaders[x].c_str(),NUMCHARS(szTemp));
        LvCol.cx=lstColWidths[x];
        LvCol.pszText=szTemp;
        SendMessage(m_hWnd,LVM_INSERTCOLUMN,x,(LPARAM)&LvCol); // Insert/Show the coloum
      }
    }
    ListView_SetExtendedListViewStyleEx(m_hWnd, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
  }
  void AddString(LPCTSTR lpszString, int iData)
  {
    TCHAR szString[MAX_PATH];
    wcsncpy_s(szString,lpszString,NUMCHARS(szString));

    LVITEM lvItem = {0};
    lvItem.mask = LVIF_PARAM | LVIF_TEXT;
    lvItem.lParam = iData;
    lvItem.pszText = szString;
    lvItem.iItem = ListView_GetItemCount(m_hWnd);
    lvItem.state = 0;
    lvItem.stateMask = -1;
    lvItem.cchTextMax = wcslen(szString);

    VERIFY(ListView_InsertItem(m_hWnd,&lvItem) >= 0);
  }
  void AddStrings(vector<wstring> lstCols, int iData)
  {
    DASSERT(lstCols.size() == m_cColumns);

    const int iItem = ListView_GetItemCount(m_hWnd);
    
    TCHAR szString[MAX_PATH];
    wcsncpy_s(szString,lstCols[0].c_str(),NUMCHARS(szString));

    {
      LVITEM lvItem = {0};
      lvItem.mask = LVIF_PARAM | LVIF_TEXT;
      lvItem.lParam = iData;
      lvItem.pszText = szString;
      lvItem.iItem = iItem;
      lvItem.iSubItem = 0;
      lvItem.state = 0;
      lvItem.stateMask = -1;
      lvItem.cchTextMax = wcslen(szString);
      VERIFY(ListView_InsertItem(m_hWnd,&lvItem) >= 0);
    }

    for(unsigned int x = 0;x < lstCols.size(); x++)
    {
      TCHAR szString[MAX_PATH];
      wcsncpy_s(szString,lstCols[x].c_str(),NUMCHARS(szString));

      LVITEM lvItem = {0};
      lvItem.mask = LVIF_TEXT;
      lvItem.pszText = szString;
      lvItem.iItem = iItem;
      lvItem.iSubItem = x;
      lvItem.state = 0;
      lvItem.stateMask = -1;
      lvItem.cchTextMax = wcslen(szString);

      VERIFY(ListView_SetItem(m_hWnd,&lvItem));
    }
  }
  void Clear()
  {
    ListView_DeleteAllItems(m_hWnd);
  }
  int GetPosition()
  {
    return ListView_GetTopIndex(m_hWnd);
  }
  void MakeVisible(LPARAM iData)
  {
    // makes sure that the item containing the LPARAM lParam is visible
    LVFINDINFO sfFind = {0};
    sfFind.flags = LVFI_PARAM;
    sfFind.lParam = iData;
    int ixIndex = ListView_FindItem(m_hWnd,-1,&sfFind);
    if(ixIndex >= 0)
    {
      ListView_EnsureVisible(m_hWnd,ixIndex,FALSE);
    }

  }
  vector<int> GetSelectedIndices()
  {
    vector<int> ret;
    int ixSelect = -1;
    do
    {
      ixSelect = SendMessage(m_hWnd, LVM_GETNEXTITEM, ixSelect, LVNI_SELECTED | LVNI_BELOW);
      if(ixSelect >= 0) ret.push_back(ixSelect);
    } while(ixSelect >= 0);
    return ret;
  }
  void SetSelectedData(const set<LPARAM>& setData)
  { 
    int ixItem = -1;
    do
    {
      ixItem = SendMessage(m_hWnd, LVM_GETNEXTITEM, ixItem, LVNI_ALL);
      if(ixItem >= 0)
      {
        LVITEM sfItem = {0};
        sfItem.iItem = ixItem;
        sfItem.iSubItem = 0;
        sfItem.stateMask = LVIS_SELECTED;
        sfItem.mask = LVIF_STATE | LVIF_PARAM;

        if(ListView_GetItem(m_hWnd,&sfItem))
        {
          DASSERT(sfItem.lParam);
          if(setData.find(sfItem.lParam) != end(setData))
          {
            // found the item in the selection set, so make it selected
            sfItem.state |= LVIS_SELECTED;
            sfItem.stateMask |= LVIS_SELECTED;
            ListView_SetItem(m_hWnd,&sfItem);
          }
          else
          {
            // this item is not supposed to be selected
            sfItem.state &= ~LVIS_SELECTED;
            sfItem.stateMask &= ~LVIS_SELECTED;
            ListView_SetItem(m_hWnd, &sfItem);
          }
        }
        else
        {
          break;
        }
      }
    } while(ixItem >= 0);
  }
  set<LPARAM> GetSelectedItemsData() const
  {
    set<LPARAM> ret;
    int ixSelect = -1;
    do
    {
      ixSelect = SendMessage(m_hWnd, LVM_GETNEXTITEM, ixSelect, LVNI_SELECTED);
      if(ixSelect >= 0) 
      {
        LVITEM sfItem = {0};
        sfItem.iItem = ixSelect;
        sfItem.iSubItem = 0;
        sfItem.stateMask = LVIS_SELECTED;
        sfItem.mask = LVIF_STATE | LVIF_PARAM;

        if(ListView_GetItem(m_hWnd,&sfItem))
        {
          // found an item!
          DASSERT(sfItem.lParam != NULL);
          ret.insert(sfItem.lParam);
        }
		else {DASSERT(FALSE); break;}		// Added by KDJ to prevent locks when no data is passed
      }
    } while(ixSelect >= 0);

    return ret;
  }
private:
  HWND m_hWnd;
  int m_cColumns; // used for debugging
};


#include "gl/gl.h"
#include "gl/glu.h"

class ArtOpenGLWindow
{
public:
  ArtOpenGLWindow() : m_hdc(NULL), m_hRC(NULL), m_hWnd(NULL), m_iFontSize(12)
  {
  }
  virtual ~ArtOpenGLWindow()
  {
    DeInit();
  }
  void Init(HWND hWnd)
  {
    m_hWnd = hWnd;
    m_hdc = GetDC(hWnd);

    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory( &pfd, sizeof( pfd ) );
    pfd.nSize = sizeof( pfd );
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
                  PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int iFormat = ChoosePixelFormat( m_hdc, &pfd );
    SetPixelFormat( m_hdc, iFormat, &pfd );
    m_hRC = wglCreateContext(m_hdc);
    
    HDC hOldDC = wglGetCurrentDC();
    HGLRC hOldRC = wglGetCurrentContext();
    wglMakeCurrent(m_hdc, m_hRC);

    VERIFY(SUCCEEDED(InitGLFont(&m_uFontBase, m_iFontSize, m_hdc)));

    wglMakeCurrent(hOldDC,hOldRC);
  }
  void DeInit()
  {
    //DeInitGLText();
    if(m_hRC != NULL && m_hdc != NULL)
    {
      wglDeleteContext(m_hRC);
      ReleaseDC(m_hWnd, m_hdc);
    }
  }
  HDC OGL_GetDC() {return m_hdc;}
  HGLRC OGL_GetRC() {return m_hRC;}
  HWND OGL_GetHWnd() const {return m_hWnd;}
  BOOL HandleMessage(HWND hWnd, UINT uMsg, LPARAM lParam, WPARAM wParam)
  {
    switch(uMsg)
    {
    case WM_MOUSEMOVE:
    {
      POINT ptLastMouse = m_ptMouse;
      RECT rcParent,rcThis;
      GetWindowRect(GetParent(m_hWnd),&rcParent);
      GetWindowRect(m_hWnd,&rcThis);
      m_ptMouse.x = LOWORD(wParam);
      m_ptMouse.y = HIWORD(wParam);

      m_ptMouse.x -= (rcThis.left - rcParent.left);
      m_ptMouse.y -= (rcThis.top - rcParent.top);

      RECT rcClient;
      GetClientRect(m_hWnd, &rcClient);

      RECT rcParentClient;
      GetClientRect(GetParent(m_hWnd), &rcParentClient);
      // the difference between rcParent and rcParentClient's height will be the height of the title bar
      m_ptMouse.y += (RECT_HEIGHT(&rcParent) - RECT_HEIGHT(&rcParentClient));
      m_ptMouse.y = RECT_HEIGHT(&rcClient) - m_ptMouse.y;
      bool fLastMouseValid = m_fMouseValid;
      m_fMouseValid = m_ptMouse.x >= 0 && m_ptMouse.y >= 0 && m_ptMouse.x < RECT_WIDTH(&rcClient) && m_ptMouse.y < RECT_HEIGHT(&rcClient);
      return FALSE;
    }
    case WM_MOUSELEAVE:
      m_fMouseValid = false;
      return TRUE;
    }
    return FALSE;
  }
  virtual void OGL_Paint() = 0;
  
  void Refresh()
  {
    HDC hOldDC = wglGetCurrentDC();
    HGLRC hOldRC = wglGetCurrentContext();
    wglMakeCurrent(m_hdc, m_hRC);
    OGL_Paint();
    wglMakeCurrent(hOldDC,hOldRC);
  }
protected:
  int GetWindowFontSize() const {return m_iFontSize*4/3;}
  void DrawText(float flX, float flY, const char* text) const;
public:
  bool GetMouse(POINT* ppt)
  {
    if(m_fMouseValid)
    {
      *ppt = m_ptMouse;
      return true;
    }
    return false;
  }
private:
  HDC m_hdc;
  HWND m_hWnd;
  HGLRC m_hRC;

  // semi-separate stuff about mouse position
  POINT m_ptMouse;
  bool m_fMouseValid;

  GLuint m_uFontBase;
  const int m_iFontSize;
};


class IUI
{
public:
  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) = 0; // note: can be called on any thread
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
  virtual DWORD GetDlgId() const = 0;
};

// for n, I recommend using your dialog's resource ID
extern map<int,IUI*> mapDialogs;
template<int n>
INT_PTR CALLBACK MessageDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  return mapDialogs[n]->DlgProc(hWnd,uMsg,wParam,lParam);
}
template<int n>
void ArtShowDialog(IUI* pDlg)
{
  mapDialogs[n] = pDlg;

  DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(pDlg->GetDlgId()), NULL, ::MessageDlgProc<n>);
}

// shows the open file dialog, sticks result in szPath...
// true -> path is good
// false -> was cancelled
bool ArtGetOpenFileName(HWND hWndOwner, LPCTSTR lpszTitle, LPTSTR lpszPath, int cchPath, LPCTSTR lpszFilter);
bool ArtGetSaveFileName(HWND hWndOwner, LPCTSTR lpszTitle, LPTSTR lpszPath, int cchPath, LPCTSTR lpszFilter);