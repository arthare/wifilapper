// Copyright 2011-2012, Art Hare
// This file is part of WifiLapper.

//WifiLapper is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//WifiLapper is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with WifiLapper.  If not, see <http://www.gnu.org/licenses/>.

package com.artsoft.wifilapper;

import java.util.ArrayList;
import java.util.List;
import java.util.Vector;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteDatabase.CursorFactory;
import android.database.sqlite.SQLiteStatement;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Environment;
import android.util.Log;
import android.widget.Toast;

public class RaceDatabase extends BetterOpenHelper
{
	public RaceDatabase(Context context, String strPath, CursorFactory factory,int version) 
	{
		super(context, strPath, factory, version);
	}
	
	private static RaceDatabase g_raceDB = null;
	
	// ahare: 1->2 clearing shit in debug
	// ahare: 2->3 adding start/finish lines to races
	// ahare: 3->14 making races table actually work, added laps and points tables
	// 14->15; removed internal lapid.  The DB lapid will be sufficiently unique
	// 15->16: messed everything up, need freshness
	// 16->17: adding accelerometer data in DB
	// 20->21: adding "point-to-point" member in races table
	private static final int m_iVersion = 23;
	private static final String DATABASE_NAME_INTERNAL = "races";
	
	public static final String KEY_RACENAME = "name";
	public static final String KEY_LAPCOUNT = "lapcount";
	public static final String KEY_RACEID = "raceid";
	public static final String KEY_LAPTIME = "laptime";
	public static final String KEY_STARTTIME = "starttime";
	public static final String KEY_P2P = "p2p";
	public static final String KEY_FINISHCOUNT = "finishcount";
	public static boolean CreateInternal(Context ctx, String strBasePath)
	{
		String strPath = strBasePath + "/" + DATABASE_NAME_INTERNAL;
		return CreateOnPath(ctx, strPath);
	}
	public static boolean CreateExternal(Context ctx)
	{
		//RaceDatabase race = new RaceDatabase(ctx, DATABASE_NAME, null, m_iVersion);
		String strPath = Environment.getExternalStorageDirectory().toString();
		return CreateOnPath(ctx, strPath + "/wifilapper/" + DATABASE_NAME_INTERNAL);
	}
	public static boolean CreateOnPath(Context ctx, String strPath)
	{
		RaceDatabase race = new RaceDatabase(ctx, strPath, null, m_iVersion);
		if(race.m_db != null)
		{
			if(g_raceDB != null && g_raceDB.m_db != null)
			{
				g_raceDB.m_db.close();
			}
			g_raceDB = race;
		}
		return race.m_db != null;
	}
	public synchronized static SQLiteDatabase Get()
	{
		return g_raceDB.getWritableDatabase();
	}
	
	public synchronized static long CreateRaceIfNotExist(SQLiteDatabase db, String strRaceName, LapAccumulator.LapAccumulatorParams lapParams, boolean fTestMode, boolean fP2P, int iFinishCount)
	{
		if(db == null)
		{
			Toast.makeText(null, "Wifilapper was unable to create a database.  Race will not be saved", Toast.LENGTH_LONG).show();
			return -1;
		}
		if(strRaceName != null && strRaceName.length() > 0 && lapParams.IsValid(fP2P))
		{
			ContentValues content = new ContentValues();
			content.put("\"name\"", strRaceName);
			content.put("\"date\"", "");
			content.put("x1", lapParams.lnStart != null ? lapParams.lnStart.GetP1().GetX() : 0);
			content.put("y1", lapParams.lnStart != null ? lapParams.lnStart.GetP1().GetY() : 0);
			content.put("x2", lapParams.lnStart != null ? lapParams.lnStart.GetP2().GetX() : 0);
			content.put("y2", lapParams.lnStart != null ? lapParams.lnStart.GetP2().GetY() : 0);
			
			content.put("x3", lapParams.lnStop != null ? lapParams.lnStop.GetP1().GetX() : 0);
			content.put("y3", lapParams.lnStop != null ? lapParams.lnStop.GetP1().GetY() : 0);
			content.put("x4", lapParams.lnStop != null ? lapParams.lnStop.GetP2().GetX() : 0);
			content.put("y4", lapParams.lnStop != null ? lapParams.lnStop.GetP2().GetY() : 0);
			
			content.put("x5", 0);
			content.put("y5", 0);
			content.put("x6", 0);
			content.put("y6", 0);
			
			content.put("vx1", lapParams.vStart != null ? lapParams.vStart.GetX() : 0);
			content.put("vy1", lapParams.vStart != null ? lapParams.vStart.GetY() : 0);
			
			content.put("vx2", lapParams.vStop != null ? lapParams.vStop.GetX() : 0);
			content.put("vy2", lapParams.vStop != null ? lapParams.vStop.GetY() : 0);
			
			content.put("vx3", 0);
			content.put("vy3", 0);
			
			
			content.put("\"testmode\"", fTestMode);
			content.put("p2p", fP2P ? 1 : 0);
			content.put("finishcount", iFinishCount);
			
			try
			{
				long id = db.insertOrThrow("races", null, content);
				return id;
			}
			catch(SQLiteException e)
			{
				return -1;
			}
		}
		return -1;
	}
	public static class RaceData
	{
		public LapAccumulator.LapAccumulatorParams lapParams;
		public String strRaceName;
		public boolean fTestMode;
		public long unixTimeMsStart;
		public long unixTimeMsEnd;
		public RaceData()
		{
			lapParams = new LapAccumulator.LapAccumulatorParams();
		}
	}
	public static class LapData
	{
		public LapData()
		{
			flLapTime = 0;
			lStartTime = 0;
			lLapId = 0;
			lRaceId = 0;
		}
		public LapData(LapData src)
		{
			flLapTime = src.flLapTime;
			lStartTime = src.lStartTime;
			lLapId = src.lLapId;
			lRaceId = src.lRaceId;
		}
		public float flLapTime;
		public long lStartTime; // in seconds since 1970
		public long lLapId;
		public long lRaceId;
	}
	public synchronized static RaceData GetRaceData(SQLiteDatabase db, long id, int iCarNumber)
	{
		Cursor cur = db.rawQuery("select x1,y1,x2,y2,x3,y3,x4,y4,x5,y5,x6,y6,vx1,vy1,vx2,vy2,vx3,vy3,name,testmode,finishcount from races where _id = " + id, null);
		if(cur != null)
		{
			cur.moveToFirst();
			RaceData ret = new RaceData();
			ret.strRaceName = cur.getString(18);
			ret.fTestMode = cur.getInt(19) != 0;
			final int iFinishCount = cur.getInt(20);
			float rgSF[] = new float[12];
			float rgSFDir[] = new float [6];
			for(int x = 0; x < 12; x++)
			{
				rgSF[x] = (float)cur.getDouble(x);
			}
			for(int x = 0; x < 6; x++)
			{
				rgSFDir[x] = (float)cur.getDouble(x+12);
			}
			ret.lapParams.InitFromRaw(rgSF,rgSFDir, iCarNumber, iFinishCount);
			
			cur.close();
			
			cur = db.rawQuery("select min(laps.unixtime) as starttime,max(laps.unixtime) as endtime from laps where raceid = " + id,null);
			if(cur != null)
			{
				cur.moveToFirst();
				ret.unixTimeMsStart = cur.getLong(0)*1000;
				ret.unixTimeMsEnd = cur.getLong(1)*1000;
				cur.close();
			}
			return ret;
		}
		return null;
	}
	private static void DoOrphanCheck(SQLiteDatabase db)
	{
		db.execSQL("create temporary table if not exists tempraceid (raceid integer);");
		db.execSQL("create temporary table if not exists templapid (lapid integer);");
		db.execSQL("create temporary table if not exists tempdataid (dataid integer);");
		db.execSQL("delete from tempraceid;");
		db.execSQL("delete from templapid;");
		db.execSQL("delete from tempdataid;");
		db.execSQL("insert into templapid select _id from laps where raceid not in (select _id from races);"); // finds all orphaned laps
		db.execSQL("insert into tempdataid select _id from channels where lapid in (select lapid from templapid);"); // finds all channels that depend on those orphaned laps
		db.execSQL("insert into tempdataid select _id from channels where lapid not in (select _id from laps);"); // finds all channels that depend on missing laps

		db.execSQL("delete from data where channelid in (select dataid from tempdataid);"); // deletes all data depending on the orphaned channels
		db.execSQL("delete from points where lapid in (select lapid from templapid);"); // deletes all points that depend on the targeted laps
		db.execSQL("delete from channels where _id in (select dataid from tempdataid);"); // deletes all channels that need deleting
		db.execSQL("delete from extras where _id in (select dataid from tempdataid);"); // deletes all channels that need deleting
		db.execSQL("delete from laps where _id in (select lapid from templapid);"); // deletes all targeted laps
		
		db.execSQL("delete from points where lapid not in (select _id from laps);"); // deletes all points that depend on nonexistent laps
		db.execSQL("delete from data where channelid not in (select _id from channels);"); // deletes all data that depend on nonexistent channels
	}
	public synchronized static void DeleteTestData(SQLiteDatabase db)
	{
		if(db == null) return;

		db.execSQL("begin transaction;");
		db.execSQL(	"create table if not exists tempraceid (raceid integer);");
		db.execSQL(	"create table if not exists templapid (lapid integer);");
		db.execSQL(	"create table if not exists tempdataid (dataid integer);");	
		db.execSQL(	"delete from tempraceid;");
		db.execSQL(	"delete from templapid;");
		db.execSQL(	"delete from tempdataid;");
		db.execSQL(	"insert into tempraceid select _id from races where testmode=1;"); // finds all the races we want to delete
		db.execSQL(	"insert into templapid select _id from laps where raceid in (select raceid from tempraceid);"); // finds all the laps that depend on those races
		db.execSQL(	"insert into tempdataid select _id from channels where lapid in (select lapid from templapid);"); // finds all the channels that depend on those laps
		
		db.execSQL("delete from data where channelid in (select dataid from tempdataid);"); // deletes all raw data that depends on those channels
		db.execSQL("delete from points where lapid in (select lapid from templapid);"); // deletes all the points that depend on the doomed laps
		db.execSQL("delete from channels where lapid in (select lapid from templapid);"); // deletes all the channels that depend on the doomed laps
		db.execSQL("delete from extras where lapid in (select lapid from templapid);"); // deletes all the channels that depend on the doomed laps
		db.execSQL("delete from laps where raceid in (select raceid from tempraceid);"); // deletes all the laps that depend on the doomed races
		db.execSQL("delete from races where _id in (select raceid from tempraceid);"); // deletes all the doomed races
		db.execSQL("commit transaction;");
		
		DoOrphanCheck(db);
	}
	public synchronized static LapAccumulator GetBestLap(SQLiteDatabase db, LapAccumulator.LapAccumulatorParams lapParams, long lRaceId)
	{
		String strSQL;
		strSQL = "select _id, laptime, unixtime, raceid, min(laps.laptime) from laps where raceid = " + lRaceId;
		Cursor curLap = db.rawQuery(strSQL, null);
		if(curLap != null)
		{
			int cLaps = curLap.getCount();
			System.out.println("There are " + cLaps + " laps");
			while(curLap.moveToNext())
			{
				double dLapTime = curLap.getDouble(4);
				strSQL = "select _id, laptime, unixtime, raceid, min(laps.laptime) from laps where raceid = " + lRaceId + " and laptime = " + dLapTime;
				Cursor curBestLap = db.rawQuery(strSQL, null);
				if(curBestLap != null)
				{
					while(curBestLap.moveToNext())
					{
						int lLapDbId = curBestLap.getInt(0);
						int iUnixTime = curBestLap.getInt(2);
						LapAccumulator lap = new LapAccumulator(lapParams, iUnixTime, lLapDbId);
						curBestLap.close();
						curLap.close();
						return lap;
					}
				}
			}
			curLap.close();
		}
		return null;
	}
	public synchronized static LapAccumulator GetLap(SQLiteDatabase db, LapAccumulator.LapAccumulatorParams lapParams, long lLapId)
	{
		String strSQL = "select _id, laptime, unixtime, raceid, min(laps.laptime) from laps where _id = " + lLapId;
		Cursor curBestLap = db.rawQuery(strSQL, null);
		if(curBestLap != null)
		{
			while(curBestLap.moveToNext())
			{
				int lLapDbId = curBestLap.getInt(0);
				int iUnixTime = curBestLap.getInt(2);
				LapAccumulator lap = new LapAccumulator(lapParams, iUnixTime, lLapDbId);
				curBestLap.close();
				return lap;
			}
		}
		return null;
	}
	public synchronized static LapData[] GetLapDataList(SQLiteDatabase db, long lRaceId)
	{
		String strSQL;
		strSQL = "select _id, laptime, unixtime from laps where raceid = " + lRaceId;
		
		Cursor curLap = db.rawQuery(strSQL, null);
		
		LapData[] lst = new LapData[curLap.getCount()];
		int ix = 0;
		while(curLap.moveToNext())
		{
			LapData lap = new LapData();
			lap.flLapTime = (float)curLap.getDouble(1);
			lap.lLapId = curLap.getInt(0);
			lap.lStartTime = curLap.getLong(2);
			lap.lRaceId = lRaceId;
			lst[ix] = lap;
			ix++;
		}
		curLap.close();
		return lst;
	}
	public synchronized static Vector<LapAccumulator> GetLaps(SQLiteDatabase db, LapAccumulator.LapAccumulatorParams lapParams, long id)
	{
		Vector<LapAccumulator> lstRet = new Vector<LapAccumulator>();
		String strSQL;
		strSQL = "select _id, laptime, unixtime, raceid from laps where raceid = " + id;
		Cursor curLap = db.rawQuery(strSQL, null);
		if(curLap != null)
		{
			int cLaps = curLap.getCount();
			System.out.println("There are " + cLaps + " laps");
			while(curLap.moveToNext())
			{
				int lLapDbId = curLap.getInt(0);
				int iUnixTime = curLap.getInt(2);
				LapAccumulator lap = new LapAccumulator(lapParams, iUnixTime, lLapDbId);
				lstRet.add(lap);
			}
			curLap.close();
		}
		return lstRet;
	}
	public synchronized static void RenameRace(SQLiteDatabase db, int id, String strNewName)
	{
		try
		{
			db.beginTransaction();
			db.execSQL("update races set name = '" + strNewName + "' where _id = '" + id + "'");
			db.setTransactionSuccessful();
		}
		catch(SQLiteException e)
		{
			
		}
		db.endTransaction();
	}
	public synchronized static void DeleteRace(SQLiteDatabase db, int id)
	{
		try
		{
			db.beginTransaction();
			db.execSQL("delete from races where _id = '" + id + "'");
			db.execSQL("delete from laps where raceid = '" + id + "'");
			db.setTransactionSuccessful();
			
			RaceDatabase.DoOrphanCheck(db);
		}
		catch(SQLiteException e)
		{
			
		}
		db.endTransaction();
	}
	public synchronized static Bitmap GetRaceOutlineImage(SQLiteDatabase db, long lRaceId, int width, int height)
	{
		RaceData raceData = GetRaceData(db, lRaceId, -1);
		if(raceData != null)
		{
			LapAccumulator.LapAccumulatorParams lapParams = raceData.lapParams;
			
			LapAccumulator lap = GetBestLap(db, lapParams, lRaceId);
			if(lap != null)
			{
				Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
				if(bmp != null)
				{
					Canvas canvas = new Canvas(bmp);
					lap.DoDeferredLoad(null, 0, false);
					
					Paint paintBlack = new Paint();
					paintBlack.setARGB(255,0,0,0);
					canvas.drawRect(0,0,canvas.getWidth(),canvas.getHeight(),paintBlack);
					
					FloatRect rcInWorld = lap.GetBounds(false);
					
					Paint paintLap = new Paint();
					paintLap.setARGB(255, 25, 140, 225);
					
					Rect rcOnScreen = new Rect(0,0,width,height);
					LapAccumulator.DrawLap(lap, false, rcInWorld, canvas, paintLap, null, rcOnScreen);
				}
				return bmp;
			}
		}
		return null;
	}
	public synchronized static Cursor GetRaceList(SQLiteDatabase db)
	{
		try
		{
			Cursor cur = db.rawQuery("select races.finishcount as " + KEY_FINISHCOUNT + ", races.p2p as " + KEY_P2P + ", races._id as raceid,min(laps.laptime) as " + KEY_LAPTIME + ", races.name as name,count(laps._id) as lapcount, min(laps.unixtime) as " + KEY_STARTTIME + " from races left join laps on races._id = laps.raceid group by races._id order by races._id", null);

			return cur;
		}
		catch(SQLiteException e)
		{
			Log.w("sqldb",e.toString());
			e.printStackTrace();
		}
		
		return null;
	}
	
	private final static String CREATE_RACE_SQL =  "create table if not exists races (	_id integer primary key asc autoincrement, " +
																						"\"name\" string, " +
																						"\"date\" string, " +
																						"\"testmode\" integer, " +
																						"x1 real, " +
																						"y1 real, " +
																						"x2 real, " +
																						"y2 real, " +
																						"x3 real, " +
																						"y3 real, " +
																						"x4 real, " +
																						"y4 real, " +
																						"x5 real, " +
																						"y5 real, " +
																						"x6 real, " +
																						"y6 real, " +
																						"vx1 real," +
																						"vy1 real," +
																						"vx2 real," +
																						"vy2 real," +
																						"vx3 real," +
																						"vy3 real," +
																						"p2p integer not null default 0," +
																						"finishcount integer not null default 1)";
	
	private final static String CREATE_LAPS_SQL = "create table if not exists laps " +
													"(_id integer primary key asc autoincrement, " +
													"laptime real, " + 
													"unixtime integer, " +
													"transmitted integer, " +
													"raceid integer," +
													"foreign key (raceid) references races(_id));";
	
	private final static String CREATE_POINTS_SQL = "create table if not exists points " +
													"(_id integer primary key asc autoincrement, " +
													"x real," +
													"y real," +
													"time integer," +
													"velocity real," +
													"lapid integer," +
													"foreign key (lapid) references laps(_id));";
	
	private final static String CREATE_CHANNELS_SQL = "create table if not exists channels" +
													  "(_id integer primary key asc autoincrement, " +
													  "lapid integer NOT NULL," +
													  "channeltype integer NOT NULL," +
													  "foreign key(lapid) references laps(_id));";
	
	private final static String CREATE_DATA_SQL = "create table if not exists data " +
														 "(_id integer primary key asc autoincrement," +
														 "time integer NOT NULL," +
														 "value real NOT NULL," +
														 "channelid integer NOT NULL," +
														 "foreign key (channelid) references channels(_id));";
	
	private final static String CREATE_EXTRA_SQL =	"create table extras " +
													"(_id integer primary key asc autoincrement," +
													"comment string," +
													"lapid integer NOT NULL unique on conflict fail," +
													"foreign key (lapid) references laps(_id))";
	
	private final static String CREATE_INDICES ="create index if not exists data_channelid on data(channelid);" +
												"create index if not exists points_lapid on points(lapid);" +
												"create index if not exists laps_raceid on laps(raceid);";
	@Override
	public void onCreate(SQLiteDatabase db)
	{
		db.execSQL(CREATE_RACE_SQL);
		db.execSQL(CREATE_LAPS_SQL);
		db.execSQL(CREATE_POINTS_SQL);
		db.execSQL(CREATE_CHANNELS_SQL);
		db.execSQL(CREATE_DATA_SQL);
		db.execSQL(CREATE_INDICES);
		db.execSQL(CREATE_EXTRA_SQL);
	}
	private void ExecAndIgnoreException(SQLiteDatabase db, String strSQL)
	{
		try
		{
			db.execSQL(strSQL);
		}
		catch(SQLiteException e)
		{
			
		}
	}
	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion)
	{
		if(oldVersion <= 18)
		{
			ExecAndIgnoreException(db,"drop table races");
			ExecAndIgnoreException(db,"drop table laps");
			ExecAndIgnoreException(db,"drop table points");
			ExecAndIgnoreException(db,"drop table channels");
			ExecAndIgnoreException(db,"drop table data");
			ExecAndIgnoreException(db,"drop table extras");
			onCreate(db);
		}
		else
		{
			if(oldVersion == 19 && newVersion > 19)
			{
				// 19->20: added indices for more speed
				db.execSQL(CREATE_INDICES);
				oldVersion = 20;
			}
			if(oldVersion == 20 && newVersion > 20)
			{
				// upgrade more...
				db.execSQL("alter table races add column p2p integer not null default 0");
				oldVersion = 21;
			}
			if(oldVersion == 21 && newVersion > 21)
			{
				// upgrade more...
				db.execSQL("alter table races add column finishcount integer not null default 1");
				oldVersion = 22;
			}
			if(oldVersion == 22 && newVersion > 22)
			{
				db.execSQL(CREATE_EXTRA_SQL); // add the "extras" table
				oldVersion = 23;
			}
		}
	}
}
