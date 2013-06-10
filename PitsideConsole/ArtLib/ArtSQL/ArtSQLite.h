#pragma once

#include <windows.h>
#include "sqlite3.h"
#include <vector>
#include <string>
#include <map>
using namespace std;

class CSfArtSQLiteDB
{
public:
  CSfArtSQLiteDB() : m_sqlite3(NULL) {};
  virtual ~CSfArtSQLiteDB() {Close();};

  HRESULT Open(LPCTSTR filename, vector<wstring>& lstTableSQL, bool fReadOnly=false);
  void Close();

  sqlite3* GetSQLite() {return m_sqlite3;};

  long long GetLastInsertId() {return sqlite3_last_insert_rowid(m_sqlite3);};
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
  CSfArtSQLiteQuery(CSfArtSQLiteDB& DB) : m_fDone(false), m_iBindIndex(0),m_DB(DB), m_stmt(NULL) {};
  virtual ~CSfArtSQLiteQuery() {DeInit();}

  bool Init(LPCTSTR lpszSQL, int cchSQL = 0);
  bool Next();
  bool IsDone() const {return m_fDone;}; // for queries that don't return results, this tells us if the query finished successfully

  bool BindValue(long long value);
  bool BindValue(int value);
  bool BindValue(double value);
  bool BindValue(LPCTSTR lpszValue);

  bool GetCol(int iCol, double* pd);
  bool GetCol(int iCol, float* pd);
  bool GetCol(int iCol, int* pd);
  bool GetCol(int iCol, long long* p);
  bool GetCol(int iCol, LPTSTR pszResult, int cchResult);

  void DeInit();
private:
  int m_iBindIndex;
  bool m_fDone;

  sqlite3_stmt* m_stmt;
  CSfArtSQLiteDB& m_DB;
};