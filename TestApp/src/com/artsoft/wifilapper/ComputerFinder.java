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

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.MulticastSocket;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;

public class ComputerFinder
{
    /** Called when the activity is first created. */
	
	public static class FoundComputer
	{
		public FoundComputer(String strName, String strIP)
		{
			this.strName = strName;
			this.strIP = strIP;
		}
		public String GetName() {return strName;}
		public String GetIP() {return strIP;}
		
		@Override
		public String toString()
		{
			return strName + " - " + strIP;
		}
		
		private String strName;
		private String strIP;
	}
	
	List<FoundComputer> m_lstResponses;
	RecvThread m_thd;
	
	
	int m_msg; // the message that we send to our the handler
	Handler m_handler; // the handler that we notify when a new computer arrives
	
	public ComputerFinder()
	{
	}
	public void Init(Handler handler, int msg)
	{
		m_msg = msg;
		m_handler = handler;
		m_thd = new RecvThread();
	}
	public void Shutdown()
	{
		m_thd.Shutdown();
		
		m_handler = null;
		m_thd = null;
	}
	
    private synchronized void ClearList()
    {
    	Log.w("computerfind","clearing");
    	m_lstResponses = new ArrayList<FoundComputer>();
    }
    private synchronized void AddComputer(FoundComputer fc)
    {
    	Log.w("computerfind","adding " + fc.GetName());
    	if(m_handler != null)
    	{
	    	if(m_lstResponses == null)
	    	{
	    		ClearList();
	    	}

	    	boolean fAlreadyThere = false;
	    	for(int x = 0; x < m_lstResponses.size(); x++)
	    	{
	    		FoundComputer fOld = (FoundComputer)m_lstResponses.get(x);
	    		if(fOld.strIP.equals(fc.strIP) && fOld.strName.equals(fc.strName) )
	    		{
	    			// already exists
	    			fAlreadyThere = true;
	    		}
	    	}
	    	if(!fAlreadyThere)
	    	{
	    		m_lstResponses.add(fc);
	    	}
	    	
	    	m_handler.sendEmptyMessage(m_msg);
    	}
    }
    public synchronized List<FoundComputer> GetComputerList()
    {
    	List<FoundComputer> lstResp = new ArrayList<FoundComputer>();
    	if(m_lstResponses != null)
    	{
	    	for(int x = 0; x < m_lstResponses.size(); x++)
	    	{
	    		lstResp.add(m_lstResponses.get(x));
	    	}
    	}
    	return lstResp;
    }
	public boolean StartFindComputers() 
	{
		ClearList();
		try
		{
			MulticastSocket ms = new MulticastSocket(63938);
			byte rgData[] = new byte[4];
			rgData[0] = 1;
			rgData[1] = 2;
			rgData[2] = 4;
			rgData[3] = 8;
			
			InetAddress group = InetAddress.getByName("239.255.39.39");
			DatagramPacket dp = new DatagramPacket(rgData, 4, group, 63938);
			ms.send(dp);
			ms.close();
			return true;
		}
		catch(IOException e)
		{
			return false;
		}
	}
	private class RecvThread extends Thread implements Runnable
	{
		private boolean m_fContinue = true;
		private DatagramSocket m_ds;
		public RecvThread()
		{
			start();
		}
		public void Shutdown()
		{
			m_fContinue = false;
			if(m_ds != null)
			{
				try 
				{
					// this is so stupid.  Because the API doesn't have a timeout version of the datagram receive call, we have to send a packet to the waiting thread
					DatagramSocket dsKiller = new DatagramSocket();

					byte rgIP[] = new byte[4];
					rgIP[0] = 127;
					rgIP[1] = 0;
					rgIP[2] = 0;
					rgIP[3] = 1;
					InetAddress localhost = InetAddress.getByAddress(rgIP);
							
					byte rgPellet[] = new byte[100];
					DatagramPacket cyanide = new DatagramPacket(rgPellet,rgPellet.length,localhost,63937);
					
					dsKiller.send(cyanide);
				} 
				catch (UnknownHostException e) 
				{
					e.printStackTrace();
				} 
				catch (IOException e) 
				{
					e.printStackTrace();
				}
			}
		}
		@Override
		public void run()
		{
			Thread.currentThread().setName("UDP Recv thread");
			while(m_fContinue)
			{
				try 
				{
					m_ds = new DatagramSocket(63937);
					
					byte rgData[] = new byte[1000];
					DatagramPacket dpRecv = new DatagramPacket(rgData,rgData.length);
					while(m_fContinue)
					{
						m_ds.receive(dpRecv);
						if(!m_fContinue) 
						{
							break;
						}
						
						// we're expecting the payload to be a string
						int cch = 0;
						for(int x = 0;x < rgData.length; x++)
						{
							if(rgData[x] == 0)
							{
								cch = x;
								break;
							}
						}
						
						String str = new String(rgData, 0, cch);
						String strIP = dpRecv.getAddress().getHostAddress();
						AddComputer(new FoundComputer(str,strIP));
					}
				} 
				catch (SocketException e) 
				{	
					Log.w("computerfind","computerfind - " + e.toString());
				} catch (IOException e) 
				{
					Log.w("computerfind","computerfind - " + e.toString());
				}
				if(m_ds != null)
				{
					m_ds.close();
					m_ds = null;
				}
				
				try { Thread.sleep(100);
				} catch (InterruptedException e) {}
			}
		}
	}
}