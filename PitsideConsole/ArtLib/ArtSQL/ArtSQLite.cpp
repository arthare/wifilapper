
#include "ArtSQLite.h"
#include "sqlite3.h"
#include "../ArtTools.h"
#include <stdio.h>

HRESULT CSfArtSQLiteDB::Open(LPCTSTR filename)
{
  HRESULT hr = S_OK;

  int iRet = sqlite3_open16((void*)filename, &m_sqlite3);
  if(iRet == SQLITE_OK)
  {
    // hooray!
  }
  else
  {
    hr = E_FAIL;
    Close();
  }

  return hr;
}
void CSfArtSQLiteDB::Close()
{
  if(m_sqlite3)
  {
    sqlite3_close(m_sqlite3);
    m_sqlite3 = NULL;
  }
}

void CSfArtSQLiteDB::ExecuteSQL(const char* lpszSQL)
{
  char* szErr = NULL;
  sqlite3_exec(this->m_sqlite3,lpszSQL,NULL, NULL,&szErr);
}

bool CSfArtSQLiteQuery::Init(LPCTSTR lpszSQL, int cchSQL)
{
  if(cchSQL <= 0)
  {
    cchSQL = wcslen(lpszSQL) * sizeof(*lpszSQL);
  }
  LPTSTR lpszEnd = NULL;
  int iRet = SQLITE_OK;
  iRet = sqlite3_prepare16(m_DB.GetSQLite(), lpszSQL, cchSQL, &m_stmt, (const void**)&lpszEnd);

  return iRet == SQLITE_OK;
}
bool CSfArtSQLiteQuery::Next()
{
  int iRet = SQLITE_OK;
  do
  {
    iRet = sqlite3_step(m_stmt);
  } while(iRet == SQLITE_BUSY);
  return iRet == SQLITE_ROW;
}
bool CSfArtSQLiteQuery::GetCol(int iCol, double* p)
{
  if(sqlite3_column_type(m_stmt, iCol) != SQLITE_FLOAT) 
  {
    DASSERT(FALSE);
    return false;
  }

  *p = sqlite3_column_double(m_stmt, iCol);
  return true;
}
bool CSfArtSQLiteQuery::GetCol(int iCol, float* p)
{
  double d = 0;
  if(GetCol(iCol,&d))
  {
    *p = (float)d;
    return true;
  }
  else
  {
    *p = 0;
    return false;
  }
}
bool CSfArtSQLiteQuery::GetCol(int iCol, int* p)
{
  if(sqlite3_column_type(m_stmt, iCol) != SQLITE_INTEGER) 
  {
    DASSERT(FALSE);
    return false;
  }

  *p = sqlite3_column_int(m_stmt, iCol);
  return true;
}
bool CSfArtSQLiteQuery::GetCol(int iCol, long long* p)
{
  if(sqlite3_column_type(m_stmt, iCol) != SQLITE_INTEGER) 
  {
    DASSERT(FALSE);
    return false;
  }

  *p = sqlite3_column_int64(m_stmt, iCol);
  return true;
}
bool CSfArtSQLiteQuery::GetCol(int iCol, LPTSTR pszResult, int cchResult)
{
  if(sqlite3_column_type(m_stmt, iCol) != SQLITE_TEXT) 
  {
    DASSERT(FALSE);
    return false;
  }

  const unsigned char* psz = sqlite3_column_text(m_stmt, iCol);
  _snwprintf(pszResult, cchResult, L"%S", psz);
  return true;
}

void CSfArtSQLiteQuery::DeInit()
{
  if(m_stmt)
  {
    sqlite3_finalize(m_stmt);
  }
}