package com.artsoft.wifilapper;

import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

public class ConfigurePointToPoint extends Activity implements OnItemSelectedListener
{
	Prefs.UNIT_SYSTEM eUnits;
	float flStartParamRaw; // in metric units
	float flStopParamRaw;
	
	@Override
	public void onCreate(Bundle extras)
	{
		super.onCreate(extras);
		setContentView(R.layout.configurep2p);
	}
	
	@Override
	public void onResume()
	{
		super.onResume();
		
		SharedPreferences settings = this.getSharedPreferences(Prefs.SHAREDPREF_NAME, 0);
		
		this.eUnits = Prefs.UNIT_SYSTEM.valueOf(settings.getString(Prefs.PREF_UNITS_STRING, Prefs.DEFAULT_UNITS_STRING.toString()));
		this.flStartParamRaw = settings.getFloat(Prefs.PREF_P2P_STARTPARAM, Prefs.DEFAULT_P2P_STARTPARAM);
		
		this.flStopParamRaw = settings.getFloat(Prefs.PREF_P2P_STOPPARAM, Prefs.DEFAULT_P2P_STOPPARAM);
		
		Spinner spnStartMode = (Spinner)findViewById(R.id.spnStartMode);
		Spinner spnStopMode = (Spinner)findViewById(R.id.spnStopMode);
		
		SetupStartMode(settings, spnStartMode);
		SetupStopMode(settings, spnStopMode);
	}
	
	private static class StartMode
	{
		StartMode(String str, int iMode)
		{
			this.str = str;
			this.iMode = iMode;
		}
		
		@Override
		public String toString()
		{
			return str;
		}
		
		String str;
		int iMode;
	}
	
	void SetupStartMode(SharedPreferences settings, Spinner spn)
	{
		List<StartMode> lstStartModes = new ArrayList<StartMode>();

		lstStartModes.add(new StartMode("Screen: Start when user taps", Prefs.P2P_STARTMODE_SCREEN));
		lstStartModes.add(new StartMode("Speed: Start when speed exceeds...", Prefs.P2P_STARTMODE_SPEED));

		ArrayAdapter<StartMode> adapter = new ArrayAdapter<StartMode>(this, R.layout.simplelistitem_blacktext, lstStartModes);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spn.setAdapter(adapter);
        
        int ixDefault = settings.getInt(Prefs.PREF_P2P_STARTMODE,Prefs.DEFAULT_P2P_STARTMODE);
        for(int x = 0;x < lstStartModes.size(); x++)
        {
        	if(lstStartModes.get(x).iMode == ixDefault)
        	{
            	spn.setSelection(x,true);
            	HandleStartMode(lstStartModes.get(x));
            	break;
        	}
        }
        
        spn.setOnItemSelectedListener(this);
	}
	void SetupStopMode(SharedPreferences settings, Spinner spn)
	{
		List<StartMode> lstStopModes = new ArrayList<StartMode>();

		lstStopModes.add(new StartMode("Screen: Stop when user taps.", Prefs.P2P_STOPMODE_SCREEN));
		lstStopModes.add(new StartMode("Speed: Stop when speed goes below...", Prefs.P2P_STOPMODE_SPEED));
		lstStopModes.add(new StartMode("Distance: Stop when the car has driven...", Prefs.P2P_STOPMODE_DISTANCE));

		ArrayAdapter<StartMode> adapter = new ArrayAdapter<StartMode>(this, R.layout.simplelistitem_blacktext, lstStopModes);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spn.setAdapter(adapter);
        
        int ixDefault = settings.getInt(Prefs.PREF_P2P_STOPMODE,Prefs.DEFAULT_P2P_STOPMODE);
        for(int x = 0;x < lstStopModes.size(); x++)
        {
        	if(lstStopModes.get(x).iMode == ixDefault)
        	{
            	spn.setSelection(x,true);
            	HandleStopMode(lstStopModes.get(x));
            	break;
        	}
        }
        
        spn.setOnItemSelectedListener(this);
	}
	
	@Override
	public void onPause()
	{
		super.onPause();

		SharedPreferences settings = this.getSharedPreferences(Prefs.SHAREDPREF_NAME, 0);
		
		Spinner spnStartMode = (Spinner)findViewById(R.id.spnStartMode);
		Spinner spnStopMode = (Spinner)findViewById(R.id.spnStopMode);
		EditText edtStartParam = (EditText)findViewById(R.id.edtStartParam);
		EditText edtStopParam = (EditText)findViewById(R.id.edtStopParam);
		
		SharedPreferences.Editor edit = settings.edit();
		try
		{ // save start mode data
			if(spnStartMode.getSelectedItemPosition() >= 0)
			{
				StartMode modeStart = (StartMode)spnStartMode.getItemAtPosition(spnStartMode.getSelectedItemPosition());
				float flParam = Float.parseFloat(edtStartParam.getText().toString());
				switch(modeStart.iMode)
				{
				case Prefs.P2P_STARTMODE_ACCEL: break; // no unit conversion required for acceleration
				case Prefs.P2P_STARTMODE_SCREEN: break; // no units involved for tapping-only setup
				case Prefs.P2P_STARTMODE_SPEED: flParam = Prefs.ConvertSpeedToMetric(flParam, eUnits); break;
				default: break;
				}
				edit.putInt(Prefs.PREF_P2P_STARTMODE, modeStart.iMode).putFloat(Prefs.PREF_P2P_STARTPARAM, flParam);
			}
		}
		catch(NumberFormatException e)
		{
			Toast.makeText(this, "Failed to parse your start parameter.  Is it a number?", Toast.LENGTH_LONG).show();
		}
		
		try
		{
			if(spnStopMode.getSelectedItemPosition() >= 0)
			{
				float flParam = Float.parseFloat(edtStopParam.getText().toString());
				StartMode modeStop = (StartMode)spnStopMode.getItemAtPosition(spnStopMode.getSelectedItemPosition());
				switch(modeStop.iMode)
				{
				case Prefs.P2P_STOPMODE_DISTANCE: flParam = Prefs.ConvertDistanceToMetric(flParam, eUnits); break; // no unit conversion required for acceleration
				case Prefs.P2P_STOPMODE_SCREEN: break; // no units involved for tapping-only setup
				case Prefs.P2P_STOPMODE_SPEED: flParam = Prefs.ConvertSpeedToMetric(flParam, eUnits); break;
				default: break;
				}
				
				edit.putInt(Prefs.PREF_P2P_STOPMODE, modeStop.iMode).putFloat(Prefs.PREF_P2P_STOPPARAM, flParam);
			}
		}
		catch(NumberFormatException e)
		{
			Toast.makeText(this, "Failed to parse your stop parameter.  Is it a number?", Toast.LENGTH_LONG).show();
		}
		edit.commit();
	}

	private void HandleStartMode(StartMode mode)
	{
		String strUnits = "";
		boolean fVisible = false;
		float flValue = 0;
		switch(mode.iMode)
		{
		case Prefs.P2P_STARTMODE_ACCEL:
			strUnits = "Gs";
			fVisible = true;
			flValue = flStartParamRaw;
			break;
		case Prefs.P2P_STARTMODE_SPEED:
			strUnits = Prefs.GetSpeedUnits(this.eUnits);
			fVisible = true;
			flValue = Prefs.ConvertMetricToSpeed(flStartParamRaw, eUnits);
			break;
		case Prefs.P2P_STARTMODE_SCREEN:
			fVisible = false;
			break;
		}
		
		TextView txtUnits = (TextView)findViewById(R.id.txtStartUnits);
		EditText edtParam = (EditText)findViewById(R.id.edtStartParam);
		
		txtUnits.setVisibility(fVisible ? View.VISIBLE : View.INVISIBLE);
		edtParam.setVisibility(fVisible ? View.VISIBLE : View.INVISIBLE);
		
		edtParam.setText("" + flValue);
		txtUnits.setText(strUnits);
	}
	
	private void HandleStopMode(StartMode mode)
	{
		String strUnits = "";
		boolean fVisible = false;
		float flValue = 0;
		switch(mode.iMode)
		{
		case Prefs.P2P_STOPMODE_SPEED:
			strUnits = Prefs.GetSpeedUnits(this.eUnits);
			fVisible = true;
			flValue = flStopParamRaw;
			break;
		case Prefs.P2P_STOPMODE_SCREEN:
			fVisible = false;
			break;
		case Prefs.P2P_STOPMODE_DISTANCE:
			fVisible = true;
			strUnits = Prefs.GetDistanceUnits(this.eUnits);
			flValue = Prefs.ConvertMetricToDistance(flStopParamRaw, eUnits);
			break;
		}
		
		TextView txtUnits = (TextView)findViewById(R.id.txtStopUnits);
		EditText edtParam = (EditText)findViewById(R.id.edtStopParam);
		
		txtUnits.setVisibility(fVisible ? View.VISIBLE : View.INVISIBLE);
		edtParam.setVisibility(fVisible ? View.VISIBLE : View.INVISIBLE);
		
		edtParam.setText("" + flValue);
		txtUnits.setText(strUnits);
	}
	
	@Override
	public void onItemSelected(AdapterView<?> arg0, View arg1, int arg2,long arg3) 
	{
		if(arg0.getId() == R.id.spnStartMode)
		{
			// they changed their start mode.
			StartMode mode = (StartMode)arg0.getItemAtPosition(arg2);
			HandleStartMode(mode);
		}
		if(arg0.getId() == R.id.spnStopMode)
		{
			// they changed their stop mode.
			StartMode mode = (StartMode)arg0.getItemAtPosition(arg2);
			HandleStopMode(mode);
		}
		arg0.invalidate(); // workaround for stupid bug where spinners don't repaint
	}

	@Override
	public void onNothingSelected(AdapterView<?> arg0) {
		// TODO Auto-generated method stub
		
	}
}
