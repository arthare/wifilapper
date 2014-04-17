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
	
	TextView lblInstructions;
	String detectString = new String("Detecting gravity... do not move the phone");
	CheckBox chkEnable;

	SensorManager sensorMan;
	Sensor accel;
	
	// Candidate gravity vector
	float flXcand, flYcand, flZcand;
		
	Timer timer;
	
	public static boolean debugMode = false;
	
	Handler m_handler;
	
	@Override
	public void onCreate(Bundle extras)
	{
		super.onCreate(extras);
		setContentView(R.layout.configureaccel);
		

		lblInstructions = (TextView)findViewById(R.id.lblInstructions);
		lblInstructions.setText(detectString);
	
		m_handler = new Handler(this);

		sensorMan = (SensorManager)getSystemService(SENSOR_SERVICE);
		accel = sensorMan.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);

		flXcand = flYcand = flZcand = 0;
		
		//btnRedo.setOnClickListener(this);
		chkEnable = (CheckBox)findViewById(R.id.chkUseAccel);
		chkEnable.setOnClickListener(this);
		
		timer = new Timer();
	}
	
	@Override
	public void onPause()
	{
		super.onPause();
		sensorMan.unregisterListener(this);

		
		CheckBox chk = (CheckBox)findViewById(R.id.chkUseAccel);
		SharedPreferences settings = this.getSharedPreferences(Prefs.SHAREDPREF_NAME, 0);
		SharedPreferences.Editor edit = settings.edit();
		edit.putBoolean(Prefs.PREF_USEACCEL_BOOLEAN, chk.isChecked());
		
		if(m_eState == STATE.STATE_GRAVITY)
		{
			// don't save anything - they left before it was configured
		}
		else
		{
			// save the gravity vector and anything they've entered for rotation.
			edit.putFloat(Prefs.PREF_ACCEL_GRAV_X, flXcand);
			edit.putFloat(Prefs.PREF_ACCEL_GRAV_Y, flYcand);
			edit.putFloat(Prefs.PREF_ACCEL_GRAV_Z, flZcand);
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
			
	    	if(accel != null)
	    	{
	    		sensorMan.registerListener(this, accel, SensorManager.SENSOR_DELAY_GAME);
	    	}
	    	
			timer.cancel();
			timer = new Timer();
			timer.schedule(new NextStater(), 2000);
			
			break;
		case STATE_ROTATE:
			lblInstructions.setText("Gravity found.  If the phone was not in its final mount in the car, you can run this test again by toggling the checkbox.");
			sensorMan.unregisterListener(this, accel);

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
	
	@Override
	public void onSensorChanged(SensorEvent event) 
	{

		if( event.accuracy == SensorManager.SENSOR_STATUS_UNRELIABLE )
			return;

		switch (event.sensor.getType() ) {
		case Sensor.TYPE_ACCELEROMETER:

			// check to see if phone has shaken.  If it has, delay the timer
			float z = event.values[0];
			float x = event.values[1];
			float y = event.values[2];
			float flDX = x - flXcand;
			float flDY = y - flYcand;
			float flDZ = z - flZcand;

			float flDifference = flDX*flDX + flDY*flDY + flDZ*flDZ;
			if( debugMode )
			{
				float dist = x*x+y*y+z*z;
				CheckBox chk = (CheckBox)findViewById(R.id.chkUseAccel);
				chk.setText(
						Float.toString(flXcand) + "     " +
								Float.toString(flYcand) + "     " +
								Float.toString(flZcand) + "   " +  Float.toString(dist) + 
								"\nmotion " + Float.toString(flDifference));
			}
			if( flDifference > 0.1 )
			{
				lblInstructions.setText(detectString + "  !! Moving !!");
				m_handler.sendEmptyMessage(RESTART);
			}
			else
			{
				lblInstructions.setText(detectString);
			}

			// Keep a moving average of the gravity vector
			flXcand = 0.95f*flXcand + 0.05f*x;
			flYcand = 0.95f*flYcand + 0.05f*y;
			flZcand = 0.95f*flZcand + 0.05f*z;
			break;
		}
	}

	@Override
	public void onClick(View arg0) 
	{
		if(arg0.getId() == R.id.chkUseAccel) 
		{
			if( chkEnable.isChecked() )
			{
				flXcand = flYcand = flZcand = 0;
				m_handler.sendEmptyMessage(RESTART);
			}
			else
				sensorMan.unregisterListener(this, accel);
		}
	}
}
