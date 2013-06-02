#pragma once
#include <math.h> // for rand()
#include <windows.h>
#include <stdlib.h>
#include <set>

#define NUMCHARS(x) ((sizeof(x)) / sizeof(*x))
#define NUMITEMS(x) ((sizeof(x)) / sizeof(*x))

#define IS_FLAG_SET(fdw,flag) (((fdw) & (flag)) != 0)

#define PI 3.14159

inline double RandDouble()
{
  return ((double)(rand() % 10000)) / 10000.0;
};

int GetCheckSum(LPVOID pvData, const int cbData);

template<class c> c FLIPBITS(c v)
{
	char output[sizeof(c)];
	char* input = (char*)&v;
	for(int x = 0; x < sizeof(v); x++)
	{
		output[x] = input[sizeof(v)-x-1];
	}
	c result = *(c*)(output);
	return result;
}
#define FLIP(x) ((x) = FLIPBITS((x)))

struct FLOATRECT
{
  float left;
  float right;
  float top;
  float bottom;
};

#define KMH_TO_MS(x) ((x)/3.6)
#define MPH_TO_MS(x) ((x)*1.609/3.6)

#define RECT_WIDTH(prc) (((prc)->right)-((prc)->left))
#define RECT_HEIGHT(prc) (((prc)->bottom)-((prc)->top))

inline void Noop()
{
  __asm nop;
}
inline void Break()
{
  __asm int 3;
}

template<int i> void TemplatedFunction();


#ifdef DEBUG
#define DASSERT(x) (  (x) ? Noop() : Break())
#else
#define DASSERT(x)
#endif

#define VERIFY(x) (  (x) ? Noop() : Break())
#define CASSERT(x) (TemplatedFunction<x>())
void FormatTimeMinutesSecondsMs(float flTimeInSeconds, LPTSTR lpszBuffer, int cchBuffer);

template<class T>
bool AreSetsEqual(const std::set<T>& set1, const std::set<T>& set2)
{
  if(set1.size() != set2.size()) return false;

  // since this implies that set 2 also has size zero, based on above if
  if(set1.size() == 0) return true;

  for(std::set<T>::const_iterator i = set1.begin(); i != set1.end(); i++)
  {
    // we need to find every single element of set1 in set2
    std::set<T>::const_iterator i2 = set2.find(*i);
    if(i2 == set2.end()) return false; // couldn't find it
  }
  return true;
}

bool SaveBufferToFile(LPCTSTR lpszPath, void* pvData, int cbData);

SYSTEMTIME SecondsSince1970ToSYSTEMTIME(int cSeconds);
int GetSecondsSince1970();

bool ArtAtoi(LPCSTR lpsz, int cch, int* pOut);

bool GetAppFolder(LPTSTR lpszBuf, const int cch);

bool DoesFileExist(LPCTSTR lpsz);

// compares two strings, ignoring spaces
int nospacecompare(LPCTSTR lpsz1, LPCTSTR lpsz2);