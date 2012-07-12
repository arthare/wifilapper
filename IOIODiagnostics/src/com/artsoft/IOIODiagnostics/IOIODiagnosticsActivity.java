// Copyright 2011-2012, Art Hare
// This file is part of IOIODiagnostics.

//IOIODiagnostics is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//WifiLapper is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with WifiLapper.  If not, see <http://www.gnu.org/licenses/>.


// a simple activity to view the IOIO's inputs
package com.artsoft.IOIODiagnostics;

import ioio.lib.api.AnalogInput;
import ioio.lib.api.IOIO;
import ioio.lib.api.IOIOFactory;
import ioio.lib.api.PulseInput;
import ioio.lib.api.PulseInput.PulseMode;
import ioio.lib.api.exception.ConnectionLostException;
import ioio.lib.api.exception.IncompatibilityException;
import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

public class IOIODiagnosticsActivity extends Activity implements Handler.Callback
{
	private IOIO m_ioio;
	
	private PinWrapper m_pins[];
	private Handler m_handler;
	
	private static final int MSG_CONNECTATTEMPTDONE = 50;
	
	@Override
	public void onCreate(Bundle extras)
	{
		m_handler = new Handler(this);
		m_pins = new PinWrapper[48];
		m_ioio = IOIOFactory.create();
	}
	
	@Override
	public boolean handleMessage(Message msg) 
	{
		// TODO Auto-generated method stub
		if(msg.what == MSG_CONNECTATTEMPTDONE)
		{
			// they've finished attempting to connect to the IOIO, so let's populate all our pins
		}
		return false;
	}
	
	
	private class ConnectThread extends Thread
	{
		private IOIO m_ioio;
		private Handler m_handler;
		
		public ConnectThread(IOIO ioio, Handler handler)
		{
			m_ioio = ioio;
			m_handler = handler;
		}
		
		@Override
		public void run()
		{
			boolean fSuccess = false;
			try 
			{
				m_ioio.waitForConnect();
				fSuccess = true;
			} 
			catch (ConnectionLostException e) {}
			catch (IncompatibilityException e) {}
			finally
			{
				m_handler.sendEmptyMessage(MSG_CONNECTATTEMPTDONE);
			}
		}
	}
	
	// class that wraps either an analog pin or a pulse pin, simplifying the main class's logic
	private static class PinWrapper
	{
		private AnalogInput m_analog;
		private PulseInput m_pulse;
		private IOIO m_ioio;
		private int m_pinNumber;
		
		public PinWrapper(IOIO ioio, int pinNumber, boolean fPulse) throws ConnectionLostException
		{
			m_ioio = ioio;
			m_pinNumber = pinNumber;
			InitPin(fPulse);
		}
		private void InitPin(boolean fPulse) throws ConnectionLostException
		{
			if(m_analog != null)
			{
				m_analog.close();
				m_analog = null;
			}
			if(m_pulse != null)
			{
				m_pulse.close();
				m_pulse = null;
			}
			
			if(fPulse)
			{
				m_pulse = m_ioio.openPulseInput(m_pinNumber, PulseMode.FREQ);
			}
			else
			{
				m_analog = m_ioio.openAnalogInput(m_pinNumber);
			}
		}
		public void SetPinParams(boolean fPulse) throws ConnectionLostException
		{
			InitPin(fPulse);
		}
		
		public float GetValue() throws ConnectionLostException,InterruptedException
		{
			if(m_analog != null)
			{
				return m_analog.getVoltage();
			}
			else if(m_pulse != null)
			{
				return m_pulse.getFrequency();
			}
			else
			{
				return 0;
			}
		}
		
	}

}
