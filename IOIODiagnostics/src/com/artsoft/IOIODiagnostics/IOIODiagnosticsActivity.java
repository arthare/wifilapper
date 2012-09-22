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

import java.text.NumberFormat;
import java.util.Timer;
import java.util.TimerTask;

import ioio.lib.api.AnalogInput;
import ioio.lib.api.DigitalInput;
import ioio.lib.api.DigitalOutput;
import ioio.lib.api.IOIO;
import ioio.lib.api.IOIOFactory;
import ioio.lib.api.IcspMaster;
import ioio.lib.api.PulseInput;
import ioio.lib.api.PwmOutput;
import ioio.lib.api.SpiMaster;
import ioio.lib.api.TwiMaster;
import ioio.lib.api.Uart;
import ioio.lib.api.IOIO.VersionType;
import ioio.lib.api.PulseInput.PulseMode;
import ioio.lib.api.TwiMaster.Rate;
import ioio.lib.api.Uart.Parity;
import ioio.lib.api.Uart.StopBits;
import ioio.lib.api.exception.ConnectionLostException;
import ioio.lib.api.exception.IncompatibilityException;
import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

public class IOIODiagnosticsActivity extends Activity implements Handler.Callback
{
	private IOIO m_ioio;
	
	private PinWrapper m_pins[];
	private Handler m_handler;
	
	private float m_rgValues[] = new float[48];
	
	private static final int MSG_CONNECTATTEMPTDONE = 50;
	private static final int MSG_REFRESHED = 51;
	
	@Override
	public void onCreate(Bundle extras)
	{
		super.onCreate(extras);
		m_handler = new Handler(this);
		m_pins = new PinWrapper[48];
		m_ioio = IOIOFactory.create();
		
		ConnectThread ct = new ConnectThread(m_ioio, m_handler);
		ct.start();
		
		Timer t = new Timer();
		t.scheduleAtFixedRate(new CheckerTask(), 100, 33);
		
		this.setContentView(R.layout.main);
	}
	
	private class CheckerTask extends TimerTask
	{
		private float flMin = 1e30f;
		private float flMax = -1e30f;
		@Override
		public void run() 
		{
			try
			{
				if(m_pins[33] != null)
				{
					m_rgValues[33] = m_pins[33].GetValue();
				}
				if(m_pins[34] != null)
				{
					m_rgValues[34] = m_pins[34].GetValue();
				}
				m_handler.sendEmptyMessage(MSG_REFRESHED);
			}
			catch(Exception e)
			{
				Toast.makeText(IOIODiagnosticsActivity.this, "Failed to get value", Toast.LENGTH_LONG).show();
			}
			
		}
		
	}
	public static String FormatFloat(float fl, int digits)
	{
		NumberFormat num = NumberFormat.getInstance();
		num.setMaximumFractionDigits(digits);
		num.setMinimumFractionDigits(digits);
		return num.format(fl);
	}
	
	@Override
	public boolean handleMessage(Message msg) 
	{
		// TODO Auto-generated method stub
		if(msg.what == MSG_CONNECTATTEMPTDONE)
		{
			// they've finished attempting to connect to the IOIO, so let's populate all our pins
			try
			{
				m_pins[34] = new PinWrapper(m_ioio, 34, true);
				m_pins[33] = new PinWrapper(m_ioio, 33, false);
			}
			catch(Exception e)
			{
				Toast.makeText(this, "Failed to connect :( " + e.toString(), Toast.LENGTH_LONG).show();
			}
			
		}
		else if(msg.what == MSG_REFRESHED)
		{
			{
				SeekBar seek = (SeekBar)findViewById(R.id.seek33);
				seek.setMax(500);
				seek.setProgress((int)(m_rgValues[33]*100));
				
				TextView txt = (TextView)findViewById(R.id.label33);
				txt.setText(FormatFloat(m_rgValues[33],2) + "V");
			}
			
			{
				SeekBar seek = (SeekBar)findViewById(R.id.seek34);
				seek.setMax(100);
				seek.setProgress((int)m_rgValues[34]);
				
				TextView txt = (TextView)findViewById(R.id.label34);
				txt.setText(FormatFloat(m_rgValues[34],2) + "hz");
			}
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

class FakeIOIO implements IOIO
{
	public void waitForConnect() throws ConnectionLostException,
			IncompatibilityException
	{
		try
		{
			Thread.sleep(500);
		}
		catch(InterruptedException e)
		{
			
		}
		return;
	}
	
	public void disconnect()
	{
		return;
	}
	public void waitForDisconnect() throws InterruptedException
	{
		return;
	}
	public void softReset() throws ConnectionLostException
	{
		return;
	}
	public void hardReset() throws ConnectionLostException
	{
		return;
	}
	public String getImplVersion(VersionType v) throws ConnectionLostException
	{
		return "";
	}
	public DigitalInput openDigitalInput(DigitalInput.Spec spec)
			throws ConnectionLostException
	{
		return null;
	}
	public DigitalInput openDigitalInput(int pin)
			throws ConnectionLostException
	{
		return null;
	}
	public DigitalInput openDigitalInput(int pin, DigitalInput.Spec.Mode mode)
			throws ConnectionLostException
	{
		return null;
	}
	public DigitalOutput openDigitalOutput(DigitalOutput.Spec spec,
			boolean startValue) throws ConnectionLostException
	{
		return null;
	}
	public DigitalOutput openDigitalOutput(int pin,
			DigitalOutput.Spec.Mode mode, boolean startValue)throws ConnectionLostException
	{
		return null;
	}
			
	public DigitalOutput openDigitalOutput(int pin, boolean startValue)
			throws ConnectionLostException
	{
		return null;
	}
	public DigitalOutput openDigitalOutput(int pin)
			throws ConnectionLostException
	{
		return null;
	}
	public AnalogInput openAnalogInput(int pin) throws ConnectionLostException
	{
		return new FakeAnalogInput(pin);
	}
	public PwmOutput openPwmOutput(DigitalOutput.Spec spec, int freqHz)
			throws ConnectionLostException
	{
		return null;
	}
	public PwmOutput openPwmOutput(int pin, int freqHz)
			throws ConnectionLostException
	{
		return null;
	}
	public PulseInput openPulseInput(DigitalInput.Spec spec,
			PulseInput.ClockRate rate, PulseInput.PulseMode mode,
			boolean doublePrecision) throws ConnectionLostException
	{
		return null;
	}
	public PulseInput openPulseInput(int pin, PulseMode mode)
			throws ConnectionLostException
	{
		return new FakePulseInput(pin);
	}
	public Uart openUart(DigitalInput.Spec rx, DigitalOutput.Spec tx, int baud,
			Parity parity, StopBits stopbits) throws ConnectionLostException
	{
		return null;
	}
	public Uart openUart(int rx, int tx, int baud, Parity parity,
			StopBits stopbits) throws ConnectionLostException
	{
		return null;
	}
	public SpiMaster openSpiMaster(DigitalInput.Spec miso,
			DigitalOutput.Spec mosi, DigitalOutput.Spec clk,
			DigitalOutput.Spec[] slaveSelect, SpiMaster.Config config)throws ConnectionLostException
	{
		return null;
	}
			
	public SpiMaster openSpiMaster(int miso, int mosi, int clk,
			int[] slaveSelect, SpiMaster.Rate rate)
			throws ConnectionLostException
	{
		return null;
	}

	public SpiMaster openSpiMaster(int miso, int mosi, int clk,
			int slaveSelect, SpiMaster.Rate rate)
			throws ConnectionLostException
	{
		return null;
	}
	public TwiMaster openTwiMaster(int twiNum, Rate rate, boolean smbus)
			throws ConnectionLostException
	{
		return null;
	}
	public IcspMaster openIcspMaster() throws ConnectionLostException
	{
		return null;
	}
	
	private class FakeAnalogInput implements AnalogInput
	{
		private int pin;
		public FakeAnalogInput(int pin)
		{
			this.pin = pin;
		}
		@Override
		public void close() {}

		@Override
		public float getReference() {return 0;}

		@Override
		public float getVoltage() throws InterruptedException,ConnectionLostException 
		{
			double dTime = (System.currentTimeMillis()%10000) / 10000.0;
			return (float)(Math.sin(dTime * pin) * 2 + 2.5);
		}

		@Override
		public float read() throws InterruptedException, ConnectionLostException 
		{
			double dTime = (System.currentTimeMillis()%10000) / 10000.0;
			return (float)(Math.sin(dTime * pin) * 2 + 2.5);
		}
	}
	private class FakePulseInput implements PulseInput
	{
		private int pin;
		public FakePulseInput(int pin)
		{
			this.pin = pin;
		}
		@Override
		public void close() {}
		@Override
		public float getDuration() throws InterruptedException,ConnectionLostException 
		{
			return 0;
		}
		@Override
		public float getFrequency() throws InterruptedException,ConnectionLostException 
		{
			return (float)(((System.currentTimeMillis()%10000) / 10000.0) * pin);
		}
		@Override
		public float waitPulseGetDuration() throws InterruptedException,ConnectionLostException 
		{
			return 0;
		}
	}
}
