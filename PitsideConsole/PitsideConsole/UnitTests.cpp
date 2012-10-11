#include "stdafx.h"
#include "UnitTests.h"
#include "SQLiteLapDB.h"

enum tests
{
  getappfolder=1,
  v21_phone_db,
  keithdb,
  noexistdb,
  racecountcheck,
  nospacecompare_test,
};

int UnitTests()
{
  LPCTSTR lpsz1 = L"create table races (	_id integer primary key asc autoincrement, \"name\" string, \"date\" string, \"testmode\" integer, x1 real, y1 real, x2 real, y2 real, x3 real, y3 real, x4 real, y4 real, x5 real, y5 real, x6 real, y6 real, vx1 real,vy1 real,vx2 real,vy2 real,vx3 real,vy3 real,p2p integer not null default 0,finishcount integer not null default 1)";
  LPCTSTR lpsz2 = L"CREATE TABLE races (	_id integer primary key asc autoincrement, \"name\" string, \"date\" string, \"testmode\" integer, x1 real, y1 real, x2 real, y2 real, x3 real, y3 real, x4 real, y4 real, x5 real, y5 real, x6 real, y6 real, vx1 real,vy1 real,vx2 real,vy2 real,vx3 real,vy3 real,p2p integer not null default 0, finishcount integer not null default 1)";
  if(nospacecompare(lpsz1,lpsz2)) return nospacecompare_test;

  TCHAR szDBFolder[MAX_PATH];
  if(!GetAppFolder(szDBFolder, NUMCHARS(szDBFolder))) return getappfolder;

  wcscat(szDBFolder,L"testdb.wflp");

  {
    if(!CopyFile(L"../PitsideConsole/v21_phonesource.wflp",szDBFolder,FALSE)) return -1;

    CSQLiteLapDB sfLaps(NULL);
    if(!sfLaps.Init(szDBFolder)) return v21_phone_db;
  }
  {
    if(!CopyFile(L"../PitsideConsole/New Bridged Test.wflp",szDBFolder,FALSE)) return -1;

    CSQLiteLapDB sfLaps(NULL);
    if(!sfLaps.Init(szDBFolder)) return keithdb;
  }
  {
    LPCTSTR lpszName = L"databasethatnoexist";
    CSQLiteLapDB sfLaps(NULL);
    if(!sfLaps.Init(lpszName)) return noexistdb;
    if(sfLaps.GetRaces().size() > 0) return noexistdb;

    DeleteFile(lpszName);
  }
  {
    // adding a couple laps to a new DB, checking counts
    DeleteFile(szDBFolder);
    LPCTSTR lpszName = szDBFolder;
    CSQLiteLapDB sfLaps(NULL);
    if(!sfLaps.Init(lpszName)) return racecountcheck;
    if(sfLaps.GetRaces().size() > 0) return racecountcheck;

  }
  // todo:
  // load laps and check lap count
  // add races and check lap count
  // add races with different car numbers and check race count
  // add races with same car numbers (different secondaries) and check race count
  // add races with same car numbers (same secondaries) and check race count

  return 0;
}