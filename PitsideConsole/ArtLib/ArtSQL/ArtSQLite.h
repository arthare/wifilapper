#pragma once

#include <windows.h>
#include "sqlite3.h"
class CSfArtSQLiteDB
{
public:
  CSfArtSQLiteDB() : m_sqlite3(NULL) {};
  virtual ~CSfArtSQLiteDB() {Close();};

  HRESULT Open(LPCTSTR filename);
  void Close();

  sqlite3* GetSQLite() {return m_sqlite3;};

  void StartTransaction() {ExecuteSQL("BEGIN TRANSACTION");}
  void StopTransaction() {ExecuteSQL("END TRANSACTION");}
private:
  void ExecuteSQL(const char* lpszSQL);
  CSfArtSQLiteDB(const CSfArtSQLiteDB& dbCopy); // kill the copy constructor
private:
  sqlite3* m_sqlite3;
};

class CSfArtSQLiteQuery
{
public:
  CSfArtSQLiteQuery(CSfArtSQLiteDB& DB) : m_DB(DB), m_stmt(NULL) {};
  virtual ~CSfArtSQLiteQuery() {DeInit();}

  bool Init(LPCTSTR lpszSQL, int cchSQL = 0);
  bool Next();

  bool GetCol(int iCol, double* pd);
  bool GetCol(int iCol, float* pd);
  bool GetCol(int iCol, int* pd);
  bool GetCol(int iCol, long long* p);
  bool GetCol(int iCol, LPTSTR pszResult, int cchResult);

  void DeInit();
private:
  sqlite3_stmt* m_stmt;
  CSfArtSQLiteDB& m_DB;
};