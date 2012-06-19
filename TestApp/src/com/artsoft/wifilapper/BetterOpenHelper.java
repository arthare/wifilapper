package com.artsoft.wifilapper;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteDatabase.CursorFactory;
import android.widget.Toast;

public abstract class BetterOpenHelper 
{
	SQLiteDatabase m_db;
	public BetterOpenHelper(Context context, String strPath, CursorFactory factory,int newestVersion)
	{
		if(m_db != null)
		{
			m_db.close();
		}
		try
		{
			m_db = SQLiteDatabase.openDatabase(strPath, factory, SQLiteDatabase.OPEN_READWRITE);
		}
		catch(Exception e)
		{
			// file probably doesn't exist
		}
		if(m_db == null)
		{
			// DB must not exist, so let's create it
			try
			{
				m_db = SQLiteDatabase.openOrCreateDatabase(strPath, factory);
				if(m_db != null)
				{
					onCreate(m_db);
					m_db.setVersion(newestVersion);
				}
			}
			catch(Exception e)
			{
				
			}
		}
		else if(m_db.needUpgrade(newestVersion))
		{
			onUpgrade(m_db,m_db.getVersion(),newestVersion);
			m_db.setVersion(newestVersion);
		}
	}
	public synchronized SQLiteDatabase getWritableDatabase()
	{
		return m_db;
	}
	public abstract void onCreate(SQLiteDatabase db);
	public abstract void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion);
}
