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

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;

import android.util.Log;

public class MessageMan 
{
	MessageThd m_thd;
	public MessageMan(MessageReceiver recv)
	{
		m_thd = new MessageThd(recv);
		m_thd.start();
	}
	public void Shutdown()
	{
		m_thd.Shutdown();
	}
	public interface MessageReceiver
	{
		public abstract void SetMessage(int iTime, String strMsg);
	}
	
	private static class MessageThd extends Thread implements Runnable
	{
		private boolean m_fContinue;
		private MessageReceiver m_recv;
		public MessageThd(MessageReceiver recv)
		{
			m_fContinue = true;
			m_recv = recv;
		}
		public void Shutdown()
		{
			m_fContinue = false;
		}
		private void KillSocket(DatagramSocket ds)
		{
			if(ds != null && (ds.isBound() || ds.isConnected()))
			{
				ds.disconnect();
				ds.close();
			}
		}
		private int GetCheckSum(byte rg[])
		{
			int iRet = 0;
			for(int x = 0; x < rg.length; x++)
			{
				iRet += rg[x];
			}
			return iRet;
		}
		public void run()
		{
			Thread.currentThread().setName("MessageThread");
			// the message thread just constantly checks for incoming udp packets to display
			// the packet will look like:
			// 0-4: unique ID ('wflp')
			// 5-8: time (in seconds since 1970) until which we should display this message
			// 9-24: message to display
			
			byte pbBuffer[] = new byte[268];
			DatagramSocket dsIncoming = null;
			DatagramSocket dsOutgoing = null;
			while(m_fContinue)
			{
				try
				{
					dsIncoming = new DatagramSocket(63940);
					dsOutgoing = new DatagramSocket(63941);
					dsIncoming.setSoTimeout(1000);
					if(dsIncoming.isBound() && !dsIncoming.isClosed())
					{
						DatagramPacket dp = new DatagramPacket(pbBuffer, 268);
						dsIncoming.receive(dp);
						
						int checksum = GetCheckSum(pbBuffer);
						
						ByteArrayInputStream bais = new ByteArrayInputStream(pbBuffer);
						DataInputStream dis = new DataInputStream(bais);
						byte pbUnique[] = new byte[4];
						pbUnique[0] = dis.readByte();
						pbUnique[1] = dis.readByte();
						pbUnique[2] = dis.readByte();
						pbUnique[3] = dis.readByte();
						int time = dis.readInt();
						
						int strlen = 0;
						for(int x = 8; x < 268; x++)
						{
							if(pbBuffer[x] == 0)
							{
								strlen = x-8;
								break;
							}
						}
						String str = new String(pbBuffer,8,strlen);
						if(pbUnique[3] == 'W' && pbUnique[2] == 'F' && pbUnique[1] == 'L' && pbUnique[0] == 'P')
						{
							final int iCurrentTime = (int)(System.currentTimeMillis()/1000);
							time = iCurrentTime + time*60;
							final int iMaxTime = iCurrentTime + 4*60; // compute 4 minutes in the future for max-time determination
							time = Math.min(iMaxTime, time); // we don't want, in the case of a network error, to display a message forever.
							
							m_recv.SetMessage(time, str);
							

							dsOutgoing.connect(dp.getAddress(), 63941);
							ByteArrayOutputStream baos = new ByteArrayOutputStream(268);
							DataOutputStream dos = new DataOutputStream(baos);
							dos.write(new String("wflp").getBytes());
							dos.writeInt(checksum);

							byte rgReceive[] = baos.toByteArray();
							DatagramPacket dpOut = new DatagramPacket(rgReceive,rgReceive.length);
							dpOut.setData(rgReceive);
							dsOutgoing.send(dpOut);
						}
					}
				}
				catch(IOException e)
				{
					// meh.
					//Log.w("excr",e.toString());
				}
				catch(Exception e)
				{
					//Log.w("other ecxr",e.toString());
				}
				KillSocket(dsIncoming);
				KillSocket(dsOutgoing);
				try
				{
					Thread.sleep(250);
				}
				catch(InterruptedException e)
				{
					// nobody cares
				}
			}
		}
	}
}
