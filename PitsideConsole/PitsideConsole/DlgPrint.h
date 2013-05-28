#pragma once
#include "resource.h"

#include "artui.h"
#include "ArtTools.h"

struct PRINT_RESULT
{
public:
  PRINT_RESULT()
  {
    fCancelled = false;
  }
  bool fCancelled;
};

struct PRINTER_RESULT
{
public:
  PRINTER_RESULT()
  {
    fCancelled = false;
  }
  bool fCancelled;
};

class CPrintDlg : public IUI
{
public:
  CPrintDlg(PRINT_RESULT* pResults) : m_sfResult(pResults) {};
  virtual ~CPrintDlg() {};

  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_PRINT;}
public:
  PRINT_RESULT* m_sfResult;
private:
};

class m_CPrintToPrinter : public IUI
{
public:
	m_CPrintToPrinter(PRINTER_RESULT* p_pResults) : p_sfResult(p_pResults) {};
	virtual ~m_CPrintToPrinter() {};
	virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
public: 
	PRINTER_RESULT* p_sfResult;
};

////////////////////////////////////////////////////////////////////////////////
	HDC GetPrinterDC (HWND Hwnd);
	BOOL OpenFileDialog(HWND hwnd, LPTSTR pFileName ,LPTSTR pTitleName);
	BOOL SaveFileDialog(HWND hwnd, LPTSTR pFileName ,LPTSTR pTitleName);
	void InitializeDialog(HWND hwnd);
	PBITMAPINFO CreateBitmapInfoStruct(HWND hwnd, HBITMAP hBmp);
	void SaveBMPFile(HWND hwnd, LPTSTR pszFile, PBITMAPINFO pbi, HBITMAP hBMP, HDC hDC);
	BITMAP ConvertToGrayscale(BITMAP source, HDC hDC);
////////////////////////////////////////////////////////////////////////////////

