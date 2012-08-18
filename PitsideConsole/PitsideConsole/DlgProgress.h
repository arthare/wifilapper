#pragma once

#include "ArtUI.h"
#include "ArtTools.h"
#include "resource.h"

interface IProgress
{
public:
  virtual void SetProgress(LONG iFinished) = 0;
  virtual void SetTotal(LONG iTotal) = 0;
  virtual void SetDone() = 0;
};

const static DWORD WM_NOTIFY_PROGRESS = WM_USER + 2;
const static DWORD WM_NOTIFY_TOTAL = WM_USER + 3;
const static DWORD WM_NOTIFY_DONE = WM_USER + 4;

class CProgressDlg : public IUI, public IProgress
{
public:
  CProgressDlg() : m_cProgress(0),m_cTotal(0),m_hWnd(NULL) {};
  virtual ~CProgressDlg() {};

  virtual void NotifyChange(WPARAM wParam, LPARAM lParam) {DASSERT(FALSE);};
  virtual LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual DWORD GetDlgId() const {return IDD_PROGRESS;}

  virtual void SetProgress(LONG iFinished) override {InterlockedExchange(&m_cProgress, iFinished); PostMessage(m_hWnd, WM_NOTIFY_PROGRESS, 0, 0); }
  virtual void SetTotal(LONG iTotal) override {InterlockedExchange(&m_cTotal,iTotal); PostMessage(m_hWnd, WM_NOTIFY_TOTAL, 0, 0); }
  virtual void SetDone() override {PostMessage(m_hWnd, WM_NOTIFY_DONE, 0, 0);}
private:
  void InitProgress();
  void UpdateProgress();
private:
  HWND m_hWndProgress;
  LONG m_cProgress;
  LONG m_cTotal;
  HWND m_hWnd;
};