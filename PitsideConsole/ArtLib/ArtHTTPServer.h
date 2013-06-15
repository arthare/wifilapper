#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include "ArtNet.h"
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#include <Windows.h>

using namespace std;

struct HTTPREQUEST
{
  string strType;
  string strPage;
  string strResponseType;
  map<string,string> mapParams;
};

// interface: if you want to handle HTTP requests, then implement this and pass yourself to a SimpleHTTPServer
class ArtHTTPResponder
{
public:
  virtual bool MakePage(HTTPREQUEST& strURL, ostream& out) = 0;
};

bool ParseRequest(const char* pRequest, int cbRequest, HTTPREQUEST* pParsed);
class SimpleHTTPServer
{
public:
  SimpleHTTPServer();
  virtual ~SimpleHTTPServer();

  bool Init(int iPort, ArtHTTPResponder* pResponder);

  void ThreadProc();

private:
  HANDLE m_hThread;
  int m_iPort;
  DWORD m_dwId;
  ArtHTTPResponder* m_pResponder;

  HANDLE m_hInitEvent;
  bool m_fInitSuccess;
};
