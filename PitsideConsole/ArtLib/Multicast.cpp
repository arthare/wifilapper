#include "StdAfx.h"
#include "Multicast.h"
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>
#include <iostream>
using namespace std;

int joinMulticastGroup(SOCKET s, string strInterface, unsigned int grpaddr)
{
   IP_MREQ imr; 
   imr.imr_interface.S_un.S_addr = inet_addr(strInterface.c_str());
   imr.imr_multiaddr.S_un.S_addr = grpaddr;
   return setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &imr, sizeof(imr));  
}
int leaveMulticastGroup(SOCKET s)
{
   /*IP_MREQ imr; 
   imr.imr_interface.S_un.S_addr = inet_addr(strInterface.c_str());
   imr.imr_multiaddr.S_un.S_addr = grpaddr;
   return setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &imr, sizeof(imr));  */
  return 0;
}

DWORD ThreadProc(LPVOID pv)
{
  MulticastListener* p = (MulticastListener*)pv;
  p->ThreadRoutine();
  return 0;
}

bool MulticastListener::Start()
{
  m_fContinue = true;
  m_hFinished = CreateEvent(NULL, FALSE, FALSE, NULL);
  if(m_hFinished)
  {
    m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&ThreadProc, this, 0, 0);
    if(m_hThread)
    {
      return true;
    }
  }
  DeInit();
  return false;
}
void MulticastListener::ThreadRoutine()
{
  WSADATA data;
  WORD dwVersion = MAKEWORD(2,2);
  int iRet = WSAStartup(dwVersion,&data);
  if(iRet == 0)
  {
    while(m_fContinue)
    {
      m_s = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
      if(m_s != INVALID_SOCKET)
      {
        sockaddr_in service;
        service.sin_family = AF_INET;
        service.sin_addr.s_addr = inet_addr(m_strInterface.c_str());
        service.sin_port = htons(63938);
        iRet = bind(m_s,(sockaddr*)&service,sizeof(service));
        if(iRet == 0)
        {
          iRet = joinMulticastGroup(m_s,m_strInterface, inet_addr("239.255.39.39"));
          while(m_fContinue)
          {
            sockaddr_in from;
            int cbFrom = sizeof(from);
            char buf[1000];
            iRet = recvfrom(m_s,buf,sizeof(buf),0,(sockaddr*)&from,&cbFrom);
            if(iRet > 0)
            {
              char* pbResponse = NULL;
              int cbResponse = 0;
              m_pResponder->GetResponse(buf,iRet,&pbResponse,&cbResponse);
              if(pbResponse)
              {
                SOCKET sSend = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
                if(sSend != INVALID_SOCKET)
                {
                  from.sin_port = htons(63937);

                  int cbSent = sendto(sSend,pbResponse,cbResponse,0,(sockaddr*)&from,cbFrom);
                  cout<<"sent "<<cbSent<<" in response"<<endl;
                }
                delete pbResponse;
              }
            }
            Sleep(100);
          }
        }
      }

      Sleep(100);
    }
  }
  SetEvent(m_hFinished);
}
void MulticastListener::Stop()
{
  shutdown((SOCKET)m_s,SD_BOTH);
  closesocket((SOCKET)m_s);
  m_fContinue = false;
  WaitForSingleObject(m_hFinished,INFINITE);
  DeInit();
}
void MulticastListener::DeInit()
{
  if(m_hThread)
  {
    CloseHandle(m_hThread);
  }
  if(m_hFinished)
  {
    CloseHandle(m_hFinished);
  }
}