#include "stdafx.h"
#include "ArtHTTPServer.h"
#include "ArtTools.h"

bool ParseRequest(const char* pRequest, int cbRequest, HTTPREQUEST* pParsed)
{
  char szURL[200];
  char szRequestType[200];
  szURL[0] = szRequestType[0] = 0;

  int ixStart = 0;
  int ixPos = ixStart;
  while(ixPos < cbRequest && ixPos < sizeof(szRequestType))
  {
    if(pRequest[ixPos] == ' ') // found the end of the request type
    {
      strncpy(szRequestType,&pRequest[ixStart],ixPos-ixStart);
      szRequestType[ixPos-ixStart] = '\0';
      break;
    }

    ixPos++;
  }

  // we have the request type, let's get the URL
  ixStart = ixPos+1;
  ixPos = ixStart;
  while(ixPos < cbRequest && (ixPos-ixStart) < sizeof(szURL))
  {
    if(pRequest[ixPos] == ' ' || pRequest[ixPos] == '?')
    {
      strncpy(szURL,&pRequest[ixStart],ixPos-ixStart);
      szURL[ixPos-ixStart] = 0;
      break;
    }
    ixPos++;
  }

  if(pRequest[ixPos] == '?')
  {
    // this thing has a parameter list!
    const char* pszEnd = strstr(&pRequest[ixPos]," ");
    if(pszEnd)
    {
      ixPos++; 
      const char* pszStart = &pRequest[ixPos];
      while(pszStart < pszEnd)
      {
        const char* pszEquals = strstr(pszStart,"=");
        if(pszEquals && pszEquals < pszEnd)
        {
          const char* pszAmper = strstr(pszStart,"&");
          if(!pszAmper || pszAmper > pszEnd) pszAmper = pszEnd;
          if(pszAmper && pszAmper <= pszEnd)
          {
            // now we've got the key and the value positions
            const int cchMaxValue = 10000;
            char szKey[cchMaxValue];
            char szValue[cchMaxValue];
            if(pszAmper - pszEquals >= cchMaxValue || pszEquals - pszStart > cchMaxValue)
            {
              return false; // key or value is too big!
            }
            else
            {
              const int cchKey = pszEquals-pszStart;
              const int cchValue = pszAmper-pszEquals-1;
              strncpy(szKey,pszStart,cchKey);
              strncpy(szValue,pszEquals+1,cchValue);
              szKey[cchKey] = 0;
              szValue[cchValue] = 0;
              pParsed->mapParams[szKey] = szValue;
              
              pszStart = pszAmper+1;
            }
          }
          else
          {
            // couldn't find an ampersand, or its past the end
            break;
          }
        }
        else
        {
          // couldn't find an equals, or its past the end
          break;
        }
      }
    }
    else
    {
      return false;
    }
  }

  pParsed->strPage = string(szURL);
  pParsed->strType = string(szRequestType);
  return szURL[0] && szRequestType[0];
}

DWORD ThreadProcStub(LPVOID pvParam);

int c = 0;

SimpleHTTPServer::SimpleHTTPServer() : m_iPort(0), m_pResponder(NULL),m_hInitEvent(NULL),m_fInitSuccess(false)
{
}
SimpleHTTPServer::~SimpleHTTPServer() 
{
  WaitForSingleObject(m_hThread,INFINITE);
};

bool SimpleHTTPServer::Init(int iPort, ArtHTTPResponder* pResponder)
{
  m_iPort = iPort;
  m_pResponder = pResponder;

  m_fInitSuccess = false;
  WaitForSingleObject(m_hThread,INFINITE);

  if(m_hInitEvent != NULL)
  {
    CloseHandle(m_hInitEvent);
    m_hInitEvent = NULL;
  }
  m_hInitEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

  m_hThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ThreadProcStub,(LPVOID)this,0,&m_dwId);
  DWORD dwRet = WaitForSingleObject(m_hInitEvent,INFINITE);
  
  DASSERT(dwRet == WAIT_OBJECT_0);
  return m_fInitSuccess;
}

void SimpleHTTPServer::ThreadProc()
{
  WSADATA wsaData;
  SOCKET aSocket = NULL;
  WORD wVersionRequested = MAKEWORD(2, 2);
  int err = WSAStartup(wVersionRequested, &wsaData);
  if(err == 0)
  {
    aSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(aSocket != INVALID_SOCKET)
    {
      SOCKADDR_IN        ReceiverAddr;
      // The IPv4 family
      ReceiverAddr.sin_family = AF_INET;
      // Port no. 5150
      ReceiverAddr.sin_port = htons(m_iPort);
      // From all interface (0.0.0.0)
      ReceiverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
      err = bind(aSocket, (SOCKADDR*)&ReceiverAddr, sizeof(ReceiverAddr));

      m_fInitSuccess = (err == 0);
      SetEvent(m_hInitEvent);

      while(m_fInitSuccess)
      {
        c++;
        SOCKET sDataSocket = NULL;

        if(err == 0)
        {
          // socket is ready to listen
		      err = listen(aSocket, SOMAXCONN);
		      if(err == SOCKET_ERROR)
		      {
			      closesocket(aSocket);
			      aSocket = INVALID_SOCKET;
		      }
		      else
		      {
			      sockaddr sfAddr = {0};
			      int cbAddr = sizeof(sfAddr);
			      sDataSocket = accept(aSocket, &sfAddr, &cbAddr);
		      }
        }
    
        bool fConnectionLost = false;
        while(!fConnectionLost && m_fInitSuccess)
        {
	        char buf[1024];
	        int cbRead = TimeoutRead(sDataSocket,buf,sizeof(buf)-1,0,10000, &fConnectionLost);
      
          buf[cbRead] = '\0';
          cout<<buf;
          stringstream ss (stringstream::in | stringstream::out | stringstream::binary);
          stringstream ssHeader;
          HTTPREQUEST aData;
          if(ParseRequest(buf,cbRead,&aData))
          {
            m_pResponder->MakePage(aData,ss);
          
            ssHeader<<"HTTP/1.0 200 OK"<<endl;
            ssHeader<<"Server: WifiLapper"<<endl;
            ssHeader<<"Content-Type: "<<aData.strResponseType<<endl;
            ssHeader<<"Content-Length: "<<ss.str().size()<<endl;
            ssHeader<<endl; // empty line ends the headers?

            stringstream ssFinal;
            ssFinal<<ssHeader.str()<<ss.str();

            string strFinal = ssFinal.str();
            const int cbSent = send(sDataSocket,strFinal.c_str(),strFinal.size(),0);
            if(cbSent == strFinal.size())
            {
              // hooray!  a response was sent!
              // let's kill this connection and wait for the next one
              break;
            }
            else
            {
              fConnectionLost = true;
              break;
            }
          }
          else
          {
            ss<<"HTTP/1.0 500 Error";
          }
        }

        if(sDataSocket != INVALID_SOCKET)
        {
	        closesocket(sDataSocket);
	        sDataSocket = INVALID_SOCKET;
        }

      } // listening loop
    }
    else
    {
      m_fInitSuccess = false;
      SetEvent(m_hInitEvent);
    }
    
    if(aSocket != INVALID_SOCKET)
    {
	    closesocket(aSocket);
	    aSocket = INVALID_SOCKET;
    }
    WSACleanup();
  }
  else
  {
    // failed to start up winsock
    m_fInitSuccess = false;
    SetEvent(m_hInitEvent);
  }
} // end of function

DWORD ThreadProcStub(LPVOID pvParam)
{
  SimpleHTTPServer* pServer = (SimpleHTTPServer*)pvParam;
  pServer->ThreadProc();
  return 0;
}
