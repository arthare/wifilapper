#pragma once
#include <string>

class MulticastResponseGenerator
{
public:
  virtual void GetResponse(const char* pbReceived, int cbReceived, char** ppbResponse, int* pcbResponse) = 0;
};

class MulticastListener
{
public:
  MulticastListener(MulticastResponseGenerator* pResponder, std::string strInterface)
    : m_pResponder(pResponder),
      m_strInterface(strInterface)
  {
  }
  virtual ~MulticastListener() {};

  bool Start();
  void ThreadRoutine();
  void Stop();
private:
  void DeInit();
private:
  MulticastResponseGenerator* m_pResponder;
  void* m_hThread;
  void* m_hFinished; // event set by thread once it exits
  unsigned int m_s;
  bool m_fContinue;

  std::string m_strInterface; // what interface this object listens on
};