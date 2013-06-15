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

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.List;
import java.util.Vector;

import com.artsoft.wifilapper.RaceDatabase.RaceData;
import com.artsoft.wifilapper.Utility.MultiStateObject.STATE;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.location.LocationManager;
import android.net.DhcpInfo;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Debug;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.Toast;

public class LapSender 
{
	// this is the outgiong queue of laps that haven't been sent yet
	private Vector<LapAccumulator> lstLapsToSend; 

	private long m_lRaceId;
	
	private static final int PRUNE_MINUTES = 30; // how many minutes we allow things to languish before busting out the prunes
	
	private boolean fContinue;
	private SendThd m_thd;
	private boolean m_fDBDead = false;

	public interface LapSenderListener
	{
		public enum CONNLEVEL {SEARCHING,CONNECTED,FULLYCONNECTED};
		public abstract void SetConnectionLevel(CONNLEVEL eLevel);
	}
	
	public LapSender(Utility.MultiStateObject pStateMan,LapSenderListener listener, WifiManager pWifi, String strIP, String strSSID, boolean fRequireWifi)
	{
		lstLapsToSend = new Vector<LapAccumulator>();
		fContinue = true;
		m_thd = new SendThd(pStateMan, listener, pWifi, strIP, strSSID, fRequireWifi);
		m_thd.start();
	}
	public int GetLapSentCount()
	{
		if(m_thd != null)
		{
			return m_thd.GetLapSentCount();
		}
		return 0;
	}
	public void SetRaceId(long lRaceId)
	{
		m_lRaceId = lRaceId;
	}
	public void Shutdown()
	{
		synchronized(this)
		{
			if(lstLapsToSend != null)
			{
				lstLapsToSend.clear();
				lstLapsToSend = null;
			}
			fContinue = false;
		}
	}
	public void SendLap(LapAccumulator lap)
	{
		assert(m_lRaceId != -1);
		synchronized(this)
		{
			if(lstLapsToSend != null)
			{
				lstLapsToSend.add(lap);
			}
		}
	}
	private synchronized void SaveLapsToDB()
	{
		if(lstLapsToSend == null) return;
		if(m_lRaceId != -1)
		{
			synchronized(RaceDatabase.class)
			{
				for(int x = 0;x < lstLapsToSend.size(); x++)
				{
					try
					{
						if(!m_fDBDead)
						{
							lstLapsToSend.get(x).SendToDB(RaceDatabase.Get(), m_lRaceId, false);
						}
					}
					catch(SQLiteException e)
					{
						m_fDBDead = true;
					}
					if(!lstLapsToSend.get(x).IsPruned())
					{
						// if something is pruned, we're never going to send it until a reboot.  The data is there, but we're not sending it...
						lstLapsToSend.get(x).PrepareForNetwork(); // makes sure all our laps are precached
					}
				}
			}
		}
	}
	public synchronized void SetIP(String strWifiIP)
	{
		if(m_thd != null)
		{
			m_thd.SetIP(strWifiIP);
		}
	}
	public synchronized void SetSSID(String strWifiSSID)
	{
		if(m_thd != null)
		{
			m_thd.SetSSID(strWifiSSID);
		}
	}
	
	
	private synchronized void PruneLaps()
	{
		if(m_lRaceId != -1)
		{
			if(lstLapsToSend != null)
			{
				for(int x = 0;x < lstLapsToSend.size(); x++)
				{
					LapAccumulator lap = lstLapsToSend.get(x);
					long lSeconds = lap.GetAgeInSeconds();
					if(lSeconds > PRUNE_MINUTES * 60 && lap.IsInDB() && lap.IsDoneLap() && !lap.IsPruned())
					{
						lap.Prune();
					}
				}
			}
		}
	}
	
	public static void SendDBToPitside(final int raceId, final String strIP, final Handler handler, final int msgSuccess, final int msgError)
	{
		new Thread(new Runnable() {
			public void run()
			{
				SQLiteDatabase db = RaceDatabase.Get();
				db.beginTransaction();
				try
				{
					InetAddress addr = InetAddress.getByName(strIP);
					SocketAddress sConnect = new InetSocketAddress(addr, 63939);

					Socket s = new Socket();
					s.connect(sConnect, 500);
					s.setSoTimeout(3000);
					
					String strDBPath = db.getPath();
					InputStream in = new FileInputStream(strDBPath);
					OutputStream out = s.getOutputStream();
					byte rgBuf[] = new byte[1044480];
					
					out.write(new String("racingdbincoming").getBytes());
					
					while(in.available() > 0)
					{
						int cb = in.read(rgBuf,0,rgBuf.length);
						out.write(rgBuf,0,cb);
					}
					out.write(new String("racingdbcomplete").getBytes());
					out.flush();
					out.close();
					s.close();
					
					
					Message m = Message.obtain(handler, msgSuccess, "Sent database to " + strIP);
					m.sendToTarget();
				}
				catch(UnknownHostException e)
				{
					Message m = Message.obtain(handler, msgError, "Failed to find to " + strIP + ".  Are you on the correct network?");
					m.sendToTarget();
				}
				catch(IOException e)
				{
					Message m = Message.obtain(handler, msgError, "Lap retransmission to " + strIP + " interrupted.  Is the computer still on and router still available?");
					m.sendToTarget();
				}
				db.endTransaction();
			}
		}).start();
	}
	
	private interface SocketWatchdogInterface
	{
		public int GetCheck(); // returns some value that must change over time.  If it doesn't change for GetTimeout() ms, the socketwatchdog will kill the socket
		public int GetTimeout(); // returns the time in ms that the socket watchdog should allow GetCheck() to not change before killing the socket
		public void Kill(Socket s); // tells the SWI that the jig is up
	}
	private class SocketWatchdog extends Thread implements Runnable
	{
		private Socket m_socket;
		private SocketWatchdogInterface m_swi;
		private boolean m_fContinue;
		public SocketWatchdog(SocketWatchdogInterface swi, Socket s)
		{
			m_socket = s;
			m_swi = swi;
			m_fContinue = true;
			start();
		}
		public void Shutdown()
		{
			m_fContinue = false;
		}
		public void KillTarget()
		{
			m_swi.Kill(m_socket);
		}
		public void run()
		{
			Thread.currentThread().setName("Socket watchdog");
			int iLast = 0;
			long tmLastChange = System.currentTimeMillis();
			
			while(m_fContinue && (m_socket != null) && m_socket.isConnected())
			{
				final int iCurrent = m_swi.GetCheck();
				if(iLast != iCurrent)
				{
					iLast = iCurrent;
					tmLastChange = System.currentTimeMillis();
				}
				
				final int iTimeout = m_swi.GetTimeout();
				if(System.currentTimeMillis() - tmLastChange > iTimeout)
				{
					KillTarget();
					break;
				}
				
				
				try {
					Thread.sleep(500);
				} catch (InterruptedException e) {}
			}
			m_swi = null;
			m_socket = null;
		}
	}
	private class SendThd extends Thread implements Runnable, SocketWatchdogInterface
	{
		private LapSenderListener listener;
		
		private int cLapsSent;
		private String strIP;
		private String strSSID;
		private WifiManager pWifi;
		private Utility.MultiStateObject pStateMan;
		private InetAddress addr;
		private SocketWatchdog m_watchdog;
		private int m_cLoops;
		private boolean fRequireWifi;
		
		public SendThd(Utility.MultiStateObject pWifiStateReceiver, LapSenderListener listener, WifiManager pWifi, String strIP, String strSSID, boolean fRequireWifi)
		{
			this.pStateMan = pWifiStateReceiver;
			pStateMan.SetState(LapSender.class, STATE.TROUBLE_GOOD, "Starting...");
			this.listener = listener;
			this.pWifi = pWifi;
			this.strIP = strIP;
			this.strSSID = strSSID;
			this.fRequireWifi = fRequireWifi;
		}
		public int GetLapSentCount()
		{
			return cLapsSent;
		}
		public synchronized void SetIP(String strNewIP)
		{
			if(strNewIP != strIP)
			{
				strIP = strNewIP;
			}
		}
		public synchronized void SetSSID(String strNewSSID)
		{
			if(strNewSSID != strSSID)
			{
				strSSID = strNewSSID;
			}
		}
		private int cSuccessfulBeats = 0;
		private boolean HeartBeat(InputStream in, OutputStream out)
		{
			DataInputStream dataIn = null;
			try
			{
				// attempts to send a 'htbt' DWORD to the other end of the socket and get a HTBT back
				// if it doesn't get a HTBT back within a second, returns false
				DataOutputStream dataOut = new DataOutputStream(out);
				dataOut.writeByte('h');
				dataOut.writeByte('t');
				dataOut.writeByte('b');
				dataOut.writeByte('t');
				
				dataIn = new DataInputStream(in);

				long tmStart = System.currentTimeMillis();
				while(true)
				{
					if(System.currentTimeMillis() - tmStart > 3000)
					{
						return false;
					}
					if(dataIn.available() >= 4)
					{
						break;
					}
					Thread.sleep(100);
				}
				
				byte b1,b2,b3,b4;
				b1 = dataIn.readByte();
				b2 = dataIn.readByte();
				b3 = dataIn.readByte();
				b4 = dataIn.readByte();
				if(b1 == 'H' && b2 == 'T' && b3 == 'B' && b4 == 'T')
				{
					cSuccessfulBeats++;
					pStateMan.SetState(LapSender.class, Utility.MultiStateObject.STATE.ON, "Connected");
					listener.SetConnectionLevel(LapSenderListener.CONNLEVEL.FULLYCONNECTED);
					return true;
				}
				else
				{
					pStateMan.SetState(LapSender.class, Utility.MultiStateObject.STATE.TROUBLE_GOOD, "Wifi present.  Is Pitside running?");
				}
				return false;
			}
			catch(Exception e)
			{
				return false;
			}
		}
		private synchronized String GetSSID()
		{
			return strSSID;
		}
		private void AcquireWIFI() throws InterruptedException
		{
			if(Build.MODEL.equals("sdk"))
			{
				return; // don't try to do this on the emulator
			}
			// first: is wifi even on?
			if(!pWifi.isWifiEnabled())
			{
				pWifi.setWifiEnabled(true);
			}
			
			// second: check if we're already connected to the desired network
			WifiInfo pInfo = pWifi.getConnectionInfo();
			if(pInfo != null)
			{
				String strCurrentSSID = pInfo.getSSID();
				if(strCurrentSSID != null)
				{
					String strLocalSSID = GetSSID();
					if(strCurrentSSID.equalsIgnoreCase(strLocalSSID))
					{
						DhcpInfo pDHCPInfo = pWifi.getDhcpInfo();
						if(pDHCPInfo != null)
						{
							if(pDHCPInfo.ipAddress != 0)
							{
								// we're good!
								listener.SetConnectionLevel(LapSenderListener.CONNLEVEL.CONNECTED);
								pStateMan.SetState(LapSender.class, Utility.MultiStateObject.STATE.TROUBLE_GOOD, "Connected to " + strLocalSSID + ", searching for Pitside @ " + this.strIP);
								return;
							}
						}
					}
					else
					{
						// we're connected to the wrong network!
						pWifi.disconnect();
					}
				}
				else
				{
					// we're not connected to any network.
				}
			}
			else if(pInfo == null)
			{
				// we're not connected to any network
			}
			listener.SetConnectionLevel(LapSenderListener.CONNLEVEL.SEARCHING);
			
			// ok, we should now be fully disconnected from the network we had before (if any)
			List<WifiConfiguration> lstNetworks = pWifi.getConfiguredNetworks();
			for(int x = 0; x < lstNetworks.size(); x++)
			{
				String strPickedSSID = GetSSID();
				WifiConfiguration pConfig = lstNetworks.get(x);
				if(pConfig.SSID.equalsIgnoreCase("\"" + strPickedSSID + "\""))
				{
					pWifi.enableNetwork(pConfig.networkId, true);
					pWifi.reconnect();
					int cAttempts = 0;
					while(fContinue)
					{
						String strLocalSSID = GetSSID();
						if(!strLocalSSID.equals(strPickedSSID)) break; // they must have changed their target SSID.  break out of this attempt to connect to the old one and look for the other one

						listener.SetConnectionLevel(LapSenderListener.CONNLEVEL.SEARCHING);
						pStateMan.SetState(LapSender.class, Utility.MultiStateObject.STATE.TROUBLE_GOOD, "Trying to connect to " + strLocalSSID + ", " + cAttempts + " attempts so far.");
						pInfo = pWifi.getConnectionInfo();
						if(pInfo != null)
						{
							String strCurrentSSID = pInfo.getSSID();
							if(strCurrentSSID != null)
							{	
								listener.SetConnectionLevel(LapSenderListener.CONNLEVEL.CONNECTED);
								pStateMan.SetState(LapSender.class, Utility.MultiStateObject.STATE.TROUBLE_GOOD, "Connected to " + strLocalSSID + ", searching for Pitside @ " + this.strIP);
								return;
							}
						}
						cAttempts++;
						Thread.sleep(250);
						SaveLapsToDB();
					}
				}
			}
			pStateMan.SetState(LapSender.class, Utility.MultiStateObject.STATE.TROUBLE_BAD, "SSID '" + strSSID + "' not found on phone.");
		}
		private Socket BuildSocket(InetAddress addr, int iPort) throws IOException,InterruptedException
		{
			if(m_watchdog != null) m_watchdog.Shutdown();
			
			if(fRequireWifi)
			{
				AcquireWIFI();
			}

			Socket s = null;
			SocketAddress sConnect = null;
			
			String strLocalIP = null;
			synchronized(this)
			{
				strLocalIP = strIP;
			}
			
			try
			{
				addr = InetAddress.getByName(strLocalIP);
				s = new Socket();
				sConnect = new InetSocketAddress(addr, iPort);
				s.connect(sConnect, 500);
				s.setSoTimeout(3000);
				
				m_watchdog = new SocketWatchdog(this, s);
			}
			catch(IOException e)
			{
				if(sConnect != null)
				{
					sConnect = null;
				}
				if(s != null)
				{
					s.close();
				}
				throw e;
			}
			return s;
		}
		@Override
		public void run()
		{
			Thread.currentThread().setName("LapSender");
			while(fContinue) // outer loop: builds a socket, and then does stuff.
			{
				Socket s = null;
				InputStream in = null;
				OutputStream out = null;
				try
				{
					m_cLoops++;
					// in case we can never get a socket connection, we still want to dump things into the DB.  future DB-save calls will be short-circuited since each lap will have a lap id
					SaveLapsToDB();
					
					// in case we really never get a socket connection, we'll want to make old DB-saved laps revert to a low-memory configuration (no points or data channels in memory) 
					PruneLaps();
					s = BuildSocket(addr,63939);
					in = s.getInputStream();
					out = s.getOutputStream();
					listener.SetConnectionLevel(LapSenderListener.CONNLEVEL.CONNECTED);
					while(fContinue) // inner loop: now that we have a socket, let's send laps until we can't anymore
					{
						m_cLoops++;
						// we have to send what we have
						if(!s.isConnected() || s.isClosed() || s.isOutputShutdown())
						{
							if(!s.isClosed()) 
							{
								in.close();
								out.close();
								s.close();
							}
							break; // need to make a new socket
						}
						
						// we've got a socket, so now let's send all the laps we need to send
						LapAccumulator lapToSend = null;
						synchronized(LapSender.this)
						{
							if(lstLapsToSend == null) return;
							
							if(lstLapsToSend.size() <= 0)
							{
								// nothing to do here.  lapToSend being null will trigger a HeartBeat() 
							}
							else
							{
								// pop the first lap off the front of our queue
								do
								{
									lapToSend = lstLapsToSend.get(0);
									lstLapsToSend.remove(0);
								}
								while(lapToSend != null && (lapToSend.IsPruned()));
							}
						}
						
						if(lapToSend != null)
						{
							lapToSend.SendToDB(RaceDatabase.Get(), m_lRaceId,false);
							boolean fNeedToResendLap = true;
							if(lapToSend.SendToOutput(out))
							{
								// hooray, we sent this lap to output!
								// if we've got connectivity on the same socket now, then we must have sent that whole thing
								fNeedToResendLap = !HeartBeat(in,out);
								if(!fNeedToResendLap)
								{
									lapToSend.MarkSentInDB(RaceDatabase.Get());
									cLapsSent++;
								}
							}
							else
							{
								// failed to send lap.  Exit the lap-sending loop
								fNeedToResendLap = true;
								
								in.close();
								out.close();
								s.close();
								s = BuildSocket(addr,63939);
								in = s.getInputStream();
								out = s.getOutputStream();
							}
	
							// failed to send.  Put it at the end of the list, it'll get sent later.
							if(fNeedToResendLap)
							{
								synchronized(LapSender.this)
								{
									if(lstLapsToSend == null) return;
									
									lstLapsToSend.add(lapToSend);
								}
							}
						}
						else
						{
							if(!HeartBeat(in,out))
							{
								// if we failed at our heartbeat attempt, try a new socket
								in.close();
								out.close();
								s.close();
								s = BuildSocket(addr,63939);
								in = s.getInputStream();
								out = s.getOutputStream();
							}
							else
							{
								pStateMan.SetState(LapSender.class, Utility.MultiStateObject.STATE.ON, "Connected to pitside.");
								Thread.sleep(250); // sleep for a bit
							}
						}
					}
					
				}
				catch(IOException e)
				{
					// not in range anymore.  That's fine, we'll get it later
					System.out.println(e.toString());
				} catch (InterruptedException e) 
				{
					e.printStackTrace();
				}
				try 
				{
					Thread.sleep(100);
				} 
				catch (InterruptedException e) 
				{}
				
			}
			pStateMan.SetState(LapSender.class, Utility.MultiStateObject.STATE.OFF, "Wifi shut down.");
		}
		@Override
		public int GetCheck() 
		{
			return m_cLoops;
		}
		@Override
		public int GetTimeout() 
		{
			return 10000;
		}
		@Override
		public void Kill(Socket s) 
		{
			try
			{
				s.shutdownInput();
				s.shutdownOutput();
				s.close();
			}
			catch(Exception e)
			{
				
			}
		}
	}
}
