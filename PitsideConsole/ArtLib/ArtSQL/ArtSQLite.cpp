
#include "ArtSQLite.h"
#include "sqlite3.h"
#include "../ArtTools.h"
#include <stdio.h>

HRESULT CSfArtSQLiteDB::Open(LPCTSTR filename, vector<wstring>& lstTables, bool fReadOnly)
{
  HRESULT hr = S_OK;

  int iRet = 0;
  if(fReadOnly)
  {
    std::wstring wstr(filename);
    std::string str(wstr.begin(),wstr.end());

    iRet = sqlite3_open_v2(str.c_str(),&m_sqlite3,SQLITE_OPEN_READONLY,NULL);
  }
  else
  {
    iRet = sqlite3_open16((void*)filename, &m_sqlite3);
  }
  if(iRet == SQLITE_OK)
  {
    // hooray!.  Let's get the table list
    CSfArtSQLiteQuery sfQuery(*this);
    if(sfQuery.Init(L"select sql from sqlite_master order by tbl_name,type desc"))
    {
      while(sfQuery.Next())
      {
        TCHAR szSQL[1000];
        if(sfQuery.GetCol(0,szSQL,NUMCHARS(szSQL)))
        {
          lstTables.push_back(wstring(szSQL));
        }
      }
    }
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
  m_fDone = false;
  m_iBindIndex = 0;
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

  m_fDone = iRet == SQLITE_DONE;
  return iRet == SQLITE_ROW;
}
bool CSfArtSQLiteQuery::BindValue(long long value)
{
  m_iBindIndex++;

  return sqlite3_bind_int64(m_stmt,m_iBindIndex,value) == SQLITE_OK;
}
bool CSfArtSQLiteQuery::BindValue(int value)
{
  m_iBindIndex++;
  
  return sqlite3_bind_int(m_stmt,m_iBindIndex,value) == SQLITE_OK;
}
bool CSfArtSQLiteQuery::BindValue(double value)
{
  m_iBindIndex++;
  
  return sqlite3_bind_double(m_stmt,m_iBindIndex,value) == SQLITE_OK;
}
bool CSfArtSQLiteQuery::BindValue(LPCTSTR lpszValue)
{
  m_iBindIndex++;

  char szValue[2000]; // convert to single-wide text
  _snprintf(szValue, NUMCHARS(szValue), "%S", lpszValue);

  return sqlite3_bind_text(m_stmt,m_iBindIndex,szValue, strlen(szValue), SQLITE_TRANSIENT) == SQLITE_OK;
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