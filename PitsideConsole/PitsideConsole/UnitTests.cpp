#include "stdafx.h"
#include "UnitTests.h"
#include "SQLiteLapDB.h";

int UnitTests()
{
  {
    CSQLiteLapDB sfLaps(NULL);
    if(!sfLaps.Init(L"../PitsideConsole/v21_phonesource.wflp")) return 1;
  }
  {
    CSQLiteLapDB sfLaps(NULL);
    if(!sfLaps.Init(L"../PitsideConsole/v21_pitsidesource.wflp")) return 2;
  }
  {
    LPCTSTR lpszName = L"databasethatnoexist";
    CSQLiteLapDB sfLaps(NULL);
    if(!sfLaps.Init(lpszName)) return 3;
    if(sfLaps.GetRaces().size() > 0) return 4;

    DeleteFile(lpszName);
  }
  {
    // adding a couple laps to a new DB, checking counts
    LPCTSTR lpszName = L"../PitsideConsole/v21_phonesource.wflp";
    CSQLiteLapDB sfLaps(NULL);
    if(!sfLaps.Init(lpszName)) return 5;
    if(sfLaps.GetRaces().size() <= 0) return 6;

  }
  // todo:
  // load laps and check lap count
  // add races and check lap count
  // add races with different car numbers and check race count
  // add races with same car numbers (different secondaries) and check race count
  // add races with same car numbers (same secondaries) and check race count

  return 0;
}