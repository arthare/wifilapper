package com.artsoft.wifilapper;

import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;
import java.util.zip.Deflater;
import java.util.zip.DeflaterOutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteStatement;
import android.graphics.*;
import android.location.Location;
import android.os.Debug;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class LapAccumulator 
{
	public LapAccumulator(List<LineSeg> lstLines, List<Vector2D> lstLineDirections, Point2D ptStart, int iUnixStartTime, int iLapId, int iStartTime, double dVelocity)
	{
		Init(lstLines, lstLineDirections, ptStart, iUnixStartTime, iLapId, iStartTime, dVelocity);
	}
	// used for the first lap, when we don't have a unixstarttime or a lapid
	public LapAccumulator(List<LineSeg> lstLines, List<Vector2D> lstLineDirections, Point2D ptStart, double dVelocity)
	{
		int iUnixStartTime = (int)(System.currentTimeMillis() / 1000);
		Init(lstLines, lstLineDirections, ptStart, iUnixStartTime, -1, 0, dVelocity);
	}
	
	// used for laps we load from the DB, and want to defer point-loading
	public LapAccumulator(List<LineSeg> lstLines, List<Vector2D> lstLineDirections, int iUnixStartTime, int lLapId)
	{
		m_fDeferredLoad = true;
		Init(lstLines, lstLineDirections, null, iUnixStartTime, lLapId, 0, 0);
	}
	public ArrayList<TimePoint2D> GetPoints()
	{
		return this.m_lstPositions;
	}
	private void Init(List<LineSeg> lstLines, List<Vector2D> lstLineDirections, Point2D ptStart, int iUnixStartTime, int iLapId, int iStartTime, double dVelocity)
	{
		m_lstChannels = new ArrayList<DataChannel>();
		
		m_iUnixStartTime = (int)(iUnixStartTime);
		m_iLapId = iLapId;
		m_lstLines = lstLines;
		m_lstLineDirections = lstLineDirections;
		m_iStartTime = iStartTime;
		m_lstPositions = new ArrayList<TimePoint2D>();
		m_lstCrossTimes = new ArrayList<Integer>();

		m_dCumulativeDistance = 0;
		assert(ptStart != null);
		if(ptStart != null)
		{
			 m_lstPositions.add(new TimePoint2D(ptStart, iStartTime, dVelocity, m_dCumulativeDistance));
		}
	}
	public LapAccumulator CreateCopy()
	{
		DoDeferredLoad(null, 0, true);
		
		TimePoint2D pt = m_lstPositions.get(0);
		LapAccumulator lap = new LapAccumulator(m_lstLines, m_lstLineDirections, pt.pt, m_iUnixStartTime, m_iLapId, m_iStartTime, pt.dVelocity);

		for(int x = 0;x < m_lstPositions.size(); x++)
		{
			pt = m_lstPositions.get(x);
			lap.AddPosition(pt.pt, pt.iTime, pt.dVelocity);
		}
		for(int x = 0;x < m_lstChannels.size(); x++)
		{
			lap.AddDataChannel(m_lstChannels.get(x));
		}
		
		return lap;
	}
	
	// calculates and returns this guy's length in km
	public float GetCalculatedLength()
	{
		float flLength = 0;
		for(int x = 0;x < m_lstPositions.size()-1; x++)
		{
			TimePoint2D pt = m_lstPositions.get(x);
			TimePoint2D ptNext = m_lstPositions.get(x+1);
			
			float results[] = new float[1];
			Location.distanceBetween(pt.pt.y, pt.pt.x, ptNext.pt.y, ptNext.pt.x, results);
			flLength += results[0];
		}
		flLength/=1000.0f;
		return flLength;
	}
	// starts a thread to do a deferred load, and will sent progress reports back to pCallback using msg.what = iMsg
	public void ThdDoDeferredLoad(Handler pCallback, int iMsg, boolean fLoadData)
	{
		Thread deferred = new DeferredLoader(this, pCallback, iMsg, fLoadData);
		deferred.start();
	}
	private static class DeferredLoader extends Thread
	{
		Handler 		m_pCallback;
		int 			m_iMsg;
		LapAccumulator 	m_pLoadThis;
		boolean 		m_fLoadData;
		public DeferredLoader(LapAccumulator pLoadThis, Handler pCallback, int iMsg, boolean fLoadData)
		{
			m_pLoadThis = pLoadThis;
			m_pCallback = pCallback;
			m_iMsg = iMsg;
			m_fLoadData = fLoadData;
		}
		@Override
		public void run()
		{
			m_pLoadThis.DoDeferredLoad(m_pCallback, m_iMsg, m_fLoadData);
		}
	}
	public DataChannel GetDataChannel(int iChannel)
	{
		for(int x = 0;x < m_lstChannels.size(); x++)
		{
			DataChannel data = m_lstChannels.get(x);
			if(data.iType == iChannel)
			{
				return data;
			}
		}
		return null;
	}
	public void DoDeferredLoad(Handler pCallback, int iMsg, boolean fLoadData)
	{
		if(!m_fDeferredLoad) return; // if we've already loaded or aren't a deferred-load lap, then don't load
		if(m_fPruned) return; // we're a pruned lap, and shouldn't be loading from the network.
		
		assert(m_iLapId != -1);
		if(m_iLapId != -1)
		{
			SQLiteDatabase db = RaceDatabase.Get();
			if(m_iLapId != -1)
			{
				int cData = 0;
				int cPoints = 0;
				int cItemsDone = 0; // the total number of points and data we've loaded
				
				if(pCallback != null) // if we need to know the count of data and points for this lap to do progress, figure that out
				{
					if(fLoadData)
					{ // get data count
						final String strDataSQL = "select count(*) from data,channels where data.channelid=channels._id and channels.lapid=" + m_iLapId;
						Cursor curCount = db.rawQuery(strDataSQL, null);
						if(curCount.moveToNext())
						{
							cData = curCount.getInt(0);
						}
						curCount.close();
					}
					{ // get points count
						final String strPointsSQL = "select count(*) from points where points.lapid = " + m_iLapId;
						Cursor curCount = db.rawQuery(strPointsSQL, null);
						if(curCount.moveToNext())
						{
							cPoints = curCount.getInt(0);
						}
						curCount.close();
					}
				}
				
				{
					final String strPointsSQL = "select x, y, time, velocity from points where lapid = " + m_iLapId;
					Cursor curPoints = db.rawQuery(strPointsSQL, null);
					while(curPoints.moveToNext())
					{ // load points
						Point2D p = new Point2D((float)curPoints.getDouble(0), (float)curPoints.getDouble(1));
						int iTime = curPoints.getInt(2);
						double dVelocity = curPoints.getDouble(3);
						AddPosition(p, iTime, dVelocity);
						cItemsDone++;
						if(pCallback != null && cItemsDone % 100 == 0) {Message m = Message.obtain(); m.what = iMsg; m.arg1 = cItemsDone; m.arg2 = (cData+cPoints); pCallback.sendMessage(m);}
					}
					curPoints.close();
				}
				
				if(fLoadData)
				{ // load data
					final String strChannelsSQL = "select _id,channeltype from channels where lapid = " + m_iLapId;
					Cursor curChannel = db.rawQuery(strChannelsSQL, null);
					while(curChannel.moveToNext())
					{
						final long iChannelId = curChannel.getLong(0);
						final long iChannelType = curChannel.getLong(1);
						DataChannel d = new DataChannel((int)iChannelType, this);
						String strDataSQL = "select time,value from data where channelid = " + iChannelId;
						Cursor curData = db.rawQuery(strDataSQL, null);
						while(curData.moveToNext())
						{
							final long iTime = curData.getLong(0);
							final float flValue = curData.getFloat(1);
							d.AddData(flValue, (int)iTime);
							cItemsDone++;
							if(pCallback != null && cItemsDone % 100 == 0) {Message m = Message.obtain(); m.what = iMsg; m.arg1 = cItemsDone; m.arg2 = (cData+cPoints); pCallback.sendMessage(m);}
						}
						curData.close();
						this.AddDataChannel(d);
					}
					curChannel.close();
				}
				if(pCallback != null)
				{
					// tell them we're done
					Message m = Message.obtain(); 
					m.what = iMsg; 
					m.arg1 = m.arg2 = (cData+cPoints); 
					pCallback.sendMessage(m);
				}
			}
		}
		m_fDeferredLoad = false; // even if we had a failure here, we're not doing this again (unless on a future run of the app)
	}
	public boolean IsInDB()
	{
		return m_iLapId != -1;
	}
	public long GetAgeInSeconds()
	{
		int iUnixNowTime = (int)(System.currentTimeMillis() / 1000);
		return iUnixNowTime - m_iUnixStartTime;
	}
	public long GetAgeInMilliseconds()
	{
		if(m_lstPositions.size() > 0)
		{
			return m_lstPositions.get(m_lstPositions.size()-1).iTime - m_lstPositions.get(0).iTime;
		}
		return 0;
	}
	public float GetDistance()
	{
		synchronized(this)
		{
			return (float)m_lstPositions.get(m_lstPositions.size()-1).dDistance;
		}
	}
	public synchronized boolean IsPruned()
	{
		return m_fPruned;
	}
	public synchronized void Prune()
	{
		// shuts down the data channels and lstPoints, because they're gobbling all our memory
		m_fDeferredLoad = true;
		m_fPruned = true;
		for(int x = 0;x < m_lstChannels.size(); x++)
		{
			m_lstChannels.get(x).Clear();
		}
		this.m_lstChannels.clear();
		m_lstChannels = null;
		this.m_lstPositions.clear();
		m_lstPositions = null;
		m_pPrecachedToNetwork = null;
	}
	private synchronized void BuildByteArray()
	{
		if(m_pPrecachedToNetwork != null) return; // work has already been done!
		if(IsPruned()) return; // we're a pruned lap, and don't need to precache network stuff!
		DoDeferredLoad(null, 0, true);
		
		ByteArrayOutputStream bytes = new ByteArrayOutputStream(100000);
		DataOutputStream out = new DataOutputStream(bytes);
		try
		{
			out.writeByte('j');
			out.writeByte('n');
			out.writeByte('d');
			out.writeByte('a');
			out.writeByte('d');
			out.writeByte('e');
			out.writeByte('r');
			out.writeByte('e');

			out.writeInt((int)m_iLapId);
			out.writeInt(this.m_lstPositions.size());
			out.writeFloat((float)GetLapTime());
			out.writeInt(m_iUnixStartTime);
			
			for(int x = 0;x < 3; x++)
			{
				if(x < m_lstLines.size())
				{
					LineSeg ln = m_lstLines.get(x);
					out.writeFloat(ln.GetP1().x);
					out.writeFloat(ln.GetP1().y);
					out.writeFloat(ln.GetP2().x);
					out.writeFloat(ln.GetP2().y);
				}
				else
				{
					out.writeFloat(0);
					out.writeFloat(0);
					out.writeFloat(0);
					out.writeFloat(0);
				}
			}
			for(int x = 0;x < 3; x++)
			{
				if(x < m_lstLineDirections.size())
				{
					Vector2D v = m_lstLineDirections.get(x);
					out.writeFloat(v.GetX());
					out.writeFloat(v.GetY());
				}
				else
				{
					out.writeFloat(0);
					out.writeFloat(0);
				}
			}
			for(int x = 0;x < m_lstPositions.size(); x++)
			{
				TimePoint2D t = m_lstPositions.get(x);
				if(!t.SendToOutput(out))
				{
					m_pPrecachedToNetwork = null;
					return;
				}
			}
			
			out.writeByte('d');
			out.writeByte('o');
			out.writeByte('n');
			out.writeByte('e');
			out.writeByte('l');
			out.writeByte('a');
			out.writeByte('p');
			out.writeByte('_');
			out.flush();

			for(int x = 0; x < this.m_lstChannels.size(); x++)
			{
				m_lstChannels.get(x).SendToOutput(out);
			}
			m_pPrecachedToNetwork = bytes.toByteArray();
		}
		catch(Exception e)
		{
			// outputstream failed, probably lost network
			m_pPrecachedToNetwork = null;
		}
	}
	public boolean SendToOutput(OutputStream os)
	{
		BuildByteArray();
		
		byte pPrecached[] = null;
		synchronized(this)
		{
			pPrecached = m_pPrecachedToNetwork; // grabs a local copy in case it gets updated
		}
		
		if(pPrecached != null)
		{
			try
			{
				os.write(pPrecached);
				os.flush();
				return true;
			}
			catch(Exception e)
			{
				return false;
			}
		}
		else
		{
			return false;
		}
		
	}
	public void MarkSentInDB(SQLiteDatabase db)
	{
		assert(!m_fDeferredLoad); // how would we send this lap without having all its points?
		assert(m_iLapId != -1);
		if(m_iLapId != -1)
		{
			db.execSQL("update laps set transmitted=1 where _id = " + this.m_iLapId);
		}
	}
	public void PrepareForNetwork()
	{
		BuildByteArray();
	}
	public boolean SendToDB(SQLiteDatabase db, long lRaceId, boolean fTransmitted) throws SQLiteException
	{
		assert(lRaceId != -1);
		if(m_iLapId != -1 || m_fDeferredLoad)
		{
			// this lap is already in the database!
			return true;
		}
		// each lap has:
		// lapId
		// laptime
		// unix start time
		// transmitted
		// race id
		boolean fRet = true;
		ContentValues content = new ContentValues();
		content.put("laptime",GetLapTime());
		content.put("unixtime", m_iUnixStartTime);
		content.put("transmitted", fTransmitted ? 1 : 0);
		content.put("raceid", lRaceId);
		
		db.beginTransaction();
		try
		{
			m_iLapId = (int)db.insertOrThrow("laps", null, content);
			if(m_iLapId != -1)
			{
				SQLiteStatement pSQL = db.compileStatement("insert into points (x,y,time,velocity,lapid) values (?,?,?,?,?)");
				for(int x = 0;x < m_lstPositions.size(); x++)
				{
					TimePoint2D t = m_lstPositions.get(x);
					t.SendToDB(pSQL,m_iLapId);
				}
				pSQL.close();
				for(int x = 0;x < this.m_lstChannels.size(); x++)
				{
					DataChannel d = m_lstChannels.get(x);
					d.SendToDB(db, m_iLapId);
				}
			}
			db.setTransactionSuccessful();
		}
		catch(Exception e)
		{
			fRet = false;
		}
		finally
		{
			db.endTransaction();
		}
		return fRet;
	}
	public List<LineSeg> GetSplitPoints(boolean fStartOnly)
	{
		if(fStartOnly)
		{
			List<LineSeg> lstLines = new ArrayList<LineSeg>();
			lstLines.add(m_lstLines.get(m_lstLines.size()-1));
			return lstLines;
		}
		else
		{
			return this.m_lstLines;
		}
	}
	public void AddPosition(Point2D ptNewRaw, int iTime, double dVelocity)
	{
		if(m_lstPositions.size() == 0)
		{
			assert(m_fDeferredLoad);
			Init(m_lstLines, m_lstLineDirections, ptNewRaw, m_iUnixStartTime, m_iLapId, iTime, dVelocity);
			return; // we're done here.
		}
		TimePoint2D ptLast = m_lstPositions.get(m_lstPositions.size() - 1);
		TimePoint2D ptNew = new TimePoint2D(ptNewRaw,iTime, dVelocity, m_dCumulativeDistance);
		
		assert(ptNew != null && ptLast.pt != null);
		LineSeg lnLast = new LineSeg(ptNewRaw, ptLast.pt);
		
		m_dCumulativeDistance += lnLast.GetLength(); // add how far we've travelled here
		
		m_lstPositions.add(ptNew);
		
		if(!m_fBoundsValid)
		{
			m_rcSDBounds = new FloatRect((float)ptNew.dDistance,(float)ptNew.dVelocity,(float)ptNew.dDistance,(float)ptNew.dVelocity);
			m_rcMapBounds = new FloatRect(ptNew.pt.x,ptNew.pt.y,ptNew.pt.x,ptNew.pt.y);
			m_fBoundsValid = true;
		}
		else
		{
			m_rcSDBounds.left = (float)Math.min(m_rcSDBounds.left,ptNew.dDistance);
			m_rcSDBounds.top = (float)Math.min(m_rcSDBounds.top,ptNew.dVelocity);
			m_rcSDBounds.right = (float)Math.max(m_rcSDBounds.right,ptNew.dDistance);
			m_rcSDBounds.bottom = (float)Math.max(m_rcSDBounds.bottom,ptNew.dVelocity);
			
			m_rcMapBounds.left = Math.min(m_rcMapBounds.left,ptNew.pt.x);
			m_rcMapBounds.top = Math.min(m_rcMapBounds.top,ptNew.pt.y);
			m_rcMapBounds.right = Math.max(m_rcMapBounds.right,ptNew.pt.x);
			m_rcMapBounds.bottom = Math.max(m_rcMapBounds.bottom,ptNew.pt.y);
		}
		
		if(m_lstPositions.size() >= 3)
		{
			// it is now possible that we'll have a triangle, so we need to check to see if the last line segment
			// overlaps the start/finish line segment
			
			float dLastInSeconds = ((float)ptLast.iTime / 1000.0f);
			
			final int cLines = m_lstLines.size(); 
			LineSeg.IntersectData intersect = new LineSeg.IntersectData();
			for(int ixLine = 0; ixLine < cLines; ixLine++)
			{
				if(ixLine == m_lstLines.size() - 1 && iTime - this.m_iStartTime < DISARM_TIME)
				{
					continue; // don't even try to intersect with the finish line if we're not more than 3 seconds into a lap
				}
				if(lnLast.Intersect(m_lstLines.get(ixLine),intersect, true, true))
				{
					Vector2D vDir = Vector2D.P1MinusP2(ptNewRaw, ptLast.pt);
					if(vDir.DotProduct(m_lstLineDirections.get(ixLine)) > 0)
					{
						// we've crossed a line, so let's add a time to the report
						float dTimeInSeconds = ((float)iTime / 1000.0f);
						float dIntersectTime = (intersect.GetThisFraction() * dLastInSeconds) + ((1 - intersect.GetThisFraction()) * dTimeInSeconds);
						int iIntersectTime = (int)(dIntersectTime * 1000);
						assert(iIntersectTime > m_iStartTime);
						while(m_lstCrossTimes.size() <= ixLine) m_lstCrossTimes.add(null);
						m_lstCrossTimes.set(ixLine, new Integer(iIntersectTime));
						if(IsDoneLap())
						{
							m_ptFinishPoint = intersect.GetPoint();
						}
					}
				}
			}
		}
		
		//VerifyPositions();
	}
	
	public Point2D GetFinishPoint()
	{
		assert(m_ptFinishPoint != null); // don't ask for this when the lap isn't done!
		return m_ptFinishPoint;
	}
	public boolean IsDoneLap()
	{
		return m_lstCrossTimes != null && m_lstCrossTimes.size() == m_lstLines.size();
	}
	public int GetLastCrossTime()
	{
		if(m_lstCrossTimes.size() > 0)
		{
			return m_lstCrossTimes.get(m_lstCrossTimes.size() - 1).intValue();
		}
		else
		{
			assert(false); // you shouldn't be asking about this!
			return 0;
		}
	}
	public double GetLapTime()
	{
		if(m_lstCrossTimes.size() == m_lstLines.size())
		{
			return (GetLastCrossTime() - m_iStartTime)/1000.0;
		}
		return 0.0;
	}
	
	// the input here is in milliseconds since the app start
	public float GetTimeSinceLastSplit(int iCurrentTime)
	{
		if(m_lstCrossTimes.size() == 0)
		{
			// we haven't hit a split yet, since the time since last split will
			// be the time since we were created
			return (iCurrentTime - m_iStartTime) / 1000.0f;
		}
		else
		{
			// m_lstCrossTimes are in absolute time
			int ixLast = m_lstCrossTimes.size() - 1;
			return (iCurrentTime - m_lstCrossTimes.get(ixLast).intValue()) / 1000.0f;
		}
	}
	private int FindPointIndexClosestToTime(final int iCrossTime)
	{
		TimePoint2D ptBest = null;
		int ixBest = -1;
		final int cSize = m_lstPositions.size();
		for(int ixCheck = 0; ixCheck < cSize; ixCheck++)
		{
			TimePoint2D pt = m_lstPositions.get(ixCheck);
			if(pt.iTime > iCrossTime)
			{
				// our search is done
				break;
			}
			else
			{
				if(ptBest == null || pt.iTime > ptBest.iTime)
				{
					ptBest = pt;
					ixBest = ixCheck;
				}
			}
		}
		return ixBest;
	}
	private int GetSplitStartIndex(final int ixSplit)
	{
		if(ixSplit < 0) return 0;
		if(ixSplit == 0) return 0; // the first split always starts at index zero
		if(ixSplit > m_lstCrossTimes.size()) return m_lstPositions.size();
		Integer iCrossTime = m_lstCrossTimes.get(ixSplit-1);
		if(iCrossTime == null) return 0; // we haven't made it to this split yet...
		// now let's do a binary search to find the point closest to but less than the cross time
		
		return FindPointIndexClosestToTime(iCrossTime.intValue());
	}
	private int GetSplitEndIndex(final int ixSplit)
	{
		if(ixSplit < 0) return m_lstPositions.size();
		if(ixSplit >= m_lstCrossTimes.size()) return m_lstPositions.size();
		
		Integer iCrossTime = m_lstCrossTimes.get(ixSplit);
		if(iCrossTime == null) return m_lstPositions.size(); // we haven't made it to this split yet...
		// now let's do a binary search to find the point closest to but less than the cross time
	
		return FindPointIndexClosestToTime(iCrossTime.intValue());
	}
	public FloatRect GetBounds(final boolean fSpeedDistance)
	{
		if(m_fBoundsValid)
		{
			return fSpeedDistance ? m_rcSDBounds : m_rcMapBounds;
		}
		else
		{
			return new FloatRect(0.0f,0.0f,0.0f,0.0f);
		}
	}
	private static void DrawMapStyle(LapAccumulator lap, List<LineSeg> lstSF, FloatRect rcInWorld, Canvas canvas, Paint paintLap, Paint paintSplits, final Rect rcOnScreen)
	{
		canvas.save();
		
		Point2D ptTopLeft = new Point2D(rcInWorld.left,rcInWorld.top);
		Point2D ptTopRight = new Point2D(rcInWorld.right, rcInWorld.top);
		Point2D ptBotRight = new Point2D(rcInWorld.right, rcInWorld.bottom);
		
		
		final double dWidth = rcInWorld.right - rcInWorld.left;
		final double dHeight = rcInWorld.bottom - rcInWorld.top;
		
		int iTargetWidth = rcOnScreen.right - rcOnScreen.left;
		int iTargetHeight = rcOnScreen.bottom - rcOnScreen.top;
		
		if(dHeight == 0) return; // nothing to do if the world is 0-width
		
		{ // calculating aspect ratios and adjusting target locations
			float flTemp[] = new float[1];
			Location.distanceBetween(ptTopLeft.y, ptTopLeft.x, ptTopRight.y, ptTopRight.x, flTemp);
			final double dActualWidth = flTemp[0];
			Location.distanceBetween(ptBotRight.y, ptBotRight.x, ptTopRight.y, ptTopRight.x, flTemp);
			final double dActualHeight = flTemp[0];
			
			double dAspectWorld = dActualWidth / dActualHeight;
			double dAspectWindow = (double)iTargetWidth / (double)iTargetHeight;
			if(dAspectWorld > dAspectWindow)
			{
				int iOldHeight = iTargetHeight;
				// track is wider than the window, so the height of the target needs to shrink
				iTargetHeight = (int)(iTargetHeight / dAspectWorld);
				rcOnScreen.bottom = rcOnScreen.top + iTargetHeight;
				rcOnScreen.top += (iOldHeight-iTargetHeight)/2;
				rcOnScreen.bottom += (iOldHeight-iTargetHeight)/2;
			}
			else
			{
				// track is taller than the window, so the width of the target needs to shrink
				int iOldWidth = iTargetWidth;
				iTargetWidth = (int)(iTargetHeight * dAspectWorld);
				rcOnScreen.right = rcOnScreen.left + iTargetWidth;
				rcOnScreen.left += (iOldWidth-iTargetWidth)/2;
				rcOnScreen.right += (iOldWidth-iTargetWidth)/2;
			}
		}
		if(dWidth > 0 && dHeight > 0 && iTargetWidth > 0 && iTargetHeight > 0)
		{
			TimePoint2D ptLast = null;
			float rgflPoints[] = new float[4 * (lap.m_lstPositions.size() - 1)];
			
			int cSkipRate = 1;
			
			for(int ix = 0; ix < lap.m_lstPositions.size(); ix+=cSkipRate)
			{
				TimePoint2D pt = lap.m_lstPositions.get(ix);
				if(ptLast != null)
				{
					float x1 = (float)((pt.pt.GetX() - rcInWorld.left) / dWidth) * iTargetWidth;
					float y1 = (float)((pt.pt.GetY() - rcInWorld.top) / dHeight) * iTargetHeight;
					float x2 = (float)((ptLast.pt.GetX() - rcInWorld.left) / dWidth) * iTargetWidth;
					float y2 = (float)((ptLast.pt.GetY() - rcInWorld.top) / dHeight) * iTargetHeight;
					x1 += rcOnScreen.left;
					y1 += rcOnScreen.top;
					x2 += rcOnScreen.left;
					y2 += rcOnScreen.top;
					
					//x1 = (rcOnScreen.right - x1) + rcOnScreen.left;
					//x2 = (rcOnScreen.right - x2) + rcOnScreen.left;
					y1 = (rcOnScreen.bottom - y1) + rcOnScreen.top;
					y2 = (rcOnScreen.bottom - y2) + rcOnScreen.top;
					rgflPoints[4*(ix-1)] = x1;
					rgflPoints[4*(ix-1) + 1] = y1;
					rgflPoints[4*(ix-1) + 2] = x2;
					rgflPoints[4*(ix-1) + 3] = y2;
					
				}
				ptLast = pt;
			}
			canvas.drawLines(rgflPoints, paintLap);
			final int ixX = rgflPoints.length - 2;
			final int ixY = rgflPoints.length - 1;
			canvas.drawRect(rgflPoints[ixX]-5,rgflPoints[ixY]-5, rgflPoints[ixX]+5,rgflPoints[ixY]+5, paintLap);

			if(lstSF != null && paintSplits != null)
			{
				for(int ix = 0; ix < lstSF.size(); ix++)
				{
					LineSeg ln = lstSF.get(ix);
					float x1 = (float)((ln.GetP1().GetX() - rcInWorld.left) / dWidth) * iTargetWidth;
					float x2 = (float)((ln.GetP2().GetX() - rcInWorld.left) / dWidth) * iTargetWidth;
					float y1 = (float)((ln.GetP1().GetY() - rcInWorld.top) / dHeight) * iTargetHeight;
					float y2 = (float)((ln.GetP2().GetY() - rcInWorld.top) / dHeight) * iTargetHeight;
					x1 += rcOnScreen.left;
					x2 += rcOnScreen.left;
					y1 += rcOnScreen.top;
					y2 += rcOnScreen.top;
					y1 = (rcOnScreen.bottom - y1) + rcOnScreen.top;
					y2 = (rcOnScreen.bottom - y2) + rcOnScreen.top;
					
					//x1 = rcOnScreen.right - x1 + rcOnScreen.left;
					//x2 = rcOnScreen.right - x2 + rcOnScreen.left;
					
					canvas.drawLine(x1, y1, x2, y2, paintSplits);
				}
			}
		}
		canvas.restore();
	}
	private static void DrawSpeedDistance(LapAccumulator lap, List<LineSeg> lstSF, FloatRect rcInWorld, Canvas canvas, Paint paintLap, Paint paintSplits, final Rect rcOnScreen)
	{
		// rcInWorld will be:
		// left = distance in whatever units to start drawing
		// right = distance in whatever units to stop drawing
		// top = ignored
		// bottom = ignored
		
		// we will draw the graph as:
		// distance on x axis
		// speed on y axis
		
		TimePoint2D ptLast = null;
		float rgflPoints[] = new float[4 * (lap.m_lstPositions.size() - 1)];
		int ixPoint = 0;
		
		final float dDistanceSpan = (float)(rcInWorld.right - rcInWorld.left); // how "wide" the timespan to draw is
		final float dSpeedSpan = rcInWorld.bottom - rcInWorld.top;
		final int iTargetWidth = rcOnScreen.right - rcOnScreen.left;
		final int iTargetHeight = rcOnScreen.bottom - rcOnScreen.top;
		
		final int cSize = lap.m_lstPositions.size();
		for(int ix = 0; ix < cSize; ix+=1)
		{
			TimePoint2D pt = lap.m_lstPositions.get(ix);
			if(ptLast != null)
			{
				if(pt.dDistance >= rcInWorld.left && pt.dDistance <= rcInWorld.right)
				{
					float x1 = (float)((pt.dDistance - rcInWorld.left) / dDistanceSpan) * iTargetWidth;
					float y1 = (float)((pt.dVelocity - rcInWorld.top) / dSpeedSpan) * iTargetHeight;
					float x2 = (float)((ptLast.dDistance - rcInWorld.left) / dDistanceSpan) * iTargetWidth;
					float y2 = (float)((ptLast.dVelocity - rcInWorld.top) / dSpeedSpan) * iTargetHeight;
					
					y1 = iTargetHeight - y1;
					y2 = iTargetHeight - y2;
					
					x1 += rcOnScreen.left;
					x2 += rcOnScreen.left;
					y1 += rcOnScreen.top;
					y2 += rcOnScreen.top;

					ixPoint++;
					final int ixBase = 4*(ixPoint-1);
					rgflPoints[ixBase] = x1;
					rgflPoints[ixBase + 1] = y1;
					rgflPoints[ixBase + 2] = x2;
					rgflPoints[ixBase + 3] = y2;
				}
			}
			ptLast = pt;
		}

		canvas.drawLines(rgflPoints, 0, ixPoint*4, paintLap);
		final int ixX = ixPoint*4-2;
		final int ixY = ixPoint*4-1;
		if(ixX >= 0 && ixX < rgflPoints.length && ixY >= 0 && ixY < rgflPoints.length)
		{
			canvas.drawRect(rgflPoints[ixX]-3,rgflPoints[ixY]-3, rgflPoints[ixX]+3,rgflPoints[ixY]+3, paintLap);
		}
	}
	public static void DrawLap(LapAccumulator lap, boolean fSpeedDist, List<LineSeg> lstSF, final FloatRect _rcInWorld, Canvas canvas, Paint paintLap, Paint paintSplits, final Rect _rcOnScreen)
	{
		//canvas.clipRect(rcOnScreen,Region.Op.REPLACE);
	
		FloatRect rcInWorld = new FloatRect(_rcInWorld);
		Rect rcOnScreen = new Rect(_rcOnScreen);
		
		if(fSpeedDist)
		{
			DrawSpeedDistance(lap,lstSF,rcInWorld,canvas,paintLap,paintSplits, rcOnScreen);
		}
		else
		{
			DrawMapStyle(lap,lstSF,rcInWorld,canvas,paintLap,paintSplits,rcOnScreen);
		}
		
	}
	public int GetLastSplitIndex()
	{
		return m_lstCrossTimes.size() - 1;
	}
	public double GetSplit(int ixIndex)
	{
		if(ixIndex == 0)
		{
			if(m_lstCrossTimes.get(0) != null)
			{
				return (m_lstCrossTimes.get(0).intValue() - m_iStartTime)/1000.0;
			}
		}
		else if(ixIndex > 0)
		{
			if(m_lstCrossTimes.get(ixIndex) != null && m_lstCrossTimes.get(ixIndex-1) != null)
			{
				return (m_lstCrossTimes.get(ixIndex).intValue() - m_lstCrossTimes.get(ixIndex-1).intValue())/1000.0;
			}
		}
		return 0.0;
	}
	private TimePoint2D GetInterpolatedPointAtPosition(final TimePoint2D p, double dPercentage)
	{
		int ixBestIndex = -1;
		final int cSize = m_lstPositions.size();
		{
			double dClosest = 1e30;
			TimePoint2D ptNext = m_lstPositions.get(1%cSize);
			TimePoint2D ptThis = m_lstPositions.get(0);
			for(int x = 0; x < cSize-1; x++)
			{
				final int ixNext = ((x+1)%cSize);
			
				ptThis = ptNext;
				ptNext = m_lstPositions.get(ixNext);
				
				final float dx1 = ptThis.pt.x - p.pt.x;
				final float dx2 = ptNext.pt.x - p.pt.x;
				final float dy1 = ptThis.pt.y - p.pt.y;
				final float dy2 = ptNext.pt.y - p.pt.y;
				final float d1 = dx1*dx1 + dy1*dy1;
				final float d2 = dx2*dx2 + dy2*dy2;
				final float dAvg = (d1+d2)/2.0f;
				final float dPct = (float)x / (float)cSize;
				final float dPctDiff = (float)Math.abs(dPct - dPercentage);
			    if((ixBestIndex == -1 || dAvg < dClosest) && dPctDiff < 0.5)
			    {
			      dClosest = dAvg;
			      ixBestIndex = x;
			    }
			}
		}
		
		if(ixBestIndex >= 0)
		{
			TimePoint2D pt1 = m_lstPositions.get(ixBestIndex);
			TimePoint2D pt2 = null;
			int ixNext = ixBestIndex+1;
			
			// it turns that when moving slowly, the 10hz GPS will actually return the exact same point twice in a row, so we need to check for that.
			do
			{
				pt2 = m_lstPositions.get(ixNext % cSize);
				ixNext++;
			} while(pt1.EqualsPoint(pt2) && (ixNext % cSize) != ixBestIndex);
				
			
			if(pt1.EqualsPoint(pt2))
			{
				return new TimePoint2D(pt1.GetPT(), pt1.iTime, pt1.dVelocity, pt1.dDistance);
			}
			else
			{
		        // hooray, we found the closest two points.
		        Vector2D vD = Vector2D.P1MinusP2(pt2.pt,pt1.pt);
		        LineSeg lnD = new LineSeg(pt1.pt, vD); // gets a vector going from pt1 to pt2
		        
		        LineSeg lnPerp = new LineSeg(p.pt,vD.RotateAboutOrigin(Math.PI/2)); // gets a vector running perpindicular to pt1->pt2 through the queried point
		        if(lnPerp.GetLength() <= 0) return pt1;
		        
		        LineSeg.IntersectData pIntersect = new LineSeg.IntersectData();
		        
		        boolean fIntersect = lnD.Intersect(lnPerp, pIntersect, false, false);
		        
		        if(fIntersect)
		        {
		          // hooray, they intersect
		          final double dPercent = pIntersect.GetThisFraction();
		          if(dPercent == Float.NaN || pIntersect.GetOtherFraction() == Float.NaN)
		          {
		        	  return pt1;
		          }
		          else
		          {
			          double dThisTime = (pt1.iTime * (1-dPercent)) + (pt2.iTime * dPercent);
			          double dThisVelocity = (pt1.dVelocity * (1-dPercent)) + (pt2.dVelocity * dPercent);
			          double dThisDistance = (pt1.dDistance * (1-dPercent)) + (pt2.dDistance * dPercent);
			          TimePoint2D ptRet = new TimePoint2D(pIntersect.GetPoint(), (int)dThisTime, dThisVelocity, dThisDistance);
			          return ptRet;
		          }
		        }
			}
		}
		return null;
	}
	public double GetSpeedAtPosition(final TimePoint2D p)
	{
		TimePoint2D ptInterpolate = GetInterpolatedPointAtPosition(p, 0.5);
		if(ptInterpolate != null)
		{
			return ptInterpolate.dVelocity;
		}
		return 0;
	}
	public double GetTimeAtPosition(final TimePoint2D p, double dInputPercentage)
	{
		TimePoint2D ptInterpolate = GetInterpolatedPointAtPosition(p, 0.5);
		if(ptInterpolate != null)
		{
			return ptInterpolate.iTime - m_lstPositions.get(0).iTime;
		}
		return 0;
	}
	public TimePoint2D GetLastPoint()
	{
		if(this.m_lstPositions.size() > 0)
		{
			return m_lstPositions.get(m_lstPositions.size()-1);
		}
		return null;
	}
	public double GetLastSplit()
	{
		return GetSplit(GetLastSplitIndex());
	}
	public static class DataChannel
	{
		public static final int CHANNEL_ACCEL_X = 5; // see LapData.h in PitsideConsole for constants
		public static final int CHANNEL_ACCEL_Y = 6; // see LapData.h in PitsideConsole for constants
		public static final int CHANNEL_TEMP = 7;
		public static final int CHANNEL_SIGSTRENGTH = 8;
		public static final int CHANNEL_PID_START = 0x100;
		public static final int CHANNEL_PID_END = 0x200;
		public static final int CHANNEL_IOIO_START = 0x200;
		public static final int CHANNEL_IOIO_END = 0x300;
		
		// or the user can choose a custom IOIO type
		public static final int CHANNEL_IOIOCUSTOM_START = 0x300;
		public static final int CHANNEL_IOIO_FUEL = 0x301;
		public static final int CHANNEL_IOIO_RPM = 0x302;
		public static final int CHANNEL_LF_WS = 0x303; // left-front wheelspeed
		public static final int CHANNEL_RF_WS = 0x304;
		public static final int CHANNEL_LR_WS = 0x305;
		public static final int CHANNEL_RR_WS = 0x306;
		public static final int CHANNEL_EXHAUST_TEMP = 0x307;
		public static final int CHANNEL_LF_TIRE_TEMP = 0x308;
		public static final int CHANNEL_RF_TIRE_TEMP = 0x309;
		public static final int CHANNEL_LR_TIRE_TEMP = 0x310;
		public static final int CHANNEL_RR_TIRE_TEMP = 0x311;
		public static final int CHANNEL_LF_BRAKE_TEMP = 0x312;
		public static final int CHANNEL_RF_BRAKE_TEMP = 0x313;
		public static final int CHANNEL_LR_BRAKE_TEMP = 0x314;
		public static final int CHANNEL_RR_BRAKE_TEMP = 0x315;
		
		public int iType;
		private List<Integer> lstTimes; // time in milliseconds for each point of data
		private List<Float> lstData;
		private LapAccumulator pParent;
		private boolean fCleared;
		public DataChannel(int iType, LapAccumulator pParent)
		{
			fCleared = false;
			pParent.AddDataChannel(this);
			this.iType = iType;
			this.pParent = pParent;
			
			lstTimes = new ArrayList<Integer>();
			lstData = new ArrayList<Float>();
		}
		public boolean IsParent(LapAccumulator lap)
		{
			return lap == pParent;
		}
		public synchronized void AddData(float flValue, int iTimeMs)
		{
			if(fCleared) return;
			
			lstData.add(flValue);
			lstTimes.add(iTimeMs);
		}
		public int GetPointCount() {return lstTimes.size();}
		public Integer GetTime(int ix) {return lstTimes.get(ix);}
		public Float GetData(int ix) {return lstData.get(ix);}
		
		public synchronized void Clear()
		{
			lstTimes.clear();
			lstTimes = null;
			lstData.clear();
			lstData = null;
			pParent = null;
			fCleared = true;
		}
		public synchronized boolean SendToDB(SQLiteDatabase db, int iLapId)
		{
			if(fCleared) return false;
			if(lstTimes.size() != lstData.size()) return false;
			
			try
			{
				ContentValues content = new ContentValues();
				content.put("lapid",iLapId);
				content.put("channeltype", iType);
				// 1) insert lap->data mapping in channels table
				// 2) insert all the data in data table (mapped to the entry in the channels table)
				long iChannelId = db.insertOrThrow("channels", null, content);
				
				SQLiteStatement pSQL = db.compileStatement("insert into data (time,value,channelid) values (?,?,?)");
				for(int x = 0;x < lstData.size(); x++)
				{
					Integer i = lstTimes.get(x);
					Float f = lstData.get(x);
					pSQL.clearBindings();
					pSQL.bindLong(1, i.longValue());
					pSQL.bindDouble(2, f.doubleValue());
					pSQL.bindLong(3, iChannelId);
					pSQL.executeInsert();
				}
				pSQL.close();
				
				
				return true;
			}
			catch(Exception e)
			{
				return false;
			}
			
		}
		private List CopyList(List lst)
		{
			List output = new ArrayList();
			Iterator i = lst.iterator();
			while(i.hasNext())
			{
				output.add(i.next());
			}
			return output;
		}
		public boolean SendToOutput(DataOutputStream out)
		{
			if(fCleared) return false;
			List<Float> lstLocalData = null;
			List<Integer> lstLocalTimes = null;
			int iLocalType = 0;
			int iLapId = 0;
			synchronized(this)
			{
				if(lstData.size() != lstTimes.size()) return false;
				lstLocalData = (List<Float>)CopyList(lstData);
				lstLocalTimes = (List<Integer>)CopyList(lstTimes);
				iLapId = pParent.m_iLapId;
				iLocalType = iType;
			}
			
			try
			{
				// first, write "datachaz" (it used to be "datachan", but we added the "chaz" to indicate this is a zipped one)
				out.writeByte('d');
				out.writeByte('a');
				out.writeByte('t');
				out.writeByte('a');
				out.writeByte('c');
				out.writeByte('h');
				out.writeByte('a');
				out.writeByte('n');
				
				// write all our crap to a byte array, then write the byte array to the DeflaterOutputStream
				
				out.writeInt(iLapId);
				out.writeInt(iLocalType);
				out.writeInt(lstLocalData.size());
				for(int x = 0; x < lstLocalData.size(); x++)
				{
					out.writeFloat(lstLocalData.get(x).floatValue());
					out.writeInt(lstLocalTimes.get(x).intValue());
				}
				
				out.writeByte('d');
				out.writeByte('o');
				out.writeByte('n');
				out.writeByte('e');
				out.writeByte('d');
				out.writeByte('a');
				out.writeByte('t');
				out.writeByte('a');
				return true;
			}
			catch(Exception e)
			{
				return false;
			}
		}
		
	} // end DataChannel class
	
	public void AddDataChannel(DataChannel data)
	{
		m_lstChannels.add(data);
	}
	private int					m_iUnixStartTime; // unix time (seconds since 1970) that this lap started
	private int 				m_iLapId; // the DB id for this lap
	private ArrayList<TimePoint2D> m_lstPositions; // vector of the positions we've seen this lap
	private ArrayList<DataChannel> m_lstChannels; // vector of data channels associated with this lap
	
	private int  				m_iStartTime; // when this lap started (in milliseconds since the first location ever was received by the app)
	private static final int 	DISARM_TIME = 3000; // how long we wait before allowing a lap to actually complete.  This so that jittery 10hz GPSes don't cross the finish line several times a second if you're going slow
	private double				m_dCumulativeDistance; // how long this lap has taken
	private boolean				m_fDeferredLoad; // whether we're deferring the loading of points from the DB
	private boolean				m_fPruned; // whether this is a pruned lap - a pruned lap never re-loads from the DB, as it is too old for people to care.
	
	// an array of bytes all ready to be sent over the network.
	// if this doesn't exist pre-network, then call "BuildByteArray()".
	// this should be built as soon as the lap is completed, so that
	// everything is ready to go for the network send.
	private byte				m_pPrecachedToNetwork[]; 
	
	Point2D					m_ptFinishPoint;
	private List<LineSeg> m_lstLines;
	private List<Vector2D> m_lstLineDirections;
	private List<Integer> m_lstCrossTimes; // times we crossed various splits, given in milliseconds since the first location was received
	
	private boolean m_fBoundsValid;
	private FloatRect m_rcSDBounds; // cached bounds of the speed/distance graph
	private FloatRect m_rcMapBounds; // cached bounds of the map
}
class TimePoint2D 
{
	public TimePoint2D(Point2D pt, int iTime, double dVelocity, double dDistance)
	{
		assert(pt != null);
		this.pt = pt;
		this.iTime = iTime;
		this.dVelocity = dVelocity;
		this.dDistance = dDistance;
		
	}
	public boolean EqualsPoint(TimePoint2D pt)
	{
		return this.pt.x == pt.pt.x && this.pt.y == pt.pt.y;
	}
	Point2D GetPT() {return pt;}
	double GetTime() {return iTime;}
	double GetVel() {return dVelocity;}
	double GetDistance() {return dDistance;}
	
	public boolean SendToOutput(DataOutputStream os)
	{
		try
		{
			os.writeFloat((float)pt.x);
			os.writeFloat((float)pt.y);
			os.writeInt(iTime);
			os.writeFloat((float)dVelocity);
			float flSum = (pt.x + pt.y);
			os.writeFloat((float)(flSum));
			return true;
		}
		catch(Exception e)
		{
			return false;
		}
	}
	public void SendToDB(SQLiteStatement pSQL, long lLapId) throws SQLiteException
	{
		pSQL.clearBindings();
		pSQL.bindDouble(1, pt.GetX());
		pSQL.bindDouble(2, pt.GetY());
		pSQL.bindLong(3, iTime);
		pSQL.bindDouble(4, dVelocity);
		pSQL.bindLong(5, lLapId);
		pSQL.executeInsert();
	}
	
	public Point2D pt;
	public int iTime;
	public double dVelocity;
	public double dDistance;
}
