package com.artsoft.wifilapper;

import java.io.IOException;
import java.io.InputStream;
import java.util.Calendar;
import java.util.Date;
import java.util.Iterator;
import java.util.Set;
import java.util.UUID;
import java.util.Vector;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.location.LocationProvider;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class BluetoothGPS 
{
	DeviceRecvThread m_thd;
	public BluetoothGPS(BluetoothDevice dev, LocationListener listener)
	{
		m_thd = new DeviceRecvThread(dev, listener);
		m_thd.start();
	}
	public BluetoothGPS(String strDeviceName, LocationListener listener)
	{
		BluetoothAdapter ba = BluetoothAdapter.getDefaultAdapter();
		if(ba.isEnabled())
		{
			Set<BluetoothDevice> lstDevices = ba.getBondedDevices();
			Iterator<BluetoothDevice> i = lstDevices.iterator();
			while(i.hasNext())
			{
				BluetoothDevice bd = i.next();
				if(strDeviceName.equals(bd.getName()))
				{
					// found our device
					m_thd = new DeviceRecvThread(bd, listener);
					m_thd.start();
					break;
				}
			}
		}
	}
	public boolean IsValid()
	{
		return m_thd != null;
	}
	public void Shutdown()
	{
		if(m_thd != null)
		{
			m_thd.Shutdown();
			m_thd = null;
		}
	}
	
	private static class DeviceRecvThread extends Thread implements Runnable, Handler.Callback
	{
		BluetoothDevice m_dev;
		LocationListener m_listener;
		boolean m_shutdown;
		Handler m_handler;
		Vector<Location> m_lstLocs; // must be accessed inside DeviceRecvThread's lock
		final int MSG_SENDLOCS = 101;
		final int MSG_LOSTGPS = 102;
		final int MSG_NOGPSDEVICE = 103;
		
		public DeviceRecvThread(BluetoothDevice dev, LocationListener listener)
		{
			m_lstLocs = new Vector<Location>();
			
			m_dev = dev;
			m_listener = listener;
			m_handler = new Handler(this);
		}
		public synchronized void Shutdown()
		{
			m_shutdown = true;
		}
		public static boolean ValidateNMEA(String nmea)
		{
			nmea = nmea.replace("\r\n", "");
			
			if(nmea.charAt(0) == '$')
			{
				final int ixAst = nmea.lastIndexOf("*");
				if(ixAst >= 0 && ixAst < nmea.length())
				{	
					final int ixLen = Math.min(ixAst+3, nmea.length());
					String strHex = nmea.substring(ixAst+1,ixLen);
					if(strHex.length() >= 1 && strHex.length() <= 2)
					{
						try
						{
						int iValue = Integer.parseInt(strHex, 16);
						byte bXORed = 0;
						for(int x = 1;x < ixAst; x++)
						{
							byte bValue = (byte)nmea.charAt(x);
							bXORed ^= bValue;
						}
						if(bXORed == iValue)
						{
							return true;
						}
						}
						catch(NumberFormatException e)
						{
							// just fall through and we'll return false below...
						}
					}
				}
			}
			return false;
		}
		private String ParseAndSendNMEA(String strNMEA)
		{
			String strLastLeftover = "";
			int ixCur = strNMEA.indexOf("$GPGGA");
			while(ixCur != -1)
			{
				int ixNext = strNMEA.indexOf("$", ixCur+1);
				int ixVTG = strNMEA.indexOf("$GPVTG",ixCur+1); // finds the next command
				int ixNextAfterVTG = strNMEA.indexOf("$",ixVTG+1);
				if(ixNextAfterVTG == -1) ixNextAfterVTG = strNMEA.length();
				
				if(ixVTG == -1 || ixNext == -1)
				{
					strLastLeftover = strNMEA.substring(ixCur,strNMEA.length());
					break;
				}
				strLastLeftover = "";
				String strGGACommand = strNMEA.substring(ixCur,ixNext);
				String strVTGCommand = strNMEA.substring(ixVTG,ixNextAfterVTG);
				String strGGABits[] = strGGACommand.split(",");
				String strVTGBits[] = strVTGCommand.split(",");
				
				if(strGGABits.length >= 6 && strVTGBits.length >= 8 && ValidateNMEA(strGGACommand) && ValidateNMEA(strVTGCommand))
				{
					/*
					1    = UTC of Position (hhmmss.mmmm)
					2    = Latitude
					3    = N or S
					4    = Longitude
					5    = E or W
					6    = GPS quality indicator (0=invalid; 1=GPS fix; 2=Diff. GPS fix)
					7    = Number of satellites in use [not those in view]
					8    = Horizontal dilution of position
					9    = Antenna altitude above/below mean sea level (geoid)
					10   = Meters  (Antenna height unit)
					11   = Geoidal separation (Diff. between WGS-84 earth ellipsoid and
					       mean sea level.  -=geoid is below WGS-84 ellipsoid)
					12   = Meters  (Units of geoidal separation)
					13   = Age in seconds since last update from diff. reference station
					14   = Diff. reference station ID#
					15   = Checksum
					*/
					String strUTC = strGGABits[1];
					String strLat = strGGABits[2];
					String strNS = strGGABits[3];
					String strLong = strGGABits[4];
					String strEW = strGGABits[5];
					String strSpeed = strVTGBits[7];
					
					if(strUTC.length() > 0 && strLat.length() > 0 && strLong.length() > 0 && strNS.length() >= 1 && strEW.length() >= 1)
					{
						try
						{
							int iLatBase = Integer.parseInt(strLat.substring(0,2));
							int iLatFraction = Integer.parseInt(strLat.substring(2,4));
							double dLatFinal = Double.parseDouble(strLat.substring(4,strLat.length()));
							
							int iLongBase = Integer.parseInt(strLong.substring(0,3));
							int iLongFraction = Integer.parseInt(strLong.substring(3,5));
							double dLongFinal = Double.parseDouble(strLong.substring(5,strLong.length()));
							
							double dLat = iLatBase + ((double)iLatFraction+dLatFinal)/60.0;
							double dLong = iLongBase + ((double)iLongFraction+dLongFinal)/60.0;
							
							if(strNS.charAt(0) == 'S') dLat = -dLat;
							if(strEW.charAt(0) == 'W') dLong = -dLong;
							
							String strMinutes = strUTC.substring(2,4);
							String strSeconds = strUTC.substring(4,strUTC.length());
							int cMinutes = Integer.parseInt(strMinutes);
							double cSeconds = Double.parseDouble(strSeconds);
							int cMs = (int)((cSeconds - (int)cSeconds) * 1000);
							
							Calendar cal = Calendar.getInstance();
							cal.setTimeInMillis(System.currentTimeMillis());
							cal.set(Calendar.MINUTE, cMinutes);
							cal.set(Calendar.SECOND, (int)cSeconds);
							cal.set(Calendar.MILLISECOND, cMs);
							
							long lTime = cal.getTimeInMillis();
							
							float dSpeed = (float)Double.parseDouble(strSpeed);
							
							Location l = new Location("ArtBT");
							l.setLatitude(dLat);
							l.setLongitude(dLong);
							l.setTime(lTime);
							l.setSpeed(dSpeed/3.6f);

							synchronized(this)
							{
								m_lstLocs.add(l);
								m_handler.sendEmptyMessage(MSG_SENDLOCS);
							}
						}
						catch(Exception e)
						{
							// if it fails to parse the lat long, game over
							break;
						}
					}
					ixCur = strNMEA.indexOf("$GPGGA",ixCur+1);
				}
				else
				{
					break;
				}

				strLastLeftover = "";
			}
			return strLastLeftover;
		}
		public void run()
		{
			Thread.currentThread().setName("BT GPS thread");
			while(!m_shutdown)
			{
				InputStream in = null;
				BluetoothSocket bs = null;
				boolean fDeviceGood = true; // assume it's good to begin with...
				try
				{
					bs = m_dev.createRfcommSocketToServiceRecord(UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"));
					bs.connect();
					in = bs.getInputStream();
					fDeviceGood = true; // if we got this far without an exception, the device is good
				}
				catch(IOException e)
				{
					m_handler.sendEmptyMessage(MSG_NOGPSDEVICE);
					fDeviceGood = false;
				}
				String strLastLeftover = "";
				while(fDeviceGood && !m_shutdown)
				{
					// 1. read buffer
					// 2. convert to string
					// 3. find last GPGGA
					// 4. decode and send
					int cbRead = 0;
					byte rgBuffer[] = new byte[10000];
					try 
					{
						cbRead = in.read(rgBuffer,0,rgBuffer.length);
					} 
					catch (IOException e) 
					{
						fDeviceGood = false; // we may need to re-acquire the BT device...
						m_handler.sendEmptyMessage(MSG_LOSTGPS);
						break;
					}
					if(cbRead > 0)
					{
						String str = strLastLeftover + new String(rgBuffer);
						strLastLeftover = ParseAndSendNMEA(str);
					}
				}
				if(in != null)
				{
					try{in.close(); } catch(IOException e2) {}
				}
				if(bs != null)
				{
					try{bs.close(); } catch(IOException e2) {}
				}
				
				try{Thread.sleep(1000);} catch(Exception e) {} // give the lost device some time to re-acquire
			}
		}
		private boolean fLostGPS = true;
		@Override
		public boolean handleMessage(Message msg) 
		{
			if(msg.what == MSG_SENDLOCS)
			{
				synchronized(this)
				{
					if(fLostGPS)
					{
						m_listener.onStatusChanged(LocationManager.GPS_PROVIDER, LocationProvider.AVAILABLE, null);
						fLostGPS = false;
					}
					for(int x = 0;x < m_lstLocs.size(); x++)
					{
						m_listener.onLocationChanged(m_lstLocs.get(x));
					}
					m_lstLocs.clear();
				}
			}
			else if(msg.what == MSG_LOSTGPS)
			{
				synchronized(this)
				{
					m_listener.onStatusChanged(LocationManager.GPS_PROVIDER, LocationProvider.TEMPORARILY_UNAVAILABLE, null);
					fLostGPS = true;
				}
			}
			else if(msg.what == MSG_NOGPSDEVICE)
			{
				synchronized(this)
				{
					m_listener.onStatusChanged(LocationManager.GPS_PROVIDER, LocationProvider.OUT_OF_SERVICE, null);
					fLostGPS = true;
				}
			}
			return false;
		}
	}
}
