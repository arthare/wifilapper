#pragma once
#include "windows.h"

class ManagedCS
{
public:
	ManagedCS()
	{
		InitializeCriticalSection(&cs);
	}
	void Enter()
	{
		EnterCriticalSection(&cs);
	}
	void Leave()
	{
		LeaveCriticalSection(&cs);
	}
private:
	CRITICAL_SECTION cs;
};

class AutoLeaveCS
{
public:
	AutoLeaveCS(ManagedCS* pCS) : pCS(pCS)
	{
		pCS->Enter();
	}
	~AutoLeaveCS()
	{
		pCS->Leave();
	}
private:
	ManagedCS* pCS;
};