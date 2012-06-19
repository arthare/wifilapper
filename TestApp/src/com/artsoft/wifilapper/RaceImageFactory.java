package com.artsoft.wifilapper;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.graphics.Bitmap;
import android.os.Handler;

public class RaceImageFactory 
{
	private Map<Long,Bitmap> m_map;
	private Handler m_handler; // used to report to our owner when things happen (new images, etc)
	private RaceLoader m_thd;
	private int m_ixMessage; // what message we should send to the handler when new images are available
	public RaceImageFactory(Handler handler, int ixMessage)
	{
		m_thd = new RaceLoader();
		m_thd.start();
		m_map = new HashMap<Long,Bitmap>();
		m_handler = handler;
		m_ixMessage = ixMessage;
	}
	public synchronized void Shutdown()
	{
		if(m_thd != null)
		{
			m_thd.Shutdown();
			m_thd.interrupt();
			m_thd = null;
		}
		m_map = null;
		m_handler = null;
		
	}
	private synchronized void SetImage(long lRaceId, Bitmap bmp)
	{
		if(m_map != null)
		{
			m_map.put(new Long(lRaceId), bmp);
			m_handler.sendEmptyMessage(m_ixMessage);
		}
	}
	public Bitmap GetImage(long lRaceId, boolean fRequestOnWorker)
	{
		synchronized(this)
		{
			if(m_map != null && m_map.containsKey(new Long(lRaceId)))
			{
				return m_map.get(new Long(lRaceId));
			}
		}
		// if we got here, we don't currently have this race's image cached.  Return null for now and fire an update request
		if(!fRequestOnWorker)
		{
			ThdLoadImage(lRaceId);
		}
		return null;
	}
	
	private void ThdLoadImage(long lRaceId)
	{
		RaceLoader thd = null;
		synchronized(this)
		{
			thd = m_thd;
		}
		
		if(thd != null)
		{
			thd.AddTarget(lRaceId);
		}
	}
	
	private class RaceLoader extends Thread implements Runnable
	{
		private List<Long> lstRequests;
		private boolean m_fContinue;
		public RaceLoader()
		{
			lstRequests = new ArrayList<Long>();
			m_fContinue = true;
		}
		public synchronized void AddTarget(long lRaceId)
		{
			lstRequests.add(new Long(lRaceId));
			notify();
		}
		public synchronized void Shutdown()
		{
			m_fContinue = false;
		}
		public void run()
		{
			Thread.currentThread().setName("Race image thread");
			while(m_fContinue)
			{
				// check for requests
				try 
				{
					Long lRaceId = null;
					synchronized(this)
					{
						wait();
					}
					
					while(m_fContinue)
					{
						// we must have a request.
						synchronized(this)
						{
							if(lstRequests.size() > 0)
							{
								lRaceId = lstRequests.get(0);
								lstRequests.remove(0);
							}
							else
							{
								break;
							}
							if(RaceImageFactory.this.GetImage(lRaceId.longValue(), true) != null)
							{
								continue; // we already have this image, no need to re-render it.  go to the next
							}
						}
						if(lRaceId != null)
						{
							Bitmap bmp = RaceDatabase.GetRaceOutlineImage(RaceDatabase.Get(), lRaceId, 100, 100);
							RaceImageFactory.this.SetImage(lRaceId.longValue(), bmp);
						}
					}
				} 
				catch (InterruptedException e) 
				{
					// we're done
					break;
				}
			}
		}
	}
}
