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

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.media.AudioManager;
import android.media.SoundPool;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.RadioGroup;
import android.widget.RadioGroup.OnCheckedChangeListener;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.Spinner;
import android.widget.TextView;

public class ConfigureAccelerometerActivity extends Activity implements
		SensorEventListener, Handler.Callback, OnClickListener,
		OnSeekBarChangeListener, OnCheckedChangeListener, OnItemSelectedListener {

	// Views and Widgets
	private TextView lblInstructions;
	private TextView lblPitch, lblRoll;
	private CheckBox chkEnable;
	private CheckBox chkEnableCorrection;
	private Button btnCalibrateSensor;
	private Button btnConfigHelp;
	private SeekBar skPitch;
	private SeekBar skRoll;
	private RadioGroup rgCorrectionType;
	private Spinner spnFilter;

	// Sensors
	private SensorManager sensorMan;
	private Sensor accel;
	private Sensor mSensor;	
	private long lastSampleTime;
	
	// Sounds 
	SoundPool soundPool;
	int streamId;
	int soundId;
	
	// Variables
	private float flPitch;
	private float flRoll;
	private float [] flSensorOffset = new float[3];
	private boolean bMaskRadio1;
	private boolean bCalibrating;
	private boolean bOrienting;

	// Variables to determine phone orientation angle
	private float[] I = new float[16];
	private float[] inR = new float[16];
	private float[] gravity = new float[3];
	private float[] savedGravity = new float[3];
	private float[] geomag = new float[3];
	private float[] orientVals = new float[3];
	
	// Variables for filtering the Sensor
	private int iAccelCount, iRotCount;
	private float[] flAccelEvent = new float[3];
	private float[] flRotEvent = new float[3];
	private final int iFilterLength = 10;
	private int iFilterType=1;
	
	// Variables dealing with the timer
	Timer timer;
	Handler m_handler;
	private static final int REVERT = 203;	
			
	@Override
	public void onCreate(Bundle extras) {
		super.onCreate(extras);
		setContentView(R.layout.configureaccel);

		bCalibrating = false;
		bOrienting = false;
				
		soundPool = new SoundPool(1, AudioManager.STREAM_MUSIC, 0);
		soundId = soundPool.load(this,R.raw.short_1khz,1);
		
		flPitch = 0;
		flRoll = 0;

		skPitch = (SeekBar) findViewById(R.id.skPitch);
		skPitch.setOnSeekBarChangeListener(this);

		skRoll = (SeekBar) findViewById(R.id.skRoll);
		skRoll.setOnSeekBarChangeListener(this);

		lblPitch = (TextView) findViewById(R.id.lblPitch);
		lblRoll = (TextView) findViewById(R.id.lblRoll);

		rgCorrectionType = (RadioGroup) findViewById(R.id.rgCorrectionType);
		rgCorrectionType.setOnCheckedChangeListener(this);
		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
		bMaskRadio1 = false;
		
		chkEnableCorrection = (CheckBox) findViewById(R.id.chkEnableCorrection);
		chkEnableCorrection.setOnClickListener(this);
		chkEnableCorrection.setChecked(false);
		
		btnConfigHelp = (Button)findViewById(R.id.btnConfigHelp);
		btnConfigHelp.setOnClickListener(this);
		
		lblInstructions = (TextView) findViewById(R.id.lblInstructions);
		
		chkEnable = (CheckBox) findViewById(R.id.chkUseAccel);
		chkEnable.setOnClickListener(this);
		chkEnable.setChecked(false);

		btnCalibrateSensor = (Button)findViewById(R.id.btnCalibrateSensor);
		btnCalibrateSensor.setOnClickListener(this);
		btnCalibrateSensor.setEnabled(chkEnable.isChecked());

		spnFilter = (Spinner)findViewById(R.id.spnFilter);
		spnFilter.setOnItemSelectedListener(this);
		
		iAccelCount = 0;
		iRotCount = 0;

		sensorMan = (SensorManager) getSystemService(SENSOR_SERVICE);
		accel = sensorMan.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
		mSensor = sensorMan.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);

		gravity = null;
		geomag = null;
		savedGravity = null;

		// Update widgets based on variables
		processChkEnableCorrection();
		updatePitchAndRollSliders(flPitch,flRoll);
		processCalibration();
		
		m_handler = new Handler(this);
	}

	@Override
	public void onPause() {
		super.onPause();
		
		if( soundPool != null ) {
			soundPool.release();
			soundPool=null;
		}
		
		// disable sensors when leaving
		enableSensors(false);

		SharedPreferences settings = this.getSharedPreferences(Prefs.SHAREDPREF_NAME, 0);
		SharedPreferences.Editor edit = settings.edit();

		edit.putBoolean(Prefs.PREF_USEACCEL_BOOLEAN, chkEnable.isChecked());
		edit.putInt(Prefs.PREF_ACCEL_FILTER, iFilterType);
		edit.putBoolean(Prefs.PREF_ACCEL_CORRECTION, chkEnableCorrection.isChecked());
		edit.putFloat(Prefs.PREF_ACCEL_CORRECTION_PITCH, flPitch);
		edit.putFloat(Prefs.PREF_ACCEL_CORRECTION_ROLL, flRoll);
		if( flSensorOffset != null ) {
			edit.putFloat(Prefs.PREF_ACCEL_OFFSET_X, flSensorOffset[1]);
			edit.putFloat(Prefs.PREF_ACCEL_OFFSET_Y, flSensorOffset[2]);
			edit.putFloat(Prefs.PREF_ACCEL_OFFSET_Z, flSensorOffset[0]);
		}
		edit.commit();
	}

	@Override
	public void onResume() {
		super.onResume();

		SharedPreferences settings = this.getSharedPreferences(Prefs.SHAREDPREF_NAME, 0);

		flPitch =settings.getFloat(Prefs.PREF_ACCEL_CORRECTION_PITCH,Prefs.DEFAULT_ACCEL_CORRECTION_PITCH);
		flRoll =settings.getFloat(Prefs.PREF_ACCEL_CORRECTION_ROLL,Prefs.DEFAULT_ACCEL_CORRECTION_ROLL);
		updatePitchAndRollSliders(flPitch,flRoll);

		chkEnable.setChecked(settings.getBoolean(Prefs.PREF_USEACCEL_BOOLEAN,Prefs.DEFAULT_USEACCEL));

		iFilterType = settings.getInt(Prefs.PREF_ACCEL_FILTER,Prefs.DEFAULT_ACCEL_FILTER);
		spnFilter.setSelection(iFilterType,true);

		btnCalibrateSensor.setEnabled(chkEnable.isChecked());
		bCalibrating = false;

		chkEnableCorrection.setChecked(settings.getBoolean(Prefs.PREF_ACCEL_CORRECTION,Prefs.DEFAULT_ACCEL_CORRECTION));
		processChkEnableCorrection();

		flSensorOffset[1] = settings.getFloat(Prefs.PREF_ACCEL_OFFSET_X, Prefs.DEFAULT_ACCEL_OFFSET_X);
		flSensorOffset[2] = settings.getFloat(Prefs.PREF_ACCEL_OFFSET_Y, Prefs.DEFAULT_ACCEL_OFFSET_Y);
		flSensorOffset[0] = settings.getFloat(Prefs.PREF_ACCEL_OFFSET_Z, Prefs.DEFAULT_ACCEL_OFFSET_Z);
		
		processCalibration();
	}
	@Override
	public void onSensorChanged(SensorEvent event) {
		if (event.accuracy == SensorManager.SENSOR_STATUS_UNRELIABLE)
			return;
		boolean doCalc = false;
		
		switch (event.sensor.getType()) {
		case Sensor.TYPE_ACCELEROMETER:

			// When enabling the sensor, sometimes the first few samples are garbage.  Skip those.
			if( ++iAccelCount <= 0) {
				flAccelEvent = new float[] {0,0,0};
				break;
			}

			// Correct sensor by adding the offsets
			for (int i = 0; i < 3; i++) 
				if( flSensorOffset != null)
					flAccelEvent[i] += event.values[i]+flSensorOffset[i];
				else
					flAccelEvent[i] += event.values[i];

			// exit if still collecting samples
			if( System.currentTimeMillis() - lastSampleTime < 50)
				break;
			
			lastSampleTime = System.currentTimeMillis();

			// Got enough samples, divide to determine average
			for (int i = 0; i < 3; i++) {
				flAccelEvent[i] /= iAccelCount;
			}

			gravity = flAccelEvent.clone();
			if( savedGravity == null ){
				// Save the initial gravity vector during mount angle compensation
				savedGravity=gravity.clone();
			}
			doCalc = true;
			
			// Reset variables for next loop
			iAccelCount = 0;
			flAccelEvent = new float[] {0,0,0};
			break;

		case Sensor.TYPE_MAGNETIC_FIELD:
			// We don't actually care about this sensor, since we just need pitch and roll
			for (int i = 0; i < 3; i++)
				flRotEvent[i] += event.values[i];

			// exit if still collecting samples
			if (iRotCount++ < iFilterLength - 1)
				break;

			// reset the loop count, and calculate average
			for (int i = 0; i < 3; i++)
				flRotEvent[i] /= iFilterLength;

			geomag = flRotEvent.clone();
			iRotCount = 0;
			flRotEvent = new float[] {0,0,0};
			break;
		}

		if ( doCalc && gravity != null && geomag != null) {
			// try to find the rotation matrix
			boolean success = SensorManager.getRotationMatrix(inR, I, gravity, geomag);
			if (success) {
				SensorManager.getOrientation(inR, orientVals);
				updatePitchAndRollSliders((float)Math.toDegrees(orientVals[1]),(float)Math.toDegrees(orientVals[2]));
				if( !bCalibrating ) {
					flPitch = (float) Math.toDegrees(orientVals[1]);
					flRoll = (float)Math.toDegrees(orientVals[2]);
				}
			}
			if( savedGravity != null ){
				float[] flMag = new float[] {0,0};

				// Calculate the magnitude of the gravity and initial gravity vectors
				for( int i=0; i<3; i++ ) {
					flMag[0] += Math.pow(gravity[i],2);
					flMag[1] += Math.pow(savedGravity[i],2);
				}
				flMag[0] = (float) Math.sqrt(flMag[0]);
				flMag[1] = (float) Math.sqrt(flMag[1]);

				// Calculate the dot product of the two vectors
				float flDotProduct=0;
				for( int i=0; i<3; i++ ) 
					flDotProduct += gravity[i]*savedGravity[i];

				// Formula for the angle between two 3D vectors
				float flAngle;
				flAngle = (float) Math.toDegrees(Math.acos(flDotProduct/(flMag[0]*flMag[1])));

				if( bOrienting )
				{
					// Notify the user aurally, since the screen will be tipped away from view
					if(Math.abs(flAngle-90)<1) {
						// Reached 90 degrees--first make sure the phone is not accelerating
						if( Math.sqrt(Math.pow(gravity[0],2)+Math.pow(gravity[1],2)+Math.pow(gravity[2],2)) < SensorManager.GRAVITY_EARTH*1.02) {
							// we're done
							streamId = soundPool.play(soundId, 1.0f, 1.0f, 1, 3, 2.0f);
							bOrienting = false;
							m_handler.sendEmptyMessage(REVERT);
						}
					} else
						// Getting close--issue warning tone to slow down the rotation
						if(Math.abs(flAngle-90)<10) {							
							streamId = soundPool.play(soundId, 1.0f, 1.0f, 1, 0, 1.0f);
						}
						else
							soundPool.stop(streamId);
				}
			}
		}
	}

	public void updatePitchAndRollSliders(float pitch, float roll) {
		skRoll.setProgress((int)Math.round(roll * 100f / 180f + 50f));
		skPitch.setProgress((int)Math.round(pitch * 100f / 180f + 50f));
		lblPitch.setText("Pitch: " + String.valueOf(Math.round(pitch)));
		lblRoll.setText("Roll: " + String.valueOf(Math.round(roll)));
	}
	
	public void onProgressChanged(SeekBar arg0, int arg1, boolean arg2) {
		// This routine is used when the user moves the sliders manually
		int fProgress;
		fProgress = Math.round(arg1*180f/100f-90f);
		if (arg0.getId() == R.id.skPitch) {
			if( !bCalibrating && (rgCorrectionType.getCheckedRadioButtonId() == R.id.radio0))
				flPitch = fProgress;
			lblPitch.setText("Pitch: " + String.valueOf(fProgress));
		}
		if (arg0.getId() == R.id.skRoll) {
			if( !bCalibrating && (rgCorrectionType.getCheckedRadioButtonId() == R.id.radio0))
				flRoll = fProgress;
			lblRoll.setText("Roll: " + String.valueOf(fProgress));
		}
	}

	@Override
	public void onClick(View arg0) {
		if (arg0.getId() == R.id.chkUseAccel) {
			boolean isChecked = chkEnable.isChecked();
			btnCalibrateSensor.setEnabled(isChecked);
			chkEnableCorrection.setEnabled(isChecked);
			if( !isChecked ) {
				enableSensors(false);
				chkEnableCorrection.setEnabled(false);
				rgCorrectionType.setEnabled(false);
				for (int i = 0; i < rgCorrectionType.getChildCount(); i++)
					rgCorrectionType.getChildAt(i).setEnabled(false);
				skRoll.setEnabled(false);
				skPitch.setEnabled(false);
				lblPitch.setEnabled(false);
				lblRoll.setEnabled(false);		
				lblInstructions.setEnabled(false);
				spnFilter.setEnabled(false);
				btnConfigHelp.setEnabled(false);
			} else {
				processChkEnableCorrection();
				lblInstructions.setEnabled(true);
				spnFilter.setEnabled(true);
				btnConfigHelp.setEnabled(true);
			}
		}

		if (arg0.getId() == R.id.chkEnableCorrection) {
			processChkEnableCorrection();
		}

		if (arg0.getId() == R.id.btnCalibrateSensor ) {
			if( gravity != null && flSensorOffset==null ) {
				// The offsets are what would be added to the gravity vector to give a perfect (0,0,9.81)
				flSensorOffset = new float[] {-gravity[0],-gravity[1],SensorManager.GRAVITY_EARTH-gravity[2]};
				lblInstructions.setText(R.string.strResetCal);
				btnCalibrateSensor.setText("Reset");
				bCalibrating = false;
				enableSensors(false);
				updatePitchAndRollSliders(flPitch,flRoll);
			}
			else {
				// We had calibration data, user requested to reset it
				flSensorOffset = null;
				lblInstructions.setText(R.string.strReadyToCal);
				btnCalibrateSensor.setText("Calibrate");
				bCalibrating = true;
				enableSensors(true);
			}
		}
		
		if( arg0.getId() == R.id.btnConfigHelp ) {
			AlertDialog.Builder builder = new AlertDialog.Builder(this);
			builder.setTitle("Angle Correction Help")
			.setMessage(R.string.strConfigInstructions)
			.setPositiveButton("Dismiss", new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog, int id) {
					// do nothing
				}
			});
			AlertDialog dialog = builder.create();
			dialog.show();
		}
	}

	public void processChkEnableCorrection() {
		if (chkEnableCorrection.isChecked()) {
			skRoll.setEnabled(true);
			skPitch.setEnabled(true);
			lblPitch.setEnabled(true);
			lblRoll.setEnabled(true);
			btnConfigHelp.setEnabled(true);
			rgCorrectionType.setEnabled(true);
			for (int i = 0; i < rgCorrectionType.getChildCount(); i++)
				rgCorrectionType.getChildAt(i).setEnabled(true);
			
		} else {
			skRoll.setEnabled(false);
			skPitch.setEnabled(false);
			lblPitch.setEnabled(false);
			lblRoll.setEnabled(false);
			btnConfigHelp.setEnabled(false);
			rgCorrectionType.setEnabled(false);
			for (int i = 0; i < rgCorrectionType.getChildCount(); i++)
				rgCorrectionType.getChildAt(i).setEnabled(false);
			
			enableSensors(false);
		}
	}
	
	public void processCalibration() {
		//if( flSensorOffset[0]==0 && flSensorOffset[1]==0 && flSensorOffset[2]==0 ) { 
		if( flSensorOffset==null ) {
			lblInstructions.setText(R.string.strReadyToCal);
			btnCalibrateSensor.setText("Calibrate");

		} else {
			lblInstructions.setText(R.string.strResetCal);
			btnCalibrateSensor.setText("Reset");
		}
	}
	
	// Enables or disables accelerometer and magnetic
	public void enableSensors(boolean enable) 
	{
		if( enable ) {
			// set negative loop count, to discard first few samples
			iAccelCount = -3;
			lastSampleTime = -1;
			gravity=null;
			savedGravity=null;
			sensorMan.registerListener(this, accel,	SensorManager.SENSOR_DELAY_GAME);
			sensorMan.registerListener(this, mSensor, SensorManager.SENSOR_DELAY_NORMAL);
		}
		else {
			sensorMan.unregisterListener(this);
		}
	}
	
	@Override
	public void onCheckedChanged(RadioGroup group, int checkedId) {
		if (rgCorrectionType.getCheckedRadioButtonId() == R.id.radio1) {
			if( !bMaskRadio1 ) {
				bOrienting = true;
				enableSensors(true);
				// Start a timer, to revert to manual mode
				if( timer != null ) timer.cancel();
				timer = new Timer();
				timer.schedule(new revertAuto(), 10000);
			}		
		}
		else {
			bMaskRadio1 = false;
			timer.cancel();
			enableSensors(false);
		}
	}
	
	private class revertAuto extends TimerTask {
		@Override
		public void run() {
			streamId = soundPool.play(soundId, 1.0f, 1.0f, 1, 1, 0.5f);
			bOrienting = false;
			m_handler.sendEmptyMessage(REVERT);
		}
	}

	@Override
	public boolean handleMessage(Message msg)
	{
		switch( msg.what ) {
		case REVERT:
			// Select the 'manual' radio button when timer expires
			bMaskRadio1 = true;
			rgCorrectionType.check(R.id.radio0);
			break;
		}
		return false;
	}

	@Override
	public void onStartTrackingTouch(SeekBar seekBar) {
		// TODO Auto-generated method stub

	}

	@Override
	public void onStopTrackingTouch(SeekBar seekBar) {
		// TODO Auto-generated method stub

	}

	@Override
	public void onAccuracyChanged(Sensor arg0, int arg1) {
		// TODO Auto-generated method stub
	}

	@Override
	public void onItemSelected(AdapterView<?> arg0, View arg1, int arg2,
			long arg3) {
		if( arg0 == spnFilter)
			iFilterType = arg2;
	}

	@Override
	public void onNothingSelected(AdapterView<?> arg0) {
		// TODO Auto-generated method stub
		
	}

}

