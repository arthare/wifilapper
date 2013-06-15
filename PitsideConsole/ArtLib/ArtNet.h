#pragma once
#include "winsock2.h"

int TimeoutRead(SOCKET s, char* buf, int cbBuf, int flags, int timeout, bool* pfConnectionLost);