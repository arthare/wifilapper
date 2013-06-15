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

import java.util.Timer;
import java.util.TimerTask;

import com.artsoft.wifilapper.LapAccumulator.DataChannel;

import android.app.Activity;
import android.content.SharedPreferences;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.TableLayout;
import android.widget.TextView;
import android.widget.Toast;

public class ConfigureAccelerometerActivity extends Activity implements SensorEventListener, Handler.Callback, OnClickListener
{

	private static int RESTART = 201;
	private static int ROTATE = 202;
	// idea: when the user enters the activity, it tries to detect gravity for 1 second.  Once done that, the user can then enter the rotation of the phone.
	enum STATE {STATE_GRAVITY,STATE_ROTATE};
	STATE m_eState = STATE.STATE_GRAVITY;
	long tmStart;
	
	//Button btnRedo;
	//TextView lblInstructions;
	
	float flXSum;
	float flYSum;
	float flZSum;
	int cSamples;
	
	Timer timer;
	
	Handler m_handler;
	
	@Override
	public void onCreate(Bundle extras)
	{
		super.onCreate(extras);
		setContentView(R.layout.configureaccel);
		
		//btnRedo = (Button)findViewById(R.id.btnRedo);
		//lblInstructions = (TextView)findViewById(R.id.lblInstructions);
		m_handler = new Handler(this);
		
		//btnRedo.setOnClickListener(this);
		
		timer = new Timer();
	}
	
	@Override
	public void onPause()
	{
		super.onPause();
		
		CheckBox chk = (CheckBox)findViewById(R.id.chkUseAccel);
		SharedPreferences settings = this.getSharedPreferences(Prefs.SHAREDPREF_NAME, 0);
		SharedPreferences.Editor edit = settings.edit();
		edit.putBoolean(Prefs.PREF_USEACCEL_BOOLEAN, chk.isChecked());
		
		if(m_eState == STATE.STATE_GRAVITY || cSamples <= 0)
		{
			// don't save anything - they left before it was configured
		}
		else
		{
			// save the gravity vector and anything they've entered for rotation.
			edit.putFloat(Prefs.PREF_ACCEL_GRAV_X, flXSum / cSamples);
			edit.putFloat(Prefs.PREF_ACCEL_GRAV_Y, flYSum / cSamples);
			edit.putFloat(Prefs.PREF_ACCEL_GRAV_Z, flZSum / cSamples);
		}
		edit.commit();
	}
	@Override
	public void onResume()
	{
		super.onResume();
		
		CheckBox chk = (CheckBox)findViewById(R.id.chkUseAccel);
		SharedPreferences settings = this.getSharedPreferences(Prefs.SHAREDPREF_NAME, 0);
		chk.setChecked(settings.getBoolean(Prefs.PREF_USEACCEL_BOOLEAN, Prefs.DEFAULT_USEACCEL));
		
		SetState(STATE.STATE_GRAVITY);
	}
	
	
	private void SetState(STATE eState)
	{
		switch(eState)
		{
		case STATE_GRAVITY:
			// we're moving to a state where we're trying to detect gravity.  We need to wait 1 second while we find out where down is
			
			//lblInstructions.setText("Detecting gravity... do not move the phone");
			
			SensorManager sensorMan = (SensorManager)getSystemService(SENSOR_SERVICE);
			Sensor accel = sensorMan.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
	    	if(accel != null)
	    	{
	    		sensorMan.registerListener(this, accel, SensorManager.SENSOR_DELAY_FASTEST);
	    	}
	    	
			timer.cancel();
			timer = new Timer();
			timer.schedule(new NextStater(), 2000);
			
			break;
		case STATE_ROTATE:
			//lblInstructions.setText("Gravity found.  If the phone was not in its final mount in the car, you can run this test again then.");
			break;
		}
		m_eState = eState;
	}

	private class NextStater extends TimerTask
	{
		@Override
		public void run()
		{
			m_handler.sendEmptyMessage(ROTATE);
		}
	}
	
	@Override
	public void onAccuracyChanged(Sensor arg0, int arg1) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public boolean handleMessage(Message msg)
	{
		if(msg.what == RESTART)
		{
			SetState(STATE.STATE_GRAVITY);
		}
		else if(msg.what == ROTATE)
		{
			SetState(STATE.STATE_ROTATE);
		}
		return false;
	}
	
	SensorEvent lastEvent;
	@Override
	public void onSensorChanged(SensorEvent event) 
	{
		if(event.sensor.getType() == Sensor.TYPE_ACCELEROMETER && m_eState == STATE.STATE_GRAVITY)
		{
			// check to see if phone has shaken.  If it has, delay the timer
			float x = event.values[0];
			float y = event.values[1];
			float z = event.values[2];
			if(lastEvent != null)
			{
				float lastX = lastEvent.values[0];
				float lastY = lastEvent.values[1];
				float lastZ = lastEvent.values[2];
				float flDX = x - lastX;
				float flDY = y - lastY;
				float flDZ = z - lastZ;
				
				float flMovement = (float)Math.sqrt(flDX*flDX + flDY*flDY + flDZ*flDZ);
				if(flMovement > 1.0)
				{
					m_handler.sendEmptyMessage(RESTART);
				}
				else
				{
					flXSum += x;
					flYSum += y;
					flZSum += z;
					cSamples++;
				}
			}
			else
			{
				flXSum = flYSum = flZSum = cSamples = 0;
				m_handler.sendEmptyMessage(RESTART);
			}
		}
		lastEvent = event;
	}

	@Override
	public void onClick(View arg0) 
	{
		/*if(arg0.getId() == R.id.btnRedo)
		{
			flXSum = flYSum = flZSum = cSamples = 0;
			m_handler.sendEmptyMessage(RESTART);
		}*/
	}
}
